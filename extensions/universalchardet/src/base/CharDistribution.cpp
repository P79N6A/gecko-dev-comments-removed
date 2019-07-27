




#include "CharDistribution.h"

#include "JISFreq.tab"
#include "mozilla/ArrayUtils.h"

#define SURE_YES 0.99f
#define SURE_NO  0.01f


float CharDistributionAnalysis::GetConfidence(void)
{ 
  
  
  
  if (mTotalChars <= 0 || mFreqChars <= mDataThreshold)
    return SURE_NO;

  if (mTotalChars != mFreqChars) {
    float r = mFreqChars / ((mTotalChars - mFreqChars) * mTypicalDistributionRatio);

    if (r < SURE_YES)
      return r;
  }
  
  return SURE_YES;
}

SJISDistributionAnalysis::SJISDistributionAnalysis()
{
  mCharToFreqOrder = JISCharToFreqOrder;
  mTableSize = mozilla::ArrayLength(JISCharToFreqOrder);
  mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO;
}

EUCJPDistributionAnalysis::EUCJPDistributionAnalysis()
{
  mCharToFreqOrder = JISCharToFreqOrder;
  mTableSize = mozilla::ArrayLength(JISCharToFreqOrder);
  mTypicalDistributionRatio = JIS_TYPICAL_DISTRIBUTION_RATIO;
}

