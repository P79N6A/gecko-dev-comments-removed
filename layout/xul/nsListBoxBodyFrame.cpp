




#include "nsListBoxBodyFrame.h"

#include "nsListBoxLayout.h"

#include "mozilla/MathAlgorithms.h"
#include "nsCOMPtr.h"
#include "nsGridRowGroupLayout.h"
#include "nsIServiceManager.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsNameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDOMElement.h"
#include "nsIDOMNodeList.h"
#include "nsCSSFrameConstructor.h"
#include "nsIScrollableFrame.h"
#include "nsScrollbarFrame.h"
#include "nsView.h"
#include "nsViewManager.h"
#include "nsStyleContext.h"
#include "nsFontMetrics.h"
#include "nsITimer.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsPIBoxObject.h"
#include "nsLayoutUtils.h"
#include "nsPIListBoxObject.h"
#include "nsContentUtils.h"
#include "ChildIterator.h"
#include "nsRenderingContext.h"
#include "prtime.h"
#include <algorithm>

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

using namespace mozilla;
using namespace mozilla::dom;













#define USER_TIME_THRESHOLD 150000



#define TIME_PER_ROW_INITAL  50000



#define SMOOTH_INTERVAL 100

class nsListScrollSmoother final : public nsITimerCallback
{
private:
  virtual ~nsListScrollSmoother();

public:
  NS_DECL_ISUPPORTS

  explicit nsListScrollSmoother(nsListBoxBodyFrame* aOuter);

  
  NS_DECL_NSITIMERCALLBACK

  void Start();
  void Stop();
  bool IsRunning();

  nsCOMPtr<nsITimer> mRepeatTimer;
  int32_t mDelta;
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

bool
nsListScrollSmoother::IsRunning()
{
  return mRepeatTimer ? true : false;
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
    mRepeatTimer = nullptr;
  }
}

NS_IMPL_ISUPPORTS(nsListScrollSmoother, nsITimerCallback)



nsListBoxBodyFrame::nsListBoxBodyFrame(nsStyleContext* aContext,
                                       nsBoxLayout* aLayoutManager)
  : nsBoxFrame(aContext, false, aLayoutManager),
    mTopFrame(nullptr),
    mBottomFrame(nullptr),
    mLinkupFrame(nullptr),
    mScrollSmoother(nullptr),
    mRowsToPrepend(0),
    mRowCount(-1),
    mRowHeight(0),
    mAvailableHeight(0),
    mStringWidth(-1),
    mCurrentIndex(0),
    mOldIndex(0),
    mYPosition(0),
    mTimePerRow(TIME_PER_ROW_INITAL),
    mRowHeightWasSet(false),
    mScrolling(false),
    mAdjustScroll(false),
    mReflowCallbackPosted(false)
{
}

nsListBoxBodyFrame::~nsListBoxBodyFrame()
{
  NS_IF_RELEASE(mScrollSmoother);

#if USE_TIMER_TO_DELAY_SCROLLING
  StopScrollTracking();
  mAutoScrollTimer = nullptr;
#endif

}

NS_QUERYFRAME_HEAD(nsListBoxBodyFrame)
  NS_QUERYFRAME_ENTRY(nsIScrollbarMediator)
  NS_QUERYFRAME_ENTRY(nsListBoxBodyFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)



void
nsListBoxBodyFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent, 
                         nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);
  
  
  nsIScrollableFrame* scrollFrame = do_QueryFrame(aParent);
  if (scrollFrame) {
    nsIFrame* verticalScrollbar = scrollFrame->GetScrollbarBox(true);
    nsScrollbarFrame* scrollbarFrame = do_QueryFrame(verticalScrollbar);
    if (scrollbarFrame) {
      scrollbarFrame->SetScrollbarMediatorContent(GetContent());
    }
  }
  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  mRowHeight = fm->MaxHeight();
}

void
nsListBoxBodyFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (mReflowCallbackPosted)
     PresContext()->PresShell()->CancelReflowCallback(this);

  
  for (uint32_t i = 0; i < mPendingPositionChangeEvents.Length(); ++i) {
    mPendingPositionChangeEvents[i]->Revoke();
  }

  
  if (mBoxObject) {
    mBoxObject->ClearCachedValues();
  }

  nsBoxFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsListBoxBodyFrame::AttributeChanged(int32_t aNameSpaceID,
                                     nsIAtom* aAttribute, 
                                     int32_t aModType)
{
  nsresult rv = NS_OK;

  if (aAttribute == nsGkAtoms::rows) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }
  else
    rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);

  return rv;
 
}

 void
nsListBoxBodyFrame::MarkIntrinsicISizesDirty()
{
  mStringWidth = -1;
  nsBoxFrame::MarkIntrinsicISizesDirty();
}



NS_IMETHODIMP
nsListBoxBodyFrame::DoLayout(nsBoxLayoutState& aBoxLayoutState)
{
  if (mScrolling)
    aBoxLayoutState.SetPaintingDisabled(true);

  nsresult rv = nsBoxFrame::DoLayout(aBoxLayoutState);

  
  
  nsRect rect(nsPoint(0, 0), GetSize());
  nsOverflowAreas overflow(rect, rect);
  if (mLayoutManager) {
    nsIFrame* childFrame = mFrames.FirstChild();
    while (childFrame) {
      ConsiderChildOverflow(overflow, childFrame);
      childFrame = childFrame->GetNextSibling();
    }

    nsSize prefSize = mLayoutManager->GetPrefSize(this, aBoxLayoutState);
    NS_FOR_FRAME_OVERFLOW_TYPES(otype) {
      nsRect& o = overflow.Overflow(otype);
      o.height = std::max(o.height, prefSize.height);
    }
  }
  FinishAndStoreOverflow(overflow, GetSize());

  if (mScrolling)
    aBoxLayoutState.SetPaintingDisabled(false);

  
  
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

  int32_t size = GetFixedRowSize();
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



void
nsListBoxBodyFrame::ScrollByPage(nsScrollbarFrame* aScrollbar, int32_t aDirection)
{
  MOZ_ASSERT(aScrollbar != nullptr);
  aScrollbar->SetIncrementToPage(aDirection);
  nsWeakFrame weakFrame(this);
  int32_t newPos = aScrollbar->MoveToNewPosition();
  if (!weakFrame.IsAlive()) {
    return;
  }
  UpdateIndex(newPos);
}

void
nsListBoxBodyFrame::ScrollByWhole(nsScrollbarFrame* aScrollbar, int32_t aDirection)
{
  MOZ_ASSERT(aScrollbar != nullptr);
  aScrollbar->SetIncrementToWhole(aDirection);
  nsWeakFrame weakFrame(this);
  int32_t newPos = aScrollbar->MoveToNewPosition();
  if (!weakFrame.IsAlive()) {
    return;
  }
  UpdateIndex(newPos);
}

void
nsListBoxBodyFrame::ScrollByLine(nsScrollbarFrame* aScrollbar, int32_t aDirection)
{
  MOZ_ASSERT(aScrollbar != nullptr);
  aScrollbar->SetIncrementToLine(aDirection);
  nsWeakFrame weakFrame(this);
  int32_t newPos = aScrollbar->MoveToNewPosition();
  if (!weakFrame.IsAlive()) {
    return;
  }
  UpdateIndex(newPos);
}

void
nsListBoxBodyFrame::RepeatButtonScroll(nsScrollbarFrame* aScrollbar)
{
  nsWeakFrame weakFrame(this);
  int32_t newPos = aScrollbar->MoveToNewPosition();
  if (!weakFrame.IsAlive()) {
    return;
  }
  UpdateIndex(newPos);
}

int32_t
nsListBoxBodyFrame::ToRowIndex(nscoord aPos) const
{
  return NS_roundf(float(std::max(aPos, 0)) / mRowHeight);
}

void
nsListBoxBodyFrame::ThumbMoved(nsScrollbarFrame* aScrollbar,
                               nscoord aOldPos,
                               nscoord aNewPos)
{ 
  if (mScrolling || mRowHeight == 0)
    return;

  int32_t newIndex = ToRowIndex(aNewPos);
  if (newIndex == mCurrentIndex) {
    return;
  }
  int32_t rowDelta = newIndex - mCurrentIndex;

  nsListScrollSmoother* smoother = GetSmoother();

  
  
  if (smoother->IsRunning() || Abs(rowDelta)*mTimePerRow > USER_TIME_THRESHOLD) {

     smoother->Stop();

     smoother->mDelta = rowDelta;

     smoother->Start();

     return;
  }

  smoother->Stop();

  mCurrentIndex = newIndex;
  smoother->mDelta = 0;
  
  if (mCurrentIndex < 0) {
    mCurrentIndex = 0;
    return;
  }
  InternalPositionChanged(rowDelta < 0, Abs(rowDelta));
}

void
nsListBoxBodyFrame::VisibilityChanged(bool aVisible)
{
  if (mRowHeight == 0)
    return;

  int32_t lastPageTopRow = GetRowCount() - (GetAvailableHeight() / mRowHeight);
  if (lastPageTopRow < 0)
    lastPageTopRow = 0;
  int32_t delta = mCurrentIndex - lastPageTopRow;
  if (delta > 0) {
    mCurrentIndex = lastPageTopRow;
    InternalPositionChanged(true, delta);
  }
}

nsIFrame*
nsListBoxBodyFrame::GetScrollbarBox(bool aVertical)
{
  nsIScrollableFrame* scrollFrame = nsLayoutUtils::GetScrollableFrameFor(this);
  return scrollFrame ? scrollFrame->GetScrollbarBox(true) : nullptr;
}

void
nsListBoxBodyFrame::UpdateIndex(int32_t aNewPos)
{
  int32_t newIndex = ToRowIndex(nsPresContext::CSSPixelsToAppUnits(aNewPos));
  if (newIndex == mCurrentIndex) {
    return;
  }
  bool up = newIndex < mCurrentIndex;
  int32_t indexDelta = Abs(newIndex - mCurrentIndex);
  mCurrentIndex = newIndex;
  InternalPositionChanged(up, indexDelta);
}
 


bool
nsListBoxBodyFrame::ReflowFinished()
{
  nsAutoScriptBlocker scriptBlocker;
  
  CreateRows();

  
  if (mAdjustScroll) {
     VerticalScroll(mYPosition);
     mAdjustScroll = false;
  }

  
  
  if (mRowHeightWasSet) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
     int32_t pos = mCurrentIndex * mRowHeight;
     if (mYPosition != pos) 
       mAdjustScroll = true;
    mRowHeightWasSet = false;
  }

  mReflowCallbackPosted = false;
  return true;
}

void
nsListBoxBodyFrame::ReflowCallbackCanceled()
{
  mReflowCallbackPosted = false;
}



int32_t
nsListBoxBodyFrame::GetNumberOfVisibleRows()
{
  return mRowHeight ? GetAvailableHeight() / mRowHeight : 0;
}

int32_t
nsListBoxBodyFrame::GetIndexOfFirstVisibleRow()
{
  return mCurrentIndex;
}

nsresult
nsListBoxBodyFrame::EnsureIndexIsVisible(int32_t aRowIndex)
{
  if (aRowIndex < 0)
    return NS_ERROR_ILLEGAL_VALUE;

  int32_t rows = 0;
  if (mRowHeight)
    rows = GetAvailableHeight()/mRowHeight;
  if (rows <= 0)
    rows = 1;
  int32_t bottomIndex = mCurrentIndex + rows;
  
  
  if (mCurrentIndex <= aRowIndex && aRowIndex < bottomIndex)
    return NS_OK;

  int32_t delta;

  bool up = aRowIndex < mCurrentIndex;
  if (up) {
    delta = mCurrentIndex - aRowIndex;
    mCurrentIndex = aRowIndex;
  }
  else {
    
    if (aRowIndex >= GetRowCount())
      return NS_ERROR_ILLEGAL_VALUE;

    
    delta = 1 + (aRowIndex-bottomIndex);
    mCurrentIndex += delta; 
  }

  
  
  DoInternalPositionChangedSync(up, delta);
  return NS_OK;
}

nsresult
nsListBoxBodyFrame::ScrollByLines(int32_t aNumLines)
{
  int32_t scrollIndex = GetIndexOfFirstVisibleRow(),
    visibleRows = GetNumberOfVisibleRows();

  scrollIndex += aNumLines;
  
  if (scrollIndex < 0)
    scrollIndex = 0;
  else {
    int32_t numRows = GetRowCount();
    int32_t lastPageTopRow = numRows - visibleRows;
    if (scrollIndex > lastPageTopRow)
      scrollIndex = lastPageTopRow;
  }
  
  ScrollToIndex(scrollIndex);

  return NS_OK;
}


nsresult
nsListBoxBodyFrame::GetIndexOfItem(nsIDOMElement* aItem, int32_t* _retval)
{
  if (aItem) {
    *_retval = 0;
    nsCOMPtr<nsIContent> itemContent(do_QueryInterface(aItem));

    FlattenedChildIterator iter(mContent);
    for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
      
      if (child->IsXULElement(nsGkAtoms::listitem)) {
        
        if (child == itemContent)
          return NS_OK;

        ++(*_retval);
      }
    }
  }

  
  *_retval = -1;
  return NS_OK;
}

nsresult
nsListBoxBodyFrame::GetItemAtIndex(int32_t aIndex, nsIDOMElement** aItem)
{
  *aItem = nullptr;
  if (aIndex < 0)
    return NS_OK;

  int32_t itemCount = 0;
  FlattenedChildIterator iter(mContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    
    if (child->IsXULElement(nsGkAtoms::listitem)) {
      
      if (itemCount == aIndex) {
        return CallQueryInterface(child, aItem);
      }
      ++itemCount;
    }
  }

  
  return NS_OK;
}



int32_t
nsListBoxBodyFrame::GetRowCount()
{
  if (mRowCount < 0)
    ComputeTotalRowCount();
  return mRowCount;
}

int32_t
nsListBoxBodyFrame::GetFixedRowSize()
{
  nsresult dummy;

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

    
    
    
    mRowHeightWasSet = true;
    PostReflowCallback();
  }
}

nscoord
nsListBoxBodyFrame::GetAvailableHeight()
{
  nsIScrollableFrame* scrollFrame =
    nsLayoutUtils::GetScrollableFrameFor(this);
  if (scrollFrame) {
    return scrollFrame->GetScrollPortRect().height;
  }
  return 0;
}

nscoord
nsListBoxBodyFrame::GetYPosition()
{
  return mYPosition;
}

nscoord
nsListBoxBodyFrame::ComputeIntrinsicISize(nsBoxLayoutState& aBoxLayoutState)
{
  if (mStringWidth != -1)
    return mStringWidth;

  nscoord largestWidth = 0;

  int32_t index = 0;
  nsCOMPtr<nsIDOMElement> firstRowEl;
  GetItemAtIndex(index, getter_AddRefs(firstRowEl));
  nsCOMPtr<nsIContent> firstRowContent(do_QueryInterface(firstRowEl));

  if (firstRowContent) {
    nsRefPtr<nsStyleContext> styleContext;
    nsPresContext *presContext = aBoxLayoutState.PresContext();
    styleContext = presContext->StyleSet()->
      ResolveStyleFor(firstRowContent->AsElement(), nullptr);

    nscoord width = 0;
    nsMargin margin(0,0,0,0);

    if (styleContext->StylePadding()->GetPadding(margin))
      width += margin.LeftRight();
    width += styleContext->StyleBorder()->GetComputedBorder().LeftRight();
    if (styleContext->StyleMargin()->GetMargin(margin))
      width += margin.LeftRight();

    FlattenedChildIterator iter(mContent);
    for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
      if (child->IsXULElement(nsGkAtoms::listitem)) {
        nsRenderingContext* rendContext = aBoxLayoutState.GetRenderingContext();
        if (rendContext) {
          nsAutoString value;
          uint32_t textCount = child->GetChildCount();
          for (uint32_t j = 0; j < textCount; ++j) {
            nsIContent* text = child->GetChildAt(j);
            if (text && text->IsNodeOfType(nsINode::eTEXT)) {
              text->AppendTextTo(value);
            }
          }

          nsRefPtr<nsFontMetrics> fm;
          nsLayoutUtils::GetFontMetricsForStyleContext(styleContext,
                                                       getter_AddRefs(fm));

          nscoord textWidth =
            nsLayoutUtils::AppUnitWidthOfStringBidi(value, this, *fm,
                                                    *rendContext);
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
  FlattenedChildIterator iter(mContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    if (child->IsXULElement(nsGkAtoms::listitem)) {
      ++mRowCount;
    }
  }
}

void
nsListBoxBodyFrame::PostReflowCallback()
{
  if (!mReflowCallbackPosted) {
    mReflowCallbackPosted = true;
    PresContext()->PresShell()->PostReflowCallback(this);
  }
}



nsresult
nsListBoxBodyFrame::ScrollToIndex(int32_t aRowIndex)
{
  if (( aRowIndex < 0 ) || (mRowHeight == 0))
    return NS_OK;
    
  int32_t newIndex = aRowIndex;
  int32_t delta = mCurrentIndex > newIndex ? mCurrentIndex - newIndex : newIndex - mCurrentIndex;
  bool up = newIndex < mCurrentIndex;

  
  int32_t lastPageTopRow = GetRowCount() - (GetAvailableHeight() / mRowHeight);
  if (lastPageTopRow < 0)
    lastPageTopRow = 0;

  if (aRowIndex > lastPageTopRow)
    return NS_OK;

  mCurrentIndex = newIndex;

  nsWeakFrame weak(this);

  
  DoInternalPositionChangedSync(up, delta);

  if (!weak.IsAlive()) {
    return NS_OK;
  }

  
  
  
  mContent->GetComposedDoc()->FlushPendingNotifications(Flush_Layout);

  return NS_OK;
}

nsresult
nsListBoxBodyFrame::InternalPositionChangedCallback()
{
  nsListScrollSmoother* smoother = GetSmoother();

  if (smoother->mDelta == 0)
    return NS_OK;

  mCurrentIndex += smoother->mDelta;

  if (mCurrentIndex < 0)
    mCurrentIndex = 0;

  return DoInternalPositionChangedSync(smoother->mDelta < 0,
                                       smoother->mDelta < 0 ?
                                         -smoother->mDelta : smoother->mDelta);
}

nsresult
nsListBoxBodyFrame::InternalPositionChanged(bool aUp, int32_t aDelta)
{
  nsRefPtr<nsPositionChangedEvent> ev =
    new nsPositionChangedEvent(this, aUp, aDelta);
  nsresult rv = NS_DispatchToCurrentThread(ev);
  if (NS_SUCCEEDED(rv)) {
    if (!mPendingPositionChangeEvents.AppendElement(ev)) {
      rv = NS_ERROR_OUT_OF_MEMORY;
      ev->Revoke();
    }
  }
  return rv;
}

nsresult
nsListBoxBodyFrame::DoInternalPositionChangedSync(bool aUp, int32_t aDelta)
{
  nsWeakFrame weak(this);
  
  
  nsTArray< nsRefPtr<nsPositionChangedEvent> > temp;
  temp.SwapElements(mPendingPositionChangeEvents);
  for (uint32_t i = 0; i < temp.Length(); ++i) {
    if (weak.IsAlive()) {
      temp[i]->Run();
    }
    temp[i]->Revoke();
  }

  if (!weak.IsAlive()) {
    return NS_OK;
  }

  return DoInternalPositionChanged(aUp, aDelta);
}

nsresult
nsListBoxBodyFrame::DoInternalPositionChanged(bool aUp, int32_t aDelta)
{
  if (aDelta == 0)
    return NS_OK;

  nsRefPtr<nsPresContext> presContext(PresContext());
  nsBoxLayoutState state(presContext);

  
  PRTime start = PR_Now();

  nsWeakFrame weakThis(this);
  mContent->GetComposedDoc()->FlushPendingNotifications(Flush_Layout);
  if (!weakThis.IsAlive()) {
    return NS_OK;
  }

  {
    nsAutoScriptBlocker scriptBlocker;

    int32_t visibleRows = 0;
    if (mRowHeight)
      visibleRows = GetAvailableHeight()/mRowHeight;
  
    if (aDelta < visibleRows) {
      int32_t loseRows = aDelta;
      if (aUp) {
        
        ReverseDestroyRows(loseRows);
        mRowsToPrepend += aDelta;
        mLinkupFrame = nullptr;
      }
      else {
        
        DestroyRows(loseRows);
        mRowsToPrepend = 0;
      }
    }
    else {
      
      
      nsIFrame *currBox = mFrames.FirstChild();
      nsCSSFrameConstructor* fc = presContext->PresShell()->FrameConstructor();
      fc->BeginUpdate();
      while (currBox) {
        nsIFrame *nextBox = currBox->GetNextSibling();
        RemoveChildFrame(state, currBox);
        currBox = nextBox;
      }
      fc->EndUpdate();
    }

    
    mTopFrame = mBottomFrame = nullptr; 
  
    mYPosition = mCurrentIndex*mRowHeight;
    mScrolling = true;
    presContext->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eResize, NS_FRAME_HAS_DIRTY_CHILDREN);
  }
  if (!weakThis.IsAlive()) {
    return NS_OK;
  }
  
  
  presContext->PresShell()->FlushPendingNotifications(Flush_Layout);
  if (!weakThis.IsAlive()) {
    return NS_OK;
  }

  mScrolling = false;
  
  VerticalScroll(mYPosition);

  PRTime end = PR_Now();

  int32_t newTime = int32_t(end - start) / aDelta;

  
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
nsListBoxBodyFrame::VerticalScroll(int32_t aPosition)
{
  nsIScrollableFrame* scrollFrame
    = nsLayoutUtils::GetScrollableFrameFor(this);
  if (!scrollFrame) {
    return;
  }

  nsPoint scrollPosition = scrollFrame->GetScrollPosition();
 
  nsWeakFrame weakFrame(this);
  scrollFrame->ScrollTo(nsPoint(scrollPosition.x, aPosition),
                        nsIScrollableFrame::INSTANT);
  if (!weakFrame.IsAlive()) {
    return;
  }

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

bool
nsListBoxBodyFrame::SupportsOrdinalsInChildren()
{
  return false;
}



void
nsListBoxBodyFrame::CreateRows()
{
  
  nsRect clientRect;
  GetClientRect(clientRect);

  
  
  nscoord availableHeight = GetAvailableHeight();
  
  if (availableHeight <= 0) {
    bool fixed = (GetFixedRowSize() != -1);
    if (fixed)
      availableHeight = 10;
    else
      return;
  }
  
  
  bool created = false;
  nsIFrame* box = GetFirstItemBox(0, &created);
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
  mLinkupFrame = nullptr;
}

void
nsListBoxBodyFrame::DestroyRows(int32_t& aRowsToLose) 
{
  
  
  nsIFrame* childFrame = GetFirstFrame();
  nsBoxLayoutState state(PresContext());

  nsCSSFrameConstructor* fc = PresContext()->PresShell()->FrameConstructor();
  fc->BeginUpdate();
  while (childFrame && aRowsToLose > 0) {
    --aRowsToLose;

    nsIFrame* nextFrame = childFrame->GetNextSibling();
    RemoveChildFrame(state, childFrame);

    mTopFrame = childFrame = nextFrame;
  }
  fc->EndUpdate();

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsListBoxBodyFrame::ReverseDestroyRows(int32_t& aRowsToLose) 
{
  
  
  nsIFrame* childFrame = GetLastFrame();
  nsBoxLayoutState state(PresContext());

  nsCSSFrameConstructor* fc = PresContext()->PresShell()->FrameConstructor();
  fc->BeginUpdate();
  while (childFrame && aRowsToLose > 0) {
    --aRowsToLose;
    
    nsIFrame* prevFrame;
    prevFrame = childFrame->GetPrevSibling();
    RemoveChildFrame(state, childFrame);

    mBottomFrame = childFrame = prevFrame;
  }
  fc->EndUpdate();

  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}

static bool
IsListItemChild(nsListBoxBodyFrame* aParent, nsIContent* aChild,
                nsIFrame** aChildFrame)
{
  *aChildFrame = nullptr;
  if (!aChild->IsXULElement(nsGkAtoms::listitem)) {
    return false;
  }
  nsIFrame* existingFrame = aChild->GetPrimaryFrame();
  if (existingFrame && existingFrame->GetParent() != aParent) {
    return false;
  }
  *aChildFrame = existingFrame;
  return true;
}





nsIFrame*
nsListBoxBodyFrame::GetFirstItemBox(int32_t aOffset, bool* aCreated)
{
  if (aCreated)
   *aCreated = false;

  
  mBottomFrame = mTopFrame;

  if (mTopFrame) {
    return mTopFrame->IsBoxFrame() ? mTopFrame : nullptr;
  }

  
  mTopFrame = GetFirstFrame();
  mBottomFrame = mTopFrame;

  if (mTopFrame && mRowsToPrepend <= 0) {
    return mTopFrame->IsBoxFrame() ? mTopFrame : nullptr;
  }

  
  
  
  

  nsCOMPtr<nsIContent> startContent;
  if (mTopFrame && mRowsToPrepend > 0) {
    
    nsIContent* topContent = mTopFrame->GetContent();
    nsIContent* topParent = topContent->GetParent();
    int32_t contentIndex = topParent->IndexOf(topContent);
    contentIndex -= aOffset;
    if (contentIndex < 0)
      return nullptr;
    startContent = topParent->GetChildAt(contentIndex - mRowsToPrepend);
  } else {
    
    
    GetListItemContentAt(mCurrentIndex+aOffset, getter_AddRefs(startContent));
  }

  if (startContent) {  
    nsIFrame* existingFrame;
    if (!IsListItemChild(this, startContent, &existingFrame)) {
      return GetFirstItemBox(++aOffset, aCreated);
    }
    if (existingFrame) {
      return existingFrame->IsBoxFrame() ? existingFrame : nullptr;
    }

    
    
    
    bool isAppend = mRowsToPrepend <= 0;
    
    nsPresContext* presContext = PresContext();
    nsCSSFrameConstructor* fc = presContext->PresShell()->FrameConstructor();
    nsIFrame* topFrame = nullptr;
    fc->CreateListBoxContent(presContext, this, nullptr, startContent,
                             &topFrame, isAppend, false, nullptr);
    mTopFrame = topFrame;
    if (mTopFrame) {
      if (aCreated)
        *aCreated = true;

      mBottomFrame = mTopFrame;

      return mTopFrame->IsBoxFrame() ? mTopFrame : nullptr;
    } else
      return GetFirstItemBox(++aOffset, 0);
  }

  return nullptr;
}





nsIFrame*
nsListBoxBodyFrame::GetNextItemBox(nsIFrame* aBox, int32_t aOffset,
                                   bool* aCreated)
{
  if (aCreated)
    *aCreated = false;

  nsIFrame* result = aBox->GetNextSibling();

  if (!result || result == mLinkupFrame || mRowsToPrepend > 0) {
    
    nsIContent* prevContent = aBox->GetContent();
    nsIContent* parentContent = prevContent->GetParent();

    int32_t i = parentContent->IndexOf(prevContent);

    uint32_t childCount = parentContent->GetChildCount();
    if (((uint32_t)i + aOffset + 1) < childCount) {
      
      nsIContent *nextContent = parentContent->GetChildAt(i + aOffset + 1);

      nsIFrame* existingFrame;
      if (!IsListItemChild(this, nextContent, &existingFrame)) {
        return GetNextItemBox(aBox, ++aOffset, aCreated);
      }
      if (!existingFrame) {
        
        bool isAppend = result != mLinkupFrame && mRowsToPrepend <= 0;
        nsIFrame* prevFrame = isAppend ? nullptr : aBox;
      
        nsPresContext* presContext = PresContext();
        nsCSSFrameConstructor* fc = presContext->PresShell()->FrameConstructor();
        fc->CreateListBoxContent(presContext, this, prevFrame, nextContent,
                                 &result, isAppend, false, nullptr);

        if (result) {
          if (aCreated)
            *aCreated = true;
        } else
          return GetNextItemBox(aBox, ++aOffset, aCreated);
      } else {
        result = existingFrame;
      }
            
      mLinkupFrame = nullptr;
    }
  }

  if (!result)
    return nullptr;

  mBottomFrame = result;

  NS_ASSERTION(!result->IsBoxFrame() || result->GetParent() == this,
               "returning frame that is not in childlist");

  return result->IsBoxFrame() ? result : nullptr;
}

bool
nsListBoxBodyFrame::ContinueReflow(nscoord height) 
{
#ifdef ACCESSIBILITY
  if (nsIPresShell::IsAccessibilityActive()) {
    
    
    return true;
  }
#endif

  if (height <= 0) {
    nsIFrame* lastChild = GetLastFrame();
    nsIFrame* startingPoint = mBottomFrame;
    if (startingPoint == nullptr) {
      
      startingPoint = GetFirstFrame();
    }

    if (lastChild != startingPoint) {
      
      
      nsIFrame* currFrame = startingPoint->GetNextSibling();
      nsBoxLayoutState state(PresContext());

      nsCSSFrameConstructor* fc =
        PresContext()->PresShell()->FrameConstructor();
      fc->BeginUpdate();
      while (currFrame) {
        nsIFrame* nextFrame = currFrame->GetNextSibling();
        RemoveChildFrame(state, currFrame);
        currFrame = nextFrame;
      }
      fc->EndUpdate();

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                         NS_FRAME_HAS_DIRTY_CHILDREN);
    }
    return false;
  }
  else
    return true;
}

NS_IMETHODIMP
nsListBoxBodyFrame::ListBoxAppendFrames(nsFrameList& aFrameList)
{
  
  nsBoxLayoutState state(PresContext());
  const nsFrameList::Slice& newFrames = mFrames.AppendFrames(nullptr, aFrameList);
  if (mLayoutManager)
    mLayoutManager->ChildrenAppended(this, state, newFrames);
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
  
  return NS_OK;
}

NS_IMETHODIMP
nsListBoxBodyFrame::ListBoxInsertFrames(nsIFrame* aPrevFrame,
                                        nsFrameList& aFrameList)
{
  
  nsBoxLayoutState state(PresContext());
  const nsFrameList::Slice& newFrames =
    mFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);
  if (mLayoutManager)
    mLayoutManager->ChildrenInserted(this, state, aPrevFrame, newFrames);
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

  
  
  
  
  
  nsIFrame* childFrame = aChildContent->GetPrimaryFrame();
  if (childFrame)
    return;

  int32_t siblingIndex;
  nsCOMPtr<nsIContent> nextSiblingContent;
  GetListItemNextSibling(aChildContent, getter_AddRefs(nextSiblingContent), siblingIndex);
  
  
  
  if (siblingIndex >= 0 &&  siblingIndex-1 <= mCurrentIndex) {
    mTopFrame = nullptr;
    mRowsToPrepend = 1;
  } else if (nextSiblingContent) {
    
    nsIFrame* nextSiblingFrame = nextSiblingContent->GetPrimaryFrame();
    mLinkupFrame = nextSiblingFrame;
  }
  
  CreateRows();
  PresContext()->PresShell()->
    FrameNeedsReflow(this, nsIPresShell::eTreeChange,
                     NS_FRAME_HAS_DIRTY_CHILDREN);
}




void
nsListBoxBodyFrame::OnContentRemoved(nsPresContext* aPresContext,
                                     nsIContent* aContainer,
                                     nsIFrame* aChildFrame,
                                     nsIContent* aOldNextSibling)
{
  NS_ASSERTION(!aChildFrame || aChildFrame->GetParent() == this,
               "Removing frame that's not our child... Not good");
  
  if (mRowCount >= 0)
    --mRowCount;

  if (aContainer) {
    if (!aChildFrame) {
      
      
      int32_t siblingIndex = -1;
      if (aOldNextSibling) {
        nsCOMPtr<nsIContent> nextSiblingContent;
        GetListItemNextSibling(aOldNextSibling,
                               getter_AddRefs(nextSiblingContent),
                               siblingIndex);
      }
    
      
      
      if (siblingIndex >= 0 && siblingIndex-1 < mCurrentIndex) {
        NS_PRECONDITION(mCurrentIndex > 0, "mCurrentIndex > 0");
        --mCurrentIndex;
        mYPosition = mCurrentIndex*mRowHeight;
        nsWeakFrame weakChildFrame(aChildFrame);
        VerticalScroll(mYPosition);
        if (!weakChildFrame.IsAlive()) {
          return;
        }
      }
    } else if (mCurrentIndex > 0) {
      
      
      
      
      

      
      nsIContent* lastChild = nullptr;
      FlattenedChildIterator iter(mContent);
      for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
        lastChild = child;
      }

      if (lastChild) {
        nsIFrame* lastChildFrame = lastChild->GetPrimaryFrame();

        if (lastChildFrame) {
          mTopFrame = nullptr;
          mRowsToPrepend = 1;
          --mCurrentIndex;
          mYPosition = mCurrentIndex*mRowHeight;
          nsWeakFrame weakChildFrame(aChildFrame);
          VerticalScroll(mYPosition);
          if (!weakChildFrame.IsAlive()) {
            return;
          }
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
nsListBoxBodyFrame::GetListItemContentAt(int32_t aIndex, nsIContent** aContent)
{
  *aContent = nullptr;

  int32_t itemsFound = 0;
  FlattenedChildIterator iter(mContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    if (child->IsXULElement(nsGkAtoms::listitem)) {
      ++itemsFound;
      if (itemsFound-1 == aIndex) {
        *aContent = child;
        NS_IF_ADDREF(*aContent);
        return;
      }
    }
  }
}

void
nsListBoxBodyFrame::GetListItemNextSibling(nsIContent* aListItem, nsIContent** aContent, int32_t& aSiblingIndex)
{
  *aContent = nullptr;
  aSiblingIndex = -1;
  nsIContent *prevKid = nullptr;
  FlattenedChildIterator iter(mContent);
  for (nsIContent* child = iter.GetNextChild(); child; child = iter.GetNextChild()) {
    if (child->IsXULElement(nsGkAtoms::listitem)) {
      ++aSiblingIndex;
      if (prevKid == aListItem) {
        *aContent = child;
        NS_IF_ADDREF(*aContent);
        return;
      }
    }
    prevKid = child;
  }

  aSiblingIndex = -1; 
}

void
nsListBoxBodyFrame::RemoveChildFrame(nsBoxLayoutState &aState,
                                     nsIFrame         *aFrame)
{
  MOZ_ASSERT(mFrames.ContainsFrame(aFrame));
  MOZ_ASSERT(aFrame != GetContentInsertionFrame());

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    nsIContent* content = aFrame->GetContent();
    accService->ContentRemoved(PresContext()->PresShell(), content);
  }
#endif

  mFrames.RemoveFrame(aFrame);
  if (mLayoutManager)
    mLayoutManager->ChildrenRemoved(this, aState, aFrame);
  aFrame->Destroy();
}



already_AddRefed<nsBoxLayout> NS_NewListBoxLayout();

nsIFrame*
NS_NewListBoxBodyFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsCOMPtr<nsBoxLayout> layout = NS_NewListBoxLayout();
  return new (aPresShell) nsListBoxBodyFrame(aContext, layout);
}

NS_IMPL_FRAMEARENA_HELPERS(nsListBoxBodyFrame)
