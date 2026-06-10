#pragma once

#include "easy_pc_private.h"

#include <easy_pc/easy_pc.h>
#include <stddef.h>

typedef struct epc_parser_t epc_parser_t;

typedef struct epc_memo_entry_t
{
    epc_parser_t * parser;
    size_t input_offset;
    epc_parse_result_t result;
    size_t hit_count;
    struct epc_memo_entry_t * next; // For separate chaining
} epc_memo_entry_t;

typedef struct epc_memo_table_t
{
    epc_memo_entry_t ** buckets;
    size_t bucket_count;
    size_t entry_count;
} epc_memo_table_t;

EASY_PC_HIDDEN
epc_memo_table_t * epc_parser_ctx_get_memo_table(epc_parser_ctx_t * ctx);

EASY_PC_HIDDEN
epc_parse_result_t * epc_memo_table_get(epc_parser_ctx_t * ctx, epc_parser_t * parser, size_t input_offset);

EASY_PC_HIDDEN
void epc_memo_table_set(epc_parser_ctx_t * ctx, epc_parser_t * parser, size_t input_offset, epc_parse_result_t result);

EASY_PC_HIDDEN
void epc_memo_table_cleanup(epc_parser_ctx_t * ctx);

EASY_PC_HIDDEN
void epc_memo_table_reset(epc_parser_ctx_t * ctx);
