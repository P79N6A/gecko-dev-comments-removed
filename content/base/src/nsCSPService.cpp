





































#include "prlog.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIObserver.h"
#include "nsIDocument.h"
#include "nsIContent.h"
#include "nsContentUtils.h"
#include "nsCSPService.h"
#include "nsIContentSecurityPolicy.h"


static PRBool gCSPEnabled = PR_TRUE;

#ifdef PR_LOGGING
static PRLogModuleInfo* gCspPRLog;
#endif

CSPService::CSPService()
{
  nsContentUtils::AddBoolPrefVarCache("security.csp.enable", &gCSPEnabled);

#ifdef PR_LOGGING
  if (!gCspPRLog)
    gCspPRLog = PR_NewLogModule("CSP");
#endif
}

CSPService::~CSPService()
{
}

NS_IMPL_ISUPPORTS1(CSPService, nsIContentPolicy)


NS_IMETHODIMP
CSPService::ShouldLoad(PRUint32 aContentType,
                       nsIURI *aContentLocation,
                       nsIURI *aRequestOrigin,
                       nsISupports *aRequestContext,
                       const nsACString &aMimeTypeGuess,
                       nsISupports *aExtra,
                       PRInt16 *aDecision)
{
    if (!aContentLocation)
        return NS_ERROR_FAILURE;

#ifdef PR_LOGGING 
    {
        nsCAutoString location;
        aContentLocation->GetSpec(location);
        PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
            ("CSPService::ShouldLoad called for %s", location.get()));
    }
#endif
    
    *aDecision = nsIContentPolicy::ACCEPT;

    
    if (!gCSPEnabled)
        return NS_OK;

    
    
    nsresult rv;
    nsCOMPtr<nsINode> node(do_QueryInterface(aRequestContext));
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIContentSecurityPolicy> csp;
    if (node) {
        principal = node->NodePrincipal();
        principal->GetCsp(getter_AddRefs(csp));

        if (csp) {
#ifdef PR_LOGGING 
            nsAutoString policy;
            csp->GetPolicy(policy);
            PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
                    ("Document has CSP: %s", 
                     NS_ConvertUTF16toUTF8(policy).get()));
#endif
            
            csp->ShouldLoad(aContentType,
                            aContentLocation,
                            aRequestOrigin,
                            aRequestContext,
                            aMimeTypeGuess,
                            aExtra,
                            aDecision);
        }
    }
#ifdef PR_LOGGING
    else {
        nsCAutoString uriSpec;
        aContentLocation->GetSpec(uriSpec);
        PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
            ("COULD NOT get nsINode for location: %s", uriSpec.get()));
    }
#endif
	
    return NS_OK;
}

NS_IMETHODIMP
CSPService::ShouldProcess(PRUint32         aContentType,
                          nsIURI           *aContentLocation,
                          nsIURI           *aRequestOrigin,
                          nsISupports      *aRequestContext,
                          const nsACString &aMimeTypeGuess,
                          nsISupports      *aExtra,
                          PRInt16          *aDecision)
{
    if (!aContentLocation)
        return NS_ERROR_FAILURE;

    
    *aDecision = nsIContentPolicy::ACCEPT;

    
    if (!gCSPEnabled)
        return NS_OK;

    
    
    nsresult rv;
    nsCOMPtr<nsINode> node(do_QueryInterface(aRequestContext));
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIContentSecurityPolicy> csp;
    if (node) {
        principal = node->NodePrincipal();
        principal->GetCsp(getter_AddRefs(csp));

        if (csp) {
#ifdef PR_LOGGING
            nsAutoString policy;
            csp->GetPolicy(policy);
            PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
                  ("shouldProcess - document has policy: %s",
                    NS_ConvertUTF16toUTF8(policy).get()));
#endif
            
            csp->ShouldProcess(aContentType,
                               aContentLocation,
                               aRequestOrigin,
                               aRequestContext,
                               aMimeTypeGuess,
                               aExtra,
                               aDecision);
        }
    }
#ifdef PR_LOGGING
    else {
        nsCAutoString uriSpec;
        aContentLocation->GetSpec(uriSpec);
        PR_LOG(gCspPRLog, PR_LOG_DEBUG, 
            ("COULD NOT get nsINode for location: %s", uriSpec.get()));
    }
#endif
    return NS_OK;
}
