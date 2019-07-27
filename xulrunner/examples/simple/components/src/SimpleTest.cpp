




#include <stdio.h>
#include "nsISimpleTest.h"
#include "mozilla/ModuleUtils.h"

class SimpleTest : public nsISimpleTest
{
  ~SimpleTest() {}
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSISIMPLETEST
};

NS_IMPL_ISUPPORTS(SimpleTest, nsISimpleTest)

NS_IMETHODIMP
SimpleTest::Add(int32_t a, int32_t b, int32_t *r)
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
  { &kSIMPLETEST_CID, false, nullptr, SimpleTestConstructor },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kSimpleContracts[] = {
  { "@test.mozilla.org/simple-test;1?impl=c++", &kSIMPLETEST_CID },
  { nullptr }
};

static const mozilla::Module kSimpleModule = {
  mozilla::Module::kVersion,
  kSimpleCIDs,
  kSimpleContracts
};

NSMODULE_DEFN(SimpleTestModule) = &kSimpleModule;
