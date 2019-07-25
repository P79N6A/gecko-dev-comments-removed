




































#ifndef NS_SMILTIMEVALUESPECPARAMS_H_
#define NS_SMILTIMEVALUESPECPARAMS_H_

#include "nsSMILTimeValue.h"
#include "nsAutoPtr.h"
#include "nsIAtom.h"







class nsSMILTimeValueSpecParams
{
public:
  nsSMILTimeValueSpecParams()
  :
    mType(INDEFINITE),
    mSyncBegin(PR_FALSE),
    mRepeatIterationOrAccessKey(0)
  { }

  
  enum {
    OFFSET,
    SYNCBASE,
    EVENT,
    REPEAT,
    ACCESSKEY,
    WALLCLOCK,
    INDEFINITE
  } mType;

  
  
  
  
  
  
  
  nsSMILTimeValue   mOffset;

  
  
  
  nsRefPtr<nsIAtom> mDependentElemID;

  
  
  nsRefPtr<nsIAtom> mEventSymbol;

  
  
  
  bool              mSyncBegin;

  
  
  PRUint32          mRepeatIterationOrAccessKey;
};

#endif 
