set(GDL_INPUT_FILE ${CMAKE_CURRENT_SOURCE_DIR}/c_grammar.gdl)
set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR}/generated)
file(MAKE_DIRECTORY ${GENERATED_DIR})

set(GENERATED_HEADER ${GENERATED_DIR}/c_grammar.h)
set(GENERATED_SOURCE ${GENERATED_DIR}/c_grammar.c)
set(GENERATED_ACTIONS_HEADER ${GENERATED_DIR}/c_grammar_actions.h)

add_custom_command(
    OUTPUT ${GENERATED_HEADER} ${GENERATED_SOURCE} ${GENERATED_ACTIONS_HEADER}
    COMMAND gdl_compiler ${GDL_INPUT_FILE} --output-dir=${GENERATED_DIR} --header=callbacks.h
    DEPENDS ${GDL_INPUT_FILE} gdl_compiler ${CMAKE_CURRENT_SOURCE_DIR}/callbacks.h
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
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_link_libraries(c_parser_example PRIVATE
    easy_pc_shared
)

# Copy test files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
