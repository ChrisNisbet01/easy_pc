#include "json_grammar.h"

#include "semantic_actions.h"

// Building blocks
static epc_parser_t *
json_escaped_char(epc_parser_list * list)
{
    // Escaped characters: \" \\ \/ \b \f \n \r \t \uXXXX
    epc_parser_t * simple_escape = epc_and(
        list,
        "simple_escape",
        2,
        epc_char(list, "backslash", '\\'),
        epc_one_of(list, "simple_escape_char", "\"\\/bfnrt")
    );

    epc_parser_t * hex_digit_parser = epc_hex_digit(list, "hex_digit");

    epc_parser_t * unicode_escape = epc_and(
        list,
        "unicode_escape",
        2,
        epc_string(list, "unicode_prefix", "\\u"),
        epc_count(list, "hex_digits", 4, hex_digit_parser)
    );

    return epc_or(list, "escaped_char", 2, simple_escape, unicode_escape);
}

static epc_parser_t *
json_quoted_string(epc_parser_list * list)
{
    epc_parser_t * string_content_char = epc_or(
        list, "string_content_char", 2, epc_none_of(list, "non_quote_non_backslash", "\"\\"), json_escaped_char(list)
    );

    epc_parser_t * inner_string = epc_many(list, "inner_string", string_content_char);

    return epc_between(
        list,
        "quoted_string",
        epc_char(list, "opening_quote", '\"'),
        inner_string,
        epc_char(list, "closing_quote", '\"')
    );
}

static epc_parser_t *
json_null(epc_parser_list * list)
{
    return epc_string(list, "null", "null");
}

static epc_parser_t *
json_boolean(epc_parser_list * list)
{
    return epc_or(list, "boolean", 2, epc_string(list, "true", "true"), epc_string(list, "false", "false"));
}

static epc_parser_t *
json_number(epc_parser_list * list)
{
    return epc_double(list, "number");
}

// Forward references (allocated here, duplicated later)
static epc_parser_t *
json_value_parser_ref(epc_parser_list * list)
{
    return epc_parser_fwd_decl(list, "json_value_ref");
}

static epc_parser_t *
json_array_parser_ref(epc_parser_list * list)
{
    return epc_parser_fwd_decl(list, "json_array_ref");
}

static epc_parser_t *
json_object_parser_ref(epc_parser_list * list)
{
    return epc_parser_fwd_decl(list, "json_object_ref");
}

epc_parser_t *
create_json_grammar(epc_parser_list * list)
{
    // 1. Allocate the forward references first
    epc_parser_t * value_ref = json_value_parser_ref(list);
    epc_parser_t * array_ref = json_array_parser_ref(list);
    epc_parser_t * object_ref = json_object_parser_ref(list);

    // Get common building block parsers
    epc_parser_t * quoted_string_p = json_quoted_string(list);
    epc_parser_set_ast_action(quoted_string_p, JSON_ACTION_CREATE_STRING);

    epc_parser_t * null_p = json_null(list);
    epc_parser_set_ast_action(null_p, JSON_ACTION_CREATE_NULL);

    epc_parser_t * boolean_p = json_boolean(list);
    epc_parser_set_ast_action(boolean_p, JSON_ACTION_CREATE_BOOLEAN);

    epc_parser_t * number_p = json_number(list);
    epc_parser_set_ast_action(number_p, JSON_ACTION_CREATE_NUMBER);

    // --- Define Actual JSON Grammar Components using the refs ---

    // json_value
    epc_parser_t * json_value_actual = epc_strip(
        list,
        "json_value",
        epc_or(list, "value_choice", 6, quoted_string_p, number_p, boolean_p, null_p, object_ref, array_ref)
    );

    // json_array
    epc_parser_t * comma_strip = epc_strip(list, "comma_strip", epc_char(list, "comma", ','));
    epc_parser_t * open_bracket_strip = epc_strip(list, "open_bracket_strip", epc_char(list, "open_bracket", '['));
    epc_parser_t * close_bracket_strip = epc_strip(list, "close_bracket_strip", epc_char(list, "close_bracket", ']'));

    epc_parser_t * array_elements = epc_delimited(list, "array_elements", value_ref, comma_strip); // Use value_ref
    epc_parser_set_ast_action(array_elements, JSON_ACTION_CREATE_ARRAY_ELEMENTS);

    epc_parser_t * optional_array_elements = epc_optional(list, "optional_elements_in_array", array_elements);
    epc_parser_set_ast_action(optional_array_elements, JSON_ACTION_CREATE_OPTIONAL_ARRAY_ELEMENTS);

    epc_parser_t * json_array_actual
        = epc_and(list, "json_array_parser", 3, open_bracket_strip, optional_array_elements, close_bracket_strip);
    epc_parser_set_ast_action(json_array_actual, JSON_ACTION_CREATE_ARRAY);

    // json_object
    epc_parser_t * colon_strip = epc_strip(list, "colon_strip", epc_char(list, "colon", ':'));
    epc_parser_t * open_brace_strip = epc_strip(list, "open_brace_strip", epc_char(list, "open_brace", '{'));
    epc_parser_t * close_brace_strip = epc_strip(list, "close_brace_strip", epc_char(list, "close_brace", '}'));

    epc_parser_t * member = epc_and(
        list,
        "member",
        3,
        quoted_string_p, // Key
        colon_strip,
        value_ref // Value (Use value_ref)
    );
    epc_parser_set_ast_action(member, JSON_ACTION_CREATE_MEMBER);

    epc_parser_t * object_elements = epc_delimited(list, "object_members", member, comma_strip);
    epc_parser_set_ast_action(object_elements, JSON_ACTION_CREATE_OBJECT_ELEMENTS);

    epc_parser_t * optional_object_elements = epc_optional(list, "optional_elements_in_object", object_elements);
    epc_parser_set_ast_action(optional_object_elements, JSON_ACTION_CREATE_OPTIONAL_OBJECT_ELEMENTS);

    epc_parser_t * json_object_actual
        = epc_and(list, "json_object_parser", 3, open_brace_strip, optional_object_elements, close_brace_strip);
    epc_parser_set_ast_action(json_object_actual, JSON_ACTION_CREATE_OBJECT);

    // 3. Resolve forward references using epc_parser_duplicate
    epc_parser_duplicate(value_ref, json_value_actual);
    epc_parser_duplicate(array_ref, json_array_actual);
    epc_parser_duplicate(object_ref, json_object_actual);

    // The top-level parser for a JSON document is a json_value followed by EOI
    epc_parser_t * top = epc_and(list, "json_document", 2, json_value_actual, epc_eoi(list, "end_of_input"));

    return top;
}