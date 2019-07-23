




































#include "nsCOMPtr.h"
#include "nsIModule.h"
#include "nsIGenericFactory.h"

#include "nsIServiceManager.h"
#include "nsIComponentManager.h"
#include "nsIAccessibilityService.h"
#include "nsIAccessibleRetrieval.h"
#include "nscore.h"

static NS_IMETHODIMP
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


static const nsModuleComponentInfo components[] = 
{
    { "AccessibilityService", 
      NS_ACCESSIBILITY_SERVICE_CID,
      "@mozilla.org/accessibilityService;1", 
      NS_ConstructAccessibilityService
    },
    { "AccessibleRetrieval", 
      NS_ACCESSIBLE_RETRIEVAL_CID,
      "@mozilla.org/accessibleRetrieval;1", 
      NS_ConstructAccessibilityService
    },
};

NS_IMPL_NSGETMODULE(nsAccessibilityModule, components)


