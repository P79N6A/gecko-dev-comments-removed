











































#include "nsScrollbarFrame.h"
#include "nsScrollbarButtonFrame.h"
#include "nsGkAtoms.h"
#include "nsIScrollableFrame.h"
#include "nsIScrollbarMediator.h"






nsIFrame*
NS_NewScrollbarFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsScrollbarFrame (aPresShell, aContext);
} 




NS_INTERFACE_MAP_BEGIN(nsScrollbarFrame)
  NS_INTERFACE_MAP_ENTRY(nsIScrollbarFrame)
NS_INTERFACE_MAP_END_INHERITING(nsBoxFrame)

NS_IMETHODIMP
nsScrollbarFrame::Reflow(nsPresContext*          aPresContext,
                         nsHTMLReflowMetrics&     aDesiredSize,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus&          aStatus)
{
  nsresult rv = nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (aReflowState.availableWidth == 0) {
    aDesiredSize.width = 0;
  }
  if (aReflowState.availableHeight == 0) {
    aDesiredSize.height = 0;
  }

  return NS_OK;
}

 PRBool
nsScrollbarFrame::IsContainingBlock() const
{
  
  
  return PR_TRUE;
}

nsIAtom*
nsScrollbarFrame::GetType() const
{
  return nsGkAtoms::scrollbarFrame;
}

NS_IMETHODIMP
nsScrollbarFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                   nsIAtom* aAttribute,
                                   PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);

  
  
  if (aAttribute != nsGkAtoms::curpos)
    return rv;

  nsIFrame* parent = GetParent();
  if (!parent)
    return rv;

  nsIScrollableFrame* scrollable = nsnull;
  parent->QueryInterface(NS_GET_IID(nsIScrollableFrame), (void**)&scrollable);
  if (!scrollable)
    return rv;

  scrollable->CurPosAttributeChanged(mContent, aModType);
  return rv;
}

NS_IMETHODIMP
nsScrollbarFrame::HandlePress(nsPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP
nsScrollbarFrame::HandleMultiplePress(nsPresContext* aPresContext, 
                     nsGUIEvent*     aEvent,
                     nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsScrollbarFrame::HandleDrag(nsPresContext* aPresContext, 
                              nsGUIEvent*     aEvent,
                              nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

NS_IMETHODIMP 
nsScrollbarFrame::HandleRelease(nsPresContext* aPresContext, 
                                 nsGUIEvent*     aEvent,
                                 nsEventStatus*  aEventStatus)
{
  return NS_OK;
}

void
nsScrollbarFrame::SetScrollbarMediatorContent(nsIContent* aMediator)
{
  mScrollbarMediator = aMediator;
}

nsIScrollbarMediator*
nsScrollbarFrame::GetScrollbarMediator()
{
  if (!mScrollbarMediator)
    return nsnull;
  nsIFrame* f =
    PresContext()->PresShell()->GetPrimaryFrameFor(mScrollbarMediator);
  if (!f)
    return nsnull;

  
  
  nsIScrollableFrame* scrollFrame;
  CallQueryInterface(f, &scrollFrame);
  if (scrollFrame) {
    f = scrollFrame->GetScrolledFrame();
    if (!f)
      return nsnull;
  }

  nsIScrollbarMediator* sbm;
  CallQueryInterface(f, &sbm);
  return sbm;
}
