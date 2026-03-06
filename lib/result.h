#pragma once

#include "parsers.h"

#include <easy_pc/easy_pc.h>
#include <stddef.h>

EASY_PC_HIDDEN
epc_line_col_t epc_calculate_line_and_column(epc_parser_ctx_t * ctx, size_t offset);

EASY_PC_HIDDEN
epc_parser_error_t * epc_parser_error_alloc(
    epc_parser_ctx_t * ctx, size_t input_offset, char const * message, char const * expected, char const * found
);

EASY_PC_HIDDEN
void epc_parser_error_free(epc_parser_error_t * error);

EASY_PC_HIDDEN
epc_parser_error_t * epc_parser_error_copy(epc_parser_ctx_t * ctx, epc_parser_error_t * e);

EASY_PC_HIDDEN
epc_parser_error_t * parser_furthest_error_copy(epc_parser_ctx_t * ctx);

EASY_PC_HIDDEN
void update_furthest_error(epc_parser_ctx_t * ctx, epc_parser_error_t * new_error);

EASY_PC_HIDDEN
epc_parse_result_t epc_parser_error_result(
    epc_parser_ctx_t * ctx, size_t input_offset, char const * message, char const * expected, char const * found
);

EASY_PC_HIDDEN
epc_parse_result_t epc_parser_success_result(epc_cpt_node_t * success_node);

EASY_PC_HIDDEN
epc_parse_result_t epc_parse_result_copy(epc_parser_ctx_t * ctx, epc_parse_result_t result);

EASY_PC_HIDDEN
void parser_furthest_error_restore(epc_parser_ctx_t * ctx, epc_parser_error_t ** replacement);

EASY_PC_HIDDEN
epc_parse_result_t
epc_unparsed_error_result(size_t input_offset, char const * message, char const * expected, char const * found);

void epc_parser_result_cleanup(epc_parse_result_t * result);
