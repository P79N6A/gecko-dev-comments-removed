






































#include "nsXMLDocument.h"
#include "nsParserCIID.h"
#include "nsIParser.h"
#include "nsIXMLContentSink.h"
#include "nsPresContext.h" 
#include "nsIContent.h"
#include "nsIContentViewerContainer.h"
#include "nsIContentViewer.h"
#include "nsIDocShell.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsHTMLParts.h"
#include "nsIComponentManager.h"
#include "nsIDOMComment.h"
#include "nsIDOMElement.h"
#include "nsIDOMText.h"
#include "nsIBaseWindow.h"
#include "nsIDOMWindow.h"
#include "nsIDOMDocumentType.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIHttpChannel.h"
#include "nsIURI.h"
#include "nsIServiceManager.h"
#include "nsICharsetAlias.h"
#include "nsICharsetAlias.h"
#include "nsNetUtil.h"
#include "nsDOMError.h"
#include "nsIScriptSecurityManager.h"
#include "nsIPrincipal.h"
#include "nsLayoutCID.h"
#include "nsDOMAttribute.h"
#include "nsGUIEvent.h"
#include "nsCExternalHandlerService.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsEventListenerManager.h"
#include "nsContentUtils.h"
#include "nsThreadUtils.h"
#include "nsJSUtils.h"
#include "nsCRT.h"
#include "nsIAuthPrompt.h"
#include "nsIScriptGlobalObjectOwner.h"
#include "nsIJSContextStack.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentPolicyUtils.h"
#include "nsContentErrors.h"
#include "nsIDOMUserDataHandler.h"
#include "nsEventDispatcher.h"
#include "nsNodeUtils.h"
#include "nsIConsoleService.h"
#include "nsIScriptError.h"
#include "nsIHTMLDocument.h"
#include "nsGenericElement.h"






nsresult
NS_NewDOMDocument(nsIDOMDocument** aInstancePtrResult,
                  const nsAString& aNamespaceURI, 
                  const nsAString& aQualifiedName, 
                  nsIDOMDocumentType* aDoctype,
                  nsIURI* aDocumentURI,
                  nsIURI* aBaseURI,
                  nsIPrincipal* aPrincipal,
                  PRBool aLoadedAsData)
{
  
  
  
  
  nsresult rv;

  *aInstancePtrResult = nsnull;

  nsCOMPtr<nsIDocument> d;
  PRBool isHTML = PR_FALSE;
  PRBool isXHTML = PR_FALSE;
  if (aDoctype) {
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
      isHTML = PR_TRUE;
    } else if (publicId.EqualsLiteral("-//W3C//DTD XHTML 1.0 Strict//EN") ||
               publicId.EqualsLiteral("-//W3C//DTD XHTML 1.0 Transitional//EN") ||
               publicId.EqualsLiteral("-//W3C//DTD XHTML 1.0 Frameset//EN")) {
      rv = NS_NewHTMLDocument(getter_AddRefs(d));
      isHTML = PR_TRUE;
      isXHTML = PR_TRUE;
    }
    else if (publicId.EqualsLiteral("-//W3C//DTD SVG 1.1//EN")) {
      rv = NS_NewSVGDocument(getter_AddRefs(d));
    }
    
    else {
      rv = NS_NewXMLDocument(getter_AddRefs(d));
    }
  } else {
    rv = NS_NewXMLDocument(getter_AddRefs(d));
  }

  if (NS_FAILED(rv)) {
    return rv;
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
NS_NewXMLDocument(nsIDocument** aInstancePtrResult)
{
  nsXMLDocument* doc = new nsXMLDocument();
  NS_ENSURE_TRUE(doc, NS_ERROR_OUT_OF_MEMORY);

  NS_ADDREF(doc);
  nsresult rv = doc->Init();

  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
  }

  *aInstancePtrResult = doc;

  return rv;
}

  
  

nsXMLDocument::nsXMLDocument(const char* aContentType)
  : nsDocument(aContentType),
    mAsync(PR_TRUE)
{

  
  
}

nsXMLDocument::~nsXMLDocument()
{
  
  mLoopingForSyncLoad = PR_FALSE;
}

DOMCI_NODE_DATA(XMLDocument, nsXMLDocument)


NS_INTERFACE_TABLE_HEAD(nsXMLDocument)
  NS_DOCUMENT_INTERFACE_TABLE_BEGIN(nsXMLDocument)
    NS_INTERFACE_TABLE_ENTRY(nsXMLDocument, nsIDOMXMLDocument)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(XMLDocument)
NS_INTERFACE_MAP_END_INHERITING(nsDocument)


NS_IMPL_ADDREF_INHERITED(nsXMLDocument, nsDocument)
NS_IMPL_RELEASE_INHERITED(nsXMLDocument, nsDocument)


nsresult
nsXMLDocument::Init()
{
  nsresult rv = nsDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  return rv;
}

void
nsXMLDocument::Reset(nsIChannel* aChannel, nsILoadGroup* aLoadGroup)
{
  nsDocument::Reset(aChannel, aLoadGroup);
}

void
nsXMLDocument::ResetToURI(nsIURI *aURI, nsILoadGroup *aLoadGroup,
                          nsIPrincipal* aPrincipal)
{
  if (mChannelIsPending) {
    StopDocumentLoad();
    mChannel->Cancel(NS_BINDING_ABORTED);
    mChannelIsPending = nsnull;
  }

  nsDocument::ResetToURI(aURI, aLoadGroup, aPrincipal);
}

NS_IMETHODIMP
nsXMLDocument::GetAsync(PRBool *aAsync)
{
  NS_ENSURE_ARG_POINTER(aAsync);
  *aAsync = mAsync;
  return NS_OK;
}

NS_IMETHODIMP
nsXMLDocument::SetAsync(PRBool aAsync)
{
  mAsync = aAsync;
  return NS_OK;
}

static void
ReportUseOfDeprecatedMethod(nsIDocument *aDoc, const char* aWarning)
{
  nsContentUtils::ReportToConsole(nsContentUtils::eDOM_PROPERTIES,
                                  aWarning,
                                  nsnull, 0,
                                  nsnull,
                                  EmptyString(), 0, 0,
                                  nsIScriptError::warningFlag,
                                  "DOM3 Load", aDoc);
}

NS_IMETHODIMP
nsXMLDocument::Load(const nsAString& aUrl, PRBool *aReturn)
{
  PRBool hasHadScriptObject = PR_TRUE;
  nsIScriptGlobalObject* scriptObject =
    GetScriptHandlingObject(hasHadScriptObject);
  NS_ENSURE_STATE(scriptObject || !hasHadScriptObject);

  ReportUseOfDeprecatedMethod(this, "UseOfDOM3LoadMethodWarning");

  NS_ENSURE_ARG_POINTER(aReturn);
  *aReturn = PR_FALSE;

  nsCOMPtr<nsIDocument> callingDoc =
    do_QueryInterface(nsContentUtils::GetDocumentFromContext());

  nsIURI *baseURI = mDocumentURI;
  nsCAutoString charset;

  if (callingDoc) {
    baseURI = callingDoc->GetDocBaseURI();
    charset = callingDoc->GetDocumentCharacterSet();
  }

  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUrl, charset.get(), baseURI);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  

  
  
  
  nsCOMPtr<nsIPrincipal> principal = NodePrincipal();
  if (!nsContentUtils::IsSystemPrincipal(principal)) {
    rv = principal->CheckMayLoad(uri, PR_FALSE);
    NS_ENSURE_SUCCESS(rv, rv);

    PRInt16 shouldLoad = nsIContentPolicy::ACCEPT;
    rv = NS_CheckContentLoadPolicy(nsIContentPolicy::TYPE_XMLHTTPREQUEST,
                                   uri,
                                   principal,
                                   callingDoc ? callingDoc.get() :
                                     static_cast<nsIDocument*>(this),
                                   NS_LITERAL_CSTRING("application/xml"),
                                   nsnull,
                                   &shouldLoad,
                                   nsContentUtils::GetContentPolicy(),
                                   nsContentUtils::GetSecurityManager());
    NS_ENSURE_SUCCESS(rv, rv);
    if (NS_CP_REJECTED(shouldLoad)) {
      return NS_ERROR_CONTENT_BLOCKED;
    }
  } else {
    
    

    PRBool isChrome = PR_FALSE;
    if (NS_FAILED(uri->SchemeIs("chrome", &isChrome)) || !isChrome) {
      nsCAutoString spec;
      if (mDocumentURI)
        mDocumentURI->GetSpec(spec);

      nsAutoString error;
      error.AssignLiteral("Cross site loading using document.load is no "
                          "longer supported. Use XMLHttpRequest instead.");
      nsCOMPtr<nsIScriptError2> errorObject =
          do_CreateInstance(NS_SCRIPTERROR_CONTRACTID, &rv);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = errorObject->InitWithWindowID(error.get(), NS_ConvertUTF8toUTF16(spec).get(),
                                         nsnull, 0, 0, nsIScriptError::warningFlag,
                                         "DOM",
                                         callingDoc ?
                                           callingDoc->OuterWindowID() :
                                           this->OuterWindowID());

      NS_ENSURE_SUCCESS(rv, rv);

      nsCOMPtr<nsIConsoleService> consoleService =
        do_GetService(NS_CONSOLESERVICE_CONTRACTID);
      nsCOMPtr<nsIScriptError> logError = do_QueryInterface(errorObject);
      if (consoleService && logError) {
        consoleService->LogMessage(logError);
      }

      return NS_ERROR_DOM_SECURITY_ERR;
    }
  }

  
  
  
  
  
  
  
  nsRefPtr<nsEventListenerManager> elm(mListenerManager);
  mListenerManager = nsnull;

  
  
  

  nsCOMPtr<nsILoadGroup> loadGroup;
  if (callingDoc) {
    loadGroup = callingDoc->GetDocumentLoadGroup();
  }

  ResetToURI(uri, loadGroup, principal);

  mListenerManager = elm;

  
  nsCOMPtr<nsIInterfaceRequestor> req = nsContentUtils::GetSameOriginChecker();
  NS_ENSURE_TRUE(req, NS_ERROR_OUT_OF_MEMORY);  

  nsCOMPtr<nsIChannel> channel;
  
  
  rv = NS_NewChannel(getter_AddRefs(channel), uri, nsnull, loadGroup, req, 
                     nsIRequest::LOAD_BACKGROUND);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsCOMPtr<nsIStreamListener> listener;
  if (NS_FAILED(rv = StartDocumentLoad(kLoadAsData, channel, 
                                       loadGroup, nsnull, 
                                       getter_AddRefs(listener),
                                       PR_FALSE))) {
    NS_ERROR("nsXMLDocument::Load: Failed to start the document load.");
    return rv;
  }

  
  

  
  rv = channel->AsyncOpen(listener, nsnull);
  if (NS_FAILED(rv)) {
    mChannelIsPending = PR_FALSE;
    return rv;
  }

  if (!mAsync) {
    nsCOMPtr<nsIThread> thread = do_GetCurrentThread();

    mLoopingForSyncLoad = PR_TRUE;
    while (mLoopingForSyncLoad) {
      if (!NS_ProcessNextEvent(thread))
        break;
    }

    
    nsCOMPtr<nsIDOMNode> node = do_QueryInterface(GetRootElement());
    if (node) {
      nsAutoString name, ns;      
      if (NS_SUCCEEDED(node->GetLocalName(name)) &&
          name.EqualsLiteral("parsererror") &&
          NS_SUCCEEDED(node->GetNamespaceURI(ns)) &&
          ns.EqualsLiteral("http://www.mozilla.org/newlayout/xml/parsererror.xml")) {
        
      } else {
        *aReturn = PR_TRUE;
      }
    }
  } else {
    *aReturn = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsXMLDocument::StartDocumentLoad(const char* aCommand,
                                 nsIChannel* aChannel,
                                 nsILoadGroup* aLoadGroup,
                                 nsISupports* aContainer,
                                 nsIStreamListener **aDocListener,
                                 PRBool aReset,
                                 nsIContentSink* aSink)
{
  nsresult rv = nsDocument::StartDocumentLoad(aCommand,
                                              aChannel, aLoadGroup,
                                              aContainer, 
                                              aDocListener, aReset, aSink);
  if (NS_FAILED(rv)) return rv;

  if (nsCRT::strcmp("loadAsInteractiveData", aCommand) == 0) {
    mLoadedAsInteractiveData = PR_TRUE;
    aCommand = kLoadAsData; 
  }


  PRInt32 charsetSource = kCharsetFromDocTypeDefault;
  nsCAutoString charset(NS_LITERAL_CSTRING("UTF-8"));
  TryChannelCharset(aChannel, charsetSource, charset);

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
  mChannelIsPending = PR_TRUE;
  
  SetDocumentCharacterSet(charset);
  mParser->SetDocumentCharset(charset, charsetSource);
  mParser->SetCommand(aCommand);
  mParser->SetContentSink(sink);
  mParser->Parse(aUrl, nsnull, (void *)this);

  return NS_OK;
}

void
nsXMLDocument::EndLoad()
{
  mChannelIsPending = PR_FALSE;
  mLoopingForSyncLoad = PR_FALSE;

  mSynchronousDOMContentLoaded = (mLoadedAsData || mLoadedAsInteractiveData);
  nsDocument::EndLoad();
  if (mSynchronousDOMContentLoaded) {
    mSynchronousDOMContentLoaded = PR_FALSE;
    nsDocument::SetReadyStateInternal(nsIDocument::READYSTATE_COMPLETE);
    
    
    
    nsEvent event(PR_TRUE, NS_LOAD);
    nsEventDispatcher::Dispatch(static_cast<nsIDocument*>(this), nsnull,
                                &event);
  }    
}
 


nsresult
nsXMLDocument::Clone(nsINodeInfo *aNodeInfo, nsINode **aResult) const
{
  NS_ASSERTION(aNodeInfo->NodeInfoManager() == mNodeInfoManager,
               "Can't import this document into another document!");

  nsRefPtr<nsXMLDocument> clone = new nsXMLDocument();
  NS_ENSURE_TRUE(clone, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = CloneDocHelper(clone);
  NS_ENSURE_SUCCESS(rv, rv);

  
  clone->mAsync = mAsync;

  return CallQueryInterface(clone.get(), aResult);
}
