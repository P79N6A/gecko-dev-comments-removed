







#ifndef mozilla_RubyReflowState_h_
#define mozilla_RubyReflowState_h_

#include "mozilla/Attributes.h"
#include "WritingModes.h"
#include "nsTArray.h"

#define RTC_ARRAY_SIZE 1

class nsRubyTextContainerFrame;

namespace mozilla {

class MOZ_STACK_CLASS RubyReflowState MOZ_FINAL
{
public:
  explicit RubyReflowState(
    const nsTArray<nsRubyTextContainerFrame*>& aTextContainers);

  struct TextContainerInfo
  {
    nsRubyTextContainerFrame* mFrame;

    TextContainerInfo(nsRubyTextContainerFrame* aFrame)
      : mFrame(aFrame) { }
  };

  void AdvanceCurrentContainerIndex() { mCurrentContainerIndex++; }

  void SetTextContainerInfo(int32_t aIndex,
                            nsRubyTextContainerFrame* aContainer)
  {
    MOZ_ASSERT(mTextContainers[aIndex].mFrame == aContainer);
  }

  const TextContainerInfo&
    GetCurrentTextContainerInfo(nsRubyTextContainerFrame* aFrame) const
  {
    MOZ_ASSERT(mTextContainers[mCurrentContainerIndex].mFrame == aFrame);
    return mTextContainers[mCurrentContainerIndex];
  }

private:
  static MOZ_CONSTEXPR_VAR int32_t kBaseContainerIndex = -1;
  
  
  
  int32_t mCurrentContainerIndex;

  nsAutoTArray<TextContainerInfo, RTC_ARRAY_SIZE> mTextContainers;
};

}

#endif 
