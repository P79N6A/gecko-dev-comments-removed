




































#ifndef NS_SMILINTERVAL_H_
#define NS_SMILINTERVAL_H_

#include "nsSMILInstanceTime.h"










class nsSMILInterval
{
public:
  void Set(nsSMILInstanceTime& aBegin, nsSMILInstanceTime& aEnd)
  {
    NS_ABORT_IF_FALSE(aBegin.Time().IsResolved(),
        "Attempting to set unresolved begin time on an interval");
    mBegin = &aBegin;
    mEnd = &aEnd;
  }

  PRBool IsSet() const
  {
    NS_ABORT_IF_FALSE(!mBegin == !mEnd, "Bad interval: only one endpoint set");
    return !!mBegin;
  }

  void Reset()
  {
    mBegin = nsnull;
    mEnd = nsnull;
  }

  
  

  const nsSMILInstanceTime* Begin() const
  {
    NS_ABORT_IF_FALSE(mBegin, "Calling Begin() on un-set interval");
    return mBegin;
  }

  nsSMILInstanceTime* Begin()
  {
    NS_ABORT_IF_FALSE(mBegin, "Calling Begin() on un-set interval");
    return mBegin;
  }

  const nsSMILInstanceTime* End() const
  {
    NS_ABORT_IF_FALSE(mEnd, "Calling End() on un-set interval");
    return mEnd;
  }

  nsSMILInstanceTime* End()
  {
    NS_ABORT_IF_FALSE(mEnd, "Calling End() on un-set interval");
    return mEnd;
  }

  void SetBegin(nsSMILInstanceTime& aBegin)
  {
    NS_ABORT_IF_FALSE(mBegin, "Calling SetBegin() on un-set interval");
    NS_ABORT_IF_FALSE(aBegin.Time().IsResolved(),
        "Attempting to set unresolved begin time on interval");
    mBegin = &aBegin;
  }

  void SetEnd(nsSMILInstanceTime& aEnd)
  {
    NS_ABORT_IF_FALSE(mEnd, "Calling SetEnd() on un-set interval");
    mEnd = &aEnd;
  }

  void FreezeBegin()
  {
    NS_ABORT_IF_FALSE(mBegin, "Calling FreezeBegin() on un-set interval");
    mBegin->MarkNoLongerUpdating();
  }

  void FreezeEnd()
  {
    NS_ABORT_IF_FALSE(mEnd, "Calling FreezeEnd() on un-set interval");
    NS_ABORT_IF_FALSE(!mBegin->MayUpdate(),
        "Freezing the end of an interval without a fixed begin");
    mEnd->MarkNoLongerUpdating();
  }

private:
  nsRefPtr<nsSMILInstanceTime> mBegin;
  nsRefPtr<nsSMILInstanceTime> mEnd;
};

#endif 
