add_executable(gdl_lsp
    main.c
    gdl_lsp_server.c
    gdl_lsp_handlers.c
    gdl_lsp_semantic_tokens.c
    documents.c
    utils.c
)
target_compile_options(gdl_lsp PRIVATE -Wall -Wextra -pedantic)
target_compile_definitions(gdl_lsp PRIVATE _GNU_SOURCE)
target_link_libraries(gdl_lsp PRIVATE gdl_parser_objects easy_pc_shared rpc2)
target_include_directories(gdl_lsp PRIVATE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../gdl_compiler>
)
