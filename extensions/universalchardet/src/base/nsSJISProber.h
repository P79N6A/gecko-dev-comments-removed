









#ifndef nsSJISProber_h__
#define nsSJISProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "JpCntx.h"
#include "CharDistribution.h"


class nsSJISProber: public nsCharSetProber {
public:
  explicit nsSJISProber(bool aIsPreferredLanguage)
    :mIsPreferredLanguage(aIsPreferredLanguage)
  {mCodingSM = new nsCodingStateMachine(&SJISSMModel);
    Reset();}
  virtual ~nsSJISProber(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  const char* GetCharSetName() {return "Shift_JIS";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);

protected:
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  SJISContextAnalysis mContextAnalyser;
  SJISDistributionAnalysis mDistributionAnalyser;

  char mLastChar[2];
  bool mIsPreferredLanguage;

};


#endif 

