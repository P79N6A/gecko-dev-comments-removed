




#include "MediaDocument.h"
#include "nsRect.h"
#include "nsHTMLDocument.h"
#include "nsIImageDocument.h"
#include "nsIImageLoadingContent.h"
#include "nsGenericHTMLElement.h"
#include "nsIDocumentInlines.h"
#include "nsDOMTokenList.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDOMEvent.h"
#include "nsIDOMKeyEvent.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMEventListener.h"
#include "nsGkAtoms.h"
#include "imgIRequest.h"
#include "imgILoader.h"
#include "imgIContainer.h"
#include "imgINotificationObserver.h"
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
#include "nsIDOMHTMLElement.h"
#include "nsError.h"
#include "nsURILoader.h"
#include "nsIDocShell.h"
#include "nsIContentViewer.h"
#include "nsIMarkupDocumentViewer.h"
#include "nsThreadUtils.h"
#include "nsIScrollableFrame.h"
#include "nsContentUtils.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLImageElement.h"
#include "mozilla/Preferences.h"
#include <algorithm>

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
                    , public imgINotificationObserver
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
                                     bool                aReset = true,
                                     nsIContentSink*     aSink = nullptr);

  virtual void SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject);
  virtual void Destroy();
  virtual void OnPageShow(bool aPersisted,
                          EventTarget* aDispatchStartTarget);

  NS_DECL_NSIIMAGEDOCUMENT
  NS_DECL_IMGINOTIFICATIONOBSERVER

  
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(ImageDocument, MediaDocument)

  friend class ImageListener;

  void DefaultCheckOverflowing() { CheckOverflowing(mResizeImageByDefault); }

  virtual nsXPCClassInfo* GetClassInfo();
protected:
  virtual nsresult CreateSyntheticDocument();

  nsresult CheckOverflowing(bool changeState);

  void UpdateTitleAndCharset();

  enum eScaleOptions {
    eDevicePixelScale,
    eCSSPixelScale
  };
  nsresult ScrollImageTo(int32_t aX, int32_t aY, eScaleOptions aScaleOption);

  float GetDevicePixelSizeInCSSPixels() {
    nsIPresShell *shell = GetShell();
    if (!shell) {
      return 1.0f;
    }
    return shell->GetPresContext()->DevPixelsToFloatCSSPixels(1);
  }

  float GetShrinkToFitRatio() {
    return std::min(mVisibleWidth / mImageWidth,
                    mVisibleHeight / mImageHeight);
  }

  float GetCurrentRatio() {
    return mImageIsScaledToDevicePixels ? GetDevicePixelSizeInCSSPixels() :
           mImageIsResized ? GetShrinkToFitRatio() : 1.0f;
  }

  void ResetZoomLevel();
  float GetZoomLevel();

  enum eModeClasses {
    eNone,
    eShrinkToFit,
    eScaleToDevicePixels,
    eOverflowing
  };
  void SetModeClass(eModeClasses mode);

  nsresult OnStartContainer(imgIRequest* aRequest, imgIContainer* aImage);
  nsresult OnStopRequest(imgIRequest *aRequest, nsresult aStatus);

  nsCOMPtr<nsIContent>          mImageContent;

  float                         mVisibleWidth;
  float                         mVisibleHeight;
  int32_t                       mImageWidth;
  int32_t                       mImageHeight;

  bool                          mResizeImageByDefault;
  bool                          mClickResizingEnabled;
  bool                          mImageIsOverflowing;
  
  
  
  
  bool                          mImageIsResized;
  bool                          mImageIsScaledToDevicePixels;
  
  
  
  bool                          mShouldResize;
  bool                          mFirstResize;
  
  bool                          mObservingImageLoader;

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

  nsAutoCString mimeType;
  channel->GetContentType(mimeType);

  nsIScriptSecurityManager* secMan = nsContentUtils::GetSecurityManager();
  nsCOMPtr<nsIPrincipal> channelPrincipal;
  if (secMan) {
    secMan->GetChannelPrincipal(channel, getter_AddRefs(channelPrincipal));
  }
  
  int16_t decision = nsIContentPolicy::ACCEPT;
  nsresult rv = NS_CheckContentProcessPolicy(nsIContentPolicy::TYPE_IMAGE,
                                             channelURI,
                                             channelPrincipal,
                                             domWindow->GetFrameElementInternal(),
                                             mimeType,
                                             nullptr,
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
  imgDoc->mObservingImageLoader = true;
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


NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(ImageDocument, MediaDocument)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mImageContent)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(ImageDocument, MediaDocument)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mImageContent)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_ADDREF_INHERITED(ImageDocument, MediaDocument)
NS_IMPL_RELEASE_INHERITED(ImageDocument, MediaDocument)

DOMCI_NODE_DATA(ImageDocument, ImageDocument)

NS_INTERFACE_TABLE_HEAD_CYCLE_COLLECTION_INHERITED(ImageDocument)
  NS_HTML_DOCUMENT_INTERFACE_TABLE_BEGIN(ImageDocument)
    NS_INTERFACE_TABLE_ENTRY(ImageDocument, nsIImageDocument)
    NS_INTERFACE_TABLE_ENTRY(ImageDocument, imgINotificationObserver)
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
  mFirstResize = true;

  return NS_OK;
}

nsresult
ImageDocument::StartDocumentLoad(const char*         aCommand,
                                 nsIChannel*         aChannel,
                                 nsILoadGroup*       aLoadGroup,
                                 nsISupports*        aContainer,
                                 nsIStreamListener** aDocListener,
                                 bool                aReset,
                                 nsIContentSink*     aSink)
{
  nsresult rv =
    MediaDocument::StartDocumentLoad(aCommand, aChannel, aLoadGroup, aContainer,
                                     aDocListener, aReset, aSink);
  if (NS_FAILED(rv)) {
    return rv;
  }

  mOriginalZoomLevel =
    Preferences::GetBool(SITE_SPECIFIC_ZOOM, false) ? 1.0 : GetZoomLevel();

  NS_ASSERTION(aDocListener, "null aDocListener");
  *aDocListener = new ImageListener(this);
  NS_ADDREF(*aDocListener);

  return NS_OK;
}

void
ImageDocument::Destroy()
{
  if (mImageContent) {
    
    nsCOMPtr<EventTarget> target = do_QueryInterface(mImageContent);
    target->RemoveEventListener(NS_LITERAL_STRING("click"), this, false);

    
    if (mObservingImageLoader) {
      nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
      if (imageLoader) {
        
        
        
        nsCxPusher pusher;
        pusher.PushNull();
        imageLoader->RemoveObserver(this);
      }
    }

    mImageContent = nullptr;
  }

  MediaDocument::Destroy();
}

void
ImageDocument::SetScriptGlobalObject(nsIScriptGlobalObject* aScriptGlobalObject)
{
  
  
  nsCOMPtr<EventTarget> target;
  if (mScriptGlobalObject &&
      aScriptGlobalObject != mScriptGlobalObject) {
    target = do_QueryInterface(mScriptGlobalObject);
    target->RemoveEventListener(NS_LITERAL_STRING("resize"), this, false);
    target->RemoveEventListener(NS_LITERAL_STRING("keypress"), this,
                                false);
  }

  
  
  MediaDocument::SetScriptGlobalObject(aScriptGlobalObject);

  if (aScriptGlobalObject) {
    if (!GetRootElement()) {
      
#ifdef DEBUG
      nsresult rv =
#endif
        CreateSyntheticDocument();
      NS_ASSERTION(NS_SUCCEEDED(rv), "failed to create synthetic document");

      target = do_QueryInterface(mImageContent);
      target->AddEventListener(NS_LITERAL_STRING("click"), this, false);
    }

    target = do_QueryInterface(aScriptGlobalObject);
    target->AddEventListener(NS_LITERAL_STRING("resize"), this, false);
    target->AddEventListener(NS_LITERAL_STRING("keypress"), this, false);

    if (!nsContentUtils::IsChildOfSameType(this) &&
        GetReadyStateEnum() != nsIDocument::READYSTATE_COMPLETE) {
      LinkStylesheet(NS_LITERAL_STRING("resource://gre/res/TopLevelImageDocument.css"));
      LinkStylesheet(NS_LITERAL_STRING("chrome://global/skin/media/TopLevelImageDocument.css"));
    }
    BecomeInteractive();
  }
}

void
ImageDocument::OnPageShow(bool aPersisted,
                          EventTarget* aDispatchStartTarget)
{
  if (aPersisted) {
    mOriginalZoomLevel =
      Preferences::GetBool(SITE_SPECIFIC_ZOOM, false) ? 1.0 : GetZoomLevel();
  }
  MediaDocument::OnPageShow(aPersisted, aDispatchStartTarget);
}

NS_IMETHODIMP
ImageDocument::GetImageResizingEnabled(bool* aImageResizingEnabled)
{
  *aImageResizingEnabled = true;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::GetImageIsOverflowing(bool* aImageIsOverflowing)
{
  *aImageIsOverflowing = mImageIsOverflowing;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::GetImageIsResized(bool* aImageIsResized)
{
  *aImageIsResized = mImageIsResized;
  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::GetImageIsScaledToDevicePixels(bool* aImageIsScaledToDevPix)
{
  *aImageIsScaledToDevPix = mImageIsScaledToDevicePixels;
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

  *aImageRequest = nullptr;
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
  image->SetWidth(std::max(1, NSToCoordFloor(GetShrinkToFitRatio() * mImageWidth)));
  image->SetHeight(std::max(1, NSToCoordFloor(GetShrinkToFitRatio() * mImageHeight)));

  SetModeClass(eShrinkToFit);

  mImageIsResized = true;

  UpdateTitleAndCharset();

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::RestoreImageTo(int32_t aX, int32_t aY)
{
  return ScrollImageTo(aX, aY, eCSSPixelScale);
}

NS_IMETHODIMP
ImageDocument::ScaleToDevicePixelsTo(int32_t aX, int32_t aY)
{
  return ScrollImageTo(aX, aY, eDevicePixelScale);
}

nsresult
ImageDocument::ScrollImageTo(int32_t aX, int32_t aY,
                             eScaleOptions aScaleOption)
{
  
  
  

  nsIPresShell *shell = GetShell();
  if (!shell) {
    return NS_OK;
  }

  nsIScrollableFrame* sf = shell->GetRootScrollFrameAsScrollable();
  if (!sf) {
    return NS_OK;
  }

  
  
  float ratio = GetCurrentRatio();
  float imageX = aX / ratio;
  float imageY = aY / ratio;

  
  
  
  nsRefPtr<nsClientRect> clientRect =
    mImageContent->AsElement()->GetBoundingClientRect();
  float origLeft = clientRect->Left();
  float origTop = clientRect->Top();

  
  switch (aScaleOption) {
  case eDevicePixelScale:
    ScaleToDevicePixels();
    break;
  case eCSSPixelScale:
    RestoreImage();
    break;
  }
  FlushPendingNotifications(Flush_Layout);

  nsPoint scroll = sf->GetScrollPosition();

  
  
  clientRect = mImageContent->AsElement()->GetBoundingClientRect();
  float clientLeft = clientRect->Left();
  float clientTop = clientRect->Top();
  ratio = GetCurrentRatio();

  
  
  
  scroll.x +=
    nsPresContext::CSSPixelsToAppUnits((clientLeft + imageX * ratio) -
                                       (origLeft + aX));
  scroll.y +=
    nsPresContext::CSSPixelsToAppUnits((clientTop + imageY * ratio) -
                                       (origTop + aY));

  
  
  
  sf->ScrollTo(scroll, nsIScrollableFrame::INSTANT);

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::RestoreImage()
{
  if (!mImageContent) {
    return NS_OK;
  }
  
  nsCOMPtr<nsIContent> imageContent = mImageContent;
  imageContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::width, true);
  imageContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::height, true);

  
  
  
  if (mImageIsOverflowing || GetDevicePixelSizeInCSSPixels() < 1.0f) {
    SetModeClass(eOverflowing);
  }
  else {
    SetModeClass(eNone);
  }

  mImageIsResized = false;
  mImageIsScaledToDevicePixels = false;

  UpdateTitleAndCharset();

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::ScaleToDevicePixels()
{
  if (!mImageContent) {
    return NS_OK;
  }

  float ratio = GetDevicePixelSizeInCSSPixels();
  if (ratio == 1.0f) {
    
    return RestoreImage();
  }

  nsCOMPtr<nsIDOMHTMLImageElement> image = do_QueryInterface(mImageContent);
  image->SetWidth(std::max(1, NSToCoordFloor(ratio * mImageWidth)));
  image->SetHeight(std::max(1, NSToCoordFloor(ratio * mImageHeight)));

  SetModeClass(eScaleToDevicePixels);

  mImageIsResized = false;
  mImageIsScaledToDevicePixels = true;

  UpdateTitleAndCharset();

  return NS_OK;
}


NS_IMETHODIMP
ImageDocument::ToggleImageSize()
{
  mShouldResize = true;
  if (mImageIsResized) {
    mShouldResize = false;
    ResetZoomLevel();
    RestoreImage();
  } else if (mImageIsOverflowing) {
    ResetZoomLevel();
    ShrinkToFit();
  }

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::Notify(imgIRequest* aRequest, int32_t aType, const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    return OnStartContainer(aRequest, image);
  }

  nsDOMTokenList* classList = mImageContent->AsElement()->GetClassList();
  mozilla::ErrorResult rv;
  if (aType == imgINotificationObserver::DECODE_COMPLETE) {
    if (mImageContent) {
      
      
      
      classList->Add(NS_LITERAL_STRING("decoded"), rv);
      NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());
    }
  }

  if (aType == imgINotificationObserver::DISCARD) {
    
    if (mImageContent) {
      
      classList->Remove(NS_LITERAL_STRING("decoded"), rv);
      NS_ENSURE_SUCCESS(rv.ErrorCode(), rv.ErrorCode());
    }
  }

  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    uint32_t reqStatus;
    aRequest->GetImageStatus(&reqStatus);
    nsresult status =
        reqStatus & imgIRequest::STATUS_ERROR ? NS_ERROR_FAILURE : NS_OK;
    return OnStopRequest(aRequest, status);
  }

  return NS_OK;
}

void
ImageDocument::SetModeClass(eModeClasses mode)
{
  nsDOMTokenList* classList = mImageContent->AsElement()->GetClassList();
  mozilla::ErrorResult rv;

  if (mode == eShrinkToFit) {
    classList->Add(NS_LITERAL_STRING("shrinkToFit"), rv);
  } else {
    classList->Remove(NS_LITERAL_STRING("shrinkToFit"), rv);
  }

  if (mode == eScaleToDevicePixels) {
    classList->Add(NS_LITERAL_STRING("scaleToDevicePixels"), rv);
  } else {
    classList->Remove(NS_LITERAL_STRING("scaleToDevicePixels"), rv);
  }

  if (mode == eOverflowing) {
    classList->Add(NS_LITERAL_STRING("overflowing"), rv);
  } else {
    classList->Remove(NS_LITERAL_STRING("overflowing"), rv);
  }
}

nsresult
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

nsresult
ImageDocument::OnStopRequest(imgIRequest *aRequest,
                             nsresult aStatus)
{
  UpdateTitleAndCharset();

  
  if (NS_FAILED(aStatus) && mStringBundle && mImageContent) {
    nsAutoCString src;
    mDocumentURI->GetSpec(src);
    NS_ConvertUTF8toUTF16 srcString(src);
    const PRUnichar* formatString[] = { srcString.get() };
    nsXPIDLString errorMsg;
    NS_NAMED_LITERAL_STRING(str, "InvalidImage");
    mStringBundle->FormatStringFromName(str.get(), formatString, 1,
                                        getter_Copies(errorMsg));

    mImageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::alt, errorMsg, false);
  }

  return NS_OK;
}

NS_IMETHODIMP
ImageDocument::HandleEvent(nsIDOMEvent* aEvent)
{
  nsAutoString eventType;
  aEvent->GetType(eventType);
  if (eventType.EqualsLiteral("resize")) {
    CheckOverflowing(false);
    return NS_OK;
  }

  if (eventType.EqualsLiteral("click") && mClickResizingEnabled) {
    ResetZoomLevel();
    mShouldResize = true;

    float devPixelRatio = GetDevicePixelSizeInCSSPixels();
    float shrinkToFitRatio = GetShrinkToFitRatio();

    
    
    int32_t x = 0, y = 0;
    nsCOMPtr<nsIDOMMouseEvent> event(do_QueryInterface(aEvent));
    if (event) {
      event->GetClientX(&x);
      event->GetClientY(&y);
      
      
      nsRefPtr<nsClientRect> clientRect =
        mImageContent->AsElement()->GetBoundingClientRect();
      x -= NSToIntRound(clientRect->Left());
      y -= NSToIntRound(clientRect->Top());
    }

    if (mImageIsResized || mImageIsScaledToDevicePixels) {
      
      if (devPixelRatio == 1.0f) {
        RestoreImageTo(x, y);
        mShouldResize = false;
        return NS_OK;
      }

      if (mImageIsResized) {
        
        
        
        if (devPixelRatio > shrinkToFitRatio) {
          ScaleToDevicePixelsTo(x, y);
        } else {
          RestoreImageTo(x, y);
        }
        mShouldResize = false;
      } else {
        if (shrinkToFitRatio > devPixelRatio && shrinkToFitRatio < 1.0f) {
          ShrinkToFit();
        } else {
          RestoreImageTo(x, y);
          mShouldResize = false;
        }
      }
      return NS_OK;
    }

    if (mImageIsOverflowing) {
      
      
      if (devPixelRatio < shrinkToFitRatio) {
        ScaleToDevicePixelsTo(x, y);
        mShouldResize = false;
      } else {
        ShrinkToFit();
      }
      return NS_OK;
    }

    if (devPixelRatio != 1.0f) {
      ScaleToDevicePixelsTo(x, y);
      mShouldResize = false;
    }
  }

  return NS_OK;
}

nsresult
ImageDocument::CreateSyntheticDocument()
{
  
  nsresult rv = MediaDocument::CreateSyntheticDocument();
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  
  
  Element* head = GetHeadElement();
  NS_ENSURE_TRUE(head, NS_ERROR_FAILURE);

  nsCOMPtr<nsINodeInfo> nodeInfo;
  if (nsContentUtils::IsChildOfSameType(this)) {
    nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::style, nullptr,
                                             kNameSpaceID_XHTML,
                                             nsIDOMNode::ELEMENT_NODE);
    nsRefPtr<nsGenericHTMLElement> styleContent = NS_NewHTMLStyleElement(nodeInfo.forget());
    NS_ENSURE_TRUE(styleContent, NS_ERROR_OUT_OF_MEMORY);

    ErrorResult error;
    styleContent->SetTextContent(NS_LITERAL_STRING("img { display: block; }"),
                                 error);
    head->AppendChildTo(styleContent, false);
  }

  
  Element* body = GetBodyElement();
  if (!body) {
    NS_WARNING("no body on image document!");
    return NS_ERROR_FAILURE;
  }

  nodeInfo = mNodeInfoManager->GetNodeInfo(nsGkAtoms::img, nullptr,
                                           kNameSpaceID_XHTML,
                                           nsIDOMNode::ELEMENT_NODE);

  mImageContent = NS_NewHTMLImageElement(nodeInfo.forget());
  if (!mImageContent) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mImageContent);
  NS_ENSURE_TRUE(imageLoader, NS_ERROR_UNEXPECTED);

  nsAutoCString src;
  mDocumentURI->GetSpec(src);

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  NS_ConvertUTF8toUTF16 srcString(src);
  
  imageLoader->SetLoadingEnabled(false);
  mImageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::src, srcString, false);
  mImageContent->SetAttr(kNameSpaceID_None, nsGkAtoms::alt, srcString, false);

  body->AppendChildTo(mImageContent, false);
  imageLoader->SetLoadingEnabled(true);

  return NS_OK;
}

nsresult
ImageDocument::CheckOverflowing(bool changeState)
{
  



  {
    nsIPresShell *shell = GetShell();
    if (!shell) {
      return NS_OK;
    }

    nsPresContext *context = shell->GetPresContext();
    nsRect visibleArea = context->GetVisibleArea();

    mVisibleWidth = nsPresContext::AppUnitsToFloatCSSPixels(visibleArea.width);
    mVisibleHeight = nsPresContext::AppUnitsToFloatCSSPixels(visibleArea.height);
  }

  bool imageWasOverflowing = mImageIsOverflowing;
  mImageIsOverflowing =
    mImageWidth > mVisibleWidth || mImageHeight > mVisibleHeight;
  bool windowBecameBigEnough = imageWasOverflowing && !mImageIsOverflowing;

  if (changeState || mShouldResize || mFirstResize ||
      windowBecameBigEnough) {
    if (mImageIsOverflowing && (changeState || mShouldResize)) {
      ShrinkToFit();
    }
    else if (mImageIsResized || mFirstResize || windowBecameBigEnough) {
      RestoreImage();
    }
  }
  mFirstResize = false;

  return NS_OK;
}

void 
ImageDocument::UpdateTitleAndCharset()
{
  nsAutoCString typeStr;
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
  if (mImageIsResized || mImageIsScaledToDevicePixels) {
    nsAutoString ratioStr;
    ratioStr.AppendInt(NSToCoordFloor(GetCurrentRatio() * 100));

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
