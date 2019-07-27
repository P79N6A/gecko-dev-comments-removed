





#include "mozilla/ModuleUtils.h"


#define NS_TESTING_CID \
{ 0xf18fb09b, 0x28b4, 0x4435, \
  { 0xbc, 0x5b, 0x80, 0x27, 0xf1, 0x8d, 0xf7, 0x43 } }

NS_DEFINE_NAMED_CID(NS_TESTING_CID);

static nsresult
DummyConstructorFunc(nsISupports* aOuter, const nsIID& aIID, void** aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

static const mozilla::Module::CIDEntry kTestCIDs[] = {
  { &kNS_TESTING_CID, false, nullptr, DummyConstructorFunc },
  { nullptr }
};

static const mozilla::Module kTestModule = {
  3, 
  kTestCIDs
};

NSMODULE_DEFN(dummy) = &kTestModule;
