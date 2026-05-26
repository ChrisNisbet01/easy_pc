set(app "python_parser")

# Generate the grammar library using the macro
epc_generate_grammar(
    TARGET python_tokenizer_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/python_tokenizer.gdl
)

add_executable(${app}
  main.c
)

add_dependencies(${app} all_unit_tests)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic -Werror)

target_link_libraries(${app} PRIVATE
    easy_pc_shared
    python_tokenizer_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
    ${CMAKE_SOURCE_DIR}/lib
)

target_compile_features(${app} PRIVATE c_std_11)
