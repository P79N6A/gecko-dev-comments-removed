





#include "mozilla/dom/XMLDocument.h"
#include "nsParserCIID.h"
#include "nsCharsetSource.h"
#include "nsIXMLContentSink.h"
#include "nsPresContext.h" 
#include "nsIContent.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIDocShell.h"
#include "nsHTMLParts.h"
#include "nsIComponentManager.h"
#include "nsIDOMElement.h"
#include "nsIBaseWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocumentType.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIHttpChannel.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
#include "nsNetUtil.h"
#include "nsError.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsLayoutCID.h"
#include "mozilla/dom/Attr.h"
#include "nsCExternalHandlerService.h"
#include "nsMimeTypes.h"
#include "mozilla/EventListenerManager.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "nsJSUtils.h"
#include "nsCRT.h"
#include "nsIAuthPrompt.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentPolicyUtils.h"
#include "nsNodeUtils.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIHTMLDocument.h"
#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/XMLDocumentBinding.h"
#include "mozilla/dom/DocumentBinding.h"

using namespace mozilla;
using namespace mozilla::dom;






nsresult
NS_NewDOMDocument(nsIDOMDocument** aInstancePtrResult,
                  const nsAString& aNamespaceURI, 
                  const nsAString& aQualifiedName, 
                  nsIDOMDocumentType* aDoctype,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal,
                  bool aLoadedAsData,
                  nsIGlobalObject* aEventObject,
                  DocumentFlavor aFlavor)
{
  
  
  
  
  nsresult rv;

  *aInstancePtrResult = nullptr;

  nsCOMPtr<nsIDocument> d;
  bool isHTML = false;
  bool isXHTML = false;
  if (aFlavor == DocumentFlavorSVG) {
    rv = NS_NewSVGDocument(getter_AddRefs(d));
  } else if (aFlavor == DocumentFlavorHTML) {
    rv = NS_NewHTMLDocument(getter_AddRefs(d));
    isHTML = true;
  } else if (aFlavor == DocumentFlavorPlain) {
    rv = NS_NewXMLDocument(getter_AddRefs(d), aLoadedAsData, true);
  } else if (aDoctype) {
    MOZ_ASSERT(aFlavor == DocumentFlavorLegacyGuess);
    nsAutoString publicId, name;
    aDoctype->GetPublicId(publicId);
    if (publicId.IsEmpty()) {
      aDoctype->GetName(name);
    }
    if (name.EqualsLiteral("html") ||
        publicId.EqualsLiteral("-//W3C//DTD HTML 4.01//EN") ||
        publicId.EqualsLiteral("-//W3C//DTD HTML 4.01 Frameset//EN") ||
        publicId.EqualsLiteral("-//W3C//DTD HTML 4.01 Transitional//EN") ||
        publicId.EqualsLiteral("-//W3C//DTD HTML 4.0//EN") ||
        publicId.EqualsLiteral("-//W3C//DTD HTML 4.0 Frameset//EN") ||
        publicId.EqualsLiteral("-//W3C//DTD HTML 4.0 Transitional//EN")) {
      rv = NS_NewHTMLDocument(getter_AddRefs(d));
      isHTML = true;
    } else if (publicId.EqualsLiteral("-//W3C//DTD XHTML 1.0 Strict//EN") ||
               publicId.EqualsLiteral("-//W3C//DTD XHTML 1.0 Transitional//EN") ||
               publicId.EqualsLiteral("-//W3C//DTD XHTML 1.0 Frameset//EN")) {
      rv = NS_NewHTMLDocument(getter_AddRefs(d));
      isHTML = true;
      isXHTML = true;
    }
    else if (publicId.EqualsLiteral("-//W3C//DTD SVG 1.1//EN")) {
      rv = NS_NewSVGDocument(getter_AddRefs(d));
    }
    
    else {
      rv = NS_NewXMLDocument(getter_AddRefs(d));
    }
  } else {
    MOZ_ASSERT(aFlavor == DocumentFlavorLegacyGuess);
    rv = NS_NewXMLDocument(getter_AddRefs(d));
  }

  if (NS_FAILED(rv)) {
    return rv;
  }

  if (nsCOMPtr<nsIScriptGlobalObject> sgo = do_QueryInterface(aEventObject)) {
    d->SetScriptHandlingObject(sgo);
  } else if (aEventObject){
    d->SetScopeObject(aEventObject);
  }

  if (isHTML) {
    nsCOMPtr<nsIHTMLDocument> htmlDoc = do_QueryInterface(d);
    NS_ASSERTION(htmlDoc, "HTML Document doesn't implement nsIHTMLDocument?");
    htmlDoc->SetCompatibilityMode(eCompatibility_FullStandards);
    htmlDoc->SetIsXHTML(isXHTML);
  }
  nsDocument* doc = static_cast<nsDocument*>(d.get());
  doc->SetLoadedAsData(aLoadedAsData);
  doc->nsDocument::SetDocumentURI(aDocumentURI);
  
  doc->SetPrincipal(aPrincipal);
  doc->SetBaseURI(aBaseURI);

  
  
  doc->SetDocumentCharacterSet(NS_LITERAL_CSTRING("UTF-8"));
  
  if (aDoctype) {
    nsCOMPtr<nsIDOMNode> tmpNode;
    rv = doc->AppendChild(aDoctype, getter_AddRefs(tmpNode));
    NS_ENSURE_SUCCESS(rv, rv);
  }
  
  if (!aQualifiedName.IsEmpty()) {
    nsCOMPtr<nsIDOMElement> root;
    rv = doc->CreateElementNS(aNamespaceURI, aQualifiedName,
                              getter_AddRefs(root));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsIDOMNode> tmpNode;

    rv = doc->AppendChild(root, getter_AddRefs(tmpNode));
    NS_ENSURE_SUCCESS(rv, rv);
  }

  *aInstancePtrResult = doc;
  NS_ADDREF(*aInstancePtrResult);

  return NS_OK;
}

nsresult
NS_NewXMLDocument(nsIDocument** aInstancePtrResult, bool aLoadedAsData,
                  bool aIsPlainDocument)
{
  nsRefPtr<XMLDocument> doc = new XMLDocument();

  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    *aInstancePtrResult = nullptr;
    return rv;
  }

  doc->SetLoadedAsData(aLoadedAsData);
  doc->mIsPlainDocument = aIsPlainDocument;
  doc.forget(aInstancePtrResult);

  return NS_OK;
}

nsresult
NS_NewXBLDocument(nsIDOMDocument** aInstancePtrResult,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal)
{
  nsresult rv = NS_NewDOMDocument(aInstancePtrResult,
                                  NS_LITERAL_STRING("http://www.mozilla.org/xbl"),
                                  NS_LITERAL_STRING("bindings"), nullptr,
                                  aDocumentURI, aBaseURI, aPrincipal, false,
                                  nullptr, DocumentFlavorLegacyGuess);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIDocument> idoc = do_QueryInterface(*aInstancePtrResult);
  nsDocument* doc = static_cast<nsDocument*>(idoc.get());
  doc->SetLoadedAsInteractiveData(true);
  doc->SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);

  return NS_OK;
}

namespace mozilla {
namespace dom {

XMLDocument::XMLDocument(const char* aContentType)
  : nsDocument(aContentType),
    mAsync(true)
{
  
  

  mType = eGenericXML;
}

XMLDocument::~XMLDocument()
{
  
  mLoopingForSyncLoad = false;
}


NS_IMPL_ISUPPORTS_INHERITED(XMLDocument, nsDocument, nsIDOMXMLDocument)

nsresult
XMLDocument::Init()
{
  nsresult rv = nsDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

void
XMLDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsDocument::Reset(aChannel, aLoadGroup);
}

void
XMLDocument::ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                        nsIPrincipal* aPrincipal)
{
  if (mChannelIsPending) {
    StopDocumentLoad();
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannelIsPending = false;
  }

  nsDocument::ResetToURI(aURI, aLoadGroup, aPrincipal);
}

NS_IMETHODIMP
XMLDocument::GetAsync(bool *aAsync)
{
  NS_ENSURE_ARG_POINTER(aAsync);
  *aAsync = mAsync;
  return NS_OK;
}

NS_IMETHODIMP
XMLDocument::SetAsync(bool aAsync)
{
  mAsync = aAsync;
  return NS_OK;
}

NS_IMETHODIMP
XMLDocument::Load(const nsAString& aUrl, bool *aReturn)
{
  ErrorResult rv;
  *aReturn = Load(aUrl, rv);
  return rv.StealNSResult();
}

bool
XMLDocument::Load(const nsAString& aUrl, ErrorResult& aRv)
{
  bool hasHadScriptObject = true;
  nsIScriptGlobalObject* scriptObject =
    GetScriptHandlingObject(hasHadScriptObject);
  if (!scriptObject && hasHadScriptObject) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return false;
  }

  WarnOnceAbout(nsIDocument::eUseOfDOM3LoadMethod);

  nsCOMPtr<nsIDocument> callingDoc = GetEntryDocument();
  nsCOMPtr<nsIPrincipal> principal = NodePrincipal();

  
  if (callingDoc && (callingDoc->NodePrincipal() != principal)) {
    nsContentUtils::ReportToConsole(nsIScriptError::errorFlag,
                                    NS_LITERAL_CSTRING("DOM"),
                                    callingDoc,
                                    nsContentUtils::eDOM_PROPERTIES,
                                    "XMLDocumentLoadPrincipalMismatch");
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return false;
  }

  nsIURI *baseURI = mDocumentURI;
  nsAutoCString charset;

  if (callingDoc) {
    baseURI = callingDoc->GetDocBaseURI();
    charset = callingDoc->GetDocumentCharacterSet();
  }

  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUrl, charset.get(), baseURI);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  
  
  
  

  
  
  
  if (!nsContentUtils::IsSystemPrincipal(principal)) {
    rv = principal->CheckMayLoad(uri, false, false);
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return false;
    }

    int16_t shouldLoad = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_XMLHTTPREQUEST,
                                   uri,
                                   principal,
                                   callingDoc ? callingDoc.get() :
                                     static_cast<nsIDocument*>(this),
                                   NS_LITERAL_CSTRING("application/xml"),
                                   nullptr,
                                   &shouldLoad,
                                   nsContentUtils::GetContentPolicy(),
                                   nsContentUtils::GetSecurityManager());
    if (NS_FAILED(rv)) {
      aRv.Throw(rv);
      return false;
    }
    if (NS_CP_REJECTED(shouldLoad)) {
      aRv.Throw(NS_ERROR_CONTENT_BLOCKED);
      return false;
    }
  } else {
    
    

    bool isChrome = false;
    if (NS_FAILED(uri->SchemeIs("chrome", &isChrome)) || !isChrome) {
      nsAutoCString spec;
      if (mDocumentURI)
        mDocumentURI->GetSpec(spec);

      nsAutoString error;
      error.AssignLiteral("Cross site loading using document.load is no "
                          "longer supported. Use XMLHttpRequest instead.");
      nsCOMPtr<nsIScriptError> errorObject =
          do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return false;
      }

      rv = errorObject->InitWithWindowID(error,
                                         NS_ConvertUTF8toUTF16(spec),
                                         EmptyString(),
                                         0, 0, nsIScriptError::warningFlag,
                                         "DOM",
                                         callingDoc ?
                                           callingDoc->InnerWindowID() :
                                           this->InnerWindowID());

      if (NS_FAILED(rv)) {
        aRv.Throw(rv);
        return false;
      }

      nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      if (consoleService) {
        consoleService->LogMessage(errorObject);
      }

      aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
      return false;
    }
  }

  
  
  
  
  
  
  
  nsRefPtr<EventListenerManager> elm(mListenerManager);
  mListenerManager = nullptr;

  
  
  

  nsCOMPtr<nsILoadGroup> loadGroup;
  if (callingDoc) {
    loadGroup = callingDoc->GetDocumentLoadGroup();
  }

  ResetToURI(uri, loadGroup, principal);

  mListenerManager = elm;

  
  nsCOMPtr<nsIInterfaceRequestor> req = nsContentUtils::SameOriginChecker();

  nsCOMPtr<nsIChannel> channel;
  
  
  rv = NS_NewChannel(getter_AddRefs(channel),
                     uri,
                     callingDoc ? callingDoc.get() :
                                  static_cast<nsIDocument*>(this),
                     nsILoadInfo::SEC_NORMAL,
                     nsIContentPolicy::TYPE_XMLHTTPREQUEST,
                     loadGroup,
                     req,
                     nsIRequest::LOAD_BACKGROUND);

  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return false;
  }

  
  
  
  
  MOZ_ASSERT(GetReadyStateEnum() == nsIDocument::READYSTATE_COMPLETE,
             "Bad readyState");
  SetReadyStateInternal(nsIDocument::READYSTATE_UNINITIALIZED);

  
  nsCOMPtr<nsIStreamListener> listener;
  if (NS_FAILED(rv = StartDocumentLoad(kLoadAsData, channel, 
                                       loadGroup, nullptr, 
                                       getter_AddRefs(listener),
                                       false))) {
    NS_ERROR("XMLDocument::Load: Failed to start the document load.");
    aRv.Throw(rv);
    return false;
  }

  
  

  
  rv = channel->AsyncOpen(listener, nullptr);
  if (NS_FAILED(rv)) {
    mChannelIsPending = false;
    aRv.Throw(rv);
    return false;
  }

  if (!mAsync) {
    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

    nsAutoSyncOperation sync(this);
    mLoopingForSyncLoad = true;
    while (mLoopingForSyncLoad) {
      if (!NS_ProcessNextEvent(thread))
        break;
    }

    
    Element* rootElement = GetRootElement();
    if (!rootElement) {
      return false;
    }

    if (rootElement->LocalName().EqualsLiteral("parsererror")) {
      nsAutoString ns;
      rootElement->GetNamespaceURI(ns);
      if (ns.EqualsLiteral("http://www.mozilla.org/newlayout/xml/parsererror.xml")) {
        return false;
      }
    }
  }

  return true;
}

nsresult
XMLDocument::StartDocumentLoad(const char* aCommand,
                               nsIChannel* aChannel,
                               nsILoadGroup* aLoadGroup,
                               nsISupports* aContainer,
                               nsIStreamListener **aDocListener,
                               bool aReset,
                               nsIContentSink* aSink)
{
  nsresult rv = nsDocument::StartDocumentLoad(aCommand,
                                              aChannel, aLoadGroup,
                                              aContainer, 
                                              aDocListener, aReset, aSink);
  if (NS_FAILED(rv)) return rv;

  if (nsCRT::strcmp("loadAsInteractiveData", aCommand) == 0) {
    mLoadedAsInteractiveData = true;
    aCommand = kLoadAsData; 
  }


  int32_t charsetSource = kCharsetFromDocTypeDefault;
  nsAutoCString charset(NS_LITERAL_CSTRING("UTF-8"));
  TryChannelCharset(aChannel, charsetSource, charset, nullptr);

  nsCOMPtr<nsIURI> aUrl;
  rv = aChannel->GetURI(getter_AddRefs(aUrl));
  if (NS_FAILED(rv)) return rv;

  static NS_DEFINE_CID(kCParserCID, NS_PARSER_CID);

  mParser = do_CreateInstance(kCParserCID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIXMLContentSink> sink;
    
  if (aSink) {
    sink = do_QueryInterface(aSink);
  }
  else {
    nsCOMPtr<nsIDocShell> docShell;
    if (aContainer) {
      docShell = do_QueryInterface(aContainer);
      NS_ENSURE_TRUE(docShell, NS_ERROR_FAILURE);
    }
    rv = NS_NewXMLContentSink(getter_AddRefs(sink), this, aUrl, docShell,
                              aChannel);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = CallQueryInterface(mParser, aDocListener);
  NS_ENSURE_SUCCESS(rv, rv);

  NS_ASSERTION(mChannel, "How can we not have a channel here?");
  mChannelIsPending = true;
  
  SetDocumentCharacterSet(charset);
  mParser->SetDocumentCharset(charset, charsetSource);
  mParser->SetCommand(aCommand);
  mParser->SetContentSink(sink);
  mParser->Parse(aUrl, nullptr, (void *)this);

  return NS_OK;
}

void
XMLDocument::EndLoad()
{
  mChannelIsPending = false;
  mLoopingForSyncLoad = false;

  mSynchronousDOMContentLoaded = (mLoadedAsData || mLoadedAsInteractiveData);
  nsDocument::EndLoad();
  if (mSynchronousDOMContentLoaded) {
    mSynchronousDOMContentLoaded = false;
    nsDocument::SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);
    
    
    
    WidgetEvent event(true, NS_LOAD);
    EventDispatcher::Dispatch(static_cast<nsIDocument*>(this), nullptr, &event);
  }
}

 void
XMLDocument::DocAddSizeOfExcludingThis(nsWindowSizes* aWindowSizes) const
{
  nsDocument::DocAddSizeOfExcludingThis(aWindowSizes);
}



nsresult
XMLDocument::Clone(mozilla::dom::NodeInfo *aNodeInfo, nsINode **aResult) const
{
  NS_ASSERTION(aNodeInfo->NodeInfoManager() == mNodeInfoManager,
               "Can't import this document into another document!");

  nsRefPtr<XMLDocument> clone = new XMLDocument();
  nsresult rv = CloneDocHelper(clone);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clone->mAsync = mAsync;
  clone->mIsPlainDocument = mIsPlainDocument;

  return CallQueryInterface(clone.get(), aResult);
}

JSObject*
XMLDocument::WrapNode(JSContext *aCx, JS::Handle<JSObject*> aGivenProto)
{
  if (mIsPlainDocument) {
    return DocumentBinding::Wrap(aCx, this, aGivenProto);
  }

  return XMLDocumentBinding::Wrap(aCx, this, aGivenProto);
}

} 
} 
