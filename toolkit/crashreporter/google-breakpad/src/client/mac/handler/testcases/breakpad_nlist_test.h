




































#ifndef CLIENT_MAC_HANDLER_TESTCASES_BREAKPAD_NLIST_TEST_H__
#define CLIENT_MAC_HANDLER_TESTCASES_BREAKPAD_NLIST_TEST_H__

#include <CPlusTest/CPlusTest.h>

class BreakpadNlistTest : public TestCase {
 private:

  
  
  
  
  bool IsSymbolMoreThanOnceInDyld(const char *symbolName);

 public:
  explicit BreakpadNlistTest(TestInvocation* invocation);
  virtual ~BreakpadNlistTest();


  

  void CompareToNM();
};

#endif 
