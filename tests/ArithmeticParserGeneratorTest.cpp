#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

extern "C" {
#include "tools/gdl_compiler/gdl_code_generator.h"
#include "tools/gdl_compiler/gdl_parser.h"
#include "tools/gdl_compiler/gdl_ast_builder.h"
}

#include <string.h>
#include <stdio.h>
#include <stdbool.h>

// Helper to read file content
static char* read_file_content(const char* filepath) {
    FILE* fp = fopen(filepath, "r");
    if (!fp) return NULL;
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    char* content = (char*)malloc(fsize + 1);
    if (!content) {
        fclose(fp);
        return NULL;
    }
    fread(content, 1, fsize, fp);
    fclose(fp);
    content[fsize] = 0;
    return content;
}

TEST_GROUP(ArithmeticParserGeneratorTest)
{
    epc_parser_list *parser_list;
    epc_parser_t *gdl_grammar;
    epc_parse_session_t session;
    gdl_ast_builder_data_t ast_builder_data;

    void setup() override
    {
        // Clean up generated files
        //system("rm -f simple_test_language.h simple_test_language.c simple_test_language_actions.h");
        parser_list = epc_parser_list_create();
        gdl_grammar = create_gdl_parser(parser_list);

        gdl_ast_builder_init(&ast_builder_data);
    }

    void parse_gdl_and_build_ast(char const * gdl_input)
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

// Test that code generation for arithmetic_parser_language works without errors
TEST(ArithmeticParserGeneratorTest, GeneratesCodeSuccessfully)
{
    const char *gdl_input = read_file_content("../../tests/arithmetic_parser_test_language.gdl");
    CHECK_TRUE_TEXT(gdl_input != NULL, "Failed to read arithmetic_parser_language.gdl");

    parse_gdl_and_build_ast(gdl_input);
    free((void*)gdl_input); // Free the content read from file

    bool success = gdl_generate_c_code(ast_builder_data.ast_root, "arithmetic_parser_language", ".");
    CHECK_TRUE(success);

    // Verify generated files exist
    STRCMP_CONTAINS("arithmetic_parser_language.h", "./arithmetic_parser_language.h");
    STRCMP_CONTAINS("arithmetic_parser_language.c", "./arithmetic_parser_language.c");
    STRCMP_CONTAINS("arithmetic_parser_language_actions.h", "./arithmetic_parser_language_actions.h");

    // Basic content checks (can be expanded later for correctness)
    char* header_content = read_file_content("./arithmetic_parser_language.h");
    CHECK_TRUE_TEXT(header_content != NULL, "Generated header file is empty or missing.");
    STRCMP_CONTAINS("epc_parser_t * create_arithmetic_parser_language_parser", header_content);
    free(header_content);

    char* source_content = read_file_content("./arithmetic_parser_language.c");
    CHECK_TRUE_TEXT(source_content != NULL, "Generated source file is empty or missing.");
    STRCMP_CONTAINS("create_arithmetic_parser_language_parser", source_content);
    free(source_content);

    char* actions_content = read_file_content("./arithmetic_parser_language_actions.h");
    CHECK_TRUE_TEXT(actions_content != NULL, "Generated actions header file is empty or missing.");
    STRCMP_CONTAINS("typedef enum {", actions_content);
    free(actions_content);
}
