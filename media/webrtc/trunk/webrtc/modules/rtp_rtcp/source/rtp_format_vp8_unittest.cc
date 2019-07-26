














#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/system_wrappers/interface/compile_assert.h"

#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8.h"
#include "webrtc/modules/rtp_rtcp/source/rtp_format_vp8_test_helper.h"
#include "webrtc/typedefs.h"

namespace webrtc {

class RtpFormatVp8Test : public ::testing::Test {
 protected:
  RtpFormatVp8Test() : helper_(NULL) {}
  virtual void TearDown() { delete helper_; }
  bool Init(const int* partition_sizes, int num_partitions) {
    hdr_info_.pictureId = kNoPictureId;
    hdr_info_.nonReference = false;
    hdr_info_.temporalIdx = kNoTemporalIdx;
    hdr_info_.layerSync = false;
    hdr_info_.tl0PicIdx = kNoTl0PicIdx;
    hdr_info_.keyIdx = kNoKeyIdx;
    if (helper_ != NULL) return false;
    helper_ = new test::RtpFormatVp8TestHelper(&hdr_info_);
    return helper_->Init(partition_sizes, num_partitions);
  }

  RTPVideoHeaderVP8 hdr_info_;
  test::RtpFormatVp8TestHelper* helper_;
};

TEST_F(RtpFormatVp8Test, TestStrictMode) {
  const int kSizeVector[] = {10, 8, 27};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 200;  
  const int kMaxSize = 13;
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kStrict);

  
  const int kExpectedSizes[] = {9, 9, 12, 11, 11, 11, 10};
  const int kExpectedPart[] = {0, 0, 1, 2, 2, 2, 2};
  const bool kExpectedFragStart[] =
      {true, false, true, true, false, false, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}

TEST_F(RtpFormatVp8Test, TestAggregateMode) {
  const int kSizeVector[] = {60, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 25;
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[] = {23, 23, 23, 23};
  const int kExpectedPart[] = {0, 0, 0, 1};
  const bool kExpectedFragStart[] = {true, false, false, true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}

TEST_F(RtpFormatVp8Test, TestAggregateModeManyPartitions1) {
  const int kSizeVector[] = {1600, 200, 200, 200, 200, 200, 200, 200, 200};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 1500;
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[] = {803, 803, 803, 803};
  const int kExpectedPart[] = {0, 0, 1, 5};
  const bool kExpectedFragStart[] = {true, false, true, true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}

TEST_F(RtpFormatVp8Test, TestAggregateModeManyPartitions2) {
  const int kSizeVector[] = {1599, 200, 200, 200, 1600, 200, 200, 200, 200};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 1500;
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[] = {803, 802, 603, 803, 803, 803};
  const int kExpectedPart[] = {0, 0, 1, 4, 4, 5};
  const bool kExpectedFragStart[] = {true, false, true, true, false, true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}

TEST_F(RtpFormatVp8Test, TestAggregateModeTwoLargePartitions) {
  const int kSizeVector[] = {1654, 2268};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 20;  
  const int kMaxSize = 1460;
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[] = {830, 830, 1137, 1137};
  const int kExpectedPart[] = {0, 0, 1, 1};
  const bool kExpectedFragStart[] = {true, false, true, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}


TEST_F(RtpFormatVp8Test, TestEqualSizeModeFallback) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.pictureId = 200;  
  const int kMaxSize = 12;  
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize);

  
  const int kExpectedSizes[] = {12, 11, 12, 11};
  const int kExpectedPart[] = {0, 0, 0, 0};  
  
  const bool kExpectedFragStart[] = {true, false, false, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->set_sloppy_partitioning(true);
  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}


TEST_F(RtpFormatVp8Test, TestNonReferenceBit) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.nonReference = true;
  const int kMaxSize = 25;  
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize);

  
  const int kExpectedSizes[] = {16, 16};
  const int kExpectedPart[] = {0, 0};  
  
  const bool kExpectedFragStart[] = {true, false};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->set_sloppy_partitioning(true);
  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}


TEST_F(RtpFormatVp8Test, TestTl0PicIdxAndTID) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.tl0PicIdx = 117;
  hdr_info_.temporalIdx = 2;
  hdr_info_.layerSync = true;
  
  const int kMaxSize = helper_->buffer_size();
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[1] = {helper_->payload_size() + 4};
  const int kExpectedPart[1] = {0};  
  const bool kExpectedFragStart[1] = {true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}


TEST_F(RtpFormatVp8Test, TestKeyIdx) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.keyIdx = 17;
  
  const int kMaxSize = helper_->buffer_size();
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[1] = {helper_->payload_size() + 3};
  const int kExpectedPart[1] = {0};  
  const bool kExpectedFragStart[1] = {true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}


TEST_F(RtpFormatVp8Test, TestTIDAndKeyIdx) {
  const int kSizeVector[] = {10, 10, 10};
  const int kNumPartitions = sizeof(kSizeVector) / sizeof(kSizeVector[0]);
  ASSERT_TRUE(Init(kSizeVector, kNumPartitions));

  hdr_info_.temporalIdx = 1;
  hdr_info_.keyIdx = 5;
  
  const int kMaxSize = helper_->buffer_size();
  RtpFormatVp8 packetizer(helper_->payload_data(),
                          helper_->payload_size(),
                          hdr_info_,
                          kMaxSize,
                          *(helper_->fragmentation()),
                          kAggregate);

  
  const int kExpectedSizes[1] = {helper_->payload_size() + 3};
  const int kExpectedPart[1] = {0};  
  const bool kExpectedFragStart[1] = {true};
  const int kExpectedNum = sizeof(kExpectedSizes) / sizeof(kExpectedSizes[0]);
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedPart) / sizeof(kExpectedPart[0]));
  COMPILE_ASSERT(kExpectedNum ==
      sizeof(kExpectedFragStart) / sizeof(kExpectedFragStart[0]));

  helper_->GetAllPacketsAndCheck(&packetizer, kExpectedSizes, kExpectedPart,
                                 kExpectedFragStart, kExpectedNum);
}

}  
