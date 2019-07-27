



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
  const unsigned char* const charToOrderMap;    
  const uint8_t* const precedenceMatrix;  
  float  mTypicalPositiveRatio;     
  bool keepEnglishLetter;         
  const char* const charsetName;
} SequenceModel;


class nsSingleByteCharSetProber : public nsCharSetProber{
public:
  explicit nsSingleByteCharSetProber(const SequenceModel *model) 
    :mModel(model), mReversed(false), mNameProber(0) { Reset(); }
  nsSingleByteCharSetProber(const SequenceModel *model, bool reversed, nsCharSetProber* nameProber)
    :mModel(model), mReversed(reversed), mNameProber(nameProber) { Reset(); }

  virtual const char* GetCharSetName();
  virtual nsProbingState HandleData(const char* aBuf, uint32_t aLen);
  virtual nsProbingState GetState(void) {return mState;}
  virtual void      Reset(void);
  virtual float     GetConfidence(void);
  
  
  
  
  
  
  
  bool KeepEnglishLetters() {return mModel->keepEnglishLetter;} 

#ifdef DEBUG_chardet
  virtual void  DumpStatus();
#endif

protected:
  nsProbingState mState;
  const SequenceModel* const mModel;
  const bool mReversed; 

  
  unsigned char mLastOrder;

  uint32_t mTotalSeqs;
  uint32_t mSeqCounters[NUMBER_OF_SEQ_CAT];

  uint32_t mTotalChar;
  
  uint32_t mFreqChar;
  
  
  nsCharSetProber* mNameProber; 

};


extern const SequenceModel Koi8rModel;
extern const SequenceModel Win1251Model;
extern const SequenceModel Latin5Model;
extern const SequenceModel MacCyrillicModel;
extern const SequenceModel Ibm866Model;
extern const SequenceModel Ibm855Model;
extern const SequenceModel Latin7Model;
extern const SequenceModel Win1253Model;
extern const SequenceModel Latin5BulgarianModel;
extern const SequenceModel Win1251BulgarianModel;
extern const SequenceModel Latin2HungarianModel;
extern const SequenceModel Win1250HungarianModel;
extern const SequenceModel Win1255Model;
extern const SequenceModel TIS620ThaiModel;

#endif 

