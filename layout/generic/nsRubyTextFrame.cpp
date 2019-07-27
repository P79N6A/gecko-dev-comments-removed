







#include "nsRubyTextFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"






NS_QUERYFRAME_HEAD(nsRubyTextFrame)
  NS_QUERYFRAME_ENTRY(nsRubyTextFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyTextFrame)

nsContainerFrame*
NS_NewRubyTextFrame(nsIPresShell* aPresShell,
                    nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyTextFrame(aContext);
}







nsIAtom*
nsRubyTextFrame::GetType() const
{
  return nsGkAtoms::rubyTextFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyTextFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyText"), aResult);
}
#endif
