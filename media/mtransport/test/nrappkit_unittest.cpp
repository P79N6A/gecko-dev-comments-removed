







#include <iostream>

#include "nsThreadUtils.h"
#include "nsXPCOM.h"


extern "C" {
#include "nr_api.h"
#include "async_timer.h"
}

#include "mtransport_test_utils.h"
#include "runnable_utils.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"
#include "gtest_utils.h"

using namespace mozilla;

MtransportTestUtils *test_utils;

namespace {

class TimerTest : public ::testing::Test {
 public:
  TimerTest() : handle_(nullptr), fired_(false) {}

  int ArmTimer(int timeout) {
    int ret;

    test_utils->sts_target()->Dispatch(
        WrapRunnableRet(&ret, this, &TimerTest::ArmTimer_w, timeout),
        NS_DISPATCH_SYNC);

    return ret;
  }

  int ArmCancelTimer(int timeout) {
    int ret;

    test_utils->sts_target()->Dispatch(
        WrapRunnableRet(&ret, this, &TimerTest::ArmCancelTimer_w, timeout),
        NS_DISPATCH_SYNC);

    return ret;
  }

  int ArmTimer_w(int timeout) {
    return NR_ASYNC_TIMER_SET(timeout, cb, this, &handle_);
  }

  int ArmCancelTimer_w(int timeout) {
    int r;
    r = ArmTimer_w(timeout);
    if (r)
      return r;

    return CancelTimer_w();
  }

  int CancelTimer() {
    int ret;

    test_utils->sts_target()->Dispatch(
        WrapRunnableRet(&ret, this, &TimerTest::CancelTimer_w),
        NS_DISPATCH_SYNC);

    return ret;
  }

  int CancelTimer_w() {
    return NR_async_timer_cancel(handle_);
  }

  int Schedule() {
    int ret;

    test_utils->sts_target()->Dispatch(
        WrapRunnableRet(&ret, this, &TimerTest::Schedule_w),
        NS_DISPATCH_SYNC);

    return ret;
  }

  int Schedule_w() {
    NR_ASYNC_SCHEDULE(cb, this);

    return 0;
  }


  static void cb(NR_SOCKET r, int how, void *arg) {
    std::cerr << "Timer fired " << std::endl;

    TimerTest *t = static_cast<TimerTest *>(arg);

    t->fired_ = true;
  }

 protected:
  void *handle_;
  bool fired_;
};
}

TEST_F(TimerTest, SimpleTimer) {
  ArmTimer(100);
  ASSERT_TRUE_WAIT(fired_, 1000);
}

TEST_F(TimerTest, CancelTimer) {
  ArmTimer(1000);
  CancelTimer();
  PR_Sleep(2000);
  ASSERT_FALSE(fired_);
}

TEST_F(TimerTest, CancelTimer0) {
  ArmCancelTimer(0);
  PR_Sleep(100);
  ASSERT_FALSE(fired_);
}

TEST_F(TimerTest, ScheduleTest) {
  Schedule();
  ASSERT_TRUE_WAIT(fired_, 1000);
}

int main(int argc, char **argv) {
  test_utils = new MtransportTestUtils();

  
  ::testing::InitGoogleTest(&argc, argv);

  int rv = RUN_ALL_TESTS();
  delete test_utils;
  return rv;
}
