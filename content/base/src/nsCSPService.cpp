





































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
#include "nsIChannelPolicy.h"
#include "nsIChannelEventSink.h"
#include "nsIPropertyBag2.h"
#include "nsIWritablePropertyBag2.h"
#include "nsNetError.h"
#include "nsChannelProperties.h"


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

NS_IMPL_ISUPPORTS2(CSPService, nsIContentPolicy, nsIChannelEventSink)


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

    
    
    nsCOMPtr<nsINode> node(do_QueryInterface(aRequestContext));
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIContentSecurityPolicy> csp;
    if (node) {
        principal = node->NodePrincipal();
        principal->GetCsp(getter_AddRefs(csp));

        if (csp) {
#ifdef PR_LOGGING
            nsAutoString policy;
            csp->GetEnforcedPolicy(policy);
            if (policy.Length() > 0) {
              PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                      ("Document CSP: %s",
                      NS_ConvertUTF16toUTF8(policy).get()));
            }
            csp->GetReportOnlyPolicy(policy);
            if (policy.Length() > 0) {
              PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                      ("Report-Only CSP: %s",
                      NS_ConvertUTF16toUTF8(policy).get()));
            }
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

    
    
    nsCOMPtr<nsINode> node(do_QueryInterface(aRequestContext));
    nsCOMPtr<nsIPrincipal> principal;
    nsCOMPtr<nsIContentSecurityPolicy> csp;
    if (node) {
        principal = node->NodePrincipal();
        principal->GetCsp(getter_AddRefs(csp));

        if (csp) {
#ifdef PR_LOGGING
            nsAutoString policy;
            csp->GetEnforcedPolicy(policy);
            if (policy.Length() > 0) {
              PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                      ("shouldProcess - CSP: %s",
                      NS_ConvertUTF16toUTF8(policy).get()));
            }
            csp->GetReportOnlyPolicy(policy);
            if (policy.Length() > 0) {
              PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                      ("shouldProcess - Report-Only CSP: %s",
                      NS_ConvertUTF16toUTF8(policy).get()));
            }
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


NS_IMETHODIMP
CSPService::OnChannelRedirect(nsIChannel *oldChannel,
                              nsIChannel *newChannel,
                              PRUint32   flags)
{
  
  nsCOMPtr<nsISupports> policyContainer;
  nsCOMPtr<nsIPropertyBag2> props(do_QueryInterface(oldChannel));
  if (!props)
    return NS_OK;

  props->GetPropertyAsInterface(NS_CHANNEL_PROP_CHANNEL_POLICY,
                                NS_GET_IID(nsISupports),
                                getter_AddRefs(policyContainer));

  
  nsCOMPtr<nsIChannelPolicy> channelPolicy(do_QueryInterface(policyContainer));
  if (!channelPolicy)
    return NS_OK;

  nsCOMPtr<nsIContentSecurityPolicy> csp;
  channelPolicy->GetContentSecurityPolicy(getter_AddRefs(csp));
  PRUint32 loadType;
  channelPolicy->GetLoadType(&loadType);

  
  if (!csp)
    return NS_OK;

  







  
  
  nsCOMPtr<nsIURI> newUri;
  newChannel->GetURI(getter_AddRefs(newUri));
  PRInt16 aDecision = nsIContentPolicy::ACCEPT;
  csp->ShouldLoad(loadType,        
                  newUri,          
                  nsnull,          
                  nsnull,          
                  EmptyCString(),  
                  nsnull,          
                  &aDecision);

#ifdef PR_LOGGING
  if (newUri) {
    nsCAutoString newUriSpec("None");
    newUri->GetSpec(newUriSpec);
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::OnChannelRedirect called for %s", newUriSpec.get()));
  }
  if (aDecision == 1)
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::OnChannelRedirect ALLOWING request."));
  else
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::OnChannelRedirect CANCELLING request."));
#endif

  
  if (aDecision != 1)
    return NS_BINDING_FAILED;

  
  
  nsresult rv;
  nsCOMPtr<nsIWritablePropertyBag2> props2 = do_QueryInterface(newChannel, &rv);
  if (props2)
    props2->SetPropertyAsInterface(NS_CHANNEL_PROP_CHANNEL_POLICY,
                                   channelPolicy);

  return NS_OK;
}
