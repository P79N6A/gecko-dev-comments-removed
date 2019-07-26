









#include "testing/gtest/include/gtest/gtest.h"

#include "webrtc/modules/remote_bitrate_estimator/remote_bitrate_estimator_unittest_helper.h"
#include "webrtc/system_wrappers/interface/constructor_magic.h"

namespace webrtc {

class RemoteBitrateEstimatorSingleTest : public RemoteBitrateEstimatorTest {
 public:
  static const uint32_t kRemoteBitrateEstimatorMinBitrateBps = 30000;

  RemoteBitrateEstimatorSingleTest() {}
  virtual void SetUp() {
    bitrate_estimator_.reset(RemoteBitrateEstimatorFactory().Create(
        bitrate_observer_.get(),
        &clock_,
        kRemoteBitrateEstimatorMinBitrateBps));
  }
 protected:
  DISALLOW_COPY_AND_ASSIGN(RemoteBitrateEstimatorSingleTest);
};

TEST_F(RemoteBitrateEstimatorSingleTest, InitialBehavior) {
  InitialBehaviorTestHelper(498075);
}

TEST_F(RemoteBitrateEstimatorSingleTest, RateIncreaseReordering) {
  RateIncreaseReorderingTestHelper();
}

TEST_F(RemoteBitrateEstimatorSingleTest, RateIncreaseRtpTimestamps) {
  RateIncreaseRtpTimestampsTestHelper();
}



TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropOneStream) {
  CapacityDropTestHelper(1, false, 956214, 367);
}




TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropOneStreamWrap) {
  CapacityDropTestHelper(1, true, 956214, 367);
}




TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropTwoStreamsWrap) {
  CapacityDropTestHelper(2, true, 927088, 267);
}




TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropThreeStreamsWrap) {
  CapacityDropTestHelper(3, true, 920944, 333);
}

TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropThirteenStreamsWrap) {
  CapacityDropTestHelper(13, true, 938944, 300);
}

TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropNineteenStreamsWrap) {
  CapacityDropTestHelper(19, true, 926718, 300);
}

TEST_F(RemoteBitrateEstimatorSingleTest, CapacityDropThirtyStreamsWrap) {
  CapacityDropTestHelper(30, true, 927016, 300);
}
}  
