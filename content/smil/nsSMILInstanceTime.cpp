




































#include "nsSMILInstanceTime.h"
#include "nsSMILInterval.h"
#include "nsSMILTimeValueSpec.h"




namespace
{
  
  
  class AutoBoolSetter
  {
  public:
    AutoBoolSetter(PRPackedBool& aValue)
    : mValue(aValue)
    {
      mValue = PR_TRUE;
    }
 
    ~AutoBoolSetter()
    {
      mValue = PR_FALSE;
    }

  private:
    PRPackedBool&   mValue;
  };
}




nsSMILInstanceTime::nsSMILInstanceTime(const nsSMILTimeValue& aTime,
                                       nsSMILInstanceTimeSource aSource,
                                       nsSMILTimeValueSpec* aCreator,
                                       nsSMILInterval* aBaseInterval)
  : mTime(aTime),
    mFlags(0),
    mSerial(0),
    mVisited(PR_FALSE),
    mChainEnd(PR_FALSE),
    mCreator(aCreator),
    mBaseInterval(nsnull)
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

  SetBaseInterval(aBaseInterval);
}

nsSMILInstanceTime::~nsSMILInstanceTime()
{
  NS_ABORT_IF_FALSE(!mBaseInterval && !mCreator,
      "Destroying instance time without first calling Unlink()");
}

void
nsSMILInstanceTime::Unlink()
{
  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  if (mBaseInterval) {
    mBaseInterval->RemoveDependentTime(*this);
    mBaseInterval = nsnull;
  }
  mCreator = nsnull;
}

void
nsSMILInstanceTime::HandleChangedInterval(
    const nsSMILTimeContainer* aSrcContainer,
    PRBool aBeginObjectChanged,
    PRBool aEndObjectChanged)
{
  NS_ABORT_IF_FALSE(mBaseInterval,
      "Got call to HandleChangedInterval on an independent instance time.");
  NS_ABORT_IF_FALSE(mCreator, "Base interval is set but creator is not.");

  if (mVisited || mChainEnd) {
    
    
    
    
    mChainEnd = PR_TRUE;
    return;
  }

  PRBool objectChanged = mCreator->DependsOnBegin() ? aBeginObjectChanged
                                                    : aEndObjectChanged;
  AutoBoolSetter setVisited(mVisited);

  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  mCreator->HandleChangedInstanceTime(*GetBaseTime(), aSrcContainer, *this,
                                      objectChanged);
}

void
nsSMILInstanceTime::HandleDeletedInterval()
{
  NS_ABORT_IF_FALSE(mBaseInterval,
      "Got call to HandleDeletedInterval on an independent instance time.");
  NS_ABORT_IF_FALSE(mCreator, "Base interval is set but creator is not.");

  mBaseInterval = nsnull;

  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  mCreator->HandleDeletedInstanceTime(*this);
  mCreator = nsnull;
}

PRBool
nsSMILInstanceTime::IsDependent(const nsSMILInstanceTime& aOther,
                                PRUint32 aRecursionDepth) const
{
  NS_ABORT_IF_FALSE(aRecursionDepth < 1000,
      "We seem to have created a cycle between instance times");

  const nsSMILInstanceTime* myBaseTime = GetBaseTime();
  if (!myBaseTime)
    return PR_FALSE;

  if (myBaseTime == &aOther)
    return PR_TRUE;

  return myBaseTime->IsDependent(aOther, ++aRecursionDepth);
}

void
nsSMILInstanceTime::SetBaseInterval(nsSMILInterval* aBaseInterval)
{
  NS_ABORT_IF_FALSE(!mBaseInterval,
      "Attepting to reassociate an instance time with a different interval.");

  
  
  if (aBaseInterval) {
    NS_ABORT_IF_FALSE(mCreator,
        "Attempting to create a dependent instance time without reference "
        "to the creating nsSMILTimeValueSpec object.");
    if (!mCreator)
      return;

    const nsSMILInstanceTime* dependentTime = mCreator->DependsOnBegin()
                                            ? aBaseInterval->Begin()
                                            : aBaseInterval->End();
    dependentTime->BreakPotentialCycle(this);
    aBaseInterval->AddDependentTime(*this);
  }

  mBaseInterval = aBaseInterval;
}

const nsSMILInstanceTime*
nsSMILInstanceTime::GetBaseTime() const
{
  if (!mBaseInterval) {
    return nsnull;
  }

  NS_ABORT_IF_FALSE(mCreator, "Base interval is set but there is no creator.");
  if (!mCreator) {
    return nsnull;
  }

  return mCreator->DependsOnBegin() ? mBaseInterval->Begin()
                                    : mBaseInterval->End();
}

void
nsSMILInstanceTime::BreakPotentialCycle(
    const nsSMILInstanceTime* aNewTail) const
{
  const nsSMILInstanceTime* myBaseTime = GetBaseTime();
  if (!myBaseTime)
    return;

  if (myBaseTime == aNewTail) {
    
    
    mBaseInterval->RemoveDependentTime(*this);
    return;
  }

  myBaseTime->BreakPotentialCycle(aNewTail);
}
