# Vion Roadmap

## Completed in v0.2

- Lexical block scope with parent environments.
- Variable reassignment with `name = expression`.
- Expression statements for function calls and assignments.
- `while` loops.
- Logical `and` / `or` with short-circuit behavior.
- `nil` value.
- User-defined functions with parameters, closures, recursion, and `return`.
- Integration test runner for CLI behavior.

## Next Milestones

### v0.3 Collections

- Arrays and indexing.
- Built-in functions such as `len`.
- Better runtime errors with source locations.

### v0.4 Developer Experience

- Interactive REPL.
- Formatter for `.vion` files.
- More detailed parser recovery instead of stopping at the first parse error.

### v0.5 Modules

- `import` support.
- Standard library layout.
- File-level module cache.

### v0.6 Execution Engine

- Bytecode compiler.
- Virtual machine.
- Optional optimizer passes.

### v1.0 Language Stability

- Freeze core syntax.
- Publish full language reference.
- Add compatibility tests for each language feature.
