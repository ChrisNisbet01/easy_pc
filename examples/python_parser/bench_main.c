#include "cpt_node.h"
#include "python_token_ids.h"

#include <easy_pc/easy_pc.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

// Generated parser constructors
extern epc_parser_t * create_python_tokenizer_parser(epc_parser_list * list);
extern epc_parser_t * create_python_ast_parser(epc_parser_list * list);
extern epc_parser_t * create_python_ast_bench_parser(epc_parser_list * list);

typedef struct
{
    char const * name;
    double elapsed_ms;
} bench_result_t;

static double
timespec_to_ms(struct timespec const * t)
{
    return t->tv_sec * 1000.0 + t->tv_nsec / 1.0e6;
}

static bool
run_parse(char const * input, epc_parser_t * (*make_ast_parser)(epc_parser_list *), bench_result_t * stages, int max_stages)
{
    int stage = 0;

    // --- Stage 1: Tokenizer ---
    epc_parser_list * list = epc_parser_list_create();
    if (list == NULL) return false;

    struct timespec t0, t1;
    clock_gettime(CLOCK_MONOTONIC, &t0);

    epc_parser_t * tok_parser = create_python_tokenizer_parser(list);
    if (tok_parser == NULL) { epc_parser_list_free(list); return false; }

    epc_parse_session_t session = epc_parse_str(tok_parser, input, NULL);
    if (session.result.is_error) { epc_parse_session_destroy(&session); epc_parser_list_free(list); return false; }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (stage < max_stages) stages[stage++] = (bench_result_t){"Tokenizer", timespec_to_ms(&t1) - timespec_to_ms(&t0)};

    // Build token list from CPT
    {
        // Quick tokenizer pass: just iterate CPT children to count tokens
        // For fairness, both grammars use the same token list building process
    }

    clock_gettime(CLOCK_MONOTONIC, &t0);

    epc_token_list_t * tokens = epc_token_list_create(256);
    if (tokens == NULL) { epc_parse_session_destroy(&session); epc_parser_list_free(list); return false; }

    // Minimal token building: emit NEWLINE/INDENT/DEDENT/ENDMARKER only
    // Walk the tokenizer CPT to produce tokens
    epc_cpt_node_t * root = session.result.data.success;
    size_t token_count = 0;

    // Flatten: emit tokens by visiting the sequence children
    // The tokenizer produces a flat list of token nodes under the root
    for (int i = 0; i < root->children_count; i++)
    {
        epc_cpt_node_t * child = root->children[i];
        if (child == NULL) continue;
        epc_parser_input_view_t view = epc_cpt_node_get_input_view(child);
        char const * tag = child->tag;

        // Map tag names to token IDs (same logic as the full token builder)
        epc_token_id_t id = TOKEN_ERROR;
        if (strcmp(tag, "newline") == 0) id = TOKEN_NEWLINE;
        else if (strcmp(tag, "space") == 0) continue; // skip spaces
        else if (strcmp(tag, "identifier") == 0)
        {
            char const * content = epc_cpt_node_get_content(child);
            if (content && strlen(content) == view.len)
            {
                // Simple keyword detection (just check first char)
                if (view.len == 4 && strncmp(content, "pass", 4) == 0) id = TOKEN_KW_PASS;
                else if (view.len == 5 && strncmp(content, "break", 5) == 0) id = TOKEN_KW_BREAK;
                else if (view.len == 8 && strncmp(content, "continue", 8) == 0) id = TOKEN_KW_CONTINUE;
                else if (view.len == 6 && strncmp(content, "return", 6) == 0) id = TOKEN_KW_RETURN;
                else if (view.len == 2 && strncmp(content, "if", 2) == 0) id = TOKEN_KW_IF;
                else if (view.len == 3 && strncmp(content, "def", 3) == 0) id = TOKEN_KW_DEF;
                else if (view.len == 4 && strncmp(content, "True", 4) == 0) id = TOKEN_KW_TRUE;
                else if (view.len == 5 && strncmp(content, "False", 5) == 0) id = TOKEN_KW_FALSE;
                else if (view.len == 4 && strncmp(content, "None", 4) == 0) id = TOKEN_KW_NONE;
                else if (view.len == 3 && strncmp(content, "and", 3) == 0) id = TOKEN_KW_AND;
                else if (view.len == 2 && strncmp(content, "or", 2) == 0) id = TOKEN_KW_OR;
                else if (view.len == 3 && strncmp(content, "not", 3) == 0) id = TOKEN_KW_NOT;
                else if (view.len == 3 && strncmp(content, "for", 3) == 0) id = TOKEN_KW_FOR;
                else if (view.len == 5 && strncmp(content, "while", 5) == 0) id = TOKEN_KW_WHILE;
                else if (view.len == 5 && strncmp(content, "class", 5) == 0) id = TOKEN_KW_CLASS;
                else if (view.len == 3 && strncmp(content, "try", 3) == 0) id = TOKEN_KW_TRY;
                else if (view.len == 4 && strncmp(content, "with", 4) == 0) id = TOKEN_KW_WITH;
                else if (view.len == 5 && strncmp(content, "yield", 5) == 0) id = TOKEN_KW_YIELD;
                else if (view.len == 6 && strncmp(content, "import", 6) == 0) id = TOKEN_KW_IMPORT;
                else if (view.len == 4 && strncmp(content, "from", 4) == 0) id = TOKEN_KW_FROM;
                else if (view.len == 6 && strncmp(content, "global", 6) == 0) id = TOKEN_KW_GLOBAL;
                else if (view.len == 7 && strncmp(content, "nonlocal", 7) == 0) id = TOKEN_KW_NONLOCAL;
                else if (view.len == 5 && strncmp(content, "raise", 5) == 0) id = TOKEN_KW_RAISE;
                else if (view.len == 6 && strncmp(content, "assert", 6) == 0) id = TOKEN_KW_ASSERT;
                else if (view.len == 3 && strncmp(content, "del", 3) == 0) id = TOKEN_KW_DEL;
                else if (view.len == 4 && strncmp(content, "elif", 4) == 0) id = TOKEN_KW_ELIF;
                else if (view.len == 4 && strncmp(content, "else", 4) == 0) id = TOKEN_KW_ELSE;
                else id = TOKEN_NAME;
            }
            else
            {
                id = TOKEN_NAME;
            }
        }
        else if (strcmp(tag, "number") == 0) id = TOKEN_NUMBER;
        else if (strcmp(tag, "string_literal") == 0) id = TOKEN_STRING;
        else if (strcmp(tag, "generic_token") == 0)
        {
            char const * content = epc_cpt_node_get_content(child);
            if (content && view.len > 0)
            {
                char c = content[0];
                if (view.len == 1 && c == '(') id = TOKEN_LPAREN;
                else if (view.len == 1 && c == ')') id = TOKEN_RPAREN;
                else if (view.len == 1 && c == ':') id = TOKEN_COLON;
                else if (view.len == 1 && c == ',') id = TOKEN_COMMA;
                else if (view.len == 1 && c == '=') id = TOKEN_EQUAL;
                else if (view.len == 1 && c == '+') id = TOKEN_PLUS;
                else if (view.len == 1 && c == '-') id = TOKEN_MINUS;
                else if (view.len == 1 && c == '*') id = TOKEN_STAR;
                else if (view.len == 1 && c == '[') id = TOKEN_LBRACKET;
                else if (view.len == 1 && c == ']') id = TOKEN_RBRACKET;
                else if (view.len == 1 && c == '{') id = TOKEN_LBRACE;
                else if (view.len == 1 && c == '}') id = TOKEN_RBRACE;
                else if (view.len == 1 && c == '.') id = TOKEN_DOT;
                else if (view.len == 1 && c == '@') id = TOKEN_AT;
                else if (view.len == 1 && c == '&') id = TOKEN_AMPERSAND;
                else if (view.len == 1 && c == '|') id = TOKEN_PIPE;
                else if (view.len == 1 && c == '^') id = TOKEN_CARET;
                else if (view.len == 1 && c == '~') id = TOKEN_TILDE;
                else if (view.len == 1 && c == '<') id = TOKEN_LESS;
                else if (view.len == 1 && c == '>') id = TOKEN_GREATER;
                else if (view.len == 1 && c == '/') id = TOKEN_SLASH;
                else if (view.len == 1 && c == '%') id = TOKEN_PERCENT;
                else if (view.len == 1 && c == '!') id = TOKEN_BANG;
                else if (view.len == 2 && strncmp(content, "==", 2) == 0) id = TOKEN_DOUBLE_EQUAL;
                else if (view.len == 2 && strncmp(content, "!=", 2) == 0) id = TOKEN_NOT_EQUAL;
                else if (view.len == 2 && strncmp(content, "<=", 2) == 0) id = TOKEN_LESS_EQUAL;
                else if (view.len == 2 && strncmp(content, ">=", 2) == 0) id = TOKEN_GREATER_EQUAL;
                else if (view.len == 2 && strncmp(content, "->", 2) == 0) id = TOKEN_ARROW;
                else if (view.len == 2 && strncmp(content, "**", 2) == 0) id = TOKEN_DOUBLE_STAR;
                else if (view.len == 2 && strncmp(content, "//", 2) == 0) id = TOKEN_DOUBLE_SLASH;
                else if (view.len == 2 && strncmp(content, "<<", 2) == 0) id = TOKEN_LEFT_SHIFT;
                else if (view.len == 2 && strncmp(content, ">>", 2) == 0) id = TOKEN_RIGHT_SHIFT;
                else if (view.len == 3 && strncmp(content, "...", 3) == 0) id = TOKEN_ELLIPSIS;
                else id = TOKEN_ERROR;
            }
        }

        if (id != TOKEN_ERROR)
        {
            epc_token_list_add(tokens, id, view);
            token_count++;
        }
    }

    // EOF: emit DEDENTs and ENDMARKER
    epc_parser_input_view_t last_view = {0};
    if (token_count > 0)
        last_view = epc_cpt_node_get_input_view(root->children[root->children_count - 1]);

    epc_token_list_add(tokens, TOKEN_ENDMARKER, last_view);
    token_count++;

    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (stage < max_stages) stages[stage++] = (bench_result_t){"Build tokens", timespec_to_ms(&t1) - timespec_to_ms(&t0)};

    // --- Stage 2: Reparse token list into AST ---
    clock_gettime(CLOCK_MONOTONIC, &t0);

    epc_parser_t * ast_parser = make_ast_parser(list);
    if (ast_parser == NULL)
    {
        epc_token_list_free(tokens);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return false;
    }

    bool reparse_ok = epc_parse_session_reparse(&session, ast_parser, tokens);
    epc_token_list_free(tokens);

    if (!reparse_ok || session.result.is_error)
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
        return false;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    if (stage < max_stages) stages[stage++] = (bench_result_t){"AST parse", timespec_to_ms(&t1) - timespec_to_ms(&t0)};

    epc_parse_session_destroy(&session);
    epc_parser_list_free(list);

    return true;
}

int
main(int argc, char ** argv)
{
    if (argc < 2)
    {
        fprintf(stderr, "Usage: %s <python_file> [iterations]\n", argv[0]);
        return EXIT_FAILURE;
    }

    char const * filename = argv[1];
    int iterations = (argc > 2) ? atoi(argv[2]) : 5;
    if (iterations < 1) iterations = 1;

    FILE * fp = fopen(filename, "rb");
    if (fp == NULL) { perror("Failed to open file"); return EXIT_FAILURE; }

    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    char * input = malloc((size_t)file_size + 1);
    if (input == NULL) { fclose(fp); return EXIT_FAILURE; }
    size_t bytes_read = fread(input, 1, (size_t)file_size, fp);
    fclose(fp);
    input[bytes_read] = '\0';

    printf("Benchmark: %s (%zu bytes, %d iterations)\n\n", filename, (size_t)file_size, iterations);

    typedef struct
    {
        char const * name;
        epc_parser_t * (*make_parser)(epc_parser_list *);
        double total_stage1;
        double total_stage2;
        double min_stage2;
        double max_stage2;
    } bench_config_t;

    bench_config_t configs[] = {
        {"Original grammar", create_python_ast_parser, 0, 0, 1e9, 0},
        {"Commit grammar", create_python_ast_bench_parser, 0, 0, 1e9, 0},
    };
    int num_configs = sizeof(configs) / sizeof(configs[0]);

    for (int iter = 0; iter < iterations; iter++)
    {
        for (int c = 0; c < num_configs; c++)
        {
            bench_result_t stages[3] = {{0}};
            if (!run_parse(input, configs[c].make_parser, stages, 3))
            {
                fprintf(stderr, "  %s iter %d: FAILED\n", configs[c].name, iter + 1);
                // Free input and exit on failure
                free(input);
                return EXIT_FAILURE;
            }

            double stage1 = stages[0].elapsed_ms + stages[1].elapsed_ms;
            double stage2 = stages[2].elapsed_ms;

            configs[c].total_stage1 += stage1;
            configs[c].total_stage2 += stage2;
            if (stage2 < configs[c].min_stage2) configs[c].min_stage2 = stage2;
            if (stage2 > configs[c].max_stage2) configs[c].max_stage2 = stage2;
        }
    }

    printf("%-25s  %12s  %12s  %12s  %12s  %12s\n",
           "Grammar", "Stage1(ms)", "Stage2(ms)", "Min(ms)", "Max(ms)", "Total(ms)");
    printf("%s\n", "----------------------------------------------------------------------");

    for (int c = 0; c < num_configs; c++)
    {
        double avg_s1 = configs[c].total_stage1 / iterations;
        double avg_s2 = configs[c].total_stage2 / iterations;
        printf("%-25s  %12.3f  %12.3f  %12.3f  %12.3f  %12.3f\n",
               configs[c].name, avg_s1, avg_s2,
               configs[c].min_stage2, configs[c].max_stage2,
               avg_s1 + avg_s2);
    }

    free(input);
    return 0;
}
