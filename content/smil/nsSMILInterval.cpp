




































#include "nsSMILInterval.h"

nsSMILInterval::nsSMILInterval()
:
  mBeginObjectChanged(PR_FALSE),
  mEndObjectChanged(PR_FALSE)
{
}

nsSMILInterval::nsSMILInterval(const nsSMILInterval& aOther)
:
  mBegin(aOther.mBegin),
  mEnd(aOther.mEnd),
  mBeginObjectChanged(PR_FALSE),
  mEndObjectChanged(PR_FALSE)
{
  NS_ABORT_IF_FALSE(aOther.mDependentTimes.IsEmpty(),
      "Attempting to copy-construct an interval with dependent times, "
      "this will lead to instance times being shared between intervals.");
}

nsSMILInterval::~nsSMILInterval()
{
  NS_ABORT_IF_FALSE(mDependentTimes.IsEmpty(),
      "Destroying interval without disassociating dependent instance times. "
      "NotifyDeleting was not called.");
}

void
nsSMILInterval::NotifyChanged(const nsSMILTimeContainer* aContainer)
{
  for (PRInt32 i = mDependentTimes.Length() - 1; i >= 0; --i) {
    mDependentTimes[i]->HandleChangedInterval(aContainer,
                                              mBeginObjectChanged,
                                              mEndObjectChanged);
  }
  mBeginObjectChanged = PR_FALSE;
  mEndObjectChanged = PR_FALSE;
}

void
nsSMILInterval::NotifyDeleting()
{
  for (PRInt32 i = mDependentTimes.Length() - 1; i >= 0; --i) {
    mDependentTimes[i]->HandleDeletedInterval();
  }
  mDependentTimes.Clear();
}

nsSMILInstanceTime*
nsSMILInterval::Begin()
{
  NS_ABORT_IF_FALSE(mBegin && mEnd,
      "Requesting Begin() on un-initialized instance time.");
  return mBegin;
}

nsSMILInstanceTime*
nsSMILInterval::End()
{
  NS_ABORT_IF_FALSE(mBegin && mEnd,
      "Requesting End() on un-initialized instance time.");
  return mEnd;
}

void
nsSMILInterval::SetBegin(nsSMILInstanceTime& aBegin)
{
  NS_ABORT_IF_FALSE(aBegin.Time().IsResolved(),
      "Attempting to set unresolved begin time on interval.");

  if (mBegin == &aBegin)
    return;

  mBegin = &aBegin;
  mBeginObjectChanged = PR_TRUE;
}

void
nsSMILInterval::SetEnd(nsSMILInstanceTime& aEnd)
{
  if (mEnd == &aEnd)
    return;

  mEnd = &aEnd;
  mEndObjectChanged = PR_TRUE;
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
