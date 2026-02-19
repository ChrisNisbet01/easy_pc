**Completed Steps (as of the last interaction):**

1.  **Project Setup:**
    *   Set up the `tools/gdl_compiler` directory structure.
    *   Created initial `gdl_parser.h`, `gdl_parser.c`, and `main.c` files for the GDL compiler.
    *   Configured `CMakeLists.txt` files to build the `gdl_compiler` executable and link it with `easy_pc`.

2.  **Basic Terminal Implementation & Testing:**
    *   Implemented GDL parser rules for several basic terminals:
        *   Identifiers (`alpha`, `digit`, `underscore`).
        *   String Literals (`"..."`).
        *   Character Literals (`'a'`).
        *   Character Ranges (`[a-z]`).
        *   Keywords (e.g., `alpha`, `digit`, `oneof`, `not`, etc.).
    *   Integrated `gdl_raw_char` for handling raw characters in contexts like character ranges.

3.  **Unit Testing Framework (`GdlParserTest.cpp`):**
    *   Created `tests/GdlParserTest.cpp` with proper `setup()` and `teardown()` functions for managing `epc_parser_list` and `gdl_parser_root`.
    *   Implemented initial tests:
        *   `TEST(GdlParserTest, CreateParserSuccess)`: Confirms parser initialization.
        *   `TEST(GdlParserTest, ParseSimpleIdentifierRule)`: Verifies parsing of a rule defining an identifier.
        *   `TEST(GdlParserTest, ParseSimpleStringLiteralRule)`: Verifies parsing of a rule defining a string literal.
        *   `TEST(GdlParserTest, ParseSimpleCharLiteralRule)`: Verifies parsing of a rule defining a character literal.
        *   `TEST(GdlParserTest, ParseSimpleCharRangeRule)`: Verifies parsing of a rule defining a character range.

4.  **Whitespace Handling and Keyword Resolution Fixes:**
    *   Identified and fixed issues related to whitespace consumption and the correct identification of keywords versus identifiers, particularly within sequences.
    *   **Crucially, it was established that wrapping the `epc_or` for the GDL `Keyword` rule itself within `epc_lexeme_l()` in `gdl_parser.c` correctly handles whitespace and resolves ambiguity with general identifiers.** This approach has been implemented and confirmed working through passing tests.
    *   Adjusted CPT traversal and assertions in existing tests (`STRNCMP_EQUAL`) to correctly extract semantic content and length after these `epc_lexeme` changes.

5.  **Sequence Rule Testing:**
    *   `TEST(GdlParserTest, ParseSimpleSequenceRule)`: Added and fixed this test to correctly parse rules involving a sequence of two keywords (e.g., `MySeqRule = alpha digit;`). This test now passes.
6.  **Alternative Rule Testing:**
    *   `TEST(GdlParserTest, ParseSimpleChoiceRule)`: Added this test to correctly parse rules involving alternatives (e.g., `MyChoiceRule = alpha | digit;`). This test now passes.
7.  **Optional Rule Testing:**
    *   `TEST(GdlParserTest, ParseSimpleOptionalRule)`: Added this test to correctly parse rules involving optional elements (e.g., `MyOptionalRule = alpha?;`). This test now passes.
8.  **One or More Rule Testing:**
    *   `TEST(GdlParserTest, ParseSimplePlusRule)`: Added this test to correctly parse rules involving one or more elements (e.g., `MyPlusRule = alpha+;`). This test now passes.
9.  **Zero or More Rule Testing:**
    *   `TEST(GdlParserTest, ParseSimpleStarRule)`: Added this test to correctly parse rules involving zero or more elements (e.g., `MyStarRule = alpha*;`). This test now passes.
10. **Oneof Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseOneofCallRule)`: Added this test to correctly parse rules involving the `oneof` combinator call (e.g., `MyOneofRule = oneof('a', 'b', 'c');`). This test now passes.
11. **Noneof Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseNoneofCallRule)`: Added this test to correctly parse rules involving the `none_of` combinator call (e.g., `MyNoneofRule = none_of('x', 'y', 'z');`). This test now passes.
12. **Count Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseCountCallRule)`: Added this test to correctly parse rules involving the `count` combinator call (e.g., `MyCountRule = count(5, alpha);`). This test now passes.
13. **Between Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseBetweenCallRule)`: Added this test to correctly parse rules involving the `between` combinator call (e.g., `MyBetweenRule = between('[', alpha, ']');`). This test now passes.
14. **Delimited Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseDelimitedCallRule)`: Added this test to correctly parse rules involving the `delimited` combinator call (e.g., `MyDelimitedRule = delimited('(', alpha);`). This test now passes.
15. **Lookahead Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseLookaheadCallRule)`: Added this test to correctly parse rules involving the `lookahead` combinator call (e.g., `MyLookaheadRule = lookahead(alpha);`). This test now passes.
16. **Not Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseNotCallRule)`: Added this test to correctly parse rules involving the `not` combinator call (e.g., `MyNotRule = not(alpha);`). This test now passes.
17. **Lexeme Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseLexemeCallRule)`: Added this test to correctly parse rules involving the `lexeme` combinator call (e.g., `MyLexemeRule = lexeme(alpha);`). This test now passes.
18. **Skip Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseSkipCallRule)`: Added this test to correctly parse rules involving the `skip` combinator call (e.g., `MySkipRule = skip(alpha);`). This test now passes.
19. **Chainl1 Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseChainl1CallRule)`: Added this test to correctly parse rules involving the `chainl1` combinator call (e.g., `MyChainl1Rule = chainl1(digit, '+');`). This test now passes.
20. **Chainr1 Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseChainr1CallRule)`: Added this test to correctly parse rules involving the `chainr1` combinator call (e.g., `MyChainr1Rule = chainr1(digit, '+');`). This test now passes.
21. **Passthru Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParsePassthruCallRule)`: Added this test to correctly parse rules involving the `passthru` combinator call (e.g., `MyPassthruRule = passthru(alpha);`). This test now passes.
22. **Semantic Action Testing:**
    *   `TEST(GdlParserTest, ParseSemanticActionRule)`: Added this test to correctly parse rules involving semantic actions (e.g., `MyActionRule = alpha @my_action;`). This test now passes.

23. **Number Literal Testing:**
    *   `TEST(GdlParserTest, ParseNumberLiteralRule)`: Added this test to correctly parse rules involving a number literal (e.g., `MyNumberRule = 123;`). This test now passes.

24. **Parenthesized Expressions Testing:**
    *   `TEST(GdlParserTest, ParseParenthesizedExpressionRule)`: Added this test to correctly parse rules involving a parenthesized expression (e.g., `MyRule = (alpha | digit);`). This test now passes.

25. **Optional Combinator Call Testing:**
    *   `TEST(GdlParserTest, ParseOptionalCombinatorCallRule)`: Added this test to correctly parse rules involving the `optional` combinator call (e.g., `MyOptionalCallRule = optional(alpha);`). This test now passes.
26. **Double Keyword Rule Testing:**
    *   `TEST(GdlParserTest, ParseDoubleKeywordRule)`: Added this test to correctly parse rules involving the `double` keyword (e.g., `MyDoubleRule = double;`). This test now passes.


## AST Builder Implementation & Testing

**Current Immediate Task:** All listed AST Builder Unit Tests have been implemented and are passing.

**Outstanding Tasks:**

1.  **Implement `GDL_AST_ACTION_CREATE_FUNCTION_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Implemented in this turn)
2.  **Implement `GDL_AST_ACTION_CREATE_CHAR_RANGE` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
3.  **Implement `GDL_AST_ACTION_COLLECT_ARGUMENTS` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
4.  **Implement `GDL_AST_ACTION_CREATE_ONEOF_CALL` and `GDL_AST_ACTION_CREATE_NONEOF_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
5.  **Implement `GDL_AST_ACTION_CREATE_COUNT_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
6.  **Implement `GDL_AST_ACTION_CREATE_BETWEEN_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
7.  **Implement `GDL_AST_ACTION_CREATE_DELIMITED_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
8.  **Implement `GDL_AST_ACTION_PROMOTE_CHILD` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
9.  **Implement `GDL_AST_ACTION_CREATE_LOOKAHEAD_CALL`, `GDL_AST_ACTION_CREATE_NOT_CALL`, `GDL_AST_ACTION_CREATE_LEXEME_CALL`, `GDL_AST_ACTION_CREATE_SKIP_CALL`, `GDL_AST_ACTION_CREATE_PASSTHRU_CALL`, `GDL_AST_ACTION_CREATE_OPTIONAL_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
10.  **Implement `GDL_AST_ACTION_CREATE_CHAINL1_CALL` and `GDL_AST_ACTION_CREATE_CHAINR1_CALL` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)
11. **Implement `GDL_AST_ACTION_CREATE_SEQUENCE` and `GDL_AST_ACTION_CREATE_ALTERNATIVE` in `gdl_ast_builder_exit_node`.**
    *   **COMPLETED** (Already implemented)

---

## C Code Generation Implementation

**Current Immediate Task:** Implement traversal of the GDL AST to generate basic `easy_pc` parser declarations for `simple_test_language.gdl`.

**Details:**
*   Initial focus on generating the necessary C header and source files.
    *   **COMPLETED** (Initial `gdl_code_generator.h` and `gdl_code_generator.c` created, integrated into `main.c`, and successfully generated placeholder files for `simple_test_language.gdl`).
*   The generated C code will construct `easy_pc` parsers from the GDL AST.
*   Ensure that forward allocation of parsers is only performed if strictly necessary - i.e. if a rule is referenced before it is created.
*   This will involve writing the C code generation logic in `gdl_code_generator.c` and integrating it into `gdl_compiler/main.c`.
