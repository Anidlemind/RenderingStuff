#include "text.h"

#include <cstdint>

#include "render/frame_buffer.h"

namespace {

struct Glyph {
  uint8_t rows[8];
};

constexpr Glyph kDigit0 = {{0b00111100, 0b01100110, 0b01101110, 0b01110110,
                            0b01100110, 0b01100110, 0b00111100, 0b00000000}};
constexpr Glyph kDigit1 = {{0b00011000, 0b00111000, 0b00011000, 0b00011000,
                            0b00011000, 0b00011000, 0b01111110, 0b00000000}};
constexpr Glyph kDigit2 = {{0b00111100, 0b01100110, 0b00000110, 0b00001100,
                            0b00110000, 0b01100000, 0b01111110, 0b00000000}};
constexpr Glyph kDigit3 = {{0b00111100, 0b01100110, 0b00000110, 0b00011100,
                            0b00000110, 0b01100110, 0b00111100, 0b00000000}};
constexpr Glyph kDigit4 = {{0b00001100, 0b00011100, 0b00101100, 0b01001100,
                            0b01111110, 0b00001100, 0b00001100, 0b00000000}};
constexpr Glyph kDigit5 = {{0b01111110, 0b01100000, 0b01111100, 0b00000110,
                            0b00000110, 0b01100110, 0b00111100, 0b00000000}};
constexpr Glyph kDigit6 = {{0b00111100, 0b01100110, 0b01100000, 0b01111100,
                            0b01100110, 0b01100110, 0b00111100, 0b00000000}};
constexpr Glyph kDigit7 = {{0b01111110, 0b00000110, 0b00001100, 0b00011000,
                            0b00110000, 0b00110000, 0b00110000, 0b00000000}};
constexpr Glyph kDigit8 = {{0b00111100, 0b01100110, 0b01100110, 0b00111100,
                            0b01100110, 0b01100110, 0b00111100, 0b00000000}};
constexpr Glyph kDigit9 = {{0b00111100, 0b01100110, 0b01100110, 0b00111110,
                            0b00000110, 0b01100110, 0b00111100, 0b00000000}};

constexpr Glyph kUpperF = {{0b01111110, 0b01100000, 0b01100000, 0b01111100,
                            0b01100000, 0b01100000, 0b01100000, 0b00000000}};
constexpr Glyph kUpperP = {{0b01111100, 0b01100110, 0b01100110, 0b01111100,
                            0b01100000, 0b01100000, 0b01100000, 0b00000000}};
constexpr Glyph kUpperS = {{0b00111100, 0b01100110, 0b01100000, 0b00111100,
                            0b00000110, 0b01100110, 0b00111100, 0b00000000}};

constexpr Glyph kLowerF = {{0b00011100, 0b00110000, 0b00110000, 0b01111100,
                            0b00110000, 0b00110000, 0b00110000, 0b00000000}};
constexpr Glyph kLowerP = {{0b00000000, 0b01111100, 0b01100110, 0b01100110,
                            0b01111100, 0b01100000, 0b01100000, 0b00000000}};
constexpr Glyph kLowerS = {{0b00000000, 0b00111100, 0b01100000, 0b00111100,
                            0b00000110, 0b01100110, 0b00111100, 0b00000000}};

constexpr Glyph kColon = {{0b00000000, 0b00011000, 0b00011000, 0b00000000,
                           0b00011000, 0b00011000, 0b00000000, 0b00000000}};
constexpr Glyph kSpace = {{0, 0, 0, 0, 0, 0, 0, 0}};
constexpr Glyph kDash  = {{0b00000000, 0b00000000, 0b00000000, 0b01111110,
                           0b00000000, 0b00000000, 0b00000000, 0b00000000}};

const Glyph& glyphFor(char c) {
  switch (c) {
    case '0': return kDigit0;
    case '1': return kDigit1;
    case '2': return kDigit2;
    case '3': return kDigit3;
    case '4': return kDigit4;
    case '5': return kDigit5;
    case '6': return kDigit6;
    case '7': return kDigit7;
    case '8': return kDigit8;
    case '9': return kDigit9;

    case 'F': return kUpperF;
    case 'P': return kUpperP;
    case 'S': return kUpperS;

    case 'f': return kLowerF;
    case 'p': return kLowerP;
    case 's': return kLowerS;

    case ':': return kColon;
    case ' ': return kSpace;
    case '-': return kDash;

    default:  return kSpace;
  }
}

}  // namespace

int textWidth(const std::string& text) {
  return static_cast<int>(text.size()) * kTextCharWidth;
}

void drawText(FrameBuffer& fb, int x, int y, const std::string& text, Color color) {
  int cx = x;

  for (char ch : text) {
    const Glyph& g = glyphFor(ch);

    for (int row = 0; row < 8; ++row) {
      const uint8_t bits = g.rows[row];
      for (int col = 0; col < 8; ++col) {
        // Bit 7 (0x80) is the leftmost pixel.
        if (bits & (0x80u >> col)) {
          fb.setPixel(cx + col, y + row, color);
        }
      }
    }

    cx += kTextCharWidth;
  }
}