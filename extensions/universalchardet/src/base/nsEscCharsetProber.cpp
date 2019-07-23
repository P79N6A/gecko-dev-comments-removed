





































#include "nsEscCharsetProber.h"

nsEscCharSetProber::nsEscCharSetProber(void)
{
  mCodingSM[0] = new nsCodingStateMachine(&HZSMModel);
  mCodingSM[1] = new nsCodingStateMachine(&ISO2022CNSMModel);
  mCodingSM[2] = new nsCodingStateMachine(&ISO2022JPSMModel);
  mCodingSM[3] = new nsCodingStateMachine(&ISO2022KRSMModel);
  mActiveSM = NUM_OF_ESC_CHARSETS;
  mState = eDetecting;
  mDetectedCharset = nsnull;
}

nsEscCharSetProber::~nsEscCharSetProber(void)
{
  for (PRUint32 i = 0; i < NUM_OF_ESC_CHARSETS; i++)
    delete mCodingSM[i];
}

void nsEscCharSetProber::Reset(void)
{
  mState = eDetecting;
  for (PRUint32 i = 0; i < NUM_OF_ESC_CHARSETS; i++)
    mCodingSM[i]->Reset();
  mActiveSM = NUM_OF_ESC_CHARSETS;
  mDetectedCharset = nsnull;
}

nsProbingState nsEscCharSetProber::HandleData(const char* aBuf, PRUint32 aLen)
{
  nsSMState codingState;
  PRInt32 j;
  PRUint32 i;

  for ( i = 0; i < aLen && mState == eDetecting; i++)
  {
    for (j = mActiveSM-1; j>= 0; j--)
    {
      
      codingState = mCodingSM[j]->NextState(aBuf[i]);
      if (codingState == eError)
      {
        
        mActiveSM--;
        if (mActiveSM == 0)
        {
          mState = eNotMe;
          return mState;
        }
        else if (j != (PRInt32)mActiveSM)
        {
          nsCodingStateMachine* t;
          t = mCodingSM[mActiveSM];
          mCodingSM[mActiveSM] = mCodingSM[j];
          mCodingSM[j] = t;
        }
      }
      else if (codingState == eItsMe)
      {
        mState = eFoundIt;
        mDetectedCharset = mCodingSM[j]->GetCodingStateMachine();
        return mState;
      }
    }
  }

  return mState;
}

