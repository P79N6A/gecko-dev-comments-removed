




































#ifndef nsGB2312Prober_h__
#define nsGB2312Prober_h__

#include "nsCharSetProber.h"
#include "nsCodingStateMachine.h"
#include "CharDistribution.h"



class nsGB18030Prober: public nsCharSetProber {
public:
  nsGB18030Prober(void){mCodingSM = new nsCodingStateMachine(&GB18030SMModel);
                      Reset();}
  virtual ~nsGB18030Prober(void){delete mCodingSM;}
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "gb18030";}
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {}

protected:
  void      GetDistribution(PRUint32 aCharLen, const char* aStr);
  
  nsCodingStateMachine* mCodingSM;
  nsProbingState mState;

  
  GB2312DistributionAnalysis mDistributionAnalyser;
  char mLastChar[2];

};


#endif 

