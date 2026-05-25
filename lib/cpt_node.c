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
    copy->token.offset = node->token.offset;
    copy->token.count = node->token.count;
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
epc_cpt_get_content_at_offset(epc_parser_ctx_t const * ctx, size_t input_offset)
{
    char const * input_start = parse_ctx_get_input_start(ctx);

    if (input_start == NULL)
    {
        return NULL;
    }
    if (input_offset > parse_ctx_get_input_len(ctx))
    {
        return NULL;
    }

    return input_start + input_offset;
}

EASY_PC_API const char *
epc_cpt_node_get_semantic_content(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return NULL;
    }

    size_t token_offset;
    // Ensure start offset does not go beyond the actual content.
    // If it does, effectively, there is no semantic content.
    if (node->semantic_start_offset >= node->token.count)
    {
        token_offset = node->token.offset + node->token.count; // Point to end of string or null
    }
    else
    {
        token_offset = node->token.offset + node->semantic_start_offset;
    }

    epc_parser_token_t const * token = parse_ctx_get_token_at_offset(node->ctx, token_offset);

    if (token == NULL)
    {
        /* Shouldn't happen unless there is some kind of data/logic error. */
        return NULL;
    }

    return epc_cpt_get_content_at_offset(node->ctx, token->view.offset);
}

EASY_PC_API epc_parser_input_view_t
epc_cpt_node_get_input_semantic_view(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return (epc_parser_input_view_t){0};
    }

    size_t first_token_offset;

    // Ensure start offset does not go beyond the actual content.
    // If it does, effectively, there is no semantic content.
    if (node->semantic_start_offset >= node->token.count)
    {
        first_token_offset = node->token.offset + node->token.count;
    }
    else
    {
        first_token_offset = node->token.offset + node->semantic_start_offset;
    }

    size_t last_token_offset;

    if (node->semantic_end_offset >= node->token.count)
    {
        last_token_offset = first_token_offset;
    }
    else
    {
        last_token_offset = node->token.offset + node->token.count - node->semantic_end_offset - 1;
    }

    size_t view_len = 0;
    if (node->token.count > 0)
    {
        for (size_t i = first_token_offset; i <= last_token_offset; i++)
        {
            epc_parser_token_t const * token = parse_ctx_get_token_at_offset(node->ctx, i);

            if (token == NULL) /* Shouldn't happen. */
            {
                continue;
            }
            view_len += token->view.len;
        }
    }

    epc_parser_token_t const * first_token = parse_ctx_get_token_at_offset(node->ctx, first_token_offset);

    if (first_token == NULL)
    {
        /* Shouldn't happen unless there is some kind of data/logic error. */
        return (epc_parser_input_view_t){0};
    }

    epc_parser_input_view_t view = first_token->view;
    view.len = view_len;

    return view;
}

EASY_PC_API size_t
epc_cpt_node_get_semantic_content_offset(epc_cpt_node_t const * node)
{
    return epc_cpt_node_get_input_semantic_view(node).offset;
}

EASY_PC_API size_t
epc_cpt_node_get_semantic_len(epc_cpt_node_t const * node)
{
    return epc_cpt_node_get_input_semantic_view(node).len;
}

EASY_PC_API epc_parser_input_view_t
epc_cpt_node_get_input_view(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return (epc_parser_input_view_t){0};
    }

    size_t view_len = 0;
    if (node->token.count > 0)
    {
        for (size_t i = node->token.offset; i <= node->token.offset + node->token.count - 1; i++)
        {
            epc_parser_token_t const * token = parse_ctx_get_token_at_offset(node->ctx, i);

            if (token == NULL) /* Shouldn't happen. */
            {
                continue;
            }
            view_len += token->view.len;
        }
    }

    epc_parser_token_t const * first_token = parse_ctx_get_token_at_offset(node->ctx, node->token.offset);

    if (first_token == NULL)
    {
        /* Shouldn't happen unless there is some kind of data/logic error. */
        return (epc_parser_input_view_t){0};
    }

    epc_parser_input_view_t view = first_token->view;
    view.len = view_len;

    return view;
}

EASY_PC_API const char *
epc_cpt_node_get_content(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return NULL;
    }

    epc_parser_token_t const * token = parse_ctx_get_token_at_offset(node->ctx, node->token.offset);

    if (token == NULL)
    {
        return NULL;
    }

    return epc_cpt_get_content_at_offset(node->ctx, token->view.offset);
}

EASY_PC_API size_t
epc_cpt_node_get_content_offset(epc_cpt_node_t const * node)
{
    return epc_cpt_node_get_input_view(node).offset;
}

EASY_PC_API size_t
epc_cpt_node_get_content_len(epc_cpt_node_t const * node)
{
    return epc_cpt_node_get_input_view(node).len;
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
