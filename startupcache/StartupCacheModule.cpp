





































#include <string.h>

#include "nscore.h"
#include "pratom.h"
#include "prmem.h"
#include "prio.h"
#include "plstr.h"
#include "prlog.h"

#include "nsID.h"
#include "nsIComponentManager.h"
#include "nsIServiceManager.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "mozilla/ModuleUtils.h"
#include "mozilla/scache/StartupCache.h"

using namespace mozilla::scache;


NS_GENERIC_FACTORY_SINGLETON_CONSTRUCTOR(StartupCacheWrapper,
                                         StartupCacheWrapper::GetSingleton)
NS_DEFINE_NAMED_CID(NS_STARTUPCACHE_CID);

static const mozilla::Module::CIDEntry kStartupCacheCIDs[] = {
    { &kNS_STARTUPCACHE_CID, false, NULL, StartupCacheWrapperConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kStartupCacheContracts[] = {
    { "@mozilla.org/startupcache/cache;1", &kNS_STARTUPCACHE_CID },
    { NULL }
};

static const mozilla::Module kStartupCacheModule = {
    mozilla::Module::kVersion,
    kStartupCacheCIDs,
    kStartupCacheContracts,
    NULL,
    NULL,
    NULL,
    NULL
};

NSMODULE_DEFN(StartupCacheModule) = &kStartupCacheModule;
