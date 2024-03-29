





#include "MediaDocument.h"
#include "nsGkAtoms.h"
#include "nsRect.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsIScrollable.h"
#include "nsViewManager.h"
#include "nsITextToSubURI.h"
#include "nsIURL.h"
#include "nsIContentViewer.h"
#include "nsIDocShell.h"
#include "nsCharsetSource.h" 
#include "nsNodeInfoManager.h"
#include "nsContentUtils.h"
#include "nsDocElementCreatedNotificationRunner.h"
#include "mozilla/Services.h"
#include "nsServiceManagerUtils.h"
#include "nsIPrincipal.h"
#include "nsIMultiPartChannel.h"

namespace mozilla {
namespace dom {

MediaDocumentStreamListener::MediaDocumentStreamListener(MediaDocument *aDocument)
{
  mDocument = aDocument;
}

MediaDocumentStreamListener::~MediaDocumentStreamListener()
{
}


NS_IMPL_ISUPPORTS(MediaDocumentStreamListener,
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

  
  bool lastPart = true;
  nsCOMPtr<nsIMultiPartChannel> mpchan(do_QueryInterface(request));
  if (mpchan) {
    mpchan->GetIsLastPart(&lastPart);
  }

  if (lastPart) {
    mDocument = nullptr;
  }
  return rv;
}

NS_IMETHODIMP
MediaDocumentStreamListener::OnDataAvailable(nsIRequest* request,
                                             nsISupports *ctxt,
                                             nsIInputStream *inStr,
                                             uint64_t sourceOffset,
                                             uint32_t count)
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
    : nsHTMLDocument(),
      mDocumentElementInserted(false)
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

  nsAutoCString charset;
  int32_t source;
  nsCOMPtr<nsIPrincipal> principal;
  
  docShell->GetParentCharset(charset, &source, getter_AddRefs(principal));

  if (!charset.IsEmpty() &&
      !charset.EqualsLiteral("UTF-8") &&
      NodePrincipal()->Equals(principal)) {
    SetDocumentCharacterSetSource(source);
    SetDocumentCharacterSet(charset);
  }

  return NS_OK;
}

void
MediaDocument::BecomeInteractive()
{
  
  
  bool restoring = false;
  nsPIDOMWindow* window = GetWindow();
  if (window) {
    nsIDocShell* docShell = window->GetDocShell();
    if (docShell) {
      docShell->GetRestoringDocument(&restoring);
    }
  }
  if (!restoring) {
    MOZ_ASSERT(GetReadyStateEnum() == nsIDocument::READYSTATE_LOADING,
               "Bad readyState");
    SetReadyStateInternal(nsIDocument::READYSTATE_INTERACTIVE);
  }
}

nsresult
MediaDocument::CreateSyntheticDocument()
{
  
  nsresult rv;

  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::html, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

  nsRefPtr<nsGenericHTMLElement> root = NS_NewHTMLHtmlElement(nodeInfo.forget());
  NS_ENSURE_TRUE(root, NS_ERROR_OUT_OF_MEMORY);

  NS_ASSERTION(GetChildCount() == 0, "Shouldn't have any kids");
  rv = AppendChildTo(root, false);
  NS_ENSURE_SUCCESS(rv, rv);

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::head, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

  
  nsRefPtr<nsGenericHTMLElement> head = NS_NewHTMLHeadElement(nodeInfo.forget());
  NS_ENSURE_TRUE(head, NS_ERROR_OUT_OF_MEMORY);

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::meta, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

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

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::body, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

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
  
  
  if (shell && !shell->DidInitialize()) {
    nsRect visibleArea = shell->GetPresContext()->GetVisibleArea();
    nsresult rv = shell->Initialize(visibleArea.width, visibleArea.height);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
MediaDocument::GetFileName(nsAString& aResult, nsIChannel* aChannel)
{
  aResult.Truncate();

  if (aChannel) {
    aChannel->GetContentDispositionFilename(aResult);
    if (!aResult.IsEmpty())
      return;
  }

  nsCOMPtr<nsIURL> url = do_QueryInterface(mDocumentURI);
  if (!url)
    return;

  nsAutoCString fileName;
  url->GetFileName(fileName);
  if (fileName.IsEmpty())
    return;

  nsAutoCString docCharset;
  
  
  
  
  
  
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
  nsRefPtr<mozilla::dom::NodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::link, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

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
                                     nsIChannel* aChannel,
                                     const char* const* aFormatNames,
                                     int32_t aWidth, int32_t aHeight,
                                     const nsAString& aStatus)
{
  nsXPIDLString fileStr;
  GetFileName(fileStr, aChannel);

  NS_ConvertASCIItoUTF16 typeStr(aTypeStr);
  nsXPIDLString title;

  if (mStringBundle) {
    
    if (aWidth != 0 && aHeight != 0) {
      nsAutoString widthStr;
      nsAutoString heightStr;
      widthStr.AppendInt(aWidth);
      heightStr.AppendInt(aHeight);
      
      if (!fileStr.IsEmpty()) {
        const char16_t *formatStrings[4]  = {fileStr.get(), typeStr.get(), 
          widthStr.get(), heightStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithDimAndFile]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 4,
                                            getter_Copies(title));
      } 
      else {
        const char16_t *formatStrings[3]  = {typeStr.get(), widthStr.get(), 
          heightStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithDim]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 3,
                                            getter_Copies(title));
      }
    } 
    else {
    
      if (!fileStr.IsEmpty()) {
        const char16_t *formatStrings[2] = {fileStr.get(), typeStr.get()};
        NS_ConvertASCIItoUTF16 fmtName(aFormatNames[eWithFile]);
        mStringBundle->FormatStringFromName(fmtName.get(), formatStrings, 2,
                                            getter_Copies(title));
      }
      else {
        const char16_t *formatStrings[1] = {typeStr.get()};
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
    const char16_t *formatStrings[2] = {title.get(), status.get()};
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
