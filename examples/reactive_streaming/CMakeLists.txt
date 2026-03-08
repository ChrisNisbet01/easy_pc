set(app "reactive_streaming_example")

# Generate the grammar library using the macro
epc_generate_grammar(
    TARGET packet_grammar
    GDL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/packet.gdl
)

add_executable(${app} 
  main.c 
)

target_compile_options(${app} PRIVATE -Wall -Wextra -pedantic)

target_link_libraries(${app} PRIVATE
    easy_pc_static
    packet_grammar
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/include
)

# Enable LTO if supported
if(lto_supported)
  message(STATUS "LTO enabled for ${app} and packet_grammar.")
  set_target_properties(${app} packet_grammar PROPERTIES
    INTERPROCEDURAL_OPTIMIZATION TRUE
  )
endif()
