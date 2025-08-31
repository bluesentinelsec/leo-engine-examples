// win_dll_dirs.h
#pragma once
#ifdef _WIN32
void leo_init_windows_dll_search(void);
#else
static inline void leo_init_windows_dll_search(void) {}
#endif

