# AdaScript Language Guide

This guide covers syntax and core semantics of AdaScript.

## Basics

- Statements end with a semicolon `;`.
- Dynamic types: number (double), string, bool, null, list, dict, function, class/instance.
- Variables: `let name = expr;` or `let a, b, c = [1, 2, 3];`
- Multiple assignment supports unpacking from lists. Uninitialized `let x;` defines `x` as `null`.

## Literals

- Numbers: `42`, `3.14`
- Strings: `"hello"`
- Booleans: `true`, `false`
- Null: `null`
- Lists: `[1, 2, 3]`
- Dicts: `{ "a": 1, "b": 2 }`

## Expressions

- Arithmetic: `+ - * / %`
- Comparisons: `< <= > >= == !=`
- Logical: `!` (not), `&&` (and), `||` (or)
  - Textual synonyms also supported: `not`, `and`, `or`, and `equals` (==)
- Grouping: `(expr)`
- Indexing: `list[i]`, `dict["key"]`
- Property access: `obj.prop` or `dict.prop` (dot on dict mirrors `dict["prop"]`)

## Variables and Assignment

- `let x = 10; x = x + 1;`
- Multi-assign: `a, b, c = [1, 2, 3];`
- Index assignment: `xs[i] = v; d["k"] = v; obj.list[i] = v; obj.dict["k"] = v;`

## Control Flow

- If/else:
  ```ad
  if (x > 0) { print("pos"); } else { print("non-pos"); }
  ```
- While:
  ```ad
  while (cond) { ... }
  ```
- For-in (lists, dict keys, strings):
  ```ad
  for (i in [1,2,3]) { print(i); }
  for (k in {"a":1, "b":2}) { print(k); }
  for (ch in "abc") { print(ch); }
  ```

## Functions

- Define function:
  ```ad
  func add(a, b) { return a + b; }
  ```
- Call: `add(1, 2);`
- Return: `return expr;` or bare `return;` (returns null)

## Classes and Instances

- Define class with methods; `init` is the constructor:
  ```ad
  class Point {
    func init(x, y) { this.x = x; this.y = y; }
    func sum() { return this.x + this.y; }
  }
  let p = Point(3, 9); print(p.sum());
  ```

## Structs and Unions (lightweight metadata)

- Struct:
  ```ad
  struct Person { name; age; }
  ```
- Union:
  ```ad
  union Shape { Circle; Square; }
  ```

## Imports

- Import a file relative to the running script's directory:
  ```ad
  import "../builtins/libs";
  ```

## Builtins (selection)

- `print, input, len, map, range, int, float, str, split, join, abs, has`
- String instance method: `"a b c".split()` or `"a,b".split(",")`

## Error Handling

- Runtime errors surface as: `Runtime error: message`

