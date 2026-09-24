#include "key_state.hpp"

#include <cassert>

int main() {
  KeyState state;
  assert(state.apply(Source::Caps, Edge::Down) == ShiftAction::Down);
  assert(state.apply(Source::Caps, Edge::Down) == ShiftAction::None);
  assert(state.apply(Source::LeftShift, Edge::Down) == ShiftAction::None);
  assert(state.apply(Source::Caps, Edge::Up) == ShiftAction::None);
  assert(state.held());
  assert(state.apply(Source::LeftShift, Edge::Up) == ShiftAction::Up);
  assert(!state.held());
  assert(state.apply(Source::Caps, Edge::Up) == ShiftAction::None);

  assert(state.apply(Source::LeftShift, Edge::Down) == ShiftAction::Down);
  assert(state.apply(Source::Caps, Edge::Down) == ShiftAction::None);
  assert(state.apply(Source::LeftShift, Edge::Up) == ShiftAction::None);
  assert(state.apply(Source::Caps, Edge::Up) == ShiftAction::Up);

  state.apply(Source::Caps, Edge::Down);
  state.clear();
  assert(!state.held());
}
