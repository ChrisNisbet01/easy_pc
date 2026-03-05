set(app "c_parser_example")

# Generate the grammar library using the macro
epc_generate_grammar(
    TARGET c_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/c_grammar.gdl
    HEADER callbacks.h
)

add_executable(${app} 
  main.c 
)

add_dependencies(${app} all_unit_tests)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(${app} PRIVATE
    easy_pc_shared
    c_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Copy test files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
