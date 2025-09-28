/*
AdaScript Native Concat Plugin
(C) 2025 Afif Ali Saadman, Chief Author of AdaScript
License: LGPL
*/
#include "AdaScript.h"
#include <stdlib.h>

static char* c_concat(void* user, const char* const* args, int argc){
    (void)user;
    size_t len=0; for(int i=0;i<argc;++i){ for(const char* p=args[i]; *p; ++p) ++len; }
    char* out=(char*)malloc(len+1); if(!out) return NULL; char* w=out;
    for(int i=0;i<argc;++i){ const char* s=args[i]; while(*s) *w++=*s++; }
    *w='\0'; return out;
}

// Exported plugin init
#ifdef _WIN32
__declspec(dllexport)
#endif
int AdaScript_ModuleInit(AdaScript_RegisterFn reg, void* host_ctx){
    (void)host_ctx;
    reg("plugin_concat", -1, c_concat, NULL);
    return 0;
}
