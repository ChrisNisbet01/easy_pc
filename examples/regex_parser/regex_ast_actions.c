#include "regex_ast_actions.h"

#include "regex_actions.h"
#include "regex_ast.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef __GNUC__
#define UNUSED(x) (void)(x)
#else
#define UNUSED(x)
#endif

static void
copy_cpt_node_content(regex_node_t * const node, epc_cpt_node_t * const cpt_node)
{
    if (node != NULL && cpt_node != NULL)
    {
        char const * content = epc_cpt_node_get_content(cpt_node);

        if (content == NULL)
        {
            content = "";
        }
        node->len = epc_cpt_node_get_len(cpt_node);
        node->content = strndup(epc_cpt_node_get_content(cpt_node), node->len);
        node->content_offset = epc_cpt_node_get_content_offset(cpt_node);
    }
}

static regex_node_t *
alloc_node(regex_node_type_t const type, epc_cpt_node_t * const cpt_node)
{
    regex_node_t * const node = calloc(1, sizeof(*node));

    if (node != NULL)
    {
        node->type = type;
        copy_cpt_node_content(node, cpt_node);
    }

    return node;
}

void
regex_node_free(void * const node_ptr, void * const user_data)
{
    UNUSED(user_data);

    if (node_ptr == NULL)
    {
        return;
    }

    regex_node_t * const node = (regex_node_t *)node_ptr;

    switch (node->type)
    {
    case REGEX_NODE_ALTERNATION:
    case REGEX_NODE_CONCATENATION:
    {
        for (size_t i = 0; i < node->data.list.count; i++)
        {
            regex_node_free(node->data.list.nodes[i], user_data);
        }
        free(node->data.list.nodes);
        break;
    }
    case REGEX_NODE_REPETITION:
    {
        regex_node_free(node->data.repetition.primary, user_data);
        regex_node_free(node->data.repetition.quantifier, user_data);
        break;
    }
    case REGEX_NODE_CHAR_RANGE:
    {
        regex_node_free(node->data.char_range.start, user_data);
        regex_node_free(node->data.char_range.end, user_data);
        break;
    }
    case REGEX_NODE_PRIMARY_LITERAL:
    case REGEX_NODE_PRIMARY_ESCAPED_CHAR:
    case REGEX_NODE_CC_CHAR:
    case REGEX_NODE_CLASS_ESCAPE:
    case REGEX_NODE_NUMBER:
    {
        free(node->data.text);
        break;
    }
    case REGEX_NODE_PRIMARY_CHAR_CLASS:
    {
        for (size_t i = 0; i < node->data.char_class.body.count; i++)
        {
            regex_node_free(node->data.char_class.body.nodes[i], user_data);
        }
        free(node->data.char_class.body.nodes);
        break;
    }
    case REGEX_NODE_PRIMARY_GROUP:
    case REGEX_NODE_LOOKAHEAD:
    case REGEX_NODE_NEGATIVE_LOOKAHEAD:
    case REGEX_NODE_NON_CAPTURING_GROUP:
    {
        regex_node_free(node->data.group_content, user_data);
        break;
    }
    case REGEX_NODE_NEGATION_MARKER:
    case REGEX_NODE_LAZY_MARKER:
    case REGEX_NODE_PRIMARY_DOT:
    case REGEX_NODE_PRIMARY_ANCHOR:
    case REGEX_NODE_QUANTIFIER:
    case REGEX_NODE_WORD_BOUNDARY:
    case REGEX_NODE_NON_WORD_BOUNDARY:
    {
        /* No extra data to free */
        break;
    }
    }
    free((char *)node->content);
    free(node);
}

static void
handle_regex_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    UNUSED(cpt_node);

    if (count >= 1)
    {
        epc_ast_push(ctx, children[0]);

        /* Free any other children (like EOI) */
        for (int i = 1; i < count; i++)
        {
            regex_node_free(children[i], user_data);
        }
    }
}

static void
handle_literal_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_LITERAL, cpt_node);

    if (node != NULL)
    {
        node->data.text = strndup(epc_cpt_node_get_semantic_content(cpt_node), epc_cpt_node_get_semantic_len(cpt_node));
        epc_ast_push(ctx, node);
    }
}

static void
handle_escaped_char_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_ESCAPED_CHAR, cpt_node);

    if (node != NULL)
    {
        node->data.text = strndup(epc_cpt_node_get_semantic_content(cpt_node), epc_cpt_node_get_semantic_len(cpt_node));
        epc_ast_push(ctx, node);
    }
}

static void
handle_dot_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_DOT, cpt_node);

    if (node != NULL)
    {
        epc_ast_push(ctx, node);
    }
}

static void
handle_anchor_start_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_ANCHOR, cpt_node);

    if (node != NULL)
    {
        node->data.anchor.type = ANCHOR_START;
        epc_ast_push(ctx, node);
    }
}

static void
handle_anchor_end_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_ANCHOR, cpt_node);

    if (node != NULL)
    {
        node->data.anchor.type = ANCHOR_END;
        epc_ast_push(ctx, node);
    }
}

static void
handle_word_boundary_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_WORD_BOUNDARY, cpt_node);

    if (node != NULL)
    {
        epc_ast_push(ctx, node);
    }
}

static void
handle_non_word_boundary_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_NON_WORD_BOUNDARY, cpt_node);

    if (node != NULL)
    {
        epc_ast_push(ctx, node);
    }
}

static void
handle_negation_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_NEGATION_MARKER, cpt_node);

    if (node != NULL)
    {
        epc_ast_push(ctx, node);
    }
}

static void
handle_lazy_marker_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_LAZY_MARKER, cpt_node);

    if (node != NULL)
    {
        epc_ast_push(ctx, node);
    }
}

static void
handle_char_class_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_CHAR_CLASS, cpt_node);

    if (node == NULL)
    {
        return;
    }

    int start_idx = 0;

    if (count > 0 && children[0] != NULL)
    {
        regex_node_t * const first_child = (regex_node_t *)children[0];

        if (first_child->type == REGEX_NODE_NEGATION_MARKER)
        {
            node->data.char_class.negated = true;
            start_idx = 1;
            regex_node_free(first_child, user_data);
        }
    }

    int const body_count = count - start_idx;

    if (body_count > 0)
    {
        node->data.char_class.body.count = (size_t)body_count;
        node->data.char_class.body.nodes = calloc(node->data.char_class.body.count, sizeof(regex_node_t *));

        if (node->data.char_class.body.nodes != NULL)
        {
            for (int i = 0; i < body_count; i++)
            {
                node->data.char_class.body.nodes[i] = (regex_node_t *)children[start_idx + i];
            }
        }
    }

    epc_ast_push(ctx, node);
}

static void
handle_char_range_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_CHAR_RANGE, cpt_node);

    if (node == NULL)
    {
        return;
    }

    if (count >= 2)
    {
        node->data.char_range.start = (regex_node_t *)children[0];
        node->data.char_range.end = (regex_node_t *)children[1];

        /* Free any unexpected extra children */
        for (int i = 2; i < count; i++)
        {
            regex_node_free(children[i], user_data);
        }
    }
    else
    {
        for (int i = 0; i < count; i++)
        {
            regex_node_free(children[i], user_data);
        }
    }

    epc_ast_push(ctx, node);
}

static void
handle_cc_char_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_CC_CHAR, cpt_node);

    if (node != NULL)
    {
        node->data.text = strndup(epc_cpt_node_get_semantic_content(cpt_node), epc_cpt_node_get_semantic_len(cpt_node));
        epc_ast_push(ctx, node);
    }
}

static void
handle_cc_char_escaped_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_CC_CHAR, cpt_node);

    if (node != NULL)
    {
        node->data.text = strndup(epc_cpt_node_get_semantic_content(cpt_node), epc_cpt_node_get_semantic_len(cpt_node));
        epc_ast_push(ctx, node);
    }
}

static void
handle_cc_escape_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_CLASS_ESCAPE, cpt_node);

    if (node != NULL)
    {
        node->data.text = strndup(epc_cpt_node_get_semantic_content(cpt_node), epc_cpt_node_get_semantic_len(cpt_node));
        epc_ast_push(ctx, node);
    }
}

static void
handle_group_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_PRIMARY_GROUP, cpt_node);

    if (node == NULL)
    {
        return;
    }

    if (count > 0)
    {
        regex_node_t * const content = (regex_node_t *)children[0];

        if (content->type == REGEX_NODE_LOOKAHEAD || content->type == REGEX_NODE_NEGATIVE_LOOKAHEAD
            || content->type == REGEX_NODE_NON_CAPTURING_GROUP)
        {
            /* This is a special group, we don't need the PRIMARY_GROUP node */
            epc_ast_push(ctx, content);
            regex_node_free(node, user_data);

            for (int i = 1; i < count; i++)
            {
                regex_node_free(children[i], user_data);
            }
            return;
        }

        node->data.group_content = content;

        for (int i = 1; i < count; i++)
        {
            regex_node_free(children[i], user_data);
        }
    }

    epc_ast_push(ctx, node);
}

static void
handle_positive_lookahead_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_LOOKAHEAD, cpt_node);

    if (node != NULL)
    {
        if (count > 0)
        {
            node->data.group_content = (regex_node_t *)children[0];
            for (int i = 1; i < count; i++)
            {
                regex_node_free(children[i], user_data);
            }
        }
        epc_ast_push(ctx, node);
    }
}

static void
handle_negative_lookahead_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_NEGATIVE_LOOKAHEAD, cpt_node);

    if (node != NULL)
    {
        if (count > 0)
        {
            node->data.group_content = (regex_node_t *)children[0];
            for (int i = 1; i < count; i++)
            {
                regex_node_free(children[i], user_data);
            }
        }
        epc_ast_push(ctx, node);
    }
}

static void
handle_non_capturing_group_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_NON_CAPTURING_GROUP, cpt_node);

    if (node != NULL)
    {
        if (count > 0)
        {
            node->data.group_content = (regex_node_t *)children[0];
            for (int i = 1; i < count; i++)
            {
                regex_node_free(children[i], user_data);
            }
        }
        epc_ast_push(ctx, node);
    }
}

static void
handle_star_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_QUANTIFIER, cpt_node);

    if (node != NULL)
    {
        node->data.quantifier.type = QUANTIFIER_STAR;
        epc_ast_push(ctx, node);
    }
}

static void
handle_plus_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_QUANTIFIER, cpt_node);

    if (node != NULL)
    {
        node->data.quantifier.type = QUANTIFIER_PLUS;
        epc_ast_push(ctx, node);
    }
}

static void
handle_question_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_QUANTIFIER, cpt_node);

    if (node != NULL)
    {
        node->data.quantifier.type = QUANTIFIER_QUESTION;
        epc_ast_push(ctx, node);
    }
}

static void
handle_number_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    for (int i = 0; i < count; i++)
    {
        regex_node_free(children[i], user_data);
    }

    regex_node_t * const node = alloc_node(REGEX_NODE_NUMBER, cpt_node);

    if (node != NULL)
    {
        node->data.text = strndup(epc_cpt_node_get_semantic_content(cpt_node), epc_cpt_node_get_semantic_len(cpt_node));
        epc_ast_push(ctx, node);
    }
}

static void
handle_range_quantifier_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    regex_node_t * const node = alloc_node(REGEX_NODE_QUANTIFIER, cpt_node);

    if (node == NULL)
    {
        return;
    }

    node->data.quantifier.type = QUANTIFIER_RANGE;
    node->data.quantifier.range.min = 0;
    node->data.quantifier.range.max = -1;

    if (count > 0)
    {
        regex_node_t * const min_node = (regex_node_t *)children[0];

        node->data.quantifier.range.min = atoi(min_node->data.text);
        regex_node_free(min_node, user_data);
    }

    if (count > 1)
    {
        regex_node_t * const max_node = (regex_node_t *)children[1];

        if (max_node != NULL)
        {
            node->data.quantifier.range.max = atoi(max_node->data.text);
            regex_node_free(max_node, user_data);
        }
    }

    epc_ast_push(ctx, node);
}

static void
handle_repetition_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    if (count == 1)
    {
        epc_ast_push(ctx, children[0]);
    }
    else
    {
        regex_node_t * const node = alloc_node(REGEX_NODE_REPETITION, cpt_node);

        if (node != NULL)
        {
            node->data.repetition.primary = (regex_node_t *)children[0];
            node->data.repetition.quantifier = (regex_node_t *)children[1];

            if (count > 2)
            {
                regex_node_t * const marker = (regex_node_t *)children[2];
                if (marker->type == REGEX_NODE_LAZY_MARKER)
                {
                    node->data.repetition.quantifier->data.quantifier.lazy = true;
                }
                regex_node_free(marker, user_data);
            }

            for (int i = 3; i < count; i++)
            {
                regex_node_free(children[i], user_data);
            }

            epc_ast_push(ctx, node);
        }
    }
}

static void
handle_concatenation_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    UNUSED(user_data);

    if (count == 1)
    {
        epc_ast_push(ctx, children[0]);
    }
    else
    {
        regex_node_t * const node = alloc_node(REGEX_NODE_CONCATENATION, cpt_node);

        if (node != NULL)
        {
            node->data.list.count = (size_t)count;
            node->data.list.nodes = calloc(node->data.list.count, sizeof(regex_node_t *));

            if (node->data.list.nodes != NULL)
            {
                memcpy(node->data.list.nodes, children, sizeof(regex_node_t *) * node->data.list.count);
            }

            epc_ast_push(ctx, node);
        }
    }
}

static void
handle_alternation_action(
    epc_ast_builder_ctx_t * const ctx,
    epc_cpt_node_t * const cpt_node,
    void ** const children,
    int const count,
    void * const user_data
)
{
    UNUSED(user_data);

    if (count == 1)
    {
        epc_ast_push(ctx, children[0]);
    }
    else
    {
        regex_node_t * const node = alloc_node(REGEX_NODE_ALTERNATION, cpt_node);

        if (node != NULL)
        {
            node->data.list.count = (size_t)count;
            node->data.list.nodes = calloc(node->data.list.count, sizeof(regex_node_t *));

            if (node->data.list.nodes != NULL)
            {
                memcpy(node->data.list.nodes, children, sizeof(regex_node_t *) * node->data.list.count);
            }

            epc_ast_push(ctx, node);
        }
    }
}

void
regex_ast_hook_registry_init(epc_ast_hook_registry_t * const registry)
{
    epc_ast_hook_registry_set_free_node(registry, regex_node_free);
    epc_ast_hook_registry_set_action(registry, HANDLE_REGEX, handle_regex_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_LITERAL, handle_literal_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_ESCAPED_CHAR, handle_escaped_char_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_DOT, handle_dot_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_ANCHOR_END, handle_anchor_end_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_ANCHOR_START, handle_anchor_start_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_WORD_BOUNDARY, handle_word_boundary_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_NON_WORD_BOUNDARY, handle_non_word_boundary_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_CHAR_CLASS, handle_char_class_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_CC_ESCAPE, handle_cc_escape_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_CHAR_RANGE, handle_char_range_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_CC_CHAR_ESCAPED, handle_cc_char_escaped_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_CC_CHAR, handle_cc_char_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_NEGATION, handle_negation_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_LAZY_MARKER, handle_lazy_marker_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_POSITIVE_LOOKAHEAD, handle_positive_lookahead_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_NEGATIVE_LOOKAHEAD, handle_negative_lookahead_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_NON_CAPTURING_GROUP, handle_non_capturing_group_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_GROUP, handle_group_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_RANGE_QUANTIFIER, handle_range_quantifier_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_NUMBER, handle_number_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_QUESTION, handle_question_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_PLUS, handle_plus_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_STAR, handle_star_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_REPETITION, handle_repetition_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_CONCATENATION, handle_concatenation_action);
    epc_ast_hook_registry_set_action(registry, HANDLE_ALTERNATION, handle_alternation_action);
}
