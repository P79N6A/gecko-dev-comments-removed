






































#include <stdio.h>
#include "TestFactory.h"
#include "nsIGenericFactory.h"





class TestDynamicClassImpl: public ITestClass {
  NS_DECL_ISUPPORTS

  TestDynamicClassImpl() {
  }
  void Test();
};

NS_IMPL_ISUPPORTS1(TestDynamicClassImpl, ITestClass)

void TestDynamicClassImpl::Test() {
  printf("hello, dynamic world!\n");
}





NS_GENERIC_FACTORY_CONSTRUCTOR(TestDynamicClassImpl)

static const nsModuleComponentInfo components[] =
{
  { "Test Dynamic", NS_TESTLOADEDFACTORY_CID, NS_TESTLOADEDFACTORY_CONTRACTID,
    TestDynamicClassImplConstructor
  }
};

NS_IMPL_NSGETMODULE(nsTestDynamicModule, components)

