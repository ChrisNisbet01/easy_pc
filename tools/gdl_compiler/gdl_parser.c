#include "gdl_ast.h"
#include "gdl_token_ids.h"

#include <easy_pc/easy_pc.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

epc_parser_t *
create_gdl_parser(epc_parser_list * l)
{
    epc_parser_t * gdl_definition_expression = epc_parser_fwd_decl(l, "DefinitionExpression");

    // --- Token terminals (all matched by tokenizer, consumed as single tokens) ---

    // Identifier
    epc_parser_t * gdl_identifier = epc_token(l, "Identifier", TOKEN_IDENTIFIER);
    epc_parser_set_ast_action(gdl_identifier, GDL_AST_ACTION_CREATE_IDENTIFIER_REF);

    // Literals
    epc_parser_t * gdl_string_literal = epc_token(l, "StringLiteral", TOKEN_STRING_LITERAL);
    epc_parser_set_ast_action(gdl_string_literal, GDL_AST_ACTION_CREATE_STRING_LITERAL);

    epc_parser_t * gdl_char_literal = epc_token(l, "CharLiteral", TOKEN_CHAR_LITERAL);
    epc_parser_set_ast_action(gdl_char_literal, GDL_AST_ACTION_CREATE_CHAR_LITERAL);

    epc_parser_t * gdl_raw_char_literal = epc_token(l, "RawCharLiteral", TOKEN_RAW_CHAR_LITERAL);
    epc_parser_set_ast_action(gdl_raw_char_literal, GDL_AST_ACTION_CREATE_RAW_CHAR_LITERAL);

    epc_parser_t * gdl_token_literal = epc_token(l, "TokenLiteral", TOKEN_TOKEN_LITERAL);
    epc_parser_set_ast_action(gdl_token_literal, GDL_AST_ACTION_CREATE_TOKEN_LITERAL);

    epc_parser_t * gdl_number_literal = epc_token(l, "NumberLiteral", TOKEN_NUMBER);
    epc_parser_set_ast_action(gdl_number_literal, GDL_AST_ACTION_CREATE_NUMBER_LITERAL);

    // --- Structural tokens ---
    epc_parser_t * gdl_equals = epc_token(l, "Equals", TOKEN_EQUALS);
    epc_parser_t * gdl_semicolon = epc_token(l, "Semicolon", TOKEN_SEMICOLON);
    epc_parser_t * gdl_pipe = epc_token(l, "Pipe", TOKEN_PIPE);
    epc_parser_t * gdl_at = epc_token(l, "AtSign", TOKEN_AT);
    epc_parser_t * gdl_lparen = epc_token(l, "LParen", TOKEN_LPAREN);
    epc_parser_t * gdl_rparen = epc_token(l, "RParen", TOKEN_RPAREN);
    epc_parser_t * gdl_comma = epc_token(l, "Comma", TOKEN_COMMA);
    epc_parser_t * gdl_star = epc_token(l, "Star", TOKEN_STAR);
    epc_parser_t * gdl_plus = epc_token(l, "Plus", TOKEN_PLUS);
    epc_parser_t * gdl_minus = epc_token(l, "Minus", TOKEN_MINUS);
    epc_parser_t * gdl_question = epc_token(l, "Question", TOKEN_QUESTION);

    epc_parser_t * gdl_char_range = epc_and(
        l,
        "CharRange",
        5,
        epc_token(l, "LBracket", TOKEN_LBRACKET),
        gdl_raw_char_literal,
        gdl_minus,
        gdl_raw_char_literal,
        epc_token(l, "RBracket", TOKEN_RBRACKET)
    );
    epc_parser_set_ast_action(gdl_char_range, GDL_AST_ACTION_CREATE_CHAR_RANGE);

    // --- Keyword terminals ---

    // Terminal parser keywords
    epc_parser_t * p_char = epc_token(l, "KW_char", TOKEN_KW_CHAR);
    epc_parser_t * p_digit = epc_token(l, "KW_digit", TOKEN_KW_DIGIT);
    epc_parser_t * p_alphanum = epc_token(l, "KW_alphanum", TOKEN_KW_ALPHANUM);
    epc_parser_t * p_alpha = epc_token(l, "KW_alpha", TOKEN_KW_ALPHA);
    epc_parser_t * p_identifier = epc_token(l, "KW_identifier", TOKEN_KW_IDENTIFIER);
    epc_parser_t * p_int = epc_token(l, "KW_int", TOKEN_KW_INT);
    epc_parser_t * p_octal = epc_token(l, "KW_octal", TOKEN_KW_OCTAL);
    epc_parser_t * p_hex = epc_token(l, "KW_hex", TOKEN_KW_HEX);
    epc_parser_t * p_double_kw = epc_token(l, "KW_double", TOKEN_KW_DOUBLE);
    epc_parser_t * p_long_double = epc_token(l, "KW_long_double", TOKEN_KW_LONG_DOUBLE);
    epc_parser_t * p_space = epc_token(l, "KW_space", TOKEN_KW_SPACE);
    epc_parser_t * p_any = epc_token(l, "KW_any", TOKEN_KW_ANY);
    epc_parser_t * p_succeed = epc_token(l, "KW_succeed", TOKEN_KW_SUCCEED);
    epc_parser_t * p_hex_digit = epc_token(l, "KW_hex_digit", TOKEN_KW_HEX_DIGIT);
    epc_parser_t * p_soi = epc_token(l, "KW_soi", TOKEN_KW_SOI);
    epc_parser_t * p_eoi = epc_token(l, "KW_eoi", TOKEN_KW_EOI);
    epc_parser_t * p_fail_kw = epc_token(l, "KW_fail", TOKEN_KW_FAIL);
    epc_parser_t * p_cpp_comment = epc_token(l, "KW_cpp_comment", TOKEN_KW_CPP_COMMENT);
    epc_parser_t * p_c_comment = epc_token(l, "KW_c_comment", TOKEN_KW_C_COMMENT);
    epc_parser_t * p_bash_comment = epc_token(l, "KW_bash_comment", TOKEN_KW_BASH_COMMENT);

    /* Combinator keywords. */
    epc_parser_t * p_string = epc_token(l, "KW_string", TOKEN_KW_STRING);
    epc_parser_t * p_char_range_kw = epc_token(l, "KW_char_range", TOKEN_KW_CHAR_RANGE);
    epc_parser_t * p_none_of = epc_token(l, "KW_noneof", TOKEN_KW_NONEOF);
    epc_parser_t * p_many = epc_token(l, "KW_many", TOKEN_KW_MANY);
    epc_parser_t * p_count = epc_token(l, "KW_count", TOKEN_KW_COUNT);
    epc_parser_t * p_count_range = epc_token(l, "KW_count_range", TOKEN_KW_COUNT_RANGE);
    epc_parser_t * p_between = epc_token(l, "KW_between", TOKEN_KW_BETWEEN);
    epc_parser_t * p_delimited = epc_token(l, "KW_delimited", TOKEN_KW_DELIMITED);
    epc_parser_t * p_delimited_flex = epc_token(l, "KW_delimited_flex", TOKEN_KW_DELIMITED_FLEX);
    epc_parser_t * p_optional = epc_token(l, "KW_optional", TOKEN_KW_OPTIONAL);
    epc_parser_t * p_lookahead = epc_token(l, "KW_lookahead", TOKEN_KW_LOOKAHEAD);
    epc_parser_t * p_not = epc_token(l, "KW_not", TOKEN_KW_NOT);
    epc_parser_t * p_one_of = epc_token(l, "KW_oneof", TOKEN_KW_ONEOF);
    epc_parser_t * p_lexeme = epc_token(l, "KW_lexeme", TOKEN_KW_LEXEME);
    epc_parser_t * p_strip = epc_token(l, "KW_strip", TOKEN_KW_STRIP);
    epc_parser_t * p_stripl = epc_token(l, "KW_stripl", TOKEN_KW_STRIPL);
    epc_parser_t * p_stripr = epc_token(l, "KW_stripr", TOKEN_KW_STRIPR);
    epc_parser_t * p_chainl1 = epc_token(l, "KW_chainl1", TOKEN_KW_CHAINL1);
    epc_parser_t * p_chainr1 = epc_token(l, "KW_chainr1", TOKEN_KW_CHAINR1);
    epc_parser_t * p_skip = epc_token(l, "KW_skip", TOKEN_KW_SKIP);
    epc_parser_t * p_memoize = epc_token(l, "KW_memoize", TOKEN_KW_MEMOIZE);
    epc_parser_t * p_satisfy = epc_token(l, "KW_satisfy", TOKEN_KW_SATISFY);
    epc_parser_t * p_wrap = epc_token(l, "KW_wrap", TOKEN_KW_WRAP);
    epc_parser_t * p_commit = epc_token(l, "KW_commit", TOKEN_KW_COMMIT);

    /* Lexeme flags. */
    epc_parser_t * lexeme_flag_ws = epc_token(l, "KW_ws", TOKEN_KW_WS);
    epc_parser_t * lexeme_flag_c_comment = epc_token(l, "KW_c_comment", TOKEN_KW_C_COMMENT);
    epc_parser_t * lexeme_flag_cpp_comment = epc_token(l, "KW_cpp_comment", TOKEN_KW_CPP_COMMENT);
    epc_parser_t * lexeme_flag_bash_comment = epc_token(l, "KW_bash_comment", TOKEN_KW_BASH_COMMENT);
    epc_parser_t * lexeme_flag_all_comments = epc_token(l, "KW_all_comments", TOKEN_KW_ALL_COMMENTS);
    epc_parser_t * lexeme_flag_all = epc_token(l, "KW_all", TOKEN_KW_ALL);
    epc_parser_t * lexeme_flag_all_styles = epc_token(l, "KW_all_styles", TOKEN_KW_ALL_STYLES);

    // --- Keyword grouping ---

    epc_parser_t * terminal_no_arg_parser = epc_or(
        l,
        "TerminalNoArgKeyword",
        18,
        p_char,
        p_digit,
        p_alphanum,
        p_alpha,
        p_identifier,
        p_int,
        p_octal,
        p_hex,
        p_double_kw,
        p_long_double,
        p_space,
        p_any,
        p_succeed,
        p_hex_digit,
        p_soi,
        p_eoi,
        p_cpp_comment,
        p_c_comment,
        p_bash_comment
    );
    epc_parser_set_ast_action(terminal_no_arg_parser, GDL_AST_ACTION_CREATE_KEYWORD);

    epc_parser_t * terminal_with_arg_parser = epc_or(l, "TerminalWithArgKeyword", 1, p_fail_kw);
    epc_parser_set_ast_action(terminal_with_arg_parser, GDL_AST_ACTION_CREATE_KEYWORD);

    epc_parser_t * terminal_keyword = terminal_no_arg_parser;

    epc_parser_t * combinator_parser = epc_or(
        l,
        "CombinatorKeyword",
        24,
        p_string,
        p_char_range_kw,
        p_none_of,
        p_many,
        p_count,
        p_count_range,
        p_between,
        p_delimited,
        p_delimited_flex,
        p_optional,
        p_lookahead,
        p_not,
        p_one_of,
        p_lexeme,
        p_strip,
        p_stripl,
        p_stripr,
        p_chainl1,
        p_chainr1,
        p_skip,
        p_memoize,
        p_satisfy,
        p_wrap,
        p_commit
    );
    epc_parser_set_ast_action(combinator_parser, GDL_AST_ACTION_CREATE_KEYWORD);

    // --- FailCall: fail(<string_literal>) ---
    epc_parser_t * fail_call = epc_and(l, "FailCall", 4, p_fail_kw, gdl_lparen, gdl_string_literal, gdl_rparen);
    epc_parser_set_ast_action(fail_call, GDL_AST_ACTION_CREATE_FAIL_CALL);

    // --- Terminal ---
    epc_parser_t * gdl_terminal = epc_or(
        l,
        "Terminal",
        7,
        gdl_string_literal,
        gdl_char_literal,
        gdl_token_literal,
        terminal_keyword,
        fail_call,
        gdl_identifier,
        gdl_number_literal
    );
    epc_parser_set_ast_action(gdl_terminal, GDL_AST_ACTION_CREATE_TERMINAL);

    // --- Expression arg forward declaration ---
    epc_parser_t * gdl_expression_arg = epc_parser_fwd_decl(l, "ExpressionArgFwd");

    // --- Combinator calls ---

    // noneof_call: noneof '(' string_literal ')'
    epc_parser_t * none_of_call = epc_and(l, "NoneofCall", 4, p_none_of, gdl_lparen, gdl_string_literal, gdl_rparen);
    epc_parser_set_ast_action(none_of_call, GDL_AST_ACTION_CREATE_NONEOF_CALL);

    // count_call: count '(' number_literal ',' definition_expression ')'
    epc_parser_t * count_args = epc_and(l, "CountArgs", 3, gdl_number_literal, gdl_comma, gdl_definition_expression);
    epc_parser_t * count_call = epc_and(l, "CountCall", 4, p_count, gdl_lparen, count_args, gdl_rparen);
    epc_parser_set_ast_action(count_call, GDL_AST_ACTION_CREATE_COUNT_CALL);

    // count_range_call: count_range '(' number_literal ',' number_literal ',' definition_expression ')'
    epc_parser_t * count_range_args = epc_and(
        l, "CountRangeArgs", 5, gdl_number_literal, gdl_comma, gdl_number_literal, gdl_comma, gdl_definition_expression
    );
    epc_parser_t * count_range_call
        = epc_and(l, "CountRangeCall", 4, p_count_range, gdl_lparen, count_range_args, gdl_rparen);
    epc_parser_set_ast_action(count_range_call, GDL_AST_ACTION_CREATE_COUNT_RANGE_CALL);

    // between_call: between '(' expression ',' expression ',' expression ')'
    epc_parser_t * between_args = epc_and(
        l, "BetweenArgs", 5, gdl_expression_arg, gdl_comma, gdl_expression_arg, gdl_comma, gdl_expression_arg
    );
    epc_parser_t * between_call = epc_and(l, "BetweenCall", 4, p_between, gdl_lparen, between_args, gdl_rparen);
    epc_parser_set_ast_action(between_call, GDL_AST_ACTION_CREATE_BETWEEN_CALL);

    // delimited_call: delimited '(' expression ',' expression ')'
    epc_parser_t * delimited_args = epc_and(l, "DelimitedArgs", 3, gdl_expression_arg, gdl_comma, gdl_expression_arg);
    epc_parser_t * delimited_call = epc_and(l, "DelimitedCall", 4, p_delimited, gdl_lparen, delimited_args, gdl_rparen);
    epc_parser_set_ast_action(delimited_call, GDL_AST_ACTION_CREATE_DELIMITED_CALL);

    // delimited_flex_call: delimited_flex '(' expression ',' expression ')'
    epc_parser_t * delimited_flex_call
        = epc_and(l, "DelimitedFlexCall", 4, p_delimited_flex, gdl_lparen, delimited_args, gdl_rparen);
    epc_parser_set_ast_action(delimited_flex_call, GDL_AST_ACTION_CREATE_DELIMITED_FLEX_CALL);

    // lookahead_call: lookahead '(' expression ')'
    epc_parser_t * lookahead_call
        = epc_and(l, "LookaheadCall", 4, p_lookahead, gdl_lparen, gdl_expression_arg, gdl_rparen);
    epc_parser_set_ast_action(lookahead_call, GDL_AST_ACTION_CREATE_LOOKAHEAD_CALL);

    // not_call: not '(' expression ')'
    epc_parser_t * not_call = epc_and(l, "NotCall", 4, p_not, gdl_lparen, gdl_expression_arg, gdl_rparen);
    epc_parser_set_ast_action(not_call, GDL_AST_ACTION_CREATE_NOT_CALL);

    // oneof_call: oneof '(' string_literal ')'
    epc_parser_t * one_of_call = epc_and(l, "OneofCall", 4, p_one_of, gdl_lparen, gdl_string_literal, gdl_rparen);
    epc_parser_set_ast_action(one_of_call, GDL_AST_ACTION_CREATE_ONEOF_CALL);

    // lexeme/strip flags
    epc_parser_t * lexeme_flag = epc_or(
        l,
        "LexemeFlag",
        7,
        lexeme_flag_ws,
        lexeme_flag_c_comment,
        lexeme_flag_cpp_comment,
        lexeme_flag_bash_comment,
        lexeme_flag_all_comments,
        lexeme_flag_all_styles,
        lexeme_flag_all
    );
    epc_parser_set_ast_action(lexeme_flag, GDL_AST_ACTION_CREATE_KEYWORD);

    // lexeme_call: lexeme '(' expression (',' flag)* ')'
    epc_parser_t * extra_lexeme_arg = epc_and(l, "ExtraLexemeArg", 2, gdl_comma, lexeme_flag);
    epc_parser_t * extra_lexeme_args = epc_many(l, "ExtraLexemeArgs", extra_lexeme_arg);
    epc_parser_set_ast_action(extra_lexeme_args, GDL_AST_ACTION_CREATE_SEQUENCE);
    epc_parser_t * lexeme_args = epc_and(l, "LexemeArgs", 2, gdl_expression_arg, extra_lexeme_args);
    epc_parser_t * lexeme_call = epc_and(l, "LexemeCall", 4, p_lexeme, gdl_lparen, lexeme_args, gdl_rparen);
    epc_parser_set_ast_action(lexeme_call, GDL_AST_ACTION_CREATE_LEXEME_CALL);

    // strip variants
    epc_parser_t * strip_call = epc_and(l, "StripCall", 4, p_strip, gdl_lparen, lexeme_args, gdl_rparen);
    epc_parser_set_ast_action(strip_call, GDL_AST_ACTION_CREATE_STRIP_CALL);

    epc_parser_t * stripl_call = epc_and(l, "StriplCall", 4, p_stripl, gdl_lparen, lexeme_args, gdl_rparen);
    epc_parser_set_ast_action(stripl_call, GDL_AST_ACTION_CREATE_STRIPL_CALL);

    epc_parser_t * stripr_call = epc_and(l, "StriprCall", 4, p_stripr, gdl_lparen, lexeme_args, gdl_rparen);
    epc_parser_set_ast_action(stripr_call, GDL_AST_ACTION_CREATE_STRIPR_CALL);

    // chain calls
    epc_parser_t * chain_args = epc_and(l, "ChainArgs", 3, gdl_expression_arg, gdl_comma, gdl_expression_arg);

    epc_parser_t * chainl1_call = epc_and(l, "ChainL1Call", 4, p_chainl1, gdl_lparen, chain_args, gdl_rparen);
    epc_parser_set_ast_action(chainl1_call, GDL_AST_ACTION_CREATE_CHAINL1_CALL);

    epc_parser_t * chainr1_call = epc_and(l, "ChainR1Call", 4, p_chainr1, gdl_lparen, chain_args, gdl_rparen);
    epc_parser_set_ast_action(chainr1_call, GDL_AST_ACTION_CREATE_CHAINR1_CALL);

    // skip_call: skip '(' expression ')'
    epc_parser_t * skip_call = epc_and(l, "SkipCall", 4, p_skip, gdl_lparen, gdl_expression_arg, gdl_rparen);
    epc_parser_set_ast_action(skip_call, GDL_AST_ACTION_CREATE_SKIP_CALL);

    // memoize_call: memoize '(' expression ')'
    epc_parser_t * memoize_call = epc_and(l, "MemoizeCall", 4, p_memoize, gdl_lparen, gdl_expression_arg, gdl_rparen);
    epc_parser_set_ast_action(memoize_call, GDL_AST_ACTION_CREATE_MEMOIZE_CALL);

    // satisfy_call: satisfy '(' expression ',' string_literal ',' identifier ',' identifier ')'
    epc_parser_t * satisfy_args = epc_and(
        l,
        "SatisfyArgs",
        7,
        gdl_expression_arg,
        gdl_comma,
        gdl_string_literal,
        gdl_comma,
        gdl_identifier,
        gdl_comma,
        gdl_identifier
    );
    epc_parser_t * satisfy_call = epc_and(l, "SatisfyCall", 4, p_satisfy, gdl_lparen, satisfy_args, gdl_rparen);
    epc_parser_set_ast_action(satisfy_call, GDL_AST_ACTION_CREATE_SATISFY_CALL);

    // wrap_call: wrap '(' expression ',' identifier ',' identifier ')'
    epc_parser_t * wrap_args
        = epc_and(l, "WrapArgs", 5, gdl_expression_arg, gdl_comma, gdl_identifier, gdl_comma, gdl_identifier);
    epc_parser_t * wrap_call = epc_and(l, "WrapCall", 4, p_wrap, gdl_lparen, wrap_args, gdl_rparen);
    epc_parser_set_ast_action(wrap_call, GDL_AST_ACTION_CREATE_WRAP_CALL);

    // commit_call: commit '(' expression ')'
    epc_parser_t * commit_call = epc_and(l, "CommitCall", 4, p_commit, gdl_lparen, gdl_expression_arg, gdl_rparen);
    epc_parser_set_ast_action(commit_call, GDL_AST_ACTION_CREATE_COMMIT_CALL);

    // --- All combinator calls combined ---
    epc_parser_t * gdl_combinator_call = epc_or(
        l,
        "CombinatorCall",
        21,
        none_of_call,
        count_call,
        count_range_call,
        between_call,
        delimited_call,
        delimited_flex_call,
        lookahead_call,
        not_call,
        fail_call,
        one_of_call,
        lexeme_call,
        strip_call,
        stripl_call,
        stripr_call,
        chainl1_call,
        chainr1_call,
        skip_call,
        memoize_call,
        satisfy_call,
        wrap_call,
        commit_call
    );

    // --- Expression grammar ---

    // PrimaryExpression: Terminal | CharRange | CombinatorCall | '(' definition_expression ')'
    epc_parser_t * gdl_parenthesized_expression
        = epc_and(l, "ParenthesizedExpression", 3, gdl_lparen, gdl_definition_expression, gdl_rparen);

    epc_parser_t * gdl_primary_expression = epc_or(
        l,
        "PrimaryExpression",
        5,
        gdl_combinator_call,
        gdl_terminal,
        gdl_char_range,
        gdl_parenthesized_expression,
        gdl_number_literal
    );

    // ExpressionFactor: primary_expression repetition_operator?
    epc_parser_t * gdl_repetition_operator = epc_or(l, "RepetitionOperator", 3, gdl_star, gdl_plus, gdl_question);
    epc_parser_set_ast_action(gdl_repetition_operator, GDL_AST_ACTION_CREATE_REPETITION_OPERATOR);

    epc_parser_t * gdl_optional_repetition = epc_optional(l, "OptionalRepetition", gdl_repetition_operator);
    epc_parser_set_ast_action(gdl_optional_repetition, GDL_AST_ACTION_CREATE_OPTIONAL);

    epc_parser_t * gdl_expression_factor
        = epc_and(l, "ExpressionFactor", 2, gdl_primary_expression, gdl_optional_repetition);
    epc_parser_set_ast_action(gdl_expression_factor, GDL_AST_ACTION_CREATE_EXPRESSION_FACTOR);

    // ExpressionTerm: expression_factor+
    epc_parser_t * gdl_expression_term = epc_plus(l, "ExpressionTerm", gdl_expression_factor);
    epc_parser_set_ast_action(gdl_expression_term, GDL_AST_ACTION_CREATE_SEQUENCE);

    // DefinitionExpression: expression_term ('|' expression_term)*
    epc_parser_t * gdl_alternative_part = epc_and(l, "AlternativePart", 2, gdl_pipe, gdl_expression_term);
    epc_parser_t * gdl_many_alternatives = epc_many(l, "ManyAlternatives", gdl_alternative_part);

    epc_parser_t * temp_definition_expression
        = epc_and(l, "DefinitionExpression", 2, gdl_expression_term, gdl_many_alternatives);
    epc_parser_set_ast_action(temp_definition_expression, GDL_AST_ACTION_CREATE_ALTERNATIVE);

    // Resolve forward references
    epc_parser_duplicate(gdl_definition_expression, temp_definition_expression);
    epc_parser_duplicate(gdl_expression_arg, gdl_definition_expression);

    // --- Semantic action: '@' identifier ---
    epc_parser_t * gdl_semantic_action = epc_and(l, "SemanticAction", 2, gdl_at, gdl_identifier);
    epc_parser_set_ast_action(gdl_semantic_action, GDL_AST_ACTION_CREATE_SEMANTIC_ACTION);

    epc_parser_t * gdl_optional_semantic_action = epc_optional(l, "OptionalSemanticAction", gdl_semantic_action);
    epc_parser_set_ast_action(gdl_optional_semantic_action, GDL_AST_ACTION_CREATE_OPTIONAL_SEMANTIC_ACTION);

    // --- Caret marker for commit boundary: '^' ---
    epc_parser_t * gdl_caret_marker = epc_token(l, "CaretMarker", TOKEN_CARET);
    epc_parser_set_ast_action(gdl_caret_marker, GDL_AST_ACTION_CREATE_CARET_BOUNDARY);
    epc_parser_t * gdl_optional_caret = epc_optional(l, "OptionalCaret", gdl_caret_marker);

    // --- RuleDefinition: '^'? identifier '=' definition_expression semantic_action? ';' ---
    epc_parser_t * gdl_rule_definition = epc_and(
        l,
        "RuleDefinition",
        6,
        gdl_optional_caret,
        gdl_identifier,
        gdl_equals,
        gdl_definition_expression,
        gdl_optional_semantic_action,
        gdl_semicolon
    );
    epc_parser_set_ast_action(gdl_rule_definition, GDL_AST_ACTION_CREATE_RULE_DEFINITION);

    // --- Program: rule_definition+ eoi ---
    epc_parser_t * gdl_many_rule_definitions = epc_plus(l, "ManyRuleDefinitions", gdl_rule_definition);
    epc_parser_set_ast_action(gdl_many_rule_definitions, GDL_AST_ACTION_CREATE_SEQUENCE);

    epc_parser_t * gdl_eoi_parser = epc_eoi(l, "EOI");

    epc_parser_t * gdl_program = epc_and(l, "Program", 2, gdl_many_rule_definitions, gdl_eoi_parser);
    epc_parser_set_ast_action(gdl_program, GDL_AST_ACTION_CREATE_PROGRAM);

    return gdl_program;
}
