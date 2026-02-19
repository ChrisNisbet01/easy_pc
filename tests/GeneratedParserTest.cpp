#include "CppUTest/TestHarness.h"
#include "easy_pc/easy_pc.h"

extern "C" {
#include "gdl_parser.h"
#include "gdl_ast_builder.h"
#include "gdl_code_generator.h"
}

#include <string.h>
#include <stdio.h>
#include <stdlib.h> // For system()

TEST_GROUP(GeneratedParserTest)
{
    epc_parser_list *parser_list;
    epc_parser_t *gdl_grammar;
    epc_parse_session_t session;
    gdl_ast_builder_data_t ast_builder_data;

    void setup() override
    {
        // Clean up generated files
        system("rm -f simple_test_language.h simple_test_language.c simple_test_language_actions.h");
        parser_list = epc_parser_list_create();
        gdl_grammar = create_gdl_parser(parser_list);

        gdl_ast_builder_init(&ast_builder_data);
    }

    void generate_ast(char const * gdl_input)
    {
        epc_cpt_visitor_t ast_builder_visitor = {
            .enter_node = gdl_ast_builder_enter_node,
            .exit_node = gdl_ast_builder_exit_node,
            .user_data = &ast_builder_data
        };
        session = epc_parse_input(gdl_grammar, gdl_input);
        CHECK_FALSE(session.result.is_error);
        epc_cpt_visit_nodes(session.result.data.success, &ast_builder_visitor);
        CHECK_FALSE(ast_builder_data.has_error);
        CHECK(ast_builder_data.ast_root != NULL);
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
        gdl_ast_builder_cleanup(&ast_builder_data);
        // Clean up generated files
        // system("rm -f simple_test_language.h simple_test_language.c simple_test_language_actions.h");
    }
};

TEST(GeneratedParserTest, GeneratesFilesSuccessfully)
{
    // Output directory for generated files in the test environment
    const char *output_dir = ".";
    const char *base_name = "simple_test_language";
    // Create AST for the simple test language
    const char *gdl_input =
        "Greeting = \"hello\";\n"
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

    CHECK_TRUE(gdl_generate_c_code(ast_builder_data.ast_root, base_name, output_dir));
}

TEST(GeneratedParserTest, GeneratesFilesWithFwdRefSuccessfully)
{
    // Output directory for generated files in the test environment
    const char *output_dir = ".";
    const char *base_name = "simple_fwd_ref_test_language";
    // Create AST for the simple test language
    const char *gdl_input =
        "Greeting = \"hello\";\n"
        "World = \"world\";\n"
        "CharX = 'x';\n"
        "CharY = 'y';\n"
        "CharZ = 'z';\n"
        "SeqGreeting = Greeting World SeqChar;\n"
        "SeqChar = CharX CharY CharZ;\n"
        "SimpleRule = SeqGreeting | SeqChar;\n"
        "EOI = eoi;\n"
        "Program = SimpleRule EOI @EPC_AST_SEMANTIC_ACTION_PROGRAM_RULE;\n";

    generate_ast(gdl_input);

    CHECK_TRUE(gdl_generate_c_code(ast_builder_data.ast_root, base_name, output_dir));
}

