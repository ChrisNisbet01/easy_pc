#include "CppUTest/TestHarness.h"

#include <iostream>

extern "C" {
#include "cpt_node.h"
#include "easy_pc_private.h"

#include <stdlib.h>
#include <string.h>
}

TEST_GROUP(Utf8Parsers)
{
    epc_parse_session_t session = {0};
    epc_parser_list * list;

    void setup() override
    {
        list = epc_parser_list_create();
    }

    epc_parse_result_t parse(epc_parser_t * parser, char const * input)
    {
        void * user_ctx = NULL;
        session = epc_parse_str(parser, input, NULL);
        return session.result;
    }

    void teardown() override
    {
        epc_parse_session_destroy(&session);
        epc_parser_list_free(list);
    }
};

TEST(Utf8Parsers, Utf8CharMatchesByStringLiteral)
{
    epc_parser_t * p = epc_utf8_char(list, "pi", "π");
    epc_parse_result_t result = parse(p, "π");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(1, result.data.success->token.count); // π is one token (2 bytes)
}

TEST(Utf8Parsers, Utf8CharMatchesFromCodepoint)
{
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "pi", 0x03C0);
    epc_parse_result_t result = parse(p, "π");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(Utf8Parsers, Utf8CharDoesNotMatchWrongCharacter)
{
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "pi", 0x03C0);
    epc_parse_result_t result = parse(p, "ν");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, Utf8CharFailsOnEmptyInput)
{
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "pi", 0x03C0);
    epc_parse_result_t result = parse(p, "");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, Utf8CharRejectsMultiCharacterString)
{
    epc_parser_t * p = epc_utf8_char(list, "bad", "πa");

    POINTERS_EQUAL(NULL, p);
}

TEST(Utf8Parsers, Utf8CharRejectsInvalidUtf8)
{
    epc_parser_t * p = epc_utf8_char(list, "bad", "\xC0\xAF"); /* Overlong '/' */

    POINTERS_EQUAL(NULL, p);
}

TEST(Utf8Parsers, Utf8CharFromCodepointRejectsSurrogate)
{
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "bad", 0xD800);

    POINTERS_EQUAL(NULL, p);
}

TEST(Utf8Parsers, Utf8MatchesJapaneseCharacter)
{
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "day", 0x65E5);
    epc_parse_result_t result = parse(p, "日");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(1, result.data.success->token.count); // 日 is one token (3 bytes)
}

TEST(Utf8Parsers, Utf8MatchesEmoji)
{
    char const * smiley = "😀";
    epc_parser_t * p = epc_utf8_char(list, "smiley", smiley);
    epc_parse_result_t result = parse(p, smiley);

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(1, result.data.success->token.count); // one token (4 bytes)
}

TEST(Utf8Parsers, Utf8TwoByteCharacter)
{
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "acute", 0x00B4); // ´
    epc_parse_result_t result = parse(p, "\xC2\xB4");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(1, result.data.success->token.count);
}

TEST(Utf8Parsers, InvalidUtf8InputFails)
{
    char const * invalid = "\xC2"; /* Truncated 2-byte sequence at end of input. */
    epc_parser_t * p = epc_utf8_char_from_codepoint(list, "pi", 0x03C0);
    epc_parse_result_t result = parse(p, invalid);

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, Utf8NoneOfAllowsCharsNotInSet)
{
    epc_parser_t * p = epc_none_of(list, "not_a", "abc");
    epc_parse_result_t result = parse(p, "d");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
}

TEST(Utf8Parsers, Utf8NoneOfFailsOnCharInSet)
{
    epc_parser_t * p = epc_none_of(list, "not_a", "abc");
    epc_parse_result_t result = parse(p, "a");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, Utf8NoneOfAllowsUtf8CharsNotInSet)
{
    epc_parser_t * p = epc_none_of(list, "not_a", "abc");
    epc_parse_result_t result = parse(p, "π");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
}

TEST(Utf8Parsers, Utf8NoneOfExcludesUtf8CharsInSet)
{
    epc_parser_t * p = epc_none_of(list, "not_jan", "日本");
    epc_parse_result_t result = parse(p, "日");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, NoneOfRejectsInvalidUtf8Set)
{
    epc_parser_t * p = epc_none_of(list, "bad", "\xC0\xAF");

    POINTERS_EQUAL(NULL, p);
}

TEST(Utf8Parsers, StringMatchesUtf8Content)
{
    epc_parser_t * p = epc_string(list, "s", "日本語");
    epc_parse_result_t result = parse(p, "日本語");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(3, result.data.success->token.count); // three characters
}

TEST(Utf8Parsers, StringMatchesMixedAsciiAndUtf8)
{
    epc_parser_t * p = epc_string(list, "s", "aπb");
    epc_parse_result_t result = parse(p, "aπb");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(3, result.data.success->token.count);
}

TEST(Utf8Parsers, StringFailsOnMismatchedUtf8)
{
    epc_parser_t * p = epc_string(list, "s", "日本");
    epc_parse_result_t result = parse(p, "日");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, StringRejectsInvalidUtf8)
{
    epc_parser_t * p = epc_string(list, "s", "ab\xC0\xAF");

    POINTERS_EQUAL(NULL, p);
}

TEST(Utf8Parsers, SimpleOrAndCombination)
{
    epc_parser_t * p_a = epc_char(list, "A", 'a');
    epc_parser_t * p_b = epc_char(list, "B", 'b');
    epc_parser_t * p_or = epc_or(list, "OR", 2, p_a, p_b);
    epc_parser_t * p_and = epc_and(list, "AND", 2, p_or, epc_char(list, "C", 'c'));
    
    epc_parse_result_t result = parse(p_and, "ac");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
}

TEST(Utf8Parsers, ManyInsideAnd)
{
    epc_parser_t * p_a = epc_char(list, "A", 'a');
    epc_parser_t * p_many = epc_many(list, "MANY", p_a);
    epc_parser_t * p_and = epc_and(list, "AND", 2, p_many, epc_char(list, "B", 'b'));
    
    epc_parse_result_t result = parse(p_and, "aaab");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
}

TEST(Utf8Parsers, Utf8OrAndWithNonAscii)
{
    epc_parser_t * p_underscore = epc_char(list, "_", '_');
    epc_parser_t * p_utf8_start = epc_any(list, "AnyStart");
    epc_parser_t * p_start = epc_or(list, "Start", 2, p_underscore, p_utf8_start);
    epc_parser_t * p_cont = epc_or(list, "Cont", 3, 
        epc_alpha(list, NULL),
        epc_digit(list, NULL),
        epc_any(list, "Any"));
    epc_parser_t * p_id = epc_and(list, "Identifier", 2, p_start, epc_many(list, "Rest", p_cont));
    
    epc_parse_result_t result = parse(p_id, "παράδειγμα");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
}

TEST(Utf8Parsers, Utf8IdentifierMatchesAscii)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "hello_world");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(11, result.data.success->token.count);
}

TEST(Utf8Parsers, Utf8IdentifierMatchesUtf8Start)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "παράδειγμα");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
}

TEST(Utf8Parsers, Utf8IdentifierTokenCountCorrect)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "π");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(1, result.data.success->token.count); // π is one token
}

TEST(Utf8Parsers, Utf8IdentifierTokenCountForJapanese)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "日本"); // two characters

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(2, result.data.success->token.count);
}

TEST(Utf8Parsers, Utf8IdentifierMultiCharUtf8)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "日本語"); // three characters

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(3, result.data.success->token.count);
}

TEST(Utf8Parsers, Utf8IdentifierMixedAsciiAndUtf8)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "aπb");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(3, result.data.success->token.count); // a + π + b
}

TEST(Utf8Parsers, Utf8IdentifierFailsOnDigitStart)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "1hello");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, Utf8IdentifierFailsOnEmptyInput)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "");

    CHECK_TRUE(result.is_error);
}

TEST(Utf8Parsers, Utf8IdentifierContinuesOverUtf8)
{
    epc_parser_t * p = epc_identifier(list, "Id");
    epc_parse_result_t result = parse(p, "π123");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(4, result.data.success->token.count); // π + 1 + 2 + 3
}

TEST(Utf8Parsers, Utf8IdentifierMatchFollowedByOperator)
{
    epc_parser_t * p_id = epc_identifier(list, "Id");
    epc_parser_t * p_plus = epc_char(list, "+", '+');
    epc_parser_t * p_id2 = epc_identifier(list, "Id2");
    epc_parser_t * p_expr = epc_and(list, "Expr", 3, p_id, p_plus, p_id2);
    
    epc_parse_result_t result = parse(p_expr, "π+αβ");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(4, result.data.success->token.count); // π + + + α + β
}

TEST(Utf8Parsers, Utf8IdentifierMultiCharFollowedCorrectly)
{
    epc_parser_t * p_id = epc_identifier(list, "Id");
    epc_parser_t * p_space = epc_char(list, "Space", ' ');
    epc_parser_t * p_id2 = epc_identifier(list, "Id2");
    epc_parser_t * p_seq = epc_and(list, "Seq", 3, p_id, p_space, p_id2);
    
    epc_parse_result_t result = parse(p_seq, "π αβ");

    CHECK_FALSE(result.is_error);
    CHECK_TRUE(result.data.success != NULL);
    LONGS_EQUAL(4, result.data.success->token.count); // π + space + α + β
}
