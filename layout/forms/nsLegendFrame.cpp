






































#include "nsLegendFrame.h"
#include "nsIDOMNode.h"
#include "nsIDOMHTMLLegendElement.h"
#include "nsCSSRendering.h"
#include "nsIContent.h"
#include "nsIFrame.h"
#include "nsISupports.h"
#include "nsIAtom.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsFont.h"
#include "nsFormControlFrame.h"

static NS_DEFINE_IID(kLegendFrameCID, NS_LEGEND_FRAME_CID);
 
nsIFrame*
NS_NewLegendFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  nsIFrame* f = new (aPresShell) nsLegendFrame(aContext);
  if (f) {
    f->AddStateBits(NS_BLOCK_SPACE_MGR | NS_BLOCK_MARGIN_ROOT);
  }
  return f;
}

nsIAtom*
nsLegendFrame::GetType() const
{
  return nsGkAtoms::legendFrame; 
}

void
nsLegendFrame::Destroy()
{
  nsFormControlFrame::RegUnRegAccessKey(NS_STATIC_CAST(nsIFrame*, this), PR_FALSE);
  nsAreaFrame::Destroy();
}


NS_IMETHODIMP
nsLegendFrame::QueryInterface(REFNSIID aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(kLegendFrameCID)) {
    *aInstancePtr = this;
    return NS_OK;
  }

  return nsAreaFrame::QueryInterface(aIID, aInstancePtr);
}

NS_IMETHODIMP 
nsLegendFrame::Reflow(nsPresContext*          aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsLegendFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(NS_STATIC_CAST(nsIFrame*, this), PR_TRUE);
  }
  return nsAreaFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
}



PRInt32 nsLegendFrame::GetAlign()
{
  PRInt32 intValue = NS_STYLE_TEXT_ALIGN_LEFT;
#ifdef IBMBIDI
  if (mParent && NS_STYLE_DIRECTION_RTL == mParent->GetStyleVisibility()->mDirection) {
    intValue = NS_STYLE_TEXT_ALIGN_RIGHT;
  }
#endif 

  nsGenericHTMLElement *content = nsGenericHTMLElement::FromContent(mContent);

  if (content) {
    const nsAttrValue* attr = content->GetParsedAttr(nsGkAtoms::align);
    if (attr && attr->Type() == nsAttrValue::eEnum) {
      intValue = attr->GetEnumValue();
    }
  }
  return intValue;
}

#ifdef NS_DEBUG
NS_IMETHODIMP
nsLegendFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Legend"), aResult);
}
#endif
