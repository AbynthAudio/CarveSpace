# Cross-compiling the Windows VST3 from Linux (MinGW)

This document records the **local Windows cross-build path** for CarveSpace.
The reference Windows binaries are still produced by the MSVC
`windows-latest` job in [`.github/workflows/build.yml`](../.github/workflows/build.yml);
this page exists so a Linux-only contributor can still produce and smoke-test a
Windows VST3 without a Windows machine.

**Status (validated 2026-11-08): SUCCESS** — the VST3 DLL builds, links and
passes the PE/export checks below. It has **not** yet been loaded in a Windows
DAW (no Windows host on this machine).

---

## 1. Result

| Item | Value |
|---|---|
| Toolchain | LLVM-MinGW `20261104`, UCRT, Linux x86_64 host package (`llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz`) |
| Compiler | clang 23.1.5, target `x86_64-w64-windows-gnu` |
| CMake / Ninja | CMake 4.4.4 / Ninja 1.13.2 |
| Command | see section 2 |
| Output | `build-win/CarveSpace_artefacts/Release/VST3/CarveSpace.vst3` |
| Staged copy | `dist/windows/CarveSpace.vst3` |
| Inner binary | `Contents/x86_64-win/CarveSpace.vst3` |
| Size | 9,088,512 bytes (8.7 MiB) |
| SHA-256 | `a0b35b96528bdb957dbf478570db5ac09cf541874c8a326de9905623b8473f27` |
| Type | `PE32+ executable for MS Windows 6.00 (DLL), x86-64, 14 sections` |
| Exports | `InitDll`, `GetPluginFactory`, `ExitDll` (standard VST3 entry points) |
| Runtime deps | UCRT (`api-ms-win-crt-*`) + Win32 system DLLs only — **no** `libstdc++-6.dll`, `libgcc_s_seh-1.dll`, `libc++.dll` or `libunwind.dll` |

The hash is a snapshot of the 2026-11-08 source tree; rebuild and re-hash after
source changes. The optional `CarveSpace_Standalone` target also cross-builds
successfully with the same toolchain (`dist/windows/CarveSpace.exe`,
PE32+ GUI executable); CI still ships only the VST3 for Windows.

The static-runtime link (`-static-libgcc -static-libstdc++`) means the bundle can
be copied to a Windows machine as-is. UCRT (`ucrtbase.dll` and the
`api-ms-win-crt-*` API sets) is part of Windows 10+; Windows 7/8 are not
supported by this binary.

Package it with:

```bash
./scripts/package.sh --windows --zip
# -> dist/windows/CarveSpace.vst3
# -> dist/CarveSpace-windows-x64.zip
```

---

## 2. Reproduce it

### 2.1 Get the portable toolchain (Linux host)

```bash
mkdir -p /tmp/llvm-mingw-dl
cd /tmp/llvm-mingw-dl
curl -LO https://github.com/mstorsjo/llvm-mingw/releases/download/20261104/llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz
mkdir -p /tmp/llvm-mingw
tar -xf llvm-mingw-20261104-ucrt-ubuntu-22.04-x86_64.tar.xz \
    -C /tmp/llvm-mingw --strip-components=1
```

> WinLibs GCC release zips are **Windows-host** binaries (`.exe`) and cannot be
> executed on a Linux host, so they are not usable here. LLVM-MinGW is the
> portable Linux-host MinGW option. On a Windows host with MSYS2/WinLibs the
> same toolchain file also supports a GCC-family compiler (auto-detected).

### 2.2 Configure and build

```bash
export LLVM_MINGW_ROOT=/tmp/llvm-mingw

cmake -S . -B build-win -G Ninja \
      -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/mingw-w64.cmake" \
      -DCMAKE_BUILD_TYPE=Release

ninja -C build-win -j6 CarveSpace_VST3
```

`build-win/` is gitignored (`build*/`). JUCE 7.0.5 must already exist in
`JUCE/` (see [BUILD.md](BUILD.md#0-get-juce-705-first-required)).

The toolchain file automatically:

- sets `CMAKE_SYSTEM_NAME=Windows`, the `x86_64-w64-mingw32-*` compilers, `windres`, `ar`, `ranlib`, `strip`, `ld`, `dlltool`;
- points the sysroot/find-root at `<toolchain>/x86_64-w64-mingw32`;
- defines `_WIN32_WINNT=0x0A00` (Windows 10);
- statically links the GCC/Clang runtimes;
- applies the two JUCE 7.0.5 compatibility flags described below (clang family only).

Override the root with `-DLLVM_MINGW_ROOT=...`, or force the compiler family
with `-DCARVESPACE_MINGW_FAMILY=gcc|clang`.

---

## 3. JUCE 7.0.5 + modern clang: the two required workarounds

A plain clang cross-build fails inside JUCE 7.0.5 headers. Neither error is
caused by CarveSpace code, and **neither needs a JUCE or `src/` edit**; the
toolchain file adds compiler flags instead.

### 3.1 `std::alignment_of_v` is not declared

```
JUCE/modules/juce_audio_basics/buffers/juce_AudioSampleBuffer.h:1196:45:
error: no member named 'alignment_of_v' in namespace 'std'
```

`juce_AudioSampleBuffer.h` uses `std::alignment_of_v<Type>` without including
`<type_traits>`. GCC's libstdc++ pulls `<type_traits>` in transitively from
other headers; libc++ (used by LLVM-MinGW) does not. Fix:

```
-include type_traits
```

### 3.2 Eager template-constructor diagnostic in `juce_AudioPluginInstance.h`

```
JUCE/modules/juce_audio_processors/processors/juce_AudioPluginInstance.h:164:74:
error: no matching constructor for initialization of 'AudioProcessor'
  164 | AudioPluginInstance (const short channelLayoutList[numLayouts][2]) : AudioProcessor (channelLayoutList) {}
```

The JUCE 7.0.5 constructor template passes a decayed `const short(*)[2]` array
pointer to `AudioProcessor`'s `std::initializer_list<const short[2]>`
constructor. GCC and MSVC defer the check until instantiation (and the
constructor is never instantiated, so it compiles); clang 23 checks the
mem-initializer eagerly and rejects the header. Fix:

```
-fdelayed-template-parsing
```

With both flags the whole target compiles with only warnings. Verified by
building the clean tree in `build-win/` after `rm -rf build-win`.

Note: this is purely a cross-build workaround. The MSVC CI build needs neither
flag, and a GCC-family MinGW-w64 toolchain (if you have one) does not need them
either — the toolchain file only applies them for clang.

---

## 4. Verification performed

```bash
file build-win/.../Contents/x86_64-win/CarveSpace.vst3
#   PE32+ executable for MS Windows 6.00 (DLL), x86-64, 14 sections

/tmp/llvm-mingw/bin/x86_64-w64-mingw32-objdump -p \
  build-win/.../Contents/x86_64-win/CarveSpace.vst3 \
  | grep -iE 'GetPluginFactory|InitDll|ExitDll'
#   1  0x1aba0  ExitDll
#   2  0x1abb0  GetPluginFactory
#   3  0x1ab90  InitDll

# dependencies: only KERNEL32/USER32/GDI32/comdlg32/dxgi/... and api-ms-win-crt-*
# (no third-party runtime DLLs)
```

Bundle layout (a `.vst3` on Windows is a directory):

```
CarveSpace.vst3/
  Contents/
    x86_64-win/
      CarveSpace.vst3      # the PE DLL
```

Installing on a Windows machine: copy the `CarveSpace.vst3` directory to
`C:\Program Files\Common Files\VST3\`, then rescan plugins in the DAW.

---

## 5. Limitations and risks

- **Not runtime-tested here.** The binary has not been loaded in a Windows DAW
  or `CarveSpaceHost`; the MSVC CI artifact is the reference. The WIN32
  programmatic smoke test (Wine) belongs to the test/reference tooling, not this
  build path.
- **JUCE does not officially support MinGW.** This works for JUCE 7.0.5 with
  the flags above, but future JUCE/compiler updates may break it. CI (MSVC)
  remains the source of truth.
- **Unsigned binary.** No Authenticode certificate: Windows SmartScreen may
  warn on first load ("Unknown publisher"). Code-signing is a release-time
  decision outside this cross-build.
- **Windows 10+ / UCRT only.** The UCRT baseline and `_WIN32_WINNT=0x0A00`
  exclude Windows 7/8.
- **x86_64 only.** No 32-bit (i686) or ARM64 Windows build is attempted.
- **Only `CarveSpace_VST3` is cross-built.** `CarveSpace_Standalone` and
  `CarveSpaceHost` for Windows are not part of this workflow (CI builds the
  VST3 with MSVC).
- **macOS cannot be cross-compiled from Linux.** Apple's SDK/toolchain is
  required; the `macos-14` CI job (arm64) is the only path to a Mac Silicon
  binary. That build is unsigned/non-notarised — see
  [BUILD.md](BUILD.md#unsigned-build-warning-gatekeeper).

If the local cross-build breaks in a future change, do not block releases on
it: the `windows-latest` MSVC job in CI covers Windows independently. Record
the new blocker here with the exact compiler output.
