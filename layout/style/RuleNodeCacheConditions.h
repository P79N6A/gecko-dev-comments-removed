










#ifndef RuleNodeCacheConditions_h_
#define RuleNodeCacheConditions_h_

#include "mozilla/Attributes.h"
#include "nsCoord.h"
#include "nsTArray.h"

class nsStyleContext;

namespace mozilla {

class RuleNodeCacheConditions
{
public:
  RuleNodeCacheConditions()
    : mFontSize(0), mBits(0) {}
  RuleNodeCacheConditions(const RuleNodeCacheConditions& aOther)
    : mFontSize(aOther.mFontSize), mBits(aOther.mBits) {}
  RuleNodeCacheConditions& operator=(const RuleNodeCacheConditions& aOther)
  {
    mFontSize = aOther.mFontSize;
    mBits = aOther.mBits;
    return *this;
  }
  bool operator==(const RuleNodeCacheConditions& aOther) const
  {
    return mFontSize == aOther.mFontSize &&
           mBits == aOther.mBits;
  }
  bool operator!=(const RuleNodeCacheConditions& aOther) const
  {
    return !(*this == aOther);
  }

  bool Matches(nsStyleContext* aStyleContext) const;

  void SetFontSizeDependency(nscoord aCoord)
  {
    MOZ_ASSERT(!(mBits & eHaveFontSize) || mFontSize == aCoord);
    mFontSize = aCoord;
    mBits |= eHaveFontSize;
  }

  void SetWritingModeDependency(uint8_t aWritingMode)
  {
    MOZ_ASSERT(!(mBits & eHaveWritingMode) || GetWritingMode() == aWritingMode);
    mBits |= (static_cast<uint64_t>(aWritingMode) << eWritingModeShift) |
             eHaveWritingMode;
  }

  void SetUncacheable()
  {
    mBits |= eUncacheable;
  }

  bool Cacheable() const
  {
    return !(mBits & eUncacheable);
  }

  bool CacheableWithDependencies() const
  {
    return !(mBits & eUncacheable) &&
           (mBits & eHaveBitsMask) != 0;
  }

  bool CacheableWithoutDependencies() const
  {
    
    
    return (mBits & eHaveBitsMask) == 0;
  }

#ifdef DEBUG
  void List() const;
#endif

private:
  enum {
    eUncacheable      = 0x0001,
    eHaveFontSize     = 0x0002,
    eHaveWritingMode  = 0x0004,
    eHaveBitsMask     = 0x00ff,
    eWritingModeMask  = 0xff00,
    eWritingModeShift = 8,
  };

  uint8_t GetWritingMode() const
  {
    return static_cast<uint8_t>(
        (mBits & eWritingModeMask) >> eWritingModeShift);
  }

  
  nscoord mFontSize;

  
  
  
  
  
  
  
  uint32_t mBits;
};

} 

#endif 
