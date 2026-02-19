# GDL Compiler Improvements and Enhancements

This document outlines potential improvements and enhancements for the `gdl_compiler` tool, focusing on expanding `easy_pc` combinator support in the code generator module and refining AST node generation for better usability in the code generation process.

## 1. Code Generator Module (gdl_code_generator.c) Improvements

The `gdl_code_generator.c` module is responsible for translating the GDL Abstract Syntax Tree (AST) into `easy_pc` C code. While it supports many core `easy_pc` combinators, several could be added to enhance the expressiveness and power of the GDL language.

### 1.1 Expand `epc_xxx_l()` Combinator Support

The current code generator already handles `epc_string_l`, `epc_char_l`, `epc_eoi_l`, `epc_alpha_l`, `epc_digit_l`, `epc_alphanum_l`, `epc_space_l`, `epc_hex_digit_l`, `epc_int_l`, `epc_double_l`, `epc_and_l`, `epc_or_l`, `epc_many_l`, `epc_plus_l`, `epc_optional_l`, `epc_count_l`, `epc_lexeme_l`, `epc_delimited_l`, `epc_between_l`, `epc_chainl1_l`, `epc_chainr1_l`, and `epc_char_range_l`.

The following `easy_pc` combinators are currently recognized by the GDL grammar but are only partially implemented in the code generator (e.g., in `GDL_AST_NODE_TYPE_FUNCTION_CALL` case of `generate_expression_code`) or explicitly noted with `TODO`s:

*   **`fail` / `succeed`:** These are keywords in GDL but their generation is not explicitly handled in `gdl_code_generator.c`'s `GDL_AST_NODE_TYPE_KEYWORD` switch. They should map to `epc_fail_l` and `epc_succeed_l`.
*   **`lookahead(expression)`:** While `GDL_AST_ACTION_CREATE_LOOKAHEAD_CALL` exists, its code generation is generic (`unary_combinator_call`) and might need specific handling if different label strings or other parameters are required. It maps to `epc_lookahead_l`.
*   **`not(expression)`:** Similar to `lookahead`, this maps to `epc_not_l`.
*   **`skip(expression)`:** Similar to `lookahead`, this maps to `epc_skip_l`.
*   **`passthru(expression)`:** Similar to `lookahead`, this maps to `epc_passthru_l`.
*   **`oneof(char_literal, ...)` / `none_of(char_literal, ...)`:**
    *   Currently, the GDL grammar's `oneof` and `none_of` combinator calls are parsed using `gdl_char_literal` for arguments and handled in `gdl_parser.c` to construct `epc_oneof_chars_l` and `epc_none_of_chars_l` by stringifying the char literals.
    *   The `gdl_code_generator.c` does *not* explicitly handle `GDL_AST_NODE_TYPE_COMBINATOR_ONEOF` or `GDL_AST_NODE_TYPE_COMBINATOR_NONEOF` in its `generate_expression_code` function. It seems there is a mismatch where the AST builder creates these nodes, but the code generator's `FUNCTION_CALL` branch uses `strcmp(func_name, "oneof")` etc.
    *   **Recommendation:** Ensure `GDL_AST_NODE_TYPE_COMBINATOR_ONEOF` and `GDL_AST_NODE_TYPE_COMBINATOR_NONEOF` nodes are directly handled in `generate_expression_code` to properly construct `epc_oneof_chars_l` and `epc_noneof_chars_l` from their arguments (which would be a list of characters).

### 1.2 Improve Labeling for Generated Parsers

Currently, many generated `epc_xxx_l` calls (especially for sequences and alternatives) use generic labels like `"seq"`, `"alt"`, `"optional_rep"`. While the rule names provide context, more descriptive labels (e.g., derived from the parent rule or a unique identifier) would greatly aid in debugging complex parse trees.

*   **Recommendation:** Pass a more specific label to nested `epc_xxx_l` calls. This could be a combination of the parent rule's name and a sequence number or a more descriptive context.

### 1.3 Support for Generic `epc_char_range_l`

While `GDL_AST_NODE_TYPE_CHAR_RANGE` is handled, exploring how to represent and generate more generic `epc_char_range_l` (or even `epc_any_char_l`, `epc_eof_l`) directly via GDL syntax.

## 2. Parser Module (gdl_parser.c & gdl_ast_builder.c) Improvements

The `gdl_parser.c` module defines the GDL grammar itself, and `gdl_ast_builder.c` constructs the AST from the Concrete Parse Tree (CPT). Enhancing the AST structure can simplify the code generation process.

### 2.1 Refine AST Node Typing for Combinators

Currently, many combinator calls (e.g., `lookahead`, `not`, `skip`, `passthru`, `optional`) are processed as `GDL_AST_NODE_TYPE_FUNCTION_CALL`s and then mapped to a generic `GDL_AST_NODE_TYPE_COMBINATOR_UNARY_T` for code generation. While functional, creating more specific AST node types could provide clearer semantics.

*   **Recommendation:** Introduce distinct AST node types for each unary combinator (e.g., `GDL_AST_NODE_TYPE_COMBINATOR_LOOKAHEAD`, `GDL_AST_NODE_TYPE_COMBINATOR_NOT`, etc.) to explicitly represent their unique behavior in the AST. This would require corresponding changes in `gdl_ast.h`, `gdl_ast_builder.c`, and `gdl_code_generator.c`.

### 2.2 Explicit AST Nodes for Sequence and Alternative Elements

The AST currently uses `GDL_AST_NODE_TYPE_SEQUENCE` and `GDL_AST_NODE_TYPE_ALTERNATIVE` with generic `gdl_ast_list_t` elements. For `gdl_code_generator.c`, handling `SEQUENCE`s and `ALTERNATIVE`s with only one element involves promoting the single child directly.

*   **Recommendation:** Consider whether explicit `GDL_AST_ACTION_CREATE_SINGLE_SEQUENCE_ELEMENT` or `GDL_AST_ACTION_CREATE_SINGLE_ALTERNATIVE_ELEMENT` actions/nodes (or a more complex representation for sequences/alternatives) might better represent the grammar intent and simplify generation logic, reducing special casing in `gdl_code_generator.c`.

### 2.3 Semantic Actions for GDL Keywords

The GDL grammar's handling of built-in keywords like `alpha`, `digit`, `underscore`, etc., is currently a mix between `GDL_AST_ACTION_CREATE_KEYWORD` directly (if lexemized in `gdl_parser.c`) and being detected by `is_gdl_builtin_keyword` when initially parsed as an `IDENTIFIER_REF`.

*   **Recommendation:** Standardize the AST representation of built-in GDL keywords. Ensure that `alpha`, `digit`, `eoi`, `underscore`, `fail`, `succeed`, etc., *always* result in `GDL_AST_NODE_TYPE_KEYWORD` nodes, possibly with a specialized `epc_parser_t *` for each in `gdl_parser.c` (like `gdl_kw_double` is handled) rather than relying on `IDENTIFIER_REF` and then a runtime check. This would make the AST more explicit and less dependent on string comparisons in the builder.

### 2.4 Better Error Reporting

While `gdl_compiler` catches parsing and AST building errors, the messages could be more user-friendly, especially for common GDL syntax mistakes.

*   **Recommendation:** Enhance error reporting in `gdl_ast_builder.c` and `gdl_parser.c` to pinpoint the exact location and nature of GDL syntax errors more clearly.

This document serves as a starting point for improving the `gdl_compiler` tool, aiming for increased robustness, expressiveness, and maintainability.
