#include "autostart.hpp"

#include <windows.h>

#include <utility>

Autostart::Autostart(std::wstring subkey, std::wstring valueName,
                     const std::wstring& executablePath)
    : subkey_(std::move(subkey)), valueName_(std::move(valueName)),
      command_(L"\"" + executablePath + L"\"") {}

std::optional<bool> Autostart::enabled() const {
  // Anything longer than our own command cannot match, so cap the read there.
  std::wstring value(command_.size() + 1, L'\0');
  auto bytes = static_cast<DWORD>(value.size() * sizeof(wchar_t));
  const LSTATUS status =
      RegGetValueW(HKEY_CURRENT_USER, subkey_.c_str(), valueName_.c_str(),
                   RRF_RT_REG_SZ, nullptr, value.data(), &bytes);
  if (status == ERROR_SUCCESS) return value.c_str() == command_;
  if (status == ERROR_FILE_NOT_FOUND || status == ERROR_MORE_DATA ||
      status == ERROR_UNSUPPORTED_TYPE)
    return false;
  return std::nullopt;
}

bool Autostart::setEnabled(bool enabled) const {
  if (enabled) {
    const auto bytes = static_cast<DWORD>((command_.size() + 1) * sizeof(wchar_t));
    return RegSetKeyValueW(HKEY_CURRENT_USER, subkey_.c_str(), valueName_.c_str(),
                           REG_SZ, command_.c_str(), bytes) == ERROR_SUCCESS;
  }
  const LSTATUS status = RegDeleteKeyValueW(HKEY_CURRENT_USER, subkey_.c_str(),
                                            valueName_.c_str());
  return status == ERROR_SUCCESS || status == ERROR_FILE_NOT_FOUND;
}
