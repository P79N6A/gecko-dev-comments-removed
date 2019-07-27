



#ifndef nsCodingStateMachine_h__
#define nsCodingStateMachine_h__

#include "mozilla/ArrayUtils.h"
 
#include "nsPkgInt.h"

typedef enum {
   eStart = 0,
   eError = 1,
   eItsMe = 2 
} nsSMState;

#define GETCLASS(c) GETFROMPCK(((unsigned char)(c)), mModel->classTable)


typedef struct 
{
  nsPkgInt classTable;
  uint32_t classFactor;
  nsPkgInt stateTable;
  const uint32_t* charLenTable;
#ifdef DEBUG
  const size_t charLenTableLength;
#endif
  const char* name;
} SMModel;

class nsCodingStateMachine {
public:
  explicit nsCodingStateMachine(const SMModel* sm) : mModel(sm) { mCurrentState = eStart; }
  nsSMState NextState(char c){
    
    uint32_t byteCls = GETCLASS(c);
    if (mCurrentState == eStart)
    { 
      mCurrentBytePos = 0; 
      MOZ_ASSERT(byteCls < mModel->charLenTableLength);
      mCurrentCharLen = mModel->charLenTable[byteCls];
    }
    
    mCurrentState=(nsSMState)GETFROMPCK(mCurrentState*(mModel->classFactor)+byteCls,
                                       mModel->stateTable);
    mCurrentBytePos++;
    return mCurrentState;
  }
  uint32_t  GetCurrentCharLen(void) {return mCurrentCharLen;}
  void      Reset(void) {mCurrentState = eStart;}
  const char * GetCodingStateMachine() {return mModel->name;}

protected:
  nsSMState mCurrentState;
  uint32_t mCurrentCharLen;
  uint32_t mCurrentBytePos;

  const SMModel *mModel;
};

extern const SMModel UTF8SMModel;
extern const SMModel Big5SMModel;
extern const SMModel EUCJPSMModel;
extern const SMModel EUCKRSMModel;
extern const SMModel EUCTWSMModel;
extern const SMModel GB18030SMModel;
extern const SMModel SJISSMModel;


extern const SMModel HZSMModel;
extern const SMModel ISO2022CNSMModel;
extern const SMModel ISO2022JPSMModel;
extern const SMModel ISO2022KRSMModel;

#undef CHAR_LEN_TABLE
#ifdef DEBUG
#define CHAR_LEN_TABLE(x) x, mozilla::ArrayLength(x)
#else
#define CHAR_LEN_TABLE(x) x
#endif

#endif 

