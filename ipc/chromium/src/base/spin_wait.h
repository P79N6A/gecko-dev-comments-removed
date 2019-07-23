













#ifndef BASE_SPIN_WAIT_H__
#define BASE_SPIN_WAIT_H__

#include "base/platform_thread.h"
#include "base/time.h"













#define SPIN_FOR_1_SECOND_OR_UNTIL_TRUE(expression) \
    SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(base::TimeDelta::FromSeconds(1), \
                                     (expression))

#define SPIN_FOR_TIMEDELTA_OR_UNTIL_TRUE(delta, expression) do { \
  base::Time start = base::Time::Now(); \
  const base::TimeDelta kTimeout = delta; \
    while(!(expression)) { \
      if (kTimeout < base::Time::Now() - start) { \
      EXPECT_LE((base::Time::Now() - start).InMilliseconds(), \
                kTimeout.InMilliseconds()) << "Timed out"; \
        break; \
      } \
      PlatformThread::Sleep(50); \
    } \
  } \
  while(0)

#endif  
