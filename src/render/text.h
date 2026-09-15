#pragma once

#include <string>

#include "render/color.h"

class FrameBuffer;

void drawText(FrameBuffer& fb, int x, int y, const std::string& text, Color color);

int textWidth(const std::string& text);

constexpr int kTextHeight = 8;
constexpr int kTextCharWidth = 8;