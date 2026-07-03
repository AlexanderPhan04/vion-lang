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

## Completed in v0.3

- **Arrays:** literal `[1, 2, 3]`, indexing `arr[i]`, mutation `arr[i] = val`, negative indexing.
- **Array concatenation:** `arr1 + arr2`.
- **String indexing:** `str[i]` and negative indexing.
- **For-in loop:** `for item in arr { ... }` iterates arrays and strings.
- **Break and Continue:** `break` / `continue` in while and for loops.
- **Modulo operator:** `%`.
- **String escape sequences:** `\n`, `\t`, `\r`, `\\`, `\"`.
- **Built-in functions:** `len`, `push`, `pop`, `str`, `num`, `type`, `input`, `clock`, `floor`, `ceil`, `sqrt`, `abs`, `max`, `min`, `array`.
- **Source location in errors:** runtime errors now report line numbers.
- **Fixed number formatting:** integers print without `.0` (e.g., `10` not `10.0`).

## Next Milestones

### v0.4 Developer Experience

- Interactive multi-line REPL.
- Formatter for `.vion` files.
- Better parser error recovery (continue past first error).

### v0.5 Objects / Maps

- Object / dictionary type: `{ key: value }` literals.
- Property access: `obj.key`, `obj["key"]`.
- Object spread and copy.

### v0.6 Modules

- `import "path/file"` support.
- Standard library layout.
- File-level module cache.

### v0.7 Execution Engine

- Bytecode compiler.
- Virtual machine.
- Optional optimizer passes.

### v1.0 Language Stability

- Freeze core syntax.
- Publish full language reference.
- Add compatibility tests for each language feature.
