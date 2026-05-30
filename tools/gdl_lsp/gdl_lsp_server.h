#pragma once

#include "framing.h"
#include "rpc.h"

#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h>
#include <json-c/json.h>
#include <libubox/list.h>
#include <libubox/uloop.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

typedef void (*transport_msg_cb)(char const * body, size_t len, void * user_data);

typedef struct rpc_method_st
{
    char * name;
    rpc_handler_fn handler;
} rpc_method_st;

typedef struct rpc_method_registry_st
{
    rpc_method_st * methods;
    size_t count;
    size_t capacity;
} rpc_method_registry_st;

struct rpc_server_st
{
    int in_fd;
    int out_fd;

    struct uloop_fd stdin_fd;
    struct uloop_fd out_uloop_fd;
    struct list_head write_queue;

    char * buf;
    size_t buf_len;
    size_t buf_cap;

    framing_st * framing;

    transport_msg_cb on_transport_msg;
    void * on_transport_msg_data;

    bool eof_reached;

    rpc_method_registry_st registry;

    bool shutdown_requested;
    int exit_code;
};

#include "gdl_token_ids.h"
#include "gdl_tokenizer_actions.h"

typedef struct {
    epc_token_id_t id;
    size_t offset;
    size_t line;
    size_t column;
    size_t length;
    int lsp_type;
} gdl_token_entry_t;

typedef struct {
    char uri[4096];
    gdl_token_entry_t * tokens;
    int token_count;
    bool ast_available;
    int * ast_token_types;
} gdl_document_cache_t;

typedef struct {
    rpc_server_st base;

    epc_parser_list * parser_list;
    epc_parser_t * tokenizer_parser;
    epc_parser_t * grammar_parser;
    epc_ast_hook_registry_t * tokenizer_registry;
    epc_ast_hook_registry_t * ast_registry;

    gdl_document_cache_t * caches;
    int cache_count;
    int cache_capacity;

    struct uloop_timeout debounce_timer;
    char * pending_uri;
} gdl_lsp_server_st;

void run_gdl_lsp_server(gdl_lsp_server_st * svr, int in_fd, int out_fd);
