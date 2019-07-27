









#include "nsSJISProber.h"
#include "nsDebug.h"

void  nsSJISProber::Reset(void)
{
  mCodingSM->Reset(); 
  mState = eDetecting;
  mContextAnalyser.Reset();
  mDistributionAnalyser.Reset();
}

nsProbingState nsSJISProber::HandleData(const char* aBuf, uint32_t aLen)
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
        mContextAnalyser.HandleOneChar(mLastChar+2-charLen, charLen);
        mDistributionAnalyser.HandleOneChar(mLastChar, charLen);
      }
      else
      {
        mContextAnalyser.HandleOneChar(aBuf+i+1-charLen, charLen);
        mDistributionAnalyser.HandleOneChar(aBuf+i-1, charLen);
      }
    }
  }

  mLastChar[0] = aBuf[aLen-1];

  if (mState == eDetecting)
    if (mContextAnalyser.GotEnoughData() && GetConfidence() > SHORTCUT_THRESHOLD)
      mState = eFoundIt;

  return mState;
}

float nsSJISProber::GetConfidence(void)
{
  float contxtCf = mContextAnalyser.GetConfidence();
  float distribCf = mDistributionAnalyser.GetConfidence();

  return (contxtCf > distribCf ? contxtCf : distribCf);
}

