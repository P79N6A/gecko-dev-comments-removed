









#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "webrtc/modules/rtp_rtcp/interface/rtp_rtcp_defines.h"
#include "webrtc/system_wrappers/interface/scoped_ptr.h"
#include "webrtc/system_wrappers/interface/tick_util.h"
#include "webrtc/video_engine/call_stats.h"

using ::testing::_;
using ::testing::AnyNumber;
using ::testing::Return;

namespace webrtc {

class MockStatsObserver : public StatsObserver {
 public:
  MockStatsObserver() {}
  virtual ~MockStatsObserver() {}

  MOCK_METHOD1(OnRttUpdate, void(uint32_t));
};

class CallStatsTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    TickTime::UseFakeClock(12345);
    call_stats_.reset(new CallStats());
  }
  scoped_ptr<CallStats> call_stats_;
};

TEST_F(CallStatsTest, AddAndTriggerCallback) {
  MockStatsObserver stats_observer;
  RtcpRttObserver* rtcp_observer = call_stats_->rtcp_rtt_observer();
  call_stats_->RegisterStatsObserver(&stats_observer);
  TickTime::AdvanceFakeClock(1000);

  uint32_t rtt = 25;
  rtcp_observer->OnRttUpdate(rtt);
  EXPECT_CALL(stats_observer, OnRttUpdate(rtt))
      .Times(1);
  call_stats_->Process();

  call_stats_->DeregisterStatsObserver(&stats_observer);
}

TEST_F(CallStatsTest, ProcessTime) {
  MockStatsObserver stats_observer;
  call_stats_->RegisterStatsObserver(&stats_observer);
  RtcpRttObserver* rtcp_observer = call_stats_->rtcp_rtt_observer();
  rtcp_observer->OnRttUpdate(100);

  
  EXPECT_CALL(stats_observer, OnRttUpdate(_))
      .Times(0);
  call_stats_->Process();

  
  TickTime::AdvanceFakeClock(1000);
  EXPECT_CALL(stats_observer, OnRttUpdate(_))
      .Times(1);
  call_stats_->Process();

  
  TickTime::AdvanceFakeClock(999);
  rtcp_observer->OnRttUpdate(100);
  EXPECT_CALL(stats_observer, OnRttUpdate(_))
      .Times(0);
  call_stats_->Process();

  
  TickTime::AdvanceFakeClock(1);
  EXPECT_CALL(stats_observer, OnRttUpdate(_))
      .Times(1);
  call_stats_->Process();

  call_stats_->DeregisterStatsObserver(&stats_observer);
}



TEST_F(CallStatsTest, MultipleObservers) {
  MockStatsObserver stats_observer_1;
  call_stats_->RegisterStatsObserver(&stats_observer_1);
  
  
  MockStatsObserver stats_observer_2;
  call_stats_->RegisterStatsObserver(&stats_observer_2);
  call_stats_->RegisterStatsObserver(&stats_observer_2);

  RtcpRttObserver* rtcp_observer = call_stats_->rtcp_rtt_observer();
  uint32_t rtt = 100;
  rtcp_observer->OnRttUpdate(rtt);

  
  TickTime::AdvanceFakeClock(1000);
  EXPECT_CALL(stats_observer_1, OnRttUpdate(rtt))
      .Times(1);
  EXPECT_CALL(stats_observer_2, OnRttUpdate(rtt))
      .Times(1);
  call_stats_->Process();

  
  
  call_stats_->DeregisterStatsObserver(&stats_observer_2);
  rtcp_observer->OnRttUpdate(rtt);
  TickTime::AdvanceFakeClock(1000);
  EXPECT_CALL(stats_observer_1, OnRttUpdate(rtt))
      .Times(1);
  EXPECT_CALL(stats_observer_2, OnRttUpdate(rtt))
      .Times(0);
  call_stats_->Process();

  
  call_stats_->DeregisterStatsObserver(&stats_observer_1);
  rtcp_observer->OnRttUpdate(rtt);
  TickTime::AdvanceFakeClock(1000);
  EXPECT_CALL(stats_observer_1, OnRttUpdate(rtt))
      .Times(0);
  EXPECT_CALL(stats_observer_2, OnRttUpdate(rtt))
      .Times(0);
  call_stats_->Process();
}


TEST_F(CallStatsTest, ChangeRtt) {
  MockStatsObserver stats_observer;
  call_stats_->RegisterStatsObserver(&stats_observer);
  RtcpRttObserver* rtcp_observer = call_stats_->rtcp_rtt_observer();

  
  TickTime::AdvanceFakeClock(1000);

  
  const uint32_t first_rtt = 100;
  rtcp_observer->OnRttUpdate(first_rtt);
  EXPECT_CALL(stats_observer, OnRttUpdate(first_rtt))
      .Times(1);
  call_stats_->Process();

  
  TickTime::AdvanceFakeClock(1000);
  const uint32_t high_rtt = first_rtt + 20;
  rtcp_observer->OnRttUpdate(high_rtt);
  EXPECT_CALL(stats_observer, OnRttUpdate(high_rtt))
      .Times(1);
  call_stats_->Process();

  
  
  
  TickTime::AdvanceFakeClock(1000);
  const uint32_t low_rtt = first_rtt - 20;
  rtcp_observer->OnRttUpdate(low_rtt);
  EXPECT_CALL(stats_observer, OnRttUpdate(high_rtt))
      .Times(1);
  call_stats_->Process();

  
  
  TickTime::AdvanceFakeClock(1000);
  EXPECT_CALL(stats_observer, OnRttUpdate(low_rtt))
      .Times(1);
  call_stats_->Process();

  call_stats_->DeregisterStatsObserver(&stats_observer);
}

}  
