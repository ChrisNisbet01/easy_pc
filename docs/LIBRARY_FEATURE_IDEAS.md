# easy_pc Library Feature Ideas

This document tracks proposed additions to the core `easy_pc` library to increase its utility across various languages and data formats.

## 1. Combinators

### 1.1 Flexible Delimited (`epc_delimited_flex`)
A more lenient version of the standard `delimited` combinator.
- **Logic:** `item (sep item)*`
- **Behavior:** Attempts to match a separator and a subsequent item. If the separator matches but the item fails, it backtracks over the separator and succeeds with the items matched so far.
- **Rationale:** Allows the trailing separator to be handled by the parent rule (e.g., `delimited_flex(item, ',') ','?`) or simply left in the stream without causing a hard parse error.

## 2. Terminal Parsers

### 2.1 Standard Identifier (`epc_identifier`)
A built-in parser for common programming identifiers.
- **Pattern:** `[_a-zA-Z][_a-zA-Z0-9]*`
- **Rationale:** This pattern is nearly universal in C-family languages, Python, Java, etc. Having a high-performance built-in reduces GDL boilerplate.

### 2.2 Octal Integer (`epc_octal`)
A parser for octal (base-8) numeric literals.
- **Pattern:** `0[0-7]*`
- **Rationale:** Common in system programming and standard configuration formats.

### 2.3 Hexadecimal Integer (`epc_hex`)
A parser for hexadecimal (base-16) numeric literals.
- **Pattern:** `0[xX][0-9a-fA-F]+`
- **Rationale:** Essential for representing memory addresses, color codes, and byte values.
