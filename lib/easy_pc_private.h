#pragma once

#include <easy_pc/easy_pc.h>

/**
 * @brief Initiates a parsing operation with a given grammar and input.
 */
EASY_PC_HIDDEN epc_parse_session_t epc_parse_input(epc_parser_t * top_parser, epc_parse_input_t input, void * user_ctx);
