




































#include "nsLegendFrame.h"
#include "nsIContent.h"
#include "nsIAtom.h"
#include "nsGenericHTMLElement.h"
#include "nsHTMLParts.h"
#include "nsGkAtoms.h"
#include "nsStyleConsts.h"
#include "nsFormControlFrame.h"

nsIFrame*
NS_NewLegendFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
#ifdef DEBUG
  const nsStyleDisplay* disp = aContext->GetStyleDisplay();
  NS_ASSERTION(!disp->IsAbsolutelyPositioned() && !disp->IsFloating(),
               "Legends should not be positioned and should not float");
#endif

  nsIFrame* f = new (aPresShell) nsLegendFrame(aContext);
  if (f) {
    f->AddStateBits(NS_BLOCK_FLOAT_MGR | NS_BLOCK_MARGIN_ROOT);
  }
  return f;
}

NS_IMPL_FRAMEARENA_HELPERS(nsLegendFrame)

nsIAtom*
nsLegendFrame::GetType() const
{
  return nsGkAtoms::legendFrame; 
}

void
nsLegendFrame::Destroy()
{
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_FALSE);
  nsBlockFrame::Destroy();
}

NS_QUERYFRAME_HEAD(nsLegendFrame)
  NS_QUERYFRAME_ENTRY(nsLegendFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

NS_IMETHODIMP 
nsLegendFrame::Reflow(nsPresContext*          aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsLegendFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_TRUE);
  }
  return nsBlockFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);
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
