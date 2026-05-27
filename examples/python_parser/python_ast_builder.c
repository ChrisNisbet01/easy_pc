#include "python_ast_actions.h"
#include "python_ast_builder.h"
#include "python_ast_types.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Node allocation helpers ---

py_ast_node_t *
py_node_create(py_node_type_t type)
{
    py_ast_node_t * node = calloc(1, sizeof(*node));
    if (node)
        node->type = type;
    return node;
}

py_ast_node_t *
py_node_create_text(py_node_type_t type, char const * text, size_t len)
{
    py_ast_node_t * node = py_node_create(type);
    if (node)
        node->text = strndup(text, len);
    return node;
}

void
py_node_add_child(py_ast_node_t * parent, py_ast_node_t * child)
{
    if (!parent || !child)
        return;

    if (parent->children.count >= parent->children.capacity)
    {
        size_t new_cap = parent->children.capacity ? parent->children.capacity * 2 : 4;
        py_ast_node_t ** new_items = realloc(parent->children.items, new_cap * sizeof(py_ast_node_t *));
        if (!new_items)
            return;
        parent->children.items = new_items;
        parent->children.capacity = new_cap;
    }
    parent->children.items[parent->children.count++] = child;
}

void
py_node_free(py_ast_node_t * node)
{
    if (!node)
        return;
    free(node->text);
    for (size_t i = 0; i < node->children.count; i++)
        py_node_free(node->children.items[i]);
    free(node->children.items);
    free(node);
}

// --- Free callback for epc_ast ---

static void
py_node_free_cb(void * node, void * user_data)
{
    (void)user_data;
    py_node_free((py_ast_node_t *)node);
}

// --- Helper: create an operator node from a CPT node ---

static py_ast_node_t *
make_op(epc_cpt_node_t * node)
{
    char const * content = epc_cpt_node_get_content(node);
    size_t len = epc_cpt_node_get_content_len(node);
    return py_node_create_text(NODE_OP, content, len);
}

// --- Helper: free children array and set error ---

static void
set_error_free(epc_ast_builder_ctx_t * ctx, void ** children, int count, char const * msg)
{
    for (int i = 0; i < count; i++)
        py_node_free((py_ast_node_t *)children[i]);
    epc_ast_builder_set_error(ctx, "%s", msg);
}

// --- Operator callbacks ---

static void
op_add_or_sub(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_mul_div_mod_floor(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_unary(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_pow(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_shift(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_bit_and(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_bit_xor(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_bit_or(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_not_in(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    (void)node;
    epc_ast_push(ctx, py_node_create_text(NODE_OP, "not in", 6));
}

static void
op_is_not(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    (void)node;
    epc_ast_push(ctx, py_node_create_text(NODE_OP, "is not", 6));
}

static void
op_comp_single(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

static void
op_not(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    (void)node;
    epc_ast_push(ctx, py_node_create_text(NODE_OP, "not", 3));
}

static void
op_aug_assign(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    epc_ast_push(ctx, make_op(node));
}

// --- F-string action ---

static void
atom_fstring_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)user_data;
    // Free incoming children (Name node)
    for (int i = 0; i < count; i++)
        py_node_free((py_ast_node_t *)children[i]);
    // Create constant with full f-string text from CPT node
    char const * content = epc_cpt_node_get_content(node);
    size_t len = epc_cpt_node_get_content_len(node);
    epc_ast_push(ctx, py_node_create_text(NODE_CONSTANT, content, len));
}

// --- Name action ---

static void
name_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    char const * content = epc_cpt_node_get_content(node);
    size_t len = epc_cpt_node_get_content_len(node);
    epc_ast_push(ctx, py_node_create_text(NODE_NAME, content, len));
}

// --- Atom actions (now mostly passthrough since Name pushes) ---

static void
atom_passthrough(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "atom_passthrough expected 1 child");
        return;
    }
    epc_ast_push(ctx, children[0]);
}

static void
atom_constant_number(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    char const * content = epc_cpt_node_get_content(node);
    size_t len = epc_cpt_node_get_content_len(node);
    epc_ast_push(ctx, py_node_create_text(NODE_CONSTANT, content, len));
}

static void
atom_constant_string(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    char const * content = epc_cpt_node_get_content(node);
    size_t len = epc_cpt_node_get_content_len(node);
    epc_ast_push(ctx, py_node_create_text(NODE_CONSTANT, content, len));
}

static void
atom_constant_true(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    (void)node;
    epc_ast_push(ctx, py_node_create_text(NODE_CONSTANT, "True", 4));
}

static void
atom_constant_false(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    (void)node;
    epc_ast_push(ctx, py_node_create_text(NODE_CONSTANT, "False", 5));
}

static void
atom_constant_none(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)user_data;
    (void)node;
    epc_ast_push(ctx, py_node_create_text(NODE_CONSTANT, "None", 4));
}

// --- List/Dict/Tuple actions ---

static void
list_items(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
list_display(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "list_display expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_LIST;
    epc_ast_push(ctx, children[0]);
}

static void
dict_item_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "dict_item expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_DICT_ITEM);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

static void
dict_items_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
dict_display(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "dict_display expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_DICT;
    epc_ast_push(ctx, children[0]);
}

static void
tuple_items_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
tuple_display(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "tuple_display expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_TUPLE;
    epc_ast_push(ctx, children[0]);
}

static void
paren_expr(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "paren_expr expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_PAREN);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    epc_ast_push(ctx, n);
}

// --- Trail actions ---

static void
call_trail(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_CALL_TRAIL);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
attr_trail(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "attr_trail expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_ATTR_TRAIL);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    epc_ast_push(ctx, n);
}

static void
subscript_trail(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_SUBSCR_TRAIL);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
trails_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

// --- Primary: chain atom + trails ---

static void
primary_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "primary expected 2 children");
        return;
    }
    py_ast_node_t * atom = (py_ast_node_t *)children[0];
    py_ast_node_t * trails_list = (py_ast_node_t *)children[1];

    if (trails_list->children.count == 0)
    {
        py_node_free(trails_list);
        epc_ast_push(ctx, atom);
        return;
    }

    py_ast_node_t * current = atom;
    for (size_t i = 0; i < trails_list->children.count; i++)
    {
        py_ast_node_t * trail = trails_list->children.items[i];
        if (trail->type == NODE_CALL_TRAIL)
        {
            py_ast_node_t * call = py_node_create(NODE_CALL);
            py_node_add_child(call, current);
            for (size_t j = 0; j < trail->children.count; j++)
                py_node_add_child(call, trail->children.items[j]);
            trail->children.count = 0;
            py_node_free(trail);
            current = call;
        }
        else if (trail->type == NODE_ATTR_TRAIL)
        {
            py_ast_node_t * attr = py_node_create(NODE_ATTRIBUTE);
            py_node_add_child(attr, current);
            if (trail->children.count > 0)
                py_node_add_child(attr, trail->children.items[0]);
            trail->children.count = 0;
            py_node_free(trail);
            current = attr;
        }
        else if (trail->type == NODE_SUBSCR_TRAIL)
        {
            py_ast_node_t * sub = py_node_create(NODE_SUBSCRIPT);
            py_node_add_child(sub, current);
            for (size_t j = 0; j < trail->children.count; j++)
                py_node_add_child(sub, trail->children.items[j]);
            trail->children.count = 0;
            py_node_free(trail);
            current = sub;
        }
        else
        {
            py_node_add_child(current, trail);
        }
    }
    trails_list->children.count = 0;
    py_node_free(trails_list);
    epc_ast_push(ctx, current);
}

// --- Power ---

static void
power_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 1)
    {
        epc_ast_push(ctx, children[0]);
    }
    else if (count == 3)
    {
        py_ast_node_t * n = py_node_create(NODE_POWER);
        py_node_add_child(n, (py_ast_node_t *)children[0]);
        py_node_add_child(n, (py_ast_node_t *)children[1]);
        py_node_add_child(n, (py_ast_node_t *)children[2]);
        epc_ast_push(ctx, n);
    }
    else
    {
        set_error_free(ctx, children, count, "power expected 1 or 3 children");
    }
}

// --- Unary ---

static void
unary_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "unary expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_UNARYOP);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

// --- BinOp (chainl1) ---

static void
binop_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 1)
    {
        epc_ast_push(ctx, children[0]);
    }
    else if (count == 3)
    {
        py_ast_node_t * n = py_node_create(NODE_BINOP);
        py_node_add_child(n, (py_ast_node_t *)children[0]);
        py_node_add_child(n, (py_ast_node_t *)children[1]);
        py_node_add_child(n, (py_ast_node_t *)children[2]);
        epc_ast_push(ctx, n);
    }
    else
    {
        set_error_free(ctx, children, count, "binop expected 1 or 3 children");
    }
}

// --- Comparison ---

static void
comp_tails_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
compare_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "compare expected 2 children");
        return;
    }
    py_ast_node_t * left = (py_ast_node_t *)children[0];
    py_ast_node_t * tails = (py_ast_node_t *)children[1];

    if (tails->children.count == 0)
    {
        py_node_free(tails);
        epc_ast_push(ctx, left);
        return;
    }

    py_ast_node_t * n = py_node_create(NODE_COMPARE);
    py_node_add_child(n, left);
    for (size_t i = 0; i < tails->children.count; i++)
        py_node_add_child(n, tails->children.items[i]);
    tails->children.count = 0;
    py_node_free(tails);
    epc_ast_push(ctx, n);
}

// --- Not test ---

static void
not_test_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "not_test expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_UNARYOP);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

// --- And test ---

static void
and_tests_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
and_test_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "and_test expected 2 children");
        return;
    }
    py_ast_node_t * first = (py_ast_node_t *)children[0];
    py_ast_node_t * rest = (py_ast_node_t *)children[1];

    if (rest->children.count == 0)
    {
        py_node_free(rest);
        epc_ast_push(ctx, first);
        return;
    }

    py_ast_node_t * n = py_node_create(NODE_BOOLOP);
    py_node_add_child(n, first);
    py_node_add_child(n, py_node_create_text(NODE_OP, "and", 3));
    for (size_t i = 0; i < rest->children.count; i++)
        py_node_add_child(n, rest->children.items[i]);
    rest->children.count = 0;
    py_node_free(rest);
    epc_ast_push(ctx, n);
}

// --- Or test ---

static void
or_tests_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
or_test_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "or_test expected 2 children");
        return;
    }
    py_ast_node_t * first = (py_ast_node_t *)children[0];
    py_ast_node_t * rest = (py_ast_node_t *)children[1];

    if (rest->children.count == 0)
    {
        py_node_free(rest);
        epc_ast_push(ctx, first);
        return;
    }

    py_ast_node_t * n = py_node_create(NODE_BOOLOP);
    py_node_add_child(n, first);
    py_node_add_child(n, py_node_create_text(NODE_OP, "or", 2));
    for (size_t i = 0; i < rest->children.count; i++)
        py_node_add_child(n, rest->children.items[i]);
    rest->children.count = 0;
    py_node_free(rest);
    epc_ast_push(ctx, n);
}

// --- Ternary ---

static void
ternary_else_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "ternary_else expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    epc_ast_push(ctx, n);
}

static void
ternary_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 1)
    {
        epc_ast_push(ctx, children[0]);
    }
    else if (count == 3)
    {
        py_ast_node_t * else_wrapper = (py_ast_node_t *)children[2];
        py_ast_node_t * else_expr = else_wrapper->children.count > 0
            ? else_wrapper->children.items[0]
            : NULL;
        else_wrapper->children.count = 0;
        py_node_free(else_wrapper);

        py_ast_node_t * n = py_node_create(NODE_TERNARY);
        py_node_add_child(n, (py_ast_node_t *)children[0]);
        py_node_add_child(n, (py_ast_node_t *)children[1]);
        if (else_expr)
            py_node_add_child(n, else_expr);
        epc_ast_push(ctx, n);
    }
    else
    {
        set_error_free(ctx, children, count, "ternary expected 1 or 3 children");
    }
}

// --- Argument ---

static void
arg_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "arg expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_ARG);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    epc_ast_push(ctx, n);
}

static void
arglist_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

// --- Simple statements ---

static void
more_stmts_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
simple_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "simple_stmt expected 2 children");
        return;
    }
    py_node_free((py_ast_node_t *)children[1]);
    epc_ast_push(ctx, children[0]);
}

static void
expr_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "expr_stmt expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_EXPR_STMT);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    epc_ast_push(ctx, n);
}

static void
assign_tails_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
assign_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "assign expected 2 children");
        return;
    }
    py_ast_node_t * target = (py_ast_node_t *)children[0];
    py_ast_node_t * tails = (py_ast_node_t *)children[1];
    py_ast_node_t * n = py_node_create(NODE_ASSIGN);
    py_node_add_child(n, target);
    for (size_t i = 0; i < tails->children.count; i++)
        py_node_add_child(n, tails->children.items[i]);
    tails->children.count = 0;
    py_node_free(tails);
    epc_ast_push(ctx, n);
}

static void
aug_assign_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 3)
    {
        set_error_free(ctx, children, count, "aug_assign expected 3 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_AUG_ASSIGN);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    py_node_add_child(n, (py_ast_node_t *)children[2]);
    epc_ast_push(ctx, n);
}

static void
pass_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)node;
    (void)user_data;
    epc_ast_push(ctx, py_node_create(NODE_PASS));
}

static void
break_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)node;
    (void)user_data;
    epc_ast_push(ctx, py_node_create(NODE_BREAK));
}

static void
continue_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    (void)node;
    (void)user_data;
    epc_ast_push(ctx, py_node_create(NODE_CONTINUE));
}

static void
return_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_RETURN);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

// --- Import ---

static void
dotted_name_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_DOTTED_NAME);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
import_alias_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_IMPORT_ALIAS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
import_names_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
import_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "import expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_IMPORT;
    epc_ast_push(ctx, children[0]);
}

static void
from_names_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
from_import_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "from_import expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_FROM_IMPORT);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

// --- Global / Nonlocal ---

static void
global_names_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
global_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "global expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_GLOBAL;
    epc_ast_push(ctx, children[0]);
}

static void
nonlocal_names_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
nonlocal_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "nonlocal expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_NONLOCAL;
    epc_ast_push(ctx, children[0]);
}

// --- Assert ---

static void
assert_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ASSERT);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

// --- Raise ---

static void
raise_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_RAISE);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

// --- Del ---

static void
del_targets_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
del_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "del expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_DEL;
    epc_ast_push(ctx, children[0]);
}

// --- Yield ---

static void
yield_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_YIELD);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

// --- Compound statements ---

static void
stmts_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_STMTS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
block_suite_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "block_suite expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_SUITE);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    epc_ast_push(ctx, n);
}

// --- If ---

static void
elif_clause_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "elif_clause expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_ELIF_CLAUSE);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

static void
elifs_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ELIFS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
else_clause_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "else_clause expected 1 child");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_ELIF_CLAUSE);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    // Set type to NODE_EMPTY to indicate it's an else (no condition).
    // The printer will check: if it has 1 child (just suite), it's else.
    // For elif, it has 2 children (test + suite).
    // Actually, let me just mark the else with a different convention.
    // Store the suite under an EMPTY wrapper.
    n->type = NODE_EMPTY;
    epc_ast_push(ctx, n);
}

static void
opt_else_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        epc_ast_push(ctx, children[0]);
    }
}

static void
if_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 4)
    {
        set_error_free(ctx, children, count, "if_stmt expected 4 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_IF);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    py_node_add_child(n, (py_ast_node_t *)children[2]);
    py_node_add_child(n, (py_ast_node_t *)children[3]);
    epc_ast_push(ctx, n);
}

// --- While ---

static void
while_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "while expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_WHILE);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

// --- For ---

static void
for_targets_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
for_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 3)
    {
        set_error_free(ctx, children, count, "for expected 3 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_FOR);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    py_node_add_child(n, (py_ast_node_t *)children[2]);
    epc_ast_push(ctx, n);
}

// --- Function def ---

static void
params_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_PARAMS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
param_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_PARAM);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
opt_params_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        epc_ast_push(ctx, children[0]);
    }
}

static void
opt_return_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        epc_ast_push(ctx, children[0]);
    }
}

static void
func_def_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 4)
    {
        set_error_free(ctx, children, count, "func_def expected 4 children");
        return;
    }
    // children[0] = NODE_NAME (from Name)
    // children[1] = params (from opt_params)
    // children[2] = return type (from opt_return)
    // children[3] = suite
    py_ast_node_t * n = py_node_create(NODE_FUNC_DEF);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    py_node_add_child(n, (py_ast_node_t *)children[2]);
    py_node_add_child(n, (py_ast_node_t *)children[3]);
    epc_ast_push(ctx, n);
}

// --- Class def ---

static void
opt_args_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        epc_ast_push(ctx, children[0]);
    }
}

static void
class_def_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 3)
    {
        set_error_free(ctx, children, count, "class_def expected 3 children");
        return;
    }
    // children[0] = NODE_NAME
    // children[1] = opt_args (EMPTY or arglist)
    // children[2] = suite
    py_ast_node_t * n = py_node_create(NODE_CLASS_DEF);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    py_node_add_child(n, (py_ast_node_t *)children[2]);
    epc_ast_push(ctx, n);
}

// --- Try ---

static void
except_clause_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EXCEPT_CLAUSE);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
excepts_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_EMPTY);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
opt_excepts_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        epc_ast_push(ctx, children[0]);
    }
}

static void
opt_try_else_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        py_ast_node_t * n = py_node_create(NODE_EMPTY);
        py_node_add_child(n, (py_ast_node_t *)children[0]);
        epc_ast_push(ctx, n);
    }
}

static void
opt_try_finally_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count == 0)
    {
        epc_ast_push(ctx, py_node_create(NODE_EMPTY));
    }
    else
    {
        py_ast_node_t * n = py_node_create(NODE_EMPTY);
        py_node_add_child(n, (py_ast_node_t *)children[0]);
        epc_ast_push(ctx, n);
    }
}

static void
try_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 4)
    {
        set_error_free(ctx, children, count, "try expected 4 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_TRY);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    py_node_add_child(n, (py_ast_node_t *)children[2]);
    py_node_add_child(n, (py_ast_node_t *)children[3]);
    epc_ast_push(ctx, n);
}

// --- With ---

static void
with_item_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_WITH_ITEM);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
with_items_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_ARGS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
with_stmt_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 2)
    {
        set_error_free(ctx, children, count, "with expected 2 children");
        return;
    }
    py_ast_node_t * n = py_node_create(NODE_WITH);
    py_node_add_child(n, (py_ast_node_t *)children[0]);
    py_node_add_child(n, (py_ast_node_t *)children[1]);
    epc_ast_push(ctx, n);
}

// --- Top level ---

static void
opt_stmts_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    py_ast_node_t * n = py_node_create(NODE_STMTS);
    for (int i = 0; i < count; i++)
        py_node_add_child(n, (py_ast_node_t *)children[i]);
    epc_ast_push(ctx, n);
}

static void
module_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)node;
    (void)user_data;
    if (count != 1)
    {
        set_error_free(ctx, children, count, "module expected 1 child");
        return;
    }
    ((py_ast_node_t *)children[0])->type = NODE_MODULE;
    epc_ast_push(ctx, children[0]);
}

// --- Registry init ---

void
py_ast_hook_registry_init(epc_ast_hook_registry_t * registry)
{
    epc_ast_hook_registry_set_free_node(registry, py_node_free_cb);

    epc_ast_hook_registry_set_action(registry, AST_OP_ADD_OR_SUB, op_add_or_sub);
    epc_ast_hook_registry_set_action(registry, AST_OP_MUL_DIV_MOD_FLOOR, op_mul_div_mod_floor);
    epc_ast_hook_registry_set_action(registry, AST_OP_UNARY, op_unary);
    epc_ast_hook_registry_set_action(registry, AST_OP_POW, op_pow);
    epc_ast_hook_registry_set_action(registry, AST_OP_SHIFT, op_shift);
    epc_ast_hook_registry_set_action(registry, AST_OP_BIT_AND, op_bit_and);
    epc_ast_hook_registry_set_action(registry, AST_OP_BIT_XOR, op_bit_xor);
    epc_ast_hook_registry_set_action(registry, AST_OP_BIT_OR, op_bit_or);
    epc_ast_hook_registry_set_action(registry, AST_OP_NOT_IN, op_not_in);
    epc_ast_hook_registry_set_action(registry, AST_OP_IS_NOT, op_is_not);
    epc_ast_hook_registry_set_action(registry, AST_OP_COMP_SINGLE, op_comp_single);
    epc_ast_hook_registry_set_action(registry, AST_OP_NOT, op_not);
    epc_ast_hook_registry_set_action(registry, AST_OP_AUG_ASSIGN, op_aug_assign);
    epc_ast_hook_registry_set_action(registry, AST_LIST_ITEMS, list_items);
    epc_ast_hook_registry_set_action(registry, AST_LIST, list_display);
    epc_ast_hook_registry_set_action(registry, AST_DICT_ITEM, dict_item_action);
    epc_ast_hook_registry_set_action(registry, AST_DICT_ITEMS, dict_items_action);
    epc_ast_hook_registry_set_action(registry, AST_DICT, dict_display);
    epc_ast_hook_registry_set_action(registry, AST_TUPLE_ITEMS, tuple_items_action);
    epc_ast_hook_registry_set_action(registry, AST_TUPLE, tuple_display);
    epc_ast_hook_registry_set_action(registry, AST_PAREN_EXPR, paren_expr);
    epc_ast_hook_registry_set_action(registry, AST_NAME, name_action);
    epc_ast_hook_registry_set_action(registry, AST_ATOM_NAME, atom_passthrough);
    epc_ast_hook_registry_set_action(registry, AST_CONSTANT_NUMBER, atom_constant_number);
    epc_ast_hook_registry_set_action(registry, AST_CONSTANT_STRING, atom_constant_string);
    epc_ast_hook_registry_set_action(registry, AST_CONSTANT_TRUE, atom_constant_true);
    epc_ast_hook_registry_set_action(registry, AST_CONSTANT_FALSE, atom_constant_false);
    epc_ast_hook_registry_set_action(registry, AST_CONSTANT_NONE, atom_constant_none);
    epc_ast_hook_registry_set_action(registry, AST_FSTRING, atom_fstring_action);
    epc_ast_hook_registry_set_action(registry, AST_CALL_TRAIL, call_trail);
    epc_ast_hook_registry_set_action(registry, AST_ATTR_TRAIL, attr_trail);
    epc_ast_hook_registry_set_action(registry, AST_SUBSCR_TRAIL, subscript_trail);
    epc_ast_hook_registry_set_action(registry, AST_TRAILS, trails_action);
    epc_ast_hook_registry_set_action(registry, AST_PRIMARY, primary_action);
    epc_ast_hook_registry_set_action(registry, AST_POWER, power_action);
    epc_ast_hook_registry_set_action(registry, AST_UNARY, unary_action);
    epc_ast_hook_registry_set_action(registry, AST_BINOP, binop_action);
    epc_ast_hook_registry_set_action(registry, AST_COMP_TAILS, comp_tails_action);
    epc_ast_hook_registry_set_action(registry, AST_COMPARE, compare_action);
    epc_ast_hook_registry_set_action(registry, AST_NOT_TEST, not_test_action);
    epc_ast_hook_registry_set_action(registry, AST_AND_TESTS, and_tests_action);
    epc_ast_hook_registry_set_action(registry, AST_AND_TEST, and_test_action);
    epc_ast_hook_registry_set_action(registry, AST_OR_TESTS, or_tests_action);
    epc_ast_hook_registry_set_action(registry, AST_OR_TEST, or_test_action);
    epc_ast_hook_registry_set_action(registry, AST_TERNARY_ELSE, ternary_else_action);
    epc_ast_hook_registry_set_action(registry, AST_TERNARY, ternary_action);
    epc_ast_hook_registry_set_action(registry, AST_ARG, arg_action);
    epc_ast_hook_registry_set_action(registry, AST_ARGLIST, arglist_action);
    epc_ast_hook_registry_set_action(registry, AST_MORE_STMTS, more_stmts_action);
    epc_ast_hook_registry_set_action(registry, AST_SIMPLE_STMT, simple_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_EXPR_STMT, expr_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_ASSIGN_TAILS, assign_tails_action);
    epc_ast_hook_registry_set_action(registry, AST_ASSIGN, assign_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_AUG_ASSIGN, aug_assign_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_PASS, pass_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_BREAK, break_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_CONTINUE, continue_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_RETURN, return_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_IMPORT_NAMES, import_names_action);
    epc_ast_hook_registry_set_action(registry, AST_IMPORT, import_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_FROM_NAMES, from_names_action);
    epc_ast_hook_registry_set_action(registry, AST_FROM_IMPORT, from_import_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_DOTTED_NAME, dotted_name_action);
    epc_ast_hook_registry_set_action(registry, AST_IMPORT_ALIAS, import_alias_action);
    epc_ast_hook_registry_set_action(registry, AST_GLOBAL_NAMES, global_names_action);
    epc_ast_hook_registry_set_action(registry, AST_GLOBAL, global_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_NONLOCAL_NAMES, nonlocal_names_action);
    epc_ast_hook_registry_set_action(registry, AST_NONLOCAL, nonlocal_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_ASSERT, assert_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_RAISE, raise_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_DEL_TARGETS, del_targets_action);
    epc_ast_hook_registry_set_action(registry, AST_DEL, del_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_YIELD, yield_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_STMTS, stmts_action);
    epc_ast_hook_registry_set_action(registry, AST_BLOCK_SUITE, block_suite_action);
    epc_ast_hook_registry_set_action(registry, AST_ELIF_CLAUSE, elif_clause_action);
    epc_ast_hook_registry_set_action(registry, AST_ELIFS, elifs_action);
    epc_ast_hook_registry_set_action(registry, AST_ELSE_CLAUSE, else_clause_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_ELSE, opt_else_action);
    epc_ast_hook_registry_set_action(registry, AST_IF, if_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_WHILE, while_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_FOR_TARGETS, for_targets_action);
    epc_ast_hook_registry_set_action(registry, AST_FOR, for_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_PARAMS, params_action);
    epc_ast_hook_registry_set_action(registry, AST_PARAM, param_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_PARAMS, opt_params_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_RETURN, opt_return_action);
    epc_ast_hook_registry_set_action(registry, AST_FUNC_DEF, func_def_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_ARGS, opt_args_action);
    epc_ast_hook_registry_set_action(registry, AST_CLASS_DEF, class_def_action);
    epc_ast_hook_registry_set_action(registry, AST_EXCEPT_CLAUSE, except_clause_action);
    epc_ast_hook_registry_set_action(registry, AST_EXCEPTS, excepts_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_EXCEPTS, opt_excepts_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_TRY_ELSE, opt_try_else_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_TRY_FINALLY, opt_try_finally_action);
    epc_ast_hook_registry_set_action(registry, AST_TRY, try_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_WITH_ITEMS, with_items_action);
    epc_ast_hook_registry_set_action(registry, AST_WITH_ITEM, with_item_action);
    epc_ast_hook_registry_set_action(registry, AST_WITH, with_stmt_action);
    epc_ast_hook_registry_set_action(registry, AST_OPT_STMTS, opt_stmts_action);
    epc_ast_hook_registry_set_action(registry, AST_MODULE, module_action);
}
