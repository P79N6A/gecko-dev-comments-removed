



#include <math.h>

#include "base/gfx/png_encoder.h"
#include "base/gfx/png_decoder.h"
#include "testing/gtest/include/gtest/gtest.h"

static void MakeRGBImage(int w, int h, std::vector<unsigned char>* dat) {
  dat->resize(w * h * 3);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      unsigned char* org_px = &(*dat)[(y * w + x) * 3];
      org_px[0] = x * 3;      
      org_px[1] = x * 3 + 1;  
      org_px[2] = x * 3 + 2;  
    }
  }
}





static void MakeRGBAImage(int w, int h, bool use_transparency,
                          std::vector<unsigned char>* dat) {
  dat->resize(w * h * 4);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      unsigned char* org_px = &(*dat)[(y * w + x) * 4];
      org_px[0] = x * 3;      
      org_px[1] = x * 3 + 1;  
      org_px[2] = x * 3 + 2;  
      if (use_transparency)
        org_px[3] = x*3 + 3;  
      else
        org_px[3] = 0xFF;     
    }
  }
}

TEST(PNGCodec, EncodeDecodeRGB) {
  const int w = 20, h = 20;

  
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(PNGEncoder::Encode(&original[0], PNGEncoder::FORMAT_RGB, w, h,
                               w * 3, false, &encoded));

  
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(PNGDecoder::Decode(&encoded[0], encoded.size(),
                               PNGDecoder::FORMAT_RGB, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  
  ASSERT_TRUE(original == decoded);
}

TEST(PNGCodec, EncodeDecodeRGBA) {
  const int w = 20, h = 20;

  
  
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, true, &original);

  
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(PNGEncoder::Encode(&original[0], PNGEncoder::FORMAT_RGBA, w, h,
                               w * 4, false, &encoded));

  
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(PNGDecoder::Decode(&encoded[0], encoded.size(),
                               PNGDecoder::FORMAT_RGBA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  
  ASSERT_TRUE(original == decoded);
}


TEST(PNGCodec, DecodeCorrupted) {
  int w = 20, h = 20;

  
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  
  std::vector<unsigned char> output;
  int outw, outh;
  EXPECT_FALSE(PNGDecoder::Decode(&original[0], original.size(),
                                PNGDecoder::FORMAT_RGB, &output,
                                &outw, &outh));

  
  std::vector<unsigned char> compressed;
  EXPECT_TRUE(PNGEncoder::Encode(&original[0], PNGEncoder::FORMAT_RGB, w, h,
                               w * 3, false, &compressed));

  
  EXPECT_FALSE(PNGDecoder::Decode(&compressed[0], compressed.size() / 2,
                                PNGDecoder::FORMAT_RGB, &output,
                                &outw, &outh));

  
  for (int i = 10; i < 30; i++)
    compressed[i] = i;
  EXPECT_FALSE(PNGDecoder::Decode(&compressed[0], compressed.size(),
                                PNGDecoder::FORMAT_RGB, &output,
                                &outw, &outh));
}

TEST(PNGCodec, EncodeDecodeBGRA) {
  const int w = 20, h = 20;

  
  
  std::vector<unsigned char> original;
  MakeRGBAImage(w, h, true, &original);

  
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(PNGEncoder::Encode(&original[0], PNGEncoder::FORMAT_BGRA, w, h,
                               w * 4, false, &encoded));

  
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(PNGDecoder::Decode(&encoded[0], encoded.size(),
                               PNGDecoder::FORMAT_BGRA, &decoded,
                               &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  
  ASSERT_TRUE(original == decoded);
}

TEST(PNGCodec, StripAddAlpha) {
  const int w = 20, h = 20;

  
  std::vector<unsigned char> original_rgb;
  MakeRGBImage(w, h, &original_rgb);
  std::vector<unsigned char> original_rgba;
  MakeRGBAImage(w, h, false, &original_rgba);

  
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(PNGEncoder::Encode(&original_rgba[0],
                                 PNGEncoder::FORMAT_RGBA,
                                 w, h,
                                 w * 4, true, &encoded));

  
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(PNGDecoder::Decode(&encoded[0], encoded.size(),
                               PNGDecoder::FORMAT_RGBA, &decoded,
                               &outw, &outh));

  
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original_rgba.size(), decoded.size());
  ASSERT_TRUE(original_rgba == decoded);

  
  EXPECT_TRUE(PNGEncoder::Encode(&original_rgba[0],
                                 PNGEncoder::FORMAT_RGBA,
                                 w, h,
                                 w * 4, false, &encoded));

  
  EXPECT_TRUE(PNGDecoder::Decode(&encoded[0], encoded.size(),
                               PNGDecoder::FORMAT_RGB, &decoded,
                               &outw, &outh));

  
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original_rgb.size(), decoded.size());
  ASSERT_TRUE(original_rgb == decoded);
}
