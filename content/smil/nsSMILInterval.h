




































#ifndef NS_SMILINTERVAL_H_
#define NS_SMILINTERVAL_H_

#include "nsSMILInstanceTime.h"
#include "nsTArray.h"










class nsSMILInterval
{
public:
  nsSMILInterval();
  nsSMILInterval(const nsSMILInterval& aOther);
  ~nsSMILInterval();
  void NotifyChanged(const nsSMILTimeContainer* aContainer);
  void NotifyDeleting();

  const nsSMILInstanceTime* Begin() const
  {
    NS_ABORT_IF_FALSE(mBegin && mEnd,
        "Requesting Begin() on un-initialized instance time");
    return mBegin;
  }
  nsSMILInstanceTime* Begin();

  const nsSMILInstanceTime* End() const
  {
    NS_ABORT_IF_FALSE(mBegin && mEnd,
        "Requesting End() on un-initialized instance time");
    return mEnd;
  }
  nsSMILInstanceTime* End();

  void SetBegin(nsSMILInstanceTime& aBegin);
  void SetEnd(nsSMILInstanceTime& aEnd);
  void Set(nsSMILInstanceTime& aBegin, nsSMILInstanceTime& aEnd)
  {
    SetBegin(aBegin);
    SetEnd(aEnd);
  }

  void FreezeBegin()
  {
    NS_ABORT_IF_FALSE(mBegin && mEnd,
        "Freezing Begin() on un-initialized instance time");
    mBegin->MarkNoLongerUpdating();
  }

  void FreezeEnd()
  {
    NS_ABORT_IF_FALSE(mBegin && mEnd,
        "Freezing End() on un-initialized instance time");
    NS_ABORT_IF_FALSE(!mBegin->MayUpdate(),
        "Freezing the end of an interval without a fixed begin");
    mEnd->MarkNoLongerUpdating();
  }

  
  void Unfreeze()
  {
    
    UnfreezeEnd();
  }

  void UnfreezeEnd()
  {
    
  }

  void AddDependentTime(nsSMILInstanceTime& aTime);
  void RemoveDependentTime(const nsSMILInstanceTime& aTime);

private:
  nsRefPtr<nsSMILInstanceTime> mBegin;
  nsRefPtr<nsSMILInstanceTime> mEnd;

  typedef nsTArray<nsRefPtr<nsSMILInstanceTime> > InstanceTimeList;
  InstanceTimeList mDependentTimes;

  
  
  
  
  
  
  
  
  
  
  
  
  PRPackedBool mBeginObjectChanged;
  PRPackedBool mEndObjectChanged;
};

#endif 
