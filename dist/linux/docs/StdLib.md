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
