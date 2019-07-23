





































#include "nsScrollPortView.h"
#include "nsIWidget.h"
#include "nsUnitConversion.h"
#include "nsIDeviceContext.h"
#include "nsGUIEvent.h"
#include "nsWidgetsCID.h"
#include "nsViewsCID.h"
#include "nsIScrollableView.h"
#include "nsILookAndFeel.h"
#include "nsISupportsArray.h"
#include "nsIScrollPositionListener.h"
#include "nsIRegion.h"
#include "nsViewManager.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"

#include <math.h>

static NS_DEFINE_IID(kWidgetCID, NS_CHILD_CID);

#define SMOOTH_SCROLL_MSECS_PER_FRAME 10
#define SMOOTH_SCROLL_FRAMES    10

#define SMOOTH_SCROLL_PREF_NAME "general.smoothScroll"

class SmoothScroll {
public:
  SmoothScroll() {}
  ~SmoothScroll() {
    if (mScrollAnimationTimer) mScrollAnimationTimer->Cancel();
  }

  nsCOMPtr<nsITimer> mScrollAnimationTimer;
  PRInt32 mVelocities[SMOOTH_SCROLL_FRAMES*2];
  PRInt32 mFrameIndex;
  nscoord mDestinationX;
  nscoord mDestinationY;
};

nsScrollPortView::nsScrollPortView(nsViewManager* aViewManager)
  : nsView(aViewManager)
{
  mOffsetX = mOffsetY = 0;
  mOffsetXpx = mOffsetYpx = 0;
  nsCOMPtr<nsIDeviceContext> dev;
  mViewManager->GetDeviceContext(*getter_AddRefs(dev));
  mLineHeight = dev->AppUnitsPerInch() / 6; 

  mListeners = nsnull;
  mSmoothScroll = nsnull;
}

nsScrollPortView::~nsScrollPortView()
{    
  if (nsnull != mListeners) {
    mListeners->Clear();
    NS_RELEASE(mListeners);
  }

  if (nsnull != mViewManager) {
     nsIScrollableView* scrollingView;
     mViewManager->GetRootScrollableView(&scrollingView);
     if ((nsnull != scrollingView) && (this == scrollingView)) {
       mViewManager->SetRootScrollableView(nsnull);
     }
  }

  delete mSmoothScroll;
}

nsresult nsScrollPortView::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  if (nsnull == aInstancePtr) {
    return NS_ERROR_NULL_POINTER;
  }
  *aInstancePtr = nsnull;
  if (aIID.Equals(NS_GET_IID(nsIScrollableView))) {
    *aInstancePtr = (void*)(nsIScrollableView*)this;
    return NS_OK;
  }

  return nsView::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP_(nsIView*) nsScrollPortView::View()
{
  return this;
}

NS_IMETHODIMP nsScrollPortView::AddScrollPositionListener(nsIScrollPositionListener* aListener)
{
  if (nsnull == mListeners) {
    nsresult rv = NS_NewISupportsArray(&mListeners);
    if (NS_FAILED(rv))
      return rv;
  }
  return mListeners->AppendElement(aListener);
}

NS_IMETHODIMP nsScrollPortView::RemoveScrollPositionListener(nsIScrollPositionListener* aListener)
{
  if (nsnull != mListeners) {
    return mListeners->RemoveElement(aListener);
  }
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP nsScrollPortView::CreateScrollControls(nsNativeWidget aNative)
{
  nsWidgetInitData  initData;
  initData.clipChildren = PR_TRUE;
  initData.clipSiblings = PR_TRUE;

  CreateWidget(kWidgetCID, &initData,
               mWindow ? nsnull : aNative);
  
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetWidget(nsIWidget *aWidget)
{
  if (nsnull != aWidget) {
    NS_ASSERTION(PR_FALSE, "please don't try and set a widget here");
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetContainerSize(nscoord *aWidth, nscoord *aHeight) const
{
  if (!aWidth || !aHeight)
    return NS_ERROR_NULL_POINTER;

  *aWidth  = 0;
  *aHeight = 0;

  nsView *scrolledView = GetScrolledView();

  if (!scrolledView)
    return NS_ERROR_FAILURE;

  nsSize sz;
  scrolledView->GetDimensions(sz);
  *aWidth = sz.width;
  *aHeight = sz.height;
  return NS_OK;
}

static void ComputeVelocities(PRInt32 aCurVelocity, nscoord aCurPos, nscoord aDstPos,
                              PRInt32* aVelocities, PRInt32 aP2A) {
  
  
  
  
  aCurPos = NSAppUnitsToIntPixels(aCurPos, aP2A);
  aDstPos = NSAppUnitsToIntPixels(aDstPos, aP2A);

  PRInt32 i;
  PRInt32 direction = (aCurPos < aDstPos ? 1 : -1);
  PRInt32 absDelta = (aDstPos - aCurPos)*direction;
  PRInt32 baseVelocity = absDelta/SMOOTH_SCROLL_FRAMES;

  for (i = 0; i < SMOOTH_SCROLL_FRAMES; i++) {
    aVelocities[i*2] = baseVelocity;
  }
  nscoord total = baseVelocity*SMOOTH_SCROLL_FRAMES;
  for (i = 0; i < SMOOTH_SCROLL_FRAMES; i++) {
    if (total < absDelta) {
      aVelocities[i*2]++;
      total++;
    }
  }
  NS_ASSERTION(total == absDelta, "Invalid velocity sum");

  PRInt32 scale = NSIntPixelsToAppUnits(direction, aP2A);
  for (i = 0; i < SMOOTH_SCROLL_FRAMES; i++) {
    aVelocities[i*2] *= scale;
  }
}
  
static nsresult ClampScrollValues(nscoord& aX, nscoord& aY, nsScrollPortView* aThis) {
  
  nsView* scrolledView = aThis->GetScrolledView();
  if (!scrolledView) return NS_ERROR_FAILURE;
  
  nsRect scrolledRect;
  scrolledView->GetDimensions(scrolledRect);
  
  nsSize portSize;
  aThis->GetDimensions(portSize);
  
  nscoord maxX = scrolledRect.XMost() - portSize.width;
  nscoord maxY = scrolledRect.YMost() - portSize.height;
  
  if (aX > maxX)
    aX = maxX;
  
  if (aY > maxY)
    aY = maxY;
  
  if (aX < scrolledRect.x)
    aX = scrolledRect.x;
  
  if (aY < scrolledRect.y)
    aY = scrolledRect.y;
  
  return NS_OK;
}
  




NS_IMETHODIMP nsScrollPortView::ScrollTo(nscoord aDestinationX, nscoord aDestinationY,
                                         PRUint32 aUpdateFlags)
{
  
  if (aDestinationX == mOffsetX && aDestinationY == mOffsetY) {
    
    delete mSmoothScroll;
    mSmoothScroll = nsnull;
    return NS_OK;
  }
  
  if ((aUpdateFlags & NS_VMREFRESH_SMOOTHSCROLL) == 0
      || !IsSmoothScrollingEnabled()) {
    
    
    delete mSmoothScroll;
    mSmoothScroll = nsnull;
    return ScrollToImpl(aDestinationX, aDestinationY, aUpdateFlags);
  }

  PRInt32 currentVelocityX;
  PRInt32 currentVelocityY;

  if (mSmoothScroll) {
    currentVelocityX = mSmoothScroll->mVelocities[mSmoothScroll->mFrameIndex*2];
    currentVelocityY = mSmoothScroll->mVelocities[mSmoothScroll->mFrameIndex*2 + 1];
  } else {
    currentVelocityX = 0;
    currentVelocityY = 0;

    mSmoothScroll = new SmoothScroll;
    if (mSmoothScroll) {
      mSmoothScroll->mScrollAnimationTimer = do_CreateInstance("@mozilla.org/timer;1");
      if (!mSmoothScroll->mScrollAnimationTimer) {
        delete mSmoothScroll;
        mSmoothScroll = nsnull;
      }
    }
    if (!mSmoothScroll) {
      
      return ScrollToImpl(aDestinationX, aDestinationY, aUpdateFlags);
    }
    mSmoothScroll->mScrollAnimationTimer->InitWithFuncCallback(
      SmoothScrollAnimationCallback, this, SMOOTH_SCROLL_MSECS_PER_FRAME,
      nsITimer::TYPE_REPEATING_PRECISE);
    mSmoothScroll->mDestinationX = mOffsetX;
    mSmoothScroll->mDestinationY = mOffsetY;
  }

  
  
  
  
  
  
  mSmoothScroll->mDestinationX += aDestinationX - mOffsetX;
  mSmoothScroll->mDestinationY += aDestinationY - mOffsetY;
  mSmoothScroll->mFrameIndex = 0;
  ClampScrollValues(mSmoothScroll->mDestinationX, mSmoothScroll->mDestinationY, this);

  nsCOMPtr<nsIDeviceContext> dev;
  mViewManager->GetDeviceContext(*getter_AddRefs(dev));
  PRInt32 p2a = dev->AppUnitsPerDevPixel();

  
  ComputeVelocities(currentVelocityX, mOffsetX,
                    mSmoothScroll->mDestinationX, mSmoothScroll->mVelocities,
                    p2a);
  ComputeVelocities(currentVelocityY, mOffsetY,
                    mSmoothScroll->mDestinationY, mSmoothScroll->mVelocities + 1,
                    p2a);

  return NS_OK;
}

static void AdjustChildWidgets(nsView *aView,
  nsPoint aWidgetToParentViewOrigin, PRInt32 aP2A, PRBool aInvalidate)
{
  if (aView->HasWidget()) {
    nsIWidget* widget = aView->GetWidget();
    nsWindowType type;
    widget->GetWindowType(type);
    if (type != eWindowType_popup) {
      nsRect bounds = aView->GetBounds();
      nsPoint widgetOrigin = aWidgetToParentViewOrigin
        + nsPoint(bounds.x, bounds.y);
      widget->Move(NSAppUnitsToIntPixels(widgetOrigin.x, aP2A),
                   NSAppUnitsToIntPixels(widgetOrigin.y, aP2A));
      if (aInvalidate) {
        
        
        
        
        
        
        widget->Show(PR_FALSE);
        widget->Show(PR_TRUE);
      }
    }
  } else {
    
    
    nsPoint widgetToViewOrigin = aWidgetToParentViewOrigin
      + aView->GetPosition();

    for (nsView* kid = aView->GetFirstChild(); kid; kid = kid->GetNextSibling())
    {
      AdjustChildWidgets(kid, widgetToViewOrigin, aP2A, aInvalidate);
    }
  }
}


NS_IMETHODIMP nsScrollPortView::SetScrolledView(nsIView *aScrolledView)
{
  NS_ASSERTION(GetFirstChild() == nsnull || GetFirstChild()->GetNextSibling() == nsnull,
               "Error scroll port has too many children");

  
  if (GetFirstChild() != nsnull)
  {
    mViewManager->RemoveChild(GetFirstChild());
  }

  return mViewManager->InsertChild(this, aScrolledView, 0);
}

NS_IMETHODIMP nsScrollPortView::GetScrolledView(nsIView *&aScrolledView) const
{
  aScrolledView = GetScrolledView();
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetScrollPosition(nscoord &aX, nscoord &aY) const
{
  aX = mOffsetX;
  aY = mOffsetY;

  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetScrollProperties(PRUint32 aProperties)
{
  mScrollProperties = aProperties;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetScrollProperties(PRUint32 *aProperties)
{
  *aProperties = mScrollProperties;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::SetLineHeight(nscoord aHeight)
{
  mLineHeight = aHeight;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::GetLineHeight(nscoord *aHeight)
{
  *aHeight = mLineHeight;
  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ScrollByLines(PRInt32 aNumLinesX, PRInt32 aNumLinesY)
{
  nscoord dx = mLineHeight*aNumLinesX;
  nscoord dy = mLineHeight*aNumLinesY;

  return ScrollTo(mOffsetX + dx, mOffsetY + dy, NS_VMREFRESH_SMOOTHSCROLL);
}

NS_IMETHODIMP nsScrollPortView::GetPageScrollDistances(nsSize *aDistances)
{
  nsSize size;
  GetDimensions(size);

  
  
  aDistances->width  = size.width  - PR_MIN(size.width  / 10, 2 * mLineHeight);
  aDistances->height = size.height - PR_MIN(size.height / 10, 2 * mLineHeight);

  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ScrollByPages(PRInt32 aNumPagesX, PRInt32 aNumPagesY)
{
  nsSize delta;
  GetPageScrollDistances(&delta);
    
  
  delta.width *= aNumPagesX;
  delta.height *= aNumPagesY;

  return ScrollTo(mOffsetX + delta.width, mOffsetY + delta.height,
                  NS_VMREFRESH_SMOOTHSCROLL);
}

NS_IMETHODIMP nsScrollPortView::ScrollByWhole(PRBool aTop)
{
  nscoord   newPos = 0;

  if (!aTop) {
    nsSize scrolledSize;
    nsView* scrolledView = GetScrolledView();
    scrolledView->GetDimensions(scrolledSize);
    newPos = scrolledSize.height;
  }

  ScrollTo(mOffsetX, newPos, 0);

  return NS_OK;
}

NS_IMETHODIMP nsScrollPortView::ScrollByPixels(PRInt32 aNumPixelsX,
                                               PRInt32 aNumPixelsY)
{
  nsCOMPtr<nsIDeviceContext> dev;
  mViewManager->GetDeviceContext(*getter_AddRefs(dev));
  PRInt32 p2a = dev->AppUnitsPerDevPixel(); 

  nscoord dx = NSIntPixelsToAppUnits(aNumPixelsX, p2a);
  nscoord dy = NSIntPixelsToAppUnits(aNumPixelsY, p2a);

  return ScrollTo(mOffsetX + dx, mOffsetY + dy, 0);
}

NS_IMETHODIMP nsScrollPortView::CanScroll(PRBool aHorizontal,
                                          PRBool aForward,
                                          PRBool &aResult)
{
  nscoord offset = aHorizontal ? mOffsetX : mOffsetY;

  nsView* scrolledView = GetScrolledView();
  if (!scrolledView) {
    aResult = PR_FALSE;
    return NS_ERROR_FAILURE;
  }

  nsRect scrolledRect;
  scrolledView->GetDimensions(scrolledRect);

  
  if (!aForward) {
    aResult = offset > (aHorizontal ? scrolledRect.x : scrolledRect.y);
    return NS_OK;
  }

  nsSize portSize;
  GetDimensions(portSize);

  nsCOMPtr<nsIDeviceContext> dev;
  mViewManager->GetDeviceContext(*getter_AddRefs(dev));
  PRInt32 p2a = dev->AppUnitsPerDevPixel();

  nscoord max;
  if (aHorizontal) {
    max = scrolledRect.XMost() - portSize.width;
    
    nscoord maxPx = NSAppUnitsToIntPixels(max, p2a);
    max = NSIntPixelsToAppUnits(maxPx, p2a);
  } else {
    max = scrolledRect.YMost() - portSize.height;
    
    nscoord maxPx = NSAppUnitsToIntPixels(max, p2a);
    max = NSIntPixelsToAppUnits(maxPx, p2a);
  }

  
  aResult = (offset < max) ? PR_TRUE : PR_FALSE;

  return NS_OK;
}

void nsScrollPortView::Scroll(nsView *aScrolledView, nsPoint aTwipsDelta, nsPoint aPixDelta,
                              PRInt32 aP2A)
{
  if (aTwipsDelta.x != 0 || aTwipsDelta.y != 0)
  {
    nsIWidget *scrollWidget = GetWidget();
    nsRegion updateRegion;
    PRBool canBitBlit = PR_TRUE;
    if (!scrollWidget) {
      canBitBlit = PR_FALSE;
    } else {
      PRUint32 scrolledViewFlags = aScrolledView->GetViewFlags();

      if ((!(mScrollProperties & NS_SCROLL_PROPERTY_ALWAYS_BLIT) && 
           !mViewManager->CanScrollWithBitBlt(aScrolledView, aTwipsDelta, &updateRegion))) {
        canBitBlit = PR_FALSE;
      }
    }

    if (canBitBlit) {
      
      
      mViewManager->WillBitBlit(this, aTwipsDelta);
    }
    
    if (!scrollWidget)
    {
      NS_ASSERTION(!canBitBlit, "Someone screwed up");
      nsPoint offsetToWidget;
      GetNearestWidget(&offsetToWidget);
      
      
      
      
      
      AdjustChildWidgets(aScrolledView, offsetToWidget, aP2A, PR_TRUE);
      
      
      
      mViewManager->UpdateView(this, 0);
    } else if (!canBitBlit) {
      
      
      
      nsRect bounds(GetBounds());
      nsPoint topLeft(bounds.x, bounds.y);
      AdjustChildWidgets(aScrolledView,
                         GetPosition() - topLeft, aP2A, PR_FALSE);
      
      
      mViewManager->UpdateView(this, 0);
    } else { 
      
      
      scrollWidget->Scroll(aPixDelta.x, aPixDelta.y, nsnull);
      mViewManager->UpdateViewAfterScroll(this, updateRegion);
    }
  }
}

NS_IMETHODIMP nsScrollPortView::ScrollToImpl(nscoord aX, nscoord aY, PRUint32 aUpdateFlags)
{
  PRInt32           dxPx = 0, dyPx = 0;

  
  nsCOMPtr<nsIDeviceContext> dev;
  mViewManager->GetDeviceContext(*getter_AddRefs(dev));
  PRInt32 p2a = dev->AppUnitsPerDevPixel();

  
  nsresult rv = ClampScrollValues(aX, aY, this);
  if (NS_FAILED(rv)) {
    return rv;
  }
  
  
  nscoord aXpx = NSAppUnitsToIntPixels(aX, p2a);
  nscoord aYpx = NSAppUnitsToIntPixels(aY, p2a);
  
  aX = NSIntPixelsToAppUnits(aXpx, p2a);
  aY = NSIntPixelsToAppUnits(aYpx, p2a);
  
  
  
  
  if (aX == mOffsetX && aY == mOffsetY) {
    return NS_OK;
  }

  
  dxPx = mOffsetXpx - aXpx;
  dyPx = mOffsetYpx - aYpx;

  
  PRUint32 listenerCount;
  const nsIID& kScrollPositionListenerIID = NS_GET_IID(nsIScrollPositionListener);
  nsIScrollPositionListener* listener;
  if (nsnull != mListeners) {
    if (NS_SUCCEEDED(mListeners->Count(&listenerCount))) {
      for (PRUint32 i = 0; i < listenerCount; i++) {
        if (NS_SUCCEEDED(mListeners->QueryElementAt(i, kScrollPositionListenerIID, (void**)&listener))) {
          listener->ScrollPositionWillChange(this, aX, aY);
          NS_RELEASE(listener);
        }
      }
    }
  }
  
  nsView* scrolledView = GetScrolledView();
  if (!scrolledView) return NS_ERROR_FAILURE;

  
  
  
  scrolledView->SetPositionIgnoringChildWidgets(-aX, -aY);
      
  
  
  mOffsetXpx = aXpx;
  mOffsetYpx = aYpx;
      
  nsPoint twipsDelta(aX - mOffsetX, aY - mOffsetY);

  
  mOffsetX = aX;
  mOffsetY = aY;

  Scroll(scrolledView, twipsDelta, nsPoint(dxPx, dyPx), p2a);

  mViewManager->SynthesizeMouseMove(PR_TRUE);
  
  
  if (nsnull != mListeners) {
    if (NS_SUCCEEDED(mListeners->Count(&listenerCount))) {
      for (PRUint32 i = 0; i < listenerCount; i++) {
        if (NS_SUCCEEDED(mListeners->QueryElementAt(i, kScrollPositionListenerIID, (void**)&listener))) {
          listener->ScrollPositionDidChange(this, aX, aY);
          NS_RELEASE(listener);
        }
      }
    }
  }
 
  return NS_OK;
}







PRBool nsScrollPortView::IsSmoothScrollingEnabled() {
  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    PRBool enabled;
    nsresult rv = prefs->GetBoolPref(SMOOTH_SCROLL_PREF_NAME, &enabled);
    if (NS_SUCCEEDED(rv)) {
      return enabled;
    }
  }
  return PR_FALSE;
}






void
nsScrollPortView::SmoothScrollAnimationCallback (nsITimer *aTimer, void* anInstance) 
{
  nsScrollPortView* self = NS_STATIC_CAST(nsScrollPortView*, anInstance);
  if (self) {
    self->IncrementalScroll();
  }
}



 
void
nsScrollPortView::IncrementalScroll()
{
  if (!mSmoothScroll) {
    return;
  }

  if (mSmoothScroll->mFrameIndex < SMOOTH_SCROLL_FRAMES) {
    ScrollToImpl(mOffsetX + mSmoothScroll->mVelocities[mSmoothScroll->mFrameIndex*2],
                 mOffsetY + mSmoothScroll->mVelocities[mSmoothScroll->mFrameIndex*2 + 1],
                 0);
    mSmoothScroll->mFrameIndex++;
  } else {
    delete mSmoothScroll;
    mSmoothScroll = nsnull;
  }
}
