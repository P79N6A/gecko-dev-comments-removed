




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
  if (!aContentLocation)
    return NS_ERROR_FAILURE;

#ifdef PR_LOGGING
  {
    nsAutoCString location;
    aContentLocation->GetSpec(location);
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::ShouldLoad called for %s", location.get()));
  }
#endif

  
  *aDecision = nsIContentPolicy::ACCEPT;

  
  if (!sCSPEnabled)
    return NS_OK;

  
  
  bool schemeMatch = false;
  NS_ENSURE_SUCCESS(aContentLocation->SchemeIs("about", &schemeMatch), NS_OK);
  if (schemeMatch)
    return NS_OK;
  NS_ENSURE_SUCCESS(aContentLocation->SchemeIs("chrome", &schemeMatch), NS_OK);
  if (schemeMatch)
    return NS_OK;
  NS_ENSURE_SUCCESS(aContentLocation->SchemeIs("resource", &schemeMatch), NS_OK);
  if (schemeMatch)
    return NS_OK;
  NS_ENSURE_SUCCESS(aContentLocation->SchemeIs("javascript", &schemeMatch), NS_OK);
  if (schemeMatch)
    return NS_OK;


  
  
  
  
  if (aContentType == nsIContentPolicy::TYPE_CSP_REPORT ||
    aContentType == nsIContentPolicy::TYPE_REFRESH ||
    aContentType == nsIContentPolicy::TYPE_DOCUMENT) {
    return NS_OK;
  }

  
  

  
  uint16_t status = nsIPrincipal::APP_STATUS_NOT_INSTALLED;
  nsAutoCString contentOrigin;
  aContentLocation->GetPrePath(contentOrigin);
  if (aRequestPrincipal && !mAppStatusCache.Get(contentOrigin, &status)) {
    aRequestPrincipal->GetAppStatus(&status);
    mAppStatusCache.Put(contentOrigin, status);
  }

  if (status == nsIPrincipal::APP_STATUS_CERTIFIED) {
    
    
    
    
    
    
    

    switch (aContentType) {
      case nsIContentPolicy::TYPE_SCRIPT:
      case nsIContentPolicy::TYPE_STYLESHEET:
        {
          
          auto themeOrigin = Preferences::GetCString("b2g.theme.origin");
          nsAutoCString sourceOrigin;
          aRequestOrigin->GetPrePath(sourceOrigin);

          if (!(sourceOrigin.Equals(contentOrigin) ||
                (themeOrigin && themeOrigin.Equals(contentOrigin)))) {
            *aDecision = nsIContentPolicy::REJECT_SERVER;
          }
        }
        break;

      case nsIContentPolicy::TYPE_OBJECT:
        *aDecision = nsIContentPolicy::REJECT_SERVER;
        break;

      default:
        *aDecision = nsIContentPolicy::ACCEPT;
    }

    
    
    if (*aDecision == nsIContentPolicy::ACCEPT) {
      return NS_OK;
    }
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

  nsCOMPtr<nsILoadInfo> loadInfo;
  nsresult rv = oldChannel->GetLoadInfo(getter_AddRefs(loadInfo));

  
  if (!loadInfo) {
    return NS_OK;
  }

  
  
  
  

  nsCOMPtr<nsINode> loadingNode = loadInfo->LoadingNode();
  nsCOMPtr<nsIPrincipal> principal = loadingNode ?
                                     loadingNode->NodePrincipal() :
                                     loadInfo->LoadingPrincipal();
  NS_ASSERTION(principal, "Can not evaluate CSP without a principal");
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = principal->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (!csp) {
    return NS_OK;
  }

  







  nsCOMPtr<nsIURI> newUri;
  rv = newChannel->GetURI(getter_AddRefs(newUri));
  NS_ENSURE_SUCCESS(rv, rv);
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
  if (aDecision == 1)
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::AsyncOnChannelRedirect ALLOWING request."));
  else
    PR_LOG(gCspPRLog, PR_LOG_DEBUG,
           ("CSPService::AsyncOnChannelRedirect CANCELLING request."));
#endif

  
  if (!NS_CP_ACCEPTED(aDecision)) {
    autoCallback.DontCallback();
    return NS_BINDING_FAILED;
  }
  return NS_OK;
}
