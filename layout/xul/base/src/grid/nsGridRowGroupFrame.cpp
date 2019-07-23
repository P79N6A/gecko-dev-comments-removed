











































#include "nsGridRowGroupFrame.h"
#include "nsGridRowLeafLayout.h"
#include "nsGridRow.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"

already_AddRefed<nsIBoxLayout> NS_NewGridRowGroupLayout();

nsIFrame*
NS_NewGridRowGroupFrame(nsIPresShell* aPresShell,
                        nsStyleContext* aContext)
{
  nsCOMPtr<nsIBoxLayout> layout = NS_NewGridRowGroupLayout();
  if (!layout) {
    return nsnull;
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
  nsIBox* child = GetChildBox();
  while (child)
  {
    totalFlex += child->GetFlex(aState);
    child = child->GetNextBox();
  }

  mFlex = totalFlex;

  return totalFlex;
}


