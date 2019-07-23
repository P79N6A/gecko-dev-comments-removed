




































#ifndef nsHebrewProber_h__
#define nsHebrewProber_h__

#include "nsSBCharSetProber.h"



class nsHebrewProber: public nsCharSetProber
{
public:
  nsHebrewProber(void) :mLogicalProb(0), mVisualProb(0) { Reset(); }

  virtual ~nsHebrewProber(void) {}
  virtual nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  virtual const char* GetCharSetName();
  virtual void Reset(void);

  virtual nsProbingState GetState(void);

  virtual float     GetConfidence(void) { return (float)0.0; }
  virtual void      SetOpion() {}

  void SetModelProbers(nsCharSetProber *logicalPrb, nsCharSetProber *visualPrb) 
  { mLogicalProb = logicalPrb; mVisualProb = visualPrb; }

#ifdef DEBUG_chardet
  virtual void  DumpStatus();
#endif

protected:
  static PRBool isFinal(char c);
  static PRBool isNonFinal(char c);

  PRInt32 mFinalCharLogicalScore, mFinalCharVisualScore;

  
  char mPrev, mBeforePrev;

  
  nsCharSetProber *mLogicalProb, *mVisualProb;
};

































































































#endif 
