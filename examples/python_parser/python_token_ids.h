#pragma once

#include <easy_pc/easy_pc.h>

// Token ID range:
//  0-255        = ASCII character codes (used for single-char tokens)
//  256-299      = EPC reserved range
//  300+         = Custom Python token IDs

typedef enum
{
    // Delimiters (use char codes where possible, custom for multi-char)
    TOKEN_DOT = '.',
    TOKEN_COMMA = ',',
    TOKEN_COLON = ':',
    TOKEN_SEMICOLON = ';',
    TOKEN_LPAREN = '(',
    TOKEN_RPAREN = ')',
    TOKEN_LBRACE = '{',
    TOKEN_RBRACE = '}',
    TOKEN_LBRACKET = '[',
    TOKEN_RBRACKET = ']',
    TOKEN_BACKSLASH = '\\',

    // Operators
    TOKEN_PLUS = '+',
    TOKEN_MINUS = '-',
    TOKEN_STAR = '*',
    TOKEN_SLASH = '/',
    TOKEN_PERCENT = '%',
    TOKEN_AMPERSAND = '&',
    TOKEN_PIPE = '|',
    TOKEN_CARET = '^',
    TOKEN_TILDE = '~',
    TOKEN_LESS = '<',
    TOKEN_GREATER = '>',
    TOKEN_EQUAL = '=',
    TOKEN_BANG = '!',
    TOKEN_AT = '@',

    // Line structure
    TOKEN_NEWLINE = EPC_TOKEN_ID_FIRST_USER,
    TOKEN_INDENT,
    TOKEN_DEDENT,

    // Generic token types
    TOKEN_NAME,
    TOKEN_NUMBER,
    TOKEN_STRING,
    TOKEN_COMMENT,
    TOKEN_ENDMARKER,
    TOKEN_ERROR,

    // Keywords (alphabetical order)
    TOKEN_KW_FALSE,
    TOKEN_KW_NONE,
    TOKEN_KW_TRUE,
    TOKEN_KW_AND,
    TOKEN_KW_AS,
    TOKEN_KW_ASSERT,
    TOKEN_KW_ASYNC,
    TOKEN_KW_AWAIT,
    TOKEN_KW_BREAK,
    TOKEN_KW_CLASS,
    TOKEN_KW_CONTINUE,
    TOKEN_KW_DEF,
    TOKEN_KW_DEL,
    TOKEN_KW_ELIF,
    TOKEN_KW_ELSE,
    TOKEN_KW_EXCEPT,
    TOKEN_KW_FINALLY,
    TOKEN_KW_FOR,
    TOKEN_KW_FROM,
    TOKEN_KW_GLOBAL,
    TOKEN_KW_IF,
    TOKEN_KW_IMPORT,
    TOKEN_KW_IN,
    TOKEN_KW_IS,
    TOKEN_KW_LAMBDA,
    TOKEN_KW_NONLOCAL,
    TOKEN_KW_NOT,
    TOKEN_KW_OR,
    TOKEN_KW_PASS,
    TOKEN_KW_RAISE,
    TOKEN_KW_RETURN,
    TOKEN_KW_TRY,
    TOKEN_KW_WHILE,
    TOKEN_KW_WITH,
    TOKEN_KW_YIELD,
    TOKEN_KW_MATCH,
    TOKEN_KW_CASE,
    TOKEN_KW_PRINT,

    // Operators (specific IDs useful for stage 2)
    TOKEN_DOUBLE_STAR,
    TOKEN_DOUBLE_SLASH,
    TOKEN_LEFT_SHIFT,
    TOKEN_RIGHT_SHIFT,
    TOKEN_DOUBLE_EQUAL,
    TOKEN_NOT_EQUAL,
    TOKEN_LESS_EQUAL,
    TOKEN_GREATER_EQUAL,
    TOKEN_ARROW,
    TOKEN_WALRUS,
    TOKEN_ELLIPSIS,
    TOKEN_PLUS_ASSIGN,
    TOKEN_MINUS_ASSIGN,
    TOKEN_STAR_ASSIGN,
    TOKEN_SLASH_ASSIGN,
    TOKEN_PERCENT_ASSIGN,
    TOKEN_AMPERSAND_ASSIGN,
    TOKEN_PIPE_ASSIGN,
    TOKEN_CARET_ASSIGN,
    TOKEN_LEFT_SHIFT_ASSIGN,
    TOKEN_RIGHT_SHIFT_ASSIGN,
    TOKEN_DOUBLE_STAR_ASSIGN,
    TOKEN_DOUBLE_SLASH_ASSIGN,
} python_token_type_t;

// Lookup a token name string for display
static inline char const *
token_id_name(epc_token_id_t id)
{
    switch (id)
    {
    case TOKEN_NEWLINE:
        return "NEWLINE";
    case TOKEN_INDENT:
        return "INDENT";
    case TOKEN_DEDENT:
        return "DEDENT";
    case TOKEN_NAME:
        return "NAME";
    case TOKEN_NUMBER:
        return "NUMBER";
    case TOKEN_STRING:
        return "STRING";
    case TOKEN_COMMENT:
        return "COMMENT";
    case TOKEN_ENDMARKER:
        return "ENDMARKER";
    case TOKEN_ERROR:
        return "ERROR";
    case TOKEN_KW_FALSE:
        return "KW_FALSE";
    case TOKEN_KW_NONE:
        return "KW_NONE";
    case TOKEN_KW_TRUE:
        return "KW_TRUE";
    case TOKEN_KW_AND:
        return "KW_AND";
    case TOKEN_KW_AS:
        return "KW_AS";
    case TOKEN_KW_ASSERT:
        return "KW_ASSERT";
    case TOKEN_KW_ASYNC:
        return "KW_ASYNC";
    case TOKEN_KW_AWAIT:
        return "KW_AWAIT";
    case TOKEN_KW_BREAK:
        return "KW_BREAK";
    case TOKEN_KW_CLASS:
        return "KW_CLASS";
    case TOKEN_KW_CONTINUE:
        return "KW_CONTINUE";
    case TOKEN_KW_DEF:
        return "KW_DEF";
    case TOKEN_KW_DEL:
        return "KW_DEL";
    case TOKEN_KW_ELIF:
        return "KW_ELIF";
    case TOKEN_KW_ELSE:
        return "KW_ELSE";
    case TOKEN_KW_EXCEPT:
        return "KW_EXCEPT";
    case TOKEN_KW_FINALLY:
        return "KW_FINALLY";
    case TOKEN_KW_FOR:
        return "KW_FOR";
    case TOKEN_KW_FROM:
        return "KW_FROM";
    case TOKEN_KW_GLOBAL:
        return "KW_GLOBAL";
    case TOKEN_KW_IF:
        return "KW_IF";
    case TOKEN_KW_IMPORT:
        return "KW_IMPORT";
    case TOKEN_KW_IN:
        return "KW_IN";
    case TOKEN_KW_IS:
        return "KW_IS";
    case TOKEN_KW_LAMBDA:
        return "KW_LAMBDA";
    case TOKEN_KW_NONLOCAL:
        return "KW_NONLOCAL";
    case TOKEN_KW_NOT:
        return "KW_NOT";
    case TOKEN_KW_OR:
        return "KW_OR";
    case TOKEN_KW_PASS:
        return "KW_PASS";
    case TOKEN_KW_RAISE:
        return "KW_RAISE";
    case TOKEN_KW_RETURN:
        return "KW_RETURN";
    case TOKEN_KW_TRY:
        return "KW_TRY";
    case TOKEN_KW_WHILE:
        return "KW_WHILE";
    case TOKEN_KW_WITH:
        return "KW_WITH";
    case TOKEN_KW_YIELD:
        return "KW_YIELD";
    case TOKEN_KW_MATCH:
        return "KW_MATCH";
    case TOKEN_KW_CASE:
        return "KW_CASE";
    case TOKEN_KW_PRINT:
        return "KW_PRINT";
    case TOKEN_PLUS:
        return "+";
    case TOKEN_MINUS:
        return "-";
    case TOKEN_STAR:
        return "*";
    case TOKEN_SLASH:
        return "/";
    case TOKEN_PERCENT:
        return "%";
    case TOKEN_DOUBLE_STAR:
        return "**";
    case TOKEN_DOUBLE_SLASH:
        return "//";
    case TOKEN_AMPERSAND:
        return "&";
    case TOKEN_PIPE:
        return "|";
    case TOKEN_CARET:
        return "^";
    case TOKEN_TILDE:
        return "~";
    case TOKEN_LESS:
        return "<";
    case TOKEN_GREATER:
        return ">";
    case TOKEN_EQUAL:
        return "=";
    case TOKEN_BANG:
        return "!";
    case TOKEN_AT:
        return "@";
    case TOKEN_LEFT_SHIFT:
        return "<<";
    case TOKEN_RIGHT_SHIFT:
        return ">>";
    case TOKEN_DOUBLE_EQUAL:
        return "==";
    case TOKEN_NOT_EQUAL:
        return "!=";
    case TOKEN_LESS_EQUAL:
        return "<=";
    case TOKEN_GREATER_EQUAL:
        return ">=";
    case TOKEN_ARROW:
        return "->";
    case TOKEN_WALRUS:
        return ":=";
    case TOKEN_ELLIPSIS:
        return "...";
    case TOKEN_PLUS_ASSIGN:
        return "+=";
    case TOKEN_MINUS_ASSIGN:
        return "-=";
    case TOKEN_STAR_ASSIGN:
        return "*=";
    case TOKEN_SLASH_ASSIGN:
        return "/=";
    case TOKEN_PERCENT_ASSIGN:
        return "%=";
    case TOKEN_AMPERSAND_ASSIGN:
        return "&=";
    case TOKEN_PIPE_ASSIGN:
        return "|=";
    case TOKEN_CARET_ASSIGN:
        return "^=";
    case TOKEN_LEFT_SHIFT_ASSIGN:
        return "<<=";
    case TOKEN_RIGHT_SHIFT_ASSIGN:
        return ">>=";
    case TOKEN_DOUBLE_STAR_ASSIGN:
        return "**=";
    case TOKEN_DOUBLE_SLASH_ASSIGN:
        return "//=";
    case TOKEN_DOT:
        return ".";
    case TOKEN_COMMA:
        return ",";
    case TOKEN_COLON:
        return ":";
    case TOKEN_SEMICOLON:
        return ";";
    case TOKEN_LPAREN:
        return "(";
    case TOKEN_RPAREN:
        return ")";
    case TOKEN_LBRACE:
        return "{";
    case TOKEN_RBRACE:
        return "}";
    case TOKEN_LBRACKET:
        return "[";
    case TOKEN_RBRACKET:
        return "]";
    case TOKEN_BACKSLASH:
        return "\\";
    default:
        if (id < 256 && id > 32)
        {
            static char buf[2] = {0, 0};
            buf[0] = (char)id;
            return buf;
        }
        return "UNKNOWN";
    }
}
