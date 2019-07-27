





































#ifndef gtest_utils_h__
#define gtest_utils_h__

#include <iostream>

#include "nspr.h"
#include "prinrval.h"
#include "prthread.h"

#define GTEST_HAS_RTTI 0
#include "gtest/gtest.h"


#define WAIT(expression, timeout) \
  do { \
    for (PRIntervalTime start = PR_IntervalNow(); !(expression) && \
           ! ((PR_IntervalNow() - start) > PR_MillisecondsToInterval(timeout));) { \
      PR_Sleep(10); \
    } \
  } while(0)




#define WAIT_(expression, timeout, res) \
  do { \
    for (PRIntervalTime start = PR_IntervalNow(); !(res = (expression)) && \
           ! ((PR_IntervalNow() - start) > PR_MillisecondsToInterval(timeout));) { \
      PR_Sleep(10); \
    } \
  } while(0)

#define ASSERT_TRUE_WAIT(expression, timeout) \
  do { \
    bool res; \
    WAIT_(expression, timeout, res); \
    ASSERT_TRUE(res); \
  } while(0)

#define EXPECT_TRUE_WAIT(expression, timeout) \
  do { \
    bool res; \
    WAIT_(expression, timeout, res); \
    EXPECT_TRUE(res); \
  } while(0)

#define ASSERT_EQ_WAIT(expected, actual, timeout) \
  do { \
    WAIT(expected == actual, timeout); \
    ASSERT_EQ(expected, actual); \
  } while(0)

#endif
