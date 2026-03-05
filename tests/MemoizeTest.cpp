#include "easy_pc_private.h"

#include "CppUTest/TestHarness.h"

#include <iostream>
#include <stdio.h>
#include <string.h>

typedef struct
{
    int calls;
} memo_test_ctx_t;

static void
on_entry_count(epc_parser_t * parser, epc_parser_ctx_t * ctx, void * user_ctx)
{
    (void)parser;
    (void)ctx;
    memo_test_ctx_t * tctx = (memo_test_ctx_t *)user_ctx;
    tctx->calls++;
}

TEST_GROUP(MemoizeTest)
{
    epc_parse_session_t session;
    epc_parser_list * list;
    memo_test_ctx_t tctx;

    void setup() override
    {
        session = (epc_parse_session_t){0};
        list = epc_parser_list_create();
        memset(&tctx, 0, sizeof(tctx));
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }

    void check_success(
        char const * expected_tag, char const * expected_content, size_t expected_len, int expected_children_count
    )
    {
        if (session.result.is_error)
        {
            std::cout << "Parse error: " << session.result.data.error->message << std::endl;
            std::cout << "Expected: "
                      << (session.result.data.error->expected ? session.result.data.error->expected : "unknown")
                      << std::endl;
            std::cout << "Found: " << (session.result.data.error->found ? session.result.data.error->found : "unknown")
                      << std::endl;
            FAIL("Expected success but got error");
        }
        CHECK_TRUE(session.result.data.success != NULL);
        STRCMP_EQUAL(expected_tag, session.result.data.success->tag);
        STRNCMP_EQUAL(expected_content, session.result.data.success->content, expected_len);
        LONGS_EQUAL(expected_len, session.result.data.success->len);
        LONGS_EQUAL(expected_children_count, session.result.data.success->children_count);
    }
};

TEST(MemoizeTest, Memoize_CallsWrappedParserOnlyOnce)
{
    epc_wrap_callbacks_t callbacks = {on_entry_count, NULL};
    /* Construct a parser that calls p_memo twice at the same position */
    epc_parser_t * p_a = epc_char_l(list, "a", 'a');
    epc_parser_t * pwrap_a = epc_wrap_l(list, "wrap", p_a, callbacks, &tctx);
    epc_parser_t * p_memo = epc_memoize_l(list, "memo", pwrap_a);
    epc_parser_t * p_b = epc_char_l(list, "b", 'b');
    epc_parser_t * p_aa = epc_and_l(list, "aa", 2, p_memo, p_a);
    epc_parser_t * p_ab = epc_and_l(list, "ab", 2, p_memo, p_b);
    epc_parser_t * p_aa_or_ab = epc_or_l(list, "aa_or_ab", 2, p_aa, p_ab);

    session = epc_parse_str(p_aa_or_ab, "ab", NULL);

    check_success("or", "ab", 2, 1);
    LONGS_EQUAL(1, tctx.calls);
}

TEST(MemoizeTest, Memoize_WorksAtDifferentPositions)
{
    epc_parser_t * p_a = epc_char(NULL, 'a');
    epc_wrap_callbacks_t callbacks = {on_entry_count, NULL};
    epc_parser_t * p_wrap = epc_wrap_l(list, "wrap", p_a, callbacks, &tctx);
    epc_parser_t * p_memo = epc_memoize_l(list, "memo", p_wrap);

    /* Construct a parser that calls p_memo twice at different positions */
    epc_parser_t * p_double = epc_and_l(list, "double", 2, p_memo, p_memo);

    session = epc_parse_str(p_double, "aa", NULL);

    check_success("and", "aa", 2, 2);
    LONGS_EQUAL(2, tctx.calls);
}

TEST(MemoizeTest, Memoize_CachesFailures)
{
    epc_parser_t * p_a = epc_char(NULL, 'a');
    epc_wrap_callbacks_t callbacks = {on_entry_count, NULL};
    epc_parser_t * p_wrap = epc_wrap_l(list, "wrap", p_a, callbacks, &tctx);
    epc_parser_t * p_memo = epc_memoize_l(list, "memo", p_wrap);

    /* Construct a parser that calls p_memo twice at the same position, where it fails */
    epc_parser_t * p_double = epc_or_l(list, "double", 2, p_memo, p_memo);

    session = epc_parse_str(p_double, "b", NULL);

    CHECK_TRUE(session.result.is_error);
    LONGS_EQUAL(1, tctx.calls);
}
