




































#ifndef _CLIENT_MAC_HANDLER_TESTCASES_DYNAMICIMAGESTESTS_H__
#define _CLIENT_MAC_HANDLER_TESTCASES_DYNAMICIMAGESTESTS_H__

#include <CPlusTest/CPlusTest.h>

class DynamicImagesTests : public TestCase {
 public:
  explicit DynamicImagesTests(TestInvocation* invocation);
  virtual ~DynamicImagesTests();

  void ReadTaskMemoryTest();
  void ReadLibrariesFromLocalTaskTest();
};

#endif 
