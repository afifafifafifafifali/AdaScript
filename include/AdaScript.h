/*
AdaScript
(C) 2025 Afif Ali Saadman, Chief Author of AdaScript
License: LGPL
*/
#ifndef ADASCRIPT_H
#define ADASCRIPT_H

#ifdef __cplusplus
extern "C" {
#endif

// Export macro
#if defined(_WIN32)
  #if defined(ADASCRIPT_BUILD_DLL)
    #define ADASCRIPT_API __declspec(dllexport)
  #else
    #define ADASCRIPT_API
  #endif
#else
  #define ADASCRIPT_API
#endif

// Opaque VM handle
typedef struct AdaScriptVM AdaScriptVM;

// Create a VM. entry_dir may be NULL; if provided, it's used as the base for relative imports.
ADASCRIPT_API AdaScriptVM* AdaScript_Create(const char* entry_dir);

// Destroy a VM created with AdaScript_Create.
ADASCRIPT_API void AdaScript_Destroy(AdaScriptVM* vm);

// Evaluate source code. filename is optional and used for error messages; may be NULL.
// Returns 0 on success, non-zero on error. On error, *error_message is set to a malloc-allocated string
// that must be freed with AdaScript_FreeString.
ADASCRIPT_API int AdaScript_Eval(AdaScriptVM* vm, const char* source, const char* filename, char** error_message);

// Run a file from disk. Returns 0 on success; on error, sets *error_message similarly to Eval.
ADASCRIPT_API int AdaScript_RunFile(AdaScriptVM* vm, const char* path, char** error_message);

// Call a global function by name with string arguments.
// - args: array of C strings of length argc. They will be passed to AdaScript as strings.
// - On success, returns a malloc-allocated C string representation of the return value (using AdaScript str semantics),
//   which must be freed with AdaScript_FreeString. On error, returns NULL and sets *error_message.
ADASCRIPT_API char* AdaScript_Call(AdaScriptVM* vm, const char* func_name, const char* const* args, int argc, char** error_message);

// Register a native function that receives C string arguments and returns a malloc-allocated C string.
// The returned string will be freed by the VM after converting to an AdaScript string.
// Your callback may use user_data to carry context; it must be thread-safe if you call from multiple threads.
// Return value must be a malloc-allocated NUL-terminated string, or NULL to indicate error. If returning NULL,
// you should also set an error string via an out-of-band mechanism if desired; the VM will return an empty string.
typedef char* (*AdaScript_NativeStringFn)(void* user_data, const char* const* args, int argc);
ADASCRIPT_API int AdaScript_RegisterNativeStringFn(AdaScriptVM* vm, const char* name, int arity, AdaScript_NativeStringFn fn, void* user_data);

// Free strings returned by AdaScript_Call or provided in *error_message.
ADASCRIPT_API void AdaScript_FreeString(char* s);

#ifdef __cplusplus
} // extern "C"
#endif

#endif // ADASCRIPT_H
