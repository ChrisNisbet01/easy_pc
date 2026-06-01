#include "gdl_lsp_handlers.h"

#include "documents.h"
#include "gdl_lsp_semantic_tokens.h"
#include "gdl_lsp_server.h"
#include "utils.h"

#include <json-c/json.h>
#include <rpc2/rpc2.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Lifecycle handlers --- */

static bool
handle_initialize(struct rpc_request * req)
{
    struct json_object * result = json_object_new_object();
    struct json_object * capabilities = json_object_new_object();

    json_object_object_add(capabilities, "textDocumentSync", json_object_new_int(1));

    struct json_object * semantic_tokens = json_object_new_object();
    struct json_object * legend = json_object_new_object();
    struct json_object * token_types = json_object_new_array();

    json_object_array_add(token_types, json_object_new_string("keyword"));
    json_object_array_add(token_types, json_object_new_string("string"));
    json_object_array_add(token_types, json_object_new_string("number"));
    json_object_array_add(token_types, json_object_new_string("variable"));
    json_object_array_add(token_types, json_object_new_string("operator"));
    json_object_array_add(token_types, json_object_new_string("parameter"));
    json_object_array_add(token_types, json_object_new_string("function"));
    json_object_array_add(token_types, json_object_new_string("decorator"));

    json_object_object_add(legend, "tokenTypes", token_types);
    json_object_object_add(legend, "tokenModifiers", json_object_new_array());
    json_object_object_add(semantic_tokens, "legend", legend);
    json_object_object_add(semantic_tokens, "full", json_object_new_boolean(true));
    json_object_object_add(capabilities, "semanticTokensProvider", semantic_tokens);

    json_object_object_add(result, "capabilities", capabilities);

    rpc_ok(req, result);

    return true;
}

static bool
handle_initialized(struct rpc_request * req)
{
    UNUSED_PARAM(req);
    return true;
}

static bool
handle_shutdown(struct rpc_request * req)
{
    rpc_ok(req, NULL);
    return true;
}

static bool
handle_exit(struct rpc_request * req)
{
    rpc_ctx * ctx = rpc_request_ctx(req);

    rpc_ctx_close_stdin(ctx);
    return true;
}

/* --- Debounce callback --- */

static void
debounce_cb(rpc_timer * t, void * user_data)
{
    UNUSED_PARAM(t);
    gdl_lsp_server_st * svr = user_data;

    if (!svr->pending_uri)
    {
        return;
    }

    char * uri = svr->pending_uri;
    svr->pending_uri = NULL;

    fprintf(stderr, "[LSP] debounce fired for %s\n", uri);

    document_st * doc = documents_lookup(svr->documents, uri);
    if (doc != NULL)
    {
        gdl_tokenize_document(svr, uri, doc->text);
    }

    free(uri);
}

/* --- Document sync handlers --- */

static bool
handle_text_document_did_open(struct rpc_request * req)
{
    struct json_object * params = rpc_params(req);
    struct json_object * text_document = NULL;

    if (!json_object_object_get_ex(params, "textDocument", &text_document))
    {
        return false;
    }

    struct json_object * uri_obj = NULL;
    struct json_object * text_obj = NULL;

    if (!json_object_object_get_ex(text_document, "uri", &uri_obj)
        || !json_object_object_get_ex(text_document, "text", &text_obj))
    {
        return false;
    }

    gdl_lsp_server_st * svr = rpc_handler_data(req);

    documents_update(svr->documents, json_object_get_string(uri_obj), json_object_get_string(text_obj));

    free(svr->pending_uri);
    svr->pending_uri = strdup(json_object_get_string(uri_obj));
    rpc_timer_start(svr->ctx, &svr->debounce_timer, 100);

    return true;
}

static bool
handle_text_document_did_change(struct rpc_request * req)
{
    struct json_object * params = rpc_params(req);
    struct json_object * text_document = NULL;

    if (!json_object_object_get_ex(params, "textDocument", &text_document))
    {
        return false;
    }

    struct json_object * uri_obj = NULL;

    if (!json_object_object_get_ex(text_document, "uri", &uri_obj))
    {
        return false;
    }

    struct json_object * content_changes = NULL;

    if (!json_object_object_get_ex(params, "contentChanges", &content_changes)
        || json_object_array_length(content_changes) < 1)
    {
        return false;
    }

    struct json_object * change = json_object_array_get_idx(content_changes, 0);
    struct json_object * text_obj = NULL;

    if (!json_object_object_get_ex(change, "text", &text_obj))
    {
        return false;
    }

    gdl_lsp_server_st * svr = rpc_handler_data(req);
    char const * uri_str = json_object_get_string(uri_obj);

    documents_update(svr->documents, uri_str, json_object_get_string(text_obj));

    fprintf(stderr, "[LSP] didChange: uri=%s, debounce timer set\n", uri_str);

    free(svr->pending_uri);
    svr->pending_uri = strdup(uri_str);
    rpc_timer_start(svr->ctx, &svr->debounce_timer, 100);

    return true;
}

static bool
handle_text_document_did_close(struct rpc_request * req)
{
    struct json_object * params = rpc_params(req);
    struct json_object * text_document = NULL;

    if (!json_object_object_get_ex(params, "textDocument", &text_document))
    {
        return false;
    }

    struct json_object * uri_obj = NULL;

    if (!json_object_object_get_ex(text_document, "uri", &uri_obj))
    {
        return false;
    }

    gdl_lsp_server_st * svr = rpc_handler_data(req);
    char const * uri_str = json_object_get_string(uri_obj);

    documents_remove(svr->documents, uri_str);
    gdl_clear_document_cache(svr, uri_str);

    return true;
}

static bool
handle_semantic_tokens_full(struct rpc_request * req)
{
    struct json_object * params = rpc_params(req);
    struct json_object * text_document = NULL;

    if (!json_object_object_get_ex(params, "textDocument", &text_document))
    {
        return false;
    }

    struct json_object * uri_obj = NULL;

    if (!json_object_object_get_ex(text_document, "uri", &uri_obj))
    {
        return false;
    }

    gdl_lsp_server_st * svr = rpc_handler_data(req);
    char const * uri_str = json_object_get_string(uri_obj);

    fprintf(stderr, "[LSP] semanticTokens: uri=%s\n", uri_str);

    if (svr->pending_uri && strcmp(svr->pending_uri, uri_str) == 0)
    {
        rpc_timer_cancel(&svr->debounce_timer);
        char * pending = svr->pending_uri;
        svr->pending_uri = NULL;

        document_st * doc = documents_lookup(svr->documents, pending);
        if (doc)
        {
            gdl_tokenize_document(svr, pending, doc->text);
        }
        free(pending);
    }

    struct json_object * result = gdl_encode_semantic_tokens(svr, uri_str);

    if (result)
    {
        rpc_ok(req, result);
    }
    else
    {
        rpc_err(req, -32603, "No cached tokens for document");
    }

    return true;
}

/* --- Registration --- */

void
gdl_lsp_register_handlers(gdl_lsp_server_st * svr)
{
    rpc_timer_init(&svr->debounce_timer, debounce_cb, svr);

    rpc_add_handler(svr->ctx, "initialize", handle_initialize, 0, NULL, svr);
    rpc_add_handler(svr->ctx, "initialized", handle_initialized, 0, NULL, NULL);
    rpc_add_handler(svr->ctx, "shutdown", handle_shutdown, 0, NULL, NULL);
    rpc_add_handler(svr->ctx, "exit", handle_exit, 0, NULL, NULL);
    rpc_add_handler(svr->ctx, "textDocument/didOpen", handle_text_document_did_open, 0, NULL, svr);
    rpc_add_handler(svr->ctx, "textDocument/didChange", handle_text_document_did_change, 0, NULL, svr);
    rpc_add_handler(svr->ctx, "textDocument/didClose", handle_text_document_did_close, 0, NULL, svr);
    rpc_add_handler(svr->ctx, "textDocument/semanticTokens/full", handle_semantic_tokens_full, 0, NULL, svr);
}
