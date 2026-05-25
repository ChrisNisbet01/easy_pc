#include "parsers.h"

#include "child_list.h"
#include "cpt_node.h"
#include "easy_pc_private.h"
#include "result.h"

#include <ctype.h> // For isdigit
#include <errno.h>
#include <stdarg.h> // For va_list, va_start, va_arg, va_end
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FOUND_BUFFER_SIZE 21
#define ERROR_MSG_BUFFER_SIZE 64

static char const memory_allocation_error[] = "Memory allocation error";
static char const infinite_recursion_detected_msg[] = "Infinite recursion detected";

// --- Internal Helper Functions ---
typedef enum
{
    MATCH_OK,
    MATCH_EOF,
    MATCH_MISMATCH
} match_status_t;

static size_t
get_semantic_end_offset(epc_cpt_node_t ** children, size_t const count)
{
    size_t semantic_end_offset = 0;

    /*
     * Work through the children in reverse order to find the first one that has a len > 0, then take the semantic
     * offset of that child.
     */
    if (children != NULL)
    {
        for (size_t i = count; i > 0; i--)
        {
            epc_cpt_node_t const * child = children[i - 1];
            if (child->token.count > 0)
            {
                semantic_end_offset = child->semantic_end_offset;
                break;
            }
        }
    }

    return semantic_end_offset;
}

static size_t
get_semantic_start_offset(epc_cpt_node_t ** children, size_t const count)
{
    size_t semantic_start_offset = 0;

    /*
     * Work through the children in order to find the first one that has a len > 0, then take the semantic
     * offset of that child.
     */
    if (children != NULL)
    {
        for (size_t i = 0; i < count; i++)
        {
            epc_cpt_node_t const * child = children[i];
            if (child->token.count > 0)
            {
                semantic_start_offset = child->semantic_start_offset;
                break;
            }
        }
    }

    return semantic_start_offset;
}

static match_status_t
try_match_token(epc_parser_ctx_t * ctx, size_t offset, char expected, epc_parser_token_t * found_out)
{
    parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, offset);
    if (res.is_eof)
    {
        return MATCH_EOF;
    }
    *found_out = res.token;

    return (res.token.id == (unsigned)expected) ? MATCH_OK : MATCH_MISMATCH;
}

static size_t
consume_until_newline(epc_parser_ctx_t * ctx, size_t token_offset)
{
    size_t token_count = 0;

    while (1)
    {
        parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + token_count);
        if (res.is_eof)
        {
            return token_count;
        }
        token_count++;
        if (res.token.id == '\n')
        {
            break;
        }
    }

    return token_count;
}

static size_t
consume_c_comment_content(epc_parser_ctx_t * ctx, size_t token_offset)
{
    size_t token_count = 0;
    bool prev_was_star = false;

    while (1)
    {
        parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + token_count);
        if (res.is_eof)
        {
            return token_count;
        }
        token_count++;

        epc_token_id_t const c = res.token.id;

        if (prev_was_star && c == '/')
        {
            break;
        }
        prev_was_star = (c == '*');
    }

    return token_count;
}

// --- Parser List free. ---
static void
parser_list_free(parser_list_t * list)
{
    if (list == NULL)
    {
        return;
    }
    free(list->parsers);
    free(list);
}

// --- Parser List Creation ---
static parser_list_t *
parser_list_create_v(int count, va_list parsers)
{
    if (count <= 0)
    {
        return NULL;
    }

    parser_list_t * list = calloc(1, sizeof(*list));
    if (list == NULL)
    {
        return NULL;
    }

    list->parsers = calloc(count, sizeof(*list->parsers));
    if (list->parsers == NULL)
    {
        free(list);
        return NULL;
    }

    for (int i = 0; i < count; ++i)
    {
        list->parsers[i] = va_arg(parsers, epc_parser_t *);
    }
    list->count = count;

    return list;
}

static void
string_set(char const ** const dst, char const * src)
{
    free((char *)*dst);
    if (src == NULL)
    {
        *dst = NULL;
    }
    else
    {
        *dst = strdup(src);
    }
}

static void
parser_data_free(parser_data_type_st * data)
{
    switch (data->type)
    {
    case PARSER_DATA_TYPE_NONE:
    case PARSER_DATA_TYPE_PARSER:
    case PARSER_DATA_TYPE_CHAR_RANGE:
    case PARSER_DATA_TYPE_COUNT:
    case PARSER_DATA_TYPE_BETWEEN:
    case PARSER_DATA_TYPE_DELIMITED:
    case PARSER_DATA_TYPE_LEXEME:
    case PARSER_DATA_TYPE_PREDICATE:
    case PARSER_DATA_TYPE_WRAP:
    case PARSER_DATA_TYPE_MEMOIZE:
        /* Nothing to do. */
        break;

    case PARSER_DATA_TYPE_STRING:
        free((char *)data->string);
        data->string = NULL;
        break;

    case PARSER_DATA_TYPE_PARSER_LIST:
        parser_list_free(data->parser_list);
        data->parser_list = NULL;
        break;

    case PARSER_DATA_TYPE_BYTE:
        free((char *)data->byte.str);
        data->byte.str = NULL;
        break;
    }

    data->type = PARSER_DATA_TYPE_NONE;
}

void
epc_parser_free(epc_parser_t * parser)
{
    if (parser == NULL)
    {
        return;
    }
    parser_data_free(&parser->data);
    string_set(&parser->name, NULL);
    free(parser);
}

ATTR_NONNULL(2)
static epc_parser_t *
epc_parser_allocate(char const * name, char const * tag, parse_fn_t parse_fn)
{
    epc_parser_t * p = calloc(1, sizeof(*p));

    if (p == NULL)
    {
        return NULL;
    }
    string_set(&p->name, name);
    p->tag = tag;
    p->parse_fn = parse_fn;

    return p;
}

static epc_parser_t *
_epc_parser_fwd_decl(char const * name)
{
    return epc_parser_allocate(name, "forward_decl", NULL);
}

EASY_PC_API epc_parser_t *
epc_parser_fwd_decl(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_parser_fwd_decl(name));
}

static char const *
parser_get_expected_str(epc_parser_t const * p)
{
    if (p == NULL)
    {
        /* Shouldn't happen. */
        return "NULL_PARSER";
    }

    if (p->expected_value != NULL)
    {
        return p->expected_value;
    }

    return epc_parser_get_name(p);
}

#define WITH_PARSE_DEBUG 0

// Parser helper function
static epc_parse_result_t
parse(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
#if WITH_PARSE_DEBUG
    epc_parser_token_t const * token = parse_ctx_get_token_at_offset(ctx, token_offset);
    if (token != NULL)
    {
        char const * input = epc_cpt_get_content_at_offset(ctx, token->view.offset);

        if (input != NULL)
        {
            fprintf(
                stderr,
                "%*sparsing: name: %s, tag %s. input '%.*s', offset: %zu\n",
                ctx->depth * 2,
                "",
                epc_parser_get_name(self),
                self->tag,
                25,
                input,
                token_offset
            );
        }
    }
#endif

    ctx->depth++;
    epc_parse_result_t result = self->parse_fn(self, ctx, token_offset);

    ctx->depth--;

#if WITH_PARSE_DEBUG
    if (result.is_error)
    {
        fprintf(
            stderr,
            "%*sfailed to parse: name: %s (expected: %s)\n",
            ctx->depth * 2,
            "",
            epc_parser_get_name(self),
            parser_get_expected_str(self)
        );
    }
    else
    {
        epc_parser_token_t const * token = parse_ctx_get_token_at_offset(ctx, token_offset);

        if (token != NULL)
        {
            size_t content_len = epc_cpt_node_get_content_len(result.data.success);

            fprintf(
                stderr,
                "%*smatched: %s `%.*s` len: %zu\n",
                ctx->depth * 2,
                "",
                epc_parser_get_name(self),
                (int)content_len,
                epc_cpt_get_content_at_offset(ctx, token->view.offset),
                content_len
            );
        }
    }
#endif

    return result;
}

static epc_parse_result_t
pchar_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char const * expected_str = parser_get_expected_str(self);
    char expected_char = self->data.string[0];
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", expected_str, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (input != (epc_token_id_t)expected_char)
    {
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected character", expected_str, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_char(char const * name, char c)
{
    epc_parser_t * p = epc_parser_allocate(name, "char", pchar_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    char buf[2] = {c, '\0'};
    char * data = strdup(buf);
    if (data == NULL)
    {
        free(p);
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_STRING;
    p->data.string = data;
    p->expected_value = p->data.string;

    return p;
}

EASY_PC_API epc_parser_t *
epc_char(epc_parser_list * list, char const * name, char c)
{
    return epc_parser_list_add(list, _epc_char(name, c));
}

static epc_parse_result_t
pbyte_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char const * expected_str = parser_get_expected_str(self);
    uint8_t expected_byte = self->data.byte.byte;
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", expected_str, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (input != expected_byte)
    {
        // else Mismatch
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected byte", expected_str, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_byte(char const * name, char b)
{
    epc_parser_t * p = epc_parser_allocate(name, "byte", pbyte_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    char * data;
    int len = asprintf(&data, "0x%02x", (unsigned char)b);
    if (len < 0 || data == NULL)
    {
        free(p);
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_BYTE;
    p->data.byte.str = data;
    p->data.byte.byte = b;
    p->expected_value = p->data.byte.str;

    return p;
}

EASY_PC_API epc_parser_t *
epc_byte(epc_parser_list * list, char const * name, char b)
{
    return epc_parser_list_add(list, _epc_byte(name, b));
}

static epc_parse_result_t
pstring_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char const * expected_str = parser_get_expected_str(self);
    char const * match_string = self->data.string;
    size_t expected_len = strlen(match_string);

    for (size_t matched_tokens = 0; matched_tokens < expected_len; matched_tokens++)
    {
        parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset + matched_tokens);
        epc_token_id_t const input = input_result.token.id;

        if (input_result.is_eof)
        {
            char const * found_str = "EOF";
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", expected_str, found_str);
        }

        if (input != (epc_token_id_t)expected_str[matched_tokens])
        {
            /* Match not found. */
            char found_buffer[FOUND_BUFFER_SIZE];

            if (isascii(input))
            {
                snprintf(found_buffer, sizeof(found_buffer), "%c (pos: %zu)", (char)input, matched_tokens);
            }
            else
            {
                snprintf(found_buffer, sizeof(found_buffer), "%u (pos: %zu)", input, matched_tokens);
            }

            return epc_parser_error_result(ctx, token_offset, "Unexpected string", expected_str, found_buffer);
        }
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = expected_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_string(char const * name, char const * s)
{
    epc_parser_t * p = epc_parser_allocate(name, "string", pstring_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    char * data = strdup(s);
    if (data == NULL)
    {
        free(p);
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_STRING;
    p->data.string = data;
    p->expected_value = p->data.string;

    return p;
}

EASY_PC_API epc_parser_t *
epc_string(epc_parser_list * list, char const * name, char const * s)
{
    return epc_parser_list_add(list, _epc_string(name, s));
}

static epc_parse_result_t
psoi_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    if (token_offset != 0)
    {
        return epc_parser_error_result(
            ctx, token_offset, "Start of input not found", "<start of input>", "<post input>"
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 0;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_soi(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "soi", psoi_parse_fn);

    return p;
}

EASY_PC_API epc_parser_t *
epc_soi(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_soi(name));
}

static epc_parse_result_t
peoi_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (!input_result.is_eof)
    {
        /* Still some input left. */
        char buf[FOUND_BUFFER_SIZE];
        if (isascii(input_result.token.id))
        {
            buf[0] = (char)input_result.token.id;
            buf[1] = '\0';
        }
        else
        {
            snprintf(buf, sizeof(buf), "Token ID: %u", input_result.token.id);
            buf[sizeof(buf) - 1] = '\0';
        }

        return epc_parser_error_result(ctx, token_offset, "End of input not found", "<end of input>", buf);
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 0;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_eoi(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "eoi", peoi_parse_fn);

    return p;
}

EASY_PC_API epc_parser_t *
epc_eoi(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_eoi(name));
}

static epc_parse_result_t
pdigit_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (!isdigit(input))
    {
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected token", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_digit(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "digit", pdigit_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_digit(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_digit(name));
}

static bool
is_double_prefix(char const * s, size_t len)
{
    if (len == 0)
    {
        return false;
    }
    for (size_t i = 0; i < len; i++)
    {
        if (strchr(".eE+-xXpP", s[i]) == NULL)
        {
            return false;
        }
    }
    return true;
}

static epc_parse_result_t
pint_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    size_t parsed_len = 0;

    {
        char * temp_buffer = ctx->temp_parse_buffer;
        size_t current_len = 0;

        while (current_len < TEMP_PARSE_BUFFER_SIZE)
        {
            parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + current_len);
            if (res.is_eof)
            {
                break;
            }

            epc_token_id_t const input = res.token.id;

            if (!isascii(input))
            {
                break;
            }

            temp_buffer[current_len++] = input;
            temp_buffer[current_len] = '\0';

            char * endptr;

            (void)strtoll(temp_buffer, &endptr, 10);

            parsed_len = (size_t)(endptr - temp_buffer);

            if (parsed_len < current_len)
            {
                // If it parsed 0 but the first char is a minus sign, we wait for more digits.
                if (parsed_len > 0 || temp_buffer[0] != '-')
                {
                    break;
                }
                // Continue loop to wait for more data or EOF
            }
        }
        if (current_len == TEMP_PARSE_BUFFER_SIZE)
        {
            return epc_parser_error_result(ctx, token_offset, "Integer literal too long", "integer", "<too long>");
        }
    }

    if (parsed_len == 0)
    {
        parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);
        if (input_result.is_eof)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
        }

        /* No match to an integer. */
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Expected an integer", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = parsed_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_int(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "integer", pint_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_int(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_int(name));
}

static epc_parse_result_t
pspace_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(
            ctx, token_offset, "Unexpected end of input", parser_get_expected_str(self), "EOF"
        );
    }

    epc_token_id_t const input = input_result.token.id;

    if (!isspace(input))
    {
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected character", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;
    node->semantic_start_offset = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_space(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "whitespace", pspace_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_space(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_space(name));
}

static epc_parse_result_t
palpha_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (!isalpha(input))
    {
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected character", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_alpha(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "alpha", palpha_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_alpha(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_alpha(name));
}

static epc_parse_result_t
palphanum_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (!isalnum(input))
    {
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected character", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_alphanum(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "alphanum", palphanum_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_alphanum(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_alphanum(name));
}

static epc_parse_result_t
pdouble_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    size_t parsed_len = 0;

    {
        char * double_buf = ctx->temp_parse_buffer;
        size_t current_len = 0;

        while (current_len < TEMP_PARSE_BUFFER_SIZE)
        {
            parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + current_len);
            if (res.is_eof)
            {
                break;
            }

            if (!isascii(res.token.id))
            {
                break;
            }

            double_buf[current_len++] = res.token.id;
            double_buf[current_len] = '\0';

            char * endptr;

            errno = 0;
            (void)strtod(double_buf, &endptr);

            if (errno == ERANGE)
            {
                char found_str[FOUND_BUFFER_SIZE];
                snprintf(found_str, sizeof(found_str), "%.*s", (int)sizeof(found_str) - 1, double_buf);
                return epc_parser_error_result(ctx, token_offset, "Double out of range", self->tag, found_str);
            }

            parsed_len = (size_t)(endptr - double_buf);

            if (parsed_len < current_len)
            {
                // Check if the suffix is a potential numeric prefix
                if (!is_double_prefix(double_buf + parsed_len, current_len - parsed_len))
                {
                    break;
                }
                // Continue loop and wait for more or EOF
            }
        }
        if (current_len == TEMP_PARSE_BUFFER_SIZE)
        {
            char found_str[FOUND_BUFFER_SIZE];
            snprintf(found_str, sizeof(found_str), "%.*s", (int)sizeof(found_str) - 1, double_buf);
            return epc_parser_error_result(ctx, token_offset, "Double is too large", self->tag, found_str);
        }
    }

    if (parsed_len == 0)
    {
        parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

        if (input_result.is_eof)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
        }
        if (isascii(input_result.token.id))
        {
            char found_str[FOUND_BUFFER_SIZE];
            snprintf(found_str, sizeof(found_str), "%c", input_result.token.id);
            found_str[sizeof(found_str) - 1] = '\0';

            return epc_parser_error_result(ctx, token_offset, "Expected a double", self->tag, found_str);
        }
        else
        {
            return epc_parser_error_result_token_list(
                ctx, token_offset, "Expected a double", self->tag, &input_result.token, 1
            );
        }
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = parsed_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_double(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "double", pdouble_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_double(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_double(name));
}

static epc_parse_result_t
plong_double_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    size_t parsed_len = 0;

    {
        char * double_buf = ctx->temp_parse_buffer;
        size_t current_len = 0;

        while (current_len < TEMP_PARSE_BUFFER_SIZE)
        {
            parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + current_len);
            if (res.is_eof)
            {
                break;
            }

            if (!isascii(res.token.id))
            {
                break;
            }

            double_buf[current_len++] = res.token.id;
            double_buf[current_len] = '\0';

            char * endptr;

            errno = 0;
            (void)strtold(double_buf, &endptr);

            if (errno == ERANGE)
            {
                char found_str[FOUND_BUFFER_SIZE];
                snprintf(found_str, sizeof(found_str), "%.*s", (int)sizeof(found_str) - 1, double_buf);
                return epc_parser_error_result(ctx, token_offset, "Long double out of range", self->tag, found_str);
            }

            parsed_len = (size_t)(endptr - double_buf);

            if (parsed_len < current_len)
            {
                // Check if the suffix is a potential numeric prefix
                if (!is_double_prefix(double_buf + parsed_len, current_len - parsed_len))
                {
                    break;
                }
                // Continue loop and wait for more or EOF
            }
        }
        if (current_len == TEMP_PARSE_BUFFER_SIZE)
        {
            char found_str[FOUND_BUFFER_SIZE];
            snprintf(found_str, sizeof(found_str), "%.*s", (int)sizeof(found_str) - 1, double_buf);
            return epc_parser_error_result(ctx, token_offset, "Long double is too large", self->tag, found_str);
        }
    }

    if (parsed_len == 0)
    {
        parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

        if (input_result.is_eof)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
        }
        if (isascii(input_result.token.id))
        {
            char found_str[FOUND_BUFFER_SIZE];
            snprintf(found_str, sizeof(found_str), "%c", input_result.token.id);
            found_str[sizeof(found_str) - 1] = '\0';

            return epc_parser_error_result(ctx, token_offset, "Expected a long double", self->tag, found_str);
        }

        return epc_parser_error_result_token_list(
            ctx, token_offset, "Expected a long double", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = parsed_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_long_double(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "long_double", plong_double_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_long_double(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_long_double(name));
}

static epc_parse_result_t
phex_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    {
        parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

        if (input_result.is_eof)
        {
            return epc_parser_error_result(
                ctx, token_offset, "Unexpected end of input", parser_get_expected_str(self), "EOF"
            );
        }

        epc_token_id_t const input = input_result.token.id;

        // Must start with '0'
        if (input != '0')
        {
            if (isascii(input))
            {
                char found_str[2] = {input, '\0'};
                return epc_parser_error_result(
                    ctx, token_offset, "Expected hex literal", parser_get_expected_str(self), found_str
                );
            }
            return epc_parser_error_result_token_list(
                ctx, token_offset, "Expected hex literal", self->tag, &input_result.token, 1
            );
        }
    }
    {
        parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset + 1);

        if (input_result.is_eof)
        {
            return epc_parser_error_result(
                ctx, token_offset, "Unexpected end of input", parser_get_expected_str(self), "EOF"
            );
        }

        epc_token_id_t const input = input_result.token.id;

        // Next char must be either 'x' or 'X'
        if (input != 'x' && input != 'X')
        {
            if (isascii(input))
            {
                char found_str[3] = {'0', input, '\0'};
                return epc_parser_error_result(
                    ctx, token_offset, "Expected hex literal", parser_get_expected_str(self), found_str
                );
            }
            return epc_parser_error_result_token_list(
                ctx, token_offset, "Expected hex literal", self->tag, &input_result.token, 1
            );
        }
    }

    // Must have at least one hex digit after the prefix
    parse_get_input_result_t digit_check = parse_ctx_get_input_at_offset(ctx, token_offset + 2);
    if (digit_check.is_eof || !isxdigit(digit_check.token.id))
    {
        if (digit_check.is_eof || isascii(digit_check.token.id))
        {
            char found_str[2] = {digit_check.is_eof ? '\0' : digit_check.token.id, '\0'};
            return epc_parser_error_result(
                ctx, token_offset + 2, "Expected hex digit", "hex digit", digit_check.is_eof ? "EOF" : found_str
            );
        }
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Expected hex digit", "hex digit", &digit_check.token, 1
        );
    }

    size_t current_len = 3;
    while (1)
    {
        parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + current_len);
        if (res.is_eof)
        {
            break;
        }
        if (!isxdigit(res.token.id))
        {
            break;
        }
        current_len++;
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = current_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_hex(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "hex", phex_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->expected_value = "hex literal";

    return p;
}

EASY_PC_API epc_parser_t *
epc_hex(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_hex(name));
}

static epc_parse_result_t
poctal_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(
            ctx, token_offset, "Unexpected end of input", parser_get_expected_str(self), "EOF"
        );
    }

    epc_token_id_t const input = input_result.token.id;

    // Must start with '0'
    if (input != '0')
    {
        if (isascii(input))
        {
            char found_str[2] = {input, '\0'};
            return epc_parser_error_result(
                ctx, token_offset, "Expected octal literal", parser_get_expected_str(self), found_str
            );
        }
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Expected octal literal", parser_get_expected_str(self), &input_result.token, 1
        );
    }

    size_t current_len = 1;
    while (1)
    {
        parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + current_len);
        if (res.is_eof)
        {
            break;
        }
        if (res.token.id < '0' || res.token.id > '7')
        {
            break;
        }
        current_len++;
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = current_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_octal(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "octal", poctal_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->expected_value = "octal literal";

    return p;
}

EASY_PC_API epc_parser_t *
epc_octal(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_octal(name));
}

static epc_parse_result_t
pidentifier_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(
            ctx, token_offset, "Unexpected end of input", parser_get_expected_str(self), "EOF"
        );
    }

    epc_token_id_t const input = input_result.token.id;

    // First char must be alpha or underscore
    if (!isalpha(input) && input != '_')
    {
        if (isascii(input))
        {
            char found_str[2] = {input, '\0'};
            return epc_parser_error_result(
                ctx, token_offset, "Expected identifier", parser_get_expected_str(self), found_str
            );
        }
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Expected identifier", parser_get_expected_str(self), &input_result.token, 1
        );
    }

    size_t current_len = 1;
    while (1)
    {
        parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + current_len);
        if (res.is_eof)
        {
            break;
        }
        if (!isalnum(res.token.id) && res.token.id != '_')
        {
            break;
        }
        current_len++;
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = current_len;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_identifier(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "identifier", pidentifier_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_identifier(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_identifier(name));
}

static epc_parse_result_t
por_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_error_t * original_furthest_error = NULL;
    parser_list_t * alternatives = self->data.parser_list;

    if (alternatives == NULL || alternatives->count == 0)
    {
        return epc_parser_error_result(
            ctx, token_offset, "No alternatives provided to 'or' parser", epc_parser_get_name(self), "N/A"
        );
    }

    original_furthest_error = parser_furthest_error_copy(ctx);

    for (int i = 0; i < alternatives->count; ++i)
    {
        epc_parser_t * current_parser = alternatives->parsers[i];
        if (current_parser)
        {
            epc_parse_result_t child_result = parse(current_parser, ctx, token_offset);
            if (!child_result.is_error)
            {
                // Return the child's success, but mark the CPT node with this 'or' parser
                epc_cpt_node_t * or_node = epc_node_alloc(ctx, self, self->tag);
                if (or_node == NULL)
                {
                    epc_parser_result_cleanup(&child_result);
                    epc_parser_error_free(original_furthest_error);

                    return epc_parser_error_result(
                        ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
                    );
                }

                or_node->token.offset = child_result.data.success->token.offset;
                or_node->token.count = child_result.data.success->token.count;
                or_node->semantic_start_offset = child_result.data.success->semantic_start_offset;
                or_node->semantic_end_offset = child_result.data.success->semantic_end_offset;
                or_node->children = calloc(1, sizeof(*or_node->children));
                if (or_node->children == NULL)
                {
                    epc_parser_result_cleanup(&child_result);
                    epc_parser_error_free(original_furthest_error);

                    return epc_parser_error_result(
                        ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
                    );
                }

                or_node->children[0] = child_result.data.success;
                or_node->children_count = 1;

                parser_furthest_error_restore(ctx, &original_furthest_error);

                return epc_parser_success_result(or_node);
            }
            else
            {
                epc_parser_result_cleanup(&child_result);
            }
        }
    }

    /* No alternatives matched if we get here. */
    epc_parser_error_free(original_furthest_error);

    size_t estimated_len = 0;
    for (int i = 0; i < alternatives->count; ++i)
    {
        if (alternatives->parsers[i])
        {
            char const * temp_expected = parser_get_expected_str(alternatives->parsers[i]);

            estimated_len += strlen(temp_expected);
            if (i < alternatives->count - 1)
            {
                estimated_len += strlen(" or ");
            }
        }
    }

    char const * expected_str;
    char * aggregated_expected_str = NULL;
    if (estimated_len > 0)
    {
        aggregated_expected_str = malloc(estimated_len + 1);

        if (aggregated_expected_str != NULL)
        {
            aggregated_expected_str[0] = '\0';
            for (int i = 0; i < alternatives->count; ++i)
            {
                if (alternatives->parsers[i])
                {
                    char const * child_expected = parser_get_expected_str(alternatives->parsers[i]);
                    if (child_expected)
                    {
                        strcat(aggregated_expected_str, child_expected);
                        if (i < alternatives->count - 1)
                        {
                            strcat(aggregated_expected_str, " or ");
                        }
                    }
                }
            }
        }
        expected_str = aggregated_expected_str;
    }
    else
    {
        expected_str = epc_parser_get_name(self);
    }

    /* No alternatives matched. */
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    epc_token_id_t const input = input_result.token.id;
    epc_parse_result_t result;
    static char const * no_alternative_matched = "No alternative matched";

    if (isascii(input))
    {
        char found_buffer[FOUND_BUFFER_SIZE];
        snprintf(found_buffer, sizeof(found_buffer), "%c", input);

        result = epc_parser_error_result(ctx, token_offset, no_alternative_matched, expected_str, found_buffer);
    }
    else
    {
        result = epc_parser_error_result_token_list(
            ctx, token_offset, no_alternative_matched, expected_str, &input_result.token, 1
        );
    }

    free(aggregated_expected_str);

    return result;
}

static epc_parser_t *
vepc_or(char const * name, int count, va_list args)
{
    epc_parser_t * p = epc_parser_allocate(name, "or", por_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser_list = parser_list_create_v(count, args);
    p->data.type = PARSER_DATA_TYPE_PARSER_LIST;

    return p;
}

EASY_PC_API epc_parser_t *
epc_or(epc_parser_list * list, char const * name, int count, ...)
{
    va_list args;

    va_start(args, count);
    epc_parser_t * p = vepc_or(name, count, args);
    va_end(args);

    epc_parser_list_add(list, p);
    return p;
}

// --- C++ Comment Parser Implementation ---
// Matches "//" followed by any characters until a newline or EOF.
static epc_parse_result_t
pcpp_comment_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_token_t found_tokens[3] = {0};
    match_status_t status;
    size_t token_count;

    // 1. Match the opening double slashes.
    for (token_count = 0; token_count < 2; token_count++)
    {
        status = try_match_token(ctx, token_offset + token_count, '/', &found_tokens[token_count]);
        if (status == MATCH_EOF)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "//", "EOF");
        }
        if (status == MATCH_MISMATCH)
        {
            return epc_parser_error_result_token_list(
                ctx, token_offset, "Expected '//'", "//", found_tokens, token_count + 1
            );
        }
    }

    // 2. Match content until newline or EOF
    size_t const res = consume_until_newline(ctx, token_offset + token_count);
    token_count += res;

    // Success - create a CPT node for the whole comment
    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = token_count;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_cpp_comment(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "cpp_comment", pcpp_comment_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->expected_value = "// C++ style comment";

    return p;
}

EASY_PC_API epc_parser_t *
epc_cpp_comment(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_cpp_comment(name));
}

// --- C-style Comment Parser Implementation ---
// Matches "/* ... */".
static epc_parse_result_t
pc_comment_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_token_t found_tokens[3] = {0};
    match_status_t status;
    size_t token_count;
    char c_comment_prefix[] = "/*";

    // 1. Match the C comment prefix.
    for (token_count = 0; token_count < 2; token_count++)
    {
        status = try_match_token(
            ctx, token_offset + token_count, c_comment_prefix[token_count], &found_tokens[token_count]
        );
        if (status == MATCH_EOF)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "/*", "EOF");
        }
        if (status == MATCH_MISMATCH)
        {
            return epc_parser_error_result_token_list(
                ctx, token_offset, "Expected '/*'", "/*", found_tokens, token_count + 1
            );
        }
    }

    bool prev_was_star = false;

    // 2. Match content until "*/"
    while (1)
    {
        parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + token_count);
        if (res.is_eof)
        {
            return epc_parser_error_result(ctx, token_offset, "Unterminated C-style comment", "*/", "EOF");
        }

        epc_token_id_t const c = res.token.id;
        token_count++;

        if (prev_was_star && c == '/')
        {
            break;
        }
        prev_was_star = (c == '*');
    }

    // Success - create a CPT node for the whole comment
    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = token_count;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_c_comment(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "c_comment", pc_comment_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->expected_value = "/* C-style comment */";

    return p;
}

EASY_PC_API epc_parser_t *
epc_c_comment(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_c_comment(name));
}

// --- Bash Comment Parser Implementation ---
// Matches "#" followed by any characters until a newline or EOF.
static epc_parse_result_t
pbash_comment_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_token_t found_tokens[2] = {0};
    match_status_t status = try_match_token(ctx, token_offset, '#', &found_tokens[0]);
    if (status == MATCH_EOF)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "#", "EOF");
    }
    if (status == MATCH_MISMATCH)
    {
        return epc_parser_error_result_token_list(ctx, token_offset, "Expected '#'", "#", found_tokens, 1);
    }

    size_t token_count = 1;

    // 2. Match content until newline or EOF
    size_t const res = consume_until_newline(ctx, token_offset + token_count);
    token_count += res;

    // Success - create a CPT node for the whole comment
    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = token_count;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_bash_comment(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "bash_comment", pbash_comment_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->expected_value = "# Bash style comment";

    return p;
}

EASY_PC_API epc_parser_t *
epc_bash_comment(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_bash_comment(name));
}

static epc_parse_result_t
pand_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parser_list_t * sequence = self->data.parser_list;

    if (sequence == NULL || sequence->count == 0)
    {
        return epc_parser_error_result(
            ctx, token_offset, "No parsers in 'and' sequence", epc_parser_get_name(self), "N/A"
        );
    }

    epc_cpt_node_t ** children_nodes = calloc(sequence->count, sizeof(*children_nodes));

    if (children_nodes == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    size_t current_token_offset = token_offset;

    epc_parse_result_t failed_child_result = {0};
    epc_parse_result_t null_child_result = {0};
    int child_count = 0;
    for (int i = 0; i < sequence->count; i++, child_count++)
    {
        epc_parser_t * current_parser = sequence->parsers[i];
        if (current_parser)
        {
            epc_parse_result_t child_result = parse(current_parser, ctx, current_token_offset);
            if (child_result.is_error)
            {
                failed_child_result = child_result;
                break;
            }
            children_nodes[i] = child_result.data.success;
            current_token_offset += child_result.data.success->token.count;
        }
        else
        {
            null_child_result = epc_parser_error_result(
                ctx, current_token_offset, "NULL parser found in 'and' sequence", epc_parser_get_name(self), "NULL"
            );
            break;
        }
    }

    /* Check if any errors occurred while checking the sequence of parsers. */
    if (null_child_result.is_error || failed_child_result.is_error)
    {
        for (int i = 0; i < child_count; i++)
        {
            epc_node_free(children_nodes[i]);
        }
        free(children_nodes);
    }

    if (null_child_result.is_error)
    {
        return null_child_result;
    }
    if (failed_child_result.is_error)
    {
        return failed_child_result;
    }

    /* No child errors, so the AND condition has succeeded. */

    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        for (int i = 0; i < sequence->count; i++)
        {
            epc_node_free(children_nodes[i]);
        }
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    parent_node->children = children_nodes;
    parent_node->children_count = sequence->count;
    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    parent_node->semantic_start_offset = get_semantic_start_offset(parent_node->children, parent_node->children_count);
    parent_node->semantic_end_offset = get_semantic_end_offset(parent_node->children, parent_node->children_count);

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
vepc_and(char const * name, int count, va_list args)
{
    epc_parser_t * p = epc_parser_allocate(name, "and", pand_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser_list = parser_list_create_v(count, args);
    p->data.type = PARSER_DATA_TYPE_PARSER_LIST;

    return p;
}

epc_parser_t *
epc_and(epc_parser_list * list, char const * name, int count, ...)
{
    va_list args;

    va_start(args, count);
    epc_parser_t * p = vepc_and(name, count, args);
    va_end(args);

    epc_parser_list_add(list, p);
    return p;
}

static epc_parse_result_t
pskip_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "skip", "EOF");
    }
    epc_parser_t * parser_to_skip = self->data.parser;
    if (parser_to_skip == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "p_skip received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    size_t current_token_offset = token_offset;
    size_t total_skipped_len = 0;

    while (1)
    {
        epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);
        epc_parse_result_t child_result = parse(parser_to_skip, ctx, current_token_offset);
        if (child_result.is_error)
        {
            parser_furthest_error_restore(ctx, &original_furthest_error);
            epc_parser_result_cleanup(&child_result);
            break;
        }
        if (child_result.data.success->token.count == 0)
        {
            /*
             * No progress is being made through the input, so this will loop
             * indefinitely.
             * Return with an error.
             */
            epc_parser_error_free(original_furthest_error);
            epc_parser_result_cleanup(&child_result);
            return epc_parser_error_result(
                ctx, token_offset, infinite_recursion_detected_msg, epc_parser_get_name(self), "N/A"
            );
        }
        total_skipped_len += child_result.data.success->token.count;
        current_token_offset += child_result.data.success->token.count;
        epc_parser_error_free(original_furthest_error);
        epc_parser_result_cleanup(&child_result);
    }

    epc_cpt_node_t * dummy_node = epc_node_alloc(ctx, self, self->tag);
    if (dummy_node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    dummy_node->token.offset = token_offset;
    dummy_node->token.count = total_skipped_len;

    return epc_parser_success_result(dummy_node);
}

static epc_parser_t *
_epc_skip(char const * name, epc_parser_t * parser_to_skip)
{
    epc_parser_t * p = epc_parser_allocate(name, "skip", pskip_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser = parser_to_skip;
    p->data.type = PARSER_DATA_TYPE_PARSER;

    return p;
}

EASY_PC_API epc_parser_t *
epc_skip(epc_parser_list * list, char const * name, epc_parser_t * parser_to_skip)
{
    return epc_parser_list_add(list, _epc_skip(name, parser_to_skip));
}

static epc_parse_result_t
pplus_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_t * parser_to_repeat = self->data.parser;

    if (parser_to_repeat == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "p_plus received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    child_list_t children = {0};
    if (!child_list_init(ctx, &children, 4))
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    size_t current_token_offset = token_offset;
    size_t plus_start_input_offset = token_offset;

    epc_parse_result_t first_child_result = parse(parser_to_repeat, ctx, current_token_offset);
    if (first_child_result.is_error)
    {
        child_list_release(&children);
        return first_child_result;
    }

    if (!child_list_append(&children, first_child_result.data.success))
    {
        child_list_release(&children);
        return epc_parser_error_result(
            ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
        );
    }
    current_token_offset += first_child_result.data.success->token.count;

    bool infinite_recursion_detected = false;
    while (!infinite_recursion_detected)
    {
        size_t loop_start_input_offset = current_token_offset;
        epc_parse_result_t child_result = parse(parser_to_repeat, ctx, current_token_offset);
        if (!child_result.is_error)
        {
            if (!child_list_append(&children, child_result.data.success))
            {
                child_list_release(&children);
                return epc_parser_error_result(
                    ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
                );
            }
            current_token_offset += child_result.data.success->token.count;
        }
        else
        {
            epc_parser_result_cleanup(&child_result);
            break;
        }
        infinite_recursion_detected = current_token_offset == loop_start_input_offset;
    }

    if (infinite_recursion_detected)
    {
        child_list_release(&children);
        return epc_parser_error_result(
            ctx, current_token_offset, infinite_recursion_detected_msg, "Progress", "No progress"
        );
    }

    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        child_list_release(&children);
        return epc_parser_error_result(
            ctx, plus_start_input_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
        );
    }

    child_list_transfer(&children, parent_node);
    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    parent_node->semantic_start_offset = get_semantic_start_offset(parent_node->children, parent_node->children_count);
    parent_node->semantic_end_offset = get_semantic_end_offset(parent_node->children, parent_node->children_count);

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
_epc_plus(char const * name, epc_parser_t * parser_to_repeat)
{
    epc_parser_t * p = epc_parser_allocate(name, "plus", pplus_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser = parser_to_repeat;
    p->data.type = PARSER_DATA_TYPE_PARSER;

    return p;
}

EASY_PC_API epc_parser_t *
epc_plus(epc_parser_list * list, char const * name, epc_parser_t * parser_to_repeat)
{
    return epc_parser_list_add(list, _epc_plus(name, parser_to_repeat));
}

static epc_parse_result_t
pchar_range_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char_range_data_t * range = &self->data.range;
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    {
        char expected_str[EPC_ERROR_EXPECTED_MAX_LEN]; // e.g., "character in range [a-z]"
        if (isascii(range->start) && isascii(range->end))
        {
            snprintf(expected_str, sizeof(expected_str), "character in range [%c-%c]", range->start, range->end);
        }
        else
        {
            snprintf(expected_str, sizeof(expected_str), "token in range [%u-%u]", range->start, range->end);
        }

        if (input_result.is_eof)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", expected_str, "EOF");
        }

        epc_token_id_t const input = input_result.token.id;

        if (input < (epc_token_id_t)range->start || input > (epc_token_id_t)range->end)
        {
            /* Not in range. */
            if (isascii(input))
            {
                char found_str[2] = {input, '\0'};

                return epc_parser_error_result(ctx, token_offset, "Unexpected character", expected_str, found_str);
            }
            return epc_parser_error_result_token_list(
                ctx, token_offset, "Unexpected token", expected_str, &input_result.token, 1
            );
        }
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }
    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_char_range(char const * name, char char_start, char char_end)
{
    epc_parser_t * p = epc_parser_allocate(name, "char_range", pchar_range_parse_fn);

    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_CHAR_RANGE;
    p->data.range.start = char_start;
    p->data.range.end = char_end;

    return p;
}

EASY_PC_API epc_parser_t *
epc_char_range(epc_parser_list * list, char const * name, char char_start, char char_end)
{
    return epc_parser_list_add(list, _epc_char_range(name, char_start, char_end));
}

static epc_parse_result_t
pany_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "any character", "EOF");
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }
    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_any(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "any", pany_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_any(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_any(name));
}

static epc_parse_result_t
pnone_of_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char const * chars_to_avoid = self->data.string;
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    {
        char expected_str[ERROR_MSG_BUFFER_SIZE];

        snprintf(expected_str, sizeof(expected_str), "character not in set '%s'", chars_to_avoid);

        if (input_result.is_eof)
        {
            return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", expected_str, "EOF");
        }

        epc_token_id_t const input = input_result.token.id;

        if (!isascii(input) || strchr(chars_to_avoid, input) != NULL)
        {
            if (isascii(input))
            {
                char found_str[2] = {input, '\0'};

                return epc_parser_error_result(
                    ctx, token_offset, "Character found in forbidden set", expected_str, found_str
                );
            }

            return epc_parser_error_result_token_list(
                ctx, token_offset, "Token found in forbidden set", expected_str, &input_result.token, 1
            );
        }
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }
    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_none_of(char const * name, char const * chars_to_avoid)
{
    epc_parser_t * p = epc_parser_allocate(name, "none_of", pnone_of_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    char * duplicated_chars = strdup(chars_to_avoid);

    if (duplicated_chars == NULL)
    {
        free(p);
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_STRING;
    p->data.string = duplicated_chars;

    return p;
}

EASY_PC_API epc_parser_t *
epc_none_of(epc_parser_list * list, char const * name, char const * chars_to_avoid)
{
    return epc_parser_list_add(list, _epc_none_of(name, chars_to_avoid));
}

static epc_parse_result_t
pmany_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_t * parser_to_repeat = self->data.parser;

    if (parser_to_repeat == NULL)
    {
        // Should not happen if grammar is well-formed
        return epc_parser_error_result(
            ctx, token_offset, "p_many received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    size_t current_token_offset = token_offset;
    child_list_t children = {0};

    if (!child_list_init(ctx, &children, 4))
    {
        return epc_parser_error_result(
            ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
        );
    }

    bool infinite_recursion_detected = false;
    while (!infinite_recursion_detected) // Loop as long as child parser matches
    {
        size_t loop_start_input_offset = current_token_offset;
        epc_parse_result_t child_result = parse(parser_to_repeat, ctx, current_token_offset);
        if (child_result.is_error)
        {
            epc_parser_result_cleanup(&child_result);
            break;
        }
        if (!child_list_append(&children, child_result.data.success))
        {
            child_list_release(&children);
            return epc_parser_error_result(
                ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }
        current_token_offset += child_result.data.success->token.count;

        infinite_recursion_detected = current_token_offset == loop_start_input_offset;
    }

    if (infinite_recursion_detected)
    {
        child_list_release(&children);
        return epc_parser_error_result(
            ctx, current_token_offset, infinite_recursion_detected_msg, "Progress", "No progress"
        );
    }

    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        child_list_release(&children);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    child_list_transfer(&children, parent_node);
    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    if (parent_node->children_count > 0)
    {
        parent_node->semantic_start_offset
            = get_semantic_start_offset(parent_node->children, parent_node->children_count);
        parent_node->semantic_end_offset = get_semantic_end_offset(parent_node->children, parent_node->children_count);
    }

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
_epc_many(char const * name, epc_parser_t * p_to_repeat)
{
    epc_parser_t * p = epc_parser_allocate(name, "many", pmany_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser = p_to_repeat;
    p->data.type = PARSER_DATA_TYPE_PARSER;

    return p;
}

EASY_PC_API epc_parser_t *
epc_many(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_many(name, p));
}

static epc_parse_result_t
pcount_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    count_data_t * count_data = &self->data.count;
    epc_parser_t * parser_to_repeat = count_data->parser;
    size_t min_matches = count_data->count_min;
    size_t max_matches = count_data->count_max;

    if (parser_to_repeat == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }
    /* max must be >= min. */
    if (max_matches < min_matches)
    {
        char found[FOUND_BUFFER_SIZE];

        snprintf(found, sizeof(found), "min: %zu, max: %zu", min_matches, max_matches);
        return epc_parser_error_result(ctx, token_offset, "Max should be >= min", epc_parser_get_name(self), found);
    }

    size_t current_token_offset = token_offset;
    child_list_t children = {0};

    if (!child_list_init(ctx, &children, 4))
    {
        return epc_parser_error_result(
            ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
        );
    }

    size_t match_count;
    for (match_count = 0; match_count < max_matches; match_count++)
    {
        epc_parse_result_t child_result = parse(parser_to_repeat, ctx, current_token_offset);

        if (child_result.is_error)
        {
            if (match_count < min_matches)
            {
                child_list_release(&children);

                // Child parser failed to match required number of times
                char msg[ERROR_MSG_BUFFER_SIZE];

                snprintf(msg, sizeof(msg), "Failed to match child at count %zu", match_count + 1);
                epc_parse_result_t error_result = epc_parser_error_result(
                    ctx, current_token_offset, msg, child_result.data.error->expected, child_result.data.error->found
                );
                epc_parser_result_cleanup(&child_result);

                return error_result;
            }
            epc_parser_result_cleanup(&child_result);
            break;
        }

        if (!child_list_append(&children, child_result.data.success))
        {
            child_list_release(&children);
            return epc_parser_error_result(
                ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }
        current_token_offset += child_result.data.success->token.count;
    }

    if (min_matches > 0 && max_matches > 0 && match_count == max_matches)
    {
        /* Matched exactly max times. The next parse needs to fail for this to be called a success. */
        epc_parse_result_t child_result = parse(parser_to_repeat, ctx, current_token_offset);

        if (!child_result.is_error)
        {
            child_list_release(&children);

            // Child parser matched more than the maximum number of allowed times.
            char msg[ERROR_MSG_BUFFER_SIZE];

            snprintf(msg, sizeof(msg), "Child matched unexpectedly at count %zu", max_matches + 1);
            epc_parse_result_t error_result = epc_parser_error_result(
                ctx, current_token_offset, msg, child_result.data.error->expected, child_result.data.error->found
            );
            epc_parser_result_cleanup(&child_result);

            return error_result;
        }
        epc_parser_result_cleanup(&child_result);
    }

    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        child_list_release(&children);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    child_list_transfer(&children, parent_node);
    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    if (parent_node->children_count > 0)
    {
        parent_node->semantic_start_offset
            = get_semantic_start_offset(parent_node->children, parent_node->children_count);
        parent_node->semantic_end_offset = get_semantic_end_offset(parent_node->children, parent_node->children_count);
    }

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
_epc_count(char const * name, size_t num, epc_parser_t * p_to_repeat)
{
    epc_parser_t * p = epc_parser_allocate(name, "count", pcount_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_COUNT;
    p->data.count.count_min = num;
    p->data.count.count_max = num;
    p->data.count.parser = p_to_repeat;
    return p;
}

EASY_PC_API epc_parser_t *
epc_count(epc_parser_list * list, char const * name, size_t num, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_count(name, num, p));
}

static epc_parser_t *
_epc_count_range(char const * name, size_t min, size_t max, epc_parser_t * p_to_repeat)
{
    epc_parser_t * p = epc_parser_allocate(name, "count_range", pcount_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_COUNT;
    p->data.count.count_min = min;
    p->data.count.count_max = max;
    p->data.count.parser = p_to_repeat;
    return p;
}

EASY_PC_API epc_parser_t *
epc_count_range(epc_parser_list * list, char const * name, size_t min, size_t max, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_count_range(name, min, max, p));
}

static epc_parse_result_t
pbetween_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "between", "EOF");
    }

    epc_parser_error_t * original_furthest_error = NULL;
    between_data_t * between_data = &self->data.between;
    epc_parser_t * p_open = between_data->open;
    epc_parser_t * p_wrapped = between_data->parser;
    epc_parser_t * p_close = between_data->close;
    size_t open_len = 0;
    size_t close_len = 0;

    if (p_open == NULL || p_wrapped == NULL || p_close == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "between received NULL child parser(s)", epc_parser_get_name(self), "NULL"
        );
    }

    size_t current_token_offset = token_offset;
    original_furthest_error = parser_furthest_error_copy(ctx);

    // 1. Match 'open'
    epc_parse_result_t open_result = parse(p_open, ctx, current_token_offset);
    if (open_result.is_error)
    {
        epc_parser_error_free(original_furthest_error);

        return open_result;
    }

    open_len = open_result.data.success->token.count;
    current_token_offset += open_result.data.success->token.count;
    epc_parser_result_cleanup(&open_result);

    // 2. Match 'wrapped' parser
    epc_parse_result_t wrapped_result = parse(p_wrapped, ctx, current_token_offset);
    if (wrapped_result.is_error)
    {
        epc_parser_error_free(original_furthest_error);

        return wrapped_result;
    }
    current_token_offset += wrapped_result.data.success->token.count;
    /* Don't clean up the wrapped result as that is what gets returned on success. */

    // 3. Match 'close'
    epc_parse_result_t close_result = parse(p_close, ctx, current_token_offset);
    if (close_result.is_error)
    {
        epc_parser_error_free(original_furthest_error);

        return close_result;
    }
    close_len = close_result.data.success->token.count;
    current_token_offset += close_result.data.success->token.count;
    epc_parser_result_cleanup(&close_result);

    // Success - create a node for 'between'
    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        epc_parser_result_cleanup(&wrapped_result);
        epc_parser_error_free(original_furthest_error);

        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    parent_node->children = calloc(1, sizeof(*parent_node->children));
    if (parent_node->children == NULL)
    {
        epc_parser_result_cleanup(&wrapped_result);
        epc_parser_error_free(original_furthest_error);

        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    // Restore furthest error as this parser suppresses it
    parser_furthest_error_restore(ctx, &original_furthest_error);

    parent_node->children[0] = wrapped_result.data.success; // Only the wrapped result is kept as a child
    parent_node->children_count = 1;

    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    parent_node->semantic_start_offset = open_len;
    parent_node->semantic_end_offset = close_len;

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
_epc_between(char const * name, epc_parser_t * p_open, epc_parser_t * p_wrapped, epc_parser_t * p_close)
{
    epc_parser_t * p = epc_parser_allocate(name, "between", pbetween_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_BETWEEN;
    p->data.between.open = p_open;
    p->data.between.parser = p_wrapped;
    p->data.between.close = p_close;

    return p;
}

EASY_PC_API epc_parser_t *
epc_between(epc_parser_list * list, char const * name, epc_parser_t * open, epc_parser_t * p, epc_parser_t * close)
{
    return epc_parser_list_add(list, _epc_between(name, open, p, close));
}

static epc_parse_result_t
pdelimited_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    delimited_data_t * delimited_data = &self->data.delimited;
    epc_parser_t * item_parser = delimited_data->item;
    epc_parser_t * delimiter_parser = delimited_data->delimiter;

    if (item_parser == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "p_delimited received NULL item parser", epc_parser_get_name(self), "NULL"
        );
    }
    // Delimiter can be NULL, meaning no delimiter, just sequence of items

    size_t current_token_offset = token_offset;
    child_list_t children = {0};

    if (!child_list_init(ctx, &children, 4))
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    // First item (must match)
    epc_parse_result_t first_item_result = parse(item_parser, ctx, current_token_offset);

    if (first_item_result.is_error)
    {
        child_list_release(&children);
        return first_item_result;
    }
    if (!child_list_append(&children, first_item_result.data.success))
    {
        child_list_release(&children);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    current_token_offset += first_item_result.data.success->token.count;

    // Remaining items (item + delimiter)
    bool infinite_recursion_detected = false;

    while (!infinite_recursion_detected)
    {
        size_t loop_start_input_offset = current_token_offset;
        size_t offset_before_delimiter = current_token_offset;

        if (delimiter_parser != NULL)
        {
            epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);
            epc_parse_result_t delim_result = parse(delimiter_parser, ctx, current_token_offset);

            if (delim_result.is_error)
            {
                // Delimiter not found, stop parsing further items
                epc_parser_result_cleanup(&delim_result);
                parser_furthest_error_restore(ctx, &original_furthest_error);
                break;
            }
            epc_parser_error_free(original_furthest_error);
            current_token_offset += delim_result.data.success->token.count;
            epc_parser_result_cleanup(&delim_result);
        }
        epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);
        epc_parse_result_t item_result = parse(item_parser, ctx, current_token_offset);
        if (item_result.is_error)
        {
            if (delimiter_parser != NULL)
            {
                if (delimited_data->is_flexible)
                {
                    // Flexible mode: backtrack over delimiter and finish
                    epc_parser_result_cleanup(&item_result);
                    parser_furthest_error_restore(ctx, &original_furthest_error);
                    current_token_offset = offset_before_delimiter;
                    break;
                }

                child_list_release(&children);
                parser_furthest_error_restore(ctx, &original_furthest_error);
                epc_parser_result_cleanup(&item_result);

                parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, current_token_offset);

                return epc_parser_error_result_token_list(
                    ctx,
                    current_token_offset,
                    "Unexpected trailing delimiter",
                    parser_get_expected_str(item_parser),
                    &input_result.token,
                    1
                );
            }
            // Item not found, stop parsing further items
            parser_furthest_error_restore(ctx, &original_furthest_error);
            epc_parser_result_cleanup(&item_result);
            break;
        }
        parser_furthest_error_restore(ctx, &original_furthest_error);
        if (!child_list_append(&children, item_result.data.success))
        {
            child_list_release(&children);
            return epc_parser_error_result(
                ctx, current_token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }
        current_token_offset += item_result.data.success->token.count;

        infinite_recursion_detected = current_token_offset == loop_start_input_offset;
    }

    if (infinite_recursion_detected)
    {
        child_list_release(&children);
        return epc_parser_error_result(
            ctx, current_token_offset, infinite_recursion_detected_msg, "Progress", "No progress"
        );
    }

    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        child_list_release(&children);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    child_list_transfer(&children, parent_node);
    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    if (parent_node->children_count > 0)
    {
        parent_node->semantic_start_offset
            = get_semantic_start_offset(parent_node->children, parent_node->children_count);
        parent_node->semantic_end_offset = get_semantic_end_offset(parent_node->children, parent_node->children_count);
    }

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
_epc_delimited(char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser)
{
    epc_parser_t * p = epc_parser_allocate(name, "delimited", pdelimited_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_DELIMITED;
    p->data.delimited.item = item_parser;
    p->data.delimited.delimiter = delimiter_parser;

    return p;
}

EASY_PC_API epc_parser_t *
epc_delimited(epc_parser_list * list, char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser)
{
    return epc_parser_list_add(list, _epc_delimited(name, item_parser, delimiter_parser));
}

static epc_parser_t *
_epc_delimited_flex(char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser)
{
    epc_parser_t * p = epc_parser_allocate(name, "delimited_flex", pdelimited_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_DELIMITED;
    p->data.delimited.item = item_parser;
    p->data.delimited.delimiter = delimiter_parser;
    p->data.delimited.is_flexible = true;

    return p;
}

EASY_PC_API epc_parser_t *
epc_delimited_flex(
    epc_parser_list * list, char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser
)
{
    return epc_parser_list_add(list, _epc_delimited_flex(name, item_parser, delimiter_parser));
}

static epc_parse_result_t
poptional_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
        if (node == NULL)
        {
            return epc_parser_error_result(
                ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }

        node->token.offset = token_offset;
        node->token.count = 0;

        return epc_parser_success_result(node);
    }

    epc_parser_error_t * original_furthest_error = NULL;
    epc_parser_t * child_parser = self->data.parser;

    if (child_parser == NULL) // Should not happen if grammar is well-formed
    {
        return epc_parser_error_result(
            ctx, token_offset, "p_optional received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    original_furthest_error = parser_furthest_error_copy(ctx); // Save before child parse

    epc_parse_result_t child_result = parse(child_parser, ctx, token_offset);

    if (!child_result.is_error)
    {
        // Child matched, return its success result wrapped in an optional node
        epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
        if (parent_node == NULL)
        {
            epc_parser_result_cleanup(&child_result);
            epc_parser_error_free(original_furthest_error);
            return epc_parser_error_result(
                ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }
        parent_node->children = calloc(1, sizeof(*parent_node->children));
        if (parent_node->children == NULL)
        {
            epc_parser_result_cleanup(&child_result);
            epc_parser_error_free(original_furthest_error);
            return epc_parser_error_result(
                ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }
        parent_node->children[0] = child_result.data.success;
        parent_node->children_count = 1;

        parser_furthest_error_restore(ctx, &original_furthest_error);

        parent_node->token.offset = child_result.data.success->token.offset;
        parent_node->token.count = child_result.data.success->token.count;
        parent_node->semantic_start_offset = child_result.data.success->semantic_start_offset;
        parent_node->semantic_end_offset = child_result.data.success->semantic_end_offset;

        return epc_parser_success_result(parent_node);
    }

    // Child failed, p_optional still succeeds, consuming no input.
    // Return an empty optional node.
    epc_parser_result_cleanup(&child_result);
    epc_parser_error_free(original_furthest_error);

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 0;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_optional(char const * name, epc_parser_t * p_to_make_optional)
{
    epc_parser_t * p = epc_parser_allocate(name, "optional", poptional_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser = p_to_make_optional;
    p->data.type = PARSER_DATA_TYPE_PARSER;

    return p;
}

EASY_PC_API epc_parser_t *
epc_optional(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_optional(name, p));
}

static epc_parse_result_t
plookahead_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "lookahead", "EOF");
    }

    epc_parser_t * child_parser = self->data.parser;

    if (child_parser == NULL) // Should not happen if grammar is well-formed
    {
        return epc_parser_error_result(
            ctx, token_offset, "p_lookahead received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);
    epc_parse_result_t child_result = parse(child_parser, ctx, token_offset);

    parser_furthest_error_restore(ctx, &original_furthest_error);

    if (child_result.is_error)
    {
        // Child failed, p_lookahead fails. Propagate the child's error.
        return child_result;
    }

    epc_parser_result_cleanup(&child_result);

    // Child matched, but p_lookahead consumes no input.
    // Return a dummy success node of length 0.
    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 0;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_lookahead(char const * name, epc_parser_t * p_to_lookahead)
{
    epc_parser_t * p = epc_parser_allocate(name, "lookahead", plookahead_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser = p_to_lookahead;
    p->data.type = PARSER_DATA_TYPE_PARSER;

    return p;
}

EASY_PC_API epc_parser_t *
epc_lookahead(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_lookahead(name, p));
}

static epc_parse_result_t
pnot_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "not", "EOF");
    }

    epc_parser_t * child_parser = self->data.parser;

    if (child_parser == NULL) // Should not happen if grammar is well-formed
    {
        return epc_parser_error_result(
            ctx, token_offset, "p_not received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx); // Save before child parse
    epc_parse_result_t child_result = parse(child_parser, ctx, token_offset);

    parser_furthest_error_restore(ctx, &original_furthest_error);

    if (child_result.is_error)
    {
        // Child failed, p_not succeeds.
        epc_parser_result_cleanup(&child_result);
        // Return a dummy success node of length 0.
        epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
        if (node == NULL)
        {
            return epc_parser_error_result(
                ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }

        node->token.offset = token_offset;
        node->token.count = 0;

        return epc_parser_success_result(node);
    }

    // Child succeeded, p_not fails.
    // Create a specific error message for p_not.
    char expected_str[FOUND_BUFFER_SIZE];

    snprintf(expected_str, sizeof(expected_str), "not %s", parser_get_expected_str(child_parser));

    char found_str[FOUND_BUFFER_SIZE];
    snprintf(
        found_str,
        sizeof(found_str),
        "'%.*s'",
        (int)child_result.data.success->token.count,
        epc_cpt_node_get_content(child_result.data.success)
    );

    epc_parse_result_t result
        = epc_parser_error_result(ctx, token_offset, "Parser unexpectedly matched", expected_str, found_str);
    epc_parser_result_cleanup(&child_result);

    return result;
}

static epc_parser_t *
_epc_not(char const * name, epc_parser_t * p_to_not_match)
{
    epc_parser_t * p = epc_parser_allocate(name, "not", pnot_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.parser = p_to_not_match;
    p->data.type = PARSER_DATA_TYPE_PARSER;

    return p;
}

EASY_PC_API epc_parser_t *
epc_not(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_not(name, p));
}

static epc_parse_result_t
pfail_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char const * failure_message = self->data.string;

    return epc_parser_error_result(ctx, token_offset, failure_message, "Failure", "Failure");
}

static epc_parser_t *
_epc_fail(char const * name, char const * message)
{
    epc_parser_t * p = epc_parser_allocate(name, "fail", pfail_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    char * duplicated_message = strdup(message);
    if (duplicated_message == NULL)
    {
        free(p);
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_STRING;
    p->data.string = duplicated_message;
    return p;
}

EASY_PC_API epc_parser_t *
epc_fail(epc_parser_list * list, char const * name, char const * message)
{
    return epc_parser_list_add(list, _epc_fail(name, message));
}

static epc_parse_result_t
psucceed_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);
    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 0;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_succeed(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "succeed", psucceed_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_succeed(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_succeed(name));
}

static epc_parse_result_t
phex_digit_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", self->tag, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (!isxdigit(input))
    {
        if (isascii(input))
        {
            char found_str[2] = {input, '\0'};

            return epc_parser_error_result(ctx, token_offset, "Unexpected character", self->tag, found_str);
        }
        return epc_parser_error_result_token_list(
            ctx, token_offset, "Unexpected character", self->tag, &input_result.token, 1
        );
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_hex_digit(char const * name)
{
    epc_parser_t * p = epc_parser_allocate(name, "hex_digit", phex_digit_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }

    return p;
}

EASY_PC_API epc_parser_t *
epc_hex_digit(epc_parser_list * list, char const * name)
{
    return epc_parser_list_add(list, _epc_hex_digit(name));
}

static epc_parse_result_t
pone_of_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    char const * chars_to_match = self->data.string;
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);
    char expected_str[ERROR_MSG_BUFFER_SIZE];

    snprintf(expected_str, sizeof(expected_str), "character in set '%s'", chars_to_match);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", expected_str, "EOF");
    }

    epc_token_id_t const input = input_result.token.id;

    if (strchr(chars_to_match, input) == NULL)
    {
        char found_str[2] = {input, '\0'};

        return epc_parser_error_result(ctx, token_offset, "Character not found in set", expected_str, found_str);
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }
    node->token.offset = token_offset;
    node->token.count = 1;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_one_of(char const * name, char const * chars_to_match)
{
    epc_parser_t * p = epc_parser_allocate(name, "one_of", pone_of_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    char * duplicated_chars = strdup(chars_to_match);
    if (duplicated_chars == NULL)
    {
        free(p);
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_STRING;
    p->data.string = duplicated_chars;

    return p;
}

EASY_PC_API epc_parser_t *
epc_one_of(epc_parser_list * list, char const * name, char const * chars_to_match)
{
    return epc_parser_list_add(list, _epc_one_of(name, chars_to_match));
}

static size_t
consume_whitespace(epc_parser_ctx_t * ctx, size_t token_offset, epc_consume_flags_t flags)
{
    size_t len = 0;
    bool consumed_something;

    do
    {
        consumed_something = false;

        /* 1. Consume simple whitespace. */
        if (flags & EPC_CONSUME_WS)
        {
            while (1)
            {
                parse_get_input_result_t res = parse_ctx_get_input_at_offset(ctx, token_offset + len);
                if (res.is_eof)
                {
                    break;
                }
                if (!isspace((unsigned char)res.token.id))
                {
                    break;
                }
                len++;
                consumed_something = true;
            }
        }

        /* 2. Consume comments. */
        if (flags & EPC_CONSUME_ALL_COMMENTS)
        {
            epc_parser_token_t c0;
            match_status_t const s0 = try_match_token(ctx, token_offset + len, '\0', &c0);
            if (s0 == MATCH_EOF)
            {
                break;
            }

            if ((flags & (EPC_CONSUME_CPP_COMMENT | EPC_CONSUME_C_COMMENT)) && c0.id == '/')
            {
                epc_parser_token_t c1;
                match_status_t const s1 = try_match_token(ctx, token_offset + len + 1, '\0', &c1);
                if (s1 != MATCH_EOF)
                {
                    if ((flags & EPC_CONSUME_CPP_COMMENT) && c1.id == '/')
                    {
                        size_t const res = consume_until_newline(ctx, token_offset + len + 2);
                        len += 2 + res;
                        consumed_something = true;
                    }
                    else if ((flags & EPC_CONSUME_C_COMMENT) && c1.id == '*')
                    {
                        size_t const res = consume_c_comment_content(ctx, token_offset + len + 2);
                        len += 2 + res;
                        consumed_something = true;
                    }
                }
            }
            else if ((flags & EPC_CONSUME_BASH_COMMENT) && c0.id == '#')
            {
                size_t const res = consume_until_newline(ctx, token_offset + len + 1);
                len += 1 + res;
                consumed_something = true;
            }
        }
    } while (consumed_something);

    return len;
}

static epc_parse_result_t
plexeme_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    lexeme_data_t * data = &self->data.lexeme;
    epc_parser_t * child_parser = data->parser;
    epc_consume_flags_t flags = data->consume_flags;

    if (child_parser == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "Lexeme/Strip received NULL child parser", epc_parser_get_name(self), "NULL"
        );
    }

    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);

    /* 1. Consume leading whitespace/comments. */
    size_t const leading_ws_len = data->strip_leading ? consume_whitespace(ctx, token_offset, flags) : 0;
    size_t current_token_offset = token_offset + leading_ws_len;

    /* 2. Parse the actual item. */
    epc_parse_result_t item_result = parse(child_parser, ctx, current_token_offset);
    if (item_result.is_error)
    {
        epc_parser_error_free(original_furthest_error);
        return item_result;
    }
    current_token_offset += item_result.data.success->token.count;

    /* 3. Consume trailing whitespace/comments. */
    size_t const trailing_ws_len = data->strip_trailing ? consume_whitespace(ctx, current_token_offset, flags) : 0;
    current_token_offset += trailing_ws_len;

    /* Success - create a node for 'lexeme' or 'strip'. */
    epc_cpt_node_t * parent_node = epc_node_alloc(ctx, self, self->tag);
    if (parent_node == NULL)
    {
        epc_parser_result_cleanup(&item_result);
        epc_parser_error_free(original_furthest_error);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    parser_furthest_error_restore(ctx, &original_furthest_error);

    parent_node->children = calloc(1, sizeof(*parent_node->children));
    if (parent_node->children == NULL)
    {
        epc_parser_result_cleanup(&item_result);
        epc_node_free(parent_node);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    parent_node->children[0] = item_result.data.success;
    parent_node->children_count = 1;

    parent_node->token.offset = token_offset;
    parent_node->token.count = current_token_offset - token_offset;
    parent_node->semantic_start_offset = leading_ws_len;
    parent_node->semantic_end_offset = trailing_ws_len;

    return epc_parser_success_result(parent_node);
}

static epc_parser_t *
_epc_lexeme_ex(char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    epc_parser_t * lex = epc_parser_allocate(name, "lexeme", plexeme_parse_fn);
    if (lex == NULL)
    {
        return NULL;
    }
    lex->data.type = PARSER_DATA_TYPE_LEXEME;
    lex->data.lexeme.parser = p;
    lex->data.lexeme.consume_flags = flags;
    lex->data.lexeme.strip_leading = true;
    lex->data.lexeme.strip_trailing = true;

    return lex;
}

static epc_parser_t *
_epc_lexeme(char const * name, epc_parser_t * p)
{
    return _epc_lexeme_ex(name, p, EPC_CONSUME_ALL);
}

EASY_PC_API epc_parser_t *
epc_lexeme(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_lexeme(name, p));
}

EASY_PC_API epc_parser_t *
epc_lexeme_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    return epc_parser_list_add(list, _epc_lexeme_ex(name, p, flags));
}

static epc_parser_t *
_epc_strip_ex(char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    epc_parser_t * lex = epc_parser_allocate(name, "strip", plexeme_parse_fn);
    if (lex == NULL)
    {
        return NULL;
    }
    lex->data.type = PARSER_DATA_TYPE_LEXEME;
    lex->data.lexeme.parser = p;
    lex->data.lexeme.consume_flags = flags;
    lex->data.lexeme.strip_leading = true;
    lex->data.lexeme.strip_trailing = true;

    return lex;
}

static epc_parser_t *
_epc_strip(char const * name, epc_parser_t * p)
{
    return _epc_strip_ex(name, p, EPC_CONSUME_WS);
}

EASY_PC_API epc_parser_t *
epc_strip(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_strip(name, p));
}

EASY_PC_API epc_parser_t *
epc_strip_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    return epc_parser_list_add(list, _epc_strip_ex(name, p, flags));
}

static epc_parser_t *
_epc_stripl_ex(char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    epc_parser_t * lex = epc_parser_allocate(name, "stripl", plexeme_parse_fn);
    if (lex == NULL)
    {
        return NULL;
    }
    lex->data.type = PARSER_DATA_TYPE_LEXEME;
    lex->data.lexeme.parser = p;
    lex->data.lexeme.consume_flags = flags;
    lex->data.lexeme.strip_leading = true;
    lex->data.lexeme.strip_trailing = false;

    return lex;
}

static epc_parser_t *
_epc_stripl(char const * name, epc_parser_t * p)
{
    return _epc_stripl_ex(name, p, EPC_CONSUME_WS);
}

EASY_PC_API epc_parser_t *
epc_stripl(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_stripl(name, p));
}

EASY_PC_API epc_parser_t *
epc_stripl_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    return epc_parser_list_add(list, _epc_stripl_ex(name, p, flags));
}

static epc_parser_t *
_epc_stripr_ex(char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    epc_parser_t * lex = epc_parser_allocate(name, "stripr", plexeme_parse_fn);
    if (lex == NULL)
    {
        return NULL;
    }
    lex->data.type = PARSER_DATA_TYPE_LEXEME;
    lex->data.lexeme.parser = p;
    lex->data.lexeme.consume_flags = flags;
    lex->data.lexeme.strip_leading = false;
    lex->data.lexeme.strip_trailing = true;

    return lex;
}

static epc_parser_t *
_epc_stripr(char const * name, epc_parser_t * p)
{
    return _epc_stripr_ex(name, p, EPC_CONSUME_WS);
}

EASY_PC_API epc_parser_t *
epc_stripr(epc_parser_list * list, char const * name, epc_parser_t * p)
{
    return epc_parser_list_add(list, _epc_stripr(name, p));
}

EASY_PC_API epc_parser_t *
epc_stripr_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags)
{
    return epc_parser_list_add(list, _epc_stripr_ex(name, p, flags));
}

static epc_parse_result_t
pchainl1_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "chainl1", "EOF");
    }

    delimited_data_t * chain_data = &self->data.delimited;
    epc_parser_t * item_parser = chain_data->item;
    epc_parser_t * op_parser = chain_data->delimiter;

    if (item_parser == NULL || op_parser == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "epc_chainl1 received NULL child parser(s)", epc_parser_get_name(self), "NULL"
        );
    }

    size_t current_token_offset = token_offset;
    epc_parse_result_t left_result;
    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);

    // Parse the first item (must succeed)
    left_result = parse(item_parser, ctx, current_token_offset);
    if (left_result.is_error)
    {
        epc_parser_error_free(original_furthest_error); // Cleanup in error path
        return left_result;
    }
    current_token_offset += left_result.data.success->token.count;

    // Loop to parse (op item) pairs
    while (1)
    {
        epc_parser_error_t * loop_furthest_error = parser_furthest_error_copy(ctx); // Save for loop iteration
        epc_parse_result_t op_result = parse(op_parser, ctx, current_token_offset);
        if (op_result.is_error)
        {
            epc_parser_result_cleanup(&op_result);
            parser_furthest_error_restore(ctx, &loop_furthest_error); // Restore if op fails
            break;                                                    // No more operators, chain ends
        }
        epc_parser_error_free(loop_furthest_error); // Operator matched, clear previous furthest error
        current_token_offset += op_result.data.success->token.count;

        epc_parse_result_t right_result = parse(item_parser, ctx, current_token_offset);
        if (right_result.is_error)
        {
            epc_parser_result_cleanup(&op_result); // op succeeded, but not used in a final success
            epc_parser_result_cleanup(
                &left_result
            ); // accumulated left part needs to be freed. It's not part of the final CPT.
            epc_parser_error_free(original_furthest_error); // Cleanup in error path
            return right_result;                            // Item after operator failed, so chain fails
        }
        current_token_offset += right_result.data.success->token.count;

        // Combine left_result, op_result, and right_result into a new left_result
        epc_cpt_node_t * new_parent_node = epc_node_alloc(ctx, self, self->tag);
        if (new_parent_node == NULL)
        {
            epc_parser_result_cleanup(&op_result);
            epc_parser_result_cleanup(&right_result);
            epc_parser_result_cleanup(&left_result);
            epc_parser_error_free(original_furthest_error); // Cleanup in error path
            return epc_parser_error_result(
                ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }

        new_parent_node->children = calloc(3, sizeof(*new_parent_node->children));
        if (new_parent_node->children == NULL)
        {
            epc_parser_result_cleanup(&op_result);
            epc_parser_result_cleanup(&right_result);
            epc_parser_result_cleanup(&left_result);
            epc_node_free(new_parent_node);
            epc_parser_error_free(original_furthest_error); // Cleanup in error path
            return epc_parser_error_result(
                ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
            );
        }

        new_parent_node->children[0] = left_result.data.success;
        new_parent_node->children[1] = op_result.data.success;
        new_parent_node->children[2] = right_result.data.success;
        new_parent_node->children_count = 3;

        new_parent_node->token.offset = left_result.data.success->token.offset;
        new_parent_node->token.count = current_token_offset - token_offset;
        new_parent_node->semantic_start_offset = left_result.data.success->semantic_start_offset;
        new_parent_node->semantic_end_offset = right_result.data.success->semantic_end_offset;

        // This becomes the new 'left' result
        left_result = epc_parser_success_result(new_parent_node);
    }

    // Restore furthest error before returning final success
    parser_furthest_error_restore(ctx, &original_furthest_error);

    // Final result is the accumulated left_result
    return left_result;
}

static epc_parser_t *
_epc_chainl1(char const * name, epc_parser_t * item_parser, epc_parser_t * op_parser)
{
    epc_parser_t * p = epc_parser_allocate(name, "chainl1", pchainl1_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_DELIMITED; // Reusing this for item/op
    p->data.delimited.item = item_parser;
    p->data.delimited.delimiter = op_parser;

    return p;
}

EASY_PC_API epc_parser_t *
epc_chainl1(epc_parser_list * list, char const * name, epc_parser_t * item, epc_parser_t * op)
{
    return epc_parser_list_add(list, _epc_chainl1(name, item, op));
}

typedef struct
{
    epc_cpt_node_t * op_node;
    epc_cpt_node_t * item_node;
} op_item_pair_t;

static epc_parse_result_t
pchainr1_parse_fn(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, token_offset);

    if (input_result.is_eof)
    {
        return epc_parser_error_result(ctx, token_offset, "Unexpected end of input", "chainr1", "EOF");
    }

    delimited_data_t * chain_data = &self->data.delimited;
    epc_parser_t * item_parser = chain_data->item;
    epc_parser_t * op_parser = chain_data->delimiter;

    if (item_parser == NULL || op_parser == NULL)
    {
        return epc_parser_error_result(
            ctx, token_offset, "epc_chainr1 received NULL child parser(s)", epc_parser_get_name(self), "NULL"
        );
    }

    size_t current_token_offset = token_offset;
    epc_parse_result_t first_item_result;
    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx); // Declare here

    // Parse the first item (must succeed)
    first_item_result = parse(item_parser, ctx, current_token_offset);
    if (first_item_result.is_error)
    {
        epc_parser_error_free(original_furthest_error); // Cleanup in error path
        return first_item_result;
    }
    current_token_offset += first_item_result.data.success->token.count;

    // Collect (op item) pairs
    op_item_pair_t * pairs = NULL;
    int pair_count = 0;
    int pair_capacity = 4; // Initial capacity
    pairs = calloc(pair_capacity, sizeof(op_item_pair_t));
    if (pairs == NULL)
    {
        epc_parser_result_cleanup(&first_item_result);  // Cleanup the first item's result
        epc_parser_error_free(original_furthest_error); // Cleanup in error path
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    while (1)
    {
        epc_parser_error_t * loop_furthest_error = parser_furthest_error_copy(ctx); // Save for loop iteration
        epc_parse_result_t op_result = parse(op_parser, ctx, current_token_offset);
        if (op_result.is_error)
        {
            epc_parser_result_cleanup(&op_result);
            parser_furthest_error_restore(ctx, &loop_furthest_error); // Restore if op fails
            break;                                                    // No more operators, chain ends
        }
        epc_parser_error_free(loop_furthest_error); // Operator matched, clear previous furthest error
        current_token_offset += op_result.data.success->token.count;

        epc_parse_result_t item_result = parse(item_parser, ctx, current_token_offset);
        if (item_result.is_error)
        {
            epc_parser_result_cleanup(&op_result);
            epc_parser_result_cleanup(&first_item_result);
            for (int i = 0; i < pair_count; ++i)
            {
                epc_node_free(pairs[i].op_node);
                epc_node_free(pairs[i].item_node);
            }
            free(pairs);
            epc_parser_error_free(original_furthest_error); // Cleanup in error path
            return item_result;                             // Item after operator failed, so chain fails
        }
        current_token_offset += item_result.data.success->token.count;

        // Store pair
        if (pair_count == pair_capacity)
        {
            pair_capacity *= 2;
            op_item_pair_t * new_pairs = realloc(pairs, pair_capacity * sizeof(op_item_pair_t));
            if (new_pairs == NULL)
            {
                epc_parser_result_cleanup(&op_result);
                epc_parser_result_cleanup(&item_result);
                epc_parser_result_cleanup(&first_item_result); // The first item's result
                for (int i = 0; i < pair_count; ++i)
                {
                    epc_node_free(pairs[i].op_node);
                    epc_node_free(pairs[i].item_node);
                }
                free(pairs);
                epc_parser_error_free(original_furthest_error); // Cleanup in error path
                return epc_parser_error_result(
                    ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
                );
            }
            pairs = new_pairs;
        }
        pairs[pair_count].op_node = op_result.data.success;
        pairs[pair_count].item_node = item_result.data.success;
        pair_count++;
    }

    // Build the CPT for right-associativity
    epc_cpt_node_t * final_cpt_node = first_item_result.data.success; // Default if no operators

    // If there are collected (op item) pairs, construct the right-associative tree
    if (pair_count > 0)
    {
        // The initial right-hand side of the innermost expression is the rightmost item.
        // Example: 1 ^ 2 ^ 3. The innermost is (2 ^ 3). So '3' is the initial right-hand side for (2 ^ 3).
        // The last item from the 'pairs' is the base for the right-hand side construction.
        epc_cpt_node_t * current_right_operand = pairs[pair_count - 1].item_node;

        // Loop backwards from the second-to-last operator/item pair
        // to form the structure: Left_Operand op Right_Subtree
        for (int i = pair_count - 1; i >= 0; --i)
        {
            epc_cpt_node_t * new_parent_node = epc_node_alloc(ctx, self, self->tag);
            if (new_parent_node == NULL)
            {
                epc_node_free(current_right_operand);
                // Free any op/item nodes from pairs that haven't been adopted yet
                for (int j = 0; j <= i; ++j)
                {
                    epc_node_free(pairs[j].op_node);
                    epc_node_free(pairs[j].item_node);
                }
                epc_node_free(first_item_result.data.success); // The initial item
                free(pairs);
                epc_parser_error_free(original_furthest_error);
                return epc_parser_error_result(
                    ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
                );
            }

            epc_cpt_node_t * left_operand_node;
            if (i == 0)
            {
                // For the outermost operation, the left operand is the very first item matched
                left_operand_node = first_item_result.data.success;
            }
            else
            {
                // For inner operations, the left operand is the item from the previous pair (i-1)
                left_operand_node = pairs[i - 1].item_node;
            }
            epc_cpt_node_t * operator_node = pairs[i].op_node;

            new_parent_node->children = calloc(3, sizeof(*new_parent_node->children));
            if (new_parent_node->children == NULL)
            {
                epc_node_free(current_right_operand);
                epc_node_free(left_operand_node);
                epc_node_free(operator_node);
                // Free any op/item nodes from pairs that haven't been adopted yet
                for (int j = 0; j <= i; ++j)
                {
                    epc_node_free(pairs[j].op_node);
                    epc_node_free(pairs[j].item_node);
                }
                epc_node_free(first_item_result.data.success);
                epc_node_free(new_parent_node);
                free(pairs);
                epc_parser_error_free(original_furthest_error);
                return epc_parser_error_result(
                    ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A"
                );
            }

            new_parent_node->children[0] = left_operand_node;
            new_parent_node->children[1] = operator_node;
            new_parent_node->children[2] = current_right_operand;
            new_parent_node->children_count = 3;

            new_parent_node->token.offset = left_operand_node->token.offset;
            new_parent_node->token.count = current_right_operand->token.offset + current_right_operand->token.count
                                           - left_operand_node->token.offset;
            new_parent_node->semantic_start_offset = left_operand_node->semantic_start_offset;
            new_parent_node->semantic_end_offset = current_right_operand->semantic_end_offset;

            current_right_operand
                = new_parent_node; // This newly formed node becomes the right operand for the next outer iteration
        }
        final_cpt_node = current_right_operand; // The fully built right-associative tree
    }

    free(pairs); // Free the array of op_item_pair_t structs, not the nodes they point to

    // Restore furthest error before returning final success
    parser_furthest_error_restore(ctx, &original_furthest_error);

    return epc_parser_success_result(final_cpt_node);
}

static epc_parser_t *
_epc_chainr1(char const * name, epc_parser_t * item_parser, epc_parser_t * op_parser)
{
    epc_parser_t * p = epc_parser_allocate(name, "chainr1", pchainr1_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_DELIMITED; // Reusing for item/op
    p->data.delimited.item = item_parser;
    p->data.delimited.delimiter = op_parser;

    return p;
}

EASY_PC_API epc_parser_t *
epc_chainr1(epc_parser_list * list, char const * name, epc_parser_t * item, epc_parser_t * op)
{
    return epc_parser_list_add(list, _epc_chainr1(name, item, op));
}

static parser_list_t *
parser_list_duplicate(parser_list_t * src)
{
    parser_list_t * l;
    if (src == NULL)
    {
        return NULL;
    }
    l = calloc(1, sizeof(*l));
    if (l == NULL)
    {
        return NULL;
    }
    l->parsers = calloc(src->count, sizeof(*l->parsers));
    if (l->parsers == NULL)
    {
        free(l);
        return NULL;
    }
    for (int i = 0; i < src->count; i++)
    {
        l->parsers[i] = src->parsers[i];
    }
    l->count = src->count;
    return l;
}

void
epc_parser_duplicate(epc_parser_t * const dst, epc_parser_t const * const src)
{
    dst->parse_fn = src->parse_fn;
    dst->ast_config = src->ast_config;
    string_set(&dst->name, src->name);
    dst->tag = src->tag;

    parser_data_free(&dst->data);
    dst->data.type = src->data.type;
    switch (src->data.type)
    {
    case PARSER_DATA_TYPE_NONE:
    case PARSER_DATA_TYPE_PARSER:
    case PARSER_DATA_TYPE_CHAR_RANGE:
    case PARSER_DATA_TYPE_COUNT:
    case PARSER_DATA_TYPE_BETWEEN:
    case PARSER_DATA_TYPE_DELIMITED:
    case PARSER_DATA_TYPE_LEXEME:
    case PARSER_DATA_TYPE_PREDICATE:
    case PARSER_DATA_TYPE_WRAP:
    case PARSER_DATA_TYPE_MEMOIZE:
        dst->data = src->data;
        break;

    case PARSER_DATA_TYPE_STRING:
        dst->data.string = strdup(src->data.string);
        break;

    case PARSER_DATA_TYPE_BYTE:
        dst->data.byte = src->data.byte;
        dst->data.byte.str = strdup(src->data.byte.str);
        break;

    case PARSER_DATA_TYPE_PARSER_LIST:
        dst->data.parser_list = parser_list_duplicate(src->data.parser_list);
        break;
    }

    if (src->data.type == PARSER_DATA_TYPE_BYTE && src->expected_value == src->data.byte.str)
    {
        dst->expected_value = dst->data.byte.str;
    }
    else if (src->expected_value == src->data.string)
    {
        dst->expected_value = dst->data.string;
    }
    else
    {
        dst->expected_value = src->expected_value;
    }
}

static epc_parse_result_t
psatisfy_parse_fn(epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);
    epc_parse_result_t token_result = parse(self->data.predicate.parser, ctx, token_offset);

    if (token_result.is_error)
    {
        parser_furthest_error_restore(ctx, &original_furthest_error);
        epc_parse_result_t result = epc_parser_error_result(
            ctx,
            token_offset,
            "Failed to match the satisfy token parser",
            token_result.data.error->expected,
            token_result.data.error->found
        );
        epc_parser_result_cleanup(&token_result);

        return result;
    }
    epc_parser_error_free(original_furthest_error);

    if (!self->data.predicate.predicate_fn(token_result.data.success, ctx, self->data.predicate.parser_data))
    {
        char found_str[FOUND_BUFFER_SIZE];
        char const * error_message = token_result.data.success->error_message;

        if (error_message == NULL)
        {
            snprintf(
                found_str,
                sizeof(found_str),
                "token '%.*s'",
                (int)token_result.data.success->token.count,
                epc_cpt_node_get_content(token_result.data.success)
            );
            error_message = found_str;
        }

        epc_parse_result_t result = epc_parser_error_result(
            ctx, token_offset, "Predicate failed", parser_get_expected_str(self), error_message
        );
        /*
         *  Result cleanup must be done after the error result is created in case the error_message refers to the
         *  error message in the result node.
         */
        epc_parser_result_cleanup(&token_result);

        return result;
    }

    epc_cpt_node_t * node = epc_node_alloc(ctx, self, self->tag);

    if (node == NULL)
    {
        epc_parser_result_cleanup(&token_result);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->children = calloc(1, sizeof(*node->children));
    if (node->children == NULL)
    {
        epc_parser_result_cleanup(&token_result);
        epc_node_free(node);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    node->children[0] = token_result.data.success;
    node->children_count = 1;

    node->token.offset = token_result.data.success->token.offset;
    node->token.count = token_result.data.success->token.count;
    node->semantic_start_offset = token_result.data.success->semantic_start_offset;
    node->semantic_end_offset = token_result.data.success->semantic_end_offset;

    return epc_parser_success_result(node);
}

static epc_parser_t *
_epc_satisfy(
    char const * name,
    epc_parser_t * token_parser,
    char const * message_on_failure,
    epc_satisfy_parser_predicate_fn predicate,
    void * parser_data
)
{
    epc_parser_t * p = epc_parser_allocate(name, "satisfy", psatisfy_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_PREDICATE;
    p->data.predicate.parser = token_parser;
    p->data.predicate.predicate_fn = predicate;
    p->data.predicate.parser_data = parser_data;
    p->expected_value = message_on_failure;

    return p;
}

EASY_PC_API epc_parser_t *
epc_satisfy(
    epc_parser_list * list,
    char const * name,
    epc_parser_t * token_parser,
    char const * message,
    epc_satisfy_parser_predicate_fn predicate,
    void * parser_data
)
{
    return epc_parser_list_add(list, _epc_satisfy(name, token_parser, message, predicate, parser_data));
}

static epc_parse_result_t
pwrap_parse_fn(epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    wrap_data_t const * wrap_data = &self->data.wrap;
    epc_parser_t * wrapped_parser = wrap_data->parser;
    epc_wrap_callbacks_t callbacks = wrap_data->callbacks;
    void * parser_data = wrap_data->parser_data;

    if (callbacks.on_entry != NULL)
    {
        callbacks.on_entry(wrapped_parser, ctx, parser_data);
    }
    epc_parser_error_t * original_furthest_error = parser_furthest_error_copy(ctx);
    epc_parse_result_t result = parse(wrapped_parser, ctx, token_offset);

    if (callbacks.on_exit != NULL && !callbacks.on_exit(result, ctx, parser_data))
    {
        // If on_exit returns false, we treat it as a failure of the wrapper parser.
        char found_str[FOUND_BUFFER_SIZE];
        if (!result.is_error) /* The callback wishes to override the child parse success to be an error. */
        {
            snprintf(
                found_str,
                sizeof(found_str),
                "node '%.*s'",
                (int)result.data.success->token.count,
                (char *)epc_cpt_node_get_content(result.data.success)
            );
            parser_furthest_error_restore(ctx, &original_furthest_error);
            epc_parser_result_cleanup(&result);
            return epc_parser_error_result(
                ctx, token_offset, "on_exit callback indicated failure", parser_get_expected_str(self), found_str
            );
        }
    }

    epc_parser_error_free(original_furthest_error);

    if (result.is_error)
    {
        return result;
    }

    // If we get here, the child parser succeeded.
    // We now create a new parent node to represent the 'wrap' parser itself.
    epc_cpt_node_t * wrap_node = epc_node_alloc(ctx, self, self->tag);
    if (wrap_node == NULL)
    {
        epc_parser_result_cleanup(&result);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    // The child of the wrap_node is the success node from the wrapped parser.
    wrap_node->children = calloc(1, sizeof(*wrap_node->children));
    if (wrap_node->children == NULL)
    {
        epc_parser_result_cleanup(&result);
        epc_node_free(wrap_node);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }
    wrap_node->children[0] = result.data.success;
    wrap_node->children_count = 1;

    // The wrap_node spans the same content as its child.
    wrap_node->token.offset = result.data.success->token.offset;
    wrap_node->token.count = result.data.success->token.count;
    wrap_node->semantic_start_offset = result.data.success->semantic_start_offset;
    wrap_node->semantic_end_offset = result.data.success->semantic_end_offset;

    return epc_parser_success_result(wrap_node);
}

static epc_parser_t *
_epc_wrap(char const * name, epc_parser_t * wrapped_parser, epc_wrap_callbacks_t callbacks, void * parser_data)
{
    epc_parser_t * p = epc_parser_allocate(name, "wrap", pwrap_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_WRAP;
    p->data.wrap.parser = wrapped_parser;
    p->data.wrap.callbacks = callbacks;
    p->data.wrap.parser_data = parser_data;

    return p;
}

EASY_PC_API epc_parser_t *
epc_wrap(
    epc_parser_list * list,
    char const * name,
    epc_parser_t * wrapped_parser,
    epc_wrap_callbacks_t callbacks,
    void * parser_data
)
{
    return epc_parser_list_add(list, _epc_wrap(name, wrapped_parser, callbacks, parser_data));
}

static epc_parse_result_t
pmemoize_parse_fn(epc_parser_t * self, epc_parser_ctx_t * ctx, size_t token_offset)
{
    epc_parser_t * wrapped_parser = self->data.parser;

    /* 1. Look up in memo table */
    epc_parse_result_t * cached_result = epc_memo_table_get(ctx, self, token_offset);
    if (cached_result != NULL)
    {
        return epc_parse_result_copy(ctx, *cached_result);
    }

    /* 2. Not found, parse it */
    epc_parse_result_t child_result = parse(wrapped_parser, ctx, token_offset);

    if (child_result.is_error)
    {
        // Cache and return the error result as is.
        epc_memo_table_set(ctx, self, token_offset, child_result);
        return child_result;
    }

    // On success, create a new parent node for the memoize parser.
    epc_cpt_node_t * memoize_node = epc_node_alloc(ctx, self, self->tag);
    if (memoize_node == NULL)
    {
        epc_parser_result_cleanup(&child_result);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }

    // The child of the memoize_node is the success node from the wrapped parser.
    memoize_node->children = calloc(1, sizeof(*memoize_node->children));
    if (memoize_node->children == NULL)
    {
        epc_parser_result_cleanup(&child_result);
        epc_node_free(memoize_node);
        return epc_parser_error_result(ctx, token_offset, memory_allocation_error, epc_parser_get_name(self), "N/A");
    }
    memoize_node->children[0] = child_result.data.success;
    memoize_node->children_count = 1;

    // The memoize_node spans the same content as its child.
    memoize_node->token.offset = child_result.data.success->token.offset;
    memoize_node->token.count = child_result.data.success->token.count;
    memoize_node->semantic_start_offset = child_result.data.success->semantic_start_offset;
    memoize_node->semantic_end_offset = child_result.data.success->semantic_end_offset;

    epc_parse_result_t parent_result = epc_parser_success_result(memoize_node);

    /* 3. Store the new parent result in the memo table */
    epc_memo_table_set(ctx, self, token_offset, parent_result);

    return parent_result;
}

static epc_parser_t *
_epc_memoize(char const * name, epc_parser_t * p_to_memoize)
{
    epc_parser_t * p = epc_parser_allocate(name, "memoize", pmemoize_parse_fn);
    if (p == NULL)
    {
        return NULL;
    }
    p->data.type = PARSER_DATA_TYPE_MEMOIZE;
    p->data.parser = p_to_memoize;

    return p;
}

EASY_PC_API epc_parser_t *
epc_memoize(epc_parser_list * list, char const * name, epc_parser_t * p_to_memoize)
{
    return epc_parser_list_add(list, _epc_memoize(name, p_to_memoize));
}

void
epc_parser_set_ast_action(epc_parser_t * p, int action_type)
{
    if (p == NULL)
    {
        return;
    }
    p->ast_config.action = action_type;
    p->ast_config.assigned = true;
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
