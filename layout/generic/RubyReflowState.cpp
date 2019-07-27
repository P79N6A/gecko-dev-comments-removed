







#include "RubyReflowState.h"

using namespace mozilla;

RubyReflowState::RubyReflowState(
  WritingMode aLineWM,
  const nsTArray<nsRubyTextContainerFrame*>& aTextContainers)
  : mCurrentContainerIndex(kBaseContainerIndex)
{
  uint32_t rtcCount = aTextContainers.Length();
  mTextContainers.SetCapacity(rtcCount);
  for (uint32_t i = 0; i < rtcCount; i++) {
    mTextContainers.AppendElement(
      TextContainerInfo(aLineWM, aTextContainers[i]));
  }
}
