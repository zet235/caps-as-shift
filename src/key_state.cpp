#include "key_state.hpp"

ShiftAction KeyState::apply(Source source, Edge edge) {
  const bool before = held();
  (source == Source::Caps ? caps_ : left_) = (edge == Edge::Down);
  if (before == held()) return ShiftAction::None;
  return held() ? ShiftAction::Down : ShiftAction::Up;
}
