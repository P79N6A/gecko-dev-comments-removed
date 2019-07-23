




































#include "nsUTF8Prober.h"

void  nsUTF8Prober::Reset(void)
{
  mCodingSM->Reset(); 
  mNumOfMBChar = 0;
  mState = eDetecting;
}

nsProbingState nsUTF8Prober::HandleData(const char* aBuf, PRUint32 aLen)
{
  nsSMState codingState;

  for (PRUint32 i = 0; i < aLen; i++)
  {
    codingState = mCodingSM->NextState(aBuf[i]);
    if (codingState == eError)
    {
      mState = eNotMe;
      break;
    }
    if (codingState == eItsMe)
    {
      mState = eFoundIt;
      break;
    }
    if (codingState == eStart)
    {
      if (mCodingSM->GetCurrentCharLen() >= 2)
        mNumOfMBChar++;
    }
  }

  if (mState == eDetecting)
    if (GetConfidence() > SHORTCUT_THRESHOLD)
      mState = eFoundIt;
  return mState;
}

#define ONE_CHAR_PROB   (float)0.50

float nsUTF8Prober::GetConfidence(void)
{
  float unlike = (float)0.99;

  if (mNumOfMBChar < 6)
  {
    for (PRUint32 i = 0; i < mNumOfMBChar; i++)
      unlike *= ONE_CHAR_PROB;
    return (float)1.0 - unlike;
  }
  else
    return (float)0.99;
}

