# Semantic Actions: Using GDL Action Annotations Instead of Manual CPT Walking

## Problem

Walking CPT nodes manually to build a token list is fragile because:

- It requires access to private CPT struct fields (`tag`, `name`, `children`)
- It depends on brittle string comparisons against compiler-generated rule names (which may have unpredictable casing)
- It needs complex tree-navigation helpers (`find_many_node`, `unwrap_token_node`) that must match the exact CPT structure

## Solution: Semantic Action Callbacks

The GDL compiler supports annotating grammar rules with semantic action identifiers using the `@` syntax. At parse time, the AST builder framework calls your registered callback whenever a rule with an action annotation is matched.

### Step 1: Annotate GDL rules

Add `@ACTION_NAME` after a rule definition:

```
Newline = '\n' @SEM_ACTION_NEWLINE;
Name = identifier @SEM_ACTION_IDENTIFIER;
Number = ImagNumber | PointFloat | ... @SEM_ACTION_NUMBER;
String = TripleDQ | TripleSQ | ... @SEM_ACTION_STRING;
Plus = '+' @SEM_ACTION_SINGLE_CHAR;
DoubleStar = "**" @SEM_ACTION_DOUBLE_CHAR;
Ellipsis = "..." @SEM_ACTION_TRIPLE_CHAR;
```

The action name must be a valid C identifier. The GDL compiler generates an enum and assigns each action a unique integer ID.

### Step 2: Generated actions header

The GDL compiler produces a `{grammar}_actions.h` header with an enum of all action IDs:

```c
typedef enum {
    SEM_ACTION_NEWLINE,
    SEM_ACTION_IDENTIFIER,
    SEM_ACTION_NUMBER,
    SEM_ACTION_STRING,
    SEM_ACTION_DOUBLE_CHAR,
    SEM_ACTION_TRIPLE_CHAR,
    SEM_ACTION_SINGLE_CHAR,
    SEM_ACTION_SPACE,
    GRAMMARNAME_AST_ACTION_COUNT__,
} grammarname_semantic_action_t;
```

The `COUNT__` sentinel is used to allocate the hook registry.

### Step 3: Generated parser code calls `epc_parser_set_ast_action`

For each annotated rule, the generated C code sets the action on the parser:

```c
epc_parser_t * Newline = epc_char(list, "Newline", '\n');
epc_parser_set_ast_action(Newline, SEM_ACTION_NEWLINE);
```

This stores the action ID in the parser's metadata but does not execute it — execution happens during the AST build walk.

### Step 4: Register callback functions

Create a hook registry, set callbacks for each action, then call `epc_ast_build`:

```c
epc_ast_hook_registry_t * registry = epc_ast_hook_registry_create(GRAMMARNAME_AST_ACTION_COUNT__);
epc_ast_hook_registry_set_action(registry, SEM_ACTION_NEWLINE, handle_newline_action);
epc_ast_hook_registry_set_action(registry, SEM_ACTION_IDENTIFIER, handle_identifier_action);
// ... etc

epc_ast_build(root, registry, &user_data);

epc_ast_hook_registry_free(registry);
```

### Step 5: Callback signature

Every callback has the same signature:

```c
void my_callback(
    epc_ast_builder_ctx_t * ctx,   // builder context
    epc_cpt_node_t * node,         // the CPT node for the matched rule
    void ** children,              // AST results from children (usually NULL if not building an AST)
    int count,                     // number of children
    void * user_data               // opaque pointer passed through from epc_ast_build
);
```

### Step 6: The walk

`epc_ast_build` performs a depth-first traversal of the CPT. When it encounters a node whose parser has a registered action, it invokes the callback with that node and its children.

Callback timing:
- The callback fires when the node's entire subtree has been parsed (post-order)
- By this point, child callbacks (if any) have already fired
- The `children` array contains any AST nodes pushed by child callbacks

### Benefits over manual CPT walking

| Aspect | Manual Walking | Semantic Actions |
|--------|---------------|------------------|
| Tree navigation | Manual `find_many_node`/`unwrap_token_node` | Framework handles traversal |
| Node identification | `node->tag`/`node->name` string compares | Action ID enum (type-safe) |
| Private struct access | Requires `cpt_node.h` (private header) | Only public API needed |
| Roboustness to GDL changes | Fragile — structure must match | Only need the right action annotation |
| Code separation | One big walker function | Per-rule callbacks, cleanly separated |
| Reusability | Tied to specific grammar structure | Callbacks work with any grammar |

## When to use which approach

- **Use semantic actions** when you want to react to specific rules being matched (e.g., building a token list, constructing an AST, collecting statistics)
- **Use manual CPT walking** only when you need to inspect the raw tree structure for debugging or when the action callback model is insufficient

## Reference

- `epc_ast_hook_registry_create(count)` — create registry with `count` action slots
- `epc_ast_hook_registry_set_action(registry, action_index, callback)` — bind callback
- `epc_ast_hook_registry_free(registry)` — free registry (callbacks are NOT freed)
- `epc_ast_build(root, registry, user_data)` — walk CPT and fire callbacks
- `epc_ast_builder_set_error(ctx, fmt, ...)` — signal an error from within a callback
- `epc_ast_push(ctx, node)` — push an AST node result (for building ASTs)
- `epc_parser_set_ast_action(parser, action_id)` — assign action to parser (called by generated code)
