# AdaScript C API

This document describes the public C API for embedding the AdaScript VM in a C/C++ program. Include headers from `include/` and link with the shared library `adascript_core`.

- Header: include/AdaScript.h
- Windows link: adascript_core.lib with adascript_core.dll at runtime
- Linux/WSL link: -ladascript_core (ensure libadascript_core.so is in runtime loader path)

## Initialization and teardown

- AdaScriptVM* AdaScript_Create(const char* entry_dir)
  - Creates a VM instance. If `entry_dir` is non-null, it sets the base directory used for relative imports; otherwise the current working directory is used.
  - Returns NULL on failure.

- void AdaScript_Destroy(AdaScriptVM* vm)
  - Destroys a VM and frees its resources.

## Evaluating and running code

- int AdaScript_Eval(AdaScriptVM* vm, const char* source, const char* filename, char** error_message)
  - Parses and executes AdaScript source code from memory.
  - `filename` is optional (used only for error context and to adjust `entry_dir` for relative imports when non-null).
  - Returns 0 on success. On error, returns non-zero and, if `error_message` is not NULL, sets `*error_message` to a malloc-allocated message. Free it with `AdaScript_FreeString`.

- int AdaScript_RunFile(AdaScriptVM* vm, const char* path, char** error_message)
  - Loads, parses, and executes a script from disk.
  - Returns 0 on success. On error, non-zero and sets `*error_message` like `Eval`.

## Calling AdaScript functions from C

- char* AdaScript_Call(AdaScriptVM* vm, const char* func_name, const char* const* args, int argc, char** error_message)
  - Calls a global callable (function or class) by name with string arguments. Each C string is passed to AdaScript as a string.
  - On success, returns a malloc-allocated C string representation of the return value (using AdaScript `str` semantics). You must free the returned string with `AdaScript_FreeString`.
  - On failure, returns NULL and sets `*error_message`.

- void AdaScript_FreeString(char* s)
  - Frees strings returned by the C API (`AdaScript_Call`, or error messages).

## Registering native functions implemented in C

- int AdaScript_RegisterNativeStringFn(AdaScriptVM* vm, const char* name, int arity, AdaScript_NativeStringFn fn, void* user_data)
  - Registers a global function in the VM named `name`.
  - `arity`: number of arguments, or -1 for variadic.
  - `fn` signature:
    - typedef char* (*AdaScript_NativeStringFn)(void* user_data, const char* const* args, int argc);
    - It must return a malloc-allocated NUL-terminated string (AdaScript will take ownership and free it). Return NULL to signal an error; AdaScript will convert that to an empty string.
  - Returns 0 on success, non-zero on error.

## Example (C)

```c
#include "AdaScript.h"
#include <stdio.h>
#include <stdlib.h>

static char* native_concat(void* user, const char* const* args, int argc) {
    size_t len = 0; for (int i = 0; i < argc; ++i) for (const char* p=args[i]; *p; ++p) ++len;
    char* out = (char*)malloc(len + 1); if (!out) return NULL;
    char* w = out; for (int i = 0; i < argc; ++i) { const char* s=args[i]; while (*s) *w++ = *s++; } *w = '\0';
    return out;
}

int main(void) {
    AdaScriptVM* vm = AdaScript_Create(NULL);
    if (!vm) return 1;

    if (AdaScript_RegisterNativeStringFn(vm, "c_concat", -1, native_concat, NULL) != 0) return 1;

    const char* src =
        "func greet(name) { return \"Hello, \" + name; }\n"
        "print(greet(\"Ada\"));\n";
    char* err = NULL;
    if (AdaScript_Eval(vm, src, "inline", &err) != 0) {
        fprintf(stderr, "Eval error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
    }

    const char* args[] = { "World" };
    char* res = AdaScript_Call(vm, "greet", args, 1, &err);
    if (res) { printf("%s\n", res); AdaScript_FreeString(res); }
    else { fprintf(stderr, "Call error: %s\n", err ? err : "(unknown)"); AdaScript_FreeString(err); }

    AdaScript_Destroy(vm);
    return 0;
}
```

## Linking

- Windows (MinGW or MSVC)
  - Headers: -I include
  - Link: adascript_core.lib (ensure adascript_core.dll is beside your executable or in PATH)
- Linux/WSL
  - Headers: -I include
  - Link: -ladascript_core (ensure libadascript_core.so is in a searchable path, e.g. LD_LIBRARY_PATH)
