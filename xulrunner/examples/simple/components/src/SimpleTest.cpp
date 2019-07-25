




































#include <stdio.h>
#include "nsISimpleTest.h"
#include "mozilla/ModuleUtils.h"

class SimpleTest : public nsISimpleTest
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLETEST
};

NS_IMPL_ISUPPORTS1(SimpleTest, nsISimpleTest)

NS_IMETHODIMP
SimpleTest::Add(PRInt32 a, PRInt32 b, PRInt32 *r)
{
  printf("add(%d,%d) from C++\n", a, b);

  *r = a + b;
  return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(SimpleTest)


#define SIMPLETEST_CID \
  { 0x5e14b432, 0x37b6, 0x4377, \
    { 0x92, 0x3b, 0xc9, 0x87, 0x41, 0x8d, 0x84, 0x29 } }

NS_DEFINE_NAMED_CID(SIMPLETEST_CID);

static const mozilla::Module::CIDEntry kSimpleCIDs[] = {
  { &kSIMPLETEST_CID, false, NULL, SimpleTestConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kSimpleContracts[] = {
  { "@test.mozilla.org/simple-test;1?impl=c++", &kSIMPLETEST_CID },
  { NULL }
};

static const mozilla::Module kSimpleModule = {
  mozilla::Module::kVersion,
  kSimpleCIDs,
  kSimpleContracts
};

NSMODULE_DEFN(SimpleTestModule) = &kSimpleModule;
