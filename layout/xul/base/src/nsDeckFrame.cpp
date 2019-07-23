











































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


nsDeckFrame::nsDeckFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
  : nsBoxFrame(aPresShell, aContext), mIndex(0)
{
  nsCOMPtr<nsIBoxLayout> layout;
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
    IndexChanged(PresContext());
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

static void
CreateViewsForFrames(const nsFrameList& aFrames)
{
  for (nsFrameList::Enumerator f(aFrames); !f.AtEnd(); f.Next()) {
    nsHTMLContainerFrame::CreateViewForFrame(f.get(), PR_TRUE);
  }
}

NS_IMETHODIMP
nsDeckFrame::SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList)
{
  CreateViewsForFrames(aChildList);
  return nsBoxFrame::SetInitialChildList(aListName, aChildList);
}

NS_IMETHODIMP
nsDeckFrame::AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList)
{
  CreateViewsForFrames(aFrameList);
  return nsBoxFrame::AppendFrames(aListName, aFrameList);
}

NS_IMETHODIMP
nsDeckFrame::InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList)
{
  CreateViewsForFrames(aFrameList);
  return nsBoxFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
}

void
nsDeckFrame::HideBox(nsPresContext* aPresContext, nsIBox* aBox)
{
  nsIView* view = aBox->GetView();

  if (view) {
    nsIViewManager* viewManager = view->GetViewManager();
    viewManager->SetViewVisibility(view, nsViewVisibility_kHide);
    viewManager->ResizeView(view, nsRect(0, 0, 0, 0));
  }
}

void
nsDeckFrame::ShowBox(nsPresContext* aPresContext, nsIBox* aBox)
{
  nsRect rect = aBox->GetRect();
  nsIView* view = aBox->GetView();
  if (view) {
    nsIViewManager* viewManager = view->GetViewManager();
    rect.x = rect.y = 0;
    viewManager->ResizeView(view, rect);
    viewManager->SetViewVisibility(view, nsViewVisibility_kShow);
  }
}

void
nsDeckFrame::IndexChanged(nsPresContext* aPresContext)
{
  
  PRInt32 index = GetSelectedIndex();
  if (index == mIndex)
    return;

  
  nsBoxLayoutState state(aPresContext);
  Redraw(state);

  
  nsIBox* currentBox = GetSelectedBox();
  if (currentBox) 
     HideBox(aPresContext, currentBox);

  mIndex = index;

  
  nsIBox* newBox = GetSelectedBox();
  if (newBox) 
     ShowBox(aPresContext, newBox);
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

nsIBox* 
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
    
    if (count == mIndex) 
      ShowBox(aState.PresContext(), box);
    else
      HideBox(aState.PresContext(), box);

    box = box->GetNextBox();
    count++;
  }

  aState.SetLayoutFlags(oldFlags);

  return rv;
}

