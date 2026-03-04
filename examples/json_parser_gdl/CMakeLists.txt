set(app "json_parser_gdl")

# Generate the grammar library using the macro
epc_generate_grammar(
    TARGET json_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/json.gdl
    BOOTSTRAP_AST
)

add_executable(${app} 
  main.c 
  json_ast_actions.c
)

add_dependencies(${app} all_unit_tests)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic -Werror)

target_link_libraries(${app} PRIVATE
    easy_pc_shared
    json_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_features(${app} PRIVATE c_std_11)