#include "gdl_lsp_semantic_tokens.h"

#include "gdl_ast.h"
#include "gdl_compiler_ast_actions.h"
#include "gdl_tokenizer_actions.h"
#include "gdl_tokenizer_parser.h"
#include "rpc.h"

#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LSP_TYPE_KEYWORD   0
#define LSP_TYPE_STRING    1
#define LSP_TYPE_NUMBER    2
#define LSP_TYPE_VARIABLE  3
#define LSP_TYPE_OPERATOR  4
#define LSP_TYPE_PARAMETER 5
#define LSP_TYPE_FUNCTION  6
#define LSP_TYPE_DECORATOR 7

static int
token_id_to_lsp_type(epc_token_id_t id)
{
    if (id >= TOKEN_KW_CHAR && id <= TOKEN_KW_BASH_COMMENT)
    {
        return LSP_TYPE_KEYWORD;
    }
    if (id >= TOKEN_KW_STRING && id <= TOKEN_KW_ALL_COMMENTS)
    {
        return LSP_TYPE_KEYWORD;
    }

    switch (id)
    {
    case TOKEN_STRING_LITERAL:
    case TOKEN_CHAR_LITERAL:
    case TOKEN_RAW_CHAR_LITERAL:
    case TOKEN_TOKEN_LITERAL:
        return LSP_TYPE_STRING;

    case TOKEN_NUMBER:
        return LSP_TYPE_NUMBER;

    case TOKEN_IDENTIFIER:
        return LSP_TYPE_VARIABLE;

    case TOKEN_EQUALS:
    case TOKEN_PIPE:
    case TOKEN_SEMICOLON:
    case TOKEN_AT:
    case TOKEN_COMMA:
    case TOKEN_MINUS:
    case TOKEN_STAR:
    case TOKEN_PLUS:
    case TOKEN_QUESTION:
    case TOKEN_LPAREN:
    case TOKEN_RPAREN:
    case TOKEN_LBRACKET:
    case TOKEN_RBRACKET:
        return LSP_TYPE_OPERATOR;

    default:
        return LSP_TYPE_OPERATOR;
    }
}

static gdl_document_cache_t *
cache_lookup(gdl_lsp_server_st * svr, char const * uri)
{
    for (int i = 0; i < svr->cache_count; i++)
    {
        if (strcmp(svr->caches[i].uri, uri) == 0)
        {
            return &svr->caches[i];
        }
    }
    return NULL;
}

static gdl_document_cache_t *
cache_add(gdl_lsp_server_st * svr, char const * uri)
{
    if (svr->cache_count == svr->cache_capacity)
    {
        svr->cache_capacity = svr->cache_capacity == 0 ? 4 : svr->cache_capacity * 2;
        svr->caches = realloc(svr->caches, (size_t)svr->cache_capacity * sizeof(gdl_document_cache_t));
    }

    gdl_document_cache_t * cache = &svr->caches[svr->cache_count];
    memset(cache, 0, sizeof(*cache));
    strncpy(cache->uri, uri, sizeof(cache->uri) - 1);
    cache->uri[sizeof(cache->uri) - 1] = '\0';
    svr->cache_count++;
    return cache;
}

void
gdl_clear_document_cache(gdl_lsp_server_st * svr, char const * uri)
{
    for (int i = 0; i < svr->cache_count; i++)
    {
        if (strcmp(svr->caches[i].uri, uri) == 0)
        {
            free(svr->caches[i].tokens);
            free(svr->caches[i].ast_token_types);
            svr->caches[i] = svr->caches[svr->cache_count - 1];
            svr->cache_count--;
            return;
        }
    }
}

bool
gdl_tokenize_document(gdl_lsp_server_st * svr, char const * uri, char const * text)
{
    gdl_clear_document_cache(svr, uri);

    gdl_tokenizer_ctx_t tokenizer_ctx;
    tokenizer_ctx.tokens = epc_token_list_create(256);
    tokenizer_ctx.last_match = NULL;

    epc_ast_hook_registry_t * reg = epc_ast_hook_registry_create(TOKENIZER_ACTION_COUNT);
    gdl_tokenizer_hook_registry_init(reg, &tokenizer_ctx);

    epc_parse_session_t session = epc_parse_str(svr->tokenizer_parser, text, NULL);

    if (session.result.is_error)
    {
        fprintf(stderr, "[LSP] tokenize %s: parse failed\n", uri);
        epc_parse_session_destroy(&session);
        epc_ast_hook_registry_free(reg);
        epc_token_list_free(tokenizer_ctx.tokens);
        return false;
    }

    epc_ast_result_t token_build = epc_ast_build(session.result.data.success, reg, &tokenizer_ctx);

    if (token_build.has_error)
    {
        fprintf(stderr, "[LSP] tokenize %s: AST build failed: %s\n", uri, token_build.error_message);
        epc_parse_session_destroy(&session);
        epc_ast_hook_registry_free(reg);
        epc_token_list_free(tokenizer_ctx.tokens);
        return false;
    }

    size_t const token_count = epc_token_list_count(tokenizer_ctx.tokens);
    fprintf(stderr, "[LSP] tokenized %s: %zu tokens\n", uri, token_count);

    gdl_document_cache_t * cache = cache_add(svr, uri);
    if (token_count > 0)
    {
        cache->tokens = malloc(token_count * sizeof(gdl_token_entry_t));
        cache->token_count = (int)token_count;

        for (size_t i = 0; i < token_count; i++)
        {
            epc_token_id_t id;
            epc_parser_input_view_t view;
            epc_token_list_get(tokenizer_ctx.tokens, i, &id, &view);

            cache->tokens[i].id = id;
            cache->tokens[i].offset = view.offset;
            cache->tokens[i].line = view.line_number > 0 ? view.line_number - 1 : 0;
            cache->tokens[i].column = view.column_number > 0 ? view.column_number - 1 : 0;
            cache->tokens[i].length = view.len;
            cache->tokens[i].lsp_type = token_id_to_lsp_type(id);
        }
    }

    // Heuristic pass: refine types based on token stream patterns.
    // IDENTIFIER followed by '='  → rule definition (function)
    // '@' followed by IDENTIFIER → semantic action (decorator)
    cache->ast_token_types = calloc((size_t)cache->token_count, sizeof(int));
    for (int i = 0; i < cache->token_count; i++)
    {
        cache->ast_token_types[i] = cache->tokens[i].lsp_type;
    }
    cache->ast_available = true;

    for (int i = 0; i < cache->token_count - 1; i++)
    {
        if (cache->tokens[i].id == TOKEN_IDENTIFIER && cache->tokens[i + 1].id == TOKEN_EQUALS)
        {
            cache->ast_token_types[i] = LSP_TYPE_FUNCTION;
        }
        if (cache->tokens[i].id == TOKEN_AT && cache->tokens[i + 1].id == TOKEN_IDENTIFIER)
        {
            cache->ast_token_types[i]     = LSP_TYPE_DECORATOR;
            cache->ast_token_types[i + 1] = LSP_TYPE_DECORATOR;
        }
    }

    // Level 2: attempt full parse + AST build (validation only, doesn't affect coloring)
    epc_ast_hook_registry_t * grammar_reg = epc_ast_hook_registry_create(GDL_AST_ACTION_MAX);
    gdl_ast_hook_registry_init(grammar_reg, NULL);

    bool reparse_ok = epc_parse_session_reparse(&session, svr->grammar_parser, tokenizer_ctx.tokens);

    if (reparse_ok && !session.result.is_error)
    {
        fprintf(stderr, "[LSP] full parse %s: success\n", uri);
    }
    else
    {
        fprintf(stderr, "[LSP] full parse %s: parse failed\n", uri);
    }

    epc_ast_hook_registry_free(grammar_reg);
    epc_parse_session_destroy(&session);
    epc_ast_hook_registry_free(reg);
    epc_token_list_free(tokenizer_ctx.tokens);

    return true;
}

void
gdl_encode_semantic_tokens(gdl_lsp_server_st * svr, char const * uri, struct json_object * id)
{
    gdl_document_cache_t * cache = cache_lookup(svr, uri);

    if (!cache)
    {
        rpc_send_error(&svr->base, id, -32603, "No cached tokens for document");
        return;
    }

    struct json_object * result = json_object_new_object();
    struct json_object * data_arr = json_object_new_array();

    size_t prev_line = 0;
    size_t prev_col = 0;

    for (int i = 0; i < cache->token_count; i++)
    {
        gdl_token_entry_t * tok = &cache->tokens[i];
        int lsp_type = cache->ast_available && cache->ast_token_types
                           ? cache->ast_token_types[i]
                           : tok->lsp_type;

        size_t delta_line = tok->line - prev_line;
        size_t delta_col;
        if (delta_line == 0)
        {
            delta_col = tok->column - prev_col;
        }
        else
        {
            delta_col = tok->column;
        }

        json_object_array_add(data_arr, json_object_new_int64((int64_t)delta_line));
        json_object_array_add(data_arr, json_object_new_int64((int64_t)delta_col));
        json_object_array_add(data_arr, json_object_new_int64((int64_t)tok->length));
        json_object_array_add(data_arr, json_object_new_int(lsp_type));
        json_object_array_add(data_arr, json_object_new_int(0));

        prev_line = tok->line;
        prev_col = tok->column;
    }

    json_object_object_add(result, "data", data_arr);
    rpc_send_response(&svr->base, id, result);
}
