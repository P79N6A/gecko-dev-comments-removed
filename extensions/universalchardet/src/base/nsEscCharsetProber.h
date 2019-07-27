




#ifndef nsEscCharSetProber_h__
#define nsEscCharSetProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"

#define NUM_OF_ESC_CHARSETS   4

class nsEscCharSetProber: public nsCharSetProber {
public:
  explicit nsEscCharSetProber(uint32_t aLanguageFilter);
  virtual ~nsEscCharSetProber(void);
  nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  const char* GetCharSetName() {return mDetectedCharset;}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void){return (float)0.99;}

protected:
  void      GetDistribution(uint32_t aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM[NUM_OF_ESC_CHARSETS] ;
  uint32_t    mActiveSM;
  nsProbingState mState;
  const char *  mDetectedCharset;
};

#endif 

