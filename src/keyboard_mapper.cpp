#include "keyboard_mapper.hpp"

DispatchResult KeyboardMapper::dispatch(WPARAM message,
                                         const KBDLLHOOKSTRUCT& key,
                                         const std::function<bool(bool)>& send) {
  if (key.dwExtraInfo == kOwnInputMarker) return {false, false};

  const bool down = message == WM_KEYDOWN || message == WM_SYSKEYDOWN;
  if (!down && message != WM_KEYUP && message != WM_SYSKEYUP)
    return {false, false};

  const bool caps = key.vkCode == VK_CAPITAL;
  const bool left = key.vkCode == VK_LSHIFT ||
      (key.vkCode == VK_SHIFT &&
       key.scanCode == MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC));
  if (!caps && !left) return {false, false};

  if (key.flags & LLKHF_INJECTED) {
    // An external release can resolve an inherited startup hold, but must never
    // clear a confirmed physical hold or change our synthetic-output obligation.
    if (left && !down) inheritedLeft_ = false;
    return {false, false};
  }

  const bool before = held();
  // A release with no recorded press (made before the hook saw it) needs its
  // real key-up. Left Shift counts as recorded while anything holds Shift.
  if (!down && !(caps ? caps_ : before)) return {false, false};
  if (left) inheritedLeft_ = false;
  (caps ? caps_ : left_) = down;
  if (before != held()) {
    if (!send(held())) return {false, true};
    releasePending_ = held();
  }
  return {true, false};
}

bool KeyboardMapper::stop(const std::function<bool(bool)>& send) {
  // Hand a held native modifier back to Windows; its real key-up will be passed
  // through after unhooking. Only a Caps-owned synthetic hold needs releasing.
  if (left_ || inheritedLeft_) releasePending_ = false;
  if (releasePending_ && send(false)) releasePending_ = false;
  caps_ = left_ = inheritedLeft_ = false;
  return !releasePending_;
}
