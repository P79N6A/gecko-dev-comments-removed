









































#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "mozilla/ModuleUtils.h"
#include "nsCRT.h"
#include "nsIClassInfoImpl.h"
#include "xpctest_private.h"

#define NS_XPCTESTOBJECTREADONLY_CID \
{ 0x492609a7, 0x2582, 0x436b, \
   { 0xb0, 0xef, 0x92, 0xe2, 0x9b, 0xb9, 0xe1, 0x43 } }

#define NS_XPCTESTOBJECTREADWRITE_CID \
{ 0x8f37f760, 0x3686, 0x4dbb, \
   { 0xb1, 0x21, 0x96, 0x93, 0xba, 0x81, 0x3f, 0x8f } }

NS_GENERIC_FACTORY_CONSTRUCTOR(xpcTestObjectReadOnly);
NS_GENERIC_FACTORY_CONSTRUCTOR(xpcTestObjectReadWrite);
NS_DEFINE_NAMED_CID(NS_XPCTESTOBJECTREADONLY_CID);
NS_DEFINE_NAMED_CID(NS_XPCTESTOBJECTREADWRITE_CID);

static const mozilla::Module::CIDEntry kXPCTestCIDs[] = {
    { &kNS_XPCTESTOBJECTREADONLY_CID, false, NULL, xpcTestObjectReadOnlyConstructor },
    { &kNS_XPCTESTOBJECTREADWRITE_CID, false, NULL, xpcTestObjectReadWriteConstructor },
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
