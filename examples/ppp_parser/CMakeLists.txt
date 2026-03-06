set(app ppp_parser)

add_executable(${app} main.c)
target_link_libraries(${app} PRIVATE
    easy_pc_shared
)

target_include_directories(${app} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/../../include
)
