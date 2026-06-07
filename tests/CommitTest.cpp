#include "CppUTest/TestHarness.h"

#include <iostream>

extern "C" {
#include "cpt_node.h"
#include "easy_pc_private.h"

#include <stdlib.h>
#include <string.h>
}

TEST_GROUP(CommitTest)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list;

    void setup() override
    {
        session = (epc_parse_session_t){0};
        list = epc_parser_list_create();
    }

    epc_parse_result_t parse(epc_parser_t * parser, char const * input)
    {
        void * user_ctx = NULL;
        session = epc_parse_str(parser, input, user_ctx);
        return session.result;
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(CommitTest, CommitConvertsBacktrackToCommitted)
{
    epc_parser_t * p_char_x = epc_char(list, "X", 'x');
    epc_parser_t * p_char_y = epc_char(list, "Y", 'y');
    epc_parser_t * p_seq = epc_and(list, "SEQ", 2, p_char_x, p_char_y);
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_seq);

    result = parse(p_commit, "x");

    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);
}

TEST(CommitTest, CommitPassesSuccessThrough)
{
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_char_a);

    result = parse(p_commit, "a");

    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, OrSkipsAlternativesOnCommit)
{
    /* First alternative: match 'x', then commit('y'). Since only 'x' is present
       in input, 'y' fails → FAIL_COMMITTED → OR should skip the second alternative. */
    epc_parser_t * p_x = epc_char(list, "X", 'x');
    epc_parser_t * p_y = epc_char(list, "Y", 'y');
    epc_parser_t * p_seq = epc_and(list, "FIRST", 2, p_x, epc_commit(list, "COMMIT", p_y));
    epc_parser_t * p_alt = epc_char(list, "ALT", 'a');
    epc_parser_t * p_or = epc_or(list, "OR", 2, p_seq, p_alt);

    result = parse(p_or, "x");

    /* Because the first alternative committed, the second should be skipped.
       The result should be a FAIL_COMMITTED error, not a success matching 'a'. */
    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);
}

TEST(CommitTest, OrTriesAlternativesOnBacktrack)
{
    /* or(and('a','b'), and('a','c')) with input "ac":
       First alt: 'a' matches, then 'b' fails at offset 1 (input is 'c').
       FAIL_BACKTRACK → OR should try second alt from offset 0: 'a' matches, 'c' matches → success. */
    epc_parser_t * p_a1 = epc_char(list, "A", 'a');
    epc_parser_t * p_b = epc_char(list, "B", 'b');
    epc_parser_t * p_seq1 = epc_and(list, "FIRST", 2, p_a1, p_b);
    epc_parser_t * p_a2 = epc_char(list, "A2", 'a');
    epc_parser_t * p_c = epc_char(list, "C", 'c');
    epc_parser_t * p_seq2 = epc_and(list, "SECOND", 2, p_a2, p_c);
    epc_parser_t * p_or = epc_or(list, "OR", 2, p_seq1, p_seq2);

    result = parse(p_or, "ac");

    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, OrBacktrackFirstAltFails)
{
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_char_b = epc_char(list, "B", 'b');
    epc_parser_t * p_or = epc_or(list, "OR", 2, p_char_a, p_char_b);

    result = parse(p_or, "b");

    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
    STRCMP_EQUAL("or", result.data.success->tag);
}

TEST(CommitTest, ManyPropagatesCommit)
{
    /* Many requires a committed child's failure to propagate, not be consumed. */
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_char_b = epc_char(list, "B", 'b');
    epc_parser_t * p_seq = epc_and(list, "SEQ", 2, p_char_a, p_char_b);
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_seq);
    epc_parser_t * p_many = epc_many(list, "MANY", p_commit);

    result = parse(p_many, "ab");

    /* 'ab' matches the sequence once. Then at EOF, sequence fails. The first match
       succeeds, so the many succeeds. Let's test the committed propagation on the
       SECOND iteration where the child was partially matched. */
    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, ManyPropagatesCommitOnPartialInput)
{
    /* First iteration succeeds ('ab'), second iteration partially matches ('a' then 'b' fails).
       The FAIL_COMMITTED from the second iteration should propagate through many. */
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_char_b = epc_char(list, "B", 'b');
    epc_parser_t * p_seq = epc_and(list, "SEQ", 2, p_char_a, p_char_b);
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_seq);
    epc_parser_t * p_many = epc_many(list, "MANY", p_commit);

    result = parse(p_many, "aba");

    /* 'ab' matches, then 'a' is partially matched but 'b' is missing.
       FAIL_COMMITTED → propagates up, many should not silently consume. */
    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);
}

TEST(CommitTest, ManyConsumesBacktrackOnEof)
{
    /* Many should still work normally — at EOF, child fails with FAIL_BACKTRACK,
       which many treats as "no more matches". */
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_many = epc_many(list, "MANY", p_char_a);

    result = parse(p_many, "a");

    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, OptionalPropagatesCommit)
{
    /* Optional should not swallow FAIL_COMMITTED. */
    epc_parser_t * p_char_x = epc_char(list, "X", 'x');
    epc_parser_t * p_char_y = epc_char(list, "Y", 'y');
    epc_parser_t * p_seq = epc_and(list, "SEQ", 2, p_char_x, p_char_y);
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_seq);
    epc_parser_t * p_opt = epc_optional(list, "OPT", p_commit);

    result = parse(p_opt, "x");

    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);
}

TEST(CommitTest, OptionalReturnsAbsentOnBacktrack)
{
    /* Optional should still work normally with FAIL_BACKTRACK — return "absent". */
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_opt = epc_optional(list, "OPT", p_char_a);

    result = parse(p_opt, "b");

    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
    LONGS_EQUAL(0, result.data.success->token.count);
}

TEST(CommitTest, SequencePropagatesCommit)
{
    /* Sequence passes errors through — a FAIL_COMMITTED child should propagate. */
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    epc_parser_t * p_char_x = epc_char(list, "X", 'x');
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_char_x);
    epc_parser_t * p_seq = epc_and(list, "SEQ", 2, p_char_a, p_commit);

    result = parse(p_seq, "a");

    /* 'a' matches, then 'x' fails at EOF → FAIL_COMMITTED propagates through sequence */
    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);
}

TEST(CommitTest, IfStatementPattern)
{
    /* Classic "if statement" pattern:
       Stmt = IfStmt | OtherStmt;
       IfStmt = kw_if LParen commit(Expr RParen) StmtBody;
       If the '(' matches but Expr doesn't, we commit — don't try OtherStmt. */
    epc_parser_t * p_kw_if = epc_string(list, "if", "if");
    epc_parser_t * p_lparen = epc_char(list, "LParen", '(');
    epc_parser_t * p_expr = epc_identifier(list, "Expr");
    epc_parser_t * p_rparen = epc_char(list, "RParen", ')');
    epc_parser_t * p_body = epc_char(list, "Body", '{');

    /* IfStmt = kw_if LParen commit(Expr RParen) Body */
    epc_parser_t * p_commit_tail = epc_commit(list, "COMMIT", epc_and(list, "TAIL", 2, p_expr, p_rparen));
    epc_parser_t * p_if_stmt = epc_and(list, "IfStmt", 4, p_kw_if, p_lparen, p_commit_tail, p_body);

    /* OtherStmt = 'x' */
    epc_parser_t * p_other = epc_char(list, "Other", 'x');

    /* Stmt = IfStmt | OtherStmt */
    epc_parser_t * p_stmt = epc_or(list, "Stmt", 2, p_if_stmt, p_other);

    /* Case 1: full valid if statement succeeds */
    result = parse(p_stmt, "if(foo){");
    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);

    /* Case 2: "if(" is matched but expression is invalid (empty after '(')
       → commit kicks in, should NOT try OtherStmt which would match 'x'.
       The input is just "if(" — EOF after lparen. */
    result = parse(p_stmt, "if(");
    CHECK_TRUE(result.is_error);
    /* Since the commit was contained within IfStmt (not LHS ^), FAIL_COMMITTED
       propagates out of IfStmt and is seen by the OR. The OR should skip OtherStmt. */
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);

    /* Case 3: no "if" at all → regular backtrack to OtherStmt */
    result = parse(p_stmt, "x");
    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);

    /* Case 2: "if(" is matched but expression is invalid (empty after '(')
       → commit kicks in, should NOT try OtherStmt which would match 'x'.
       The input is just "if(" — EOF after lparen. */
    result = parse(p_stmt, "if(");
    CHECK_TRUE(result.is_error);
    /* Since the commit was contained within IfStmt (not LHS ^), FAIL_COMMITTED
       propagates out of IfStmt and is seen by the OR. The OR should skip OtherStmt. */
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);

    /* Case 3: no "if" at all → regular backtrack to OtherStmt */
    result = parse(p_stmt, "x");
    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, CommitBoundaryContainsCommitted)
{
    /* IfStmt = seq('i','f') commit(seq(LParen, Expr, RParen, Body))
       The commit boundary on the rule converts FAIL_COMMITTED → FAIL_BACKTRACK
       so the parent OR still tries the second alternative. */
    epc_parser_t * p_kw_if = epc_string(list, "if", "if");
    epc_parser_t * p_lparen = epc_char(list, "LParen", '(');
    epc_parser_t * p_expr = epc_identifier(list, "Expr");
    epc_parser_t * p_rparen = epc_char(list, "RParen", ')');
    epc_parser_t * p_body = epc_char(list, "Body", '{');
    epc_parser_t * p_commit_tail = epc_commit(list, "COMMIT", epc_and(list, "TAIL", 2, p_expr, p_rparen));
    epc_parser_t * p_if_stmt = epc_and(list, "IfStmt", 4, p_kw_if, p_lparen, p_commit_tail, p_body);

    /* Set commit boundary on IfStmt — as the GDL compiler would for a rule with ^ */
    epc_parser_set_commit_boundary(p_if_stmt);

    /* OtherStmt = 'x' */
    epc_parser_t * p_other = epc_char(list, "Other", 'x');

    /* Stmt = IfStmt | OtherStmt */
    epc_parser_t * p_stmt = epc_or(list, "Stmt", 2, p_if_stmt, p_other);

    /* Case 1: "if(foo){ " → full match, IfStmt succeeds */
    result = parse(p_stmt, "if(foo){");
    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);

    /* Case 2: "if(" → IfStmt matches "if(", then commit fails on Expr at EOF.
       FAIL_COMMITTED propagates to the boundary, where it becomes FAIL_BACKTRACK.
       The OR then tries OtherStmt, which fails. The overall parse fails with
       the original backtrack error info. */
    result = parse(p_stmt, "if(");
    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_BACKTRACK, result.error_type);

    /* Case 3: "x" → IfStmt fails on "i" at offset 0 (no "if" prefix → backtrack),
       OR tries OtherStmt → matches 'x'. */
    result = parse(p_stmt, "x");
    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, SuccessResultHasCorrectErrorType)
{
    epc_parser_t * p_eoi = epc_eoi(list, "EOI");
    result = parse(p_eoi, "");

    CHECK_FALSE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_SUCCESS, result.error_type);
}

TEST(CommitTest, BacktrackResultHasCorrectErrorType)
{
    epc_parser_t * p_char_a = epc_char(list, "A", 'a');
    result = parse(p_char_a, "b");

    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_BACKTRACK, result.error_type);
}

TEST(CommitTest, ErrorPositionPreservedOnCommit)
{
    /* The error position should point to where the actual failure occurred,
       not to the commit point. */
    epc_parser_t * p_char_x = epc_char(list, "X", 'x');
    epc_parser_t * p_char_y = epc_char(list, "Y", 'y');
    epc_parser_t * p_seq = epc_and(list, "SEQ", 2, p_char_x, p_char_y);
    epc_parser_t * p_commit = epc_commit(list, "COMMIT", p_seq);

    result = parse(p_commit, "xz");

    /* 'x' matches at offset 0, then 'y' expected but 'z' found at offset 1.
       The error should point to offset 1, where 'y' was expected. */
    CHECK_TRUE(result.is_error);
    CHECK_EQUAL(EPC_RESULT_FAIL_COMMITTED, result.error_type);
    CHECK_EQUAL(1, result.data.error->view.offset);
    STRCMP_EQUAL("y", result.data.error->expected);
}
