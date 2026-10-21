#pragma once

#include <functional>
#include <windows.h>

constexpr ULONG_PTR kOwnInputMarker = static_cast<ULONG_PTR>(0x43415348494654ULL);

struct DispatchResult {
  bool suppress;
  bool failed;
};

class KeyboardMapper {
 public:
  DispatchResult dispatch(WPARAM message, const KBDLLHOOKSTRUCT& key,
                          const std::function<bool(bool)>& send);
  // The sampled Windows state may belong to a physical key or another injector.
  void primeLeftShift() { inheritedLeft_ = true; }
  bool held() const { return caps_ || left_ || inheritedLeft_; }
  bool stop(const std::function<bool(bool)>& send);

 private:
  bool caps_ = false;
  bool left_ = false;
  bool inheritedLeft_ = false;
  bool releasePending_ = false;
};
