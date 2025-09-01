# Shared Library Linking and Runtime Search Paths

This document explains how **Leo Engine Showcase** handles dynamic library linking across **Windows**, **macOS**, and **Linux**, and why the `CMakeLists.txt` is structured the way it is.

---

## Background

When building against shared libraries (like `libleo` and `libSDL3`), the *compiler* and *linker* embed information into the executable:

* **Link-time dependency:** the executable records *what libraries it depends on* (`libleo.0.dylib`, `libSDL3.0.dylib`, etc.).
* **Runtime search path (RPATH):** hints to the OS loader about *where to look* for those libraries at runtime.

Each platform has a different mechanism for this, so we configure CMake carefully to make the app bundle portable.

---

## Windows

* **Shared libraries:** `.dll` (dynamic link libraries).

* **Default search path:**

  1. Directory of the `.exe`.
  2. System paths (`C:\Windows\System32`, etc.).
  3. `PATH` environment variable.

* **CMake behavior in Showcase:**

  * `target_link_options(... /DELAYLOAD:SDL3.dll /DELAYLOAD:leo.dll)` tells MSVC to delay-load DLLs. The loader doesn’t try to resolve them until the first function call, which allows us to control search paths programmatically.
  * `win_dll_dirs.c` + `leo_init_windows_dll_search()` extend the DLL search path so the runtime can find DLLs in the `lib/` directory of the package.
  * Post-build step copies `SDL3.dll` and `leo.dll` into the build output folder or `lib/`.

**Takeaway:** On Windows, the loader always looks in the EXE directory first, so shipping DLLs alongside the executable is the most reliable solution.

---

## macOS

* **Shared libraries:** `.dylib` (dynamic libraries).

* **Identifiers:**

  * Each `.dylib` has an **install name** (e.g., `@rpath/libleo.0.dylib`).
  * The executable records that name and expects to resolve it at runtime.

* **Search path keywords:**

  * `@rpath` → runtime paths configured in the binary.
  * `@loader_path` → directory containing the binary itself.
  * `@executable_path` → same as `@loader_path` for executables (slightly different for plugins).

* **CMake behavior in Showcase:**

  * Libraries are copied into the `.app/Contents/Frameworks` folder.
  * `INSTALL_RPATH`/`BUILD_RPATH` set to `@loader_path/../Frameworks`, so the loader resolves `@rpath/libleo.0.dylib` relative to the executable inside `.app/Contents/MacOS/`.
  * `install_name_tool -id` rewrites the dylib’s own ID to `@rpath/...`, ensuring consistency inside the bundle.
  * Symlinks are created (`libleo.0.dylib → libleo.0.1.0.dylib`) so that versioned SONAMEs match what the executable expects.

**Takeaway:** On macOS, we rely on `@rpath` + `@loader_path/../Frameworks`, which is the standard app bundle layout. Everything is self-contained inside the `.app`.

---

## Linux

* **Shared libraries:** `.so` (shared objects).

* **SONAMEs:**

  * Libraries typically have versioned filenames (`libleo.so.0.1.0`) and SONAMEs (`libleo.so.0`).
  * The executable records the SONAME, not the full filename.

* **Search path:**

  1. `$ORIGIN` relative to the executable (if set in RPATH).
  2. `LD_LIBRARY_PATH` environment variable.
  3. System library paths (`/lib`, `/usr/lib`, etc.).

* **CMake behavior in Showcase:**

  * RPATH set to `$ORIGIN` and `$ORIGIN/lib`, so the executable finds libraries in its own folder or a sibling `lib/`.
  * Post-build step copies `libleo.so` and `libSDL3.so` into `lib/`.
  * CMake helper script creates symlinks (`libleo.so.0 → libleo.so.0.1.0`) so SONAMEs resolve correctly.

**Takeaway:** On Linux, `$ORIGIN` makes executables relocatable, so the package can run from any directory without `LD_LIBRARY_PATH` tweaks.

---

## Summary Table

| Platform | Library Ext. | What the Executable Records | Where We Copy Libraries    | Runtime Path Setting         |
| -------- | ------------ | --------------------------- | -------------------------- | ---------------------------- |
| Windows  | `.dll`       | `SDL3.dll`, `leo.dll`       | `exe` dir + `lib/`         | EXE dir + PATH               |
| macOS    | `.dylib`     | `@rpath/libleo.0.dylib`     | `.app/Contents/Frameworks` | `@loader_path/../Frameworks` |
| Linux    | `.so`        | `libleo.so.0` (SONAME)      | `./lib/`                   | `$ORIGIN`, `$ORIGIN/lib`     |

---

## Practical Notes for Developers

* **otool -L (macOS)** and **ldd (Linux)** are your friends to inspect linkage.
* If you see "not found" errors:

  * Windows → check DLLs are next to the EXE.
  * macOS → check `otool -L` for `@rpath` correctness and that `Frameworks/` contains the `.dylib` and symlink.
  * Linux → check `ldd` output and that `$ORIGIN/lib` has the `.so` and SONAME symlink.
* Avoid hardcoding absolute paths into binaries; rely on `$ORIGIN`/`@loader_path`.

---


