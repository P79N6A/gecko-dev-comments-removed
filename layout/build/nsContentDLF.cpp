




































#include "nsCOMPtr.h"
#include "nsContentDLF.h"
#include "nsGenericHTMLElement.h"
#include "nsGkAtoms.h"
#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsICategoryManager.h"
#include "nsIDocumentLoaderFactory.h"
#include "nsIDocument.h"
#include "nsIDocumentViewer.h"
#include "nsIURL.h"
#include "nsNodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsIScriptSecurityManager.h"
#include "nsString.h"
#include "nsContentCID.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsCRT.h"
#include "nsIViewSourceChannel.h"
#ifdef MOZ_MEDIA
#include "nsHTMLMediaElement.h"
#endif

#include "imgILoader.h"
#include "nsIParser.h"
#include "nsMimeTypes.h"

#include "mozilla/FunctionTimer.h"


#include "nsIPluginHost.h"
#include "nsPluginHost.h"
static NS_DEFINE_CID(kPluginDocumentCID, NS_PLUGINDOCUMENT_CID);



#undef NOISY_REGISTRY

static NS_DEFINE_IID(kHTMLDocumentCID, NS_HTMLDOCUMENT_CID);
static NS_DEFINE_IID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);
static NS_DEFINE_IID(kSVGDocumentCID, NS_SVGDOCUMENT_CID);
#ifdef MOZ_MEDIA
static NS_DEFINE_IID(kVideoDocumentCID, NS_VIDEODOCUMENT_CID);
#endif
static NS_DEFINE_IID(kImageDocumentCID, NS_IMAGEDOCUMENT_CID);
static NS_DEFINE_IID(kXULDocumentCID, NS_XULDOCUMENT_CID);

nsresult
NS_NewDocumentViewer(nsIDocumentViewer** aResult);



static const char* const gHTMLTypes[] = {
  TEXT_HTML,
  TEXT_PLAIN,
  TEXT_CSS,
  TEXT_JAVASCRIPT,
  TEXT_ECMASCRIPT,
  APPLICATION_JAVASCRIPT,
  APPLICATION_ECMASCRIPT,
  APPLICATION_XJAVASCRIPT,
  VIEWSOURCE_CONTENT_TYPE,
  APPLICATION_XHTML_XML,
  0
};
  
static const char* const gXMLTypes[] = {
  TEXT_XML,
  APPLICATION_XML,
  APPLICATION_MATHML_XML,
  APPLICATION_RDF_XML,
  TEXT_RDF,
  0
};

static const char* const gSVGTypes[] = {
  IMAGE_SVG_XML,
  0
};

static const char* const gXULTypes[] = {
  TEXT_XUL,
  APPLICATION_CACHED_XUL,
  0
};

nsresult
NS_NewContentDocumentLoaderFactory(nsIDocumentLoaderFactory** aResult)
{
  NS_PRECONDITION(aResult, "null OUT ptr");
  if (!aResult) {
    return NS_ERROR_NULL_POINTER;
  }
  nsContentDLF* it = new nsContentDLF();
  if (!it) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return CallQueryInterface(it, aResult);
}

nsContentDLF::nsContentDLF()
{
}

nsContentDLF::~nsContentDLF()
{
}

NS_IMPL_ISUPPORTS1(nsContentDLF,
                   nsIDocumentLoaderFactory)

PRBool
MayUseXULXBL(nsIChannel* aChannel)
{
  nsIScriptSecurityManager *securityManager =
    nsContentUtils::GetSecurityManager();
  if (!securityManager) {
    return PR_FALSE;
  }

  nsCOMPtr<nsIPrincipal> principal;
  securityManager->GetChannelPrincipal(aChannel, getter_AddRefs(principal));
  NS_ENSURE_TRUE(principal, PR_FALSE);

  return nsContentUtils::AllowXULXBLForPrincipal(principal);
}

NS_IMETHODIMP
nsContentDLF::CreateInstance(const char* aCommand,
                             nsIChannel* aChannel,
                             nsILoadGroup* aLoadGroup,
                             const char* aContentType, 
                             nsISupports* aContainer,
                             nsISupports* aExtraInfo,
                             nsIStreamListener** aDocListener,
                             nsIContentViewer** aDocViewer)
{
#ifdef NS_FUNCTION_TIMER
  nsCAutoString channelURL__("N/A");
  nsCOMPtr<nsIURI> url__;
  if (aChannel && NS_SUCCEEDED(aChannel->GetURI(getter_AddRefs(url__)))) {
    url__->GetSpec(channelURL__);
  }
  NS_TIME_FUNCTION_FMT("%s (line %d) (url: %s)", MOZ_FUNCTION_NAME,
                       __LINE__, channelURL__.get());
#endif

  
  
  
  nsCAutoString type;

  
  nsCOMPtr<nsIViewSourceChannel> viewSourceChannel = do_QueryInterface(aChannel);
  if (viewSourceChannel)
  {
    aCommand = "view-source";

    
    
    
    
    viewSourceChannel->GetOriginalContentType(type);
    PRBool knownType = PR_FALSE;
    PRInt32 typeIndex;
    for (typeIndex = 0; gHTMLTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gHTMLTypes[typeIndex]) &&
          !type.EqualsLiteral(VIEWSOURCE_CONTENT_TYPE)) {
        knownType = PR_TRUE;
      }
    }

    for (typeIndex = 0; gXMLTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gXMLTypes[typeIndex])) {
        knownType = PR_TRUE;
      }
    }

    for (typeIndex = 0; gSVGTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gSVGTypes[typeIndex])) {
        knownType = PR_TRUE;
      }
    }

    for (typeIndex = 0; gXULTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gXULTypes[typeIndex])) {
        knownType = PR_TRUE;
      }
    }

    if (knownType) {
      viewSourceChannel->SetContentType(type);
    } else if (IsImageContentType(type.get())) {
      
      
      aContentType = type.get();
    } else {
      viewSourceChannel->SetContentType(NS_LITERAL_CSTRING(TEXT_PLAIN));
    }
  } else if (0 == PL_strcmp(VIEWSOURCE_CONTENT_TYPE, aContentType)) {
    aChannel->SetContentType(NS_LITERAL_CSTRING(TEXT_PLAIN));
    aContentType = TEXT_PLAIN;
  }
  
  int typeIndex=0;
  while(gHTMLTypes[typeIndex]) {
    if (0 == PL_strcmp(gHTMLTypes[typeIndex++], aContentType)) {
      return CreateDocument(aCommand, 
                            aChannel, aLoadGroup,
                            aContainer, kHTMLDocumentCID,
                            aDocListener, aDocViewer);
    }
  }

  
  typeIndex = 0;
  while(gXMLTypes[typeIndex]) {
    if (0== PL_strcmp(gXMLTypes[typeIndex++], aContentType)) {
      return CreateDocument(aCommand, 
                            aChannel, aLoadGroup,
                            aContainer, kXMLDocumentCID,
                            aDocListener, aDocViewer);
    }
  }

  
  typeIndex = 0;
  while(gSVGTypes[typeIndex]) {
    if (!PL_strcmp(gSVGTypes[typeIndex++], aContentType)) {
      return CreateDocument(aCommand,
                            aChannel, aLoadGroup,
                            aContainer, kSVGDocumentCID,
                            aDocListener, aDocViewer);
    }
  }

  
  typeIndex = 0;
  while (gXULTypes[typeIndex]) {
    if (0 == PL_strcmp(gXULTypes[typeIndex++], aContentType)) {
      if (!MayUseXULXBL(aChannel)) {
        return NS_ERROR_REMOTE_XUL;
      }

      return CreateXULDocument(aCommand,
                               aChannel, aLoadGroup,
                               aContentType, aContainer,
                               aExtraInfo, aDocListener, aDocViewer);
    }
  }

#ifdef MOZ_MEDIA
  if (nsHTMLMediaElement::ShouldHandleMediaType(aContentType)) {
    return CreateDocument(aCommand, 
                          aChannel, aLoadGroup,
                          aContainer, kVideoDocumentCID,
                          aDocListener, aDocViewer);
  }  
#endif

  
  if (IsImageContentType(aContentType)) {
    return CreateDocument(aCommand, 
                          aChannel, aLoadGroup,
                          aContainer, kImageDocumentCID,
                          aDocListener, aDocViewer);
  }

  nsCOMPtr<nsIPluginHost> pluginHostCOM(do_GetService(MOZ_PLUGIN_HOST_CONTRACTID));
  nsPluginHost *pluginHost = static_cast<nsPluginHost*>(pluginHostCOM.get());
  if(pluginHost &&
     NS_SUCCEEDED(pluginHost->IsPluginEnabledForType(aContentType))) {
    return CreateDocument(aCommand,
                          aChannel, aLoadGroup,
                          aContainer, kPluginDocumentCID,
                          aDocListener, aDocViewer);
  }

  
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP
nsContentDLF::CreateInstanceForDocument(nsISupports* aContainer,
                                        nsIDocument* aDocument,
                                        const char *aCommand,
                                        nsIContentViewer** aDocViewerResult)
{
  NS_TIME_FUNCTION;

  nsresult rv = NS_ERROR_FAILURE;  

  do {
    nsCOMPtr<nsIDocumentViewer> docv;
    rv = NS_NewDocumentViewer(getter_AddRefs(docv));
    if (NS_FAILED(rv))
      break;

    
    nsIContentViewer* cv = static_cast<nsIContentViewer*>(docv.get());
    rv = cv->LoadStart(aDocument);
    NS_ADDREF(*aDocViewerResult = cv);
  } while (PR_FALSE);

  return rv;
}

NS_IMETHODIMP
nsContentDLF::CreateBlankDocument(nsILoadGroup *aLoadGroup,
                                  nsIPrincipal* aPrincipal,
                                  nsIDocument **aDocument)
{
  NS_TIME_FUNCTION;

  *aDocument = nsnull;

  nsresult rv = NS_ERROR_FAILURE;

  
  nsCOMPtr<nsIDocument> blankDoc(do_CreateInstance(kHTMLDocumentCID));

  if (blankDoc) {
    
    nsCOMPtr<nsIURI> uri;
    NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING("about:blank"));
    if (uri) {
      blankDoc->ResetToURI(uri, aLoadGroup, aPrincipal);
      rv = NS_OK;
    }
  }

  
  if (NS_SUCCEEDED(rv)) {
    rv = NS_ERROR_FAILURE;

    nsNodeInfoManager *nim = blankDoc->NodeInfoManager();

    nsCOMPtr<nsINodeInfo> htmlNodeInfo;

    
    htmlNodeInfo = nim->GetNodeInfo(nsGkAtoms::html, 0, kNameSpaceID_XHTML,
                                    nsIDOMNode::ELEMENT_NODE);
    nsCOMPtr<nsIContent> htmlElement =
      NS_NewHTMLHtmlElement(htmlNodeInfo.forget());

    
    htmlNodeInfo = nim->GetNodeInfo(nsGkAtoms::head, 0, kNameSpaceID_XHTML,
                                    nsIDOMNode::ELEMENT_NODE);
    nsCOMPtr<nsIContent> headElement =
      NS_NewHTMLHeadElement(htmlNodeInfo.forget());

    
    htmlNodeInfo = nim->GetNodeInfo(nsGkAtoms::body, 0, kNameSpaceID_XHTML,
                                    nsIDOMNode::ELEMENT_NODE);
    nsCOMPtr<nsIContent> bodyElement =
      NS_NewHTMLBodyElement(htmlNodeInfo.forget());

    
    if (htmlElement && headElement && bodyElement) {
      NS_ASSERTION(blankDoc->GetChildCount() == 0,
                   "Shouldn't have children");
      rv = blankDoc->AppendChildTo(htmlElement, PR_FALSE);
      if (NS_SUCCEEDED(rv)) {
        rv = htmlElement->AppendChildTo(headElement, PR_FALSE);

        if (NS_SUCCEEDED(rv)) {
          
          htmlElement->AppendChildTo(bodyElement, PR_FALSE);
        }
      }
    }
  }

  
  if (NS_SUCCEEDED(rv)) {
    blankDoc->SetDocumentCharacterSetSource(kCharsetFromDocTypeDefault);
    blankDoc->SetDocumentCharacterSet(NS_LITERAL_CSTRING("UTF-8"));
    
    *aDocument = blankDoc;
    NS_ADDREF(*aDocument);
  }
  return rv;
}


nsresult
nsContentDLF::CreateDocument(const char* aCommand,
                             nsIChannel* aChannel,
                             nsILoadGroup* aLoadGroup,
                             nsISupports* aContainer,
                             const nsCID& aDocumentCID,
                             nsIStreamListener** aDocListener,
                             nsIContentViewer** aDocViewer)
{
  NS_TIME_FUNCTION;

  nsresult rv = NS_ERROR_FAILURE;

  nsCOMPtr<nsIURI> aURL;
  rv = aChannel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

#ifdef NOISY_CREATE_DOC
  if (nsnull != aURL) {
    nsAutoString tmp;
    aURL->ToString(tmp);
    fputs(NS_LossyConvertUTF16toASCII(tmp).get(), stdout);
    printf(": creating document\n");
  }
#endif

  nsCOMPtr<nsIDocument> doc;
  nsCOMPtr<nsIDocumentViewer> docv;
  do {
    
    doc = do_CreateInstance(aDocumentCID, &rv);
    if (NS_FAILED(rv))
      break;

    
    rv = NS_NewDocumentViewer(getter_AddRefs(docv));
    if (NS_FAILED(rv))
      break;

    doc->SetContainer(aContainer);

    
    
    
    rv = doc->StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer, aDocListener, PR_TRUE);
    if (NS_FAILED(rv))
      break;

    
    rv = docv->LoadStart(doc);
    *aDocViewer = docv;
    NS_IF_ADDREF(*aDocViewer);
  } while (PR_FALSE);

  return rv;
}

nsresult
nsContentDLF::CreateXULDocument(const char* aCommand,
                                nsIChannel* aChannel,
                                nsILoadGroup* aLoadGroup,
                                const char* aContentType,
                                nsISupports* aContainer,
                                nsISupports* aExtraInfo,
                                nsIStreamListener** aDocListener,
                                nsIContentViewer** aDocViewer)
{
  NS_TIME_FUNCTION;

  nsresult rv;
  nsCOMPtr<nsIDocument> doc = do_CreateInstance(kXULDocumentCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDocumentViewer> docv;
  rv = NS_NewDocumentViewer(getter_AddRefs(docv));
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIURI> aURL;
  rv = aChannel->GetURI(getter_AddRefs(aURL));
  if (NS_FAILED(rv)) return rv;

  






  doc->SetContainer(aContainer);

  rv = doc->StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer, aDocListener, PR_TRUE);
  if (NS_SUCCEEDED(rv)) {
    


    rv = docv->LoadStart(doc);
    *aDocViewer = docv;
    NS_IF_ADDREF(*aDocViewer);
  }
   
  return rv;
}

PRBool nsContentDLF::IsImageContentType(const char* aContentType) {
  nsCOMPtr<imgILoader> loader(do_GetService("@mozilla.org/image/loader;1"));
  PRBool isDecoderAvailable = PR_FALSE;
  loader->SupportImageWithMimeType(aContentType, &isDecoderAvailable);
  return isDecoderAvailable;
}
