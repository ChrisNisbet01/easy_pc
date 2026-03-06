#pragma once

#include "parsers.h"

#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h>
#include <stdbool.h>

// Internal types for AST builder stack management
typedef enum
{
    EPC_AST_ITEM_PLACEHOLDER,
    EPC_AST_ITEM_USER_NODE
} epc_ast_item_type_t;

typedef struct
{
    epc_ast_item_type_t type;
    void * ptr; // Points to user's AST node or is NULL for a placeholder
} epc_ast_stack_entry_t;

struct epc_ast_builder_ctx_t
{
    epc_ast_stack_entry_t * stack;
    int top;      // Number of items currently on stack
    int capacity; // Current allocated capacity of the stack
    epc_ast_hook_registry_t * registry;
    void * user_data;
    bool has_error;
    char error_message[512];
};

struct epc_ast_hook_registry_t
{
    epc_ast_action_cb * callbacks;  /**< @brief Array of semantic action callbacks, indexed by action value. */
    int action_count;               /**< @brief The number of possible semantic actions. */
    epc_ast_node_free_cb free_node; /**< @brief Callback to free a user-defined AST node. */
    epc_ast_enter_cb enter_node;    /**< @brief Callback for entering a CPT node. */
};

/**
 * @brief Initiates a parsing operation with a given grammar and input.
 */
EASY_PC_HIDDEN epc_parse_session_t epc_parse_input(epc_parser_t * top_parser, epc_parse_input_t input, void * user_ctx);
