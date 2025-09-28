# Examples

This document lists example scripts included in the repository and how to run them with the CLI.

## Running examples

Windows PowerShell:
```
./build/adascript.exe ./examples/hello.ad
```

Linux/WSL:
```
./build/adascript ./examples/hello.ad
```

## Example list

- Command execution demo: testings/proc_demo.ad

- examples/hello.ad – language basics (arithmetic, map/range, functions, loops)
- examples/smoke_libs.ad – Stack/Queue/Deque, gcd/lcm, binary_search, quicksort, bfs, dfs
- examples/test_http.ad – HTTP GET request
- examples/test_post_and_request.ad – HTTP POST and generic request
- examples/test_fs_content.ad – filesystem helpers and content.get

## Embedding C example

See docs/C_API.md for a standalone C snippet. A quick compile with MinGW on Windows or GCC on Linux:

Windows (MinGW):
```
gcc -Iinclude tests/c_api_test.c -Lbuild -ladascript_core -o build/c_api_test.exe
./build/c_api_test.exe
```

Linux/WSL:
```
cc -Iinclude tests/c_api_test.c -Lbuild -ladascript_core -o build/c_api_test
./build/c_api_test
```
