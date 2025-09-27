/*
AdaScript
(C) 2025 Afif Ali Saadman, Chief Author of AdaScript
License: LGPL
*/
#include "AdaScript.h"
#include <stdio.h>
#include <stdlib.h>

// Minimal smoke test of AdaScript C API
// - Registers a native function
// - Evaluates code that uses it
// - Calls an AdaScript function from C

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

int main(void) {
    AdaScriptVM* vm = AdaScript_Create(NULL);
    if (!vm) { fprintf(stderr, "VM create failed\n"); return 1; }

    if (AdaScript_RegisterNativeStringFn(vm, "c_concat", -1, c_concat, NULL) != 0) {
        fprintf(stderr, "register failed\n");
        AdaScript_Destroy(vm);
        return 1;
    }

    const char* code =
        "func greet(x) { return c_concat(\"Hello, \", x); }\n"
        "print(greet(\"Ada\"));\n";

    char* err = NULL;
    if (AdaScript_Eval(vm, code, "inline", &err) != 0) {
        fprintf(stderr, "Eval error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
    }

    const char* args[] = { "World" };
    char* res = AdaScript_Call(vm, "greet", args, 1, &err);
    if (!res) {
        fprintf(stderr, "Call error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
    }
    printf("%s\n", res);
    AdaScript_FreeString(res);

    AdaScript_Destroy(vm);
    return 0;
}
