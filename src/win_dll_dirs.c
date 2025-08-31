// win_dll_dirs.c
#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <wchar.h>

static void join_path(wchar_t *out, size_t cap, const wchar_t *a, const wchar_t *b) {
    wcsncpy(out, a, cap); out[cap-1]=L'\0';
    size_t len = wcslen(out);
    if (len && out[len-1] != L'\\' && out[len-1] != L'/') wcscat(out, L"\\");
    wcscat(out, b);
}

void leo_init_windows_dll_search(void) {
    // Get exe dir
    wchar_t exe[MAX_PATH];
    DWORD n = GetModuleFileNameW(NULL, exe, MAX_PATH);
    if (n == 0 || n == MAX_PATH) return;
    for (int i=(int)wcslen(exe)-1; i>=0; --i) { if (exe[i]==L'\\' || exe[i]==L'/'){ exe[i]=0; break; } }

    // Prefer modern, safe APIs if available
    HMODULE k32 = GetModuleHandleW(L"kernel32.dll");
    typedef BOOL (WINAPI *PFN_SetDefaultDllDirectories)(DWORD);
    typedef PVOID (WINAPI *PFN_AddDllDirectory)(PCWSTR);
    PFN_SetDefaultDllDirectories pSetDefaultDllDirectories =
        (PFN_SetDefaultDllDirectories)GetProcAddress(k32, "SetDefaultDllDirectories");
    PFN_AddDllDirectory pAddDllDirectory =
        (PFN_AddDllDirectory)GetProcAddress(k32, "AddDllDirectory");

    if (pSetDefaultDllDirectories && pAddDllDirectory) {
        // Search app dir + user dirs (which we add below) + system dirs. No CurrentDir.
        pSetDefaultDllDirectories(LOAD_LIBRARY_SEARCH_APPLICATION_DIR |
                                  LOAD_LIBRARY_SEARCH_USER_DIRS |
                                  LOAD_LIBRARY_SEARCH_SYSTEM32);

        wchar_t libdir[MAX_PATH];
        join_path(libdir, MAX_PATH, exe, L"lib");
        pAddDllDirectory(libdir);           // now the loader can resolve from .\lib
    } else {
        // Fallback for very old systems: include .\lib in legacy search path
        // NOTE: This also removes the current directory from search order for LoadLibrary.
        // We still have the EXE dir in the default order, which is fine.
        SetDllDirectoryW(L"lib");
    }
}
#endif

