#pragma once

#include <optional>
#include <string>

class Autostart {
 public:
  Autostart(std::wstring subkey, std::wstring valueName,
            std::wstring executablePath);

  std::optional<bool> enabled() const;
  bool setEnabled(bool enabled) const;

 private:
  std::wstring subkey_;
  std::wstring valueName_;
  std::wstring executablePath_;
};
