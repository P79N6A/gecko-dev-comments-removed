



































#ifndef nsCodingStateMachine_h__
#define nsCodingStateMachine_h__

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
  PRUint32 classFactor;
  nsPkgInt stateTable;
  const PRUint32* charLenTable;
  const char* name;
} SMModel;

class nsCodingStateMachine {
public:
  nsCodingStateMachine(SMModel* sm){
          mCurrentState = eStart;
          mModel = sm;
        }
  nsSMState NextState(char c){
    
    PRUint32 byteCls = GETCLASS(c);
    if (mCurrentState == eStart)
    { 
      mCurrentBytePos = 0; 
      mCurrentCharLen = mModel->charLenTable[byteCls];
    }
    
    mCurrentState=(nsSMState)GETFROMPCK(mCurrentState*(mModel->classFactor)+byteCls,
                                       mModel->stateTable);
    mCurrentBytePos++;
    return mCurrentState;
  }
  PRUint32  GetCurrentCharLen(void) {return mCurrentCharLen;}
  void      Reset(void) {mCurrentState = eStart;}
  const char * GetCodingStateMachine() {return mModel->name;}

protected:
  nsSMState mCurrentState;
  PRUint32 mCurrentCharLen;
  PRUint32 mCurrentBytePos;

  SMModel *mModel;
};

extern SMModel UTF8SMModel;
extern SMModel Big5SMModel;
extern SMModel EUCJPSMModel;
extern SMModel EUCKRSMModel;
extern SMModel EUCTWSMModel;
extern SMModel GB18030SMModel;
extern SMModel SJISSMModel;
extern SMModel UCS2BESMModel;


extern SMModel HZSMModel;
extern SMModel ISO2022CNSMModel;
extern SMModel ISO2022JPSMModel;
extern SMModel ISO2022KRSMModel;

#endif 

