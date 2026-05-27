[![C/C++ CI](https://github.com/ChrisNisbet01/easy_pc/actions/workflows/c-cpp.yml/badge.svg)](https://github.com/ChrisNisbet01/easy_pc/actions/workflows/c-cpp.yml)
# easy_pc: A C-based Parser Combinator Library

`easy_pc` is a lightweight, embeddable parser combinator library written in C. It provides a set of small, composable functions (parsers) that can be combined to build more complex parsers. This approach allows for the construction of grammar definitions directly in C code, making it easy to create parsers for various domain-specific languages, configuration files, or other structured text formats.

The library focuses on:
- **Parser Combinators:** Building complex parsing logic from simple, reusable primitives.
- **Concrete Parse Tree (CPT) Generation:** Producing a detailed tree representation of the parsed input, showing how each parser contributed to the match.
- **Abstract Syntax Tree (AST) Support:** Providing mechanisms for attaching semantic actions to parsers, facilitating the transformation of the CPT into a more abstract AST suitable for interpretation or compilation.

## Building the Project

`easy_pc` uses CMake for its build system.

**Prerequisites:**
-   A C compiler (e.g., GCC, Clang)
-   CMake (version 3.10 or higher recommended)

**Steps to build:**

1.  **Clone the repository (if you haven't already):**
    ```bash
    git clone <repository_url>
    cd easy_pc
    ```

2.  **Create a build directory and navigate into it:**
    ```bash
    mkdir build
    cd build
    ```

3.  **Run CMake to configure the project:**
    ```bash
    cmake ..
    ```
    *   For multi-configuration generators (like Visual Studio), you might need to specify a generator:
        `cmake -G "Visual Studio 16 2019" ..`

4.  **Build the project:**
    ```bash
    cmake --build .
    ```
    *   Alternatively, for Unix Makefiles:
        `make`

## AST Builder API

The `easy_pc` library includes a powerful and flexible API for transforming Concrete Parse Trees (CPTs) into Abstract Syntax Trees (ASTs). This callback-driven system simplifies memory management and tree traversal, allowing developers to define custom semantic actions for each grammar rule. This approach facilitates the creation of language-specific ASTs suitable for further processing, interpretation, or compilation.

[Learn more about the AST Builder API here.](AST_BUILDER_API.md)

## Build Options (Examples and Tests)

You can customize the build process using CMake options:

-   `BUILD_EXAMPLES`: Controls whether example executables are built.
    -   Default: `ON`
    -   To disable: `cmake -DBUILD_EXAMPLES=OFF ..`
-   `BUILD_TESTS`: Controls whether unit tests are built.
    -   Default: `ON`
    -   To disable: `cmake -DBUILD_TESTS=OFF ..`
-   `WITH_INPUT_STREAM_SUPPORT`: Enables support for streaming input from file descriptors (requires pthreads on Linux).
    -   Default: `ON`
    -   To disable: `cmake -DWITH_INPUT_STREAM_SUPPORT=OFF ..`

To configure with specific options, run CMake like this from your `build` directory:

```bash
cmake -DBUILD_EXAMPLES=OFF -DBUILD_TESTS=ON ..
```

After building, browse the generated API documentation in `docs/help/html/index.html` (open in any web browser). This includes detailed function descriptions from the header comments.

## Getting Started: Bare Minimum Usage

Here's a minimal example demonstrating how to parse a simple string:

```c
#include "easy_pc/easy_pc.h"
#include <stdio.h>
#include <stdlib.h> // For malloc, free
#include <string.h> // For strlen

int main() {
    const char* input_string = "hello world";
    epc_parser_list * list = epc_parser_list_create();
    if (list == NULL)
    {
        fprintf(stderr, "Failed to create parser list.\n");
        return 1;
    }

    // 1. Define a simple parser (e.g., to match the string "hello") using _l helpers
    epc_parser_t* p_hello = epc_string(list, "hello_literal", "hello");
    if (p_hello == NULL)
    {
        fprintf(stderr, "Failed to create 'hello' parser.\n");
        epc_parser_list_free(list);
        return 1;
    }

    // 2. Combine with an End-Of-Input parser to ensure the whole string is matched
    epc_parser_t* p_eoi_marker = epc_eoi(list, "eoi_marker");
    if (p_eoi_marker == NULL)
    {
        fprintf(stderr, "Failed to create EOI parser.\n");
        epc_parser_list_free(list);
        return 1;
    }

    epc_parser_t* p_full_match = epc_and(2, p_hello, p_eoi_marker);
    if (p_full_match == NULL)
    {
        fprintf(stderr, "Failed to create 'full_match' parser.\n");
        epc_parser_list_free(list);
        return 1;
    }

    // 3. Parse the input string
    epc_parse_session_t session = epc_parse_input(p_full_match, input_string);

    // 4. Check the result
    if (!session.result.is_error)
    {
        fprintf(stdout, "Parse successful!\n");
        fprintf(stdout, "Matched: '%.*s'\n", (int)session.result.data.success->len, session.result.data.success->content);
        
        // Optionally, print the Concrete Parse Tree (CPT) for debugging
        char* cpt_str = epc_cpt_to_string(session.internal_parse_ctx, session.result.data.success);
        if (cpt_str != NULL)
        {
            fprintf(stdout, "--- CPT ---\n%s\n-----------\n", cpt_str);
            free(cpt_str);
        }
    }
    else
    {
        fprintf(stderr, "Parse failed!\n");
        if (session.result.data.error) {
            fprintf(stderr, "Error: %s at '%.*s' (expected '%s', found '%.*s')\n",
                    session.result.data.error->message,
                    (int)(input_string + strlen(input_string) - session.result.data.error->input_position),
                    session.result.data.error->input_position,
                    session.result.data.error->expected ? session.result.data.error->expected : "N/A",
                    (int)(session.result.data.error->found ? strlen(session.result.data.error->found) : 3), // max 3 for EOF
                    session.result.data.error->found ? session.result.data.error->found : "EOF"
                   );
        }
    }

    int exit_code = session.result.is_error ? 1 : 0;
    // 5. Clean up resources
    epc_parse_session_destroy(&session); // Frees CPT nodes or error
    epc_parser_list_free(list); // Free the parser list

    return exit_code;
}
```

## Streaming Input Support

`easy_pc` supports parsing from streaming sources (like sockets or pipes) where data arrives incrementally. When built with `WITH_INPUT_STREAM_SUPPORT=ON` (the default), you can use `epc_parse_fd()` to parse directly from a Linux file descriptor.

The library uses a producer-consumer model: the main thread reads from the file descriptor and fills an internal buffer, while a dedicated parsing thread processes the data as it becomes available. "Greedy" parsers (like integers and doubles) are designed to block and wait for more data if the input stream ends prematurely, ensuring they only match complete tokens.

### Reactive (Non-Blocking) Streaming

For event-driven applications (e.g., using `poll`, `epoll`, or `libuv`), `easy_pc` provides a **Reactive Streaming** mode. This allows the parser to run in the background while your main thread remains non-blocking.

1.  **Initiate:** Use `epc_parse_fd_reactive()` to start a session. It returns immediately and automatically sets the file descriptor to `O_NONBLOCK`.
2.  **Notify:** Call `epc_streaming_notify_readable()` when your event loop detects the FD is ready for reading.
3.  **Callback:** Provide an `on_complete` callback that the library will invoke when a parse finishes. 
    *   **Note:** This callback is executed in the **background consumer thread**. Use it to signal your main thread (e.g., by writing to a pipe) that a result is ready.
4.  **Sync Result:** Call `epc_parse_session_sync_result()` in your main thread to move the parse result from internal storage into your session. This call transfers ownership of the result details to the session for processing.
5.  **Advance:** Use `epc_parse_session_advance()` to parse a sequence of objects from the same stream without destroying the context.

See the [Reactive Streaming Example](examples/reactive_streaming/) for a complete implementation using `poll()`.

## Using Parser List Helper Functions (the `_l` functions)

The `easy_pc` library provides convenience helper functions, denoted by an `_l` suffix (e.g., `epc_char_l`, `epc_string_l`), which combine the creation of a parser with automatically adding it to an `epc_parser_list`. This helps manage memory for parsers, especially when building complex grammars with many intermediate parser objects.

All `_l` functions take an `epc_parser_list * list` as their first argument, followed by the arguments of their non-`_l` counterpart.

**Example:**

Instead of:
```c
epc_parser_t* my_char_parser = epc_char("my_char", 'A');
epc_parser_list_add(list, my_char_parser);
```

You can simply use:
```c
epc_parser_t* my_char_parser = epc_char(list, "my_char", 'A');
```

## Installation

To install the library and tools:

```bash
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=/usr/local ..
cmake --build .
cmake --install .
```

This will install:
- The shared and static libraries.
- All public headers.
- The `gdl_compiler` and `gdl_debug` tools.
- A `pkg-config` file for easy compilation.
- HTML documentation.

### Compiling Against `easy_pc`

You can use `pkg-config` to get the correct compiler flags:

```bash
gcc my_app.c $(pkg-config --cflags --libs easy_pc) -o my_app
```

## Documentation

After installation, full HTML documentation is available at `/usr/local/share/doc/easy_pc`.

## Example Applications

The `easy_pc` library comes with several example applications demonstrating its usage and capabilities. You can explore their source code to understand how to build parsers for different scenarios.

*   **GDL Compiler:** A tool located in `tools/gdl_compiler/` that allows you to define grammars using the Grammar Definition Language (GDL) and automatically generates C parser code. This simplifies the process of creating complex parsers without writing boilerplate `easy_pc` code by hand.
    *   [Learn more about the GDL Language and its usage here.](docs/GDL_Language_Description.md)

*   **Arithmetic Parser:** Located in [examples/arithmetic_parser/](examples/arithmetic_parser/)
    This example demonstrates how to build a parser for simple arithmetic expressions, including support for basic operations (+, -, *, /), parentheses, and integer/double numbers. It also includes an AST builder and evaluator.

*   **JSON Parser:** Located in [examples/json_parser/](examples/json_parser/)
    This example showcases how to create a parser for a subset of the JSON specification. It illustrates parsing complex data structures like objects and arrays, and handling string, number, boolean, and null literals.

*   **C Parser:** Located in [examples/c_parser/](examples/c_parser/)
    A more advanced example that implements a substantial subset of the C programming language (C89/C99). It demonstrates handling complex recursive declarators, function pointers, designated initializers, and uses the GDL compiler for its grammar definition. It also includes support for typedefs using transactional callbacks.

*   **Reactive Streaming Parser:** Located in [examples/reactive_streaming/](examples/reactive_streaming/)
    Demonstrates the non-blocking reactive streaming API. It shows how to integrate the parser with a `poll()` loop and use a completion callback to process a sequence of packets from a single stream.

*   **Python Parser:** Located in [examples/python_parser/](examples/python_parser/)
    A two-stage Python parser that uses the library's token-list and reparse API to handle Python's context-sensitive grammar and significant indentation. Stage 1 tokenizes raw source into a flat list of typed tokens (keywords, operators, NEWLINE/INDENT/DEDENT markers). Stage 2 then reparses that token list with a separate grammar to build an AST — this separation is essential because Python's lexical structure (indentation-based block delimiting) must be resolved before meaningful syntactic analysis can begin. The AST grammar is defined in GDL and uses semantic action callbacks to construct a custom node tree, which is then printed back as reconstructed source code to verify correctness.
