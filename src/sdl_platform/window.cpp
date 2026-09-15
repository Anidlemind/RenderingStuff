#include "sdl_platform/window.h"

#include <cstdio>
#include <stdexcept>
#include <string>

#include <SDL.h>

Window::Window(const std::string& title, int width, int height)
  : width_(width), height_(height) {
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    throw std::runtime_error(std::string("SDL_Init failed: ") + SDL_GetError());
  }

  window_ = SDL_CreateWindow(
    title.c_str(),
    SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
    width, height,
    SDL_WINDOW_SHOWN);

  if (!window_) {
    throw std::runtime_error(std::string("SDL_CreateWindow failed: ") + SDL_GetError());
  }

  renderer_ = SDL_CreateRenderer(
    window_, -1,
    SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);

  if (!renderer_) {
    throw std::runtime_error(std::string("SDL_CreateRenderer failed: ") + SDL_GetError());
  }

  texture_ = SDL_CreateTexture(
    renderer_,
    SDL_PIXELFORMAT_ARGB8888,
    SDL_TEXTUREACCESS_STREAMING,
    width, height);

  if (!texture_) {
    throw std::runtime_error(std::string("SDL_CreateTexture failed: ") + SDL_GetError());
  }

  SDL_SetTextureBlendMode(texture_, SDL_BLENDMODE_NONE);
}

Window::~Window() {
  if (texture_)  SDL_DestroyTexture(texture_);
  if (renderer_) SDL_DestroyRenderer(renderer_);
  if (window_)   SDL_DestroyWindow(window_);
  SDL_Quit();
}

void Window::pollEvents() {
  input_.mouseDX = 0;
  input_.mouseDY = 0;

  SDL_Event e;
  while (SDL_PollEvent(&e)) {
    switch (e.type) {
      case SDL_QUIT:
        input_.quit = true;
        break;

      case SDL_MOUSEMOTION:
        if (!mouseInitialized_) {
          mouseInitialized_ = true;
          break;
        }
        if (input_.rmbDown) {
          input_.mouseDX += e.motion.xrel;
          input_.mouseDY += e.motion.yrel;
        }
        break;

      case SDL_MOUSEBUTTONDOWN:
        if (e.button.button == SDL_BUTTON_RIGHT) {
          input_.rmbDown = true;
        }
        break;

      case SDL_MOUSEBUTTONUP:
        if (e.button.button == SDL_BUTTON_RIGHT) {
          input_.rmbDown = false;
        }
        break;

      case SDL_KEYDOWN:
      case SDL_KEYUP: {
        const bool down = (e.type == SDL_KEYDOWN);
        switch (e.key.keysym.scancode) {
          case SDL_SCANCODE_W:      input_.forward  = down; break;
          case SDL_SCANCODE_S:      input_.backward = down; break;
          case SDL_SCANCODE_A:      input_.left     = down; break;
          case SDL_SCANCODE_D:      input_.right    = down; break;
          case SDL_SCANCODE_SPACE:  input_.up       = down; break;
          case SDL_SCANCODE_LCTRL:  input_.down     = down; break;
          case SDL_SCANCODE_LSHIFT: input_.sprint   = down; break;
          case SDL_SCANCODE_ESCAPE: input_.quit     = true; break;
          default: break;
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

void Window::present(const uint32_t* pixels, int width, int height) {
  if (width != width_ || height != height_) {
    return;
  }

  SDL_UpdateTexture(texture_, nullptr, pixels, width * static_cast<int>(sizeof(uint32_t)));

  SDL_RenderClear(renderer_);
  SDL_RenderCopy(renderer_, texture_, nullptr, nullptr);
  SDL_RenderPresent(renderer_);
}
