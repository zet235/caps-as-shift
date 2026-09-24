# Caps Lock → 左 Shift Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** 製作一個 Windows 常駐 EXE，將實體 Caps Lock 視為左 Shift，提供系統匣退出與可切換的登入自啟。

**Architecture:** 純狀態機追蹤 Caps Lock 與實體左 Shift 的聯集；Win32 低階鍵盤 hook 將狀態變化轉為 `SendInput`，略過自身注入事件。隱藏視窗管理 hook、訊息迴圈及系統匣；自啟封裝為可在測試用登錄檔路徑驗證的獨立單元。

**Tech Stack:** C++17、Win32 API、MinGW-w64 `g++` 16.2.0、PowerShell 5.1；不需套件管理器。

**Spec:** `docs/superpowers/specs/2026-09-24-caps-lock-to-left-shift-design.md`

## Global Constraints

- Windows GUI 單一 EXE；無第三方執行期依賴，編譯時靜態連結 MinGW 執行期。
- 只有實體 Caps Lock 與實體左 Shift 進入狀態機；其他按鍵、外部注入按鍵原樣放行。
- 自啟僅存放於目前使用者的 `HKCU\Software\Microsoft\Windows\CurrentVersion\Run`，由使用者自行勾選。
- 按鍵切換輸入法是否成功依目前 Windows 輸入法的「單按 Shift」設定實測；不能以單元測試聲稱驗證了實際輸入法。
- 目前資料夾不是 git 儲存庫；除非使用者另有要求，不建立提交。

## Review Focus

- Caps Lock 按住產生自動重複：不得送出多次 Shift-down；Task 1 狀態機測試。
- Caps Lock／左 Shift 交疊並反向放開：在最後一鍵放開前不可送 Shift-up；Task 1 狀態機測試。
- 外部注入的 Caps Lock 或自身注入的 Shift：不可變更實體按鍵狀態或無限遞迴；Task 2 分派測試。
- `SendInput` 失敗：hook 不得繼續吞鍵，需提示錯誤並嘗試釋放 Shift；Task 2 分派失敗測試與 Task 4 視窗錯誤處理檢查。
- 搬動 EXE 後登錄檔自啟路徑過期：選單不可錯誤顯示已啟用，再次勾選應更新為目前路徑；Task 3 隔離登錄檔測試。

---

## File Map

- `src/key_state.hpp`, `src/key_state.cpp`：無 Win32 相依的兩鍵聯集狀態機。
- `src/keyboard_mapper.hpp`, `src/keyboard_mapper.cpp`：分類 Win32 鍵盤事件、呼叫送鍵函式並回報攔截／送鍵失敗。
- `src/autostart.hpp`, `src/autostart.cpp`：讀寫 HKCU 的單一自啟項目，允許測試指定隔離子鍵。
- `src/main.cpp`：WinMain、mutex、隱藏視窗、hook 安裝與回收、系統匣選單與錯誤訊息。
- `tests/key_state_test.cpp`, `tests/keyboard_mapper_test.cpp`, `tests/autostart_test.cpp`：各單元的可執行測試程式（用 `assert`，無測試框架）。
- `build.ps1`：編譯測試程式、執行測試、靜態連結 `CapsAsShift.exe`。
- `README.md`：編譯、使用、退出與實機驗收方法。

### Task 1: 按鍵聯集狀態機

**Files:** Create `src/key_state.hpp`, `src/key_state.cpp`, `tests/key_state_test.cpp`.

**Interfaces:** Produces `enum class Source { Caps, LeftShift }`, `enum class Edge { Down, Up }`, `enum class ShiftAction { None, Down, Up }`; `KeyState::apply(Source, Edge) -> ShiftAction`, `KeyState::held() const -> bool`, `KeyState::clear() -> void`. No dependencies beyond the standard library.

- [ ] **Step 1: Write the failing test** in `tests/key_state_test.cpp`:

```cpp
#include "key_state.hpp"
#include <cassert>
int main() {
  KeyState s;
  assert(s.apply(Source::Caps, Edge::Down) == ShiftAction::Down);
  assert(s.apply(Source::Caps, Edge::Down) == ShiftAction::None); // repeat
  assert(s.apply(Source::LeftShift, Edge::Down) == ShiftAction::None);
  assert(s.apply(Source::Caps, Edge::Up) == ShiftAction::None);
  assert(s.held());
  assert(s.apply(Source::LeftShift, Edge::Up) == ShiftAction::Up);
  assert(!s.held());
  assert(s.apply(Source::Caps, Edge::Up) == ShiftAction::None); // stray release
  assert(s.apply(Source::LeftShift, Edge::Down) == ShiftAction::Down);
  assert(s.apply(Source::Caps, Edge::Down) == ShiftAction::None);
  assert(s.apply(Source::LeftShift, Edge::Up) == ShiftAction::None);
  assert(s.apply(Source::Caps, Edge::Up) == ShiftAction::Up);
  s.apply(Source::Caps, Edge::Down);
  s.clear();
  assert(!s.held());
}
```

- [ ] **Step 2: Verify red.** Run `g++ -std=c++17 -Isrc tests/key_state_test.cpp src/key_state.cpp -o tests/key_state_test.exe`; expect missing header/source or undefined `KeyState`.
- [ ] **Step 3: Implement the minimum** in the new header/source; logic is one transition of `(caps_ || left_)` before and after updating the selected flag:

```cpp
// src/key_state.hpp
#pragma once
enum class Source { Caps, LeftShift };
enum class Edge { Down, Up };
enum class ShiftAction { None, Down, Up };
class KeyState {
public:
  ShiftAction apply(Source source, Edge edge);
  bool held() const { return caps_ || left_; }
  void clear() { caps_ = left_ = false; }
private:
  bool caps_ = false;
  bool left_ = false;
};

// src/key_state.cpp
#include "key_state.hpp"
ShiftAction KeyState::apply(Source source, Edge edge) {
  const bool before = held();
  (source == Source::Caps ? caps_ : left_) = (edge == Edge::Down);
  if (before == held()) return ShiftAction::None;
  return held() ? ShiftAction::Down : ShiftAction::Up;
}
```

- [ ] **Step 4: Verify green.** Re-run compile command, then `& .\tests\key_state_test.exe`; expect exit code 0. Re-run with `-Wall -Wextra -Werror`.

### Task 2: Hook 事件分類、注入與失敗傳遞

**Files:** Create `src/keyboard_mapper.hpp`, `src/keyboard_mapper.cpp`, `tests/keyboard_mapper_test.cpp`; consume Task 1 files.

**Interfaces:** Consumes `KeyState::apply`, `held`, `clear`. Produces `constexpr ULONG_PTR kOwnInputMarker`, `struct DispatchResult { bool suppress; bool failed; }`, `KeyboardMapper::dispatch(WPARAM, const KBDLLHOOKSTRUCT&, const std::function<bool(bool)>&) -> DispatchResult`, `KeyboardMapper::held() const`, `KeyboardMapper::clear()`. The callback's `bool` is true for Shift-down, false for Shift-up; it returns false if `SendInput` did not send exactly one input.

- [ ] **Step 1: Write the failing test** in `tests/keyboard_mapper_test.cpp`. Construct `KBDLLHOOKSTRUCT` with `vkCode`, `scanCode`, `flags`, `dwExtraInfo`; use a lambda collecting `bool down` values. Check Caps down/up emit `{true, false}` and suppress; repeated down emits once; physical left Shift (`VK_LSHIFT` or `VK_SHIFT` with left scan code) overlaps Caps; right Shift and arbitrary letters pass through; `LLKHF_INJECTED` from another app passes through; own marker passes through; `WM_SYSKEYDOWN`/`WM_SYSKEYUP` work; a sender returning false yields `{false, true}` and `held()` is cleared. Example core assertion:

```cpp
KeyboardMapper mapper;
std::vector<bool> sent;
auto send = [&](bool down) { sent.push_back(down); return true; };
KBDLLHOOKSTRUCT caps{};
caps.vkCode = VK_CAPITAL;
assert(mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
assert(mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
assert(mapper.dispatch(WM_KEYUP, caps, send).suppress);
assert((sent == std::vector<bool>{true, false}));
caps.flags = LLKHF_INJECTED;
assert(!mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
caps.flags = 0;
auto fails = [](bool) { return false; };
assert(mapper.dispatch(WM_KEYDOWN, caps, fails).failed);
assert(!mapper.held());
```

- [ ] **Step 2: Verify red.** Run `g++ -std=c++17 -Wall -Wextra -Werror -Isrc tests/keyboard_mapper_test.cpp src/key_state.cpp src/keyboard_mapper.cpp -o tests/keyboard_mapper_test.exe`; expect missing mapper types.
- [ ] **Step 3: Implement the mapper** with the following decision order; include `<windows.h>` and `<functional>` in the header:

```cpp
// src/keyboard_mapper.hpp
#pragma once
#include "key_state.hpp"
#include <functional>
#include <windows.h>
constexpr ULONG_PTR kOwnInputMarker = static_cast<ULONG_PTR>(0x43415348494654ULL);
struct DispatchResult { bool suppress; bool failed; };
class KeyboardMapper {
public:
  DispatchResult dispatch(WPARAM message, const KBDLLHOOKSTRUCT& key,
                          const std::function<bool(bool)>& send);
  bool held() const { return state_.held(); }
  void clear() { state_.clear(); }
private:
  KeyState state_;
};

// src/keyboard_mapper.cpp: body of KeyboardMapper::dispatch
if (key.dwExtraInfo == kOwnInputMarker || (key.flags & LLKHF_INJECTED))
  return {false, false};
if (message != WM_KEYDOWN && message != WM_SYSKEYDOWN &&
    message != WM_KEYUP && message != WM_SYSKEYUP) return {false, false};
const bool caps = key.vkCode == VK_CAPITAL;
const bool left = key.vkCode == VK_LSHIFT ||
    (key.vkCode == VK_SHIFT && key.scanCode == MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC));
if (!caps && !left) return {false, false};
const auto edge = (message == WM_KEYDOWN || message == WM_SYSKEYDOWN) ? Edge::Down : Edge::Up;
const auto action = state_.apply(caps ? Source::Caps : Source::LeftShift, edge);
if (action != ShiftAction::None && !send(action == ShiftAction::Down)) {
  state_.clear();
  return {false, true};
}
return {true, false};
```

- [ ] **Step 4: Verify green.** Compile with the Step 2 command, run `& .\tests\keyboard_mapper_test.exe`, expect exit code 0; test both `VK_LSHIFT` and `VK_SHIFT` + left scan code, ensure `VK_RSHIFT` stays untouched.
- [ ] **Step 5: Wire the actual Win32 hook** in `src/main.cpp` (Task 4 extends this file). Use `SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, GetModuleHandleW(nullptr), 0)` and `GetMessageW` loop. In `HookProc`, only if `code == HC_ACTION`, call `mapper.dispatch(message, *reinterpret_cast<KBDLLHOOKSTRUCT*>(lParam), SendShift)`; `SendShift` creates one `INPUT` of type `INPUT_KEYBOARD`, `wScan = MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC)`, flags `KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP)`, `dwExtraInfo = kOwnInputMarker`, then returns `SendInput(1, &input, sizeof input) == 1`. Suppressed events return `1`; other events return `CallNextHookEx(nullptr, code, message, lParam)`. On `failed`, disable/uninstall hook, try `SendShift(false)`, then `PostMessageW(hwnd, WM_APP + 2, 0, 0)`; the window handler shows `MessageBoxW` and exits. A second instance exits if `CreateMutexW` reports `ERROR_ALREADY_EXISTS`. On shutdown unhook first, then if `mapper.held()` call `SendShift(false)` and `mapper.clear()`.

```cpp
// Illustrative essential callback branch in src/main.cpp
if (code == HC_ACTION && hookEnabled) {
  const auto result = mapper.dispatch(static_cast<WPARAM>(message),
    *reinterpret_cast<const KBDLLHOOKSTRUCT*>(lParam), SendShift);
  if (result.failed) { hookEnabled = false; PostMessageW(hwnd, WM_APP + 2, 0, 0); }
  if (result.suppress) return 1;
}
return CallNextHookEx(nullptr, code, message, lParam);
```

- [ ] **Step 6: Compile and smoke-test.** Run `g++ -std=c++17 -Wall -Wextra -Werror -static -mwindows src/main.cpp src/key_state.cpp src/keyboard_mapper.cpp -o CapsAsShift.exe -luser32`; expect an EXE. Start it once, inspect that a second launch returns without a second instance, then close it using a temporary `WM_CLOSE` handling path; Task 4 replaces that path with the tray menu. Do not simulate physical Caps Lock with `SendInput`: those events are intentionally excluded by the spec.

### Task 3: 使用者登入自啟

**Files:** Create `src/autostart.hpp`, `src/autostart.cpp`, `tests/autostart_test.cpp`.

**Interfaces:** Produces `Autostart(std::wstring subkey, std::wstring valueName, std::wstring executablePath)`, `std::optional<bool> enabled() const`, `bool setEnabled(bool enabled) const`. UI passes `L"Software\\Microsoft\\Windows\\CurrentVersion\\Run"`, `L"CapsAsShift"`, and an absolute `GetModuleFileNameW` path; test passes an isolated HKCU key. `nullopt` means registry I/O failed; `false` means absent or stale path.

- [ ] **Step 1: Write the failing test.** In `tests/autostart_test.cpp`, use `HKCU\Software\CapsAsShiftTests\<process-id>` as subkey and `RegDeleteTreeW` for cleanup. Assert absent entry is disabled, `setEnabled(true)` produces exactly `L"\"C:\\Program Files\\CapsAsShift.exe\""`, and `setEnabled(false)` removes it. Set the same value to `L"\"C:\\Old\\CapsAsShift.exe\""` with `RegSetValueExW`; assert `enabled() == false`, `setEnabled(true)` replaces it with the current path. Read the raw value with `RegQueryValueExW` and assert `REG_SZ` and correct terminating NUL.

```cpp
const std::wstring path = L"Software\\CapsAsShiftTests\\" + std::to_wstring(GetCurrentProcessId());
Autostart startup(path, L"CapsAsShift", L"C:\\Program Files\\CapsAsShift.exe");
assert(startup.enabled().has_value() && !*startup.enabled());
assert(startup.setEnabled(true));
assert(startup.enabled() == true);
HKEY key = nullptr;
assert(RegOpenKeyExW(HKEY_CURRENT_USER, path.c_str(), 0,
                     KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS);
const std::wstring expected = L"\"C:\\Program Files\\CapsAsShift.exe\"";
wchar_t buffer[256]{};
DWORD bytes = sizeof(buffer), type = 0;
assert(RegQueryValueExW(key, L"CapsAsShift", nullptr, &type,
       reinterpret_cast<BYTE*>(buffer), &bytes) == ERROR_SUCCESS);
assert(type == REG_SZ && std::wstring(buffer) == expected);
assert(bytes == (expected.size() + 1) * sizeof(wchar_t));
const std::wstring stale = L"\"C:\\Old\\CapsAsShift.exe\"";
assert(RegSetValueExW(key, L"CapsAsShift", 0, REG_SZ,
       reinterpret_cast<const BYTE*>(stale.c_str()),
       static_cast<DWORD>((stale.size() + 1) * sizeof(wchar_t))) == ERROR_SUCCESS);
RegCloseKey(key);
assert(startup.enabled() == false);
assert(startup.setEnabled(true));
assert(startup.enabled() == true);
assert(startup.setEnabled(false));
assert(startup.enabled() == false);
RegDeleteTreeW(HKEY_CURRENT_USER, path.c_str());
```

- [ ] **Step 2: Verify red.** Run `g++ -std=c++17 -Wall -Wextra -Werror -Isrc tests/autostart_test.cpp src/autostart.cpp -o tests/autostart_test.exe -ladvapi32`; expect missing type or symbols.
- [ ] **Step 3: Implement registry wrapper.** Use `RegOpenKeyExW` for reading (`ERROR_FILE_NOT_FOUND` = disabled; other failures = `nullopt`), two-pass `RegQueryValueExW` for a `REG_SZ` value (wrong type = disabled), compare the exact quoted current path, `RegCreateKeyExW` + `RegSetValueExW` to enable, `RegDeleteValueW` to disable (`ERROR_FILE_NOT_FOUND` = success); close handles on every path. Re-query state after a UI operation instead of guessing. Example write expression:

```cpp
const std::wstring command = L"\"" + executablePath_ + L"\"";
const auto bytes = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
const LONG result = RegSetValueExW(key, valueName_.c_str(), 0, REG_SZ,
  reinterpret_cast<const BYTE*>(command.c_str()), bytes);
return result == ERROR_SUCCESS;
```

- [ ] **Step 4: Verify green.** Compile and run `& .\tests\autostart_test.exe`, expect exit code 0; inspect that the temporary HKCU key was removed and the real Run key was untouched.

### Task 4: 系統匣、完成打包與實機驗收

**Files:** Modify `src/main.cpp`; create `build.ps1`, `README.md`; consume all Tasks 1–3 files/tests.

**Interfaces:** `main.cpp` consumes `Autostart::enabled/setEnabled` and `KeyboardMapper::dispatch/held/clear`; outputs GUI EXE `CapsAsShift.exe` with tray menu commands `Exit` and `Start with Windows`.

- [ ] **Step 1: Add the tray lifecycle and menu** to `src/main.cpp`. Register a window class, create a hidden top-level window (needed for `TaskbarCreated` broadcasts), register `TaskbarCreated`, add an icon with `Shell_NotifyIconW(NIM_ADD, ...)` using `LoadIconW(nullptr, IDI_APPLICATION)`, `NIF_ICON | NIF_MESSAGE | NIF_TIP`, and tray callback `WM_APP + 1`. If initial icon creation fails, show an error and exit; do not leave an unmanageable hidden process. On right click, `SetForegroundWindow(hwnd)`, `TrackPopupMenu` for a checked auto-start item based on `startup.enabled()` and an exit item. On double-click, show the same menu. Handle auto-start toggling with `setEnabled`, then re-read registry and report errors by `MessageBoxW`; handle Exit and `WM_ENDSESSION` via one idempotent cleanup routine. On `TaskbarCreated`, re-add icon. On `WM_APP + 2` (injection error), unhook, try Shift-up, show an error box, exit. Keep hook callback fast: schedule error UI by `PostMessageW`, never display a modal dialog within the callback. Obtain the executable path via a growing `GetModuleFileNameW` buffer rather than truncating long paths.

```cpp
// Core menu dispatch in WndProc (IDs are local constexpr values).
case WM_COMMAND:
  if (LOWORD(wParam) == kExit) { DestroyWindow(hwnd); return 0; }
  if (LOWORD(wParam) == kAutostart) {
    const auto current = startup.enabled();
    if (!current || !startup.setEnabled(!*current) ||
        startup.enabled() != std::optional<bool>(!*current))
      MessageBoxW(hwnd, L"無法更新登入自啟設定", L"CapsAsShift", MB_ICONERROR);
    return 0;
  }
  break;
case WM_DESTROY:
  CleanupHookAndTray();
  PostQuitMessage(0);
  return 0;
```

- [ ] **Step 2: Add reproducible build/test script** `build.ps1` using `$ErrorActionPreference = 'Stop'`, three `g++ -std=c++17 -Wall -Wextra -Werror -Isrc` test compiles with exact source/link combinations from Tasks 1–3, run each resulting EXE and check `$LASTEXITCODE`, then build the GUI app:

```powershell
& g++ -std=c++17 -O2 -Wall -Wextra -Werror -static -mwindows `
  src/main.cpp src/key_state.cpp src/keyboard_mapper.cpp src/autostart.cpp `
  -o CapsAsShift.exe -luser32 -lshell32 -ladvapi32
if ($LASTEXITCODE -ne 0) { throw 'GUI EXE build failed' }
```

   Verify DLL imports with `objdump -p .\CapsAsShift.exe` and ensure no `libstdc++-6.dll`, `libgcc_s_*.dll`, or `libwinpthread-1.dll` appears; Win32 system DLLs are expected. Quote the EXE path when launching via Explorer and when setting HKCU Run. If any compiler warning/error occurs, fix it rather than dropping `-Werror`.

- [ ] **Step 3: Write `README.md`** with build command `powershell -ExecutionPolicy Bypass -File .\build.ps1`, startup by launching `CapsAsShift.exe`, tray exit and checkbox, user-scoped registry location, and precise manual tests: compare single physical Shift and single Caps Lock in the actual IME; test Caps+letter, sustained Caps, alternate release orders, right Shift, duplicate launch, Explorer restart, tray exit, self-start toggle and new logon. State the elevated-window limitation and that forcibly terminating the process cannot guarantee a key-up.
- [ ] **Step 4: Run script and smoke tests.** Run `powershell -ExecutionPolicy Bypass -File .\build.ps1` (all three tests exit 0 and EXE exists); check imports with `objdump -p .\CapsAsShift.exe`; launch EXE and check one tray icon, menu, exit, and absence of residual pressed Shift. Test self-start against actual HKCU Run only when explicitly toggled in the tray. Perform physical keyboard/IME and new-logon checks when interactive access is available; report any unverified checks accurately rather than marking them passing.
