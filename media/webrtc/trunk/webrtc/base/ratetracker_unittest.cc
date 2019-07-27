









#include "webrtc/base/gunit.h"
#include "webrtc/base/ratetracker.h"

namespace rtc {

class RateTrackerForTest : public RateTracker {
 public:
  RateTrackerForTest() : time_(0) {}
  virtual uint32 Time() const { return time_; }
  void AdvanceTime(uint32 delta) { time_ += delta; }

 private:
  uint32 time_;
};

TEST(RateTrackerTest, TestBasics) {
  RateTrackerForTest tracker;
  EXPECT_EQ(0U, tracker.total_units());
  EXPECT_EQ(0U, tracker.units_second());

  
  tracker.Update(1234);
  
  tracker.AdvanceTime(100);
  
  EXPECT_EQ(1234U, tracker.total_units());
  EXPECT_EQ(0U, tracker.units_second());

  
  tracker.Update(1234);
  tracker.AdvanceTime(100);
  EXPECT_EQ(1234U * 2, tracker.total_units());
  EXPECT_EQ(0U, tracker.units_second());

  
  
  tracker.AdvanceTime(800);
  EXPECT_EQ(1234U * 2, tracker.total_units());
  EXPECT_EQ(1234U * 2, tracker.units_second());

  
  EXPECT_EQ(1234U * 2, tracker.total_units());
  EXPECT_EQ(1234U * 2, tracker.units_second());

  
  tracker.AdvanceTime(1000);
  EXPECT_EQ(1234U * 2, tracker.total_units());
  EXPECT_EQ(0U, tracker.units_second());

  
  
  for (int i = 0; i < 5500; i += 100) {
    tracker.Update(9876U);
    tracker.AdvanceTime(100);
  }
  EXPECT_EQ(9876U * 10, tracker.units_second());

  
  
  tracker.AdvanceTime(500);
  EXPECT_EQ(9876U * 5, tracker.units_second());
}

}  
