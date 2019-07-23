




































#include "nsHTMLParts.h"
#include "nsIDocument.h"
#include "nsIRenderingContext.h"
#include "nsGUIEvent.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsBoxFrame.h"
#include "nsStackLayout.h"
#include "nsIRootBox.h"
#include "nsIContent.h"
#include "nsXULTooltipListener.h"
#include "nsFrameManager.h"






nsIRootBox*
nsIRootBox::GetRootBox(nsIPresShell* aShell)
{
  if (!aShell) {
    return nsnull;
  }
  nsIFrame* rootFrame = aShell->FrameManager()->GetRootFrame();
  if (!rootFrame) {
    return nsnull;
  }

  if (rootFrame) {
    rootFrame = rootFrame->GetFirstChild(nsnull);
  }

  nsIRootBox* rootBox = do_QueryFrame(rootFrame);
  return rootBox;
}

class nsRootBoxFrame : public nsBoxFrame, public nsIRootBox {
public:

  friend nsIFrame* NS_NewBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  nsRootBoxFrame(nsIPresShell* aShell, nsStyleContext *aContext);

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  virtual nsPopupSetFrame* GetPopupSetFrame();
  virtual void SetPopupSetFrame(nsPopupSetFrame* aPopupSet);
  virtual nsIContent* GetDefaultTooltip();
  virtual void SetDefaultTooltip(nsIContent* aTooltip);
  virtual nsresult AddTooltipSupport(nsIContent* aNode);
  virtual nsresult RemoveTooltipSupport(nsIContent* aNode);

  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  NS_IMETHOD HandleEvent(nsPresContext* aPresContext, 
                         nsGUIEvent*     aEvent,
                         nsEventStatus*  aEventStatus);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  




  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return PR_FALSE;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }
  
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

  nsPopupSetFrame* mPopupSetFrame;

protected:
  nsIContent* mDefaultTooltip;
};



nsIFrame*
NS_NewRootBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsRootBoxFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsRootBoxFrame)

nsRootBoxFrame::nsRootBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext, PR_TRUE)
{
  mPopupSetFrame = nsnull;

  nsCOMPtr<nsIBoxLayout> layout;
  NS_NewStackLayout(aShell, layout);
  SetLayoutManager(layout);
}

NS_IMETHODIMP
nsRootBoxFrame::AppendFrames(nsIAtom*        aListName,
                             nsFrameList&    aFrameList)
{
  nsresult  rv;

  NS_ASSERTION(!aListName, "unexpected child list name");
  NS_PRECONDITION(mFrames.IsEmpty(), "already have a child frame");
  if (aListName) {
    
    rv = NS_ERROR_INVALID_ARG;

  } else if (!mFrames.IsEmpty()) {
    
    rv = NS_ERROR_FAILURE;

  } else {
    rv = nsBoxFrame::AppendFrames(aListName, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
nsRootBoxFrame::InsertFrames(nsIAtom*        aListName,
                             nsIFrame*       aPrevFrame,
                             nsFrameList&    aFrameList)
{
  nsresult  rv;

  
  
  NS_PRECONDITION(!aPrevFrame, "unexpected previous sibling frame");
  if (aPrevFrame) {
    rv = NS_ERROR_UNEXPECTED;
  } else {
    rv = AppendFrames(aListName, aFrameList);
  }

  return rv;
}

NS_IMETHODIMP
nsRootBoxFrame::RemoveFrame(nsIAtom*        aListName,
                            nsIFrame*       aOldFrame)
{
  nsresult  rv;

  NS_ASSERTION(!aListName, "unexpected child list name");
  if (aListName) {
    
    rv = NS_ERROR_INVALID_ARG;
  
  } else if (aOldFrame == mFrames.FirstChild()) {
     rv = nsBoxFrame::RemoveFrame(aListName, aOldFrame);
  } else {
    rv = NS_ERROR_FAILURE;
  }

  return rv;
}

#ifdef DEBUG_REFLOW
PRInt32 gReflows = 0;
#endif

NS_IMETHODIMP
nsRootBoxFrame::Reflow(nsPresContext*          aPresContext,
                       nsHTMLReflowMetrics&     aDesiredSize,
                       const nsHTMLReflowState& aReflowState,
                       nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRootBoxFrame");

#ifdef DEBUG_REFLOW
  gReflows++;
  printf("----Reflow %d----\n", gReflows);
#endif
  return nsBoxFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}

nsresult
nsRootBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists)
{
  
  
  
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists, PR_TRUE);
  NS_ENSURE_SUCCESS(rv, rv);

  return BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsRootBoxFrame::HandleEvent(nsPresContext* aPresContext, 
                       nsGUIEvent* aEvent,
                       nsEventStatus* aEventStatus)
{
  NS_ENSURE_ARG_POINTER(aEventStatus);
  if (nsEventStatus_eConsumeNoDefault == *aEventStatus) {
    return NS_OK;
  }

  if (aEvent->message == NS_MOUSE_BUTTON_UP) {
    nsFrame::HandleEvent(aPresContext, aEvent, aEventStatus);
  }

  return NS_OK;
}



nsIAtom*
nsRootBoxFrame::GetType() const
{
  return nsGkAtoms::rootFrame;
}

nsPopupSetFrame*
nsRootBoxFrame::GetPopupSetFrame()
{
  return mPopupSetFrame;
}

void
nsRootBoxFrame::SetPopupSetFrame(nsPopupSetFrame* aPopupSet)
{
  
  
  
  
  
  
  
  
  if (!mPopupSetFrame || !aPopupSet) {
    mPopupSetFrame = aPopupSet;
  } else {
    NS_NOTREACHED("Popup set is already defined! Only 1 allowed.");
  }
}

nsIContent*
nsRootBoxFrame::GetDefaultTooltip()
{
  return mDefaultTooltip;
}

void
nsRootBoxFrame::SetDefaultTooltip(nsIContent* aTooltip)
{
  mDefaultTooltip = aTooltip;
}

nsresult
nsRootBoxFrame::AddTooltipSupport(nsIContent* aNode)
{
  NS_ENSURE_TRUE(aNode, NS_ERROR_NULL_POINTER);

  nsXULTooltipListener *listener = nsXULTooltipListener::GetInstance();
  if (!listener)
    return NS_ERROR_OUT_OF_MEMORY;

  return listener->AddTooltipSupport(aNode);
}

nsresult
nsRootBoxFrame::RemoveTooltipSupport(nsIContent* aNode)
{
  
  
  
  
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_QUERYFRAME_HEAD(nsRootBoxFrame)
  NS_QUERYFRAME_ENTRY(nsIRootBox)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)

#ifdef DEBUG
NS_IMETHODIMP
nsRootBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RootBox"), aResult);
}
#endif
