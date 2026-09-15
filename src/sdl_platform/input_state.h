#pragma once

struct InputState {
  int mouseDX = 0;
  int mouseDY = 0;

  bool rmbDown = false;

  bool forward  = false;   // W
  bool backward = false;   // S
  bool left     = false;   // A
  bool right    = false;   // D
  bool up       = false;   // Space
  bool down     = false;   // Ctrl

  bool sprint   = false;   // Shift

  bool quit     = false;
};