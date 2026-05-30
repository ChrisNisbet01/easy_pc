#include "gdl_bootstrap_generator.h"
#include "gdl_code_generator.h"
#include "gdl_compiler_ast_actions.h"
#include "gdl_parser.h"
#include "gdl_token_ids.h"
#include "gdl_tokenizer_actions.h"
#include "gdl_tokenizer_parser.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

static bool
directory_exists(char const * path)
{
    struct stat sb;

    if (stat(path, &sb) != 0 || !S_ISDIR(sb.st_mode))
    {
        return false;
    }
    return true;
}

int
main(int argc, char ** argv)
{
    int exit_code = EXIT_SUCCESS;
    char const * gdl_filepath = NULL;
    char const * output_dir = ".";
    char const * header_to_include = NULL;
    bool bootstrap_ast = false;

    for (int i = 1; i < argc; ++i)
    {
        if (strcmp(argv[i], "--version") == 0 || strcmp(argv[i], "-v") == 0)
        {
            printf("gdl_compiler version: %s\n", GDL_COMPILER_VERSION);
            printf("easy_pc library version: %s\n", epc_get_version());
            return EXIT_SUCCESS;
        }
        else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0)
        {
            printf("Usage: %s <gdl_file> [options]\n", argv[0]);
            printf("Options:\n");
            printf("  --output-dir <dir>  Set the output directory for generated files.\n");
            printf("  --header <header>   Add an extra #include to the generated .c file.\n");
            printf("  --bootstrap-ast     Generate skeleton AST and semantic action files.\n");
            printf("  --version, -v       Show version information and exit.\n");
            printf("  --help, -h          Show this help message and exit.\n");
            return EXIT_SUCCESS;
        }
        else if (strncmp(argv[i], "--output-dir", strlen("--output-dir")) == 0)
        {
            char const * arg = argv[i];
            char const * value_start = strchr(arg, '=');
            if (value_start)
            {
                output_dir = value_start + 1;
            }
            else
            {
                if (i + 1 < argc)
                {
                    output_dir = argv[++i];
                }
                else
                {
                    fprintf(stderr, "Error: --output-dir requires an argument.\n");
                    return EXIT_FAILURE;
                }
            }
        }
        else if (strncmp(argv[i], "--header", strlen("--header")) == 0)
        {
            char const * arg = argv[i];
            char const * value_start = strchr(arg, '=');
            if (value_start)
            {
                header_to_include = value_start + 1;
            }
            else
            {
                if (i + 1 < argc)
                {
                    header_to_include = argv[++i];
                }
                else
                {
                    fprintf(stderr, "Error: --header requires an argument.\n");
                    return EXIT_FAILURE;
                }
            }
        }
        else if (strcmp(argv[i], "--bootstrap-ast") == 0)
        {
            bootstrap_ast = true;
        }
        else if (gdl_filepath == NULL)
        {
            gdl_filepath = argv[i];
        }
        else
        {
            fprintf(stderr, "Usage: %s <gdl_file> [options]\n", argv[0]);
            fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
            return EXIT_FAILURE;
        }
    }

    if (gdl_filepath == NULL)
    {
        fprintf(stderr, "Usage: %s <gdl_file> [options]\n", argv[0]);
        fprintf(stderr, "Try '%s --help' for more information.\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Read input file into memory
    FILE * fp = fopen(gdl_filepath, "rb");
    if (fp == NULL)
    {
        fprintf(stderr, "Error: Could not open file '%s'.\n", gdl_filepath);
        return EXIT_FAILURE;
    }
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char * input = (char *)malloc((size_t)file_size + 1);
    if (input == NULL)
    {
        fprintf(stderr, "Error: Memory allocation failed.\n");
        fclose(fp);
        return EXIT_FAILURE;
    }
    size_t bytes_read = fread(input, 1, (size_t)file_size, fp);
    fclose(fp);
    input[bytes_read] = '\0';

    // Create parser list (shared between tokenizer and AST parser)
    epc_parser_list * parser_list = epc_parser_list_create();
    if (parser_list == NULL)
    {
        fprintf(stderr, "Failed to create parser list.\n");
        free(input);
        return EXIT_FAILURE;
    }

    // === Stage 1: Tokenize ===
    printf("--- Stage 1: Tokenizing ---\n");

    epc_parser_t * tokenizer_parser = create_gdl_tokenizer_parser(parser_list);
    if (tokenizer_parser == NULL)
    {
        fprintf(stderr, "Failed to create tokenizer parser.\n");
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    epc_parse_session_t session = epc_parse_str(tokenizer_parser, input, NULL);

    if (session.result.is_error)
    {
        fprintf(
            stderr,
            "Tokenizer Parsing Error: %s at input offset %zu\n",
            session.result.data.error->message,
            session.result.data.error->view.offset
        );
        fprintf(
            stderr,
            "    Expected %s, found: %s at line %zu, col %zu\n",
            session.result.data.error->expected,
            session.result.data.error->found,
            session.result.data.error->view.line_number,
            session.result.data.error->view.column_number
        );
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("Tokenizer parse succeeded. Building token list...\n");

    // Build token list from tokenizer CPT
    epc_ast_hook_registry_t * tokenizer_registry = epc_ast_hook_registry_create(TOKENIZER_ACTION_COUNT);
    if (tokenizer_registry == NULL)
    {
        fprintf(stderr, "Failed to create tokenizer AST hook registry.\n");
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    gdl_tokenizer_ctx_t tokenizer_ctx;
    tokenizer_ctx.tokens = epc_token_list_create(256);
    tokenizer_ctx.last_match = NULL;
    gdl_tokenizer_hook_registry_init(tokenizer_registry, &tokenizer_ctx);

    epc_ast_result_t token_build = epc_ast_build(session.result.data.success, tokenizer_registry, &tokenizer_ctx);

    if (token_build.has_error)
    {
        fprintf(stderr, "Token list building error: %s\n", token_build.error_message);
        epc_token_list_free(tokenizer_ctx.tokens);
        epc_ast_hook_registry_free(tokenizer_registry);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("Token list built: %zu tokens.\n", epc_token_list_count(tokenizer_ctx.tokens));

    // Tokenizer registry no longer needed
    epc_ast_hook_registry_free(tokenizer_registry);

    // === Stage 2: Parse with GDL grammar ===
    printf("--- Stage 2: Parsing GDL grammar ---\n");

    epc_parser_t * gdl_grammar_parser = create_gdl_parser(parser_list);
    if (gdl_grammar_parser == NULL)
    {
        fprintf(stderr, "Failed to create GDL grammar parser.\n");
        epc_token_list_free(tokenizer_ctx.tokens);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    bool reparse_ok = epc_parse_session_reparse(&session, gdl_grammar_parser, tokenizer_ctx.tokens);
    epc_token_list_free(tokenizer_ctx.tokens);

    if (!reparse_ok || session.result.is_error)
    {
        fprintf(
            stderr,
            "GDL Grammar Parsing Error: %s at input offset %zu\n",
            session.result.data.error->message,
            session.result.data.error->view.offset
        );
        fprintf(
            stderr,
            "    Expected %s, found: %s at line %zu, col %zu\n",
            session.result.data.error->expected,
            session.result.data.error->found,
            session.result.data.error->view.line_number,
            session.result.data.error->view.column_number
        );
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("GDL grammar parsed successfully! Building AST...\n");

    // === Build AST ===
    epc_ast_hook_registry_t * ast_registry = epc_ast_hook_registry_create(GDL_AST_ACTION_MAX);
    if (ast_registry == NULL)
    {
        fprintf(stderr, "Error: Failed to create AST hook registry.\n");
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    gdl_ast_hook_registry_init(ast_registry, NULL);

    epc_ast_result_t ast_build_result = epc_ast_build(session.result.data.success, ast_registry, NULL);

    if (ast_build_result.has_error)
    {
        fprintf(stderr, "GDL AST Building Error: %s\n", ast_build_result.error_message);
        epc_ast_hook_registry_free(ast_registry);
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        free(input);
        return EXIT_FAILURE;
    }

    printf("GDL AST built successfully!\n");

    // === Code generation ===
    char * gdl_filename = strrchr(gdl_filepath, '/');
    if (gdl_filename)
    {
        gdl_filename++;
    }
    else
    {
        gdl_filename = (char *)gdl_filepath;
    }
    char base_name[256];
    strncpy(base_name, gdl_filename, sizeof(base_name) - 1);
    base_name[sizeof(base_name) - 1] = '\0';

    char * dot = strrchr(base_name, '.');
    if (dot)
    {
        *dot = '\0';
    }

    if (!gdl_generate_c_code((gdl_ast_node_t *)ast_build_result.ast_root, base_name, output_dir, header_to_include))
    {
        fprintf(stderr, "C code generation failed.\n");
        exit_code = EXIT_FAILURE;
    }
    else
    {
        printf("C code generation completed successfully.\n");
    }

    if (bootstrap_ast)
    {
        char const * bootstrap_output_dir;
        char bootstrap_subdir[512];
        snprintf(bootstrap_subdir, sizeof(bootstrap_subdir), "%s/bootstrap", output_dir);
        if (directory_exists(bootstrap_subdir))
        {
            bootstrap_output_dir = bootstrap_subdir;
        }
        else
        {
            bootstrap_output_dir = output_dir;
        }
        printf("AST bootstrap files generation requested.\n");
        generate_ast_bootstrap_files((gdl_ast_node_t *)ast_build_result.ast_root, base_name, bootstrap_output_dir);
        printf("AST bootstrap files generation completed.\n");
    }

    gdl_ast_node_free((gdl_ast_node_t *)ast_build_result.ast_root, NULL);

    // Cleanup
    epc_ast_hook_registry_free(ast_registry);
    epc_parse_session_destroy(&session);
    epc_parser_list_free(parser_list);
    free(input);

    return exit_code;
}
