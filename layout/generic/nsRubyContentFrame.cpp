







#include "nsRubyContentFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"

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
