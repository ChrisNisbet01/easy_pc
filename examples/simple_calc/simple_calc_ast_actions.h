#pragma once

#include "ast.h"
#include "easy_pc/easy_pc.h"
#include "easy_pc/easy_pc_ast.h" // Added for epc_ast_hook_registry_t

// AST Action Definitions for Semantic Actions
typedef enum {
    AST_ACTION_CREATE_NUMBER_FROM_CONTENT, // Create AST_TYPE_NUMBER from parser content (long long)
    AST_ACTION_CREATE_OPERATOR_FROM_CHAR,  // Create AST_TYPE_OPERATOR from parser content (char)
    AST_ACTION_COLLECT_CHILD_RESULTS,      // Collect all AST results from successful children into a list
    AST_ACTION_BUILD_BINARY_EXPRESSION,    // Build binary expression from (left, op_list, right_list)
    AST_ACTION_CREATE_IDENTIFIER,          // Create an identifier node (for functions, constants, variables)
    AST_ACTION_CREATE_FUNCTION_CALL,       // For function_call = name '(' args ')'
    AST_ACTION_ASSIGN_ROOT,                // Sets the root node of the AST. (Should be the top of the node stack)
    AST_ACTION_MAX,                        // Sentinel for the maximum number of AST actions
} ast_action_type_t;

// Function to free the root AST node returned after building the AST tree
void
ast_node_free(void * node, void * user_data);

// Function to initialize the AST hook registry for simple_calc
void
simple_calc_ast_hook_registry_init(epc_ast_hook_registry_t * registry);

