





































#ifndef nsSBCSGroupProber_h__
#define nsSBCSGroupProber_h__


#define NUM_OF_SBCS_PROBERS    13

class nsCharSetProber;
class nsSBCSGroupProber: public nsCharSetProber {
public:
  nsSBCSGroupProber();
  virtual ~nsSBCSGroupProber();
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName();
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {}

#ifdef DEBUG_chardet
  void  DumpStatus();
#endif

protected:
  nsProbingState mState;
  nsCharSetProber* mProbers[NUM_OF_SBCS_PROBERS];
  PRBool          mIsActive[NUM_OF_SBCS_PROBERS];
  PRInt32 mBestGuess;
  PRUint32 mActiveNum;
};

#endif 

