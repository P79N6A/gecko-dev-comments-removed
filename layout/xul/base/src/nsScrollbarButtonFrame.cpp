











































#include "nsScrollbarButtonFrame.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsSliderFrame.h"
#include "nsIScrollbarFrame.h"
#include "nsIScrollbarMediator.h"
#include "nsRepeatService.h"
#include "nsGUIEvent.h"
#include "nsILookAndFeel.h"






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
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  
  if (aEvent->message == NS_MOUSE_EXIT_SYNTH ||
      aEvent->message == NS_MOUSE_BUTTON_UP)
     HandleRelease(aPresContext, aEvent, aEventStatus);
  
  
  if (!HandleButtonPress(aPresContext, aEvent, aEventStatus))
    return nsButtonBoxFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  return NS_OK;
}


PRBool
nsScrollbarButtonFrame::HandleButtonPress(nsPresContext* aPresContext, 
                                          nsGUIEvent*     aEvent,
                                          nsEventStatus*  aEventStatus)
{
  
  nsILookAndFeel::nsMetricID tmpAction;
  if (aEvent->eventStructType == NS_MOUSE_EVENT &&
      aEvent->message == NS_MOUSE_BUTTON_DOWN) {
    PRUint16 button = static_cast<nsMouseEvent*>(aEvent)->button;
    if (button == nsMouseEvent::eLeftButton) {
      tmpAction = nsILookAndFeel::eMetric_ScrollButtonLeftMouseButtonAction;
    } else if (button == nsMouseEvent::eMiddleButton) {
      tmpAction = nsILookAndFeel::eMetric_ScrollButtonMiddleMouseButtonAction;
    } else if (button == nsMouseEvent::eRightButton) {
      tmpAction = nsILookAndFeel::eMetric_ScrollButtonRightMouseButtonAction;
    } else {
      return PR_FALSE;
    }
  } else {
    return PR_FALSE;
  }

  
  PRInt32 pressedButtonAction;
  if (NS_FAILED(aPresContext->LookAndFeel()->GetMetric(tmpAction,
                                                       pressedButtonAction)))
    return PR_FALSE;

  
  nsIFrame* scrollbar;
  GetParentWithTag(nsGkAtoms::scrollbar, this, scrollbar);

  if (scrollbar == nsnull)
    return PR_FALSE;

  
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
    return PR_FALSE;

  
  PRBool repeat = PR_TRUE;
  
  PRBool smoothScroll = PR_TRUE;
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
      
      
      repeat = smoothScroll = PR_FALSE;
      break;
    case 3:
    default:
      
      
      return PR_FALSE;
  }
  
  nsWeakFrame weakFrame(this);
  mContent->SetAttr(kNameSpaceID_None, nsGkAtoms::active, NS_LITERAL_STRING("true"), PR_TRUE);

  if (weakFrame.IsAlive()) {
    DoButtonAction(smoothScroll);
  }
  if (repeat)
    StartRepeat();
  return PR_TRUE;
}

NS_IMETHODIMP 
nsScrollbarButtonFrame::HandleRelease(nsPresContext* aPresContext, 
                                      nsGUIEvent*     aEvent,
                                      nsEventStatus*  aEventStatus)
{
  
  mContent->UnsetAttr(kNameSpaceID_None, nsGkAtoms::active, PR_TRUE);
  StopRepeat();
  return NS_OK;
}

void nsScrollbarButtonFrame::Notify()
{
  
  
  DoButtonAction(PR_TRUE);
}

void
nsScrollbarButtonFrame::MouseClicked(nsPresContext* aPresContext, nsGUIEvent* aEvent) 
{
  nsButtonBoxFrame::MouseClicked(aPresContext, aEvent);
  
}

void
nsScrollbarButtonFrame::DoButtonAction(PRBool aSmoothScroll) 
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

  nsIScrollbarFrame* sb = do_QueryFrame(scrollbar);
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
    content->SetAttr(kNameSpaceID_None, nsGkAtoms::smooth, NS_LITERAL_STRING("true"), PR_FALSE);
  content->SetAttr(kNameSpaceID_None, nsGkAtoms::curpos, curposStr, PR_TRUE);
  if (aSmoothScroll)
    content->UnsetAttr(kNameSpaceID_None, nsGkAtoms::smooth, PR_FALSE);
}

nsresult
nsScrollbarButtonFrame::GetChildWithTag(nsPresContext* aPresContext,
                                        nsIAtom* atom, nsIFrame* start,
                                        nsIFrame*& result)
{
  
  nsIFrame* childFrame = start->GetFirstChild(nsnull);
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
nsScrollbarButtonFrame::Destroy()
{
  
  
  StopRepeat();
  nsButtonBoxFrame::Destroy();
}
