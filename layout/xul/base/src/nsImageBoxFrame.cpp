











































#include "nsImageBoxFrame.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsGkAtoms.h"
#include "nsStyleContext.h"
#include "nsStyleConsts.h"
#include "nsCOMPtr.h"
#include "nsPresContext.h"
#include "nsBoxLayoutState.h"

#include "nsHTMLParts.h"
#include "nsString.h"
#include "nsLeafFrame.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsIImage.h"
#include "nsIWidget.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsStyleConsts.h"
#include "nsImageMap.h"
#include "nsILinkHandler.h"
#include "nsIURL.h"
#include "nsILoadGroup.h"
#include "nsHTMLContainerFrame.h"
#include "prprf.h"
#include "nsIFontMetrics.h"
#include "nsCSSRendering.h"
#include "nsIDOMHTMLImageElement.h"
#include "nsIDeviceContext.h"
#include "nsINameSpaceManager.h"
#include "nsTextFragment.h"
#include "nsIDOMHTMLMapElement.h"
#include "nsBoxLayoutState.h"
#include "nsIDOMDocument.h"
#include "nsTransform2D.h"
#include "nsITheme.h"

#include "nsIServiceManager.h"
#include "nsIURI.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsGUIEvent.h"
#include "nsEventDispatcher.h"
#include "nsDisplayList.h"

#include "nsContentUtils.h"

#define ONLOAD_CALLED_TOO_EARLY 1

class nsImageBoxFrameEvent : public nsRunnable
{
public:
  nsImageBoxFrameEvent(nsIContent *content, PRUint32 message)
    : mContent(content), mMessage(message) {}

  NS_IMETHOD Run();

private:
  nsCOMPtr<nsIContent> mContent;
  PRUint32 mMessage;
};

NS_IMETHODIMP
nsImageBoxFrameEvent::Run()
{
  nsIDocument* doc = mContent->GetOwnerDoc();
  if (!doc) {
    return NS_OK;
  }

  nsIPresShell *pres_shell = doc->GetShellAt(0);
  if (!pres_shell) {
    return NS_OK;
  }

  nsCOMPtr<nsPresContext> pres_context = pres_shell->GetPresContext();
  if (!pres_context) {
    return NS_OK;
  }

  nsEventStatus status = nsEventStatus_eIgnore;
  nsEvent event(PR_TRUE, mMessage);

  event.flags |= NS_EVENT_FLAG_CANT_BUBBLE;
  nsEventDispatcher::Dispatch(mContent, pres_context, &event, nsnull, &status);
  return NS_OK;
}









void
FireImageDOMEvent(nsIContent* aContent, PRUint32 aMessage)
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
  return new (aPresShell) nsImageBoxFrame (aPresShell, aContext);
} 

NS_IMETHODIMP
nsImageBoxFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32 aModType)
{
  nsresult rv = nsLeafBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                                 aModType);

  if (aAttribute == nsGkAtoms::src) {
    UpdateImage();
    AddStateBits(NS_FRAME_IS_DIRTY);
    GetPresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange);
  }
  else if (aAttribute == nsGkAtoms::validate)
    UpdateLoadFlags();

  return rv;
}

nsImageBoxFrame::nsImageBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsLeafBoxFrame(aShell, aContext),
  mUseSrcAttr(PR_FALSE),
  mSuppressStyleCheck(PR_FALSE),
  mIntrinsicSize(0,0),
  mLoadFlags(nsIRequest::LOAD_NORMAL)
{
  MarkIntrinsicWidthsDirty();
}

nsImageBoxFrame::~nsImageBoxFrame()
{
}


 void
nsImageBoxFrame::MarkIntrinsicWidthsDirty()
{
  SizeNeedsRecalc(mImageSize);
  nsLeafBoxFrame::MarkIntrinsicWidthsDirty();
}

void
nsImageBoxFrame::Destroy()
{
  
  if (mImageRequest)
    mImageRequest->Cancel(NS_ERROR_FAILURE);

  if (mListener)
    NS_REINTERPRET_CAST(nsImageBoxListener*, mListener.get())->SetFrame(nsnull); 

  nsLeafBoxFrame::Destroy();
}


NS_IMETHODIMP
nsImageBoxFrame::Init(nsIContent*      aContent,
                      nsIFrame*        aParent,
                      nsIFrame*        aPrevInFlow)
{
  if (!mListener) {
    nsImageBoxListener *listener;
    NS_NEWXPCOM(listener, nsImageBoxListener);
    NS_ADDREF(listener);
    listener->SetFrame(this);
    listener->QueryInterface(NS_GET_IID(imgIDecoderObserver), getter_AddRefs(mListener));
    NS_RELEASE(listener);
  }

  mSuppressStyleCheck = PR_TRUE;
  nsresult rv = nsLeafBoxFrame::Init(aContent, aParent, aPrevInFlow);
  mSuppressStyleCheck = PR_FALSE;

  UpdateLoadFlags();
  UpdateImage();

  return rv;
}

void
nsImageBoxFrame::UpdateImage()
{
  if (mImageRequest) {
    mImageRequest->Cancel(NS_ERROR_FAILURE);
    mImageRequest = nsnull;
  }

  
  nsAutoString src;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::src, src);
  mUseSrcAttr = !src.IsEmpty();
  if (mUseSrcAttr) {
    nsIDocument* doc = mContent->GetDocument();
    if (!doc) {
      
      return;
    }
    nsCOMPtr<nsIURI> baseURI = mContent->GetBaseURI();
    nsCOMPtr<nsIURI> uri;
    nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(uri),
                                              src,
                                              doc,
                                              baseURI);

    if (uri && nsContentUtils::CanLoadImage(uri, mContent, doc)) {
      nsContentUtils::LoadImage(uri, doc, doc->GetDocumentURI(),
                                mListener, mLoadFlags,
                                getter_AddRefs(mImageRequest));
    }
  } else {
    
    
    PRUint8 appearance = GetStyleDisplay()->mAppearance;
    if (!(appearance && nsBox::gTheme && 
          nsBox::gTheme->ThemeSupportsWidget(nsnull, this, appearance))) {
      
      imgIRequest *styleRequest = GetStyleList()->mListStyleImage;
      if (styleRequest) {
        styleRequest->Clone(mListener, getter_AddRefs(mImageRequest));
      }
    }
  }

  if (!mImageRequest) {
    
    mIntrinsicSize.SizeTo(0, 0);
  }
}

void
nsImageBoxFrame::UpdateLoadFlags()
{
  static nsIContent::AttrValuesArray strings[] =
    {&nsGkAtoms::always, &nsGkAtoms::never, nsnull};
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

class nsDisplayXULImage : public nsDisplayItem {
public:
  nsDisplayXULImage(nsImageBoxFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayXULImage);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayXULImage() {
    MOZ_COUNT_DTOR(nsDisplayXULImage);
  }
#endif

  
  
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("XULImage")
};

void nsDisplayXULImage::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect)
{
  NS_STATIC_CAST(nsImageBoxFrame*, mFrame)->
    PaintImage(*aCtx, aDirtyRect, aBuilder->ToReferenceFrame(mFrame));
}

NS_IMETHODIMP
nsImageBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{       
  nsresult rv = nsLeafBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  if ((0 == mRect.width) || (0 == mRect.height)) {
    
    
    
    return NS_OK;
  }

  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;

  return aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplayXULImage(this));
}

void
nsImageBoxFrame::PaintImage(nsIRenderingContext& aRenderingContext,
                            const nsRect& aDirtyRect, nsPoint aPt)
{
  nsRect rect;
  GetClientRect(rect);

  rect += aPt;

  if (!mImageRequest)
    return;

  
  nsRect dirty;
  if (!dirty.IntersectRect(aDirtyRect, rect))
    return;

  nsCOMPtr<imgIContainer> imgCon;
  mImageRequest->GetImage(getter_AddRefs(imgCon));

  if (imgCon) {
    PRBool hasSubRect = !mUseSrcAttr && (mSubRect.width > 0 || mSubRect.height > 0);
    nsLayoutUtils::DrawImage(&aRenderingContext, imgCon,
                             rect, dirty, hasSubRect ? &mSubRect : nsnull);
  }
}







NS_IMETHODIMP
nsImageBoxFrame::DidSetStyleContext()
{
  
  const nsStyleList* myList = GetStyleList();
  mSubRect = myList->mImageRegion; 

  if (mUseSrcAttr || mSuppressStyleCheck)
    return NS_OK; 

  
  const nsStyleDisplay* disp = GetStyleDisplay();
  if (disp->mAppearance && nsBox::gTheme && 
      nsBox::gTheme->ThemeSupportsWidget(nsnull, this, disp->mAppearance))
    return NS_OK;

  
  nsCOMPtr<nsIURI> oldURI, newURI;
  if (mImageRequest)
    mImageRequest->GetURI(getter_AddRefs(oldURI));
  if (myList->mListStyleImage)
    myList->mListStyleImage->GetURI(getter_AddRefs(newURI));
  PRBool equal;
  if (newURI == oldURI ||   
      (newURI && oldURI &&
       NS_SUCCEEDED(newURI->Equals(oldURI, &equal)) && equal))
    return NS_OK;

  UpdateImage();
  return NS_OK;
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
    size = nsSize(mSubRect.width, mSubRect.height);
  else
    size = mImageSize;
  AddBorderAndPadding(size);
  nsIBox::AddCSSPrefSize(aState, this, size);

  nsSize minSize = GetMinSize(aState);
  nsSize maxSize = GetMaxSize(aState);
  BoundsCheck(minSize, size, maxSize);

  return size;
}

nsSize
nsImageBoxFrame::GetMinSize(nsBoxLayoutState& aState)
{
  
  nsSize size(0,0);
  DISPLAY_MIN_SIZE(this, size);
  AddBorderAndPadding(size);
  nsIBox::AddCSSMinSize(aState, this, size);
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

#ifdef DEBUG
NS_IMETHODIMP
nsImageBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ImageBox"), aResult);
}
#endif


NS_IMETHODIMP nsImageBoxFrame::OnStartContainer(imgIRequest *request,
                                                imgIContainer *image)
{
  NS_ENSURE_ARG_POINTER(image);

  
  image->StartAnimation();

  nscoord w, h;
  image->GetWidth(&w);
  image->GetHeight(&h);

  mIntrinsicSize.SizeTo(nsPresContext::CSSPixelsToAppUnits(w),
                        nsPresContext::CSSPixelsToAppUnits(h));

  AddStateBits(NS_FRAME_IS_DIRTY);
  GetPresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eStyleChange);

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopContainer(imgIRequest *request,
                                               imgIContainer *image)
{
  nsBoxLayoutState state(GetPresContext());
  this->Redraw(state);

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::OnStopDecode(imgIRequest *request,
                                            nsresult aStatus,
                                            const PRUnichar *statusArg)
{
  if (NS_SUCCEEDED(aStatus))
    
    FireImageDOMEvent(mContent, NS_LOAD);
  else {
    
    mIntrinsicSize.SizeTo(0, 0);
    AddStateBits(NS_FRAME_IS_DIRTY);
    GetPresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange);
    FireImageDOMEvent(mContent, NS_LOAD_ERROR);
  }

  return NS_OK;
}

NS_IMETHODIMP nsImageBoxFrame::FrameChanged(imgIContainer *container,
                                            gfxIImageFrame *newframe,
                                            nsRect * dirtyRect)
{
  nsBoxLayoutState state(GetPresContext());
  this->Redraw(state);

  return NS_OK;
}

NS_IMPL_ISUPPORTS2(nsImageBoxListener, imgIDecoderObserver, imgIContainerObserver)

nsImageBoxListener::nsImageBoxListener()
{
}

nsImageBoxListener::~nsImageBoxListener()
{
}

NS_IMETHODIMP nsImageBoxListener::OnStartContainer(imgIRequest *request,
                                                   imgIContainer *image)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStartContainer(request, image);
}

NS_IMETHODIMP nsImageBoxListener::OnStopContainer(imgIRequest *request,
                                                  imgIContainer *image)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStopContainer(request, image);
}

NS_IMETHODIMP nsImageBoxListener::OnStopDecode(imgIRequest *request,
                                               nsresult status,
                                               const PRUnichar *statusArg)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->OnStopDecode(request, status, statusArg);
}

NS_IMETHODIMP nsImageBoxListener::FrameChanged(imgIContainer *container,
                                               gfxIImageFrame *newframe,
                                               nsRect * dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  return mFrame->FrameChanged(container, newframe, dirtyRect);
}

