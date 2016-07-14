#include "CImg.h"

#include <algorithm>
#include <cstdint>
#include <cwchar>
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
  int out_aspx = 101; // Take some number of characters in X and measure the amount of room they take to get the aspect ratio
  int out_aspy = 168; // Take some number of characters in Y and measure the amount of room they take to get the aspect ratio

  int var = 5; // How much variability we want in written characters. Means "a percentage of all characters", so 5 means we'll choose randomly from the top 5% of chars

  // Good settings for Visual Studio console output
  //int px = 5; // Size of padding in X
  //int py = -2; // Size of padding in Y

  // Good settings for Windows command prompt
  //int px = 5; // Size of padding in X
  //int py = 2; // Size of padding in Y

  int r = 10; // Number of rows in input font image
  int c = 20; // Number of columns
  int bx = 1; // Size of border in X before image starts
  int by = 1; // Size of border in Y before image starts
  int inw = 20; // Total input width on image (includes padding but not border)
  int inh = 24; // Total input height on image (includes padding but not border)

  int px = 4; // Change these to crop directly to the font in x and y
  int py = 2; // Change these to crop directly to the font in x and y

  int outw = inw - 2 * px; // Width of output chars
  int outh = inh - 2 * py; // Height of output chars

  int disp_ch = 0; // Character to display
  int& adj = disp_ch; // Our variable to adjust

  // Create list of chars for text output
  std::vector<wchar_t> allchars = { 
    L'!', L'"', L'#', L'$', L'%', L'&', L'\'', L'(', L')', L'*', L'+', L',', L'-', L'.', L'/', L'0', L'1', L'2', L'3', L'4',
    L'5', L'6', L'7', L'8', L'9', L':', L';',  L'<', L'=', L'>', L'?', L'@', L'A', L'B', L'C', L'D', L'E', L'F', L'G', L'H',
    L'I', L'J', L'K', L'L', L'M', L'N', L'O',  L'P', L'Q', L'R', L'S', L'T', L'U', L'V', L'W', L'X', L'Y', L'Z', L'[', L'\\',
    L']', L'^', L'_', L'`', L'a', L'b', L'c',  L'd', L'e', L'f', L'g', L'h', L'i', L'j', L'k', L'l', L'm', L'n', L'o', L'p',
    L'q', L'r', L's', L't', L'u', L'v', L'w',  L'x', L'y', L'z', L'{', L'|', L'}', L'~', L' ', L'¡', L'¢', L'£', L'¤', L'¥',
    L'¦', L'§', L'¨', L' ', L'ª', L'«', L'¬',  L' ', L' ', L'¯', L'°', L'±', L'²', L'³', L'´', L'µ', L'¶', L'·', L'¸', L'¹',
    L' ', L'»', L'¼', L'½', L'¾'
  };

  // We want to know where the space is so that we can make an exception for it
  // In general, we don't want the same characters repeated over and over, but spaces are OK
  int space_idx = 94; 

  // Create list of allowed output characters
  std::set<int> allow_idxs;
  for (int i = 0; i <= space_idx; ++i) {
    allow_idxs.insert(i);
  }

  // Remove entries for characters we don't support
  allow_idxs.erase(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'\\')));
  allow_idxs.erase(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'{')));
  allow_idxs.erase(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'|')));
  allow_idxs.erase(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'}')));

  // Add some special ones we do support
  allow_idxs.insert(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'¢')));
  allow_idxs.insert(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'§')));
  allow_idxs.insert(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'¶')));
  allow_idxs.insert(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'¼')));
  allow_idxs.insert(std::distance(allchars.begin(), std::find(allchars.begin(), allchars.end(), L'½')));

  std::wcout << "Output characters are " << outw << " x " << outh << std::endl;
  std::wcout << "Output aspect is " << static_cast<float>(outw) / outh << std::endl;

  /*for (;;)*/ {
    
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
    //disp.wait();

    //if (disp.is_keyESC() || disp.is_closed()) {
    //  return EXIT_SUCCESS;
    //}

    //if (disp.is_keyARROWUP()) {
    //  ++adj;
    //  std::wcout << adj << std::endl;
    //}
    //if (disp.is_keyARROWDOWN()) {
    //  --adj;
    //  std::wcout << adj << std::endl;
    //}
  }

  cimg_library::CImg<uint8_t> match_img("image.pgm");
  int char_cols = 60; // Output columns per page
  int char_rows = char_cols * match_img.width() / match_img.height() * outw / outh; // Make it same aspect as input

  std::vector<uint32_t> chosen_chars;
  for (int i = 0; i < char_rows; ++i) {
    for (int j = 0; j < char_cols; ++j) {

      // Go through each output row and column and find best matching character
      int src_blkx0 = match_img.width()  * j / char_cols;
      int src_blky0 = match_img.height() * i / char_rows;
      int src_blkx1 = match_img.width()  * (j + 1) / char_cols - 1;
      int src_blky1 = match_img.height() * (i + 1) / char_rows - 1;

      // Grab a crop from the source image
      auto src_blk = match_img.get_crop(src_blkx0, src_blky0, src_blkx1, src_blky1);

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

      std::wcout << allchars[chosen_chars[k]];
    }

    std::wcout << std::endl;
  }
  out_disp.display(out_img);
  //while (out_disp.wait()) {
  //  if (out_disp.is_closed()) {
  //    break;
  //  }
  //}

  // Generate instructions
  for (int i = 0; i < char_rows; ++i) {
    int j = 0;

    while (j < char_cols) {

      // Check for repeats
      int repeated = 0;
      while (j < char_cols - 1 && chosen_chars[i * char_cols + j] == chosen_chars[i * char_cols + j + 1]) {
        ++repeated;
        ++j;
      }

      // Only care to write special text if there's a bunch written together
      if (repeated >= 2) {
        std::wcout << std::endl << "Write " << repeated + 1 << " '" << allchars[chosen_chars[i * char_cols + j]] << "'" << std::endl;
      }
      else {
        // Can still get in here if we had 1 repeat, so we want to loop here to catch everything
        for (int k = repeated; k >= 0; --k) {
          std::wcout << allchars[chosen_chars[i * char_cols + j - k]];
        }
      }

      ++j;
    }

    std::wcout << std::endl << "Next line" << std::endl;
  }

  return EXIT_SUCCESS;
}