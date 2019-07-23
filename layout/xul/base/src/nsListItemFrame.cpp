






































#include "nsListItemFrame.h"

#include "nsCOMPtr.h"
#include "nsINameSpaceManager.h" 
#include "nsGkAtoms.h"
#include "nsDisplayList.h"

NS_IMETHODIMP_(nsrefcnt) 
nsListItemFrame::AddRef(void)
{
  return NS_OK;
}

NS_IMETHODIMP_(nsrefcnt)
nsListItemFrame::Release(void)
{
  return NS_OK;
}

NS_INTERFACE_MAP_BEGIN(nsListItemFrame)
NS_INTERFACE_MAP_END_INHERITING(nsGridRowLeafFrame)

nsListItemFrame::nsListItemFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext,
                                 PRBool aIsRoot,
                                 nsIBoxLayout* aLayoutManager)
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

  
  
  size.height = PR_MAX(mRect.height, size.height);
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



nsIFrame*
NS_NewListItemFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRBool aIsRoot, nsIBoxLayout* aLayoutManager)
{
  return new (aPresShell) nsListItemFrame(aPresShell, aContext, aIsRoot, aLayoutManager);
} 

