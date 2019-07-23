




































#ifndef NS_SMILTIMEVALUE_H_
#define NS_SMILTIMEVALUE_H_

#include "prtypes.h"
#include "prlong.h"
#include "nsSMILTypes.h"
#include "nsDebug.h"

























































class nsSMILTimeValue
{
public:
  
  nsSMILTimeValue()
  : mMilliseconds(kUnresolvedMillis),
    mState(STATE_UNRESOLVED)
  { }

  
  explicit nsSMILTimeValue(nsSMILTime aMillis)
  : mMilliseconds(aMillis),
    mState(STATE_RESOLVED)
  { }

  
  static nsSMILTimeValue Indefinite()
  {
    nsSMILTimeValue value;
    value.SetIndefinite();
    return value;
  }

  PRBool IsIndefinite() const { return mState == STATE_INDEFINITE; }
  void SetIndefinite()
  {
    mState = STATE_INDEFINITE;
    mMilliseconds = kUnresolvedMillis;
  }

  PRBool IsResolved() const { return mState == STATE_RESOLVED; }
  void SetUnresolved()
  {
    mState = STATE_UNRESOLVED;
    mMilliseconds = kUnresolvedMillis;
  }

  nsSMILTime GetMillis() const
  {
    NS_ABORT_IF_FALSE(mState == STATE_RESOLVED,
       "GetMillis() called for unresolved time");

    return mState == STATE_RESOLVED ? mMilliseconds : kUnresolvedMillis;
  }

  void SetMillis(nsSMILTime aMillis)
  {
    mState = STATE_RESOLVED;
    mMilliseconds = aMillis;
  }

  PRInt8 CompareTo(const nsSMILTimeValue& aOther) const;

  PRBool operator==(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) == 0; }

  PRBool operator!=(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) != 0; }

  PRBool operator<(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) < 0; }

  PRBool operator>(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) > 0; }

  PRBool operator<=(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) <= 0; }

  PRBool operator>=(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) >= 0; }

private:
  static nsSMILTime kUnresolvedMillis;

  nsSMILTime        mMilliseconds;
  enum {
    STATE_RESOLVED,
    STATE_INDEFINITE,
    STATE_UNRESOLVED
  } mState;
};

#endif 
