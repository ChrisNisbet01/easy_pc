#pragma once

#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>

// For symbol visibility control
#if defined _WIN32 || defined __CYGWIN__
#ifdef BUILDING_EASY_PC
#ifdef __GNUC__
#define EASY_PC_API __attribute__((dllexport))
#else
#define EASY_PC_API __declspec(dllexport)
#endif
#else
#ifdef __GNUC__
#define EASY_PC_API __attribute__((dllimport))
#else
#define EASY_PC_API __declspec(dllimport)
#endif
#endif
#define EASY_PC_HIDDEN
#else
#if __GNUC__ >= 4
#define EASY_PC_API __attribute__((visibility("default")))
#define EASY_PC_HIDDEN __attribute__((visibility("hidden")))
#else
#define EASY_PC_API
#define EASY_PC_HIDDEN
#endif
#endif

#ifdef __GNUC__
#define ATTR_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))
#else
#define ATTR_NONNULL(...)
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef enum
{
    EPC_ERROR_MESSAGE_MAX_LEN = 60,
    EPC_ERROR_FOUND_MAX_LEN = 40,
    EPC_ERROR_EXPECTED_MAX_LEN = 40,
} EPC_ERROR_MAX_MESSAGE_LENGTHS;

// Forward declarations of structs
typedef struct epc_parser_t epc_parser_t;
typedef struct epc_cpt_node_t epc_cpt_node_t;
typedef struct epc_parser_ctx_t epc_parser_ctx_t;
typedef struct epc_parser_list epc_parser_list;
typedef struct epc_parse_session_t epc_parse_session_t;
typedef struct epc_parse_result_t epc_parse_result_t;
// line and column information.
typedef struct epc_line_col_t
{
    size_t line;
    size_t col;
} epc_line_col_t;

/**
 * @brief Represents the input to be parsed, which can be either a string or a file stream.
 */
typedef enum epc_parse_type_t
{
    EPC_PARSE_TYPE_STRING,
    EPC_PARSE_TYPE_FILE,
    EPC_PARSE_TYPE_FILENAME,
    EPC_PARSE_TYPE_FD,
    EPC_PARSE_TYPE_FD_REACTIVE,
    EPC_PARSE_TYPE_BUFFER,
} epc_parse_type_t;

#ifdef WITH_INPUT_STREAM_SUPPORT
/**
 * @brief Callback function type invoked when a reactive streaming parse completes.
 * @param user_data User-defined data passed to the initiation call.
 */
typedef void (*epc_streaming_complete_cb)(void * user_data);
#endif

/**
 * @brief A union type that encapsulates the different forms of input that can be parsed.
 *
 * The `type` field indicates which member of the union is valid. For `EPC_PARSE_TYPE_STRING`,
 * the `input_string` member should be used, and for `EPC_PARSE_TYPE_FILE`, the `fp` member
 * should be used. This structure allows the parsing API to accept multiple input types in a
 * flexible manner.
 */
typedef struct epc_parse_input_t
{
    epc_parse_type_t type;
    union
    {
        char const * input_string;
        FILE * fp;
        char const * filename;
        int fd;
#ifdef WITH_INPUT_STREAM_SUPPORT
        struct
        {
            int fd;
            epc_streaming_complete_cb on_complete;
            void * cb_user_data;
        } reactive;
#endif
        struct
        {
            char const * buf;
            size_t len;
        } buffer;
    };
} epc_parse_input_t;

// Error Handling struct
/**
 * @brief Detailed error information from a parsing attempt.
 */
typedef struct
{
    char const * input_position; /**< @brief Pointer to the exact position in the input where the error occurred. */
    epc_line_col_t position;     /**< @brief Line and column coordinates of the error position. */
    /**< @brief A descriptive message explaining the nature of the error. */
    char message[EPC_ERROR_MESSAGE_MAX_LEN + 1];
    /**< @brief A string describing what the parser was expecting at the error position. */
    char expected[EPC_ERROR_EXPECTED_MAX_LEN + 1];
    /**< @brief A string describing what the parser actually found at the error position. */
    char found[EPC_ERROR_FOUND_MAX_LEN + 1];
    epc_parser_ctx_t *
        internal_parse_ctx; /**< @brief Internal context for the parsing operation, managing CPT/error memory. */
} epc_parser_error_t;

// Structure to hold AST-related metadata for each parser
/**
 * @brief Configuration for semantic actions to be performed during AST generation.
 *
 * This structure is associated with each parser and dictates how the parser's
 * result should contribute to the Abstract Syntax Tree (AST).
 * The `action` field specifies a particular semantic action to take.
 */
typedef struct
{
    bool assigned; /**< @brief True if a semantic action identifier has been assigned to the node. */
    int action;    /**< @brief The identifier for the semantic action to perform.
                    *          Concrete values for actions are defined elsewhere (e.g., in ast_builder.h).
                    */
} epc_ast_semantic_action_t;

// The Result of a Parse Attempt
struct epc_parse_result_t
{
    bool is_error; /**< @brief A flag: false for success, true for error. */
    union
    {
        epc_cpt_node_t * success;   /**< @brief Pointer to the root of the generated CPT on successful parsing. */
        epc_parser_error_t * error; /**< @brief Pointer to detailed error information on parsing failure. */
    } data;                         /**< @brief Union holding either the success node or error details. */
};

// --- Parse Session Result ---
// Contains the parse result and the transient context for cleanup
/**
 * @brief Represents the complete result and context of a parsing session.
 *
 * This structure holds the final outcome of a top-level parsing operation
 * (either a successful CPT or an error) along with the internal parser context
 * necessary for proper memory cleanup. Users should always destroy a session
 * with `epc_parse_session_destroy` to release all associated resources.
 */
struct epc_parse_session_t
{
    epc_parse_result_t result;             /**< @brief The final result of the parse (CPT root or error details). */
    epc_parser_ctx_t * internal_parse_ctx; /**< @brief Pointer to the internal context for memory management. */
};

// Visitor struct for CPT traversal
/**
 * @brief Structure for defining a visitor pattern to traverse the Concrete Parse Tree (CPT).
 *
 * A `pt_visitor_t` allows defining custom actions to be performed when entering
 * and exiting each node of a CPT during a traversal. This is useful for tasks
 * like AST building, printing, or validation.
 */
typedef struct
{
    /**
     * @brief Function pointer called when entering a CPT node.
     * @param node The CPT node being entered.
     * @param user_data A user-defined data pointer passed through the visitation.
     */
    void (*enter_node)(epc_cpt_node_t * node, void * user_data);
    /**
     * @brief Function pointer called when exiting a CPT node.
     * @param node The CPT node being exited.
     * @param user_data A user-defined data pointer passed through the visitation.
     */
    void (*exit_node)(epc_cpt_node_t * node, void * user_data);
    void *
        user_data; /**< @brief A user-defined data pointer that is passed to `enter_node` and `exit_node` callbacks. */
} epc_cpt_visitor_t;

/**
 * @brief Flags for controlling whitespace and comment consumption in lexeme and strip parsers.
 */
typedef enum
{
    EPC_CONSUME_WS = 1 << 0,           /**< Consume standard whitespace (isspace). */
    EPC_CONSUME_C_COMMENT = 1 << 1,    /**< Consume C-style comments (slash star ... star slash). */
    EPC_CONSUME_CPP_COMMENT = 1 << 2,  /**< Consume C++-style comments (// ...). */
    EPC_CONSUME_BASH_COMMENT = 1 << 3, /**< Consume Bash-style comments (# ...). */
    EPC_CONSUME_ALL_COMMENTS = (EPC_CONSUME_C_COMMENT | EPC_CONSUME_CPP_COMMENT | EPC_CONSUME_BASH_COMMENT),
    EPC_CONSUME_ALL = (EPC_CONSUME_WS | EPC_CONSUME_C_COMMENT | EPC_CONSUME_CPP_COMMENT),
    EPC_CONSUME_ALL_STYLES = (EPC_CONSUME_ALL | EPC_CONSUME_BASH_COMMENT)
} epc_consume_flags_t;

/**
 * @brief Traverses a Concrete Parse Tree (CPT) in a depth-first manner.
 *
 * This function applies a given `pt_visitor_t` to each node in the CPT,
 * calling `enter_node` before visiting children and `exit_node` after
 * all children have been visited.
 *
 * @param root A pointer to the root `pt_node_t` of the CPT to traverse.
 * @param visitor A pointer to a `pt_visitor_t` structure defining the callbacks
 *                to be executed during the traversal.
 */
EASY_PC_API void epc_cpt_visit_nodes(epc_cpt_node_t * root, epc_cpt_visitor_t * visitor);

/**
 * @brief Creates a new parser list.
 *
 * @return A new `epc_parser_list` instance, or NULL on error.
 */
EASY_PC_API epc_parser_list * epc_parser_list_create(void);

/**
 * @brief Frees all parsers in the list and the list itself.
 *
 * @param list The parser list to free.
 */
EASY_PC_API void epc_parser_list_free(epc_parser_list * list);

/**
 * @brief Creates a parser that matches a single specific character and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param c The character to match.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_char(epc_parser_list * list, char const * name, char c);

/**
 * @brief Creates a parser that matches a single specific byte.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param b The character to match.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_byte(epc_parser_list * list, char const * name, char b);

/**
 * @brief Creates a parser that matches a specific string literal and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param s The string literal to match. The lifetime of `s` must exceed the parser's.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_string(epc_parser_list * list, char const * name, char const * s);

/**
 * @brief Creates a parser that matches a single digit character (0-9) and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_digit(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a single alphabetic character (a-z, A-Z) and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_alpha(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a single alphanumeric character (a-z, A-Z, 0-9) and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_alphanum(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches an integer number (e.g., "123", "-45") and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_int(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a floating-point number (e.g., "3.14", "-.5", "1e-3") and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_double(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a single whitespace character and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_space(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches any character.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_any(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a standard programming identifier and adds it to the list.
 * It matches the pattern `[_a-zA-Z][_a-zA-Z0-9]*`.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_identifier(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches an octal integer literal and adds it to the list.
 *
 * It matches the pattern `0[0-7]*`.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_octal(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a hexadecimal integer literal and adds it to the list.
 * It matches the pattern `0[xX][0-9a-fA-F]+`.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_hex(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that always succeeds and produces a node with specified content and adds it to the list.
 * This parser consumes no input and always returns a successful result
 * containing a CPT node with the given `content` and its length.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_succeed(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a single hexadecimal digit (0-9, a-f, A-F) and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_hex_digit(epc_parser_list * list, char const * name);

/* Combinators, or parsers that take more than one argument. */
/**
 * @brief Creates a parser that matches a single character within a specified range and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param char_start The beginning character of the range (inclusive).
 * @param char_end The ending character of the range (inclusive).
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_char_range(epc_parser_list * list, char const * name, char char_start, char char_end);

/**
 * @brief Creates a parser that matches any single character NOT in the provided set and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param chars_to_avoid A null-terminated string of characters that should NOT be matched.
 *                       The lifetime of this string must exceed the parser's.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_none_of(epc_parser_list * list, char const * name, char const * chars_to_avoid);

/**
 * @brief Creates a parser that matches the given parser zero or more times and adds it to the list.
 * This parser always succeeds. If the child parser matches zero times, it
 * consumes no input and returns a success result with an empty list of children.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to repeat.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_many(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that matches the given parser exactly `num` times and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param num The exact number of times the child parser must match.
 *            If `num` is 0, it always succeeds, consuming no input.
 * @param p The child parser to repeat.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_count(epc_parser_list * list, char const * name, size_t num, epc_parser_t * p);

/**
 * @brief Creates a parser that matches the given parser between `min` and `max` times and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param min The minimum number of times the child parser must match.
 *            If `num` is 0, it always succeeds, consuming no input.
 * @param max The maximum number of times the child parser must match.
 * @param p The child parser to repeat.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t *
epc_count_range(epc_parser_list * list, char const * name, size_t min, size_t max, epc_parser_t * p);

/**
 * @brief Creates a parser that matches `open`, then `p` (wrapped content), then `close` and adds it to the list.
 * The resulting CPT node will represent the entire `open` + `p` + `close` sequence,
 * but its direct children will only include the result of the wrapped parser.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param open The parser for the opening delimiter.
 * @param p The parser for the content wrapped by the delimiters.
 * @param close The parser for the closing delimiter.
 * @return A new `parser_t` instance, or NULL on error.
 */
epc_parser_t *
epc_between(epc_parser_list * list, char const * name, epc_parser_t * open, epc_parser_t * p, epc_parser_t * close);

/**
 * @brief Creates a parser that matches one or more `item` parsers, optionally separated by a `delimiter` and adds it to
 * the list.
 * It requires at least one `item` to match. If a `delimiter_parser` is provided,
 * it attempts to match it between items.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param item_parser The parser for the items in the list.
 * @param delimiter_parser An optional parser for the delimiter between items. Can be NULL.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t *
epc_delimited(epc_parser_list * list, char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser);

/**
 * @brief Creates a parser that matches one or more `item` parsers, optionally separated by a `delimiter` and adds it to
 * the list. This is a "flexible" version that backtracks if it finds a delimiter but no subsequent item.
 * It requires at least one `item` to match. If a `delimiter_parser` is provided,
 * it attempts to match it between items. If it matches a delimiter but the next item fails,
 * it backtracks over the delimiter and succeeds with the items matched so far.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param item_parser The parser for the items in the list.
 * @param delimiter_parser An optional parser for the delimiter between items. Can be NULL.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_delimited_flex(
    epc_parser_list * list, char const * name, epc_parser_t * item_parser, epc_parser_t * delimiter_parser
);

/**
 * @brief Creates a parser that attempts to match `p`, but always succeeds and adds it to the list.
 * If `p` matches, its result is included. If `p` fails, `p_optional` succeeds
 * without consuming any input and produces an empty (zero-length) node.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to make optional.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_optional(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that attempts to match `p` but consumes no input and adds it to the list.
 * `p_lookahead` succeeds if `p` succeeds, and fails if `p` fails.
 * However, regardless of success or failure, it never consumes any input.
 * Useful for asserting conditions without advancing the input stream.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to look ahead for.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_lookahead(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that succeeds if `p` FAILS, and fails if `p` SUCCEEDS and adds it to the list.
 * `p_not` never consumes any input. It acts as a negative lookahead.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to check for non-matching.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_not(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that always fails with a specified error message and adds it to the list.
 * Useful for explicitly indicating invalid states in a grammar.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param message The error message to report when this parser fails.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_fail(epc_parser_list * list, char const * name, char const * message);

/**
 * @brief Creates a parser that matches any single character from a specified set and adds it to the list.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param chars_to_match A null-terminated string of characters that are allowed to match.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_one_of(epc_parser_list * list, char const * name, char const * chars_to_match);

/**
 * @brief Creates a parser that matches its child parser, optionally surrounded by whitespace and adds it to the list.
 * The whitespace itself is skipped and not included in the CPT node for the lexeme.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to wrap.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_lexeme(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that matches its child parser, optionally surrounded by whitespace/comments.
 * Similar to epc_lexeme, but allows specifying which whitespace/comments to consume.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to wrap.
 * @param flags Flags for controlling whitespace and comment consumption.
 */
EASY_PC_API epc_parser_t *
epc_lexeme_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags);

/**
 * @brief Creates a parser that strips leading and trailing whitespace. Similar to epc_lexeme, except that
 * if doesn't strip comments.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to strip
 */
EASY_PC_API epc_parser_t * epc_strip(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that strips leading and trailing whitespace. Similar to epc_strip, but allows
 * specifying which whitespace/comments to consume.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to strip
 * @param flags Flags for controlling whitespace and comment consumption.
 */
EASY_PC_API epc_parser_t *
epc_strip_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags);

/**
 * @brief Creates a parser that strips leading whitespace. Similar to epc_strip, but only strips leading whitespace.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to strip
 */
EASY_PC_API epc_parser_t * epc_stripl(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that strips leading whitespace. Similar to epc_stripl, but allows
 * specifying which whitespace/comments to consume.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to strip
 * @param flags Flags for controlling whitespace and comment consumption.
 */
EASY_PC_API epc_parser_t *
epc_stripl_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags);

/**
 * @brief Creates a parser that strips trailing whitespace. Similar to epc_strip, but only strips trailing whitespace.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to strip
 */
EASY_PC_API epc_parser_t * epc_stripr(epc_parser_list * list, char const * name, epc_parser_t * p);

/**
 * @brief Creates a parser that strips trailing whitespace. Similar to epc_stripr, but allows
 * specifying which whitespace/comments to consume.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p The child parser to strip
 * @param flags Flags for controlling whitespace and comment consumption.
 */
EASY_PC_API epc_parser_t *
epc_stripr_ex(epc_parser_list * list, char const * name, epc_parser_t * p, epc_consume_flags_t flags);

/**
 * @brief Creates a parser that matches one or more `item` parsers,
 *        separated by an `op` parser, applying `op` left-associatively and adds it to the list.
 *        Useful for arithmetic expressions like 1 + 2 - 3.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param item The parser for the individual items.
 * @param op The parser for the operator.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t *
epc_chainl1(epc_parser_list * list, char const * name, epc_parser_t * item, epc_parser_t * op);

/**
 * @brief Creates a parser that matches one or more `item` parsers,
 *        separated by an `op` parser, applying `op` right-associatively and adds it to the list.
 *        Useful for expressions like 1 ^ 2 ^ 3.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param item The parser for the individual items.
 * @param op The parser for the operator.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t *
epc_chainr1(epc_parser_list * list, char const * name, epc_parser_t * item, epc_parser_t * op);

/**
 * @brief Creates a parser that tries to match one of several alternative parsers.
 * `p_or` tries each parser in the list in order. The first one that succeeds
 * determines the result. If all fail, `p_or` fails.
 *
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param count The number of alternative parsers.
 * @param ... A variable argument list of `parser_t*` pointers, one for each alternative.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_or(epc_parser_list * list, char const * name, int count, ...);

/**
 * @brief Creates a parser that matches a sequence of parsers in order.
 * `p_and` succeeds if all provided parsers in the sequence succeed,
 * consuming input as each parser matches. If any parser in the sequence fails,
 * `p_and` fails. The CPT node will have children for each successful parser in the sequence.
 *
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param count The number of parsers in the sequence.
 * @param ... A variable argument list of `parser_t*` pointers, one for each part of the sequence.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_and(epc_parser_list * list, char const * name, int count, ...);

/**
 * @brief Creates a parser that attempts to match `parser_to_skip` zero or more times, discarding its results and adds
 * it to the list.
 *
 * This is similar to `p_many`, but it explicitly discards the CPT nodes generated
 * by `parser_to_skip`. It always succeeds, consuming as much input as `parser_to_skip`
 * matches, but generating a single "skip" node with combined length and no children.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param parser_to_skip The parser whose matches should be skipped.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_skip(epc_parser_list * list, char const * name, epc_parser_t * parser_to_skip);

/**
 * @brief Creates a parser that matches `parser_to_repeat` one or more times.
 * This parser requires at least one successful match of the child parser.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param parser_to_repeat The child parser to repeat.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_plus(epc_parser_list * list, char const * name, epc_parser_t * parser_to_repeat);

/**
 * @brief Creates a parser that matches the end of the input stream and adds it to the list.
 * This parser succeeds only if the stream is at the start of input.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_soi(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches the end of the input stream and adds it to the list.
 * This parser succeeds only if there are no more characters to consume in the input.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_eoi(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a C++ style comment (// to end of line or EOF).
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_cpp_comment(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a Bash style comment (# to end of line or EOF).
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_bash_comment(epc_parser_list * list, char const * name);

/**
 * @brief Creates a parser that matches a C-style comment.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_c_comment(epc_parser_list * list, char const * name);

/**
 * @brief A predicate function type for use with `epc_satisfy()`.
 * The function should return true if the parser should succeed at the current position, or false if it should fail.
 * The function is called with the parser instance and a user-defined context pointer, allowing for custom, dynamic
 * parsing logic that can depend on external state or complex conditions that are not easily expressed with the standard
 * combinators.
 * @param token The current CPT node token to evaluate.
 * @param parse_ctx The parser context for the current parse attempt, which can be used to access information about the
 * input and manage state during parsing.
 * @param user_ctx A user-defined context pointer that will be passed to the predicate function. The lifetime of this
 * pointer must exceed that of the parser.
 * @return true if the parser should succeed at the current position, or false if it should fail.
 */
typedef bool (*epc_satisfy_parser_predicate_fn)(epc_cpt_node_t * token, epc_parser_ctx_t * parse_ctx, void * user_ctx);

/**
 * @brief Assigns an error message to a CPT node. Intended to be used by satisfy predicet functions when they want to
 * attach an error message to a CPT node to indicate a reason for the predicate failure.
 * @param node The node to attach the message to.
 * @param fmt A printf-like format string, followed by the appropriate format values.
 */
void epc_cpt_node_assign_error_message(epc_cpt_node_t * node, char const * fmt, ...);

/**
 * @brief Creates a parser that matches if the provided predicate function returns true for the current input position.
 *
 * The predicate function is called with the parser instance and a user-defined context pointer. It should
 * return true if the parser should succeed at the current position, or false if it should fail.
 *
 * This allows for custom, dynamic parsing logic that can depend on external state or complex conditions
 * that are not easily expressed with the standard combinators.
 *
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param token_parser A parser that produces a token to be evaluated by the predicate function.
 * @param message A message to be displayed when the predicate fails.
 * @param predicate The predicate function to evaluate at parse time.
 * @param parser_data A user-defined context pointer that will be passed to the predicate function.
 *                 The lifetime of this pointer must exceed that of the parser.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_satisfy(
    epc_parser_list * list,
    char const * name,
    epc_parser_t * token_parser,
    char const * message,
    epc_satisfy_parser_predicate_fn predicate,
    void * parser_data
);

/**
 * @brief A callback function type for the entry point of `epc_wrap()`.
 * The function is called with the parser instance and a user-defined context pointer, allowing for custom logic to be
 * executed before attempting to match the wrapped parser. This can be used for tasks such as initializing state,
 * logging, or modifying the parse result before the wrapped parser runs.
 * @param parser The parser instance being entered.
 * @param parse_ctx The parser context for the current parse attempt, which can be used to access information about the
 * input and manage state during parsing.
 * @param parser_data A user-defined context pointer that will be passed to the callback function. The lifetime of this
 * pointer must exceed that of the parser.
 */
typedef void (*epc_wrap_entry_fn)(epc_parser_t * parser, epc_parser_ctx_t * parse_ctx, void * parser_data);

/**
 * @brief A callback function type for the exit point of `epc_wrap()`.
 * The function is called with the parse result and a user-defined context pointer, allowing for custom logic to be
 * executed after attempting to match the wrapped parser. This can be used for tasks such as modifying the parse result
 * based on the outcome of the wrapped parser, implementing custom error handling, or integrating with external systems
 * after the match attempt has completed.
 * @param result The result of the parse attempt for the wrapped parser, including success/failure status and any
 * generated CPT nodes.
 * @param parser_data A user-defined context pointer that will be passed to the callback function. The lifetime of this
 * pointer must exceed that of the parser.
 * @return true if the parse result should be returned as-is, or false if the callback wants to override a successful
 * child result to be an error.
 */
typedef bool (*epc_wrap_exit_fn)(epc_parse_result_t result, epc_parser_ctx_t * parse_ctx, void * parser_data);

/**
 * @brief A struct containing entry and exit callback functions for use with `epc_wrap()`.
 */
typedef struct epc_wrap_callbacks
{
    epc_wrap_entry_fn on_entry;
    epc_wrap_exit_fn on_exit;
} epc_wrap_callbacks_t;

/**
 * @brief Creates a parser that wraps another parser, allowing for custom entry and exit callbacks to be invoked during
 * parsing.
 * The entry callback is called before attempting to match the wrapped parser, and the exit callback is called after the
 * match attempt, regardless of success or failure. This allows for custom logic to be executed at these critical points
 * in the parsing process, such as modifying the parse result, implementing custom error handling, or integrating with
 * external systems.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param wrapped_parser The parser to wrap with the callbacks.
 * @param callbacks A struct containing the entry and exit callback functions to be invoked during parsing.
 * @param parser_data A user-defined context pointer that will be passed to the callback functions. The lifetime of this
 * pointer must exceed that of the parser.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_wrap(
    epc_parser_list * list,
    char const * name,
    epc_parser_t * wrapped_parser,
    epc_wrap_callbacks_t callbacks,
    void * parser_data
);

/**
 * @brief Creates a parser that wraps another parser and memoizes its results.
 * @param list The parser list to add to.
 * @param name The name of the parser for debugging/CPT.
 * @param p_to_memoize The parser to wrap and memoize.
 * @return A new `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_memoize(epc_parser_list * list, char const * name, epc_parser_t * p_to_memoize);

/**
 * @brief Allocates and initializes a new parser object within the grammar's memory context.
 *
 * This function is typically used to create a forward reference to a parser that is needed to
 * overcome circular references to parsers that haven't been defined. The call this function to
 * obtain a base parser, refer to it when constructing other parsers as needed, create the actual
 * parser definition when its dependencies have been created, then finally copy the final parser
 * details to the forward reference by calling by epc_parser_duplicate().
 *
 * @param list The parser list to add to.
 * @param name A string name for the parser, primarily for debugging and CPT visualization.
 * @return A pointer to the newly allocated and initialized `parser_t` instance, or NULL on error.
 */
EASY_PC_API epc_parser_t * epc_parser_fwd_decl(epc_parser_list * list, char const * name);

/**
 * @brief Duplicates the contents of a source parser into a destination parser.
 *
 * This function performs a shallow copy of a parser's data. It's particularly
 * useful for forward declarations where a placeholder parser needs to be
 * filled in later with the definition of another parser.
 *
 * @param dst A pointer to the destination `parser_t` to be filled.
 * @param src A pointer to the source `parser_t` whose contents will be copied.
 */
EASY_PC_API void epc_parser_duplicate(epc_parser_t * dst, epc_parser_t const * src);

/**
 * @brief Sets the AST semantic action for a parser.
 *
 * This function configures how the parse result of a specific parser
 * should be transformed or processed during Abstract Syntax Tree (AST)
 * construction.
 *
 * @param p A pointer to the `parser_t` for which to set the AST action.
 * @param action_type An integer identifier specifying the semantic action.
 *                    Concrete action types are defined elsewhere (e.g., `AST_ACTION_CREATE_NUMBER_FROM_CONTENT`).
 */
EASY_PC_API void epc_parser_set_ast_action(epc_parser_t * p, int action_type);

/**
 * @brief Retrieves the user-defined context pointer from the parser context.
 *        This is the pointer passed in when initiating a parse session (e.g., via `epc_parse_str()`, `epc_parse_fp()`,
 * etc.) and is accessible within parser callbacks (e.g. epc_wrap callbacks).
 * @param ctx A pointer to the `epc_parser_ctx_t` from which to retrieve the user context.
 * @return The user-defined context pointer associated with the current parse session.
 */
EASY_PC_API
void * parse_ctx_get_user_ctx(epc_parser_ctx_t const * ctx);

// --- Updated Top-Level API ---
/**
 * @brief Initiates a parsing operation with a given grammar and NUL-terminated input string.
 *
 * @param top_parser The starting parser for the grammar (e.g., the root rule).
 * @param input_string The null-terminated string to be parsed.
 * @param user_ctx A user-defined context pointer that will be passed to the internal parser context. The lifetime of
 * this pointer must exceed that of the parse session.
 * @return An `easy_pc_parse_session_t` structure containing the result of the
 *         parsing operation (success CPT or error details) and an internal
 *         context for cleanup.
 *         This session MUST be destroyed with `easy_pc_parse_session_destroy`.
 */
EASY_PC_API epc_parse_session_t epc_parse_str(epc_parser_t * top_parser, char const * input_string, void * user_ctx);

/**
 * @brief Initiates a parsing operation with a given grammar and input file stream.
 *
 * This function is similar to `epc_parse_str()`, but it reads from a `FILE*`
 * stream instead of a string. It attempts to match the `top_parser` against
 * the content of the file.
 *
 * @param top_parser The starting parser for the grammar (e.g., the root rule).
 * @param fp A pointer to an open `FILE` stream to be parsed.
 * @param user_ctx A user-defined context pointer that will be passed to the internal parser context. The lifetime of
 * this pointer must exceed that of the parse session.
 * @return An `easy_pc_parse_session_t` structure containing the result of the
 *         parsing operation (success CPT or error details) and an internal
 *         context for cleanup.
 *         This session MUST be destroyed with `easy_pc_parse_session_destroy`.
 */
EASY_PC_API epc_parse_session_t epc_parse_fp(epc_parser_t * top_parser, FILE * fp, void * user_ctx);

/**
 * @brief Initiates a parsing operation with a given grammar and input file specified by filename.
 *
 * This function is similar to `epc_parse_str()`, but it reads from a file specified by its name.
 * It attempts to match the `top_parser` against the content of the file.
 *
 * @param top_parser The starting parser for the grammar (e.g., the root rule).
 * @param filename A null-terminated string containing the path to the file to be parsed.
 * @param user_ctx A user-defined context pointer that will be passed to the internal parser context. The lifetime of
 * this pointer must exceed that of the parse session.
 * @return An `easy_pc_parse_session_t` structure containing the result of the
 *         parsing operation (success CPT or error details) and an internal
 *         context for cleanup.
 *         This session MUST be destroyed with `easy_pc_parse_session_destroy`.
 */
EASY_PC_API epc_parse_session_t epc_parse_file(epc_parser_t * top_parser, char const * filename, void * user_ctx);

/**
 * @brief Initiates a parsing operation with a given grammar and input from a file descriptor.
 *
 * This function initiates a streaming parse from the given file descriptor (fd).
 * The parsing happens in a separate thread, while the main thread reads from the fd.
 * Only supported if the library is compiled with `WITH_INPUT_STREAM_SUPPORT`. This allows for parsing large inputs
 * without loading them entirely into memory, or for parsing data from a pipe or socket in real-time.
 *
 * @param top_parser The starting parser for the grammar.
 * @param fd The file descriptor to read from.
 * @param user_ctx A user-defined context pointer that will be passed to the internal parser context. The lifetime of
 * this pointer must exceed that of the parse session.
 * @return An `easy_pc_parse_session_t` structure.
 */
EASY_PC_API epc_parse_session_t epc_parse_fd(epc_parser_t * top_parser, int fd, void * user_ctx);

#ifdef WITH_INPUT_STREAM_SUPPORT

/**
 * @brief Initiates a reactive (non-blocking) streaming parse from a file descriptor.
 *
 * This function returns immediately. The parsing happens in a background thread.
 * The main thread must notify the library when data is available using `epc_streaming_notify_readable`.
 *
 * @param top_parser The starting parser for the grammar.
 * @param fd The file descriptor to read from.
 * @param on_complete Callback invoked when the parse finishes (successfully or with error).
 * @param cb_user_data User-defined data passed to the callback.
 * @param user_ctx User-defined context pointer for the parse session.
 * @return An `easy_pc_parse_session_t` structure.
 */
EASY_PC_API epc_parse_session_t epc_parse_fd_reactive(
    epc_parser_t * top_parser, int fd, epc_streaming_complete_cb on_complete, void * cb_user_data, void * user_ctx
);

/**
 * @brief Notifies the library that the file descriptor is ready for reading.
 * @param session Pointer to the active reactive session.
 */
EASY_PC_API void epc_streaming_notify_readable(epc_parse_session_t * session);

/**
 * @brief Notifies the library that the end of the stream has been reached.
 * @param session Pointer to the active reactive session.
 */
EASY_PC_API void epc_streaming_notify_eof(epc_parse_session_t * session);

/**
 * @brief Notifies the library of a fatal error on the stream.
 * @param session Pointer to the active reactive session.
 * @param error_code The errno or custom error code.
 */
EASY_PC_API void epc_streaming_notify_error(epc_parse_session_t * session, int error_code);

/**
 * @brief Prepares a session for the next parse operation in a sequence.
 *
 * This can only be called after the previous parse operation has completed.
 * It compacts the input buffer (moving leftover data to the start) and resets
 * the internal state for the new parser.
 *
 * @param session Pointer to the session to advance.
 * @param next_parser The parser to use for the next object in the stream.
 * @return false if the session can't be advanced (e.g. input closed or error), else true.
 */
EASY_PC_API bool epc_parse_session_advance(epc_parse_session_t * session, epc_parser_t * next_parser);

/**
 * @brief Checks if the background parsing thread is currently active.
 * @param session Pointer to the active reactive session.
 * @return True if the thread is active, false otherwise.
 */
EASY_PC_API bool epc_parse_session_is_active(epc_parse_session_t const * session);

/**
 * @brief Synchronizes the result from the background parsing thread to the session.
 *
 * This function moves the parse result from the internal context into the
 * `session->result` field. Once synchronized, the session takes ownership
 * of the result, and it will be cleaned up by `epc_parse_session_destroy()`.
 *
 * @param session Pointer to the session to synchronize.
 * @return True if a result was available and synchronized, false otherwise.
 */
EASY_PC_API bool epc_parse_session_sync_result(epc_parse_session_t * session);
#endif

/**
 * @brief Destroys an `easy_pc_parse_session_t` and frees all associated resources.
 *
 * This function is crucial for preventing memory leaks. It frees the CPT
 * (if parsing was successful) or the error details (if parsing failed),
 * along with the internal parser context.
 *
 * @param session A pointer to the `easy_pc_parse_session_t` to be destroyed.
 */
EASY_PC_API void epc_parse_session_destroy(epc_parse_session_t * session);

EASY_PC_API void epc_parse_session_print_cpt(FILE * fp, epc_parse_session_t const * session);

/**
 * @brief Initiates a parsing operation with a given grammar and a byte buffer.
 *
 * @param top_parser The starting parser for the grammar (e.g., the root rule).
 * @param buf A pointer to the byte buffer to be parsed.
 * @param len The length of the buffer.
 * @param user_ctx A user-defined context pointer that will be passed to the internal parser context.
 * @return An `easy_pc_parse_session_t` structure containing the result.
 *         This session MUST be destroyed with `easy_pc_parse_session_destroy`.
 */
EASY_PC_API epc_parse_session_t
epc_parse_bytes(epc_parser_t * top_parser, char const * buf, size_t len, void * user_ctx);

/**
 * @brief Retrieves the semantically relevant content from a CPT node.
 *
 * This function returns a pointer to the start of the substring within the
 * node's full `content` that is considered semantically relevant, after
 * accounting for any `semantic_start_offset`.
 *
 * @param node A pointer to the `epc_cpt_node_t`.
 * @return A `const char*` pointer to the semantic content.
 */
EASY_PC_API const char * epc_cpt_node_get_semantic_content(epc_cpt_node_t const * node);

/**
 * @brief Retrieves the offset from the input start to the semantically relevant content from a CPT node.
 *
 * This function returns a pointer to the start of the substring within the
 * node's full `content` that is considered semantically relevant, after
 * accounting for any `semantic_start_offset`.
 *
 * @param node A pointer to the `epc_cpt_node_t`.
 * @return A `const char*` pointer to the semantic content.
 */
EASY_PC_API size_t epc_cpt_node_get_semantic_content_offset(epc_cpt_node_t const * node);

/**
 * @brief Retrieves the length of the semantically relevant content from a CPT node.
 *
 * This function returns the length of the substring within the node's full
 * `content` that is considered semantically relevant, after accounting for
 * `semantic_start_offset` and `semantic_end_offset`.
 *
 * @param node A pointer to the `epc_cpt_node_t`.
 * @return A `size_t` representing the length of the semantic content.
 */
EASY_PC_API size_t epc_cpt_node_get_semantic_len(epc_cpt_node_t const * node);

/**
 * @brief Retrieves the content from a CPT node.
 *
 * This function returns a pointer to the start of the node's full `content`.
 * Usually the same as the semantic content, except for the epc_lexeme parser,
 * which excludes leading/trailing whitespace.
 *
 * @param node A pointer to the `epc_cpt_node_t`.
 * @return A `const char*` pointer to the content.
 */
EASY_PC_API const char * epc_cpt_node_get_content(epc_cpt_node_t const * node);

/**
 * @brief Retrieves the content offset from a CPT node.
 *
 * This function returns an offset from the start of the input to the node's content from a CTP node.
 *
 * @param node A pointer to the `epc_cpt_node_t`.
 * @return A `const char*` pointer to the content.
 */
EASY_PC_API size_t epc_cpt_node_get_content_offset(epc_cpt_node_t const * node);

/**
 * @brief Retrieves the length of the content from a CPT node.
 *
 * This function returns the length of the the node's full `content`.
 * Usually the same as the semantic content length, except for the epc_lexeme
 * parser, which excludes leading/trailing whitespace.
 *
 * @param node A pointer to the `epc_cpt_node_t`.
 * @return A `size_t` representing the length of the content.
 */
EASY_PC_API size_t epc_cpt_node_get_len(epc_cpt_node_t const * node);

/**
 * @brief Prints a Concrete Parse Tree (CPT) to a dynamically allocated string.
 *
 * This utility function generates a human-readable string representation of
 * the CPT for debugging or visualization purposes. The returned string
 * must be freed by the caller.
 *
 * @param parse_ctx The parser context associated with the CPT.
 * @param node The root `pt_node_t` of the CPT (or any sub-tree) to print.
 * @return A dynamically allocated string containing the CPT representation,
 *         or NULL on allocation failure. The caller is responsible for freeing this string.
 */
EASY_PC_API char * epc_cpt_to_string(epc_parser_ctx_t * parse_ctx, epc_cpt_node_t * node);

/**
 * @brief Returns the version of the easy_pc library.
 *
 * @return A string representing the version (e.g., "0.1.0").
 */
EASY_PC_API char const * epc_get_version(void);

/**
 * @brief Calculate the line and column in the input where the supplied offset is.
 *
 * This function takes an offset into the input and returns the line and column that corresponds to that offset.
 *
 * @param parse_ctx The parser context associated with the CPT.
 * @param offset The offset into the input
 * @return A structure containing the calculated (1-based) line and column.
 */
epc_line_col_t epc_calculate_line_and_column(epc_parser_ctx_t * ctx, size_t offset);

/**
 * @brief Get the line of input that contains the supplied offset into the parsed input.
 *
 * @param parse_ctx The parser context associated with the CPT.
 * @param offset The offset into the input
 * @return A heap-allocated string where the offset occurs. NB - The user is responsible for freeing this string.
 */
EASY_PC_API
char * epc_get_line_at_offset(epc_parser_ctx_t * ctx, size_t const offset);

#ifdef __cplusplus
}
#endif
