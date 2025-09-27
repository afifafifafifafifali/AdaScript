/*
AdaScript
(C) 2025 Afif Ali Saadman, Chief Author of AdaScript
License: LGPL
*/
#include "AdaScript.h"
#include <stdio.h>
#include <stdlib.h>

// Native function: concatenate all args
static char* c_concat(void* user, const char* const* args, int argc) {
    (void)user;
    size_t len = 0;
    for (int i = 0; i < argc; ++i) {
        for (const char* p = args[i]; *p; ++p) ++len;
    }
    char* out = (char*)malloc(len + 1);
    if (!out) return NULL;
    char* w = out;
    for (int i = 0; i < argc; ++i) {
        const char* s = args[i];
        while (*s) *w++ = *s++;
    }
    *w = '\0';
    return out;
}

// Optional: expose a version string
static char* c_version(void* user, const char* const* args, int argc) {
    (void)user; (void)args; (void)argc;
    const char* v = "native-api:1.0";
    size_t n = 0; while (v[n]) ++n;
    char* out = (char*)malloc(n + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < n; ++i) out[i] = v[i];
    out[n] = '\0';
    return out;
}

int main(void) {
    AdaScriptVM* vm = AdaScript_Create(NULL); // base dir = current working dir
    if (!vm) { fprintf(stderr, "Failed to create AdaScript VM\n"); return 1; }

    // Register native functions as globals
    if (AdaScript_RegisterNativeStringFn(vm, "c_concat", -1, c_concat, NULL) != 0) {
        fprintf(stderr, "Failed to register c_concat\n");
        AdaScript_Destroy(vm);
        return 1;
    }
    (void)AdaScript_RegisterNativeStringFn(vm, "c_version", 0, c_version, NULL);

    // Prelude: define an AdaScript function that calls into our native
    const char* prelude =
        "func greet(name) { return c_concat(\"Hello, \", name); }\n"
        "// expose a tiny namespace (dict keys must be strings)\n"
        "let native = { \"concat\": c_concat };\n"
        "print(\"[prelude]\");\n";
    char* err = NULL;
    if (AdaScript_Eval(vm, prelude, "prelude", &err) != 0) {
        fprintf(stderr, "Eval error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
    }

    // Run a file that uses c_concat (path relative to current working dir)
    // Example expected at .\\testings\\concat_native.ad
    (void)AdaScript_RunFile(vm, ".\\testings\\concat_native.ad", &err);
    if (err) { fprintf(stderr, "RunFile warning: %s\n", err); AdaScript_FreeString(err); err = NULL; }

    // Call an AdaScript function from C
    const char* argv1[] = { "World" };
    char* res = AdaScript_Call(vm, "greet", argv1, 1, &err);
    if (!res) {
        fprintf(stderr, "Call error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
  }
  printf("C->AdaScript greet(): %s\n", res);
  AdaScript_FreeString(res);

  AdaScript_Destroy(vm);
  return 0;
}