




#include "nsRuleData.h"

#include "mozilla/Poison.h"
#include <stdint.h>

inline size_t
nsRuleData::GetPoisonOffset()
{
  
  
  
  static_assert(sizeof(uintptr_t) == sizeof(size_t),
                "expect uintptr_t and size_t to be the same size");
  static_assert(uintptr_t(-1) > uintptr_t(0),
                "expect uintptr_t to be unsigned");
  static_assert(size_t(-1) > size_t(0),
                "expect size_t to be unsigned");
  uintptr_t framePoisonValue = mozPoisonValue();
  return size_t(framePoisonValue - uintptr_t(mValueStorage)) /
         sizeof(nsCSSValue);
}

nsRuleData::nsRuleData(uint32_t aSIDs, nsCSSValue* aValueStorage,
                       nsPresContext* aContext, nsStyleContext* aStyleContext)
  : mSIDs(aSIDs),
    mCanStoreInRuleTree(true),
    mPresContext(aContext),
    mStyleContext(aStyleContext),
    mValueStorage(aValueStorage)
{
#ifndef MOZ_VALGRIND
  size_t framePoisonOffset = GetPoisonOffset();
  for (size_t i = 0; i < nsStyleStructID_Length; ++i) {
    mValueOffsets[i] = framePoisonOffset;
  }
#endif
}

#ifdef DEBUG
nsRuleData::~nsRuleData()
{
#ifndef MOZ_VALGRIND
  
  size_t framePoisonOffset = GetPoisonOffset();
  for (size_t i = 0; i < nsStyleStructID_Length; ++i) {
    MOZ_ASSERT(!(mSIDs & (1 << i)) || mValueOffsets[i] != framePoisonOffset,
               "value in SIDs was left with poison offset");
  }
#endif
}
#endif
