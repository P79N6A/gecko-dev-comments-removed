






































#include "nsListItemFrame.h"

#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h" 
#include "nsGkAtoms.h"
#include "nsDisplayList.h"
#include "nsBoxLayout.h"

nsListItemFrame::nsListItemFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext,
                                 bool aIsRoot,
                                 nsBoxLayout* aLayoutManager)
  : nsGridRowLeafFrame(aPresShell, aContext, aIsRoot, aLayoutManager) 
{
}

nsListItemFrame::~nsListItemFrame()
{
}

nsSize
nsListItemFrame::GetPrefSize(nsBoxLayoutState& aState)
{
  nsSize size = nsBoxFrame::GetPrefSize(aState);  
  DISPLAY_PREF_SIZE(this, size);

  
  
  size.height = NS_MAX(mRect.height, size.height);
  return size;
}

NS_IMETHODIMP
nsListItemFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                             const nsRect&           aDirtyRect,
                                             const nsDisplayListSet& aLists)
{
  if (aBuilder->IsForEventDelivery()) {
    if (!mContent->AttrValueIs(kNameSpaceID_None, nsGkAtoms::allowevents,
                               nsGkAtoms::_true, eCaseMatters))
      return NS_OK;
  }
  
  return nsGridRowLeafFrame::BuildDisplayListForChildren(aBuilder, aDirtyRect, aLists);
}



already_AddRefed<nsBoxLayout> NS_NewGridRowLeafLayout();

nsIFrame*
NS_NewListItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsCOMPtr<nsBoxLayout> layout = NS_NewGridRowLeafLayout();
  if (!layout) {
    return nsnull;
  }
  
  return new (aPresShell) nsListItemFrame(aPresShell, aContext, false, layout);
}

NS_IMPL_FRAMEARENA_HELPERS(nsListItemFrame)
