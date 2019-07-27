











#include "nsStackFrame.h"
#include "nsStyleContext.h"
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
  return new (aPresShell) nsStackFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsStackFrame)

nsStackFrame::nsStackFrame(nsStyleContext* aContext):
  nsBoxFrame(aContext)
{
  nsCOMPtr<nsBoxLayout> layout;
  NS_NewStackLayout(PresContext()->PresShell(), layout);
  SetLayoutManager(layout);
}







void
nsStackFrame::BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                          const nsRect&           aDirtyRect,
                                          const nsDisplayListSet& aLists)
{
  
  
  
  nsDisplayList* content = aLists.Content();
  nsDisplayListSet kidLists(content, content, content, content, content, content);
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    
    BuildDisplayListForChild(aBuilder, kid, aDirtyRect, kidLists,
                             DISPLAY_CHILD_FORCE_STACKING_CONTEXT);
    kid = kid->GetNextSibling();
  }
}
