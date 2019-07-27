





#include "nsHTMLDocument.h"

#include "nsIContentPolicy.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/dom/HTMLAllCollection.h"
#include "nsCOMPtr.h"
#include "nsGlobalWindow.h"
#include "nsXPIDLString.h"
#include "nsPrintfCString.h"
#include "nsReadableUtils.h"
#include "nsUnicharUtils.h"
#include "nsIHTMLContentSink.h"
#include "nsIXMLContentSink.h"
#include "nsHTMLParts.h"
#include "nsHTMLStyleSheet.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIDOMNode.h" 
#include "nsIDOMNodeList.h"
#include "nsIDOMElement.h"
#include "nsPIDOMWindow.h"
#include "nsDOMString.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsIIOService.h"
#include "nsNetUtil.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsDocShell.h"
#include "nsDocShellLoadTypes.h"
#include "nsIWebNavigation.h"
#include "nsIBaseWindow.h"
#include "nsIWebShellServices.h"
#include "nsIScriptContext.h"
#include "nsIXPConnect.h"
#include "nsContentList.h"
#include "nsError.h"
#include "nsIPrincipal.h"
#include "nsJSPrincipals.h"
#include "nsIScriptSecurityManager.h"
#include "nsAttrName.h"
#include "nsNodeUtils.h"

#include "nsNetCID.h"
#include "nsICookieService.h"

#include "nsIServiceManager.h"
#include "nsIConsoleService.h"
#include "nsIComponentManager.h"
#include "nsParserCIID.h"
#include "nsIDOMHTMLElement.h"
#include "nsIDOMHTMLHeadElement.h"
#include "nsNameSpaceManager.h"
#include "nsGenericHTMLElement.h"
#include "mozilla/css/Loader.h"
#include "nsIHttpChannel.h"
#include "nsIFile.h"
#include "nsFrameSelection.h"
#include "nsISelectionPrivate.h"

#include "nsContentUtils.h"
#include "nsJSUtils.h"
#include "nsIDocumentInlines.h"
#include "nsIDocumentEncoder.h" 
#include "nsICachingChannel.h"
#include "nsIContentViewer.h"
#include "nsIWyciwygChannel.h"
#include "nsIScriptElement.h"
#include "nsIScriptError.h"
#include "nsIMutableArray.h"
#include "nsArrayUtils.h"
#include "nsIEffectiveTLDService.h"


#include "nsBidiUtils.h"

#include "mozilla/dom/EncodingUtils.h"
#include "mozilla/dom/FallbackEncoding.h"
#include "mozilla/LoadInfo.h"
#include "nsIEditingSession.h"
#include "nsIEditor.h"
#include "nsNodeInfoManager.h"
#include "nsIPlaintextEditor.h"
#include "nsIHTMLEditor.h"
#include "nsIEditorStyleSheets.h"
#include "nsIInlineSpellChecker.h"
#include "nsRange.h"
#include "mozAutoDocUpdate.h"
#include "nsCCUncollectableMarker.h"
#include "nsHtml5Module.h"
#include "prprf.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Preferences.h"
#include "nsMimeTypes.h"
#include "nsIRequest.h"
#include "nsHtml5TreeOpExecutor.h"
#include "nsHtml5Parser.h"
#include "nsIDOMJSWindow.h"
#include "nsSandboxFlags.h"
#include "nsIImageDocument.h"
#include "mozilla/dom/HTMLBodyElement.h"
#include "mozilla/dom/HTMLDocumentBinding.h"
#include "nsCharsetSource.h"
#include "nsIStringBundle.h"
#include "nsDOMClassInfo.h"
#include "nsFocusManager.h"
#include "nsIFrame.h"
#include "nsIContent.h"

using namespace mozilla;
using namespace mozilla::dom;

#define NS_MAX_DOCUMENT_WRITE_DEPTH 20

#include "prtime.h"



static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

uint32_t       nsHTMLDocument::gWyciwygSessionCnt = 0;







static bool ConvertToMidasInternalCommand(const nsAString & inCommandID,
                                            const nsAString & inParam,
                                            nsACString& outCommandID,
                                            nsACString& outParam,
                                            bool& isBoolean,
                                            bool& boolValue);

static bool ConvertToMidasInternalCommand(const nsAString & inCommandID,
                                            nsACString& outCommandID);




static nsresult
RemoveFromAgentSheets(nsCOMArray<nsIStyleSheet> &aAgentSheets, const nsAString& url)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), url);
  NS_ENSURE_SUCCESS(rv, rv);

  for (int32_t i = aAgentSheets.Count() - 1; i >= 0; --i) {
    nsIStyleSheet* sheet = aAgentSheets[i];
    nsIURI* sheetURI = sheet->GetSheetURI();

    bool equals = false;
    uri->Equals(sheetURI, &equals);
    if (equals) {
      aAgentSheets.RemoveObjectAt(i);
    }
  }

  return NS_OK;
}

nsresult
NS_NewHTMLDocument(nsIDocument** aInstancePtrResult, bool aLoadedAsData)
{
  nsRefPtr<nsHTMLDocument> doc = new nsHTMLDocument();

  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    *aInstancePtrResult = nullptr;
    return rv;
  }

  doc->SetLoadedAsData(aLoadedAsData);
  doc.forget(aInstancePtrResult);

  return NS_OK;
}

  
  

nsHTMLDocument::nsHTMLDocument()
  : nsDocument("text/html")
{
  
  

  mType = eHTML;
  mDefaultElementType = kNameSpaceID_XHTML;
  mCompatMode = eCompatibility_NavQuirks;
}

nsHTMLDocument::~nsHTMLDocument()
{
}

NS_IMPL_CYCLE_COLLECTION_INHERITED(nsHTMLDocument, nsDocument,
                                   mAll,
                                   mImages,
                                   mApplets,
                                   mEmbeds,
                                   mLinks,
                                   mAnchors,
                                   mScripts,
                                   mForms,
                                   mFormControls,
                                   mWyciwygChannel,
                                   mMidasCommandManager)

NS_IMPL_ADDREF_INHERITED(nsHTMLDocument, nsDocument)
NS_IMPL_RELEASE_INHERITED(nsHTMLDocument, nsDocument)


NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(nsHTMLDocument)
  NS_INTERFACE_TABLE_INHERITED(nsHTMLDocument, nsIHTMLDocument,
                               nsIDOMHTMLDocument)
NS_INTERFACE_TABLE_TAIL_INHERITING(nsDocument)

JSObject*
nsHTMLDocument::WrapNode(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return HTMLDocumentBinding::Wrap(aCx, this, aGivenProto);
}

nsresult
nsHTMLDocument::Init()
{
  nsresult rv = nsDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  CSSLoader()->SetCompatibilityMode(mCompatMode);

  return NS_OK;
}


void
nsHTMLDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsDocument::Reset(aChannel, aLoadGroup);

  if (aChannel) {
    aChannel->GetLoadFlags(&mLoadFlags);
  }
}

void
nsHTMLDocument::ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                           nsIPrincipal* aPrincipal)
{
  mLoadFlags = nsIRequest::LOAD_NORMAL;

  nsDocument::ResetToURI(aURI, aLoadGroup, aPrincipal);

  mImages = nullptr;
  mApplets = nullptr;
  mEmbeds = nullptr;
  mLinks = nullptr;
  mAnchors = nullptr;
  mScripts = nullptr;

  mForms = nullptr;

  NS_ASSERTION(!mWyciwygChannel,
               "nsHTMLDocument::Reset() - Wyciwyg Channel  still exists!");

  mWyciwygChannel = nullptr;

  
  
  
  SetContentTypeInternal(nsDependentCString("text/html"));
}

already_AddRefed<nsIPresShell>
nsHTMLDocument::CreateShell(nsPresContext* aContext,
                            nsViewManager* aViewManager,
                            nsStyleSet* aStyleSet)
{
  return doCreateShell(aContext, aViewManager, aStyleSet, mCompatMode);
}

void
nsHTMLDocument::TryHintCharset(nsIContentViewer* aCv,
                               int32_t& aCharsetSource, nsACString& aCharset)
{
  if (aCv) {
    int32_t requestCharsetSource;
    nsresult rv = aCv->GetHintCharacterSetSource(&requestCharsetSource);

    if(NS_SUCCEEDED(rv) && kCharsetUninitialized != requestCharsetSource) {
      nsAutoCString requestCharset;
      rv = aCv->GetHintCharacterSet(requestCharset);
      aCv->SetHintCharacterSetSource((int32_t)(kCharsetUninitialized));

      if(requestCharsetSource <= aCharsetSource)
        return;

      if(NS_SUCCEEDED(rv) && EncodingUtils::IsAsciiCompatible(requestCharset)) {
        aCharsetSource = requestCharsetSource;
        aCharset = requestCharset;

        return;
      }
    }
  }
  return;
}


void
nsHTMLDocument::TryUserForcedCharset(nsIContentViewer* aCv,
                                     nsIDocShell*  aDocShell,
                                     int32_t& aCharsetSource,
                                     nsACString& aCharset)
{
  nsresult rv = NS_OK;

  if(kCharsetFromUserForced <= aCharsetSource)
    return;

  
  if (WillIgnoreCharsetOverride() || !EncodingUtils::IsAsciiCompatible(aCharset)) {
    return;
  }

  nsAutoCString forceCharsetFromDocShell;
  if (aCv) {
    
    rv = aCv->GetForceCharacterSet(forceCharsetFromDocShell);
  }

  if(NS_SUCCEEDED(rv) &&
     !forceCharsetFromDocShell.IsEmpty() &&
     EncodingUtils::IsAsciiCompatible(forceCharsetFromDocShell)) {
    aCharset = forceCharsetFromDocShell;
    aCharsetSource = kCharsetFromUserForced;
    return;
  }

  if (aDocShell) {
    
    nsAutoCString charset;
    rv = aDocShell->GetForcedCharset(charset);

    if (NS_SUCCEEDED(rv) && !charset.IsEmpty()) {
      if (!EncodingUtils::IsAsciiCompatible(charset)) {
        return;
      }
      aCharset = charset;
      aCharsetSource = kCharsetFromUserForced;
      aDocShell->SetForcedCharset(NS_LITERAL_CSTRING(""));
    }
  }
}

void
nsHTMLDocument::TryCacheCharset(nsICachingChannel* aCachingChannel,
                                int32_t& aCharsetSource,
                                nsACString& aCharset)
{
  nsresult rv;

  if (kCharsetFromCache <= aCharsetSource) {
    return;
  }

  nsCString cachedCharset;
  rv = aCachingChannel->GetCacheTokenCachedCharset(cachedCharset);
  
  
  
  if (NS_SUCCEEDED(rv) &&
      !cachedCharset.IsEmpty() &&
      EncodingUtils::IsAsciiCompatible(cachedCharset))
  {
    aCharset = cachedCharset;
    aCharsetSource = kCharsetFromCache;
  }
}

void
nsHTMLDocument::TryParentCharset(nsIDocShell*  aDocShell,
                                 int32_t& aCharsetSource,
                                 nsACString& aCharset)
{
  if (!aDocShell) {
    return;
  }
  if (aCharsetSource >= kCharsetFromParentForced) {
    return;
  }

  int32_t parentSource;
  nsAutoCString parentCharset;
  nsCOMPtr<nsIPrincipal> parentPrincipal;
  aDocShell->GetParentCharset(parentCharset,
                              &parentSource,
                              getter_AddRefs(parentPrincipal));
  if (parentCharset.IsEmpty()) {
    return;
  }
  if (kCharsetFromParentForced == parentSource ||
      kCharsetFromUserForced == parentSource) {
    if (WillIgnoreCharsetOverride() ||
        !EncodingUtils::IsAsciiCompatible(aCharset) || 
        !EncodingUtils::IsAsciiCompatible(parentCharset)) {
      return;
    }
    aCharset.Assign(parentCharset);
    aCharsetSource = kCharsetFromParentForced;
    return;
  }

  if (aCharsetSource >= kCharsetFromParentFrame) {
    return;
  }

  if (kCharsetFromCache <= parentSource) {
    
    if (!NodePrincipal()->Equals(parentPrincipal) ||
        !EncodingUtils::IsAsciiCompatible(parentCharset)) {
      return;
    }

    aCharset.Assign(parentCharset);
    aCharsetSource = kCharsetFromParentFrame;
  }
}

void
nsHTMLDocument::TryTLD(int32_t& aCharsetSource, nsACString& aCharset)
{
  if (aCharsetSource >= kCharsetFromTopLevelDomain) {
    return;
  }
  if (!FallbackEncoding::sGuessFallbackFromTopLevelDomain) {
    return;
  }
  if (!mDocumentURI) {
    return;
  }
  nsAutoCString host;
  mDocumentURI->GetAsciiHost(host);
  if (host.IsEmpty()) {
    return;
  }
  
  
  if (host.Last() == '.') {
    host.SetLength(host.Length() - 1);
    if (host.IsEmpty()) {
      return;
    }
  }
  
  
  if (host.Last() == '.') {
    return;
  }
  int32_t index = host.RFindChar('.');
  if (index == kNotFound) {
    
    return;
  }
  
  
  
  nsAutoCString tld;
  ToLowerCase(Substring(host, index + 1, host.Length() - (index + 1)), tld);
  
  if (!FallbackEncoding::IsParticipatingTopLevelDomain(tld)) {
    return;
  }
  
  bool seenNonDigit = false;
  for (size_t i = 0; i < tld.Length(); ++i) {
    char c = tld.CharAt(i);
    if (c < '0' || c > '9') {
      seenNonDigit = true;
      break;
    }
  }
  if (!seenNonDigit) {
    return;
  }
  aCharsetSource = kCharsetFromTopLevelDomain;
  FallbackEncoding::FromTopLevelDomain(tld, aCharset);
}

void
nsHTMLDocument::TryFallback(int32_t& aCharsetSource, nsACString& aCharset)
{
  if (kCharsetFromFallback <= aCharsetSource)
    return;

  aCharsetSource = kCharsetFromFallback;
  FallbackEncoding::FromLocale(aCharset);
}

void
nsHTMLDocument::SetDocumentCharacterSet(const nsACString& aCharSetID)
{
  nsDocument::SetDocumentCharacterSet(aCharSetID);
  
  
  nsCOMPtr<nsIWyciwygChannel> wyciwygChannel = do_QueryInterface(mChannel);
  if (wyciwygChannel) {
    wyciwygChannel->SetCharsetAndSource(GetDocumentCharacterSetSource(),
                                        aCharSetID);
  }
}

nsresult
nsHTMLDocument::StartDocumentLoad(const char* aCommand,
                                  nsIChannel* aChannel,
                                  nsILoadGroup* aLoadGroup,
                                  nsISupports* aContainer,
                                  nsIStreamListener **aDocListener,
                                  bool aReset,
                                  nsIContentSink* aSink)
{
  if (!aCommand) {
    MOZ_ASSERT(false, "Command is mandatory");
    return NS_ERROR_INVALID_POINTER;
  }
  if (aSink) {
    MOZ_ASSERT(false, "Got a sink override. Should not happen for HTML doc.");
    return NS_ERROR_INVALID_ARG;
  }
  if (mType != eHTML) {
    MOZ_ASSERT(mType == eXHTML);
    MOZ_ASSERT(false, "Must not set HTML doc to XHTML mode before load start.");
    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  nsAutoCString contentType;
  aChannel->GetContentType(contentType);

  bool view = !strcmp(aCommand, "view") ||
              !strcmp(aCommand, "external-resource");
  bool viewSource = !strcmp(aCommand, "view-source");
  bool asData = !strcmp(aCommand, kLoadAsData);
  bool import = !strcmp(aCommand, "import");
  if (!(view || viewSource || asData || import)) {
    MOZ_ASSERT(false, "Bad parser command");
    return NS_ERROR_INVALID_ARG;
  }

  bool html = contentType.EqualsLiteral(TEXT_HTML);
  bool xhtml = !html && contentType.EqualsLiteral(APPLICATION_XHTML_XML);
  bool plainText = !html && !xhtml && nsContentUtils::IsPlainTextType(contentType);
  if (!(html || xhtml || plainText || viewSource)) {
    MOZ_ASSERT(false, "Channel with bad content type.");
    return NS_ERROR_INVALID_ARG;
  }

  bool loadAsHtml5 = true;

  if (!viewSource && xhtml) {
      
      mType = eXHTML;
      mCompatMode = eCompatibility_FullStandards;
      loadAsHtml5 = false;
  }
  
  
  if (loadAsHtml5 && view) {
    
    nsCOMPtr<nsIURI> uri;
    aChannel->GetOriginalURI(getter_AddRefs(uri));
    
    
    bool isAbout = false;
    if (uri && NS_SUCCEEDED(uri->SchemeIs("about", &isAbout)) && isAbout) {
      nsAutoCString str;
      uri->GetSpec(str);
      if (str.EqualsLiteral("about:blank")) {
        loadAsHtml5 = false;    
      }
    }
  }
  
  CSSLoader()->SetCompatibilityMode(mCompatMode);
  
  nsresult rv = nsDocument::StartDocumentLoad(aCommand,
                                              aChannel, aLoadGroup,
                                              aContainer,
                                              aDocListener, aReset);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  aChannel->GetSecurityInfo(getter_AddRefs(mSecurityInfo));

  nsCOMPtr<nsIURI> uri;
  rv = aChannel->GetURI(getter_AddRefs(uri));
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsCOMPtr<nsICachingChannel> cachingChan = do_QueryInterface(aChannel);

  if (loadAsHtml5) {
    mParser = nsHtml5Module::NewHtml5Parser();
    if (plainText) {
      if (viewSource) {
        mParser->MarkAsNotScriptCreated("view-source-plain");
      } else {
        mParser->MarkAsNotScriptCreated("plain-text");
      }
    } else if (viewSource && !html) {
      mParser->MarkAsNotScriptCreated("view-source-xml");
    } else {
      mParser->MarkAsNotScriptCreated(aCommand);
    }
  } else {
    mParser = do_CreateInstance(kCParserCID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  

  
  
  
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aContainer));
  nsCOMPtr<nsIDocShellTreeItem> parentAsItem;
  if (docShell) {
    docShell->GetSameTypeParent(getter_AddRefs(parentAsItem));
  }

  nsCOMPtr<nsIDocShell> parent(do_QueryInterface(parentAsItem));
  nsCOMPtr<nsIContentViewer> parentContentViewer;
  if (parent) {
    rv = parent->GetContentViewer(getter_AddRefs(parentContentViewer));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIContentViewer> cv;
  if (docShell) {
    docShell->GetContentViewer(getter_AddRefs(cv));
  }
  if (!cv) {
    cv = parentContentViewer.forget();
  }

  nsAutoCString urlSpec;
  uri->GetSpec(urlSpec);
#ifdef DEBUG_charset
  printf("Determining charset for %s\n", urlSpec.get());
#endif

  
  int32_t charsetSource;
  nsAutoCString charset;

  
  
  int32_t parserCharsetSource;
  nsAutoCString parserCharset;

  nsCOMPtr<nsIWyciwygChannel> wyciwygChannel;
  
  
  nsHtml5TreeOpExecutor* executor = nullptr;
  if (loadAsHtml5) {
    executor = static_cast<nsHtml5TreeOpExecutor*> (mParser->GetContentSink());
    if (mReferrerPolicySet) {
      
      
      executor->SetSpeculationReferrerPolicy(static_cast<ReferrerPolicy>(mReferrerPolicy));
    }
  }

  if (!IsHTMLDocument() || !docShell) { 
    charsetSource = IsHTMLDocument() ? kCharsetFromFallback
                                     : kCharsetFromDocTypeDefault;
    charset.AssignLiteral("UTF-8");
    TryChannelCharset(aChannel, charsetSource, charset, executor);
    parserCharsetSource = charsetSource;
    parserCharset = charset;
  } else {
    NS_ASSERTION(docShell, "Unexpected null value");

    charsetSource = kCharsetUninitialized;
    wyciwygChannel = do_QueryInterface(aChannel);

    
    
    
    
    
    

    
    
    if (!wyciwygChannel) {
      
      
      
      
      
      
      
      TryChannelCharset(aChannel, charsetSource, charset, executor);
    }

    TryUserForcedCharset(cv, docShell, charsetSource, charset);

    TryHintCharset(cv, charsetSource, charset); 
    TryParentCharset(docShell, charsetSource, charset);

    if (cachingChan && !urlSpec.IsEmpty()) {
      TryCacheCharset(cachingChan, charsetSource, charset);
    }

    TryTLD(charsetSource, charset);
    TryFallback(charsetSource, charset);

    if (wyciwygChannel) {
      
      parserCharset = "UTF-16";
      parserCharsetSource = charsetSource < kCharsetFromChannel ?
        kCharsetFromChannel : charsetSource;
        
      nsAutoCString cachedCharset;
      int32_t cachedSource;
      rv = wyciwygChannel->GetCharsetAndSource(&cachedSource, cachedCharset);
      if (NS_SUCCEEDED(rv)) {
        if (cachedSource > charsetSource) {
          charsetSource = cachedSource;
          charset = cachedCharset;
        }
      } else {
        
        rv = NS_OK;
      }
      
    } else {
      parserCharset = charset;
      parserCharsetSource = charsetSource;
    }
  }

  SetDocumentCharacterSetSource(charsetSource);
  SetDocumentCharacterSet(charset);

  if (cachingChan) {
    NS_ASSERTION(charset == parserCharset,
                 "How did those end up different here?  wyciwyg channels are "
                 "not nsICachingChannel");
    rv = cachingChan->SetCacheTokenCachedCharset(charset);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "cannot SetMetaDataElement");
    rv = NS_OK; 
  }

  
  rv = NS_OK;
  nsCOMPtr<nsIStreamListener> listener = mParser->GetStreamListener();
  listener.forget(aDocListener);

#ifdef DEBUG_charset
  printf(" charset = %s source %d\n",
        charset.get(), charsetSource);
#endif
  mParser->SetDocumentCharset(parserCharset, parserCharsetSource);
  mParser->SetCommand(aCommand);

  if (!IsHTMLDocument()) {
    MOZ_ASSERT(!loadAsHtml5);
    nsCOMPtr<nsIXMLContentSink> xmlsink;
    NS_NewXMLContentSink(getter_AddRefs(xmlsink), this, uri,
                         docShell, aChannel);
    mParser->SetContentSink(xmlsink);
  } else {
    if (loadAsHtml5) {
      nsHtml5Module::Initialize(mParser, this, uri, docShell, aChannel);
    } else {
      
      nsCOMPtr<nsIHTMLContentSink> htmlsink;
      NS_NewHTMLContentSink(getter_AddRefs(htmlsink), this, uri,
                            docShell, aChannel);
      mParser->SetContentSink(htmlsink);
    }
  }

  if (plainText && !nsContentUtils::IsChildOfSameType(this) &&
      Preferences::GetBool("plain_text.wrap_long_lines")) {
    nsCOMPtr<nsIStringBundleService> bundleService = do_GetService(NS_STRINGBUNDLE_CONTRACTID, &rv);
    NS_ASSERTION(NS_SUCCEEDED(rv) && bundleService, "The bundle service could not be loaded");
    nsCOMPtr<nsIStringBundle> bundle;
    rv = bundleService->CreateBundle("chrome://global/locale/browser.properties",
                                     getter_AddRefs(bundle));
    NS_ASSERTION(NS_SUCCEEDED(rv) && bundle, "chrome://global/locale/browser.properties could not be loaded");
    nsXPIDLString title;
    if (bundle) {
      bundle->GetStringFromName(MOZ_UTF16("plainText.wordWrap"), getter_Copies(title));
    }
    SetSelectedStyleSheetSet(title);
  }

  
  mParser->Parse(uri, nullptr, (void *)this);

  return rv;
}

void
nsHTMLDocument::StopDocumentLoad()
{
  BlockOnload();

  
  
  RemoveWyciwygChannel();
  NS_ASSERTION(!mWyciwygChannel, "nsHTMLDocument::StopDocumentLoad(): "
               "nsIWyciwygChannel could not be removed!");

  nsDocument::StopDocumentLoad();
  UnblockOnload(false);
  return;
}

void
nsHTMLDocument::BeginLoad()
{
  if (IsEditingOn()) {
    
    
    
    

    TurnEditingOff();
    EditingStateChanged();
  }
  nsDocument::BeginLoad();
}

void
nsHTMLDocument::EndLoad()
{
  bool turnOnEditing =
    mParser && (HasFlag(NODE_IS_EDITABLE) || mContentEditableCount > 0);
  
  nsDocument::EndLoad();
  if (turnOnEditing) {
    EditingStateChanged();
  }
}

void
nsHTMLDocument::SetCompatibilityMode(nsCompatibility aMode)
{
  NS_ASSERTION(IsHTMLDocument() || aMode == eCompatibility_FullStandards,
               "Bad compat mode for XHTML document!");

  mCompatMode = aMode;
  CSSLoader()->SetCompatibilityMode(mCompatMode);
  nsCOMPtr<nsIPresShell> shell = GetShell();
  if (shell) {
    nsPresContext *pc = shell->GetPresContext();
    if (pc) {
      pc->CompatibilityModeChanged();
    }
  }
}




already_AddRefed<nsIURI>
nsHTMLDocument::GetDomainURI()
{
  nsIPrincipal* principal = NodePrincipal();

  nsCOMPtr<nsIURI> uri;
  principal->GetDomain(getter_AddRefs(uri));
  if (uri) {
    return uri.forget();
  }

  principal->GetURI(getter_AddRefs(uri));
  return uri.forget();
}


NS_IMETHODIMP
nsHTMLDocument::GetDomain(nsAString& aDomain)
{
  nsCOMPtr<nsIURI> uri = GetDomainURI();

  if (!uri) {
    SetDOMStringToNull(aDomain);
    return NS_OK;
  }

  nsAutoCString hostName;

  if (NS_SUCCEEDED(uri->GetHost(hostName))) {
    CopyUTF8toUTF16(hostName, aDomain);
  } else {
    
    
    SetDOMStringToNull(aDomain);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetDomain(const nsAString& aDomain)
{
  ErrorResult rv;
  SetDomain(aDomain, rv);
  return rv.StealNSResult();
}

void
nsHTMLDocument::SetDomain(const nsAString& aDomain, ErrorResult& rv)
{
  if (mSandboxFlags & SANDBOXED_DOMAIN) {
    
    rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  if (aDomain.IsEmpty()) {
    rv.Throw(NS_ERROR_DOM_BAD_DOCUMENT_DOMAIN);
    return;
  }

  
  nsCOMPtr<nsIURI> uri = GetDomainURI();

  if (!uri) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsAutoCString newURIString;
  if (NS_FAILED(uri->GetScheme(newURIString))) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }
  nsAutoCString path;
  if (NS_FAILED(uri->GetPath(path))) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }
  newURIString.AppendLiteral("://");
  AppendUTF16toUTF8(aDomain, newURIString);
  newURIString.Append(path);

  nsCOMPtr<nsIURI> newURI;
  if (NS_FAILED(NS_NewURI(getter_AddRefs(newURI), newURIString))) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  
  
  nsAutoCString current, domain;
  if (NS_FAILED(uri->GetAsciiHost(current)))
    current.Truncate();
  if (NS_FAILED(newURI->GetAsciiHost(domain)))
    domain.Truncate();

  bool ok = current.Equals(domain);
  if (current.Length() > domain.Length() &&
      StringEndsWith(current, domain) &&
      current.CharAt(current.Length() - domain.Length() - 1) == '.') {
    
    
    nsCOMPtr<nsIEffectiveTLDService> tldService =
      do_GetService(NS_EFFECTIVETLDSERVICE_CONTRACTID);
    if (!tldService) {
      rv.Throw(NS_ERROR_NOT_AVAILABLE);
      return;
    }

    nsAutoCString currentBaseDomain;
    ok = NS_SUCCEEDED(tldService->GetBaseDomain(uri, 0, currentBaseDomain));
    NS_ASSERTION(StringEndsWith(domain, currentBaseDomain) ==
                 (domain.Length() >= currentBaseDomain.Length()),
                 "uh-oh!  slight optimization wasn't valid somehow!");
    ok = ok && domain.Length() >= currentBaseDomain.Length();
  }
  if (!ok) {
    
    rv.Throw(NS_ERROR_DOM_BAD_DOCUMENT_DOMAIN);
    return;
  }

  rv = NodePrincipal()->SetDomain(newURI);
}

nsGenericHTMLElement*
nsHTMLDocument::GetBody()
{
  Element* html = GetHtmlElement();
  if (!html) {
    return nullptr;
  }

  for (nsIContent* child = html->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsHTMLElement(nsGkAtoms::body) ||
        child->IsHTMLElement(nsGkAtoms::frameset)) {
      return static_cast<nsGenericHTMLElement*>(child);
    }
  }

  return nullptr;
}

NS_IMETHODIMP
nsHTMLDocument::GetBody(nsIDOMHTMLElement** aBody)
{
  *aBody = nullptr;

  nsIContent *body = GetBody();

  return body ? CallQueryInterface(body, aBody) : NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetBody(nsIDOMHTMLElement* aBody)
{
  nsCOMPtr<nsIContent> newBody = do_QueryInterface(aBody);
  MOZ_ASSERT(!newBody || newBody->IsHTMLElement(),
             "How could we be an nsIContent but not actually HTML here?");
  ErrorResult rv;
  SetBody(static_cast<nsGenericHTMLElement*>(newBody.get()), rv);
  return rv.StealNSResult();
}

void
nsHTMLDocument::SetBody(nsGenericHTMLElement* newBody, ErrorResult& rv)
{
  Element* root = GetRootElement();

  
  
  
  if (!newBody ||
      !newBody->IsAnyOfHTMLElements(nsGkAtoms::body, nsGkAtoms::frameset) ||
      !root || !root->IsHTMLElement() ||
      !root->IsHTMLElement(nsGkAtoms::html)) {
    rv.Throw(NS_ERROR_DOM_HIERARCHY_REQUEST_ERR);
    return;
  }

  
  nsCOMPtr<Element> currentBody = GetBodyElement();
  if (currentBody) {
    root->ReplaceChild(*newBody, *currentBody, rv);
  } else {
    root->AppendChild(*newBody, rv);
  }
}

NS_IMETHODIMP
nsHTMLDocument::GetHead(nsIDOMHTMLHeadElement** aHead)
{
  *aHead = nullptr;

  Element* head = GetHeadElement();

  return head ? CallQueryInterface(head, aHead) : NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetImages(nsIDOMHTMLCollection** aImages)
{
  NS_ADDREF(*aImages = Images());
  return NS_OK;
}

nsIHTMLCollection*
nsHTMLDocument::Images()
{
  if (!mImages) {
    mImages = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::img, nsGkAtoms::img);
  }
  return mImages;
}

NS_IMETHODIMP
nsHTMLDocument::GetApplets(nsIDOMHTMLCollection** aApplets)
{
  NS_ADDREF(*aApplets = Applets());
  return NS_OK;
}

nsIHTMLCollection*
nsHTMLDocument::Applets()
{
  if (!mApplets) {
    mApplets = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::applet, nsGkAtoms::applet);
  }
  return mApplets;
}

bool
nsHTMLDocument::MatchLinks(nsIContent *aContent, int32_t aNamespaceID,
                           nsIAtom* aAtom, void* aData)
{
  nsIDocument* doc = aContent->GetUncomposedDoc();

  if (doc) {
    NS_ASSERTION(aContent->IsInDoc(),
                 "This method should never be called on content nodes that "
                 "are not in a document!");
#ifdef DEBUG
    {
      nsCOMPtr<nsIHTMLDocument> htmldoc =
        do_QueryInterface(aContent->GetUncomposedDoc());
      NS_ASSERTION(htmldoc,
                   "Huh, how did this happen? This should only be used with "
                   "HTML documents!");
    }
#endif

    mozilla::dom::NodeInfo *ni = aContent->NodeInfo();

    nsIAtom *localName = ni->NameAtom();
    if (ni->NamespaceID() == kNameSpaceID_XHTML &&
        (localName == nsGkAtoms::a || localName == nsGkAtoms::area)) {
      return aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::href);
    }
  }

  return false;
}

NS_IMETHODIMP
nsHTMLDocument::GetLinks(nsIDOMHTMLCollection** aLinks)
{
  NS_ADDREF(*aLinks = Links());
  return NS_OK;
}

nsIHTMLCollection*
nsHTMLDocument::Links()
{
  if (!mLinks) {
    mLinks = new nsContentList(this, MatchLinks, nullptr, nullptr);
  }
  return mLinks;
}

bool
nsHTMLDocument::MatchAnchors(nsIContent *aContent, int32_t aNamespaceID,
                             nsIAtom* aAtom, void* aData)
{
  NS_ASSERTION(aContent->IsInDoc(),
               "This method should never be called on content nodes that "
               "are not in a document!");
#ifdef DEBUG
  {
    nsCOMPtr<nsIHTMLDocument> htmldoc =
      do_QueryInterface(aContent->GetUncomposedDoc());
    NS_ASSERTION(htmldoc,
                 "Huh, how did this happen? This should only be used with "
                 "HTML documents!");
  }
#endif

  if (aContent->NodeInfo()->Equals(nsGkAtoms::a, kNameSpaceID_XHTML)) {
    return aContent->HasAttr(kNameSpaceID_None, nsGkAtoms::name);
  }

  return false;
}

NS_IMETHODIMP
nsHTMLDocument::GetAnchors(nsIDOMHTMLCollection** aAnchors)
{
  NS_ADDREF(*aAnchors = Anchors());
  return NS_OK;
}

nsIHTMLCollection*
nsHTMLDocument::Anchors()
{
  if (!mAnchors) {
    mAnchors = new nsContentList(this, MatchAnchors, nullptr, nullptr);
  }
  return mAnchors;
}

NS_IMETHODIMP
nsHTMLDocument::GetScripts(nsIDOMHTMLCollection** aScripts)
{
  NS_ADDREF(*aScripts = Scripts());
  return NS_OK;
}

nsIHTMLCollection*
nsHTMLDocument::Scripts()
{
  if (!mScripts) {
    mScripts = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::script, nsGkAtoms::script);
  }
  return mScripts;
}

NS_IMETHODIMP
nsHTMLDocument::GetCookie(nsAString& aCookie)
{
  ErrorResult rv;
  GetCookie(aCookie, rv);
  return rv.StealNSResult();
}

already_AddRefed<nsIChannel>
nsHTMLDocument::CreateDummyChannelForCookies(nsIURI* aCodebaseURI)
{
  
  
  
  
  
  
  MOZ_ASSERT(!mChannel);

  nsCOMPtr<nsIChannel> channel;
  NS_NewChannel(getter_AddRefs(channel), aCodebaseURI, this,
                nsILoadInfo::SEC_NORMAL,
                nsIContentPolicy::TYPE_INVALID);
  nsCOMPtr<nsIPrivateBrowsingChannel> pbChannel =
    do_QueryInterface(channel);
  nsCOMPtr<nsIDocShell> docShell(mDocumentContainer);
  nsCOMPtr<nsILoadContext> loadContext = do_QueryInterface(docShell);
  if (!pbChannel || !loadContext) {
    return nullptr;
  }
  pbChannel->SetPrivate(loadContext->UsePrivateBrowsing());
  return channel.forget();
}

void
nsHTMLDocument::GetCookie(nsAString& aCookie, ErrorResult& rv)
{
  aCookie.Truncate(); 
                      

  if (mDisableCookieAccess) {
    return;
  }

  
  
  if (mSandboxFlags & SANDBOXED_ORIGIN) {
    rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }
  
  
  nsCOMPtr<nsICookieService> service = do_GetService(NS_COOKIESERVICE_CONTRACTID);
  if (service) {
    
    
    nsCOMPtr<nsIURI> codebaseURI;
    NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));

    if (!codebaseURI) {
      
      

      return;
    }

    nsCOMPtr<nsIChannel> channel(mChannel);
    if (!channel) {
      channel = CreateDummyChannelForCookies(codebaseURI);
      if (!channel) {
        return;
      }
    }

    nsXPIDLCString cookie;
    service->GetCookieString(codebaseURI, channel, getter_Copies(cookie));
    
    
    nsContentUtils::ConvertStringFromEncoding(NS_LITERAL_CSTRING("UTF-8"),
                                              cookie, aCookie);
  }
}

NS_IMETHODIMP
nsHTMLDocument::SetCookie(const nsAString& aCookie)
{
  ErrorResult rv;
  SetCookie(aCookie, rv);
  return rv.StealNSResult();
}

void
nsHTMLDocument::SetCookie(const nsAString& aCookie, ErrorResult& rv)
{
  if (mDisableCookieAccess) {
    return;
  }

  
  
  if (mSandboxFlags & SANDBOXED_ORIGIN) {
    rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return;
  }

  
  nsCOMPtr<nsICookieService> service = do_GetService(NS_COOKIESERVICE_CONTRACTID);
  if (service && mDocumentURI) {
    
    nsCOMPtr<nsIURI> codebaseURI;
    NodePrincipal()->GetURI(getter_AddRefs(codebaseURI));

    if (!codebaseURI) {
      
      

      return;
    }

    nsCOMPtr<nsIChannel> channel(mChannel);
    if (!channel) {
      channel = CreateDummyChannelForCookies(codebaseURI);
      if (!channel) {
        return;
      }
    }

    NS_ConvertUTF16toUTF8 cookie(aCookie);
    service->SetCookieString(codebaseURI, nullptr, cookie.get(), channel);
  }
}

NS_IMETHODIMP
nsHTMLDocument::Open(const nsAString& aContentTypeOrUrl,
                     const nsAString& aReplaceOrName,
                     const nsAString& aFeatures,
                     JSContext* cx, uint8_t aOptionalArgCount,
                     nsISupports** aReturn)
{
  
  if (aOptionalArgCount > 2) {
    ErrorResult rv;
    *aReturn = Open(cx, aContentTypeOrUrl, aReplaceOrName, aFeatures,
                    false, rv).take();
    return rv.StealNSResult();
  }

  nsString type;
  if (aOptionalArgCount > 0) {
    type = aContentTypeOrUrl;
  } else {
    type.AssignLiteral("text/html");
  }
  nsString replace;
  if (aOptionalArgCount > 1) {
    replace = aReplaceOrName;
  }
  ErrorResult rv;
  *aReturn = Open(cx, type, replace, rv).take();
  return rv.StealNSResult();
}

already_AddRefed<nsIDOMWindow>
nsHTMLDocument::Open(JSContext* ,
                     const nsAString& aURL,
                     const nsAString& aName,
                     const nsAString& aFeatures,
                     bool aReplace,
                     ErrorResult& rv)
{
  NS_ASSERTION(nsContentUtils::CanCallerAccess(static_cast<nsIDOMHTMLDocument*>(this)),
               "XOW should have caught this!");

  nsCOMPtr<nsIDOMWindow> window = GetInnerWindow();
  if (!window) {
    rv.Throw(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return nullptr;
  }
  nsCOMPtr<nsIDOMJSWindow> win = do_QueryInterface(window);
  nsCOMPtr<nsIDOMWindow> newWindow;
  
  rv = win->OpenJS(aURL, aName, aFeatures, getter_AddRefs(newWindow));
  return newWindow.forget();
}

already_AddRefed<nsIDocument>
nsHTMLDocument::Open(JSContext* cx,
                     const nsAString& aType,
                     const nsAString& aReplace,
                     ErrorResult& rv)
{
  NS_ASSERTION(nsContentUtils::CanCallerAccess(static_cast<nsIDOMHTMLDocument*>(this)),
               "XOW should have caught this!");
  if (!IsHTMLDocument() || mDisableDocWrite || !IsMasterDocument()) {
    
    rv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return nullptr;
  }

  nsAutoCString contentType;
  contentType.AssignLiteral("text/html");

  nsAutoString type;
  nsContentUtils::ASCIIToLower(aType, type);
  nsAutoCString actualType, dummy;
  NS_ParseContentType(NS_ConvertUTF16toUTF8(type), actualType, dummy);
  if (!actualType.EqualsLiteral("text/html") &&
      !type.EqualsLiteral("replace")) {
    contentType.AssignLiteral("text/plain");
  }

  
  if (mParser || mParserAborted) {
    
    
    
    
    
    
    
    
    
    nsCOMPtr<nsIDocument> ret = this;
    return ret.forget();
  }

  
  if (!mScriptGlobalObject) {
    nsCOMPtr<nsIDocument> ret = this;
    return ret.forget();
  }

  nsPIDOMWindow* outer = GetWindow();
  if (!outer || (GetInnerWindow() != outer->GetCurrentInnerWindow())) {
    nsCOMPtr<nsIDocument> ret = this;
    return ret.forget();
  }

  
  nsCOMPtr<nsIDocShell> shell(mDocumentContainer);
  if (!shell) {
    
    nsCOMPtr<nsIDocument> ret = this;
    return ret.forget();
  }

  bool inUnload;
  shell->GetIsInUnload(&inUnload);
  if (inUnload) {
    nsCOMPtr<nsIDocument> ret = this;
    return ret.forget();
  }

  
  
  
  nsCOMPtr<nsIDocument> callerDoc = GetEntryDocument();
  if (!callerDoc) {
    
    
    
    
    
    

    rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  
  
  nsCOMPtr<nsISupports> securityInfo = callerDoc->GetSecurityInfo();
  nsCOMPtr<nsIURI> uri = callerDoc->GetDocumentURI();
  nsCOMPtr<nsIURI> baseURI = callerDoc->GetBaseURI();
  nsCOMPtr<nsIPrincipal> callerPrincipal = callerDoc->NodePrincipal();
  nsCOMPtr<nsIChannel> callerChannel = callerDoc->GetChannel();

  
  
  
  
  

  bool equals = false;
  if (NS_FAILED(callerPrincipal->Equals(NodePrincipal(), &equals)) ||
      !equals) {

#ifdef DEBUG
    nsCOMPtr<nsIURI> callerDocURI = callerDoc->GetDocumentURI();
    nsCOMPtr<nsIURI> thisURI = nsIDocument::GetDocumentURI();
    nsAutoCString callerSpec;
    nsAutoCString thisSpec;
    if (callerDocURI) {
      callerDocURI->GetSpec(callerSpec);
    }
    if (thisURI) {
      thisURI->GetSpec(thisSpec);
    }
    printf("nsHTMLDocument::Open callerDoc %s this %s\n", callerSpec.get(), thisSpec.get());
#endif

    rv.Throw(NS_ERROR_DOM_SECURITY_ERR);
    return nullptr;
  }

  
  if (mScriptGlobalObject) {
    nsCOMPtr<nsIContentViewer> cv;
    shell->GetContentViewer(getter_AddRefs(cv));

    if (cv) {
      bool okToUnload;
      if (NS_SUCCEEDED(cv->PermitUnload(false, &okToUnload)) && !okToUnload) {
        
        
        nsCOMPtr<nsIDocument> ret = this;
        return ret.forget();
      }
    }

    nsCOMPtr<nsIWebNavigation> webnav(do_QueryInterface(shell));
    webnav->Stop(nsIWebNavigation::STOP_NETWORK);

    
    
    
    
    EnsureOnloadBlocker();
  }

  
  
  nsCOMPtr<nsIChannel> channel;
  nsCOMPtr<nsILoadGroup> group = do_QueryReferent(mDocumentLoadGroup);
  rv = NS_NewChannel(getter_AddRefs(channel),
                     uri,
                     callerDoc,
                     nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                     nsIContentPolicy::TYPE_OTHER,
                     group);

  if (rv.Failed()) {
    return nullptr;
  }

  
  

  if (rv.Failed()) {
    return nullptr;
  }

  if (callerChannel) {
    nsLoadFlags callerLoadFlags;
    rv = callerChannel->GetLoadFlags(&callerLoadFlags);
    if (rv.Failed()) {
      return nullptr;
    }

    nsLoadFlags loadFlags;
    rv = channel->GetLoadFlags(&loadFlags);
    if (rv.Failed()) {
      return nullptr;
    }

    loadFlags |= callerLoadFlags & nsIRequest::INHIBIT_PERSISTENT_CACHING;

    rv = channel->SetLoadFlags(loadFlags);
    if (rv.Failed()) {
      return nullptr;
    }

    
    
    bool rootHasSecureConnection = false;
    bool allowMixedContent = false;
    bool isDocShellRoot = false;
    nsresult rvalue = shell->GetAllowMixedContentAndConnectionData(&rootHasSecureConnection, &allowMixedContent, &isDocShellRoot);
    if (NS_SUCCEEDED(rvalue) && allowMixedContent && isDocShellRoot) {
       shell->SetMixedContentChannel(channel);
    }
  }

  
  
  

  
  nsCOMPtr<nsIDocument> kungFuDeathGrip = this;

  nsPIDOMWindow *window = GetInnerWindow();
  if (window) {
    
    nsCOMPtr<nsIScriptGlobalObject> oldScope(do_QueryReferent(mScopeObject));

#ifdef DEBUG
    bool willReparent = mWillReparent;
    mWillReparent = true;

    nsDocument* templateContentsOwner =
      static_cast<nsDocument*>(mTemplateContentsOwner.get());

    if (templateContentsOwner) {
      templateContentsOwner->mWillReparent = true;
    }
#endif

    
    rv = window->SetNewDocument(this, nullptr, false);
    if (rv.Failed()) {
      return nullptr;
    }

#ifdef DEBUG
    if (templateContentsOwner) {
      templateContentsOwner->mWillReparent = willReparent;
    }

    mWillReparent = willReparent;
#endif

    
    
    
    SetIsInitialDocument(false);

    nsCOMPtr<nsIScriptGlobalObject> newScope(do_QueryReferent(mScopeObject));
    JS::Rooted<JSObject*> wrapper(cx, GetWrapper());
    if (oldScope && newScope != oldScope && wrapper) {
      JSAutoCompartment ac(cx, wrapper);
      rv = mozilla::dom::ReparentWrapper(cx, wrapper);
      if (rv.Failed()) {
        return nullptr;
      }

      
      
      if (mTemplateContentsOwner) {
        JS::Rooted<JSObject*> contentsOwnerWrapper(cx,
          mTemplateContentsOwner->GetWrapper());
        if (contentsOwnerWrapper) {
          rv = mozilla::dom::ReparentWrapper(cx, contentsOwnerWrapper);
          if (rv.Failed()) {
            return nullptr;
          }
        }
      }
    }
  }

  mDidDocumentOpen = true;

  
  Reset(channel, group);
  if (baseURI) {
    mDocumentBaseURI = baseURI;
  }

  
  
  mSecurityInfo = securityInfo;

  mParserAborted = false;
  mParser = nsHtml5Module::NewHtml5Parser();
  nsHtml5Module::Initialize(mParser, this, uri, shell, channel);
  if (mReferrerPolicySet) {
    
    
    nsHtml5TreeOpExecutor* executor = nullptr;
    executor = static_cast<nsHtml5TreeOpExecutor*> (mParser->GetContentSink());
    if (executor && mReferrerPolicySet) {
      executor->SetSpeculationReferrerPolicy(static_cast<ReferrerPolicy>(mReferrerPolicy));
    }
  }

  
  SetContentTypeInternal(contentType);

  
  
  shell->PrepareForNewContentModel();

  
  
  
  
  shell->SetLoadType(aReplace.LowerCaseEqualsLiteral("replace") ?
                       LOAD_NORMAL_REPLACE : LOAD_NORMAL);

  nsCOMPtr<nsIContentViewer> cv;
  shell->GetContentViewer(getter_AddRefs(cv));
  if (cv) {
    cv->LoadStart(this);
  }

  
  NS_ASSERTION(!mWyciwygChannel, "nsHTMLDocument::Open(): wyciwyg "
               "channel already exists!");

  
  
  
  
  ++mWriteLevel;

  CreateAndAddWyciwygChannel();

  --mWriteLevel;

  SetReadyStateInternal(nsIDocument::READYSTATE_LOADING);

  
  
  DebugOnly<JSObject*> wrapper = GetWrapperPreserveColor();
  MOZ_ASSERT_IF(wrapper,
                JS_GetCompartmentPrincipals(js::GetObjectCompartment(wrapper)) ==
                nsJSPrincipals::get(NodePrincipal()));

  return kungFuDeathGrip.forget();
}

NS_IMETHODIMP
nsHTMLDocument::Clear()
{
  
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::Close()
{
  ErrorResult rv;
  Close(rv);
  return rv.StealNSResult();
}

void
nsHTMLDocument::Close(ErrorResult& rv)
{
  if (!IsHTMLDocument()) {
    

    rv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  if (!mParser || !mParser->IsScriptCreated()) {
    return;
  }

  ++mWriteLevel;
  rv = (static_cast<nsHtml5Parser*>(mParser.get()))->Parse(
    EmptyString(), nullptr, GetContentTypeInternal(), true);
  --mWriteLevel;

  

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (GetShell()) {
    FlushPendingNotifications(Flush_Layout);
  }

  
  
  
  NS_ASSERTION(mWyciwygChannel, "nsHTMLDocument::Close(): Trying to remove "
               "nonexistent wyciwyg channel!");
  RemoveWyciwygChannel();
  NS_ASSERTION(!mWyciwygChannel, "nsHTMLDocument::Close(): "
               "nsIWyciwygChannel could not be removed!");
}

void
nsHTMLDocument::WriteCommon(JSContext *cx,
                            const Sequence<nsString>& aText,
                            bool aNewlineTerminate,
                            mozilla::ErrorResult& rv)
{
  
  if (aText.Length() == 1) {
    rv = WriteCommon(cx, aText[0], aNewlineTerminate);
  } else {
    
    
    
    nsString text;
    for (uint32_t i = 0; i < aText.Length(); ++i) {
      text.Append(aText[i]);
    }
    rv = WriteCommon(cx, text, aNewlineTerminate);
  }
}

nsresult
nsHTMLDocument::WriteCommon(JSContext *cx,
                            const nsAString& aText,
                            bool aNewlineTerminate)
{
  mTooDeepWriteRecursion =
    (mWriteLevel > NS_MAX_DOCUMENT_WRITE_DEPTH || mTooDeepWriteRecursion);
  NS_ENSURE_STATE(!mTooDeepWriteRecursion);

  if (!IsHTMLDocument() || mDisableDocWrite || !IsMasterDocument()) {
    

    return NS_ERROR_DOM_INVALID_STATE_ERR;
  }

  if (mParserAborted) {
    
    
    
    return NS_OK;
  }

  nsresult rv = NS_OK;

  void *key = GenerateParserKey();
  if (mParser && !mParser->IsInsertionPointDefined()) {
    if (mExternalScriptsBeingEvaluated) {
      
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("DOM Events"), this,
                                      nsContentUtils::eDOM_PROPERTIES,
                                      "DocumentWriteIgnored",
                                      nullptr, 0,
                                      mDocumentURI);
      return NS_OK;
    }
    mParser->Terminate();
    NS_ASSERTION(!mParser, "mParser should have been null'd out");
  }

  if (!mParser) {
    if (mExternalScriptsBeingEvaluated) {
      
      nsContentUtils::ReportToConsole(nsIScriptError::warningFlag,
                                      NS_LITERAL_CSTRING("DOM Events"), this,
                                      nsContentUtils::eDOM_PROPERTIES,
                                      "DocumentWriteIgnored",
                                      nullptr, 0,
                                      mDocumentURI);
      return NS_OK;
    }
    nsCOMPtr<nsISupports> ignored;
    rv = Open(NS_LITERAL_STRING("text/html"), EmptyString(), EmptyString(), cx,
              1, getter_AddRefs(ignored));

    
    
    
    if (NS_FAILED(rv) || !mParser) {
      return rv;
    }
    MOZ_ASSERT(!JS_IsExceptionPending(cx),
               "Open() succeeded but JS exception is pending");
  }

  static NS_NAMED_LITERAL_STRING(new_line, "\n");

  
  if (mWyciwygChannel && !key) {
    if (!aText.IsEmpty()) {
      mWyciwygChannel->WriteToCacheEntry(aText);
    }

    if (aNewlineTerminate) {
      mWyciwygChannel->WriteToCacheEntry(new_line);
    }
  }

  ++mWriteLevel;

  
  
  
  
  if (aNewlineTerminate) {
    rv = (static_cast<nsHtml5Parser*>(mParser.get()))->Parse(
      aText + new_line, key, GetContentTypeInternal(), false);
  } else {
    rv = (static_cast<nsHtml5Parser*>(mParser.get()))->Parse(
      aText, key, GetContentTypeInternal(), false);
  }

  --mWriteLevel;

  mTooDeepWriteRecursion = (mWriteLevel != 0 && mTooDeepWriteRecursion);

  return rv;
}

NS_IMETHODIMP
nsHTMLDocument::Write(const nsAString& aText, JSContext *cx)
{
  return WriteCommon(cx, aText, false);
}

void
nsHTMLDocument::Write(JSContext* cx, const Sequence<nsString>& aText,
                      ErrorResult& rv)
{
  WriteCommon(cx, aText, false, rv);
}

NS_IMETHODIMP
nsHTMLDocument::Writeln(const nsAString& aText, JSContext *cx)
{
  return WriteCommon(cx, aText, true);
}

void
nsHTMLDocument::Writeln(JSContext* cx, const Sequence<nsString>& aText,
                        ErrorResult& rv)
{
  WriteCommon(cx, aText, true, rv);
}

bool
nsHTMLDocument::MatchNameAttribute(nsIContent* aContent, int32_t aNamespaceID,
                                   nsIAtom* aAtom, void* aData)
{
  NS_PRECONDITION(aContent, "Must have content node to work with!");
  nsString* elementName = static_cast<nsString*>(aData);
  return
    aContent->GetNameSpaceID() == kNameSpaceID_XHTML &&
    aContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::name,
                          *elementName, eCaseMatters);
}


void*
nsHTMLDocument::UseExistingNameString(nsINode* aRootNode, const nsString* aName)
{
  return const_cast<nsString*>(aName);
}

NS_IMETHODIMP
nsHTMLDocument::GetElementsByName(const nsAString& aElementName,
                                  nsIDOMNodeList** aReturn)
{
  *aReturn = GetElementsByName(aElementName).take();
  return NS_OK;
}

static bool MatchItems(nsIContent* aContent, int32_t aNameSpaceID, 
                       nsIAtom* aAtom, void* aData)
{
  if (!aContent->IsHTMLElement()) {
    return false;
  }

  nsGenericHTMLElement* elem = static_cast<nsGenericHTMLElement*>(aContent);
  if (!elem->HasAttr(kNameSpaceID_None, nsGkAtoms::itemscope) ||
      elem->HasAttr(kNameSpaceID_None, nsGkAtoms::itemprop)) {
    return false;
  }

  nsTArray<nsCOMPtr<nsIAtom> >* tokens = static_cast<nsTArray<nsCOMPtr<nsIAtom> >*>(aData);
  if (tokens->IsEmpty()) {
    return true;
  }
 
  const nsAttrValue* attr = elem->GetParsedAttr(nsGkAtoms::itemtype);
  if (!attr)
    return false;

  for (uint32_t i = 0; i < tokens->Length(); i++) {
    if (!attr->Contains(tokens->ElementAt(i), eCaseMatters)) {
      return false;
    }
  }
  return true;
}

static void DestroyTokens(void* aData)
{
  nsTArray<nsCOMPtr<nsIAtom> >* tokens = static_cast<nsTArray<nsCOMPtr<nsIAtom> >*>(aData);
  delete tokens;
}

static void* CreateTokens(nsINode* aRootNode, const nsString* types)
{
  nsTArray<nsCOMPtr<nsIAtom> >* tokens = new nsTArray<nsCOMPtr<nsIAtom> >();
  nsAString::const_iterator iter, end;
  types->BeginReading(iter);
  types->EndReading(end);
  
  
  while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
    ++iter;
  }

  
  while (iter != end) {
    nsAString::const_iterator start(iter);

    do {
      ++iter;
    } while (iter != end && !nsContentUtils::IsHTMLWhitespace(*iter));

    tokens->AppendElement(do_GetAtom(Substring(start, iter)));

    
    while (iter != end && nsContentUtils::IsHTMLWhitespace(*iter)) {
      ++iter;
    }
  }
  return tokens;
}

NS_IMETHODIMP
nsHTMLDocument::GetItems(const nsAString& types, nsIDOMNodeList** aReturn)
{
  *aReturn = GetItems(types).take();
  return NS_OK;
}

already_AddRefed<nsINodeList>
nsHTMLDocument::GetItems(const nsAString& aTypeNames)
{
  return NS_GetFuncStringNodeList(this, MatchItems, DestroyTokens, CreateTokens,
                                  aTypeNames);
}

void
nsHTMLDocument::AddedForm()
{
  ++mNumForms;
}

void
nsHTMLDocument::RemovedForm()
{
  --mNumForms;
}

int32_t
nsHTMLDocument::GetNumFormsSynchronous()
{
  return mNumForms;
}

NS_IMETHODIMP
nsHTMLDocument::GetAlinkColor(nsAString& aAlinkColor)
{
  aAlinkColor.Truncate();

  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->GetALink(aAlinkColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetAlinkColor(const nsAString& aAlinkColor)
{
  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->SetALink(aAlinkColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetLinkColor(nsAString& aLinkColor)
{
  aLinkColor.Truncate();

  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->GetLink(aLinkColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetLinkColor(const nsAString& aLinkColor)
{
  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->SetLink(aLinkColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetVlinkColor(nsAString& aVlinkColor)
{
  aVlinkColor.Truncate();

  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->GetVLink(aVlinkColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetVlinkColor(const nsAString& aVlinkColor)
{
  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->SetVLink(aVlinkColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetBgColor(nsAString& aBgColor)
{
  aBgColor.Truncate();

  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->GetBgColor(aBgColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetBgColor(const nsAString& aBgColor)
{
  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->SetBgColor(aBgColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::GetFgColor(nsAString& aFgColor)
{
  aFgColor.Truncate();

  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->GetText(aFgColor);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetFgColor(const nsAString& aFgColor)
{
  HTMLBodyElement* body = GetBodyElement();
  if (body) {
    body->SetText(aFgColor);
  }

  return NS_OK;
}


NS_IMETHODIMP
nsHTMLDocument::GetEmbeds(nsIDOMHTMLCollection** aEmbeds)
{
  NS_ADDREF(*aEmbeds = Embeds());
  return NS_OK;
}

nsIHTMLCollection*
nsHTMLDocument::Embeds()
{
  if (!mEmbeds) {
    mEmbeds = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::embed, nsGkAtoms::embed);
  }
  return mEmbeds;
}

NS_IMETHODIMP
nsHTMLDocument::GetSelection(nsISelection** aReturn)
{
  ErrorResult rv;
  NS_IF_ADDREF(*aReturn = GetSelection(rv));
  return rv.StealNSResult();
}

Selection*
nsHTMLDocument::GetSelection(ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(GetScopeObject());
  if (!window) {
    return nullptr;
  }

  NS_ASSERTION(window->IsInnerWindow(), "Should have inner window here!");
  if (!window->IsCurrentInnerWindow()) {
    return nullptr;
  }

  return static_cast<nsGlobalWindow*>(window.get())->GetSelection(aRv);
}

NS_IMETHODIMP
nsHTMLDocument::CaptureEvents()
{
  WarnOnceAbout(nsIDocument::eUseOfCaptureEvents);
  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::ReleaseEvents()
{
  WarnOnceAbout(nsIDocument::eUseOfReleaseEvents);
  return NS_OK;
}


NS_IMETHODIMP
nsHTMLDocument::GetPlugins(nsIDOMHTMLCollection** aPlugins)
{
  *aPlugins = nullptr;

  return GetEmbeds(aPlugins);
}

nsIHTMLCollection*
nsHTMLDocument::Plugins()
{
  return Embeds();
}

nsISupports*
nsHTMLDocument::ResolveName(const nsAString& aName, nsWrapperCache **aCache)
{
  nsIdentifierMapEntry *entry = mIdentifierMap.GetEntry(aName);
  if (!entry) {
    *aCache = nullptr;
    return nullptr;
  }

  nsBaseContentList *list = entry->GetNameContentList();
  uint32_t length = list ? list->Length() : 0;

  if (length > 0) {
    if (length == 1) {
      
      
      nsIContent *node = list->Item(0);
      *aCache = node;
      return node;
    }

    
    *aCache = list;
    return list;
  }

  
  Element *e = entry->GetIdElement();

  if (e && nsGenericHTMLElement::ShouldExposeIdAsHTMLDocumentProperty(e)) {
    *aCache = e;
    return e;
  }

  *aCache = nullptr;
  return nullptr;
}

void
nsHTMLDocument::NamedGetter(JSContext* cx, const nsAString& aName, bool& aFound,
                            JS::MutableHandle<JSObject*> aRetval,
                            ErrorResult& rv)
{
  nsWrapperCache* cache;
  nsISupports* supp = ResolveName(aName, &cache);
  if (!supp) {
    aFound = false;
    aRetval.set(nullptr);
    return;
  }

  JS::Rooted<JS::Value> val(cx);
  
  
  if (!dom::WrapObject(cx, supp, cache, nullptr, &val)) {
    rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }
  aFound = true;
  aRetval.set(&val.toObject());
}

bool
nsHTMLDocument::NameIsEnumerable(const nsAString& aName)
{
  return true;
}

static PLDHashOperator
IdentifierMapEntryAddNames(nsIdentifierMapEntry* aEntry, void* aArg)
{
  nsTArray<nsString>* aNames = static_cast<nsTArray<nsString>*>(aArg);
  if (aEntry->HasNameElement() ||
      aEntry->HasIdElementExposedAsHTMLDocumentProperty()) {
    aNames->AppendElement(aEntry->GetKey());
  }
  return PL_DHASH_NEXT;
}

void
nsHTMLDocument::GetSupportedNames(unsigned, nsTArray<nsString>& aNames)
{
  mIdentifierMap.EnumerateEntries(IdentifierMapEntryAddNames, &aNames);
}





NS_IMETHODIMP
nsHTMLDocument::GetForms(nsIDOMHTMLCollection** aForms)
{
  NS_ADDREF(*aForms = nsHTMLDocument::GetForms());
  return NS_OK;
}

nsContentList*
nsHTMLDocument::GetForms()
{
  if (!mForms) {
    mForms = new nsContentList(this, kNameSpaceID_XHTML, nsGkAtoms::form, nsGkAtoms::form);
  }

  return mForms;
}

static bool MatchFormControls(nsIContent* aContent, int32_t aNamespaceID,
                                nsIAtom* aAtom, void* aData)
{
  return aContent->IsNodeOfType(nsIContent::eHTML_FORM_CONTROL);
}

nsContentList*
nsHTMLDocument::GetFormControls()
{
  if (!mFormControls) {
    mFormControls = new nsContentList(this, MatchFormControls, nullptr, nullptr);
  }

  return mFormControls;
}

nsresult
nsHTMLDocument::CreateAndAddWyciwygChannel(void)
{
  nsresult rv = NS_OK;
  nsAutoCString url, originalSpec;

  mDocumentURI->GetSpec(originalSpec);

  
  url = NS_LITERAL_CSTRING("wyciwyg://")
      + nsPrintfCString("%d", gWyciwygSessionCnt++)
      + NS_LITERAL_CSTRING("/")
      + originalSpec;

  nsCOMPtr<nsIURI> wcwgURI;
  NS_NewURI(getter_AddRefs(wcwgURI), url);

  
  
  nsCOMPtr<nsIChannel> channel;
  
  rv = NS_NewChannel(getter_AddRefs(channel),
                     wcwgURI,
                     NodePrincipal(),
                     nsILoadInfo::SEC_FORCE_INHERIT_PRINCIPAL,
                     nsIContentPolicy::TYPE_OTHER);

  NS_ENSURE_SUCCESS(rv, rv);

  mWyciwygChannel = do_QueryInterface(channel);

  mWyciwygChannel->SetSecurityInfo(mSecurityInfo);

  
  
  SetDocumentCharacterSetSource(kCharsetFromHintPrevDoc);
  mWyciwygChannel->SetCharsetAndSource(kCharsetFromHintPrevDoc,
                                       GetDocumentCharacterSet());

  
  channel->SetLoadFlags(mLoadFlags);

  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();

  
  if (loadGroup && channel) {
    rv = channel->SetLoadGroup(loadGroup);
    NS_ENSURE_SUCCESS(rv, rv);

    nsLoadFlags loadFlags = 0;
    channel->GetLoadFlags(&loadFlags);
    loadFlags |= nsIChannel::LOAD_DOCUMENT_URI;
    channel->SetLoadFlags(loadFlags);

    channel->SetOriginalURI(wcwgURI);

    rv = loadGroup->AddRequest(mWyciwygChannel, nullptr);
    NS_ASSERTION(NS_SUCCEEDED(rv), "Failed to add request to load group.");
  }

  return rv;
}

nsresult
nsHTMLDocument::RemoveWyciwygChannel(void)
{
  nsCOMPtr<nsILoadGroup> loadGroup = GetDocumentLoadGroup();

  
  
  if (loadGroup && mWyciwygChannel) {
    mWyciwygChannel->CloseCacheEntry(NS_OK);
    loadGroup->RemoveRequest(mWyciwygChannel, nullptr, NS_OK);
  }

  mWyciwygChannel = nullptr;

  return NS_OK;
}

void *
nsHTMLDocument::GenerateParserKey(void)
{
  if (!mScriptLoader) {
    
    
    return nullptr;
  }

  
  
  nsIScriptElement* script = mScriptLoader->GetCurrentParserInsertedScript();
  if (script && mParser && mParser->IsScriptCreated()) {
    nsCOMPtr<nsIParser> creatorParser = script->GetCreatorParser();
    if (creatorParser != mParser) {
      
      
      
      return nullptr;
    }
  }
  return script;
}


NS_IMETHODIMP
nsHTMLDocument::GetDesignMode(nsAString & aDesignMode)
{
  if (HasFlag(NODE_IS_EDITABLE)) {
    aDesignMode.AssignLiteral("on");
  }
  else {
    aDesignMode.AssignLiteral("off");
  }
  return NS_OK;
}

void
nsHTMLDocument::MaybeEditingStateChanged()
{
  if (!mPendingMaybeEditingStateChanged &&
      mUpdateNestLevel == 0 && (mContentEditableCount > 0) != IsEditingOn()) {
    if (nsContentUtils::IsSafeToRunScript()) {
      EditingStateChanged();
    } else if (!mInDestructor) {
      nsContentUtils::AddScriptRunner(
        NS_NewRunnableMethod(this, &nsHTMLDocument::MaybeEditingStateChanged));
    }
  }
}

void
nsHTMLDocument::EndUpdate(nsUpdateType aUpdateType)
{
  const bool reset = !mPendingMaybeEditingStateChanged;
  mPendingMaybeEditingStateChanged = true;
  nsDocument::EndUpdate(aUpdateType);
  if (reset) {
    mPendingMaybeEditingStateChanged = false;
  }
  MaybeEditingStateChanged();
}



class DeferredContentEditableCountChangeEvent : public nsRunnable
{
public:
  DeferredContentEditableCountChangeEvent(nsHTMLDocument *aDoc,
                                          nsIContent *aElement)
    : mDoc(aDoc)
    , mElement(aElement)
  {
  }

  NS_IMETHOD Run() {
    if (mElement && mElement->OwnerDoc() == mDoc) {
      mDoc->DeferredContentEditableCountChange(mElement);
    }
    return NS_OK;
  }

private:
  nsRefPtr<nsHTMLDocument> mDoc;
  nsCOMPtr<nsIContent> mElement;
};

nsresult
nsHTMLDocument::ChangeContentEditableCount(nsIContent *aElement,
                                           int32_t aChange)
{
  NS_ASSERTION(int32_t(mContentEditableCount) + aChange >= 0,
               "Trying to decrement too much.");

  mContentEditableCount += aChange;

  nsContentUtils::AddScriptRunner(
    new DeferredContentEditableCountChangeEvent(this, aElement));

  return NS_OK;
}

void
nsHTMLDocument::DeferredContentEditableCountChange(nsIContent *aElement)
{
  if (mParser ||
      (mUpdateNestLevel > 0 && (mContentEditableCount > 0) != IsEditingOn())) {
    return;
  }

  EditingState oldState = mEditingState;

  nsresult rv = EditingStateChanged();
  NS_ENSURE_SUCCESS_VOID(rv);

  if (oldState == mEditingState && mEditingState == eContentEditable) {
    
    
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(aElement);
    if (node) {
      nsPIDOMWindow *window = GetWindow();
      if (!window)
        return;

      nsIDocShell *docshell = window->GetDocShell();
      if (!docshell)
        return;

      nsCOMPtr<nsIEditor> editor;
      docshell->GetEditor(getter_AddRefs(editor));
      if (editor) {
        nsRefPtr<nsRange> range = new nsRange(aElement);
        rv = range->SelectNode(node);
        if (NS_FAILED(rv)) {
          
          
          
          return;
        }

        nsCOMPtr<nsIInlineSpellChecker> spellChecker;
        rv = editor->GetInlineSpellChecker(false,
                                           getter_AddRefs(spellChecker));
        NS_ENSURE_SUCCESS_VOID(rv);

        if (spellChecker) {
          rv = spellChecker->SpellCheckRange(range);
        }
      }
    }
  }
}

HTMLAllCollection*
nsHTMLDocument::All()
{
  if (!mAll) {
    mAll = new HTMLAllCollection(this);
  }
  return mAll;
}

static void
NotifyEditableStateChange(nsINode *aNode, nsIDocument *aDocument)
{
  for (nsIContent* child = aNode->GetFirstChild();
       child;
       child = child->GetNextSibling()) {
    if (child->IsElement()) {
      child->AsElement()->UpdateState(true);
    }
    NotifyEditableStateChange(child, aDocument);
  }
}

void
nsHTMLDocument::TearingDownEditor(nsIEditor *aEditor)
{
  if (IsEditingOn()) {
    EditingState oldState = mEditingState;
    mEditingState = eTearingDown;

    nsCOMPtr<nsIPresShell> presShell = GetShell();
    if (!presShell)
      return;

    nsCOMArray<nsIStyleSheet> agentSheets;
    presShell->GetAgentStyleSheets(agentSheets);

    RemoveFromAgentSheets(agentSheets, NS_LITERAL_STRING("resource://gre/res/contenteditable.css"));
    if (oldState == eDesignMode)
      RemoveFromAgentSheets(agentSheets, NS_LITERAL_STRING("resource://gre/res/designmode.css"));

    presShell->SetAgentStyleSheets(agentSheets);

    presShell->ReconstructStyleData();
  }
}

nsresult
nsHTMLDocument::TurnEditingOff()
{
  NS_ASSERTION(mEditingState != eOff, "Editing is already off.");

  nsPIDOMWindow *window = GetWindow();
  if (!window)
    return NS_ERROR_FAILURE;

  nsIDocShell *docshell = window->GetDocShell();
  if (!docshell)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCOMPtr<nsIEditingSession> editSession = do_GetInterface(docshell, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = editSession->TearDownEditorOnWindow(window);
  NS_ENSURE_SUCCESS(rv, rv);

  mEditingState = eOff;

  return NS_OK;
}

static bool HasPresShell(nsPIDOMWindow *aWindow)
{
  nsIDocShell *docShell = aWindow->GetDocShell();
  if (!docShell)
    return false;
  return docShell->GetPresShell() != nullptr;
}

nsresult
nsHTMLDocument::SetEditingState(EditingState aState)
{
  mEditingState = aState;
  return NS_OK;
}

nsresult
nsHTMLDocument::EditingStateChanged()
{
  if (mRemovedFromDocShell) {
    return NS_OK;
  }

  if (mEditingState == eSettingUp || mEditingState == eTearingDown) {
    
    return NS_OK;
  }

  bool designMode = HasFlag(NODE_IS_EDITABLE);
  EditingState newState = designMode ? eDesignMode :
                          (mContentEditableCount > 0 ? eContentEditable : eOff);
  if (mEditingState == newState) {
    
    return NS_OK;
  }

  if (newState == eOff) {
    
    nsAutoScriptBlocker scriptBlocker;
    NotifyEditableStateChange(this, this);
    return TurnEditingOff();
  }

  
  
  if (mParentDocument) {
    mParentDocument->FlushPendingNotifications(Flush_Style);
  }

  
  
  nsCOMPtr<nsPIDOMWindow> window = GetWindow();
  if (!window)
    return NS_ERROR_FAILURE;

  nsIDocShell *docshell = window->GetDocShell();
  if (!docshell)
    return NS_ERROR_FAILURE;

  nsresult rv;
  nsCOMPtr<nsIEditingSession> editSession = do_GetInterface(docshell, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIEditor> existingEditor;
  editSession->GetEditorForWindow(window, getter_AddRefs(existingEditor));
  if (existingEditor) {
    
    
    nsCOMPtr<nsIHTMLEditor> htmlEditor = do_QueryInterface(existingEditor);
    MOZ_ASSERT(htmlEditor, "If we have an editor, it must be an HTML editor");
    uint32_t flags = 0;
    existingEditor->GetFlags(&flags);
    if (flags & nsIPlaintextEditor::eEditorMailMask) {
      
      
      return NS_OK;
    }
  }

  if (!HasPresShell(window)) {
    
    
    return NS_OK;
  }

  bool makeWindowEditable = mEditingState == eOff;
  bool updateState = false;
  bool spellRecheckAll = false;
  bool putOffToRemoveScriptBlockerUntilModifyingEditingState = false;
  nsCOMPtr<nsIEditor> editor;

  {
    EditingState oldState = mEditingState;
    nsAutoEditingState push(this, eSettingUp);

    nsCOMPtr<nsIPresShell> presShell = GetShell();
    NS_ENSURE_TRUE(presShell, NS_ERROR_FAILURE);

    
    
    
    nsCOMArray<nsIStyleSheet> agentSheets;
    rv = presShell->GetAgentStyleSheets(agentSheets);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri),
                   NS_LITERAL_STRING("resource://gre/res/contenteditable.css"));
    NS_ENSURE_SUCCESS(rv, rv);

    nsRefPtr<CSSStyleSheet> sheet;
    rv = LoadChromeSheetSync(uri, true, getter_AddRefs(sheet));
    NS_ENSURE_TRUE(sheet, rv);

    bool result = agentSheets.AppendObject(sheet);
    NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

    
    
    
    if (designMode) {
      
      rv = NS_NewURI(getter_AddRefs(uri),
                     NS_LITERAL_STRING("resource://gre/res/designmode.css"));
      NS_ENSURE_SUCCESS(rv, rv);

      rv = LoadChromeSheetSync(uri, true, getter_AddRefs(sheet));
      NS_ENSURE_TRUE(sheet, rv);

      result = agentSheets.AppendObject(sheet);
      NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);

      updateState = true;
      spellRecheckAll = oldState == eContentEditable;
    }
    else if (oldState == eDesignMode) {
      
      RemoveFromAgentSheets(agentSheets,
        NS_LITERAL_STRING("resource://gre/res/designmode.css"));

      updateState = true;
    }

    rv = presShell->SetAgentStyleSheets(agentSheets);
    NS_ENSURE_SUCCESS(rv, rv);

    presShell->ReconstructStyleData();

    
    
    nsAutoScriptBlocker scriptBlocker;
    if (designMode) {
      nsCOMPtr<nsPIDOMWindow> focusedWindow;
      nsIContent* focusedContent =
        nsFocusManager::GetFocusedDescendant(window, false,
                                             getter_AddRefs(focusedWindow));
      if (focusedContent) {
        nsIFrame* focusedFrame = focusedContent->GetPrimaryFrame();
        bool clearFocus = focusedFrame ? !focusedFrame->IsFocusable() :
                                         !focusedContent->IsFocusable();
        if (clearFocus) {
          nsFocusManager* fm = nsFocusManager::GetFocusManager();
          if (fm) {
            fm->ClearFocus(window);
            
            
            
            putOffToRemoveScriptBlockerUntilModifyingEditingState = true;
          }
        }
      }
    }

    if (makeWindowEditable) {
      
      
      
      
      rv = editSession->MakeWindowEditable(window, "html", false, false,
                                           true);
      NS_ENSURE_SUCCESS(rv, rv);
    }

    
    docshell->GetEditor(getter_AddRefs(editor));
    if (!editor)
      return NS_ERROR_FAILURE;

    
    
    if (designMode && oldState == eOff) {
      editor->BeginningOfDocument();
    }

    if (putOffToRemoveScriptBlockerUntilModifyingEditingState) {
      nsContentUtils::AddScriptBlocker();
    }
  }

  mEditingState = newState;
  if (putOffToRemoveScriptBlockerUntilModifyingEditingState) {
    nsContentUtils::RemoveScriptBlocker();
    
    
    if (mEditingState == eOff) {
      return NS_OK;
    }
  }

  if (makeWindowEditable) {
    
    
    
    bool unused;
    rv = ExecCommand(NS_LITERAL_STRING("insertBrOnReturn"), false,
                     NS_LITERAL_STRING("false"), &unused);

    if (NS_FAILED(rv)) {
      
      
      editSession->TearDownEditorOnWindow(window);
      mEditingState = eOff;

      return rv;
    }
  }

  if (updateState) {
    nsAutoScriptBlocker scriptBlocker;
    NotifyEditableStateChange(this, this);
  }

  
  if (spellRecheckAll) {
    nsCOMPtr<nsISelectionController> selcon;
    nsresult rv = editor->GetSelectionController(getter_AddRefs(selcon));
    NS_ENSURE_SUCCESS(rv, rv); 

    nsCOMPtr<nsISelection> spellCheckSelection;
    rv = selcon->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                              getter_AddRefs(spellCheckSelection));
    if (NS_SUCCEEDED(rv)) {
      spellCheckSelection->RemoveAllRanges();
    }
  }
  editor->SyncRealTimeSpell();

  return NS_OK;
}

NS_IMETHODIMP
nsHTMLDocument::SetDesignMode(const nsAString & aDesignMode)
{
  ErrorResult rv;
  SetDesignMode(aDesignMode, rv);
  return rv.StealNSResult();
}

void
nsHTMLDocument::SetDesignMode(const nsAString& aDesignMode, ErrorResult& rv)
{
  if (!nsContentUtils::SubjectPrincipal()->Subsumes(NodePrincipal())) {
    rv.Throw(NS_ERROR_DOM_PROP_ACCESS_DENIED);
    return;
  }
  bool editableMode = HasFlag(NODE_IS_EDITABLE);
  if (aDesignMode.LowerCaseEqualsASCII(editableMode ? "off" : "on")) {
    SetEditableFlag(!editableMode);

    rv = EditingStateChanged();
  }
}

nsresult
nsHTMLDocument::GetMidasCommandManager(nsICommandManager** aCmdMgr)
{
  
  NS_ENSURE_ARG_POINTER(aCmdMgr);

  
  if (mMidasCommandManager) {
    NS_ADDREF(*aCmdMgr = mMidasCommandManager);
    return NS_OK;
  }

  *aCmdMgr = nullptr;

  nsPIDOMWindow *window = GetWindow();
  if (!window)
    return NS_ERROR_FAILURE;

  nsIDocShell *docshell = window->GetDocShell();
  if (!docshell)
    return NS_ERROR_FAILURE;

  mMidasCommandManager = do_GetInterface(docshell);
  if (!mMidasCommandManager)
    return NS_ERROR_FAILURE;

  NS_ADDREF(*aCmdMgr = mMidasCommandManager);

  return NS_OK;
}


struct MidasCommand {
  const char*  incomingCommandString;
  const char*  internalCommandString;
  const char*  internalParamString;
  bool useNewParam;
  bool convertToBoolean;
};

static const struct MidasCommand gMidasCommandTable[] = {
  { "bold",          "cmd_bold",            "", true,  false },
  { "italic",        "cmd_italic",          "", true,  false },
  { "underline",     "cmd_underline",       "", true,  false },
  { "strikethrough", "cmd_strikethrough",   "", true,  false },
  { "subscript",     "cmd_subscript",       "", true,  false },
  { "superscript",   "cmd_superscript",     "", true,  false },
  { "cut",           "cmd_cut",             "", true,  false },
  { "copy",          "cmd_copy",            "", true,  false },
  { "paste",         "cmd_paste",           "", true,  false },
  { "delete",        "cmd_deleteCharBackward", "", true,  false },
  { "forwarddelete", "cmd_deleteCharForward", "", true,  false },
  { "selectall",     "cmd_selectAll",       "", true,  false },
  { "undo",          "cmd_undo",            "", true,  false },
  { "redo",          "cmd_redo",            "", true,  false },
  { "indent",        "cmd_indent",          "", true,  false },
  { "outdent",       "cmd_outdent",         "", true,  false },
  { "backcolor",     "cmd_highlight",       "", false, false },
  { "forecolor",     "cmd_fontColor",       "", false, false },
  { "hilitecolor",   "cmd_highlight",       "", false, false },
  { "fontname",      "cmd_fontFace",        "", false, false },
  { "fontsize",      "cmd_fontSize",        "", false, false },
  { "increasefontsize", "cmd_increaseFont", "", false, false },
  { "decreasefontsize", "cmd_decreaseFont", "", false, false },
  { "inserthorizontalrule", "cmd_insertHR", "", true,  false },
  { "createlink",    "cmd_insertLinkNoUI",  "", false, false },
  { "insertimage",   "cmd_insertImageNoUI", "", false, false },
  { "inserthtml",    "cmd_insertHTML",      "", false, false },
  { "inserttext",    "cmd_insertText",      "", false, false },
  { "gethtml",       "cmd_getContents",     "", false, false },
  { "justifyleft",   "cmd_align",       "left", true,  false },
  { "justifyright",  "cmd_align",      "right", true,  false },
  { "justifycenter", "cmd_align",     "center", true,  false },
  { "justifyfull",   "cmd_align",    "justify", true,  false },
  { "removeformat",  "cmd_removeStyles",    "", true,  false },
  { "unlink",        "cmd_removeLinks",     "", true,  false },
  { "insertorderedlist",   "cmd_ol",        "", true,  false },
  { "insertunorderedlist", "cmd_ul",        "", true,  false },
  { "insertparagraph", "cmd_paragraphState", "p", true,  false },
  { "formatblock",   "cmd_paragraphState",  "", false, false },
  { "heading",       "cmd_paragraphState",  "", false, false },
  { "styleWithCSS",  "cmd_setDocumentUseCSS", "", false, true },
  { "contentReadOnly", "cmd_setDocumentReadOnly", "", false, true },
  { "insertBrOnReturn", "cmd_insertBrOnReturn", "", false, true },
  { "enableObjectResizing", "cmd_enableObjectResizing", "", false, true },
  { "enableInlineTableEditing", "cmd_enableInlineTableEditing", "", false, true },
#if 0

  { "justifynone",   "cmd_align",           "", true,  false },


  { "saveas",        "cmd_saveAs",          "", true,  false },
  { "print",         "cmd_print",           "", true,  false },
#endif
  { nullptr, nullptr, nullptr, false, false }
};

#define MidasCommandCount ((sizeof(gMidasCommandTable) / sizeof(struct MidasCommand)) - 1)

static const char* const gBlocks[] = {
  "ADDRESS",
  "BLOCKQUOTE",
  "DD",
  "DIV",
  "DL",
  "DT",
  "H1",
  "H2",
  "H3",
  "H4",
  "H5",
  "H6",
  "P",
  "PRE"
};

static bool
ConvertToMidasInternalCommandInner(const nsAString& inCommandID,
                                   const nsAString& inParam,
                                   nsACString& outCommandID,
                                   nsACString& outParam,
                                   bool& outIsBoolean,
                                   bool& outBooleanValue,
                                   bool aIgnoreParams)
{
  NS_ConvertUTF16toUTF8 convertedCommandID(inCommandID);

  
  bool invertBool = false;
  if (convertedCommandID.LowerCaseEqualsLiteral("usecss")) {
    convertedCommandID.AssignLiteral("styleWithCSS");
    invertBool = true;
  } else if (convertedCommandID.LowerCaseEqualsLiteral("readonly")) {
    convertedCommandID.AssignLiteral("contentReadOnly");
    invertBool = true;
  }

  uint32_t i;
  bool found = false;
  for (i = 0; i < MidasCommandCount; ++i) {
    if (convertedCommandID.Equals(gMidasCommandTable[i].incomingCommandString,
                                  nsCaseInsensitiveCStringComparator())) {
      found = true;
      break;
    }
  }

  if (!found) {
    
    outCommandID.SetLength(0);
    outParam.SetLength(0);
    outIsBoolean = false;
    return false;
  }

  
  outCommandID.Assign(gMidasCommandTable[i].internalCommandString);

  
  outIsBoolean = gMidasCommandTable[i].convertToBoolean;

  if (aIgnoreParams) {
    
    return true;
  }

  if (gMidasCommandTable[i].useNewParam) {
    
    outParam.Assign(gMidasCommandTable[i].internalParamString);
    return true;
  }

  
  if (outIsBoolean) {
    
    
    
    if (invertBool) {
      outBooleanValue = inParam.LowerCaseEqualsLiteral("false");
    } else {
      outBooleanValue = !inParam.LowerCaseEqualsLiteral("false");
    }
    outParam.Truncate();

    return true;
  }

  
  
  if (outCommandID.EqualsLiteral("cmd_paragraphState")) {
    const char16_t* start = inParam.BeginReading();
    const char16_t* end = inParam.EndReading();
    if (start != end && *start == '<' && *(end - 1) == '>') {
      ++start;
      --end;
    }

    NS_ConvertUTF16toUTF8 convertedParam(Substring(start, end));
    uint32_t j;
    for (j = 0; j < ArrayLength(gBlocks); ++j) {
      if (convertedParam.Equals(gBlocks[j],
                                nsCaseInsensitiveCStringComparator())) {
        outParam.Assign(gBlocks[j]);
        break;
      }
    }

    if (j == ArrayLength(gBlocks)) {
      outParam.Truncate();
    }
  } else if (outCommandID.EqualsLiteral("cmd_fontSize")) {
    
    
    
    
    outParam.Truncate();
    int32_t size = nsContentUtils::ParseLegacyFontSize(inParam);
    if (size) {
      outParam.AppendInt(size);
    }
  } else {
    CopyUTF16toUTF8(inParam, outParam);
  }

  return true;
}

static bool
ConvertToMidasInternalCommand(const nsAString & inCommandID,
                              const nsAString & inParam,
                              nsACString& outCommandID,
                              nsACString& outParam,
                              bool& outIsBoolean,
                              bool& outBooleanValue)
{
  return ConvertToMidasInternalCommandInner(inCommandID, inParam, outCommandID,
                                            outParam, outIsBoolean,
                                            outBooleanValue, false);
}

static bool
ConvertToMidasInternalCommand(const nsAString & inCommandID,
                              nsACString& outCommandID)
{
  nsAutoCString dummyCString;
  nsAutoString dummyString;
  bool dummyBool;
  return ConvertToMidasInternalCommandInner(inCommandID, dummyString,
                                            outCommandID, dummyCString,
                                            dummyBool, dummyBool, true);
}




NS_IMETHODIMP
nsHTMLDocument::ExecCommand(const nsAString& commandID,
                            bool doShowUI,
                            const nsAString& value,
                            bool* _retval)
{
  ErrorResult rv;
  *_retval = ExecCommand(commandID, doShowUI, value, rv);
  return rv.StealNSResult();
}

bool
nsHTMLDocument::ExecCommand(const nsAString& commandID,
                            bool doShowUI,
                            const nsAString& value,
                            ErrorResult& rv)
{
  
  

  nsAutoCString cmdToDispatch, paramStr;
  bool isBool, boolVal;
  if (!ConvertToMidasInternalCommand(commandID, value,
                                     cmdToDispatch, paramStr,
                                     isBool, boolVal)) {
    return false;
  }

  
  if (!IsEditingOnAfterFlush()) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  
  if (doShowUI) {
    return false;
  }

  if (commandID.LowerCaseEqualsLiteral("gethtml")) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  bool restricted = commandID.LowerCaseEqualsLiteral("cut") ||
                    commandID.LowerCaseEqualsLiteral("copy")||
                    commandID.LowerCaseEqualsLiteral("paste");
  if (restricted && !nsContentUtils::IsCallerChrome()) {
    rv = NS_ERROR_DOM_SECURITY_ERR;
    return false;
  }

  
  nsCOMPtr<nsICommandManager> cmdMgr;
  GetMidasCommandManager(getter_AddRefs(cmdMgr));
  if (!cmdMgr) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  nsIDOMWindow* window = GetWindow();
  if (!window) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  if ((cmdToDispatch.EqualsLiteral("cmd_fontSize") ||
       cmdToDispatch.EqualsLiteral("cmd_insertImageNoUI") ||
       cmdToDispatch.EqualsLiteral("cmd_insertLinkNoUI") ||
       cmdToDispatch.EqualsLiteral("cmd_paragraphState")) &&
      paramStr.IsEmpty()) {
    
    return false;
  }

  
  bool enabled = false;
  cmdMgr->IsCommandEnabled(cmdToDispatch.get(), window, &enabled);
  if (!enabled) {
    return false;
  }

  if (!isBool && paramStr.IsEmpty()) {
    rv = cmdMgr->DoCommand(cmdToDispatch.get(), nullptr, window);
  } else {
    
    nsCOMPtr<nsICommandParams> cmdParams = do_CreateInstance(
                                            NS_COMMAND_PARAMS_CONTRACTID);
    if (!cmdParams) {
      rv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return false;
    }

    if (isBool) {
      rv = cmdParams->SetBooleanValue("state_attribute", boolVal);
    } else if (cmdToDispatch.EqualsLiteral("cmd_fontFace")) {
      rv = cmdParams->SetStringValue("state_attribute", value);
    } else if (cmdToDispatch.EqualsLiteral("cmd_insertHTML") ||
               cmdToDispatch.EqualsLiteral("cmd_insertText")) {
      rv = cmdParams->SetStringValue("state_data", value);
    } else {
      rv = cmdParams->SetCStringValue("state_attribute", paramStr.get());
    }
    if (rv.Failed()) {
      return false;
    }
    rv = cmdMgr->DoCommand(cmdToDispatch.get(), cmdParams, window);
  }

  return !rv.Failed();
}


NS_IMETHODIMP
nsHTMLDocument::QueryCommandEnabled(const nsAString& commandID,
                                    bool* _retval)
{
  ErrorResult rv;
  *_retval = QueryCommandEnabled(commandID, rv);
  return rv.StealNSResult();
}

bool
nsHTMLDocument::QueryCommandEnabled(const nsAString& commandID, ErrorResult& rv)
{
  nsAutoCString cmdToDispatch;
  if (!ConvertToMidasInternalCommand(commandID, cmdToDispatch)) {
    return false;
  }

  
  if (!IsEditingOnAfterFlush()) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  
  nsCOMPtr<nsICommandManager> cmdMgr;
  GetMidasCommandManager(getter_AddRefs(cmdMgr));
  if (!cmdMgr) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  nsIDOMWindow* window = GetWindow();
  if (!window) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  bool retval;
  rv = cmdMgr->IsCommandEnabled(cmdToDispatch.get(), window, &retval);
  return retval;
}


NS_IMETHODIMP
nsHTMLDocument::QueryCommandIndeterm(const nsAString & commandID,
                                     bool *_retval)
{
  ErrorResult rv;
  *_retval = QueryCommandIndeterm(commandID, rv);
  return rv.StealNSResult();
}

bool
nsHTMLDocument::QueryCommandIndeterm(const nsAString& commandID, ErrorResult& rv)
{
  nsAutoCString cmdToDispatch;
  if (!ConvertToMidasInternalCommand(commandID, cmdToDispatch)) {
    return false;
  }

  
  if (!IsEditingOnAfterFlush()) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  
  nsCOMPtr<nsICommandManager> cmdMgr;
  GetMidasCommandManager(getter_AddRefs(cmdMgr));
  if (!cmdMgr) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  nsIDOMWindow* window = GetWindow();
  if (!window) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  nsresult res;
  nsCOMPtr<nsICommandParams> cmdParams = do_CreateInstance(
                                           NS_COMMAND_PARAMS_CONTRACTID, &res);
  if (NS_FAILED(res)) {
    rv.Throw(res);
    return false;
  }

  rv = cmdMgr->GetCommandState(cmdToDispatch.get(), window, cmdParams);
  if (rv.Failed()) {
    return false;
  }

  
  
  
  bool retval = false;
  cmdParams->GetBooleanValue("state_mixed", &retval);
  return retval;
}


NS_IMETHODIMP
nsHTMLDocument::QueryCommandState(const nsAString & commandID, bool *_retval)
{
  ErrorResult rv;
  *_retval = QueryCommandState(commandID, rv);
  return rv.StealNSResult();
}

bool
nsHTMLDocument::QueryCommandState(const nsAString& commandID, ErrorResult& rv)
{
  nsAutoCString cmdToDispatch, paramToCheck;
  bool dummy, dummy2;
  if (!ConvertToMidasInternalCommand(commandID, commandID,
                                     cmdToDispatch, paramToCheck,
                                     dummy, dummy2)) {
    return false;
  }

  
  if (!IsEditingOnAfterFlush()) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  
  nsCOMPtr<nsICommandManager> cmdMgr;
  GetMidasCommandManager(getter_AddRefs(cmdMgr));
  if (!cmdMgr) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  nsIDOMWindow* window = GetWindow();
  if (!window) {
    rv.Throw(NS_ERROR_FAILURE);
    return false;
  }

  if (commandID.LowerCaseEqualsLiteral("usecss")) {
    
    
    return false;
  }

  nsCOMPtr<nsICommandParams> cmdParams = do_CreateInstance(
                                           NS_COMMAND_PARAMS_CONTRACTID);
  if (!cmdParams) {
    rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return false;
  }

  rv = cmdMgr->GetCommandState(cmdToDispatch.get(), window, cmdParams);
  if (rv.Failed()) {
    return false;
  }

  
  
  
  
  
  
  if (cmdToDispatch.EqualsLiteral("cmd_align")) {
    char * actualAlignmentType = nullptr;
    rv = cmdParams->GetCStringValue("state_attribute", &actualAlignmentType);
    bool retval = false;
    if (!rv.Failed() && actualAlignmentType && actualAlignmentType[0]) {
      retval = paramToCheck.Equals(actualAlignmentType);
    }
    if (actualAlignmentType) {
      free(actualAlignmentType);
    }
    return retval;
  }

  
  
  
  bool retval = false;
  cmdParams->GetBooleanValue("state_all", &retval);
  return retval;
}


NS_IMETHODIMP
nsHTMLDocument::QueryCommandSupported(const nsAString & commandID,
                                      bool *_retval)
{
  *_retval = QueryCommandSupported(commandID);
  return NS_OK;
}

bool
nsHTMLDocument::QueryCommandSupported(const nsAString& commandID)
{
  
  nsAutoCString cmdToDispatch;
  return ConvertToMidasInternalCommand(commandID, cmdToDispatch);
}


NS_IMETHODIMP
nsHTMLDocument::QueryCommandValue(const nsAString & commandID,
                                  nsAString &_retval)
{
  ErrorResult rv;
  QueryCommandValue(commandID, _retval, rv);
  return rv.StealNSResult();
}

void
nsHTMLDocument::QueryCommandValue(const nsAString& commandID,
                                  nsAString& aValue,
                                  ErrorResult& rv)
{
  aValue.Truncate();

  nsAutoCString cmdToDispatch, paramStr;
  if (!ConvertToMidasInternalCommand(commandID, cmdToDispatch)) {
    
    return;
  }

  
  if (!IsEditingOnAfterFlush()) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  nsCOMPtr<nsICommandManager> cmdMgr;
  GetMidasCommandManager(getter_AddRefs(cmdMgr));
  if (!cmdMgr) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  nsIDOMWindow* window = GetWindow();
  if (!window) {
    rv.Throw(NS_ERROR_FAILURE);
    return;
  }

  
  nsCOMPtr<nsICommandParams> cmdParams = do_CreateInstance(
                                           NS_COMMAND_PARAMS_CONTRACTID);
  if (!cmdParams) {
    rv.Throw(NS_ERROR_OUT_OF_MEMORY);
    return;
  }

  
  
  if (cmdToDispatch.EqualsLiteral("cmd_getContents")) {
    rv = cmdParams->SetBooleanValue("selection_only", true);
    if (rv.Failed()) {
      return;
    }
    rv = cmdParams->SetCStringValue("format", "text/html");
    if (rv.Failed()) {
      return;
    }
    rv = cmdMgr->DoCommand(cmdToDispatch.get(), cmdParams, window);
    if (rv.Failed()) {
      return;
    }
    rv = cmdParams->GetStringValue("result", aValue);
    return;
  }

  rv = cmdParams->SetCStringValue("state_attribute", paramStr.get());
  if (rv.Failed()) {
    return;
  }

  rv = cmdMgr->GetCommandState(cmdToDispatch.get(), window, cmdParams);
  if (rv.Failed()) {
    return;
  }

  
  
  
  
  nsXPIDLCString cStringResult;
  cmdParams->GetCStringValue("state_attribute",
                             getter_Copies(cStringResult));
  CopyUTF8toUTF16(cStringResult, aValue);
}

nsresult
nsHTMLDocument::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  NS_ASSERTION(aNodeInfo->NodeInfoManager() == mNodeInfoManager,
               "Can't import this document into another document!");

  nsRefPtr<nsHTMLDocument> clone = new nsHTMLDocument();
  nsresult rv = CloneDocHelper(clone.get());
  NS_ENSURE_SUCCESS(rv, rv);

  
  clone->mLoadFlags = mLoadFlags;

  return CallQueryInterface(clone.get(), aResult);
}

bool
nsHTMLDocument::IsEditingOnAfterFlush()
{
  nsIDocument* doc = GetParentDocument();
  if (doc) {
    
    
    doc->FlushPendingNotifications(Flush_Frames);
  }

  return IsEditingOn();
}

void
nsHTMLDocument::RemovedFromDocShell()
{
  mEditingState = eOff;
  nsDocument::RemovedFromDocShell();
}

 void
nsHTMLDocument::DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const
{
  nsDocument::DocAddSizeOfExcludingThis(aWindowSizes);

  
  
  
  
  
  
  
  
  
  
  
  
}

bool
nsHTMLDocument::WillIgnoreCharsetOverride()
{
  if (mType != eHTML) {
    MOZ_ASSERT(mType == eXHTML);
    return true;
  }
  if (mCharacterSetSource == kCharsetFromByteOrderMark) {
    return true;
  }
  if (!EncodingUtils::IsAsciiCompatible(mCharacterSet)) {
    return true;
  }
  nsCOMPtr<nsIWyciwygChannel> wyciwyg = do_QueryInterface(mChannel);
  if (wyciwyg) {
    return true;
  }
  nsIURI* uri = GetOriginalURI();
  if (uri) {
    bool schemeIs = false;
    uri->SchemeIs("about", &schemeIs);
    if (schemeIs) {
      return true;
    }
    bool isResource;
    nsresult rv = NS_URIChainHasFlags(uri,
                                      nsIProtocolHandler::URI_IS_UI_RESOURCE,
                                      &isResource);
    if (NS_FAILED(rv) || isResource) {
      return true;
    }
  }
  return false;
}
