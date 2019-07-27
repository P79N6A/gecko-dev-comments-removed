







#include "nsRubyContentFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsCSSAnonBoxes.h"

using namespace mozilla;






NS_IMPL_FRAMEARENA_HELPERS(nsRubyContentFrame)






 bool
nsRubyContentFrame::IsFrameOfType(uint32_t aFlags) const
{
  if (aFlags & eBidiInlineContainer) {
    return false;
  }
  return nsRubyContentFrameSuper::IsFrameOfType(aFlags);
}

bool
nsRubyContentFrame::IsIntraLevelWhitespace() const
{
  nsIAtom* pseudoType = StyleContext()->GetPseudo();
  if (pseudoType != nsCSSAnonBoxes::rubyBase &&
      pseudoType != nsCSSAnonBoxes::rubyText) {
    return false;
  }

  nsIFrame* child = mFrames.OnlyChild();
  return child && child->GetContent()->TextIsOnlyWhitespace();
}
