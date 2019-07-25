




































#include "nscore.h"
#include "nsLayoutDebugCIID.h"
#include "mozilla/ModuleUtils.h"
#include "nsIFactory.h"
#include "nsISupports.h"
#include "nsRegressionTester.h"
#include "nsLayoutDebuggingTools.h"
#include "nsLayoutDebugCLH.h"
#include "nsIServiceManager.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsRegressionTester)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLayoutDebuggingTools)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLayoutDebugCLH)

NS_DEFINE_NAMED_CID(NS_REGRESSION_TESTER_CID);
NS_DEFINE_NAMED_CID(NS_LAYOUT_DEBUGGINGTOOLS_CID);
NS_DEFINE_NAMED_CID(NS_LAYOUTDEBUGCLH_CID);

static const mozilla::Module::CIDEntry kLayoutDebugCIDs[] = {
  { &kNS_REGRESSION_TESTER_CID, false, NULL, nsRegressionTesterConstructor },
  { &kNS_LAYOUT_DEBUGGINGTOOLS_CID, false, NULL, nsLayoutDebuggingToolsConstructor },
  { &kNS_LAYOUTDEBUGCLH_CID, false, NULL, nsLayoutDebugCLHConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kLayoutDebugContracts[] = {
  { "@mozilla.org/layout-debug/regressiontester;1", &kNS_REGRESSION_TESTER_CID },
  { NS_LAYOUT_DEBUGGINGTOOLS_CONTRACTID, &kNS_LAYOUT_DEBUGGINGTOOLS_CID },
  { "@mozilla.org/commandlinehandler/general-startup;1?type=layoutdebug", &kNS_LAYOUTDEBUGCLH_CID },
  { NULL }
};

static const mozilla::Module::CategoryEntry kLayoutDebugCategories[] = {
  { "command-line-handler", "m-layoutdebug", "@mozilla.org/commandlinehandler/general-startup;1?type=layoutdebug" },
  { NULL }
};

static const mozilla::Module kLayoutDebugModule = {
  mozilla::Module::kVersion,
  kLayoutDebugCIDs,
  kLayoutDebugContracts,
  kLayoutDebugCategories
};

NSMODULE_DEFN(nsLayoutDebugModule) = &kLayoutDebugModule;
