set(app "python_parser")

# Generate the tokenizer grammar library (stage 1)
epc_generate_grammar(
    TARGET python_tokenizer_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python_tokenizer.gdl
)

# Generate the AST grammar library (stage 2)
epc_generate_grammar(
    TARGET python_ast_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python_ast.gdl
    HEADER python_token_ids.h
    BOOTSTRAP_AST
)

# Generate the commit-bench AST grammar library (stage 2, with commit points)
epc_generate_grammar(
    TARGET python_ast_bench_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python_ast_bench.gdl
    HEADER python_token_ids.h
)

add_executable(${app}
  main.c
  python_ast_builder.c
  python_ast_printer.c
)

add_dependencies(${app} all_unit_tests)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic -Werror)

target_link_libraries(${app} PRIVATE
    easy_pc_shared
    python_tokenizer_grammar
    python_ast_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/lib
)

target_compile_features(${app} PRIVATE c_std_11)

# === Benchmark executable ===
add_executable(python_parser_bench
  bench_main.c
)
target_compile_options(python_parser_bench PRIVATE -Wall -Wextra -pedantic -Werror)
target_link_libraries(python_parser_bench PRIVATE
    easy_pc_shared
    python_tokenizer_grammar
    python_ast_grammar
    python_ast_bench_grammar
)
target_include_directories(python_parser_bench PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/lib
    ${CMAKE_CURRENT_BINARY_DIR}/generated
)
target_compile_features(python_parser_bench PRIVATE c_std_11)
