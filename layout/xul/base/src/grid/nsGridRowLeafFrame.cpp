











































#include "nsGridRowLeafFrame.h"
#include "nsGridRowLeafLayout.h"
#include "nsGridRow.h"
#include "nsBoxLayoutState.h"
#include "nsGridLayout2.h"

already_AddRefed<nsIBoxLayout> NS_NewGridRowLeafLayout();

nsIFrame*
NS_NewGridRowLeafFrame(nsIPresShell* aPresShell,
                       nsStyleContext* aContext)
{
  nsCOMPtr<nsIBoxLayout> layout = NS_NewGridRowLeafLayout();
  if (!layout) {
    return nsnull;
  }
  
  return new (aPresShell) nsGridRowLeafFrame(aPresShell, aContext, PR_FALSE,
                                             layout);
}

NS_IMPL_FRAMEARENA_HELPERS(nsGridRowLeafFrame)





NS_IMETHODIMP
nsGridRowLeafFrame::GetBorderAndPadding(nsMargin& aBorderAndPadding)
{
  
  nsresult rv = nsBoxFrame::GetBorderAndPadding(aBorderAndPadding);

  nsCOMPtr<nsIBoxLayout> layout;
  GetLayoutManager(getter_AddRefs(layout));
  if (!layout)
    return rv;

  nsCOMPtr<nsIGridPart> part = do_QueryInterface(layout);
  if (!part)
    return rv;
    
  PRInt32 index = 0;
  nsGrid* grid = part->GetGrid(this, &index);

  if (!grid) 
    return rv;

  PRBool isHorizontal = IsHorizontal();

  nsBoxLayoutState state(PresContext());

  PRInt32 firstIndex = 0;
  PRInt32 lastIndex = 0;
  nsGridRow* firstRow = nsnull;
  nsGridRow* lastRow = nsnull;
  grid->GetFirstAndLastRow(state, firstIndex, lastIndex, firstRow, lastRow, isHorizontal);

  
  if (firstRow && firstRow->GetBox() == this) {
    
    nscoord top = 0;
    nscoord bottom = 0;
    grid->GetRowOffsets(state, firstIndex, top, bottom, isHorizontal);

    if (isHorizontal) {
      if (top > aBorderAndPadding.top)
        aBorderAndPadding.top = top;
    } else {
      if (top > aBorderAndPadding.left)
        aBorderAndPadding.left = top;
    } 
  }

  if (lastRow && lastRow->GetBox() == this) {
    
    nscoord top = 0;
    nscoord bottom = 0;
    grid->GetRowOffsets(state, lastIndex, top, bottom, isHorizontal);

    if (isHorizontal) {
      if (bottom > aBorderAndPadding.bottom)
        aBorderAndPadding.bottom = bottom;
    } else {
      if (bottom > aBorderAndPadding.right)
        aBorderAndPadding.right = bottom;
    }
    
  }  
  
  return rv;
}


