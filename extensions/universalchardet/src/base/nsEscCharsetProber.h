




#ifndef nsEscCharSetProber_h__
#define nsEscCharSetProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "nsAutoPtr.h"

class nsEscCharSetProber: public nsCharSetProber {
public:
  nsEscCharSetProber();
  virtual ~nsEscCharSetProber(void);
  nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  const char* GetCharSetName() {return mDetectedCharset;}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void){return (float)0.99;}

protected:
  void      GetDistribution(uint32_t aCharLen, const char* aStr);
  
  nsAutoPtr<nsCodingStateMachine> mCodingSM;
  nsProbingState mState;
  const char *  mDetectedCharset;
};

#endif 

