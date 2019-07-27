







#include "nsRubyTextContainerFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"






NS_QUERYFRAME_HEAD(nsRubyTextContainerFrame)
  NS_QUERYFRAME_ENTRY(nsRubyTextContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyTextContainerFrame)

nsContainerFrame*
NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                             nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyTextContainerFrame(aContext);
}







nsIAtom*
nsRubyTextContainerFrame::GetType() const
{
  return nsGkAtoms::rubyTextContainerFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyTextContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyTextContainer"), aResult);
}
#endif
