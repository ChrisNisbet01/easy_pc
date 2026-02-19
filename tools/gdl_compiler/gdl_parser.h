#pragma once

#include <easy_pc/easy_pc.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Creates and returns the top-level parser for the GDL grammar.
 *        All parsers created are added to an internal list for proper management.
 *
 * @return A pointer to the top-level epc_parser_t for parsing GDL files, or NULL on error.
 */
epc_parser_t *create_gdl_parser(epc_parser_list * l);

#ifdef __cplusplus
}
#endif

