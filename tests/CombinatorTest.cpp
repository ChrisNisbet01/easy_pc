#include "CppUTest/TestHarness.h"

#include <iostream>

extern "C" {
#include "cpt_node.h"
#include "easy_pc_private.h"

#include <stdlib.h> // For calloc, free
#include <string.h> // For strlen, strcmp
}

TEST_GROUP(CombinatorTest)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list;

    void setup() override
    {
        session = (epc_parse_session_t){0}; // Reset session before each test
        list = epc_parser_list_create();
    }

    epc_parse_result_t parse(epc_parser_t * parser, char const * input)
    {
        void * user_ctx = NULL; // No user context for these tests
        session = epc_parse_str(parser, input, user_ctx);
        return session.result;
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(CombinatorTest, SOIMatchesStartOfInput)
{
    epc_parser_t * p_soi = epc_soi(list, "SOI");

    result = parse(p_soi, "abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("soi", result.data.success->tag);
    STRNCMP_EQUAL("", content, 0);
    LONGS_EQUAL(0, result.data.success->token.count);
    LONGS_EQUAL(0, result.data.success->children_count);
}

TEST(CombinatorTest, SOIfailsAfterStartOfInput)
{
    epc_parser_t * p_soi = epc_soi(list, "SOI");
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_or = epc_and(list, "AND", 2, p_char_a, p_soi);
    result = parse(p_or, "abc");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Start of input not found", result.data.error->message);
    CHECK_EQUAL(1, result.data.error->view.offset);
    STRCMP_EQUAL("<start of input>", result.data.error->expected);
    STRCMP_EQUAL("<post input>", result.data.error->found);
}

TEST(CombinatorTest, PStarMatchesZero)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_star_a = epc_many(list, NULL, p_char_a);

    result = parse(p_star_a, "");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("many", result.data.success->tag);
    STRCMP_EQUAL("many", result.data.success->name);
    STRNCMP_EQUAL("", content, 0);
    LONGS_EQUAL(0, result.data.success->token.count);
    LONGS_EQUAL(0, result.data.success->children_count);
}

TEST(CombinatorTest, PStarMatchesOne)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_star_a = epc_many(list, NULL, p_char_a);

    result = parse(p_star_a, "abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("many", result.data.success->tag);
    STRCMP_EQUAL("many", result.data.success->name);
    STRNCMP_EQUAL("a", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
    LONGS_EQUAL(1, result.data.success->children_count);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
}

TEST(CombinatorTest, PStarMatchesMultiple)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_star_a = epc_many(list, NULL, p_char_a);

    result = parse(p_star_a, "aaabc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("many", result.data.success->tag);
    STRCMP_EQUAL("many", result.data.success->name);
    STRNCMP_EQUAL("aaa", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
    LONGS_EQUAL(3, result.data.success->children_count);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[1]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[2]), 1);
}

TEST(CombinatorTest, PStarMatchesMultipleThenFails)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_star_a = epc_many(list, NULL, p_char_a);

    result = parse(p_star_a, "aaabbc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("many", result.data.success->tag);
    STRCMP_EQUAL("many", result.data.success->name);
    STRNCMP_EQUAL("aaa", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
    LONGS_EQUAL(3, result.data.success->children_count);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[1]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[2]), 1);
}

TEST(CombinatorTest, PPlusMatchesOne)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_plus_a = epc_plus(list, NULL, p_char_a);

    result = parse(p_plus_a, "abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("plus", result.data.success->tag);
    STRCMP_EQUAL("plus", result.data.success->name);
    STRNCMP_EQUAL("a", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
    LONGS_EQUAL(1, result.data.success->children_count);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
}

TEST(CombinatorTest, PPlusMatchesMultiple)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_plus_a = epc_plus(list, NULL, p_char_a);

    result = parse(p_plus_a, "aaabc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("plus", result.data.success->tag);
    STRCMP_EQUAL("plus", result.data.success->name);
    STRNCMP_EQUAL("aaa", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
    LONGS_EQUAL(3, result.data.success->children_count);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[1]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[2]), 1);
}

TEST(CombinatorTest, PPlusFailsOnZeroMatches)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_plus_a = epc_plus(list, NULL, p_char_a);

    result = parse(p_plus_a, "bbc");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected character", result.data.error->message);
    CHECK_EQUAL(0, result.data.error->view.offset);
    STRCMP_EQUAL("a", result.data.error->expected);
    STRCMP_EQUAL("b", result.data.error->found);
}

TEST(CombinatorTest, PPlusMatchesMultipleThenFails)
{
    epc_parser_t * p_char_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_plus_a = epc_plus(list, NULL, p_char_a);

    result = parse(p_plus_a, "aaabbc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("plus", result.data.success->tag);
    STRCMP_EQUAL("plus", result.data.success->name);
    STRNCMP_EQUAL("aaa", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
    LONGS_EQUAL(3, result.data.success->children_count);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[1]), 1);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[2]), 1);
}
