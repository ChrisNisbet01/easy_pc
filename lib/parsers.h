#pragma once

#include <easy_pc/easy_pc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef epc_parse_result_t (*parse_fn_t)(struct epc_parser_t * self, epc_parser_ctx_t * ctx, size_t input_offset);

// Structure for user-managed parser list
struct epc_parser_list
{
    epc_parser_t ** parsers;
    size_t count;
    size_t capacity;
};

// Structure to hold a list of parsers (e.g., for combinators like p_or)
typedef struct parser_list_t
{
    epc_parser_t ** parsers;
    int count;
} parser_list_t;

typedef struct
{
    char start;
    char end;
} char_range_data_t;

typedef struct
{
    size_t count_min;
    size_t count_max;
    epc_parser_t * parser;
} count_data_t;

typedef struct
{
    epc_parser_t * open;
    epc_parser_t * parser;
    epc_parser_t * close;
} between_data_t;

typedef struct
{
    epc_parser_t * item;
    epc_parser_t * delimiter;
    bool is_flexible;
} delimited_data_t;

typedef struct
{
    epc_parser_t * parser;
    epc_consume_flags_t consume_flags;
    bool strip_leading;
    bool strip_trailing;
} lexeme_data_t;

typedef struct
{
    epc_parser_t * parser; // A parser to produce a token for the predicate
    epc_satisfy_parser_predicate_fn predicate_fn;
    void * parser_data; // User-defined context passed to the predicate function
} predicate_data_t;

typedef struct
{
    epc_parser_t * parser; // The parser to wrap
    epc_wrap_callbacks_t callbacks;
    void * parser_data;
} wrap_data_t;

typedef struct epc_byte_data_t
{
    char const * str; /* The byte represented as a string (e.g. 0x42) */
    uint8_t byte;     /* The byte value; */
} epc_byte_data_t;

typedef struct epc_token_data_t
{
    char const * str;   /* The token ID represented as a string (e.g. "token(256)") */
    epc_token_id_t id;  /* The token ID value; */
} epc_token_data_t;

typedef enum parser_data_type_t
{
    PARSER_DATA_TYPE_NONE,
    PARSER_DATA_TYPE_PARSER,
    PARSER_DATA_TYPE_STRING,
    PARSER_DATA_TYPE_PARSER_LIST,
    PARSER_DATA_TYPE_CHAR_RANGE,
    PARSER_DATA_TYPE_COUNT,
    PARSER_DATA_TYPE_BETWEEN,
    PARSER_DATA_TYPE_DELIMITED,
    PARSER_DATA_TYPE_LEXEME,
    PARSER_DATA_TYPE_PREDICATE,
    PARSER_DATA_TYPE_WRAP,
    PARSER_DATA_TYPE_MEMOIZE,
    PARSER_DATA_TYPE_BYTE,
    PARSER_DATA_TYPE_TOKEN,
    PARSER_DATA_TYPE_COMMIT,
} parser_data_type_t;

typedef struct parser_data_type_st
{
    parser_data_type_t type;
    union
    {
        epc_parser_t * parser;
        char const * string;
        parser_list_t * parser_list;
        char_range_data_t range;
        count_data_t count;
        between_data_t between;
        delimited_data_t delimited;
        lexeme_data_t lexeme;
        predicate_data_t predicate;
        wrap_data_t wrap;
        epc_byte_data_t byte;
        epc_token_data_t token;
    };
} parser_data_type_st;

// Parser struct
struct epc_parser_t
{
    parse_fn_t parse_fn;

    // Parser-specific data
    parser_data_type_st data;

    char const * name; /**< @brief Must be freed when parser is destroyed. */
    char const * tag;  /**< @brief Unique tag for each parser type. Must _not_ be freed when the parser is destroyed. */
    char const * expected_value; /**< @brief Optional string to use in preference to name or tag when reporting parse
                                  * errors. Must _not_ be freed when the parser is destroyed.
                                  */

    epc_ast_semantic_action_t ast_config;

    bool is_commit_boundary;
};

void epc_parser_free(epc_parser_t * parser);

EASY_PC_HIDDEN
char const * epc_parser_get_name(epc_parser_t const * p);
