




































#include "nsSMILInstanceTime.h"

nsSMILInstanceTime::nsSMILInstanceTime(const nsSMILTimeValue& aTime,
    const nsSMILInstanceTime* aDependentTime,
    nsSMILInstanceTimeSource aSource)
: mTime(aTime),
  mFlags(0),
  mSerial(0)
{
  switch (aSource) {
    case SOURCE_NONE:
      
      break;

    case SOURCE_DOM:
      mFlags = kClearOnReset | kFromDOM;
      break;

    case SOURCE_SYNCBASE:
      mFlags = kMayUpdate;
      break;

    case SOURCE_EVENT:
      mFlags = kClearOnReset;
      break;
  }

  SetDependentTime(aDependentTime);
}

void
nsSMILInstanceTime::SetDependentTime(const nsSMILInstanceTime* aDependentTime)
{
  
  
  
  nsSMILInstanceTime* mutableDependentTime =
    const_cast<nsSMILInstanceTime*>(aDependentTime);

  
  
  
  
  if (aDependentTime) {
    mutableDependentTime->BreakPotentialCycle(this);
  }

  mDependentTime = mutableDependentTime;
}

void
nsSMILInstanceTime::BreakPotentialCycle(const nsSMILInstanceTime* aNewTail)
{
  if (!mDependentTime)
    return;

  if (mDependentTime == aNewTail) {
    
    
    mDependentTime = nsnull;
    return;
  }

  mDependentTime->BreakPotentialCycle(aNewTail);
}

PRBool
nsSMILInstanceTime::IsDependent(const nsSMILInstanceTime& aOther,
                                PRUint32 aRecursionDepth) const
{
  NS_ABORT_IF_FALSE(aRecursionDepth < 1000,
      "We seem to have created a cycle between instance times");

  if (!mDependentTime)
    return PR_FALSE;

  if (mDependentTime == &aOther)
    return PR_TRUE;

  return mDependentTime->IsDependent(aOther, ++aRecursionDepth);
}
