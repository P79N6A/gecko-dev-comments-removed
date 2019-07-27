





#include "mozilla/ModuleUtils.h"

#define NS_TESTING_CID \
{ 0x335fb596, 0xe52d, 0x418f, \
  { 0xb0, 0x1c, 0x1b, 0xf1, 0x6c, 0xe5, 0xe7, 0xe4 } }
#define NS_NONEXISTENT_CID \
{ 0x1e61fb15, 0xead4, 0x45cd, \
  { 0x80, 0x13, 0x40, 0x99, 0xa7, 0x10, 0xa2, 0xfa } }

NS_DEFINE_NAMED_CID(NS_TESTING_CID);
NS_DEFINE_NAMED_CID(NS_NONEXISTENT_CID);

static nsresult
DummyConstructorFunc(nsISupports* aOuter, const nsIID& aIID, void** aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

static const mozilla::Module::CIDEntry kTestCIDs[] = {
  { &kNS_TESTING_CID, false, nullptr, DummyConstructorFunc },
  { &kNS_TESTING_CID, false, nullptr, DummyConstructorFunc },
  { nullptr }
};

static const mozilla::Module::ContractIDEntry kTestContractIDs[] = {
  { "@testing/foo", &kNS_NONEXISTENT_CID },
  { nullptr }
};

static const mozilla::Module kTestModule = {
  mozilla::Module::kVersion,
  kTestCIDs,
  kTestContractIDs
};

NSMODULE_DEFN(dummy) = &kTestModule;

  
