





































#ifndef nsLatin1Prober_h__
#define nsLatin1Prober_h__

#include "nsCharSetProber.h"

#define FREQ_CAT_NUM    4

class nsLatin1Prober: public nsCharSetProber {
public:
  nsLatin1Prober(void){Reset();};
  virtual ~nsLatin1Prober(void){};
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName() {return "windows-1252";};
  nsProbingState GetState(void) {return mState;};
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {};

#ifdef DEBUG_chardet
  virtual void  DumpStatus();
#endif

protected:
  
  nsProbingState mState;
  char mLastCharClass;
  PRUint32 mFreqCounter[FREQ_CAT_NUM];
};


#endif 

