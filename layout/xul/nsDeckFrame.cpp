











#include "nsDeckFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsNameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsViewManager.h"
#include "nsBoxLayoutState.h"
#include "nsStackLayout.h"
#include "nsDisplayList.h"
#include "nsContainerFrame.h"

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

nsIFrame*
NS_NewDeckFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsDeckFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsDeckFrame)

NS_QUERYFRAME_HEAD(nsDeckFrame)
  NS_QUERYFRAME_ENTRY(nsDeckFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBoxFrame)


nsDeckFrame::nsDeckFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
  : nsBoxFrame(aPresShell, aContext), mIndex(0)
{
  nsCOMPtr<nsBoxLayout> layout;
  NS_NewStackLayout(aPresShell, layout);
  SetLayoutManager(layout);
}

nsIAtom*
nsDeckFrame::GetType() const
{
  return nsGkAtoms::deckFrame;
}

nsresult
nsDeckFrame::AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);


   
  if (aAttribute == nsGkAtoms::selectedIndex) {
    IndexChanged();
  }

  return rv;
}

void
nsDeckFrame::Init(nsIContent*       aContent,
                  nsContainerFrame* aParent,
                  nsIFrame*         aPrevInFlow)
{
  nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  mIndex = GetSelectedIndex();
}

void
nsDeckFrame::HideBox(nsIFrame* aBox)
{
  nsIPresShell::ClearMouseCapture(aBox);
}

void
nsDeckFrame::IndexChanged()
{
  
  int32_t index = GetSelectedIndex();
  if (index == mIndex)
    return;

  
  InvalidateFrame();

  
  nsIFrame* currentBox = GetSelectedBox();
  if (currentBox) 
    HideBox(currentBox);

  mIndex = index;

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = GetAccService();
  if (accService) {
    accService->DeckPanelSwitched(PresContext()->GetPresShell(), mContent,
                                  currentBox, GetSelectedBox());
  }
#endif
}

int32_t
nsDeckFrame::GetSelectedIndex()
{
  
  int32_t index = 0;

  
  nsAutoString value;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::selectedIndex, value))
  {
    nsresult error;

    
    index = value.ToInteger(&error);
  }

  return index;
}

nsIFrame* 
nsDeckFrame::GetSelectedBox()
{
  return (mIndex >= 0) ? mFrames.FrameAt(mIndex) : nullptr; 
}

void
nsDeckFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  
  if (!StyleVisibility()->mVisible)
    return;
    
  nsBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

void
nsDeckFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  
  nsIFrame* box = GetSelectedBox();
  if (!box)
    return;

  
  
  nsDisplayListSet set(aLists, aLists.BlockBorderBackgrounds());
  BuildDisplayListForChild(aBuilder, box, aDirtyRect, set);
}

NS_IMETHODIMP
nsDeckFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  
  uint32_t oldFlags = aState.LayoutFlags();
  aState.SetLayoutFlags(NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_VISIBILITY);

  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  
  nsIFrame* box = nsBox::GetChildBox(this);

  nscoord count = 0;
  while (box) 
  {
    
    if (count != mIndex) 
      HideBox(box);

    box = GetNextBox(box);
    count++;
  }

  aState.SetLayoutFlags(oldFlags);

  return rv;
}

