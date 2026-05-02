#include "result.h"

#include "cpt_node.h"
#include "easy_pc_private.h"

#include <search.h>
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
    epc_parser_ctx_t * ctx = error->internal_parse_ctx;
    parse_ctx_free_error(ctx, error);
}

static size_t
calculate_next_newline_index(epc_parser_ctx_t * ctx, size_t const offset)
{

    size_t lo = 0;
    size_t hi = ctx->newline.count;
    while (lo < hi)
    {
        size_t mid = lo + (hi - lo) / 2;
        if (ctx->newline.positions[mid] < offset)
        {
            lo = mid + 1;
        }
        else
        {
            hi = mid;
        }
    }

    return lo;
}

EASY_PC_API
char *
epc_get_line_at_offset(epc_parser_ctx_t * ctx, size_t const offset)
{
    if (ctx == NULL)
    {
        return NULL;
    }

    size_t const next_newline_index = calculate_next_newline_index(ctx, offset);
    char * line;
    size_t start_offset;

    if (next_newline_index == 0)
    {
        start_offset = 0;
    }
    else
    {
        start_offset = ctx->newline.positions[next_newline_index - 1] + 1;
    }

    size_t len_to_copy;

    if (next_newline_index >= ctx->newline.count)
    {
        /* Take everything from the previous newline to the end. */
        len_to_copy = ctx->input_len - start_offset;
    }
    else
    {
        len_to_copy = ctx->newline.positions[next_newline_index] - start_offset;
    }

    line = strndup(&ctx->input_start[start_offset], len_to_copy);

    return line;
}

EASY_PC_API
epc_line_col_t
epc_calculate_line_and_column(epc_parser_ctx_t * ctx, size_t const offset)
{
    epc_line_col_t res = {0};

    if (ctx == NULL)
    {
        return res;
    }

    if (ctx->newline.positions == NULL || ctx->newline.count == 0)
    {
        res.line = 1;
        res.col = offset;
        return res;
    }

    size_t next_index = calculate_next_newline_index(ctx, offset);

    res.line = next_index + 1;
    if (next_index == 0)
    {
        res.col = offset + 1;
    }
    else
    {
        size_t prev_newline_offset = ctx->newline.positions[next_index - 1];

        res.col = offset - prev_newline_offset;
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
    if (expected == NULL)
    {
        expected = "";
    }
    if (found == NULL)
    {
        found = "";
    }

    epc_parser_error_t * error = parse_ctx_alloc_error(ctx);

    if (error == NULL)
    {
        return error;
    }

    char const * input_start = parse_ctx_get_input_start(ctx);
    char const * current = input_start + input_offset;

    error->input_position = current;
    error->position = epc_calculate_line_and_column(ctx, input_offset);

    strncpy(error->message, message, sizeof(error->message));
    error->message[sizeof(error->message) - 1] = '\0';

    strncpy(error->expected, expected, sizeof(error->expected));
    error->expected[sizeof(error->expected) - 1] = '\0';

    strncpy(error->found, found, sizeof(error->found));
    error->found[sizeof(error->found) - 1] = '\0';

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

    epc_parser_error_t * error = parse_ctx_alloc_error(ctx);
    if (error == NULL)
    {
        return error;
    }

    /* Errors contain no other allocated memory, so the contents can simply be copied over. */
    *error = *e;

    return error;
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
