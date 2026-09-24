<div align="center">
  <img src="assets/caps-as-shift.png" width="128" height="128" alt="CapsAsShift: a white Shift arrow on a blue keycap">
  <h1>CapsAsShift</h1>
  <p><strong>Your Caps Lock key, now a Left Shift key.</strong></p>
  <p>A small Windows tray utility. One executable, with optional startup at sign-in.</p>
  <a href="https://github.com/zet235/caps-as-shift/actions/workflows/build.yml"><img src="https://github.com/zet235/caps-as-shift/actions/workflows/build.yml/badge.svg?branch=main" alt="Windows build status"></a>
  <a href="https://github.com/zet235/caps-as-shift/releases/latest"><img src="https://img.shields.io/github/v/release/zet235/caps-as-shift?color=315b96" alt="Latest release"></a>
  <img src="https://img.shields.io/badge/platform-Windows%20x64-0078d6" alt="Platform: Windows x64">
  <p><a href="https://github.com/zet235/caps-as-shift/releases/latest/download/CapsAsShift.exe"><strong>Download for Windows x64</strong></a> · <a href="https://github.com/zet235/caps-as-shift/releases">All releases</a></p>
</div>

<p align="center"><strong>English</strong> · <a href="README.zh-TW.md">繁體中文</a></p>

---

## Why CapsAsShift?

If your input method uses a single Shift press to switch languages, CapsAsShift gives you another key in a convenient position. It remaps the physical Caps Lock key to Left Shift while running, including press-and-hold shortcuts.

| Feature | What you get |
| --- | --- |
| **Caps Lock → Left Shift** | Tap Caps Lock for Shift behavior, or hold it while pressing another key. |
| **Overlapping keys** | Caps Lock and physical Left Shift are tracked together, keeping Shift held until the last source is released. |
| **Tray controls** | An English menu with **Start with Windows** and **Exit**. |
| **Portable executable** | The icon and MinGW runtime are included in the EXE. No installation or PowerToys required. |
| **GitHub-built releases** | Starting with **v0.1.1**, release binaries are compiled and tested on GitHub Actions. |

> **Input method behavior:** a tap switches languages only if your current input method is configured to switch on Shift. CapsAsShift does not change your input method settings.

## Get started

1. [Download **CapsAsShift.exe**](https://github.com/zet235/caps-as-shift/releases/latest/download/CapsAsShift.exe) and save it somewhere you want to keep it.
2. Turn off Caps Lock's existing capitalization lock, then launch the EXE.
3. Find the blue Shift icon in the system tray; it may be inside the hidden-icons menu.

Try holding <kbd>Caps Lock</kbd> while pressing a letter, then compare a single tap with your usual <kbd>Shift</kbd> language-switch action.

### Tray menu

| Item | Action |
| --- | --- |
| **Start with Windows** | Enable or disable startup when your Windows account signs in. |
| **Exit** | Stop remapping and close the app. |

Only one instance runs at a time. Exiting does not disable startup at the next sign-in. If you move the EXE, launch it from its new location and enable **Start with Windows** again to update the path.

Startup uses the `CapsAsShift` value under `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`.

## Built on GitHub

The [Windows build workflow](.github/workflows/build.yml) uses a GitHub-hosted Windows runner and a MinGW-w64 UCRT64 toolchain.

| Trigger | Result |
| --- | --- |
| Push to `main` or a pull request | Compile, run four C++ test programs, generate the icon, and upload a Windows build artifact. |
| **Run workflow** in the Actions tab | Build a downloadable artifact on demand. |
| Push a `v*` version tag | Run the same build and publish its EXE and `SHA256SUMS.txt` to GitHub Releases. |

The release job downloads the artifact from **that same workflow run**, verifies its SHA-256 checksum, then publishes it. It does not upload a developer's local EXE.

The hosted build uses `-SkipGuiSmoke`: state, event-dispatch, registry and lifecycle tests run in CI; interactive tray behavior and physical keyboard/IME checks require a Windows desktop.

<details>
<summary><strong>Verify a downloaded executable</strong></summary>

Download `SHA256SUMS.txt` from the same release, then compare it with:

```powershell
Get-FileHash .\CapsAsShift.exe -Algorithm SHA256
```

</details>

## Build from source

For development, use Windows, PowerShell 5.1, and MinGW-w64 `g++` / `windres` on `PATH`. The code requires C++17.

Exit any running CapsAsShift instance before rebuilding:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1
```

This runs the four C++ test programs, builds `CapsAsShift.exe`, and runs a tray/startup/exit smoke test. On a non-interactive runner, use:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File .\build.ps1 -SkipGuiSmoke
```

## How it works

A Win32 `WH_KEYBOARD_LL` hook intercepts physical Caps Lock and Left Shift events. A small state machine combines their held states, and `SendInput` emits Left Shift transitions. Tagged self-generated events bypass remapping to prevent recursion. The app does not record key history or use the network.

The blue keycap artwork is generated by [`tools/generate-icon.ps1`](tools/generate-icon.ps1) using Windows' .NET drawing library. The [ICO](assets/caps-as-shift.ico) contains nine sizes from 16 to 256 pixels; the [PNG](assets/caps-as-shift.png) is a 512-pixel preview. Both tray and executable icons come from embedded Windows resources.

## Limits and desktop checks

- **Existing Caps Lock state:** the app preserves the current capitalization lock. If it was on, choose **Exit**, turn Caps Lock off, and relaunch.
- **Elevated windows:** a normally launched app may not remap input in administrator-level windows.
- **Heavy CPU load or stalls:** high CPU usage alone does not guarantee failure, but a late hook can be skipped or silently removed by Windows. The tray icon may remain even after remapping stops. Exit and relaunch to reinstall the hook.
- **Graceful exit:** use **Exit** rather than forcibly terminating the process so it can clean up synthetic key state.

The hook currently shares a thread with tray operations. On Windows 10 1709 and later, the maximum allowed hook timeout is 1,000 ms; the actual `LowLevelHooksTimeout` setting may be lower. High-load physical-input behavior has not been stress-tested. See [Microsoft's hook documentation](https://learn.microsoft.com/en-us/windows/win32/winmsg/lowlevelkeyboardproc).

Before relying on it in your setup, check actual IME switching, rapid and overlapping key presses, lock/unlock, sleep/resume, Explorer restart, and sign-in startup. The [security and code review report](docs/reviews/2026-09-24-review.md) records the tested fixes and remaining limitations.
