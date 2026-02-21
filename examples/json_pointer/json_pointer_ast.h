#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum {
    JSON_POINTER_NODE_CHAR,
    JSON_POINTER_NODE_STRING,
    JSON_POINTER_NODE_LIST,
} json_pointer_node_type_t;

typedef struct json_pointer_node_t json_pointer_node_t;

typedef struct json_pointer_list_node_t {
    json_pointer_node_t * item;
    struct json_pointer_list_node_t * next;
} json_pointer_list_node_t;

typedef struct {
    json_pointer_list_node_t * head;
    json_pointer_list_node_t * tail;
    int count;
} json_pointer_list_t;

typedef struct {
    char * key;
    json_pointer_node_t * value;
} json_pointer_member_t;

struct json_pointer_node_t {
    json_pointer_node_type_t type;
    union {
        json_pointer_list_t list;
        char * string;
        char ch;
    } data;
};

/* Function to free a JSON AST node and its children */
void
json_pointer_node_free(void * node, void * user_data);
