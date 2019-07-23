




































#include "nsSMILInstanceTime.h"
#include "nsSMILTimeValueSpec.h"
#include "nsSMILTimeValue.h"




nsSMILInstanceTime::nsSMILInstanceTime(const nsSMILTimeValue& aTime,
                                       nsSMILTimeValueSpec* ,
                                       PRBool aClearOnReset )
  : mTime(aTime), 
    mClearOnReset(aClearOnReset)
{
  
}

nsSMILInstanceTime::~nsSMILInstanceTime()
{
  
  
  
}
