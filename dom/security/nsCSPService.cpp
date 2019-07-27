




#include "prlog.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIObserver.h"
#include "nsIContent.h"
#include "nsCSPService.h"
#include "nsIContentSecurityPolicy.h"
#include "nsError.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "mozilla/Preferences.h"
#include "nsIScriptError.h"
#include "nsContentUtils.h"
#include "nsContentPolicyUtils.h"
#include "nsPrincipal.h"

using namespace mozilla;


bool CSPService::sCSPEnabled = true;

#ifdef PR_LOGGING
static PRLogModuleInfo* gCspPRLog;
#endif

CSPService::CSPService()
{
  Preferences::AddBoolVarCache(&sCSPEnabled, "security.csp.enable");

#ifdef PR_LOGGING
  if (!gCspPRLog)
    gCspPRLog = PR_NewLogModule("CSP");
#endif
}

CSPService::~CSPService()
{
  mAppStatusCache.Clear();
}

NS_IMPL_ISUPPORTS(CSPService, nsIContentPolicy, nsIChannelEventSink)


bool
subjectToCSP(nsIURI* aURI) {
  
  
  
  
  
  
  bool match = false;
  nsresult rv = aURI->SchemeIs("data", &match);
  if (NS_SUCCEEDED(rv) && match) {
    return true;
  }
  rv = aURI->SchemeIs("blob", &match);
  if (NS_SUCCEEDED(rv) && match) {
    return true;
  }
  rv = aURI->SchemeIs("filesystem", &match);
  if (NS_SUCCEEDED(rv) && match) {
    return true;
  }
  
  
  rv = aURI->SchemeIs("about", &match);
  if (NS_SUCCEEDED(rv) && match) {
    return false;
  }

  
  
  
  
  
  
  
  
  
  rv = NS_URIChainHasFlags(aURI, nsIProtocolHandler::URI_IS_LOCAL_RESOURCE, &match);
  if (NS_SUCCEEDED(rv) && match) {
    return false;
  }
  rv = NS_URIChainHasFlags(aURI, nsIProtocolHandler::URI_INHERITS_SECURITY_CONTEXT, &match);
  if (NS_SUCCEEDED(rv) && match) {
    return false;
  }
  
  return true;
}


NS_IMETHODIMP
CSPService::ShouldLoad(uint32_t aContentType,
                       nsIURI *aContentLocation,
                       nsIURI *aRequestOrigin,
                       nsISupports *aRequestContext,
                       const nsACString &aMimeTypeGuess,
                       nsISupports *aExtra,
                       nsIPrincipal *aRequestPrincipal,
                       int16_t *aDecision)
{
  if (!aContentLocation) {
    return NS_ERROR_FAILURE;
  }

#ifdef PR_LOGGING
  {
    nsAutoCString location;
    aContentLocation->GetSpec(location);
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::ShouldLoad called for %s", location.get()));
  }
#endif

  
  *aDecision = nsIContentPolicy::ACCEPT;

  
  
  
  
  
  if (!sCSPEnabled || !subjectToCSP(aContentLocation)) {
    return NS_OK;
  }

  
  
  
  
  if (aContentType == nsIContentPolicy::TYPE_CSP_REPORT ||
    aContentType == nsIContentPolicy::TYPE_REFRESH ||
    aContentType == nsIContentPolicy::TYPE_DOCUMENT) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsINode> node(do_QueryInterface(aRequestContext));
  nsCOMPtr<nsIPrincipal> principal;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  if (node) {
    principal = node->NodePrincipal();
    principal->GetCsp(getter_AddRefs(csp));

    if (csp) {
#ifdef PR_LOGGING
      {
        uint32_t numPolicies = 0;
        nsresult rv = csp->GetPolicyCount(&numPolicies);
        if (NS_SUCCEEDED(rv)) {
          for (uint32_t i=0; i<numPolicies; i++) {
            nsAutoString policy;
            csp->GetPolicy(i, policy);
            PR_LOG(gCspPRLog, PR_LOG_DEBUG,
                   ("Document has CSP[%d]: %s", i,
                   NS_ConvertUTF16toUTF8(policy).get()));
          }
        }
      }
#endif
      
      
      csp->ShouldLoad(aContentType,
                      aContentLocation,
                      aRequestOrigin,
                      aRequestContext,
                      aMimeTypeGuess,
                      nullptr,
                      aDecision);
    }
  }
#ifdef PR_LOGGING
  else {
    nsAutoCString uriSpec;
    aContentLocation->GetSpec(uriSpec);
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("COULD NOT get nsINode for location: %s", uriSpec.get()));
  }
#endif

  return NS_OK;
}

NS_IMETHODIMP
CSPService::ShouldProcess(uint32_t         aContentType,
                          nsIURI           *aContentLocation,
                          nsIURI           *aRequestOrigin,
                          nsISupports      *aRequestContext,
                          const nsACString &aMimeTypeGuess,
                          nsISupports      *aExtra,
                          nsIPrincipal     *aRequestPrincipal,
                          int16_t          *aDecision)
{
  if (!aContentLocation)
    return NS_ERROR_FAILURE;

  *aDecision = nsIContentPolicy::ACCEPT;
  return NS_OK;
}


NS_IMETHODIMP
CSPService::AsyncOnChannelRedirect(nsIChannel *oldChannel,
                                   nsIChannel *newChannel,
                                   uint32_t flags,
                                   nsIAsyncVerifyRedirectCallback *callback)
{
  nsAsyncRedirectAutoCallback autoCallback(callback);

  nsCOMPtr<nsIURI> newUri;
  nsresult rv = newChannel->GetURI(getter_AddRefs(newUri));
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  if (!sCSPEnabled || !subjectToCSP(newUri)) {
    return NS_OK;
  }

  nsCOMPtr<nsILoadInfo> loadInfo;
  rv = oldChannel->GetLoadInfo(getter_AddRefs(loadInfo));

  
  if (!loadInfo) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = loadInfo->LoadingPrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!csp) {
    return NS_OK;
  }

  






  nsCOMPtr<nsIURI> originalUri;
  rv = oldChannel->GetOriginalURI(getter_AddRefs(originalUri));
  NS_ENSURE_SUCCESS(rv, rv);
  nsContentPolicyType policyType = loadInfo->GetContentPolicyType();

  int16_t aDecision = nsIContentPolicy::ACCEPT;
  csp->ShouldLoad(policyType,     
                  newUri,         
                  nullptr,        
                  nullptr,        
                  EmptyCString(), 
                  originalUri,    
                  &aDecision);

#ifdef PR_LOGGING
  if (newUri) {
    nsAutoCString newUriSpec("None");
    newUri->GetSpec(newUriSpec);
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::AsyncOnChannelRedirect called for %s",
            newUriSpec.get()));
  }
  if (aDecision == 1) {
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::AsyncOnChannelRedirect ALLOWING request."));
  }
  else {
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::AsyncOnChannelRedirect CANCELLING request."));
  }
#endif

  
  if (!NS_CP_ACCEPTED(aDecision)) {
    autoCallback.DontCallback();
    return NS_BINDING_FAILED;
  }
  return NS_OK;
}
