




































#ifndef nsEUCKRProber_h__
#define nsEUCKRProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "CharDistribution.h"

class nsEUCKRProber: public nsCharSetProber {
public:
  nsEUCKRProber(void){mCodingSM = new nsCodingStateMachine(&EUCKRSMModel);
                      Reset();};
  virtual ~nsEUCKRProber(void){delete mCodingSM;};
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "EUC-KR";};
  nsProbingState GetState(void) {return mState;};
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {};

protected:
  void      GetDistribution(PRUint32 aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  
  EUCKRDistributionAnalysis mDistributionAnalyser;
  char mLastChar[2];

};


#endif 

