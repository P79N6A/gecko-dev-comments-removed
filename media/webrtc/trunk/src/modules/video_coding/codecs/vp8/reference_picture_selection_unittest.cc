









#include "gtest/gtest.h"
#include "reference_picture_selection.h"
#include "vpx/vpx_encoder.h"
#include "vpx/vp8cx.h"

using webrtc::ReferencePictureSelection;



enum { kMinUpdateInterval = 10 };


enum { kRtt = 10 };

enum {
  kNoPropagationGolden    = VP8_EFLAG_NO_REF_ARF |
                            VP8_EFLAG_NO_UPD_GF |
                            VP8_EFLAG_NO_UPD_ARF,
  kNoPropagationAltRef    = VP8_EFLAG_NO_REF_GF |
                            VP8_EFLAG_NO_UPD_GF |
                            VP8_EFLAG_NO_UPD_ARF,
  kPropagateGolden        = VP8_EFLAG_FORCE_GF |
                            VP8_EFLAG_NO_UPD_ARF |
                            VP8_EFLAG_NO_REF_GF |
                            VP8_EFLAG_NO_REF_LAST,
  kPropagateAltRef        = VP8_EFLAG_FORCE_ARF |
                            VP8_EFLAG_NO_UPD_GF |
                            VP8_EFLAG_NO_REF_ARF |
                            VP8_EFLAG_NO_REF_LAST,
  kRefreshFromGolden      = VP8_EFLAG_NO_REF_LAST |
                            VP8_EFLAG_NO_REF_ARF,
  kRefreshFromAltRef      = VP8_EFLAG_NO_REF_LAST |
                            VP8_EFLAG_NO_REF_GF
};

class TestRPS : public ::testing::Test {
 protected:
  virtual void SetUp() {
    rps_.Init();
    
    rps_.EncodedKeyFrame(0);
    rps_.ReceivedRPSI(0);
    rps_.SetRtt(kRtt);
  }

  ReferencePictureSelection rps_;
};

TEST_F(TestRPS, TestPropagateReferenceFrames) {
  
  uint32_t time = (4 * kMinUpdateInterval) / 3 + 1;
  EXPECT_EQ(rps_.EncodeFlags(1, false, 90 * time), kPropagateAltRef);
  rps_.ReceivedRPSI(1);
  time += (4 * (time + kMinUpdateInterval)) / 3 + 1;
  
  EXPECT_EQ(rps_.EncodeFlags(2, false, 90 * time), kPropagateGolden);
  rps_.ReceivedRPSI(2);
  
  time = (4 * (time + kMinUpdateInterval)) / 3 + 1;
  EXPECT_EQ(rps_.EncodeFlags(3, false, 90 * time), kPropagateAltRef);
  rps_.ReceivedRPSI(3);
  
  
  time = time + kMinUpdateInterval;
  EXPECT_EQ(rps_.EncodeFlags(4, false, 90 * time), kNoPropagationAltRef);
}

TEST_F(TestRPS, TestDecoderRefresh) {
  uint32_t time = kRtt + 1;
  
  EXPECT_EQ(rps_.ReceivedSLI(90 * time), true);
  time += 5;
  EXPECT_EQ(rps_.ReceivedSLI(90 * time), false);
  time += kRtt - 4;
  EXPECT_EQ(rps_.ReceivedSLI(90 * time), true);
  
  
  EXPECT_EQ(rps_.EncodeFlags(5, true, 90 * time), kRefreshFromGolden |
            kPropagateAltRef);
  rps_.ReceivedRPSI(5);
  time += kRtt + 1;
  
  
  EXPECT_EQ(rps_.ReceivedSLI(90 * time), true);
  EXPECT_EQ(rps_.EncodeFlags(6, true, 90 * time), kRefreshFromAltRef |
            kNoPropagationAltRef);
}

TEST_F(TestRPS, TestWrap) {
  EXPECT_EQ(rps_.ReceivedSLI(0xffffffff), true);
  EXPECT_EQ(rps_.ReceivedSLI(1), false);
  EXPECT_EQ(rps_.ReceivedSLI(90 * 100), true);

  EXPECT_EQ(rps_.EncodeFlags(7, false, 0xffffffff), kPropagateAltRef);
  EXPECT_EQ(rps_.EncodeFlags(8, false, 1), kNoPropagationGolden);
  EXPECT_EQ(rps_.EncodeFlags(10, false, 90 * 100), kPropagateAltRef);
}
