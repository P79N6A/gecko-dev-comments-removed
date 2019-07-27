











#include "nsSliderFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsNameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsIDOMMouseEvent.h"
#include "nsScrollbarButtonFrame.h"
#include "nsISliderListener.h"
#include "nsIScrollbarMediator.h"
#include "nsScrollbarFrame.h"
#include "nsRepeatService.h"
#include "nsBoxLayoutState.h"
#include "nsSprocketLayout.h"
#include "nsIServiceManager.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include "mozilla/MouseEvents.h"
#include <algorithm>

using namespace mozilla;

bool nsSliderFrame::gMiddlePref = false;
int32_t nsSliderFrame::gSnapMultiplier;


#undef DEBUG_SLIDER

static already_AddRefed<nsIContent>
GetContentOfBox(nsIFrame *aBox)
{
  nsCOMPtr<nsIContent> content = aBox->GetContent();
  return content.forget();
}

nsIFrame*
NS_NewSliderFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSliderFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSliderFrame)

NS_QUERYFRAME_HEAD(nsSliderFrame)
  NS_QUERYFRAME_ENTRY(nsSliderFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)

nsSliderFrame::nsSliderFrame(nsStyleContext* aContext):
  nsBoxFrame(aContext),
  mCurPos(0),
  mChange(0),
  mDragFinished(true),
  mUserChanged(false)
{
}


nsSliderFrame::~nsSliderFrame()
{
}

void
nsSliderFrame::Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  static bool gotPrefs = false;
  if (!gotPrefs) {
    gotPrefs = true;

    gMiddlePref = Preferences::GetBool("middlemouse.scrollbarPosition");
    gSnapMultiplier = Preferences::GetInt("slider.snapMultiplier");
  }

  mCurPos = GetCurrentPosition(aContent);
}

void
nsSliderFrame::RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame)
{
  nsBoxFrame::RemoveFrame(aListID, aOldFrame);
  if (mFrames.IsEmpty())
    RemoveListener();
}

void
nsSliderFrame::InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  bool wasEmpty = mFrames.IsEmpty();
  nsBoxFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
  if (wasEmpty)
    AddListener();
}

void
nsSliderFrame::AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList)
{
  
  
  bool wasEmpty = mFrames.IsEmpty();
  nsBoxFrame::AppendFrames(aListID, aFrameList);
  if (wasEmpty)
    AddListener();
}

int32_t
nsSliderFrame::GetCurrentPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::curpos, 0);
}

int32_t
nsSliderFrame::GetMinPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::minpos, 0);
}

int32_t
nsSliderFrame::GetMaxPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::maxpos, 100);
}

int32_t
nsSliderFrame::GetIncrement(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::increment, 1);
}


int32_t
nsSliderFrame::GetPageIncrement(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::pageincrement, 10);
}

int32_t
nsSliderFrame::GetIntegerAttribute(nsIContent* content, nsIAtom* atom, int32_t defaultValue)
{
    nsAutoString value;
    content->GetAttr(kNameSpaceID_None, atom, value);
    if (!value.IsEmpty()) {
      nsresult error;

      
      defaultValue = value.ToInteger(&error);
    }

    return defaultValue;
}

class nsValueChangedRunnable : public nsRunnable
{
public:
  nsValueChangedRunnable(nsISliderListener* aListener,
                         nsIAtom* aWhich,
                         int32_t aValue,
                         bool aUserChanged)
  : mListener(aListener), mWhich(aWhich),
    mValue(aValue), mUserChanged(aUserChanged)
  {}

  NS_IMETHODIMP Run()
  {
    return mListener->ValueChanged(nsDependentAtomString(mWhich),
                                   mValue, mUserChanged);
  }

  nsCOMPtr<nsISliderListener> mListener;
  nsCOMPtr<nsIAtom> mWhich;
  int32_t mValue;
  bool mUserChanged;
};

class nsDragStateChangedRunnable : public nsRunnable
{
public:
  nsDragStateChangedRunnable(nsISliderListener* aListener,
                             bool aDragBeginning)
  : mListener(aListener),
    mDragBeginning(aDragBeginning)
  {}

  NS_IMETHODIMP Run()
  {
    return mListener->DragStateChanged(mDragBeginning);
  }

  nsCOMPtr<nsISliderListener> mListener;
  bool mDragBeginning;
};

nsresult
nsSliderFrame::AttributeChanged(int32_t aNameSpaceID,
                                nsIAtom* aAttribute,
                                int32_t aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::curpos) {
     CurrentPositionChanged();
  } else if (aAttribute == nsGkAtoms::minpos ||
             aAttribute == nsGkAtoms::maxpos) {
      

      nsIFrame* scrollbarBox = GetScrollbar();
      nsCOMPtr<nsIContent> scrollbar;
      scrollbar = GetContentOfBox(scrollbarBox);
      int32_t current = GetCurrentPosition(scrollbar);
      int32_t min = GetMinPosition(scrollbar);
      int32_t max = GetMaxPosition(scrollbar);

      
      nsIFrame* parent = GetParent();
      if (parent) {
        nsCOMPtr<nsISliderListener> sliderListener = do_QueryInterface(parent->GetContent());
        if (sliderListener) {
          nsContentUtils::AddScriptRunner(
            new nsValueChangedRunnable(sliderListener, aAttribute,
                                       aAttribute == nsGkAtoms::minpos ? min : max, false));
        }
      }

      if (current < min || current > max)
      {
        int32_t direction = 0;
        if (current < min || max < min) {
          current = min;
          direction = -1;
        } else if (current > max) {
          current = max;
          direction = 1;
        }

        
        nsScrollbarFrame* scrollbarFrame = do_QueryFrame(scrollbarBox);
        if (scrollbarFrame) {
          nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
          scrollbarFrame->SetIncrementToWhole(direction);
          if (mediator) {
            mediator->ScrollByWhole(scrollbarFrame, direction,
                                    nsIScrollbarMediator::ENABLE_SNAP);
          }
        }
        

        nsContentUtils::AddScriptRunner(
          new nsSetAttrRunnable(scrollbar, nsGkAtoms::curpos, current));
      }
  }

  if (aAttribute == nsGkAtoms::minpos ||
      aAttribute == nsGkAtoms::maxpos ||
      aAttribute == nsGkAtoms::pageincrement ||
      aAttribute == nsGkAtoms::increment) {

      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }

  return rv;
}

void
nsSliderFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  if (aBuilder->IsForEventDelivery() && isDraggingThumb()) {
    
    
    aLists.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayEventReceiver(aBuilder, this));
    return;
  }
  
  nsBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

void
nsSliderFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists)
{
  
  nsIFrame* thumb = nsBox::GetChildBox(this);

  if (thumb) {
    nsRect thumbRect(thumb->GetRect());
    nsMargin m;
    thumb->GetMargin(m);
    thumbRect.Inflate(m);

    nsRect crect;
    GetClientRect(crect);

    if (crect.width < thumbRect.width || crect.height < thumbRect.height)
      return;

    
    
    
    
    
    

    uint32_t flags = 0;
    mozilla::layers::FrameMetrics::ViewID scrollTargetId =
      mozilla::layers::FrameMetrics::NULL_SCROLL_ID;
    aBuilder->GetScrollbarInfo(&scrollTargetId, &flags);
    bool thumbGetsLayer = (scrollTargetId != layers::FrameMetrics::NULL_SCROLL_ID);
    nsLayoutUtils::SetScrollbarThumbLayerization(thumb, thumbGetsLayer);

    if (thumbGetsLayer) {
      nsDisplayListCollection tempLists;
      nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, tempLists);

      
      
      nsDisplayList masterList;
      masterList.AppendToTop(tempLists.BorderBackground());
      masterList.AppendToTop(tempLists.BlockBorderBackgrounds());
      masterList.AppendToTop(tempLists.Floats());
      masterList.AppendToTop(tempLists.Content());
      masterList.AppendToTop(tempLists.PositionedDescendants());
      masterList.AppendToTop(tempLists.Outlines());

      
      aLists.Content()->AppendNewToTop(new (aBuilder)
        nsDisplayOwnLayer(aBuilder, this, &masterList, flags, scrollTargetId,
                          GetThumbRatio()));

      return;
    }
  }
  
  nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsSliderFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsIFrame* thumbBox = nsBox::GetChildBox(this);

  if (!thumbBox) {
    SyncLayout(aState);
    return NS_OK;
  }

  EnsureOrient();

#ifdef DEBUG_LAYOUT
  if (mState & NS_STATE_DEBUG_WAS_SET) {
      if (mState & NS_STATE_SET_TO_DEBUG)
          SetDebug(aState, true);
      else
          SetDebug(aState, false);
  }
#endif

  
  nsRect clientRect;
  GetClientRect(clientRect);

  
  nsIFrame* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  
  nsSize thumbSize = thumbBox->GetPrefSize(aState);

  if (IsHorizontal())
    thumbSize.height = clientRect.height;
  else
    thumbSize.width = clientRect.width;

  int32_t curPos = GetCurrentPosition(scrollbar);
  int32_t minPos = GetMinPosition(scrollbar);
  int32_t maxPos = GetMaxPosition(scrollbar);
  int32_t pageIncrement = GetPageIncrement(scrollbar);

  maxPos = std::max(minPos, maxPos);
  curPos = clamped(curPos, minPos, maxPos);

  nscoord& availableLength = IsHorizontal() ? clientRect.width : clientRect.height;
  nscoord& thumbLength = IsHorizontal() ? thumbSize.width : thumbSize.height;

  if ((pageIncrement + maxPos - minPos) > 0 && thumbBox->GetFlex(aState) > 0) {
    float ratio = float(pageIncrement) / float(maxPos - minPos + pageIncrement);
    thumbLength = std::max(thumbLength, NSToCoordRound(availableLength * ratio));
  }

  
  nsPresContext* presContext = PresContext();
  thumbLength = presContext->DevPixelsToAppUnits(
                  presContext->AppUnitsToDevPixels(thumbLength));

  
  mRatio = (minPos != maxPos) ? float(availableLength - thumbLength) / float(maxPos - minPos) : 1;

  
  
  
  bool reverse = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                         nsGkAtoms::reverse, eCaseMatters);
  nscoord pos = reverse ? (maxPos - curPos) : (curPos - minPos);

  
  nsRect thumbRect(clientRect.x, clientRect.y, thumbSize.width, thumbSize.height);
  int32_t& thumbPos = (IsHorizontal() ? thumbRect.x : thumbRect.y);
  thumbPos += NSToCoordRound(pos * mRatio);

  nsRect oldThumbRect(thumbBox->GetRect());
  LayoutChildAt(aState, thumbBox, thumbRect);

  SyncLayout(aState);

  
  if (!oldThumbRect.IsEqualInterior(thumbRect))
    Redraw(aState);

  return NS_OK;
}


nsresult
nsSliderFrame::HandleEvent(nsPresContext* aPresContext,
                           WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  
  
  if (!mContent->IsInNativeAnonymousSubtree() &&
      nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  if (!mDragFinished && !isDraggingThumb()) {
    StopDrag();
    return NS_OK;
  }

  nsIFrame* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);
  bool isHorizontal = IsHorizontal();

  if (isDraggingThumb())
  {
    switch (aEvent->message) {
    case NS_TOUCH_MOVE:
    case NS_MOUSE_MOVE: {
      nsPoint eventPoint;
      if (!GetEventPoint(aEvent, eventPoint)) {
        break;
      }
      if (mChange) {
        
        
        
#ifndef MOZ_WIDGET_GTK
        
        mDestinationPoint = eventPoint;
        StopRepeat();
        StartRepeat();
#endif
        break;
      }

      nscoord pos = isHorizontal ? eventPoint.x : eventPoint.y;

      nsIFrame* thumbFrame = mFrames.FirstChild();
      if (!thumbFrame) {
        return NS_OK;
      }

      
      pos -= mDragStart;
      bool isMouseOutsideThumb = false;
      if (gSnapMultiplier) {
        nsSize thumbSize = thumbFrame->GetSize();
        if (isHorizontal) {
          
          
          
          if (eventPoint.y < -gSnapMultiplier * thumbSize.height ||
              eventPoint.y > thumbSize.height +
                               gSnapMultiplier * thumbSize.height)
            isMouseOutsideThumb = true;
        }
        else {
          
          if (eventPoint.x < -gSnapMultiplier * thumbSize.width ||
              eventPoint.x > thumbSize.width +
                               gSnapMultiplier * thumbSize.width)
            isMouseOutsideThumb = true;
        }
      }
      if (aEvent->mClass == eTouchEventClass) {
        *aEventStatus = nsEventStatus_eConsumeNoDefault;
      }
      if (isMouseOutsideThumb)
      {
        SetCurrentThumbPosition(scrollbar, mThumbStart, false, false);
        return NS_OK;
      }

      
      SetCurrentThumbPosition(scrollbar, pos, false, true); 
    }
    break;

    case NS_TOUCH_END:
    case NS_MOUSE_BUTTON_UP:
      if (ShouldScrollForEvent(aEvent)) {
        StopDrag();
        
        return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
      }
    }

    
    return NS_OK;
  } else if (ShouldScrollToClickForEvent(aEvent)) {
    nsPoint eventPoint;
    if (!GetEventPoint(aEvent, eventPoint)) {
      return NS_OK;
    }
    nscoord pos = isHorizontal ? eventPoint.x : eventPoint.y;

    
    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      return NS_OK;
    }
    nsSize thumbSize = thumbFrame->GetSize();
    nscoord thumbLength = isHorizontal ? thumbSize.width : thumbSize.height;

    
    nsWeakFrame weakFrame(this);
    
    SetCurrentThumbPosition(scrollbar, pos - thumbLength/2, false, false);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

    DragThumb(true);

#ifdef MOZ_WIDGET_GTK
    nsCOMPtr<nsIContent> thumb = thumbFrame->GetContent();
    thumb->SetAttr(kNameSpaceID_None, nsGkAtoms::active, NS_LITERAL_STRING("true"), true);
#endif

    if (aEvent->mClass == eTouchEventClass) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
    }

    if (isHorizontal)
      mThumbStart = thumbFrame->GetPosition().x;
    else
      mThumbStart = thumbFrame->GetPosition().y;

    mDragStart = pos - mThumbStart;
  }
#ifdef MOZ_WIDGET_GTK
  else if (ShouldScrollForEvent(aEvent) &&
           aEvent->mClass == eMouseEventClass &&
           aEvent->AsMouseEvent()->button == WidgetMouseEvent::eRightButton) {
    
    
    if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
      HandlePress(aPresContext, aEvent, aEventStatus);
    } else if (aEvent->message == NS_MOUSE_BUTTON_UP) {
      HandleRelease(aPresContext, aEvent, aEventStatus);
    }

    return NS_OK;
  }
#endif

  

  

  if (aEvent->message == NS_MOUSE_EXIT_SYNTH && mChange)
     HandleRelease(aPresContext, aEvent, aEventStatus);

  return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}




bool
nsSliderFrame::GetScrollToClick()
{
  if (GetScrollbar() != this) {
    return LookAndFeel::GetInt(LookAndFeel::eIntID_ScrollToClick, false);
  }

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::movetoclick,
                            nsGkAtoms::_true, eCaseMatters)) {
    return true;
  }
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::movetoclick,
                            nsGkAtoms::_false, eCaseMatters)) {
    return false;
  }

#ifdef XP_MACOSX
  return true;
#else
  return false;
#endif
}

nsIFrame*
nsSliderFrame::GetScrollbar()
{
  
  
   nsIFrame* scrollbar;
   nsScrollbarButtonFrame::GetParentWithTag(nsGkAtoms::scrollbar, this, scrollbar);

   if (scrollbar == nullptr)
       return this;

   return scrollbar->IsBoxFrame() ? scrollbar : this;
}

void
nsSliderFrame::PageUpDown(nscoord change)
{
  
  
  
  nsIFrame* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  nscoord pageIncrement = GetPageIncrement(scrollbar);
  int32_t curpos = GetCurrentPosition(scrollbar);
  int32_t minpos = GetMinPosition(scrollbar);
  int32_t maxpos = GetMaxPosition(scrollbar);

  
  int32_t newpos = curpos + change * pageIncrement;
  if (newpos < minpos || maxpos < minpos)
    newpos = minpos;
  else if (newpos > maxpos)
    newpos = maxpos;

  SetCurrentPositionInternal(scrollbar, newpos, true);
}


void
nsSliderFrame::CurrentPositionChanged()
{
  nsIFrame* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  
  int32_t curPos = GetCurrentPosition(scrollbar);

  
  if (mCurPos == curPos)
    return;

  
  int32_t minPos = GetMinPosition(scrollbar);
  int32_t maxPos = GetMaxPosition(scrollbar);

  maxPos = std::max(minPos, maxPos);
  curPos = clamped(curPos, minPos, maxPos);

  
  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame)
    return; 

  nsRect thumbRect = thumbFrame->GetRect();

  nsRect clientRect;
  GetClientRect(clientRect);

  
  nsRect newThumbRect(thumbRect);

  bool reverse = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                         nsGkAtoms::reverse, eCaseMatters);
  nscoord pos = reverse ? (maxPos - curPos) : (curPos - minPos);

  if (IsHorizontal())
     newThumbRect.x = clientRect.x + NSToCoordRound(pos * mRatio);
  else
     newThumbRect.y = clientRect.y + NSToCoordRound(pos * mRatio);

  
  nscoord appUnitsPerPixel = PresContext()->AppUnitsPerDevPixel();
  nsRect snappedThumbRect = ToAppUnits(newThumbRect.ToNearestPixels(appUnitsPerPixel), appUnitsPerPixel);
  if (IsHorizontal()) {
    newThumbRect.x = snappedThumbRect.x;
    newThumbRect.width = snappedThumbRect.width;
  } else {
    newThumbRect.y = snappedThumbRect.y;
    newThumbRect.height = snappedThumbRect.height;
  }
  newThumbRect = newThumbRect.Intersect(clientRect);

  
  thumbFrame->SetRect(newThumbRect);

  
  SchedulePaint();

  mCurPos = curPos;

  
  nsIFrame* parent = GetParent();
  if (parent) {
    nsCOMPtr<nsISliderListener> sliderListener = do_QueryInterface(parent->GetContent());
    if (sliderListener) {
      nsContentUtils::AddScriptRunner(
        new nsValueChangedRunnable(sliderListener, nsGkAtoms::curpos, mCurPos, mUserChanged));
    }
  }
}

static void UpdateAttribute(nsIContent* aScrollbar, nscoord aNewPos, bool aNotify, bool aIsSmooth) {
  nsAutoString str;
  str.AppendInt(aNewPos);
  
  if (aIsSmooth) {
    aScrollbar->SetAttr(kNameSpaceID_None, nsGkAtoms::smooth, NS_LITERAL_STRING("true"), false);
  }
  aScrollbar->SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, str, aNotify);
  if (aIsSmooth) {
    aScrollbar->UnsetAttr(kNameSpaceID_None, nsGkAtoms::smooth, false);
  }
}




void
nsSliderFrame::SetCurrentThumbPosition(nsIContent* aScrollbar, nscoord aNewThumbPos,
                                       bool aIsSmooth, bool aMaySnap)
{
  nsRect crect;
  GetClientRect(crect);
  nscoord offset = IsHorizontal() ? crect.x : crect.y;
  int32_t newPos = NSToIntRound((aNewThumbPos - offset) / mRatio);
  
  if (aMaySnap && mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::snap,
                                        nsGkAtoms::_true, eCaseMatters)) {
    
    
    int32_t increment = GetIncrement(aScrollbar);
    newPos = NSToIntRound(newPos / float(increment)) * increment;
  }
  
  SetCurrentPosition(aScrollbar, newPos, aIsSmooth);
}





void
nsSliderFrame::SetCurrentPosition(nsIContent* aScrollbar, int32_t aNewPos,
                                  bool aIsSmooth)
{
   
  int32_t minpos = GetMinPosition(aScrollbar);
  int32_t maxpos = GetMaxPosition(aScrollbar);

  
  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                            nsGkAtoms::reverse, eCaseMatters))
    aNewPos = maxpos - aNewPos;
  else
    aNewPos += minpos;

  
  if (aNewPos < minpos || maxpos < minpos)
    aNewPos = minpos;
  else if (aNewPos > maxpos)
    aNewPos = maxpos;

  SetCurrentPositionInternal(aScrollbar, aNewPos, aIsSmooth);
}

void
nsSliderFrame::SetCurrentPositionInternal(nsIContent* aScrollbar, int32_t aNewPos,
                                          bool aIsSmooth)
{
  nsCOMPtr<nsIContent> scrollbar = aScrollbar;
  nsIFrame* scrollbarBox = GetScrollbar();
  nsWeakFrame weakFrame(this);

  mUserChanged = true;

  nsScrollbarFrame* scrollbarFrame = do_QueryFrame(scrollbarBox);
  if (scrollbarFrame) {
    
    nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
    if (mediator) {
      nsCOMPtr<nsIContent> content = GetContent();
      nscoord oldPos = nsPresContext::CSSPixelsToAppUnits(GetCurrentPosition(scrollbar));
      nscoord newPos = nsPresContext::CSSPixelsToAppUnits(aNewPos);
      mediator->ThumbMoved(scrollbarFrame, oldPos, newPos);
      if (!weakFrame.IsAlive()) {
        return;
      }
      CurrentPositionChanged();
      mUserChanged = false;
      return;
    }
  }

  UpdateAttribute(scrollbar, aNewPos, true, aIsSmooth);
  if (!weakFrame.IsAlive()) {
    return;
  }
  mUserChanged = false;

#ifdef DEBUG_SLIDER
  printf("Current Pos=%d\n",aNewPos);
#endif

}

nsIAtom*
nsSliderFrame::GetType() const
{
  return nsGkAtoms::sliderFrame;
}

void
nsSliderFrame::SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList)
{
  nsBoxFrame::SetInitialChildList(aListID, aChildList);
  AddListener();
}

nsresult
nsSliderMediator::HandleEvent(nsIDOMEvent* aEvent)
{
  
  if (mSlider && !mSlider->isDraggingThumb())
    return mSlider->StartDrag(aEvent);

  return NS_OK;
}

nsresult
nsSliderFrame::StartDrag(nsIDOMEvent* aEvent)
{
#ifdef DEBUG_SLIDER
  printf("Begin dragging\n");
#endif
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return NS_OK;

  WidgetGUIEvent* event = aEvent->GetInternalNSEvent()->AsGUIEvent();

  if (!ShouldScrollForEvent(event)) {
    return NS_OK;
  }

  nsPoint pt;
  if (!GetEventPoint(event, pt)) {
    return NS_OK;
  }
  bool isHorizontal = IsHorizontal();
  nscoord pos = isHorizontal ? pt.x : pt.y;

  
  
  nsCOMPtr<nsIContent> scrollbar;
  nscoord newpos = pos;
  bool scrollToClick = ShouldScrollToClickForEvent(event);
  if (scrollToClick) {
    
    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      return NS_OK;
    }
    nsSize thumbSize = thumbFrame->GetSize();
    nscoord thumbLength = isHorizontal ? thumbSize.width : thumbSize.height;

    newpos -= (thumbLength/2);

    nsIFrame* scrollbarBox = GetScrollbar();
    scrollbar = GetContentOfBox(scrollbarBox);
  }

  DragThumb(true);

  if (scrollToClick) {
    
    SetCurrentThumbPosition(scrollbar, newpos, false, false);
  }

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) {
    return NS_OK;
  }

#ifdef MOZ_WIDGET_GTK
  nsCOMPtr<nsIContent> thumb = thumbFrame->GetContent();
  thumb->SetAttr(kNameSpaceID_None, nsGkAtoms::active, NS_LITERAL_STRING("true"), true);
#endif

  if (isHorizontal)
    mThumbStart = thumbFrame->GetPosition().x;
  else
    mThumbStart = thumbFrame->GetPosition().y;

  mDragStart = pos - mThumbStart;

#ifdef DEBUG_SLIDER
  printf("Pressed mDragStart=%d\n",mDragStart);
#endif

  return NS_OK;
}

nsresult
nsSliderFrame::StopDrag()
{
  AddListener();
  DragThumb(false);

#ifdef MOZ_WIDGET_GTK
  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (thumbFrame) {
    nsCOMPtr<nsIContent> thumb = thumbFrame->GetContent();
    thumb->UnsetAttr(kNameSpaceID_None, nsGkAtoms::active, true);
  }
#endif

  if (mChange) {
    StopRepeat();
    mChange = 0;
  }
  return NS_OK;
}

void
nsSliderFrame::DragThumb(bool aGrabMouseEvents)
{
  mDragFinished = !aGrabMouseEvents;

  
  nsIFrame* parent = GetParent();
  if (parent) {
    nsCOMPtr<nsISliderListener> sliderListener = do_QueryInterface(parent->GetContent());
    if (sliderListener) {
      nsContentUtils::AddScriptRunner(
        new nsDragStateChangedRunnable(sliderListener, aGrabMouseEvents));
    }
  }

  nsIPresShell::SetCapturingContent(aGrabMouseEvents ? GetContent() : nullptr,
                                    aGrabMouseEvents ? CAPTURE_IGNOREALLOWED : 0);
}

bool
nsSliderFrame::isDraggingThumb()
{
  return (nsIPresShell::GetCapturingContent() == GetContent());
}

void
nsSliderFrame::AddListener()
{
  if (!mMediator) {
    mMediator = new nsSliderMediator(this);
  }

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) {
    return;
  }
  thumbFrame->GetContent()->
    AddSystemEventListener(NS_LITERAL_STRING("mousedown"), mMediator,
                           false, false);
  thumbFrame->GetContent()->
    AddSystemEventListener(NS_LITERAL_STRING("touchstart"), mMediator,
                           false, false);
}

void
nsSliderFrame::RemoveListener()
{
  NS_ASSERTION(mMediator, "No listener was ever added!!");

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame)
    return;

  thumbFrame->GetContent()->
    RemoveSystemEventListener(NS_LITERAL_STRING("mousedown"), mMediator, false);
}

bool
nsSliderFrame::ShouldScrollForEvent(WidgetGUIEvent* aEvent)
{
  switch (aEvent->message) {
    case NS_TOUCH_START:
    case NS_TOUCH_END:
      return true;
    case NS_MOUSE_BUTTON_DOWN:
    case NS_MOUSE_BUTTON_UP: {
      uint16_t button = aEvent->AsMouseEvent()->button;
#ifdef MOZ_WIDGET_GTK
      return (button == WidgetMouseEvent::eLeftButton) ||
             (button == WidgetMouseEvent::eRightButton && GetScrollToClick()) ||
             (button == WidgetMouseEvent::eMiddleButton && gMiddlePref && !GetScrollToClick());
#else
      return (button == WidgetMouseEvent::eLeftButton) ||
             (button == WidgetMouseEvent::eMiddleButton && gMiddlePref);
#endif
    }
    default:
      return false;
  }
}

bool
nsSliderFrame::ShouldScrollToClickForEvent(WidgetGUIEvent* aEvent)
{
  if (!ShouldScrollForEvent(aEvent)) {
    return false;
  }

  if (aEvent->message == NS_TOUCH_START) {
    return GetScrollToClick();
  }

  if (aEvent->message != NS_MOUSE_BUTTON_DOWN) {
    return false;
  }

#if defined(XP_MACOSX) || defined(MOZ_WIDGET_GTK)
  
  if (IsEventOverThumb(aEvent)) {
    return false;
  }
#endif

  WidgetMouseEvent* mouseEvent = aEvent->AsMouseEvent();
  if (mouseEvent->button == WidgetMouseEvent::eLeftButton) {
#ifdef XP_MACOSX
    bool invertPref = mouseEvent->IsAlt();
#else
    bool invertPref = mouseEvent->IsShift();
#endif
    return GetScrollToClick() != invertPref;
  }

#ifdef MOZ_WIDGET_GTK
  if (mouseEvent->button == WidgetMouseEvent::eRightButton) {
    return !GetScrollToClick();
  }
#endif

  return true;
}

bool
nsSliderFrame::IsEventOverThumb(WidgetGUIEvent* aEvent)
{
  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) {
    return false;
  }

  nsPoint eventPoint;
  if (!GetEventPoint(aEvent, eventPoint)) {
    return false;
  }

  bool isHorizontal = IsHorizontal();
  nsRect thumbRect = thumbFrame->GetRect();
  nscoord eventPos = isHorizontal ? eventPoint.x : eventPoint.y;
  nscoord thumbStart = isHorizontal ? thumbRect.x : thumbRect.y;
  nscoord thumbEnd = isHorizontal ? thumbRect.XMost() : thumbRect.YMost();

  return eventPos >= thumbStart && eventPos < thumbEnd;
}

NS_IMETHODIMP
nsSliderFrame::HandlePress(nsPresContext* aPresContext,
                           WidgetGUIEvent* aEvent,
                           nsEventStatus* aEventStatus)
{
  if (!ShouldScrollForEvent(aEvent) || ShouldScrollToClickForEvent(aEvent)) {
    return NS_OK;
  }

  if (IsEventOverThumb(aEvent)) {
    return NS_OK;
  }

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) 
    return NS_OK;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return NS_OK;
  
  nsRect thumbRect = thumbFrame->GetRect();
  
  nscoord change = 1;
  nsPoint eventPoint;
  if (!GetEventPoint(aEvent, eventPoint)) {
    return NS_OK;
  }
  if (IsHorizontal() ? eventPoint.x < thumbRect.x 
                     : eventPoint.y < thumbRect.y)
    change = -1;

  mChange = change;
  DragThumb(true);
  
  
  
  
#ifdef MOZ_WIDGET_GTK
  nsRect clientRect;
  GetClientRect(clientRect);

  
  
  if (change > 0) {
    mDestinationPoint = nsPoint(clientRect.width, clientRect.height);
  }
  else {
    mDestinationPoint = nsPoint(0, 0);
  }
#else
  mDestinationPoint = eventPoint;
#endif
  StartRepeat();
  PageScroll(change);

  return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame::HandleRelease(nsPresContext* aPresContext,
                             WidgetGUIEvent* aEvent,
                             nsEventStatus* aEventStatus)
{
  StopRepeat();

  nsIFrame* scrollbar = GetScrollbar();
  nsScrollbarFrame* sb = do_QueryFrame(scrollbar);
  if (sb) {
    nsIScrollbarMediator* m = sb->GetScrollbarMediator();
    if (m) {
      m->ScrollbarReleased(sb);
    }
  }
  return NS_OK;
}

void
nsSliderFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (mMediator) {
    mMediator->SetSlider(nullptr);
    mMediator = nullptr;
  }
  StopRepeat();

  
  nsBoxFrame::DestroyFrom(aDestructRoot);
}

nsSize
nsSliderFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  EnsureOrient();
  return nsBoxFrame::GetPrefSize(aState);
}

nsSize
nsSliderFrame::GetMinSize(nsBoxLayoutState& aState)
{
  EnsureOrient();

  
  return nsBox::GetMinSize(aState);
}

nsSize
nsSliderFrame::GetMaxSize(nsBoxLayoutState& aState)
{
  EnsureOrient();
  return nsBoxFrame::GetMaxSize(aState);
}

void
nsSliderFrame::EnsureOrient()
{
  nsIFrame* scrollbarBox = GetScrollbar();

  bool isHorizontal = (scrollbarBox->GetStateBits() & NS_STATE_IS_HORIZONTAL) != 0;
  if (isHorizontal)
      mState |= NS_STATE_IS_HORIZONTAL;
  else
      mState &= ~NS_STATE_IS_HORIZONTAL;
}


void
nsSliderFrame::Notify(void)
{
    bool stop = false;

    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      StopRepeat();
      return;
    }
    nsRect thumbRect = thumbFrame->GetRect();

    bool isHorizontal = IsHorizontal();

    
    
    if (isHorizontal) {
        if (mChange < 0) {
            if (thumbRect.x < mDestinationPoint.x)
                stop = true;
        } else {
            if (thumbRect.x + thumbRect.width > mDestinationPoint.x)
                stop = true;
        }
    } else {
         if (mChange < 0) {
            if (thumbRect.y < mDestinationPoint.y)
                stop = true;
        } else {
            if (thumbRect.y + thumbRect.height > mDestinationPoint.y)
                stop = true;
        }
    }


    if (stop) {
      StopRepeat();
    } else {
      PageScroll(mChange);
    }
}

void
nsSliderFrame::PageScroll(nscoord aChange)
{
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                            nsGkAtoms::reverse, eCaseMatters)) {
    aChange = -aChange;
  }
  nsIFrame* scrollbar = GetScrollbar();
  nsScrollbarFrame* sb = do_QueryFrame(scrollbar);
  if (sb) {
    nsIScrollbarMediator* m = sb->GetScrollbarMediator();
    sb->SetIncrementToPage(aChange);
    if (m) {
      m->ScrollByPage(sb, aChange, nsIScrollbarMediator::ENABLE_SNAP);
      return;
    }
  }
  PageUpDown(aChange);
}

float
nsSliderFrame::GetThumbRatio() const
{
  
  
  return mRatio / mozilla::AppUnitsPerCSSPixel();
}

NS_IMPL_ISUPPORTS(nsSliderMediator,
                  nsIDOMEventListener)
