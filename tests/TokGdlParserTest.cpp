#include "CppUTest/TestHarness.h"
#include "CppUTestExt/MockSupport.h"

#include <stdio.h>

extern "C" {
#include "gdl_compiler_ast_actions.h"
#include "gdl_parser.h"
#include "gdl_token_ids.h"
#include "gdl_tokenizer_actions.h"
#include "gdl_tokenizer_parser.h"
}

static void
run_two_stage_pipeline(
    epc_parser_list * parser_list, char const * input, epc_parse_session_t * session, gdl_ast_node_t ** ast_root
)
{
    epc_parser_t * tokenizer = create_gdl_tokenizer_parser(parser_list);
    CHECK(tokenizer != NULL);

    *session = epc_parse_str(tokenizer, input, NULL);
    CHECK_FALSE(session->result.is_error);

    epc_ast_hook_registry_t * tokenizer_registry = epc_ast_hook_registry_create(TOKENIZER_ACTION_COUNT);
    CHECK(tokenizer_registry != NULL);

    gdl_tokenizer_ctx_t tokenizer_ctx;
    tokenizer_ctx.tokens = epc_token_list_create(256);
    tokenizer_ctx.last_match = NULL;
    gdl_tokenizer_hook_registry_init(tokenizer_registry, &tokenizer_ctx);

    epc_ast_result_t token_build = epc_ast_build(session->result.data.success, tokenizer_registry, &tokenizer_ctx);
    CHECK_FALSE(token_build.has_error);

    epc_ast_hook_registry_free(tokenizer_registry);

    epc_parser_t * gdl_parser = create_gdl_parser(parser_list);
    CHECK(gdl_parser != NULL);

    bool reparse_ok = epc_parse_session_reparse(session, gdl_parser, tokenizer_ctx.tokens);
    epc_token_list_free(tokenizer_ctx.tokens);
    CHECK_TRUE(reparse_ok);
    CHECK_FALSE(session->result.is_error);

    epc_ast_hook_registry_t * ast_registry = epc_ast_hook_registry_create(GDL_AST_ACTION_MAX);
    CHECK(ast_registry != NULL);
    gdl_ast_hook_registry_init(ast_registry, NULL);

    epc_ast_result_t ast_build = epc_ast_build(session->result.data.success, ast_registry, NULL);
    CHECK_FALSE(ast_build.has_error);
    CHECK(ast_build.ast_root != NULL);

    *ast_root = (gdl_ast_node_t *)ast_build.ast_root;
}

TEST_GROUP(TokGdlParserTest)
{
    epc_parser_list * parser_list;
    epc_parse_session_t session;
    gdl_ast_node_t * ast_root;

    void setup()
    {
        parser_list = epc_parser_list_create();
        CHECK(parser_list != NULL);
        ast_root = NULL;
        session = (epc_parse_session_t){0};
    }

    void teardown()
    {
        if (ast_root)
        {
            gdl_ast_node_free(ast_root, NULL);
        }
        epc_parse_session_destroy(&session);
        epc_parser_list_free(parser_list);
    }
};

TEST(TokGdlParserTest, SimpleStringLiteral)
{
    char const * gdl_input = "MyRule = \"hello\";";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_RULE_DEFINITION, rule->type);
    STRCMP_EQUAL("MyRule", rule->data.rule_def.name);
}

TEST(TokGdlParserTest, RuleWithCharRange)
{
    char const * gdl_input = "MyRange = [a-z];";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_RULE_DEFINITION, rule->type);
    STRCMP_EQUAL("MyRange", rule->data.rule_def.name);

    gdl_ast_node_t * alt = rule->data.rule_def.definition;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_ALTERNATIVE, alt->type);
    LONGS_EQUAL(1, alt->data.alternative.alternatives.count);

    gdl_ast_node_t * seq = alt->data.alternative.alternatives.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_SEQUENCE, seq->type);
    LONGS_EQUAL(1, seq->data.sequence.elements.count);

    gdl_ast_node_t * char_range = seq->data.sequence.elements.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_CHAR_RANGE, char_range->type);
    LONGS_EQUAL('a', char_range->data.char_range.start_char);
    LONGS_EQUAL('z', char_range->data.char_range.end_char);
}

TEST(TokGdlParserTest, RuleWithSemanticAction)
{
    char const * gdl_input = "MyAction = 'a' @my_action;";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_RULE_DEFINITION, rule->type);
    STRCMP_EQUAL("MyAction", rule->data.rule_def.name);

    gdl_ast_node_t * action = rule->data.rule_def.semantic_action;
    CHECK(action != NULL);
    LONGS_EQUAL(GDL_AST_NODE_TYPE_SEMANTIC_ACTION, action->type);
    STRCMP_EQUAL("my_action", action->data.semantic_action.action_name);
}

TEST(TokGdlParserTest, RuleWithSequence)
{
    char const * gdl_input = "MySeq = 'a' 'b';";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    gdl_ast_node_t * alt = rule->data.rule_def.definition;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_ALTERNATIVE, alt->type);
    LONGS_EQUAL(1, alt->data.alternative.alternatives.count);

    gdl_ast_node_t * seq = alt->data.alternative.alternatives.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_SEQUENCE, seq->type);
    LONGS_EQUAL(2, seq->data.sequence.elements.count);
}

TEST(TokGdlParserTest, RuleWithAlternative)
{
    char const * gdl_input = "MyAlt = 'a' | 'b';";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    gdl_ast_node_t * alt = rule->data.rule_def.definition;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_ALTERNATIVE, alt->type);
    LONGS_EQUAL(2, alt->data.alternative.alternatives.count);
}

TEST(TokGdlParserTest, RuleWithRepetition)
{
    char const * gdl_input = "MyStar = 'a'*;";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    gdl_ast_node_t * alt = rule->data.rule_def.definition;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_ALTERNATIVE, alt->type);
    LONGS_EQUAL(1, alt->data.alternative.alternatives.count);

    gdl_ast_node_t * seq = alt->data.alternative.alternatives.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_SEQUENCE, seq->type);
    LONGS_EQUAL(1, seq->data.sequence.elements.count);

    gdl_ast_node_t * rep = seq->data.sequence.elements.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_REPETITION_EXPRESSION, rep->type);
    LONGS_EQUAL('*', rep->data.repetition_expr.repetition->data.repetition_op.operator_char);
}

TEST(TokGdlParserTest, FailTerminal)
{
    char const * gdl_input = "MyRule = fail(\"msg\");";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);
}

TEST(TokGdlParserTest, MultipleRules)
{
    char const * gdl_input = "A = \"hello\";\nB = 'x';\nC = digit;\n";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(3, ast_root->data.program.rules.count);

    gdl_ast_list_node_t * cur = ast_root->data.program.rules.head;
    STRCMP_EQUAL("A", cur->item->data.rule_def.name);
    cur = cur->next;
    STRCMP_EQUAL("B", cur->item->data.rule_def.name);
    cur = cur->next;
    STRCMP_EQUAL("C", cur->item->data.rule_def.name);
}

TEST(TokGdlParserTest, ComplexExpressionWithOptional)
{
    char const * gdl_input = "MyOpt = ('a' | 'b')?;";
    run_two_stage_pipeline(parser_list, gdl_input, &session, &ast_root);

    LONGS_EQUAL(GDL_AST_NODE_TYPE_PROGRAM, ast_root->type);
    LONGS_EQUAL(1, ast_root->data.program.rules.count);

    gdl_ast_node_t * rule = ast_root->data.program.rules.head->item;
    gdl_ast_node_t * alt = rule->data.rule_def.definition;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_ALTERNATIVE, alt->type);
    LONGS_EQUAL(1, alt->data.alternative.alternatives.count);

    gdl_ast_node_t * seq = alt->data.alternative.alternatives.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_SEQUENCE, seq->type);
    LONGS_EQUAL(1, seq->data.sequence.elements.count);

    gdl_ast_node_t * rep = seq->data.sequence.elements.head->item;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_REPETITION_EXPRESSION, rep->type);
    LONGS_EQUAL('?', rep->data.repetition_expr.repetition->data.repetition_op.operator_char);

    gdl_ast_node_t * inner = rep->data.repetition_expr.expression;
    LONGS_EQUAL(GDL_AST_NODE_TYPE_ALTERNATIVE, inner->type);
    LONGS_EQUAL(2, inner->data.alternative.alternatives.count);
}
