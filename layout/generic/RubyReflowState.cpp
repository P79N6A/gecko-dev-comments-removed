







#include "RubyReflowState.h"

using namespace mozilla;

RubyReflowState::RubyReflowState(
  const nsTArray<nsRubyTextContainerFrame*>& aTextContainers)
  : mCurrentContainerIndex(kBaseContainerIndex)
{
  uint32_t rtcCount = aTextContainers.Length();
  mTextContainers.SetCapacity(rtcCount);
  for (uint32_t i = 0; i < rtcCount; i++) {
    mTextContainers.AppendElement(TextContainerInfo(aTextContainers[i]));
  }
}
