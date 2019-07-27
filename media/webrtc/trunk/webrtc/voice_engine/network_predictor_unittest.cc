









#include <math.h>

#include "testing/gtest/include/gtest/gtest.h"
#include "webrtc/voice_engine/network_predictor.h"
#include "webrtc/system_wrappers/interface/clock.h"

namespace webrtc {
namespace voe {

class TestNetworkPredictor : public ::testing::Test {
 protected:
  TestNetworkPredictor()
      : clock_(0),
        network_predictor_(new NetworkPredictor(&clock_)) {}
  SimulatedClock clock_;
  scoped_ptr<NetworkPredictor> network_predictor_;
};

TEST_F(TestNetworkPredictor, TestPacketLossRateFilter) {
  
  EXPECT_EQ(0, network_predictor_->GetLossRate());
  network_predictor_->UpdatePacketLossRate(32);
  
  EXPECT_EQ(32, network_predictor_->GetLossRate());
  clock_.AdvanceTimeMilliseconds(1000);
  network_predictor_->UpdatePacketLossRate(40);
  float exp = pow(0.9999f, 1000);
  float value = 32.0f * exp + (1 - exp) * 40.0f;
  EXPECT_EQ(static_cast<uint8_t>(value + 0.5f),
            network_predictor_->GetLossRate());
}
}  
}  
