





#include "mozilla/ModuleUtils.h"

#define NS_TESTING_CID \
{ 0x335fb596, 0xe52d, 0x418f, \
  { 0xb0, 0x1c, 0x1b, 0xf1, 0x6c, 0xe5, 0xe7, 0xe4 } }

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
  mozilla::Module::kVersion,
  kTestCIDs
};

NSMODULE_DEFN(dummy) = &kTestModule;

  
