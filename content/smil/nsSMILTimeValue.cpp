




































#include "nsSMILTimeValue.h"
#include "nsDebug.h"

nsSMILTime nsSMILTimeValue::kUnresolvedSeconds = LL_MAXINT;





nsSMILTimeValue::nsSMILTimeValue()
  : mMilliseconds(LL_MAXINT),
    mState(STATE_UNRESOLVED)
{
}




inline PRInt8
nsSMILTimeValue::Cmp(PRInt64 aA, PRInt64 aB) const
{
  return aA == aB ? 0 : (aA > aB ? 1 : -1);
}

void
nsSMILTimeValue::SetIndefinite()
{
  mState = STATE_INDEFINITE;
  mMilliseconds = LL_MAXINT;
}

void
nsSMILTimeValue::SetUnresolved()
{
  mState = STATE_UNRESOLVED;
  mMilliseconds = LL_MAXINT;
}

nsSMILTime
nsSMILTimeValue::GetMillis() const
{
  NS_ASSERTION(mState == STATE_RESOLVED,
               "GetMillis() called for unresolved time.");

  if (mState != STATE_RESOLVED)
      return kUnresolvedSeconds;

  return mMilliseconds;
}

void
nsSMILTimeValue::SetMillis(nsSMILTime aMillis)
{
  mState = STATE_RESOLVED;
  mMilliseconds = aMillis;
}

PRInt8
nsSMILTimeValue::CompareTo(const nsSMILTimeValue& aOther) const
{
  PRInt8 result;

  if (mState == STATE_RESOLVED) {
    result = (aOther.mState == STATE_RESOLVED)
           ? Cmp(mMilliseconds, aOther.mMilliseconds)
           : -1;
  } else if (mState == STATE_INDEFINITE) {
    if (aOther.mState == STATE_RESOLVED)
      result = 1;
    else if (aOther.mState == STATE_INDEFINITE)
      result = 0;
    else
      result = -1;
  } else {
    result = (aOther.mState != STATE_UNRESOLVED) ? 1 : 0;
  }

  return result;
}
