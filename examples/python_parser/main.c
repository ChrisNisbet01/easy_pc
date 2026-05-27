#include "cpt_node.h"
#include "python_ast.h"
#include "python_ast_actions.h"
#include "python_ast_builder.h"
#include "python_ast_printer.h"
#include "python_token_ids.h"
#include "python_tokenizer.h"

#include <easy_pc/easy_pc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Keyword lookup ---

typedef struct
{
    char const * name;
    epc_token_id_t id;
} keyword_entry_t;

static keyword_entry_t const keywords[] = {
    {.name = "False", .id = TOKEN_KW_FALSE},
    {.name = "None", .id = TOKEN_KW_NONE},
    {.name = "True", .id = TOKEN_KW_TRUE},
    {.name = "and", .id = TOKEN_KW_AND},
    {.name = "as", .id = TOKEN_KW_AS},
    {.name = "assert", .id = TOKEN_KW_ASSERT},
    {.name = "async", .id = TOKEN_KW_ASYNC},
    {.name = "await", .id = TOKEN_KW_AWAIT},
    {.name = "break", .id = TOKEN_KW_BREAK},
    {.name = "class", .id = TOKEN_KW_CLASS},
    {.name = "continue", .id = TOKEN_KW_CONTINUE},
    {.name = "def", .id = TOKEN_KW_DEF},
    {.name = "del", .id = TOKEN_KW_DEL},
    {.name = "elif", .id = TOKEN_KW_ELIF},
    {.name = "else", .id = TOKEN_KW_ELSE},
    {.name = "except", .id = TOKEN_KW_EXCEPT},
    {.name = "finally", .id = TOKEN_KW_FINALLY},
    {.name = "for", .id = TOKEN_KW_FOR},
    {.name = "from", .id = TOKEN_KW_FROM},
    {.name = "global", .id = TOKEN_KW_GLOBAL},
    {.name = "if", .id = TOKEN_KW_IF},
    {.name = "import", .id = TOKEN_KW_IMPORT},
    {.name = "in", .id = TOKEN_KW_IN},
    {.name = "is", .id = TOKEN_KW_IS},
    {.name = "lambda", .id = TOKEN_KW_LAMBDA},
    {.name = "nonlocal", .id = TOKEN_KW_NONLOCAL},
    {.name = "not", .id = TOKEN_KW_NOT},
    {.name = "or", .id = TOKEN_KW_OR},
    {.name = "pass", .id = TOKEN_KW_PASS},
    // 'print' is intentionally omitted: it is a built-in function, not a keyword, in Python 3
    {.name = "raise", .id = TOKEN_KW_RAISE},
    {.name = "return", .id = TOKEN_KW_RETURN},
    {.name = "try", .id = TOKEN_KW_TRY},
    {.name = "while", .id = TOKEN_KW_WHILE},
    {.name = "with", .id = TOKEN_KW_WITH},
    {.name = "yield", .id = TOKEN_KW_YIELD},
    {.name = "match", .id = TOKEN_KW_MATCH},
    {.name = "case", .id = TOKEN_KW_CASE},
};

static epc_token_id_t
keyword_id_for_name(char const * name, size_t len)
{
    for (size_t i = 0; i < sizeof(keywords) / sizeof(keywords[0]); i++)
    {
        if (strlen(keywords[i].name) == len && strncmp(keywords[i].name, name, len) == 0)
            return keywords[i].id;
    }
    return TOKEN_NAME;
}

static keyword_entry_t const ids[] = {
    {.name = "**", .id = TOKEN_DOUBLE_STAR},
    {.name = "//", .id = TOKEN_DOUBLE_SLASH},
    {.name = "<<", .id = TOKEN_LEFT_SHIFT},
    {.name = ">>", .id = TOKEN_RIGHT_SHIFT},
    {.name = "==", .id = TOKEN_DOUBLE_EQUAL},
    {.name = "!=", .id = TOKEN_NOT_EQUAL},
    {.name = "<=", .id = TOKEN_LESS_EQUAL},
    {.name = ">=", .id = TOKEN_GREATER_EQUAL},
    {.name = "->", .id = TOKEN_ARROW},
    {.name = "+=", .id = TOKEN_PLUS_ASSIGN},
    {.name = "-=", .id = TOKEN_MINUS_ASSIGN},
    {.name = "*=", .id = TOKEN_STAR_ASSIGN},
    {.name = "/=", .id = TOKEN_SLASH_ASSIGN},
    {.name = "%=", .id = TOKEN_PERCENT_ASSIGN},
    {.name = "&=", .id = TOKEN_AMPERSAND_ASSIGN},
    {.name = "|=", .id = TOKEN_PIPE_ASSIGN},
    {.name = "^=", .id = TOKEN_CARET_ASSIGN},
    {.name = ":=", .id = TOKEN_WALRUS},

    {.name = "<<=", .id = TOKEN_LEFT_SHIFT_ASSIGN},
    {.name = ">>=", .id = TOKEN_RIGHT_SHIFT_ASSIGN},
    {.name = "**=", .id = TOKEN_DOUBLE_STAR_ASSIGN},
    {.name = "//=", .id = TOKEN_DOUBLE_SLASH_ASSIGN},
    {.name = "...", .id = TOKEN_ELLIPSIS},
};

// --- Token ID mapping from CPT node ---

static epc_token_id_t
token_id_for_single_char_node(char c)
{
    switch (c)
    {
    // Operators
    case '+':
        return TOKEN_PLUS;
    case '-':
        return TOKEN_MINUS;
    case '*':
        return TOKEN_STAR;
    case '/':
        return TOKEN_SLASH;
    case '%':
        return TOKEN_PERCENT;
    case '<':
        return TOKEN_LESS;
    case '>':
        return TOKEN_GREATER;
    case '=':
        return TOKEN_EQUAL;
    case '!':
        return TOKEN_BANG;
    case '&':
        return TOKEN_AMPERSAND;
    case '|':
        return TOKEN_PIPE;
    case '^':
        return TOKEN_CARET;
    case '~':
        return TOKEN_TILDE;
    case '@':
        return TOKEN_AT;

    // Delimiters
    case ',':
        return TOKEN_COMMA;
    case ':':
        return TOKEN_COLON;
    case ';':
        return TOKEN_SEMICOLON;
    case '.':
        return TOKEN_DOT;
    case '(':
        return TOKEN_LPAREN;
    case ')':
        return TOKEN_RPAREN;
    case '[':
        return TOKEN_LBRACKET;
    case ']':
        return TOKEN_RBRACKET;
    case '{':
        return TOKEN_LBRACE;
    case '}':
        return TOKEN_RBRACE;
    case '\\':
        return TOKEN_BACKSLASH;
    }

    return TOKEN_ERROR;
}

static epc_token_id_t
token_id_for_string(char const * str, size_t len)
{
    if (len == 1)
    {
        return token_id_for_single_char_node(*str);
    }

    for (size_t i = 0; i < sizeof(ids) / sizeof(ids[0]); i++)
    {
        if (strlen(ids[i].name) == len && strncmp(ids[i].name, str, len) == 0)
        {
            return ids[i].id;
        }
    }

    return TOKEN_ERROR;
}

// --- Indentation state ---

#define MAX_INDENT_STACK 64

typedef struct
{
    size_t stack[MAX_INDENT_STACK];
    size_t top;
    bool at_line_start;
    size_t current_indent;
    size_t indent_offset;  // offset of first whitespace char in current line's indent
    size_t indent_length;  // byte length of current line's indent whitespace
    bool has_tab;          // current indent contains tabs
    bool has_space;        // current indent contains spaces
    bool file_tabs_seen;   // any indent in this file has used tabs
    bool file_spaces_seen; // any indent in this file has used spaces
} indent_state_t;

static void
indent_state_init(indent_state_t * state)
{
    state->stack[0] = 0;
    state->top = 0;
    state->at_line_start = true;
    state->current_indent = 0;
    state->indent_offset = 0;
    state->indent_length = 0;
    state->has_tab = false;
    state->has_space = false;
    state->file_tabs_seen = false;
    state->file_spaces_seen = false;
}

// Build a view pointing at the indent whitespace (or zero-length if no whitespace).
static epc_parser_input_view_t
indent_view(indent_state_t const * state)
{
    epc_parser_input_view_t v = {0};
    v.offset = state->indent_offset;
    v.len = state->indent_length;
    return v;
}

// Process indent/dedent at the start of a logical line.
static void
handle_indent(epc_ast_builder_ctx_t * ctx, indent_state_t * state, epc_token_list_t * tokens)
{
    epc_parser_input_view_t view = indent_view(state);

    // Cross-line tab/space consistency check
    if (state->current_indent > 0)
    {
        if (state->has_tab && state->file_spaces_seen)
        {
            epc_ast_builder_set_error(ctx, "TabError: inconsistent use of tabs and spaces in indentation");
            return;
        }
        if (state->has_space && state->file_tabs_seen)
        {
            epc_ast_builder_set_error(ctx, "TabError: inconsistent use of tabs and spaces in indentation");
            return;
        }
        if (state->has_tab)
            state->file_tabs_seen = true;
        if (state->has_space)
            state->file_spaces_seen = true;
    }

    if (state->current_indent > state->stack[state->top])
    {
        state->top++;
        state->stack[state->top] = state->current_indent;
        epc_token_list_add(tokens, TOKEN_INDENT, view);
        printf(
            "  [%3zu] %-20s off=%-4zu len=%-3zu\n", epc_token_list_count(tokens) - 1, "INDENT", view.offset, view.len
        );
    }
    else if (state->current_indent < state->stack[state->top])
    {
        while (state->top > 0 && state->current_indent < state->stack[state->top])
        {
            state->top--;
            epc_token_list_add(tokens, TOKEN_DEDENT, view);
            printf(
                "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
                epc_token_list_count(tokens) - 1,
                "DEDENT",
                view.offset,
                view.len
            );
        }
    }
    state->at_line_start = false;
}

// --- CPT walker callbacks ---

typedef struct
{
    indent_state_t * indent;
    epc_token_list_t * tokens;
    epc_cpt_node_t * last_match;
} build_token_st_t;

static void
handle_newline_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)ctx;
    (void)children;
    (void)count;
    build_token_st_t * build_ctx = user_data;
    epc_token_list_t * tokens = build_ctx->tokens;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    indent_state_t * indent = build_ctx->indent;

    epc_token_list_add(tokens, TOKEN_NEWLINE, view);
    printf("  [%3zu] %-20s off=%-4zu len=%-3zu\n", epc_token_list_count(tokens) - 1, "NEWLINE", view.offset, view.len);
    indent->at_line_start = true;
    indent->current_indent = 0;
    indent->indent_offset = 0;
    indent->indent_length = 0;
    indent->has_tab = false;
    indent->has_space = false;

    build_ctx->last_match = node;
}

static void
handle_space_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    build_token_st_t * build_ctx = user_data;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    indent_state_t * indent = build_ctx->indent;

    char const * content = epc_cpt_node_get_content(node);
    bool is_tab = (content && content[0] == '\t');

    // Accumulate logical indent depth (tab = 8-space tab stop)
    if (is_tab)
        indent->current_indent += 8 - (indent->current_indent % 8);
    else
        indent->current_indent += 1;

    if (indent->at_line_start)
    {
        // Record whitespace byte position on first space in this line's indent
        if (indent->indent_length == 0)
            indent->indent_offset = view.offset;

        indent->indent_length += view.len;

        if (is_tab)
            indent->has_tab = true;
        else
            indent->has_space = true;

        if (indent->has_tab && indent->has_space)
        {
            epc_ast_builder_set_error(ctx, "TabError: inconsistent use of tabs and spaces in indentation");
            return;
        }
    }

    build_ctx->last_match = node;
}

static void
handle_identifier_action(
    epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data
)
{
    (void)children;
    (void)count;
    build_token_st_t * build_ctx = user_data;
    epc_token_list_t * tokens = build_ctx->tokens;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    indent_state_t * indent = build_ctx->indent;
    char const * content = epc_cpt_node_get_content(node);
    epc_parser_input_view_t v = epc_cpt_node_get_input_view(node);
    epc_token_id_t id = keyword_id_for_name(content, v.len);

    // At start of line: handle indent/dedent
    if (indent->at_line_start)
    {
        handle_indent(ctx, indent, tokens);
    }

    epc_token_list_add(tokens, id, view);

    printf(
        "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
        epc_token_list_count(tokens) - 1,
        token_id_name(id),
        view.offset,
        view.len
    );

    build_ctx->last_match = node;
}

static void
handle_number_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    build_token_st_t * build_ctx = user_data;
    epc_token_list_t * tokens = build_ctx->tokens;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    indent_state_t * indent = build_ctx->indent;
    epc_token_id_t id = TOKEN_NUMBER;

    // At start of line: handle indent/dedent
    if (indent->at_line_start)
    {
        handle_indent(ctx, indent, tokens);
    }

    epc_token_list_add(tokens, id, view);

    printf(
        "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
        epc_token_list_count(tokens) - 1,
        token_id_name(id),
        view.offset,
        view.len
    );

    build_ctx->last_match = node;
}

static void
handle_string_literal_action(
    epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data
)
{
    (void)children;
    (void)count;
    build_token_st_t * build_ctx = user_data;
    epc_token_list_t * tokens = build_ctx->tokens;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    indent_state_t * indent = build_ctx->indent;
    epc_token_id_t id = TOKEN_STRING;

    // At start of line: handle indent/dedent
    if (indent->at_line_start)
    {
        handle_indent(ctx, indent, tokens);
    }

    epc_token_list_add(tokens, id, view);

    printf(
        "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
        epc_token_list_count(tokens) - 1,
        token_id_name(id),
        view.offset,
        view.len
    );

    build_ctx->last_match = node;
}

static void
handle_generic_action(epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node, void ** children, int count, void * user_data)
{
    (void)children;
    (void)count;
    build_token_st_t * build_ctx = user_data;
    epc_token_list_t * tokens = build_ctx->tokens;
    epc_parser_input_view_t view = epc_cpt_node_get_input_view(node);
    indent_state_t * indent = build_ctx->indent;
    char const * s = epc_cpt_node_get_content(node);
    epc_token_id_t id = token_id_for_string(s, view.len);

    if (id == TOKEN_ERROR)
    {
        epc_ast_builder_set_error(ctx, "Unable to get token ID for %.*s", view.len, s);
        return;
    }

    // At start of line: handle indent/dedent
    if (indent->at_line_start)
    {
        handle_indent(ctx, indent, tokens);
    }

    epc_token_list_add(tokens, id, view);

    printf(
        "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
        epc_token_list_count(tokens) - 1,
        token_id_name(id),
        view.offset,
        view.len
    );

    build_ctx->last_match = node;
}

static bool
build_token_list(epc_parse_session_t const * session, epc_token_list_t * tokens, char const * input)
{
    if (session->result.is_error || session->result.data.success == NULL)
        return false;

    epc_cpt_node_t * root = session->result.data.success;

    epc_ast_hook_registry_t * registry = epc_ast_hook_registry_create(PYTHON_TOKENIZER_AST_ACTION_COUNT__);
    if (registry == NULL)
    {
        fprintf(stderr, "Failed to create AST hook registry\n");
        return false;
    }
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_NEWLINE, handle_newline_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_IDENTIFIER, handle_identifier_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_NUMBER, handle_number_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_STRING_LITERAL, handle_string_literal_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_GENERIC_TOKEN, handle_generic_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_GENERIC_TOKEN, handle_generic_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_GENERIC_TOKEN, handle_generic_action);
    epc_ast_hook_registry_set_action(registry, SEM_ACTION_SPACE, handle_space_action);

    printf("--- Token List ---\n");

    indent_state_t indent;
    indent_state_init(&indent);

    build_token_st_t ctx = {
        .indent = &indent,
        .tokens = tokens,
    };

    epc_ast_result_t result = epc_ast_build(root, registry, &ctx);

    if (result.has_error)
    {
        printf("Error during tokenization stage\n");
        printf("  %s\n", result.error_message);
        epc_ast_hook_registry_free(registry);

        return false;
    }

    // EOF: emit DEDENTs to return to level 0
    epc_parser_input_view_t last_view = epc_cpt_node_get_input_view(ctx.last_match);

    while (indent.top > 0)
    {
        indent.top--;
        epc_token_list_add(tokens, TOKEN_DEDENT, last_view);
        printf(
            "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
            epc_token_list_count(tokens) - 1,
            "DEDENT",
            last_view.offset,
            last_view.len
        );
    }
    epc_token_list_add(tokens, TOKEN_ENDMARKER, last_view);
    printf(
        "  [%3zu] %-20s off=%-4zu len=%-3zu\n",
        epc_token_list_count(tokens) - 1,
        "ENDMARKER",
        last_view.offset,
        last_view.len
    );

    printf("--- Total: %zu tokens ---\n", epc_token_list_count(tokens));

    size_t token_count = epc_token_list_count(tokens);
    printf(
        "\n--- Token list is ready for stage 2 reparse (%zu tokens, %zu bytes input) ---\n", token_count, strlen(input)
    );

    epc_ast_hook_registry_free(registry);

    return true;
}

int
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <python_file>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char const * filename = argv[1];
    FILE * fp = fopen(filename, "rb");
    if (fp == NULL)
    {
        perror("Failed to open file");
        return EXIT_FAILURE;
    }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char * input = (char *)malloc((size_t)file_size + 1);
    if (input == NULL)
    {
        fprintf(stderr, "Memory allocation error\n");
        fclose(fp);
        return EXIT_FAILURE;
    }
    size_t bytes_read = fread(input, 1, (size_t)file_size, fp);
    fclose(fp);
    input[bytes_read] = '\0';

    printf("=== Python Source ===\n%s\n", input);
    printf("=== End Source (%zu bytes) ===\n\n", bytes_read);

    epc_parser_list * parser_list = epc_parser_list_create();
    if (parser_list == NULL)
    {
        fprintf(stderr, "Failed to create parser list\n");
        free(input);
        return EXIT_FAILURE;
    }

    epc_parser_t * parser = create_python_tokenizer_parser(parser_list);
    if (parser == NULL)
    {
        fprintf(stderr, "Failed to create tokenizer parser\n");
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    epc_parse_session_t session = epc_parse_str(parser, input, NULL);

    if (session.result.is_error)
    {
        fprintf(stderr, "\nStage 1 parse FAILED:\n");
        if (session.result.data.error)
        {
            fprintf(stderr, "  Message: %s\n", session.result.data.error->message);
            fprintf(stderr, "  Expected: %s\n", session.result.data.error->expected);
            fprintf(stderr, "  Found: %s\n", session.result.data.error->found);
            fprintf(
                stderr,
                "  Line: %zu, Col: %zu\n",
                session.result.data.error->view.line_number,
                session.result.data.error->view.column_number
            );
        }
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("Stage 1 parse SUCCEEDED\n\n");

    // Print CPT for debugging
    {
        char * cpt_str = epc_cpt_to_string(session.internal_parse_ctx, session.result.data.success);
        if (cpt_str)
        {
            printf("=== CPT ===\n%s=== End CPT ===\n\n", cpt_str);
            free(cpt_str);
        }
    }

    // Build token list
    epc_token_list_t * tokens = epc_token_list_create(256);
    if (tokens == NULL)
    {
        fprintf(stderr, "Failed to create token list\n");
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    if (!build_token_list(&session, tokens, input))
    {
        fprintf(stderr, "Failed to build token list from CPT\n");
        epc_token_list_free(tokens);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    // === Stage 2: Parse token list into AST ===
    printf("\n=== Stage 2: Reparse with AST grammar ===\n");

    epc_parser_t * ast_parser = create_python_ast_parser(parser_list);
    if (ast_parser == NULL)
    {
        fprintf(stderr, "Failed to create AST parser\n");
        epc_token_list_free(tokens);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    bool reparse_ok = epc_parse_session_reparse(&session, ast_parser, tokens);
    // Tokens have been copied into the session; we can free our copy
    epc_token_list_free(tokens);
    tokens = NULL;

    if (!reparse_ok || session.result.is_error)
    {
        fprintf(stderr, "\nStage 2 reparse FAILED:\n");
        if (session.result.is_error && session.result.data.error)
        {
            fprintf(stderr, "  Message: %s\n", session.result.data.error->message);
            fprintf(stderr, "  Expected: %s\n", session.result.data.error->expected);
            fprintf(stderr, "  Found: %s\n", session.result.data.error->found);
            fprintf(
                stderr,
                "  Line: %zu, Col: %zu\n",
                session.result.data.error->view.line_number,
                session.result.data.error->view.column_number
            );
        }
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("Stage 2 parse SUCCEEDED\n");

    epc_cpt_node_t * ast_cpt_root = session.result.data.success;

    // Print CPT
    {
        char * cpt_str = epc_cpt_to_string(session.internal_parse_ctx, ast_cpt_root);
        if (cpt_str)
        {
            printf("\n=== AST CPT ===\n%s=== End AST CPT ===\n\n", cpt_str);
            free(cpt_str);
        }
    }

    // Build AST via semantic actions
    epc_ast_hook_registry_t * ast_registry = epc_ast_hook_registry_create(PYTHON_AST_AST_ACTION_COUNT__);
    if (ast_registry == NULL)
    {
        fprintf(stderr, "Failed to create AST hook registry\n");
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    py_ast_hook_registry_init(ast_registry);

    epc_ast_result_t ast_result = epc_ast_build(ast_cpt_root, ast_registry, NULL);

    if (ast_result.has_error)
    {
        fprintf(stderr, "\nAST build FAILED: %s\n", ast_result.error_message);
        epc_ast_hook_registry_free(ast_registry);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("AST build SUCCEEDED\n");

    // Print reconstructed source
    printf("\n=== Reconstructed Source ===\n");
    py_ast_node_t * ast_root = (py_ast_node_t *)ast_result.ast_root;
    py_ast_print(ast_root, 0);
    printf("=== End Reconstructed Source ===\n");

    // Cleanup
    py_node_free(ast_root);
    epc_ast_hook_registry_free(ast_registry);
    epc_parse_session_destroy(&session);
    epc_parser_list_free(parser_list);
    free(input);

    return EXIT_SUCCESS;
}
