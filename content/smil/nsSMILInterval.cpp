




































#include "nsSMILInterval.h"

nsSMILInterval::nsSMILInterval()
:
  mBeginFixed(PR_FALSE),
  mEndFixed(PR_FALSE)
{
}

nsSMILInterval::nsSMILInterval(const nsSMILInterval& aOther)
:
  mBegin(aOther.mBegin),
  mEnd(aOther.mEnd),
  mBeginFixed(PR_FALSE),
  mEndFixed(PR_FALSE)
{
  NS_ABORT_IF_FALSE(aOther.mDependentTimes.IsEmpty(),
      "Attempting to copy-construct an interval with dependent times, "
      "this will lead to instance times being shared between intervals.");

  
  
  
  
  NS_ABORT_IF_FALSE(!aOther.mBeginFixed && !aOther.mEndFixed,
      "Attempting to copy-construct an interval with fixed endpoints");
}

nsSMILInterval::~nsSMILInterval()
{
  NS_ABORT_IF_FALSE(mDependentTimes.IsEmpty(),
      "Destroying interval without disassociating dependent instance times. "
      "Unlink was not called");
}

void
nsSMILInterval::Unlink(PRBool aFiltered)
{
  for (PRInt32 i = mDependentTimes.Length() - 1; i >= 0; --i) {
    if (aFiltered) {
      mDependentTimes[i]->HandleFilteredInterval();
    } else {
      mDependentTimes[i]->HandleDeletedInterval();
    }
  }
  mDependentTimes.Clear();
  if (mBegin && mBeginFixed) {
    mBegin->ReleaseFixedEndpoint();
  }
  mBegin = nsnull;
  if (mEnd && mEndFixed) {
    mEnd->ReleaseFixedEndpoint();
  }
  mEnd = nsnull;
}

nsSMILInstanceTime*
nsSMILInterval::Begin()
{
  NS_ABORT_IF_FALSE(mBegin && mEnd,
      "Requesting Begin() on un-initialized interval.");
  return mBegin;
}

nsSMILInstanceTime*
nsSMILInterval::End()
{
  NS_ABORT_IF_FALSE(mBegin && mEnd,
      "Requesting End() on un-initialized interval.");
  return mEnd;
}

void
nsSMILInterval::SetBegin(nsSMILInstanceTime& aBegin)
{
  NS_ABORT_IF_FALSE(aBegin.Time().IsResolved(),
      "Attempting to set unresolved begin time on interval");
  NS_ABORT_IF_FALSE(!mBeginFixed,
      "Attempting to set begin time but the begin point is fixed");
  
  
  
  NS_ABORT_IF_FALSE(!mBegin || aBegin.GetBaseTime() != mBegin,
      "Attempting to make self-dependent instance time");

  mBegin = &aBegin;
}

void
nsSMILInterval::SetEnd(nsSMILInstanceTime& aEnd)
{
  NS_ABORT_IF_FALSE(!mEndFixed,
      "Attempting to set end time but the end point is fixed");
  
  
  NS_ABORT_IF_FALSE(!mEnd || aEnd.GetBaseTime() != mEnd,
      "Attempting to make self-dependent instance time");

  mEnd = &aEnd;
}

void
nsSMILInterval::FixBegin()
{
  NS_ABORT_IF_FALSE(mBegin && mEnd,
      "Fixing begin point on un-initialized interval");
  NS_ABORT_IF_FALSE(!mBeginFixed, "Duplicate calls to FixBegin()");
  mBeginFixed = PR_TRUE;
  mBegin->AddRefFixedEndpoint();
}

void
nsSMILInterval::FixEnd()
{
  NS_ABORT_IF_FALSE(mBegin && mEnd,
      "Fixing end point on un-initialized interval");
  NS_ABORT_IF_FALSE(mBeginFixed,
      "Fixing the end of an interval without a fixed begin");
  NS_ABORT_IF_FALSE(!mEndFixed, "Duplicate calls to FixEnd()");
  mEndFixed = PR_TRUE;
  mEnd->AddRefFixedEndpoint();
}

void
nsSMILInterval::AddDependentTime(nsSMILInstanceTime& aTime)
{
  nsRefPtr<nsSMILInstanceTime>* inserted =
    mDependentTimes.InsertElementSorted(&aTime);
  if (!inserted) {
    NS_WARNING("Insufficient memory to insert instance time.");
  }
}

void
nsSMILInterval::RemoveDependentTime(const nsSMILInstanceTime& aTime)
{
#ifdef DEBUG
  PRBool found =
#endif
    mDependentTimes.RemoveElementSorted(&aTime);
  NS_ABORT_IF_FALSE(found, "Couldn't find instance time to delete.");
}

void
nsSMILInterval::GetDependentTimes(InstanceTimeList& aTimes)
{
  aTimes = mDependentTimes;
}

PRBool
nsSMILInterval::IsDependencyChainLink() const
{
  if (!mBegin || !mEnd)
    return PR_FALSE; 

  if (mDependentTimes.IsEmpty())
    return PR_FALSE; 

  
  
  
  return (mBegin->IsDependent() && mBegin->GetBaseInterval() != this) ||
         (mEnd->IsDependent() && mEnd->GetBaseInterval() != this);
}
