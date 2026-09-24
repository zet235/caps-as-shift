#define UNICODE
#define _UNICODE
#include "autostart.hpp"
#include "keyboard_mapper.hpp"
#include "../assets/resource.h"

#include <shellapi.h>
#include <windows.h>

#include <memory>
#include <string>
#include <vector>

namespace {

constexpr wchar_t kWindowClass[] = L"CapsAsShiftWindow";
constexpr UINT kTrayMessage = WM_APP + 1;
constexpr UINT kHookError = WM_APP + 2;
constexpr UINT kTrayId = 1;
constexpr WORD kExit = 2;
constexpr WORD kAutostart = 3;

KeyboardMapper mapper;
HHOOK hook = nullptr;
HWND window = nullptr;
bool hookEnabled = false;
bool trayInstalled = false;
UINT taskbarCreated = 0;
std::unique_ptr<Autostart> startup;

bool SendShift(bool down) {
  INPUT input{};
  input.type = INPUT_KEYBOARD;
  input.ki.wScan = static_cast<WORD>(MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC));
  input.ki.dwFlags = KEYEVENTF_SCANCODE | (down ? 0 : KEYEVENTF_KEYUP);
  input.ki.dwExtraInfo = kOwnInputMarker;
  return SendInput(1, &input, sizeof(input)) == 1;
}

void StopHook() {
  hookEnabled = false;
  if (hook) {
    UnhookWindowsHookEx(hook);
    hook = nullptr;
  }
  mapper.stop(SendShift);
}

bool AddTray(HWND hwnd) {
  NOTIFYICONDATAW icon{};
  icon.cbSize = sizeof(icon);
  icon.hWnd = hwnd;
  icon.uID = kTrayId;
  icon.uFlags = NIF_MESSAGE | NIF_ICON | NIF_TIP;
  icon.uCallbackMessage = kTrayMessage;
  icon.hIcon = static_cast<HICON>(LoadImageW(
      GetModuleHandleW(nullptr), MAKEINTRESOURCEW(IDI_CAPS_AS_SHIFT), IMAGE_ICON,
      GetSystemMetrics(SM_CXSMICON), GetSystemMetrics(SM_CYSMICON), LR_SHARED));
  if (!icon.hIcon) return false;
  lstrcpynW(icon.szTip, L"CapsAsShift - Caps Lock = Left Shift",
            sizeof(icon.szTip) / sizeof(icon.szTip[0]));
  trayInstalled = Shell_NotifyIconW(NIM_ADD, &icon) != FALSE;
  return trayInstalled;
}

void RemoveTray() {
  if (!trayInstalled) return;
  NOTIFYICONDATAW icon{};
  icon.cbSize = sizeof(icon);
  icon.hWnd = window;
  icon.uID = kTrayId;
  Shell_NotifyIconW(NIM_DELETE, &icon);
  trayInstalled = false;
}

void ShowMenu(HWND hwnd) {
  const auto enabled = startup->enabled();
  if (!enabled) {
    MessageBoxW(hwnd, L"Unable to read the startup setting.", L"CapsAsShift", MB_ICONERROR);
  }
  HMENU menu = CreatePopupMenu();
  if (!menu) return;
  const UINT startupFlags = !enabled ? MF_GRAYED : (*enabled ? MF_CHECKED : 0);
  AppendMenuW(menu, MF_STRING | startupFlags, kAutostart,
              L"Start with Windows");
  AppendMenuW(menu, MF_SEPARATOR, 0, nullptr);
  AppendMenuW(menu, MF_STRING, kExit, L"Exit");
  POINT point{};
  GetCursorPos(&point);
  SetForegroundWindow(hwnd);
  const UINT command = TrackPopupMenu(menu, TPM_RETURNCMD | TPM_RIGHTBUTTON,
                                       point.x, point.y, 0, hwnd, nullptr);
  DestroyMenu(menu);
  if (command) PostMessageW(hwnd, WM_COMMAND, command, 0);
  PostMessageW(hwnd, WM_NULL, 0, 0);
}

std::wstring ExecutablePath() {
  std::vector<wchar_t> path(MAX_PATH);
  while (path.size() <= 32768) {
    const DWORD copied = GetModuleFileNameW(nullptr, path.data(),
                                              static_cast<DWORD>(path.size()));
    if (copied == 0) return {};
    if (copied < path.size()) return std::wstring(path.data(), copied);
    path.resize(path.size() * 2);
  }
  return {};
}

LRESULT CALLBACK HookProc(int code, WPARAM message, LPARAM detail) {
  if (code == HC_ACTION && hookEnabled) {
    const auto result = mapper.dispatch(
        message, *reinterpret_cast<const KBDLLHOOKSTRUCT*>(detail), SendShift);
    if (result.failed) {
      hookEnabled = false;
      PostMessageW(window, kHookError, 0, 0);
    }
    if (result.suppress) return 1;
  }
  return CallNextHookEx(nullptr, code, message, detail);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam) {
  if (message == taskbarCreated && taskbarCreated != 0) {
    trayInstalled = false;
    if (!AddTray(hwnd)) {
      MessageBoxW(hwnd, L"Unable to restore the tray icon.", L"CapsAsShift", MB_ICONERROR);
      DestroyWindow(hwnd);
    }
    return 0;
  }
  switch (message) {
    case kTrayMessage:
      if (lParam == WM_RBUTTONUP || lParam == WM_LBUTTONDBLCLK) ShowMenu(hwnd);
      return 0;
    case WM_COMMAND:
      if (LOWORD(wParam) == kExit) {
        DestroyWindow(hwnd);
        return 0;
      }
      if (LOWORD(wParam) == kAutostart) {
        const auto current = startup->enabled();
        if (!current || !startup->setEnabled(!*current) ||
            startup->enabled() != std::optional<bool>(!*current)) {
          MessageBoxW(hwnd, L"Unable to update the startup setting.", L"CapsAsShift", MB_ICONERROR);
        }
        return 0;
      }
      break;
    case kHookError:
      StopHook();
      MessageBoxW(hwnd, L"Unable to send Left Shift input. CapsAsShift will now exit.",
                  L"CapsAsShift", MB_OK | MB_ICONERROR);
      DestroyWindow(hwnd);
      return 0;
    case WM_CLOSE:
      DestroyWindow(hwnd);
      return 0;
    case WM_ENDSESSION:
      if (wParam) {
        StopHook();
        RemoveTray();
      }
      return 0;
    case WM_DESTROY:
      StopHook();
      RemoveTray();
      PostQuitMessage(0);
      return 0;
  }
  return DefWindowProcW(hwnd, message, wParam, lParam);
}

}  // namespace

int WINAPI WinMain(HINSTANCE instance, HINSTANCE, LPSTR, int) {
  HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\CapsAsShift.Unique");
  if (!mutex) {
    MessageBoxW(nullptr, L"Unable to create the single-instance lock.", L"CapsAsShift", MB_ICONERROR);
    return 1;
  }
  if (GetLastError() == ERROR_ALREADY_EXISTS) {
    CloseHandle(mutex);
    return 0;
  }

  WNDCLASSW klass{};
  klass.lpfnWndProc = WndProc;
  klass.hInstance = instance;
  klass.hIcon = LoadIconW(instance, MAKEINTRESOURCEW(IDI_CAPS_AS_SHIFT));
  klass.lpszClassName = kWindowClass;
  if (!RegisterClassW(&klass)) {
    MessageBoxW(nullptr, L"Unable to register the window class.", L"CapsAsShift", MB_ICONERROR);
    CloseHandle(mutex);
    return 1;
  }

  window = CreateWindowExW(0, kWindowClass, L"CapsAsShift", WS_OVERLAPPED,
                           0, 0, 0, 0, nullptr, nullptr, instance, nullptr);
  if (!window) {
    MessageBoxW(nullptr, L"Unable to create the message window.", L"CapsAsShift", MB_ICONERROR);
    CloseHandle(mutex);
    return 1;
  }

  const std::wstring path = ExecutablePath();
  if (path.empty()) {
    MessageBoxW(window, L"Unable to determine the executable path.", L"CapsAsShift", MB_ICONERROR);
    DestroyWindow(window);
    CloseHandle(mutex);
    return 1;
  }
  startup = std::make_unique<Autostart>(
      L"Software\\Microsoft\\Windows\\CurrentVersion\\Run", L"CapsAsShift",
      path);
  taskbarCreated = RegisterWindowMessageW(L"TaskbarCreated");
  if (!taskbarCreated || !AddTray(window)) {
    MessageBoxW(window, L"Unable to create the tray icon.", L"CapsAsShift", MB_ICONERROR);
    DestroyWindow(window);
    CloseHandle(mutex);
    return 1;
  }

  hook = SetWindowsHookExW(WH_KEYBOARD_LL, HookProc, instance, 0);
  if (!hook) {
    MessageBoxW(window, L"Unable to install the keyboard hook.", L"CapsAsShift", MB_ICONERROR);
    DestroyWindow(window);
    CloseHandle(mutex);
    return 1;
  }
  if (GetAsyncKeyState(VK_LSHIFT) & 0x8000) mapper.primeLeftShift();
  hookEnabled = true;

  MSG message{};
  while (GetMessageW(&message, nullptr, 0, 0) > 0) {
    TranslateMessage(&message);
    DispatchMessageW(&message);
  }
  StopHook();
  ReleaseMutex(mutex);
  CloseHandle(mutex);
  return static_cast<int>(message.wParam);
}
