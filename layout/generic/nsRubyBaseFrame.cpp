







#include "nsRubyBaseFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"






NS_QUERYFRAME_HEAD(nsRubyBaseFrame)
  NS_QUERYFRAME_ENTRY(nsRubyBaseFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyBaseFrame)

nsContainerFrame*
NS_NewRubyBaseFrame(nsIPresShell* aPresShell,
                    nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyBaseFrame(aContext);
}







nsIAtom*
nsRubyBaseFrame::GetType() const
{
  return nsGkAtoms::rubyBaseFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyBaseFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyBase"), aResult);
}
#endif
