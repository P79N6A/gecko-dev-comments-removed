



#ifndef TESTING_PLATFORM_TEST_H_
#define TESTING_PLATFORM_TEST_H_

#include <gtest/gtest.h>

#if defined(GTEST_OS_MAC)
#ifdef __OBJC__
@class NSAutoreleasePool;
#else
class NSAutoreleasePool;
#endif






class PlatformTest : public testing::Test {
 protected:
  PlatformTest();
  virtual ~PlatformTest();

 private:
  NSAutoreleasePool* pool_;
};
#else
typedef testing::Test PlatformTest;
#endif 

#endif 
