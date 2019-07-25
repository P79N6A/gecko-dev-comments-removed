







































#include "MediaDocument.h"
#include "nsRect.h"
#include "nsHTMLDocument.h"
#include "nsIImageDocument.h"
#include "nsIImageLoadingContent.h"
#include "nsGenericHTMLElement.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventListener.h"
#include "nsGkAtoms.h"
#include "imgIRequest.h"
#include "imgILoader.h"
#include "imgIContainer.h"
#include "nsStubImageDecoderObserver.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsIChannel.h"
#include "nsIContentPolicy.h"
#include "nsContentPolicyUtils.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMElement.h"
#include "nsIDOMNSHTMLElement.h"
#include "nsContentErrors.h"
#include "nsURILoader.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsIDocShellTreeItem.h"
#include "nsThreadUtils.h"
#include "nsIScrollableFrame.h"
#include "mozilla/dom/Element.h"
#include "mozilla/Preferences.h"

#define AUTOMATIC_IMAGE_RESIZING_PREF "browser.enable_automatic_image_resizing"
#define CLICK_IMAGE_RESIZING_PREF "browser.enable_click_image_resizing"

#define SITE_SPECIFIC_ZOOM "browser.zoom.siteSpecific"

namespace mozilla {
namespace dom {
 
class ImageDocument;

class ImageListener : public MediaDocumentStreamListener
{
public:
  ImageListener(ImageDocument* aDocument);
  virtual ~ImageListener();

  
  NS_IMETHOD OnStartRequest(nsIRequest* request, nsISupports *ctxt);
};

class ImageDocument : public MediaDocument
                    , public nsIImageDocument
                    , public nsStubImageDecoderObserver
                    , public nsIDOMEventListener
{
public:
  ImageDocument();
  virtual ~ImageDocument();

  NS_DECL_ISUPPORTS_INHERITED

  virtual nsresult Init();

  virtual nsresult StartDocumentLoad(const char*         aCommand,
                                     nsIChannel*         aChannel,
                                     nsILoadGroup*       aLoadGroup,
                                     nsISupports*        aContainer,
                                     nsIStreamListener** aDocListener,
                                     PRBool              aReset = PR_TRUE,
                                     nsIContentSink*     aSink = nsnull);

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject);
  virtual void Destroy();
  virtual void OnPageShow(PRBool aPersisted,
                          nsIDOMEventTarget* aDispatchStartTarget);

  NS_DECL_NSIIMAGEDOCUMENT

  
  NS_IMETHOD OnStartContainer(imgIRequest* aRequest, imgIContainer* aImage);
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest, nsresult aStatus, const PRUnichar *aStatusArg);

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ImageDocument, MediaDocument)

  friend class ImageListener;

  void DefaultCheckOverflowing() { CheckOverflowing(mResizeImageByDefault); }

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  virtual nsresult CreateSyntheticDocument();

  nsresult CheckOverflowing(PRBool changeState);

  void UpdateTitleAndCharset();

  nsresult ScrollImageTo(PRInt32 aX, PRInt32 aY, PRBool restoreImage);

  float GetRatio() {
    return NS_MIN((float)mVisibleWidth / mImageWidth,
                  (float)mVisibleHeight / mImageHeight);
  }

  void ResetZoomLevel();
  float GetZoomLevel();

  nsCOMPtr<nsIContent>          mImageContent;

  PRInt32                       mVisibleWidth;
  PRInt32                       mVisibleHeight;
  PRInt32                       mImageWidth;
  PRInt32                       mImageHeight;

  PRPackedBool                  mResizeImageByDefault;
  PRPackedBool                  mClickResizingEnabled;
  PRPackedBool                  mImageIsOverflowing;
  
  PRPackedBool                  mImageIsResized;
  
  
  
  PRPackedBool                  mShouldResize;
  PRPackedBool                  mFirstResize;
  
  PRPackedBool                  mObservingImageLoader;

  float                         mOriginalZoomLevel;
};

ImageListener::ImageListener(ImageDocument* aDocument)
  : MediaDocumentStreamListener(aDocument)
{
}

ImageListener::~ImageListener()
{
}

NS_IMETHODIMP
ImageListener::OnStartRequest(nsIRequest* request, nsISupports *ctxt)
{
  NS_ENSURE_TRUE(mDocument, NS_ERROR_FAILURE);

  ImageDocument *imgDoc = static_cast<ImageDocument*>(mDocument.get());
  nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
  if (!channel) {
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsPIDOMWindow> domWindow =
    do_QueryInterface(imgDoc->GetScriptGlobalObject());
  NS_ENSURE_TRUE(domWindow, NS_ERROR_UNEXPECTED);

  
  nsCOMPtr<nsIURI> channelURI;
  channel->GetURI(getter_AddRefs(channelURI));

  nsCAutoString mimeType;
  channel->GetContentType(mimeType);

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> channelPrincipal;
  if (secMan) {
    secMan->GetChannelPrincipal(channel, getter_AddRefs(channelPrincipal));
  }
  
  PRInt16 decision = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentProcessPolicy(nsIContentPolicy::TYPE_IMAGE,
                                             channelURI,
                                             channelPrincipal,
                                             domWindow->GetFrameElementInternal(),
                                             mimeType,
                                             nsnull,
                                             &decision,
                                             nsContentUtils::GetContentPolicy(),
                                             secMan);
                                               
  if (NS_FAILED(rv) || NS_CP_REJECTED(decision)) {
    request->Cancel(NS_ERROR_CONTENT_BLOCKED);
    return NS_OK;
  }

  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(imgDoc->mImageContent);
  NS_ENSURE_TRUE(imageLoader, NS_ERROR_UNEXPECTED);

  imageLoader->AddObserver(imgDoc);
  imgDoc->mObservingImageLoader = PR_TRUE;
  imageLoader->LoadImageWithChannel(channel, getter_AddRefs(mNextStream));

  return MediaDocumentStreamListener::OnStartRequest(request, ctxt);
}

ImageDocument::ImageDocument()
  : mOriginalZoomLevel(1.0)
{
  
  
}

ImageDocument::~ImageDocument()
{
}

NS_IMPL_CYCLE_COLLECTION_CLASS(ImageDocument)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ImageDocument, MediaDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mImageContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ImageDocument, MediaDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mImageContent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(ImageDocument, MediaDocument)
NS_IMPL_RELEASE_INHERITED(ImageDocument, MediaDocument)

DOMCI_NODE_DATA(ImageDocument, ImageDocument)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(ImageDocument)
  NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(ImageDocument)
    NS_INTERFACE_TABLE_ENTRY(ImageDocument, nsIImageDocument)
    NS_INTERFACE_TABLE_ENTRY(ImageDocument, imgIDecoderObserver)
    NS_INTERFACE_TABLE_ENTRY(ImageDocument, imgIContainerObserver)
    NS_INTERFACE_TABLE_ENTRY(ImageDocument, nsIDOMEventListener)
  NS_OFFSET_AND_INTERFACE_TABLE_END
  NS_OFFSET_AND_INTERFACE_TABLE_TO_MAP_SEGUE
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(ImageDocument)
NS_INTERFACE_MAP_END_INHERITING(MediaDocument)


nsresult
ImageDocument::Init()
{
  nsresult rv = MediaDocument::Init();
  NS_ENSURE_SUCCESS(rv, rv);

  mResizeImageByDefault = Preferences::GetBool(AUTOMATIC_IMAGE_RESIZING_PREF);
  mClickResizingEnabled = Preferences::GetBool(CLICK_IMAGE_RESIZING_PREF);
  mShouldResize = mResizeImageByDefault;
  mFirstResize = PR_TRUE;

  return NS_OK;
}

nsresult
ImageDocument::StartDocumentLoad(const char*         aCommand,
                                 nsIChannel*         aChannel,
                                 nsILoadGroup*       aLoadGroup,
                                 nsISupports*        aContainer,
                                 nsIStreamListener** aDocListener,
                                 PRBool              aReset,
                                 nsIContentSink*     aSink)
{
  nsresult rv =
    MediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer,
                                     aDocListener, aReset, aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mOriginalZoomLevel =
    Preferences::GetBool(SITE_SPECIFIC_ZOOM, PR_FALSE) ? 1.0 : GetZoomLevel();

  NS_ASSERTION(aDocListener, "null aDocListener");
  *aDocListener = new ImageListener(this);
  NS_ADDREF(*aDocListener);

  return NS_OK;
}

void
ImageDocument::Destroy()
{
  if (mImageContent) {
    
    nsCOMPtr<nsIDOMEventTarget> target = do_QueryInterface(mImageContent);
    target->RemoveEventListener(NS_LITERAL_STRING("click"), this, PR_FALSE);

    
    if (mObservingImageLoader) {
      nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
      if (imageLoader) {
        
        
        
        nsCxPusher pusher;
        pusher.PushNull();
        imageLoader->RemoveObserver(this);
      }
    }

    mImageContent = nsnull;
  }

  MediaDocument::Destroy();
}

void
ImageDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject)
{
  
  
  nsCOMPtr<nsIDOMEventTarget> target;
  if (mScriptGlobalObject &&
      aScriptGlobalObject != mScriptGlobalObject) {
    target = do_QueryInterface(mScriptGlobalObject);
    target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
    target->RemoveEventListener(NS_LITERAL_STRING("keypress"), this,
                                PR_FALSE);
  }

  
  
  nsHTMLDocument::SetScriptGlobalObject(aScriptGlobalObject);

  if (aScriptGlobalObject) {
    if (!GetRootElement()) {
      
#ifdef DEBUG
      nsresult rv =
#endif
        CreateSyntheticDocument();
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create synthetic document");

      target = do_QueryInterface(mImageContent);
      target->AddEventListener(NS_LITERAL_STRING("click"), this, PR_FALSE);
    }

    target = do_QueryInterface(aScriptGlobalObject);
    target->AddEventListener(NS_LITERAL_STRING("resize"), this, PR_FALSE);
    target->AddEventListener(NS_LITERAL_STRING("keypress"), this, PR_FALSE);
  }
}

void
ImageDocument::OnPageShow(PRBool aPersisted,
                          nsIDOMEventTarget* aDispatchStartTarget)
{
  if (aPersisted) {
    mOriginalZoomLevel =
      Preferences::GetBool(SITE_SPECIFIC_ZOOM, PR_FALSE) ? 1.0 : GetZoomLevel();
  }
  MediaDocument::OnPageShow(aPersisted, aDispatchStartTarget);
}

NS_IMETHODIMP
ImageDocument::GetImageResizingEnabled(PRBool* aImageResizingEnabled)
{
  *aImageResizingEnabled = PR_TRUE;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::GetImageIsOverflowing(PRBool* aImageIsOverflowing)
{
  *aImageIsOverflowing = mImageIsOverflowing;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::GetImageIsResized(PRBool* aImageIsResized)
{
  *aImageIsResized = mImageIsResized;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::GetImageRequest(imgIRequest** aImageRequest)
{
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
  if (imageLoader) {
    return imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                                   aImageRequest);
  }

  *aImageRequest = nsnull;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::ShrinkToFit()
{
  if (!mImageContent) {
    return NS_OK;
  }
  if (GetZoomLevel() != mOriginalZoomLevel && mImageIsResized &&
      !nsContentUtils::IsChildOfSameType(this)) {
    return NS_OK;
  }

  
  nsCOMPtr<nsIContent> imageContent = mImageContent;
  nsCOMPtr<nsIDOMHTMLImageElement> image = do_QueryInterface(mImageContent);
  image->SetWidth(NS_MAX(1, NSToCoordFloor(GetRatio() * mImageWidth)));
  image->SetHeight(NS_MAX(1, NSToCoordFloor(GetRatio() * mImageHeight)));
  
  
  
  (void) ScrollImageTo(0, 0, PR_FALSE);

  imageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
                        NS_LITERAL_STRING("cursor: -moz-zoom-in"), PR_TRUE);
  
  mImageIsResized = PR_TRUE;
  
  UpdateTitleAndCharset();

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::RestoreImageTo(PRInt32 aX, PRInt32 aY)
{
  return ScrollImageTo(aX, aY, PR_TRUE);
}

nsresult
ImageDocument::ScrollImageTo(PRInt32 aX, PRInt32 aY, PRBool restoreImage)
{
  float ratio = GetRatio();

  if (restoreImage) {
    RestoreImage();
    FlushPendingNotifications(Flush_Layout);
  }

  nsIPresShell *shell = GetShell();
  if (!shell)
    return NS_OK;

  nsIScrollableFrame* sf = shell->GetRootScrollFrameAsScrollable();
  if (!sf)
    return NS_OK;

  nsRect portRect = sf->GetScrollPortRect();
  sf->ScrollTo(nsPoint(nsPresContext::CSSPixelsToAppUnits(aX/ratio) - portRect.width/2,
                       nsPresContext::CSSPixelsToAppUnits(aY/ratio) - portRect.height/2),
               nsIScrollableFrame::INSTANT);
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::RestoreImage()
{
  if (!mImageContent) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIContent> imageContent = mImageContent;
  imageContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::width, PR_TRUE);
  imageContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::height, PR_TRUE);
  
  if (mImageIsOverflowing) {
    imageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::style,
                          NS_LITERAL_STRING("cursor: -moz-zoom-out"), PR_TRUE);
  }
  else {
    imageContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::style, PR_TRUE);
  }
  
  mImageIsResized = PR_FALSE;
  
  UpdateTitleAndCharset();

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::ToggleImageSize()
{
  mShouldResize = PR_TRUE;
  if (mImageIsResized) {
    mShouldResize = PR_FALSE;
    ResetZoomLevel();
    RestoreImage();
  }
  else if (mImageIsOverflowing) {
    ResetZoomLevel();
    ShrinkToFit();
  }

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::OnStartContainer(imgIRequest* aRequest, imgIContainer* aImage)
{
  aImage->GetWidth(&mImageWidth);
  aImage->GetHeight(&mImageHeight);
  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(this, &ImageDocument::DefaultCheckOverflowing);
  nsContentUtils::AddScriptRunner(runnable);
  UpdateTitleAndCharset();

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::OnStopDecode(imgIRequest *aRequest,
                            nsresult aStatus,
                            const PRUnichar *aStatusArg)
{
  UpdateTitleAndCharset();

  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
  if (imageLoader) {
    mObservingImageLoader = PR_FALSE;
    imageLoader->RemoveObserver(this);
  }

  
  if (NS_FAILED(aStatus) && mStringBundle && mImageContent) {
    nsCAutoString src;
    mDocumentURI->GetSpec(src);
    NS_ConvertUTF8toUTF16 srcString(src);
    const PRUnichar* formatString[] = { srcString.get() };
    nsXPIDLString errorMsg;
    NS_NAMED_LITERAL_STRING(str, "InvalidImage");
    mStringBundle->FormatStringFromName(str.get(), formatString, 1,
                                        getter_Copies(errorMsg));

    mImageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::alt, errorMsg, PR_FALSE);
  }

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("resize")) {
    CheckOverflowing(PR_FALSE);
  }
  else if (eventType.EqualsLiteral("click") && mClickResizingEnabled) {
    ResetZoomLevel();
    mShouldResize = PR_TRUE;
    if (mImageIsResized) {
      PRInt32 x = 0, y = 0;
      nsCOMPtr<nsIDOMMouseEvent> event(do_QueryInterface(aEvent));
      if (event) {
        event->GetClientX(&x);
        event->GetClientY(&y);
        PRInt32 left = 0, top = 0;
        nsCOMPtr<nsIDOMNSHTMLElement> nsElement(do_QueryInterface(mImageContent));
        nsElement->GetOffsetLeft(&left);
        nsElement->GetOffsetTop(&top);
        x -= left;
        y -= top;
      }
      mShouldResize = PR_FALSE;
      RestoreImageTo(x, y);
    }
    else if (mImageIsOverflowing) {
      ShrinkToFit();
    }
  }
  else if (eventType.EqualsLiteral("keypress")) {
    nsCOMPtr<nsIDOMKeyEvent> keyEvent = do_QueryInterface(aEvent);
    PRUint32 charCode;
    PRBool ctrlKey, metaKey, altKey;
    keyEvent->GetCharCode(&charCode);
    keyEvent->GetCtrlKey(&ctrlKey);
    keyEvent->GetMetaKey(&metaKey);
    keyEvent->GetAltKey(&altKey);
    
    if (charCode == 0x2B && !ctrlKey && !metaKey && !altKey) {
      mShouldResize = PR_FALSE;
      if (mImageIsResized) {
        ResetZoomLevel();
        RestoreImage();
      }
    }
    
    else if (charCode == 0x2D && !ctrlKey && !metaKey && !altKey) {
      mShouldResize = PR_TRUE;
      if (mImageIsOverflowing) {
        ResetZoomLevel();
        ShrinkToFit();
      }
    }
  }

  return NS_OK;
}

nsresult
ImageDocument::CreateSyntheticDocument()
{
  
  nsresult rv = MediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  Element* body = GetBodyElement();
  if (!body) {
    NS_WARNING("no body on image document!");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::style, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  nsRefPtr<nsGenericHTMLElement> styleContent = NS_NewHTMLStyleElement(nodeInfo.forget());
  if (!styleContent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  styleContent->SetTextContent(NS_LITERAL_STRING("img { display: block; }"));
  body->AppendChildTo(styleContent, PR_FALSE);

  
  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::img, nsnull,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  mImageContent = NS_NewHTMLImageElement(nodeInfo.forget());
  if (!mImageContent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
  NS_ENSURE_TRUE(imageLoader, NS_ERROR_UNEXPECTED);

  nsCAutoString src;
  mDocumentURI->GetSpec(src);

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  NS_ConvertUTF8toUTF16 srcString(src);
  
  imageLoader->SetLoadingEnabled(PR_FALSE);
  mImageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::src, srcString, PR_FALSE);
  mImageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::alt, srcString, PR_FALSE);

  body->AppendChildTo(mImageContent, PR_FALSE);
  imageLoader->SetLoadingEnabled(PR_TRUE);

  return NS_OK;
}

nsresult
ImageDocument::CheckOverflowing(PRBool changeState)
{
  



  {
    nsIPresShell *shell = GetShell();
    if (!shell) {
      return NS_OK;
    }

    nsPresContext *context = shell->GetPresContext();
    nsRect visibleArea = context->GetVisibleArea();

    Element* body = GetBodyElement();
    if (!body) {
      NS_WARNING("no body on image document!");
      return NS_ERROR_FAILURE;
    }

    nsRefPtr<nsStyleContext> styleContext =
      context->StyleSet()->ResolveStyleFor(body, nsnull);

    nsMargin m;
    if (styleContext->GetStyleMargin()->GetMargin(m))
      visibleArea.Deflate(m);
    m = styleContext->GetStyleBorder()->GetActualBorder();
    visibleArea.Deflate(m);
    if (styleContext->GetStylePadding()->GetPadding(m))
      visibleArea.Deflate(m);

    mVisibleWidth = nsPresContext::AppUnitsToIntCSSPixels(visibleArea.width);
    mVisibleHeight = nsPresContext::AppUnitsToIntCSSPixels(visibleArea.height);
  }

  PRBool imageWasOverflowing = mImageIsOverflowing;
  mImageIsOverflowing =
    mImageWidth > mVisibleWidth || mImageHeight > mVisibleHeight;
  PRBool windowBecameBigEnough = imageWasOverflowing && !mImageIsOverflowing;

  if (changeState || mShouldResize || mFirstResize ||
      windowBecameBigEnough) {
    if (mImageIsOverflowing && (changeState || mShouldResize)) {
      ShrinkToFit();
    }
    else if (mImageIsResized || mFirstResize || windowBecameBigEnough) {
      RestoreImage();
    }
  }
  mFirstResize = PR_FALSE;

  return NS_OK;
}

void 
ImageDocument::UpdateTitleAndCharset()
{
  nsCAutoString typeStr;
  nsCOMPtr<imgIRequest> imageRequest;
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
  if (imageLoader) {
    imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                            getter_AddRefs(imageRequest));
  }
    
  if (imageRequest) {
    nsXPIDLCString mimeType;
    imageRequest->GetMimeType(getter_Copies(mimeType));
    ToUpperCase(mimeType);
    nsXPIDLCString::const_iterator start, end;
    mimeType.BeginReading(start);
    mimeType.EndReading(end);
    nsXPIDLCString::const_iterator iter = end;
    if (FindInReadable(NS_LITERAL_CSTRING("IMAGE/"), start, iter) && 
        iter != end) {
      
      if (*iter == 'X') {
        ++iter;
        if (iter != end && *iter == '-') {
          ++iter;
          if (iter == end) {
            
            mimeType.BeginReading(iter);
          }
        } else {
          --iter;
        }
      }
      typeStr = Substring(iter, end);
    } else {
      typeStr = mimeType;
    }
  }

  nsXPIDLString status;
  if (mImageIsResized) {
    nsAutoString ratioStr;
    ratioStr.AppendInt(NSToCoordFloor(GetRatio() * 100));

    const PRUnichar* formatString[1] = { ratioStr.get() };
    mStringBundle->FormatStringFromName(NS_LITERAL_STRING("ScaledImage").get(),
                                        formatString, 1,
                                        getter_Copies(status));
  }

  static const char* const formatNames[4] = 
  {
    "ImageTitleWithNeitherDimensionsNorFile",
    "ImageTitleWithoutDimensions",
    "ImageTitleWithDimensions",
    "ImageTitleWithDimensionsAndFile",
  };

  MediaDocument::UpdateTitleAndCharset(typeStr, formatNames,
                                       mImageWidth, mImageHeight, status);
}

void
ImageDocument::ResetZoomLevel()
{
  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocumentContainer);
  if (docShell) {
    if (nsContentUtils::IsChildOfSameType(this)) {
      return;
    }

    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));
    nsCOMPtr<nsIMarkupDocumentViewer> mdv = do_QueryInterface(cv);
    if (mdv) {
      mdv->SetFullZoom(mOriginalZoomLevel);
    }
  }
}

float
ImageDocument::GetZoomLevel()
{
  float zoomLevel = mOriginalZoomLevel;
  nsCOMPtr<nsIDocShell> docShell = do_QueryReferent(mDocumentContainer);
  if (docShell) {
    nsCOMPtr<nsIContentViewer> cv;
    docShell->GetContentViewer(getter_AddRefs(cv));
    nsCOMPtr<nsIMarkupDocumentViewer> mdv = do_QueryInterface(cv);
    if (mdv) {
      mdv->GetFullZoom(&zoomLevel);
    }
  }
  return zoomLevel;
}

} 
} 

DOMCI_DATA(ImageDocument, mozilla::dom::ImageDocument)

nsresult
NS_NewImageDocument(nsIDocument** aResult)
{
  mozilla::dom::ImageDocument* doc = new mozilla::dom::ImageDocument();
  NS_ADDREF(doc);

  nsresult rv = doc->Init();
  if (NS_FAILED(rv)) {
    NS_RELEASE(doc);
  }

  *aResult = doc;

  return rv;
}
