




































#ifndef nsEscCharSetProber_h__
#define nsEscCharSetProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"

#define NUM_OF_ESC_CHARSETS   4

class nsEscCharSetProber: public nsCharSetProber {
public:
  nsEscCharSetProber(void);
  virtual ~nsEscCharSetProber(void);
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return mDetectedCharset;};
  nsProbingState GetState(void) {return mState;};
  void      Reset(void);
  float     GetConfidence(void){return (float)0.99;};
  void      SetOpion() {};

protected:
  void      GetDistribution(PRUint32 aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM[NUM_OF_ESC_CHARSETS] ;
  PRUint32    mActiveSM;
  nsProbingState mState;
  const char *  mDetectedCharset;
};

#endif 

