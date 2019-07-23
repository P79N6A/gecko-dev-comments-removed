










































#include "nsScriptLoader.h"
#include "nsIDOMCharacterData.h"
#include "nsParserUtils.h"
#include "nsIMIMEHeaderParam.h"
#include "nsICharsetConverterManager.h"
#include "nsIUnicodeDecoder.h"
#include "nsIContent.h"
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
#include "jscntxt.h"
#include "nsContentUtils.h"
#include "nsUnicharUtils.h"
#include "nsAutoPtr.h"
#include "nsIXPConnect.h"
#include "nsContentErrors.h"
#include "nsIParser.h"
#include "nsThreadUtils.h"









static already_AddRefed<nsIPrincipal>
MaybeDowngradeToCodebase(nsIPrincipal *aMaybeCertPrincipal,
                         nsIPrincipal *aNewPrincipal)
{
  NS_PRECONDITION(aMaybeCertPrincipal, "Null old principal!");
  NS_PRECONDITION(aNewPrincipal, "Null new principal!");

  nsIPrincipal *principal = aMaybeCertPrincipal;

  PRBool hasCert;
  aMaybeCertPrincipal->GetHasCertificate(&hasCert);
  if (hasCert) {
    PRBool equal;
    aMaybeCertPrincipal->Equals(aNewPrincipal, &equal);
    if (!equal) {
      nsCOMPtr<nsIURI> uri, domain;
      aMaybeCertPrincipal->GetURI(getter_AddRefs(uri));
      aMaybeCertPrincipal->GetDomain(getter_AddRefs(domain));

      nsContentUtils::GetSecurityManager()->GetCodebasePrincipal(uri,
                                                                 &principal);
      if (principal && domain) {
        principal->SetDomain(domain);
      }

      return principal;
    }
  }

  NS_ADDREF(principal);

  return principal;
}





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

  nsCOMPtr<nsIScriptElement> mElement;
  PRPackedBool mLoading;             
  PRPackedBool mIsInline;            
  nsString mScriptText;              
  PRUint32 mJSVersion;
  nsCOMPtr<nsIURI> mURI;
  PRInt32 mLineNo;
};




NS_IMPL_THREADSAFE_ISUPPORTS0(nsScriptLoadRequest)





nsScriptLoader::nsScriptLoader(nsIDocument *aDocument)
  : mDocument(aDocument),
    mBlockerCount(0),
    mEnabled(PR_TRUE),
    mHadPendingScripts(PR_FALSE)
{
}

nsScriptLoader::~nsScriptLoader()
{
  mObservers.Clear();

  for (PRInt32 i = 0; i < mPendingRequests.Count(); i++) {
    mPendingRequests[i]->FireScriptAvailable(NS_ERROR_ABORT);
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

  const nsAString& for_str = nsContentUtils::TrimWhitespace(forAttr);
  if (!for_str.LowerCaseEqualsLiteral("window")) {
    return PR_TRUE;
  }

  
  const nsAString& event_str = nsContentUtils::TrimWhitespace(eventAttr, PR_FALSE);
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

  
  
  PRUint32 typeID = mDocument->GetRootContent()->GetScriptTypeID();
  PRUint32 version = 0;
  nsAutoString language, type, src;
  nsresult rv = NS_OK;

  
  
  aElement->GetScriptType(type);
  if (!type.IsEmpty()) {
    nsCOMPtr<nsIMIMEHeaderParam> mimeHdrParser =
      do_GetService("@mozilla.org/network/mime-hdrparam;1");
    NS_ENSURE_TRUE(mimeHdrParser, NS_ERROR_FAILURE);

    NS_ConvertUTF16toUTF8 typeAndParams(type);

    nsAutoString mimeType;
    rv = mimeHdrParser->GetParameter(typeAndParams, nsnull,
                                     EmptyCString(), PR_FALSE, nsnull,
                                     mimeType);
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
      rv = mimeHdrParser->GetParameter(typeAndParams, "version",
                                       EmptyCString(), PR_FALSE, nsnull,
                                       versionName);
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

      rv = mimeHdrParser->GetParameter(typeAndParams, "e4x",
                                       EmptyCString(), PR_FALSE, nsnull,
                                       value);
      if (NS_FAILED(rv)) {
        if (rv != NS_ERROR_INVALID_ARG)
          return rv;
      } else {
        if (value.Length() == 1 && value[0] == '1')
          
          
          
          
          version |= JSVERSION_HAS_XML;
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

  
  nsRefPtr<nsScriptLoadRequest> request = new nsScriptLoadRequest(aElement, version);
  NS_ENSURE_TRUE(request, NS_ERROR_OUT_OF_MEMORY);

  
  nsCOMPtr<nsIURI> scriptURI = aElement->GetScriptURI();
  if (scriptURI) {
    
    rv = nsContentUtils::GetSecurityManager()->
      CheckLoadURIWithPrincipal(mDocument->NodePrincipal(), scriptURI,
                                nsIScriptSecurityManager::ALLOW_CHROME);

    NS_ENSURE_SUCCESS(rv, rv);

    
    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
    nsIURI *docURI = mDocument->GetDocumentURI();
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_SCRIPT,
                                   scriptURI,
                                   docURI,
                                   aElement,
                                   NS_LossyConvertUTF16toASCII(type),
                                   nsnull,    
                                   &shouldLoad,
                                   nsContentUtils::GetContentPolicy());
    if (NS_FAILED(rv) || NS_CP_REJECTED(shouldLoad)) {
      if (NS_FAILED(rv) || shouldLoad != nsIContentPolicy::REJECT_TYPE) {
        return NS_ERROR_CONTENT_BLOCKED;
      }
      return NS_ERROR_CONTENT_BLOCKED_SHOW_ALT;
    }

    request->mURI = scriptURI;
    request->mIsInline = PR_FALSE;
    request->mLoading = PR_TRUE;

    nsCOMPtr<nsILoadGroup> loadGroup = mDocument->GetDocumentLoadGroup();
    nsCOMPtr<nsIStreamLoader> loader;

    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(globalObject));
    nsIDocShell *docshell = window->GetDocShell();

    nsCOMPtr<nsIInterfaceRequestor> prompter(do_QueryInterface(docshell));

    nsCOMPtr<nsIChannel> channel;
    rv = NS_NewChannel(getter_AddRefs(channel),
                       scriptURI, nsnull, loadGroup,
                       prompter, nsIRequest::LOAD_NORMAL);
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

    rv = channel->AsyncOpen(loader, request);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    request->mLoading = PR_FALSE;
    request->mIsInline = PR_TRUE;
    request->mURI = mDocument->GetDocumentURI();

    request->mLineNo = aElement->GetScriptLineNumber();

    
    
    if (ReadyToExecuteScripts() && mPendingRequests.Count() == 0) {
      return ProcessRequest(request);
    }
  }

  
  NS_ENSURE_TRUE(mPendingRequests.AppendObject(request),
                 NS_ERROR_OUT_OF_MEMORY);

  
  return NS_ERROR_HTMLPARSER_BLOCK;
}

nsresult
nsScriptLoader::ProcessRequest(nsScriptLoadRequest* aRequest)
{
  NS_ASSERTION(ReadyToExecuteScripts(),
               "Caller forgot to check ReadyToExecuteScripts()");

  NS_ENSURE_ARG(aRequest);
  nsAFlatString* script;
  nsAutoString textData;

  
  if (aRequest->mIsInline) {
    
    
    aRequest->mElement->GetScriptText(textData);

    script = &textData;
  }
  else {
    script = &aRequest->mScriptText;
  }

  FireScriptAvailable(NS_OK, aRequest);
  nsresult rv = EvaluateScript(aRequest, *script);
  FireScriptEvaluated(rv, aRequest);

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

  nsIScriptGlobalObject *globalObject = mDocument->GetScriptGlobalObject();
  NS_ENSURE_TRUE(globalObject, NS_ERROR_FAILURE);

  
  nsCOMPtr<nsIContent> scriptContent(do_QueryInterface(aRequest->mElement));
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

  nsCAutoString url;

  if (aRequest->mURI) {
    rv = aRequest->mURI->GetSpec(url);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  PRBool oldProcessingScriptTag = context->GetProcessingScriptTag();
  context->SetProcessingScriptTag(PR_TRUE);

  
  nsCOMPtr<nsIScriptElement> oldCurrent = mCurrentScript;
  mCurrentScript = aRequest->mElement;

  PRBool isUndefined;
  rv = context->EvaluateString(aScript,
                          globalObject->GetScriptGlobal(stid),
                          mDocument->NodePrincipal(), url.get(),
                          aRequest->mLineNo, aRequest->mJSVersion, nsnull,
                          &isUndefined);

  
  mCurrentScript = oldCurrent;

  JSContext *cx = nsnull; 
  if (stid == nsIProgrammingLanguage::JAVASCRIPT) {
    cx = (JSContext *)context->GetNativeContext();
    ::JS_BeginRequest(cx);
    ::JS_ReportPendingException(cx);
  }

  context->SetProcessingScriptTag(oldProcessingScriptTag);

  if (stid == nsIProgrammingLanguage::JAVASCRIPT) {
    nsCOMPtr<nsIXPCNativeCallContext> ncc;
    nsContentUtils::XPConnect()->
      GetCurrentNativeCallContext(getter_AddRefs(ncc));

    if (ncc) {
      NS_ASSERTION(!::JS_IsExceptionPending(cx),
                   "JS_ReportPendingException wasn't called");
      ncc->SetExceptionWasThrown(PR_FALSE);
    }
    ::JS_EndRequest(cx);
  }
  return rv;
}

void
nsScriptLoader::ProcessPendingRequestsAsync()
{
  if (mPendingRequests.Count()) {
    nsCOMPtr<nsIRunnable> ev = new nsRunnableMethod<nsScriptLoader>(this,
      &nsScriptLoader::ProcessPendingRequests);

    NS_DispatchToCurrentThread(ev);
  }
}

void
nsScriptLoader::ProcessPendingRequests()
{
  nsRefPtr<nsScriptLoadRequest> request;
  while (ReadyToExecuteScripts() && mPendingRequests.Count() &&
         !(request = mPendingRequests[0])->mLoading) {
    mPendingRequests.RemoveObjectAt(0);
    ProcessRequest(request);
  }
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
      
      
      oCharset.Assign("UTF-16BE");
    }
    break;
  case 0xFF:
    if (0xFE == aBytes[1]) {
      
      
      oCharset.Assign("UTF-16LE");
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

  if (characterSet.IsEmpty()) {
    
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

    rv = unicodeDecoder->GetMaxLength(NS_REINTERPRET_CAST(const char*, aData),
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
        rv = unicodeDecoder->Convert(NS_REINTERPRET_CAST(const char*, aData),
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
  nsScriptLoadRequest* request = NS_STATIC_CAST(nsScriptLoadRequest*, aContext);
  NS_ASSERTION(request, "null request in stream complete handler");
  NS_ENSURE_TRUE(request, NS_ERROR_FAILURE);

  nsresult rv = PrepareLoadedRequest(request, aLoader, aStatus, aStringLen,
                                     aString);
  if (NS_FAILED(rv)) {
    mPendingRequests.RemoveObject(request);
    FireScriptAvailable(rv, request);
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
  if (aStringLen) {
    
    nsAutoString hintCharset;
    aRequest->mElement->GetScriptCharset(hintCharset);
    rv = ConvertToUTF16(channel, aString, aStringLen, hintCharset, mDocument,
                        aRequest->mScriptText);

    NS_ASSERTION(NS_SUCCEEDED(rv),
                 "Could not convert external JavaScript to Unicode!");
    NS_ENSURE_SUCCESS(rv, rv);

    
    
    
    if (channel) {
      nsCOMPtr<nsISupports> owner;
      channel->GetOwner(getter_AddRefs(owner));
      nsCOMPtr<nsIPrincipal> principal = do_QueryInterface(owner);

      if (principal) {
        nsCOMPtr<nsIPrincipal> newPrincipal =
          MaybeDowngradeToCodebase(mDocument->NodePrincipal(), principal);

        mDocument->SetPrincipal(newPrincipal);
      }
    }
  }

  
  
  
  
  NS_ASSERTION(mPendingRequests.IndexOf(aRequest) >= 0,
               "aRequest should be pending!");

  
  aRequest->mLoading = PR_FALSE;

  return NS_OK;
}
