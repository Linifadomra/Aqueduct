#define _GNU_SOURCE

#include "librclone_shim.h"
#include <stdio.h>
#include <string.h>

#if defined(_WIN32)
  #include <windows.h>
  #define LIBHANDLE HMODULE
  #define DLOPEN(path)  LoadLibraryA(path)
  #define DLSYM(h, sym) GetProcAddress(h, sym)
  #define PATH_SEP '\\'
  #define VENDOR_LIB_NAME "librclone.dll"
#else
  #include <dlfcn.h>
  #define LIBHANDLE void*
  #define DLOPEN(path)  dlopen(path, RTLD_NOW)
  #define DLSYM(h, sym) dlsym(h, sym)
  #define PATH_SEP '/'
  #define VENDOR_LIB_NAME "librclone.so"
#endif

typedef void            (*RcloneInitialize_t)(void);
typedef void            (*RcloneFinalize_t)(void);
typedef RcloneRPCResult (*RcloneRPC_t)(char*, char*);
typedef void            (*RcloneFreeString_t)(char*);

static LIBHANDLE lib_handle = NULL;

static RcloneInitialize_t p_RcloneInitialize = NULL;
static RcloneFinalize_t   p_RcloneFinalize   = NULL;
static RcloneRPC_t        p_RcloneRPC        = NULL;
static RcloneFreeString_t p_RcloneFreeString = NULL;

static void self_locate_marker(void) {}

#if defined(_WIN32)

static int resolve_sibling_path(char* out, size_t out_size) {
    HMODULE self_module = NULL;
    if (!GetModuleHandleExA(
            GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
            GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            (LPCSTR)&self_locate_marker,
            &self_module)) {
        return -1;
    }

    char self_path[MAX_PATH];
    DWORD len = GetModuleFileNameA(self_module, self_path, sizeof(self_path));
    if (len == 0 || len == sizeof(self_path)) {
        return -1;
    }

    /* Truncate at the last path separator to get the containing directory. */
    char* last_sep = strrchr(self_path, PATH_SEP);
    if (!last_sep) {
        return -1;
    }
    *last_sep = '\0';

    int written = snprintf(out, out_size, "%s\\%s", self_path, VENDOR_LIB_NAME);
    if (written < 0 || (size_t)written >= out_size) {
        return -1;
    }
    return 0;
}

#else /* Linux/macOS */

static int resolve_sibling_path(char* out, size_t out_size) {
    Dl_info info;
    if (!dladdr((void*)&self_locate_marker, &info) || !info.dli_fname) {
        return -1;
    }

    char self_path[4096];
    if (strlen(info.dli_fname) >= sizeof(self_path)) {
        return -1;
    }
    strcpy(self_path, info.dli_fname);

    char* last_sep = strrchr(self_path, PATH_SEP);
    if (!last_sep) {
        return -1;
    }
    *last_sep = '\0';

    int written = snprintf(out, out_size, "%s/%s", self_path, VENDOR_LIB_NAME);
    if (written < 0 || (size_t)written >= out_size) {
        return -1;
    }
    return 0;
}

#endif

static int librclone_load(const char* path) {
    lib_handle = DLOPEN(path);
    if (!lib_handle) {
        fprintf(stderr, "librclone: failed to load %s\n", path);
        return -1;
    }

    p_RcloneInitialize = (RcloneInitialize_t)DLSYM(lib_handle, "RcloneInitialize");
    p_RcloneFinalize   = (RcloneFinalize_t)DLSYM(lib_handle, "RcloneFinalize");
    p_RcloneRPC        = (RcloneRPC_t)DLSYM(lib_handle, "RcloneRPC");
    p_RcloneFreeString = (RcloneFreeString_t)DLSYM(lib_handle, "RcloneFreeString");

    if (!p_RcloneInitialize || !p_RcloneFinalize || !p_RcloneRPC || !p_RcloneFreeString) {
        fprintf(stderr, "librclone: failed to resolve one or more symbols\n");
        return -1;
    }

    return 0;
}

int librclone_ensure_loaded(void) {
    if (lib_handle) return 0; /* idempotent */

    char sibling_path[4096];
    if (resolve_sibling_path(sibling_path, sizeof(sibling_path)) != 0) {
        fprintf(stderr, "librclone: failed to resolve own module path\n");
        return -1;
    }

    return librclone_load(sibling_path);
}

// NOTE: RClone is not unloaded, rather it lives for the entire program lifetime.
// See https://github.com/rclone/rclone/blob/master/librclone/README.md#unloading

void RcloneInitialize(void)                       { p_RcloneInitialize(); }
void RcloneFinalize(void)                         { p_RcloneFinalize(); }
RcloneRPCResult RcloneRPC(char* method, char* in) { return p_RcloneRPC(method, in); }
void RcloneFreeString(char* str)                  { p_RcloneFreeString(str); }
