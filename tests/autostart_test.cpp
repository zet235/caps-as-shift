#include "autostart.hpp"

#include <windows.h>

#include <cassert>
#include <string>

int main() {
  const std::wstring subkey = L"Software\\CapsAsShiftTests\\" +
                              std::to_wstring(GetCurrentProcessId()) + L"_" +
                              std::to_wstring(GetTickCount64());
  Autostart startup(subkey, L"CapsAsShift", L"C:\\Program Files\\CapsAsShift.exe");
  assert(startup.enabled() == false);
  assert(startup.setEnabled(true));
  assert(startup.enabled() == true);

  HKEY key = nullptr;
  assert(RegOpenKeyExW(HKEY_CURRENT_USER, subkey.c_str(), 0,
                       KEY_QUERY_VALUE | KEY_SET_VALUE, &key) == ERROR_SUCCESS);
  wchar_t buffer[256]{};
  DWORD bytes = sizeof(buffer);
  DWORD type = 0;
  assert(RegQueryValueExW(key, L"CapsAsShift", nullptr, &type,
                          reinterpret_cast<BYTE*>(buffer), &bytes) == ERROR_SUCCESS);
  const std::wstring expected = L"\"C:\\Program Files\\CapsAsShift.exe\"";
  assert(type == REG_SZ && std::wstring(buffer) == expected);
  assert(bytes == (expected.size() + 1) * sizeof(wchar_t));

  const std::wstring stale = L"\"C:\\Old\\CapsAsShift.exe\"";
  assert(RegSetValueExW(key, L"CapsAsShift", 0, REG_SZ,
                        reinterpret_cast<const BYTE*>(stale.c_str()),
                        static_cast<DWORD>((stale.size() + 1) * sizeof(wchar_t))) ==
         ERROR_SUCCESS);
  assert(startup.enabled() == false);
  assert(startup.setEnabled(true));
  assert(startup.enabled() == true);

  const DWORD wrongType = 17;
  assert(RegSetValueExW(key, L"CapsAsShift", 0, REG_DWORD,
                        reinterpret_cast<const BYTE*>(&wrongType), sizeof(wrongType)) ==
         ERROR_SUCCESS);
  assert(startup.enabled() == false);

  RegCloseKey(key);
  assert(startup.setEnabled(false));
  assert(startup.enabled() == false);
  assert(startup.setEnabled(false));
  assert(RegDeleteTreeW(HKEY_CURRENT_USER, subkey.c_str()) == ERROR_SUCCESS);
}
