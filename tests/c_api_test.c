/*
AdaScript
(C) 2025 Afif Ali Saadman, Chief Author of AdaScript
License: LGPL
*/
#include "AdaScript.h"
#include <stdio.h>
#include <stdlib.h>

static char* native_concat(void* user, const char* const* args, int argc) {
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
    char* err = NULL;
    AdaScriptVM* vm = AdaScript_Create(NULL);
    if (!vm) { fprintf(stderr, "Failed to create AdaScript VM\n"); return 1; }

    if (AdaScript_RegisterNativeStringFn(vm, "c_concat", -1, native_concat, NULL) != 0) {
        fprintf(stderr, "Failed to register native fn\n");
        AdaScript_Destroy(vm);
        return 1;
    }

    const char* src =
        "func greet(name) { return \"Hello, \" + name; }\n"
        "let dir = \"./c_api_fs\"; if (!fs.exists(dir)) { fs.mkdirs(dir); }\n"
        "fs.write_text(dir + \"/note.txt\", \"ok\");\n"
        "print(fs.read_text(dir + \"/note.txt\"));\n";
    if (AdaScript_Eval(vm, src, "inline_c", &err) != 0) {
        fprintf(stderr, "Eval error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
    }

    const char* args[] = { "Ada" };
    char* res = AdaScript_Call(vm, "greet", args, 1, &err);
    if (!res) {
        fprintf(stderr, "Call error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
    }
    printf("greet(): %s\n", res);
    AdaScript_FreeString(res);

    const char* args2[] = { "Hello, ", "from ", "C!" };
    char* res2 = AdaScript_Call(vm, "c_concat", args2, 3, &err);
    if (!res2) {
        fprintf(stderr, "Call error: %s\n", err ? err : "(unknown)");
        AdaScript_FreeString(err);
        AdaScript_Destroy(vm);
        return 1;
    }
    printf("c_concat(): %s\n", res2);
    AdaScript_FreeString(res2);

    AdaScript_Destroy(vm);
    return 0;
}
