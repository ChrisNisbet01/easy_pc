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
    char const * tag;     /**< @brief A string tag identifying the type of this node (e.g., "char", "string", "and"). */
    char const * name;    /**< @brief The name assigned to the parser that generated this node, for
                           *    debugging/identification.
                           */
    char const * content; /**< @brief A pointer to the start of the matched substring in the original input (or within
                             the parser itself in the case of epc_succeed()). */
    size_t len;           /**< @brief The length of the matched substring. */
    size_t semantic_start_offset; /**< @brief Offset from `content` to the start of the semantically relevant part. */
    size_t semantic_end_offset;   /**< @brief Length from the end of `content` to exclude from the semantically relevant
                                     part. */
    epc_cpt_node_t ** children;   /**< @brief An array of pointers to child `pt_node_t`s, representing sub-matches. */
    int children_count;           /**< @brief The number of children in the `children` array. */
    epc_ast_semantic_action_t ast_config; /**< @brief A copy of the ast action assigned to the associated parser that
                                           *    created the node.
                                           */
    epc_parser_ctx_t * ctx;
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
