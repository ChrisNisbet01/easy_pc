#pragma once

#include <easy_pc/easy_pc.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    TOKEN_IDENTIFIER = EPC_TOKEN_ID_FIRST_USER,

    // Literals
    TOKEN_STRING_LITERAL,
    TOKEN_CHAR_LITERAL,
    TOKEN_RAW_CHAR_LITERAL,
    TOKEN_NUMBER,
    TOKEN_TOKEN_LITERAL,
    TOKEN_CHAR_RANGE,

    // Structural single-character tokens
    TOKEN_EQUALS,
    TOKEN_SEMICOLON,
    TOKEN_PIPE,
    TOKEN_AT,
    TOKEN_LPAREN,
    TOKEN_RPAREN,
    TOKEN_LBRACKET,
    TOKEN_RBRACKET,
    TOKEN_COMMA,
    TOKEN_MINUS,
    TOKEN_STAR,
    TOKEN_PLUS,
    TOKEN_QUESTION,

    // Keywords — terminal parsers
    TOKEN_KW_CHAR,
    TOKEN_KW_DIGIT,
    TOKEN_KW_ALPHANUM,
    TOKEN_KW_ALPHA,
    TOKEN_KW_IDENTIFIER,
    TOKEN_KW_INT,
    TOKEN_KW_OCTAL,
    TOKEN_KW_HEX,
    TOKEN_KW_DOUBLE,
    TOKEN_KW_LONG_DOUBLE,
    TOKEN_KW_SPACE,
    TOKEN_KW_ANY,
    TOKEN_KW_SUCCEED,
    TOKEN_KW_HEX_DIGIT,
    TOKEN_KW_SOI,
    TOKEN_KW_EOI,
    TOKEN_KW_FAIL,
    TOKEN_KW_CPP_COMMENT,
    TOKEN_KW_C_COMMENT,
    TOKEN_KW_BASH_COMMENT,

    // Keywords — combinator parsers
    TOKEN_KW_STRING,
    TOKEN_KW_CHAR_RANGE,
    TOKEN_KW_NONEOF,
    TOKEN_KW_MANY,
    TOKEN_KW_COUNT,
    TOKEN_KW_COUNT_RANGE,
    TOKEN_KW_BETWEEN,
    TOKEN_KW_DELIMITED,
    TOKEN_KW_DELIMITED_FLEX,
    TOKEN_KW_OPTIONAL,
    TOKEN_KW_LOOKAHEAD,
    TOKEN_KW_NOT,
    TOKEN_KW_ONEOF,
    TOKEN_KW_LEXEME,
    TOKEN_KW_STRIP,
    TOKEN_KW_STRIPL,
    TOKEN_KW_STRIPR,
    TOKEN_KW_CHAINL1,
    TOKEN_KW_CHAINR1,
    TOKEN_KW_SKIP,
    TOKEN_KW_MEMOIZE,
    TOKEN_KW_SATISFY,
    TOKEN_KW_WRAP,

    // Keywords — lexeme flags
    TOKEN_KW_WS,
    TOKEN_KW_ALL,
    TOKEN_KW_ALL_STYLES,
    TOKEN_KW_ALL_COMMENTS,

    // Special end-of-input marker
    TOKEN_ENDMARKER,

    TOKEN_COUNT,
} gdl_token_id_t;

static inline char const *
gdl_token_id_name(epc_token_id_t id)
{
    switch (id)
    {
    case TOKEN_IDENTIFIER:
        return "IDENTIFIER";
    case TOKEN_STRING_LITERAL:
        return "STRING_LITERAL";
    case TOKEN_CHAR_LITERAL:
        return "CHAR_LITERAL";
    case TOKEN_NUMBER:
        return "NUMBER";
    case TOKEN_TOKEN_LITERAL:
        return "TOKEN_LITERAL";
    case TOKEN_CHAR_RANGE:
        return "CHAR_RANGE";
    case TOKEN_EQUALS:
        return "EQUALS";
    case TOKEN_SEMICOLON:
        return "SEMICOLON";
    case TOKEN_PIPE:
        return "PIPE";
    case TOKEN_AT:
        return "AT";
    case TOKEN_LPAREN:
        return "LPAREN";
    case TOKEN_RPAREN:
        return "RPAREN";
    case TOKEN_LBRACKET:
        return "LBRACKET";
    case TOKEN_RBRACKET:
        return "RBRACKET";
    case TOKEN_COMMA:
        return "COMMA";
    case TOKEN_MINUS:
        return "MINUS";
    case TOKEN_STAR:
        return "STAR";
    case TOKEN_PLUS:
        return "PLUS";
    case TOKEN_QUESTION:
        return "QUESTION";
    case TOKEN_KW_CHAR:
        return "KW_CHAR";
    case TOKEN_KW_DIGIT:
        return "KW_DIGIT";
    case TOKEN_KW_ALPHANUM:
        return "KW_ALPHANUM";
    case TOKEN_KW_ALPHA:
        return "KW_ALPHA";
    case TOKEN_KW_IDENTIFIER:
        return "KW_IDENTIFIER";
    case TOKEN_KW_INT:
        return "KW_INT";
    case TOKEN_KW_OCTAL:
        return "KW_OCTAL";
    case TOKEN_KW_HEX:
        return "KW_HEX";
    case TOKEN_KW_DOUBLE:
        return "KW_DOUBLE";
    case TOKEN_KW_LONG_DOUBLE:
        return "KW_LONG_DOUBLE";
    case TOKEN_KW_SPACE:
        return "KW_SPACE";
    case TOKEN_KW_ANY:
        return "KW_ANY";
    case TOKEN_KW_SUCCEED:
        return "KW_SUCCEED";
    case TOKEN_KW_HEX_DIGIT:
        return "KW_HEX_DIGIT";
    case TOKEN_KW_SOI:
        return "KW_SOI";
    case TOKEN_KW_EOI:
        return "KW_EOI";
    case TOKEN_KW_FAIL:
        return "KW_FAIL";
    case TOKEN_KW_CPP_COMMENT:
        return "KW_CPP_COMMENT";
    case TOKEN_KW_C_COMMENT:
        return "KW_C_COMMENT";
    case TOKEN_KW_BASH_COMMENT:
        return "KW_BASH_COMMENT";
    case TOKEN_KW_STRING:
        return "KW_STRING";
    case TOKEN_KW_CHAR_RANGE:
        return "KW_CHAR_RANGE";
    case TOKEN_KW_NONEOF:
        return "KW_NONEOF";
    case TOKEN_KW_MANY:
        return "KW_MANY";
    case TOKEN_KW_COUNT:
        return "KW_COUNT";
    case TOKEN_KW_COUNT_RANGE:
        return "KW_COUNT_RANGE";
    case TOKEN_KW_BETWEEN:
        return "KW_BETWEEN";
    case TOKEN_KW_DELIMITED:
        return "KW_DELIMITED";
    case TOKEN_KW_DELIMITED_FLEX:
        return "KW_DELIMITED_FLEX";
    case TOKEN_KW_OPTIONAL:
        return "KW_OPTIONAL";
    case TOKEN_KW_LOOKAHEAD:
        return "KW_LOOKAHEAD";
    case TOKEN_KW_NOT:
        return "KW_NOT";
    case TOKEN_KW_ONEOF:
        return "KW_ONEOF";
    case TOKEN_KW_LEXEME:
        return "KW_LEXEME";
    case TOKEN_KW_STRIP:
        return "KW_STRIP";
    case TOKEN_KW_STRIPL:
        return "KW_STRIPL";
    case TOKEN_KW_STRIPR:
        return "KW_STRIPR";
    case TOKEN_KW_CHAINL1:
        return "KW_CHAINL1";
    case TOKEN_KW_CHAINR1:
        return "KW_CHAINR1";
    case TOKEN_KW_SKIP:
        return "KW_SKIP";
    case TOKEN_KW_MEMOIZE:
        return "KW_MEMOIZE";
    case TOKEN_KW_SATISFY:
        return "KW_SATISFY";
    case TOKEN_KW_WRAP:
        return "KW_WRAP";
    case TOKEN_KW_WS:
        return "KW_WS";
    case TOKEN_KW_ALL:
        return "KW_ALL";
    case TOKEN_KW_ALL_STYLES:
        return "KW_ALL_STYLES";
    case TOKEN_KW_ALL_COMMENTS:
        return "KW_ALL_COMMENTS";
    case TOKEN_ENDMARKER:
        return "ENDMARKER";
    default:
        return "UNKNOWN";
    }
}

#ifdef __cplusplus
}
#endif
