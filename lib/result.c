#include "cpt_node.h"
#include "easy_pc_private.h"
#include "result.h"

#include <stdlib.h>
#include <string.h>

// --- Result management ---

EASY_PC_HIDDEN
void
epc_parser_error_free(epc_parser_error_t * error)
{
    if (error == NULL)
    {
        return;
    }
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
    if (message == NULL)
    {
        message = "";
    }
    size_t const message_len = strlen(message);
    if (expected == NULL)
    {
        expected = "";
    }
    size_t const expected_len = strlen(expected);
    if (found == NULL)
    {
        found = "";
    }
    size_t const found_len = strlen(found);

    epc_parser_error_t * error;
    size_t total_size = sizeof(*error) + message_len + 1 + expected_len + 1 + found_len + 1;

    error = calloc(1, total_size);
    if (error == NULL)
    {
        return error;
    }

    char const * input_start = parse_ctx_get_input_start(ctx);
    char const * current = input_start + input_offset;

    error->input_position = current;
    error->position = epc_calculate_line_and_column(ctx, input_offset);

    error->message = (char const *)(error + 1);
    memcpy((char *)error->message, message, message_len + 1);

    error->expected = error->message + message_len + 1;
    memcpy((char *)error->expected, expected, expected_len + 1);

    error->found = error->expected + expected_len + 1;
    memcpy((char *)error->found, found, found_len + 1);

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
