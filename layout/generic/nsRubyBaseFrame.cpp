







#include "nsRubyBaseFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyBaseFrame)
  NS_QUERYFRAME_ENTRY(nsRubyBaseFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsInlineFrame)

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

 bool
nsRubyBaseFrame::IsFrameOfType(uint32_t aFlags) const
{
  if (aFlags & eBidiInlineContainer) {
    return false;
  }
  return nsRubyBaseFrameSuper::IsFrameOfType(aFlags);
}
