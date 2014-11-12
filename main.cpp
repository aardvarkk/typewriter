#include "CImg-1.5.9/CImg.h"

#include <cstdint>
#include <iostream>

// Input: 
// 1. Font image from character map that's been converted to grayscale (PGM is easily loadable by CImg)
int main(int argc, char const* agrv[])
{
  cimg_library::CImgDisplay disp, font_disp;
  cimg_library::CImg<uint8_t> font_img("font_img.pgm");

  int ox = 12; // Offset to first border pixel in X
  int oy = 10; // Offset to first border pixel in Y
  int px = 5; // Size of padding in X
  int py = 3; // Size of padding in Y

  int r = 10; // Number of rows
  int c = 20; // Number of columns
  int bx = 1; // Size of border in X
  int by = 1; // Size of border in Y
  int inw = 20; // Total input width on image (includes padding but not border)
  int inh = 24; // Total input height on image (includes padding but not border)
  int outw = inw - 2 * px; // Width of output chars
  int outh = inh - 2 * py; // Height of output chars

  int disp_ch = 0; // Character to display
  int& adj = disp_ch; // Our variable to adjust

  for (;;) {
    // Create the font table
    cimg_library::CImgList<uint8_t> font_chars;
    for (int i = 0; i < r; ++i) {
      for (int j = 0; j < c; ++j) {
        auto sx = ox + (j + 1) * bx + j * inw + px;
        auto sy = oy + (i + 1) * by + i * inh + py;
        auto ex = sx + outw - 1;
        auto ey = sy + outh - 1;
        auto ch = font_img.get_crop(sx, sy, ex, ey);
        font_chars.push_back(ch);
      }
    }
  
    // Draw the font table (in the newly-sized dimensions)
    cimg_library::CImg<uint8_t> font_table(c * outw, r * outh);
    for (int i = 0; i < r; ++i) {
      for (int j = 0; j < c; ++j) {
        for (int iy = 0; iy < outh; ++iy) {
          for (int ix = 0; ix < outw; ++ix) {
            int dstx = j * outw + ix;
            int dsty = i * outh + iy;
            int srcx = ix;
            int srcy = iy;
            font_table.at(dsty * font_table.width() + dstx) = 
              font_chars[i * c + j].at(srcy * font_chars[i * c + j].width() + srcx);
          }
        }
      }
    }

    // Show it
    font_disp.display(font_chars[disp_ch]);
    disp.display(font_table);
    disp.wait();

    if (disp.is_keyESC() || disp.is_closed()) {
      return EXIT_SUCCESS;
    }

    if (disp.is_keyARROWUP()) {
      ++adj;
      std::cout << adj << std::endl;
    }
    if (disp.is_keyARROWDOWN()) {
      --adj;
      std::cout << adj << std::endl;
    }
  }

  return EXIT_SUCCESS;
}