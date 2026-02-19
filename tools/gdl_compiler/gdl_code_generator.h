#pragma once

#include "gdl_ast.h"

#include <stdbool.h>

// Function to generate C code from the GDL AST
// ast_root: The root of the GDL AST
// base_name: The base name for the generated files (e.g., "simple_test_language")
// output_dir: The directory where the files should be written
// Returns true on success, false on failure.
bool gdl_generate_c_code(gdl_ast_node_t * ast_root, const char * base_name, const char * output_dir);

