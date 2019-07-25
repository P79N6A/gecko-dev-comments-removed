









#include "modules/video_coding/codecs/test/stats.h"

#include "gtest/gtest.h"
#include "typedefs.h"

namespace webrtc {
namespace test {

class StatsTest: public testing::Test {
 protected:
  StatsTest() {
  }

  virtual ~StatsTest() {
  }

  void SetUp() {
    stats_ = new Stats();
  }

  void TearDown() {
    delete stats_;
  }

  Stats* stats_;
};


TEST_F(StatsTest, Uninitialized) {
  EXPECT_EQ(0u, stats_->stats_.size());
  stats_->PrintSummary();  
}


TEST_F(StatsTest, AddOne) {
  stats_->NewFrame(0u);
  FrameStatistic* frameStat = &stats_->stats_[0];
  EXPECT_EQ(0, frameStat->frame_number);
}


TEST_F(StatsTest, AddMany) {
  int nbr_of_frames = 1000;
  for (int i = 0; i < nbr_of_frames; ++i) {
    FrameStatistic& frameStat = stats_->NewFrame(i);
    EXPECT_EQ(i, frameStat.frame_number);
  }
  EXPECT_EQ(nbr_of_frames, static_cast<int>(stats_->stats_.size()));

  stats_->PrintSummary();  
}

}  
}  
