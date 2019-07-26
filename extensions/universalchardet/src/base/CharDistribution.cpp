




#include "CharDistribution.h"

#include "JISFreq.tab"
#include "Big5Freq.tab"
#include "EUCKRFreq.tab"
#include "EUCTWFreq.tab"
#include "GB2312Freq.tab"
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

EUCTWDistributionAnalysis::EUCTWDistributionAnalysis()
{
  mCharToFreqOrder = EUCTWCharToFreqOrder;
  mTableSize = mozilla::ArrayLength(EUCTWCharToFreqOrder);
  mTypicalDistributionRatio = EUCTW_TYPICAL_DISTRIBUTION_RATIO;
}

EUCKRDistributionAnalysis::EUCKRDistributionAnalysis()
{
  mCharToFreqOrder = EUCKRCharToFreqOrder;
  mTableSize = mozilla::ArrayLength(EUCKRCharToFreqOrder);
  mTypicalDistributionRatio = EUCKR_TYPICAL_DISTRIBUTION_RATIO;
}

GB2312DistributionAnalysis::GB2312DistributionAnalysis()
{
  mCharToFreqOrder = GB2312CharToFreqOrder;
  mTableSize = mozilla::ArrayLength(GB2312CharToFreqOrder);
  mTypicalDistributionRatio = GB2312_TYPICAL_DISTRIBUTION_RATIO;
}

Big5DistributionAnalysis::Big5DistributionAnalysis()
{
  mCharToFreqOrder = Big5CharToFreqOrder;
  mTableSize = mozilla::ArrayLength(Big5CharToFreqOrder);
  mTypicalDistributionRatio = BIG5_TYPICAL_DISTRIBUTION_RATIO;
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

