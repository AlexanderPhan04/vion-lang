# Vion Language Specification v0.3

## Goal

Vion is a dynamically-typed, expression-oriented programming language built in C++17. It supports first-class functions, closures, arrays, and a standard library of built-in functions.

---

## Supported syntax

### Variables

```vion
let x = 10
let name = "Vion"
let active = true
let nothing = nil
x = x + 1
```

### Print

```vion
print x
print "hello"
print x + 20
```

### Blocks and Scope

Blocks create lexical scope:

```vion
let name = "global"

{
  let name = "block"
  print name    // block
}

print name      // global
```

### If / else

```vion
if x >= 10 {
  print "large"
} else {
  print "small"
}
```

### While

```vion
let index = 0

while index < 3 {
  print index
  index = index + 1
}
```

### For-in loop (v0.3)

Iterate over an array or a string:

```vion
let fruits = ["apple", "banana", "cherry"]

for fruit in fruits {
  print fruit
}

for ch in "hello" {
  print ch
}
```

### Break and Continue (v0.3)

```vion
let i = 0
while i < 10 {
  if i == 5 { break }
  if i % 2 == 0 {
    i = i + 1
    continue
  }
  print i
  i = i + 1
}
```

### Functions

```vion
fn add(a, b) {
  return a + b
}

print add(10, 20)  // 30
```

Functions are values and close over their parent scope:

```vion
fn makeAdder(x) {
  fn addInner(y) {
    return x + y
  }
  return addInner
}

let addTwo = makeAdder(2)
print addTwo(5)  // 7
```

### Arrays (v0.3)

```vion
let nums = [1, 2, 3]

// Access
print nums[0]   // 1
print nums[-1]  // 3  (negative index from end)

// Mutation
nums[0] = 99

// Concatenation
let more = nums + [4, 5]
print len(more)  // 5
```

### Comments

```vion
// This is a comment
```

---

## String Escape Sequences (v0.3)

| Escape | Meaning |
|--------|---------|
| `\n` | Newline |
| `\t` | Tab |
| `\r` | Carriage return |
| `\\` | Backslash |
| `\"` | Double quote |
| `\0` | Null character |

---

## Supported value types

| Type | Example |
|------|---------|
| `number` | `42`, `3.14` |
| `string` | `"hello"` |
| `boolean` | `true`, `false` |
| `function` | `fn add(a, b) { ... }` |
| `array` | `[1, 2, 3]` |
| `nil` | `nil` |

---

## Operators

- **Arithmetic:** `+ - * / %`
- **Comparison:** `> >= < <=`
- **Equality:** `== !=`
- **Logical:** `and or !`
- **Grouping:** `( )`
- **Call:** `name(arguments)`
- **Index:** `arr[i]`, `arr[i] = val`

---

## Built-in Functions (v0.3)

| Function | Description |
|----------|-------------|
| `len(val)` | Length of array or string |
| `push(arr, val)` | Append element to array, return array |
| `pop(arr)` | Remove and return last element |
| `str(val)` | Convert any value to string |
| `num(val)` | Convert string to number |
| `type(val)` | Return type name as string |
| `input(prompt)` | Read a line from stdin |
| `clock()` | Seconds since epoch (float) |
| `floor(n)` | Round down |
| `ceil(n)` | Round up |
| `sqrt(n)` | Square root |
| `abs(n)` | Absolute value |
| `max(a, b)` | Larger of two numbers |
| `min(a, b)` | Smaller of two numbers |
| `array(size, fill)` | Create array of given size filled with value |

---

## CLI

```powershell
vion main.vion              # Run a file
vion run main.vion          # Run a file
vion -r main.vion           # Run a file
vion tokens main.vion       # Print tokens
vion ast main.vion          # Print AST
vion check main.vion        # Parse-check a file
vion eval "print 1 + 2"     # Run inline source
vion repl                   # Start interactive REPL
vion version                # Show version
vion help                   # Show help
```

## Not supported yet

- Objects / maps
- Imports / modules
- Type checker
- Bytecode compiler / VM
