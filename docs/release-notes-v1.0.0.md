# CarveSpace 1.0.0

<p align="center">
  <img src="https://raw.githubusercontent.com/AbynthAudio/CarveSpace/master/docs/media/skins.gif" alt="CarveSpace skins" width="650">
</p>

First public release of **CarveSpace**, the 32-band spectral sidechain carver I wrote for **Abynth Audio**.

## Downloads

| Platform | Asset |
|---|---|
| Linux x86_64 | `CarveSpace-linux-x86_64.vst3.tar.gz` (VST3 only) |
| Windows x64 | `CarveSpace-windows-x64.vst3` (direct VST3 file) — **untested** (cross-built on Linux) |
| macOS Apple Silicon | `CarveSpace-macos-arm64.vst3.tar.gz` (VST3 only, unsigned) — **untested** (CI-built, not run on macOS hardware) |

## What's inside

- Zero-latency 32-band dynamic EQ driven by a stereo external sidechain.
- Amount, Low-cut, High-cut, Attack, Release, Pan, L/R–M/S, Freeze and Sidechain Audition.
- Spectrum display with the sidechain spectrum and the applied reduction curve.
- Three skins (Dark / Light / pastel Pink), remembered per project, plus a remembered editor size.
- Animated Abynth Audio swirl: it rotates slowly and you can click it to pause or resume.
- Resizable 650×650 editor, no authorization, no telemetry, GPLv3.

## Notes

- I develop and test on Linux. The Windows and macOS builds are provided **untested**: the Windows DLL is cross-compiled with LLVM-MinGW and the macOS bundle is built by GitHub Actions, but I have not run them on Windows or macOS hardware. Please open an issue if something fails there.
- The macOS build is unsigned/not notarised: right-click → Open on the first launch if Gatekeeper complains.
- CarveSpace is my own independent implementation, inspired by the Trackspacer workflow. It contains no Wavesfactory code, binaries, artwork or fonts and is not affiliated with Wavesfactory.
- Built with JUCE 7.0.5. Comfortaa font under the SIL Open Font License.
