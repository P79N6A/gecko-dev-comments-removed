









































#ifndef nsEUCJPProber_h__
#define nsEUCJPProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "JpCntx.h"
#include "CharDistribution.h"

class nsEUCJPProber: public nsCharSetProber {
public:
  nsEUCJPProber(void){mCodingSM = new nsCodingStateMachine(&EUCJPSMModel);
                      Reset();}
  virtual ~nsEUCJPProber(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "EUC-JP";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {}

protected:
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  EUCJPContextAnalysis mContextAnalyser;
  EUCJPDistributionAnalysis mDistributionAnalyser;

  char mLastChar[2];
};


#endif 

