#pragma once

#include "epc_parser_ctx.h"
#include "parsers.h"

#include <easy_pc/easy_pc.h>
#include <easy_pc/easy_pc_ast.h>
#include <stddef.h>

// The Parse Tree Node
/**
 * @brief Represents a node in the Concrete Parse Tree (CPT).
 *
 * Each `pt_node_t` stores information about a successfully parsed segment
 * of the input, including its type (tag), the actual content matched,
 * its length, and its hierarchical relationship to other nodes (children).
 */
typedef struct epc_cpt_node_t
{
    char const * tag;  /**< @brief A string tag identifying the type of this node (e.g., "char", "string", "and"). */
    char const * name; /**< @brief The name assigned to the parser that generated this node, for
                        *    debugging/identification.
                        */
    size_t content_offset; /**< @brief Offset to the first matched token in the original input token list. */
    size_t token_count;    /**< @brief The number of matched tokens. */

    size_t semantic_start_offset; /**< @brief Offset from `content_offset` to the start of the semantically relevant
                                      part. */
    size_t semantic_end_offset;   /**< @brief Length from the end of matched content to exclude from the semantically
                                      relevant   part. */

    epc_cpt_node_t ** children; /**< @brief An array of pointers to child `pt_node_t`s, representing sub-matches. */
    int children_count;         /**< @brief The number of children in the `children` array. */

    epc_ast_semantic_action_t ast_config; /**< @brief A copy of the ast action assigned to the associated parser that
                                           *    created the node.
                                           */

    epc_parser_ctx_t * ctx; /**< @brief The parser context associated with this node. */

    char const * error_message; /**< an optional error message assigned by 'satisfy' predicate functions when the
                                   predicate fails.  */
} epc_cpt_node_t;

ATTR_NONNULL(2)
EASY_PC_API
epc_cpt_node_t * epc_node_alloc(epc_parser_ctx_t * ctx, epc_parser_t * parser, char const * const tag);

EASY_PC_HIDDEN
epc_cpt_node_t * epc_node_copy(epc_cpt_node_t * node);

EASY_PC_HIDDEN
void epc_node_free(epc_cpt_node_t * node);

EASY_PC_HIDDEN
char const * epc_node_id(epc_cpt_node_t const * node);
