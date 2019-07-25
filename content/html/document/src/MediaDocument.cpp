





































#include "MediaDocument.h"
#include "nsGkAtoms.h"
#include "nsRect.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIScrollable.h"
#include "nsIViewManager.h"
#include "nsITextToSubURI.h"
#include "nsIURL.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDocShell.h"
#include "nsCharsetSource.h" 
#include "nsNodeInfoManager.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

MediaDocumentStreamListener::MediaDocumentStreamListener(MediaDocument *aDocument)
{
  mDocument = aDocument;
}

MediaDocumentStreamListener::~MediaDocumentStreamListener()
{
}


NS_IMPL_THREADSAFE_ISUPPORTS2(MediaDocumentStreamListener,
                              nsIRequestObserver,
                              nsIStreamListener)


void
MediaDocumentStreamListener::SetStreamListener(nsIStreamListener *aListener)
{
  mNextStream = aListener;
}

NS_IMETHODIMP
MediaDocumentStreamListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  mDocument->StartLayout();

  if (mNextStream) {
    return mNextStream->OnStartRequest(request, ctxt);
  }

  return NS_BINDING_ABORTED;
}

NS_IMETHODIMP
MediaDocumentStreamListener::OnStopRequest(nsIRequest* request,
                                           nsISupports *ctxt,
                                           nsresult status)
{
  nsresult rv = NS_OK;
  if (mNextStream) {
    rv = mNextStream->OnStopRequest(request, ctxt, status);
  }

  
  mDocument = nsnull;

  return rv;
}

NS_IMETHODIMP
MediaDocumentStreamListener::OnDataAvailable(nsIRequest* request,
                                             nsISupports *ctxt,
                                             nsIInputStream *inStr,
                                             PRUint32 sourceOffset,
                                             PRUint32 count)
{
  if (mNextStream) {
    return mNextStream->OnDataAvailable(request, ctxt, inStr, sourceOffset, count);
  }

  return NS_OK;
}


const char* const MediaDocument::sFormatNames[4] = 
{
  "MediaTitleWithNoInfo",    
  "MediaTitleWithFile",      
  "",                        
  ""                         
};

MediaDocument::MediaDocument()
    : mDocumentElementInserted(false)
{
}
MediaDocument::~MediaDocument()
{
}

nsresult
MediaDocument::Init()
{
  nsresult rv = nsHTMLDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIStringBundleService> stringService =
    mozilla::services::GetStringBundleService();
  if (stringService) {
    stringService->CreateBundle(NSMEDIADOCUMENT_PROPERTIES_URI,
                                getter_AddRefs(mStringBundle));
  }

  mIsSyntheticDocument = true;

  return NS_OK;
}

nsresult
MediaDocument::StartDocumentLoad(const char*         aCommand,
                                 nsIChannel*         aChannel,
                                 nsILoadGroup*       aLoadGroup,
                                 nsISupports*        aContainer,
                                 nsIStreamListener** aDocListener,
                                 bool                aReset,
                                 nsIContentSink*     aSink)
{
  nsresult rv = nsDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup,
                                              aContainer, aDocListener, aReset,
                                              aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  

  
  
  
  
  
  
  
  
  
  
  
    
  nsCOMPtr<nsIDocShell> docShell(do_QueryInterface(aContainer));

  
  NS_ENSURE_TRUE(docShell, NS_OK); 

  nsCAutoString charset;

  nsCOMPtr<nsIAtom> csAtom;
  docShell->GetParentCharset(getter_AddRefs(csAtom));
  if (csAtom) {   
    csAtom->ToUTF8String(charset);
  }

  if (charset.IsEmpty() || charset.Equals("UTF-8")) {
    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));

    
    NS_ENSURE_TRUE(cv, NS_OK); 
    nsCOMPtr<nsIMarkupDocumentViewer> muCV = do_QueryInterface(cv);
    if (muCV) {
      muCV->GetPrevDocCharacterSet(charset);   
      if (charset.Equals("UTF-8") || charset.IsEmpty()) {
        muCV->GetDefaultCharacterSet(charset); 
      }
    } 
  }

  if (!charset.IsEmpty() && !charset.Equals("UTF-8")) {
    SetDocumentCharacterSet(charset);
    mCharacterSetSource = kCharsetFromUserDefault;
  }

  return NS_OK;
}

nsresult
MediaDocument::CreateSyntheticDocument()
{
  
  nsresult rv;

  nsRefPtr<nsNodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::html, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<nsGenericHTMLElement> root = NS_NewHTMLHtmlElement(nodeInfo.forget());
  NS_ENSURE_TRUE(root, NS_ERROR_OUT_OF_MEMORY);

  NS_ASSERTION(GetChildCount() == 0, "Shouldn't have any kids");
  rv = AppendChildTo(root, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::head, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  
  nsRefPtr<nsGenericHTMLElement> head = NS_NewHTMLHeadElement(nodeInfo.forget());
  NS_ENSURE_TRUE(head, NS_ERROR_OUT_OF_MEMORY);

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::meta, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<nsGenericHTMLElement> metaContent = NS_NewHTMLMetaElement(nodeInfo.forget());
  NS_ENSURE_TRUE(metaContent, NS_ERROR_OUT_OF_MEMORY);
  metaContent->SetAttr(kNameSpaceID_None, nsGkAtoms::name,
                       NS_LITERAL_STRING("viewport"),
                       true);

  metaContent->SetAttr(kNameSpaceID_None, nsGkAtoms::content,
                       NS_LITERAL_STRING("width=device-width; height=device-height;"),
                       true);
  head->AppendChildTo(metaContent, false);

  root->AppendChildTo(head, false);

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::body, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<nsGenericHTMLElement> body = NS_NewHTMLBodyElement(nodeInfo.forget());
  NS_ENSURE_TRUE(body, NS_ERROR_OUT_OF_MEMORY);

  root->AppendChildTo(body, false);

  return NS_OK;
}

nsresult
MediaDocument::StartLayout()
{
  mMayStartLayout = true;
  nsCOMPtr<nsIPresShell> shell = GetShell();
  
  
  if (shell && !shell->DidInitialReflow()) {
    nsRect visibleArea = shell->GetPresContext()->GetVisibleArea();
    nsresult rv = shell->InitialReflow(visibleArea.width, visibleArea.height);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
MediaDocument::GetFileName(nsAString& aResult)
{
  aResult.Truncate();

  nsCOMPtr<nsIURL> url = do_QueryInterface(mDocumentURI);
  if (!url)
    return;

  nsCAutoString fileName;
  url->GetFileName(fileName);
  if (fileName.IsEmpty())
    return;

  nsCAutoString docCharset;
  
  
  
  
  
  
  if (mCharacterSetSource != kCharsetUninitialized) {  
    docCharset = mCharacterSet;
  } else {  
    
    url->GetOriginCharset(docCharset);
    SetDocumentCharacterSet(docCharset);
  }

  nsresult rv;
  nsCOMPtr<nsITextToSubURI> textToSubURI = 
    do_GetService(NS_ITEXTTOSUBURI_CONTRACTID, &rv);
  if (NS_SUCCEEDED(rv)) {
    
    textToSubURI->UnEscapeURIForUI(docCharset, fileName, aResult);
  } else {
    CopyUTF8toUTF16(fileName, aResult);
  }
}

nsresult
MediaDocument::LinkStylesheet(const nsAString& aStylesheet)
{
  nsRefPtr<nsNodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::link, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  nsRefPtr<nsGenericHTMLElement> link = NS_NewHTMLLinkElement(nodeInfo.forget());
  NS_ENSURE_TRUE(link, NS_ERROR_OUT_OF_MEMORY);

  link->SetAttr(kNameSpaceID_None, nsGkAtoms::rel, 
                NS_LITERAL_STRING("stylesheet"), true);

  link->SetAttr(kNameSpaceID_None, nsGkAtoms::href, aStylesheet, true);

  Element* head = GetHeadElement();
  return head->AppendChildTo(link, false);
}

void 
MediaDocument::UpdateTitleAndCharset(const nsACString& aTypeStr,
                                     const char* const* aFormatNames,
                                     PRInt32 aWidth, PRInt32 aHeight,
                                     const nsAString& aStatus)
{
  nsXPIDLString fileStr;
  GetFileName(fileStr);

  NS_ConvertASCIItoUTF16 typeStr(aTypeStr);
  nsXPIDLString title;

  if (mStringBundle) {
    
    if (aWidth != 0 && aHeight != 0) {
      nsAutoString widthStr;
      nsAutoString heightStr;
      widthStr.AppendInt(aWidth);
      heightStr.AppendInt(aHeight);
      
      if (!fileStr.IsEmpty()) {
        const PRUnichar *formatStrings[4]  = {fileStr.get(), typeStr.get(), 
          widthStr.get(), heightStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithDimAndFile]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 4,
                                            getter_Copies(title));
      } 
      else {
        const PRUnichar *formatStrings[3]  = {typeStr.get(), widthStr.get(), 
          heightStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithDim]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 3,
                                            getter_Copies(title));
      }
    } 
    else {
    
      if (!fileStr.IsEmpty()) {
        const PRUnichar *formatStrings[2] = {fileStr.get(), typeStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithFile]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 2,
                                            getter_Copies(title));
      }
      else {
        const PRUnichar *formatStrings[1] = {typeStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithNoInfo]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 1,
                                            getter_Copies(title));
      }
    }
  } 

  
  if (aStatus.IsEmpty()) {
    SetTitle(title);
  }
  else {
    nsXPIDLString titleWithStatus;
    const nsPromiseFlatString& status = PromiseFlatString(aStatus);
    const PRUnichar *formatStrings[2] = {title.get(), status.get()};
    NS_NAMED_LITERAL_STRING(fmtName, "TitleWithStatus");
    mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 2,
                                        getter_Copies(titleWithStatus));
    SetTitle(titleWithStatus);
  }
}

void 
MediaDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aGlobalObject)
{
    nsHTMLDocument::SetScriptGlobalObject(aGlobalObject);
    if (!mDocumentElementInserted && aGlobalObject) {
        mDocumentElementInserted = true;
        nsContentUtils::AddScriptRunner(
            new nsDocElementCreatedNotificationRunner(this));        
    }
}

} 
} 
