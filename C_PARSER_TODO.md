# C Parser TODO List

## 1. Control Flow Statements
- [x] `for` loop
- [x] `switch` block, `case`, `default`
- [x] `do-while` loop
- [x] Jump statements: `goto`, `break`, `continue`
- [x] Labeled statements (e.g., `start:`)

## 2. User-Defined Types & Initializers
- [x] Enumerations (`enum`)
- [x] Initializer lists (e.g., `int a[] = {1, 2, 3};`)
- [ ] Designated initializers (C99)
- [x] Bit-fields in structs/unions

## 3. Complex Declarators
- [ ] Nested declarators (Function pointers: `int (*f)(void)`)
- [ ] Recursive declarator nesting (e.g., `int (*a[10])(int)`)
- [ ] Advanced array sizing (C99 `static`, `const`, `*`)

## 4. Expression Polish
- [x] Remaining compound assignments: `*=`, `/=`, `%=`, `<<=`, `>>=`, `&=`, `^=`, `|=`
- [x] Full Literal support:
    - [x] Hexadecimal and Octal integers
    - [x] Floating-point literals
    - [x] String literals (for use in expressions)
    - [x] Character constants
- [x] Proper Comma operator (as a binary operator in the hierarchy)

## 5. Modern C (C99/C11)
- [ ] `inline` keyword
- [ ] `restrict` keyword
- [ ] `_Bool`, `_Complex`, `_Imaginary`
- [ ] Variable Length Arrays (VLA)
- [ ] Variadic macros (preprocessor)
