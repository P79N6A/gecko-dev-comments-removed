




#include "nsHTMLParts.h"
#include "nsStyleConsts.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsBoxFrame.h"
#include "nsStackLayout.h"
#include "nsIRootBox.h"
#include "nsIContent.h"
#include "nsXULTooltipListener.h"
#include "nsFrameManager.h"
#include "mozilla/BasicEvents.h"

using namespace mozilla;






nsIRootBox*
nsIRootBox::GetRootBox(nsIPresShell* aShell)
{
  if (!aShell) {
    return nullptr;
  }
  nsIFrame* rootFrame = aShell->FrameManager()->GetRootFrame();
  if (!rootFrame) {
    return nullptr;
  }

  if (rootFrame) {
    rootFrame = rootFrame->GetFirstPrincipalChild();
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

  virtual nsPopupSetFrame* GetPopupSetFrame() MOZ_OVERRIDE;
  virtual void SetPopupSetFrame(nsPopupSetFrame* aPopupSet) MOZ_OVERRIDE;
  virtual nsIContent* GetDefaultTooltip() MOZ_OVERRIDE;
  virtual void SetDefaultTooltip(nsIContent* aTooltip) MOZ_OVERRIDE;
  virtual nsresult AddTooltipSupport(nsIContent* aNode) MOZ_OVERRIDE;
  virtual nsresult RemoveTooltipSupport(nsIContent* aNode) MOZ_OVERRIDE;

  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual void Reflow(nsPresContext*          aPresContext,
                          nsHTMLReflowMetrics&     aDesiredSize,
                          const nsHTMLReflowState& aReflowState,
                          nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  virtual nsresult HandleEvent(nsPresContext* aPresContext,
                               WidgetGUIEvent* aEvent,
                               nsEventStatus* aEventStatus) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    
    if (aFlags & (nsIFrame::eReplacedContainsBlock | nsIFrame::eReplaced))
      return false;
    return nsBoxFrame::IsFrameOfType(aFlags);
  }
  
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

  nsPopupSetFrame* mPopupSetFrame;

protected:
  nsIContent* mDefaultTooltip;
};



nsContainerFrame*
NS_NewRootBoxFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsRootBoxFrame (aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsRootBoxFrame)

nsRootBoxFrame::nsRootBoxFrame(nsIPresShell* aShell, nsStyleContext* aContext):
  nsBoxFrame(aShell, aContext, true)
{
  mPopupSetFrame = nullptr;

  nsCOMPtr<nsBoxLayout> layout;
  NS_NewStackLayout(aShell, layout);
  SetLayoutManager(layout);
}

void
nsRootBoxFrame::AppendFrames(ChildListID     aListID,
                             nsFrameList&    aFrameList)
{
  MOZ_ASSERT(aListID == kPrincipalList, "unexpected child list ID");
  MOZ_ASSERT(mFrames.IsEmpty(), "already have a child frame");
  nsBoxFrame::AppendFrames(aListID, aFrameList);
}

void
nsRootBoxFrame::InsertFrames(ChildListID     aListID,
                             nsIFrame*       aPrevFrame,
                             nsFrameList&    aFrameList)
{
  
  
  MOZ_ASSERT(!aPrevFrame, "unexpected previous sibling frame");
  AppendFrames(aListID, aFrameList);
}

void
nsRootBoxFrame::RemoveFrame(ChildListID     aListID,
                            nsIFrame*       aOldFrame)
{
  NS_ASSERTION(aListID == kPrincipalList, "unexpected child list ID");
  if (aOldFrame == mFrames.FirstChild()) {
    nsBoxFrame::RemoveFrame(aListID, aOldFrame);
  } else {
    MOZ_CRASH("unknown aOldFrame");
  }
}

#ifdef DEBUG_REFLOW
int32_t gReflows = 0;
#endif

void
nsRootBoxFrame::Reflow(nsPresContext*           aPresContext,
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

void
nsRootBoxFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                 const nsRect&           aDirtyRect,
                                 const nsDisplayListSet& aLists)
{
  
  
  
  DisplayBorderBackgroundOutline(aBuilder, aLists, true);

  BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}

nsresult
nsRootBoxFrame::HandleEvent(nsPresContext* aPresContext,
                            WidgetGUIEvent* aEvent,
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

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRootBoxFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RootBox"), aResult);
}
#endif
