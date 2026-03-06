#pragma once

#include "arena.h"
#include "cpt_node.h"
#include "memo.h"

#include <stdbool.h>
#include <stddef.h>
#ifdef WITH_INPUT_STREAM_SUPPORT
#include <pthread.h>
#endif

#define MAX_MMAP_INPUT_SIZE (100 * 1024 * 1024) /* 100 MB */
#define MAX_NODE_ARENA_SIZE (256 * 1024 * 1024) /* 256 MB for CPT nodes */

typedef struct mmap_input_buffer_t
{
    char * buffer;     /**< Pointer to the start of the memory-mapped input buffer. */
    size_t total_size; /**< Total size of the memory-mapped region (including guard page). */
    size_t input_size; /**< Actual size of the input string stored in the buffer. */
} mmap_input_buffer_t;

typedef struct
{
    epc_cpt_node_t ** nodes;
    size_t count;
    size_t capacity;
} epc_node_pool_t;

// The Parsing Context (for a single parse operation and its results)
// This will be internally managed by epc_parse_input
struct epc_parser_ctx_t
{
    char const * input_start;
    size_t input_len;

    mmap_input_buffer_t mmap_buffer; /* Internal buffer management for input data, using mmap for large inputs. */
    epc_parser_error_t * furthest_error;

    void * user_ctx; /* User-defined context that can be used in predicates (e.g. epc_wrap()). */

    epc_memo_table_t memo_table;

    epc_arena_t node_arena;
    epc_node_pool_t node_pool;

#ifdef WITH_INPUT_STREAM_SUPPORT
    pthread_mutex_t mutex;
    pthread_cond_t cond;
    bool is_streaming;
    bool is_eof;
    int input_error;
#endif
};

typedef struct parse_get_input_result_t
{
    char const * next_input;
    size_t available;
    bool is_eof;
} parse_get_input_result_t;

EASY_PC_HIDDEN
epc_cpt_node_t * parse_ctx_alloc_node(epc_parser_ctx_t * ctx);

EASY_PC_HIDDEN
void parse_ctx_free_node(epc_parser_ctx_t * ctx, epc_cpt_node_t * node);

EASY_PC_HIDDEN
parse_get_input_result_t parse_ctx_get_input_at_offset(epc_parser_ctx_t * ctx, size_t input_offset, size_t count);

EASY_PC_HIDDEN
bool parse_ctx_is_streaming(epc_parser_ctx_t const * ctx);

EASY_PC_HIDDEN
bool parse_ctx_is_eof(epc_parser_ctx_t * ctx);

static inline char const *
parse_ctx_get_input_start(epc_parser_ctx_t * ctx)
{
    parse_get_input_result_t input_result = parse_ctx_get_input_at_offset(ctx, 0, 0);
    return input_result.next_input;
}

EASY_PC_HIDDEN
size_t parse_ctx_get_input_len(epc_parser_ctx_t * const ctx);

EASY_PC_HIDDEN
ATTR_NONNULL(1)
size_t parse_ctx_get_offset_from_input(epc_parser_ctx_t * ctx, char const * input_position);

EASY_PC_HIDDEN
ATTR_NONNULL(1)
epc_parser_error_t * parse_ctx_get_furthest_error(epc_parser_ctx_t const * ctx);

EASY_PC_HIDDEN
ATTR_NONNULL(1)
void parser_ctx_set_furthest_error(epc_parser_ctx_t * ctx, epc_parser_error_t ** replacement);
