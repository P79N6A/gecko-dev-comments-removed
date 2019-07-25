



































#include "mozilla/ModuleUtils.h"
#include "nsCOMPtr.h"
#include "nsProfiler.h"
#include "nsProfilerCIID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsProfiler)

NS_DEFINE_NAMED_CID(NS_PROFILER_CID);

static const mozilla::Module::CIDEntry kProfilerCIDs[] = {
    { &kNS_PROFILER_CID, false, NULL, nsProfilerConstructor },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kProfilerContracts[] = {
    { "@mozilla.org/tools/profiler;1", &kNS_PROFILER_CID },
    { NULL }
};

static const mozilla::Module kProfilerModule = {
    mozilla::Module::kVersion,
    kProfilerCIDs,
    kProfilerContracts
};

NSMODULE_DEFN(nsProfilerModule) = &kProfilerModule;
