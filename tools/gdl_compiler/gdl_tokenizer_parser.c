#include "gdl_tokenizer_parser.h"

#include "gdl_token_ids.h"
#include "gdl_tokenizer_actions.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

epc_parser_t *
create_gdl_tokenizer_parser(epc_parser_list * l)
{
    // --- Identifiers and Keywords ---
    epc_parser_t * ident_start = epc_or(l, "IdStart", 2, epc_alpha(l, "Alpha"), epc_char(l, "Underscore", '_'));
    epc_parser_t * ident_cont
        = epc_or(l, "IdCont", 3, epc_alpha(l, "Alpha"), epc_digit(l, "Digit"), epc_char(l, "Underscore", '_'));
    epc_parser_t * identifier = epc_and(l, "Identifier", 2, ident_start, epc_many(l, "IdRest", ident_cont));
    epc_parser_set_ast_action(identifier, TOKENIZER_ACTION_KEYWORD);

    // --- StringLiteral: "..." with escape handling ---
    epc_parser_t * str_esc_dq = epc_string(l, "EscDQ", "\\\"");
    epc_parser_t * str_esc_bs = epc_string(l, "EscBS", "\\\\");
    epc_parser_t * str_esc_n = epc_string(l, "EscN", "\\n");
    epc_parser_t * str_esc_t = epc_string(l, "EscT", "\\t");
    epc_parser_t * str_esc_r = epc_string(l, "EscR", "\\r");
    epc_parser_t * str_any_char = epc_none_of(l, "StrAnyChar", "\"\\");
    epc_parser_t * str_char
        = epc_or(l, "StrChar", 6, str_esc_dq, str_esc_bs, str_esc_n, str_esc_t, str_esc_r, str_any_char);
    epc_parser_t * str_content = epc_many(l, "StrContent", str_char);
    epc_parser_t * str_open = epc_char(l, "StrOpen", '"');
    epc_parser_t * str_close = epc_char(l, "StrClose", '"');
    epc_parser_t * string_literal = epc_and(l, "StringLiteral", 3, str_open, str_content, str_close);
    epc_parser_set_ast_action(string_literal, TOKENIZER_ACTION_STRING_LITERAL);

    // --- CharLiteral: '...' with escape handling ---
    epc_parser_t * chr_esc_sq = epc_string(l, "ChrEscSQ", "\\'");
    epc_parser_t * chr_esc_dq = epc_string(l, "ChrEscDQ", "\\\"");
    epc_parser_t * chr_esc_bs = epc_string(l, "ChrEscBS", "\\\\");
    epc_parser_t * chr_esc_n = epc_string(l, "ChrEscN", "\\n");
    epc_parser_t * chr_esc_t = epc_string(l, "ChrEscT", "\\t");
    epc_parser_t * chr_esc_r = epc_string(l, "ChrEscR", "\\r");
    epc_parser_t * chr_any_char = epc_none_of(l, "ChrAnyChar", "'\\");
    epc_parser_t * chr_char
        = epc_or(l, "ChrChar", 7, chr_esc_sq, chr_esc_dq, chr_esc_bs, chr_esc_n, chr_esc_t, chr_esc_r, chr_any_char);
    epc_parser_t * chr_content = epc_and(l, "ChrContent", 1, chr_char);
    epc_parser_t * chr_open = epc_char(l, "ChrOpen", '\'');
    epc_parser_t * chr_close = epc_char(l, "ChrClose", '\'');
    epc_parser_t * char_literal = epc_and(l, "CharLiteral", 3, chr_open, chr_content, chr_close);
    epc_parser_set_ast_action(char_literal, TOKENIZER_ACTION_CHAR_LITERAL);

    // --- TokenLiteral: <...> ---
    epc_parser_t * tok_lt = epc_char(l, "TokLT", '<');
    epc_parser_t * tok_content = epc_plus(l, "TokContent", epc_none_of(l, "TokContentChar", ">"));
    epc_parser_t * tok_gt = epc_char(l, "TokGT", '>');
    epc_parser_t * token_literal = epc_and(l, "TokenLiteral", 3, tok_lt, tok_content, tok_gt);
    epc_parser_set_ast_action(token_literal, TOKENIZER_ACTION_TOKEN_LITERAL);

    // --- NumberLiteral: digit+ ---
    epc_parser_t * number = epc_plus(l, "Number", epc_digit(l, "NumDigit"));
    epc_parser_set_ast_action(number, TOKENIZER_ACTION_NUMBER);

    // --- Structural chars ---
    epc_parser_t * eq = epc_char(l, "EqChar", '=');
    epc_parser_set_ast_action(eq, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * semicolon = epc_char(l, "ScChar", ';');
    epc_parser_set_ast_action(semicolon, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * pipe = epc_char(l, "PiChar", '|');
    epc_parser_set_ast_action(pipe, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * at_sign = epc_char(l, "AtChar", '@');
    epc_parser_set_ast_action(at_sign, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * lparen = epc_char(l, "LPChar", '(');
    epc_parser_set_ast_action(lparen, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * rparen = epc_char(l, "RPChar", ')');
    epc_parser_set_ast_action(rparen, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * comma = epc_char(l, "CmChar", ',');
    epc_parser_set_ast_action(comma, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * minus = epc_char(l, "MnChar", '-');
    epc_parser_set_ast_action(minus, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * star = epc_char(l, "StChar", '*');
    epc_parser_set_ast_action(star, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * plus = epc_char(l, "PlChar", '+');
    epc_parser_set_ast_action(plus, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * question = epc_char(l, "QuChar", '?');
    epc_parser_set_ast_action(question, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * caret = epc_char(l, "Carret", '^');
    epc_parser_set_ast_action(caret, TOKENIZER_ACTION_STRUCTURAL);

    // --- CharRange: [char-char] ---
    epc_parser_t * cr_escape = epc_and(l, "CREscape", 2, epc_char(l, "CRBS", '\\'), epc_any(l, "CRAny"));
    epc_parser_t * cr_char = epc_or(l, "CRChar", 2, cr_escape, epc_none_of(l, "CRNonStruct", "]"));
    epc_parser_set_ast_action(cr_char, TOKENIZER_ACTION_RAW_CHAR_LITERAL);

    epc_parser_t * cr_body = epc_and(l, "CRBody", 3, cr_char, minus, cr_char);
    epc_parser_t * cr_lb = epc_char(l, "CRLB", '[');
    epc_parser_set_ast_action(cr_lb, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * cr_rb = epc_char(l, "CRRB", ']');
    epc_parser_set_ast_action(cr_rb, TOKENIZER_ACTION_STRUCTURAL);
    epc_parser_t * char_range = epc_and(l, "CharRange", 3, cr_lb, cr_body, cr_rb);

    // --- Combined token alternatives ---
    // Order matters: compound tokens before identifiers, since identifiers overlap
    epc_parser_t * raw_token = epc_or(
        l,
        "RawToken",
        18,
        string_literal,
        char_literal,
        token_literal,
        char_range,
        number,
        eq,
        semicolon,
        pipe,
        at_sign,
        lparen,
        rparen,
        comma,
        minus,
        star,
        plus,
        question,
        caret,
        identifier
    );

    // Wrap in lexeme to consume whitespace/comments between tokens
    epc_parser_t * token = epc_lexeme_ex(l, "Token", raw_token, EPC_CONSUME_ALL_STYLES);

    // Program: many(Token) eoi
    epc_parser_t * many_tokens = epc_many(l, "ManyTokens", token);
    epc_parser_t * eoi = epc_eoi(l, "EOI");
    epc_parser_t * program = epc_and(l, "Program", 2, many_tokens, eoi);

    return program;
}
