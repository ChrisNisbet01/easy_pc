#pragma once

#include "gdl_ast.h"
#include "easy_pc/easy_pc.h"

//#define AST_DEBUG 1
#define GDL_AST_BUILDER_MAX_STACK_SIZE 256 // Max depth of CPT traversal / nested expressions

// Context for the AST builder, holds the stack and final AST root
typedef struct
{
    gdl_ast_node_t * stack[GDL_AST_BUILDER_MAX_STACK_SIZE];
    int stack_size;
    gdl_ast_node_t * ast_root; // The final constructed AST root

    // Error handling
    bool has_error;
#ifdef AST_DEBUG
    bool printed_error;
#endif
    char error_message[512]; // Increased size for potentially longer messages
} gdl_ast_builder_data_t;

// Visitor callbacks
void
gdl_ast_builder_enter_node(epc_cpt_node_t * node, void * user_data);

void
gdl_ast_builder_exit_node(epc_cpt_node_t * node, void * user_data);

// Function to initialize the AST builder data
void
gdl_ast_builder_init(gdl_ast_builder_data_t * data);

// Function to clean up the AST builder data after building is complete
void
gdl_ast_builder_cleanup(gdl_ast_builder_data_t * data);
