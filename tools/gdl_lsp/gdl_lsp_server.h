#pragma once

#include "documents.h"

#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h>
#include <rpc2/rpc2.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

#include "gdl_token_ids.h"
#include "gdl_tokenizer_actions.h"

typedef struct
{
    epc_token_id_t id;
    size_t offset;
    size_t line;
    size_t column;
    size_t length;
    int lsp_type;
} gdl_token_entry_t;

typedef struct
{
    char uri[4096];
    gdl_token_entry_t * tokens;
    int token_count;
    bool ast_available;
    int * ast_token_types;
} gdl_document_cache_t;

typedef struct gdl_lsp_server_st
{
    rpc_ctx * ctx;
    documents_ctx_st * documents;

    epc_parser_list * parser_list;
    epc_parser_t * tokenizer_parser;
    epc_parser_t * grammar_parser;
    epc_ast_hook_registry_t * tokenizer_registry;
    epc_ast_hook_registry_t * ast_registry;

    gdl_document_cache_t * caches;
    int cache_count;
    int cache_capacity;

    rpc_timer debounce_timer;
    char * pending_uri;
} gdl_lsp_server_st;

void run_gdl_lsp_server(gdl_lsp_server_st * svr, int in_fd, int out_fd);
