











































#include "nsProgressMeterFrame.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsCOMPtr.h"
#include "nsBoxLayoutState.h"





nsIFrame*
NS_NewProgressMeterFrame (nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsProgressMeterFrame(aPresShell, aContext);
} 






nsProgressMeterFrame :: ~nsProgressMeterFrame ( )
{
}

NS_IMETHODIMP
nsProgressMeterFrame::SetInitialChildList(nsIAtom*        aListName,
                                          nsIFrame*       aChildList)
{ 
  
  nsresult rv = nsBoxFrame::SetInitialChildList(aListName, aChildList);
  AttributeChanged(kNameSpaceID_None, nsGkAtoms::value, 0);
  return rv;
}

NS_IMETHODIMP
nsProgressMeterFrame::AttributeChanged(PRInt32 aNameSpaceID,
                                       nsIAtom* aAttribute,
                                       PRInt32 aModType)
{
  nsresult rv = nsBoxFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                             aModType);
  if (NS_OK != rv) {
    return rv;
  }

  
  if (nsGkAtoms::value == aAttribute) {
    nsIFrame* barChild = GetFirstChild(nsnull);
    if (!barChild) return NS_OK;
    nsIFrame* remainderChild = barChild->GetNextSibling();
    if (!remainderChild) return NS_OK;
    nsCOMPtr<nsIContent> remainderContent = remainderChild->GetContent();
    if (!remainderContent) return NS_OK;

    nsAutoString value;
    mContent->GetAttr(kNameSpaceID_None, nsGkAtoms::value, value);

    PRInt32 error;
    PRInt32 flex = value.ToInteger(&error);
    if (flex < 0) flex = 0;
    if (flex > 100) flex = 100;

    PRInt32 remainder = 100 - flex;

    nsAutoString leftFlex, rightFlex;
    leftFlex.AppendInt(flex);
    rightFlex.AppendInt(remainder);
    nsWeakFrame weakFrame(this);
    barChild->GetContent()->SetAttr(kNameSpaceID_None, nsGkAtoms::flex, leftFlex, PR_TRUE);
    remainderContent->SetAttr(kNameSpaceID_None, nsGkAtoms::flex, rightFlex, PR_TRUE);

    if (weakFrame.IsAlive()) {
      AddStateBits(NS_FRAME_IS_DIRTY);
      GetPresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eTreeChange);
    }
  }
  return NS_OK;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsProgressMeterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("ProgressMeter"), aResult);
}
#endif
