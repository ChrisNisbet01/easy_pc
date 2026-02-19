# Plan for Generating C Code from GDL AST

## 1. Overview

The goal is to extend the `gdl_compiler` tool to generate C header and source files. These generated files will contain `easy_pc` parser code that is equivalent to the language definition provided in a GDL file. The C code will define the grammar as a series of `easy_pc` parser combinator calls, making it directly usable in a C project.

## 2. Output Files

The code generation process will result in the following files:

*   **`[grammar_name]_parser_actions.h`**: A header file defining an `enum` of all semantic actions (`@action_name`) encountered in the GDL grammar. This allows for type-safe references to semantic actions in other parts of the system (e.g., in an AST builder for the generated parser). Each semantic action will be assigned a unique integer value.

*   **`[grammar_name]_parser.h`**: A header file containing the prototype for the `create_[grammar_name]_parser()` function. This function will be responsible for initializing and returning the top-level `easy_pc` parser for the defined grammar. It will also declare any other necessary public functions or data structures generated.

*   **`[grammar_name]_parser.c`**: The C source file containing the implementation of the `create_[grammar_name]_parser()` function. This file will consist primarily of `easy_pc` parser combinator API calls that construct the grammar specified by the GDL AST. It will include all necessary `easy_pc.h` headers.

## 3. Code Generation Strategy

The `gdl_compiler` will traverse the GDL Abstract Syntax Tree (AST) and translate each AST node into corresponding C code.

### 3.1. GDL AST Traversal

The code generator will perform a depth-first traversal of the GDL AST. For each node, it will generate the appropriate C code snippet.

### 3.2. Mapping GDL AST Nodes to `easy_pc` Parser API Calls

Each type of GDL AST node will correspond to a specific `easy_pc` parser function call or a sequence of calls.

*   **`GDL_AST_NODE_TYPE_PROGRAM`**:
    *   Will be the entry point for code generation. It will iterate through `RULE_DEFINITION` nodes.
    *   Will include `easy_pc/easy_pc.h`.

*   **`GDL_AST_NODE_TYPE_RULE_DEFINITION`**:
    *   For each rule, a variable of type `epc_parser_t *` will be declared (e.g., `epc_parser_t * my_rule;`).
    *   Recursive rules will require forward declarations using `epc_parser_allocate_l()` and `epc_parser_duplicate()`.
    *   The rule's definition (`rule_def.definition`) will be translated.
    *   If a `semantic_action` is present, `epc_parser_set_ast_action()` will be called with the corresponding semantic action enum value.

*   **`GDL_AST_NODE_TYPE_IDENTIFIER_REF`**:
    *   If referring to another rule: `epc_parser_t * rule_name;`
    *   If referring to a terminal (e.g., `alpha`, `digit`): `epc_alpha_l(list, "alpha")`, `epc_digit_l(list, "digit")`, etc.
    *   Keywords (`double`, `eoi`, etc.) will be generated using `epc_string_l()` or the appropriate `easy_pc` terminal.

*   **`GDL_AST_NODE_TYPE_STRING_LITERAL`**:
    *   `epc_string_l(list, "rule_name", "literal_value")`

*   **`GDL_AST_NODE_TYPE_CHAR_LITERAL`**:
    *   `epc_char_l(list, "rule_name", 'literal_value')`

*   **`GDL_AST_NODE_TYPE_NUMBER_LITERAL`**:
    *   Currently, `easy_pc` does not have a direct `epc_number_l` for integers as it does for `epc_double_l`. This will be generated as a sequence of digits (e.g., `epc_plus_l(list, "digits", epc_digit_l(list, "digit"))`).

*   **`GDL_AST_NODE_TYPE_CHAR_RANGE`**:
    *   `epc_range_l(list, "rule_name", 'start_char', 'end_char')`

*   **`GDL_AST_NODE_TYPE_REPETITION_OPERATOR`**:
    *   Handled in conjunction with `REPETITION_EXPRESSION`.

*   **`GDL_AST_NODE_TYPE_SEMANTIC_ACTION`**:
    *   `epc_parser_set_ast_action(parser_var, SEMANTIC_ACTION_ENUM_VALUE)` where `SEMANTIC_ACTION_ENUM_VALUE` comes from `[grammar_name]_parser_actions.h`.

*   **`GDL_AST_NODE_TYPE_REPETITION_EXPRESSION`**:
    *   Translates an expression followed by a repetition operator (`*`, `+`, `?`).
    *   `*` -> `epc_many_l()`
    *   `+` -> `epc_plus_l()`
    *   `?` -> `epc_optional_l()`
    *   The `expression` part of `REPETITION_EXPRESSION` will be generated recursively.

*   **`GDL_AST_NODE_TYPE_SEQUENCE`**:
    *   `epc_and_l(list, "rule_name", num_elements, element1, element2, ...)`

*   **`GDL_AST_NODE_TYPE_ALTERNATIVE`**:
    *   `epc_or_l(list, "rule_name", num_alternatives, alt1, alt2, ...)`

*   **Combinator Calls (e.g., `oneof`, `count`, `between`, `delimited`, `lexeme`, `optional`)**:
    *   These directly map to their `easy_pc` `_l` counterparts (e.g., `epc_oneof_l`, `epc_count_l`). Arguments will be generated recursively.
    *   `gdl_kw_double` corresponds to `epc_double_l`.

*   **`GDL_AST_NODE_TYPE_FUNCTION_CALL`**:
    *   This is a generic function call within the *target* grammar. It will be translated as `epc_and_l` of the `function_name` identifier and the `arguments` list enclosed in parentheses. The `epc_parser_set_ast_action()` for this will use `GDL_AST_ACTION_CREATE_FUNCTION_CALL`. The actual arguments will be generated using a delimited list.

### 3.3. Semantic Action Handling

1.  **Collect Semantic Action Names:** During the initial AST traversal, all unique `@action_name` identifiers will be collected.
2.  **Generate `[grammar_name]_parser_actions.h`**: An `enum` will be generated containing these unique action names, mapping them to integer values (e.g., `ACTION_MY_ACTION = 1`).
3.  **Map to `epc_parser_set_ast_action()`**: In the generated `.c` file, `epc_parser_set_ast_action(parser_ptr, ACTION_MY_ACTION)` will be called where appropriate.

### 3.4. Forward Declarations and Recursion

For recursive rules (e.g., `Expression` referring to `Term`, and `Term` referring to `Primary`, which might refer back to `Expression` via parentheses), a two-pass approach or careful ordering is needed:
1.  **Declare all parser variables:** All `epc_parser_t *` variables for rules will be declared at the beginning of the `create_parser()` function.
2.  **Allocate placeholders for recursive rules:** For rules involved in recursion, `epc_parser_allocate_l()` will be used to create placeholder parsers.
3.  **Define non-recursive rules first:** Generate code for rules that do not depend on yet-undefined rules.
4.  **Define recursive rules:** When defining a recursive rule, `epc_parser_duplicate()` will be used to link the placeholder parser to its full definition.

## 4. Implementation Details

The `gdl_compiler` will be modified to include a new "code generation" phase after AST building. This phase will:
1.  Read the GDL AST.
2.  Collect all semantic action names.
3.  Generate `[grammar_name]_parser_actions.h`.
4.  Generate `[grammar_name]_parser.h`.
5.  Traverse the AST to generate the C code for `create_[grammar_name]_parser()` function, writing it to `[grammar_name]_parser.c`.
6.  Ensure all generated C files include `<easy_pc/easy_pc.h>` and any other standard headers as needed.

## 5. Example Walkthrough (for `arithmetic_parser_language_initial.gdl`)

Let's consider how `Program = Expression EOI @AST_ACTION_ASSIGN_ROOT;` might be translated:

**`arithmetic_parser_actions.h` (excerpt):**
```c
#pragma once
enum {
    AST_ACTION_CREATE_NUMBER_FROM_CONTENT,
    AST_ACTION_CREATE_IDENTIFIER,
    AST_ACTION_CREATE_OPERATOR_FROM_CHAR,
    AST_ACTION_CREATE_FUNCTION_CALL,
    AST_ACTION_BUILD_BINARY_EXPRESSION,
    AST_ACTION_ASSIGN_ROOT
};
```

**`arithmetic_parser.h` (excerpt):**
```c
#pragma once
#include <easy_pc/easy_pc.h>

epc_parser_t * create_arithmetic_parser(epc_parser_list * list);
```

**`arithmetic_parser.c` (excerpt):**
```c
#include "arithmetic_parser.h"
#include "arithmetic_parser_actions.h"
#include <easy_pc/easy_pc.h>
#include <stddef.h> // For NULL, size_t, etc.

epc_parser_t * create_arithmetic_parser(epc_parser_list * list)
{
    // Forward declarations for recursive rules
    epc_parser_t * Expression = epc_parser_allocate_l(list, "Expression");
    epc_parser_t * Term = epc_parser_allocate_l(list, "Term");
    epc_parser_t * Primary = epc_parser_allocate_l(list, "Primary");

    // Basic Terminals (as defined in GDL)
    epc_parser_t * Double_ = epc_double_l(list, "Double");
    epc_parser_t * Number = epc_lexeme_l(list, "Number", Double_);
    epc_parser_set_ast_action(Number, AST_ACTION_CREATE_NUMBER_FROM_CONTENT);

    epc_parser_t * Identifier_ = epc_and_l(list, "Identifier_Raw", 2,
                                            epc_or_l(list, "IdStartChar", 2, epc_alpha_l(list, "alpha"), epc_char_l(list, "underscore", '_')),
                                            epc_many_l(list, "IdContChar", epc_or_l(list, "IdContChar", 3, epc_alpha_l(list, "alpha"), epc_digit_l(list, "digit"), epc_char_l(list, "underscore", '_')))
                                          );
    epc_parser_t * Identifier = epc_lexeme_l(list, "Identifier", Identifier_);
    epc_parser_set_ast_action(Identifier, AST_ACTION_CREATE_IDENTIFIER);

    // Operators
    epc_parser_t * AddOp = epc_lexeme_l(list, "AddOp", epc_char_l(list, "add", '+'));
    epc_parser_t * SubOp = epc_lexeme_l(list, "SubOp", epc_char_l(list, "sub", '-'));
    // ... similar for MulOp, DivOp

    // Structural Tokens
    epc_parser_t * LParen = epc_lexeme_l(list, "LParen", epc_char_l(list, "lparen", '('));
    epc_parser_t * RParen = epc_lexeme_l(list, "RParen", epc_char_l(list, "rparen", ')'));
    epc_parser_t * Comma = epc_lexeme_l(list, "Comma", epc_char_l(list, "comma", ','));
    epc_parser_t * EOI = epc_eoi_l(list, "EOI");

    // Combined Operators
    epc_parser_t * AddSubOp = epc_or_l(list, "AddSubOp_Raw", 2, AddOp, SubOp);
    epc_parser_set_ast_action(AddSubOp, AST_ACTION_CREATE_OPERATOR_FROM_CHAR);
    // ... similar for MulDivOp

    // Rule for Function Arguments
    epc_parser_t * NonEmptyFunctionArgs = epc_delimited_l(list, "NonEmptyFunctionArgs", Expression, Comma);
    epc_parser_t * FunctionArgs = epc_optional_l(list, "FunctionArgs", NonEmptyFunctionArgs);

    // Rules
    epc_parser_t * FunctionCall = epc_and_l(list, "FunctionCall", 4, Identifier, LParen, FunctionArgs, RParen);
    epc_parser_set_ast_action(FunctionCall, AST_ACTION_CREATE_FUNCTION_CALL);

    // Primary expressions
    epc_parser_t * Primary_def = epc_or_l(list, "Primary", 4,
                                          Number,
                                          Identifier,
                                          FunctionCall,
                                          epc_between_l(list, "ParenExpr", LParen, Expression, RParen)
                                         );
    epc_parser_duplicate(Primary, Primary_def);

    // Term and Expression
    epc_parser_t * Term_def = epc_chainl1_l(list, "Term", Primary, MulDivOp);
    epc_parser_set_ast_action(Term_def, AST_ACTION_BUILD_BINARY_EXPRESSION);
    epc_parser_duplicate(Term, Term_def);

    epc_parser_t * Expression_def = epc_chainl1_l(list, "Expression", Term, AddSubOp);
    epc_parser_set_ast_action(Expression_def, AST_ACTION_BUILD_BINARY_EXPRESSION);
    epc_parser_duplicate(Expression, Expression_def);

    // Top-level rule
    epc_parser_t * Program = epc_and_l(list, "Program", 2, Expression, EOI);
    epc_parser_set_ast_action(Program, AST_ACTION_ASSIGN_ROOT);

    return Program;
}
```
This looks like a comprehensive plan. I will now write this plan to `c_code_from_ast_plan.md`.