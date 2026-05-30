cmake_minimum_required(VERSION 3.16)
project(easy_pc VERSION 0.2.0 LANGUAGES C)

include(FetchContent)
include(CTest)
include(GNUInstallDirs)
include(cmake/EasyPcMacros.cmake)

# --- Versioning ---
set(EASY_PC_VERSION_MAJOR ${PROJECT_VERSION_MAJOR})
set(EASY_PC_VERSION_MINOR ${PROJECT_VERSION_MINOR})
set(EASY_PC_VERSION_PATCH ${PROJECT_VERSION_PATCH})
set(EASY_PC_VERSION "${PROJECT_VERSION}")

configure_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/include/easy_pc/easy_pc_version.h.in
    ${CMAKE_CURRENT_BINARY_DIR}/include/easy_pc/easy_pc_version.h
)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Optimization settings
add_compile_options(-O3)

# LTO settings
include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported)

if(lto_supported)
    message(STATUS "LTO (Interprocedural Optimization) is supported by the toolchain.")
endif()

option(WITH_INPUT_STREAM_SUPPORT "Enable streaming input support" ON)

if(WITH_INPUT_STREAM_SUPPORT)
    find_package(Threads REQUIRED)
endif()

add_subdirectory(lib)

option(BUILD_EXAMPLES "Build example applications" OFF)
if(BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

# --- Unit Tests ---
option(WITH_UNIT_TESTS "Build unit tests" OFF)
add_custom_target(all_unit_tests)

if(WITH_UNIT_TESTS)

  enable_language(CXX) # Tests are in C++

  # CppUTest
  set(CPPUTEST_USE_GCOV OFF)
  set(CPPUTEST_USE_MEM_LEAK_DETECTION OFF)
  set(CPPUTEST_USE_VALGRIND OFF)
  set(CPPUTEST_USE_VLD OFF)
  set(CPPUTEST_USE_SANITIZERS OFF)
  set(CPPUTEST_USE_COVERAGE OFF)
  set(CPPUTEST_USE_PCH OFF)
  set(CPPUTEST_USE_MODULES OFF)
  set(CPPUTEST_USE_TDD_MACROS OFF)
  set(CPPUTEST_USE_REAL_GCC_THR OFF)
  set(CPPUTEST_USE_REAL_MINGW_THR OFF)
  set(CPPUTEST_USE_REAL_PTHREAD OFF)
  set(CPPUTEST_USE_REAL_STD_THR OFF)
  set(CPPUTEST_USE_EXTENSIONS ON)
  set(CPPUTEST_MEM_LEAK_DETECTION_DISABLED OFF CACHE BOOL "" FORCE)

  # Prevent CppUTest's own tests from being added to our test suite
  set(TESTS OFF CACHE BOOL "Disable CppUTest's own tests")

  FetchContent_Declare(
    cpputest
    GIT_REPOSITORY https://github.com/cpputest/cpputest.git
    GIT_TAG        v4.0
    EXCLUDE_FROM_ALL
    CMAKE_ARGS -DBUILD_INSTALL=OFF -DBUILD_TESTING=OFF # Correctly disable CppUTest's own installation and tests
  )
  FetchContent_MakeAvailable(cpputest)
  add_subdirectory(tests)

endif()

# --- Doxygen Documentation ---
find_package(Doxygen)

if (DOXYGEN_FOUND AND EXISTS ${CMAKE_SOURCE_DIR}/Doxyfile)
    message(STATUS "Doxygen found, documentation will be generated.")
    file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/docs/help/html)

    # Add a custom target to run Doxygen (not ALL to avoid build failures if Doxyfile is missing)
    add_custom_target(
        docs
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Generating API documentation with Doxygen"
    )

    # Make the documentation target dependent on the header file
    # This ensures Doxygen is re-run if the header file changes.
    add_custom_command(
        OUTPUT ${CMAKE_SOURCE_DIR}/docs/help/html/index.html
        COMMAND ${DOXYGEN_EXECUTABLE} Doxyfile
        MAIN_DEPENDENCY include/easy_pc/easy_pc.h
        WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
        COMMENT "Re-generating Doxygen documentation due to header change"
        VERBATIM
    )
    add_custom_target(doxygen_docs DEPENDS ${CMAKE_SOURCE_DIR}/docs/help/html/index.html)

    install(
        DIRECTORY ${CMAKE_SOURCE_DIR}/docs/help/html/
        DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/doc/easy_pc
    )

else ()
    message(STATUS "Doxygen not found or Doxyfile missing, skipping documentation generation.")
endif ()

add_subdirectory(tools/gdl_compiler)
add_subdirectory(tools/gdl_debug)
option(BUILD_GDL_LSP "Build the GDL LSP server" OFF)
if(BUILD_GDL_LSP)
  add_subdirectory(tools/gdl_lsp)
endif()
#add_subdirectory(tools/tok_gdl_parser)

# --- pkg-config ---
configure_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/easy_pc.pc.in"
    "${CMAKE_CURRENT_BINARY_DIR}/easy_pc.pc"
    @ONLY
)

install(
    FILES "${CMAKE_CURRENT_BINARY_DIR}/easy_pc.pc"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/pkgconfig"
)