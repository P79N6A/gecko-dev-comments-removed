






































#include "nsListBoxBodyFrame.h"

#include "nsListBoxLayout.h"

#include "nsCOMPtr.h"
#include "nsGridRowGroupLayout.h"
#include "nsISupportsArray.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsCSSFrameConstructor.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollbarFrame.h"
#include "nsIScrollableView.h"
#include "nsIView.h"
#include "nsIViewManager.h"
#include "nsStyleContext.h"
#include "nsIRenderingContext.h"
#include "nsIDeviceContext.h"
#include "nsIFontMetrics.h"
#include "nsITimer.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsIDOMNSDocument.h"
#include "nsPIBoxObject.h"
#include "nsINodeInfo.h"
#include "nsLayoutUtils.h"
#include "nsPIListBoxObject.h"
#include "nsContentUtils.h"











#ifdef XP_MAC
#pragma mark -
#endif



#define USER_TIME_THRESHOLD 150000



#define TIME_PER_ROW_INITAL  50000



#define SMOOTH_INTERVAL 100

class nsListScrollSmoother : public nsITimerCallback
{
public:
  NS_DECL_ISUPPORTS

  nsListScrollSmoother(nsListBoxBodyFrame* aOuter);
  virtual ~nsListScrollSmoother();

  
  NS_DECL_NSITIMERCALLBACK

  void Start();
  void Stop();
  PRBool IsRunning();

  nsCOMPtr<nsITimer> mRepeatTimer;
  PRBool mDelta;
  nsListBoxBodyFrame* mOuter;
}; 

nsListScrollSmoother::nsListScrollSmoother(nsListBoxBodyFrame* aOuter)
{
  mDelta = 0;
  mOuter = aOuter;
}

nsListScrollSmoother::~nsListScrollSmoother()
{
  Stop();
}

NS_IMETHODIMP
nsListScrollSmoother::Notify(nsITimer *timer)
{
  Stop();

  NS_ASSERTION(mOuter, "mOuter is null, see bug #68365");
  if (!mOuter) return NS_OK;

  
  mOuter->InternalPositionChangedCallback();
  return NS_OK;
}

PRBool
nsListScrollSmoother::IsRunning()
{
  return mRepeatTimer ? PR_TRUE : PR_FALSE;
}

void
nsListScrollSmoother::Start()
{
  Stop();
  mRepeatTimer = do_CreateInstance("@mozilla.org/timer;1");
  mRepeatTimer->InitWithCallback(this, SMOOTH_INTERVAL, nsITimer::TYPE_ONE_SHOT);
}

void
nsListScrollSmoother::Stop()
{
  if ( mRepeatTimer ) {
    mRepeatTimer->Cancel();
    mRepeatTimer = nsnull;
  }
}

NS_IMPL_ISUPPORTS1(nsListScrollSmoother, nsITimerCallback)



nsListBoxBodyFrame::nsListBoxBodyFrame(nsIPresShell* aPresShell,
                                       nsStyleContext* aContext,
                                       PRBool aIsRoot,
                                       nsIBoxLayout* aLayoutManager)
  : nsBoxFrame(aPresShell, aContext, aIsRoot, aLayoutManager),
    mRowCount(-1),
    mRowHeight(0),
    mRowHeightWasSet(PR_FALSE),
    mAvailableHeight(0),
    mStringWidth(-1),
    mTopFrame(nsnull),
    mBottomFrame(nsnull),
    mLinkupFrame(nsnull),
    mRowsToPrepend(0),
    mCurrentIndex(0),
    mOldIndex(0),
    mScrolling(PR_FALSE),
    mAdjustScroll(PR_FALSE),
    mYPosition(0),
    mScrollSmoother(nsnull),
    mTimePerRow(TIME_PER_ROW_INITAL),
    mReflowCallbackPosted(PR_FALSE)
{
}

nsListBoxBodyFrame::~nsListBoxBodyFrame()
{
  NS_IF_RELEASE(mScrollSmoother);

#if USE_TIMER_TO_DELAY_SCROLLING
  StopScrollTracking();
  mAutoScrollTimer = nsnull;
#endif

}



NS_IMETHODIMP_(nsrefcnt) 
nsListBoxBodyFrame::AddRef(void)
{
  return 2;
}

NS_IMETHODIMP_(nsrefcnt)
nsListBoxBodyFrame::Release(void)
{
  return 1;
}




NS_INTERFACE_MAP_BEGIN(nsListBoxBodyFrame)
  NS_INTERFACE_MAP_ENTRY(nsIListBoxObject)
  NS_INTERFACE_MAP_ENTRY(nsIScrollbarMediator)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)




NS_IMETHODIMP
nsListBoxBodyFrame::Init(nsIContent*     aContent,
                         nsIFrame*       aParent, 
                         nsIFrame*       aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  nsIScrollableFrame* scrollFrame = nsLayoutUtils::GetScrollableFrameFor(this);
  if (!scrollFrame)
    return rv;

  nsIScrollableView* scrollableView = scrollFrame->GetScrollableView();
  scrollableView->SetScrollProperties(NS_SCROLL_PROPERTY_ALWAYS_BLIT);

  nsIBox* verticalScrollbar = scrollFrame->GetScrollbarBox(PR_TRUE);
  if (!verticalScrollbar) {
    NS_ERROR("Unable to install the scrollbar mediator on the listbox widget. You must be using GFX scrollbars.");
    return NS_ERROR_FAILURE;
  }

  nsCOMPtr<nsIScrollbarFrame> scrollbarFrame(do_QueryInterface(verticalScrollbar));
  scrollbarFrame->SetScrollbarMediatorContent(GetContent());

  nsCOMPtr<nsIFontMetrics> fm;
  PresContext()->DeviceContext()->GetMetricsFor(
    GetStyleContext()->GetStyleFont()->mFont, *getter_AddRefs(fm)
    );
  fm->GetHeight(mRowHeight);

  return rv;
}

void
nsListBoxBodyFrame::Destroy()
{
  
  if (mReflowCallbackPosted)
     PresContext()->PresShell()->CancelReflowCallback(this);

  
  for (nsIFrame *a = mParent; a; a = a->GetParent()) {
    nsIContent *content = a->GetContent();
    nsIDocument *doc;

    if (content &&
        content->NodeInfo()->Equals(nsGkAtoms::listbox, kNameSpaceID_XUL) &&
        (doc = content->GetDocument())) {
      nsCOMPtr<nsIDOMElement> e(do_QueryInterface(content));
      nsCOMPtr<nsIDOMNSDocument> nsdoc(do_QueryInterface(doc));

      nsCOMPtr<nsIBoxObject> box;
      nsdoc->GetBoxObjectFor(e, getter_AddRefs(box));

      nsCOMPtr<nsPIBoxObject> piBox = do_QueryInterface(box);
      if (piBox) {
        piBox->ClearCachedValues();
      }

      break;
    }
  }

  nsBoxFrame::Destroy();
}

NS_IMETHODIMP
nsListBoxBodyFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                     nsIAtom* aAttribute, 
                                     PRInt32 aModType)
{
  nsresult rv = NS_OK;

  if (aAttribute == nsGkAtoms::rows) {
    nsAutoString rows;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rows, rows);
    
    if (!rows.IsEmpty()) {
      PRInt32 dummy;
      PRInt32 count = rows.ToInteger(&dummy);
      PRInt32 rowHeight = GetRowHeightAppUnits();
      rowHeight = nsPresContext::AppUnitsToIntCSSPixels(rowHeight);
      nsAutoString value;
      value.AppendInt(rowHeight*count);
      mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::minheight, value, PR_FALSE);

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
    }
  }
  else
    rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);

  return rv;
 
}



 void
nsListBoxBodyFrame::MarkIntrinsicWidthsDirty()
{
  mStringWidth = -1;
  nsBoxFrame::MarkIntrinsicWidthsDirty();
}



NS_IMETHODIMP
nsListBoxBodyFrame::DoLayout(nsBoxLayoutState& aBoxLayoutState)
{
  if (mScrolling)
    aBoxLayoutState.SetPaintingDisabled(PR_TRUE);

  nsresult rv = nsBoxFrame::DoLayout(aBoxLayoutState);

  if (mScrolling)
    aBoxLayoutState.SetPaintingDisabled(PR_FALSE);

  
  
  if (mAdjustScroll)
     PostReflowCallback();

  return rv;
}

nsSize
nsListBoxBodyFrame::GetMinSizeForScrollArea(nsBoxLayoutState& aBoxLayoutState)
{
  nsSize result(0, 0);
  if (nsContentUtils::HasNonEmptyAttr(GetContent(), kNameSpaceID_None,
                                      nsGkAtoms::sizemode)) {
    result = GetPrefSize(aBoxLayoutState);
    result.height = 0;
    nsIScrollableFrame* scrollFrame = nsLayoutUtils::GetScrollableFrameFor(this);
    if (scrollFrame &&
        scrollFrame->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
      nsMargin scrollbars =
        scrollFrame->GetDesiredScrollbarSizes(&aBoxLayoutState);
      result.width += scrollbars.left + scrollbars.right;
    }
  }
  return result;
}

nsSize
nsListBoxBodyFrame::GetPrefSize(nsBoxLayoutState& aBoxLayoutState)
{  
  nsSize pref = nsBoxFrame::GetPrefSize(aBoxLayoutState);

  PRInt32 size = GetFixedRowSize();
  if (size > -1)
    pref.height = size*GetRowHeightAppUnits();

  nsIScrollableFrame* scrollFrame = nsLayoutUtils::GetScrollableFrameFor(this);
  if (scrollFrame &&
      scrollFrame->GetScrollbarStyles().mVertical == NS_STYLE_OVERFLOW_AUTO) {
    nsMargin scrollbars = scrollFrame->GetDesiredScrollbarSizes(&aBoxLayoutState);
    pref.width += scrollbars.left + scrollbars.right;
  }
  return pref;
}



NS_IMETHODIMP
nsListBoxBodyFrame::PositionChanged(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32& aNewIndex)
{ 
  if (mScrolling)
    return NS_OK;

  nscoord oldTwipIndex, newTwipIndex;
  oldTwipIndex = mCurrentIndex*mRowHeight;
  newTwipIndex = nsPresContext::CSSPixelsToAppUnits(aNewIndex);
  PRInt32 twipDelta = newTwipIndex > oldTwipIndex ? newTwipIndex - oldTwipIndex : oldTwipIndex - newTwipIndex;

  PRInt32 rowDelta = twipDelta / mRowHeight;
  PRInt32 remainder = twipDelta % mRowHeight;
  if (remainder > (mRowHeight/2))
    rowDelta++;

  if (rowDelta == 0)
    return NS_OK;

  

  PRInt32 newIndex = newTwipIndex > oldTwipIndex ? mCurrentIndex + rowDelta : mCurrentIndex - rowDelta;
  

  nsListScrollSmoother* smoother = GetSmoother();

  
  
  if (smoother->IsRunning() || rowDelta*mTimePerRow > USER_TIME_THRESHOLD) {

     smoother->Stop();

     
     mContent->GetDocument()->FlushPendingNotifications(Flush_OnlyReflow);

     smoother->mDelta = newTwipIndex > oldTwipIndex ? rowDelta : -rowDelta;

     smoother->Start();

     return NS_OK;
  }

  smoother->Stop();

  mCurrentIndex = newIndex;
  smoother->mDelta = 0;
  
  if (mCurrentIndex < 0) {
    mCurrentIndex = 0;
    return NS_OK;
  }

  return InternalPositionChanged(newTwipIndex < oldTwipIndex, rowDelta);
}

NS_IMETHODIMP
nsListBoxBodyFrame::VisibilityChanged(nsISupports* aScrollbar, PRBool aVisible)
{
  PRInt32 lastPageTopRow = GetRowCount() - (GetAvailableHeight() / mRowHeight);
  if (lastPageTopRow < 0)
    lastPageTopRow = 0;
  PRInt32 delta = mCurrentIndex - lastPageTopRow;
  if (delta > 0) {
    mCurrentIndex = lastPageTopRow;
    InternalPositionChanged(PR_TRUE, delta);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::ScrollbarButtonPressed(nsISupports* aScrollbar, PRInt32 aOldIndex, PRInt32 aNewIndex)
{
  if (aOldIndex == aNewIndex)
    return NS_OK;
  if (aNewIndex < aOldIndex)
    mCurrentIndex--;
  else mCurrentIndex++;
  if (mCurrentIndex < 0) {
    mCurrentIndex = 0;
    return NS_OK;
  }
  InternalPositionChanged(aNewIndex < aOldIndex, 1);

  return NS_OK;
}



PRBool
nsListBoxBodyFrame::ReflowFinished()
{
  
  CreateRows();

  
  if (mAdjustScroll) {
     VerticalScroll(mYPosition);
     mAdjustScroll = PR_FALSE;
  }

  
  
  if (mRowHeightWasSet) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
     PRInt32 pos = mCurrentIndex * mRowHeight;
     if (mYPosition != pos) 
       mAdjustScroll = PR_TRUE;
    mRowHeightWasSet = PR_FALSE;
  }

  mReflowCallbackPosted = PR_FALSE;
  return PR_TRUE;
}



NS_IMETHODIMP
nsListBoxBodyFrame::GetListboxBody(nsIListBoxObject * *aListboxBody)
{
  *aListboxBody = this;
  NS_IF_ADDREF(*aListboxBody);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::GetRowCount(PRInt32* aResult)
{
  *aResult = GetRowCount();
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::GetNumberOfVisibleRows(PRInt32 *aResult)
{
  *aResult= mRowHeight ? GetAvailableHeight() / mRowHeight : 0;
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::GetIndexOfFirstVisibleRow(PRInt32 *aResult)
{
  *aResult = mCurrentIndex;
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::EnsureIndexIsVisible(PRInt32 aRowIndex)
{
  NS_ASSERTION(aRowIndex >= 0, "Ensure row is visible called with a negative number!");
  if (aRowIndex < 0)
    return NS_ERROR_ILLEGAL_VALUE;

  PRInt32 rows = 0;
  if (mRowHeight)
    rows = GetAvailableHeight()/mRowHeight;
  if (rows <= 0)
    rows = 1;
  PRInt32 bottomIndex = mCurrentIndex + rows;
  
  
  if (mCurrentIndex <= aRowIndex && aRowIndex < bottomIndex)
    return NS_OK;

  
  PRInt32 delta;

  PRBool up = aRowIndex < mCurrentIndex;
  if (up) {
    delta = mCurrentIndex - aRowIndex;
    mCurrentIndex = aRowIndex;
  }
  else {
    
    delta = 1 + (aRowIndex-bottomIndex);
    mCurrentIndex += delta; 
  }

  InternalPositionChanged(up, delta);
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::ScrollByLines(PRInt32 aNumLines)
{
  PRInt32 scrollIndex, visibleRows;
  GetIndexOfFirstVisibleRow(&scrollIndex);
  GetNumberOfVisibleRows(&visibleRows);

  scrollIndex += aNumLines;
  
  if (scrollIndex < 0)
    scrollIndex = 0;
  else {
    PRInt32 numRows = GetRowCount();
    PRInt32 lastPageTopRow = numRows - visibleRows;
    if (scrollIndex > lastPageTopRow)
      scrollIndex = lastPageTopRow;
  }
  
  ScrollToIndex(scrollIndex);

  
  
  
    
  
  
  PresContext()->GetViewManager()->ForceUpdate();

  return NS_OK;
}


NS_IMETHODIMP
nsListBoxBodyFrame::GetIndexOfItem(nsIDOMElement* aItem, PRInt32* _retval)
{
  if (aItem) {
    *_retval = 0;
    nsCOMPtr<nsIContent> itemContent(do_QueryInterface(aItem));

    nsIContent* listbox = mContent->GetBindingParent();
    NS_ENSURE_STATE(listbox);

    PRUint32 childCount = listbox->GetChildCount();
    for (PRUint32 childIndex = 0; childIndex < childCount; childIndex++) {
      nsIContent *child = listbox->GetChildAt(childIndex);

      
      if (child->Tag() == nsGkAtoms::listitem) {
        
        if (child == itemContent)
          return NS_OK;

        ++(*_retval);
      }
    }
  }

  
  *_retval = -1;
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::GetItemAtIndex(PRInt32 aIndex, nsIDOMElement** aItem)
{
  *aItem = nsnull;
  if (aIndex < 0)
    return NS_OK;
  
  nsIContent* listbox = mContent->GetBindingParent();
  NS_ENSURE_STATE(listbox);

  PRUint32 childCount = listbox->GetChildCount();

  PRInt32 itemCount = 0;
  for (PRUint32 childIndex = 0; childIndex < childCount; childIndex++) {
    nsIContent *child = listbox->GetChildAt(childIndex);

    
    if (child->Tag() == nsGkAtoms::listitem) {
      
      if (itemCount == aIndex) {
        return CallQueryInterface(child, aItem);
      }
      ++itemCount;
    }
  }

  
  return NS_OK;
}



PRInt32
nsListBoxBodyFrame::GetRowCount()
{
  if (mRowCount < 0)
    ComputeTotalRowCount();
  return mRowCount;
}

PRInt32
nsListBoxBodyFrame::GetFixedRowSize()
{
  PRInt32 dummy;

  nsAutoString rows;
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rows, rows);
  if (!rows.IsEmpty())
    return rows.ToInteger(&dummy);
 
  mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::size, rows);

  if (!rows.IsEmpty())
    return rows.ToInteger(&dummy);

  return -1;
}

void
nsListBoxBodyFrame::SetRowHeight(nscoord aRowHeight)
{ 
  if (aRowHeight > mRowHeight) { 
    mRowHeight = aRowHeight;
    
    nsAutoString rows;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::rows, rows);
    if (rows.IsEmpty())
      mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::size, rows);
    
    if (!rows.IsEmpty()) {
      PRInt32 dummy;
      PRInt32 count = rows.ToInteger(&dummy);
      PRInt32 rowHeight = nsPresContext::AppUnitsToIntCSSPixels(aRowHeight);
      nsAutoString value;
      value.AppendInt(rowHeight*count);
      mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::minheight, value, PR_FALSE);
    }

    
    
    
    mRowHeightWasSet = PR_TRUE;
    PostReflowCallback();
  }
}

nscoord
nsListBoxBodyFrame::GetAvailableHeight()
{
  nsIScrollableFrame* scrollFrame
    = nsLayoutUtils::GetScrollableFrameFor(this);
  nsIScrollableView* scrollView = scrollFrame->GetScrollableView();
  return scrollView->View()->GetBounds().height;
}

nscoord
nsListBoxBodyFrame::GetYPosition()
{
  return mYPosition;
}

nscoord
nsListBoxBodyFrame::ComputeIntrinsicWidth(nsBoxLayoutState& aBoxLayoutState)
{
  if (mStringWidth != -1)
    return mStringWidth;

  nscoord largestWidth = 0;

  PRInt32 index = 0;
  nsCOMPtr<nsIDOMElement> firstRowEl;
  GetItemAtIndex(index, getter_AddRefs(firstRowEl));
  nsCOMPtr<nsIContent> firstRowContent(do_QueryInterface(firstRowEl));

  if (firstRowContent) {
    nsRefPtr<nsStyleContext> styleContext;
    nsPresContext *presContext = aBoxLayoutState.PresContext();
    styleContext = presContext->StyleSet()->
      ResolveStyleFor(firstRowContent, nsnull);

    nscoord width = 0;
    nsMargin margin(0,0,0,0);

    if (styleContext->GetStylePadding()->GetPadding(margin))
      width += margin.LeftRight();
    width += styleContext->GetStyleBorder()->GetBorder().LeftRight();
    if (styleContext->GetStyleMargin()->GetMargin(margin))
      width += margin.LeftRight();

    nsIContent* listbox = mContent->GetBindingParent();
    NS_ENSURE_TRUE(listbox, largestWidth);

    PRUint32 childCount = listbox->GetChildCount();

    for (PRUint32 i = 0; i < childCount && i < 100; ++i) {
      nsIContent *child = listbox->GetChildAt(i);

      if (child->Tag() == nsGkAtoms::listitem) {
        nsIRenderingContext* rendContext = aBoxLayoutState.GetRenderingContext();
        if (rendContext) {
          nsAutoString value;
          PRUint32 textCount = child->GetChildCount();
          for (PRUint32 j = 0; j < textCount; ++j) {
            nsIContent* text = child->GetChildAt(j);
            if (text && text->IsNodeOfType(nsINode::eTEXT)) {
              text->AppendTextTo(value);
            }
          }
          nsCOMPtr<nsIFontMetrics> fm;
          presContext->DeviceContext()->
            GetMetricsFor(styleContext->GetStyleFont()->mFont,
                          *getter_AddRefs(fm));
          rendContext->SetFont(fm);

          nscoord textWidth =
            nsLayoutUtils::GetStringWidth(this, rendContext, value.get(), value.Length());
          textWidth += width;

          if (textWidth > largestWidth) 
            largestWidth = textWidth;
        }
      }
    }
  }

  mStringWidth = largestWidth;
  return mStringWidth;
}

void
nsListBoxBodyFrame::ComputeTotalRowCount()
{
  mRowCount = 0;
  nsIContent* listbox = mContent->GetBindingParent();
  ENSURE_TRUE(listbox);

  PRUint32 childCount = listbox->GetChildCount();
  for (PRUint32 i = 0; i < childCount; i++) {
    if (listbox->GetChildAt(i)->Tag() == nsGkAtoms::listitem)
      ++mRowCount;
  }
}

void
nsListBoxBodyFrame::PostReflowCallback()
{
  if (!mReflowCallbackPosted) {
    mReflowCallbackPosted = PR_TRUE;
    PresContext()->PresShell()->PostReflowCallback(this);
  }
}



NS_IMETHODIMP
nsListBoxBodyFrame::ScrollToIndex(PRInt32 aRowIndex)
{
  if (( aRowIndex < 0 ) || (mRowHeight == 0))
    return NS_OK;
    
  PRInt32 newIndex = aRowIndex;
  PRInt32 delta = mCurrentIndex > newIndex ? mCurrentIndex - newIndex : newIndex - mCurrentIndex;
  PRBool up = newIndex < mCurrentIndex;

  
  PRInt32 lastPageTopRow = GetRowCount() - (GetAvailableHeight() / mRowHeight);
  if (lastPageTopRow < 0)
    lastPageTopRow = 0;

  if (aRowIndex > lastPageTopRow)
    return NS_OK;

  mCurrentIndex = newIndex;
  InternalPositionChanged(up, delta);

  
  
  
  mContent->GetDocument()->FlushPendingNotifications(Flush_OnlyReflow);

  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::InternalPositionChangedCallback()
{
  nsListScrollSmoother* smoother = GetSmoother();

  if (smoother->mDelta == 0)
    return NS_OK;

  mCurrentIndex += smoother->mDelta;

  if (mCurrentIndex < 0)
    mCurrentIndex = 0;

  return InternalPositionChanged(smoother->mDelta < 0, smoother->mDelta < 0 ? -smoother->mDelta : smoother->mDelta);
}

NS_IMETHODIMP
nsListBoxBodyFrame::InternalPositionChanged(PRBool aUp, PRInt32 aDelta)
{  
  if (aDelta == 0)
    return NS_OK;

  nsPresContext *presContext = PresContext();
  nsBoxLayoutState state(presContext);

  
  PRTime start = PR_Now();

  mContent->GetDocument()->FlushPendingNotifications(Flush_OnlyReflow);

  PRInt32 visibleRows = 0;
  if (mRowHeight)
    visibleRows = GetAvailableHeight()/mRowHeight;
  
  if (aDelta < visibleRows) {
    PRInt32 loseRows = aDelta;
    if (aUp) {
      
      ReverseDestroyRows(loseRows);
      mRowsToPrepend += aDelta;
      mLinkupFrame = nsnull;
    }
    else {
      
      DestroyRows(loseRows);
      mRowsToPrepend = 0;
    }
  }
  else {
    
    
    nsIFrame *currBox = mFrames.FirstChild();
    while (currBox) {
      nsIFrame *nextBox = currBox->GetNextSibling();
      RemoveChildFrame(state, currBox);
      currBox = nextBox;
    }
  }

  
  mTopFrame = mBottomFrame = nsnull; 
  
  mYPosition = mCurrentIndex*mRowHeight;
  mScrolling = PR_TRUE;
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eResize, NS_FRAME_HAS_DIRTY_CHILDREN);
  
  
  presContext->PresShell()->FlushPendingNotifications(Flush_OnlyReflow);
  mScrolling = PR_FALSE;
  
  VerticalScroll(mYPosition);

  PRTime end = PR_Now();

  PRTime difTime;
  LL_SUB(difTime, end, start);

  PRInt32 newTime;
  LL_L2I(newTime, difTime);
  newTime /= aDelta;

  
  mTimePerRow = (newTime + mTimePerRow)/2;
  
  return NS_OK;
}

nsListScrollSmoother* 
nsListBoxBodyFrame::GetSmoother()
{
  if (!mScrollSmoother) {
    mScrollSmoother = new nsListScrollSmoother(this);
    NS_ASSERTION(mScrollSmoother, "out of memory");
    NS_IF_ADDREF(mScrollSmoother);
  }

  return mScrollSmoother;
}

void
nsListBoxBodyFrame::VerticalScroll(PRInt32 aPosition)
{
  nsIScrollableFrame* scrollFrame
    = nsLayoutUtils::GetScrollableFrameFor(this);

  nsPoint scrollPosition = scrollFrame->GetScrollPosition();
 
  scrollFrame->ScrollTo(nsPoint(scrollPosition.x, aPosition), NS_SCROLL_PROPERTY_ALWAYS_BLIT);

  mYPosition = aPosition;
}



nsIFrame*
nsListBoxBodyFrame::GetFirstFrame()
{
  mTopFrame = mFrames.FirstChild();
  return mTopFrame;
}

nsIFrame*
nsListBoxBodyFrame::GetLastFrame()
{
  return mFrames.LastChild();
}



void
nsListBoxBodyFrame::CreateRows()
{
  
  nsRect clientRect;
  GetClientRect(clientRect);

  
  
  nscoord availableHeight = GetAvailableHeight();
  
  if (availableHeight <= 0) {
    PRBool fixed = (GetFixedRowSize() != -1);
    if (fixed)
      availableHeight = 10;
    else
      return;
  }
  
  
  PRBool created = PR_FALSE;
  nsIBox* box = GetFirstItemBox(0, &created);
  nscoord rowHeight = GetRowHeightAppUnits();
  while (box) {  
    if (created && mRowsToPrepend > 0)
      --mRowsToPrepend;

    
    
    if (rowHeight == 0)
        return;
     
    availableHeight -= rowHeight;
    
    
    if (!ContinueReflow(availableHeight))
      break;

    
    box = GetNextItemBox(box, 0, &created);
  }

  mRowsToPrepend = 0;
  mLinkupFrame = nsnull;
}

void
nsListBoxBodyFrame::DestroyRows(PRInt32& aRowsToLose) 
{
  
  
  nsIFrame* childFrame = GetFirstFrame();
  nsBoxLayoutState state(PresContext());

  while (childFrame && aRowsToLose > 0) {
    --aRowsToLose;

    nsIFrame* nextFrame = childFrame->GetNextSibling();
    RemoveChildFrame(state, childFrame);

    mTopFrame = childFrame = nextFrame;
  }

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsListBoxBodyFrame::ReverseDestroyRows(PRInt32& aRowsToLose) 
{
  
  
  nsIFrame* childFrame = GetLastFrame();
  nsBoxLayoutState state(PresContext());

  while (childFrame && aRowsToLose > 0) {
    --aRowsToLose;
    
    nsIFrame* prevFrame;
    prevFrame = mFrames.GetPrevSiblingFor(childFrame);
    RemoveChildFrame(state, childFrame);

    mBottomFrame = childFrame = prevFrame;
  }

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}





nsIBox* 
nsListBoxBodyFrame::GetFirstItemBox(PRInt32 aOffset, PRBool* aCreated)
{
  if (aCreated)
   *aCreated = PR_FALSE;

  
  mBottomFrame = mTopFrame;

  if (mTopFrame) {
    return mTopFrame->IsBoxFrame() ? NS_STATIC_CAST(nsIBox*, mTopFrame) : nsnull;
  }

  
  mTopFrame = GetFirstFrame();
  mBottomFrame = mTopFrame;

  if (mTopFrame && mRowsToPrepend <= 0) {
    return mTopFrame->IsBoxFrame() ? NS_STATIC_CAST(nsIBox*, mTopFrame) : nsnull;
  }

  
  
  
  

  nsCOMPtr<nsIContent> startContent;
  if (mTopFrame && mRowsToPrepend > 0) {
    
    nsIContent* topContent = mTopFrame->GetContent();
    nsIContent* topParent = topContent->GetParent();
    PRInt32 contentIndex = topParent->IndexOf(topContent);
    contentIndex -= aOffset;
    if (contentIndex < 0)
      return nsnull;
    startContent = topParent->GetChildAt(contentIndex - mRowsToPrepend);
  } else {
    
    
    GetListItemContentAt(mCurrentIndex+aOffset, getter_AddRefs(startContent));
  }

  if (startContent) {  
    
    
    
    PRBool isAppend = mRowsToPrepend <= 0;
    
    nsPresContext* presContext = PresContext();
    nsCSSFrameConstructor* fc = presContext->PresShell()->FrameConstructor();
    nsIFrame* topFrame = nsnull;
    fc->CreateListBoxContent(presContext, this, nsnull, startContent,
                             &topFrame, isAppend, PR_FALSE, nsnull);
    mTopFrame = topFrame;
    if (mTopFrame) {
      if (aCreated)
        *aCreated = PR_TRUE;

      mBottomFrame = mTopFrame;

      return mTopFrame->IsBoxFrame() ? NS_STATIC_CAST(nsIBox*, mTopFrame) : nsnull;
    } else
      return GetFirstItemBox(++aOffset, 0);
  }

  return nsnull;
}





nsIBox* 
nsListBoxBodyFrame::GetNextItemBox(nsIBox* aBox, PRInt32 aOffset,
                                   PRBool* aCreated)
{
  if (aCreated)
    *aCreated = PR_FALSE;

  nsIFrame* result = aBox->GetNextSibling();

  if (!result || result == mLinkupFrame || mRowsToPrepend > 0) {
    
    nsIContent* prevContent = aBox->GetContent();
    nsIContent* parentContent = prevContent->GetParent();

    PRInt32 i = parentContent->IndexOf(prevContent);

    PRUint32 childCount = parentContent->GetChildCount();
    if (((PRUint32)i + aOffset + 1) < childCount) {
      
      nsIContent *nextContent = parentContent->GetChildAt(i + aOffset + 1);

      
      PRBool isAppend = result != mLinkupFrame && mRowsToPrepend <= 0;
      nsIFrame* prevFrame = isAppend ? nsnull : aBox;
      
      nsPresContext* presContext = PresContext();
      nsCSSFrameConstructor* fc = presContext->PresShell()->FrameConstructor();
      fc->CreateListBoxContent(presContext, this, prevFrame, nextContent,
                               &result, isAppend, PR_FALSE, nsnull);

      if (result) {
        if (aCreated)
           *aCreated = PR_TRUE;
      } else
        return GetNextItemBox(aBox, ++aOffset, aCreated);
            
      mLinkupFrame = nsnull;
    }
  }

  if (!result)
    return nsnull;

  mBottomFrame = result;

  return result->IsBoxFrame() ? result : nsnull;
}

PRBool
nsListBoxBodyFrame::ContinueReflow(nscoord height) 
{ 
  nsPresContext* presContext = PresContext();
  if (presContext->PresShell()->IsAccessibilityActive()) {
    
    
    return PR_TRUE;
  }

  if (height <= 0) {
    nsIFrame* lastChild = GetLastFrame();
    nsIFrame* startingPoint = mBottomFrame;
    if (startingPoint == nsnull) {
      
      startingPoint = GetFirstFrame();
    }

    if (lastChild != startingPoint) {
      
      
      nsIFrame* currFrame = startingPoint->GetNextSibling();
      nsBoxLayoutState state(PresContext());

      while (currFrame) {
        nsIFrame* nextFrame = currFrame->GetNextSibling();
        RemoveChildFrame(state, currFrame);
        currFrame = nextFrame;
      }

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
    return PR_FALSE;
  }
  else
    return PR_TRUE;
}

NS_IMETHODIMP
nsListBoxBodyFrame::ListBoxAppendFrames(nsIFrame* aFrameList)
{
  
  nsBoxLayoutState state(PresContext());
  mFrames.AppendFrames(nsnull, aFrameList);
  if (mLayoutManager)
    mLayoutManager->ChildrenAppended(this, state, aFrameList);
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
  
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::ListBoxInsertFrames(nsIFrame* aPrevFrame, nsIFrame* aFrameList)
{
  
  nsBoxLayoutState state(PresContext());
  mFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);
  if (mLayoutManager)
    mLayoutManager->ChildrenInserted(this, state, aPrevFrame, aFrameList);
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}




void 
nsListBoxBodyFrame::OnContentInserted(nsPresContext* aPresContext, nsIContent* aChildContent)
{
  if (mRowCount >= 0)
    ++mRowCount;

  nsIPresShell *shell = aPresContext->PresShell();
  
  
  
  
  
  nsIFrame* childFrame = shell->GetPrimaryFrameFor(aChildContent);
  if (childFrame)
    return;

  PRInt32 siblingIndex;
  nsCOMPtr<nsIContent> nextSiblingContent;
  GetListItemNextSibling(aChildContent, getter_AddRefs(nextSiblingContent), siblingIndex);
  
  
  
  if (siblingIndex >= 0 &&  siblingIndex-1 <= mCurrentIndex) {
    mTopFrame = nsnull;
    mRowsToPrepend = 1;
  } else if (nextSiblingContent) {
    
    nsIFrame* nextSiblingFrame = shell->GetPrimaryFrameFor(nextSiblingContent);
    mLinkupFrame = nextSiblingFrame;
  }
  
  CreateRows();
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}




void
nsListBoxBodyFrame::OnContentRemoved(nsPresContext* aPresContext, nsIFrame* aChildFrame, PRInt32 aIndex)
{
  NS_ASSERTION(!aChildFrame || aChildFrame->GetParent() == this,
               "Removing frame that's not our child... Not good");
  
  if (mRowCount >= 0)
    --mRowCount;

  nsIContent* listBoxContent = mContent->GetBindingParent();
  if (listBoxContent) {
    if (!aChildFrame) {
      
      
      nsIContent *oldNextSiblingContent = listBoxContent->GetChildAt(aIndex);
  
      PRInt32 siblingIndex = -1;
      if (oldNextSiblingContent) {
        nsCOMPtr<nsIContent> nextSiblingContent;
        GetListItemNextSibling(oldNextSiblingContent, getter_AddRefs(nextSiblingContent), siblingIndex);
      }
    
      
      
      if (siblingIndex >= 0 && siblingIndex-1 < mCurrentIndex) {
        NS_PRECONDITION(mCurrentIndex > 0, "mCurrentIndex > 0");
        --mCurrentIndex;
        mYPosition = mCurrentIndex*mRowHeight;
        VerticalScroll(mYPosition);
      }
    } else if (mCurrentIndex > 0) {
      
      
      
      
      
      
      
      PRUint32 childCount = listBoxContent->GetChildCount();
      if (childCount > 0) {
        nsIContent *lastChild = listBoxContent->GetChildAt(childCount - 1);
        nsIFrame* lastChildFrame = 
          aPresContext->PresShell()->GetPrimaryFrameFor(lastChild);
      
        if (lastChildFrame) {
          mTopFrame = nsnull;
          mRowsToPrepend = 1;
          --mCurrentIndex;
          mYPosition = mCurrentIndex*mRowHeight;
          VerticalScroll(mYPosition);
        }
      }
    }
  }

  
  if (mTopFrame && mTopFrame == aChildFrame)
    mTopFrame = mTopFrame->GetNextSibling();

  
  nsBoxLayoutState state(aPresContext);
  if (aChildFrame) {
    RemoveChildFrame(state, aChildFrame);
  }

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsListBoxBodyFrame::GetListItemContentAt(PRInt32 aIndex, nsIContent** aContent)
{
  *aContent = nsnull;
  nsIContent* listboxContent = mContent->GetBindingParent();
  ENSURE_TRUE(listboxContent);

  PRUint32 childCount = listboxContent->GetChildCount();
  PRInt32 itemsFound = 0;
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *kid = listboxContent->GetChildAt(i);

    if (kid->Tag() == nsGkAtoms::listitem) {
      ++itemsFound;
      if (itemsFound-1 == aIndex) {
        *aContent = kid;
        NS_IF_ADDREF(*aContent);
        return;
      }
        
    }
  }
}

void
nsListBoxBodyFrame::GetListItemNextSibling(nsIContent* aListItem, nsIContent** aContent, PRInt32& aSiblingIndex)
{
  *aContent = nsnull;
  aSiblingIndex = -1;
  nsIContent* listboxContent = mContent->GetBindingParent();
  ENSURE_TRUE(listboxContent);

  PRUint32 childCount = listboxContent->GetChildCount();
  nsIContent *prevKid = nsnull;
  for (PRUint32 i = 0; i < childCount; ++i) {
    nsIContent *kid = listboxContent->GetChildAt(i);

    if (kid->Tag() == nsGkAtoms::listitem) {
      ++aSiblingIndex;
      if (prevKid == aListItem) {
        *aContent = kid;
        NS_IF_ADDREF(*aContent);
        return;
      }
        
    }
    prevKid = kid;
  }

  aSiblingIndex = -1; 
}

void
nsListBoxBodyFrame::RemoveChildFrame(nsBoxLayoutState &aState,
                                     nsIFrame         *aFrame)
{
  nsPresContext* presContext = PresContext();
  nsCSSFrameConstructor* fc = presContext->PresShell()->FrameConstructor();
  fc->RemoveMappingsForFrameSubtree(aFrame);

#ifdef DEBUG
  PRBool removed =
#endif
    mFrames.RemoveFrame(aFrame);
  NS_ASSERTION(removed,
               "Going to destroy a frame we didn't remove.  Prepare to crash");
  if (mLayoutManager)
    mLayoutManager->ChildrenRemoved(this, aState, aFrame);
  aFrame->Destroy();
}



nsIFrame*
NS_NewListBoxBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext,
                       PRBool aIsRoot, nsIBoxLayout* aLayoutManager)
{
  return 
    new (aPresShell) nsListBoxBodyFrame(aPresShell, aContext, aIsRoot, aLayoutManager);
}
