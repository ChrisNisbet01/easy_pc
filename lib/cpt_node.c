#include "cpt_node.h"

#include <stdlib.h>
#include <string.h>

EASY_PC_API
epc_cpt_node_t *
epc_node_alloc(epc_parser_ctx_t * ctx, epc_parser_t * parser, char const * tag)
{

    epc_cpt_node_t * node = parse_ctx_alloc_node(ctx);

    if (node == NULL)
    {
        return NULL;
    }
    node->tag = tag;
    node->name = epc_parser_get_name(parser);
    node->ast_config = parser->ast_config;
    node->ctx = ctx;

    return node;
}

EASY_PC_HIDDEN
epc_cpt_node_t *
epc_node_copy(epc_cpt_node_t * node)
{
    if (node == NULL)
    {
        return NULL;
    }

    epc_cpt_node_t * copy = parse_ctx_alloc_node(node->ctx);

    if (copy == NULL)
    {
        return NULL;
    }

    copy->tag = node->tag;
    copy->name = node->name;
    copy->content_offset = node->content_offset;
    copy->token_count = node->token_count;
    copy->semantic_start_offset = node->semantic_start_offset;
    copy->semantic_end_offset = node->semantic_end_offset;
    copy->ast_config = node->ast_config;
    copy->ctx = node->ctx;

    if (node->children_count > 0)
    {
        copy->children = calloc((size_t)node->children_count, sizeof(*copy->children));
        if (copy->children == NULL)
        {
            epc_node_free(copy);
            return NULL;
        }

        for (int i = 0; i < node->children_count; ++i)
        {
            copy->children[i] = epc_node_copy(node->children[i]);
            if (copy->children[i] == NULL && node->children[i] != NULL)
            {
                /* Deep copy failed, cleanup and return NULL. */
                for (int j = 0; j < i; ++j)
                {
                    epc_node_free(copy->children[j]);
                }
                free(copy->children);
                epc_node_free(copy);
                return NULL;
            }
        }
        copy->children_count = node->children_count;
    }

    return copy;
}

EASY_PC_HIDDEN
void
epc_node_free(epc_cpt_node_t * node)
{
    if (node == NULL)
    {
        return;
    }
    free((char *)node->error_message);
    node->error_message = NULL;

    if (node->children != NULL)
    {
        for (int i = 0; i < node->children_count; i++)
        {
            epc_node_free(node->children[i]);
        }
        free(node->children);
        node->children = NULL;
    }

    parse_ctx_free_node(node->ctx, node);
}

EASY_PC_API const char *
epc_cpt_node_get_semantic_content(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return NULL;
    }
    char const * input_start = parse_ctx_get_input_start(node->ctx);

    if (input_start == NULL)
    {
        return NULL;
    }

    // Ensure start offset does not go beyond the actual content.
    // If it does, effectively, there is no semantic content.
    if (node->semantic_start_offset >= node->token_count)
    {
        return input_start + node->content_offset + node->token_count; // Point to end of string or null
    }

    return input_start + node->content_offset + node->semantic_start_offset;
}

EASY_PC_API size_t
epc_cpt_node_get_semantic_content_offset(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return 0;
    }

    // Ensure start offset does not go beyond the actual content.
    // If it does, effectively, there is no semantic content.
    if (node->semantic_start_offset >= node->token_count)
    {
        return node->content_offset + node->token_count;
    }

    return node->content_offset + node->semantic_start_offset;
}

EASY_PC_API size_t
epc_cpt_node_get_semantic_len(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return 0;
    }
    // Calculate the total trimmed length.
    // Ensure start offset is not beyond actual length
    if (node->semantic_start_offset >= node->token_count)
    {
        return 0;
    }
    size_t effective_len = node->token_count - node->semantic_start_offset;

    // Ensure end offset is not beyond the remaining effective length
    if (node->semantic_end_offset >= effective_len)
    {
        return 0;
    }
    effective_len -= node->semantic_end_offset;

    return effective_len;
}

EASY_PC_API const char *
epc_cpt_get_content_at_offset(epc_parser_ctx_t const * ctx, size_t offset)
{
    char const * input_start = parse_ctx_get_input_start(ctx);

    if (input_start == NULL)
    {
        return NULL;
    }
    if (offset > parse_ctx_get_input_len(ctx))
    {
        return NULL;
    }

    return input_start + offset;
}

EASY_PC_API const char *
epc_cpt_node_get_content(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return NULL;
    }

    epc_parser_token_t const * token = parse_ctx_get_token_at_offset(node->ctx, node->content_offset);

    if (token == NULL)
    {
        return NULL;
    }

    return epc_cpt_get_content_at_offset(node->ctx, token->view.offset);
}

EASY_PC_API size_t
epc_cpt_node_get_content_offset(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return 0;
    }

    return node->content_offset;
}

EASY_PC_API size_t
epc_cpt_node_get_len(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return 0;
    }

    return node->token_count;
}

EASY_PC_HIDDEN
char const *
epc_node_id(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return "NULL";
    }
    if (node->name != NULL)
    {
        return node->name;
    }

    return node->tag;
}

void
epc_cpt_node_assign_error_message(epc_cpt_node_t * node, char const * fmt, ...)
{
    if (node == NULL)
    {
        return;
    }

    free((char *)node->error_message);
    node->error_message = NULL;
    char * message = NULL;
    va_list args;
    va_start(args, fmt);
    if (vasprintf(&message, fmt, args) < 0 || message == NULL)
    {
        return;
    }
    va_end(args);

    node->error_message = message;
}
