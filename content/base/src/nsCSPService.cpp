




#include "prlog.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsIURI.h"
#include "nsIPrincipal.h"
#include "nsIObserver.h"
#include "nsIContent.h"
#include "nsCSPService.h"
#include "nsIContentSecurityPolicy.h"
#include "nsIChannelPolicy.h"
#include "nsIChannelEventSink.h"
#include "nsIPropertyBag2.h"
#include "nsIWritablePropertyBag2.h"
#include "nsError.h"
#include "nsChannelProperties.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "mozilla/Preferences.h"
#include "nsIScriptError.h"
#include "nsContentUtils.h"
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

  nsCOMPtr<nsISupports> supports;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  channelPolicy->GetContentSecurityPolicy(getter_AddRefs(supports));
  csp = do_QueryInterface(supports);
  uint32_t loadType;
  channelPolicy->GetLoadType(&loadType);

  
  if (!csp)
    return NS_OK;

  







  
  
  nsCOMPtr<nsIURI> newUri;
  newChannel->GetURI(getter_AddRefs(newUri));
  nsCOMPtr<nsIURI> originalUri;
  oldChannel->GetOriginalURI(getter_AddRefs(originalUri));
  int16_t aDecision = nsIContentPolicy::ACCEPT;
  csp->ShouldLoad(loadType,        
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

  
  if (aDecision != 1) {
    autoCallback.DontCallback();
    return NS_BINDING_FAILED;
  }

  
  
  nsresult rv;
  nsCOMPtr<nsIWritablePropertyBag2> props2 = do_QueryInterface(newChannel);
  if (props2) {
    rv = props2->SetPropertyAsInterface(NS_CHANNEL_PROP_CHANNEL_POLICY,
                                        channelPolicy);
    if (NS_SUCCEEDED(rv)) {
      return NS_OK;
    }
  }

  
  
  nsAutoCString newUriSpec;
  rv = newUri->GetSpec(newUriSpec);
  NS_ConvertUTF8toUTF16 unicodeSpec(newUriSpec);
  const char16_t *formatParams[] = { unicodeSpec.get() };
  if (NS_SUCCEEDED(rv)) {
    nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                    NS_LITERAL_CSTRING("Redirect Error"), nullptr,
                                    nsContentUtils::eDOM_PROPERTIES,
                                    "InvalidRedirectChannelWarning",
                                    formatParams, 1);
  }

  return NS_BINDING_FAILED;
}
