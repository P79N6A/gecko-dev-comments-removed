




































#ifndef nsEUCTWProber_h__
#define nsEUCTWProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "CharDistribution.h"

class nsEUCTWProber: public nsCharSetProber {
public:
  nsEUCTWProber(void){mCodingSM = new nsCodingStateMachine(&EUCTWSMModel);
                      Reset();}
  virtual ~nsEUCTWProber(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "x-euc-tw";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {}

protected:
  void      GetDistribution(PRUint32 aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  
  EUCTWDistributionAnalysis mDistributionAnalyser;
  char mLastChar[2];

};


#endif 

