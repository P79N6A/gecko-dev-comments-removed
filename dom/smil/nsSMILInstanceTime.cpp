




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
    mVisited(false),
    mFixedEndpointRefCnt(0),
    mSerial(0),
    mCreator(aCreator),
    mBaseInterval(nullptr) 
                          
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
  MOZ_ASSERT(!mBaseInterval,
             "Destroying instance time without first calling Unlink()");
  MOZ_ASSERT(mFixedEndpointRefCnt == 0,
             "Destroying instance time that is still used as the fixed "
             "endpoint of an interval");
}

void
nsSMILInstanceTime::Unlink()
{
  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  if (mBaseInterval) {
    mBaseInterval->RemoveDependentTime(*this);
    mBaseInterval = nullptr;
  }
  mCreator = nullptr;
}

void
nsSMILInstanceTime::HandleChangedInterval(
    const nsSMILTimeContainer* aSrcContainer,
    bool aBeginObjectChanged,
    bool aEndObjectChanged)
{
  
  
  
  
  if (!mBaseInterval)
    return;

  MOZ_ASSERT(mCreator, "Base interval is set but creator is not.");

  if (mVisited) {
    
    Unlink();
    return;
  }

  bool objectChanged = mCreator->DependsOnBegin() ? aBeginObjectChanged :
                                                      aEndObjectChanged;

  mozilla::AutoRestore<bool> setVisited(mVisited);
  mVisited = true;

  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  mCreator->HandleChangedInstanceTime(*GetBaseTime(), aSrcContainer, *this,
                                      objectChanged);
}

void
nsSMILInstanceTime::HandleDeletedInterval()
{
  MOZ_ASSERT(mBaseInterval,
             "Got call to HandleDeletedInterval on an independent instance "
             "time");
  MOZ_ASSERT(mCreator, "Base interval is set but creator is not");

  mBaseInterval = nullptr;
  mFlags &= ~kMayUpdate; 

  nsRefPtr<nsSMILInstanceTime> deathGrip(this);
  mCreator->HandleDeletedInstanceTime(*this);
  mCreator = nullptr;
}

void
nsSMILInstanceTime::HandleFilteredInterval()
{
  MOZ_ASSERT(mBaseInterval,
             "Got call to HandleFilteredInterval on an independent instance "
             "time");

  mBaseInterval = nullptr;
  mFlags &= ~kMayUpdate; 
  mCreator = nullptr;
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
  MOZ_ASSERT(mFixedEndpointRefCnt < UINT16_MAX,
             "Fixed endpoint reference count upper limit reached");
  ++mFixedEndpointRefCnt;
  mFlags &= ~kMayUpdate; 
}

void
nsSMILInstanceTime::ReleaseFixedEndpoint()
{
  MOZ_ASSERT(mFixedEndpointRefCnt > 0, "Duplicate release");
  --mFixedEndpointRefCnt;
  if (mFixedEndpointRefCnt == 0 && IsDynamic()) {
    mFlags |= kWasDynamicEndpoint;
  }
}

bool
nsSMILInstanceTime::IsDependentOn(const nsSMILInstanceTime& aOther) const
{
  if (mVisited)
    return false;

  const nsSMILInstanceTime* myBaseTime = GetBaseTime();
  if (!myBaseTime)
    return false;

  if (myBaseTime == &aOther)
    return true;

  mozilla::AutoRestore<bool> setVisited(mVisited);
  mVisited = true;
  return myBaseTime->IsDependentOn(aOther);
}

const nsSMILInstanceTime*
nsSMILInstanceTime::GetBaseTime() const
{
  if (!mBaseInterval) {
    return nullptr;
  }

  MOZ_ASSERT(mCreator, "Base interval is set but there is no creator.");
  if (!mCreator) {
    return nullptr;
  }

  return mCreator->DependsOnBegin() ? mBaseInterval->Begin() :
                                      mBaseInterval->End();
}

void
nsSMILInstanceTime::SetBaseInterval(nsSMILInterval* aBaseInterval)
{
  MOZ_ASSERT(!mBaseInterval,
             "Attempting to reassociate an instance time with a different "
             "interval.");

  if (aBaseInterval) {
    MOZ_ASSERT(mCreator,
               "Attempting to create a dependent instance time without "
               "reference to the creating nsSMILTimeValueSpec object.");
    if (!mCreator)
      return;

    aBaseInterval->AddDependentTime(*this);
  }

  mBaseInterval = aBaseInterval;
}
