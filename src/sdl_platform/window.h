#ifndef RENDERER_SRC_SDL_PLATFORM_WINDOW_H_
#define RENDERER_SRC_SDL_PLATFORM_WINDOW_H_

#include <cstdint>
#include <string>

#include "sdl_platform/input_state.h"

struct SDL_Window;
struct SDL_Renderer;
struct SDL_Texture;

class Window {
 public:
  Window(const std::string& title, int width, int height);
  ~Window();

  Window(const Window&) = delete;
  Window& operator=(const Window&) = delete;

  void PollEvents();
  void Present(const uint32_t* pixels, int width, int height);

  bool IsOpen() const { return open_; }
  const InputState& Input() const { return input_; }

  int Width() const { return width_; }
  int Height() const { return height_; }

 private:
  SDL_Window* window_ = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture* texture_ = nullptr;

  int width_ = 0;
  int height_ = 0;
  bool open_ = true;

  bool mouse_initialized_ = false;

  InputState input_;
};

#endif  // RENDERER_SRC_SDL_PLATFORM_WINDOW_H_
