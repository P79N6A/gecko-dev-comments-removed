





































#ifndef nsMBCSGroupProber_h__
#define nsMBCSGroupProber_h__

#include "nsSJISProber.h"
#include "nsUTF8Prober.h"
#include "nsEUCJPProber.h"
#include "nsGB2312Prober.h"
#include "nsEUCKRProber.h"
#include "nsBig5Prober.h"
#include "nsEUCTWProber.h"

#define NUM_OF_PROBERS    7

class nsMBCSGroupProber: public nsCharSetProber {
public:
  nsMBCSGroupProber();
  virtual ~nsMBCSGroupProber();
  nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  const char* GetCharSetName();
  nsProbingState GetState(void) {return mState;}
  void      Reset(void);
  float     GetConfidence(void);
  void      SetOpion() {}

#ifdef DEBUG_chardet
  void  DumpStatus();
#endif
#ifdef DEBUG_jgmyers
  void GetDetectorState(nsUniversalDetector::DetectorState (&states)[nsUniversalDetector::NumDetectors], PRUint32 &offset);
#endif

protected:
  nsProbingState mState;
  nsCharSetProber* mProbers[NUM_OF_PROBERS];
  PRBool          mIsActive[NUM_OF_PROBERS];
  PRInt32 mBestGuess;
  PRUint32 mActiveNum;
  PRUint32 mKeepNext;
};

#endif 

