









































#include "xpctest_private.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "mozilla/ModuleUtils.h"
#include "nsCRT.h"
#include "nsIClassInfoImpl.h"

NS_DEFINE_NAMED_CID(NS_XPCTESTOBJECTREADONLY_CID);
NS_DEFINE_NAMED_CID(NS_XPCTESTOBJECTREADWRITE_CID);

static const mozilla::Module::CIDEntry kXPCTestCIDs[] = {
    { &kNS_XPCTESTOBJECTREADONLY_CID, false, NULL, xpctest::ConstructXPCTestObjectReadOnly },
    { &kNS_XPCTESTOBJECTREADWRITE_CID, false, NULL, xpctest::ConstructXPCTestObjectReadWrite },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kXPCTestContracts[] = {
    { "@mozilla.org/js/xpc/test/ObjectReadOnly;1", &kNS_XPCTESTOBJECTREADONLY_CID },
    { "@mozilla.org/js/xpc/test/ObjectReadWrite;1", &kNS_XPCTESTOBJECTREADWRITE_CID },
    { NULL }
};

static const mozilla::Module kXPCTestModule = {
    mozilla::Module::kVersion,
    kXPCTestCIDs,
    kXPCTestContracts
};

NSMODULE_DEFN(xpconnect_test) = &kXPCTestModule;
