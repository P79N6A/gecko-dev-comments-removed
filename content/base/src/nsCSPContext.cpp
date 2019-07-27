




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
#include "nsIDOMWindowUtils.h"
#include "nsIHttpChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObjectInputStream.h"
#include "nsIObjectOutputStream.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIPropertyBag2.h"
#include "nsIStringStream.h"
#include "nsIUploadChannel.h"
#include "nsIScriptError.h"
#include "nsIWebNavigation.h"
#include "nsIWritablePropertyBag2.h"
#include "nsNetUtil.h"
#include "nsSupportsPrimitives.h"
#include "nsThreadUtils.h"
#include "nsString.h"
#include "prlog.h"
#include "mozilla/dom/CSPReportBinding.h"

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
        this->AsyncReportViolation(aContentLocation,
                                   mSelfURI,
                                   violatedDirective,
                                   p,             
                                   EmptyString(), 
                                   EmptyString(), 
                                   EmptyString(), 
                                   0);            
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
                           bool aReportOnly)
{
  CSPCONTEXTLOG(("nsCSPContext::AppendPolicy: %s",
                 NS_ConvertUTF16toUTF8(aPolicyString).get()));

  
  NS_ASSERTION(mSelfURI, "mSelfURI required for AppendPolicy, but not set");
  nsCSPPolicy* policy = nsCSPParser::parseContentSecurityPolicy(aPolicyString, mSelfURI, aReportOnly, mInnerWindowID);
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
























#define CASE_CHECK_AND_REPORT(violationType, contentPolicyType, nonceOrHash,   \
                              keyword, observerTopic)                          \
  case nsIContentSecurityPolicy::VIOLATION_TYPE_ ## violationType :            \
    PR_BEGIN_MACRO                                                             \
    if (!mPolicies[p]->allows(nsIContentPolicy::TYPE_ ## contentPolicyType,    \
                              keyword, nonceOrHash))                           \
    {                                                                          \
      nsAutoString violatedDirective;                                          \
      mPolicies[p]->getDirectiveStringForContentType(                          \
                        nsIContentPolicy::TYPE_ ## contentPolicyType,          \
                        violatedDirective);                                    \
      this->AsyncReportViolation(selfISupports, mSelfURI, violatedDirective, p, \
                                 NS_LITERAL_STRING(observerTopic),             \
                                 aSourceFile, aScriptSample, aLineNum);        \
    }                                                                          \
    PR_END_MACRO;                                                              \
    break























NS_IMETHODIMP
nsCSPContext::LogViolationDetails(uint16_t aViolationType,
                                  const nsAString& aSourceFile,
                                  const nsAString& aScriptSample,
                                  int32_t aLineNum,
                                  const nsAString& aNonce,
                                  const nsAString& aContent)
{
  for (uint32_t p = 0; p < mPolicies.Length(); p++) {
    NS_ASSERTION(mPolicies[p], "null pointer in nsTArray<nsCSPPolicy>");

    nsCOMPtr<nsISupportsCString> selfICString(do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID));
    if (selfICString) {
      selfICString->SetData(nsDependentCString("self"));
    }
    nsCOMPtr<nsISupports> selfISupports(do_QueryInterface(selfICString));

    switch (aViolationType) {
      CASE_CHECK_AND_REPORT(EVAL,              SCRIPT,     NS_LITERAL_STRING(""),
                            CSP_UNSAFE_EVAL,   EVAL_VIOLATION_OBSERVER_TOPIC);
      CASE_CHECK_AND_REPORT(INLINE_STYLE,      STYLESHEET, NS_LITERAL_STRING(""),
                            CSP_UNSAFE_INLINE, INLINE_STYLE_VIOLATION_OBSERVER_TOPIC);
      CASE_CHECK_AND_REPORT(INLINE_SCRIPT,     SCRIPT,     NS_LITERAL_STRING(""),
                            CSP_UNSAFE_INLINE, INLINE_SCRIPT_VIOLATION_OBSERVER_TOPIC);
      CASE_CHECK_AND_REPORT(NONCE_SCRIPT,      SCRIPT,     aNonce,
                            CSP_UNSAFE_INLINE, SCRIPT_NONCE_VIOLATION_OBSERVER_TOPIC);
      CASE_CHECK_AND_REPORT(NONCE_STYLE,       STYLESHEET, aNonce,
                            CSP_UNSAFE_INLINE, STYLE_NONCE_VIOLATION_OBSERVER_TOPIC);
      CASE_CHECK_AND_REPORT(HASH_SCRIPT,       SCRIPT,     aContent,
                            CSP_UNSAFE_INLINE, SCRIPT_HASH_VIOLATION_OBSERVER_TOPIC);
      CASE_CHECK_AND_REPORT(HASH_STYLE,        STYLESHEET, aContent,
                            CSP_UNSAFE_INLINE, STYLE_HASH_VIOLATION_OBSERVER_TOPIC);

      default:
        NS_ASSERTION(false, "LogViolationDetails with invalid type");
        break;
    }
  }
  return NS_OK;
}

#undef CASE_CHECK_AND_REPORT

uint64_t
getInnerWindowID(nsIRequest* aRequest) {
  
  if (!aRequest) {
    return 0;
  }

  nsCOMPtr<nsILoadGroup> loadGroup;
  nsresult rv = aRequest->GetLoadGroup(getter_AddRefs(loadGroup));

  if (NS_FAILED(rv) || !loadGroup) {
    return 0;
  }

  nsCOMPtr<nsIInterfaceRequestor> callbacks;
  rv = loadGroup->GetNotificationCallbacks(getter_AddRefs(callbacks));
  if (NS_FAILED(rv) || !callbacks) {
    return 0;
  }

  nsCOMPtr<nsILoadContext> loadContext = do_GetInterface(callbacks);
  if (!loadContext) {
    return 0;
  }

  nsCOMPtr<nsIDOMWindow> window;
  rv = loadContext->GetAssociatedWindow(getter_AddRefs(window));
  if (NS_FAILED(rv) || !window) {
    return 0;
  }

  nsCOMPtr<nsPIDOMWindow> pwindow = do_QueryInterface(window);
  if (!pwindow) {
    return 0;
  }

  nsPIDOMWindow* inner = pwindow->IsInnerWindow() ? pwindow.get() : pwindow->GetCurrentInnerWindow();

  return inner ? inner->WindowID() : 0;
}

NS_IMETHODIMP
nsCSPContext::SetRequestContext(nsIURI* aSelfURI,
                                nsIURI* aReferrer,
                                nsIChannel* aChannel)
{
  NS_PRECONDITION(aSelfURI || aChannel, "Need aSelfURI or aChannel to set the context properly");
  NS_ENSURE_ARG(aSelfURI || aChannel);

  
  mSelfURI = aSelfURI;
  if (!mSelfURI) {
    nsresult rv = aChannel->GetURI(getter_AddRefs(mSelfURI));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  NS_ASSERTION(mSelfURI, "No aSelfURI and no URI available from channel in SetRequestContext, can not translate 'self' into actual URI");

  if (aChannel) {
    mInnerWindowID = getInnerWindowID(aChannel);
    aChannel->GetLoadGroup(getter_AddRefs(mCallingChannelLoadGroup));
  }
  else {
    NS_WARNING("Channel needed (but null) in SetRequestContext.  Cannot query loadgroup, which means report sending may fail.");
  }

  mReferrer = aReferrer;
  if (!mReferrer) {
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(aChannel));
    if (httpChannel) {
      httpChannel->GetReferrer(getter_AddRefs(mReferrer));
    }
    else {
      NS_WARNING("Channel provided to SetRequestContext is not an nsIHttpChannel so referrer is not available for reporting." );
    }
  }

  return NS_OK;
}

nsresult
nsCSPContext::SendReports(nsISupports* aBlockedContentSource,
                          nsIURI* aOriginalURI,
                          nsAString& aViolatedDirective,
                          uint32_t aViolatedPolicyIndex,
                          nsAString& aSourceFile,
                          nsAString& aScriptSample,
                          uint32_t aLineNum)
{
  NS_ENSURE_ARG_MAX(aViolatedPolicyIndex, mPolicies.Length() - 1);

#ifdef MOZ_B2G
  
  
  if (!mCallingChannelLoadGroup) {
    NS_WARNING("Load group required but not present for report sending; cannot send CSP violation reports");
    return NS_ERROR_FAILURE;
  }
#endif

  dom::CSPReport report;
  nsresult rv;

  
  if (aBlockedContentSource) {
    nsAutoCString reportBlockedURI;
    nsCOMPtr<nsIURI> uri = do_QueryInterface(aBlockedContentSource);
    
    if (uri) {
      uri->GetSpecIgnoringRef(reportBlockedURI);
    } else {
      nsCOMPtr<nsISupportsCString> cstr = do_QueryInterface(aBlockedContentSource);
      if (cstr) {
        cstr->GetData(reportBlockedURI);
      }
    }
    if (reportBlockedURI.IsEmpty()) {
      
      
      NS_WARNING("No blocked URI (null aBlockedContentSource) for CSP violation report.");
    }
    report.mCsp_report.mBlocked_uri = NS_ConvertUTF8toUTF16(reportBlockedURI);
  }

  
  if (aOriginalURI) {
    nsAutoCString reportDocumentURI;
    aOriginalURI->GetSpecIgnoringRef(reportDocumentURI);
    report.mCsp_report.mDocument_uri = NS_ConvertUTF8toUTF16(reportDocumentURI);
  }

  
  nsAutoString originalPolicy;
  rv = this->GetPolicy(aViolatedPolicyIndex, originalPolicy);
  NS_ENSURE_SUCCESS(rv, rv);
  report.mCsp_report.mOriginal_policy = originalPolicy;

  
  if (mReferrer) {
    nsAutoCString referrerURI;
    mReferrer->GetSpec(referrerURI);
    report.mCsp_report.mReferrer = NS_ConvertUTF8toUTF16(referrerURI);
  }

  
  report.mCsp_report.mViolated_directive = aViolatedDirective;

  
  if (!aSourceFile.IsEmpty()) {
    
    nsCOMPtr<nsIURI> sourceURI;
    NS_NewURI(getter_AddRefs(sourceURI), aSourceFile);
    if (sourceURI) {
      nsAutoCString spec;
      sourceURI->GetSpecIgnoringRef(spec);
      aSourceFile = NS_ConvertUTF8toUTF16(spec);
    }
    report.mCsp_report.mSource_file.Construct();
    report.mCsp_report.mSource_file.Value() = aSourceFile;
  }

  
  if (!aScriptSample.IsEmpty()) {
    report.mCsp_report.mScript_sample.Construct();
    report.mCsp_report.mScript_sample.Value() = aScriptSample;
  }

  
  if (aLineNum != 0) {
    report.mCsp_report.mLine_number.Construct();
    report.mCsp_report.mLine_number.Value() = aLineNum;
  }

  nsString csp_report;
  if (!report.ToJSON(csp_report)) {
    return NS_ERROR_FAILURE;
  }

  

  nsTArray<nsString> reportURIs;
  mPolicies[aViolatedPolicyIndex]->getReportURIs(reportURIs);

  nsCOMPtr<nsIURI> reportURI;
  nsCOMPtr<nsIChannel> reportChannel;

  for (uint32_t r = 0; r < reportURIs.Length(); r++) {
    nsAutoCString reportURICstring = NS_ConvertUTF16toUTF8(reportURIs[r]);
    
    rv = NS_NewURI(getter_AddRefs(reportURI), reportURIs[r]);
    if (NS_FAILED(rv)) {
      const char16_t* params[] = { reportURIs[r].get() };
      CSPCONTEXTLOG(("Could not create nsIURI for report URI %s",
                     reportURICstring.get()));
      CSP_LogLocalizedStr(NS_LITERAL_STRING("triedToSendReport").get(),
                          params, ArrayLength(params),
                          aSourceFile, aScriptSample, aLineNum, 0,
                          nsIScriptError::errorFlag, "CSP", mInnerWindowID);
      continue; 
    }

    
    rv = NS_NewChannel(getter_AddRefs(reportChannel), reportURI);
    if (NS_FAILED(rv)) {
      CSPCONTEXTLOG(("Could not create new channel for report URI %s",
                     reportURICstring.get()));
      continue; 
    }

    
    
    nsLoadFlags flags;
    rv = reportChannel->GetLoadFlags(&flags);
    NS_ENSURE_SUCCESS(rv, rv);
    flags |= nsIRequest::LOAD_ANONYMOUS;
    rv = reportChannel->SetLoadFlags(flags);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    nsRefPtr<CSPReportRedirectSink> reportSink = new CSPReportRedirectSink();
    reportChannel->SetNotificationCallbacks(reportSink);

    
    
    
    rv = reportChannel->SetLoadGroup(mCallingChannelLoadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    
    int16_t shouldLoad = nsIContentPolicy::ACCEPT;
    nsCOMPtr<nsIContentPolicy> cp = do_GetService(NS_CONTENTPOLICY_CONTRACTID);
    if (!cp) {
      return NS_ERROR_FAILURE;
    }

    rv = cp->ShouldLoad(nsIContentPolicy::TYPE_CSP_REPORT,
                        reportURI,
                        aOriginalURI,
                        nullptr,        
                        EmptyCString(), 
                        nullptr,        
                        nullptr,        
                        &shouldLoad);

    
    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_CP_REJECTED(shouldLoad)) {
      
      CSPCONTEXTLOG(("nsIContentPolicy blocked sending report to %s",
                     reportURICstring.get()));
      continue; 
    }

    
    nsCOMPtr<nsIStringInputStream> sis(do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID));
    NS_ASSERTION(sis, "nsIStringInputStream is needed but not available to send CSP violation reports");
    rv = sis->SetData(NS_ConvertUTF16toUTF8(csp_report).get(), csp_report.Length());
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIUploadChannel> uploadChannel(do_QueryInterface(reportChannel));
    NS_ASSERTION(uploadChannel, "nsIUploadChannel is needed but not available to send CSP violation reports");
    rv = uploadChannel->SetUploadStream(sis, NS_LITERAL_CSTRING("application/json"), -1);
    NS_ENSURE_SUCCESS(rv, rv);

    
    nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(reportChannel));
    if (httpChannel) {
      httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("POST"));
    }

    nsRefPtr<CSPViolationReportListener> listener = new CSPViolationReportListener();
    rv = reportChannel->AsyncOpen(listener, nullptr);

    
    
    
    

    if (NS_FAILED(rv)) {
      const char16_t* params[] = { reportURIs[r].get() };
      CSPCONTEXTLOG(("AsyncOpen failed for report URI %s", params[0]));
      CSP_LogLocalizedStr(NS_LITERAL_STRING("triedToSendReport").get(),
                          params, ArrayLength(params),
                          aSourceFile, aScriptSample, aLineNum, 0,
                          nsIScriptError::errorFlag, "CSP", mInnerWindowID);
    } else {
      CSPCONTEXTLOG(("Sent violation report to URI %s", reportURICstring.get()));
    }
  }
  return NS_OK;
}




class CSPReportSenderRunnable MOZ_FINAL : public nsRunnable
{
  public:
    CSPReportSenderRunnable(nsISupports* aBlockedContentSource,
                            nsIURI* aOriginalURI,
                            uint32_t aViolatedPolicyIndex,
                            bool aReportOnlyFlag,
                            const nsAString& aViolatedDirective,
                            const nsAString& aObserverSubject,
                            const nsAString& aSourceFile,
                            const nsAString& aScriptSample,
                            uint32_t aLineNum,
                            uint64_t aInnerWindowID,
                            nsCSPContext* aCSPContext)
      : mBlockedContentSource(aBlockedContentSource)
      , mOriginalURI(aOriginalURI) , mViolatedPolicyIndex(aViolatedPolicyIndex)
      , mReportOnlyFlag(aReportOnlyFlag)
      , mViolatedDirective(aViolatedDirective)
      , mSourceFile(aSourceFile)
      , mScriptSample(aScriptSample)
      , mLineNum(aLineNum)
      , mInnerWindowID(aInnerWindowID)
      , mCSPContext(aCSPContext)
    {
      
      
      
      if (aObserverSubject.IsEmpty()) {
        mObserverSubject = aBlockedContentSource;
      } else {
        nsCOMPtr<nsISupportsCString> supportscstr =
          do_CreateInstance(NS_SUPPORTS_CSTRING_CONTRACTID);
        NS_ASSERTION(supportscstr, "Couldn't allocate nsISupportsCString");
        supportscstr->SetData(NS_ConvertUTF16toUTF8(aObserverSubject));
        mObserverSubject = do_QueryInterface(supportscstr);
      }
    }

    NS_IMETHOD Run()
    {
      MOZ_ASSERT(NS_IsMainThread());

      
      nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
      NS_ASSERTION(observerService, "needs observer service");
      nsresult rv = observerService->NotifyObservers(mObserverSubject,
                                                     CSP_VIOLATION_TOPIC,
                                                     mViolatedDirective.get());
      NS_ENSURE_SUCCESS(rv, rv);

      
      mCSPContext->SendReports(mBlockedContentSource, mOriginalURI,
                               mViolatedDirective, mViolatedPolicyIndex,
                               mSourceFile, mScriptSample, mLineNum);

      
      
      nsCOMPtr<nsIURI> blockedURI = do_QueryInterface(mBlockedContentSource);
      
      nsCOMPtr<nsISupportsCString> blockedString = do_QueryInterface(mBlockedContentSource);

      nsCString blockedDataStr;

      if (blockedURI) {
        blockedURI->GetSpec(blockedDataStr);
      } else if (blockedString) {
        blockedString->GetData(blockedDataStr);
      }

      if (blockedDataStr.Length() > 0) {
        nsString blockedDataChar16 = NS_ConvertUTF8toUTF16(blockedDataStr);
        const char16_t* params[] = { mViolatedDirective.get(),
                                     blockedDataChar16.get() };

        CSP_LogLocalizedStr(mReportOnlyFlag ? NS_LITERAL_STRING("CSPROViolationWithURI").get() :
                                              NS_LITERAL_STRING("CSPViolationWithURI").get(),
                            params, ArrayLength(params),
                            mSourceFile, mScriptSample, mLineNum, 0,
                            nsIScriptError::errorFlag, "CSP", mInnerWindowID);
      }
      return NS_OK;
    }

  private:
    nsCOMPtr<nsISupports>   mBlockedContentSource;
    nsCOMPtr<nsIURI>        mOriginalURI;
    uint32_t                mViolatedPolicyIndex;
    bool                    mReportOnlyFlag;
    nsString                mViolatedDirective;
    nsCOMPtr<nsISupports>   mObserverSubject;
    nsString                mSourceFile;
    nsString                mScriptSample;
    uint32_t                mLineNum;
    uint64_t                mInnerWindowID;
    nsCSPContext*           mCSPContext;
};


























nsresult
nsCSPContext::AsyncReportViolation(nsISupports* aBlockedContentSource,
                                   nsIURI* aOriginalURI,
                                   const nsAString& aViolatedDirective,
                                   uint32_t aViolatedPolicyIndex,
                                   const nsAString& aObserverSubject,
                                   const nsAString& aSourceFile,
                                   const nsAString& aScriptSample,
                                   uint32_t aLineNum)
{
  NS_ENSURE_ARG_MAX(aViolatedPolicyIndex, mPolicies.Length() - 1);

  NS_DispatchToMainThread(new CSPReportSenderRunnable(aBlockedContentSource,
                                                      aOriginalURI,
                                                      aViolatedPolicyIndex,
                                                      mPolicies[aViolatedPolicyIndex]->getReportOnlyFlag(),
                                                      aViolatedDirective,
                                                      aObserverSubject,
                                                      aSourceFile,
                                                      aScriptSample,
                                                      aLineNum,
                                                      mInnerWindowID,
                                                      this));
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
  nsCOMPtr<nsIURI> currentURI;
  nsCOMPtr<nsIURI> uriClone;

  
  while (NS_SUCCEEDED(treeItem->GetParent(getter_AddRefs(parentTreeItem))) &&
         parentTreeItem != nullptr) {

    nsIDocument* doc = parentTreeItem->GetDocument();
    NS_ASSERTION(doc, "Could not get nsIDocument from nsIDocShellTreeItem in nsCSPContext::PermitsAncestry");
    NS_ENSURE_TRUE(doc, NS_ERROR_FAILURE);

    currentURI = doc->GetDocumentURI();

    if (currentURI) {
      
      bool isChrome = false;
      rv = currentURI->SchemeIs("chrome", &isChrome);
      NS_ENSURE_SUCCESS(rv, rv);
      if (isChrome) { break; }

      
      rv = currentURI->CloneIgnoringRef(getter_AddRefs(uriClone));
      NS_ENSURE_SUCCESS(rv, rv);

      
      
      uriClone->SetUserPass(EmptyCString());

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

    
    
    if (mPolicies[i]->getReportOnlyFlag()) {
      continue;
    }

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
        
        
        
        bool okToSendAncestor = NS_SecurityCompareURIs(ancestorsArray[a], mSelfURI, true);

        this->AsyncReportViolation((okToSendAncestor ? ancestorsArray[a] : nullptr),
                                   mSelfURI,
                                   violatedDirective,
                                   i,             
                                   EmptyString(), 
                                   EmptyString(), 
                                   EmptyString(), 
                                   0);            
        *outPermitsAncestry = false;
      }
    }
  }
  return NS_OK;
}



NS_IMPL_ISUPPORTS(CSPViolationReportListener, nsIStreamListener, nsIRequestObserver, nsISupports);

CSPViolationReportListener::CSPViolationReportListener()
{
}

CSPViolationReportListener::~CSPViolationReportListener()
{
}

NS_METHOD
AppendSegmentToString(nsIInputStream* aInputStream,
                      void* aClosure,
                      const char* aRawSegment,
                      uint32_t aToOffset,
                      uint32_t aCount,
                      uint32_t* outWrittenCount)
{
  nsCString* decodedData = static_cast<nsCString*>(aClosure);
  decodedData->Append(aRawSegment, aCount);
  *outWrittenCount = aCount;
  return NS_OK;
}

NS_IMETHODIMP
CSPViolationReportListener::OnDataAvailable(nsIRequest* aRequest,
                                            nsISupports* aContext,
                                            nsIInputStream* aInputStream,
                                            uint64_t aOffset,
                                            uint32_t aCount)
{
  uint32_t read;
  nsCString decodedData;
  return aInputStream->ReadSegments(AppendSegmentToString,
                                    &decodedData,
                                    aCount,
                                    &read);
}

NS_IMETHODIMP
CSPViolationReportListener::OnStopRequest(nsIRequest* aRequest,
                                          nsISupports* aContext,
                                          nsresult aStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
CSPViolationReportListener::OnStartRequest(nsIRequest* aRequest,
                                           nsISupports* aContext)
{
  return NS_OK;
}



NS_IMPL_ISUPPORTS(CSPReportRedirectSink, nsIChannelEventSink, nsIInterfaceRequestor);

CSPReportRedirectSink::CSPReportRedirectSink()
{
}

CSPReportRedirectSink::~CSPReportRedirectSink()
{
}

NS_IMETHODIMP
CSPReportRedirectSink::AsyncOnChannelRedirect(nsIChannel* aOldChannel,
                                              nsIChannel* aNewChannel,
                                              uint32_t aRedirFlags,
                                              nsIAsyncVerifyRedirectCallback* aCallback)
{
  
  nsresult rv = aOldChannel->Cancel(NS_ERROR_ABORT);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsCOMPtr<nsIURI> uri;
  rv = aOldChannel->GetURI(getter_AddRefs(uri));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIObserverService> observerService = mozilla::services::GetObserverService();
  NS_ASSERTION(observerService, "Observer service required to log CSP violations");
  observerService->NotifyObservers(uri,
                                   CSP_VIOLATION_TOPIC,
                                   NS_LITERAL_STRING("denied redirect while sending violation report").get());

  return NS_BINDING_REDIRECTED;
}

NS_IMETHODIMP
CSPReportRedirectSink::GetInterface(const nsIID& aIID, void** aResult)
{
  return QueryInterface(aIID, aResult);
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

    nsCSPPolicy* policy = nsCSPParser::parseContentSecurityPolicy(policyString,
                                                                  mSelfURI,
                                                                  reportOnly,
                                                                  mInnerWindowID);
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
  }
  return NS_OK;
}
