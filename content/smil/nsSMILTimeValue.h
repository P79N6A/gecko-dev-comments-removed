




































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
    mState(STATE_DEFINITE)
  { }

  
  static nsSMILTimeValue Indefinite()
  {
    nsSMILTimeValue value;
    value.SetIndefinite();
    return value;
  }

  bool IsIndefinite() const { return mState == STATE_INDEFINITE; }
  void SetIndefinite()
  {
    mState = STATE_INDEFINITE;
    mMilliseconds = kUnresolvedMillis;
  }

  bool IsResolved() const { return mState != STATE_UNRESOLVED; }
  void SetUnresolved()
  {
    mState = STATE_UNRESOLVED;
    mMilliseconds = kUnresolvedMillis;
  }

  bool IsDefinite() const { return mState == STATE_DEFINITE; }
  nsSMILTime GetMillis() const
  {
    NS_ABORT_IF_FALSE(mState == STATE_DEFINITE,
       "GetMillis() called for unresolved or indefinite time");

    return mState == STATE_DEFINITE ? mMilliseconds : kUnresolvedMillis;
  }

  void SetMillis(nsSMILTime aMillis)
  {
    mState = STATE_DEFINITE;
    mMilliseconds = aMillis;
  }

  PRInt8 CompareTo(const nsSMILTimeValue& aOther) const;

  bool operator==(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) == 0; }

  bool operator!=(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) != 0; }

  bool operator<(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) < 0; }

  bool operator>(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) > 0; }

  bool operator<=(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) <= 0; }

  bool operator>=(const nsSMILTimeValue& aOther) const
  { return CompareTo(aOther) >= 0; }

private:
  static nsSMILTime kUnresolvedMillis;

  nsSMILTime mMilliseconds;
  enum {
    STATE_DEFINITE,
    STATE_INDEFINITE,
    STATE_UNRESOLVED
  } mState;
};

#endif 
