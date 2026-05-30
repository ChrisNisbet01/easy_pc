# CMakeLists.txt for tools/tok_gdl_parser
set(app "tok_gdl_parser")

# Define the executable for the token-based GDL parser
add_executable(
    ${app}
    main.c
    gdl_parser.c
    gdl_tokenizer_parser.c
    gdl_tokenizer_actions.c
    tok_gdl_parser_ast_actions.c
    gdl_code_generator.c
    gdl_bootstrap_generator.c
)
target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic)

# Set version properties for the executable (for symbolic links on install)
set_target_properties(${app} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
)

# Pass the version to the source code
target_compile_definitions(${app} PRIVATE TOK_GDL_PARSER_VERSION="${PROJECT_VERSION}")

add_dependencies(${app} all_unit_tests)

# Link the GDL parser against the easy_pc library
target_link_libraries(${app} PRIVATE easy_pc_shared)

# Set include directories for easy_pc headers
target_include_directories(${app} PRIVATE ${CMAKE_SOURCE_DIR}/include)

install(TARGETS ${app} RUNTIME DESTINATION bin)
