#pragma once

#include <stdbool.h>
#include <stddef.h>

typedef enum
{
    REGEX_NODE_ALTERNATION,
    REGEX_NODE_CONCATENATION,
    REGEX_NODE_REPETITION,
    REGEX_NODE_QUANTIFIER,
    REGEX_NODE_PRIMARY_GROUP,
    REGEX_NODE_PRIMARY_CHAR_CLASS,
    REGEX_NODE_PRIMARY_ANCHOR,
    REGEX_NODE_PRIMARY_DOT,
    REGEX_NODE_PRIMARY_ESCAPED_CHAR,
    REGEX_NODE_PRIMARY_LITERAL,
    REGEX_NODE_CHAR_RANGE,
    REGEX_NODE_CLASS_ESCAPE,
    REGEX_NODE_CC_CHAR,
    REGEX_NODE_NUMBER,
    REGEX_NODE_NEGATION_MARKER,
    REGEX_NODE_LAZY_MARKER,
    REGEX_NODE_LOOKAHEAD,
    REGEX_NODE_NEGATIVE_LOOKAHEAD,
    REGEX_NODE_NON_CAPTURING_GROUP,
    REGEX_NODE_WORD_BOUNDARY,
    REGEX_NODE_NON_WORD_BOUNDARY,
} regex_node_type_t;

typedef enum
{
    QUANTIFIER_STAR,
    QUANTIFIER_PLUS,
    QUANTIFIER_QUESTION,
    QUANTIFIER_RANGE,
} quantifier_type_t;

typedef enum
{
    ANCHOR_START,
    ANCHOR_END,
} anchor_type_t;

typedef struct regex_node_t regex_node_t;

typedef struct
{
    regex_node_t ** nodes;
    size_t count;
} regex_node_list_t;

typedef struct
{
    int min;
    int max; // -1 for unbounded
} quantifier_range_t;

struct regex_node_t
{
    regex_node_type_t type;
    char const * content;
    size_t content_offset;
    size_t len;
    union
    {
        regex_node_list_t list; // ALTERNATION, CONCATENATION, CHAR_CLASS, REGEX
        struct
        {
            regex_node_t * primary;
            regex_node_t * quantifier;
        } repetition;
        struct
        {
            quantifier_type_t type;
            quantifier_range_t range;
            bool lazy;
        } quantifier;
        struct
        {
            anchor_type_t type;
        } anchor;
        struct
        {
            regex_node_t * start;
            regex_node_t * end;
        } char_range;
        char * text; // LITERAL, ESCAPED_CHAR, CC_CHAR, CLASS_ESCAPE, NUMBER
        struct
        {
            bool negated;
            regex_node_list_t body;
        } char_class;
        regex_node_t * group_content; // PRIMARY_GROUP, LOOKAHEAD, NEGATIVE_LOOKAHEAD
    } data;
};

void regex_node_free(void * node, void * user_data);
