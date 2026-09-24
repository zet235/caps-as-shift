// Exercise the real mapper, cleanup and menu paths without injecting keys,
// displaying UI or touching the user's registry.
#define UNICODE
#define _UNICODE
#include <windows.h>
#include <shellapi.h>

#include <iostream>
#include <vector>

namespace probe {
std::vector<bool> sends;
bool rejectRelease = false;
bool exitAvailable = false;
bool exitPosted = false;

UINT WINAPI Send(UINT count, LPINPUT inputs, int) {
  if (count != 1) return 0;
  const bool down = (inputs[0].ki.dwFlags & KEYEVENTF_KEYUP) == 0;
  sends.push_back(down);
  return !down && rejectRelease ? 0 : 1;
}

LSTATUS WINAPI OpenKey(HKEY, LPCWSTR, DWORD, REGSAM, PHKEY) {
  return ERROR_ACCESS_DENIED;
}

int WINAPI Message(HWND, LPCWSTR, LPCWSTR, UINT) { return IDOK; }
BOOL WINAPI Foreground(HWND) { return TRUE; }
BOOL WINAPI Cursor(LPPOINT point) { *point = {}; return TRUE; }
BOOL WINAPI Post(HWND, UINT message, WPARAM command, LPARAM) {
  if (message == WM_COMMAND && command == 2) exitPosted = true;
  return TRUE;
}
BOOL WINAPI Menu(HMENU menu, UINT, int, int, int, HWND, const RECT*) {
  const UINT flags = GetMenuState(menu, 2, MF_BYCOMMAND);
  exitAvailable = flags != static_cast<UINT>(-1) &&
                  !(flags & (MF_DISABLED | MF_GRAYED));
  return 2;
}
}  // namespace probe

// Keep the app logic intact; replace only desktop/registry side effects.
#define SendInput probe::Send
#define MessageBoxW probe::Message
#define TrackPopupMenu probe::Menu
#define SetForegroundWindow probe::Foreground
#define GetCursorPos probe::Cursor
#define PostMessageW probe::Post
#define RegOpenKeyExW probe::OpenKey
#include "../src/main.cpp"
#include "../src/autostart.cpp"
#undef SendInput
#undef MessageBoxW
#undef TrackPopupMenu
#undef SetForegroundWindow
#undef GetCursorPos
#undef PostMessageW
#undef RegOpenKeyExW

int main() {
  int failures = 0;
  const auto check = [&](bool condition, const char* name) {
    std::cout << (condition ? "PASS: " : "FAIL: ") << name << '\n';
    if (!condition) ++failures;
  };
  KBDLLHOOKSTRUCT caps{};
  caps.vkCode = VK_CAPITAL;
  KBDLLHOOKSTRUCT left{};
  left.vkCode = VK_LSHIFT;

  mapper.dispatch(WM_KEYDOWN, caps, SendShift);
  probe::rejectRelease = true;
  check(mapper.dispatch(WM_KEYUP, caps, SendShift).failed,
        "release failure reported");
  probe::rejectRelease = false;
  StopHook(); // Shutdown happens before the queued error notification.
  check(probe::sends == std::vector<bool>({true, false, false}),
        "shutdown retries a failed synthetic release");

  mapper = KeyboardMapper{};
  probe::sends.clear();
  mapper.dispatch(WM_KEYDOWN, left, SendShift);
  StopHook();
  check(probe::sends == std::vector<bool>({true}),
        "shutdown preserves a physically held left Shift");

  mapper = KeyboardMapper{};
  probe::sends.clear();
  mapper.dispatch(WM_KEYDOWN, caps, SendShift);
  StopHook();
  check(probe::sends == std::vector<bool>({true, false}),
        "shutdown releases a Caps-only hold");

  mapper = KeyboardMapper{};
  probe::sends.clear();
  mapper.dispatch(WM_KEYDOWN, caps, SendShift);
  mapper.dispatch(WM_KEYDOWN, left, SendShift);
  StopHook();
  check(probe::sends == std::vector<bool>({true}),
        "shutdown preserves left Shift when both sources are held");

  mapper = KeyboardMapper{};
  probe::sends.clear();
  mapper.primeLeftShift();
  StopHook();
  check(probe::sends.empty(), "shutdown does not release an inherited hold");

  mapper = KeyboardMapper{};
  probe::sends.clear();
  mapper.dispatch(WM_KEYDOWN, caps, SendShift);
  probe::rejectRelease = true;
  check(!mapper.stop(SendShift), "failed cleanup reports outstanding release");
  probe::rejectRelease = false;
  check(mapper.stop(SendShift), "cleanup retries on the next call");
  check(probe::sends == std::vector<bool>({true, false, false}),
        "failed cleanup preserves its release obligation");

  mapper = KeyboardMapper{};
  probe::sends.clear();
  mapper.primeLeftShift();
  left.flags = LLKHF_INJECTED;
  mapper.dispatch(WM_KEYUP, left, SendShift);
  mapper.dispatch(WM_KEYDOWN, caps, SendShift);
  mapper.dispatch(WM_KEYUP, caps, SendShift);
  check(probe::sends == std::vector<bool>({true, false}),
        "inherited injected Shift release does not disable Caps mapping");

  startup = std::make_unique<Autostart>(L"unused", L"unused", L"unused");
  ShowMenu(nullptr);
  check(probe::exitAvailable && probe::exitPosted,
        "Exit remains usable when autostart read fails");
  return failures == 0 ? 0 : 1;
}
