









#include "nsScriptLoader.h"

#include "jsapi.h"
#include "jsfriendapi.h"
#include "nsIUnicodeDecoder.h"
#include "nsIContent.h"
#include "nsJSUtils.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/Element.h"
#include "nsGkAtoms.h"
#include "nsNetUtil.h"
#include "nsIJSRuntimeService.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsContentPolicyUtils.h"
#include "nsIHttpChannel.h"
#include "nsIClassOfService.h"
#include "nsITimedChannel.h"
#include "nsIScriptElement.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDocShell.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsAutoPtr.h"
#include "nsIXPConnect.h"
#include "nsError.h"
#include "nsThreadUtils.h"
#include "nsDocShellCID.h"
#include "nsIContentSecurityPolicy.h"
#include "prlog.h"
#include "nsCRT.h"
#include "nsContentCreatorFunctions.h"
#include "nsCORSListenerProxy.h"
#include "nsProxyRelease.h"
#include "nsSandboxFlags.h"
#include "nsContentTypeParser.h"
#include "nsINetworkPredictor.h"
#include "ImportManager.h"
#include "mozilla/dom/EncodingUtils.h"

#include "mozilla/Attributes.h"
#include "mozilla/unused.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gCspPRLog;
#endif

using namespace mozilla;
using namespace mozilla::dom;




NS_IMPL_ISUPPORTS0(nsScriptLoadRequest)

nsScriptLoadRequestList::~nsScriptLoadRequestList()
{
  Clear();
}

void
nsScriptLoadRequestList::Clear()
{
  while (!isEmpty()) {
    nsRefPtr<nsScriptLoadRequest> first = StealFirst();
    first->Cancel();
    
  }
}

#ifdef DEBUG
bool
nsScriptLoadRequestList::Contains(nsScriptLoadRequest* aElem)
{
  for (nsScriptLoadRequest* req = getFirst();
       req; req = req->getNext()) {
    if (req == aElem) {
      return true;
    }
  }

  return false;
}
#endif 





nsScriptLoader::nsScriptLoader(nsIDocument *aDocument)
  : mDocument(aDocument),
    mBlockerCount(0),
    mEnabled(true),
    mDeferEnabled(false),
    mDocumentParsingDone(false),
    mBlockingDOMContentLoaded(false)
{
  
#ifdef PR_LOGGING
  if (!gCspPRLog)
    gCspPRLog = PR_NewLogModule("CSP");
#endif
}

nsScriptLoader::~nsScriptLoader()
{
  mObservers.Clear();

  if (mParserBlockingRequest) {
    mParserBlockingRequest->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (nsScriptLoadRequest* req = mXSLTRequests.getFirst(); req;
       req = req->getNext()) {
    req->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (nsScriptLoadRequest* req = mDeferRequests.getFirst(); req;
       req = req->getNext()) {
    req->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (nsScriptLoadRequest* req = mLoadingAsyncRequests.getFirst(); req;
       req = req->getNext()) {
    req->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (nsScriptLoadRequest* req = mLoadedAsyncRequests.getFirst(); req;
       req = req->getNext()) {
    req->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for(nsScriptLoadRequest* req = mNonAsyncExternalScriptInsertedRequests.getFirst();
      req;
      req = req->getNext()) {
    req->FireScriptAvailable(NS_ERROR_ABORT);
  }

  
  
  for (uint32_t j = 0; j < mPendingChildLoaders.Length(); ++j) {
    mPendingChildLoaders[j]->RemoveExecuteBlocker();
  }  
}

NS_IMPL_ISUPPORTS(nsScriptLoader, nsIStreamLoaderObserver)









static bool
IsScriptEventHandler(nsIContent* aScriptElement)
{
  if (!aScriptElement->IsHTMLElement()) {
    return false;
  }

  nsAutoString forAttr, eventAttr;
  if (!aScriptElement->GetAttr(kNameSpaceID_None, nsGkAtoms::_for, forAttr) ||
      !aScriptElement->GetAttr(kNameSpaceID_None, nsGkAtoms::event, eventAttr)) {
    return false;
  }

  const nsAString& for_str =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(forAttr);
  if (!for_str.LowerCaseEqualsLiteral("window")) {
    return true;
  }

  
  const nsAString& event_str =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(eventAttr, false);
  if (!StringBeginsWith(event_str, NS_LITERAL_STRING("onload"),
                        nsCaseInsensitiveStringComparator())) {
    

    return true;
  }

  nsAutoString::const_iterator start, end;
  event_str.BeginReading(start);
  event_str.EndReading(end);

  start.advance(6); 

  if (start != end && *start != '(' && *start != ' ') {
    
    

    return true;
  }

  return false;
}

nsresult
nsScriptLoader::CheckContentPolicy(nsIDocument* aDocument,
                                   nsISupports *aContext,
                                   nsIURI *aURI,
                                   const nsAString &aType)
{
  int16_t shouldLoad = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_SCRIPT,
                                          aURI,
                                          aDocument->NodePrincipal(),
                                          aContext,
                                          NS_LossyConvertUTF16toASCII(aType),
                                          nullptr,    
                                          &shouldLoad,
                                          nsContentUtils::GetContentPolicy(),
                                          nsContentUtils::GetSecurityManager());
  if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
    if (NS_FAILED(rv) || shouldLoad != nsIContentPolicy::REJECT_TYPE) {
      return NS_ERROR_CONTENT_BLOCKED;
    }
    return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
  }

  return NS_OK;
}

nsresult
nsScriptLoader::ShouldLoadScript(nsIDocument* aDocument,
                                 nsISupports* aContext,
                                 nsIURI* aURI,
                                 const nsAString &aType)
{
  
  nsresult rv = nsContentUtils::GetSecurityManager()->
    CheckLoadURIWithPrincipal(aDocument->NodePrincipal(), aURI,
                              nsIScriptSecurityManager::ALLOW_CHROME);

  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = CheckContentPolicy(aDocument, aContext, aURI, aType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  return NS_OK;
}

nsresult
nsScriptLoader::StartLoad(nsScriptLoadRequest *aRequest, const nsAString &aType,
                          bool aScriptFromHead)
{
  nsISupports *context = aRequest->mElement.get()
                         ? static_cast<nsISupports *>(aRequest->mElement.get())
                         : static_cast<nsISupports *>(mDocument);
  nsresult rv = ShouldLoadScript(mDocument, context, aRequest->mURI, aType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsILoadGroup> loadGroup = mDocument->GetDocumentLoadGroup();

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mDocument->MasterDocument()->GetWindow()));

  if (!window) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIDocShell *docshell = window->GetDocShell();

  nsCOMPtr<nsIInterfaceRequestor> prompter(do_QueryInterface(docshell));

  
  if (mDocument->GetSandboxFlags() & SANDBOXED_SCRIPTS) {
    return NS_OK;
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aRequest->mURI,
                     mDocument,
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_SCRIPT,
                     loadGroup,
                     prompter,
                     nsIRequest::LOAD_NORMAL |
                     nsIChannel::LOAD_CLASSIFY_URI);

  NS_ENSURE_SUCCESS(rv, rv);

  nsIScriptElement *script = aRequest->mElement;
  nsCOMPtr<nsIClassOfService> cos(do_QueryInterface(channel));

  if (cos) {
    if (aScriptFromHead &&
        !(script && (script->GetScriptAsync() || script->GetScriptDeferred()))) {
      
      
      cos->AddClassFlags(nsIClassOfService::Leader);
    } else if (!(script && script->GetScriptDeferred())) {
      
      cos->AddClassFlags(nsIClassOfService::Unblocked);
    }
  }

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    
    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                  NS_LITERAL_CSTRING("*/*"),
                                  false);
    httpChannel->SetReferrerWithPolicy(mDocument->GetDocumentURI(),
                                       aRequest->mReferrerPolicy);
  }

  nsCOMPtr<nsILoadContext> loadContext(do_QueryInterface(docshell));
  mozilla::net::PredictorLearn(aRequest->mURI, mDocument->GetDocumentURI(),
      nsINetworkPredictor::LEARN_LOAD_SUBRESOURCE, loadContext);

  
  nsCOMPtr<nsITimedChannel> timedChannel(do_QueryInterface(httpChannel));
  if (timedChannel) {
    timedChannel->SetInitiatorType(NS_LITERAL_STRING("script"));
  }

  nsCOMPtr<nsIStreamLoader> loader;
  rv = NS_NewStreamLoader(getter_AddRefs(loader), this);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIStreamListener> listener = loader.get();

  if (aRequest->mCORSMode != CORS_NONE) {
    bool withCredentials = (aRequest->mCORSMode == CORS_USE_CREDENTIALS);
    nsRefPtr<nsCORSListenerProxy> corsListener =
      new nsCORSListenerProxy(listener, mDocument->NodePrincipal(),
                              withCredentials);
    rv = corsListener->Init(channel);
    NS_ENSURE_SUCCESS(rv, rv);
    listener = corsListener;
  }

  rv = channel->AsyncOpen(listener, aRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

bool
nsScriptLoader::PreloadURIComparator::Equals(const PreloadInfo &aPi,
                                             nsIURI * const &aURI) const
{
  bool same;
  return NS_SUCCEEDED(aPi.mRequest->mURI->Equals(aURI, &same)) &&
         same;
}

class nsScriptRequestProcessor : public nsRunnable
{
private:
  nsRefPtr<nsScriptLoader> mLoader;
  nsRefPtr<nsScriptLoadRequest> mRequest;
public:
  nsScriptRequestProcessor(nsScriptLoader* aLoader,
                           nsScriptLoadRequest* aRequest)
    : mLoader(aLoader)
    , mRequest(aRequest)
  {}
  NS_IMETHODIMP Run()
  {
    return mLoader->ProcessRequest(mRequest);
  }
};

static inline bool
ParseTypeAttribute(const nsAString& aType, JSVersion* aVersion)
{
  MOZ_ASSERT(!aType.IsEmpty());
  MOZ_ASSERT(aVersion);
  MOZ_ASSERT(*aVersion == JSVERSION_DEFAULT);

  nsContentTypeParser parser(aType);

  nsAutoString mimeType;
  nsresult rv = parser.GetType(mimeType);
  NS_ENSURE_SUCCESS(rv, false);

  if (!nsContentUtils::IsJavascriptMIMEType(mimeType)) {
    return false;
  }

  
  nsAutoString versionName;
  rv = parser.GetParameter("version", versionName);

  if (NS_SUCCEEDED(rv)) {
    *aVersion = nsContentUtils::ParseJavascriptVersion(versionName);
  } else if (rv != NS_ERROR_INVALID_ARG) {
    return false;
  }

  return true;
}

static bool
CSPAllowsInlineScript(nsIScriptElement *aElement, nsIDocument *aDocument)
{
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  
  
  nsresult rv = aDocument->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, false);

  if (!csp) {
    
    return true;
  }

  
  
  
  
  
  
  bool allowInlineScript = true;
  nsAutoTArray<unsigned short, 3> violations;

  bool reportInlineViolation = false;
  rv = csp->GetAllowsInlineScript(&reportInlineViolation, &allowInlineScript);
  NS_ENSURE_SUCCESS(rv, false);
  if (reportInlineViolation) {
    violations.AppendElement(static_cast<unsigned short>(
          nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_SCRIPT));
  }

  nsAutoString nonce;
  if (!allowInlineScript) {
    nsCOMPtr<nsIContent> scriptContent = do_QueryInterface(aElement);
    bool foundNonce = scriptContent->GetAttr(kNameSpaceID_None,
                                             nsGkAtoms::nonce, nonce);
    if (foundNonce) {
      bool reportNonceViolation;
      rv = csp->GetAllowsNonce(nonce, nsIContentPolicy::TYPE_SCRIPT,
                               &reportNonceViolation, &allowInlineScript);
      NS_ENSURE_SUCCESS(rv, false);
      if (reportNonceViolation) {
        violations.AppendElement(static_cast<unsigned short>(
              nsIContentSecurityPolicy::VIOLATION_TYPE_NONCE_SCRIPT));
      }
    }
  }

  if (!allowInlineScript) {
    bool reportHashViolation;
    nsAutoString scriptText;
    aElement->GetScriptText(scriptText);
    rv = csp->GetAllowsHash(scriptText, nsIContentPolicy::TYPE_SCRIPT,
                            &reportHashViolation, &allowInlineScript);
    NS_ENSURE_SUCCESS(rv, false);
    if (reportHashViolation) {
      violations.AppendElement(static_cast<unsigned short>(
            nsIContentSecurityPolicy::VIOLATION_TYPE_HASH_SCRIPT));
    }
  }

  
  
  
  
  
  
  
  
  
  
  
  
  if (!violations.IsEmpty()) {
    MOZ_ASSERT(violations[0] == nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_SCRIPT,
               "How did we get any violations without an initial inline script violation?");
    
    nsIURI* uri = aDocument->GetDocumentURI();
    nsAutoCString asciiSpec;
    uri->GetAsciiSpec(asciiSpec);
    nsAutoString scriptText;
    aElement->GetScriptText(scriptText);
    nsAutoString scriptSample(scriptText);

    
    if (scriptSample.Length() > 40) {
      scriptSample.Truncate(40);
      scriptSample.AppendLiteral("...");
    }

    for (uint32_t i = 0; i < violations.Length(); i++) {
      
      
      if (i > 0 || violations.Length() == 1) {
        csp->LogViolationDetails(violations[i], NS_ConvertUTF8toUTF16(asciiSpec),
                                 scriptSample, aElement->GetScriptLineNumber(),
                                 nonce, scriptText);
      }
    }
  }

  if (!allowInlineScript) {
    NS_ASSERTION(!violations.IsEmpty(),
        "CSP blocked inline script but is not reporting a violation");
   return false;
  }
  return true;
}

bool
nsScriptLoader::ProcessScriptElement(nsIScriptElement *aElement)
{
  
  NS_ENSURE_TRUE(mDocument, false);

  
  if (!mEnabled || !mDocument->IsScriptEnabled()) {
    return false;
  }

  NS_ASSERTION(!aElement->IsMalformed(), "Executing malformed script");

  nsCOMPtr<nsIContent> scriptContent = do_QueryInterface(aElement);

  
  if (IsScriptEventHandler(scriptContent)) {
    return false;
  }

  JSVersion version = JSVERSION_DEFAULT;

  
  
  nsAutoString type;
  aElement->GetScriptType(type);
  if (!type.IsEmpty()) {
    NS_ENSURE_TRUE(ParseTypeAttribute(type, &version), false);
  } else {
    
    
    
    if (scriptContent->IsHTMLElement()) {
      nsAutoString language;
      scriptContent->GetAttr(kNameSpaceID_None, nsGkAtoms::language, language);
      if (!language.IsEmpty()) {
        if (!nsContentUtils::IsJavaScriptLanguage(language)) {
          return false;
        }
      }
    }
  }

  
  nsresult rv = NS_OK;
  nsRefPtr<nsScriptLoadRequest> request;
  if (aElement->GetScriptExternal()) {
    
    nsCOMPtr<nsIURI> scriptURI = aElement->GetScriptURI();
    if (!scriptURI) {
      
      NS_DispatchToCurrentThread(
        NS_NewRunnableMethod(aElement,
                             &nsIScriptElement::FireErrorEvent));
      return false;
    }

    
    mozilla::net::ReferrerPolicy ourRefPolicy = mDocument->GetReferrerPolicy();
    CORSMode ourCORSMode = aElement->GetCORSMode();
    nsTArray<PreloadInfo>::index_type i =
      mPreloads.IndexOf(scriptURI.get(), 0, PreloadURIComparator());
    if (i != nsTArray<PreloadInfo>::NoIndex) {
      
      
      request = mPreloads[i].mRequest;
      request->mElement = aElement;
      nsString preloadCharset(mPreloads[i].mCharset);
      mPreloads.RemoveElementAt(i);

      
      
      nsAutoString elementCharset;
      aElement->GetScriptCharset(elementCharset);
      if (elementCharset.Equals(preloadCharset) &&
          ourCORSMode == request->mCORSMode &&
          ourRefPolicy == request->mReferrerPolicy) {
        rv = CheckContentPolicy(mDocument, aElement, request->mURI, type);
        NS_ENSURE_SUCCESS(rv, false);
      } else {
        
        request = nullptr;
      }
    }

    if (!request) {
      
      request = new nsScriptLoadRequest(aElement, version, ourCORSMode);
      request->mURI = scriptURI;
      request->mIsInline = false;
      request->mLoading = true;
      request->mReferrerPolicy = ourRefPolicy;

      
      
      rv = StartLoad(request, type, false);
      if (NS_FAILED(rv)) {
        
        NS_DispatchToCurrentThread(
          NS_NewRunnableMethod(aElement,
                               &nsIScriptElement::FireErrorEvent));
        return false;
      }
    }

    request->mJSVersion = version;

    if (aElement->GetScriptAsync()) {
      request->mIsAsync = true;
      if (!request->mLoading) {
        mLoadedAsyncRequests.AppendElement(request);
        
        
        ProcessPendingRequestsAsync();
      } else {
        mLoadingAsyncRequests.AppendElement(request);
      }
      return false;
    }
    if (!aElement->GetParserCreated()) {
      
      
      
      request->mIsNonAsyncScriptInserted = true;
      mNonAsyncExternalScriptInsertedRequests.AppendElement(request);
      if (!request->mLoading) {
        
        
        ProcessPendingRequestsAsync();
      }
      return false;
    }
    
    
    if (aElement->GetScriptDeferred()) {
      
      
      
      
      
      
      NS_ASSERTION(mDocument->GetCurrentContentSink() ||
                   aElement->GetParserCreated() == FROM_PARSER_XSLT,
          "Non-XSLT Defer script on a document without an active parser; bug 592366.");
      AddDeferRequest(request);
      return false;
    }

    if (aElement->GetParserCreated() == FROM_PARSER_XSLT) {
      
      NS_ASSERTION(!mParserBlockingRequest,
          "Parser-blocking scripts and XSLT scripts in the same doc!");
      request->mIsXSLT = true;
      mXSLTRequests.AppendElement(request);
      if (!request->mLoading) {
        
        
        ProcessPendingRequestsAsync();
      }
      return true;
    }
    if (!request->mLoading && ReadyToExecuteScripts()) {
      
      
      
      if (aElement->GetParserCreated() == FROM_PARSER_NETWORK) {
        return ProcessRequest(request) == NS_ERROR_HTMLPARSER_BLOCK;
      }
      
      
      
      NS_ASSERTION(!mParserBlockingRequest,
          "There can be only one parser-blocking script at a time");
      NS_ASSERTION(mXSLTRequests.isEmpty(),
          "Parser-blocking scripts and XSLT scripts in the same doc!");
      mParserBlockingRequest = request;
      ProcessPendingRequestsAsync();
      return true;
    }
    
    
    NS_ASSERTION(!mParserBlockingRequest,
        "There can be only one parser-blocking script at a time");
    NS_ASSERTION(mXSLTRequests.isEmpty(),
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    mParserBlockingRequest = request;
    return true;
  }

  
  
  if (mDocument->GetSandboxFlags() & SANDBOXED_SCRIPTS) {
    return false;
  }

  
  if (!CSPAllowsInlineScript(aElement, mDocument)) {
    return false;
  }

  
  request = new nsScriptLoadRequest(aElement, version, CORS_NONE);
  request->mJSVersion = version;
  request->mLoading = false;
  request->mIsInline = true;
  request->mURI = mDocument->GetDocumentURI();
  request->mLineNo = aElement->GetScriptLineNumber();

  if (aElement->GetParserCreated() == FROM_PARSER_XSLT &&
      (!ReadyToExecuteScripts() || !mXSLTRequests.isEmpty())) {
    
    NS_ASSERTION(!mParserBlockingRequest,
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    mXSLTRequests.AppendElement(request);
    return true;
  }
  if (aElement->GetParserCreated() == NOT_FROM_PARSER) {
    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
        "A script-inserted script is inserted without an update batch?");
    nsContentUtils::AddScriptRunner(new nsScriptRequestProcessor(this,
                                                                 request));
    return false;
  }
  if (aElement->GetParserCreated() == FROM_PARSER_NETWORK &&
      !ReadyToExecuteScripts()) {
    NS_ASSERTION(!mParserBlockingRequest,
        "There can be only one parser-blocking script at a time");
    mParserBlockingRequest = request;
    NS_ASSERTION(mXSLTRequests.isEmpty(),
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    return true;
  }
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
      "Not safe to run a parser-inserted script?");
  return ProcessRequest(request) == NS_ERROR_HTMLPARSER_BLOCK;
}

namespace {

class NotifyOffThreadScriptLoadCompletedRunnable : public nsRunnable
{
  nsRefPtr<nsScriptLoadRequest> mRequest;
  nsRefPtr<nsScriptLoader> mLoader;
  void *mToken;

public:
  NotifyOffThreadScriptLoadCompletedRunnable(nsScriptLoadRequest* aRequest,
                                             nsScriptLoader* aLoader)
    : mRequest(aRequest), mLoader(aLoader), mToken(nullptr)
  {}

  virtual ~NotifyOffThreadScriptLoadCompletedRunnable();

  void SetToken(void* aToken) {
    MOZ_ASSERT(aToken && !mToken);
    mToken = aToken;
  }

  NS_DECL_NSIRUNNABLE
};

} 

nsresult
nsScriptLoader::ProcessOffThreadRequest(nsScriptLoadRequest* aRequest, void **aOffThreadToken)
{
  nsresult rv = ProcessRequest(aRequest, aOffThreadToken);
  mDocument->UnblockOnload(false);
  return rv;
}

NotifyOffThreadScriptLoadCompletedRunnable::~NotifyOffThreadScriptLoadCompletedRunnable()
{
  if (MOZ_UNLIKELY(mRequest || mLoader) && !NS_IsMainThread()) {
    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));
    if (mainThread) {
      NS_ProxyRelease(mainThread, mRequest);
      NS_ProxyRelease(mainThread, mLoader);
    } else {
      MOZ_ASSERT(false, "We really shouldn't leak!");
      
      unused << mRequest.forget();
      unused << mLoader.forget();
    }
  }
}

NS_IMETHODIMP
NotifyOffThreadScriptLoadCompletedRunnable::Run()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsRefPtr<nsScriptLoadRequest> request = mRequest.forget();
  nsRefPtr<nsScriptLoader> loader = mLoader.forget();

  nsresult rv = loader->ProcessOffThreadRequest(request, &mToken);

  if (mToken) {
    
    
    
    nsCOMPtr<nsIJSRuntimeService> svc = do_GetService("@mozilla.org/js/xpc/RuntimeService;1");
    NS_ENSURE_TRUE(svc, NS_ERROR_FAILURE);
    JSRuntime *rt;
    svc->GetRuntime(&rt);
    NS_ENSURE_TRUE(rt, NS_ERROR_FAILURE);
    JS::FinishOffThreadScript(nullptr, rt, mToken);
  }

  return rv;
}

static void
OffThreadScriptLoaderCallback(void *aToken, void *aCallbackData)
{
  nsRefPtr<NotifyOffThreadScriptLoadCompletedRunnable> aRunnable =
    dont_AddRef(static_cast<NotifyOffThreadScriptLoadCompletedRunnable*>(aCallbackData));
  aRunnable->SetToken(aToken);
  NS_DispatchToMainThread(aRunnable);
}

nsresult
nsScriptLoader::AttemptAsyncScriptParse(nsScriptLoadRequest* aRequest)
{
  if (!aRequest->mElement->GetScriptAsync() || aRequest->mIsInline) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIScriptGlobalObject> globalObject = GetScriptGlobalObject();
  if (!globalObject) {
    return NS_ERROR_FAILURE;
  }

  AutoJSAPI jsapi;
  if (!jsapi.InitWithLegacyErrorReporting(globalObject)) {
    return NS_ERROR_FAILURE;
  }

  JSContext* cx = jsapi.cx();
  JS::Rooted<JSObject*> global(cx, globalObject->GetGlobalJSObject());
  JS::CompileOptions options(cx);
  FillCompileOptionsForRequest(jsapi, aRequest, global, &options);

  if (!JS::CanCompileOffThread(cx, options, aRequest->mScriptTextLength)) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<NotifyOffThreadScriptLoadCompletedRunnable> runnable =
    new NotifyOffThreadScriptLoadCompletedRunnable(aRequest, this);

  if (!JS::CompileOffThread(cx, options,
                            aRequest->mScriptTextBuf, aRequest->mScriptTextLength,
                            OffThreadScriptLoaderCallback,
                            static_cast<void*>(runnable))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  mDocument->BlockOnload();

  unused << runnable.forget();
  return NS_OK;
}

nsresult
nsScriptLoader::ProcessRequest(nsScriptLoadRequest* aRequest, void **aOffThreadToken)
{
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Processing requests when running scripts is unsafe.");

  if (!aOffThreadToken) {
    nsresult rv = AttemptAsyncScriptParse(aRequest);
    if (rv != NS_ERROR_FAILURE)
      return rv;
  }

  NS_ENSURE_ARG(aRequest);
  nsAutoString textData;
  const char16_t* scriptBuf = nullptr;
  size_t scriptLength = 0;
  JS::SourceBufferHolder::Ownership giveScriptOwnership =
    JS::SourceBufferHolder::NoOwnership;

  nsCOMPtr<nsIDocument> doc;

  nsCOMPtr<nsINode> scriptElem = do_QueryInterface(aRequest->mElement);

  
  if (aRequest->mIsInline) {
    
    
    aRequest->mElement->GetScriptText(textData);

    scriptBuf = textData.get();
    scriptLength = textData.Length();
    giveScriptOwnership = JS::SourceBufferHolder::NoOwnership;
  }
  else {
    scriptBuf = aRequest->mScriptTextBuf;
    scriptLength = aRequest->mScriptTextLength;

    giveScriptOwnership = JS::SourceBufferHolder::GiveOwnership;
    aRequest->mScriptTextBuf = nullptr;
    aRequest->mScriptTextLength = 0;

    doc = scriptElem->OwnerDoc();
  }

  JS::SourceBufferHolder srcBuf(scriptBuf, scriptLength, giveScriptOwnership);

  nsCOMPtr<nsIScriptElement> oldParserInsertedScript;
  uint32_t parserCreated = aRequest->mElement->GetParserCreated();
  if (parserCreated) {
    oldParserInsertedScript = mCurrentParserInsertedScript;
    mCurrentParserInsertedScript = aRequest->mElement;
  }

  FireScriptAvailable(NS_OK, aRequest);

  
  
  nsCOMPtr<nsIDocument> master = mDocument->MasterDocument();
  nsPIDOMWindow *pwin = master->GetInnerWindow();
  bool runScript = !!pwin;
  if (runScript) {
    nsContentUtils::DispatchTrustedEvent(scriptElem->OwnerDoc(),
                                         scriptElem,
                                         NS_LITERAL_STRING("beforescriptexecute"),
                                         true, true, &runScript);
  }

  
  pwin = master->GetInnerWindow();
  if (!pwin) {
    runScript = false;
  }

  nsresult rv = NS_OK;
  if (runScript) {
    if (doc) {
      doc->BeginEvaluatingExternalScript();
    }
    aRequest->mElement->BeginEvaluating();
    rv = EvaluateScript(aRequest, srcBuf, aOffThreadToken);
    aRequest->mElement->EndEvaluating();
    if (doc) {
      doc->EndEvaluatingExternalScript();
    }

    nsContentUtils::DispatchTrustedEvent(scriptElem->OwnerDoc(),
                                         scriptElem,
                                         NS_LITERAL_STRING("afterscriptexecute"),
                                         true, false);
  }

  FireScriptEvaluated(rv, aRequest);

  if (parserCreated) {
    mCurrentParserInsertedScript = oldParserInsertedScript;
  }

  return rv;
}

void
nsScriptLoader::FireScriptAvailable(nsresult aResult,
                                    nsScriptLoadRequest* aRequest)
{
  for (int32_t i = 0; i < mObservers.Count(); i++) {
    nsCOMPtr<nsIScriptLoaderObserver> obs = mObservers[i];
    obs->ScriptAvailable(aResult, aRequest->mElement,
                         aRequest->mIsInline, aRequest->mURI,
                         aRequest->mLineNo);
  }

  aRequest->FireScriptAvailable(aResult);
}

void
nsScriptLoader::FireScriptEvaluated(nsresult aResult,
                                    nsScriptLoadRequest* aRequest)
{
  for (int32_t i = 0; i < mObservers.Count(); i++) {
    nsCOMPtr<nsIScriptLoaderObserver> obs = mObservers[i];
    obs->ScriptEvaluated(aResult, aRequest->mElement,
                         aRequest->mIsInline);
  }

  aRequest->FireScriptEvaluated(aResult);
}

already_AddRefed<nsIScriptGlobalObject>
nsScriptLoader::GetScriptGlobalObject()
{
  nsCOMPtr<nsIDocument> master = mDocument->MasterDocument();
  nsPIDOMWindow *pwin = master->GetInnerWindow();
  if (!pwin) {
    return nullptr;
  }

  nsCOMPtr<nsIScriptGlobalObject> globalObject = do_QueryInterface(pwin);
  NS_ASSERTION(globalObject, "windows must be global objects");

  
  nsresult rv = globalObject->EnsureScriptEnvironment();
  if (NS_FAILED(rv)) {
    return nullptr;
  }

  return globalObject.forget();
}

void
nsScriptLoader::FillCompileOptionsForRequest(const AutoJSAPI &jsapi,
                                             nsScriptLoadRequest *aRequest,
                                             JS::Handle<JSObject *> aScopeChain,
                                             JS::CompileOptions *aOptions)
{
  
  
  nsContentUtils::GetWrapperSafeScriptFilename(mDocument, aRequest->mURI, aRequest->mURL);

  aOptions->setIntroductionType("scriptElement");
  aOptions->setFileAndLine(aRequest->mURL.get(), aRequest->mLineNo);
  aOptions->setVersion(JSVersion(aRequest->mJSVersion));
  aOptions->setIsRunOnce(true);
  
  
  aOptions->setNoScriptRval(true);
  if (aRequest->mHasSourceMapURL) {
    aOptions->setSourceMapURL(aRequest->mSourceMapURL.get());
  }
  if (aRequest->mOriginPrincipal) {
    nsIPrincipal* scriptPrin = nsContentUtils::ObjectPrincipal(aScopeChain);
    bool subsumes = scriptPrin->Subsumes(aRequest->mOriginPrincipal);
    aOptions->setMutedErrors(!subsumes);
  }

  JSContext* cx = jsapi.cx();
  JS::Rooted<JS::Value> elementVal(cx);
  MOZ_ASSERT(aRequest->mElement);
  if (NS_SUCCEEDED(nsContentUtils::WrapNative(cx, aRequest->mElement,
                                              &elementVal,
                                               true))) {
    MOZ_ASSERT(elementVal.isObject());
    aOptions->setElement(&elementVal.toObject());
  }
}

nsresult
nsScriptLoader::EvaluateScript(nsScriptLoadRequest* aRequest,
                               JS::SourceBufferHolder& aSrcBuf,
                               void** aOffThreadToken)
{
  
  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIContent> scriptContent(do_QueryInterface(aRequest->mElement));
  nsIDocument* ownerDoc = scriptContent->OwnerDoc();
  if (ownerDoc != mDocument) {
    
    return NS_ERROR_FAILURE;
  }

  
  NS_ASSERTION(scriptContent, "no content - what is default script-type?");

  nsCOMPtr<nsIScriptGlobalObject> globalObject = GetScriptGlobalObject();
  if (!globalObject) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  nsCOMPtr<nsIScriptContext> context = globalObject->GetScriptContext();
  if (!context) {
    return NS_ERROR_FAILURE;
  }

  JSVersion version = JSVersion(aRequest->mJSVersion);
  if (version == JSVERSION_UNKNOWN) {
    return NS_OK;
  }

  
  
  AutoEntryScript entryScript(globalObject, "<script> element", true,
                              context->GetNativeContext());
  JS::Rooted<JSObject*> global(entryScript.cx(),
                               globalObject->GetGlobalJSObject());

  bool oldProcessingScriptTag = context->GetProcessingScriptTag();
  context->SetProcessingScriptTag(true);
  nsresult rv;
  {
    
    AutoCurrentScriptUpdater scriptUpdater(this, aRequest->mElement);
    Maybe<AutoCurrentScriptUpdater> masterScriptUpdater;
    nsCOMPtr<nsIDocument> master = mDocument->MasterDocument();
    if (master != mDocument) {
      
      
      
      
      
      masterScriptUpdater.emplace(master->ScriptLoader(),
                                  aRequest->mElement);
    }

    JS::CompileOptions options(entryScript.cx());
    FillCompileOptionsForRequest(entryScript, aRequest, global, &options);
    rv = nsJSUtils::EvaluateString(entryScript.cx(), aSrcBuf, global, options,
                                   aOffThreadToken);
  }

  context->SetProcessingScriptTag(oldProcessingScriptTag);
  return rv;
}

void
nsScriptLoader::ProcessPendingRequestsAsync()
{
  if (mParserBlockingRequest || !mPendingChildLoaders.IsEmpty()) {
    nsCOMPtr<nsIRunnable> ev = NS_NewRunnableMethod(this,
      &nsScriptLoader::ProcessPendingRequests);

    NS_DispatchToCurrentThread(ev);
  }
}

void
nsScriptLoader::ProcessPendingRequests()
{
  nsRefPtr<nsScriptLoadRequest> request;
  if (mParserBlockingRequest &&
      !mParserBlockingRequest->mLoading &&
      ReadyToExecuteScripts()) {
    request.swap(mParserBlockingRequest);
    UnblockParser(request);
    ProcessRequest(request);
    ContinueParserAsync(request);
  }

  while (ReadyToExecuteScripts() && 
         !mXSLTRequests.isEmpty() &&
         !mXSLTRequests.getFirst()->mLoading) {
    request = mXSLTRequests.StealFirst();
    ProcessRequest(request);
  }

  while (mEnabled && !mLoadedAsyncRequests.isEmpty()) {
    request = mLoadedAsyncRequests.StealFirst();
    ProcessRequest(request);
  }

  while (mEnabled && !mNonAsyncExternalScriptInsertedRequests.isEmpty() &&
         !mNonAsyncExternalScriptInsertedRequests.getFirst()->mLoading) {
    
    
    
    
    request = mNonAsyncExternalScriptInsertedRequests.StealFirst();
    ProcessRequest(request);
  }

  if (mDocumentParsingDone && mXSLTRequests.isEmpty()) {
    while (!mDeferRequests.isEmpty() && !mDeferRequests.getFirst()->mLoading) {
      request = mDeferRequests.StealFirst();
      ProcessRequest(request);
    }
  }

  while (!mPendingChildLoaders.IsEmpty() && ReadyToExecuteScripts()) {
    nsRefPtr<nsScriptLoader> child = mPendingChildLoaders[0];
    mPendingChildLoaders.RemoveElementAt(0);
    child->RemoveExecuteBlocker();
  }

  if (mDocumentParsingDone && mDocument &&
      !mParserBlockingRequest && mLoadingAsyncRequests.isEmpty() &&
      mLoadedAsyncRequests.isEmpty() &&
      mNonAsyncExternalScriptInsertedRequests.isEmpty() &&
      mXSLTRequests.isEmpty() && mDeferRequests.isEmpty()) {
    if (MaybeRemovedDeferRequests()) {
      return ProcessPendingRequests();
    }
    
    
    
    mDocumentParsingDone = false;
    mDocument->UnblockOnload(true);
  }
}

bool
nsScriptLoader::ReadyToExecuteScripts()
{
  
  
  if (!SelfReadyToExecuteScripts()) {
    return false;
  }

  for (nsIDocument* doc = mDocument; doc; doc = doc->GetParentDocument()) {
    nsScriptLoader* ancestor = doc->ScriptLoader();
    if (!ancestor->SelfReadyToExecuteScripts() &&
        ancestor->AddPendingChildLoader(this)) {
      AddExecuteBlocker();
      return false;
    }
  }

  if (mDocument && !mDocument->IsMasterDocument()) {
    nsRefPtr<ImportManager> im = mDocument->ImportManager();
    nsRefPtr<ImportLoader> loader = im->Find(mDocument);
    MOZ_ASSERT(loader, "How can we have an import document without a loader?");

    
    
    nsCOMPtr<nsINode> referrer = loader->GetMainReferrer();
    MOZ_ASSERT(referrer, "There has to be a main referring link for each imports");

    
    
    
    
    nsRefPtr<ImportLoader> lastPred = im->GetNearestPredecessor(referrer);
    if (!lastPred) {
      
      return true;
    }

    nsCOMPtr<nsIDocument> doc = lastPred->GetDocument();
    if (lastPred->IsBlocking() || !doc || (doc && !doc->ScriptLoader()->SelfReadyToExecuteScripts())) {
      
      
      
      
      
      lastPred->AddBlockedScriptLoader(this);
      
      
      loader->SetBlockingPredecessor(lastPred);
      return false;
    }
  }

  return true;
}


static bool
DetectByteOrderMark(const unsigned char* aBytes, int32_t aLen, nsCString& oCharset)
{
  if (aLen < 2)
    return false;

  switch(aBytes[0]) {
  case 0xEF:
    if (aLen >= 3 && 0xBB == aBytes[1] && 0xBF == aBytes[2]) {
      
      
      oCharset.AssignLiteral("UTF-8");
    }
    break;
  case 0xFE:
    if (0xFF == aBytes[1]) {
      
      
      oCharset.AssignLiteral("UTF-16BE");
    }
    break;
  case 0xFF:
    if (0xFE == aBytes[1]) {
      
      
      oCharset.AssignLiteral("UTF-16LE");
    }
    break;
  }
  return !oCharset.IsEmpty();
}

 nsresult
nsScriptLoader::ConvertToUTF16(nsIChannel* aChannel, const uint8_t* aData,
                               uint32_t aLength, const nsAString& aHintCharset,
                               nsIDocument* aDocument,
                               char16_t*& aBufOut, size_t& aLengthOut)
{
  if (!aLength) {
    aBufOut = nullptr;
    aLengthOut = 0;
    return NS_OK;
  }

  
  
  
  
  

  nsAutoCString charset;

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;

  if (DetectByteOrderMark(aData, aLength, charset)) {
    
    
    
    unicodeDecoder = EncodingUtils::DecoderForEncoding(charset);
  }

  if (!unicodeDecoder &&
      aChannel &&
      NS_SUCCEEDED(aChannel->GetContentCharset(charset)) &&
      EncodingUtils::FindEncodingForLabel(charset, charset)) {
    unicodeDecoder = EncodingUtils::DecoderForEncoding(charset);
  }

  if (!unicodeDecoder &&
      EncodingUtils::FindEncodingForLabel(aHintCharset, charset)) {
    unicodeDecoder = EncodingUtils::DecoderForEncoding(charset);
  }

  if (!unicodeDecoder && aDocument) {
    charset = aDocument->GetDocumentCharacterSet();
    unicodeDecoder = EncodingUtils::DecoderForEncoding(charset);
  }

  if (!unicodeDecoder) {
    
    
    
    
    unicodeDecoder = EncodingUtils::DecoderForEncoding("windows-1252");
  }

  int32_t unicodeLength = 0;

  nsresult rv =
    unicodeDecoder->GetMaxLength(reinterpret_cast<const char*>(aData),
                                 aLength, &unicodeLength);
  NS_ENSURE_SUCCESS(rv, rv);

  aBufOut = static_cast<char16_t*>(js_malloc(unicodeLength * sizeof(char16_t)));
  if (!aBufOut) {
    aLengthOut = 0;
    return NS_ERROR_OUT_OF_MEMORY;
  }
  aLengthOut = unicodeLength;

  rv = unicodeDecoder->Convert(reinterpret_cast<const char*>(aData),
                               (int32_t *) &aLength, aBufOut,
                               &unicodeLength);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  aLengthOut = unicodeLength;
  if (NS_FAILED(rv)) {
    js_free(aBufOut);
    aBufOut = nullptr;
    aLengthOut = 0;
  }
  return rv;
}

NS_IMETHODIMP
nsScriptLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                 nsISupports* aContext,
                                 nsresult aStatus,
                                 uint32_t aStringLen,
                                 const uint8_t* aString)
{
  nsScriptLoadRequest* request = static_cast<nsScriptLoadRequest*>(aContext);
  NS_ASSERTION(request, "null request in stream complete handler");
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsresult rv = PrepareLoadedRequest(request, aLoader, aStatus, aStringLen,
                                     aString);
  if (NS_FAILED(rv)) {
    




    if (rv == NS_ERROR_TRACKING_URI) {
      nsCOMPtr<nsIContent> cont = do_QueryInterface(request->mElement);
      mDocument->AddBlockedTrackingNode(cont);
    }

    if (request->mIsDefer) {
      if (request->isInList()) {
        nsRefPtr<nsScriptLoadRequest> req = mDeferRequests.Steal(request);
        FireScriptAvailable(rv, req);
      }
    } else if (request->mIsAsync) {
      if (request->isInList()) {
        nsRefPtr<nsScriptLoadRequest> req = mLoadingAsyncRequests.Steal(request);
        FireScriptAvailable(rv, req);
      }
    } else if (request->mIsNonAsyncScriptInserted) {
      if (request->isInList()) {
        nsRefPtr<nsScriptLoadRequest> req =
          mNonAsyncExternalScriptInsertedRequests.Steal(request);
        FireScriptAvailable(rv, req);
      }
    } else if (request->mIsXSLT) {
      if (request->isInList()) {
        nsRefPtr<nsScriptLoadRequest> req = mXSLTRequests.Steal(request);
        FireScriptAvailable(rv, req);
      }
    } else if (mParserBlockingRequest == request) {
      mParserBlockingRequest = nullptr;
      UnblockParser(request);
      FireScriptAvailable(rv, request);
      ContinueParserAsync(request);
    } else {
      mPreloads.RemoveElement(request, PreloadRequestComparator());
    }
    rv = NS_OK;
  } else {
    free(const_cast<uint8_t *>(aString));
    rv = NS_SUCCESS_ADOPTED_DATA;
  }

  
  ProcessPendingRequests();

  return rv;
}

void
nsScriptLoader::UnblockParser(nsScriptLoadRequest* aParserBlockingRequest)
{
  aParserBlockingRequest->mElement->UnblockParser();
}

void
nsScriptLoader::ContinueParserAsync(nsScriptLoadRequest* aParserBlockingRequest)
{
  aParserBlockingRequest->mElement->ContinueParserAsync();
}

nsresult
nsScriptLoader::PrepareLoadedRequest(nsScriptLoadRequest* aRequest,
                                     nsIStreamLoader* aLoader,
                                     nsresult aStatus,
                                     uint32_t aStringLen,
                                     const uint8_t* aString)
{
  if (NS_FAILED(aStatus)) {
    return aStatus;
  }

  if (aRequest->IsCanceled()) {
    return NS_BINDING_ABORTED;
  }

  
  
  if (!mDocument) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  nsCOMPtr<nsIRequest> req;
  nsresult rv = aLoader->GetRequest(getter_AddRefs(req));
  NS_ASSERTION(req, "StreamLoader's request went away prematurely");
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(req);
  if (httpChannel) {
    bool requestSucceeded;
    rv = httpChannel->GetRequestSucceeded(&requestSucceeded);
    if (NS_SUCCEEDED(rv) && !requestSucceeded) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    nsAutoCString sourceMapURL;
    rv = httpChannel->GetResponseHeader(NS_LITERAL_CSTRING("X-SourceMap"), sourceMapURL);
    if (NS_SUCCEEDED(rv)) {
      aRequest->mHasSourceMapURL = true;
      aRequest->mSourceMapURL = NS_ConvertUTF8toUTF16(sourceMapURL);
    }
  }

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(req);
  
  
  
  if (aRequest->mCORSMode == CORS_NONE) {
    rv = nsContentUtils::GetSecurityManager()->
      GetChannelResultPrincipal(channel, getter_AddRefs(aRequest->mOriginPrincipal));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  if (aStringLen) {
    
    nsAutoString hintCharset;
    if (!aRequest->IsPreload()) {
      aRequest->mElement->GetScriptCharset(hintCharset);
    } else {
      nsTArray<PreloadInfo>::index_type i =
        mPreloads.IndexOf(aRequest, 0, PreloadRequestComparator());
      NS_ASSERTION(i != mPreloads.NoIndex, "Incorrect preload bookkeeping");
      hintCharset = mPreloads[i].mCharset;
    }
    rv = ConvertToUTF16(channel, aString, aStringLen, hintCharset, mDocument,
                        aRequest->mScriptTextBuf, aRequest->mScriptTextLength);

    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  NS_ASSERTION(mDeferRequests.Contains(aRequest) ||
               mLoadingAsyncRequests.Contains(aRequest) ||
               mNonAsyncExternalScriptInsertedRequests.Contains(aRequest) ||
               mXSLTRequests.Contains(aRequest)  ||
               mPreloads.Contains(aRequest, PreloadRequestComparator()) ||
               mParserBlockingRequest,
               "aRequest should be pending!");

  
  aRequest->mLoading = false;

  
  
  
  if (aRequest->mIsAsync) {
    MOZ_ASSERT(aRequest->isInList());
    if (aRequest->isInList()) {
      nsRefPtr<nsScriptLoadRequest> req = mLoadingAsyncRequests.Steal(aRequest);
      mLoadedAsyncRequests.AppendElement(req);
    }
  }

  return NS_OK;
}

void
nsScriptLoader::ParsingComplete(bool aTerminated)
{
  if (mDeferEnabled) {
    
    
    mDocumentParsingDone = true;
  }
  mDeferEnabled = false;
  if (aTerminated) {
    mDeferRequests.Clear();
    mLoadingAsyncRequests.Clear();
    mLoadedAsyncRequests.Clear();
    mNonAsyncExternalScriptInsertedRequests.Clear();
    mXSLTRequests.Clear();
    if (mParserBlockingRequest) {
      mParserBlockingRequest->Cancel();
      mParserBlockingRequest = nullptr;
    }
  }

  
  
  ProcessPendingRequests();
}

void
nsScriptLoader::PreloadURI(nsIURI *aURI, const nsAString &aCharset,
                           const nsAString &aType,
                           const nsAString &aCrossOrigin,
                           bool aScriptFromHead,
                           const mozilla::net::ReferrerPolicy aReferrerPolicy)
{
  
  if (!mEnabled || !mDocument->IsScriptEnabled()) {
    return;
  }

  nsRefPtr<nsScriptLoadRequest> request =
    new nsScriptLoadRequest(nullptr, 0,
                            Element::StringToCORSMode(aCrossOrigin));
  request->mURI = aURI;
  request->mIsInline = false;
  request->mLoading = true;
  request->mReferrerPolicy = aReferrerPolicy;

  nsresult rv = StartLoad(request, aType, aScriptFromHead);
  if (NS_FAILED(rv)) {
    return;
  }

  PreloadInfo *pi = mPreloads.AppendElement();
  pi->mRequest = request;
  pi->mCharset = aCharset;
}

void
nsScriptLoader::AddDeferRequest(nsScriptLoadRequest* aRequest)
{
  aRequest->mIsDefer = true;
  mDeferRequests.AppendElement(aRequest);
  if (mDeferEnabled && aRequest == mDeferRequests.getFirst() &&
      mDocument && !mBlockingDOMContentLoaded) {
    MOZ_ASSERT(mDocument->GetReadyStateEnum() == nsIDocument::READYSTATE_LOADING);
    mBlockingDOMContentLoaded = true;
    mDocument->BlockDOMContentLoaded();
  }
}

bool
nsScriptLoader::MaybeRemovedDeferRequests()
{
  if (mDeferRequests.isEmpty() && mDocument &&
      mBlockingDOMContentLoaded) {
    mBlockingDOMContentLoaded = false;
    mDocument->UnblockDOMContentLoaded();
    return true;
  }
  return false;
}
