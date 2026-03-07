set(app "html_parser_example")

# Generate the grammar library using the macro
epc_generate_grammar(
    TARGET html_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/html.gdl
)

add_executable(${app} 
  main.c 
)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(${app} PRIVATE
    easy_pc_static
    html_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Enable LTO if supported
if(lto_supported)
  message(STATUS "LTO enabled for ${app} and html_grammar.")
  set_target_properties(${app} html_grammar PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
endif()

# Copy test files to build directory
file(COPY ${CMAKE_CURRENT_SOURCE_DIR}/tests DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
