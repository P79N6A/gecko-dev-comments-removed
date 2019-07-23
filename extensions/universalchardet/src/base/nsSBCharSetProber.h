




































#ifndef nsSingleByteCharSetProber_h__
#define nsSingleByteCharSetProber_h__

#include "nsCharSetProber.h"

#define SAMPLE_SIZE 64
#define SB_ENOUGH_REL_THRESHOLD  1024
#define POSITIVE_SHORTCUT_THRESHOLD  (float)0.95
#define NEGATIVE_SHORTCUT_THRESHOLD  (float)0.05
#define SYMBOL_CAT_ORDER  250
#define NUMBER_OF_SEQ_CAT 4
#define POSITIVE_CAT   (NUMBER_OF_SEQ_CAT-1)
#define NEGATIVE_CAT   0

typedef struct
{
  unsigned char *charToOrderMap;    
  char *precedenceMatrix;           
  float  mTypicalPositiveRatio;     
  PRBool keepEnglishLetter;         
  const char* charsetName;
} SequenceModel;


class nsSingleByteCharSetProber : public nsCharSetProber{
public:
  nsSingleByteCharSetProber(SequenceModel *model) 
    :mModel(model), mReversed(PR_FALSE), mNameProber(0) { Reset(); }
  nsSingleByteCharSetProber(SequenceModel *model, PRBool reversed, nsCharSetProber* nameProber)
    :mModel(model), mReversed(reversed), mNameProber(nameProber) { Reset(); }

  virtual const char* GetCharSetName();
  virtual nsProbingState HandleData(const char* aBuf, PRUint32 aLen);
  virtual nsProbingState GetState(void) {return mState;};
  virtual void      Reset(void);
  virtual float     GetConfidence(void);
  virtual void      SetOpion() {};
  
  
  
  
  
  
  
  PRBool KeepEnglishLetters() {return mModel->keepEnglishLetter;}; 

#ifdef DEBUG_chardet
  virtual void  DumpStatus();
#endif

protected:
  nsProbingState mState;
  const SequenceModel *mModel;
  const PRBool mReversed; 

  
  unsigned char mLastOrder;

  PRUint32 mTotalSeqs;
  PRUint32 mSeqCounters[NUMBER_OF_SEQ_CAT];

  PRUint32 mTotalChar;
  
  PRUint32 mFreqChar;
  
  
  nsCharSetProber* mNameProber; 

};


extern SequenceModel Koi8rModel;
extern SequenceModel Win1251Model;
extern SequenceModel Latin5Model;
extern SequenceModel MacCyrillicModel;
extern SequenceModel Ibm866Model;
extern SequenceModel Ibm855Model;
extern SequenceModel Latin7Model;
extern SequenceModel Win1253Model;
extern SequenceModel Latin5BulgarianModel;
extern SequenceModel Win1251BulgarianModel;
extern SequenceModel Latin2HungarianModel;
extern SequenceModel Win1250HungarianModel;
extern SequenceModel Win1255Model;

#endif 

