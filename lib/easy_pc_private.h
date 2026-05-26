#pragma once

#include "token.h"

#include <easy_pc/easy_pc.h>

/**
 * @brief Initiates a parsing operation with a given grammar and input.
 */
EASY_PC_HIDDEN epc_parse_session_t epc_parse_input(epc_parser_t * top_parser, epc_parse_input_t input, void * user_ctx);

/**
 * @brief Adds a parser to the parser list.
 *
 * If the parser passed is NULL, nothing is added to the list and NULL is returned.
 *
 * @param list The parser list to add to.
 * @param parser The parser to add.
 * @return The parser that was added, or NULL if the input parser was NULL or an error occurred.
 */
EASY_PC_HIDDEN epc_parser_t * epc_parser_list_add(epc_parser_list * list, epc_parser_t * parser);

/**
 * @brief Return a pointer to the input at the specified offset from the start of input.
 *
 * This function returns a pointer to the start of the node's full `content`.

 * @param offset The offset from the start of input.
 * @return A `const char*` pointer to the content.
 */
EASY_PC_API const char * epc_cpt_get_content_at_offset(epc_parser_ctx_t const * ctx, size_t offset);

EASY_PC_HIDDEN epc_parser_token_t const * epc_token_list_data(epc_token_list_t const * list);
EASY_PC_HIDDEN void epc_token_list_detach_mmap(epc_token_list_t * list, void ** out_base, size_t * out_size);
