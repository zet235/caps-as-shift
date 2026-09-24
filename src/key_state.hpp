#pragma once

enum class Source { Caps, LeftShift };
enum class Edge { Down, Up };
enum class ShiftAction { None, Down, Up };

class KeyState {
 public:
  ShiftAction apply(Source source, Edge edge);
  bool held() const { return caps_ || left_; }
  bool held(Source source) const { return source == Source::Caps ? caps_ : left_; }
  void clear() { caps_ = left_ = false; }

 private:
  bool caps_ = false;
  bool left_ = false;
};
