#ifndef RENDERER_SRC_SDL_PLATFORM_INPUT_STATE_H_
#define RENDERER_SRC_SDL_PLATFORM_INPUT_STATE_H_

struct InputState {
  int mouse_dx = 0;
  int mouse_dy = 0;

  bool rmb_down = false;

  bool forward = false;   // W
  bool backward = false;  // S
  bool left = false;      // A
  bool right = false;     // D
  bool up = false;        // Space
  bool down = false;      // Ctrl

  bool sprint = false;  // Shift

  bool quit = false;
  bool next_object = false;
  bool next_debug = false;
  bool toggle_shadows = false;
  bool toggle_aa = false;
  bool object_left = false;
  bool object_right = false;
  bool object_forward = false;
  bool object_backward = false;
  bool rotate_left = false;
  bool rotate_right = false;
  bool grow = false;
  bool shrink = false;
};

#endif  // RENDERER_SRC_SDL_PLATFORM_INPUT_STATE_H_
