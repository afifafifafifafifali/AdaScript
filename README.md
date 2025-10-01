# AdaScript

AdaScript is a small, C-style interpreted language with Python-like ergonomics, implemented in C++17. It ships with data structures, algorithms, HTTP utilities, filesystem helpers, and simple imports — made in loving memory of Ada Lovelace.

- C API for embedding: see docs/C_API.md
- Filesystem API reference: see docs/FS_API.md
- Language guide: see docs/Language.md
- Standard library overview: see docs/StdLib.md (includes proc.exec for command execution)
- HTTP and content APIs: see docs/HTTP_CONTENT.md
- Build guide: see docs/BUILD.md (HTTP on Linux requires libcurl dev; otherwise HTTP is disabled at runtime)
- Examples index: see docs/Examples.md

## Features

- C-style syntax: braces + semicolons
- Dynamic types: number, string, bool, null, list, dict, function, class/instance
- Multiple assignment: `a, b, c = [1, 2, 3];`
- Functions and classes (`this`), structs, and unions
- Lists and dicts with indexing and property access
- Control flow: `if/else`, `while`, Python-like `for (x in iterable)`
- Imports: `import "path";` relative to current script, cached
- Builtins: `print, input, len, map, range, int, float, str, split, join, abs, has`
- Standard libraries:
  - Data structures: `Stack, Queue, Deque, LRUCache`
  - Algorithms: `gcd, lcm, binary_search, quicksort, bfs, dfs`
  - HTTP: `requests.get/post/request` (http/https, plus file://)
  - Filesystem: `fs.read_text/write_text/exists/listdir/mkdirs/remove`
  - Content retrieval: `content.get()` for http(s), file://, or local paths
  - Input helper: `list_input(prompt, sep?, type?)`
- Logical operators (including textual `not/and/or`) with proper short‑circuiting

## Quick start

Build (Windows, Ninja):

- cmake -S . -B build -G Ninja
- cmake --build build --config Release

Build (Linux/WSL, Ninja):

- Prereqs: libcurl dev (e.g., Ubuntu: `sudo apt-get install libcurl4-openssl-dev ca-certificates`)
- cmake -S . -B build -G Ninja
- cmake --build build --config Release

Artifacts:

- Shared library: build/ (adascript_core.dll on Windows; libadascript_core.so on Linux)
- CLI executable: build/adascript(.exe)

Run:

- Windows: .\\build\\adascript.exe path\\to\\script.ad
- Linux/WSL: ./build/adascript path/to/script.ad

Embed from C:

- Include header: include/AdaScript.h
- Link: adascript_core (import library on Windows, .so on Linux)
- Example: see docs/C_API.md

See full docs in documentation.md.

## Language overview

Variables

- let x = 10; x = x + 1;

Functions

- func add(a, b) { return a + b; }

Lists and dicts

- let xs = [1, 2, 3];
- let d = {"a": 1, "b": 2}; d.a = d.a + 1;

Classes

- class Point { func init(x, y) { this.x = x; this.y = y; } func sum() { return this.x + this.y; } }
- let p = Point(1, 2); print(p.sum());

Control flow

- if (cond) { ... } else { ... }
- while (cond) { ... }
- for (i in [1,2,3]) { ... }
- for (k in {"a":1, "b":2}) { print(k); }

Imports

- ``import "builtins/libs"; // Stack, Queue, Deque, LRUCache, algorithms``

I/O and utilities

- ``let name = input("Your name: ");``
- // Python-like: multi-assign + split + map
- let a, b, c = map(int, input("Enter three ints ").split());
- // Or use list_input (auto sep and typing)
- let xs = list_input("Enter numbers: ");

## HTTP, FS, and content(Ya'll linux programmers like me who are not lazy,can imporve requests for linux systems)

HTTP (requests)

- let r = requests.get("https://example.org/");
- let p = requests.post("https://httpbin.org/post", "x=1", {"Content-Type":"application/x-www-form-urlencoded"});
- let any = requests.request("GET", "https://example.org/");

Filesystem (fs)

- fs.mkdirs("C:/tmp/as");
- fs.write_text("C:/tmp/as/note.txt", "hello");
- print(fs.read_text("C:/tmp/as/note.txt"));
- print(fs.exists("C:/tmp/as"), fs.listdir("C:/tmp/as"));
- fs.remove("C:/tmp/as");

Content retrieval (content)

- let c1 = content.get("https://example.org/");
- let c2 = content.get("file://C:/Windows/win.ini");
- let c3 = content.get("C:/Windows/win.ini");
- print(c1.ok, len(c1.text));

## Examples

- examples/hello.ad – language basics
- examples/smoke_libs.ad – data structures + algorithms
- examples/test_http.ad – HTTP GET
- examples/test_post_and_request.ad – POST + request
- examples/test_fs_content.ad – filesystem and content.get

## Notes

- Windows build uses WinHTTP for network access. file:// is supported.
- Errors surface as "Runtime error: ..." with a message.
- See documentation.md for a deeper tour and API details.
