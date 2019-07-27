




#include "FrameMetrics.h"
#include "gfxPrefs.h"

namespace mozilla {
namespace layers {

const FrameMetrics::ViewID FrameMetrics::NULL_SCROLL_ID = 0;
const FrameMetrics FrameMetrics::sNullMetrics;

void
FrameMetrics::SetUsesContainerScrolling(bool aValue) {
  MOZ_ASSERT_IF(aValue, gfxPrefs::LayoutUseContainersForRootFrames());
  mUsesContainerScrolling = aValue;
}

}
}