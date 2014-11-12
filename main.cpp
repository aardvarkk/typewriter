#include "CImg-1.5.9/CImg.h"

#include <cstdint>
#include <iostream>
#include <vector>

// Input: 
// 1. Font image from character map that's been converted to grayscale (PGM is easily loadable by CImg)
int main(int argc, char const* agrv[])
{
  cimg_library::CImgDisplay disp, font_disp, out_disp;
  cimg_library::CImg<uint8_t> font_img("font_img.pgm");
  cimg_library::CImgList<uint8_t> font_chars;

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

  std::cout << "Output characters are " << outw << " x " << outh << std::endl;

  for (;;) {
    
    // Create the font table
    font_chars.clear();
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
    break;
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

  cimg_library::CImg<uint8_t> match_img("cameraman.pgm");
  int char_cols = 80; // Output columns per page
  int char_rows = char_cols * match_img.width() / match_img.height() * outw / outh; // Make it same aspect as input

  // Go through each output row and column and find best matching character
  int src_blkw = match_img.width() / char_cols;
  int src_blkh = match_img.height() / char_rows;

  std::vector<uint32_t> chosen_chars;
  for (int i = 0; i < char_rows; ++i) {
    for (int j = 0; j < char_cols; ++j) {

      // Get the source block
      cimg_library::CImg<uint8_t> src_blk(src_blkw, src_blkh);
      for (int di = 0; di < src_blkh; ++di) {
        for (int dj = 0; dj < src_blkw; ++dj) {
          src_blk.at(di * src_blkw + dj) = match_img.at((i * src_blkh + di) * match_img.width() + (j * src_blkw + dj));
        }
      }

      // Resize source block to same size as character images
      src_blk.resize(outw, outh);

      // Compare to each of our characters and find best match
      uint32_t min_idx = 0;
      uint32_t min_diff = UINT32_MAX;
      for (uint32_t idx = 0; idx < font_chars.size(); ++idx) {

        // Diff for this character
        uint32_t diff = 0;
        for (uint32_t k = 0; k < src_blk.size(); ++k) {
          diff += abs(src_blk[k] - font_chars[idx][k]);
        }

        if (diff < min_diff) {
          min_diff = diff;
          min_idx = idx;
        }
      }

      chosen_chars.push_back(min_idx);
    }
  }

  // Draw the output image
  cimg_library::CImg<uint8_t> out_img(char_cols * outw, char_rows * outh);
  int k = 0;
  for (int i = 0; i < char_rows; ++i) {
    for (int j = 0; j < char_cols; ++j, ++k) {
      // Get the chosen character
      auto ch = font_chars[chosen_chars[k]];

      // Draw it into the output
      for (int l = 0; l < outh; ++l) {
        for (int m = 0; m < outw; ++m) {
          out_img.at((i * outh + l) * out_img.width() + j * outw + m) = ch.at(l * outw + m);
        }
      }
    }
  }
  out_disp.display(out_img);
  while (out_disp.wait()) {
    if (out_disp.is_closed()) {
      break;
    }
  }

  return EXIT_SUCCESS;
}