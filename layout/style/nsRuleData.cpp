





































#include "nsRuleData.h"
#include "nsCSSProps.h"
#include "nsPresArena.h"

inline size_t
nsRuleData::GetPoisonOffset()
{
  
  
  
  MOZ_STATIC_ASSERT(sizeof(PRUword) == sizeof(size_t),
                    "expect PRUword and size_t to be the same size");
  MOZ_STATIC_ASSERT(PRUword(-1) > PRUword(0),
                    "expect PRUword to be unsigned");
  MOZ_STATIC_ASSERT(size_t(-1) > size_t(0),
                    "expect size_t to be unsigned");
  PRUword framePoisonValue = nsPresArena::GetPoisonValue();
  return size_t(framePoisonValue - PRUword(mValueStorage)) /
         sizeof(nsCSSValue);
}

nsRuleData::nsRuleData(PRUint32 aSIDs, nsCSSValue* aValueStorage,
                       nsPresContext* aContext, nsStyleContext* aStyleContext)
  : mSIDs(aSIDs),
    mCanStoreInRuleTree(true),
    mPresContext(aContext),
    mStyleContext(aStyleContext),
    mPostResolveCallback(nsnull),
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
