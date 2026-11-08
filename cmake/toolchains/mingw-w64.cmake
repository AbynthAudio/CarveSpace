# ==============================================================================
# CMake toolchain file: cross-compile CarveSpace for Windows x86_64 on Linux
# (or any non-Windows host) with a portable toolchain.
#
# PRIMARY (validated on this project): LLVM-MinGW clang
#   https://github.com/mstorsjo/llvm-mingw/releases
#   asset: llvm-mingw-<ver>-ucrt-ubuntu-22.04-x86_64.tar.xz   (Linux host)
#   tar -xf llvm-mingw-*.tar.xz -C /tmp/llvm-mingw --strip-components=1
#
#   JUCE 7.0.5 needs two compatibility flags with modern clang, applied
#   automatically below:
#     -include type_traits        -> juce_AudioSampleBuffer.h uses
#                                    std::alignment_of_v without including
#                                    <type_traits>; libc++ no longer pulls it in
#                                    transitively (GCC's libstdc++ does).
#     -fdelayed-template-parsing  -> avoids clang's eager diagnostic for the
#                                    JUCE 7.0.5 AudioPluginInstance template
#                                    constructor (which passes a raw array
#                                    pointer to AudioProcessor's
#                                    initializer_list constructor). GCC and MSVC
#                                    accept this code; clang 23 does not.
#   These are build-toolchain workarounds only: no JUCE or src/ edits needed.
#
# ALTERNATIVE: a GCC-family MinGW-w64 (distro mingw-w64 on Linux, MSYS2/WinLibs
# on Windows) is also detected and needs neither workaround. LLVM-MinGW's own
# x86_64-w64-mingw32-g++ wrapper is clang underneath, so family detection uses
# the presence of the clang driver.
#
# Usage:
#   export LLVM_MINGW_ROOT=/tmp/llvm-mingw
#   cmake -S . -B build-win -G Ninja \
#         -DCMAKE_TOOLCHAIN_FILE="$PWD/cmake/toolchains/mingw-w64.cmake" \
#         -DCMAKE_BUILD_TYPE=Release
#   ninja -C build-win -j6 CarveSpace_VST3
#
# The root can also be passed with -DLLVM_MINGW_ROOT=/path/to/toolchain and the
# compiler family forced with -DCARVESPACE_MINGW_FAMILY=gcc|clang.
# See docs/BUILD-CROSS.md.
# ==============================================================================

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

# --- Locate the toolchain ----------------------------------------------------
if(NOT DEFINED LLVM_MINGW_ROOT OR LLVM_MINGW_ROOT STREQUAL "")
    if(DEFINED ENV{LLVM_MINGW_ROOT})
        set(LLVM_MINGW_ROOT "$ENV{LLVM_MINGW_ROOT}")
    elseif(EXISTS "/tmp/llvm-mingw/bin/x86_64-w64-mingw32-clang")
        set(LLVM_MINGW_ROOT "/tmp/llvm-mingw")
    elseif(EXISTS "/tmp/llvm-mingw/bin/x86_64-w64-mingw32-g++")
        set(LLVM_MINGW_ROOT "/tmp/llvm-mingw")
    else()
        message(FATAL_ERROR
            "No MinGW-w64 toolchain found. Download a portable LLVM-MinGW "
            "release from https://github.com/mstorsjo/llvm-mingw/releases, "
            "extract it and re-run CMake with -DLLVM_MINGW_ROOT=/path/to/toolchain "
            "(or export LLVM_MINGW_ROOT). See docs/BUILD-CROSS.md.")
    endif()
endif()

set(LLVM_MINGW_BIN "${LLVM_MINGW_ROOT}/bin")
if(NOT IS_DIRECTORY "${LLVM_MINGW_BIN}")
    message(FATAL_ERROR
        "LLVM_MINGW_ROOT='${LLVM_MINGW_ROOT}' has no bin/ directory.")
endif()

# --- Compiler family auto-detection -----------------------------------------
# LLVM-MinGW ships a clang driver; distro/MSYS2 GCC toolchains do not.
if(NOT CARVESPACE_MINGW_FAMILY)
    if(EXISTS "${LLVM_MINGW_BIN}/x86_64-w64-mingw32-clang")
        set(CARVESPACE_MINGW_FAMILY "clang")
    elseif(EXISTS "${LLVM_MINGW_BIN}/x86_64-w64-mingw32-g++")
        set(CARVESPACE_MINGW_FAMILY "gcc")
    else()
        message(FATAL_ERROR
            "Neither x86_64-w64-mingw32-clang nor x86_64-w64-mingw32-g++ was "
            "found in '${LLVM_MINGW_BIN}'.")
    endif()
endif()
message(STATUS "CarveSpace MinGW: root=${LLVM_MINGW_ROOT} family=${CARVESPACE_MINGW_FAMILY}")

set(_prefix "${LLVM_MINGW_BIN}/x86_64-w64-mingw32-")

if(CARVESPACE_MINGW_FAMILY STREQUAL "gcc")
    set(CMAKE_C_COMPILER   "${_prefix}gcc"   CACHE FILEPATH "")
    set(CMAKE_CXX_COMPILER "${_prefix}g++"   CACHE FILEPATH "")
    set(_juce_compat_cxx_flags "")
else()
    set(CMAKE_C_COMPILER   "${_prefix}clang"   CACHE FILEPATH "")
    set(CMAKE_CXX_COMPILER "${_prefix}clang++" CACHE FILEPATH "")
    # JUCE 7.0.5 compatibility with modern clang/libc++ (see header comment).
    set(_juce_compat_cxx_flags " -include type_traits -fdelayed-template-parsing")
endif()

set(CMAKE_RC_COMPILER "${_prefix}windres" CACHE FILEPATH "")
set(CMAKE_AR      "${_prefix}ar"      CACHE FILEPATH "")
set(CMAKE_RANLIB  "${_prefix}ranlib"  CACHE FILEPATH "")
set(CMAKE_STRIP   "${_prefix}strip"   CACHE FILEPATH "")
set(CMAKE_LINKER  "${_prefix}ld"      CACHE FILEPATH "")
set(CMAKE_DLLTOOL "${_prefix}dlltool" CACHE FILEPATH "")

# --- Sysroot / find rules ----------------------------------------------------
# MinGW-w64 toolchains keep the Windows sysroot in <root>/<triple>/.
if(IS_DIRECTORY "${LLVM_MINGW_ROOT}/x86_64-w64-mingw32")
    set(CMAKE_SYSROOT "${LLVM_MINGW_ROOT}/x86_64-w64-mingw32")
    set(CMAKE_FIND_ROOT_PATH "${LLVM_MINGW_ROOT}/x86_64-w64-mingw32")
endif()

# Never look for host programs (cmake, ninja, ...) inside the Windows sysroot,
# but only look for libraries/headers/packages there.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# --- Target flags ------------------------------------------------------------
# Windows 10+ / UCRT is the realistic baseline for modern DAWs.
set(CMAKE_C_FLAGS_INIT   "-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00")
set(CMAKE_CXX_FLAGS_INIT "-D_WIN32_WINNT=0x0A00 -DWINVER=0x0A00${_juce_compat_cxx_flags}")

# Link the runtimes statically so the resulting .vst3 has no dependency on
# libc++/libunwind (clang) or libgcc/libstdc++ (gcc) DLLs.
set(CMAKE_EXE_LINKER_FLAGS_INIT    "-static-libgcc -static-libstdc++")
set(CMAKE_SHARED_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
set(CMAKE_MODULE_LINKER_FLAGS_INIT "-static-libgcc -static-libstdc++")
