#include "keyboard_mapper.hpp"

#include <cassert>
#include <vector>

int main() {
  KeyboardMapper mapper;
  std::vector<bool> sent;
  const auto send = [&](bool down) {
    sent.push_back(down);
    return true;
  };

  KBDLLHOOKSTRUCT alreadyHeldLeft{};
  alreadyHeldLeft.vkCode = VK_LSHIFT;
  assert(!mapper.dispatch(WM_KEYUP, alreadyHeldLeft, send).suppress);
  assert(sent.empty());

  KeyboardMapper startedWhileLeftHeld;
  startedWhileLeftHeld.primeLeftShift();
  assert(startedWhileLeftHeld.dispatch(WM_KEYDOWN, alreadyHeldLeft, send).suppress);
  assert(startedWhileLeftHeld.dispatch(WM_KEYUP, alreadyHeldLeft, send).suppress);
  assert((sent == std::vector<bool>{false}));
  sent.clear();

  KBDLLHOOKSTRUCT caps{};
  caps.vkCode = VK_CAPITAL;
  assert(mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
  assert(mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
  assert(mapper.dispatch(WM_KEYUP, caps, send).suppress);
  assert((sent == std::vector<bool>{true, false}));

  caps.flags = LLKHF_INJECTED;
  assert(!mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
  assert(!mapper.held());
  caps.flags = 0;
  caps.dwExtraInfo = kOwnInputMarker;
  assert(!mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
  assert(!mapper.held());
  caps.dwExtraInfo = 0;

  KBDLLHOOKSTRUCT left{};
  left.vkCode = VK_SHIFT;
  left.scanCode = MapVirtualKeyW(VK_LSHIFT, MAPVK_VK_TO_VSC);
  assert(mapper.dispatch(WM_SYSKEYDOWN, left, send).suppress);
  assert(mapper.dispatch(WM_KEYDOWN, caps, send).suppress);
  assert(mapper.dispatch(WM_SYSKEYUP, left, send).suppress);
  assert(mapper.held());
  assert(mapper.dispatch(WM_KEYUP, caps, send).suppress);
  assert((sent == std::vector<bool>{true, false, true, false}));

  left.vkCode = VK_LSHIFT;
  assert(mapper.dispatch(WM_KEYDOWN, left, send).suppress);
  assert(mapper.dispatch(WM_KEYUP, left, send).suppress);
  left.vkCode = VK_RSHIFT;
  left.scanCode = MapVirtualKeyW(VK_RSHIFT, MAPVK_VK_TO_VSC);
  assert(!mapper.dispatch(WM_KEYDOWN, left, send).suppress);
  left.vkCode = VK_SHIFT;
  assert(!mapper.dispatch(WM_KEYDOWN, left, send).suppress);
  left.vkCode = 'A';
  assert(!mapper.dispatch(WM_KEYDOWN, left, send).suppress);
  assert(!mapper.dispatch(WM_APP, caps, send).suppress);

  KeyboardMapper broken;
  const auto fails = [](bool) { return false; };
  const auto result = broken.dispatch(WM_KEYDOWN, caps, fails);
  assert(!result.suppress && result.failed);
  assert(broken.stop(send));
  assert(broken.dispatch(WM_KEYDOWN, caps, send).suppress);
  const auto failedUp = broken.dispatch(WM_KEYUP, caps, fails);
  assert(!failedUp.suppress && failedUp.failed && !broken.held());
}
