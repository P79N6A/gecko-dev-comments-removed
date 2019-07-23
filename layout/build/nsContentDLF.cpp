




































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
#include "nsICSSStyleSheet.h"
#include "nsNodeInfo.h"
#include "nsNodeInfoManager.h"
#include "nsString.h"
#include "nsContentCID.h"
#include "prprf.h"
#include "nsNetUtil.h"
#include "nsICSSLoader.h"
#include "nsCRT.h"
#include "nsIViewSourceChannel.h"

#include "imgILoader.h"
#include "nsIParser.h"


#include "nsIPluginManager.h"
#include "nsIPluginHost.h"
static NS_DEFINE_CID(kPluginManagerCID, NS_PLUGINMANAGER_CID);
static NS_DEFINE_CID(kPluginDocumentCID, NS_PLUGINDOCUMENT_CID);


#define UA_CSS_URL "resource://gre/res/ua.css"



#undef NOISY_REGISTRY

static NS_DEFINE_IID(kHTMLDocumentCID, NS_HTMLDOCUMENT_CID);
static NS_DEFINE_IID(kXMLDocumentCID, NS_XMLDOCUMENT_CID);
#ifdef MOZ_SVG
static NS_DEFINE_IID(kSVGDocumentCID, NS_SVGDOCUMENT_CID);
#endif
static NS_DEFINE_IID(kImageDocumentCID, NS_IMAGEDOCUMENT_CID);
static NS_DEFINE_IID(kXULDocumentCID, NS_XULDOCUMENT_CID);

nsresult
NS_NewDocumentViewer(nsIDocumentViewer** aResult);



static const char* const gHTMLTypes[] = {
  "text/html",
  "text/plain",
  "text/css",
  "text/javascript",
  "text/ecmascript",
  "application/javascript",
  "application/ecmascript",
  "application/x-javascript",
#ifdef MOZ_VIEW_SOURCE
  "application/x-view-source", 
#endif
  "application/xhtml+xml",
  0
};
  
static const char* const gXMLTypes[] = {
  "text/xml",
  "application/xml",
  "application/rdf+xml",
  "text/rdf",
  0
};

#ifdef MOZ_SVG
static const char* const gSVGTypes[] = {
  "image/svg+xml",
  0
};

PRBool NS_SVGEnabled();
#endif

static const char* const gXULTypes[] = {
  "application/vnd.mozilla.xul+xml",
  "mozilla.application/cached-xul",
  0
};

nsICSSStyleSheet* nsContentDLF::gUAStyleSheet;

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
  EnsureUAStyleSheet();

  
#ifdef MOZ_VIEW_SOURCE
  nsCOMPtr<nsIViewSourceChannel> viewSourceChannel = do_QueryInterface(aChannel);
  if (viewSourceChannel)
  {
    aCommand = "view-source";

    
    
    
    
    nsCAutoString type;
    viewSourceChannel->GetOriginalContentType(type);
    PRBool knownType = PR_FALSE;
    PRInt32 typeIndex;
    for (typeIndex = 0; gHTMLTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gHTMLTypes[typeIndex]) &&
          !type.EqualsLiteral("application/x-view-source")) {
        knownType = PR_TRUE;
      }
    }

    for (typeIndex = 0; gXMLTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gXMLTypes[typeIndex])) {
        knownType = PR_TRUE;
      }
    }

#ifdef MOZ_SVG
    if (NS_SVGEnabled()) {
      for (typeIndex = 0; gSVGTypes[typeIndex] && !knownType; ++typeIndex) {
        if (type.Equals(gSVGTypes[typeIndex])) {
          knownType = PR_TRUE;
        }
      }
    }
#endif 

    for (typeIndex = 0; gXULTypes[typeIndex] && !knownType; ++typeIndex) {
      if (type.Equals(gXULTypes[typeIndex])) {
        knownType = PR_TRUE;
      }
    }

    if (knownType) {
      viewSourceChannel->SetContentType(type);
    } else {
      viewSourceChannel->SetContentType(NS_LITERAL_CSTRING("text/plain"));
    }
  } else if (0 == PL_strcmp("application/x-view-source", aContentType)) {
    aChannel->SetContentType(NS_LITERAL_CSTRING("text/plain"));
    aContentType = "text/plain";
  }
#endif
  
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

#ifdef MOZ_SVG
  if (NS_SVGEnabled()) {
    
    typeIndex = 0;
    while(gSVGTypes[typeIndex]) {
      if (!PL_strcmp(gSVGTypes[typeIndex++], aContentType)) {
        return CreateDocument(aCommand,
                              aChannel, aLoadGroup,
                              aContainer, kSVGDocumentCID,
                              aDocListener, aDocViewer);
      }
    }
  }
#endif

  
  typeIndex = 0;
  while (gXULTypes[typeIndex]) {
    if (0 == PL_strcmp(gXULTypes[typeIndex++], aContentType)) {
      return CreateXULDocument(aCommand, 
                               aChannel, aLoadGroup,
                               aContentType, aContainer,
                               aExtraInfo, aDocListener, aDocViewer);
    }
  }

  
  nsCOMPtr<imgILoader> loader(do_GetService("@mozilla.org/image/loader;1"));
  PRBool isReg = PR_FALSE;
  loader->SupportImageWithMimeType(aContentType, &isReg);
  if (isReg) {
    return CreateDocument(aCommand, 
                          aChannel, aLoadGroup,
                          aContainer, kImageDocumentCID,
                          aDocListener, aDocViewer);
  }

  nsCOMPtr<nsIPluginHost> ph (do_GetService(kPluginManagerCID));
  if(ph && NS_SUCCEEDED(ph->IsPluginEnabledForType(aContentType))) {
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
  nsresult rv = NS_ERROR_FAILURE;  

  EnsureUAStyleSheet();

  do {
    nsCOMPtr<nsIDocumentViewer> docv;
    rv = NS_NewDocumentViewer(getter_AddRefs(docv));
    if (NS_FAILED(rv))
      break;

    docv->SetUAStyleSheet(NS_STATIC_CAST(nsIStyleSheet*, gUAStyleSheet));

    
    nsIContentViewer* cv = NS_STATIC_CAST(nsIContentViewer*, docv.get());
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

    
    nim->GetNodeInfo(nsGkAtoms::html, 0, kNameSpaceID_None,
                     getter_AddRefs(htmlNodeInfo));
    nsCOMPtr<nsIContent> htmlElement = NS_NewHTMLHtmlElement(htmlNodeInfo);

    
    nim->GetNodeInfo(nsGkAtoms::head, 0, kNameSpaceID_None,
                     getter_AddRefs(htmlNodeInfo));
    nsCOMPtr<nsIContent> headElement = NS_NewHTMLHeadElement(htmlNodeInfo);

    
    nim->GetNodeInfo(nsGkAtoms::body, 0, kNameSpaceID_None,
                     getter_AddRefs(htmlNodeInfo));
    nsCOMPtr<nsIContent> bodyElement = NS_NewHTMLBodyElement(htmlNodeInfo);

    
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
    docv->SetUAStyleSheet(gUAStyleSheet);

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
  nsresult rv;
  nsCOMPtr<nsIDocument> doc = do_CreateInstance(kXULDocumentCID, &rv);
  if (NS_FAILED(rv)) return rv;

  nsCOMPtr<nsIDocumentViewer> docv;
  rv = NS_NewDocumentViewer(getter_AddRefs(docv));
  if (NS_FAILED(rv)) return rv;

  
  docv->SetUAStyleSheet(gUAStyleSheet);

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

static nsresult
RegisterTypes(nsICategoryManager* aCatMgr,
              const char* const* aTypes,
              PRBool aPersist = PR_TRUE)
{
  nsresult rv = NS_OK;
  while (*aTypes) {
    const char* contentType = *aTypes++;
#ifdef NOISY_REGISTRY
    printf("Register %s => %s\n", contractid, aPath);
#endif
    
    
    
    rv = aCatMgr->AddCategoryEntry("Gecko-Content-Viewers", contentType,
                                   "@mozilla.org/content/document-loader-factory;1",
                                   aPersist, PR_TRUE, nsnull);
    if (NS_FAILED(rv)) break;
  }
  return rv;
}

static nsresult UnregisterTypes(nsICategoryManager* aCatMgr,
                                const char* const* aTypes)
{
  nsresult rv = NS_OK;
  while (*aTypes) {
    const char* contentType = *aTypes++;
    rv = aCatMgr->DeleteCategoryEntry("Gecko-Content-Viewers", contentType, PR_TRUE);
    if (NS_FAILED(rv)) break;
  }
  return rv;

}

#ifdef MOZ_SVG
NS_IMETHODIMP
nsContentDLF::RegisterSVG()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  return RegisterTypes(catmgr, gSVGTypes, PR_FALSE);
}

NS_IMETHODIMP
nsContentDLF::UnregisterSVG()
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  return UnregisterTypes(catmgr, gSVGTypes);
}
#endif

NS_IMETHODIMP
nsContentDLF::RegisterDocumentFactories(nsIComponentManager* aCompMgr,
                                        nsIFile* aPath,
                                        const char *aLocation,
                                        const char *aType,
                                        const nsModuleComponentInfo* aInfo)
{
  nsresult rv;

  nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  do {
    rv = RegisterTypes(catmgr, gHTMLTypes);
    if (NS_FAILED(rv))
      break;
    rv = RegisterTypes(catmgr, gXMLTypes);
    if (NS_FAILED(rv))
      break;
    rv = RegisterTypes(catmgr, gXULTypes);
    if (NS_FAILED(rv))
      break;
  } while (PR_FALSE);
  return rv;
}

NS_IMETHODIMP
nsContentDLF::UnregisterDocumentFactories(nsIComponentManager* aCompMgr,
                                          nsIFile* aPath,
                                          const char* aRegistryLocation,
                                          const nsModuleComponentInfo* aInfo)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> catmgr(do_GetService(NS_CATEGORYMANAGER_CONTRACTID, &rv));
  if (NS_FAILED(rv)) return rv;

  do {
    rv = UnregisterTypes(catmgr, gHTMLTypes);
    if (NS_FAILED(rv))
      break;
    rv = UnregisterTypes(catmgr, gXMLTypes);
    if (NS_FAILED(rv))
      break;
#ifdef MOZ_SVG
    rv = UnregisterTypes(catmgr, gSVGTypes);
    if (NS_FAILED(rv))
      break;
#endif
    rv = UnregisterTypes(catmgr, gXULTypes);
    if (NS_FAILED(rv))
      break;
  } while (PR_FALSE);

  return rv;
}

 nsresult
nsContentDLF::EnsureUAStyleSheet()
{
  if (gUAStyleSheet)
    return NS_OK;

  
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), NS_LITERAL_CSTRING(UA_CSS_URL));
  if (NS_FAILED(rv)) {
#ifdef DEBUG
    printf("*** open of %s failed: error=%x\n", UA_CSS_URL, rv);
#endif
    return rv;
  }
  nsCOMPtr<nsICSSLoader> cssLoader;
  NS_NewCSSLoader(getter_AddRefs(cssLoader));
  if (!cssLoader)
    return NS_ERROR_OUT_OF_MEMORY;
  rv = cssLoader->LoadSheetSync(uri, PR_TRUE, &gUAStyleSheet);
#ifdef DEBUG
  if (NS_FAILED(rv))
    printf("*** open of %s failed: error=%x\n", UA_CSS_URL, rv);
#endif
  return rv;
}
