





#include "nsEscCharsetProber.h"
#include "nsUniversalDetector.h"

nsEscCharSetProber::nsEscCharSetProber()
{
  mCodingSM = new nsCodingStateMachine(&ISO2022JPSMModel);
  mState = eDetecting;
  mDetectedCharset = nullptr;
}

nsEscCharSetProber::~nsEscCharSetProber(void)
{
}

void nsEscCharSetProber::Reset(void)
{
  mState = eDetecting;
  mCodingSM->Reset();
  mDetectedCharset = nullptr;
}

nsProbingState nsEscCharSetProber::HandleData(const char* aBuf, uint32_t aLen)
{
  nsSMState codingState;
  uint32_t i;

  for ( i = 0; i < aLen && mState == eDetecting; i++)
  {
    codingState = mCodingSM->NextState(aBuf[i]);
    if (codingState == eItsMe)
    {
      mState = eFoundIt;
      mDetectedCharset = mCodingSM->GetCodingStateMachine();
      return mState;
    }
  }

  return mState;
}

