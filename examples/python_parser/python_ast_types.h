#pragma once

#include <stddef.h>
#include <stdbool.h>

typedef enum
{
    NODE_MODULE,
    NODE_STMTS,
    NODE_SUITE,
    NODE_EXPR_STMT,
    NODE_ASSIGN,
    NODE_AUG_ASSIGN,
    NODE_RETURN,
    NODE_PASS,
    NODE_BREAK,
    NODE_CONTINUE,
    NODE_IMPORT,
    NODE_FROM_IMPORT,
    NODE_GLOBAL,
    NODE_NONLOCAL,
    NODE_ASSERT,
    NODE_RAISE,
    NODE_DEL,
    NODE_YIELD,
    NODE_IF,
    NODE_ELIF_CLAUSE,
    NODE_ELIFS,
    NODE_WHILE,
    NODE_FOR,
    NODE_FUNC_DEF,
    NODE_CLASS_DEF,
    NODE_TRY,
    NODE_EXCEPT_CLAUSE,
    NODE_WITH,
    NODE_WITH_ITEM,
    NODE_BINOP,
    NODE_UNARYOP,
    NODE_BOOLOP,
    NODE_COMPARE,
    NODE_POWER,
    NODE_TERNARY,
    NODE_CALL,
    NODE_ATTRIBUTE,
    NODE_SUBSCRIPT,
    NODE_NAME,
    NODE_CONSTANT,
    NODE_LIST,
    NODE_DICT,
    NODE_TUPLE,
    NODE_PAREN,
    NODE_ARG,
    NODE_ARGS,
    NODE_PARAM,
    NODE_PARAMS,
    NODE_DOTTED_NAME,
    NODE_IMPORT_ALIAS,
    NODE_OP,
    NODE_TEXT,
    NODE_EMPTY,
    NODE_CALL_TRAIL,
    NODE_ATTR_TRAIL,
    NODE_SUBSCR_TRAIL,
    NODE_DICT_ITEM,
} py_node_type_t;

typedef struct py_ast_node py_ast_node_t;

typedef struct
{
    py_ast_node_t ** items;
    size_t count;
    size_t capacity;
} py_ast_node_list_t;

struct py_ast_node
{
    py_node_type_t type;
    char * text;
    py_ast_node_list_t children;
};

py_ast_node_t *
py_node_create(py_node_type_t type);

py_ast_node_t *
py_node_create_text(py_node_type_t type, char const * text, size_t len);

void
py_node_add_child(py_ast_node_t * parent, py_ast_node_t * child);

void
py_node_free(py_ast_node_t * node);
