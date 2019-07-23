




































#ifndef nsBig5Prober_h__
#define nsBig5Prober_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "CharDistribution.h"

class nsBig5Prober: public nsCharSetProber {
public:
  nsBig5Prober(void){mCodingSM = new nsCodingStateMachine(&Big5SMModel);
                      Reset();};
  virtual ~nsBig5Prober(void){delete mCodingSM;};
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "Big5";};
  nsProbingState GetState(void) {return mState;};
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {};

protected:
  void      GetDistribution(PRUint32 aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  
  Big5DistributionAnalysis mDistributionAnalyser;
  char mLastChar[2];

};


#endif 

