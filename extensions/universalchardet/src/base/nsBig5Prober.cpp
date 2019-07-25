




#include "nsBig5Prober.h"
#include "nsDebug.h"

void  nsBig5Prober::Reset(void)
{
  mCodingSM->Reset(); 
  mState = eDetecting;
  mDistributionAnalyser.Reset(mIsPreferredLanguage);
}

nsProbingState nsBig5Prober::HandleData(const char* aBuf, uint32_t aLen)
{
  NS_ASSERTION(aLen, "HandleData called with empty buffer");
  nsSMState codingState;

  for (uint32_t i = 0; i < aLen; i++)
  {
    codingState = mCodingSM->NextState(aBuf[i]);
    if (codingState == eItsMe)
    {
      mState = eFoundIt;
      break;
    }
    if (codingState == eStart)
    {
      uint32_t charLen = mCodingSM->GetCurrentCharLen();

      if (i == 0)
      {
        mLastChar[1] = aBuf[0];
        mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
      }
      else
        mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
    }
  }

  mLastChar[0] = aBuf[aLen-1];

  if (mState == eDetecting)
    if (mDistributionAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
      mState = eFoundIt;

  return mState;
}

float nsBig5Prober::GetConfidence(void)
{
  float distribCf = mDistributionAnalyser.GetConfidence();

  return (float)distribCf;
}

