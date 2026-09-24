#include "autostart.hpp"

#include <windows.h>

#include <utility>
#include <vector>

Autostart::Autostart(std::wstring subkey, std::wstring valueName,
                     std::wstring executablePath)
    : subkey_(std::move(subkey)), valueName_(std::move(valueName)),
      executablePath_(std::move(executablePath)) {}

std::optional<bool> Autostart::enabled() const {
  HKEY key = nullptr;
  const LONG opened = RegOpenKeyExW(HKEY_CURRENT_USER, subkey_.c_str(), 0,
                                    KEY_QUERY_VALUE, &key);
  if (opened == ERROR_FILE_NOT_FOUND) return false;
  if (opened != ERROR_SUCCESS) return std::nullopt;

  DWORD type = 0;
  DWORD bytes = 0;
  const LONG measured = RegQueryValueExW(key, valueName_.c_str(), nullptr,
                                          &type, nullptr, &bytes);
  if (measured != ERROR_SUCCESS) {
    RegCloseKey(key);
    return measured == ERROR_FILE_NOT_FOUND ? std::optional<bool>(false)
                                             : std::nullopt;
  }
  if (type != REG_SZ || bytes < sizeof(wchar_t) || bytes % sizeof(wchar_t) != 0) {
    RegCloseKey(key);
    return false;
  }

  std::vector<wchar_t> value(bytes / sizeof(wchar_t) + 1, L'\0');
  const LONG fetched = RegQueryValueExW(key, valueName_.c_str(), nullptr,
                                         &type, reinterpret_cast<BYTE*>(value.data()),
                                         &bytes);
  RegCloseKey(key);
  if (fetched != ERROR_SUCCESS) return std::nullopt;
  if (type != REG_SZ || bytes < sizeof(wchar_t) ||
      bytes % sizeof(wchar_t) != 0 || value[bytes / sizeof(wchar_t) - 1] != L'\0')
    return false;
  return std::wstring(value.data()) == L"\"" + executablePath_ + L"\"";
}

bool Autostart::setEnabled(bool enabled) const {
  HKEY key = nullptr;
  LONG opened = ERROR_SUCCESS;
  if (enabled) {
    opened = RegCreateKeyExW(HKEY_CURRENT_USER, subkey_.c_str(), 0, nullptr,
                             REG_OPTION_NON_VOLATILE, KEY_SET_VALUE, nullptr,
                             &key, nullptr);
  } else {
    opened = RegOpenKeyExW(HKEY_CURRENT_USER, subkey_.c_str(), 0,
                           KEY_SET_VALUE, &key);
    if (opened == ERROR_FILE_NOT_FOUND) return true;
  }
  if (opened != ERROR_SUCCESS) return false;

  LONG result = ERROR_SUCCESS;
  if (enabled) {
    const std::wstring command = L"\"" + executablePath_ + L"\"";
    const auto bytes = static_cast<DWORD>((command.size() + 1) * sizeof(wchar_t));
    result = RegSetValueExW(key, valueName_.c_str(), 0, REG_SZ,
                            reinterpret_cast<const BYTE*>(command.c_str()), bytes);
  } else {
    result = RegDeleteValueW(key, valueName_.c_str());
  }
  RegCloseKey(key);
  return result == ERROR_SUCCESS || (!enabled && result == ERROR_FILE_NOT_FOUND);
}
