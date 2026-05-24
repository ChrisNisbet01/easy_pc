#include "CppUTest/TestHarness.h"

#include <iostream>

extern "C" {
#include "cpt_node.h"
#include "easy_pc_private.h"

#include <string.h> // For strlen, strcmp
}

TEST_GROUP(TerminalParsers)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list;

    void setup() override
    {
        list = epc_parser_list_create();
    }

    epc_parse_result_t parse(epc_parser_t * parser, char const * input)
    {
        void * user_ctx = NULL; // No user context for these tests
        session = epc_parse_str(parser, input, NULL);
        return session.result;
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(TerminalParsers, PCharMatchesCorrectCharacter)
{
    epc_parser_t * p = epc_char(list, NULL, 'a');
    epc_parse_result_t result = parse(p, "abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("char", result.data.success->tag);
    STRCMP_EQUAL("char", result.data.success->name);
    STRNCMP_EQUAL("a", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, PCharDoesNotMatchIncorrectCharacter)
{
    epc_parser_t * p = epc_char(list, NULL, 'b');
    epc_parse_result_t result = parse(p, "abc");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PCharFailsOnEmptyInput)
{
    epc_parser_t * p = epc_char(list, NULL, 'a');
    result = parse(p, "");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PCharFailsOnNullInput)
{
    epc_parser_t * p = epc_char(list, NULL, 'a');
    result = parse(p, NULL);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PStringMatchesCorrectString)
{
    epc_parser_t * p = epc_string(list, NULL, "hello");
    result = parse(p, "hello world");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("string", result.data.success->tag);
    STRCMP_EQUAL("string", result.data.success->name);
    STRNCMP_EQUAL("hello", content, 5);
    LONGS_EQUAL(5, result.data.success->token.count);
}

TEST(TerminalParsers, PStringDoesNotMatchIncorrectString)
{
    epc_parser_t * p = epc_string(list, NULL, "world");
    epc_parse_result_t result = parse(p, "hello world");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PStringFailsWhenInputTooShort)
{
    epc_parser_t * p = epc_string(list, NULL, "hello");
    epc_parse_result_t result = parse(p, "hell");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PStringFailsOnEmptyInput)
{
    epc_parser_t * p = epc_string(list, NULL, "hello");
    epc_parse_result_t result = parse(p, "");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PStringFailsOnNullInput)
{
    epc_parser_t * p = epc_string(list, NULL, "hello");
    epc_parse_result_t result = parse(p, NULL);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PDigitMatchesCorrectDigit)
{
    epc_parser_t * p = epc_digit(list, NULL);
    epc_parse_result_t result = parse(p, "123");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("digit", result.data.success->tag);
    STRCMP_EQUAL("digit", result.data.success->name);
    STRNCMP_EQUAL("1", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, PDigitDoesNotMatchNonDigit)
{
    epc_parser_t * p = epc_digit(list, NULL);
    epc_parse_result_t result = parse(p, "abc");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PDigitFailsOnEmptyInput)
{
    epc_parser_t * p = epc_digit(list, NULL);
    epc_parse_result_t result = parse(p, "");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PDigitFailsOnNullInput)
{
    epc_parser_t * p = epc_digit(list, NULL);
    epc_parse_result_t result = parse(p, NULL);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, POrMatchesFirstAlternative)
{
    epc_parser_t * p_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_b = epc_char(list, NULL, 'b');
    epc_parser_t * p_or_parser = epc_or(list, NULL, 2, p_a, p_b);
    epc_parse_result_t result = parse(p_or_parser, "abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("or", result.data.success->tag);
    STRCMP_EQUAL("or", result.data.success->name);
    STRNCMP_EQUAL("a", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, POrMatchesLaterAlternative)
{
    epc_parser_t * p_a = epc_char(list, NULL, 'x'); // Will fail
    epc_parser_t * p_b = epc_char(list, NULL, 'b'); // Will succeed
    epc_parser_t * p_or_parser = epc_or(list, NULL, 2, p_a, p_b);
    epc_parse_result_t result = parse(p_or_parser, "bca");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("or", result.data.success->tag);
    STRCMP_EQUAL("or", result.data.success->name);
    STRNCMP_EQUAL("b", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, POrFailsWhenAllAlternativesFail)
{
    epc_parser_t * p_a = epc_char(list, NULL, 'x');
    epc_parser_t * p_b = epc_char(list, NULL, 'y');
    epc_parser_t * p_or_parser = epc_or(list, NULL, 2, p_a, p_b);

    epc_parse_result_t result = parse(p_or_parser, "abc");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, POrFailsWithEmptyAlternativesList)
{
    epc_parser_t * p_or_parser = epc_or(list, NULL, 0);

    epc_parse_result_t result = parse(p_or_parser, "abc");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(TerminalParsers, PAndMatchesSequenceOfParsers)
{
    epc_parser_t * p_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_b = epc_char(list, NULL, 'b');
    epc_parser_t * p_c = epc_char(list, NULL, 'c');
    epc_parser_t * p_and_parser = epc_and(list, NULL, 3, p_a, p_b, p_c);

    epc_parse_result_t result = parse(p_and_parser, "abcde");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("and", result.data.success->tag);
    STRCMP_EQUAL("and", result.data.success->name);
    STRNCMP_EQUAL("abc", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
    LONGS_EQUAL(3, result.data.success->children_count);
    CHECK_TRUE(result.data.success->children[0] != NULL);
    CHECK_TRUE(result.data.success->children[1] != NULL);
    CHECK_TRUE(result.data.success->children[2] != NULL);
    STRNCMP_EQUAL("a", epc_cpt_node_get_content(result.data.success->children[0]), 1);
    STRNCMP_EQUAL("b", epc_cpt_node_get_content(result.data.success->children[1]), 1);
    STRNCMP_EQUAL("c", epc_cpt_node_get_content(result.data.success->children[2]), 1);
}

TEST(TerminalParsers, PAndFailsIfFirstChildFails)
{
    epc_parser_t * p_x = epc_char(list, NULL, 'x'); // Will fail
    epc_parser_t * p_b = epc_char(list, NULL, 'b');
    epc_parser_t * p_c = epc_char(list, NULL, 'c');
    epc_parser_t * p_and_parser = epc_and(list, NULL, 3, p_x, p_b, p_c);
    char const * input_str = "abc";
    result = parse(p_and_parser, input_str);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected character", result.data.error->message);
    CHECK_EQUAL(0, result.data.error->view.offset);
    STRCMP_EQUAL("x", result.data.error->expected);
    STRCMP_EQUAL("a", result.data.error->found);
}

TEST(TerminalParsers, PAndFailsIfMiddleChildFails)
{
    epc_parser_t * p_a = epc_char(list, NULL, 'a');
    epc_parser_t * p_x = epc_char(list, NULL, 'x'); // Will fail
    epc_parser_t * p_c = epc_char(list, NULL, 'c');
    epc_parser_t * p_and_parser = epc_and(list, NULL, 3, p_a, p_x, p_c);
    char const * input_str = "abc";

    result = parse(p_and_parser, input_str);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected character", result.data.error->message);
    CHECK_EQUAL(1, result.data.error->view.offset);
    STRCMP_EQUAL("x", result.data.error->expected);
    STRCMP_EQUAL("b", result.data.error->found);
}

TEST(TerminalParsers, PAndFailsWithEmptySequenceList)
{
    epc_parser_t * p_and_parser = epc_and(list, NULL, 0);
    char const * input_str = "abc";

    result = parse(p_and_parser, input_str);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("No parsers in 'and' sequence", result.data.error->message);
    CHECK_EQUAL(0, result.data.error->view.offset);
    STRCMP_EQUAL("and", result.data.error->expected);
    STRCMP_EQUAL("N/A", result.data.error->found);
}

TEST(TerminalParsers, PSpaceMatchesSpace)
{
    epc_parser_t * p = epc_space(list, NULL);
    result = parse(p, " abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("space", result.data.success->tag);
    STRCMP_EQUAL("space", result.data.success->name);
    STRNCMP_EQUAL(" ", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, PSpaceMatchesTab)
{
    epc_parser_t * p = epc_space(list, NULL);
    result = parse(p, "\tabc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("space", result.data.success->tag);
    STRCMP_EQUAL("space", result.data.success->name);
    STRNCMP_EQUAL("\t", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, PSpaceMatchesNewline)
{
    epc_parser_t * p = epc_space(list, NULL);
    result = parse(p, "\nabc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("space", result.data.success->tag);
    STRCMP_EQUAL("space", result.data.success->name);
    STRNCMP_EQUAL("\n", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(TerminalParsers, PSpaceDoesNotMatchNonWhitespace)
{
    epc_parser_t * p = epc_space(list, NULL);
    char const * input_str = "abc";
    result = parse(p, input_str);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected character", result.data.error->message);
    CHECK_EQUAL(0, result.data.error->view.offset);
    STRCMP_EQUAL("whitespace", result.data.error->expected);
    STRCMP_EQUAL("a", result.data.error->found);
}

TEST(TerminalParsers, PSpaceFailsOnEmptyInput)
{
    epc_parser_t * p = epc_space(list, NULL);
    result = parse(p, "");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected end of input", result.data.error->message);
    CHECK_EQUAL(0, result.data.error->view.offset);
    STRCMP_EQUAL("space", result.data.error->expected);
    STRCMP_EQUAL("EOF", result.data.error->found);
}

TEST(TerminalParsers, PSkipSkipsMultipleSpaces)
{
    epc_parser_t * p_s = epc_space(list, NULL);
    epc_parser_t * p = epc_skip(list, NULL, p_s);
    char const * input_str = "   abc";

    result = parse(p, input_str);

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("skip", result.data.success->tag);
    STRCMP_EQUAL("skip", result.data.success->name);
    STRCMP_EQUAL(input_str, content);
    LONGS_EQUAL(3, result.data.success->token.count); // Skipped 3 spaces
}

TEST(TerminalParsers, PSkipSkipsZeroSpaces)
{
    epc_parser_t * p_s = epc_space(list, NULL);
    epc_parser_t * p = epc_skip(list, NULL, p_s);
    char const * input_str = "abc";

    result = parse(p, input_str);

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("skip", result.data.success->tag);
    STRCMP_EQUAL("skip", result.data.success->name);
    STRCMP_EQUAL(input_str, content);
    LONGS_EQUAL(0, result.data.success->token.count); // Skipped 0 spaces
}

TEST(TerminalParsers, PSkipSkipsMixedWhitespace)
{
    epc_parser_t * p_s = epc_space(list, NULL);
    epc_parser_t * p = epc_skip(list, NULL, p_s);
    char const * input_str = " \t\n\r abc"; // Space, tab, newline, carriage return

    result = parse(p, input_str);

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("skip", result.data.success->tag);
    STRCMP_EQUAL("skip", result.data.success->name);
    STRNCMP_EQUAL(input_str, content, 5);             // Should be at the start of the input string
    LONGS_EQUAL(5, result.data.success->token.count); // Skipped 5 chars
}

TEST(TerminalParsers, PSkipHandlesNullChildParser)
{
    epc_parser_t * p = epc_skip(list, NULL, NULL);
    char const * input_str = "abc";

    result = parse(p, input_str);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("p_skip received NULL child parser", result.data.error->message);
    CHECK_EQUAL(0, result.data.error->view.offset);
    STRCMP_EQUAL("skip", result.data.error->expected);
    STRCMP_EQUAL("NULL", result.data.error->found);
}

// NEW TEST GROUP FOR P_DOUBLE
TEST_GROUP(DoubleParser)
{
    epc_parse_session_t session = {0};
    epc_parse_result_t result;
    epc_parser_list * list;

    void setup() override
    {
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

// Valid Double Tests
TEST(DoubleParser, PDoubleMatchesInteger)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "123abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("123", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesSimpleDecimal)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "123.45xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("123.45", content, 6);
    LONGS_EQUAL(6, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesLeadingDecimal)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, ".45xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL(".45", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesTrailingDecimal)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "123.xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("123.", content, 4);
    LONGS_EQUAL(4, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesPositiveSign)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "+123.45xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("+123.45", content, 7);
    LONGS_EQUAL(7, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesNegativeSign)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "-123xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("-123", content, 4);
    LONGS_EQUAL(4, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesExponentPositive)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "1.23e5xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("1.23e5", content, 6);
    LONGS_EQUAL(6, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesExponentNegative)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "1.23E-5xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("1.23E-5", content, 7);
    LONGS_EQUAL(7, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesExponentWithSign)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "-1e+2xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("-1e+2", content, 5);
    LONGS_EQUAL(5, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesZero)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "0xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("0", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(DoubleParser, PDoubleMatchesZeroDecimal)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "0.0xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("double", result.data.success->tag);
    STRCMP_EQUAL("double", result.data.success->name);
    STRNCMP_EQUAL("0.0", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
}

// Invalid Double Tests
TEST(DoubleParser, PDoubleFailsOnNonNumeric)
{
    epc_parser_t * p = epc_double(list, NULL);
    char const * input = "abc";
    result = parse(p, input);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a double", result.data.error->message);
    STRNCMP_EQUAL("a", result.data.error->found, 1);
}

TEST(DoubleParser, PDoubleFailsOnEmptyInput)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected end of input", result.data.error->message);
    STRCMP_EQUAL("EOF", result.data.error->found);
}

TEST(DoubleParser, PDoubleFailsOnNullInput)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, NULL);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(DoubleParser, PDoubleFailsOnJustDecimalPoint)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, ".");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a double", result.data.error->message);
    STRNCMP_EQUAL(".", result.data.error->found, 1);
}

TEST(DoubleParser, PDoubleFailsOnJustSign)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "+");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a double", result.data.error->message);
    STRNCMP_EQUAL("+", result.data.error->found, 1);
}

TEST(DoubleParser, PDoubleFailsOnSignDecimal)
{
    epc_parser_t * p = epc_double(list, NULL);
    result = parse(p, "+.");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a double", result.data.error->message);
    STRNCMP_EQUAL("+", result.data.error->found, 1); // Only '+' is reported as found
}

TEST(DoubleParser, PLongDoubleMatchesInteger)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "123abc");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("123", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesSimpleDecimal)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "123.45xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("123.45", content, 6);
    LONGS_EQUAL(6, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesLeadingDecimal)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, ".45xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL(".45", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesTrailingDecimal)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "123.xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("123.", content, 4);
    LONGS_EQUAL(4, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesPositiveSign)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "+123.45xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("+123.45", content, 7);
    LONGS_EQUAL(7, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesNegativeSign)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "-123xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("-123", content, 4);
    LONGS_EQUAL(4, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesExponentPositive)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "1.23e5xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("1.23e5", content, 6);
    LONGS_EQUAL(6, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesExponentNegative)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "1.23E-5xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("1.23E-5", content, 7);
    LONGS_EQUAL(7, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesExponentWithSign)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "-1e+2xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("-1e+2", content, 5);
    LONGS_EQUAL(5, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesZero)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "0xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("0", content, 1);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(DoubleParser, PLongDoubleMatchesZeroDecimal)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "0.0xyz");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    char const * content = epc_cpt_node_get_content(result.data.success);

    STRCMP_EQUAL("long_double", result.data.success->tag);
    STRCMP_EQUAL("long_double", result.data.success->name);
    STRNCMP_EQUAL("0.0", content, 3);
    LONGS_EQUAL(3, result.data.success->token.count);
}

// Invalid Double Tests
TEST(DoubleParser, PLongDoubleFailsOnNonNumeric)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    char const * input = "abc";
    result = parse(p, input);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a long double", result.data.error->message);
    STRNCMP_EQUAL("a", result.data.error->found, 1);
}

TEST(DoubleParser, PLongDoubleFailsOnEmptyInput)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Unexpected end of input", result.data.error->message);
    STRCMP_EQUAL("EOF", result.data.error->found);
}

TEST(DoubleParser, PLongDoubleFailsOnNullInput)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, NULL);

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
}

TEST(DoubleParser, PLongDoubleFailsOnJustDecimalPoint)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, ".");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a long double", result.data.error->message);
    STRNCMP_EQUAL(".", result.data.error->found, 1);
}

TEST(DoubleParser, PLongDoubleFailsOnJustSign)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "+");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a long double", result.data.error->message);
    STRNCMP_EQUAL("+", result.data.error->found, 1);
}

TEST(DoubleParser, PLongDoubleFailsOnSignDecimal)
{
    epc_parser_t * p = epc_long_double(list, NULL);
    result = parse(p, "+.");

    CHECK_TRUE(result.is_error);
    CHECK_TRUE(result.data.error != NULL);
    STRCMP_EQUAL("Expected a long double", result.data.error->message);
    STRNCMP_EQUAL("+", result.data.error->found, 1); // Only '+' is reported as found
}
