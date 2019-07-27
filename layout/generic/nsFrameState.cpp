






#include "nsFrameState.h"

#include "nsBlockFrame.h"
#include "nsBoxFrame.h"
#include "nsBulletFrame.h"
#include "nsFlexContainerFrame.h"
#include "nsGfxScrollFrame.h"
#include "nsIFrame.h"
#include "nsISVGChildFrame.h"
#include "nsImageFrame.h"
#include "nsInlineFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsRubyTextFrame.h"
#include "nsRubyTextContainerFrame.h"
#include "nsSVGContainerFrame.h"
#include "nsTableCellFrame.h"
#include "nsTableRowFrame.h"
#include "nsTableRowGroupFrame.h"
#include "nsTextFrame.h"

namespace mozilla {

#ifdef DEBUG
nsCString
GetFrameState(nsIFrame* aFrame)
{
  nsCString result;
  nsAutoTArray<const char*,3> groups;

  nsFrameState state = aFrame->GetStateBits();

  if (state == nsFrameState(0)) {
    result.Assign('0');
    return result;
  }

#define FRAME_STATE_GROUP(name_, class_)                                      \
  {                                                                           \
    class_* frame = do_QueryFrame(aFrame);                                    \
    if (frame && (groups.IsEmpty() || strcmp(groups.LastElement(), #name_))) {\
      groups.AppendElement(#name_);                                           \
    }                                                                         \
  }
#define FRAME_STATE_BIT(group_, value_, name_)                                \
  if ((state & NS_FRAME_STATE_BIT(value_)) && groups.Contains(#group_)) {     \
    if (!result.IsEmpty()) {                                                  \
      result.Insert(" | ", 0);                                                \
    }                                                                         \
    result.Insert(#name_, 0);                                                 \
    state = state & ~NS_FRAME_STATE_BIT(value_);                              \
  }
#include "nsFrameStateBits.h"
#undef FRAME_STATE_GROUP
#undef FRAME_STATE_BIT

  if (state) {
    result.AppendPrintf(" | 0x%0llx", state);
  }

  return result;
}

void
PrintFrameState(nsIFrame* aFrame)
{
  printf("%s\n", GetFrameState(aFrame).get());
}
#endif

} 
