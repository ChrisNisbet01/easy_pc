set(app "regex_parser")

# Generate the grammar library using the macro
epc_generate_grammar(
    TARGET regex_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/regex.gdl
    BOOTSTRAP_AST
)

# Initially, we'll only have main.c
# Once bootstrap files are copied to src, we'll add them here.
add_executable(${app} 
  main.c 
  regex_ast_actions.c
)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic -Werror)

target_link_libraries(${app} PRIVATE
    easy_pc_shared
    regex_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)

target_compile_features(${app} PRIVATE c_std_11)
