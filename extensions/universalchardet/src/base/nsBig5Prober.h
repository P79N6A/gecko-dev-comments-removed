




#ifndef nsBig5Prober_h__
#define nsBig5Prober_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "CharDistribution.h"

class nsBig5Prober: public nsCharSetProber {
public:
  explicit nsBig5Prober(bool aIsPreferredLanguage)
    :mIsPreferredLanguage(aIsPreferredLanguage) 
  {mCodingSM = new nsCodingStateMachine(&Big5SMModel); 
    Reset();}
  virtual ~nsBig5Prober(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  const char* GetCharSetName() {return "Big5";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);

protected:
  void      GetDistribution(uint32_t aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  
  Big5DistributionAnalysis mDistributionAnalyser;
  char mLastChar[2];
  bool mIsPreferredLanguage;

};


#endif 

