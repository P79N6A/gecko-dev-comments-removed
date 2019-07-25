











































#include "nsScrollbarButtonFrame.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsSliderFrame.h"
#include "nsScrollbarFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsRepeatService.h"
#include "nsGUIEvent.h"
#include "mozilla/LookAndFeel.h"

using namespace mozilla;






nsIFrame*
NS_NewScrollbarButtonFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsScrollbarButtonFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsScrollbarButtonFrame)

NS_IMETHODIMP
nsScrollbarButtonFrame::HandleEvent(nsPresContext* aPresContext, 
                                    nsGUIEvent* aEvent,
                                    nsEventStatus* aEventStatus)
{  
  NS_ENSURE_ARG_POINTER(aEventStatus);

  
  
  if (!mContent->IsInNativeAnonymousSubtree() &&
      nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  
  if (aEvent->message == NS_MOUSE_EXIT_SYNTH ||
      aEvent->message == NS_MOUSE_BUTTON_UP)
     HandleRelease(aPresContext, aEvent, aEventStatus);
  
  
  if (!HandleButtonPress(aPresContext, aEvent, aEventStatus))
    return nsButtonBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  return NS_OK;
}


bool
nsScrollbarButtonFrame::HandleButtonPress(nsPresContext* aPresContext, 
                                          nsGUIEvent*     aEvent,
                                          nsEventStatus*  aEventStatus)
{
  
  LookAndFeel::IntID tmpAction;
  if (aEvent->eventStructType == NS_MOUSE_EVENT &&
      aEvent->message == NS_MOUSE_BUTTON_DOWN) {
    PRUint16 button = static_cast<nsMouseEvent*>(aEvent)->button;
    if (button == nsMouseEvent::eLeftButton) {
      tmpAction = LookAndFeel::eIntID_ScrollButtonLeftMouseButtonAction;
    } else if (button == nsMouseEvent::eMiddleButton) {
      tmpAction = LookAndFeel::eIntID_ScrollButtonMiddleMouseButtonAction;
    } else if (button == nsMouseEvent::eRightButton) {
      tmpAction = LookAndFeel::eIntID_ScrollButtonRightMouseButtonAction;
    } else {
      return false;
    }
  } else {
    return false;
  }

  
  PRInt32 pressedButtonAction;
  if (NS_FAILED(LookAndFeel::GetInt(tmpAction, &pressedButtonAction))) {
    return false;
  }

  
  nsIFrame* scrollbar;
  GetParentWithTag(nsGkAtoms::scrollbar, this, scrollbar);

  if (scrollbar == nsnull)
    return false;

  
  nsIContent* content = scrollbar->GetContent();

  static nsIContent::AttrValuesArray strings[] = { &nsGkAtoms::increment,
                                                   &nsGkAtoms::decrement,
                                                   nsnull };
  PRInt32 index = mContent->FindAttrValueIn(kNameSpaceID_None,
                                            nsGkAtoms::type,
                                            strings, eCaseMatters);
  PRInt32 direction;
  if (index == 0) 
    direction = 1;
  else if (index == 1)
    direction = -1;
  else
    return false;

  
  bool repeat = true;
  
  bool smoothScroll = true;
  switch (pressedButtonAction) {
    case 0:
      mIncrement = direction * nsSliderFrame::GetIncrement(content);
      break;
    case 1:
      mIncrement = direction * nsSliderFrame::GetPageIncrement(content);
      break;
    case 2:
      if (direction == -1)
        mIncrement = -nsSliderFrame::GetCurrentPosition(content);
      else
        mIncrement = nsSliderFrame::GetMaxPosition(content) - 
                     nsSliderFrame::GetCurrentPosition(content);
      
      
      repeat = smoothScroll = false;
      break;
    case 3:
    default:
      
      
      return false;
  }
  
  nsWeakFrame weakFrame(this);
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::active, NS_LITERAL_STRING("true"), true);

  if (weakFrame.IsAlive()) {
    DoButtonAction(smoothScroll);
  }
  if (repeat)
    StartRepeat();
  return true;
}

NS_IMETHODIMP 
nsScrollbarButtonFrame::HandleRelease(nsPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::active, true);
  StopRepeat();
  return NS_OK;
}

void nsScrollbarButtonFrame::Notify()
{
  
  
  DoButtonAction(true);
}

void
nsScrollbarButtonFrame::MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent) 
{
  nsButtonBoxFrame::MouseClicked(aPresContext, aEvent);
  
}

void
nsScrollbarButtonFrame::DoButtonAction(bool aSmoothScroll) 
{
  
  nsIFrame* scrollbar;
  GetParentWithTag(nsGkAtoms::scrollbar, this, scrollbar);

  if (scrollbar == nsnull)
    return;

  
  nsCOMPtr<nsIContent> content = scrollbar->GetContent();

  
  PRInt32 curpos = nsSliderFrame::GetCurrentPosition(content);
  PRInt32 oldpos = curpos;

  
  PRInt32 maxpos = nsSliderFrame::GetMaxPosition(content);

  
  if (mIncrement)
    curpos += mIncrement;

  
  if (curpos < 0)
    curpos = 0;
  else if (curpos > maxpos)
    curpos = maxpos;

  nsScrollbarFrame* sb = do_QueryFrame(scrollbar);
  if (sb) {
    nsIScrollbarMediator* m = sb->GetScrollbarMediator();
    if (m) {
      m->ScrollbarButtonPressed(sb, oldpos, curpos);
      return;
    }
  }

  
  nsAutoString curposStr;
  curposStr.AppendInt(curpos);

  if (aSmoothScroll)
    content->SetAttr(kNameSpaceID_None, nsGkAtoms::smooth, NS_LITERAL_STRING("true"), false);
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, curposStr, true);
  if (aSmoothScroll)
    content->UnsetAttr(kNameSpaceID_None, nsGkAtoms::smooth, false);
}

nsresult
nsScrollbarButtonFrame::GetChildWithTag(nsPresContext* aPresContext,
                                        nsIAtom* atom, nsIFrame* start,
                                        nsIFrame*& result)
{
  
  nsIFrame* childFrame = start->GetFirstPrincipalChild();
  while (nsnull != childFrame) 
  {    
    
    nsIContent* child = childFrame->GetContent();

    if (child) {
      
       if (child->Tag() == atom)
       {
         result = childFrame;

         return NS_OK;
       }
    }

     
     GetChildWithTag(aPresContext, atom, childFrame, result);
     if (result != nsnull) 
       return NS_OK;

    childFrame = childFrame->GetNextSibling();
  }

  result = nsnull;
  return NS_OK;
}

nsresult
nsScrollbarButtonFrame::GetParentWithTag(nsIAtom* toFind, nsIFrame* start,
                                         nsIFrame*& result)
{
   while (start)
   {
      start = start->GetParent();

      if (start) {
        
        nsIContent* child = start->GetContent();

        if (child && child->Tag() == toFind) {
          result = start;
          return NS_OK;
        }
      }
   }

   result = nsnull;
   return NS_OK;
}

void
nsScrollbarButtonFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  
  StopRepeat();
  nsButtonBoxFrame::DestroyFrom(aDestructRoot);
}
