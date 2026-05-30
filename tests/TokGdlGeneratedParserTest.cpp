#include "easy_pc/easy_pc.h"

#include "CppUTest/TestHarness.h"

extern "C" {
#include "gdl_ast.h"
#include "gdl_code_generator.h"
#include "gdl_compiler_ast_actions.h"
#include "gdl_parser.h"
#include "gdl_token_ids.h"
#include "gdl_tokenizer_actions.h"
#include "gdl_tokenizer_parser.h"
}

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

TEST_GROUP(TokGdlGeneratedParserTest)
{
    epc_parser_list * parser_list;
    epc_parse_session_t session;
    gdl_ast_node_t * ast_root;

    void setup() override
    {
        int cmd_res = system("rm -f tok_test_language.h tok_test_language.c tok_test_language_actions.h");
        (void)cmd_res;
        parser_list = epc_parser_list_create();
        CHECK(parser_list != NULL);
        ast_root = NULL;
        session = (epc_parse_session_t){0};
    }

    void generate_ast(char const * gdl_input)
    {
        epc_parser_t * tokenizer = create_gdl_tokenizer_parser(parser_list);
        CHECK(tokenizer != NULL);

        session = epc_parse_str(tokenizer, gdl_input, NULL);
        CHECK_FALSE(session.result.is_error);

        epc_ast_hook_registry_t * tokenizer_registry = epc_ast_hook_registry_create(TOKENIZER_ACTION_COUNT);
        CHECK(tokenizer_registry != NULL);

        gdl_tokenizer_ctx_t tokenizer_ctx;
        tokenizer_ctx.tokens = epc_token_list_create(256);
        tokenizer_ctx.last_match = NULL;
        gdl_tokenizer_hook_registry_init(tokenizer_registry, &tokenizer_ctx);

        epc_ast_result_t token_build = epc_ast_build(session.result.data.success, tokenizer_registry, &tokenizer_ctx);
        CHECK_FALSE(token_build.has_error);
        epc_ast_hook_registry_free(tokenizer_registry);

        epc_parser_t * gdl_grammar = create_gdl_parser(parser_list);
        CHECK(gdl_grammar != NULL);

        bool reparse_ok = epc_parse_session_reparse(&session, gdl_grammar, tokenizer_ctx.tokens);
        epc_token_list_free(tokenizer_ctx.tokens);
        CHECK_TRUE(reparse_ok);
        CHECK_FALSE(session.result.is_error);

        epc_ast_hook_registry_t * ast_registry = epc_ast_hook_registry_create(GDL_AST_ACTION_MAX);
        CHECK(ast_registry != NULL);
        gdl_ast_hook_registry_init(ast_registry, NULL);

        epc_ast_result_t ast_build = epc_ast_build(session.result.data.success, ast_registry, NULL);
        CHECK_FALSE(ast_build.has_error);
        CHECK(ast_build.ast_root != NULL);

        ast_root = (gdl_ast_node_t *)ast_build.ast_root;
    }

    void teardown() override
    {
        if (ast_root)
        {
            gdl_ast_node_free(ast_root, NULL);
        }
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
    }
};

TEST(TokGdlGeneratedParserTest, GeneratesFilesSuccessfully)
{
    char const * output_dir = ".";
    char const * base_name = "tok_test_language";
    char const * gdl_input = "Greeting = \"hello\";\n"
                             "World = \"world\";\n"
                             "CharX = 'x';\n"
                             "CharY = 'y';\n"
                             "CharZ = 'z';\n"
                             "SeqGreeting = Greeting World;\n"
                             "SeqChar = CharX CharY CharZ;\n"
                             "SimpleRule = SeqGreeting | SeqChar;\n"
                             "EOI = eoi;\n"
                             "Program = SimpleRule EOI @EPC_AST_SEMANTIC_ACTION_PROGRAM_RULE;\n";

    generate_ast(gdl_input);
    CHECK_TRUE(gdl_generate_c_code(ast_root, base_name, output_dir, NULL));
}

TEST(TokGdlGeneratedParserTest, GeneratesFilesWithFwdRef)
{
    char const * output_dir = ".";
    char const * base_name = "tok_fwd_ref_test";
    char const * gdl_input = "A = \"a\";\n"
                             "B = \"b\";\n"
                             "C = A B D;\n"
                             "D = 'd';\n"
                             "EOI = eoi;\n"
                             "Program = C EOI;\n";

    generate_ast(gdl_input);
    CHECK_TRUE(gdl_generate_c_code(ast_root, base_name, output_dir, NULL));
}

TEST(TokGdlGeneratedParserTest, GeneratesFilesWithTokenLiteral)
{
    char const * output_dir = ".";
    char const * base_name = "tok_token_test";
    char const * gdl_input = "MyToken = <MY_TOKEN>;\n"
                             "EOI = eoi;\n"
                             "Program = MyToken EOI;\n";

    generate_ast(gdl_input);
    CHECK_TRUE(gdl_generate_c_code(ast_root, base_name, output_dir, NULL));
}

TEST(TokGdlGeneratedParserTest, GeneratesFilesWithKeyword)
{
    char const * output_dir = ".";
    char const * base_name = "tok_kw_test";
    char const * gdl_input = "MyRule = alpha;\n"
                             "EOI = eoi;\n"
                             "Program = MyRule EOI;\n";

    generate_ast(gdl_input);
    CHECK_TRUE(gdl_generate_c_code(ast_root, base_name, output_dir, NULL));
}
