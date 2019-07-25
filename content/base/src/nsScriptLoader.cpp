










































#include "jscntxt.h"
#include "nsScriptLoader.h"
#include "nsIDOMCharacterData.h"
#include "nsParserUtils.h"
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
#include "nsContentPolicyUtils.h"
#include "nsIDOMWindow.h"
#include "nsIHttpChannel.h"
#include "nsIScriptElement.h"
#include "nsIDOMHTMLScriptElement.h"
#include "nsIDocShell.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsAutoPtr.h"
#include "nsIXPConnect.h"
#include "nsContentErrors.h"
#include "nsIParser.h"
#include "nsThreadUtils.h"
#include "nsDocShellCID.h"
#include "nsIContentSecurityPolicy.h"
#include "prlog.h"
#include "nsIChannelPolicy.h"
#include "nsChannelPolicy.h"
#include "nsCRT.h"
#include "nsContentCreatorFunctions.h"

#include "mozilla/FunctionTimer.h"

#ifdef PR_LOGGING
static PRLogModuleInfo* gCspPRLog;
#endif

using namespace mozilla::dom;





class nsScriptLoadRequest : public nsISupports {
public:
  nsScriptLoadRequest(nsIScriptElement* aElement,
                      PRUint32 aVersion)
    : mElement(aElement),
      mLoading(PR_TRUE),
      mIsInline(PR_TRUE),
      mJSVersion(aVersion), mLineNo(1)
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

  PRBool IsPreload()
  {
    return mElement == nsnull;
  }

  nsCOMPtr<nsIScriptElement> mElement;
  PRPackedBool mLoading;             
  PRPackedBool mIsInline;            
  nsString mScriptText;              
  PRUint32 mJSVersion;
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIURI> mFinalURI;
  PRInt32 mLineNo;
};




NS_IMPL_THREADSAFE_ISUPPORTS0(nsScriptLoadRequest)





nsScriptLoader::nsScriptLoader(nsIDocument *aDocument)
  : mDocument(aDocument),
    mBlockerCount(0),
    mEnabled(PR_TRUE),
    mDeferEnabled(PR_FALSE),
    mDocumentParsingDone(PR_FALSE)
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

  for (PRUint32 i = 0; i < mXSLTRequests.Length(); i++) {
    mXSLTRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (PRUint32 i = 0; i < mDeferRequests.Length(); i++) {
    mDeferRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (PRUint32 i = 0; i < mAsyncRequests.Length(); i++) {
    mAsyncRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  for (PRUint32 i = 0; i < mNonAsyncExternalScriptInsertedRequests.Length(); i++) {
    mNonAsyncExternalScriptInsertedRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
  }

  
  
  for (PRUint32 j = 0; j < mPendingChildLoaders.Length(); ++j) {
    mPendingChildLoaders[j]->RemoveExecuteBlocker();
  }  
}

NS_IMPL_ISUPPORTS1(nsScriptLoader, nsIStreamLoaderObserver)









static PRBool
IsScriptEventHandler(nsIScriptElement *aScriptElement)
{
  nsCOMPtr<nsIContent> contElement = do_QueryInterface(aScriptElement);
  NS_ASSERTION(contElement, "nsIScriptElement isn't nsIContent");

  nsAutoString forAttr, eventAttr;
  if (!contElement->GetAttr(kNameSpaceID_None, nsGkAtoms::_for, forAttr) ||
      !contElement->GetAttr(kNameSpaceID_None, nsGkAtoms::event, eventAttr)) {
    return PR_FALSE;
  }

  const nsAString& for_str =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(forAttr);
  if (!for_str.LowerCaseEqualsLiteral("window")) {
    return PR_TRUE;
  }

  
  const nsAString& event_str =
    nsContentUtils::TrimWhitespace<nsCRT::IsAsciiSpace>(eventAttr, PR_FALSE);
  if (!StringBeginsWith(event_str, NS_LITERAL_STRING("onload"),
                        nsCaseInsensitiveStringComparator())) {
    

    return PR_TRUE;
  }

  nsAutoString::const_iterator start, end;
  event_str.BeginReading(start);
  event_str.EndReading(end);

  start.advance(6); 

  if (start != end && *start != '(' && *start != ' ') {
    
    

    return PR_TRUE;
  }

  return PR_FALSE;
}

nsresult
nsScriptLoader::CheckContentPolicy(nsIDocument* aDocument,
                                   nsISupports *aContext,
                                   nsIURI *aURI,
                                   const nsAString &aType)
{
  PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_SCRIPT,
                                          aURI,
                                          aDocument->NodePrincipal(),
                                          aContext,
                                          NS_LossyConvertUTF16toASCII(aType),
                                          nsnull,    
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
nsScriptLoader::StartLoad(nsScriptLoadRequest *aRequest, const nsAString &aType)
{
  nsISupports *context = aRequest->mElement.get()
                         ? static_cast<nsISupports *>(aRequest->mElement.get())
                         : static_cast<nsISupports *>(mDocument);
  nsresult rv = ShouldLoadScript(mDocument, context, aRequest->mURI, aType);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsILoadGroup> loadGroup = mDocument->GetDocumentLoadGroup();
  nsCOMPtr<nsIStreamLoader> loader;

  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mDocument->GetScriptGlobalObject()));
  if (!window) {
    return NS_ERROR_NULL_POINTER;
  }

  nsIDocShell *docshell = window->GetDocShell();

  nsCOMPtr<nsIInterfaceRequestor> prompter(do_QueryInterface(docshell));

  
  
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
                     aRequest->mURI, nsnull, loadGroup, prompter,
                     nsIRequest::LOAD_NORMAL | nsIChannel::LOAD_CLASSIFY_URI,
                     channelPolicy);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel(do_QueryInterface(channel));
  if (httpChannel) {
    
    httpChannel->SetRequestHeader(NS_LITERAL_CSTRING("Accept"),
                                  NS_LITERAL_CSTRING("*/*"),
                                  PR_FALSE);
    httpChannel->SetReferrer(mDocument->GetDocumentURI());
  }

  rv = NS_NewStreamLoader(getter_AddRefs(loader), this);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = channel->AsyncOpen(loader, aRequest);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

PRBool
nsScriptLoader::PreloadURIComparator::Equals(const PreloadInfo &aPi,
                                             nsIURI * const &aURI) const
{
  PRBool same;
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

nsresult
nsScriptLoader::ProcessScriptElement(nsIScriptElement *aElement)
{
  
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  
  if (!mEnabled || !mDocument->IsScriptEnabled()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  NS_ASSERTION(!aElement->IsMalformed(), "Executing malformed script");

  
  if (IsScriptEventHandler(aElement)) {
    return NS_CONTENT_SCRIPT_IS_EVENTHANDLER;
  }

  
  
  
  
  
  
  
  
  nsIScriptGlobalObject *globalObject = mDocument->GetScriptGlobalObject();
  if (!globalObject) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  nsIScriptContext *context = globalObject->GetScriptContext(
                                        nsIProgrammingLanguage::JAVASCRIPT);

  
  
  if (!context || !context->GetScriptsEnabled()) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  
  
  Element* rootElement = mDocument->GetRootElement();
  PRUint32 typeID = rootElement ? rootElement->GetScriptTypeID() :
                                  context->GetScriptTypeID();
  PRUint32 version = 0;
  nsAutoString language, type, src;
  nsresult rv = NS_OK;

  
  
  aElement->GetScriptType(type);
  if (!type.IsEmpty()) {
    nsContentTypeParser parser(type);

    nsAutoString mimeType;
    rv = parser.GetType(mimeType);
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    
    static const char *jsTypes[] = {
      "text/javascript",
      "text/ecmascript",
      "application/javascript",
      "application/ecmascript",
      "application/x-javascript",
      nsnull
    };

    PRBool isJavaScript = PR_FALSE;
    for (PRInt32 i = 0; jsTypes[i]; i++) {
      if (mimeType.LowerCaseEqualsASCII(jsTypes[i])) {
        isJavaScript = PR_TRUE;
        break;
      }
    }
    if (isJavaScript)
      typeID = nsIProgrammingLanguage::JAVASCRIPT;
    else {
      
      nsCOMPtr<nsIScriptRuntime> runtime;
      rv = NS_GetScriptRuntime(mimeType, getter_AddRefs(runtime));
      if (NS_FAILED(rv) || runtime == nsnull) {
        
        NS_WARNING("Failed to find a scripting language");
        typeID = nsIProgrammingLanguage::UNKNOWN;
      } else
        typeID = runtime->GetScriptTypeID();
    }
    if (typeID != nsIProgrammingLanguage::UNKNOWN) {
      
      nsAutoString versionName;
      rv = parser.GetParameter("version", versionName);
      if (NS_FAILED(rv)) {
        
        if (rv != NS_ERROR_INVALID_ARG)
          return rv;
      } else {
        nsCOMPtr<nsIScriptRuntime> runtime;
        rv = NS_GetScriptRuntimeByID(typeID, getter_AddRefs(runtime));
        if (NS_FAILED(rv)) {
          NS_ERROR("Failed to locate the language with this ID");
          return rv;
        }
        rv = runtime->ParseVersion(versionName, &version);
        if (NS_FAILED(rv)) {
          NS_WARNING("This script language version is not supported - ignored");
          typeID = nsIProgrammingLanguage::UNKNOWN;
        }
      }
    }

    
    if (typeID == nsIProgrammingLanguage::JAVASCRIPT) {
      nsAutoString value;
      rv = parser.GetParameter("e4x", value);
      if (NS_FAILED(rv)) {
        if (rv != NS_ERROR_INVALID_ARG)
          return rv;
      } else {
        if (value.Length() == 1 && value[0] == '1')
          
          
          
          
          version |= js::VersionFlags::HAS_XML;
      }
    }
  } else {
    
    
    
    nsCOMPtr<nsIDOMHTMLScriptElement> htmlScriptElement =
      do_QueryInterface(aElement);
    if (htmlScriptElement) {
      htmlScriptElement->GetAttribute(NS_LITERAL_STRING("language"), language);
      if (!language.IsEmpty()) {
        if (nsParserUtils::IsJavaScriptLanguage(language, &version))
          typeID = nsIProgrammingLanguage::JAVASCRIPT;
        else
          typeID = nsIProgrammingLanguage::UNKNOWN;
        
        
        
        
        
        
        NS_ASSERTION(JSVERSION_DEFAULT == 0,
                     "We rely on all languages having 0 as a version default");
        version = 0;
      }
    }
  }

  
  if (typeID == nsIProgrammingLanguage::UNKNOWN) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  
  
  
  
  
  if (typeID != nsIProgrammingLanguage::JAVASCRIPT &&
      !nsContentUtils::IsChromeDoc(mDocument)) {
    NS_WARNING("Untrusted language called from non-chrome - ignored");
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsCOMPtr<nsIContent> eltContent(do_QueryInterface(aElement));
  eltContent->SetScriptTypeID(typeID);

  

  nsRefPtr<nsScriptLoadRequest> request;
  if (aElement->GetScriptExternal()) {
    
    nsCOMPtr<nsIURI> scriptURI = aElement->GetScriptURI();
    if (!scriptURI) {
      return NS_ERROR_NOT_AVAILABLE;
    }
    nsTArray<PreloadInfo>::index_type i =
      mPreloads.IndexOf(scriptURI.get(), 0, PreloadURIComparator());
    if (i != nsTArray<PreloadInfo>::NoIndex) {
      
      
      request = mPreloads[i].mRequest;
      request->mElement = aElement;
      nsString preloadCharset(mPreloads[i].mCharset);
      mPreloads.RemoveElementAt(i);

      
      
      nsAutoString elementCharset;
      aElement->GetScriptCharset(elementCharset);
      if (elementCharset.Equals(preloadCharset)) {
        rv = CheckContentPolicy(mDocument, aElement, request->mURI, type);
        NS_ENSURE_SUCCESS(rv, rv);
      } else {
        
        request = nsnull;
      }
    }

    if (!request) {
      
      request = new nsScriptLoadRequest(aElement, version);
      NS_ENSURE_TRUE(request, NS_ERROR_OUT_OF_MEMORY);
      request->mURI = scriptURI;
      request->mIsInline = PR_FALSE;
      request->mLoading = PR_TRUE;
      rv = StartLoad(request, type);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    request->mJSVersion = version;

    if (aElement->GetScriptAsync()) {
      mAsyncRequests.AppendElement(request);
      if (!request->mLoading) {
        
        
        ProcessPendingRequestsAsync();
      }
      return NS_OK;
    }
    if (!aElement->GetParserCreated()) {
      
      
      
      mNonAsyncExternalScriptInsertedRequests.AppendElement(request);
      if (!request->mLoading) {
        
        
        ProcessPendingRequestsAsync();
      }
      return NS_OK;
    }
    
    
    if (aElement->GetScriptDeferred()) {
      
      
      
      
      
      
      NS_ASSERTION(mDocument->GetCurrentContentSink() ||
                   aElement->GetParserCreated() == FROM_PARSER_XSLT,
          "Non-XSLT Defer script on a document without an active parser; bug 592366.");
      mDeferRequests.AppendElement(request);
      return NS_OK;
    }

    if (aElement->GetParserCreated() == FROM_PARSER_XSLT) {
      
      NS_ASSERTION(!mParserBlockingRequest,
          "Parser-blocking scripts and XSLT scripts in the same doc!");
      mXSLTRequests.AppendElement(request);
      if (!request->mLoading) {
        
        
        ProcessPendingRequestsAsync();
      }
      return NS_ERROR_HTMLPARSER_BLOCK;
    }
    if (!request->mLoading && ReadyToExecuteScripts()) {
      
      
      
      if (aElement->GetParserCreated() == FROM_PARSER_NETWORK) {
        return ProcessRequest(request);
      }
      
      
      
      NS_ASSERTION(!mParserBlockingRequest,
          "There can be only one parser-blocking script at a time");
      NS_ASSERTION(mXSLTRequests.IsEmpty(),
          "Parser-blocking scripts and XSLT scripts in the same doc!");
      mParserBlockingRequest = request;
      ProcessPendingRequestsAsync();
      return NS_ERROR_HTMLPARSER_BLOCK;
    }
    
    
    NS_ASSERTION(!mParserBlockingRequest,
        "There can be only one parser-blocking script at a time");
    NS_ASSERTION(mXSLTRequests.IsEmpty(),
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    mParserBlockingRequest = request;
    return NS_ERROR_HTMLPARSER_BLOCK;
  }

  
  nsCOMPtr<nsIContentSecurityPolicy> csp;
  rv = mDocument->NodePrincipal()->GetCsp(getter_AddRefs(csp));
  NS_ENSURE_SUCCESS(rv, rv);

  if (csp) {
    PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("New ScriptLoader i ****with CSP****"));
    PRBool inlineOK;
    rv = csp->GetAllowsInlineScript(&inlineOK);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!inlineOK) {
      PR_LOG(gCspPRLog, PR_LOG_DEBUG, ("CSP blocked inline scripts (2)"));
      
      nsIURI* uri = mDocument->GetDocumentURI();
      nsCAutoString asciiSpec;
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
      return NS_ERROR_FAILURE;
    }
  }

  request = new nsScriptLoadRequest(aElement, version);
  NS_ENSURE_TRUE(request, NS_ERROR_OUT_OF_MEMORY);
  request->mJSVersion = version;
  request->mLoading = PR_FALSE;
  request->mIsInline = PR_TRUE;
  request->mURI = mDocument->GetDocumentURI();
  request->mLineNo = aElement->GetScriptLineNumber();

  if (aElement->GetParserCreated() == FROM_PARSER_XSLT &&
      (!ReadyToExecuteScripts() || !mXSLTRequests.IsEmpty())) {
    
    NS_ASSERTION(!mParserBlockingRequest,
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    mXSLTRequests.AppendElement(request);
    return NS_ERROR_HTMLPARSER_BLOCK;
  }
  if (aElement->GetParserCreated() == NOT_FROM_PARSER) {
    NS_ASSERTION(!nsContentUtils::IsSafeToRunScript(),
        "A script-inserted script is inserted without an update batch?");
    nsContentUtils::AddScriptRunner(new nsScriptRequestProcessor(this,
                                                                 request));
    return NS_OK;
  }
  if (aElement->GetParserCreated() == FROM_PARSER_NETWORK &&
      !ReadyToExecuteScripts()) {
    NS_ASSERTION(!mParserBlockingRequest,
        "There can be only one parser-blocking script at a time");
    mParserBlockingRequest = request;
    NS_ASSERTION(mXSLTRequests.IsEmpty(),
        "Parser-blocking scripts and XSLT scripts in the same doc!");
    return NS_ERROR_HTMLPARSER_BLOCK;
  }
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
      "Not safe to run a parser-inserted script?");
  return ProcessRequest(request);
}

nsresult
nsScriptLoader::ProcessRequest(nsScriptLoadRequest* aRequest)
{
  NS_ASSERTION(nsContentUtils::IsSafeToRunScript(),
               "Processing requests when running scripts is unsafe.");

  NS_ENSURE_ARG(aRequest);
  nsAFlatString* script;
  nsAutoString textData;

  NS_TIME_FUNCTION;

  nsCOMPtr<nsIDocument> doc;

  nsCOMPtr<nsINode> scriptElem = do_QueryInterface(aRequest->mElement);

  
  if (aRequest->mIsInline) {
    
    
    aRequest->mElement->GetScriptText(textData);

    script = &textData;
  }
  else {
    script = &aRequest->mScriptText;

    doc = scriptElem->GetOwnerDoc();
  }

  nsCOMPtr<nsIScriptElement> oldParserInsertedScript;
  PRUint32 parserCreated = aRequest->mElement->GetParserCreated();
  if (parserCreated) {
    oldParserInsertedScript = mCurrentParserInsertedScript;
    mCurrentParserInsertedScript = aRequest->mElement;
  }

  FireScriptAvailable(NS_OK, aRequest);

  PRBool runScript = PR_TRUE;
  nsContentUtils::DispatchTrustedEvent(scriptElem->GetOwnerDoc(),
                                       scriptElem,
                                       NS_LITERAL_STRING("beforescriptexecute"),
                                       PR_TRUE, PR_TRUE, &runScript);

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

    nsContentUtils::DispatchTrustedEvent(scriptElem->GetOwnerDoc(),
                                         scriptElem,
                                         NS_LITERAL_STRING("afterscriptexecute"),
                                         PR_TRUE, PR_FALSE);
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
  for (PRInt32 i = 0; i < mObservers.Count(); i++) {
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
  for (PRInt32 i = 0; i < mObservers.Count(); i++) {
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
  nsIDocument* ownerDoc = scriptContent->GetOwnerDoc();
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
  PRUint32 stid = scriptContent ? scriptContent->GetScriptTypeID() :
                                  nsIProgrammingLanguage::JAVASCRIPT;
  
  rv = globalObject->EnsureScriptEnvironment(stid);
  if (NS_FAILED(rv))
    return rv;

  
  
  
  nsCOMPtr<nsIScriptContext> context = globalObject->GetScriptContext(stid);
  if (!context) {
    return NS_ERROR_FAILURE;
  }

  nsIURI* uri = aRequest->mFinalURI ? aRequest->mFinalURI : aRequest->mURI;

  PRBool oldProcessingScriptTag = context->GetProcessingScriptTag();
  context->SetProcessingScriptTag(PR_TRUE);

  
  nsCOMPtr<nsIScriptElement> oldCurrent = mCurrentScript;
  mCurrentScript = aRequest->mElement;

  nsCAutoString url;
  nsContentUtils::GetWrapperSafeScriptFilename(mDocument, uri, url);

  PRBool isUndefined;
  rv = context->EvaluateString(aScript,
                          globalObject->GetScriptGlobal(stid),
                          mDocument->NodePrincipal(), url.get(),
                          aRequest->mLineNo, aRequest->mJSVersion, nsnull,
                          &isUndefined);

  
  mCurrentScript = oldCurrent;

  JSContext *cx = nsnull; 
  if (stid == nsIProgrammingLanguage::JAVASCRIPT) {
    cx = context->GetNativeContext();
    ::JS_BeginRequest(cx);
    NS_ASSERTION(!::JS_IsExceptionPending(cx),
                 "JS_ReportPendingException wasn't called in EvaluateString");
  }

  context->SetProcessingScriptTag(oldProcessingScriptTag);

  if (stid == nsIProgrammingLanguage::JAVASCRIPT) {
    NS_ASSERTION(!::JS_IsExceptionPending(cx),
                 "JS_ReportPendingException wasn't called");
    ::JS_EndRequest(cx);
  }
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
    
    ProcessRequest(request);
  }

  while (ReadyToExecuteScripts() && 
         !mXSLTRequests.IsEmpty() && 
         !mXSLTRequests[0]->mLoading) {
    request.swap(mXSLTRequests[0]);
    mXSLTRequests.RemoveElementAt(0);
    ProcessRequest(request);
  }

  PRUint32 i = 0;
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
    
    
    
    mDocumentParsingDone = PR_FALSE;
    mDocument->UnblockOnload(PR_TRUE);
  }
}

PRBool
nsScriptLoader::ReadyToExecuteScripts()
{
  
  
  if (!SelfReadyToExecuteScripts()) {
    return PR_FALSE;
  }
  
  for (nsIDocument* doc = mDocument; doc; doc = doc->GetParentDocument()) {
    nsScriptLoader* ancestor = doc->ScriptLoader();
    if (!ancestor->SelfReadyToExecuteScripts() &&
        ancestor->AddPendingChildLoader(this)) {
      AddExecuteBlocker();
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}



static PRBool
DetectByteOrderMark(const unsigned char* aBytes, PRInt32 aLen, nsCString& oCharset)
{
  if (aLen < 2)
    return PR_FALSE;

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
nsScriptLoader::ConvertToUTF16(nsIChannel* aChannel, const PRUint8* aData,
                               PRUint32 aLength, const nsString& aHintCharset,
                               nsIDocument* aDocument, nsString& aString)
{
  if (!aLength) {
    aString.Truncate();
    return NS_OK;
  }

  nsCAutoString characterSet;

  nsresult rv = NS_OK;
  if (aChannel) {
    rv = aChannel->GetContentCharset(characterSet);
  }

  if (!aHintCharset.IsEmpty() && (NS_FAILED(rv) || characterSet.IsEmpty())) {
    
    LossyCopyUTF16toASCII(aHintCharset, characterSet);
  }

  if (NS_FAILED(rv) || characterSet.IsEmpty()) {
    DetectByteOrderMark(aData, aLength, characterSet);
  }

  if (characterSet.IsEmpty() && aDocument) {
    
    characterSet = aDocument->GetDocumentCharacterSet();
  }

  if (characterSet.IsEmpty()) {
    
    characterSet.AssignLiteral("ISO-8859-1");
  }

  nsCOMPtr<nsICharsetConverterManager> charsetConv =
    do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv);

  nsCOMPtr<nsIUnicodeDecoder> unicodeDecoder;

  if (NS_SUCCEEDED(rv) && charsetConv) {
    rv = charsetConv->GetUnicodeDecoder(characterSet.get(),
                                        getter_AddRefs(unicodeDecoder));
    if (NS_FAILED(rv)) {
      
      rv = charsetConv->GetUnicodeDecoderRaw("ISO-8859-1",
                                             getter_AddRefs(unicodeDecoder));
    }
  }

  
  if (NS_SUCCEEDED(rv)) {
    PRInt32 unicodeLength = 0;

    rv = unicodeDecoder->GetMaxLength(reinterpret_cast<const char*>(aData),
                                      aLength, &unicodeLength);
    if (NS_SUCCEEDED(rv)) {
      if (!EnsureStringLength(aString, unicodeLength))
        return NS_ERROR_OUT_OF_MEMORY;

      PRUnichar *ustr = aString.BeginWriting();

      PRInt32 consumedLength = 0;
      PRInt32 originalLength = aLength;
      PRInt32 convertedLength = 0;
      PRInt32 bufferLength = unicodeLength;
      do {
        rv = unicodeDecoder->Convert(reinterpret_cast<const char*>(aData),
                                     (PRInt32 *) &aLength, ustr,
                                     &unicodeLength);
        if (NS_FAILED(rv)) {
          
          
          ustr[unicodeLength++] = (PRUnichar)0xFFFD;
          ustr += unicodeLength;

          unicodeDecoder->Reset();
        }
        aData += ++aLength;
        consumedLength += aLength;
        aLength = originalLength - consumedLength;
        convertedLength += unicodeLength;
        unicodeLength = bufferLength - convertedLength;
      } while (NS_FAILED(rv) && (originalLength > consumedLength) && (bufferLength > convertedLength));
      aString.SetLength(convertedLength);
    }
  }
  return rv;
}

NS_IMETHODIMP
nsScriptLoader::OnStreamComplete(nsIStreamLoader* aLoader,
                                 nsISupports* aContext,
                                 nsresult aStatus,
                                 PRUint32 aStringLen,
                                 const PRUint8* aString)
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
      mParserBlockingRequest = nsnull;
      
      FireScriptAvailable(rv, request);
    } else {
      mPreloads.RemoveElement(request, PreloadRequestComparator());
    }
  }

  
  ProcessPendingRequests();

  return NS_OK;
}

nsresult
nsScriptLoader::PrepareLoadedRequest(nsScriptLoadRequest* aRequest,
                                     nsIStreamLoader* aLoader,
                                     nsresult aStatus,
                                     PRUint32 aStringLen,
                                     const PRUint8* aString)
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
    PRBool requestSucceeded;
    rv = httpChannel->GetRequestSucceeded(&requestSucceeded);
    if (NS_SUCCEEDED(rv) && !requestSucceeded) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  nsCOMPtr<nsIChannel> channel = do_QueryInterface(req);
  NS_GetFinalChannelURI(channel, getter_AddRefs(aRequest->mFinalURI));
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

    if (!ShouldExecuteScript(mDocument, channel)) {
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  
  
  
  
  NS_ASSERTION(mDeferRequests.IndexOf(aRequest) >= 0 ||
               mAsyncRequests.IndexOf(aRequest) >= 0 ||
               mNonAsyncExternalScriptInsertedRequests.IndexOf(aRequest) >= 0 ||
               mXSLTRequests.IndexOf(aRequest) >= 0 ||
               mPreloads.Contains(aRequest, PreloadRequestComparator()) ||
               mParserBlockingRequest,
               "aRequest should be pending!");

  
  aRequest->mLoading = PR_FALSE;

  return NS_OK;
}


PRBool
nsScriptLoader::ShouldExecuteScript(nsIDocument* aDocument,
                                    nsIChannel* aChannel)
{
  if (!aChannel) {
    return PR_FALSE;
  }

  PRBool hasCert;
  nsIPrincipal* docPrincipal = aDocument->NodePrincipal();
  docPrincipal->GetHasCertificate(&hasCert);
  if (!hasCert) {
    return PR_TRUE;
  }

  nsCOMPtr<nsIPrincipal> channelPrincipal;
  nsresult rv = nsContentUtils::GetSecurityManager()->
    GetChannelPrincipal(aChannel, getter_AddRefs(channelPrincipal));
  NS_ENSURE_SUCCESS(rv, PR_FALSE);

  NS_ASSERTION(channelPrincipal, "Gotta have a principal here!");

  
  
  PRBool subsumes;
  rv = channelPrincipal->Subsumes(docPrincipal, &subsumes);
  return NS_SUCCEEDED(rv) && subsumes;
}

void
nsScriptLoader::ParsingComplete(PRBool aTerminated)
{
  if (mDeferEnabled) {
    
    
    mDocumentParsingDone = PR_TRUE;
  }
  mDeferEnabled = PR_FALSE;
  if (aTerminated) {
    mDeferRequests.Clear();
    mAsyncRequests.Clear();
    mNonAsyncExternalScriptInsertedRequests.Clear();
    mXSLTRequests.Clear();
    mParserBlockingRequest = nsnull;
  }

  
  
  ProcessPendingRequests();
}

void
nsScriptLoader::PreloadURI(nsIURI *aURI, const nsAString &aCharset,
                           const nsAString &aType)
{
  
  if (!mEnabled || !mDocument->IsScriptEnabled()) {
    return;
  }

  nsRefPtr<nsScriptLoadRequest> request = new nsScriptLoadRequest(nsnull, 0);
  request->mURI = aURI;
  request->mIsInline = PR_FALSE;
  request->mLoading = PR_TRUE;
  nsresult rv = StartLoad(request, aType);
  if (NS_FAILED(rv)) {
    return;
  }

  PreloadInfo *pi = mPreloads.AppendElement();
  pi->mRequest = request;
  pi->mCharset = aCharset;
}
