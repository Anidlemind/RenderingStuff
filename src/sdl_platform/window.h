#pragma once

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

  void pollEvents();
  void present(const uint32_t* pixels, int width, int height);

  bool isOpen() const { return open_; }
  const InputState& input() const { return input_; }

  int width()  const { return width_; }
  int height() const { return height_; }

private:
  SDL_Window*   window_   = nullptr;
  SDL_Renderer* renderer_ = nullptr;
  SDL_Texture*  texture_  = nullptr;

  int width_  = 0;
  int height_ = 0;
  bool open_  = true;

  bool mouseInitialized_ = false;

  InputState input_;
};