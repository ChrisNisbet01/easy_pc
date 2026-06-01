#include "gdl_lsp_server.h"

#include "gdl_ast.h"
#include "gdl_compiler_ast_actions.h"
#include "gdl_lsp_handlers.h"
#include "gdl_parser.h"
#include "gdl_tokenizer_parser.h"

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void
run_gdl_lsp_server(gdl_lsp_server_st * const svr, int const in_fd, int const out_fd)
{
    fprintf(stderr, "[LSP] Server starting on in_fd=%d, out_fd=%d\n", in_fd, out_fd);

    svr->documents = documents_init();

    svr->ctx = rpc_ctx_new();
    if (!svr->ctx)
    {
        fprintf(stderr, "[LSP] Error: failed to create rpc_ctx\n");
        documents_cleanup(svr->documents);
        return;
    }

    rpc_ctx_set_framing(svr->ctx, framing_content_length_create());
    rpc_ctx_set_fds(svr->ctx, in_fd, out_fd);

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

    gdl_lsp_register_handlers(svr);

    rpc_ctx_run(svr->ctx);

    rpc_ctx_destroy(svr->ctx);
    svr->ctx = NULL;

    documents_cleanup(svr->documents);
    svr->documents = NULL;

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

    if (in_fd != STDIN_FILENO)
    {
        close(in_fd);
    }
}
