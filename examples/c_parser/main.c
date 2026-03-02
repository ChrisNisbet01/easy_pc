#include "c_grammar.h"
#include "callbacks.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// --- Symbol Table Implementation ---

#define MAX_SYMBOLS 1024

typedef struct
{
    char * names[MAX_SYMBOLS];
    int count;
} symbol_table_t;

symbol_table_t *
symbol_table_create()
{
    return calloc(1, sizeof(symbol_table_t));
}

void
symbol_table_free(symbol_table_t * st)
{
    if (!st)
        return;
    for (int i = 0; i < st->count; i++)
    {
        free(st->names[i]);
    }
    free(st);
}

void
symbol_table_add(symbol_table_t * st, char const * name)
{
    if (st->count >= MAX_SYMBOLS)
        return;
    // Check if already exists
    for (int i = 0; i < st->count; i++)
    {
        if (strcmp(st->names[i], name) == 0)
            return;
    }
    st->names[st->count++] = strdup(name);
}

bool
symbol_table_contains(symbol_table_t * st, char const * name)
{
    for (int i = 0; i < st->count; i++)
    {
        if (strcmp(st->names[i], name) == 0)
            return true;
    }
    return false;
}

// --- Transactional Context ---

typedef struct
{
    symbol_table_t * symbols;
    char ** pending;
    int pending_count;
    int pending_capacity;
    int * marker_stack;
    int marker_top;
    int marker_capacity;
} parse_session_ctx_t;

parse_session_ctx_t *
session_ctx_create()
{
    parse_session_ctx_t * ctx = calloc(1, sizeof(parse_session_ctx_t));
    ctx->symbols = symbol_table_create();
    ctx->pending_capacity = 16;
    ctx->pending = malloc(sizeof(char *) * ctx->pending_capacity);
    ctx->marker_capacity = 16;
    ctx->marker_stack = malloc(sizeof(int) * ctx->marker_capacity);
    return ctx;
}

void
session_ctx_free(parse_session_ctx_t * ctx)
{
    if (!ctx)
        return;
    symbol_table_free(ctx->symbols);
    for (int i = 0; i < ctx->pending_count; i++)
    {
        free(ctx->pending[i]);
    }
    free(ctx->pending);
    free(ctx->marker_stack);
    free(ctx);
}

void
session_ctx_push_pending(parse_session_ctx_t * ctx, char const * name)
{
    if (ctx->pending_count >= ctx->pending_capacity)
    {
        ctx->pending_capacity *= 2;
        ctx->pending = realloc(ctx->pending, sizeof(char *) * ctx->pending_capacity);
    }
    ctx->pending[ctx->pending_count++] = strdup(name);
}

// --- GDL Callbacks and Predicates ---

bool
is_typedef_name(epc_cpt_node_t * token, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser_data;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session)
        return false;

    // Use semantic content to automatically handle whitespace if lexeme was used
    char const * name = epc_cpt_node_get_semantic_content(token);
    size_t len = epc_cpt_node_get_semantic_len(token);

    char * name_copy = strndup(name, len);
    bool found = symbol_table_contains(session->symbols, name_copy);
    free(name_copy);
    return found;
}

// Inner Wrap: Capture an identifier that might be a typedef
static void
on_capture_entry(epc_parser_t * parser, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser;
    (void)parse_ctx;
    (void)parser_data;
}

static bool
on_capture_exit(epc_parse_result_t result, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser_data;
    if (result.is_error)
        return true;

    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session)
        return true;

    char const * name = epc_cpt_node_get_semantic_content(result.data.success);
    size_t len = epc_cpt_node_get_semantic_len(result.data.success);
    char * name_copy = strndup(name, len);

    session_ctx_push_pending(session, name_copy);
    free(name_copy);

    return true;
}

// Outer Wrap: Commit or Discard pending typedefs
static void
on_commit_entry(epc_parser_t * parser, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser;
    (void)parser_data;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session)
        return;

    if (session->marker_top >= session->marker_capacity)
    {
        session->marker_capacity *= 2;
        session->marker_stack = realloc(session->marker_stack, sizeof(int) * session->marker_capacity);
    }
    session->marker_stack[session->marker_top++] = session->pending_count;
}

static bool
on_commit_exit(epc_parse_result_t result, epc_parser_ctx_t * parse_ctx, void * parser_data)
{
    (void)parser_data;
    parse_session_ctx_t * session = (parse_session_ctx_t *)parse_ctx_get_user_ctx(parse_ctx);
    if (!session || session->marker_top == 0)
        return true;

    int marker = session->marker_stack[--session->marker_top];

    if (!result.is_error)
    {
        // Success: Move pending to real table
        for (int i = marker; i < session->pending_count; i++)
        {
            symbol_table_add(session->symbols, session->pending[i]);
            printf("Committed typedef: '%s'\n", session->pending[i]);
            free(session->pending[i]);
        }
        session->pending_count = marker;
    }
    else
    {
        // Failure: Discard pending
        for (int i = marker; i < session->pending_count; i++)
        {
            free(session->pending[i]);
        }
        session->pending_count = marker;
    }

    return true;
}

epc_wrap_callbacks_t typedef_capture_callbacks = {on_capture_entry, on_capture_exit};
epc_wrap_callbacks_t typedef_commit_callbacks = {on_commit_entry, on_commit_exit};

// --- Main ---

int
main(int argc, char * argv[])
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <filename>\n", argv[0]);
        return EXIT_FAILURE;
    }

    char const * filename = argv[1];
    printf("Attempting to parse C file: %s\n", filename);

    epc_parser_list * list = epc_parser_list_create();
    if (!list)
    {
        fprintf(stderr, "Failed to create parser list.\n");
        return EXIT_FAILURE;
    }

    parse_session_ctx_t * session_ctx = session_ctx_create();

    // create_c_grammar_parser is generated from c_grammar.gdl
    epc_parser_t * c_parser = create_c_grammar_parser(list);
    if (!c_parser)
    {
        fprintf(stderr, "Failed to create C parser.\n");
        session_ctx_free(session_ctx);
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }

    // Parse from file with session context
    epc_parse_session_t session = epc_parse_file(c_parser, filename, session_ctx);

    if (session.result.is_error)
    {
        epc_parser_error_t * err = session.result.data.error;
        fprintf(stderr, "Parse Error: %s\n", err->message);
        fprintf(stderr, "At line %zu, col %zu\n", err->position.line + 1, err->position.col + 1);
        fprintf(stderr, "Expected: %s\n", err->expected ? err->expected : "unknown");
        fprintf(stderr, "Found: %s\n", err->found ? err->found : "unknown");

        epc_parse_session_destroy(&session);
        session_ctx_free(session_ctx);
        epc_parser_list_free(list);
        return EXIT_FAILURE;
    }

    printf("Successfully parsed the C file!\n");

    epc_parse_session_destroy(&session);
    session_ctx_free(session_ctx);
    epc_parser_list_free(list);
    return EXIT_SUCCESS;
}
