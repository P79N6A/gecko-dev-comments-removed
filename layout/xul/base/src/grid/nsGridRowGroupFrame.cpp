











































#include "nsGridRowGroupFrame.h"
#include "nsGridRowLeafLayout.h"
#include "nsGridRow.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"

nsIFrame*
NS_NewGridRowGroupFrame (nsIPresShell* aPresShell,
                         nsStyleContext* aContext,
                         PRBool aIsRoot,
                         nsIBoxLayout* aLayoutManager)
{
  return
    new (aPresShell) nsGridRowGroupFrame (aPresShell, aContext, aIsRoot, aLayoutManager);
} 







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


