




































#include "mozilla/ModuleUtils.h"
#include "nsTransactionManagerCID.h"
#include "nsTransactionStack.h"
#include "nsTransactionManager.h"






NS_GENERIC_FACTORY_CONSTRUCTOR(nsTransactionManager)
NS_DEFINE_NAMED_CID(NS_TRANSACTIONMANAGER_CID);

static const mozilla::Module::CIDEntry kTxMgrCIDs[] = {
    { &kNS_TRANSACTIONMANAGER_CID, false, NULL, nsTransactionManagerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kTxMgrContracts[] = {
    { NS_TRANSACTIONMANAGER_CONTRACTID, &kNS_TRANSACTIONMANAGER_CID },
    { NULL }
};

static const mozilla::Module kTxMgrModule = {
    mozilla::Module::kVersion,
    kTxMgrCIDs,
    kTxMgrContracts
};
NSMODULE_DEFN(nsTransactionManagerModule) = &kTxMgrModule;
