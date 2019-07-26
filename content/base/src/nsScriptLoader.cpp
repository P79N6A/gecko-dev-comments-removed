









#include "jsapi.h"
#include "jsfriendapi.h"
#include "nsScriptLoader.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeDecoder.h"
#include "nsIContent.h"
#include "mozilla/dom/Element.h"
#include "nsGkAtoms.h"
#include "nsNetUtil.h"
#include "nsIScriptGlobalObject.h"
#include "nsIScriptContext.h"
#include "nsIScriptRuntime.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsContentPolicyUtils.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
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
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "nsCRT.h"
#include "nsContentCreatorFunctions.h"
#include "mozilla/dom/Element.h"
#include "nsCrossSiteListenerProxy.h"
#include "nsSandboxFlags.h"

#include "mozilla/CORSMode.h"
#include "mozilla/Attributes.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gCspPRLog;
#endif

using namespace mozilla;
using namespace mozilla::dom;





class nsScriptLoadRequest MOZ_FINAL : public nsISupports {
public:
  nsScriptLoadRequest(nsIScriptElement* aElement,
                      uint32_t aVersion,
                      CORSMode aCORSMode)
    : mElement(aElement),
      mLoading(true),
      mIsInline(true),
      mJSVersion(aVersion),
      mLineNo(1),
      mCORSMode(aCORSMode)
  {
  }

  NS_DECL_ISUPPORTS

  void FireScriptAvailable(nsresult aResult)
  {
    mElement->ScriptAvailable(aResult, mElement, mIsInline, mURI, mLineNo);
  }
  void FireScriptEvaluated(nsresult aResult)
  {
    mElement->ScriptEvaluated(aResult, mElement, mIsInline);
  }

  bool IsPreload()
  {
    return mElement == nullptr;
  }

  nsCOMPtr<nsIScriptElement> mElement;
  bool mLoading;             
  bool mIsInline;            
  nsString mScriptText;              
  uint32_t mJSVersion;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIPrincipal> mOriginPrincipal;
  int32_t mLineNo;
  const CORSMode mCORSMode;
};




NS_IMPL_THREADSAFE_ISUPPORTS0(nsScriptLoadRequest)





nsScriptLoader::nsScriptLoader(nsIDocument *aDocument)
  : mDocument(aDocument),
    mBlockerCount(0),
    mEnabled(true),
    mDeferEnabled(false),
    mDocumentParsingDone(false)
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

  for (uint32_t i = 0; i < mXSLTRequests.Length(); i++) {
    mXSLTRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (uint32_t i = 0; i < mDeferRequests.Length(); i++) {
    mDeferRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (uint32_t i = 0; i < mAsyncRequests.Length(); i++) {
    mAsyncRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (uint32_t i = 0; i < mNonAsyncExternalScriptInsertedRequests.Length(); i++) {
    mNonAsyncExternalScriptInsertedRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  
  
  for (uint32_t j = 0; j < mPendingChildLoaders.Length(); ++j) {
    mPendingChildLoaders[j]->RemoveExecuteBlocker();
  }  
}

NS_IMPL_ISUPPORTS1(nsScriptLoader, nsIStreamLoaderObserver)









static bool
IsScriptEventHandler(nsIContent* aScriptElement)
{
  if (!aScriptElement->IsHTML()) {
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

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mDocument->GetScriptGlobalObject()));
  if (!window) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIDocShell *docshell = window->GetDocShell();

  nsCOMPtr<nsIInterfaceRequestor> prompter(do_QueryInterface(docshell));

  
  if (mDocument->GetSandboxFlags() & SANDBOXED_SCRIPTS) {
    return NS_OK;
  }

  
  
  nsCOMPtr<nsIChannelPolicy> channelPolicy;
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = mDocument->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);
  if (csp) {
    channelPolicy = do_CreateInstance("@mozilla.org/nschannelpolicy;1");
    channelPolicy->SetContentSecurityPolicy(csp);
    channelPolicy->SetLoadType(nsIContentPolicy::TYPE_SCRIPT);
  }

  nsCOMPtr<nsIChannel> channel;
  rv = NS_NewChannel(getter_AddRefs(channel),
                     aRequest->mURI, nullptr, loadGroup, prompter,
                     nsIRequest::LOAD_NORMAL | nsIChannel::LOAD_CLASSIFY_URI,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv, rv);

  nsIScriptElement *script = aRequest->mElement;
  if (aScriptFromHead &&
      !(script && (script->GetScriptAsync() || script->GetScriptDeferred()))) {
    nsCOMPtr<nsIHttpChannelInternal>
      internalHttpChannel(do_QueryInterface(channel));
    if (internalHttpChannel)
      internalHttpChannel->SetLoadAsBlocking(true);
  }

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    
    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                  NS_LITERAL_CSTRING("*/*"),
                                  false);
    httpChannel->SetReferrer(mDocument->GetDocumentURI());
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

  
  
  
  
  
  
  
  
  nsIScriptGlobalObject *globalObject = mDocument->GetScriptGlobalObject();
  if (!globalObject) {
    return false;
  }
  
  nsIScriptContext *context = globalObject->GetScriptContext();

  
  
  if (!context || !context->GetScriptsEnabled()) {
    return false;
  }

  JSVersion version = JSVERSION_DEFAULT;

  
  
  nsAutoString type;
  aElement->GetScriptType(type);
  if (!type.IsEmpty()) {
    NS_ENSURE_TRUE(ParseTypeAttribute(type, &version), false);
  } else {
    
    
    
    if (scriptContent->IsHTML()) {
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
          ourCORSMode == request->mCORSMode) {
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
      mAsyncRequests.AppendElement(request);
      if (!request->mLoading) {
        
        
        ProcessPendingRequestsAsync();
      }
      return false;
    }
    if (!aElement->GetParserCreated()) {
      
      
      
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
      mDeferRequests.AppendElement(request);
      return false;
    }

    if (aElement->GetParserCreated() == FROM_PARSER_XSLT) {
      
      NS_ASSERTION(!mParserBlockingRequest,
          "Parser-blocking scripts and XSLT scripts in the same doc!");
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
      NS_ASSERTION(mXSLTRequests.IsEmpty(),
          "Parser-blocking scripts and XSLT scripts in the same doc!");
      mParserBlockingRequest = request;
      ProcessPendingRequestsAsync();
      return true;
    }
    
    
    NS_ASSERTION(!mParserBlockingRequest,
        "There can be only one parser-blocking script at a time");
    NS_ASSERTION(mXSLTRequests.IsEmpty(),
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    mParserBlockingRequest = request;
    return true;
  }

  
  
  if (mDocument->GetSandboxFlags() & SANDBOXED_SCRIPTS) {
    return false;
  }

  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = mDocument->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, false);

  if (csp) {
    PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("New ScriptLoader i ****with CSP****"));
    bool inlineOK = true;
    bool reportViolations = false;
    rv = csp->GetAllowsInlineScript(&reportViolations, &inlineOK);
    NS_ENSURE_SUCCESS(rv, false);

    if (reportViolations) {
      
      nsIURI* uri = mDocument->GetDocumentURI();
      nsAutoCString asciiSpec;
      uri->GetAsciiSpec(asciiSpec);
      nsAutoString scriptText;
      aElement->GetScriptText(scriptText);

      
      if (scriptText.Length() > 40) {
        scriptText.Truncate(40);
        scriptText.Append(NS_LITERAL_STRING("..."));
      }

      csp->LogViolationDetails(nsIContentSecurityPolicy::VIOLATION_TYPE_INLINE_SCRIPT,
                               NS_ConvertUTF8toUTF16(asciiSpec),
                               scriptText,
                               aElement->GetScriptLineNumber());
    }

    if (!inlineOK) {
      PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("CSP blocked inline scripts (2)"));
      return false;
    }
  }

  
  request = new nsScriptLoadRequest(aElement, version, CORS_NONE);
  request->mJSVersion = version;
  request->mLoading = false;
  request->mIsInline = true;
  request->mURI = mDocument->GetDocumentURI();
  request->mLineNo = aElement->GetScriptLineNumber();

  if (aElement->GetParserCreated() == FROM_PARSER_XSLT &&
      (!ReadyToExecuteScripts() || !mXSLTRequests.IsEmpty())) {
    
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
    NS_ASSERTION(mXSLTRequests.IsEmpty(),
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    return true;
  }
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
      "Not safe to run a parser-inserted script?");
  return ProcessRequest(request) == NS_ERROR_HTMLPARSER_BLOCK;
}

nsresult
nsScriptLoader::ProcessRequest(nsScriptLoadRequest* aRequest)
{
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Processing requests when running scripts is unsafe.");

  NS_ENSURE_ARG(aRequest);
  nsAFlatString* script;
  nsAutoString textData;

  nsCOMPtr<nsIDocument> doc;

  nsCOMPtr<nsINode> scriptElem = do_QueryInterface(aRequest->mElement);

  
  if (aRequest->mIsInline) {
    
    
    aRequest->mElement->GetScriptText(textData);

    script = &textData;
  }
  else {
    script = &aRequest->mScriptText;

    doc = scriptElem->OwnerDoc();
  }

  nsCOMPtr<nsIScriptElement> oldParserInsertedScript;
  uint32_t parserCreated = aRequest->mElement->GetParserCreated();
  if (parserCreated) {
    oldParserInsertedScript = mCurrentParserInsertedScript;
    mCurrentParserInsertedScript = aRequest->mElement;
  }

  FireScriptAvailable(NS_OK, aRequest);

  bool runScript = true;
  nsContentUtils::DispatchTrustedEvent(scriptElem->OwnerDoc(),
                                       scriptElem,
                                       NS_LITERAL_STRING("beforescriptexecute"),
                                       true, true, &runScript);

  nsresult rv = NS_OK;
  if (runScript) {
    if (doc) {
      doc->BeginEvaluatingExternalScript();
    }
    aRequest->mElement->BeginEvaluating();
    rv = EvaluateScript(aRequest, *script);
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

nsresult
nsScriptLoader::EvaluateScript(nsScriptLoadRequest* aRequest,
                               const nsAFlatString& aScript)
{
  nsresult rv = NS_OK;

  
  if (!mDocument) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIContent> scriptContent(do_QueryInterface(aRequest->mElement));
  nsIDocument* ownerDoc = scriptContent->OwnerDoc();
  if (ownerDoc != mDocument) {
    
    return NS_ERROR_FAILURE;
  }

  nsPIDOMWindow *pwin = mDocument->GetInnerWindow();
  if (!pwin || !pwin->IsInnerWindow()) {
    return NS_ERROR_FAILURE;
  }
  nsCOMPtr<nsIScriptGlobalObject> globalObject = do_QueryInterface(pwin);
  NS_ASSERTION(globalObject, "windows must be global objects");

  
  NS_ASSERTION(scriptContent, "no content - what is default script-type?");

  
  rv = globalObject->EnsureScriptEnvironment();
  if (NS_FAILED(rv))
    return rv;

  
  
  
  nsCOMPtr<nsIScriptContext> context = globalObject->GetScriptContext();
  if (!context) {
    return NS_ERROR_FAILURE;
  }
  AutoPushJSContext cx(context->GetNativeContext());

  bool oldProcessingScriptTag = context->GetProcessingScriptTag();
  context->SetProcessingScriptTag(true);

  
  nsCOMPtr<nsIScriptElement> oldCurrent = mCurrentScript;
  mCurrentScript = aRequest->mElement;

  
  
  nsAutoCString url;
  nsContentUtils::GetWrapperSafeScriptFilename(mDocument, aRequest->mURI, url);

  JSVersion version = JSVersion(aRequest->mJSVersion);
  if (version != JSVERSION_UNKNOWN) {
    JS::CompileOptions options(cx);
    options.setFileAndLine(url.get(), aRequest->mLineNo)
           .setVersion(JSVersion(aRequest->mJSVersion));
    if (aRequest->mOriginPrincipal) {
      options.setOriginPrincipals(nsJSPrincipals::get(aRequest->mOriginPrincipal));
    }
    rv = context->EvaluateString(aScript, *globalObject->GetGlobalJSObject(),
                                 options,  false, nullptr);
  }

  
  mCurrentScript = oldCurrent;

  JSAutoRequest ar(cx);
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
         !mXSLTRequests.IsEmpty() && 
         !mXSLTRequests[0]->mLoading) {
    request.swap(mXSLTRequests[0]);
    mXSLTRequests.RemoveElementAt(0);
    ProcessRequest(request);
  }

  uint32_t i = 0;
  while (mEnabled && i < mAsyncRequests.Length()) {
    if (!mAsyncRequests[i]->mLoading) {
      request.swap(mAsyncRequests[i]);
      mAsyncRequests.RemoveElementAt(i);
      ProcessRequest(request);
      continue;
    }
    ++i;
  }

  while (mEnabled && !mNonAsyncExternalScriptInsertedRequests.IsEmpty() &&
         !mNonAsyncExternalScriptInsertedRequests[0]->mLoading) {
    
    
    
    
    request.swap(mNonAsyncExternalScriptInsertedRequests[0]);
    mNonAsyncExternalScriptInsertedRequests.RemoveElementAt(0);
    ProcessRequest(request);
  }

  if (mDocumentParsingDone && mXSLTRequests.IsEmpty()) {
    while (!mDeferRequests.IsEmpty() && !mDeferRequests[0]->mLoading) {
      request.swap(mDeferRequests[0]);
      mDeferRequests.RemoveElementAt(0);
      ProcessRequest(request);
    }
  }

  while (!mPendingChildLoaders.IsEmpty() && ReadyToExecuteScripts()) {
    nsRefPtr<nsScriptLoader> child = mPendingChildLoaders[0];
    mPendingChildLoaders.RemoveElementAt(0);
    child->RemoveExecuteBlocker();
  }

  if (mDocumentParsingDone && mDocument &&
      !mParserBlockingRequest && mAsyncRequests.IsEmpty() &&
      mNonAsyncExternalScriptInsertedRequests.IsEmpty() &&
      mXSLTRequests.IsEmpty() && mDeferRequests.IsEmpty()) {
    
    
    
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
      
      
      oCharset.Assign("UTF-8");
    }
    break;
  case 0xFE:
    if (0xFF == aBytes[1]) {
      
      
      oCharset.Assign("UTF-16");
    }
    break;
  case 0xFF:
    if (0xFE == aBytes[1]) {
      
      
      oCharset.Assign("UTF-16");
    }
    break;
  }
  return !oCharset.IsEmpty();
}

 nsresult
nsScriptLoader::ConvertToUTF16(nsIChannel* aChannel, const uint8_t* aData,
                               uint32_t aLength, const nsAString& aHintCharset,
                               nsIDocument* aDocument, nsString& aString)
{
  if (!aLength) {
    aString.Truncate();
    return NS_OK;
  }

  
  
  
  
  

  nsAutoCString charset;

  nsCOMPtr<nsICharsetConverterManager> charsetConv =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;

  if (DetectByteOrderMark(aData, aLength, charset)) {
    
    
    
    charsetConv->GetUnicodeDecoderRaw(charset.get(),
                                      getter_AddRefs(unicodeDecoder));
  }

  if (!unicodeDecoder &&
      aChannel &&
      NS_SUCCEEDED(aChannel->GetContentCharset(charset)) &&
      !charset.IsEmpty()) {
    charsetConv->GetUnicodeDecoder(charset.get(),
                                   getter_AddRefs(unicodeDecoder));
  }

  if (!unicodeDecoder && !aHintCharset.IsEmpty()) {
    CopyUTF16toUTF8(aHintCharset, charset);
    charsetConv->GetUnicodeDecoder(charset.get(),
                                   getter_AddRefs(unicodeDecoder));
  }

  if (!unicodeDecoder && aDocument) {
    charset = aDocument->GetDocumentCharacterSet();
    charsetConv->GetUnicodeDecoderRaw(charset.get(),
                                      getter_AddRefs(unicodeDecoder));
  }

  if (!unicodeDecoder) {
    
    
    
    
    charsetConv->GetUnicodeDecoderRaw("windows-1252",
                                      getter_AddRefs(unicodeDecoder));
  }

  int32_t unicodeLength = 0;

  nsresult rv =
    unicodeDecoder->GetMaxLength(reinterpret_cast<const char*>(aData),
                                 aLength, &unicodeLength);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!EnsureStringLength(aString, unicodeLength)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  PRUnichar *ustr = aString.BeginWriting();

  rv = unicodeDecoder->Convert(reinterpret_cast<const char*>(aData),
                               (int32_t *) &aLength, ustr,
                               &unicodeLength);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  aString.SetLength(unicodeLength);
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
    if (mDeferRequests.RemoveElement(request) ||
        mAsyncRequests.RemoveElement(request) ||
        mNonAsyncExternalScriptInsertedRequests.RemoveElement(request) ||
        mXSLTRequests.RemoveElement(request)) {
      FireScriptAvailable(rv, request);
    } else if (mParserBlockingRequest == request) {
      mParserBlockingRequest = nullptr;
      UnblockParser(request);
      FireScriptAvailable(rv, request);
      ContinueParserAsync(request);
    } else {
      mPreloads.RemoveElement(request, PreloadRequestComparator());
    }
  }

  
  ProcessPendingRequests();

  return NS_OK;
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
  }

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(req);
  
  
  
  if (aRequest->mCORSMode == CORS_NONE) {
    rv = nsContentUtils::GetSecurityManager()->
      GetChannelPrincipal(channel, getter_AddRefs(aRequest->mOriginPrincipal));
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
                        aRequest->mScriptText);

    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  NS_ASSERTION(mDeferRequests.Contains(aRequest) ||
               mAsyncRequests.Contains(aRequest) ||
               mNonAsyncExternalScriptInsertedRequests.Contains(aRequest) ||
               mXSLTRequests.Contains(aRequest)  ||
               mPreloads.Contains(aRequest, PreloadRequestComparator()) ||
               mParserBlockingRequest,
               "aRequest should be pending!");

  
  aRequest->mLoading = false;

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
    mAsyncRequests.Clear();
    mNonAsyncExternalScriptInsertedRequests.Clear();
    mXSLTRequests.Clear();
    mParserBlockingRequest = nullptr;
  }

  
  
  ProcessPendingRequests();
}

void
nsScriptLoader::PreloadURI(nsIURI *aURI, const nsAString &aCharset,
                           const nsAString &aType,
                           const nsAString &aCrossOrigin,
                           bool aScriptFromHead)
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
  nsresult rv = StartLoad(request, aType, aScriptFromHead);
  if (NS_FAILED(rv)) {
    return;
  }

  PreloadInfo *pi = mPreloads.AppendElement();
  pi->mRequest = request;
  pi->mCharset = aCharset;
}
