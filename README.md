# Leo Engine Showcase

**Leo Engine Showcase** is a simple launcher for running example demos and games built with [Leo Engine](https://github.com/bluesentinelsec/leo-engine).
It provides a unified command-line interface so you can list demos, run a specific one, or execute all of them (including a special CI/CD mode for automated builds).

This project is meant to **onboard demos quickly**: just add a new demo source file, register it in `demos.c`, and it will automatically appear in the showcase.

---

## ✨ Features

* Runs all bundled Leo Engine demos/games from a single entrypoint.
* Command-line interface for selecting demos by index or running them all.
* CI/CD mode (`--cicd`) runs each demo for a single frame (useful for automated testing).
* Cross-platform build via CMake (macOS, Linux, Windows).
* Dynamically linked against Leo Engine and SDL3.

---

## 🚀 Usage

After building, run the showcase binary:

```bash
./leo-engine-showcase [OPTIONS]
```

### Options

```
  -h, --help            Show this help text and exit
  -l, --list-demos      List all available demos
  -i, --index N         Run demo with index N
  -a, --all             Run all demos sequentially
  -c, --cicd            Run in CI/CD mode (1 frame only)
```

### Examples

List demos:

```bash
./leo-engine-showcase --list-demos
```

Run a single demo:

```bash
./leo-engine-showcase --index 0
```

Run a demo in CI/CD mode (exit after one frame):

```bash
./leo-engine-showcase --index 1 --cicd
```

Run all demos in CI/CD mode:

```bash
./leo-engine-showcase --all --cicd
```

---

## 🛠️ Build from Source

This project uses **CMake** and fetches Leo Engine automatically as a dependency.
A **C17-capable compiler** and **CMake 3.25+** are required.

### macOS

```bash
brew install cmake ninja git
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/leo-engine-showcase --list-demos
```

### Linux (Debian/Ubuntu)

```bash
sudo apt-get update
sudo apt-get install -y build-essential cmake ninja-build pkg-config git 

cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
./build/leo-engine-showcase --list-demos
```

### Windows (MSVC)

```powershell
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
.\build\Release\leo-engine-showcase.exe --list-demos
```

---

## 📦 CI/CD

In CI/CD pipelines, the showcase can be run in headless/one-frame mode:

```bash
./build/leo-engine-showcase --all --cicd
```

On Linux, this is typically wrapped in `xvfb-run` so SDL3 has a virtual display:

```bash
xvfb-run -a -s "-screen 0 1280x720x24" ./build/leo-engine-showcase --all --cicd
```

---

## ⚡ Contributing

Adding a new demo is as simple as:

1. Create `src/demo_yourfeature.c` with a `bool RunDemo_YourFeature(bool one_frame)` function.
2. Register it in `src/demos.c`.
3. Rebuild — it will show up automatically in `--list-demos`.

---

