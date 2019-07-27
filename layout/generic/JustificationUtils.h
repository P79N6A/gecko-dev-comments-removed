





#ifndef mozilla_JustificationUtils_h_
#define mozilla_JustificationUtils_h_

#include "mozilla/Attributes.h"

namespace mozilla {


























struct JustificationInfo
{
  
  
  int32_t mInnerOpportunities;
  
  bool mIsStartJustifiable;
  bool mIsEndJustifiable;

  MOZ_CONSTEXPR JustificationInfo()
    : mInnerOpportunities(0)
    , mIsStartJustifiable(false)
    , mIsEndJustifiable(false)
  {
  }

  
  
  void CancelOpportunityForTrimmedSpace()
  {
    if (mInnerOpportunities > 0) {
      mInnerOpportunities--;
    } else {
      
      
      
      
      mIsStartJustifiable = false;
      mIsEndJustifiable = false;
    }
  }
};

struct JustificationAssignment
{
  
  uint8_t mGapsAtStart : 2;
  uint8_t mGapsAtEnd : 2;

  MOZ_CONSTEXPR JustificationAssignment()
    : mGapsAtStart(0)
    , mGapsAtEnd(0)
  {
  }

  int32_t TotalGaps() const { return mGapsAtStart + mGapsAtEnd; }
};

struct JustificationApplicationState
{
  struct
  {
    
    int32_t mCount;
    
    int32_t mHandled;
  } mGaps;

  struct
  {
    
    nscoord mAvailable;
    
    nscoord mConsumed;
  } mWidth;

  JustificationApplicationState(int32_t aGaps, nscoord aWidth)
  {
    mGaps.mCount = aGaps;
    mGaps.mHandled = 0;
    mWidth.mAvailable = aWidth;
    mWidth.mConsumed = 0;
  }

  bool IsJustifiable() const
  {
    return mGaps.mCount > 0 && mWidth.mAvailable > 0;
  }

  nscoord Consume(int32_t aGaps)
  {
    mGaps.mHandled += aGaps;
    nscoord newAllocate = (mWidth.mAvailable * mGaps.mHandled) / mGaps.mCount;
    nscoord deltaWidth = newAllocate - mWidth.mConsumed;
    mWidth.mConsumed = newAllocate;
    return deltaWidth;
  }
};

class JustificationUtils
{
public:
  
  static int32_t CountGaps(const JustificationInfo& aInfo,
                           const JustificationAssignment& aAssign)
  {
    
    
    return aInfo.mInnerOpportunities * 2 + aAssign.TotalGaps();
  }
};

}

#endif 
