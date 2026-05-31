#include "gdl_lsp_handlers.h"

#include "documents.h"
#include "gdl_lsp_semantic_tokens.h"
#include "rpc.h"
#include "transport.h"
#include "utils.h"

#include <json-c/json.h>
#include <libubox/uloop.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* --- Lifecycle handlers --- */

static bool
handle_initialize(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)params;

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

    rpc_send_response(base, id, result);

    return true;
}

static bool
handle_initialized(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)base;
    (void)params;
    (void)id;

    return true;
}

static bool
handle_shutdown(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)params;
    base->shutdown_requested = true;
    rpc_send_response(base, id, NULL);

    return true;
}

static bool
handle_exit(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)params;
    (void)id;

    transport_close_stdin(base);

    if (transport_can_exit(base))
    {
        uloop_end();
    }

    return true;
}

/* --- Debounce callback --- */

static void
debounce_cb(struct uloop_timeout * t)
{
    gdl_lsp_server_st * svr = container_of(t, gdl_lsp_server_st, debounce_timer);

    if (!svr->pending_uri)
    {
        return;
    }

    char * uri = svr->pending_uri;
    svr->pending_uri = NULL;

    fprintf(stderr, "[LSP] debounce fired for %s\n", uri);

    document_st * doc = documents_lookup(svr->base.documents, uri);
    if (doc != NULL)
    {
        gdl_tokenize_document(svr, uri, doc->text);
    }

    free(uri);
}

/* --- Document sync handlers --- */

static bool
handle_text_document_did_open(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)id;
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

    documents_update(base->documents, json_object_get_string(uri_obj), json_object_get_string(text_obj));

    gdl_lsp_server_st * svr = (gdl_lsp_server_st *)base;
    free(svr->pending_uri);
    svr->pending_uri = strdup(json_object_get_string(uri_obj));
    uloop_timeout_set(&svr->debounce_timer, 100);

    return true;
}

static bool
handle_text_document_did_change(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)id;
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

    char const * uri_str = json_object_get_string(uri_obj);
    documents_update(base->documents, uri_str, json_object_get_string(text_obj));

    gdl_lsp_server_st * svr = (gdl_lsp_server_st *)base;
    fprintf(stderr, "[LSP] didChange: uri=%s, debounce timer set\n", uri_str);

    free(svr->pending_uri);
    svr->pending_uri = strdup(uri_str);
    uloop_timeout_set(&svr->debounce_timer, 100);

    return true;
}

static bool
handle_text_document_did_close(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
    (void)id;
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

    char const * uri_str = json_object_get_string(uri_obj);
    documents_remove(base->documents, uri_str);
    gdl_lsp_server_st * svr = (gdl_lsp_server_st *)base;
    gdl_clear_document_cache(svr, uri_str);

    return true;
}

static bool
handle_semantic_tokens_full(rpc_server_st * base, struct json_object * params, struct json_object * id)
{
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

    char const * uri_str = json_object_get_string(uri_obj);
    gdl_lsp_server_st * svr = (gdl_lsp_server_st *)base;

    fprintf(stderr, "[LSP] semanticTokens: uri=%s\n", uri_str);

    if (svr->pending_uri && strcmp(svr->pending_uri, uri_str) == 0)
    {
        uloop_timeout_cancel(&svr->debounce_timer);
        char * pending = svr->pending_uri;
        svr->pending_uri = NULL;

        document_st * doc = documents_lookup(base->documents, pending);
        if (doc)
        {
            gdl_tokenize_document(svr, pending, doc->text);
        }
        free(pending);
    }

    gdl_encode_semantic_tokens(svr, uri_str, id);

    return true;
}

/* --- Registration --- */

void
gdl_lsp_register_handlers(gdl_lsp_server_st * svr)
{
    rpc_server_st * base = &svr->base;

    svr->debounce_timer.cb = debounce_cb;

    rpc_register_method(base, "initialize", handle_initialize);
    rpc_register_method(base, "initialized", handle_initialized);
    rpc_register_method(base, "shutdown", handle_shutdown);
    rpc_register_method(base, "exit", handle_exit);
    rpc_register_method(base, "textDocument/didOpen", handle_text_document_did_open);
    rpc_register_method(base, "textDocument/didChange", handle_text_document_did_change);
    rpc_register_method(base, "textDocument/didClose", handle_text_document_did_close);
    rpc_register_method(base, "textDocument/semanticTokens/full", handle_semantic_tokens_full);
}
