# AdaScript Documentation

AdaScript is a small interpreted language with built‑in data structures, algorithms, HTTP client utilities, filesystem helpers, and simple module importing. This document covers installation, usage, language basics, and the standard library currently provided.

## Build and Run

- Prereqs: CMake (>=3.15), Ninja, C++13 compiler (Windows supported), WinHTTP available on Windows (linked automatically).
- Build (Windows, Ninja):
  - cmake -S . -B build -G Ninja
  - cmake --build build --config Release
- Run a script:
  - .\build\adascript.exe path\to\script.ad

## Language Overview

- Statements end with a semicolon ;
- Variables with let:
  ```ad
  let x = 42;
  let s = "hello";
  ```
- Lists and dicts:
  ```ad
  let xs = [1, 2, 3];
  let d = {"a": 1, "b": 2};
  ```
- Functions and classes:
  ```ad
  func add(a, b) { return a + b; }

  class Point {
    func init(x, y) { this.x = x; this.y = y; }
    func sum() { return this.x + this.y; }
  }
  ```
- Control flow:
  ```ad
  if (x > 0) { print("pos"); } else { print("non-pos"); }
  while (x > 0) { x = x - 1; }
  for (i in [1,2,3]) { print(i); }
  ```
- Import modules (relative to current file):
  ```ad
  import "../builtins/libs";
  ```

## Standard Library (Builtins)

Available globally unless namespaced.

- print(...): prints values
- len(x): length of list/string/dict
- input(prompt?): reads a line from stdin
- map(func, list): returns a new list
- range([start,] stop [, step]): Python-like range into list
- int(x), float(x), str(x)
- split(string[, sep]) -> list of strings
- join(list, sep) -> string
- abs(x)
- has(dict, key) -> bool

## Data Structures (builtins)

Import all common structures and algorithms:

```ad
import "../builtins/libs";
```

- Stack
  ```ad
  let st = Stack(); st.push(1); st.push(2); print(st.pop());
  ```
- Queue (amortized O(1))
  ```ad
  let q = Queue(); q.push(10); q.push(20); print(q.pop());
  ```
- Deque
  ```ad
  let dq = Deque(); dq.push_front(1); dq.push_back(2); print(dq.pop_back());
  ```
- LRUCache(capacity)
  ```ad
  let cache = LRUCache(2); cache.put("a",1); print(cache.get("a"));
  ```

## Algorithms (builtins)

Provided in builtins/algorithms.ad and included via builtins/libs.

- gcd(a,b), lcm(a,b)
- binary_search(sorted_list, target)
- quicksort(list) (in-place)
- Graph: bfs(graph, start), dfs(graph, start) with graph modeled as dict: { node: [neighbors] }

Example:

```ad
import "../builtins/libs";
let g = {"1":["2","3"], "2":["4"], "3":["4"], "4":[]};
print("bfs:", bfs(g, "1"));
print("dfs:", dfs(g, "1"));
```

## HTTP Client (requests)

Namespace: requests

- requests.get(url) -> {status, text, headers}
- requests.post(url, data?, headers?) -> {status, text, headers}
- requests.request(method, url, data?, headers?) -> {status, text, headers}

Notes:

- http/https supported (WinHTTP on Windows). On Linux/WSL, libcurl is used if available; if libcurl is missing, HTTP is disabled and requests.* throws: "HTTP disabled: libcurl not available in this build". file:// is supported on both.

Examples:

```ad
let r = requests.get("https://example.org/");
print(r.status, len(r.text));
let p = requests.post("https://httpbin.org/post", "x=1", {"Content-Type":"application/x-www-form-urlencoded"});
print(p.status, len(p.text));
```

## Filesystem (fs)

Namespace: fs

- fs.read_text(path) -> string
- fs.write_text(path, text) -> true
- fs.exists(path) -> bool
- fs.listdir(path) -> list of names
- fs.mkdirs(path) -> true (creates parent directories)
- fs.remove(path) -> number of entries removed (recursive for directories)

## Process execution (proc)

Namespace: proc

- proc.exec(cmd) -> { status, out }
  - Executes cmd via the host shell and returns exit status and combined stdout+stderr.

Example:

```ad
let dir = "C:/tmp/as"; if (!fs.exists(dir)) { fs.mkdirs(dir); }
let p = dir + "/note.txt";
fs.write_text(p, "hello");
print(fs.read_text(p));
print(fs.listdir(dir));
fs.remove(dir);
```

## Content Retrieval (content)

Namespace: content

- content.get(source) -> dict with fields
  - ok: bool
  - status: number (200 on success)
  - text: content string
  - type: "http" or "file"
  - source: original string
  - error: string if failed

Supports http(s)://, file://, or a local filesystem path.

Example:

```ad
let r1 = content.get("https://example.org/");
let r2 = content.get("file://C:/Windows/win.ini");
let r3 = content.get("C:/Windows/win.ini");
print(r1.ok, len(r1.text));
print(r2.ok, len(r2.text));
print(r3.ok, len(r3.text));
```

## Input Helpers(You can use these if you want,not recommended :D)

- list_input(prompt, sep?, type?): Read a line and convert to a list.
  - sep: default auto-detects comma vs whitespace
  - type: "auto" (default), "int", "float", or "str"

Examples:

```ad
let nums = list_input("Enter numbers: ");               // whitespace or comma; auto cast
let words = list_input("Words (comma): ", ",", "str");  // comma-separated strings
```

## Examples in repo

- examples/hello.ad – arithmetic, map/range, functions, loops
- examples/smoke_libs.ad – Stack/Queue/Deque, gcd/lcm, binary_search, quicksort, bfs, dfs
- examples/test_http.ad – HTTP GET
- examples/test_post_and_request.ad – POST + generic request
- examples/test_fs_content.ad – Filesystem and content.get

Run one:

```bash
# Windows PowerShell
.\build\adascript.exe .\examples\smoke_libs.ad
```

## Notes and Limitations

- This build targets Windows and uses WinHTTP for network operations.
- Interpreter supports short-circuit logical operators (and/or) and indexing on lists/dicts.
- Error handling raises "Runtime error: ..." with a message.

## A Sample code for a fun problem


```ada
// Codeforces: Greedy Grid (2122A)

let t = int(input(""));  
for (_ in range(t)) {
   let n, m = map(int, input("").split());
    if (n == 1 or m == 1 or (n == 2 and m == 2)) {
       print("NO");
    } else {
        print("YES");
    }
}

```


## License

LGPL
