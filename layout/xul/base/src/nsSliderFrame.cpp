











#include "nsSliderFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsDOMTouchEvent.h"
#include "nsEventListenerManager.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsScrollbarButtonFrame.h"
#include "nsISliderListener.h"
#include "nsIScrollbarMediator.h"
#include "nsScrollbarFrame.h"
#include "nsRepeatService.h"
#include "nsBoxLayoutState.h"
#include "nsSprocketLayout.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "mozilla/Preferences.h"
#include "mozilla/LookAndFeel.h"
#include <algorithm>

using namespace mozilla;

bool nsSliderFrame::gMiddlePref = false;
int32_t nsSliderFrame::gSnapMultiplier;


#undef DEBUG_SLIDER

static already_AddRefed<nsIContent>
GetContentOfBox(nsIFrame *aBox)
{
  nsIContent* content = aBox->GetContent();
  NS_IF_ADDREF(content);
  return content;
}

nsIFrame*
NS_NewSliderFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSliderFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSliderFrame)

nsSliderFrame::nsSliderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext):
  nsBoxFrame(aPresShell, aContext),
  mCurPos(0),
  mChange(0),
  mUserChanged(false)
{
}


nsSliderFrame::~nsSliderFrame()
{
}

NS_IMETHODIMP
nsSliderFrame::Init(nsIContent*      aContent,
                    nsIFrame*        aParent,
                    nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  static bool gotPrefs = false;
  if (!gotPrefs) {
    gotPrefs = true;

    gMiddlePref = Preferences::GetBool("middlemouse.scrollbarPosition");
    gSnapMultiplier = Preferences::GetInt("slider.snapMultiplier");
  }

  mCurPos = GetCurrentPosition(aContent);

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame)
{
  nsresult rv = nsBoxFrame::RemoveFrame(aListID, aOldFrame);
  if (mFrames.IsEmpty())
    RemoveListener();

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  bool wasEmpty = mFrames.IsEmpty();
  nsresult rv = nsBoxFrame::InsertFrames(aListID, aPrevFrame, aFrameList);
  if (wasEmpty)
    AddListener();

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList)
{
  
  
  bool wasEmpty = mFrames.IsEmpty();
  nsresult rv = nsBoxFrame::AppendFrames(aListID, aFrameList);
  if (wasEmpty)
    AddListener();

  return rv;
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

NS_IMETHODIMP
nsSliderFrame::AttributeChanged(int32_t aNameSpaceID,
                                nsIAtom* aAttribute,
                                int32_t aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::curpos) {
     rv = CurrentPositionChanged(PresContext(), false);
     NS_ASSERTION(NS_SUCCEEDED(rv), "failed to change position");
     if (NS_FAILED(rv))
        return rv;
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
        if (current < min || max < min)
            current = min;
        else if (current > max)
            current = max;

        
        nsScrollbarFrame* scrollbarFrame = do_QueryFrame(scrollbarBox);
        if (scrollbarFrame) {
          nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
          if (mediator) {
            mediator->PositionChanged(scrollbarFrame, GetCurrentPosition(scrollbar), current);
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

NS_IMETHODIMP
nsSliderFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  if (aBuilder->IsForEventDelivery() && isDraggingThumb()) {
    
    
    aLists.Outlines()->AppendNewToTop(new (aBuilder)
      nsDisplayEventReceiver(aBuilder, this));
    return NS_OK;
  }
  
  nsBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists)
{
  
  nsIFrame* thumb = GetChildBox();

  if (thumb) {
    nsRect thumbRect(thumb->GetRect());
    nsMargin m;
    thumb->GetMargin(m);
    thumbRect.Inflate(m);

    nsRect crect;
    GetClientRect(crect);

    if (crect.width < thumbRect.width || crect.height < thumbRect.height)
      return NS_OK;
  }
  
  return nsBoxFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsSliderFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  nsIFrame* thumbBox = GetChildBox();

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


NS_IMETHODIMP
nsSliderFrame::HandleEvent(nsPresContext* aPresContext,
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);

  
  
  if (!mContent->IsInNativeAnonymousSubtree() &&
      nsEventStatus_eConsumeNoDefault == *aEventStatus) {
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
        
        
        
        mDestinationPoint = eventPoint;
        StopRepeat();
        StartRepeat();
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
      if (aEvent->eventStructType == NS_TOUCH_EVENT) {
        *aEventStatus = nsEventStatus_eConsumeNoDefault;
      }
      if (isMouseOutsideThumb)
      {
        SetCurrentThumbPosition(scrollbar, mThumbStart, false, true, false);
        return NS_OK;
      }

      
      SetCurrentThumbPosition(scrollbar, pos, false, true, true); 
    }
    break;

    case NS_TOUCH_END:
    case NS_MOUSE_BUTTON_UP:
      if (aEvent->message == NS_TOUCH_END ||
          static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton ||
          (static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eMiddleButton &&
           gMiddlePref)) {
        
        AddListener();
        DragThumb(false);
        if (mChange) {
          StopRepeat();
          mChange = 0;
        }
        
        return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
      }
    }

    
    return NS_OK;
  } else if ((aEvent->message == NS_MOUSE_BUTTON_DOWN &&
              static_cast<nsMouseEvent*>(aEvent)->button ==
                nsMouseEvent::eLeftButton &&
#ifdef XP_MACOSX
              
              (static_cast<nsMouseEvent*>(aEvent)->IsAlt() != GetScrollToClick())) ||
#else
              (static_cast<nsMouseEvent*>(aEvent)->IsShift() != GetScrollToClick())) ||
#endif
             (gMiddlePref && aEvent->message == NS_MOUSE_BUTTON_DOWN &&
              static_cast<nsMouseEvent*>(aEvent)->button ==
                nsMouseEvent::eMiddleButton) ||
             (aEvent->message == NS_TOUCH_START && GetScrollToClick())) {
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
    
    SetCurrentThumbPosition(scrollbar, pos - thumbLength/2, false, false, false);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

    DragThumb(true);
    if (aEvent->eventStructType == NS_TOUCH_EVENT) {
      *aEventStatus = nsEventStatus_eConsumeNoDefault;
    }

    if (isHorizontal)
      mThumbStart = thumbFrame->GetPosition().x;
    else
      mThumbStart = thumbFrame->GetPosition().y;

    mDragStart = pos - mThumbStart;
  }

  

  

  if (aEvent->message == NS_MOUSE_EXIT_SYNTH && mChange)
     HandleRelease(aPresContext, aEvent, aEventStatus);

  return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
}




bool
nsSliderFrame::GetScrollToClick()
{
  
  
  
  if (GetScrollbar() == this)
#ifdef XP_MACOSX
    return !mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::movetoclick,
                                   nsGkAtoms::_false, eCaseMatters);
 
  
  bool scrollToClick = false;
  int32_t scrollToClickMetric;
  nsresult rv = LookAndFeel::GetInt(LookAndFeel::eIntID_ScrollToClick,
                                    &scrollToClickMetric);
  if (NS_SUCCEEDED(rv) && scrollToClickMetric == 1)
    scrollToClick = true;
  return scrollToClick;

#else
    return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::movetoclick,
                                  nsGkAtoms::_true, eCaseMatters);
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

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                            nsGkAtoms::reverse, eCaseMatters))
    change = -change;

  nscoord pageIncrement = GetPageIncrement(scrollbar);
  int32_t curpos = GetCurrentPosition(scrollbar);
  int32_t minpos = GetMinPosition(scrollbar);
  int32_t maxpos = GetMaxPosition(scrollbar);

  
  int32_t newpos = curpos + change * pageIncrement;
  if (newpos < minpos || maxpos < minpos)
    newpos = minpos;
  else if (newpos > maxpos)
    newpos = maxpos;

  SetCurrentPositionInternal(scrollbar, newpos, true, false);
}


nsresult
nsSliderFrame::CurrentPositionChanged(nsPresContext* aPresContext,
                                      bool aImmediateRedraw)
{
  nsIFrame* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  
  int32_t curPos = GetCurrentPosition(scrollbar);

  
  if (mCurPos == curPos)
      return NS_OK;

  
  int32_t minPos = GetMinPosition(scrollbar);
  int32_t maxPos = GetMaxPosition(scrollbar);

  maxPos = std::max(minPos, maxPos);
  curPos = clamped(curPos, minPos, maxPos);

  
  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame)
    return NS_OK; 

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

  return NS_OK;
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
                                       bool aIsSmooth, bool aImmediateRedraw, bool aMaySnap)
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
  
  SetCurrentPosition(aScrollbar, newPos, aIsSmooth, aImmediateRedraw);
}





void
nsSliderFrame::SetCurrentPosition(nsIContent* aScrollbar, int32_t aNewPos,
                                  bool aIsSmooth, bool aImmediateRedraw)
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

  SetCurrentPositionInternal(aScrollbar, aNewPos, aIsSmooth, aImmediateRedraw);
}

void
nsSliderFrame::SetCurrentPositionInternal(nsIContent* aScrollbar, int32_t aNewPos,
                                          bool aIsSmooth,
                                          bool aImmediateRedraw)
{
  nsCOMPtr<nsIContent> scrollbar = aScrollbar;
  nsIFrame* scrollbarBox = GetScrollbar();

  mUserChanged = true;

  nsScrollbarFrame* scrollbarFrame = do_QueryFrame(scrollbarBox);
  if (scrollbarFrame) {
    
    nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
    if (mediator) {
      nsRefPtr<nsPresContext> context = PresContext();
      nsCOMPtr<nsIContent> content = GetContent();
      mediator->PositionChanged(scrollbarFrame, GetCurrentPosition(scrollbar), aNewPos);
      
      UpdateAttribute(scrollbar, aNewPos, false, aIsSmooth);
      nsIFrame* frame = content->GetPrimaryFrame();
      if (frame && frame->GetType() == nsGkAtoms::sliderFrame) {
        static_cast<nsSliderFrame*>(frame)->
          CurrentPositionChanged(frame->PresContext(), aImmediateRedraw);
      }
      mUserChanged = false;
      return;
    }
  }

  UpdateAttribute(scrollbar, aNewPos, true, aIsSmooth);
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

NS_IMETHODIMP
nsSliderFrame::SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList)
{
  nsresult r = nsBoxFrame::SetInitialChildList(aListID, aChildList);

  AddListener();

  return r;
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

  bool isHorizontal = IsHorizontal();
  bool scrollToClick = false;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aEvent));
  if (mouseEvent) {
    uint16_t button = 0;
    mouseEvent->GetButton(&button);
    if (!(button == 0 || (button == 1 && gMiddlePref)))
      return NS_OK;
  
#ifndef XP_MACOSX
    
    mouseEvent->GetShiftKey(&scrollToClick);
    if (button != 0) {
      scrollToClick = true;
    }
#endif
  }

  nsGUIEvent *event = static_cast<nsGUIEvent*>(aEvent->GetInternalNSEvent());

  nsPoint pt;
  if (!GetEventPoint(event, pt)) {
    return NS_OK;
  }
  nscoord pos = isHorizontal ? pt.x : pt.y;

  
  
  nsCOMPtr<nsIContent> scrollbar;
  nscoord newpos = pos;
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
    
    SetCurrentThumbPosition(scrollbar, newpos, false, false, false);
  }

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) {
    return NS_OK;
  }

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

void
nsSliderFrame::DragThumb(bool aGrabMouseEvents)
{
  
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

NS_IMETHODIMP
nsSliderFrame::HandlePress(nsPresContext* aPresContext,
                           nsGUIEvent*     aEvent,
                           nsEventStatus*  aEventStatus)
{
  if (aEvent->message == NS_TOUCH_START && GetScrollToClick()) {
    return NS_OK;
  }

  if (aEvent->message == NS_MOUSE_BUTTON_DOWN) {
#ifdef XP_MACOSX
    
    if (((nsMouseEvent *)aEvent)->IsAlt() != GetScrollToClick()) {
#else
    if (((nsMouseEvent *)aEvent)->IsShift() != GetScrollToClick()) {
#endif
      return NS_OK;
    }
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
  mDestinationPoint = eventPoint;
  StartRepeat();
  PageUpDown(change);
  return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame::HandleRelease(nsPresContext* aPresContext,
                                 nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  StopRepeat();

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


void nsSliderFrame::Notify(void)
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
      PageUpDown(mChange);
    }
}

NS_IMPL_ISUPPORTS1(nsSliderMediator,
                   nsIDOMEventListener)
