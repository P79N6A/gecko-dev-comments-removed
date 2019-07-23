



#include <math.h>

#include "base/gfx/jpeg_codec.h"
#include "testing/gtest/include/gtest/gtest.h"




static int jpeg_quality = 100;




static double jpeg_equality_threshold = 1.0;




static double AveragePixelDelta(const std::vector<unsigned char>& a,
                                const std::vector<unsigned char>& b) {
  
  if (a.size() != b.size())
    return 255.0;
  if (a.size() == 0)
    return 0;  

  double acc = 0.0;
  for (size_t i = 0; i < a.size(); i++)
    acc += fabs(static_cast<double>(a[i]) - static_cast<double>(b[i]));

  return acc / static_cast<double>(a.size());
}

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

TEST(JPEGCodec, EncodeDecodeRGB) {
  int w = 20, h = 20;

  
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(JPEGCodec::Encode(&original[0], JPEGCodec::FORMAT_RGB, w, h,
                                w * 3, jpeg_quality, &encoded));
  EXPECT_GT(original.size(), encoded.size());

  
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(JPEGCodec::Decode(&encoded[0], encoded.size(),
                                JPEGCodec::FORMAT_RGB, &decoded,
                                &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  
  
  ASSERT_GE(jpeg_equality_threshold, AveragePixelDelta(original, decoded));
}

TEST(JPEGCodec, EncodeDecodeRGBA) {
  int w = 20, h = 20;

  
  
  std::vector<unsigned char> original;
  original.resize(w * h * 4);
  for (int y = 0; y < h; y++) {
    for (int x = 0; x < w; x++) {
      unsigned char* org_px = &original[(y * w + x) * 4];
      org_px[0] = x * 3;      
      org_px[1] = x * 3 + 1;  
      org_px[2] = x * 3 + 2;  
      org_px[3] = 0xFF;       
    }
  }

  
  std::vector<unsigned char> encoded;
  EXPECT_TRUE(JPEGCodec::Encode(&original[0], JPEGCodec::FORMAT_RGBA, w, h,
                                w * 4, jpeg_quality, &encoded));
  EXPECT_GT(original.size(), encoded.size());

  
  std::vector<unsigned char> decoded;
  int outw, outh;
  EXPECT_TRUE(JPEGCodec::Decode(&encoded[0], encoded.size(),
                                JPEGCodec::FORMAT_RGBA, &decoded,
                                &outw, &outh));
  ASSERT_EQ(w, outw);
  ASSERT_EQ(h, outh);
  ASSERT_EQ(original.size(), decoded.size());

  
  
  ASSERT_GE(jpeg_equality_threshold, AveragePixelDelta(original, decoded));
}


TEST(JPEGCodec, DecodeCorrupted) {
  int w = 20, h = 20;

  
  std::vector<unsigned char> original;
  MakeRGBImage(w, h, &original);

  
  std::vector<unsigned char> output;
  int outw, outh;
  ASSERT_FALSE(JPEGCodec::Decode(&original[0], original.size(),
                                 JPEGCodec::FORMAT_RGB, &output,
                                 &outw, &outh));

  
  std::vector<unsigned char> compressed;
  ASSERT_TRUE(JPEGCodec::Encode(&original[0], JPEGCodec::FORMAT_RGB, w, h,
                                w * 3, jpeg_quality, &compressed));

  
  ASSERT_FALSE(JPEGCodec::Decode(&compressed[0], compressed.size() / 2,
                                 JPEGCodec::FORMAT_RGB, &output,
                                 &outw, &outh));

  
  for (int i = 10; i < 30; i++)
    compressed[i] = i;
  ASSERT_FALSE(JPEGCodec::Decode(&compressed[0], compressed.size(),
                                 JPEGCodec::FORMAT_RGB, &output,
                                 &outw, &outh));
}
