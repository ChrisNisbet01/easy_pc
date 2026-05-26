#include "CppUTest/TestHarness.h"

#include <stdlib.h>
#include <string.h>

extern "C" {
#include "cpt_node.h"
#include "easy_pc_private.h"
}

TEST_GROUP(TokenList)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list = NULL;

    void setup() override
    {
        session = (epc_parse_session_t){0};
        list = epc_parser_list_create();
    }

    epc_parse_result_t parse(epc_parser_t * parser, char const * input)
    {
        session = epc_parse_str(parser, input, NULL);
        return session.result;
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(TokenList, CreateAndDestroy)
{
    epc_token_list_t * tokens = epc_token_list_create(10);
    CHECK_TRUE(tokens != NULL);

    LONGS_EQUAL(0, epc_token_list_count(tokens));

    epc_token_list_free(tokens);
}

TEST(TokenList, AddTokensAndVerify)
{
    epc_token_list_t * tokens = epc_token_list_create(10);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view1 = {.offset = 0, .len = 1, .line_number = 1, .column_number = 1};
    epc_parser_input_view_t view2 = {.offset = 1, .len = 1, .line_number = 1, .column_number = 2};

    CHECK_TRUE(epc_token_list_add(tokens, 'a', view1));
    CHECK_TRUE(epc_token_list_add(tokens, 'b', view2));

    LONGS_EQUAL(2, epc_token_list_count(tokens));

    epc_token_list_free(tokens);
}

TEST(TokenList, NullListReturnsZeroCount)
{
    LONGS_EQUAL(0, epc_token_list_count(NULL));
}

TEST(TokenList, AddToNullListFails)
{
    epc_parser_input_view_t view = {.offset = 0, .len = 1, .line_number = 1, .column_number = 1};
    CHECK_FALSE(epc_token_list_add(NULL, 'a', view));
}

TEST(TokenList, FreeNullListIsSafe)
{
    epc_token_list_free(NULL);
}

TEST_GROUP(TokenListReparse)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list = NULL;

    void setup() override
    {
        session = (epc_parse_session_t){0};
        list = epc_parser_list_create();
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(TokenListReparse, ReparseWithGroupedToken)
{
    /*
     * Stage 1: parse "abc" with epc_any (matches one character per call).
     * This creates a valid context with the input buffer and newline data.
     */
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "abc", NULL);
    result = session.result;

    CHECK_FALSE(result.is_error);

    /*
     * Build an enriched token list: group "abc" into a single token with ID 'x'.
     * The view covers the full 3-byte span at offset 0, line 1, col 1.
     */
    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view = {.offset = 0, .len = 3, .line_number = 1, .column_number = 1};
    CHECK_TRUE(epc_token_list_add(tokens, 'x', view));

    /*
     * Stage 2: reparse with epc_char('x'), which checks that token.id == 'x'.
     * The single token in the list has id='x', so it should match.
     */
    epc_parser_t * p_match_x = epc_char(list, "match_x", 'x');
    bool reparse_ok = epc_parse_session_reparse(&session, p_match_x, tokens);
    CHECK_TRUE(reparse_ok);
    CHECK_FALSE(session.result.is_error);

    /* Verify the result CPT node captures the correct input span. */
    epc_cpt_node_t * node = session.result.data.success;
    CHECK_TRUE(node != NULL);
    char const * content = epc_cpt_node_get_content(node);
    CHECK_TRUE(content != NULL);
    STRNCMP_EQUAL("abc", content, 3);

    epc_token_list_free(tokens);
}

TEST(TokenListReparse, ReparseFailsOnWrongToken)
{
    /*
     * Stage 1: parse "abc" with epc_any.
     */
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "abc", NULL);
    result = session.result;
    CHECK_FALSE(result.is_error);

    /*
     * Build token list with token ID 'x', but try to match 'y'.
     */
    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view = {.offset = 0, .len = 3, .line_number = 1, .column_number = 1};
    CHECK_TRUE(epc_token_list_add(tokens, 'a', view));

    /*
     * Stage 2: try to match 'y' — should fail.
     */
    epc_parser_t * p_match_y = epc_char(list, "match_y", 'y');
    bool reparse_ok = epc_parse_session_reparse(&session, p_match_y, tokens);
    CHECK_FALSE(reparse_ok);
    CHECK_TRUE(session.result.is_error);

    epc_token_list_free(tokens);
}

TEST(TokenListReparse, ReparseWithMultipleTokens)
{
    /*
     * Stage 1: parse "ab" with epc_any.
     */
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "ab", NULL);
    result = session.result;
    CHECK_FALSE(result.is_error);

    /*
     * Build token list with two tokens: 'x' spanning "a", then 'y' spanning "b".
     */
    epc_token_list_t * tokens = epc_token_list_create(2);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view1 = {.offset = 0, .len = 1, .line_number = 1, .column_number = 1};
    epc_parser_input_view_t view2 = {.offset = 1, .len = 1, .line_number = 1, .column_number = 2};
    CHECK_TRUE(epc_token_list_add(tokens, 'x', view1));
    CHECK_TRUE(epc_token_list_add(tokens, 'y', view2));

    /*
     * Stage 2: match sequence 'x' 'y' using epc_and.
     */
    epc_parser_t * p_x = epc_char(list, "x", 'x');
    epc_parser_t * p_y = epc_char(list, "y", 'y');
    epc_parser_t * p_seq = epc_and(list, "seq", 2, p_x, p_y);

    bool reparse_ok = epc_parse_session_reparse(&session, p_seq, tokens);
    CHECK_TRUE(reparse_ok);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_free(tokens);
}

TEST(TokenListReparse, ReparseWithNullSessionFails)
{
    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    bool ok = epc_parse_session_reparse(NULL, NULL, tokens);
    CHECK_FALSE(ok);

    epc_token_list_free(tokens);
}

TEST(TokenListReparse, ReparseWithNullParserFails)
{
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "a", NULL);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    bool ok = epc_parse_session_reparse(&session, NULL, tokens);
    CHECK_FALSE(ok);

    epc_token_list_free(tokens);
}

TEST(TokenListReparse, ReparseWithNullTokensFails)
{
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "a", NULL);
    CHECK_FALSE(session.result.is_error);

    epc_parser_t * p_x = epc_char(list, "x", 'x');
    bool ok = epc_parse_session_reparse(&session, p_x, NULL);
    CHECK_FALSE(ok);
}

TEST_GROUP(TokenParser)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list = NULL;

    void setup() override
    {
        session = (epc_parse_session_t){0};
        list = epc_parser_list_create();
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(TokenParser, MatchesTokenById)
{
    /*
     * Parse a character and then reparse using epc_token with the same char's numeric ID.
     * This works because char token IDs are just ASCII values, and epc_token compares by ID.
     */
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "abc", NULL);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view = {.offset = 0, .len = 3, .line_number = 1, .column_number = 1};
    CHECK_TRUE(epc_token_list_add(tokens, (epc_token_id_t)'X', view));

    /* epc_token matches the token ID 'X' (88). */
    epc_parser_t * p_token = epc_token(list, "match_x", (epc_token_id_t)'X');
    bool reparse_ok = epc_parse_session_reparse(&session, p_token, tokens);
    CHECK_TRUE(reparse_ok);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_free(tokens);
}

TEST(TokenParser, FailsOnWrongTokenId)
{
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "abc", NULL);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view = {.offset = 0, .len = 3, .line_number = 1, .column_number = 1};
    CHECK_TRUE(epc_token_list_add(tokens, (epc_token_id_t)42, view));

    /* Try to match token ID 99 — should fail since our token is ID 42. */
    epc_parser_t * p_token = epc_token(list, "match_99", (epc_token_id_t)99);
    bool reparse_ok = epc_parse_session_reparse(&session, p_token, tokens);
    CHECK_FALSE(reparse_ok);
    CHECK_TRUE(session.result.is_error);

    epc_token_list_free(tokens);
}

TEST(TokenParser, MatchesCustomTokenId)
{
    /*
     * Use a custom token ID >= EPC_TOKEN_ID_FIRST_USER (300).
     * This demonstrates the parser works with any uint32_t token ID.
     */
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "hello", NULL);
    CHECK_FALSE(session.result.is_error);

    epc_token_id_t const custom_id = EPC_TOKEN_ID_FIRST_USER + 42;

    epc_token_list_t * tokens = epc_token_list_create(1);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view = {.offset = 0, .len = 5, .line_number = 1, .column_number = 1};
    CHECK_TRUE(epc_token_list_add(tokens, custom_id, view));

    epc_parser_t * p_token = epc_token(list, "custom", custom_id);
    bool reparse_ok = epc_parse_session_reparse(&session, p_token, tokens);
    CHECK_TRUE(reparse_ok);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_free(tokens);
}

TEST(TokenParser, MatchesTokenInSequence)
{
    /*
     * Two tokens with custom IDs, matched in sequence using epc_and.
     */
    epc_parser_t * p_any = epc_any(list, "any");
    session = epc_parse_str(p_any, "abcdef", NULL);
    CHECK_FALSE(session.result.is_error);

    epc_token_id_t const id_a = EPC_TOKEN_ID_FIRST_USER;
    epc_token_id_t const id_b = EPC_TOKEN_ID_FIRST_USER + 1;

    epc_token_list_t * tokens = epc_token_list_create(2);
    CHECK_TRUE(tokens != NULL);

    epc_parser_input_view_t view_a = {.offset = 0, .len = 3, .line_number = 1, .column_number = 1};
    epc_parser_input_view_t view_b = {.offset = 3, .len = 3, .line_number = 1, .column_number = 4};
    CHECK_TRUE(epc_token_list_add(tokens, id_a, view_a));
    CHECK_TRUE(epc_token_list_add(tokens, id_b, view_b));

    epc_parser_t * p_a = epc_token(list, "id_a", id_a);
    epc_parser_t * p_b = epc_token(list, "id_b", id_b);
    epc_parser_t * p_seq = epc_and(list, "seq", 2, p_a, p_b);

    bool reparse_ok = epc_parse_session_reparse(&session, p_seq, tokens);
    CHECK_TRUE(reparse_ok);
    CHECK_FALSE(session.result.is_error);

    epc_token_list_free(tokens);
}
