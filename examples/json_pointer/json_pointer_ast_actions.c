#include "json_pointer_ast_actions.h"
#include "json_pointer_ast.h"
#include "json_pointer_actions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static json_pointer_node_t *
json_pointer_node_alloc(json_pointer_node_type_t type)
{
    json_pointer_node_t * node = calloc(1, sizeof(*node));
    if (node)
    {
        node->type = type;
    }
    return node;
}

void
json_pointer_node_free(void * node_ptr, void * user_data)
{
    json_pointer_node_t * node = (json_pointer_node_t *)node_ptr;
    if (node == NULL)
    {
        return;
    }

    switch (node->type)
    {
        case JSON_POINTER_NODE_STRING:
            free(node->data.string);
            break;

        case JSON_POINTER_NODE_LIST:
        {
            json_pointer_list_node_t * curr = node->data.list.head;
            while (curr)
            {
                json_pointer_list_node_t * next = curr->next;
                json_pointer_node_free(curr->item, user_data);
                free(curr);
                curr = next;
            }
            break;

        case JSON_POINTER_NODE_CHAR:
            /* Nothing to do. */
            break;
        }
    }
    free(node);
}

static void
ast_list_append(json_pointer_list_t * list, json_pointer_node_t * item)
{
    json_pointer_list_node_t * new_node = calloc(1, sizeof(*new_node));
    if (new_node == NULL)
    {
        return;
    }
    new_node->item = item;
    new_node->next = NULL;

    if (list->tail)
    {
        list->tail->next = new_node;
    }
    else
    {
        list->head = new_node;
    }
    list->tail = new_node;
    list->count++;
}

static void
free_children(void * * children, int count, void * user_data)
{
    for (int i = 0; i < count; i++)
    {
        if (children[i] != NULL)
        {
            json_pointer_node_free((json_pointer_node_t *)children[i], user_data);
        }
    }
}

/* --- Semantic Action Callbacks --- */

static void
create_escaped_token_action(
    epc_ast_builder_ctx_t * ctx,
    epc_cpt_node_t * node,
    void * * children,
    int count,
    void * user_data
)
{
    size_t semantic_len = epc_cpt_node_get_semantic_len(node);
    if (semantic_len != 2)
    {
        epc_ast_builder_set_error(ctx, "Create escaped token expected 2 chars, but got %zu", semantic_len);
        return;
    }
    json_pointer_node_t * jpnode = json_pointer_node_alloc(JSON_POINTER_NODE_CHAR);

    char escaped_ch = epc_cpt_node_get_semantic_content(node)[1];
    if (escaped_ch == '0')
    {
        jpnode->data.ch = '~';
    }
    else if (escaped_ch == '1')
    {
        jpnode->data.ch = '/';
    }
    else
    {
        epc_ast_builder_set_error(ctx, "Got unsupported excaped token '%c'", escaped_ch);
        return;
    }

    epc_ast_push(ctx, jpnode);
}

static void
create_unescaped_token_action(
    epc_ast_builder_ctx_t * ctx,
    epc_cpt_node_t * node,
    void * * children,
    int count,
    void * user_data
)
{
    size_t semantic_len = epc_cpt_node_get_semantic_len(node);
    if (semantic_len != 1)
    {
        epc_ast_builder_set_error(ctx, "Create unescaped token expected 1 char, but got %zu", semantic_len);
        return;
    }
    json_pointer_node_t * jpnode = json_pointer_node_alloc(JSON_POINTER_NODE_CHAR);

    jpnode->data.ch = epc_cpt_node_get_semantic_content(node)[0];

    epc_ast_push(ctx, jpnode);
}

static void
create_optional_token_action(
    epc_ast_builder_ctx_t * ctx,
    epc_cpt_node_t * node,
    void * * children,
    int count,
    void * user_data
)
{
    json_pointer_node_t * jpnode = json_pointer_node_alloc(JSON_POINTER_NODE_STRING);

    if (jpnode == NULL)
    {
        free_children(children, count, user_data);
        epc_ast_builder_set_error(ctx, "Failed to allocate JSON pointer string node");
        return;
    }

    jpnode->data.string = malloc(count + 1);
    if (jpnode->data.string == NULL)
    {
        free_children(children, count, user_data);
        json_pointer_node_free(jpnode, user_data);
        epc_ast_builder_set_error(ctx, "Failed to allocate JSON pointer string");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        json_pointer_node_t * child = children[i];
        jpnode->data.string[i] = child->data.ch;
    }
    jpnode->data.string[count] = '\0';

    epc_ast_push(ctx, jpnode);
}

static void
collect_optional_token_list_action(
    epc_ast_builder_ctx_t * ctx,
    epc_cpt_node_t * node,
    void * * children,
    int count,
    void * user_data
)
{
    (void)node;

    json_pointer_node_t * list_node = json_pointer_node_alloc(JSON_POINTER_NODE_LIST);
    if (list_node == NULL)
    {
        epc_ast_builder_set_error(ctx, "Failed to allocate list node");
        return;
    }

    for (int i = 0; i < count; i++)
    {
        ast_list_append(&list_node->data.list, (json_pointer_node_t *)children[i]);
    }

    epc_ast_push(ctx, list_node);
}

void
json_pointer_ast_hook_registry_init(epc_ast_hook_registry_t * registry)
{
    epc_ast_hook_registry_set_free_node(registry, json_pointer_node_free);
    epc_ast_hook_registry_set_action(registry, CREATE_ESCAPED_TOKEN, create_escaped_token_action);
    epc_ast_hook_registry_set_action(registry, CREATE_UNESCAPED_TOKEN, create_unescaped_token_action);
    epc_ast_hook_registry_set_action(registry, CREATE_OPTIONAL_TOKEN, create_optional_token_action);
    epc_ast_hook_registry_set_action(registry, COLLECT_OPTIONAL_TOKENS, collect_optional_token_list_action);
}
