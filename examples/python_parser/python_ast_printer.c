#include "python_ast_printer.h"

#include <stdio.h>
#include <string.h>

static void
print_indent(int indent)
{
    for (int i = 0; i < indent; i++)
        putchar(' ');
}

static void
print_children(py_ast_node_t * node, int indent, char const * sep)
{
    for (size_t i = 0; i < node->children.count; i++)
    {
        if (i > 0 && sep)
            printf("%s", sep);
        py_ast_print(node->children.items[i], indent);
    }
}

static bool
is_empty(py_ast_node_t * node)
{
    return node->type == NODE_EMPTY;
}

void
py_ast_print(py_ast_node_t * node, int indent)
{
    if (!node)
        return;

    switch (node->type)
    {
    // --- Literals and names ---
    case NODE_NAME:
    case NODE_CONSTANT:
    case NODE_OP:
    case NODE_TEXT:
        if (node->text)
            printf("%s", node->text);
        break;

    // --- Module / Statements ---
    case NODE_MODULE:
        for (size_t i = 0; i < node->children.count; i++)
        {
            print_indent(indent);
            py_ast_print(node->children.items[i], indent);
        }
        putchar('\n');
        break;

    case NODE_STMTS:
        for (size_t i = 0; i < node->children.count; i++)
        {
            print_indent(indent);
            py_ast_print(node->children.items[i], indent);
        }
        break;

    case NODE_SUITE:
        if (node->children.count > 0)
            py_ast_print(node->children.items[0], indent);
        break;

    // --- Expression statements ---
    case NODE_EXPR_STMT:
        print_children(node, indent, NULL);
        putchar('\n');
        break;

    // --- Assignment ---
    case NODE_ASSIGN:
        if (node->children.count >= 2)
        {
            py_ast_print(node->children.items[0], indent);
            for (size_t i = 1; i < node->children.count; i++)
            {
                printf(" = ");
                py_ast_print(node->children.items[i], indent);
            }
        }
        putchar('\n');
        break;

    case NODE_AUG_ASSIGN:
        if (node->children.count >= 3)
        {
            py_ast_print(node->children.items[0], indent);
            printf(" ");
            py_ast_print(node->children.items[1], indent);
            printf(" ");
            py_ast_print(node->children.items[2], indent);
        }
        putchar('\n');
        break;

    // --- Simple statements ---
    case NODE_PASS:
        printf("pass\n");
        break;

    case NODE_BREAK:
        printf("break\n");
        break;

    case NODE_CONTINUE:
        printf("continue\n");
        break;

    case NODE_RETURN:
        printf("return");
        if (node->children.count > 0)
        {
            putchar(' ');
            print_children(node, indent, " ");
        }
        putchar('\n');
        break;

    case NODE_IMPORT:
        printf("import ");
        print_children(node, indent, ", ");
        putchar('\n');
        break;

    case NODE_FROM_IMPORT:
        if (node->children.count >= 2)
        {
            printf("from ");
            py_ast_print(node->children.items[0], indent);
            printf(" import ");
            py_ast_print(node->children.items[1], indent);
        }
        putchar('\n');
        break;

    case NODE_GLOBAL:
        printf("global ");
        print_children(node, indent, ", ");
        putchar('\n');
        break;

    case NODE_NONLOCAL:
        printf("nonlocal ");
        print_children(node, indent, ", ");
        putchar('\n');
        break;

    case NODE_ASSERT:
        printf("assert ");
        print_children(node, indent, ", ");
        putchar('\n');
        break;

    case NODE_RAISE:
        printf("raise");
        if (node->children.count > 0)
        {
            putchar(' ');
            print_children(node, indent, " from ");
        }
        putchar('\n');
        break;

    case NODE_DEL:
        printf("del ");
        print_children(node, indent, ", ");
        putchar('\n');
        break;

    case NODE_YIELD:
        printf("yield");
        if (node->children.count > 0)
        {
            putchar(' ');
            print_children(node, indent, " ");
        }
        putchar('\n');
        break;

    // --- Dotted name / Import alias ---
    case NODE_DOTTED_NAME:
        print_children(node, indent, ".");
        break;

    case NODE_IMPORT_ALIAS:
        print_children(node, indent, " as ");
        break;

    // --- Expressions ---
    case NODE_BINOP:
        if (node->children.count == 3)
        {
            py_ast_print(node->children.items[0], indent);
            printf(" ");
            py_ast_print(node->children.items[1], indent);
            printf(" ");
            py_ast_print(node->children.items[2], indent);
        }
        break;

    case NODE_UNARYOP:
        if (node->children.count == 2)
        {
            py_ast_print(node->children.items[0], indent);
            py_ast_print(node->children.items[1], indent);
        }
        break;

    case NODE_BOOLOP:
        if (node->children.count >= 2)
        {
            py_ast_print(node->children.items[0], indent);
            for (size_t i = 2; i < node->children.count; i++)
            {
                printf(" ");
                py_ast_print(node->children.items[1], indent);
                printf(" ");
                py_ast_print(node->children.items[i], indent);
            }
        }
        break;

    case NODE_COMPARE:
        if (node->children.count >= 1)
        {
            py_ast_print(node->children.items[0], indent);
            for (size_t i = 1; i < node->children.count; i += 2)
            {
                printf(" ");
                if (i < node->children.count)
                    py_ast_print(node->children.items[i], indent);
                printf(" ");
                if (i + 1 < node->children.count)
                    py_ast_print(node->children.items[i + 1], indent);
            }
        }
        break;

    case NODE_POWER:
        if (node->children.count == 3)
        {
            py_ast_print(node->children.items[0], indent);
            printf(" ** ");
            py_ast_print(node->children.items[2], indent);
        }
        break;

    case NODE_TERNARY:
        if (node->children.count == 3)
        {
            py_ast_print(node->children.items[0], indent);
            printf(" if ");
            py_ast_print(node->children.items[1], indent);
            printf(" else ");
            py_ast_print(node->children.items[2], indent);
        }
        break;

    // --- Call / Attribute / Subscript ---
    case NODE_CALL:
        if (node->children.count >= 1)
        {
            py_ast_print(node->children.items[0], indent);
            putchar('(');
            // Args start at index 1
            for (size_t i = 1; i < node->children.count; i++)
            {
                if (i > 1)
                    printf(", ");
                py_ast_print(node->children.items[i], indent);
            }
            putchar(')');
        }
        break;

    case NODE_ATTRIBUTE:
        if (node->children.count >= 2)
        {
            py_ast_print(node->children.items[0], indent);
            putchar('.');
            py_ast_print(node->children.items[1], indent);
        }
        break;

    case NODE_SUBSCRIPT:
        if (node->children.count >= 2)
        {
            py_ast_print(node->children.items[0], indent);
            putchar('[');
            py_ast_print(node->children.items[1], indent);
            putchar(']');
        }
        break;

    // --- Collections ---
    case NODE_LIST:
        putchar('[');
        {
            size_t n = node->children.count;
            for (size_t i = 0; i < n; i++)
            {
                if (i > 0)
                    printf(", ");
                py_ast_print(node->children.items[i], indent);
            }
        }
        putchar(']');
        break;

    case NODE_DICT:
        putchar('{');
        {
            size_t n = node->children.count;
            for (size_t i = 0; i < n; i++)
            {
                if (i > 0)
                    printf(", ");
                py_ast_print(node->children.items[i], indent);
            }
        }
        putchar('}');
        break;

    case NODE_DICT_ITEM:
        if (node->children.count >= 2)
        {
            py_ast_print(node->children.items[0], indent);
            printf(": ");
            py_ast_print(node->children.items[1], indent);
        }
        break;

    case NODE_TUPLE:
        putchar('(');
        {
            size_t n = node->children.count;
            for (size_t i = 0; i < n; i++)
            {
                if (i > 0)
                    printf(", ");
                py_ast_print(node->children.items[i], indent);
            }
        }
        putchar(')');
        break;

    case NODE_PAREN:
        putchar('(');
        if (node->children.count > 0)
            py_ast_print(node->children.items[0], indent);
        putchar(')');
        break;

    // --- Args ---
    case NODE_ARG:
        if (node->children.count > 0)
            py_ast_print(node->children.items[0], indent);
        break;

    case NODE_ARGS:
        print_children(node, indent, ", ");
        break;

    // --- Params ---
    case NODE_PARAM:
        if (node->children.count > 0)
            py_ast_print(node->children.items[0], indent);
        for (size_t i = 1; i < node->children.count; i++)
        {
            printf(" = ");
            py_ast_print(node->children.items[i], indent);
        }
        break;

    case NODE_PARAMS:
        putchar('(');
        print_children(node, indent, ", ");
        putchar(')');
        break;

    // --- Compound statements ---
    case NODE_IF:
        if (node->children.count >= 2)
        {
            printf("if ");
            py_ast_print(node->children.items[0], indent);
            printf(":\n");
            py_ast_print(node->children.items[1], indent + 4);
            // Elifs
            if (node->children.count >= 3 && !is_empty(node->children.items[2]))
            {
                py_ast_node_t * elifs = node->children.items[2];
                for (size_t i = 0; i < elifs->children.count; i++)
                {
                    py_ast_node_t * elif_c = elifs->children.items[i];
                    if (elif_c->children.count == 2)
                    {
                        printf("elif ");
                        py_ast_print(elif_c->children.items[0], indent);
                        printf(":\n");
                        py_ast_print(elif_c->children.items[1], indent + 4);
                    }
                }
            }
            // Else
            if (node->children.count >= 4 && !is_empty(node->children.items[3]))
            {
                printf("else:\n");
                py_ast_node_t * else_wrapper = node->children.items[3];
                if (else_wrapper->children.count > 0)
                    py_ast_print(else_wrapper->children.items[0], indent + 4);
            }
        }
        break;

    case NODE_WHILE:
        if (node->children.count >= 2)
        {
            printf("while ");
            py_ast_print(node->children.items[0], indent);
            printf(":\n");
            py_ast_print(node->children.items[1], indent + 4);
        }
        break;

    case NODE_FOR:
        if (node->children.count >= 3)
        {
            printf("for ");
            py_ast_print(node->children.items[0], indent);
            printf(" in ");
            py_ast_print(node->children.items[1], indent);
            printf(":\n");
            py_ast_print(node->children.items[2], indent + 4);
        }
        break;

    case NODE_FUNC_DEF:
        if (node->children.count >= 4)
        {
            printf("def ");
            py_ast_print(node->children.items[0], indent);
            py_ast_print(node->children.items[1], indent);
            if (!is_empty(node->children.items[2]))
            {
                printf(" -> ");
                py_ast_print(node->children.items[2], indent);
            }
            printf(":\n");
            py_ast_print(node->children.items[3], indent + 4);
        }
        break;

    case NODE_CLASS_DEF:
        if (node->children.count >= 3)
        {
            printf("class ");
            py_ast_print(node->children.items[0], indent);
            if (!is_empty(node->children.items[1]))
            {
                putchar('(');
                py_ast_print(node->children.items[1], indent);
                putchar(')');
            }
            printf(":\n");
            py_ast_print(node->children.items[2], indent + 4);
        }
        break;

    case NODE_TRY:
        if (node->children.count >= 4)
        {
            printf("try:\n");
            py_ast_print(node->children.items[0], indent + 4);
            if (!is_empty(node->children.items[1]))
            {
                py_ast_node_t * excepts = node->children.items[1];
                for (size_t i = 0; i < excepts->children.count; i++)
                {
                    py_ast_node_t * exc = excepts->children.items[i];
                    printf("except");
                    if (exc->children.count > 0)
                    {
                        putchar(' ');
                        py_ast_print(exc->children.items[0], indent);
                    }
                    if (exc->children.count > 1)
                    {
                        printf(" as ");
                        py_ast_print(exc->children.items[1], indent);
                    }
                    printf(":\n");
                    if (exc->children.count > 0)
                        py_ast_print(exc->children.items[exc->children.count - 1], indent + 4);
                }
            }
            if (!is_empty(node->children.items[2]))
            {
                printf("else:\n");
                py_ast_node_t * else_node = node->children.items[2];
                if (else_node->children.count > 0)
                    py_ast_print(else_node->children.items[0], indent + 4);
            }
            if (!is_empty(node->children.items[3]))
            {
                printf("finally:\n");
                py_ast_node_t * finally_node = node->children.items[3];
                if (finally_node->children.count > 0)
                    py_ast_print(finally_node->children.items[0], indent + 4);
            }
        }
        break;

    case NODE_WITH:
        if (node->children.count >= 2)
        {
            printf("with ");
            py_ast_print(node->children.items[0], indent);
            printf(":\n");
            py_ast_print(node->children.items[1], indent + 4);
        }
        break;

    case NODE_WITH_ITEM:
        if (node->children.count >= 1)
        {
            py_ast_print(node->children.items[0], indent);
            if (node->children.count >= 2)
            {
                printf(" as ");
                py_ast_print(node->children.items[1], indent);
            }
        }
        break;

    // --- Internal / sentinel types ---
    case NODE_ELIF_CLAUSE:
        // This shouldn't be directly printed; handled by parent
        break;

    case NODE_ELIFS:
        break;

    case NODE_EXCEPT_CLAUSE:
        break;

    case NODE_EMPTY:
        break;

    case NODE_CALL_TRAIL:
    case NODE_ATTR_TRAIL:
    case NODE_SUBSCR_TRAIL:
        // These are consumed by primary_action and should not appear in final AST
        break;
    }
}
