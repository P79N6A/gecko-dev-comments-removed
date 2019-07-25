











































#include "nsDeckFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsIViewManager.h"
#include "nsBoxLayoutState.h"
#include "nsStackLayout.h"
#include "nsDisplayList.h"
#include "nsHTMLContainerFrame.h"

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

NS_IMETHODIMP
nsDeckFrame::AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);


   
  if (aAttribute == nsGkAtoms::selectedIndex) {
    IndexChanged();
  }

  return rv;
}

NS_IMETHODIMP
nsDeckFrame::Init(nsIContent*     aContent,
                  nsIFrame*       aParent,
                  nsIFrame*       aPrevInFlow)
{
  nsresult rv = nsBoxFrame::Init(aContent, aParent, aPrevInFlow);

  mIndex = GetSelectedIndex();

  return rv;
}

void
nsDeckFrame::HideBox(nsIBox* aBox)
{
  nsIPresShell::ClearMouseCapture(aBox);
}

void
nsDeckFrame::IndexChanged()
{
  
  PRInt32 index = GetSelectedIndex();
  if (index == mIndex)
    return;

  
  InvalidateOverflowRect();

  
  nsIBox* currentBox = GetSelectedBox();
  if (currentBox) 
    HideBox(currentBox);

  mIndex = index;
}

PRInt32
nsDeckFrame::GetSelectedIndex()
{
  
  PRInt32 index = 0;

  
  nsAutoString value;
  if (mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::selectedIndex, value))
  {
    PRInt32 error;

    
    index = value.ToInteger(&error);
  }

  return index;
}

nsIFrame* 
nsDeckFrame::GetSelectedBox()
{
  return (mIndex >= 0) ? mFrames.FrameAt(mIndex) : nsnull; 
}

NS_IMETHODIMP
nsDeckFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  
  if (!GetStyleVisibility()->mVisible)
    return NS_OK;
    
  
  
  return nsBoxFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP
nsDeckFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists)
{
  
  nsIBox* box = GetSelectedBox();
  if (!box)
    return NS_OK;

  
  
  nsDisplayListSet set(aLists, aLists.BlockBorderBackgrounds());
  return BuildDisplayListForChild(aBuilder, box, aDirtyRect, set);
}

NS_IMETHODIMP
nsDeckFrame::DoLayout(nsBoxLayoutState& aState)
{
  
  
  PRUint32 oldFlags = aState.LayoutFlags();
  aState.SetLayoutFlags(NS_FRAME_NO_SIZE_VIEW | NS_FRAME_NO_VISIBILITY);

  
  nsresult rv = nsBoxFrame::DoLayout(aState);

  
  nsIBox* box = GetChildBox();

  nscoord count = 0;
  while (box) 
  {
    
    if (count != mIndex) 
      HideBox(box);

    box = box->GetNextBox();
    count++;
  }

  aState.SetLayoutFlags(oldFlags);

  return rv;
}

