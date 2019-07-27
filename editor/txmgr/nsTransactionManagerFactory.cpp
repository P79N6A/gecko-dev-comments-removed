




#include <stddef.h>

#include "mozilla/Module.h"
#include "mozilla/ModuleUtils.h"
#include "nsID.h"
#include "nsITransactionManager.h"
#include "nsTransactionManager.h"
#include "nsTransactionManagerCID.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransactionManager)
NS_DEFINE_NAMED_CID(NS_TRANSACTIONMANAGER_CID);

static const mozilla::Module::CIDEntry kTxMgrCIDs[] = {
    { &kNS_TRANSACTIONMANAGER_CID, false, nullptr, nsTransactionManagerConstructor },
    { nullptr }
};

static const mozilla::Module::ContractIDEntry kTxMgrContracts[] = {
    { NS_TRANSACTIONMANAGER_CONTRACTID, &kNS_TRANSACTIONMANAGER_CID },
    { nullptr }
};

static const mozilla::Module kTxMgrModule = {
    mozilla::Module::kVersion,
    kTxMgrCIDs,
    kTxMgrContracts
};
NSMODULE_DEFN(nsTransactionManagerModule) = &kTxMgrModule;
