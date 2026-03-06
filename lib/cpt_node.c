#include "cpt_node.h"

#include <stdlib.h>

static epc_cpt_node_t *
node_alloc(void)
{
    epc_cpt_node_t * node = calloc(1, sizeof *node);

    return node;
}

EASY_PC_API
epc_cpt_node_t *
epc_node_alloc(epc_parser_ctx_t * ctx, epc_parser_t * parser, char const * tag)
{

    epc_cpt_node_t * node;

    if (true || ctx == NULL)
    {
        node = node_alloc();
    }
    // TODO: Alloc from an arena managed by the parse context if one was supplied.

    if (node == NULL)
    {
        return NULL;
    }
    node->content = ""; /* Make non-NULL. */
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
    /* TODO: Allocate from an arena managed by the parse context available in the node to be copied. */

    if (node == NULL)
    {
        return NULL;
    }

    epc_cpt_node_t * copy;

    if (true || node->ctx == NULL)
    {
        copy = node_alloc();
    }
    // TODO: obtain the copy from the parser context.

    if (copy == NULL)
    {
        return NULL;
    }

    copy->tag = node->tag;
    copy->name = node->name;
    copy->content = node->content;
    copy->len = node->len;
    copy->semantic_start_offset = node->semantic_start_offset;
    copy->semantic_end_offset = node->semantic_end_offset;
    copy->ast_config = node->ast_config;
    copy->ctx = node->ctx;

    if (node->children_count > 0)
    {
        copy->children = calloc(node->children_count, sizeof(*copy->children));
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
    /* TODO: return the node to a table of free nodes managed by the parse context in the parse context. */
    if (node == NULL)
    {
        return;
    }
    if (node->children != NULL)
    {
        for (int i = 0; i < node->children_count; i++)
        {
            epc_node_free(node->children[i]);
        }
        free(node->children);
    }
    if (true || node->ctx == NULL)
    {
        free(node);
    }
    /* TODO: Use the parser context to manage the freed node. */
}

EASY_PC_API const char *
epc_cpt_node_get_semantic_content(epc_cpt_node_t * node)
{
    if (node == NULL || node->content == NULL)
    {
        return NULL;
    }
    // Ensure start offset does not go beyond the actual content.
    // If it does, effectively, there is no semantic content.
    if (node->semantic_start_offset >= node->len)
    {
        return node->content + node->len; // Point to end of string or null
    }

    return node->content + node->semantic_start_offset;
}

EASY_PC_API size_t
epc_cpt_node_get_semantic_len(epc_cpt_node_t * node)
{
    if (node == NULL)
    {
        return 0;
    }
    // Calculate the total trimmed length.
    // Ensure start offset is not beyond actual length
    if (node->semantic_start_offset >= node->len)
    {
        return 0;
    }
    size_t effective_len = node->len - node->semantic_start_offset;

    // Ensure end offset is not beyond the remaining effective length
    if (node->semantic_end_offset >= effective_len)
    {
        return 0;
    }
    effective_len -= node->semantic_end_offset;

    return effective_len;
}

EASY_PC_API const char *
epc_cpt_node_get_content(epc_cpt_node_t * node)
{
    if (node == NULL || node->content == NULL)
    {
        return NULL;
    }

    return node->content;
}

EASY_PC_API size_t
epc_cpt_node_get_len(epc_cpt_node_t * node)
{
    if (node == NULL)
    {
        return 0;
    }

    return node->len;
}

EASY_PC_HIDDEN
char const *
epc_node_id(epc_cpt_node_t const * node)
{
    if (node == NULL)
    {
        return "NULL";
    }
    if (node->name)
    {
        return node->name;
    }

    return node->tag;
}
