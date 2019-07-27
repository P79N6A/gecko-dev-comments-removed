











#include "nsImageBoxFrame.h"
#include "nsGkAtoms.h"
#include "nsRenderingContext.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsBoxLayoutState.h"

#include "nsHTMLParts.h"
#include "nsString.h"
#include "nsLeafFrame.h"
#include "nsIPresShell.h"
#include "nsIDocument.h"
#include "nsImageMap.h"
#include "nsILinkHandler.h"
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsContainerFrame.h"
#include "prprf.h"
#include "nsCSSRendering.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsNameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsTransform2D.h"
#include "nsITheme.h"

#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsDisplayList.h"
#include "ImageLayers.h"
#include "ImageContainer.h"
#include "nsIContent.h"

#include "nsContentUtils.h"

#include "mozilla/BasicEvents.h"
#include "mozilla/EventDispatcher.h"

#define ONLOAD_CALLED_TOO_EARLY 1

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::image;
using namespace mozilla::layers;

class nsImageBoxFrameEvent : public nsRunnable
{
public:
  nsImageBoxFrameEvent(nsIContent *content, uint32_t message)
    : mContent(content), mMessage(message) {}

  NS_IMETHOD Run() override;

private:
  nsCOMPtr<nsIContent> mContent;
  uint32_t mMessage;
};

NS_IMETHODIMP
nsImageBoxFrameEvent::Run()
{
  nsIPresShell *pres_shell = mContent->OwnerDoc()->GetShell();
  if (!pres_shell) {
    return NS_OK;
  }

  nsRefPtr<nsPresContext> pres_context = pres_shell->GetPresContext();
  if (!pres_context) {
    return NS_OK;
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  WidgetEvent event(true, mMessage);

  event.mFlags.mBubbles = false;
  EventDispatcher::Dispatch(mContent, pres_context, &event, nullptr, &status);
  return NS_OK;
}









void
FireImageDOMEvent(nsIContent* aContent, uint32_t aMessage)
{
  NS_ASSERTION(aMessage == NS_LOAD || aMessage == NS_LOAD_ERROR,
               "invalid message");

  nsCOMPtr<nsIRunnable> event = new nsImageBoxFrameEvent(aContent, aMessage);
  if (NS_FAILED(NS_DispatchToCurrentThread(event)))
    NS_WARNING("failed to dispatch image event");
}






nsIFrame*
NS_NewImageBoxFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsImageBoxFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsImageBoxFrame)

nsresult
nsImageBoxFrame::AttributeChanged(int32_t aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t aModType)
{
  nsresult rv = nsLeafBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                                 aModType);

  if (aAttribute == nsGkAtoms::src) {
    UpdateImage();
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }
  else if (aAttribute == nsGkAtoms::validate)
    UpdateLoadFlags();

  return rv;
}

nsImageBoxFrame::nsImageBoxFrame(nsStyleContext* aContext):
  nsLeafBoxFrame(aContext),
  mIntrinsicSize(0,0),
  mLoadFlags(nsIRequest::LOAD_NORMAL),
  mRequestRegistered(false),
  mUseSrcAttr(false),
  mSuppressStyleCheck(false),
  mFireEventOnDecode(false)
{
  MarkIntrinsicISizesDirty();
}

nsImageBoxFrame::~nsImageBoxFrame()
{
}


 void
nsImageBoxFrame::MarkIntrinsicISizesDirty()
{
  SizeNeedsRecalc(mImageSize);
  nsLeafBoxFrame::MarkIntrinsicISizesDirty();
}

void
nsImageBoxFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mImageRequest) {
    nsLayoutUtils::DeregisterImageRequest(PresContext(), mImageRequest,
                                          &mRequestRegistered);

    
    mImageRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
  }

  if (mListener)
    reinterpret_cast<nsImageBoxListener*>(mListener.get())->SetFrame(nullptr); 

  nsLeafBoxFrame::DestroyFrom(aDestructRoot);
}


void
nsImageBoxFrame::Init(nsIContent*       aContent,
                      nsContainerFrame* aParent,
                      nsIFrame*         aPrevInFlow)
{
  if (!mListener) {
    nsRefPtr<nsImageBoxListener> listener = new nsImageBoxListener();
    listener->SetFrame(this);
    mListener = listener.forget();
  }

  mSuppressStyleCheck = true;
  nsLeafBoxFrame::Init(aContent, aParent, aPrevInFlow);
  mSuppressStyleCheck = false;

  UpdateLoadFlags();
  UpdateImage();
}

void
nsImageBoxFrame::UpdateImage()
{
  nsPresContext* presContext = PresContext();

  if (mImageRequest) {
    nsLayoutUtils::DeregisterImageRequest(presContext, mImageRequest,
                                          &mRequestRegistered);
    mImageRequest->CancelAndForgetObserver(NS_ERROR_FAILURE);
    mImageRequest = nullptr;
  }

  
  nsAutoString src;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src);
  mUseSrcAttr = !src.IsEmpty();
  if (mUseSrcAttr) {
    nsIDocument* doc = mContent->GetComposedDoc();
    if (!doc) {
      
      return;
    }
    nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
    nsCOMPtr<nsIURI> uri;
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                              src,
                                              doc,
                                              baseURI);

    if (uri && nsContentUtils::CanLoadImage(uri, mContent, doc,
                                            mContent->NodePrincipal())) {
      nsContentUtils::LoadImage(uri, doc, mContent->NodePrincipal(),
                                doc->GetDocumentURI(), doc->GetReferrerPolicy(),
                                mListener, mLoadFlags,
                                EmptyString(), getter_AddRefs(mImageRequest));

      if (mImageRequest) {
        nsLayoutUtils::RegisterImageRequestIfAnimated(presContext,
                                                      mImageRequest,
                                                      &mRequestRegistered);
      }
    }
  } else {
    
    
    uint8_t appearance = StyleDisplay()->mAppearance;
    if (!(appearance && nsBox::gTheme &&
          nsBox::gTheme->ThemeSupportsWidget(nullptr, this, appearance))) {
      
      imgRequestProxy *styleRequest = StyleList()->GetListStyleImage();
      if (styleRequest) {
        styleRequest->Clone(mListener, getter_AddRefs(mImageRequest));
      }
    }
  }

  if (!mImageRequest) {
    
    mIntrinsicSize.SizeTo(0, 0);
  } else {
    
    mImageRequest->StartDecoding();
    mImageRequest->LockImage();
  }
}

void
nsImageBoxFrame::UpdateLoadFlags()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::always, &nsGkAtoms::never, nullptr};
  switch (mContent->FindAttrValueIn(kNameSpaceID_None, nsGkAtoms::validate,
                                    strings, eCaseMatters)) {
    case 0:
      mLoadFlags = nsIRequest::VALIDATE_ALWAYS;
      break;
    case 1:
      mLoadFlags = nsIRequest::VALIDATE_NEVER|nsIRequest::LOAD_FROM_CACHE;
      break;
    default:
      mLoadFlags = nsIRequest::LOAD_NORMAL;
      break;
  }
}

void
nsImageBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  nsLeafBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);

  if ((0 == mRect.width) || (0 == mRect.height)) {
    
    
    
    return;
  }

  if (!IsVisibleForPainting(aBuilder))
    return;

  nsDisplayList list;
  list.AppendNewToTop(
    new (aBuilder) nsDisplayXULImage(aBuilder, this));

  CreateOwnLayerIfNeeded(aBuilder, &list);

  aLists.Content()->AppendToTop(&list);
}

DrawResult
nsImageBoxFrame::PaintImage(nsRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect, nsPoint aPt,
                            uint32_t aFlags)
{
  nsRect rect;
  GetClientRect(rect);

  rect += aPt;

  if (!mImageRequest) {
    
    return DrawResult::SUCCESS;
  }

  
  
  nsRect dirty;
  if (!dirty.IntersectRect(aDirtyRect, rect)) {
    return DrawResult::TEMPORARY_ERROR;
  }

  nsCOMPtr<imgIContainer> imgCon;
  mImageRequest->GetImage(getter_AddRefs(imgCon));

  if (imgCon) {
    bool hasSubRect = !mUseSrcAttr && (mSubRect.width > 0 || mSubRect.height > 0);
    return
      nsLayoutUtils::DrawSingleImage(*aRenderingContext.ThebesContext(),
        PresContext(), imgCon,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        rect, dirty, nullptr, aFlags, nullptr,
        hasSubRect ? &mSubRect : nullptr);
  }

  return DrawResult::NOT_READY;
}

void nsDisplayXULImage::Paint(nsDisplayListBuilder* aBuilder,
                              nsRenderingContext* aCtx)
{
  uint32_t flags = imgIContainer::FLAG_NONE;
  if (aBuilder->ShouldSyncDecodeImages())
    flags |= imgIContainer::FLAG_SYNC_DECODE;
  if (aBuilder->IsPaintingToWindow())
    flags |= imgIContainer::FLAG_HIGH_QUALITY_SCALING;

  DrawResult result = static_cast<nsImageBoxFrame*>(mFrame)->
    PaintImage(*aCtx, mVisibleRect, ToReferenceFrame(), flags);

  nsDisplayItemGenericImageGeometry::UpdateDrawResult(this, result);
}

nsDisplayItemGeometry*
nsDisplayXULImage::AllocateGeometry(nsDisplayListBuilder* aBuilder)
{
  return new nsDisplayItemGenericImageGeometry(this, aBuilder);
}

void
nsDisplayXULImage::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                             const nsDisplayItemGeometry* aGeometry,
                                             nsRegion* aInvalidRegion)
{
  auto boxFrame = static_cast<nsImageBoxFrame*>(mFrame);
  auto geometry =
    static_cast<const nsDisplayItemGenericImageGeometry*>(aGeometry);

  if (aBuilder->ShouldSyncDecodeImages() &&
      boxFrame->mImageRequest &&
      geometry->ShouldInvalidateToSyncDecodeImages()) {
      bool snap;
      aInvalidRegion->Or(*aInvalidRegion, GetBounds(aBuilder, &snap));
  }

  nsDisplayImageContainer::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
}

void
nsDisplayXULImage::ConfigureLayer(ImageLayer* aLayer,
                                  const ContainerLayerParameters& aParameters)
{
  aLayer->SetFilter(nsLayoutUtils::GetGraphicsFilterForFrame(mFrame));

  nsImageBoxFrame* imageFrame = static_cast<nsImageBoxFrame*>(mFrame);

  nsRect clientRect;
  imageFrame->GetClientRect(clientRect);

  const int32_t factor = mFrame->PresContext()->AppUnitsPerDevPixel();
  const LayoutDeviceRect destRect =
    LayoutDeviceRect::FromAppUnits(clientRect + ToReferenceFrame(), factor);

  nsCOMPtr<imgIContainer> imgCon;
  imageFrame->mImageRequest->GetImage(getter_AddRefs(imgCon));
  int32_t imageWidth;
  int32_t imageHeight;
  imgCon->GetWidth(&imageWidth);
  imgCon->GetHeight(&imageHeight);

  NS_ASSERTION(imageWidth != 0 && imageHeight != 0, "Invalid image size!");
  if (imageWidth > 0 && imageHeight > 0) {
    
    
    nsDisplayItemGenericImageGeometry::UpdateDrawResult(this,
                                                        DrawResult::SUCCESS);
  }

  
  
  
  
  MOZ_ASSERT(aParameters.Offset() == LayerIntPoint(0,0));

  const LayoutDevicePoint p = destRect.TopLeft();
  Matrix transform = Matrix::Translation(p.x, p.y);
  transform.PreScale(destRect.Width() / imageWidth,
                     destRect.Height() / imageHeight);
  aLayer->SetBaseTransform(gfx::Matrix4x4::From2D(transform));
}

already_AddRefed<ImageContainer>
nsDisplayXULImage::GetContainer(LayerManager* aManager,
                                nsDisplayListBuilder* aBuilder)
{
  uint32_t flags = aBuilder->ShouldSyncDecodeImages()
                 ? imgIContainer::FLAG_SYNC_DECODE
                 : imgIContainer::FLAG_NONE;

  return static_cast<nsImageBoxFrame*>(mFrame)->GetContainer(aManager, flags);
}

already_AddRefed<ImageContainer>
nsImageBoxFrame::GetContainer(LayerManager* aManager, uint32_t aFlags)
{
  bool hasSubRect = !mUseSrcAttr && (mSubRect.width > 0 || mSubRect.height > 0);
  if (hasSubRect || !mImageRequest) {
    return nullptr;
  }

  nsCOMPtr<imgIContainer> imgCon;
  mImageRequest->GetImage(getter_AddRefs(imgCon));
  if (!imgCon) {
    return nullptr;
  }
  
  return imgCon->GetImageContainer(aManager, aFlags);
}







 void
nsImageBoxFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsLeafBoxFrame::DidSetStyleContext(aOldStyleContext);

  
  const nsStyleList* myList = StyleList();
  mSubRect = myList->mImageRegion; 

  if (mUseSrcAttr || mSuppressStyleCheck)
    return; 

  
  const nsStyleDisplay* disp = StyleDisplay();
  if (disp->mAppearance && nsBox::gTheme &&
      nsBox::gTheme->ThemeSupportsWidget(nullptr, this, disp->mAppearance))
    return;

  
  nsCOMPtr<nsIURI> oldURI, newURI;
  if (mImageRequest)
    mImageRequest->GetURI(getter_AddRefs(oldURI));
  if (myList->GetListStyleImage())
    myList->GetListStyleImage()->GetURI(getter_AddRefs(newURI));
  bool equal;
  if (newURI == oldURI ||   
      (newURI && oldURI &&
       NS_SUCCEEDED(newURI->Equals(oldURI, &equal)) && equal))
    return;

  UpdateImage();
} 

void
nsImageBoxFrame::GetImageSize()
{
  if (mIntrinsicSize.width > 0 && mIntrinsicSize.height > 0) {
    mImageSize.width = mIntrinsicSize.width;
    mImageSize.height = mIntrinsicSize.height;
  } else {
    mImageSize.width = 0;
    mImageSize.height = 0;
  }
}




nsSize
nsImageBoxFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize size(0,0);
  DISPLAY_PREF_SIZE(this, size);
  if (DoesNeedRecalc(mImageSize))
     GetImageSize();

  if (!mUseSrcAttr && (mSubRect.width > 0 || mSubRect.height > 0))
    size = mSubRect.Size();
  else
    size = mImageSize;

  nsSize intrinsicSize = size;

  nsMargin borderPadding(0,0,0,0);
  GetBorderAndPadding(borderPadding);
  size.width += borderPadding.LeftRight();
  size.height += borderPadding.TopBottom();

  bool widthSet, heightSet;
  nsIFrame::AddCSSPrefSize(this, size, widthSet, heightSet);
  NS_ASSERTION(size.width != NS_INTRINSICSIZE && size.height != NS_INTRINSICSIZE,
               "non-intrinsic size expected");

  nsSize minSize = GetMinSize(aState);
  nsSize maxSize = GetMaxSize(aState);

  if (!widthSet && !heightSet) {
    if (minSize.width != NS_INTRINSICSIZE)
      minSize.width -= borderPadding.LeftRight();
    if (minSize.height != NS_INTRINSICSIZE)
      minSize.height -= borderPadding.TopBottom();
    if (maxSize.width != NS_INTRINSICSIZE)
      maxSize.width -= borderPadding.LeftRight();
    if (maxSize.height != NS_INTRINSICSIZE)
      maxSize.height -= borderPadding.TopBottom();

    size = nsLayoutUtils::ComputeAutoSizeWithIntrinsicDimensions(minSize.width, minSize.height,
                                                                 maxSize.width, maxSize.height,
                                                                 intrinsicSize.width, intrinsicSize.height);
    NS_ASSERTION(size.width != NS_INTRINSICSIZE && size.height != NS_INTRINSICSIZE,
                 "non-intrinsic size expected");
    size.width += borderPadding.LeftRight();
    size.height += borderPadding.TopBottom();
    return size;
  }

  if (!widthSet) {
    if (intrinsicSize.height > 0) {
      
      
      nscoord height = size.height - borderPadding.TopBottom();
      size.width = nscoord(int64_t(height) * int64_t(intrinsicSize.width) /
                           int64_t(intrinsicSize.height));
    }
    else {
      size.width = intrinsicSize.width;
    }

    size.width += borderPadding.LeftRight();
  }
  else if (!heightSet) {
    if (intrinsicSize.width > 0) {
      nscoord width = size.width - borderPadding.LeftRight();
      size.height = nscoord(int64_t(width) * int64_t(intrinsicSize.height) /
                            int64_t(intrinsicSize.width));
    }
    else {
      size.height = intrinsicSize.height;
    }

    size.height += borderPadding.TopBottom();
  }

  return BoundsCheck(minSize, size, maxSize);
}

nsSize
nsImageBoxFrame::GetMinSize(nsBoxLayoutState& aState)
{
  
  nsSize size(0,0);
  DISPLAY_MIN_SIZE(this, size);
  AddBorderAndPadding(size);
  bool widthSet, heightSet;
  nsIFrame::AddCSSMinSize(aState, this, size, widthSet, heightSet);
  return size;
}

nscoord
nsImageBoxFrame::GetBoxAscent(nsBoxLayoutState& aState)
{
  return GetPrefSize(aState).height;
}

nsIAtom*
nsImageBoxFrame::GetType() const
{
  return nsGkAtoms::imageBoxFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsImageBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ImageBox"), aResult);
}
#endif

nsresult
nsImageBoxFrame::Notify(imgIRequest* aRequest,
                        int32_t aType,
                        const nsIntRect* aData)
{
  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    nsCOMPtr<imgIContainer> image;
    aRequest->GetImage(getter_AddRefs(image));
    return OnSizeAvailable(aRequest, image);
  }

  if (aType == imgINotificationObserver::DECODE_COMPLETE) {
    return OnDecodeComplete(aRequest);
  }

  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    uint32_t imgStatus;
    aRequest->GetImageStatus(&imgStatus);
    nsresult status =
        imgStatus & imgIRequest::STATUS_ERROR ? NS_ERROR_FAILURE : NS_OK;
    return OnLoadComplete(aRequest, status);
  }

  if (aType == imgINotificationObserver::IS_ANIMATED) {
    return OnImageIsAnimated(aRequest);
  }

  if (aType == imgINotificationObserver::FRAME_UPDATE) {
    return OnFrameUpdate(aRequest);
  }

  return NS_OK;
}

nsresult
nsImageBoxFrame::OnSizeAvailable(imgIRequest* aRequest, imgIContainer* aImage)
{
  NS_ENSURE_ARG_POINTER(aImage);

  
  
  
  aRequest->IncrementAnimationConsumers();

  nscoord w, h;
  aImage->GetWidth(&w);
  aImage->GetHeight(&h);

  mIntrinsicSize.SizeTo(nsPresContext::CSSPixelsToAppUnits(w),
                        nsPresContext::CSSPixelsToAppUnits(h));

  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }

  return NS_OK;
}

nsresult
nsImageBoxFrame::OnDecodeComplete(imgIRequest* aRequest)
{
  if (mFireEventOnDecode) {
    mFireEventOnDecode = false;

    uint32_t reqStatus;
    aRequest->GetImageStatus(&reqStatus);
    if (!(reqStatus & imgIRequest::STATUS_ERROR)) {
      FireImageDOMEvent(mContent, NS_LOAD);
    } else {
      
      mIntrinsicSize.SizeTo(0, 0);
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
      FireImageDOMEvent(mContent, NS_LOAD_ERROR);
    }
  }

  nsBoxLayoutState state(PresContext());
  this->Redraw(state);

  return NS_OK;
}

nsresult
nsImageBoxFrame::OnLoadComplete(imgIRequest* aRequest, nsresult aStatus)
{
  uint32_t reqStatus;
  aRequest->GetImageStatus(&reqStatus);

  
  
  
  if (NS_SUCCEEDED(aStatus) && !(reqStatus & imgIRequest::STATUS_ERROR) &&
      (reqStatus & imgIRequest::STATUS_DECODE_STARTED) &&
      !(reqStatus & imgIRequest::STATUS_DECODE_COMPLETE)) {
    mFireEventOnDecode = true;
  } else {
    if (NS_SUCCEEDED(aStatus)) {
      
      FireImageDOMEvent(mContent, NS_LOAD);
    } else {
      
      mIntrinsicSize.SizeTo(0, 0);
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
      FireImageDOMEvent(mContent, NS_LOAD_ERROR);
    }
  }

  return NS_OK;
}

nsresult
nsImageBoxFrame::OnImageIsAnimated(imgIRequest* aRequest)
{
  
  nsLayoutUtils::RegisterImageRequest(PresContext(), aRequest,
                                      &mRequestRegistered);

  return NS_OK;
}

nsresult
nsImageBoxFrame::OnFrameUpdate(imgIRequest* aRequest)
{
  if ((0 == mRect.width) || (0 == mRect.height)) {
    return NS_OK;
  }
 
  InvalidateLayer(nsDisplayItem::TYPE_XUL_IMAGE);

  return NS_OK;
}

NS_IMPL_ISUPPORTS(nsImageBoxListener, imgINotificationObserver, imgIOnloadBlocker)

nsImageBoxListener::nsImageBoxListener()
{
}

nsImageBoxListener::~nsImageBoxListener()
{
}

NS_IMETHODIMP
nsImageBoxListener::Notify(imgIRequest *request, int32_t aType, const nsIntRect* aData)
{
  if (!mFrame)
    return NS_OK;

  return mFrame->Notify(request, aType, aData);
}


NS_IMETHODIMP
nsImageBoxListener::BlockOnload(imgIRequest *aRequest)
{
  if (mFrame && mFrame->GetContent() && mFrame->GetContent()->GetCurrentDoc()) {
    mFrame->GetContent()->GetCurrentDoc()->BlockOnload();
  }

  return NS_OK;
}


NS_IMETHODIMP
nsImageBoxListener::UnblockOnload(imgIRequest *aRequest)
{
  if (mFrame && mFrame->GetContent() && mFrame->GetContent()->GetCurrentDoc()) {
    mFrame->GetContent()->GetCurrentDoc()->UnblockOnload(false);
  }

  return NS_OK;
}
