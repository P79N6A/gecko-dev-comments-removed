




































#include "nsSMILInstanceTime.h"
#include "nsSMILInterval.h"
#include "nsSMILTimeValueSpec.h"
#include "mozilla/AutoRestore.h"




nsSMILInstanceTime::nsSMILInstanceTime(const nsSMILTimeValue& aTime,
                                       nsSMILInstanceTimeSource aSource,
                                       nsSMILTimeValueSpec* aCreator,
                                       nsSMILInterval* aBaseInterval)
  : mTime(aTime),
    mFlags(0),
    mVisited(PR_FALSE),
    mFixedEndpointRefCnt(0),
    mSerial(0),
    mCreator(aCreator),
    mBaseInterval(nsnull) 
                          
{
  switch (aSource) {
    case SOURCE_NONE:
      
      break;

    case SOURCE_DOM:
      mFlags = kDynamic | kFromDOM;
      break;

    case SOURCE_SYNCBASE:
      mFlags = kMayUpdate;
      break;

    case SOURCE_EVENT:
      mFlags = kDynamic;
      break;
  }

  SetBaseInterval(aBaseInterval);
}

nsSMILInstanceTime::~nsSMILInstanceTime()
{
  NS_ABORT_IF_FALSE(!mBaseInterval,
      "Destroying instance time without first calling Unlink()");
  NS_ABORT_IF_FALSE(mFixedEndpointRefCnt == 0,
      "Destroying instance time that is still used as the fixed endpoint of an "
      "interval");
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
    bool aBeginObjectChanged,
    bool aEndObjectChanged)
{
  
  
  
  
  if (!mBaseInterval)
    return;

  NS_ABORT_IF_FALSE(mCreator, "Base interval is set but creator is not.");

  if (mVisited) {
    
    Unlink();
    return;
  }

  bool objectChanged = mCreator->DependsOnBegin() ? aBeginObjectChanged :
                                                      aEndObjectChanged;

  mozilla::AutoRestore<bool> setVisited(mVisited);
  mVisited = PR_TRUE;

  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  mCreator->HandleChangedInstanceTime(*GetBaseTime(), aSrcContainer, *this,
                                      objectChanged);
}

void
nsSMILInstanceTime::HandleDeletedInterval()
{
  NS_ABORT_IF_FALSE(mBaseInterval,
      "Got call to HandleDeletedInterval on an independent instance time");
  NS_ABORT_IF_FALSE(mCreator, "Base interval is set but creator is not");

  mBaseInterval = nsnull;
  mFlags &= ~kMayUpdate; 

  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  mCreator->HandleDeletedInstanceTime(*this);
  mCreator = nsnull;
}

void
nsSMILInstanceTime::HandleFilteredInterval()
{
  NS_ABORT_IF_FALSE(mBaseInterval,
      "Got call to HandleFilteredInterval on an independent instance time");

  mBaseInterval = nsnull;
  mFlags &= ~kMayUpdate; 
  mCreator = nsnull;
}

bool
nsSMILInstanceTime::ShouldPreserve() const
{
  return mFixedEndpointRefCnt > 0 || (mFlags & kWasDynamicEndpoint);
}

void
nsSMILInstanceTime::UnmarkShouldPreserve()
{
  mFlags &= ~kWasDynamicEndpoint;
}

void
nsSMILInstanceTime::AddRefFixedEndpoint()
{
  NS_ABORT_IF_FALSE(mFixedEndpointRefCnt < PR_UINT16_MAX,
      "Fixed endpoint reference count upper limit reached");
  ++mFixedEndpointRefCnt;
  mFlags &= ~kMayUpdate; 
}

void
nsSMILInstanceTime::ReleaseFixedEndpoint()
{
  NS_ABORT_IF_FALSE(mFixedEndpointRefCnt > 0, "Duplicate release");
  --mFixedEndpointRefCnt;
  if (mFixedEndpointRefCnt == 0 && IsDynamic()) {
    mFlags |= kWasDynamicEndpoint;
  }
}

bool
nsSMILInstanceTime::IsDependentOn(const nsSMILInstanceTime& aOther) const
{
  if (mVisited)
    return PR_FALSE;

  const nsSMILInstanceTime* myBaseTime = GetBaseTime();
  if (!myBaseTime)
    return PR_FALSE;

  if (myBaseTime == &aOther)
    return PR_TRUE;

  
  mozilla::AutoRestore<bool> setVisited(const_cast<nsSMILInstanceTime*>(this)->mVisited);
  const_cast<nsSMILInstanceTime*>(this)->mVisited = PR_TRUE;
  return myBaseTime->IsDependentOn(aOther);
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

  return mCreator->DependsOnBegin() ? mBaseInterval->Begin() :
                                      mBaseInterval->End();
}

void
nsSMILInstanceTime::SetBaseInterval(nsSMILInterval* aBaseInterval)
{
  NS_ABORT_IF_FALSE(!mBaseInterval,
      "Attempting to reassociate an instance time with a different interval.");

  if (aBaseInterval) {
    NS_ABORT_IF_FALSE(mCreator,
        "Attempting to create a dependent instance time without reference "
        "to the creating nsSMILTimeValueSpec object.");
    if (!mCreator)
      return;

    aBaseInterval->AddDependentTime(*this);
  }

  mBaseInterval = aBaseInterval;
}
