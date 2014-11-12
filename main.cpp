#include "CImg-1.5.9/CImg.h"

#include <algorithm>
#include <cstdint>
#include <iostream>
#include <random>
#include <set>
#include <vector>

// Random number engine
std::mt19937 eng;

struct char_match_t
{
  uint32_t diff;
  int char_idx;

  bool operator<(char_match_t const& c) const {
    return this->diff < c.diff;
  }
};

// Input: 
// 1. Font image from character map that's been converted to grayscale (PGM is easily loadable by CImg)
int main(int argc, char const* agrv[])
{
  cimg_library::CImgDisplay disp, font_disp, out_disp;
  cimg_library::CImg<uint8_t> font_img("font_img.pgm");
  cimg_library::CImgList<uint8_t> font_chars;

  int ox = 2; // Offset to first border pixel in X
  int oy = 2; // Offset to first border pixel in Y
  
  int var = 10; // How much variability we want in written characters. Means "a percentage of all characters", so 5 means we'll choose randomly from the top 5% of chars

  // Good settings for Windows console output
  int px = 5; // Size of padding in X
  int py = -2; // Size of padding in Y
  
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

  // Create list of chars for text output
  std::vector<char> allchars = { 
    '!', '"', '#', '$', '%', '&', '\'', '(', ')', '*', '+', ',', '-', '.', '/', '0', '1', '2', '3', '4',
    '5', '6', '7', '8', '9', ':', ';', '<', '=', '>', '?', '@', 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H',
    'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z', '[', '\\',
    ']', '^', '_', '`', 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p',
    'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z', '{', '|', '}', '~', ' '
  };

  // We want to know where the space is so that we can make an exception for it
  // In general, we don't want the same characters repeated over and over, but spaces are OK
  int space_idx = 94; 

  // Create list of allowed output characters
  std::set<int> allow_idxs;
  for (int i = 0; i < 95; ++i) {
    allow_idxs.insert(i);
  }

  std::cout << "Output characters are " << outw << " x " << outh << std::endl;

  for (;;) {
    
    // Create the font table
    font_chars.clear();
    for (int i = 0; i < r; ++i) {
      for (int j = 0; j < c; ++j) {

        auto sx = ox + (j + 1) * bx + j * inw + px;
        auto sy = oy + (i + 1) * by + i * inh + py;

        // Original approach doesn't work when we have negative padding
        // We can't use get_crop() because the dimensions will be wrong given negative padding
        // We actually can't retrieve outw x outh images from the original images
        // We can only copy inw x inh into a larger output
        // This approach assumed outw <= inw and outh <= inh
        if (outw <= inw && outh <= inh) {
          auto ex = sx + outw - 1;
          auto ey = sy + outh - 1;
          font_chars.push_back(font_img.get_crop(sx, sy, ex, ey));
        }
        // More complicated copy/shifting
        else {
          cimg_library::CImg<uint8_t> char_img(outw, outh);
          char_img.fill(0xff);

          // For each output row...
          int skipped_y = 0;
          for (int k = 0; k < outh; ++k) {

            if (skipped_y++ < -py || k >= outh + py) {
              continue;
            }

            // For each output column...
            int skipped_x = 0;
            for (int l = 0; l < outw; ++l) {

              if (skipped_x++ < -px || l >= outw + px) {
                continue;
              }

              // Copy pixels
              char_img.at(k * outw + l) = font_img.at((sy + k) * font_img.width() + (sx + l));
            }
          }

          font_chars.push_back(char_img);
        }
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

      // Collect all diffs so we can choose randoms from a top percentage
      // This way, the typing is more interesting instead of repeated letters over and over
      std::set<char_match_t> matches;

      // Only allow characters we're OK with
      for (auto ai : allow_idxs) {
      //for (uint32_t idx = 0; idx < font_chars.size(); ++idx) {

        // Diff for this character
        uint32_t diff = 0;
        for (uint32_t k = 0; k < src_blk.size(); ++k) {
          diff += abs(src_blk[k] - font_chars[ai][k]);
        }

        char_match_t match;
        match.char_idx = ai;
        match.diff = diff;
        matches.insert(match);
      }

      // If our best match is a space, we'll keep it
      if (matches.begin()->char_idx == space_idx) {
        chosen_chars.push_back(space_idx);
      }
      // If it's not a space, choose a random from the top n% of matches
      else {
        auto topn = allchars.size() * var / 100;
        std::uniform_int_distribution<> distn(0, topn);
        auto it = matches.begin();
        std::advance(it, distn(eng));
        chosen_chars.push_back(it->char_idx);
      }
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

      std::cout << allchars[chosen_chars[k]];
    }

    std::cout << std::endl;
  }
  out_disp.display(out_img);
  while (out_disp.wait()) {
    if (out_disp.is_closed()) {
      break;
    }
  }

  return EXIT_SUCCESS;
}