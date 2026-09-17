#include "sdl_platform/window.h"

#include <SDL.h>

#include <cstdio>
#include <stdexcept>
#include <string>

Window::Window(const std::string& title, int width, int height)
    : width_(width), height_(height) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
  }

  try {
    window_ = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED,
                               SDL_WINDOWPOS_CENTERED, width, height,
                               SDL_WINDOW_SHOWN);

    if (!window_) {
      throw std::runtime_error(std::string("SDL_CreateWindow failed: ") +
                               SDL_GetError());
    }

    renderer_ = SDL_CreateRenderer(
        window_, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

    if (!renderer_) {
      throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") +
                               SDL_GetError());
    }

    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_ARGB8888,
                                 SDL_TEXTUREACCESS_STREAMING, width, height);

    if (!texture_) {
      throw std::runtime_error(std::string("SDL_CreateTexture failed: ") +
                               SDL_GetError());
    }

    SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
  } catch (...) {
    if (texture_) {
      SDL_DestroyTexture(texture_);
    }
    if (renderer_) {
      SDL_DestroyRenderer(renderer_);
    }
    if (window_) {
      SDL_DestroyWindow(window_);
    }
    SDL_Quit();
    throw;
  }
}

Window::~Window() {
  if (texture_) {
    SDL_DestroyTexture(texture_);
  }
  if (renderer_) {
    SDL_DestroyRenderer(renderer_);
  }
  if (window_) {
    SDL_DestroyWindow(window_);
  }
  SDL_Quit();
}

void Window::PollEvents() {
  input_.mouse_dx = 0;
  input_.mouse_dy = 0;
  input_.next_object = input_.next_debug = input_.toggle_shadows =
      input_.toggle_aa = false;

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_WINDOWEVENT:
        if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) {
          const bool quit = input_.quit;
          input_ = {};
          input_.quit = quit;
          mouse_initialized_ = false;
        }
        break;
      case SDL_QUIT:
        input_.quit = true;
        break;

      case SDL_MOUSEMOTION:
        if (!mouse_initialized_) {
          mouse_initialized_ = true;
          break;
        }
        if (input_.rmb_down) {
          input_.mouse_dx += e.motion.xrel;
          input_.mouse_dy += e.motion.yrel;
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_RIGHT) {
          input_.rmb_down = true;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_RIGHT) {
          input_.rmb_down = false;
        }
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        const bool down = (e.type == SDL_KEYDOWN);
        const bool pressed = down && !e.key.repeat;
        switch (e.key.keysym.scancode) {
          case SDL_SCANCODE_TAB:
            input_.next_object |= pressed;
            break;
          case SDL_SCANCODE_F1:
            input_.next_debug |= pressed;
            break;
          case SDL_SCANCODE_F2:
            input_.toggle_shadows |= pressed;
            break;
          case SDL_SCANCODE_F3:
            input_.toggle_aa |= pressed;
            break;
          case SDL_SCANCODE_LEFT:
            input_.object_left = down;
            break;
          case SDL_SCANCODE_RIGHT:
            input_.object_right = down;
            break;
          case SDL_SCANCODE_UP:
            input_.object_forward = down;
            break;
          case SDL_SCANCODE_DOWN:
            input_.object_backward = down;
            break;
          case SDL_SCANCODE_Q:
            input_.rotate_left = down;
            break;
          case SDL_SCANCODE_E:
            input_.rotate_right = down;
            break;
          case SDL_SCANCODE_PAGEUP:
            input_.grow = down;
            break;
          case SDL_SCANCODE_PAGEDOWN:
            input_.shrink = down;
            break;
          case SDL_SCANCODE_W:
            input_.forward = down;
            break;
          case SDL_SCANCODE_S:
            input_.backward = down;
            break;
          case SDL_SCANCODE_A:
            input_.left = down;
            break;
          case SDL_SCANCODE_D:
            input_.right = down;
            break;
          case SDL_SCANCODE_SPACE:
            input_.up = down;
            break;
          case SDL_SCANCODE_LCTRL:
            input_.down = down;
            break;
          case SDL_SCANCODE_LSHIFT:
            input_.sprint = down;
            break;
          case SDL_SCANCODE_ESCAPE:
            input_.quit = true;
            break;
          default:
            break;
        }
        break;
      }

      default:
        break;
    }
  }

  if (input_.quit) {
    open_ = false;
  }
}

void Window::Present(const uint32_t* pixels, int width, int height) {
  if (width != width_ || height != height_) {
    throw std::invalid_argument("Presentation dimensions do not match window");
  }
  if (!pixels) {
    throw std::invalid_argument("Null presentation pixels");
  }
  if (SDL_UpdateTexture(texture_, nullptr, pixels,
                        width * static_cast<int>(sizeof(uint32_t))) != 0 ||
      SDL_RenderClear(renderer_) != 0 ||
      SDL_RenderCopy(renderer_, texture_, nullptr, nullptr) != 0) {
    throw std::runtime_error(std::string("SDL presentation failed: ") +
                             SDL_GetError());
  }
  SDL_RenderPresent(renderer_);
}
