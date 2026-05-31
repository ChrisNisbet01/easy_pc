#include "gdl_lsp_server.h"

#include "gdl_ast.h"
#include "gdl_compiler_ast_actions.h"
#include "gdl_lsp_handlers.h"
#include "gdl_parser.h"
#include "gdl_tokenizer_parser.h"
#include "rpc.h"
#include "transport.h"
#include "utils.h"

#include <libubox/uloop.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
run_gdl_lsp_server(gdl_lsp_server_st * const svr, int const in_fd, int const out_fd)
{
    fprintf(stderr, "[LSP] Server starting on in_fd=%d, out_fd=%d\n", in_fd, out_fd);
    svr->base.out_fd = out_fd;
    svr->base.in_fd = in_fd;

    svr->base.framing = framing_content_length_create();

    svr->base.documents = documents_init();

    uloop_init();

    svr->base.on_transport_msg = rpc_on_transport_msg;
    svr->base.on_transport_msg_data = &svr->base;

    transport_init(&svr->base);

    svr->parser_list = epc_parser_list_create();

    svr->tokenizer_parser = create_gdl_tokenizer_parser(svr->parser_list);
    svr->grammar_parser = create_gdl_parser(svr->parser_list);

    svr->tokenizer_registry = epc_ast_hook_registry_create(TOKENIZER_ACTION_COUNT);
    svr->ast_registry = epc_ast_hook_registry_create(GDL_AST_ACTION_MAX);
    gdl_ast_hook_registry_init(svr->ast_registry, NULL);

    svr->caches = NULL;
    svr->cache_count = 0;
    svr->cache_capacity = 0;
    svr->pending_uri = NULL;

    uloop_timeout_set(&svr->debounce_timer, 0);

    gdl_lsp_register_handlers(svr);

    uloop_run();

    rpc_cleanup_registry(&svr->base);
    documents_cleanup(svr->base.documents);
    transport_cleanup(&svr->base);

    free(svr->pending_uri);
    svr->pending_uri = NULL;

    for (int i = 0; i < svr->cache_count; i++)
    {
        free(svr->caches[i].tokens);
        free(svr->caches[i].ast_token_types);
    }
    free(svr->caches);
    svr->caches = NULL;
    svr->cache_count = 0;
    svr->cache_capacity = 0;

    epc_ast_hook_registry_free(svr->ast_registry);
    svr->ast_registry = NULL;
    epc_ast_hook_registry_free(svr->tokenizer_registry);
    svr->tokenizer_registry = NULL;

    epc_parser_list_free(svr->parser_list);
    svr->parser_list = NULL;
    svr->tokenizer_parser = NULL;
    svr->grammar_parser = NULL;

    if (svr->base.framing)
    {
        svr->base.framing->destroy(svr->base.framing);
        svr->base.framing = NULL;
    }

    uloop_done();

    if (in_fd != STDIN_FILENO)
    {
        close(in_fd);
    }
}
