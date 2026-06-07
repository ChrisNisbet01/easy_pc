#include "gdl_tokenizer_actions.h"

#include "gdl_token_ids.h"

#include <cpt_node.h>
#include <stdio.h>
#include <string.h>

typedef struct
{
    char const * name;
    epc_token_id_t id;
} keyword_entry_t;

static keyword_entry_t const keywords[] = {
    // Terminal parsers
    {.name = "char", .id = TOKEN_KW_CHAR},
    {.name = "digit", .id = TOKEN_KW_DIGIT},
    {.name = "alphanum", .id = TOKEN_KW_ALPHANUM},
    {.name = "alpha", .id = TOKEN_KW_ALPHA},
    {.name = "identifier", .id = TOKEN_KW_IDENTIFIER},
    {.name = "int", .id = TOKEN_KW_INT},
    {.name = "octal", .id = TOKEN_KW_OCTAL},
    {.name = "hex", .id = TOKEN_KW_HEX},
    {.name = "double", .id = TOKEN_KW_DOUBLE},
    {.name = "long_double", .id = TOKEN_KW_LONG_DOUBLE},
    {.name = "space", .id = TOKEN_KW_SPACE},
    {.name = "any", .id = TOKEN_KW_ANY},
    {.name = "succeed", .id = TOKEN_KW_SUCCEED},
    {.name = "hex_digit", .id = TOKEN_KW_HEX_DIGIT},
    {.name = "soi", .id = TOKEN_KW_SOI},
    {.name = "eoi", .id = TOKEN_KW_EOI},
    {.name = "fail", .id = TOKEN_KW_FAIL},
    {.name = "cpp_comment", .id = TOKEN_KW_CPP_COMMENT},
    {.name = "c_comment", .id = TOKEN_KW_C_COMMENT},
    {.name = "bash_comment", .id = TOKEN_KW_BASH_COMMENT},

    // Combinator parsers
    {.name = "string", .id = TOKEN_KW_STRING},
    {.name = "char_range", .id = TOKEN_KW_CHAR_RANGE},
    {.name = "noneof", .id = TOKEN_KW_NONEOF},
    {.name = "many", .id = TOKEN_KW_MANY},
    {.name = "count", .id = TOKEN_KW_COUNT},
    {.name = "count_range", .id = TOKEN_KW_COUNT_RANGE},
    {.name = "between", .id = TOKEN_KW_BETWEEN},
    {.name = "delimited", .id = TOKEN_KW_DELIMITED},
    {.name = "delimited_flex", .id = TOKEN_KW_DELIMITED_FLEX},
    {.name = "optional", .id = TOKEN_KW_OPTIONAL},
    {.name = "lookahead", .id = TOKEN_KW_LOOKAHEAD},
    {.name = "not", .id = TOKEN_KW_NOT},
    {.name = "oneof", .id = TOKEN_KW_ONEOF},
    {.name = "lexeme", .id = TOKEN_KW_LEXEME},
    {.name = "strip", .id = TOKEN_KW_STRIP},
    {.name = "stripl", .id = TOKEN_KW_STRIPL},
    {.name = "stripr", .id = TOKEN_KW_STRIPR},
    {.name = "chainl1", .id = TOKEN_KW_CHAINL1},
    {.name = "chainr1", .id = TOKEN_KW_CHAINR1},
    {.name = "skip", .id = TOKEN_KW_SKIP},
    {.name = "memoize", .id = TOKEN_KW_MEMOIZE},
    {.name = "satisfy", .id = TOKEN_KW_SATISFY},
    {.name = "wrap", .id = TOKEN_KW_WRAP},
    {.name = "commit", .id = TOKEN_KW_COMMIT},

    // Lexeme flags
    {.name = "ws", .id = TOKEN_KW_WS},
    {.name = "all", .id = TOKEN_KW_ALL},
    {.name = "all_styles", .id = TOKEN_KW_ALL_STYLES},
    {.name = "all_comments", .id = TOKEN_KW_ALL_COMMENTS},
};

static epc_token_id_t
keyword_id_for_name(char const * name, size_t len)
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (strlen(keywords[i].name) == len && strncmp(keywords[i].name, name, len) == 0)
            return keywords[i].id;
    }
    return TOKEN_IDENTIFIER;
}

static void
handle_keyword_or_identifier(
    epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data
)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    char const * content = epc_cpt_node_get_content(node);
    epc_token_id_t id = keyword_id_for_name(content, view.len);

    epc_token_list_add(tctx->tokens, id, view);
    tctx->last_match = node;
}

static void
handle_simple_token(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    char const * s = epc_cpt_node_get_content(node);

    epc_token_id_t id = TOKEN_IDENTIFIER;
    if (s && view.len > 0)
    {
        switch (s[0])
        {
        case '=':
            id = TOKEN_EQUALS;
            break;
        case ';':
            id = TOKEN_SEMICOLON;
            break;
        case '|':
            id = TOKEN_PIPE;
            break;
        case '@':
            id = TOKEN_AT;
            break;
        case '(':
            id = TOKEN_LPAREN;
            break;
        case ')':
            id = TOKEN_RPAREN;
            break;
        case ',':
            id = TOKEN_COMMA;
            break;
        case '-':
            id = TOKEN_MINUS;
            break;
        case '*':
            id = TOKEN_STAR;
            break;
        case '+':
            id = TOKEN_PLUS;
            break;
        case '?':
            id = TOKEN_QUESTION;
            break;
        case '^':
            id = TOKEN_CARET;
            break;
        case '[':
            id = TOKEN_LBRACKET;
            break;
        case ']':
            id = TOKEN_RBRACKET;
            break;
        }
    }

    epc_token_list_add(tctx->tokens, id, view);
    tctx->last_match = node;
}

static void
handle_string_literal(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    epc_token_list_add(tctx->tokens, TOKEN_STRING_LITERAL, view);
    tctx->last_match = node;
}

static void
handle_char_literal(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    epc_token_list_add(tctx->tokens, TOKEN_CHAR_LITERAL, view);
    tctx->last_match = node;
}

static void
handle_raw_char_literal(
    epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data
)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    epc_token_list_add(tctx->tokens, TOKEN_RAW_CHAR_LITERAL, view);
    tctx->last_match = node;
}

static void
handle_number(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    epc_token_list_add(tctx->tokens, TOKEN_NUMBER, view);
    tctx->last_match = node;
}

static void
handle_token_literal(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)ctx;
    (void)children;
    (void)count;
    gdl_tokenizer_ctx_t * tctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    epc_token_list_add(tctx->tokens, TOKEN_TOKEN_LITERAL, view);
    tctx->last_match = node;
}

void
gdl_tokenizer_hook_registry_init(epc_ast_hook_registry_t * registry, gdl_tokenizer_ctx_t * ctx)
{
    (void)ctx;
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_KEYWORD, handle_keyword_or_identifier);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_IDENTIFIER, handle_keyword_or_identifier);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_STRING_LITERAL, handle_string_literal);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_CHAR_LITERAL, handle_char_literal);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_RAW_CHAR_LITERAL, handle_raw_char_literal);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_NUMBER, handle_number);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_TOKEN_LITERAL, handle_token_literal);
    epc_ast_hook_registry_set_action(registry, TOKENIZER_ACTION_STRUCTURAL, handle_simple_token);
}
