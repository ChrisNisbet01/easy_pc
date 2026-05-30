# GDL LSP Server — Implementation Plan

## Overview

Create `tools/gdl_lsp/` — a C-based LSP server for `.gdl` files that provides
syntax highlighting via the LSP `textDocument/semanticTokens/full` method.

The server reuses the GDL tokenizer and parser from `tools/gdl_compiler/` as a
CMake OBJECT library to avoid code duplication, and builds on the LSP
infrastructure (transport, framing, JSON-RPC) from
`/home/chris/projects/lsp_experiment/`.

## Two-Level Syntax Highlighting

### Level 1 (token, always succeeds)
- On document change (debounced 100ms): run the GDL tokenizer to produce a flat
  token list with positions (offset, length, line, column).
- Each token maps to an LSP semantic token type via a lookup table.
- This always succeeds — the tokenizer handles any input, even mid-edit.

### Level 2 (AST, best-effort)
- After tokenization succeeds, attempt the full parse (`epc_parse_session_reparse`)
  followed by AST build (`epc_ast_build`).
- Walk the AST to distinguish rule definitions from rule references, identify
  semantic actions, etc.
- If parse fails (user is mid-edit): silently fall back to Level 1 cache.

## Code Sharing

Modify `tools/gdl_compiler/CMakeLists.txt`:
- Extract 4 shared parser sources into an OBJECT library `gdl_parser_objects`
  - `gdl_parser.c`
  - `gdl_tokenizer_parser.c`
  - `gdl_tokenizer_actions.c`
  - `gdl_compiler_ast_actions.c`
- `gdl_compiler` links against `gdl_parser_objects`
- `gdl_lsp` also links against `gdl_parser_objects`

## Files

### From lsp_experiment (adapted)

| File | Changes |
|------|---------|
| `transport.c/h` | Remove tool_runner dependency; use `gdl_lsp_server_st` |
| `framing.c/h` | No changes (Content-Length framing is standard) |
| `rpc.c/h` | No changes (JSON-RPC dispatch is generic) |
| `documents.c/h` | Add `documents_lookup_by_index()` for cache iteration |
| `utils.c/h` | No changes |

### New files

| File | Purpose |
|------|---------|
| `main.c` | Entry point |
| `gdl_lsp_server.c/h` | Server struct (`gdl_lsp_server_st`), parser lifecycle, `run_server()` |
| `gdl_lsp_handlers.c/h` | All LSP method handlers |
| `gdl_lsp_semantic_tokens.c/h` | Tokenize → cache; full parse → AST walk; delta encoding |

## Build Integration

### `tools/gdl_lsp/CMakeLists.txt`
```cmake
find_package(PkgConfig REQUIRED)
pkg_check_modules(JSON_C REQUIRED IMPORTED_TARGET json-c)
find_library(UBOX ubox REQUIRED)

add_executable(gdl_lsp main.c gdl_lsp_server.c gdl_lsp_handlers.c
    gdl_lsp_semantic_tokens.c transport.c framing.c rpc.c documents.c utils.c)
target_compile_options(gdl_lsp PRIVATE -Wall -Wextra -pedantic)
target_compile_definitions(gdl_lsp PRIVATE _GNU_SOURCE)
target_link_libraries(gdl_lsp PRIVATE gdl_parser_objects easy_pc_shared
    PkgConfig::JSON_C ${UBOX})
```

### Root `CMakeLists.txt`
```cmake
add_subdirectory(tools/gdl_lsp)
```

## Debug Output

Every significant operation writes to stderr with `[LSP]` prefix:

| Location | Message |
|----------|---------|
| `transport.c:stdin_cb` | `"[LSP] read %zu bytes from stdin\n"` |
| `transport.c:stdin_cb` after frame | `"[LSP] decoded %zu-byte frame\n"` |
| `transport.c:transport_send` | `"[LSP] queued %zu bytes for send\n"` |
| `rpc.c:rpc_dispatch` | `"[LSP] dispatching '%s' (id=%s)\n"` |
| `rpc.c:rpc_send_response` | `"[LSP] sending response for id=%s\n"` |
| `gdl_lsp_handlers.c:didChange` | `"[LSP] didChange: uri=%s, debounce timer set\n"` |
| `gdl_lsp_handlers.c:debounce_cb` | `"[LSP] debounce fired for %s\n"` |
| `gdl_lsp_semantic_tokens.c:tokenize` | `"[LSP] tokenized %s: %zu tokens\n"` |
| `gdl_lsp_semantic_tokens.c:full_parse` | `"[LSP] full parse %s: success\n"` or `"[LSP] full parse %s: failed (%s)\n"` |
| `gdl_lsp_handlers.c:semanticTokens` | `"[LSP] semanticTokens: uri=%s, %d tokens\n"` |

## Server State

```c
typedef struct {
    epc_token_id_t id;
    unsigned long offset;   // byte offset from start
    unsigned long line;     // 0-indexed line
    unsigned long column;   // 0-indexed column
    unsigned long length;   // byte length
} gdl_token_entry_t;

typedef struct {
    char uri[4096];
    gdl_token_entry_t * tokens;
    int token_count;
    bool ast_available;     // Level 2 data available?
    int * ast_token_types;  // refined types from AST walk (or NULL)
} gdl_document_cache_t;

typedef struct {
    rpc_server_st base;

    // GDL parsers (created once, reused)
    epc_parser_list * parser_list;
    epc_parser_t * tokenizer_parser;
    epc_parser_t * grammar_parser;
    epc_ast_hook_registry_t * tokenizer_registry;
    epc_ast_hook_registry_t * ast_registry;
    gdl_tokenizer_ctx_t tokenizer_ctx;

    // Per-document token cache
    gdl_document_cache_t * caches;
    int cache_count;
    int cache_capacity;

    // Debounce
    struct uloop_timeout debounce_timer;
    char * pending_uri;     // strdup'd, freed after processing
} gdl_lsp_server_st;
```

## Semantic Token Types

| Token IDs | LSP Type |
|-----------|----------|
| `TOKEN_KW_*` | `keyword` |
| `TOKEN_STRING_LITERAL`, `TOKEN_CHAR_LITERAL` | `string` |
| `TOKEN_NUMBER` | `number` |
| `TOKEN_IDENTIFIER` | `variable` |
| `TOKEN_EQUALS`, `TOKEN_PIPE`, `TOKEN_SEMICOLON`, `TOKEN_AT`, `TOKEN_COMMA`, `TOKEN_MINUS`, `TOKEN_STAR`, `TOKEN_PLUS`, `TOKEN_QUESTION` | `operator` |
| `TOKEN_LBRACKET`, `TOKEN_RBRACKET`, `TOKEN_LPAREN`, `TOKEN_RPAREN` | `operator` |
| `TOKEN_RAW_CHAR_LITERAL`, `TOKEN_TOKEN_LITERAL` | `string` |

Level 2 (AST) may refine `variable` to `parameter` (rule definition) or
`function` (semantic action).

## Testing

- Manual: pipe JSON-RPC messages via stdin, observe stderr debug output.
- Automated Python test: `tests/test_gdl_lsp.py` sends a sequence of LSP
  messages via `subprocess.Popen` and asserts responses.

```python
import subprocess, json, time
proc = subprocess.Popen(["build/tools/gdl_lsp/gdl_lsp"],
    stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
# Send initialize, didOpen, wait for debounce, request semanticTokens, assert
```

## Implementation Order

1. Modify `tools/gdl_compiler/CMakeLists.txt` — add OBJECT library
2. Create `tools/gdl_lsp/` directory
3. Copy and adapt LSP infrastructure files from lsp_experiment
4. Create `gdl_lsp_server.c/h`
5. Create `gdl_lsp_semantic_tokens.c/h`
6. Create `gdl_lsp_handlers.c/h`
7. Create `main.c`
8. Create `tools/gdl_lsp/CMakeLists.txt`
9. Update root `CMakeLists.txt`
10. Build
11. Create Python test
12. Run test and fix issues
