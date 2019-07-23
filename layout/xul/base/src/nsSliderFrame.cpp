













































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
#include "nsIDOMEventTarget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDocument.h"
#include "nsScrollbarButtonFrame.h"
#include "nsISliderListener.h"
#include "nsIScrollbarMediator.h"
#include "nsIScrollbarFrame.h"
#include "nsILookAndFeel.h"
#include "nsRepeatService.h"
#include "nsBoxLayoutState.h"
#include "nsSprocketLayout.h"
#include "nsIServiceManager.h"
#include "nsGUIEvent.h"
#include "nsContentUtils.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"

PRBool nsSliderFrame::gMiddlePref = PR_FALSE;
PRInt32 nsSliderFrame::gSnapMultiplier;


#undef DEBUG_SLIDER

static already_AddRefed<nsIContent>
GetContentOfBox(nsIBox *aBox)
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

nsSliderFrame::nsSliderFrame(nsIPresShell* aPresShell, nsStyleContext* aContext):
  nsBoxFrame(aPresShell, aContext),
  mCurPos(0),
  mChange(0),
  mUserChanged(PR_FALSE)
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

  static PRBool gotPrefs = PR_FALSE;
  if (!gotPrefs) {
    gotPrefs = PR_TRUE;

    gMiddlePref = nsContentUtils::GetBoolPref("middlemouse.scrollbarPosition");
    gSnapMultiplier = nsContentUtils::GetIntPref("slider.snapMultiplier");
  }

  mCurPos = GetCurrentPosition(aContent);

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::RemoveFrame(nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  nsresult rv = nsBoxFrame::RemoveFrame(aListName, aOldFrame);
  if (mFrames.IsEmpty())
    RemoveListener();

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::InsertFrames(nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList)
{
  PRBool wasEmpty = mFrames.IsEmpty();
  nsresult rv = nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
  if (wasEmpty)
    AddListener();

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::AppendFrames(nsIAtom*        aListName,
                            nsFrameList&    aFrameList)
{
  
  
  PRBool wasEmpty = mFrames.IsEmpty();
  nsresult rv = nsBoxFrame::AppendFrames(aListName, aFrameList);
  if (wasEmpty)
    AddListener();

  return rv;
}

PRInt32
nsSliderFrame::GetCurrentPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::curpos, 0);
}

PRInt32
nsSliderFrame::GetMinPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::minpos, 0);
}

PRInt32
nsSliderFrame::GetMaxPosition(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::maxpos, 100);
}

PRInt32
nsSliderFrame::GetIncrement(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::increment, 1);
}


PRInt32
nsSliderFrame::GetPageIncrement(nsIContent* content)
{
  return GetIntegerAttribute(content, nsGkAtoms::pageincrement, 10);
}

PRInt32
nsSliderFrame::GetIntegerAttribute(nsIContent* content, nsIAtom* atom, PRInt32 defaultValue)
{
    nsAutoString value;
    content->GetAttr(kNameSpaceID_None, atom, value);
    if (!value.IsEmpty()) {
      PRInt32 error;

      
      defaultValue = value.ToInteger(&error);
    }

    return defaultValue;
}

class nsValueChangedRunnable : public nsRunnable
{
public:
  nsValueChangedRunnable(nsISliderListener* aListener,
                         nsIAtom* aWhich,
                         PRInt32 aValue,
                         PRBool aUserChanged)
  : mListener(aListener), mWhich(aWhich),
    mValue(aValue), mUserChanged(aUserChanged)
  {}

  NS_IMETHODIMP Run()
  {
    nsAutoString which;
    mWhich->ToString(which);
    return mListener->ValueChanged(which, mValue, mUserChanged);
  }

  nsCOMPtr<nsISliderListener> mListener;
  nsCOMPtr<nsIAtom> mWhich;
  PRInt32 mValue;
  PRBool mUserChanged;
};

class nsDragStateChangedRunnable : public nsRunnable
{
public:
  nsDragStateChangedRunnable(nsISliderListener* aListener,
                             PRBool aDragBeginning)
  : mListener(aListener),
    mDragBeginning(aDragBeginning)
  {}

  NS_IMETHODIMP Run()
  {
    return mListener->DragStateChanged(mDragBeginning);
  }

  nsCOMPtr<nsISliderListener> mListener;
  PRBool mDragBeginning;
};

NS_IMETHODIMP
nsSliderFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute,
                                PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::curpos) {
     rv = CurrentPositionChanged(PresContext(), PR_FALSE);
     NS_ASSERTION(NS_SUCCEEDED(rv), "failed to change position");
     if (NS_FAILED(rv))
        return rv;
  } else if (aAttribute == nsGkAtoms::minpos ||
             aAttribute == nsGkAtoms::maxpos) {
      

      nsIBox* scrollbarBox = GetScrollbar();
      nsCOMPtr<nsIContent> scrollbar;
      scrollbar = GetContentOfBox(scrollbarBox);
      PRInt32 current = GetCurrentPosition(scrollbar);
      PRInt32 min = GetMinPosition(scrollbar);
      PRInt32 max = GetMaxPosition(scrollbar);

      
      nsIFrame* parent = GetParent();
      if (parent) {
        nsCOMPtr<nsISliderListener> sliderListener = do_QueryInterface(parent->GetContent());
        if (sliderListener) {
          nsContentUtils::AddScriptRunner(
            new nsValueChangedRunnable(sliderListener, aAttribute,
                                       aAttribute == nsGkAtoms::minpos ? min : max, PR_FALSE));
        }
      }

      if (current < min || current > max)
      {
        if (current < min || max < min)
            current = min;
        else if (current > max)
            current = max;

        
        nsIScrollbarFrame* scrollbarFrame = do_QueryFrame(scrollbarBox);
        if (scrollbarFrame) {
          nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
          if (mediator) {
            mediator->PositionChanged(scrollbarFrame, GetCurrentPosition(scrollbar), current);
          }
        }

        nsAutoString currentStr;
        currentStr.AppendInt(current);
        nsContentUtils::AddScriptRunner(
          new nsSetAttrRunnable(scrollbar, nsGkAtoms::curpos, currentStr));
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
    
    
    return aLists.Outlines()->AppendNewToTop(new (aBuilder)
        nsDisplayEventReceiver(this));
  }
  
  return nsBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsSliderFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists)
{
  
  nsIBox* thumb = GetChildBox();

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
  
  nsIBox* thumbBox = GetChildBox();

  if (!thumbBox) {
    SyncLayout(aState);
    return NS_OK;
  }

  EnsureOrient();

#ifdef DEBUG_LAYOUT
  if (mState & NS_STATE_DEBUG_WAS_SET) {
      if (mState & NS_STATE_SET_TO_DEBUG)
          SetDebug(aState, PR_TRUE);
      else
          SetDebug(aState, PR_FALSE);
  }
#endif

  
  nsRect clientRect;
  GetClientRect(clientRect);

  
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  
  nsSize thumbSize = thumbBox->GetPrefSize(aState);

  if (IsHorizontal())
    thumbSize.height = clientRect.height;
  else
    thumbSize.width = clientRect.width;

  PRInt32 curPos = GetCurrentPosition(scrollbar);
  PRInt32 minPos = GetMinPosition(scrollbar);
  PRInt32 maxPos = GetMaxPosition(scrollbar);
  PRInt32 pageIncrement = GetPageIncrement(scrollbar);

  maxPos = PR_MAX(minPos, maxPos);
  curPos = PR_MAX(minPos, PR_MIN(curPos, maxPos));

  nscoord& availableLength = IsHorizontal() ? clientRect.width : clientRect.height;
  nscoord& thumbLength = IsHorizontal() ? thumbSize.width : thumbSize.height;

  if ((pageIncrement + maxPos - minPos) > 0 && thumbBox->GetFlex(aState) > 0) {
    float ratio = float(pageIncrement) / float(maxPos - minPos + pageIncrement);
    thumbLength = PR_MAX(thumbLength, NSToCoordRound(availableLength * ratio));
  }

  
  nsPresContext* presContext = PresContext();
  thumbLength = presContext->DevPixelsToAppUnits(
                  presContext->AppUnitsToDevPixels(thumbLength));

  
  mRatio = (minPos != maxPos) ? float(availableLength - thumbLength) / float(maxPos - minPos) : 1;

  
  
  
  PRBool reverse = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                         nsGkAtoms::reverse, eCaseMatters);
  nscoord pos = reverse ? (maxPos - curPos) : (curPos - minPos);

  
  nsRect thumbRect(clientRect.x, clientRect.y, thumbSize.width, thumbSize.height);
  PRInt32& thumbPos = (IsHorizontal() ? thumbRect.x : thumbRect.y);
  thumbPos += NSToCoordRound(pos * mRatio);

  nsRect oldThumbRect(thumbBox->GetRect());
  LayoutChildAt(aState, thumbBox, thumbRect);

  SyncLayout(aState);

  
  if (oldThumbRect != thumbRect)
    Redraw(aState);

  return NS_OK;
}


NS_IMETHODIMP
nsSliderFrame::HandleEvent(nsPresContext* aPresContext,
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);
  PRBool isHorizontal = IsHorizontal();

  if (isDraggingThumb())
  {
    switch (aEvent->message) {
    case NS_MOUSE_MOVE: {
      nsPoint eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                         this);
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
      PRBool isMouseOutsideThumb = PR_FALSE;
      if (gSnapMultiplier) {
        nsSize thumbSize = thumbFrame->GetSize();
        if (isHorizontal) {
          
          
          
          if (eventPoint.y < -gSnapMultiplier * thumbSize.height ||
              eventPoint.y > thumbSize.height +
                               gSnapMultiplier * thumbSize.height)
            isMouseOutsideThumb = PR_TRUE;
        }
        else {
          
          if (eventPoint.x < -gSnapMultiplier * thumbSize.width ||
              eventPoint.x > thumbSize.width +
                               gSnapMultiplier * thumbSize.width)
            isMouseOutsideThumb = PR_TRUE;
        }
      }
      if (isMouseOutsideThumb)
      {
        SetCurrentThumbPosition(scrollbar, mThumbStart, PR_FALSE, PR_TRUE, PR_FALSE);
        return NS_OK;
      }

      
      SetCurrentThumbPosition(scrollbar, pos, PR_FALSE, PR_TRUE, PR_TRUE); 
    }
    break;

    case NS_MOUSE_BUTTON_UP:
      if (static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eLeftButton ||
          (static_cast<nsMouseEvent*>(aEvent)->button == nsMouseEvent::eMiddleButton &&
           gMiddlePref)) {
        
        AddListener();
        DragThumb(PR_FALSE);
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
              
              (static_cast<nsMouseEvent*>(aEvent)->isAlt != GetScrollToClick())) ||
#else
              (static_cast<nsMouseEvent*>(aEvent)->isShift != GetScrollToClick())) ||
#endif
             (gMiddlePref && aEvent->message == NS_MOUSE_BUTTON_DOWN &&
              static_cast<nsMouseEvent*>(aEvent)->button ==
                nsMouseEvent::eMiddleButton)) {

    nsPoint eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                      this);
    nscoord pos = isHorizontal ? eventPoint.x : eventPoint.y;

    
    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      return NS_OK;
    }
    nsSize thumbSize = thumbFrame->GetSize();
    nscoord thumbLength = isHorizontal ? thumbSize.width : thumbSize.height;

    
    nsWeakFrame weakFrame(this);
    
    SetCurrentThumbPosition(scrollbar, pos - thumbLength/2, PR_FALSE, PR_FALSE, PR_FALSE);
    NS_ENSURE_TRUE(weakFrame.IsAlive(), NS_OK);

    DragThumb(PR_TRUE);

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




PRBool
nsSliderFrame::GetScrollToClick()
{
  
  
  
  if (GetScrollbar() == this)
#ifdef XP_MACOSX
    return !mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::movetoclick,
                                   nsGkAtoms::_false, eCaseMatters);
 
  
  PRBool scrollToClick = PR_FALSE;
  nsresult rv;
  nsCOMPtr<nsILookAndFeel> lookNFeel =
    do_GetService("@mozilla.org/widget/lookandfeel;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    PRInt32 scrollToClickMetric;
    rv = lookNFeel->GetMetric(nsILookAndFeel::eMetric_ScrollToClick,
                              scrollToClickMetric);
    if (NS_SUCCEEDED(rv) && scrollToClickMetric == 1)
      scrollToClick = PR_TRUE;
  }
  return scrollToClick;

#else
    return mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::movetoclick,
                                  nsGkAtoms::_true, eCaseMatters);
  return PR_FALSE;
#endif
}

nsIBox*
nsSliderFrame::GetScrollbar()
{
  
  
   nsIFrame* scrollbar;
   nsScrollbarButtonFrame::GetParentWithTag(nsGkAtoms::scrollbar, this, scrollbar);

   if (scrollbar == nsnull)
       return this;

   return scrollbar->IsBoxFrame() ? scrollbar : this;
}

void
nsSliderFrame::PageUpDown(nscoord change)
{
  
  
  
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                            nsGkAtoms::reverse, eCaseMatters))
    change = -change;

  nscoord pageIncrement = GetPageIncrement(scrollbar);
  PRInt32 curpos = GetCurrentPosition(scrollbar);
  PRInt32 minpos = GetMinPosition(scrollbar);
  PRInt32 maxpos = GetMaxPosition(scrollbar);

  
  PRInt32 newpos = curpos + change * pageIncrement;
  if (newpos < minpos || maxpos < minpos)
    newpos = minpos;
  else if (newpos > maxpos)
    newpos = maxpos;

  SetCurrentPositionInternal(scrollbar, newpos, PR_TRUE, PR_FALSE);
}


nsresult
nsSliderFrame::CurrentPositionChanged(nsPresContext* aPresContext,
                                      PRBool aImmediateRedraw)
{
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  
  PRInt32 curPos = GetCurrentPosition(scrollbar);

  
  if (mCurPos == curPos)
      return NS_OK;

  
  PRInt32 minPos = GetMinPosition(scrollbar);
  PRInt32 maxPos = GetMaxPosition(scrollbar);

  maxPos = PR_MAX(minPos, maxPos);
  curPos = PR_MAX(minPos, PR_MIN(curPos, maxPos));

  
  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame)
    return NS_OK; 

  nsRect thumbRect = thumbFrame->GetRect();

  nsRect clientRect;
  GetClientRect(clientRect);

  
  nsRect newThumbRect(thumbRect);

  PRBool reverse = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                         nsGkAtoms::reverse, eCaseMatters);
  nscoord pos = reverse ? (maxPos - curPos) : (curPos - minPos);

  if (IsHorizontal())
     newThumbRect.x = clientRect.x + NSToCoordRound(pos * mRatio);
  else
     newThumbRect.y = clientRect.y + NSToCoordRound(pos * mRatio);

  
  thumbFrame->SetRect(newThumbRect);

  
  InvalidateWithFlags(clientRect, aImmediateRedraw ? INVALIDATE_IMMEDIATE : 0);

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

static void UpdateAttribute(nsIContent* aScrollbar, nscoord aNewPos, PRBool aNotify, PRBool aIsSmooth) {
  nsAutoString str;
  str.AppendInt(aNewPos);
  
  if (aIsSmooth) {
    aScrollbar->SetAttr(kNameSpaceID_None, nsGkAtoms::smooth, NS_LITERAL_STRING("true"), PR_FALSE);
  }
  aScrollbar->SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, str, aNotify);
  if (aIsSmooth) {
    aScrollbar->UnsetAttr(kNameSpaceID_None, nsGkAtoms::smooth, PR_FALSE);
  }
}




void
nsSliderFrame::SetCurrentThumbPosition(nsIContent* aScrollbar, nscoord aNewThumbPos,
                                       PRBool aIsSmooth, PRBool aImmediateRedraw, PRBool aMaySnap)
{
  nsRect crect;
  GetClientRect(crect);
  nscoord offset = IsHorizontal() ? crect.x : crect.y;
  PRInt32 newPos = NSToIntRound((aNewThumbPos - offset) / mRatio);
  
  if (aMaySnap && mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::snap,
                                        nsGkAtoms::_true, eCaseMatters)) {
    
    
    PRInt32 increment = GetIncrement(aScrollbar);
    newPos = NSToIntRound(newPos / float(increment)) * increment;
  }
  
  SetCurrentPosition(aScrollbar, newPos, aIsSmooth, aImmediateRedraw);
}





void
nsSliderFrame::SetCurrentPosition(nsIContent* aScrollbar, PRInt32 aNewPos,
                                  PRBool aIsSmooth, PRBool aImmediateRedraw)
{
   
  PRInt32 minpos = GetMinPosition(aScrollbar);
  PRInt32 maxpos = GetMaxPosition(aScrollbar);

  
  
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
nsSliderFrame::SetCurrentPositionInternal(nsIContent* aScrollbar, PRInt32 aNewPos,
                                          PRBool aIsSmooth,
                                          PRBool aImmediateRedraw)
{
  nsCOMPtr<nsIContent> scrollbar = aScrollbar;
  nsIBox* scrollbarBox = GetScrollbar();

  mUserChanged = PR_TRUE;

  nsIScrollbarFrame* scrollbarFrame = do_QueryFrame(scrollbarBox);
  if (scrollbarFrame) {
    
    nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
    if (mediator) {
      nsRefPtr<nsPresContext> context = PresContext();
      nsCOMPtr<nsIContent> content = GetContent();
      mediator->PositionChanged(scrollbarFrame, GetCurrentPosition(scrollbar), aNewPos);
      
      UpdateAttribute(scrollbar, aNewPos, PR_FALSE, aIsSmooth);
      nsIPresShell* shell = context->GetPresShell();
      if (shell) {
        nsIFrame* frame = shell->GetPrimaryFrameFor(content);
        if (frame && frame->GetType() == nsGkAtoms::sliderFrame) {
          static_cast<nsSliderFrame*>(frame)->
            CurrentPositionChanged(frame->PresContext(), aImmediateRedraw);
        }
      }
      mUserChanged = PR_FALSE;
      return;
    }
  }

  UpdateAttribute(scrollbar, aNewPos, PR_TRUE, aIsSmooth);
  mUserChanged = PR_FALSE;

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
nsSliderFrame::SetInitialChildList(nsIAtom*        aListName,
                                   nsFrameList&    aChildList)
{
  nsresult r = nsBoxFrame::SetInitialChildList(aListName, aChildList);

  AddListener();

  return r;
}

nsresult
nsSliderMediator::MouseDown(nsIDOMEvent* aMouseEvent)
{
  
  if (mSlider && !mSlider->isDraggingThumb())
    return mSlider->MouseDown(aMouseEvent);

  return NS_OK;
}

nsresult
nsSliderMediator::MouseUp(nsIDOMEvent* aMouseEvent)
{
  
  if (mSlider && !mSlider->isDraggingThumb())
    return mSlider->MouseUp(aMouseEvent);

  return NS_OK;
}

nsresult
nsSliderFrame::MouseDown(nsIDOMEvent* aMouseEvent)
{
#ifdef DEBUG_SLIDER
  printf("Begin dragging\n");
#endif

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return NS_OK;

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aMouseEvent));
  PRUint16 button = 0;
  mouseEvent->GetButton(&button);
  if (!(button == 0 || (button == 1 && gMiddlePref)))
    return NS_OK;

  PRBool isHorizontal = IsHorizontal();

  PRBool scrollToClick = PR_FALSE;
#ifndef XP_MACOSX
  
  mouseEvent->GetShiftKey(&scrollToClick);
  if (button != 0) {
    scrollToClick = PR_TRUE;
  }
#endif

  nsPoint pt =  nsLayoutUtils::GetDOMEventCoordinatesRelativeTo(mouseEvent,
                                                                this);
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

    nsIBox* scrollbarBox = GetScrollbar();
    scrollbar = GetContentOfBox(scrollbarBox);
  }

  DragThumb(PR_TRUE);

  if (scrollToClick) {
    
    SetCurrentThumbPosition(scrollbar, newpos, PR_FALSE, PR_FALSE, PR_FALSE);
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

nsresult
nsSliderFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
#ifdef DEBUG_SLIDER
  printf("Finish dragging\n");
#endif

  return NS_OK;
}

void
nsSliderFrame::DragThumb(PRBool aGrabMouseEvents)
{
  
  nsIFrame* parent = GetParent();
  if (parent) {
    nsCOMPtr<nsISliderListener> sliderListener = do_QueryInterface(parent->GetContent());
    if (sliderListener) {
      nsContentUtils::AddScriptRunner(
        new nsDragStateChangedRunnable(sliderListener, aGrabMouseEvents));
    }
  }

  nsIPresShell::SetCapturingContent(aGrabMouseEvents ? GetContent() : nsnull,
                                    aGrabMouseEvents ? CAPTURE_IGNOREALLOWED : 0);
}

PRBool
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
  if (thumbFrame) {
    thumbFrame->GetContent()->
      AddEventListenerByIID(mMediator, NS_GET_IID(nsIDOMMouseListener));
  }
}

void
nsSliderFrame::RemoveListener()
{
  NS_ASSERTION(mMediator, "No listener was ever added!!");

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame)
    return;

  thumbFrame->GetContent()->
    RemoveEventListenerByIID(mMediator, NS_GET_IID(nsIDOMMouseListener));
}

NS_IMETHODIMP
nsSliderFrame::HandlePress(nsPresContext* aPresContext,
                           nsGUIEvent*     aEvent,
                           nsEventStatus*  aEventStatus)
{
#ifdef XP_MACOSX
  
  if (((nsMouseEvent *)aEvent)->isAlt != GetScrollToClick())
#else
  if (((nsMouseEvent *)aEvent)->isShift != GetScrollToClick())
#endif
    return NS_OK;

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) 
    return NS_OK;

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return NS_OK;
  
  nsRect thumbRect = thumbFrame->GetRect();
  
  nscoord change = 1;
  nsPoint eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                    this);
  if (IsHorizontal() ? eventPoint.x < thumbRect.x 
                     : eventPoint.y < thumbRect.y)
    change = -1;

  mChange = change;
  DragThumb(PR_TRUE);
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
nsSliderFrame::Destroy()
{
  
  if (mMediator) {
    mMediator->SetSlider(nsnull);
    mMediator = nsnull;
  }
  StopRepeat();

  
  nsBoxFrame::Destroy();
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
  nsIBox* scrollbarBox = GetScrollbar();

  PRBool isHorizontal = (scrollbarBox->GetStateBits() & NS_STATE_IS_HORIZONTAL) != 0;
  if (isHorizontal)
      mState |= NS_STATE_IS_HORIZONTAL;
  else
      mState &= ~NS_STATE_IS_HORIZONTAL;
}


void nsSliderFrame::Notify(void)
{
    PRBool stop = PR_FALSE;

    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      StopRepeat();
      return;
    }
    nsRect thumbRect = thumbFrame->GetRect();

    PRBool isHorizontal = IsHorizontal();

    
    
    if (isHorizontal) {
        if (mChange < 0) {
            if (thumbRect.x < mDestinationPoint.x)
                stop = PR_TRUE;
        } else {
            if (thumbRect.x + thumbRect.width > mDestinationPoint.x)
                stop = PR_TRUE;
        }
    } else {
         if (mChange < 0) {
            if (thumbRect.y < mDestinationPoint.y)
                stop = PR_TRUE;
        } else {
            if (thumbRect.y + thumbRect.height > mDestinationPoint.y)
                stop = PR_TRUE;
        }
    }


    if (stop) {
      StopRepeat();
    } else {
      PageUpDown(mChange);
    }
}

NS_IMPL_ISUPPORTS2(nsSliderMediator,
                   nsIDOMMouseListener,
                   nsIDOMEventListener)
