# Standard Library Overview

This document summarizes the standard library available in AdaScript: builtins, data structures, and algorithms.

See also:
- Filesystem: docs/FS_API.md
- HTTP and content retrieval: docs/HTTP_CONTENT.md

## Builtins

Available globally unless namespaced.

- print(...): prints values to stdout
- input(prompt?): reads a line from stdin
- len(x): length of list/string/dict
- map(func, list): returns a new list with func applied to each element
- range([start,] stop [, step]): returns a list of numbers (like Python)
- int(x), float(x), str(x): casting
- split(string[, sep]): split into list of strings
- join(list, sep): join list of strings into a single string
- abs(x): absolute value
- has(dict, key): returns true if key exists
- sqrt_bs(x): square root via binary search
- proc.exec(cmd): execute a shell command, returns { status, out }
- list_input(prompt[, sep[, type]]): reads input and parses to a list

## Data Structures (in builtins/libs)

Import them with:
```ad
import "../builtins/libs";
```

- Stack
  ```ad
  let st = Stack(); st.push(1); st.push(2); print(st.pop());
  ```
- Queue
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

## Algorithms (in builtins/libs)

- gcd(a,b), lcm(a,b)
- binary_search(sorted_list, target)
- quicksort(list) (in-place)
- Graph traversals: bfs(graph, start), dfs(graph, start)

Example:
```ad
import "../builtins/libs";
let g = {"1":["2","3"], "2":["4"], "3":["4"], "4":[]};
print("bfs:", bfs(g, "1"));
print("dfs:", dfs(g, "1"));
```

---

## Intrinsic built-ins (defined in interpreter)

The following functions are implemented directly by the AdaScript interpreter (src/main.cpp) and are available without importing the builtins directory.

Global
- print(...): variadic print to stdout
- len(x): length of list/string/dict
- input(prompt?): reads a line from stdin
- map(func, list): apply function/native function to each element and return a list
- sqrt_bs(x): square root via binary search
- range([start], stop [, step]): integer range generator
- int(x): cast to integer (number/string/bool)
- float(x): cast to float (number/string/bool)
- str(x): string representation
- split(string[, sep]): split into list of strings
- join(list, sep): join list of strings with separator
- abs(x): absolute value
- has(dict, key): true if key exists in dict
- list_input(prompt[, sep[, type]]): parse a line into a list; type in {"auto","int","float","str"}

Namespaces
- requests.get(url): HTTP/HTTPS GET (or file://) -> { status, text, headers? }
- requests.post(url, data[, headers]): POST request
- requests.request(method, url[, data[, headers]]): general request
- fs.read_text(path): read file as text
- fs.write_text(path, text): write text to file
- fs.exists(path): returns true if path exists
- fs.listdir(path): list directory names in path
- fs.mkdirs(path): create directories (recursive)
- fs.remove(path): remove file or directory tree; returns count removed
- content.get(source): fetch http(s), file://, or local path -> { ok, status, text, type, ... }
- c.run(code[, args_list]): compile+run C code with gcc (MinGW on Windows) -> { ok, compile_status, run_status, exe }
- server.serve(...): not implemented in this build (raises error)
- proc.exec(cmd): run a shell command, capture { status, out }
- native.load(path): load a native plugin (DLL/SO) exporting AdaScript_ModuleInit; registers functions into globals

String method
- s.split(sep?): string instance method; behaves like split(s, sep)

Notes
- HTTP on Windows uses WinHTTP; non-Windows optionally uses libcurl (guarded by ADASCRIPT_NO_CURL).
- content.get supports http/https/file/local fallback with structured response.
- c.run requires gcc in PATH on Windows (e.g., MinGW). On success it executes the produced binary.
- server.serve is a stub in this build.
