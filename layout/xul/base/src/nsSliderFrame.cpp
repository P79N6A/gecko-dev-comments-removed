












































#include "nsSliderFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsUnitConversion.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsIDOMEventTarget.h"
#include "nsIViewManager.h"
#include "nsIWidget.h"
#include "nsIDOMMouseEvent.h"
#include "nsIDocument.h"
#include "nsScrollbarButtonFrame.h"
#include "nsIScrollbarListener.h"
#include "nsIScrollbarMediator.h"
#include "nsIScrollbarFrame.h"
#include "nsISupportsArray.h"
#include "nsIScrollableView.h"
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
  mScrollbarListener(nsnull),
  mChange(0),
  mMediator(nsnull)
{
}


nsSliderFrame::~nsSliderFrame()
{
   mRedrawImmediate = PR_FALSE;
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

  CreateViewForFrame(PresContext(), this, GetStyleContext(), PR_TRUE);
  return rv;
}

NS_IMETHODIMP
nsSliderFrame::RemoveFrame(nsIAtom*        aListName,
                           nsIFrame*       aOldFrame)
{
  nsresult rv = nsBoxFrame::RemoveFrame(aListName, aOldFrame);
  PRInt32 start = GetChildCount();
  if (start == 0)
    RemoveListener();

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::InsertFrames(nsIAtom*        aListName,
                            nsIFrame*       aPrevFrame,
                            nsIFrame*       aFrameList)
{
  PRInt32 start = GetChildCount();
  nsresult rv = nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
  if (start == 0)
    AddListener();

  return rv;
}

NS_IMETHODIMP
nsSliderFrame::AppendFrames(nsIAtom*        aListName,
                            nsIFrame*       aFrameList)
{
  
  
  PRInt32 start = GetChildCount();
  nsresult rv = nsBoxFrame::AppendFrames(aListName, aFrameList);
  if (start == 0)
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


NS_IMETHODIMP
nsSliderFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                nsIAtom* aAttribute,
                                PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  
  if (aAttribute == nsGkAtoms::curpos) {
     rv = CurrentPositionChanged(PresContext());
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
      if (current < min || current > max)
      {
        if (current < min || max < min)
            current = min;
        else if (current > max)
            current = max;

        
        nsIScrollbarFrame* scrollbarFrame;
        CallQueryInterface(scrollbarBox, &scrollbarFrame);
        if (scrollbarFrame) {
          nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
          if (mediator) {
            mediator->PositionChanged(scrollbarFrame, GetCurrentPosition(scrollbar), current);
          }
        }

        nsAutoString currentStr;
        currentStr.AppendInt(current);
        scrollbar->SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, currentStr, PR_TRUE);
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

  
  nsRect clientRect(0,0,0,0);
  GetClientRect(clientRect);

  
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);
  PRBool isHorizontal = IsHorizontal();

  
  nsSize thumbSize = thumbBox->GetPrefSize(aState);

  if (isHorizontal)
    thumbSize.height = clientRect.height;
  else
    thumbSize.width = clientRect.width;

  
  PRInt32 curpospx = GetCurrentPosition(scrollbar);
  PRInt32 minpospx = GetMinPosition(scrollbar);
  PRInt32 maxpospx = GetMaxPosition(scrollbar);
  PRInt32 pageIncrement = GetPageIncrement(scrollbar);

  if (maxpospx < minpospx)
    maxpospx = minpospx;

  if (curpospx < minpospx)
     curpospx = minpospx;
  else if (curpospx > maxpospx)
     curpospx = maxpospx;

  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

  
  nscoord maxpos = (maxpospx - minpospx) * onePixel;

  
  
  nscoord& desiredcoord = isHorizontal ? clientRect.width : clientRect.height;
  nscoord& thumbcoord = isHorizontal ? thumbSize.width : thumbSize.height;

  nscoord ourmaxpos = desiredcoord;

  mRatio = 1;

  if ((pageIncrement + maxpospx - minpospx) > 0)
  {
    
    if (thumbBox->GetFlex(aState) > 0)
    {
      mRatio = float(pageIncrement) / float(maxpospx - minpospx + pageIncrement);
      nscoord thumbsize = NSToCoordRound(ourmaxpos * mRatio);

      
      if (thumbsize > thumbcoord)
        thumbcoord = thumbsize;
    }
  }

  ourmaxpos -= thumbcoord;
  if (float(maxpos) != 0)
    mRatio = float(ourmaxpos) / float(maxpos);

  
  
  
  PRBool reverse = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                         nsGkAtoms::reverse, eCaseMatters);
  nscoord pos = reverse ? (maxpospx - curpospx) : (curpospx - minpospx);

  
  nsRect thumbRect(clientRect.x, clientRect.y, thumbSize.width, thumbSize.height);
  if (isHorizontal)
    thumbRect.x += nscoord(float(pos * onePixel) * mRatio);
  else
    thumbRect.y += nscoord(float(pos * onePixel) * mRatio);

  nsRect oldThumbRect(thumbBox->GetRect());
  LayoutChildAt(aState, thumbBox, thumbRect);

  SyncLayout(aState);

#ifdef DEBUG_SLIDER
  PRInt32 c = GetCurrentPosition(scrollbar);
  PRInt32 min = GetMinPosition(scrollbar);
  PRInt32 max = GetMaxPosition(scrollbar);
  printf("Current=%d, min=%d, max=%d\n", c, min, max);
#endif

  
  if (oldThumbRect != thumbRect)
    Redraw(aState);

  return NS_OK;
}


NS_IMETHODIMP
nsSliderFrame::HandleEvent(nsPresContext* aPresContext,
                                      nsGUIEvent* aEvent,
                                      nsEventStatus* aEventStatus)
{
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);
  PRBool isHorizontal = IsHorizontal();

  if (isDraggingThumb())
  {
      
      
      mRedrawImmediate = PR_TRUE;

    switch (aEvent->message) {
    case NS_MOUSE_MOVE: {
      nsPoint eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                         this);
      if (mChange) {
        
        
        
        mDestinationPoint = eventPoint;
        nsRepeatService::GetInstance()->Stop();
        nsRepeatService::GetInstance()->Start(mMediator);
        break;
      }

       nscoord pos = isHorizontal ? eventPoint.x : eventPoint.y;

       nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

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
         
         SetCurrentPosition(scrollbar, (int) (mThumbStart / onePixel / mRatio), PR_FALSE);
         return NS_OK;
       }

       
       nscoord pospx = pos/onePixel;

       
       pospx = nscoord(pospx/mRatio);

       
       
       if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::snap,
                                 nsGkAtoms::_true, eCaseMatters)) {
         PRInt32 increment = GetIncrement(scrollbar);
         pospx = NSToCoordRound(pospx / (float)increment) * increment;
       }

       
       SetCurrentPosition(scrollbar, pospx, PR_FALSE);
    }
    break;

    case NS_MOUSE_BUTTON_UP:
      if (NS_STATIC_CAST(nsMouseEvent*, aEvent)->button == nsMouseEvent::eLeftButton ||
          (NS_STATIC_CAST(nsMouseEvent*, aEvent)->button == nsMouseEvent::eMiddleButton &&
           gMiddlePref)) {
        
        AddListener();
        DragThumb(PR_FALSE);
        if (mChange) {
          nsRepeatService::GetInstance()->Stop();
          mChange = 0;
        }
        mRedrawImmediate = PR_FALSE;
        return nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
      }
    }

    
    
    mRedrawImmediate = PR_FALSE;

    
    return NS_OK;
  } else if ((aEvent->message == NS_MOUSE_BUTTON_DOWN &&
              NS_STATIC_CAST(nsMouseEvent*, aEvent)->button ==
                nsMouseEvent::eLeftButton &&
              NS_STATIC_CAST(nsMouseEvent*, aEvent)->isShift) ||
             (gMiddlePref && aEvent->message == NS_MOUSE_BUTTON_DOWN &&
              NS_STATIC_CAST(nsMouseEvent*, aEvent)->button ==
                nsMouseEvent::eMiddleButton)) {
    
    nsPoint eventPoint = nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent,
                                                                      this);
    nscoord pos = isHorizontal ? eventPoint.x : eventPoint.y;

    nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
    nscoord pospx = pos/onePixel;

   
    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      return NS_OK;
    }
    nsSize thumbSize = thumbFrame->GetSize();
    nscoord thumbLength = isHorizontal ? thumbSize.width : thumbSize.height;
    thumbLength /= onePixel;
    pospx -= (thumbLength/2);

    
    pospx = nscoord(pospx/mRatio);

    
    SetCurrentPosition(scrollbar, pospx, PR_FALSE);

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

  if (mScrollbarListener)
    mScrollbarListener->PagedUpDown(); 

  nscoord pageIncrement = GetPageIncrement(scrollbar);
  PRInt32 curpos = GetCurrentPosition(scrollbar);
  PRInt32 minpos = GetMinPosition(scrollbar);
  PRInt32 maxpos = GetMaxPosition(scrollbar);

  
  PRInt32 newpos = curpos - minpos + change * pageIncrement;
  if (newpos < minpos || maxpos < minpos)
    newpos = minpos;
  else if (newpos > maxpos)
    newpos = maxpos;

  SetCurrentPositionInternal(scrollbar, newpos, PR_TRUE);
}


nsresult
nsSliderFrame::CurrentPositionChanged(nsPresContext* aPresContext)
{
  nsIBox* scrollbarBox = GetScrollbar();
  nsCOMPtr<nsIContent> scrollbar;
  scrollbar = GetContentOfBox(scrollbarBox);

  PRBool isHorizontal = IsHorizontal();

  
  PRInt32 curpospx = GetCurrentPosition(scrollbar);

  
  if (mCurPos == curpospx)
      return NS_OK;

  
  PRInt32 minpospx = GetMinPosition(scrollbar);
  PRInt32 maxpospx = GetMaxPosition(scrollbar);

  if (curpospx < minpospx || maxpospx < minpospx)
     curpospx = minpospx;
  else if (curpospx > maxpospx)
     curpospx = maxpospx;

  
  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame)
    return NS_OK; 

  nsRect thumbRect = thumbFrame->GetRect();

  nsRect clientRect;
  GetClientRect(clientRect);

  
  nsRect newThumbRect(thumbRect);

  PRBool reverse = mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                                         nsGkAtoms::reverse, eCaseMatters);
  nscoord pos = reverse ? (maxpospx - curpospx) : (curpospx - minpospx);

  
  nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
  if (isHorizontal)
     newThumbRect.x = clientRect.x + nscoord(float(pos * onePixel) * mRatio);
  else
     newThumbRect.y = clientRect.y + nscoord(float(pos * onePixel) * mRatio);

  
  thumbFrame->SetRect(newThumbRect);

  
  
  nsRect changeRect;
  changeRect.UnionRect(thumbFrame->GetOverflowRect() + thumbRect.TopLeft(),
                       thumbFrame->GetOverflowRect() + newThumbRect.TopLeft());

  
  Invalidate(changeRect, mRedrawImmediate);
    
  if (mScrollbarListener)
    mScrollbarListener->PositionChanged(aPresContext, mCurPos, curpospx);

  mCurPos = curpospx;

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
nsSliderFrame::SetCurrentPosition(nsIContent* scrollbar, nscoord newpos, PRBool aIsSmooth)
{
   
  PRInt32 minpos = GetMinPosition(scrollbar);
  PRInt32 maxpos = GetMaxPosition(scrollbar);

  
  
  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::dir,
                            nsGkAtoms::reverse, eCaseMatters))
    newpos = maxpos - newpos;
  else
    newpos += minpos;

  
  if (newpos < minpos || maxpos < minpos)
    newpos = minpos;
  else if (newpos > maxpos)
    newpos = maxpos;

  SetCurrentPositionInternal(scrollbar, newpos, aIsSmooth);
}

void
nsSliderFrame::SetCurrentPositionInternal(nsIContent* scrollbar, nscoord newpos, PRBool aIsSmooth)
{
  nsIBox* scrollbarBox = GetScrollbar();
  nsIScrollbarFrame* scrollbarFrame;
  CallQueryInterface(scrollbarBox, &scrollbarFrame);

  if (scrollbarFrame) {
    
    nsIScrollbarMediator* mediator = scrollbarFrame->GetScrollbarMediator();
    if (mediator) {
      mediator->PositionChanged(scrollbarFrame, GetCurrentPosition(scrollbar), newpos);
      
      UpdateAttribute(scrollbar, newpos, PR_FALSE, aIsSmooth);
      CurrentPositionChanged(PresContext());
      return;
    }
  }

  UpdateAttribute(scrollbar, newpos, PR_TRUE, aIsSmooth);

#ifdef DEBUG_SLIDER
  printf("Current Pos=%d\n",newpos);
#endif

}

NS_IMETHODIMP
nsSliderFrame::SetInitialChildList(nsIAtom*        aListName,
                                   nsIFrame*       aChildList)
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
  

  if (mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::disabled,
                            nsGkAtoms::_true, eCaseMatters))
    return NS_OK;

  PRBool isHorizontal = IsHorizontal();

  nsCOMPtr<nsIDOMMouseEvent> mouseEvent(do_QueryInterface(aMouseEvent));

  PRUint16 button = 0;
  PRBool scrollToClick = PR_FALSE;
  mouseEvent->GetShiftKey(&scrollToClick);
  mouseEvent->GetButton(&button);
  if (button != 0) {
    if (button != 1 || !gMiddlePref)
      return NS_OK;
    scrollToClick = PR_TRUE;
  }

  
  
  if (!scrollToClick) {
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
  }

  PRInt32 clientPosPx;
  nsIntRect screenRect = GetScreenRect();
  nscoord pos;
  if (isHorizontal) {
    mouseEvent->GetScreenX(&clientPosPx);
    pos = nsPresContext::CSSPixelsToAppUnits(clientPosPx) - 
          PresContext()->DevPixelsToAppUnits(screenRect.x);
  } else {
    mouseEvent->GetScreenY(&clientPosPx);
    pos = nsPresContext::CSSPixelsToAppUnits(clientPosPx) - 
          PresContext()->DevPixelsToAppUnits(screenRect.y);
  }

  
  
  if (scrollToClick) {
    nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);
    nscoord pospx = pos/onePixel;

    
    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      return NS_OK;
    }
    nsSize thumbSize = thumbFrame->GetSize();
    nscoord thumbLength = isHorizontal ? thumbSize.width : thumbSize.height;
    thumbLength /= onePixel;
    pospx -= (thumbLength/2);

    
    pospx = nscoord(pospx/mRatio);

    nsIBox* scrollbarBox = GetScrollbar();
    nsCOMPtr<nsIContent> scrollbar;
    scrollbar = GetContentOfBox(scrollbarBox);

    
    SetCurrentPosition(scrollbar, pospx, PR_FALSE);
  }

  DragThumb(PR_TRUE);

  nsIFrame* thumbFrame = mFrames.FirstChild();
  if (!thumbFrame) {
    return NS_OK;
  }

  if (isHorizontal)
     mThumbStart = thumbFrame->GetPosition().x;
  else
     mThumbStart = thumbFrame->GetPosition().y;

  mDragStart = pos - mThumbStart;
  

  return NS_OK;
}

nsresult
nsSliderFrame::MouseUp(nsIDOMEvent* aMouseEvent)
{
 

  return NS_OK;
}

void
nsSliderFrame::DragThumb(PRBool aGrabMouseEvents)
{
    
  nsIView* view = GetView();

  if (view) {
    nsIViewManager* viewMan = view->GetViewManager();

    if (viewMan) {
      PRBool result;

      if (aGrabMouseEvents) {
        viewMan->GrabMouseEvents(view,result);
      } else {
        viewMan->GrabMouseEvents(nsnull,result);
      }
    }
  }
}

PRBool
nsSliderFrame::isDraggingThumb()
{
    
  nsIView* view = GetView();

  if (view) {
    nsIViewManager* viewMan = view->GetViewManager();

    if (viewMan) {
        nsIView* grabbingView;
        viewMan->GetMouseEventGrabber(grabbingView);
        if (grabbingView == view)
          return PR_TRUE;
    }
  }

  return PR_FALSE;
}

void
nsSliderFrame::AddListener()
{
  if (!mMediator) {
    mMediator = new nsSliderMediator(this);
    NS_ADDREF(mMediator);
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
  if (((nsMouseEvent *)aEvent)->isShift)
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
  PageUpDown(change);
  
  nsRepeatService::GetInstance()->Start(mMediator);
  
  return NS_OK;
}

NS_IMETHODIMP
nsSliderFrame::HandleRelease(nsPresContext* aPresContext,
                                 nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  nsRepeatService::GetInstance()->Stop();

  return NS_OK;
}

void
nsSliderFrame::Destroy()
{
  
  if (mMediator) {
    mMediator->SetSlider(nsnull);
    NS_RELEASE(mMediator);
    mMediator = nsnull;
  }

  
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


void
nsSliderFrame::SetScrollbarListener(nsIScrollbarListener* aListener)
{
  
  mScrollbarListener = aListener;
}

NS_IMETHODIMP nsSliderMediator::Notify(nsITimer *timer)
{
  if (mSlider)
    mSlider->Notify(timer);
  return NS_OK;
}

NS_IMETHODIMP_(void) nsSliderFrame::Notify(nsITimer *timer)
{
    PRBool stop = PR_FALSE;

    nsIFrame* thumbFrame = mFrames.FirstChild();
    if (!thumbFrame) {
      nsRepeatService::GetInstance()->Stop();
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
       nsRepeatService::GetInstance()->Stop();
    } else {
      PageUpDown(mChange);
    }
}

NS_INTERFACE_MAP_BEGIN(nsSliderMediator)
  NS_INTERFACE_MAP_ENTRY(nsIDOMMouseListener)
  NS_INTERFACE_MAP_ENTRY(nsITimerCallback)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsITimerCallback)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsSliderMediator)
NS_IMPL_RELEASE(nsSliderMediator)

