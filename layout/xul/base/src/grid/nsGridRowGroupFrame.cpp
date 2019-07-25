











#include "nsGridRowGroupFrame.h"
#include "nsGridRowLeafLayout.h"
#include "nsGridRow.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"

already_AddRefed<nsBoxLayout> NS_NewGridRowGroupLayout();

nsIFrame*
NS_NewGridRowGroupFrame(nsIPresShell* aPresShell,
                        nsStyleContext* aContext)
{
  nsCOMPtr<nsBoxLayout> layout = NS_NewGridRowGroupLayout();
  if (!layout) {
    return nullptr;
  }

  return new (aPresShell) nsGridRowGroupFrame(aPresShell, aContext, layout);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGridRowGroupFrame)






nscoord
nsGridRowGroupFrame::GetFlex(nsBoxLayoutState& aState)
{
  
  
  

  if (!DoesNeedRecalc(mFlex))
     return mFlex;

  if (nsBoxFrame::GetFlex(aState) == 0)
    return 0;

  
  nscoord totalFlex = 0;
  nsIFrame* child = GetChildBox();
  while (child)
  {
    totalFlex += child->GetFlex(aState);
    child = child->GetNextBox();
  }

  mFlex = totalFlex;

  return totalFlex;
}


