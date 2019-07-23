











































#include "nsStackFrame.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsIPresShell.h"
#include "nsCSSRendering.h"
#include "nsBoxLayoutState.h"
#include "nsStackLayout.h"
#include "nsDisplayList.h"

nsIFrame*
NS_NewStackFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsStackFrame(aPresShell, aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsStackFrame)

nsStackFrame::nsStackFrame(nsIPresShell* aPresShell, nsStyleContext* aContext):
  nsBoxFrame(aPresShell, aContext)
{
  nsCOMPtr<nsIBoxLayout> layout;
  NS_NewStackLayout(aPresShell, layout);
  SetLayoutManager(layout);
}







NS_IMETHODIMP
nsStackFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                          const nsRect&           aDirtyRect,
                                          const nsDisplayListSet& aLists)
{
  
  
  
  nsDisplayList* content = aLists.Content();
  nsDisplayListSet kidLists(content, content, content, content, content, content);
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    
    nsresult rv =
      BuildDisplayListForChild(aBuilder, kid, aDirtyRect, kidLists,
                               DISPLAY_CHILD_FORCE_STACKING_CONTEXT);
    NS_ENSURE_SUCCESS(rv, rv);
    kid = kid->GetNextSibling();
  }
  return NS_OK;
}
