# Easy PC CMake Macros

function(epc_generate_grammar)
    set(options BOOTSTRAP_AST)
    set(oneValueArgs TARGET GDL_FILE OUTPUT_DIR HEADER)
    set(multiValueArgs)
    cmake_parse_arguments(EPC_GEN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(NOT EPC_GEN_TARGET)
        message(FATAL_ERROR "EPC_GENERATE_GRAMMAR: TARGET argument is required")
    endif()

    if(NOT EPC_GEN_GDL_FILE)
        message(FATAL_ERROR "EPC_GENERATE_GRAMMAR: GDL_FILE argument is required")
    endif()

    if(NOT EPC_GEN_OUTPUT_DIR)
        set(EPC_GEN_OUTPUT_DIR "${CMAKE_CURRENT_BINARY_DIR}/generated")
    endif()

    file(MAKE_DIRECTORY "${EPC_GEN_OUTPUT_DIR}")

    # Get the base name of the GDL file (without extension)
    get_filename_component(GDL_BASE_NAME "${EPC_GEN_GDL_FILE}" NAME_WE)

    set(LIB_SOURCES "${EPC_GEN_OUTPUT_DIR}/${GDL_BASE_NAME}.c")
    set(ALL_GENERATED_SOURCES "${LIB_SOURCES}")
    set(GENERATED_HEADERS
        "${EPC_GEN_OUTPUT_DIR}/${GDL_BASE_NAME}.h"
        "${EPC_GEN_OUTPUT_DIR}/${GDL_BASE_NAME}_actions.h"
    )

    set(EXTRA_ARGS)
    set(EXTRA_DEPENDS)
    if(EPC_GEN_BOOTSTRAP_AST)
        set(BOOTSTRAP_DIR "${EPC_GEN_OUTPUT_DIR}/bootstrap")
        file(MAKE_DIRECTORY "${BOOTSTRAP_DIR}")
        list(APPEND EXTRA_ARGS "--bootstrap-ast")
        # Bootstrap files are generated but NOT added to LIB_SOURCES
        list(APPEND ALL_GENERATED_SOURCES "${BOOTSTRAP_DIR}/${GDL_BASE_NAME}_ast_actions.c")
        list(APPEND GENERATED_HEADERS
            "${BOOTSTRAP_DIR}/${GDL_BASE_NAME}_ast.h"
            "${BOOTSTRAP_DIR}/${GDL_BASE_NAME}_ast_actions.h"
        )
    endif()

    if(EPC_GEN_HEADER)
        list(APPEND EXTRA_ARGS "--header=${EPC_GEN_HEADER}")
        # If the header exists in the source directory, add it as a dependency
        if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${EPC_GEN_HEADER}")
            list(APPEND EXTRA_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/${EPC_GEN_HEADER}")
        endif()
    endif()

    # Add custom command to run gdl_compiler
    add_custom_command(
        OUTPUT ${ALL_GENERATED_SOURCES} ${GENERATED_HEADERS}
        COMMAND gdl_compiler "${EPC_GEN_GDL_FILE}" "--output-dir=${EPC_GEN_OUTPUT_DIR}" ${EXTRA_ARGS}
        DEPENDS "${EPC_GEN_GDL_FILE}" gdl_compiler ${EXTRA_DEPENDS}
        VERBATIM
        COMMENT "Generating parser code for ${GDL_BASE_NAME}.gdl"
    )

    # Create a library with the generated core parser sources only
    add_library("${EPC_GEN_TARGET}" STATIC ${LIB_SOURCES})
    target_include_directories("${EPC_GEN_TARGET}" PUBLIC
        "${EPC_GEN_OUTPUT_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}" # Add source directory to find extra headers
        "${CMAKE_SOURCE_DIR}/include" # Add easy_pc's public include directory
    )
    target_link_libraries("${EPC_GEN_TARGET}" PUBLIC easy_pc_shared)

    # Ensure gdl_compiler is built before this library
    add_dependencies("${EPC_GEN_TARGET}" gdl_compiler)

endfunction()
