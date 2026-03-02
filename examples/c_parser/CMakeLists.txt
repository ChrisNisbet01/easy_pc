set(GDL_INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/c_grammar.gdl)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(GENERATED_HEADER ${GENERATED_DIR}/c_grammar.h)
set(GENERATED_SOURCE ${GENERATED_DIR}/c_grammar.c)
set(GENERATED_ACTIONS_HEADER ${GENERATED_DIR}/c_grammar_actions.h)

add_custom_command(
    OUTPUT ${GENERATED_HEADER} ${GENERATED_SOURCE} ${GENERATED_ACTIONS_HEADER}
    COMMAND gdl_compiler ${GDL_INPUT_FILE} --output-dir=${GENERATED_DIR}
    DEPENDS ${GDL_INPUT_FILE} gdl_compiler
    VERBATIM
    COMMENT "Generating parser code for ${GDL_INPUT_FILE}"
)

add_executable(c_parser_example
    main.c
    ${GENERATED_SOURCE}
)

target_include_directories(c_parser_example PRIVATE
    ${GENERATED_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

target_link_libraries(c_parser_example PRIVATE
    easy_pc_shared
)
