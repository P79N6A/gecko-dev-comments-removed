









#include <math.h>
#include <string.h>

#include "common_video/libyuv/include/webrtc_libyuv.h"
#include "gtest/gtest.h"
#include "system_wrappers/interface/tick_util.h"
#include "testsupport/fileutils.h"

namespace webrtc {

int PrintFrame(const uint8_t* frame, int width, int height) {
  if (frame == NULL)
    return -1;
  int k = 0;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      printf("%d ", frame[k++]);
    }
    printf(" \n");
  }
  printf(" \n");
  return 0;
}

int PrintFrame(const uint8_t* frame, int width,
                int height, const char* str) {
  if (frame == NULL)
     return -1;
  printf("%s %dx%d \n", str, width, height);

  const uint8_t* frame_y = frame;
  const uint8_t* frame_u = frame_y + width * height;
  const uint8_t* frame_v = frame_u + width * height / 4;

  int ret = 0;
  ret += PrintFrame(frame_y, width, height);
  ret += PrintFrame(frame_u, width / 2, height / 2);
  ret += PrintFrame(frame_v, width / 2, height / 2);

  return ret;
}

void CreateImage(int width, int height,
                 uint8_t* frame, int offset,
                 int height_factor, int width_factor) {
  if (frame == NULL)
    return;
  for (int i = 0; i < height; i++) {
    for (int j = 0; j < width; j++) {
      *frame = static_cast<uint8_t>((i + offset) * height_factor
                                     + j * width_factor);
      frame++;
    }
  }
}

class TestLibYuv : public ::testing::Test {
 protected:
  TestLibYuv();
  virtual void SetUp();
  virtual void TearDown();

  FILE* source_file_;
  const int width_;
  const int height_;
  const int frame_length_;
};


TestLibYuv::TestLibYuv()
    : source_file_(NULL),
      width_(352),
      height_(288),
      frame_length_(CalcBufferSize(kI420, 352, 288)) {
}

void TestLibYuv::SetUp() {
  const std::string input_file_name = webrtc::test::ProjectRootPath() +
                                      "resources/foreman_cif.yuv";
  source_file_  = fopen(input_file_name.c_str(), "rb");
  ASSERT_TRUE(source_file_ != NULL) << "Cannot read file: "<<
                                       input_file_name << "\n";
}

void TestLibYuv::TearDown() {
  if (source_file_ != NULL) {
    ASSERT_EQ(0, fclose(source_file_));
  }
  source_file_ = NULL;
}

TEST_F(TestLibYuv, ConvertSanityTest) {
  
}

TEST_F(TestLibYuv, ConvertTest) {
  
  int j = 0;
  std::string output_file_name = webrtc::test::OutputPath() +
                                 "LibYuvTest_conversion.yuv";
  FILE*  output_file = fopen(output_file_name.c_str(), "wb");
  ASSERT_TRUE(output_file != NULL);

  double psnr = 0;

  uint8_t* orig_buffer = new uint8_t[frame_length_];
  EXPECT_GT(fread(orig_buffer, 1, frame_length_, source_file_), 0U);

  
  uint8_t* res_rgb_buffer2  = new uint8_t[width_ * height_ * 3];
  uint8_t* res_i420_buffer = new uint8_t[frame_length_];

  EXPECT_EQ(0, ConvertFromI420(orig_buffer, width_, kRGB24, 0,
                               width_, height_, res_rgb_buffer2));

  EXPECT_EQ(0, ConvertToI420(kRGB24, res_rgb_buffer2, 0, 0, width_, height_,
                             0, width_, height_, width_, kRotateNone,
                             res_i420_buffer));

  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) != static_cast<unsigned int>(frame_length_)) {
    return;
  }
  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  
  EXPECT_GT(ceil(psnr), 44);
  j++;
  delete [] res_rgb_buffer2;

  
  uint8_t* out_uyvy_buffer = new uint8_t[width_ * height_ * 2];
  EXPECT_EQ(0, ConvertFromI420(orig_buffer, width_,
                               kUYVY, 0, width_, height_, out_uyvy_buffer));
  EXPECT_EQ(0, ConvertToI420(kUYVY, out_uyvy_buffer, 0, 0, width_, height_,
            0, width_, height_, width_,kRotateNone, res_i420_buffer));
  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  EXPECT_EQ(48.0, psnr);
  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) !=  static_cast<unsigned int>(frame_length_)) {
    return;
  }

  j++;
  delete [] out_uyvy_buffer;

  
  uint8_t* out_i420_buffer = new uint8_t[width_ * height_ * 3 / 2 ];
  EXPECT_EQ(0, ConvertToI420(kI420, orig_buffer, 0, 0, width_, height_,
                             0, width_, height_, width_,
                             kRotateNone, out_i420_buffer));
  EXPECT_EQ(0, ConvertFromI420(out_i420_buffer, width_, kI420, 0,
                               width_, height_, res_i420_buffer));
  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) != static_cast<unsigned int>(frame_length_)) {
    return;
  }
  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  EXPECT_EQ(48.0, psnr);
  j++;
  delete [] out_i420_buffer;

  
  uint8_t* outYV120Buffer = new uint8_t[frame_length_];

  EXPECT_EQ(0, ConvertFromI420(orig_buffer, width_, kYV12, 0,
                               width_, height_, outYV120Buffer));
  EXPECT_EQ(0, ConvertFromYV12(outYV120Buffer, width_,
                               kI420, 0,
                               width_, height_,
                               res_i420_buffer));
  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) !=  static_cast<unsigned int>(frame_length_)) {
    return;
  }

  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  EXPECT_EQ(48.0, psnr);
  j++;
  delete [] outYV120Buffer;

  
  uint8_t* out_yuy2_buffer = new uint8_t[width_ * height_ * 2];
  EXPECT_EQ(0, ConvertFromI420(orig_buffer, width_,
                               kYUY2, 0, width_, height_, out_yuy2_buffer));

  EXPECT_EQ(0, ConvertToI420(kYUY2, out_yuy2_buffer, 0, 0, width_, height_,
                             0, width_, height_, width_,
                             kRotateNone, res_i420_buffer));

  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) !=  static_cast<unsigned int>(frame_length_)) {
    return;
  }
  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  EXPECT_EQ(48.0, psnr);

  
  uint8_t* out_rgb565_buffer = new uint8_t[width_ * height_ * 2];
  EXPECT_EQ(0, ConvertFromI420(orig_buffer, width_,
                               kRGB565, 0, width_, height_, out_rgb565_buffer));

  EXPECT_EQ(0, ConvertToI420(kRGB565, out_rgb565_buffer, 0, 0, width_, height_,
                             0, width_, height_, width_,
                             kRotateNone, res_i420_buffer));

  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) !=  static_cast<unsigned int>(frame_length_)) {
    return;
  }
  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  
  
  EXPECT_GT(ceil(psnr), 40);

  
  uint8_t* out_argb8888_buffer = new uint8_t[width_ * height_ * 4];
  EXPECT_EQ(0, ConvertFromI420(orig_buffer, width_,
                               kARGB, 0, width_, height_, out_argb8888_buffer));

  EXPECT_EQ(0, ConvertToI420(kARGB, out_argb8888_buffer, 0, 0, width_, height_,
                             0, width_, height_, width_,
                             kRotateNone, res_i420_buffer));

  if (fwrite(res_i420_buffer, 1, frame_length_,
             output_file) !=  static_cast<unsigned int>(frame_length_)) {
    return;
  }
  psnr = I420PSNR(orig_buffer, res_i420_buffer, width_, height_);
  
  EXPECT_GT(ceil(psnr), 42);

  ASSERT_EQ(0, fclose(output_file));

  delete [] out_argb8888_buffer;
  delete [] out_rgb565_buffer;
  delete [] out_yuy2_buffer;
  delete [] res_i420_buffer;
  delete [] orig_buffer;
}




TEST_F(TestLibYuv, DISABLED_MirrorTest) {
  
  std::string str;
  int width = 16;
  int height = 8;
  int factor_y = 1;
  int factor_u = 1;
  int factor_v = 1;
  int start_buffer_offset = 10;
  int length = webrtc::CalcBufferSize(kI420, width, height);

  uint8_t* test_frame = new uint8_t[length];
  memset(test_frame, 255, length);

  
  uint8_t* in_frame = test_frame;
  uint8_t* in_frame_cb = in_frame + width * height;
  uint8_t* in_frame_cr = in_frame_cb + (width * height) / 4;
  CreateImage(width, height, in_frame, 10, factor_y, 1);  
  CreateImage(width / 2, height / 2, in_frame_cb, 100, factor_u, 1);  
  CreateImage(width / 2, height / 2, in_frame_cr, 200, factor_v, 1);  
  EXPECT_EQ(0, PrintFrame(test_frame, width, height, "InputFrame"));

  uint8_t* test_frame2 = new uint8_t[length + start_buffer_offset * 2];
  memset(test_frame2, 255, length + start_buffer_offset * 2);
  uint8_t* out_frame = test_frame2;

  
  std::cout << "Test Mirror function: LeftRight" << std::endl;
  EXPECT_EQ(0, MirrorI420LeftRight(in_frame, out_frame, width, height));
  EXPECT_EQ(0, PrintFrame(test_frame2, width, height, "OutputFrame"));
  EXPECT_EQ(0, MirrorI420LeftRight(out_frame, test_frame, width, height));

  EXPECT_EQ(0, memcmp(in_frame, test_frame, length));

  
  std::cout << "Test Mirror function: UpDown" << std::endl;
  EXPECT_EQ(0, MirrorI420UpDown(in_frame, out_frame, width, height));
  EXPECT_EQ(0, PrintFrame(test_frame2, width, height, "OutputFrame"));
  EXPECT_EQ(0, MirrorI420UpDown(out_frame, test_frame, width, height));

  EXPECT_EQ(0, memcmp(in_frame, test_frame, length));

  

  std::cout << "Do the mirrored frames look correct?" << std::endl;
  delete [] test_frame;
  delete [] test_frame2;
}

}  
