




#ifndef nsSBCSGroupProber_h__
#define nsSBCSGroupProber_h__


#define NUM_OF_SBCS_PROBERS    14

class nsCharSetProber;
class nsSBCSGroupProber: public nsCharSetProber {
public:
  nsSBCSGroupProber();
  virtual ~nsSBCSGroupProber();
  nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  const char* GetCharSetName();
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);

#ifdef DEBUG_chardet
  void  DumpStatus();
#endif

protected:
  nsProbingState mState;
  nsCharSetProber* mProbers[NUM_OF_SBCS_PROBERS];
  bool            mIsActive[NUM_OF_SBCS_PROBERS];
  int32_t mBestGuess;
  uint32_t mActiveNum;
};

#endif 

