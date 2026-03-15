#include "easy_pc/easy_pc_ast.h"

#include "regex.h"
#include "regex_ast.h"
#include "regex_ast_actions.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define UNUSED_PARAM(x) (void)(x)

typedef enum
{
    OUTPUT_FORMAT_MARKDOWN,
    OUTPUT_FORMAT_HTML,
} output_format_t;

// Forward declarations
static void describe_node_md(regex_node_t * const node, int const indent, char const * const full_regex);
static void describe_node_html(regex_node_t * const node, int const indent, char const * const full_regex);
static void describe_node_html_literal(regex_node_t * const node, int const indent, char const * const full_regex);

static void
print_indent_md(int const indent)
{
    for (int i = 0; i < indent; i++)
    {
        printf("  ");
    }
}

static void
print_highlight_md(char const * const full_regex, size_t content_offset, size_t const len)
{
    printf("`%s`\n\n", full_regex);
    printf("`");
    for (size_t i = 0; i < strlen(full_regex); ++i)
    {
        if (i >= content_offset && i < (content_offset + len))
        {
            printf("^");
        }
        else
        {
            printf(" ");
        }
    }
    printf("`\n\n");
}

static bool
is_identifier_start_char_class(regex_node_t * const node)
{
    if (node->type != REGEX_NODE_PRIMARY_CHAR_CLASS || node->data.char_class.negated)
    {
        return false;
    }

    bool has_valid_part = false;
    for (size_t i = 0; i < node->data.char_class.body.count; i++)
    {
        regex_node_t * item = node->data.char_class.body.nodes[i];
        if (item->type == REGEX_NODE_CHAR_RANGE)
        {
            if ((strcmp(item->data.char_range.start->data.text, "a") == 0
                 && strcmp(item->data.char_range.end->data.text, "z") == 0))
            {
                has_valid_part = true;
            }
            else if ((strcmp(item->data.char_range.start->data.text, "A") == 0
                      && strcmp(item->data.char_range.end->data.text, "Z") == 0))
            {
                has_valid_part = true;
            }
            else
            {
                return false; // contains something else
            }
        }
        else if (item->type == REGEX_NODE_CC_CHAR)
        {
            if (strcmp(item->data.text, "_") == 0)
            {
                has_valid_part = true;
            }
            else
            {
                return false; // contains something else
            }
        }
        else
        {
            return false; // contains something else
        }
    }

    return has_valid_part;
}

static bool
is_identifier_continue_char_class(regex_node_t * const node)
{
    if (node->type != REGEX_NODE_PRIMARY_CHAR_CLASS || node->data.char_class.negated)
    {
        return false;
    }

    if (node->data.char_class.body.count == 1)
    {
        regex_node_t * item = node->data.char_class.body.nodes[0];
        if (item->type == REGEX_NODE_CLASS_ESCAPE && strcmp(item->data.text, "\\w") == 0)
        {
            return true;
        }
    }

    bool has_valid_part = false;
    for (size_t i = 0; i < node->data.char_class.body.count; i++)
    {
        regex_node_t * item = node->data.char_class.body.nodes[i];
        if (item->type == REGEX_NODE_CHAR_RANGE)
        {
            if (strcmp(item->data.char_range.start->data.text, "a") == 0
                && strcmp(item->data.char_range.end->data.text, "z") == 0)
            {
                has_valid_part = true;
            }
            else if (strcmp(item->data.char_range.start->data.text, "A") == 0
                     && strcmp(item->data.char_range.end->data.text, "Z") == 0)
            {
                has_valid_part = true;
            }
            else if (strcmp(item->data.char_range.start->data.text, "0") == 0
                     && strcmp(item->data.char_range.end->data.text, "9") == 0)
            {
                has_valid_part = true;
            }
            else
            {
                return false;
            }
        }
        else if (item->type == REGEX_NODE_CC_CHAR)
        {
            if (strcmp(item->data.text, "_") == 0)
            {
                has_valid_part = true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }

    return has_valid_part;
}

static int
match_identifier_pattern_prefix(regex_node_t * const node)
{
    if (node->type != REGEX_NODE_CONCATENATION || node->data.list.count < 2)
    {
        return 0;
    }

    regex_node_t * first = node->data.list.nodes[0];
    regex_node_t * second = node->data.list.nodes[1];

    if (is_identifier_start_char_class(first) && second->type == REGEX_NODE_REPETITION
        && second->data.repetition.quantifier->type == REGEX_NODE_QUANTIFIER
        && second->data.repetition.quantifier->data.quantifier.type == QUANTIFIER_STAR
        && is_identifier_continue_char_class(second->data.repetition.primary))
    {
        return 2;
    }

    return 0;
}

static void
describe_node_md_literal(regex_node_t * const node, int const indent, char const * const full_regex)
{
    if (node == NULL)
    {
        return;
    }

    print_indent_md(indent);

    switch (node->type)
    {
    case REGEX_NODE_ALTERNATION:
        printf("- **Alternation** (matches one of the following):\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        for (size_t i = 0; i < node->data.list.count; i++)
        {
            describe_node_md(node->data.list.nodes[i], indent + 1, full_regex);
        }
        break;
    case REGEX_NODE_CONCATENATION:
    {
        int consumed_nodes = match_identifier_pattern_prefix(node);
        if (consumed_nodes > 0)
        {
            print_indent_md(indent);
            printf("- **Pattern**: Matches a standard programming language identifier.\n");
            regex_node_t * const first_part = node->data.list.nodes[0];
            regex_node_t * const second_part = node->data.list.nodes[1];
            size_t const pattern_len = first_part->len + second_part->len;
            print_highlight_md(full_regex, first_part->content_offset, pattern_len);

            if ((size_t)consumed_nodes < node->data.list.count)
            {
                print_indent_md(indent);
                printf("- Followed by:\n");
            }
        }
        else
        {
            print_indent_md(indent);
            printf("- **Sequence** (matches the following in order):\n");
            print_highlight_md(full_regex, node->content_offset, node->len);
        }

        for (size_t i = consumed_nodes; i < node->data.list.count; i++)
        {
            describe_node_md(node->data.list.nodes[i], indent + 1, full_regex);
        }
        break;
    }
    case REGEX_NODE_REPETITION:
        printf("- **Repetition**:\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        describe_node_md(node->data.repetition.primary, indent + 1, full_regex);
        describe_node_md(node->data.repetition.quantifier, indent + 1, full_regex);
        break;
    case REGEX_NODE_QUANTIFIER:
        switch (node->data.quantifier.type)
        {
        case QUANTIFIER_STAR:
            printf("- Quantifier: Zero or more times (`*%s`)\n", node->data.quantifier.lazy ? "?" : "");
            break;
        case QUANTIFIER_PLUS:
            printf("- Quantifier: One or more times (`+%s`)\n", node->data.quantifier.lazy ? "?" : "");
            break;
        case QUANTIFIER_QUESTION:
            printf("- Quantifier: Zero or one time (`?%s`)\n", node->data.quantifier.lazy ? "?" : "");
            break;
        case QUANTIFIER_RANGE:
            if (node->data.quantifier.range.max == -1)
            {
                printf(
                    "- Quantifier: %d or more times (`{%d,}%s`)\n",
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.min,
                    node->data.quantifier.lazy ? "?" : ""
                );
            }
            else if (node->data.quantifier.range.min == node->data.quantifier.range.max)
            {
                printf(
                    "- Quantifier: Exactly %d times (`{%d}%s`)\n",
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.min,
                    node->data.quantifier.lazy ? "?" : ""
                );
            }
            else
            {
                printf(
                    "- Quantifier: Between %d and %d times (`{%d,%d}%s`)\n",
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.max,
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.max,
                    node->data.quantifier.lazy ? "?" : ""
                );
            }
            break;
        }
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_LITERAL:
        printf("- Literal character: `%s`\n", node->data.text);
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_ESCAPED_CHAR:
        printf("- Escaped character or class: `%s`\n", node->data.text);
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_DOT:
        printf("- Any character (dot `.`)\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_ANCHOR:
    case REGEX_NODE_WORD_BOUNDARY:
    case REGEX_NODE_NON_WORD_BOUNDARY:
        if (node->type == REGEX_NODE_WORD_BOUNDARY)
        {
            printf("- Word boundary (`\\b`)\n");
        }
        else if (node->type == REGEX_NODE_NON_WORD_BOUNDARY)
        {
            printf("- Non-word boundary (`\\B`)\n");
        }
        else if (node->data.anchor.type == ANCHOR_START)
        {
            printf("- Start of line anchor (`^`)\n");
        }
        else
        {
            printf("- End of line anchor (`$`)\n");
        }
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_CHAR_CLASS:
        printf("- **Character Class** (%s any of):\n", node->data.char_class.negated ? "Does not match" : "Matches");
        print_highlight_md(full_regex, node->content_offset, node->len);
        for (size_t i = 0; i < node->data.char_class.body.count; i++)
        {
            describe_node_md(node->data.char_class.body.nodes[i], indent + 1, full_regex);
        }
        break;
    case REGEX_NODE_CHAR_RANGE:
        printf("- Range: `%s` to `%s`\n", node->data.char_range.start->data.text, node->data.char_range.end->data.text);
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_CC_CHAR:
        printf("- Character: `%s`\n", node->data.text);
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_CLASS_ESCAPE:
        printf("- Escaped class: `%s`\n", node->data.text);
        print_highlight_md(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_GROUP:
        printf("- **Grouped Expression**:\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        describe_node_md(node->data.group_content, indent + 1, full_regex);
        break;
    case REGEX_NODE_LOOKAHEAD:
        printf("- **Positive Lookahead** (matches if the following is present, but doesn't consume it):\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        describe_node_md(node->data.group_content, indent + 1, full_regex);
        break;
    case REGEX_NODE_NEGATIVE_LOOKAHEAD:
        printf("- **Negative Lookahead** (matches if the following is NOT present, but doesn't consume it):\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        describe_node_md(node->data.group_content, indent + 1, full_regex);
        break;
    case REGEX_NODE_NON_CAPTURING_GROUP:
        printf("- **Non-Capturing Group**:\n");
        print_highlight_md(full_regex, node->content_offset, node->len);
        describe_node_md(node->data.group_content, indent + 1, full_regex);
        break;
    default:
        /* For internal markers */
        break;
    }
}

// Main dispatcher for markdown
static void
describe_node_md(regex_node_t * const node, int const indent, char const * const full_regex)
{
    if (node == NULL)
        return;

    describe_node_md_literal(node, indent, full_regex);
}

static void
print_highlight_html(char const * const full_regex, size_t content_offset, size_t const len)
{
    printf("<pre><code>");
    for (size_t i = 0; i < strlen(full_regex); ++i)
    {
        if (i >= content_offset && i < (content_offset + len))
        {
            printf("<span class=\"highlight\">%c</span>", full_regex[i]);
        }
        else
        {
            printf("%c", full_regex[i]);
        }
    }
    printf("</code></pre>\n");
}

static void
describe_node_html(regex_node_t * const node, int const indent, char const * const full_regex)
{
    if (node == NULL)
    {
        return;
    }

    describe_node_html_literal(node, indent, full_regex);
}

static void
describe_node_html_literal(regex_node_t * const node, int const indent, char const * const full_regex)
{

    printf("<ul>\n<li>");

    switch (node->type)
    {
    case REGEX_NODE_ALTERNATION:
        printf("<strong>Alternation</strong> (matches one of the following):");
        print_highlight_html(full_regex, node->content_offset, node->len);
        for (size_t i = 0; i < node->data.list.count; i++)
        {
            describe_node_html(node->data.list.nodes[i], indent + 1, full_regex);
        }
        break;
    case REGEX_NODE_CONCATENATION:
    {
        int consumed_nodes = match_identifier_pattern_prefix(node);
        if (consumed_nodes > 0)
        {
            printf("<strong>Pattern</strong>: Matches a standard programming language identifier.");
            regex_node_t * const first_part = node->data.list.nodes[0];
            regex_node_t * const second_part = node->data.list.nodes[1];
            size_t const pattern_len = first_part->len + second_part->len;
            print_highlight_html(full_regex, first_part->content_offset, pattern_len);

            if ((size_t)consumed_nodes < node->data.list.count)
            {
                printf("Followed by:");
            }
        }
        else
        {
            printf("<strong>Sequence</strong> (matches the following in order):");
            print_highlight_html(full_regex, node->content_offset, node->len);
        }

        for (size_t i = consumed_nodes; i < node->data.list.count; i++)
        {
            describe_node_html(node->data.list.nodes[i], indent + 1, full_regex);
        }
        break;
    }
    case REGEX_NODE_REPETITION:
        printf("<strong>Repetition</strong>:\n");
        print_highlight_html(full_regex, node->content_offset, node->len);
        describe_node_html(node->data.repetition.primary, indent + 1, full_regex);
        describe_node_html(node->data.repetition.quantifier, indent + 1, full_regex);
        break;
    case REGEX_NODE_QUANTIFIER:
        switch (node->data.quantifier.type)
        {
        case QUANTIFIER_STAR:
            printf("Quantifier: Zero or more times (<code>*%s</code>)\n", node->data.quantifier.lazy ? "?" : "");
            break;
        case QUANTIFIER_PLUS:
            printf("Quantifier: One or more times (<code>+%s</code>)\n", node->data.quantifier.lazy ? "?" : "");
            break;
        case QUANTIFIER_QUESTION:
            printf("Quantifier: Zero or one time (<code>?%s</code>)\n", node->data.quantifier.lazy ? "?" : "");
            break;
        case QUANTIFIER_RANGE:
            if (node->data.quantifier.range.max == -1)
            {
                printf(
                    "Quantifier: %d or more times (<code>{%d,}%s</code>)\n",
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.min,
                    node->data.quantifier.lazy ? "?" : ""
                );
            }
            else if (node->data.quantifier.range.min == node->data.quantifier.range.max)
            {
                printf(
                    "Quantifier: Exactly %d times (<code>{%d}%s</code>)\n",
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.min,
                    node->data.quantifier.lazy ? "?" : ""
                );
            }
            else
            {
                printf(
                    "Quantifier: Between %d and %d times (<code>{%d,%d}%s</code>)\n",
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.max,
                    node->data.quantifier.range.min,
                    node->data.quantifier.range.max,
                    node->data.quantifier.lazy ? "?" : ""
                );
            }
            break;
        }
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_LITERAL:
        printf("Literal character: <code>%s</code>\n", node->data.text);
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_ESCAPED_CHAR:
        printf("Escaped character or class: <code>%s</code>\n", node->data.text);
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_DOT:
        printf("Any character (dot <code>.</code>)\n");
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_ANCHOR:
    case REGEX_NODE_WORD_BOUNDARY:
    case REGEX_NODE_NON_WORD_BOUNDARY:
        if (node->type == REGEX_NODE_WORD_BOUNDARY)
        {
            printf("Word boundary (<code>\\b</code>)\n");
        }
        else if (node->type == REGEX_NODE_NON_WORD_BOUNDARY)
        {
            printf("Non-word boundary (<code>\\B</code>)\n");
        }
        else if (node->data.anchor.type == ANCHOR_START)
        {
            printf("Start of line anchor (<code>^</code>)\n");
        }
        else
        {
            printf("End of line anchor (<code>$</code>)\n");
        }
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_CHAR_CLASS:
        printf(
            "<strong>Character Class</strong> (%s any of):\n",
            node->data.char_class.negated ? "Does not match" : "Matches"
        );
        print_highlight_html(full_regex, node->content_offset, node->len);
        for (size_t i = 0; i < node->data.char_class.body.count; i++)
        {
            describe_node_html(node->data.char_class.body.nodes[i], indent + 1, full_regex);
        }
        break;
    case REGEX_NODE_CHAR_RANGE:
        printf(
            "Range: <code>%s</code> to <code>%s</code>\n",
            node->data.char_range.start->data.text,
            node->data.char_range.end->data.text
        );
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_CC_CHAR:
        printf("Character: <code>%s</code>\n", node->data.text);
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_CLASS_ESCAPE:
        printf("Escaped class: <code>%s</code>\n", node->data.text);
        print_highlight_html(full_regex, node->content_offset, node->len);
        break;
    case REGEX_NODE_PRIMARY_GROUP:
        printf("<strong>Grouped Expression</strong>:\n");
        print_highlight_html(full_regex, node->content_offset, node->len);
        describe_node_html(node->data.group_content, indent + 1, full_regex);
        break;
    case REGEX_NODE_LOOKAHEAD:
        printf("<strong>Positive Lookahead</strong> (matches if the following is present, but doesn't consume it):\n");
        print_highlight_html(full_regex, node->content_offset, node->len);
        describe_node_html(node->data.group_content, indent + 1, full_regex);
        break;
    case REGEX_NODE_NEGATIVE_LOOKAHEAD:
        printf(
            "<strong>Negative Lookahead</strong> (matches if the following is NOT present, but doesn't consume it):\n"
        );
        print_highlight_html(full_regex, node->content_offset, node->len);
        describe_node_html(node->data.group_content, indent + 1, full_regex);
        break;
    case REGEX_NODE_NON_CAPTURING_GROUP:
        printf("<strong>Non-Capturing Group</strong>:\n");
        print_highlight_html(full_regex, node->content_offset, node->len);
        describe_node_html(node->data.group_content, indent + 1, full_regex);
        break;
    default:
        /* For internal markers */
        break;
    }
    printf("</li>\n</ul>\n");
}

int
main(int argc, char ** argv)
{
    output_format_t format = OUTPUT_FORMAT_MARKDOWN;
    char const * regex_str = NULL;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "-f") == 0)
        {
            if (i + 1 < argc)
            {
                if (strcmp(argv[i + 1], "html") == 0)
                {
                    format = OUTPUT_FORMAT_HTML;
                }
                i++; // consume argument
            }
        }
        else
        {
            regex_str = argv[i];
        }
    }

    if (regex_str == NULL)
    {
        fprintf(stderr, "Usage: %s [-f <markdown|html>] <regex>\n", argv[0]);
        return 1;
    }

    epc_parser_list * const l = epc_parser_list_create();
    epc_parser_t * const p = create_regex_parser(l);

    epc_parse_session_t session = epc_parse_str(p, regex_str, NULL);

    if (session.result.is_error != false)
    {
        fprintf(
            stderr,
            "Error: %s at %zu:%zu\n",
            session.result.data.error->message,
            session.result.data.error->position.line,
            session.result.data.error->position.col
        );
        epc_parse_session_destroy(&session);
        epc_parser_list_free(l);
        return 1;
    }

    // Build AST
    epc_ast_hook_registry_t * const registry = epc_ast_hook_registry_create(REGEX_AST_ACTION_COUNT__);
    regex_ast_hook_registry_init(registry);

    epc_ast_result_t const ast_result = epc_ast_build(session.result.data.success, registry, NULL);
    regex_node_t * const ast_root = (regex_node_t *)ast_result.ast_root;

    if (ast_root != NULL)
    {
        if (format == OUTPUT_FORMAT_HTML)
        {
            printf("<!DOCTYPE html>\n<html>\n<head>\n<title>Regex Description</title>\n");
            printf("<style>.highlight { background-color: yellow; }</style>\n");
            printf("</head>\n<body>\n");
            printf("<h1>Regex Description for: <code>%s</code></h1>\n", regex_str);
            describe_node_html(ast_root, 0, regex_str);
            printf("</body>\n</html>\n");
        }
        else
        {
            printf("# Regex Description for: `%s`\n\n", regex_str);
            describe_node_md(ast_root, 0, regex_str);
        }
        regex_node_free(ast_root, NULL);
    }
    else
    {
        if (ast_result.has_error != false)
        {
            fprintf(stderr, "AST Build Error: %s\n", ast_result.error_message);
        }
        else
        {
            fprintf(stderr, "Failed to build AST.\n");
        }
    }

    epc_ast_hook_registry_free(registry);
    epc_parse_session_destroy(&session);
    epc_parser_list_free(l);

    return 0;
}
