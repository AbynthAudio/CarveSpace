# Building CarveSpace

CarveSpace is a native VST3 plug-in + Standalone app built with **CMake** and
**JUCE 7.0.5**. This page covers local builds on Linux, Windows and macOS, the
GitHub Actions matrix, and how to package the resulting VST3 bundles.

- Product: `CarveSpace`
- Manufacturer: `Exxo`
- Bundle ID: `com.exxo.carvespace`
- CMake targets: `CarveSpace_VST3`, `CarveSpace_Standalone`,
  `CarveSpaceHost` (offline test host)
- VST3 output (single-config generators):
  `<build-dir>/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3`

---

## 0. Get JUCE 7.0.5 first (required)

`JUCE/` is **not committed to this repository** (it is in `.gitignore`), so a
fresh clone has no JUCE tree and CMake will fail with
`add_subdirectory given source "JUCE" which is not an existing directory`.

Clone the exact tag next to `CMakeLists.txt`:

```bash
git clone --depth 1 --branch 7.0.5 \
  https://github.com/juce-framework/JUCE.git JUCE
```

(Optional alternative: convert JUCE to a git submodule. The repository does not
use one so that the working tree stays clone-free; if you prefer submodules,
add `.gitmodules` pointing at the same tag and run
`git submodule add https://github.com/juce-framework/JUCE.git JUCE`.)

The same step is performed automatically by CI (`.github/workflows/build.yml`,
"Fetch JUCE 7.0.5 if missing") before configuring.

Licensing: the project is **GPLv3** and JUCE 7.0.5 is used under its GPLv3
option. See `LICENSE` and `JUCE/LICENSE.md`; binaries must be distributed with
the corresponding source available under the GPL.

---

## 1. Linux (x86_64)

### Dependencies

Debian / Ubuntu:

```bash
sudo apt-get update
sudo apt-get install -y \
  build-essential cmake ninja-build pkg-config git \
  libasound2-dev libx11-dev libxext-dev libxrandr-dev \
  libxinerama-dev libxcursor-dev libxcomposite-dev \
  libfreetype-dev libfontconfig1-dev libgl1-mesa-dev \
  libcurl4-openssl-dev
```

> `libfreetype-dev` is the name on Ubuntu 22.04+ (older releases:
> `libfreetype6-dev`). WebKit/GTK are **not** needed: the plugin is configured
> with `JUCE_WEB_BROWSER=0` and `JUCE_USE_CURL=0`. On Ubuntu 22.04 you can also
> install `libwebkit2gtk-4.0-dev` if you ever enable the browser component
> (JUCE 7.0.5 probes for `webkit2gtk-4.0`, not 4.1).

Fedora: `alsa-lib-devel libX11-devel libXext-devel libXrandr-devel
libXinerama-devel libXcursor-devel libXcomposite-devel freetype-devel
fontconfig-devel mesa-libGL-devel cmake ninja-build git`.

Arch: `base-devel cmake ninja git alsa-lib libx11 libxext libxrandr libxinerama
libxcursor libxcomposite freetype2 fontconfig mesa`.

### Configure, build, install

```bash
git clone --depth 1 --branch 7.0.5 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target CarveSpace_VST3 CarveSpace_Standalone --parallel 6
```

Outputs:

- `build/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3`
- `build/CarveSpace_artefacts/Release/Standalone/CarveSpace`

Per-user install (most hosts scan this):

```bash
mkdir -p ~/.vst3
cp -r build/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3 ~/.vst3/
```

Or stage a release directory with the packaging script:

```bash
./scripts/package.sh --linux --build     # -> dist/linux/CarveSpace.vst3
```

#### Headless / minimal-memory builds

JUCE + the bundled VST3 SDK are heavy. On a 4 GB machine use
`--parallel 2`; the project's standard cap is **`-j6`** (`--parallel 6`).

---

## 2. macOS (Apple Silicon / arm64)

Requirements: Xcode Command Line Tools (`xcode-select --install`), CMake ≥ 3.22
(`brew install cmake`) and Ninja (`brew install ninja`).

```bash
git clone --depth 1 --branch 7.0.5 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -G Ninja \
  -DCMAKE_BUILD_TYPE=Release \
  -DCMAKE_OSX_ARCHITECTURES=arm64 \
  -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0
cmake --build build --target CarveSpace_VST3 --parallel 6
```

Output: `build/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3`.

Install for the current user:

```bash
mkdir -p ~/Library/Audio/Plug-Ins/VST3
cp -r build/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3 \
      ~/Library/Audio/Plug-Ins/VST3/
```

### Unsigned build warning (Gatekeeper)

**CI produces unsigned, non-notarised macOS binaries.** There is no Apple
Developer ID on the GitHub runners. When the zip is downloaded through a
browser and unpacked, macOS may quarantine it and the host may refuse to load
the plug-in with a message such as *"CarveSpace.vst3 cannot be opened because
the developer cannot be verified"*.

Ways around it (any one):

1. Right-click the downloaded `.zip` (or `CarveSpace.vst3`) → **Open**, then
   confirm. This clears the quarantine flag for that copy.
2. Remove the quarantine attribute:
   ```bash
   xattr -dr com.apple.quarantine /path/to/CarveSpace.vst3
   ```
3. **System Settings → Privacy & Security → Open Anyway** after the first
   blocked attempt.

CI additionally applies an **ad-hoc signature**
(`codesign --force --deep --sign -`) as a best-effort convenience, but ad-hoc
signing does not remove the quarantine/Gatekeeper requirement.

The arm64 build runs on Apple Silicon macOS 11+. Intel Macs need a separate
`x86_64` (or universal) build — pass
`-DCMAKE_OSX_ARCHITECTURES="x86_64;arm64"` locally (use deployment target 11.0
for universal binaries).

---

## 3. Windows (x64)

The supported toolchain is **MSVC** (Visual Studio 2022 with *Desktop
development with C++*). A Linux → Windows cross-build with MinGW is also
possible and validated locally; see [BUILD-CROSS.md](BUILD-CROSS.md).

### Visual Studio generator (simplest)

```bat
git clone --depth 1 --branch 7.0.5 https://github.com/juce-framework/JUCE.git JUCE
cmake -S . -B build -G "Visual Studio 17 2022" -A x64 -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --target CarveSpace_VST3 --parallel 6
```

Output: `build\CarveSpace_artefacts\Release\VST3\CarveSpace.vst3`.

### Ninja (faster, from a Developer Command Prompt)

```bat
cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build --target CarveSpace_VST3 --parallel 6
```

Install (system-wide, requires admin):

```bat
xcopy /E /I "build\CarveSpace_artefacts\Release\VST3\CarveSpace.vst3" ^
      "%CommonProgramFiles%\VST3\CarveSpace.vst3"
```

Or per-user: `%LOCALAPPDATA%\Programs\Common\VST3\VST3` is **not** a standard
path; prefer the system `C:\Program Files\Common Files\VST3` folder or the
folder your DAW scans.

The CI Windows artifact is built with MSVC and is the reference binary.

---

## 4. Packaging helper: `scripts/package.sh`

Stages a built bundle into `dist/` and prints the path. Works on Linux, macOS
and Windows (Git Bash/MSYS2).

```bash
./scripts/package.sh                    # copy current platform from ./build
./scripts/package.sh --build            # build first, then copy
./scripts/package.sh --build --build-dir build-ci
./scripts/package.sh --linux --build    # explicit platform
./scripts/package.sh --windows          # copy a cross-built bundle from ./build-win
./scripts/package.sh --zip              # also write dist/CarveSpace-<os>-<arch>.zip
```

Result layout:

```
dist/
  linux/CarveSpace.vst3   (+ dist/linux/CarveSpace standalone executable)
  windows/CarveSpace.vst3
  macos/CarveSpace.vst3   (+ dist/macos/CarveSpace.app)
  CarveSpace-<os>-<arch>.zip        # with --zip
```

`dist/` is gitignored; it is a staging area, not committed content.

---

## 5. GitHub Actions: what CI builds and where to find it

Workflow: [`.github/workflows/build.yml`](../.github/workflows/build.yml)

Triggers:

- `push` to `main` / `master` and tags matching `v*`
- `pull_request`
- manual **Run workflow** (`workflow_dispatch`)

Matrix and artifacts (each is a ready-to-distribute zip, kept 90 days):

| Runner | Platform | Artifact name | Zip contents |
|---|---|---|---|
| `ubuntu-latest` | Linux x86_64 | `CarveSpace-linux-x86_64` | `CarveSpace.vst3/` + `CarveSpace` (Standalone) |
| `windows-latest` | Windows x64 (MSVC) | `CarveSpace-windows-x64` | `CarveSpace.vst3/` |
| `macos-14` | macOS arm64 (Apple Silicon) | `CarveSpace-macos-arm64` | `CarveSpace.vst3/` (arm64, unsigned) |

Downloading: **Actions → Build VST3 → (latest run) → Artifacts**. The GitHub UI
always downloads an artifact as a zip containing the `.zip` above; unpack it
once to get `CarveSpace.vst3/`.

> `macos-14` and `macos-latest` runners are both Apple Silicon (arm64) today.
> If GitHub retires `macos-14`, change that single matrix entry to
> `macos-latest`; no other change is needed.

To publish a release from an existing run (after downloading the three
artifacts/zips), for example:

```bash
gh release create v1.0.0 \
  CarveSpace-linux-x86_64.zip \
  CarveSpace-windows-x64.zip \
  CarveSpace-macos-arm64.zip
```

Notes / caveats:

- **CI is the source of truth** for shippable Windows and macOS binaries; this
  Linux machine can cross-build Windows but can never build or sign macOS.
- CI builds are **unsigned** (Windows: no Authenticode certificate; macOS: no
  Developer ID/notarisation). See the Gatekeeper note above.
- Binary compatibility target: Linux glibc 2.35+ (ubuntu-latest baseline),
  Windows 10+ x64 (MSVC runtime is bundled/static per JUCE defaults), macOS
  11+ arm64.
- The workflow clones JUCE 7.0.5 itself, so pushing the repo without `JUCE/`
  is expected and correct.
- Optional README badge:
  `![Build VST3](https://github.com/<owner>/<repo>/actions/workflows/build.yml/badge.svg)`
  (replace `<owner>/<repo>`).

---

## 6. Troubleshooting

| Symptom | Fix |
|---|---|
| `add_subdirectory given source "JUCE" which is not an existing directory` | Clone JUCE 7.0.5 into `JUCE/` (section 0). |
| `Could NOT find PkgConfig` on Linux | `sudo apt-get install pkg-config`. |
| `X11/extensions/... not found` / `alsa/asoundlib.h: No such file` | Install the X11/ALSA dev packages listed in section 1. |
| Linker OOM / long swaps on a small machine | Lower `--parallel` (e.g. `--parallel 2`); CI uses 4. Max agreed here is 6. |
| `libfreetype6-dev` not found | Ubuntu 22.04+: use `libfreetype-dev`. |
| macOS: deployment-target/arch errors | Pass `-DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=11.0`. |
| macOS host refuses to load the plug-in | Unsigned/quarantined: right-click Open, `xattr -dr com.apple.quarantine`, or Allow Anyway. |
| Clang/MinGW cross-build errors in JUCE headers | Use `cmake/toolchains/mingw-w64.cmake` (it applies the JUCE 7.0.5 compatibility flags) and read [BUILD-CROSS.md](BUILD-CROSS.md). |
| CMake version too old | Needs **≥ 3.22** (`cmake --version`). |
| `copy_vst3` target copies from the wrong config | It assumes single-config `Release`; use `scripts/package.sh` instead. |

## 7. Verifying a build

- Run the offline host (Linux, built with `CarveSpaceHost`):
  ```bash
  cmake --build build --target CarveSpaceHost --parallel 6
  build/CarveSpaceHost_artefacts/Release/CarveSpaceHost dump \
    build/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3
  ```
- Load the VST3 in a host (REAPER, Bitwig, Ardour, …) and check that the
  product name, manufacturer (`Exxo`) and version (1.0.0) are reported.
- Linux bundle inner binary: `Contents/x86_64-linux/CarveSpace.so`;
  Windows: `Contents/x86_64-win/CarveSpace.vst3` (a PE DLL);
  macOS: `Contents/MacOS/CarveSpace` (Mach-O arm64).
