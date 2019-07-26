




#include "nsRuleData.h"
#include "nsPresArena.h"

#include "mozilla/StandardInteger.h"

inline size_t
nsRuleData::GetPoisonOffset()
{
  
  
  
  MOZ_STATIC_ASSERT(sizeof(uintptr_t) == sizeof(size_t),
                    "expect uintptr_t and size_t to be the same size");
  MOZ_STATIC_ASSERT(uintptr_t(-1) > uintptr_t(0),
                    "expect uintptr_t to be unsigned");
  MOZ_STATIC_ASSERT(size_t(-1) > size_t(0),
                    "expect size_t to be unsigned");
  uintptr_t framePoisonValue = nsPresArena::GetPoisonValue();
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
    NS_ABORT_IF_FALSE(!(mSIDs & (1 << i)) ||
                      mValueOffsets[i] != framePoisonOffset,
                      "value in SIDs was left with poison offset");
  }
#endif
}
#endif
