#include "gdl_lsp_server.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int
main(void)
{
    gdl_lsp_server_st svr;
    memset(&svr, 0, sizeof(svr));

    run_gdl_lsp_server(&svr, STDIN_FILENO, STDOUT_FILENO);

    return EXIT_SUCCESS;
}
