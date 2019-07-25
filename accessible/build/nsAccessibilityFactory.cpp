




































#include "nsCOMPtr.h"
#include "mozilla/ModuleUtils.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleRetrieval.h"
#include "nscore.h"

static nsresult
NS_ConstructAccessibilityService(nsISupports *aOuter, REFNSIID aIID, void **aResult)
{
    nsresult rv;
    NS_ASSERTION(aOuter == nsnull, "no aggregation");
    nsIAccessibilityService* accessibility;
    rv = NS_GetAccessibilityService(&accessibility);
    if (NS_FAILED(rv)) {
        NS_ERROR("Unable to construct accessibility service");
        return rv;
    }
    rv = accessibility->QueryInterface(aIID, aResult);
    NS_ASSERTION(NS_SUCCEEDED(rv), "unable to find correct interface");
    NS_RELEASE(accessibility);
    return rv;
}

NS_DEFINE_NAMED_CID(NS_ACCESSIBILITY_SERVICE_CID);
NS_DEFINE_NAMED_CID(NS_ACCESSIBLE_RETRIEVAL_CID);

static const mozilla::Module::CIDEntry kA11yCIDs[] = {
    { &kNS_ACCESSIBILITY_SERVICE_CID, false, NULL, NS_ConstructAccessibilityService },
    { NULL }
};

static const mozilla::Module::ContractIDEntry kA11yContracts[] = {
    { "@mozilla.org/accessibilityService;1", &kNS_ACCESSIBILITY_SERVICE_CID },
    { "@mozilla.org/accessibleRetrieval;1", &kNS_ACCESSIBILITY_SERVICE_CID },
    { NULL }
};

static const mozilla::Module kA11yModule = {
    mozilla::Module::kVersion,
    kA11yCIDs,
    kA11yContracts
};

NSMODULE_DEFN(nsAccessibilityModule) = &kA11yModule;


