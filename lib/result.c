#include "easy_pc_private.h"

#include <stdlib.h>
#include <string.h>

// --- Result/Node/Error management ---

EASY_PC_API
epc_cpt_node_t *
epc_node_alloc(epc_parser_t * parser, char const * tag)
{
    epc_cpt_node_t * node = calloc(1, sizeof(*node));
    if (node == NULL)
    {
        return NULL;
    }
    node->content = ""; /* Make non-NULL. */
    node->tag = tag;
    node->name = parser->name;
    node->ast_config = parser->ast_config;

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

    epc_cpt_node_t * copy = calloc(1, sizeof(*copy));
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

    if (node->children_count > 0)
    {
        copy->children = calloc(node->children_count, sizeof(*copy->children));
        if (copy->children == NULL)
        {
            free(copy);
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
                free(copy);
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
    if (node->children != NULL)
    {
        for (int i = 0; i < node->children_count; i++)
        {
            epc_node_free(node->children[i]);
        }
        free(node->children);
    }
    free(node);
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

static void
pt_visit_recursive(epc_cpt_node_t * node, epc_cpt_visitor_t * visitor)
{
    if (!node || !visitor)
    {
        return;
    }

    if (visitor->enter_node)
    {
        visitor->enter_node(node, visitor->user_data);
    }
    for (int i = 0; i < node->children_count; ++i)
    {
        pt_visit_recursive(node->children[i], visitor);
    }

    if (visitor->exit_node)
    {
        visitor->exit_node(node, visitor->user_data);
    }
}

EASY_PC_API void
epc_cpt_visit_nodes(epc_cpt_node_t * root, epc_cpt_visitor_t * visitor)
{
    if (!root || !visitor)
    {
        return;
    }
    pt_visit_recursive(root, visitor);
}

EASY_PC_HIDDEN
void
epc_parser_error_free(epc_parser_error_t * error)
{
    if (error == NULL)
    {
        return;
    }
    free((char *)error->message);
    free((char *)error->expected);
    free((char *)error->found);
    free(error);
}

EASY_PC_HIDDEN
epc_line_col_t
epc_calculate_line_and_column(epc_parser_ctx_t * ctx, size_t const offset)
{
    epc_line_col_t res = {0};
    char const * const input_start = parse_ctx_get_input_start(ctx);
    size_t const input_len = parse_ctx_get_input_len(ctx);

    if (input_start == NULL || offset >= input_len)
    {
        return res;
    }

    char const * current = input_start + offset;
    if (current > input_start + input_len)
    {
        return res;
    }

    {
        char const * line_start = input_start;

        for (char const * nl = strchr(input_start, '\n'); nl != NULL && nl <= current; nl = strchr(nl + 1, '\n'))
        {
            res.line++;
            line_start = nl;
        }
        res.col = current - line_start;
    }

    return res;
}

epc_parser_error_t *
epc_parser_error_alloc(
    epc_parser_ctx_t * ctx, size_t input_offset, char const * message, char const * expected, char const * found
)
{
    epc_parser_error_t * error = calloc(1, sizeof(*error));
    if (error == NULL)
    {
        return error;
    }

    char const * input_start = parse_ctx_get_input_start(ctx);
    char const * current = input_start + input_offset;

    error->input_position = current;
    error->position = epc_calculate_line_and_column(ctx, input_offset);

    error->message = strdup(message != NULL ? message : "");
    error->expected = strdup(expected != NULL ? expected : "");
    error->found = strdup(found != NULL ? found : "");

    return error;
}

void
epc_parser_result_cleanup(epc_parse_result_t * result)
{
    if (result->is_error)
    {
        epc_parser_error_free(result->data.error);
    }
    else
    {
        epc_node_free(result->data.success);
    }
    memset(result, 0, sizeof(*result));
}

epc_parse_result_t
epc_unparsed_error_result(size_t input_offset, char const * message, char const * expected, char const * found)
{
    epc_parse_result_t result = {
        .is_error = true,
        .data.error = epc_parser_error_alloc(NULL, input_offset, message, expected, found),
    };
    return result;
}

EASY_PC_HIDDEN
void
parser_furthest_error_restore(epc_parser_ctx_t * ctx, epc_parser_error_t ** replacement)
{
    parser_ctx_set_furthest_error(ctx, replacement);
}

EASY_PC_HIDDEN
epc_parser_error_t *
epc_parser_error_copy(epc_parser_ctx_t * ctx, epc_parser_error_t * e)
{
    if (e == NULL)
    {
        return NULL;
    }
    return epc_parser_error_alloc(
        ctx, parse_ctx_get_offset_from_input(ctx, e->input_position), e->message, e->expected, e->found
    );
}

EASY_PC_HIDDEN
epc_parse_result_t
epc_parse_result_copy(epc_parser_ctx_t * ctx, epc_parse_result_t result)
{
    epc_parse_result_t copy = {.is_error = result.is_error};
    if (result.is_error)
    {
        copy.data.error = epc_parser_error_copy(ctx, result.data.error);
    }
    else
    {
        copy.data.success = epc_node_copy(result.data.success);
    }
    return copy;
}

EASY_PC_HIDDEN
void
update_furthest_error(epc_parser_ctx_t * ctx, epc_parser_error_t * new_error)
{
    if (ctx == NULL || new_error == NULL)
    {
        return;
    }

    epc_parser_error_t const * furthest_error = parse_ctx_get_furthest_error(ctx);

    if (furthest_error == NULL || (new_error->input_position >= furthest_error->input_position))
    {
        epc_parser_error_t * e_copy = epc_parser_error_copy(ctx, new_error);
        parser_furthest_error_restore(ctx, &e_copy);
    }
}

EASY_PC_HIDDEN
epc_parse_result_t
epc_parser_error_result(
    epc_parser_ctx_t * ctx, size_t input_offset, char const * message, char const * expected, char const * found
)
{
    epc_parse_result_t result = {
        .is_error = true,
        .data.error = epc_parser_error_alloc(ctx, input_offset, message, expected, found),
    };
    update_furthest_error(ctx, result.data.error);
    return result;
}

EASY_PC_HIDDEN
epc_parse_result_t
epc_parser_success_result(epc_cpt_node_t * success_node)
{
    epc_parse_result_t result = {
        .data.success = success_node,
    };

    return result;
}

EASY_PC_HIDDEN
epc_parser_error_t *
parser_furthest_error_copy(epc_parser_ctx_t * ctx)
{
    return epc_parser_error_copy(ctx, parse_ctx_get_furthest_error(ctx));
}

EASY_PC_HIDDEN
char const *
epc_parser_get_name(epc_parser_t const * p)
{
    if (p == NULL)
    {
        return "NULL_PARSER";
    }
    else if (p->name != NULL)
    {
        return p->name;
    }
    else if (p->tag != NULL)
    {
        return p->tag;
    }
    else
    {
        return "Unnamed parser";
    }
}
