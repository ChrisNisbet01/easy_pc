#include "gdl_ast_builder.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef AST_DEBUG
static int debug_indent = 0;
#endif

// Helper to get text content from CPT node
static char *
get_cpt_node_text(epc_cpt_node_t * node)
{
    size_t len = epc_cpt_node_get_semantic_len(node);
    char * text = (char *)malloc(len + 1);
    if (text == NULL)
    {
        return NULL;
    }
    strncpy(text, epc_cpt_node_get_semantic_content(node), len);
    text[len] = '\0';
    return text;
}

static gdl_ast_list_t
gdl_ast_list_init(void)
{
    gdl_ast_list_t list = { NULL, NULL, 0 };
    return list;
}

static void
gdl_ast_list_prepend(gdl_ast_list_t * list, gdl_ast_node_t * item)
{
    if (list == NULL || item == NULL)
    {
        return;
    }

    gdl_ast_list_node_t * new_list_node = (gdl_ast_list_node_t *)calloc(1, sizeof(gdl_ast_list_node_t));
    if (new_list_node == NULL)
    {
        perror("Failed to allocate gdl_ast_list_node_t for prepend");
        return;
    }
    new_list_node->item = item;
    new_list_node->next = list->head;

    list->head = new_list_node;
    if (list->tail == NULL)
    {
        list->tail = new_list_node;
    }
    list->count++;
}

static void
gdl_ast_list_free(gdl_ast_list_t * list)
{
    if (list == NULL)
    {
        return;
    }

    gdl_ast_list_node_t * current = list->head;
    while (current != NULL)
    {
        gdl_ast_list_node_t * next = current->next;
        gdl_ast_node_free(current->item); // Free the AST node itself
        free(current);                     // Free the list node wrapper
        current = next;
    }
    list->head = NULL;
    list->tail = NULL;
    list->count = 0;
}

// --- Helper Functions for AST node_t management ---
void
gdl_ast_node_free(gdl_ast_node_t * node)
{
    if (node == NULL)
    {
        return;
    }

    // Free specific data based on type (recursive for child nodes)
    switch (node->type)
    {
    case GDL_AST_NODE_TYPE_PROGRAM:
        gdl_ast_list_free(&node->data.program.rules);
        break;

    case GDL_AST_NODE_TYPE_RULE_DEFINITION:
        free((char *)node->data.rule_def.name); // Allocated by strdup
        gdl_ast_node_free(node->data.rule_def.definition);
        gdl_ast_node_free(node->data.rule_def.semantic_action);
        break;

    case GDL_AST_NODE_TYPE_IDENTIFIER_REF:
        free((char *)node->data.identifier_ref.name);
        break;

    case GDL_AST_NODE_TYPE_STRING_LITERAL:
    case GDL_AST_NODE_TYPE_FAIL_CALL:
        free((char *)node->data.string_literal.value);
        break;

    case GDL_AST_NODE_TYPE_KEYWORD:
        free((char *)node->data.keyword.name);
        break;

    case GDL_AST_NODE_TYPE_TERMINAL:
        gdl_ast_node_free(node->data.terminal.expression);
        break;

    case GDL_AST_NODE_TYPE_SEMANTIC_ACTION:
        free((char *)node->data.semantic_action.action_name);
        break;

    case GDL_AST_NODE_TYPE_REPETITION_EXPRESSION:
        gdl_ast_node_free(node->data.repetition_expr.expression);
        gdl_ast_node_free(node->data.repetition_expr.repetition);
        break;

    case GDL_AST_NODE_TYPE_COMBINATOR_ONEOF:
    case GDL_AST_NODE_TYPE_COMBINATOR_NONEOF:
        gdl_ast_list_free(&node->data.oneof_call.args); // Common arg list for both
        break;

    case GDL_AST_NODE_TYPE_COMBINATOR_COUNT:
        gdl_ast_node_free(node->data.count_call.count_node);
        gdl_ast_node_free(node->data.count_call.expression);
        break;

    case GDL_AST_NODE_TYPE_COMBINATOR_BETWEEN:
        gdl_ast_node_free(node->data.between_call.open_expr);
        gdl_ast_node_free(node->data.between_call.content_expr);
        gdl_ast_node_free(node->data.between_call.close_expr);
        break;

    case GDL_AST_NODE_TYPE_COMBINATOR_DELIMITED:
        gdl_ast_node_free(node->data.delimited_call.item_expr);
        gdl_ast_node_free(node->data.delimited_call.delimiter_expr);
        break;

    case GDL_AST_NODE_TYPE_COMBINATOR_LOOKAHEAD:
    case GDL_AST_NODE_TYPE_COMBINATOR_NOT:
    case GDL_AST_NODE_TYPE_COMBINATOR_LEXEME:
    case GDL_AST_NODE_TYPE_COMBINATOR_SKIP:
    case GDL_AST_NODE_TYPE_COMBINATOR_PASSTHRU:
        gdl_ast_node_free(node->data.unary_combinator_call.expr);
        break;

    case GDL_AST_NODE_TYPE_OPTIONAL_EXPRESSION:
        gdl_ast_node_free(node->data.optional.expr);
        break;


    case GDL_AST_NODE_TYPE_COMBINATOR_CHAINL1:
    case GDL_AST_NODE_TYPE_COMBINATOR_CHAINR1:
        gdl_ast_node_free(node->data.chain_combinator_call.item_expr);
        gdl_ast_node_free(node->data.chain_combinator_call.op_expr);
        break;

    case GDL_AST_NODE_TYPE_SEQUENCE:
        gdl_ast_list_free(&node->data.sequence.elements);
        break;

    case GDL_AST_NODE_TYPE_ALTERNATIVE:
        gdl_ast_list_free(&node->data.alternative.alternatives);
        break;

    case GDL_AST_NODE_TYPE_ARGUMENT_LIST:
        gdl_ast_list_free(&node->data.argument_list);
        break;

        /* The following nod types have no dynamic data to free. */
    case GDL_AST_NODE_TYPE_NUMBER_LITERAL: // No dynamic data to free
    case GDL_AST_NODE_TYPE_CHAR_LITERAL:   // No dynamic data to free
    case GDL_AST_NODE_TYPE_CHAR_RANGE:     // No dynamic data to free
    case GDL_AST_NODE_TYPE_REPETITION_OPERATOR: // No dynamic data to free
    case GDL_AST_NODE_TYPE_RAW_CHAR_LITERAL: // No dynamic data to free
    case GDL_AST_NODE_TYPE_NONE:
    case GDL_AST_NODE_TYPE_PLACEHOLDER: // Placeholder has no data
        break;
    }

    free(node);
}

// --- AST Builder Context Management ---

static void
gdl_ast_builder_set_error(gdl_ast_builder_data_t * data, epc_cpt_node_t * pt_node, const char * format, ...)
{
    if (data->has_error)
    {
        return; // Don't overwrite the first error
    }

    data->has_error = true;
    data->ast_root = NULL; // Ensure ast_root is NULL on error

    va_list args;
    va_start(args, format);
    char specific_message[256];
    vsnprintf(specific_message, sizeof(specific_message), format, args);
    va_end(args);

    if (pt_node)
    {
        snprintf(data->error_message, sizeof(data->error_message),
                 "AST build error at node '%s': %s",
                 pt_node->name,
                 specific_message);
    }
    else
    {
        snprintf(data->error_message, sizeof(data->error_message),
                 "AST build error: %s", specific_message);
    }
}


static gdl_ast_node_t *
gdl_ast_node_alloc(gdl_ast_builder_data_t * data, gdl_ast_node_type_t node_type)
{
    if (data->has_error)
    {
        return NULL;
    }
    gdl_ast_node_t * node = calloc(1, sizeof(*node));
    if (node == NULL)
    {
        gdl_ast_builder_set_error(data, NULL, "Failed to allocate AST node");
    }
    else
    {
        node->type = node_type;
    }

    return node;
}

#ifdef AST_DEBUG
static void
gdl_ast_stack_print(gdl_ast_builder_data_t * data)
{
    fprintf(stderr, "%*sCurrent stack state:\n", debug_indent, "");
    fprintf(stderr, "%*s--------------------\n", debug_indent, "");

    for (size_t i = 0; i < (size_t)data->stack_size; i++)
    {
        gdl_ast_node_t * node = data->stack[i];
        fprintf(stderr, "stack slot %zu node type: %u\n", i, node->type);
    }
}
#endif

static void
gdl_ast_stack_push(gdl_ast_builder_data_t * data, gdl_ast_node_t * node)
{
    if (data->has_error)
    {
        gdl_ast_node_free(node); // Free node if error already exists
        return;
    }
    if (node == NULL)
    {
        gdl_ast_builder_set_error(data, NULL, "attempted to push NULL node onto AST stack");
        return;
    }

    if (data->stack_size < GDL_AST_BUILDER_MAX_STACK_SIZE)
    {
        data->stack[data->stack_size++] = node;
#ifdef AST_DEBUG
        fprintf(stderr, "%*sAST_DEBUG: Pushed AST node type %d. Stack size: %d\n",
                debug_indent, "", node->type, data->stack_size);
#endif
    }
    else
    {
        gdl_ast_builder_set_error(data, NULL, "AST Builder Stack Overflow");
        gdl_ast_node_free(node); // Free node to prevent leak
    }
}

static gdl_ast_node_t *
gdl_ast_stack_pop(gdl_ast_builder_data_t * data)
{
    if (data->has_error)
    {
        return NULL;
    }

    if (data->stack_size > 0)
    {
        gdl_ast_node_t * node = data->stack[--data->stack_size];
#ifdef AST_DEBUG
        fprintf(stderr, "%*sAST_DEBUG: Popped AST node type %d. Stack size: %d\n",
                debug_indent, "", node->type, data->stack_size);
#endif
        return node;
    }
    gdl_ast_builder_set_error(data, NULL, "AST Builder Stack Underflow");
    return NULL;
}

static bool
is_placeholder_node(gdl_ast_node_t * node)
{
    return node == NULL || node->type == GDL_AST_NODE_TYPE_PLACEHOLDER;
}

static void
push_placeholder_node(gdl_ast_builder_data_t * data)
{
    gdl_ast_node_t * placeholder_node =
                                        gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_PLACEHOLDER);
    if (placeholder_node == NULL)
    {
        return;
    }
    gdl_ast_stack_push(data, placeholder_node);
}

void
gdl_ast_builder_init(gdl_ast_builder_data_t * data)
{
    memset(data, 0, sizeof(gdl_ast_builder_data_t));
}

void
gdl_ast_builder_cleanup(gdl_ast_builder_data_t * data)
{
    if (data->stack_size > 0)
    {
        fprintf(stderr,
                "Warning: GDL AST builder stack not empty - still has %u AST nodes pushed\n",
                data->stack_size
               );
#ifdef AST_DEBUG
        gdl_ast_stack_print(data);
#endif

        for (int i = 0; i < data->stack_size; i++)
        {
            gdl_ast_node_free(data->stack[i]);
        }
    }

    data->stack_size = 0;
    gdl_ast_node_free(data->ast_root);
    data->ast_root = NULL;
    data->has_error = false;
    memset(data->error_message, 0, sizeof(data->error_message));
}

// --- Visitor Callbacks ---

void
gdl_ast_builder_enter_node(epc_cpt_node_t * node, void * user_data)
{
#ifdef AST_DEBUG
    debug_indent += 4;
#endif

    gdl_ast_builder_data_t * data = (gdl_ast_builder_data_t *)user_data;

    if (data->has_error)
    {
#ifdef AST_DEBUG
        if (!data->printed_error)
        {
            data->printed_error = true;
            fprintf(stderr, "%*sAST_DEBUG: Error exiting CPT node '%s' (action %d), stack size %d\n",
                    debug_indent, "", node->name, node->ast_config.action, data->stack_size);
            if (data->error_message[0] != '\0')
            {
                fprintf(stderr, "%*smsg: %s\n", debug_indent, "", data->error_message);
            }
        }
#endif
        return;
    }

    if (node == NULL)
    {
        return;
    }

#ifdef AST_DEBUG
    fprintf(stderr, "%*sAST_DEBUG: ENTER CPT node '%s' (action %d), stack size %d\n",
            debug_indent - 4, "", node->name, node->ast_config.action, data->stack_size);
#endif

    switch (node->ast_config.action)
    {
    case GDL_AST_ACTION_NONE:
        return;

        // Actions that push a placeholder node and are handled in exit_node
    case GDL_AST_ACTION_CREATE_IDENTIFIER_REF:
    case GDL_AST_ACTION_CREATE_STRING_LITERAL:
    case GDL_AST_ACTION_CREATE_CHAR_LITERAL:
    case GDL_AST_ACTION_CREATE_NUMBER_LITERAL:
    case GDL_AST_ACTION_CREATE_REPETITION_OPERATOR:
    case GDL_AST_ACTION_CREATE_SEMANTIC_ACTION:
    case GDL_AST_ACTION_CREATE_OPTIONAL_SEMANTIC_ACTION:
    case GDL_AST_ACTION_CREATE_RAW_CHAR_LITERAL:
    case GDL_AST_ACTION_CREATE_PROGRAM:
    case GDL_AST_ACTION_CREATE_RULE_DEFINITION:
    case GDL_AST_ACTION_CREATE_CHAR_RANGE:
    case GDL_AST_ACTION_COLLECT_ARGUMENTS:
    case GDL_AST_ACTION_CREATE_EXPRESSION_FACTOR:
    case GDL_AST_ACTION_CREATE_ONEOF_CALL:
    case GDL_AST_ACTION_CREATE_NONEOF_CALL:
    case GDL_AST_ACTION_CREATE_COUNT_CALL:
    case GDL_AST_ACTION_CREATE_BETWEEN_CALL:
    case GDL_AST_ACTION_CREATE_DELIMITED_CALL:
    case GDL_AST_ACTION_CREATE_LOOKAHEAD_CALL:
    case GDL_AST_ACTION_CREATE_NOT_CALL:
    case GDL_AST_ACTION_CREATE_LEXEME_CALL:
    case GDL_AST_ACTION_CREATE_SKIP_CALL:
    case GDL_AST_ACTION_CREATE_CHAINL1_CALL:
    case GDL_AST_ACTION_CREATE_CHAINR1_CALL:
    case GDL_AST_ACTION_CREATE_PASSTHRU_CALL:
    case GDL_AST_ACTION_CREATE_SEQUENCE:
    case GDL_AST_ACTION_CREATE_ALTERNATIVE:
    case GDL_AST_ACTION_CREATE_KEYWORD:
    case GDL_AST_ACTION_CREATE_TERMINAL:
    case GDL_AST_ACTION_CREATE_FAIL_CALL:
    case GDL_AST_ACTION_CREATE_OPTIONAL:
        push_placeholder_node(data);
        break;

    default:
        gdl_ast_builder_set_error(data, node, "Unknown AST action encountered in enter_node: %d", node->ast_config.action);
        break;
    }
}

static void
process_unary_call_action(
    epc_cpt_node_t * node,
    gdl_ast_builder_data_t * data,
    gdl_ast_node_type_t node_type
    )
{
    gdl_ast_node_t * expr_node = gdl_ast_stack_pop(data);
    gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

    if (expr_node == NULL || !is_placeholder_node(own_placeholder))
    {
        gdl_ast_builder_set_error(data, node, "Failed to build unary/optional combinator call: missing expression or bad placeholder.");
        gdl_ast_node_free(expr_node);
        gdl_ast_node_free(own_placeholder);
        return;
    }
    gdl_ast_node_free(own_placeholder);

    gdl_ast_node_t * result_node = gdl_ast_node_alloc(data, node_type);
    if (result_node)
    {
        result_node->data.unary_combinator_call.expr = expr_node;
    }
    gdl_ast_stack_push(data, result_node);
}

void
gdl_ast_builder_exit_node(epc_cpt_node_t * node, void * user_data)
{
#ifdef AST_DEBUG
    debug_indent -= 4;
#endif
    gdl_ast_builder_data_t * data = (gdl_ast_builder_data_t *)user_data;

    if (data->has_error)
    {
#ifdef AST_DEBUG
        if (!data->printed_error)
        {
            data->printed_error = true;
            fprintf(stderr, "%*sAST_DEBUG: Error exiting CPT node '%s' (action %d), stack size %d\n",
                    debug_indent, "", node->name, node->ast_config.action, data->stack_size);
            if (data->error_message[0] != '\0')
            {
                fprintf(stderr, "%*smsg: %s\n", debug_indent, "", data->error_message);
            }
        }
#endif
        return;
    }

    if (node == NULL)
    {
        return;
    }

#ifdef AST_DEBUG
    fprintf(stderr, "%*sAST_DEBUG: EXIT CPT node '%s' (action %d), stack size %d\n",
            debug_indent, "", node->name, node->ast_config.action, data->stack_size);
#endif

    gdl_ast_node_t * result_node = NULL; // The node to push back onto the stack

    switch (node->ast_config.action)
    {
    case GDL_AST_ACTION_CREATE_IDENTIFIER_REF:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for GDL_AST_ACTION_CREATE_IDENTIFIER_REF, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_IDENTIFIER_REF);
        if (ast_node)
        {
            ast_node->data.identifier_ref.name = get_cpt_node_text(node);
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_KEYWORD:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for GDL_AST_ACTION_CREATE_KEYWORD, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_KEYWORD);
        if (ast_node)
        {
            ast_node->data.keyword.name = get_cpt_node_text(node);
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_TERMINAL:
    {
        gdl_ast_node_t * terminal_expression_node = gdl_ast_stack_pop(data);

        if (is_placeholder_node(terminal_expression_node))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build terminal node: missing expression.");
            gdl_ast_node_free(terminal_expression_node);
            return;
        }

        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);
        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build terminal node: missing placeholder.");
            gdl_ast_node_free(terminal_expression_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_TERMINAL);
        if (ast_node)
        {
            ast_node->data.terminal.expression = terminal_expression_node;
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_SEMANTIC_ACTION:
    {
        gdl_ast_node_t * identifier = gdl_ast_stack_pop(data);

        if (identifier == NULL)
        {
            return;
        }
        if (is_placeholder_node(identifier) || identifier->type != GDL_AST_NODE_TYPE_IDENTIFIER_REF)
        {
            gdl_ast_builder_set_error(data, node, "Expected identifier node for SEMANTIC_ACTION, but got something else.");
            return;
        }

        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_node_free(identifier);
            gdl_ast_node_free(own_placeholder);
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for SEMANTIC_ACTION, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_SEMANTIC_ACTION);
        if (ast_node == NULL)
        {
            gdl_ast_node_free(identifier);
            gdl_ast_builder_set_error(data, node, "Memory allocation error");
            return;
        }
        ast_node->data.semantic_action.action_name = identifier->data.identifier_ref.name;
        identifier->data.identifier_ref.name = NULL;
        gdl_ast_node_free(identifier);
        gdl_ast_stack_push(data, ast_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_OPTIONAL_SEMANTIC_ACTION:
    {
        gdl_ast_node_t * popped_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * semantic_action = NULL;
        gdl_ast_node_t * own_placeholder = NULL;

        if (!is_placeholder_node(popped_node))
        {
            semantic_action = popped_node;
            own_placeholder = gdl_ast_stack_pop(data);
        }
        else
        {
            /* No optional semantic action was added. */
            own_placeholder = popped_node;
        }
        if (own_placeholder == NULL || !is_placeholder_node(own_placeholder))
        {
            gdl_ast_node_free(semantic_action);
            gdl_ast_node_free(own_placeholder);
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for SEMANTIC_ACTION_OR_EMPTY, but got something else.");
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * ast_node = semantic_action;

        if (semantic_action == NULL)
        {
            /* Create an empty semantic action node with no semantic action name to push onto the stack. */
            ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_SEMANTIC_ACTION);
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_REPETITION_OPERATOR:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_node_free(own_placeholder);
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for CREATE_REPETITION_OPERATOR, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_REPETITION_OPERATOR);
        if (ast_node)
        {
            ast_node->data.repetition_op.operator_char = epc_cpt_node_get_semantic_content(node)[0];
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_NUMBER_LITERAL:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for GDL_AST_ACTION_CREATE_NUMBER_LITERAL, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_NUMBER_LITERAL);
        if (ast_node)
        {
            ast_node->data.number_literal.value = atoll(epc_cpt_node_get_semantic_content(node));
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_CHAR_LITERAL:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for GDL_AST_ACTION_CREATE_CHAR_LITERAL, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_CHAR_LITERAL);
        if (ast_node)
        {
            // Assuming char literal includes quotes, so extract the char in between
            size_t len = epc_cpt_node_get_semantic_len(node);
            if (len >= 3 && epc_cpt_node_get_semantic_content(node)[0] == '\'' && epc_cpt_node_get_semantic_content(node)[len - 1] == '\'')
            {
                ast_node->data.char_literal.value = epc_cpt_node_get_semantic_content(node)[1];
            }
            else if (len >= 1)
            {
                ast_node->data.char_literal.value = epc_cpt_node_get_semantic_content(node)[0];
            }
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_STRING_LITERAL:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for GDL_AST_ACTION_CREATE_STRING_LITERAL, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_STRING_LITERAL);
        if (ast_node)
        {
            // Remove quotes from string literal
            size_t len = epc_cpt_node_get_semantic_len(node);
            char * temp_str = get_cpt_node_text(node); // Includes quotes
            if (temp_str)
            {
                // Assuming string literal includes quotes, so skip first and last char
                if (len >= 2 && temp_str[0] == '"' && temp_str[len - 1] == '"')
                {
                    ast_node->data.string_literal.value = strndup(temp_str + 1, len - 2);
                }
                else
                {
                    ast_node->data.string_literal.value = strdup(temp_str);
                }
                free(temp_str);
            }
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_RAW_CHAR_LITERAL:
    {
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Expected placeholder node for CREATE_RAW_CHAR_LITERAL, but got something else.");
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * ast_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_RAW_CHAR_LITERAL);
        if (ast_node)
        {
            ast_node->data.raw_char_literal.value = epc_cpt_node_get_semantic_content(node)[0];
        }
        gdl_ast_stack_push(data, ast_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_PROGRAM:
    {
        gdl_ast_node_t * rule_sequence_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (rule_sequence_node == NULL || rule_sequence_node->type != GDL_AST_NODE_TYPE_SEQUENCE)
        {
            gdl_ast_builder_set_error(data, node, "Expected sequence of rules for PROGRAM, but got something else.");
            gdl_ast_node_free(rule_sequence_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Internal error: bad placeholder for CREATE_PROGRAM");
            gdl_ast_node_free(rule_sequence_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free placeholder

        gdl_ast_node_t * program_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_PROGRAM);
        if (program_node)
        {
            program_node->data.program.rules = rule_sequence_node->data.sequence.elements;
            // Transfer ownership of list nodes
            rule_sequence_node->data.sequence.elements.head = NULL;
            rule_sequence_node->data.sequence.elements.tail = NULL;
            rule_sequence_node->data.sequence.elements.count = 0;
        }
        gdl_ast_node_free(rule_sequence_node); // Free the wrapper node (sequence node which held the list)
        data->ast_root = program_node; // This is the final root, not pushed back to stack
    }
        break;
    case GDL_AST_ACTION_CREATE_RULE_DEFINITION:
    {
        gdl_ast_node_t * semantic_action_node = NULL;
        gdl_ast_node_t * definition_node = NULL;
        gdl_ast_node_t * identifier_ref_node = NULL;
        gdl_ast_node_t * popped_node = gdl_ast_stack_pop(data);

        if (popped_node->type == GDL_AST_NODE_TYPE_SEMANTIC_ACTION)
        {
            semantic_action_node = popped_node;
            definition_node = gdl_ast_stack_pop(data);
        }
        else if (is_placeholder_node(popped_node))
        {
            gdl_ast_builder_set_error(data, node, "expected semantic action node or definition node, but got neither.");
            gdl_ast_node_free(popped_node);
            return;
        }
        else
        {
            definition_node = popped_node;
        }
        identifier_ref_node = gdl_ast_stack_pop(data);
        if (is_placeholder_node(identifier_ref_node) || identifier_ref_node->type != GDL_AST_NODE_TYPE_IDENTIFIER_REF)
        {
            gdl_ast_node_free(semantic_action_node);
            gdl_ast_node_free(definition_node);
            gdl_ast_node_free(identifier_ref_node);
            gdl_ast_builder_set_error(data, node, "Expected identifier node, but is was missing. Got %d instead", identifier_ref_node->type);
            return;
        }
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);
        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Internal error: missing placholder for RULE_DEFINITION");
            gdl_ast_node_free(semantic_action_node);
            gdl_ast_node_free(definition_node);
            gdl_ast_node_free(identifier_ref_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * rule_def_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_RULE_DEFINITION);
        if (rule_def_node)
        {
            rule_def_node->data.rule_def.name = identifier_ref_node->data.identifier_ref.name; // Transfer ownership
            identifier_ref_node->data.identifier_ref.name = NULL; // Prevent double free
            rule_def_node->data.rule_def.definition = definition_node;
            rule_def_node->data.rule_def.semantic_action = semantic_action_node;
        }
        gdl_ast_node_free(identifier_ref_node); // Free wrapper node (IdentifierRef node)
        gdl_ast_stack_push(data, rule_def_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_CHAR_RANGE:
    {
        gdl_ast_node_t * end_char_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * start_char_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!start_char_node
            || start_char_node->type != GDL_AST_NODE_TYPE_RAW_CHAR_LITERAL
            || !end_char_node
            || end_char_node->type != GDL_AST_NODE_TYPE_RAW_CHAR_LITERAL ||
            !is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build char range: missing char literals or bad placeholder.");
            gdl_ast_node_free(start_char_node);
            gdl_ast_node_free(end_char_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * char_range_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_CHAR_RANGE);
        if (char_range_node)
        {
            char_range_node->data.char_range.start_char = start_char_node->data.char_literal.value;
            char_range_node->data.char_range.end_char = end_char_node->data.char_literal.value;
        }
        gdl_ast_node_free(start_char_node);
        gdl_ast_node_free(end_char_node);
        gdl_ast_stack_push(data, char_range_node);
    }
        break;

    case GDL_AST_ACTION_COLLECT_ARGUMENTS:
    {
        gdl_ast_node_t * popped_node;
        gdl_ast_node_t * arg_list_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_ARGUMENT_LIST);
        if (arg_list_node == NULL)
        {
            return; // Error already set by gdl_ast_node_alloc
        }

        // Pop all AST nodes pushed by children until we find our own placeholder
        // And prepend them to the argument list to maintain correct order.
        while (!data->has_error && (popped_node = gdl_ast_stack_pop(data)) != NULL && !is_placeholder_node(popped_node))
        {
            gdl_ast_list_prepend(&arg_list_node->data.argument_list, popped_node);
        }
        if (data->has_error)
        {
            gdl_ast_node_free(arg_list_node); // Free the partially built list
            gdl_ast_node_free(popped_node); // Free the placeholder
            return;
        }
        gdl_ast_node_free(popped_node); // This is the placeholder for COLLECT_ARGUMENTS

        gdl_ast_stack_push(data, arg_list_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_EXPRESSION_FACTOR:
    {
        gdl_ast_node_t * optional_repetition_node = NULL;
        gdl_ast_node_t * primary_expression_node;

        gdl_ast_node_t * popped_node = gdl_ast_stack_pop(data);
        if (popped_node->type != GDL_AST_NODE_TYPE_OPTIONAL_EXPRESSION)
        {
            primary_expression_node = popped_node;
        }
        else
        {
            optional_repetition_node = popped_node;
            primary_expression_node = gdl_ast_stack_pop(data);
            if (is_placeholder_node(popped_node))
            {
                gdl_ast_builder_set_error(data, node, "Failed to build expression factor: missing primary expression or bad placeholder.");
                gdl_ast_node_free(primary_expression_node);
                gdl_ast_node_free(optional_repetition_node);
                return;
            }
        }
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build expression factor: missing primary expression or bad placeholder.");
            gdl_ast_node_free(primary_expression_node);
            gdl_ast_node_free(optional_repetition_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * repetition_op = NULL;
        gdl_ast_node_t * promoted_expr = primary_expression_node;

        // optional_repetition_node could be:
        // 1. NULL (if optional() wrapper around repetition_operator was not matched, or epc_optional_l did not push anything)
        // 2. A GDL_AST_NODE_TYPE_OPTIONAL_EXPRESSION wrapping a REPETITION_OPERATOR (if optional() matched)
        // 3. A PLACEHOLDER (if optional() did not match, but its placeholder was pushed)

        if (optional_repetition_node != NULL)
        {
            repetition_op = optional_repetition_node->data.optional.expr;
            optional_repetition_node->data.optional.expr = NULL;
            gdl_ast_node_free(optional_repetition_node); // Free the wrapper optional node
        }

        if (repetition_op != NULL)
        {
            result_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_REPETITION_EXPRESSION);
            if (result_node != NULL)
            {
                result_node->data.repetition_expr.expression = promoted_expr;
                result_node->data.repetition_expr.repetition = repetition_op;
            }
        }
        else
        {
            result_node = promoted_expr; // Promote primary expression if no repetition
        }
        gdl_ast_stack_push(data, result_node);
    }
        break;

        // Combinator Calls - build the node based on popped children
    case GDL_AST_ACTION_CREATE_ONEOF_CALL:
    case GDL_AST_ACTION_CREATE_NONEOF_CALL:
    {
        gdl_ast_node_t * args_list_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (args_list_node == NULL || args_list_node->type != GDL_AST_NODE_TYPE_ARGUMENT_LIST ||
            !is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build oneof/noneof call: missing argument list or bad placeholder.");
            gdl_ast_node_free(args_list_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);
        gdl_ast_node_type_t node_type =
                                        (node->ast_config.action == GDL_AST_ACTION_CREATE_ONEOF_CALL)
                                        ? GDL_AST_NODE_TYPE_COMBINATOR_ONEOF
                                        : GDL_AST_NODE_TYPE_COMBINATOR_NONEOF;
        result_node = gdl_ast_node_alloc(data, node_type);
        if (result_node)
        {
            if (result_node->type == GDL_AST_NODE_TYPE_COMBINATOR_ONEOF)
            {
                result_node->data.oneof_call.args = args_list_node->data.argument_list;
            }
            else
            {
                result_node->data.noneof_call.args = args_list_node->data.argument_list;
            }
            args_list_node->data.argument_list.head = NULL; // Transfer ownership
            gdl_ast_node_free(args_list_node);
        }
        gdl_ast_stack_push(data, result_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_COUNT_CALL:
    {
        gdl_ast_node_t * expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * count_val_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!expr_node || !count_val_node || count_val_node->type != GDL_AST_NODE_TYPE_NUMBER_LITERAL ||
            !is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build count call: missing expression, count value, or bad placeholder.");
            gdl_ast_node_free(expr_node);
            gdl_ast_node_free(count_val_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        result_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_COMBINATOR_COUNT);
        if (result_node)
        {
            result_node->data.count_call.count_node = count_val_node;
            result_node->data.count_call.expression = expr_node;
        }
        gdl_ast_stack_push(data, result_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_BETWEEN_CALL:
    {
        gdl_ast_node_t * close_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * content_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * open_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!open_expr_node || !content_expr_node || !close_expr_node || !is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build between call: missing expressions or bad placeholder.");
            gdl_ast_node_free(open_expr_node);
            gdl_ast_node_free(content_expr_node);
            gdl_ast_node_free(close_expr_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        result_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_COMBINATOR_BETWEEN);
        if (result_node)
        {
            result_node->data.between_call.open_expr = open_expr_node;
            result_node->data.between_call.content_expr = content_expr_node;
            result_node->data.between_call.close_expr = close_expr_node;
        }
        gdl_ast_stack_push(data, result_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_DELIMITED_CALL:
    {
        gdl_ast_node_t * delimiter_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * item_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!item_expr_node || !delimiter_expr_node || !is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build delimited call: missing expressions or bad placeholder.");
            gdl_ast_node_free(item_expr_node);
            gdl_ast_node_free(delimiter_expr_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        result_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_COMBINATOR_DELIMITED);
        if (result_node)
        {
            result_node->data.delimited_call.item_expr = item_expr_node;
            result_node->data.delimited_call.delimiter_expr = delimiter_expr_node;
        }
        gdl_ast_stack_push(data, result_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_LOOKAHEAD_CALL:
        process_unary_call_action(node, data, GDL_AST_NODE_TYPE_COMBINATOR_LOOKAHEAD);
        break;

    case GDL_AST_ACTION_CREATE_NOT_CALL:
        process_unary_call_action(node, data, GDL_AST_NODE_TYPE_COMBINATOR_NOT);
        break;

    case GDL_AST_ACTION_CREATE_LEXEME_CALL:
        process_unary_call_action(node, data, GDL_AST_NODE_TYPE_COMBINATOR_LEXEME);
        break;

    case GDL_AST_ACTION_CREATE_SKIP_CALL:
        process_unary_call_action(node, data, GDL_AST_NODE_TYPE_COMBINATOR_SKIP);
        break;

    case GDL_AST_ACTION_CREATE_PASSTHRU_CALL:
        process_unary_call_action(node, data, GDL_AST_NODE_TYPE_COMBINATOR_PASSTHRU);
        break;

    case GDL_AST_ACTION_CREATE_CHAINL1_CALL:
    case GDL_AST_ACTION_CREATE_CHAINR1_CALL:
    {
        gdl_ast_node_t * op_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * item_expr_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!item_expr_node || !op_expr_node || !is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Failed to build chainl1/chainr1 call: missing item, op expression, or bad placeholder.");
            gdl_ast_node_free(item_expr_node);
            gdl_ast_node_free(op_expr_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        result_node = gdl_ast_node_alloc(data, (node->ast_config.action == GDL_AST_ACTION_CREATE_CHAINL1_CALL) ? GDL_AST_NODE_TYPE_COMBINATOR_CHAINL1 : GDL_AST_NODE_TYPE_COMBINATOR_CHAINR1);
        if (result_node)
        {
            result_node->data.chain_combinator_call.item_expr = item_expr_node;
            result_node->data.chain_combinator_call.op_expr = op_expr_node;
        }
        gdl_ast_stack_push(data, result_node);
    }
        break;
    case GDL_AST_ACTION_CREATE_SEQUENCE:
    case GDL_AST_ACTION_CREATE_ALTERNATIVE:
    {
        gdl_ast_list_t collected_children = gdl_ast_list_init();
        gdl_ast_node_t * popped_node = NULL;
        // Pop all AST nodes pushed by children until we find our own placeholder
        while (!data->has_error && (popped_node = gdl_ast_stack_pop(data)) != NULL && !is_placeholder_node(popped_node))
        {
            gdl_ast_list_prepend(&collected_children, popped_node); // Use prepend to maintain order
        }
        if (data->has_error)
        {
            gdl_ast_list_free(&collected_children); // Free items as well
            gdl_ast_node_free(popped_node); // Free the placeholder
            return;
        }
        gdl_ast_node_free(popped_node); // This is the placeholder for CREATE_SEQUENCE/ALTERNATIVE

        // Proceed with creating the combined node as before (no flattening here)
        gdl_ast_node_t * combined_node = gdl_ast_node_alloc(data, (node->ast_config.action == GDL_AST_ACTION_CREATE_SEQUENCE) ? GDL_AST_NODE_TYPE_SEQUENCE : GDL_AST_NODE_TYPE_ALTERNATIVE);
        if (combined_node)
        {
            if (combined_node->type == GDL_AST_NODE_TYPE_SEQUENCE)
            {
                combined_node->data.sequence.elements = collected_children;
            }
            else
            {
                combined_node->data.alternative.alternatives = collected_children;
            }
            // collected_children's head/tail/count are now owned by combined_node
        }
        gdl_ast_stack_push(data, combined_node);
    }
        break;
    case GDL_AST_ACTION_NONE: // Should not be reached, handled by the if statement above.
        break;

    case GDL_AST_ACTION_CREATE_OPTIONAL:
    {
        gdl_ast_node_t * first_popped_node = gdl_ast_stack_pop(data);
        gdl_ast_node_t * optional_content_node = NULL;
        gdl_ast_node_t * own_placeholder = NULL;

        if (is_placeholder_node(first_popped_node))
        {
            // Case 1: Optional content did NOT match.
            // The first popped node is our own placeholder.
            own_placeholder = first_popped_node;
            optional_content_node = NULL; // No content node
        }
        else
        {
            // Case 2: Optional content DID match.
            // The first popped node is the AST node for the optional content.
            optional_content_node = first_popped_node;
            // Now pop our own placeholder
            own_placeholder = gdl_ast_stack_pop(data);
            if (!is_placeholder_node(own_placeholder))
            {
                gdl_ast_builder_set_error(data, node, "Internal error: Expected placeholder node after optional content, but got something else.");
                gdl_ast_node_free(optional_content_node);
                gdl_ast_node_free(own_placeholder); // Might not be placeholder, but try to free if allocated
                return;
            }
        }

        if (!is_placeholder_node(own_placeholder)) // Should be true at this point
        {
            gdl_ast_builder_set_error(data, node, "Internal error: Missing own placeholder for GDL_AST_ACTION_CREATE_OPTIONAL.");
            gdl_ast_node_free(optional_content_node);
            return;
        }
        gdl_ast_node_free(own_placeholder); // Free our own placeholder

        gdl_ast_node_t * optional_expr_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_OPTIONAL_EXPRESSION);
        if (optional_expr_node)
        {
            optional_expr_node->data.optional.expr = optional_content_node;
        }
        gdl_ast_stack_push(data, optional_expr_node);
    }
        break;

    case GDL_AST_ACTION_CREATE_FAIL_CALL:
    {
        gdl_ast_node_t * str_lit_node = gdl_ast_stack_pop(data);
        if (data->has_error)
        {
            return;
        }
        if (str_lit_node->type != GDL_AST_NODE_TYPE_STRING_LITERAL)
        {
            gdl_ast_builder_set_error(data, node, "Internal error: Expected STRING_LITERAL node for CREATE_FAIL_CALL, but got something else.");
            gdl_ast_node_free(str_lit_node);
            return;
        }

        gdl_ast_node_t * own_placeholder = gdl_ast_stack_pop(data);

        if (!is_placeholder_node(own_placeholder))
        {
            gdl_ast_builder_set_error(data, node, "Internal error: Expected placeholder node for CREATE_FAIL_CALL, but got something else.");
            gdl_ast_node_free(str_lit_node);
            gdl_ast_node_free(own_placeholder);
            return;
        }
        gdl_ast_node_free(own_placeholder);

        gdl_ast_node_t * fail_call_node = gdl_ast_node_alloc(data, GDL_AST_NODE_TYPE_FAIL_CALL);
        if (fail_call_node != NULL)
        {
            fail_call_node->data.string_literal.value = str_lit_node->data.string_literal.value;
            str_lit_node->data.string_literal.value = NULL;
            gdl_ast_stack_push(data, fail_call_node);
        }
        gdl_ast_node_free(str_lit_node);
        break;
    }

#if 0
    default:
        gdl_ast_builder_set_error(data, node, "Unknown AST action encountered in exit_node: %d", node->ast_config.action);
        break;
#endif
    }
}
