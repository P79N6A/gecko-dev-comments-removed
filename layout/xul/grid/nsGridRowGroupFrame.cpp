











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
  nsIFrame* child = nsBox::GetChildBox(this);
  while (child)
  {
    totalFlex += child->GetFlex(aState);
    child = GetNextBox(child);
  }

  mFlex = totalFlex;

  return totalFlex;
}


