




#include "nsCOMPtr.h"
#include "nsContentPolicyUtils.h"
#include "nsContentUtils.h"
#include "nsCSPContext.h"
#include "nsCSPParser.h"
#include "nsCSPService.h"
#include "nsError.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIChannelPolicy.h"
#include "nsIClassInfoImpl.h"
#include "nsIDocShell.h"
#include "nsIDocShellTreeItem.h"
#include "nsIDOMHTMLDocument.h"
#include "nsIDOMHTMLElement.h"
#include "nsIHttpChannel.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPrincipal.h"
#include "nsIPropertyBag2.h"
#include "nsIScriptError.h"
#include "nsIWebNavigation.h"
#include "nsIWritablePropertyBag2.h"
#include "nsString.h"
#include "prlog.h"

using namespace mozilla;

#if defined(PR_LOGGING)
static PRLogModuleInfo *
GetCspContextLog()
{
  static PRLogModuleInfo *gCspContextPRLog;
  if (!gCspContextPRLog)
    gCspContextPRLog = PR_NewLogModule("CSPContext");
  return gCspContextPRLog;
}
#endif

#define CSPCONTEXTLOG(args) PR_LOG(GetCspContextLog(), 4, args)

static const uint32_t CSP_CACHE_URI_CUTOFF_SIZE = 512;





nsresult
CreateCacheKey_Internal(nsIURI* aContentLocation,
                        nsContentPolicyType aContentType,
                        nsACString& outCacheKey)
{
  if (!aContentLocation) {
    return NS_ERROR_FAILURE;
  }

  bool isDataScheme = false;
  nsresult rv = aContentLocation->SchemeIs("data", &isDataScheme);
  NS_ENSURE_SUCCESS(rv, rv);

  outCacheKey.Truncate();
  if (aContentType != nsIContentPolicy::TYPE_SCRIPT && isDataScheme) {
    
    outCacheKey.Append(NS_LITERAL_CSTRING("data:"));
    outCacheKey.AppendInt(aContentType);
    return NS_OK;
  }

  nsAutoCString spec;
  rv = aContentLocation->GetSpec(spec);
  NS_ENSURE_SUCCESS(rv, rv);

  
  if (spec.Length() <= CSP_CACHE_URI_CUTOFF_SIZE) {
    outCacheKey.Append(spec);
    outCacheKey.Append(NS_LITERAL_CSTRING("!"));
    outCacheKey.AppendInt(aContentType);
  }

  return NS_OK;
}



NS_IMETHODIMP
nsCSPContext::ShouldLoad(nsContentPolicyType aContentType,
                         nsIURI*             aContentLocation,
                         nsIURI*             aRequestOrigin,
                         nsISupports*        aRequestContext,
                         const nsACString&   aMimeTypeGuess,
                         nsISupports*        aExtra,
                         int16_t*            outDecision)
{
#ifdef PR_LOGGING
  {
  nsAutoCString spec;
  aContentLocation->GetSpec(spec);
  CSPCONTEXTLOG(("nsCSPContext::ShouldLoad, aContentLocation: %s", spec.get()));
  }
#endif

  nsresult rv = NS_OK;

  
  
  
  
  
  
  

  nsAutoCString cacheKey;
  rv = CreateCacheKey_Internal(aContentLocation, aContentType, cacheKey);
  NS_ENSURE_SUCCESS(rv, rv);

  bool isCached = mShouldLoadCache.Get(cacheKey, outDecision);
  if (isCached && cacheKey.Length() > 0) {
    
    return NS_OK;
  }

  
  *outDecision = nsIContentPolicy::ACCEPT;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  nsCOMPtr<nsIDOMHTMLDocument> doc = do_QueryInterface(aRequestContext);
  bool isPreload = doc &&
                   (aContentType == nsIContentPolicy::TYPE_SCRIPT ||
                    aContentType == nsIContentPolicy::TYPE_STYLESHEET);

  nsAutoString nonce;
  if (!isPreload) {
    nsCOMPtr<nsIDOMHTMLElement> htmlElement = do_QueryInterface(aRequestContext);
    if (htmlElement) {
      rv = htmlElement->GetAttribute(NS_LITERAL_STRING("nonce"), nonce);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  nsAutoString violatedDirective;
  for (uint32_t p = 0; p < mPolicies.Length(); p++) {
    if (!mPolicies[p]->permits(aContentType,
                               aContentLocation,
                               nonce,
                               violatedDirective)) {
      
      
      if (!mPolicies[p]->getReportOnlyFlag()) {
        CSPCONTEXTLOG(("nsCSPContext::ShouldLoad, nsIContentPolicy::REJECT_SERVER"));
        *outDecision = nsIContentPolicy::REJECT_SERVER;
      }

      
      
      
      if (!isPreload) {
        nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
        NS_ASSERTION(observerService, "CSP requires observer service.");

        observerService->NotifyObservers(aContentLocation,
                                         CSP_VIOLATION_TOPIC,
                                         violatedDirective.get());
      }

      
      
      
    }
  }
  
  if (cacheKey.Length() > 0 && !isPreload) {
    mShouldLoadCache.Put(cacheKey, *outDecision);
  }

#ifdef PR_LOGGING
  {
  nsAutoCString spec;
  aContentLocation->GetSpec(spec);
  CSPCONTEXTLOG(("nsCSPContext::ShouldLoad, decision: %s, aContentLocation: %s", *outDecision ? "load" : "deny", spec.get()));
  }
#endif
  return NS_OK;
}

NS_IMETHODIMP
nsCSPContext::ShouldProcess(nsContentPolicyType aContentType,
                            nsIURI*             aContentLocation,
                            nsIURI*             aRequestOrigin,
                            nsISupports*        aRequestContext,
                            const nsACString&   aMimeType,
                            nsISupports*        aExtra,
                            int16_t*            outDecision)
{
  *outDecision = nsIContentPolicy::ACCEPT;
  return NS_OK;
}



NS_IMPL_CLASSINFO(nsCSPContext,
                  nullptr,
                  nsIClassInfo::MAIN_THREAD_ONLY,
                  NS_CSPCONTEXT_CID)

NS_IMPL_ISUPPORTS_CI(nsCSPContext,
                     nsIContentSecurityPolicy,
                     nsISerializable)

nsCSPContext::nsCSPContext()
  : mSelfURI(nullptr)
{
  CSPCONTEXTLOG(("nsCSPContext::nsCSPContext"));
}

nsCSPContext::~nsCSPContext()
{
  CSPCONTEXTLOG(("nsCSPContext::~nsCSPContext"));
  for (uint32_t i = 0; i < mPolicies.Length(); i++) {
    delete mPolicies[i];
  }
  mShouldLoadCache.Clear();
}

NS_IMETHODIMP
nsCSPContext::GetIsInitialized(bool *outIsInitialized)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSPContext::GetPolicy(uint32_t aIndex, nsAString& outStr)
{
  if (aIndex >= mPolicies.Length()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  mPolicies[aIndex]->toString(outStr);
  return NS_OK;
}

NS_IMETHODIMP
nsCSPContext::GetPolicyCount(uint32_t *outPolicyCount)
{
  *outPolicyCount = mPolicies.Length();
  return NS_OK;
}

NS_IMETHODIMP
nsCSPContext::RemovePolicy(uint32_t aIndex)
{
  if (aIndex >= mPolicies.Length()) {
    return NS_ERROR_ILLEGAL_VALUE;
  }
  mPolicies.RemoveElementAt(aIndex);
  
  mShouldLoadCache.Clear();
  return NS_OK;
}

NS_IMETHODIMP
nsCSPContext::AppendPolicy(const nsAString& aPolicyString,
                           nsIURI* aSelfURI,
                           bool aReportOnly,
                           bool aSpecCompliant)
{
  CSPCONTEXTLOG(("nsCSPContext::AppendPolicy: %s",
                 NS_ConvertUTF16toUTF8(aPolicyString).get()));

  if (aSelfURI) {
    
    NS_WARNING("aSelfURI should be a nullptr in AppendPolicy and removed in bug 991474");
  }

  
  NS_ASSERTION(mSelfURI, "mSelfURI required for AppendPolicy, but not set");
  nsCSPPolicy* policy = nsCSPParser::parseContentSecurityPolicy(aPolicyString, mSelfURI, aReportOnly, 0);
  if (policy) {
    mPolicies.AppendElement(policy);
    
    mShouldLoadCache.Clear();
  }
  return NS_OK;
}



NS_IMETHODIMP
nsCSPContext::getAllowsInternal(nsContentPolicyType aContentType,
                                enum CSPKeyword aKeyword,
                                const nsAString& aNonceOrContent,
                                bool* outShouldReportViolation,
                                bool* outIsAllowed) const
{
  *outShouldReportViolation = false;
  *outIsAllowed = true;

  
  if (aKeyword == CSP_NONCE || aKeyword == CSP_HASH) {
    if (!(aContentType == nsIContentPolicy::TYPE_SCRIPT ||
          aContentType == nsIContentPolicy::TYPE_STYLESHEET)) {
      *outIsAllowed = false;
      return NS_OK;
    }
  }

  for (uint32_t i = 0; i < mPolicies.Length(); i++) {
    if (!mPolicies[i]->allows(aContentType,
                              aKeyword,
                              aNonceOrContent)) {
      
      
      *outShouldReportViolation = true;
      if (!mPolicies[i]->getReportOnlyFlag()) {
        *outIsAllowed = false;
      }
    }
  }
  CSPCONTEXTLOG(("nsCSPContext::getAllowsInternal, aContentType: %d, aKeyword: %s, aNonceOrContent: %s, isAllowed: %s",
                aContentType,
                aKeyword == CSP_HASH ? "hash" : CSP_EnumToKeyword(aKeyword),
                NS_ConvertUTF16toUTF8(aNonceOrContent).get(),
                *outIsAllowed ? "load" : "deny"));
  return NS_OK;
}

NS_IMETHODIMP
nsCSPContext::GetAllowsInlineScript(bool* outShouldReportViolation,
                                    bool* outAllowsInlineScript)
{
  return getAllowsInternal(nsIContentPolicy::TYPE_SCRIPT,
                           CSP_UNSAFE_INLINE,
                           EmptyString(),
                           outShouldReportViolation,
                           outAllowsInlineScript);
}

NS_IMETHODIMP
nsCSPContext::GetAllowsEval(bool* outShouldReportViolation,
                            bool* outAllowsEval)
{
  return getAllowsInternal(nsIContentPolicy::TYPE_SCRIPT,
                           CSP_UNSAFE_EVAL,
                           EmptyString(),
                           outShouldReportViolation,
                           outAllowsEval);
}

NS_IMETHODIMP
nsCSPContext::GetAllowsInlineStyle(bool* outShouldReportViolation,
                                   bool* outAllowsInlineStyle)
{
  return getAllowsInternal(nsIContentPolicy::TYPE_STYLESHEET,
                           CSP_UNSAFE_INLINE,
                           EmptyString(),
                           outShouldReportViolation,
                           outAllowsInlineStyle);
}

NS_IMETHODIMP
nsCSPContext::GetAllowsNonce(const nsAString& aNonce,
                             uint32_t aContentType,
                             bool* outShouldReportViolation,
                             bool* outAllowsNonce)
{
  return getAllowsInternal(aContentType,
                           CSP_NONCE,
                           aNonce,
                           outShouldReportViolation,
                           outAllowsNonce);
}

NS_IMETHODIMP
nsCSPContext::GetAllowsHash(const nsAString& aContent,
                            uint16_t aContentType,
                            bool* outShouldReportViolation,
                            bool* outAllowsHash)
{
  return getAllowsInternal(aContentType,
                           CSP_HASH,
                           aContent,
                           outShouldReportViolation,
                           outAllowsHash);
}

NS_IMETHODIMP
nsCSPContext::LogViolationDetails(uint16_t aViolationType,
                                  const nsAString& aSourceFile,
                                  const nsAString& aScriptSample,
                                  int32_t aLineNum,
                                  const nsAString& aNonce,
                                  const nsAString& aContent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsCSPContext::SetRequestContext(nsIURI* aSelfURI,
                                nsIURI* aReferrer,
                                nsIPrincipal* aDocumentPrincipal,
                                nsIChannel* aChannel)
{
  if (!aSelfURI && !aChannel) {
    CSPCONTEXTLOG(("nsCSPContext::SetRequestContext: !selfURI && !aChannel provided"));
    return NS_ERROR_FAILURE;
  }

  mSelfURI = aSelfURI;

  if (!mSelfURI) {
    nsresult rv = aChannel->GetURI(getter_AddRefs(mSelfURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(mSelfURI, "No mSelfURI in SetRequestContext, can not translate 'self' into actual URI");

  
  

  return NS_OK;
}


















NS_IMETHODIMP
nsCSPContext::PermitsAncestry(nsIDocShell* aDocShell, bool* outPermitsAncestry)
{
  nsresult rv;

  
  if (aDocShell == nullptr) {
    return NS_ERROR_FAILURE;
  }

  *outPermitsAncestry = true;

  
  nsCOMArray<nsIURI> ancestorsArray;

  nsCOMPtr<nsIInterfaceRequestor> ir(do_QueryInterface(aDocShell));
  nsCOMPtr<nsIDocShellTreeItem> treeItem(do_GetInterface(ir));
  nsCOMPtr<nsIDocShellTreeItem> parentTreeItem;
  nsCOMPtr<nsIWebNavigation> webNav;
  nsCOMPtr<nsIURI> currentURI;
  nsCOMPtr<nsIURI> uriClone;

  
  while (NS_SUCCEEDED(treeItem->GetParent(getter_AddRefs(parentTreeItem))) &&
         parentTreeItem != nullptr) {
    ir     = do_QueryInterface(parentTreeItem);
    NS_ASSERTION(ir, "Could not QI docShellTreeItem to nsIInterfaceRequestor");

    webNav = do_GetInterface(ir);
    NS_ENSURE_TRUE(webNav, NS_ERROR_FAILURE);

    rv = webNav->GetCurrentURI(getter_AddRefs(currentURI));
    NS_ENSURE_SUCCESS(rv, rv);

    if (currentURI) {
      
      bool isChrome = false;
      rv = currentURI->SchemeIs("chrome", &isChrome);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isChrome) { break; }

      
      rv = currentURI->CloneIgnoringRef(getter_AddRefs(uriClone));
      NS_ENSURE_SUCCESS(rv, rv);
      rv = uriClone->SetUserPass(EmptyCString());
      NS_ENSURE_SUCCESS(rv, rv);
#ifdef PR_LOGGING
      {
      nsAutoCString spec;
      uriClone->GetSpec(spec);
      CSPCONTEXTLOG(("nsCSPContext::PermitsAncestry, found ancestor: %s", spec.get()));
      }
#endif
      ancestorsArray.AppendElement(uriClone);
    }

    
    treeItem = parentTreeItem;
  }

  nsAutoString violatedDirective;

  
  
  for (uint32_t i = 0; i < mPolicies.Length(); i++) {
    for (uint32_t a = 0; a < ancestorsArray.Length(); a++) {
      
      
      
#ifdef PR_LOGGING
      {
      nsAutoCString spec;
      ancestorsArray[a]->GetSpec(spec);
      CSPCONTEXTLOG(("nsCSPContext::PermitsAncestry, checking ancestor: %s", spec.get()));
      }
#endif
      if (!mPolicies[i]->permits(nsIContentPolicy::TYPE_DOCUMENT,
                                 ancestorsArray[a],
                                 EmptyString(), 
                                 violatedDirective)) {
        
        nsCOMPtr<nsIObserverService> observerService =
          mozilla::services::GetObserverService();
        NS_ENSURE_TRUE(observerService, NS_ERROR_NOT_AVAILABLE);

        observerService->NotifyObservers(ancestorsArray[a],
                                         CSP_VIOLATION_TOPIC,
                                         violatedDirective.get());
        
        
        
        *outPermitsAncestry = false;
      }
    }
  }
  return NS_OK;
}



NS_IMETHODIMP
nsCSPContext::Read(nsIObjectInputStream* aStream)
{
  nsresult rv;
  nsCOMPtr<nsISupports> supports;

  rv = NS_ReadOptionalObject(aStream, true, getter_AddRefs(supports));
  NS_ENSURE_SUCCESS(rv, rv);

  mSelfURI = do_QueryInterface(supports);
  NS_ASSERTION(mSelfURI, "need a self URI to de-serialize");

  uint32_t numPolicies;
  rv = aStream->Read32(&numPolicies);
  NS_ENSURE_SUCCESS(rv, rv);

  nsAutoString policyString;

  while (numPolicies > 0) {
    numPolicies--;

    rv = aStream->ReadString(policyString);
    NS_ENSURE_SUCCESS(rv, rv);

    bool reportOnly = false;
    rv = aStream->ReadBoolean(&reportOnly);
    NS_ENSURE_SUCCESS(rv, rv);

    bool specCompliant = false;
    rv = aStream->ReadBoolean(&specCompliant);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    if (!specCompliant) {
      continue;
    }

    nsCSPPolicy* policy = nsCSPParser::parseContentSecurityPolicy(policyString,
                                                                  mSelfURI,
                                                                  reportOnly,
                                                                  0);
    if (policy) {
      mPolicies.AppendElement(policy);
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsCSPContext::Write(nsIObjectOutputStream* aStream)
{
  nsresult rv = NS_WriteOptionalCompoundObject(aStream,
                                               mSelfURI,
                                               NS_GET_IID(nsIURI),
                                               true);
  NS_ENSURE_SUCCESS(rv, rv);

  
  aStream->Write32(mPolicies.Length());

  nsAutoString polStr;
  for (uint32_t p = 0; p < mPolicies.Length(); p++) {
    mPolicies[p]->toString(polStr);
    aStream->WriteWStringZ(polStr.get());
    aStream->WriteBoolean(mPolicies[p]->getReportOnlyFlag());
    
    aStream->WriteBoolean(true);
  }
  return NS_OK;
}
