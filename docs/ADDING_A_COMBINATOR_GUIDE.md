# Adding a New Combinator to the GDL Compiler

This guide describes the changes needed to add support for a new library parser combinator to the GDL compiler (`tools/gdl_compiler/`). The GDL compiler is a self-hosting, token-based parser that compiles `.gdl` grammar files into C code using the `easy_pc` parser combinator library.

## Scenario

The library introduces a new combinator:

```c
epc_parser_t * epc_foo(epc_parser_list * l, char const * name, epc_parser_t * parser_a, epc_parser_t * parser_b);
```

It accepts two sub-parsers and combines them. The GDL syntax for this combinator is:

```gdl
myRule = foo(sub_expr_a, sub_expr_b);
```

This should generate C code like:

```c
epc_foo(list, "myRule", <generated_code_for_a>, <generated_code_for_b>)
```

## Pipeline Overview

Adding a combinator touches exactly **6 files** across 3 stages:

| Stage | File | Change |
|-------|------|--------|
| **1. Token ID** | `gdl_token_ids.h` | Add `TOKEN_KW_FOO` to the enum and name table |
| **2. Tokenizer** | `gdl_tokenizer_actions.c` | Map the string `"foo"` to `TOKEN_KW_FOO` |
| **3. Grammar parser** | `gdl_parser.c` | Define the `foo_call` grammar rule and integrate into the expression hierarchy |
| **4. AST types** | `gdl_ast.h` | Add action enum, node type enum, struct, and union member |
| **5. AST actions** | `gdl_compiler_ast_actions.c` | Write the handler function, free case, and register it |
| **6. Code generator** | `gdl_code_generator.c` | Add reference traversal and code emission cases |

---

## Step 1: Token ID (`gdl_token_ids.h`)

Add a new entry in the `gdl_token_id_t` enum (with the other combinator keywords):

```c
// gdl_token_ids.h — inside the enum

// Keywords — combinator parsers
TOKEN_KW_STRING,
TOKEN_KW_CHAR_RANGE,
// ...
TOKEN_KW_FOO,      // <--- new
// ...
```

Then add a name string in `gdl_token_id_name()` so the framework can produce readable debug output:

```c
// gdl_token_ids.h — inside gdl_token_id_name()

case TOKEN_KW_FOO:
    return "KW_FOO";
```

> **Note:** The ordering within the enum matters for binary compatibility of the generated token list. Add new entries at the end of the existing keyword section, or between related keywords. Changing the order of existing entries will shift enum values and can cause subtle bugs.

---

## Step 2: Tokenizer Keyword Table (`gdl_tokenizer_actions.c`)

The tokenizer uses a lookup table to map GDL keyword strings to token IDs. Add an entry in the `keywords[]` array in the "Combinator parsers" section:

```c
// gdl_tokenizer_actions.c — inside keywords[]

// Combinator parsers
{.name = "string",      .id = TOKEN_KW_STRING},
{.name = "char_range",  .id = TOKEN_KW_CHAR_RANGE},
{.name = "noneof",      .id = TOKEN_KW_NONEOF},
// ...
{.name = "foo",         .id = TOKEN_KW_FOO},   // <--- new
// ...
```

That's all that's needed — the tokenizer's `handle_keyword_or_identifier` action already uses `keyword_id_for_name()` to look up any string in the table. No new action function or registration is required.

---

## Step 3: Grammar Parser (`gdl_parser.c`)

This is the most involved step. You need to:

1. Create a terminal parser for the `TOKEN_KW_FOO` token
2. Define the `foo_call` grammar rule with sub-expression slots
3. Wire `foo_call` into the expression hierarchy

### 3a. Create the keyword terminal parser

Add a terminal parser for the `foo` keyword token alongside the other keyword terminals:

```c
// gdl_parser.c — in create_gdl_parser(), after the other keyword terminals

epc_parser_t * p_foo = epc_token(l, "KW_foo", TOKEN_KW_FOO);
```

### 3b. Define the grammar rule

The GDL syntax `foo(sub_expr_a, sub_expr_b)` should be parsed as:
```
foo_call = KW_foo '(' definition_expression ',' definition_expression ')'
```

First create an intermediate "args" rule (groups the two sub-expressions with the comma separator):

```c
// gdl_parser.c — after the p_foo definition

epc_parser_t * foo_args = epc_and(
    l, "FooArgs", 3,
    gdl_definition_expression,
    gdl_comma,
    gdl_definition_expression
);
epc_parser_t * foo_call = epc_and(
    l, "FooCall", 4,
    p_foo,
    gdl_lparen,
    foo_args,
    gdl_rparen
);
epc_parser_set_ast_action(foo_call, GDL_AST_ACTION_CREATE_FOO_CALL);
```

**Key details:**
- `gdl_definition_expression` is the top-level forward-declared parser that matches any GDL expression. Using it here allows any valid expression (literal, keyword, combinator call, etc.) as a sub-argument.
- The `FooArgs` intermediate rule groups the two expressions with their comma. CPT nodes without an `epc_parser_set_ast_action` set are transparent — their children bubble up to the parent action. This means `FooCall`'s action receives exactly **2 AST children** (the two sub-expressions), not 4.
- `gdl_comma` matches `TOKEN_COMMA` — it's already defined in the parser setup section.

### 3c. Wire into the expression hierarchy

Add `foo_call` to the `gdl_combinator_call` alternative list:

```c
// gdl_parser.c — inside gdl_combinator_call = epc_or(l, "CombinatorCall", <N>, ...)

epc_parser_t * gdl_combinator_call = epc_or(
    l, "CombinatorCall", 21,         // increment N by 1
    none_of_call,
    count_call,
    // ...
    foo_call,                         // <--- new
    // ...
);
```

Update the count argument to `epc_or()` to reflect the new total number of alternatives.

`gdl_combinator_call` is one of the alternatives in `gdl_primary_expression` (alongside terminals, literals, identifiers, etc.), so `foo_call` automatically becomes available wherever any expression is expected.

---

## Step 4: AST Types (`gdl_ast.h`)

Four changes are needed in `gdl_ast.h`:

### 4a. Action enum

```c
// gdl_ast.h — inside enum epc_ast_user_defined_action_gdl

GDL_AST_ACTION_CREATE_FOO_CALL,   // <--- new
GDL_AST_ACTION_MAX,
```

Always add new actions before `GDL_AST_ACTION_MAX`.

### 4b. Node type enum

```c
// gdl_ast.h — inside gdl_ast_node_type_t

GDL_AST_NODE_TYPE_COMBINATOR_FOO,   // <--- new
```

### 4c. Data structure

Define a struct that holds the two sub-expression nodes:

```c
// gdl_ast.h — near the other combinator structs

typedef struct
{
    gdl_ast_node_t * parser_a;
    gdl_ast_node_t * parser_b;
} gdl_ast_combinator_foo_t;
```

### 4d. Union member

Add the struct to the `gdl_ast_node_t` union:

```c
// gdl_ast.h — inside struct gdl_ast_node_t, in the data union

gdl_ast_combinator_foo_t foo_call;
```

---

## Step 5: AST Action Handler (`gdl_compiler_ast_actions.c`)

Three changes are needed: the handler function, the free case, and the registration.

### 5a. Write the handler function

```c
// gdl_compiler_ast_actions.c

static void
handle_create_foo_call(
    epc_ast_builder_ctx_t * ctx, epc_cpt_node_t * node,
    void ** children, int count, void * user_data
)
{
    (void)node;
    if (count != 2)
    {
        epc_ast_builder_set_error(
            ctx, "Foo call expects 2 children (parser_a, parser_b), got %d", count
        );
        for (int i = 0; i < count; ++i)
            gdl_ast_node_free(children[i], user_data);
        return;
    }

    gdl_ast_node_t * a = (gdl_ast_node_t *)children[0];
    gdl_ast_node_t * b = (gdl_ast_node_t *)children[1];

    gdl_ast_node_t * result = gdl_ast_node_alloc(ctx, GDL_AST_NODE_TYPE_COMBINATOR_FOO);
    if (result)
    {
        result->data.foo_call.parser_a = a;
        result->data.foo_call.parser_b = b;
        epc_ast_push(ctx, result);
    }
}
```

**Key details:**
- The handler receives 2 children because `FooArgs` is transparent (no action set), so its children (two `definition_expression` results) are promoted to the `FooCall` parent.
- It validates the count, allocates the node, stores the sub-nodes (taking ownership), and pushes the result.
- On error, it frees any received children to prevent memory leaks.

### 5b. Add the free case

In the `gdl_ast_node_free()` function, add a case to recursively free the sub-nodes:

```c
// gdl_compiler_ast_actions.c — inside gdl_ast_node_free()

case GDL_AST_NODE_TYPE_COMBINATOR_FOO:
    gdl_ast_node_free(node->data.foo_call.parser_a, user_data);
    gdl_ast_node_free(node->data.foo_call.parser_b, user_data);
    break;
```

### 5c. Register the handler

In `gdl_ast_hook_registry_init()`, add the registration:

```c
// gdl_compiler_ast_actions.c — inside gdl_ast_hook_registry_init()

epc_ast_hook_registry_set_action(
    registry, GDL_AST_ACTION_CREATE_FOO_CALL, handle_create_foo_call
);
```

---

## Step 6: Code Generator (`gdl_code_generator.c`)

Two cases needed: one for the dependency analysis pass, one for the code emission pass.

### 6a. Reference traversal (dependency analysis)

The code generator first scans the AST to determine rule dependency order. Since `foo_call` contains sub-expressions that may reference other rules, add a case to traverse them:

```c
// gdl_code_generator.c — inside traverse_expression_for_references()

case GDL_AST_NODE_TYPE_COMBINATOR_FOO:
    traverse_expression_for_references(
        expression_node->data.foo_call.parser_a, current_rule_info, all_rules
    );
    traverse_expression_for_references(
        expression_node->data.foo_call.parser_b, current_rule_info, all_rules
    );
    break;
```

### 6b. Code emission

Add a case in the expression code generation switch (the large `switch` on `expression_node->type` inside `generate_expression_code()`):

```c
// gdl_code_generator.c — inside generate_expression_code()

case GDL_AST_NODE_TYPE_COMBINATOR_FOO:
{
    fprintf(
        source_file,
        "epc_foo(list, %s%s%s, ",
        q, expr_name, q
    );
    if (!generate_expression_code(
            source_file,
            expression_node->data.foo_call.parser_a,
            indent_level + 1, rule_list, NULL
        ))
        return false;
    fprintf(source_file, ", ");
    if (!generate_expression_code(
            source_file,
            expression_node->data.foo_call.parser_b,
            indent_level + 1, rule_list, NULL
        ))
        return false;
    fprintf(source_file, ")");
    break;
}
```

**Key details:**
- `q` is a string delimiter (usually `"\"`), used to quote the parser name.
- `expr_name` is the generated parser name for this node (derived from the rule name).
- The sub-expressions are emitted recursively via `generate_expression_code()`, and are separated by a comma.
- The entire call is wrapped in `epc_foo(...)`, producing output like:
  ```c
  epc_foo(list, "myRule", epc_string(list, "hello"), epc_digit(list, "digit"))
  ```

---

## Checklist

- [ ] **`gdl_token_ids.h`** — Added `TOKEN_KW_FOO` to the enum and `gdl_token_id_name()`
- [ ] **`gdl_tokenizer_actions.c`** — Added `{.name = "foo", .id = TOKEN_KW_FOO}` to `keywords[]`
- [ ] **`gdl_parser.c`** — Created `p_foo` terminal, `foo_call` rule with sub-expressions, wired into `gdl_combinator_call`
- [ ] **`gdl_ast.h`** — Added action enum, node type enum, `gdl_ast_combinator_foo_t` struct, union member
- [ ] **`gdl_compiler_ast_actions.c`** — Added `handle_create_foo_call()`, free case, and registry registration
- [ ] **`gdl_code_generator.c`** — Added reference traversal and code emission cases
- [ ] **Build & test** — `cmake --build build && ctest -V --test-dir build`

## Visual Pipeline

```
GDL source                    Token stream                  CPT nodes                    AST nodes                  Generated C
                                                                                         
 "foo(a, b)"                  KW_FOO  LPAREN               FooCall                      COMBINATOR_FOO            epc_foo(
                                    IDENTIFIER(a)           ├─ def_expr(a)              ├─ parser_a(a)               list, "name",
                                    COMMA                   ├─ def_expr(b)              └─ parser_b(b)               <code_for_a>,
                                    IDENTIFIER(b)           └─ RPAREN                                                          
                                    RPAREN                                                                                           <code_for_b>,
                                                                                                                                    )
       │                            │                           │                           │                         │
  tokenizer                    grammar parser            AST builder                  code generator               output
  (epc_parse_str             (epc_parse_session        (epc_ast_build               (gdl_generate_c_code          .c/.h
   + epc_ast_build)           _reparse)                  + callback)                  + dependency pass)            files
```

## Common Pitfalls

1. **Forgetting to update the `epc_or()` count.** When adding `foo_call` to `gdl_combinator_call = epc_or(l, "CombinatorCall", N, ...)`, update `N` to reflect the new total number of alternatives. An incorrect count causes silent parse failures at runtime.

2. **Incorrect child count in the action handler.** The number of AST children the handler receives depends on the CPT structure — intermediate rules without actions (like `FooArgs`) are transparent and their children bubble up. Count how many sub-rules actually have `epc_parser_set_ast_action` calls, because those are the ones that push AST nodes.

3. **Missing the free case.** If the free case in `gdl_ast_node_free()` is omitted, the sub-expression nodes leak when the AST is destroyed.

4. **Forgetting the name string in `gdl_token_id_name()`.** Without it, debug output and error messages will show `"UNKNOWN"` for the new token, making debugging harder.

5. **Placement in the code generator switch.** The `generate_expression_code()` switch is large. Make sure the new `case` is placed in the correct region (combinator cases, not terminal or literal cases) and that it doesn't accidentally fall through to the next case (use `break` or `return`).
