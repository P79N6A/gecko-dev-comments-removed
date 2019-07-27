









#ifndef WEBRTC_BASE_GUNIT_H_
#define WEBRTC_BASE_GUNIT_H_

#include "webrtc/base/logging.h"
#include "webrtc/base/thread.h"
#if defined(WEBRTC_ANDROID) || defined(GTEST_RELATIVE_PATH)
#include "testing/gtest/include/gtest/gtest.h"
#else
#include "testing/base/public/gunit.h"
#endif


#define WAIT(ex, timeout) \
  for (uint32 start = rtc::Time(); \
      !(ex) && rtc::Time() < start + timeout;) \
    rtc::Thread::Current()->ProcessMessages(1);




#define WAIT_(ex, timeout, res) \
  do { \
    uint32 start = rtc::Time(); \
    res = (ex); \
    while (!res && rtc::Time() < start + timeout) { \
      rtc::Thread::Current()->ProcessMessages(1); \
      res = (ex); \
    } \
  } while (0);


#define EXPECT_TRUE_WAIT(ex, timeout) \
  do { \
    bool res; \
    WAIT_(ex, timeout, res); \
    if (!res) EXPECT_TRUE(ex); \
  } while (0);

#define EXPECT_EQ_WAIT(v1, v2, timeout) \
  do { \
    bool res; \
    WAIT_(v1 == v2, timeout, res); \
    if (!res) EXPECT_EQ(v1, v2); \
  } while (0);

#define ASSERT_TRUE_WAIT(ex, timeout) \
  do { \
    bool res; \
    WAIT_(ex, timeout, res); \
    if (!res) ASSERT_TRUE(ex); \
  } while (0);

#define ASSERT_EQ_WAIT(v1, v2, timeout) \
  do { \
    bool res; \
    WAIT_(v1 == v2, timeout, res); \
    if (!res) ASSERT_EQ(v1, v2); \
  } while (0);




#define EXPECT_TRUE_WAIT_MARGIN(ex, timeout, margin) \
  do { \
    bool res; \
    WAIT_(ex, timeout, res); \
    if (res) { \
      break; \
    } \
    LOG(LS_WARNING) << "Expression " << #ex << " still not true after " << \
        timeout << "ms; waiting an additional " << margin << "ms"; \
    WAIT_(ex, margin, res); \
    if (!res) { \
      EXPECT_TRUE(ex); \
    } \
  } while (0);

#endif  
