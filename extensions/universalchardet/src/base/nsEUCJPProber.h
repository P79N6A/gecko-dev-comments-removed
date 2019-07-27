









#ifndef nsEUCJPProber_h__
#define nsEUCJPProber_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "JpCntx.h"
#include "CharDistribution.h"

class nsEUCJPProber: public nsCharSetProber {
public:
  explicit nsEUCJPProber(bool aIsPreferredLanguage)
    :mIsPreferredLanguage(aIsPreferredLanguage)
  {mCodingSM = new nsCodingStateMachine(&EUCJPSMModel);
    Reset();}
  virtual ~nsEUCJPProber(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  const char* GetCharSetName() {return "EUC-JP";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);

protected:
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  EUCJPContextAnalysis mContextAnalyser;
  EUCJPDistributionAnalysis mDistributionAnalyser;

  char mLastChar[2];
  bool mIsPreferredLanguage;
};


#endif 

