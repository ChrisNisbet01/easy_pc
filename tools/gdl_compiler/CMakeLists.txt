# CMakeLists.txt for tools/gdl_compiler
set(app "gdl_compiler")

# Shared parser sources (consumed by gdl_compiler and gdl_lsp)
add_library(gdl_parser_objects OBJECT
    gdl_parser.c
    gdl_tokenizer_parser.c
    gdl_tokenizer_actions.c
    gdl_compiler_ast_actions.c
)
target_include_directories(gdl_parser_objects PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../lib>
)
target_compile_options(gdl_parser_objects PRIVATE -Wall -Wextra -pedantic)

# Define the executable for the token-based GDL parser
add_executable(
    ${app}
    main.c
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
target_compile_definitions(${app} PRIVATE GDL_COMPILER_VERSION="${PROJECT_VERSION}")

add_dependencies(${app} all_unit_tests)

# Link the GDL parser against the easy_pc library and shared objects
target_link_libraries(${app} PRIVATE gdl_parser_objects easy_pc_shared)

install(TARGETS ${app} RUNTIME DESTINATION bin)
