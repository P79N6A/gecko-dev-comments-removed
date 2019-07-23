









































#ifndef nsSJISProber_h__
#define nsSJISProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "JpCntx.h"
#include "CharDistribution.h"


class nsSJISProber: public nsCharSetProber {
public:
  nsSJISProber(void){mCodingSM = new nsCodingStateMachine(&SJISSMModel);
                      Reset();}
  virtual ~nsSJISProber(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "Shift_JIS";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {}

protected:
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  SJISContextAnalysis mContextAnalyser;
  SJISDistributionAnalysis mDistributionAnalyser;

  char mLastChar[2];

};


#endif 

