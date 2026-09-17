#ifndef RENDERER_SRC_UI_TEXT_H_
#define RENDERER_SRC_UI_TEXT_H_

#include <string>

#include "render/color.h"

class FrameBuffer;

void DrawText(FrameBuffer& fb, int x, int y, const std::string& text,
              Color color);

int TextWidth(const std::string& text);

constexpr int kTextHeight = 8;
constexpr int kTextCharWidth = 8;

#endif  // RENDERER_SRC_UI_TEXT_H_
