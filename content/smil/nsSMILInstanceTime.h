




































#ifndef NS_SMILINSTANCETIME_H_
#define NS_SMILINSTANCETIME_H_

#include "nsSMILTimeValue.h"
#include "nsAutoPtr.h"

class nsSMILInterval;
class nsSMILTimeContainer;
class nsSMILTimeValueSpec;























class nsSMILInstanceTime
{
public:
  
  
  
  enum nsSMILInstanceTimeSource {
    
    SOURCE_NONE,
    
    SOURCE_DOM,
    
    SOURCE_SYNCBASE,
    
    SOURCE_EVENT
  };

  nsSMILInstanceTime(const nsSMILTimeValue& aTime,
                     nsSMILInstanceTimeSource aSource = SOURCE_NONE,
                     nsSMILTimeValueSpec* aCreator = nsnull,
                     nsSMILInterval* aBaseInterval = nsnull);
  ~nsSMILInstanceTime();
  void Unlink();
  void HandleChangedInterval(const nsSMILTimeContainer* aSrcContainer,
                             PRBool aBeginObjectChanged,
                             PRBool aEndObjectChanged);
  void HandleDeletedInterval();

  const nsSMILTimeValue& Time() const { return mTime; }
  const nsSMILTimeValueSpec* GetCreator() const { return mCreator; }

  PRBool ClearOnReset() const { return !!(mFlags & kClearOnReset); }
  PRBool MayUpdate() const { return !!(mFlags & kMayUpdate); }
  PRBool FromDOM() const { return !!(mFlags & kFromDOM); }

  void MarkNoLongerUpdating() { mFlags &= ~kMayUpdate; }

  void DependentUpdate(const nsSMILTimeValue& aNewTime)
  {
    NS_ABORT_IF_FALSE(MayUpdate(),
        "Updating an instance time that is not expected to be updated");
    mTime = aNewTime;
  }

  PRBool IsDependent(const nsSMILInstanceTime& aOther,
                     PRUint32 aRecursionDepth = 0) const;

  PRBool SameTimeAndBase(const nsSMILInstanceTime& aOther) const
  {
    return mTime == aOther.mTime && GetBaseTime() == aOther.GetBaseTime();
  }

  
  
  PRUint32 Serial() const { return mSerial; }
  void SetSerial(PRUint32 aIndex) { mSerial = aIndex; }

  nsrefcnt AddRef()
  {
    if (mRefCnt == PR_UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking nsSMILInstanceTime");
      return mRefCnt;
    }
    NS_ASSERT_OWNINGTHREAD(_class);
    NS_ABORT_IF_FALSE(_mOwningThread.GetThread() == PR_GetCurrentThread(),
        "nsSMILInstanceTime addref isn't thread-safe!");
    ++mRefCnt;
    NS_LOG_ADDREF(this, mRefCnt, "nsSMILInstanceTime", sizeof(*this));
    return mRefCnt;
  }

  nsrefcnt Release()
  {
    if (mRefCnt == PR_UINT32_MAX) {
      NS_WARNING("refcount overflow, leaking nsSMILInstanceTime");
      return mRefCnt;
    }
    NS_ABORT_IF_FALSE(_mOwningThread.GetThread() == PR_GetCurrentThread(),
        "nsSMILInstanceTime release isn't thread-safe!");
    --mRefCnt;
    NS_LOG_RELEASE(this, mRefCnt, "nsSMILInstanceTime");
    if (mRefCnt == 0) {
      delete this;
      return 0;
    }
    return mRefCnt;
  }

protected:
  void SetBaseInterval(nsSMILInterval* aBaseInterval);
  void BreakPotentialCycle(const nsSMILInstanceTime* aNewTail) const;
  const nsSMILInstanceTime* GetBaseTime() const;

  nsSMILTimeValue mTime;

  nsAutoRefCnt mRefCnt;
  NS_DECL_OWNINGTHREAD

  
  enum {
    
    
    kClearOnReset = 1,

    
    
    
    
    
    kMayUpdate = 2,

    
    
    
    
    
    kFromDOM = 4
  };
  PRUint8       mFlags; 
  PRUint32      mSerial; 
                         
                         
  PRPackedBool  mVisited;
  PRPackedBool  mChainEnd;

  nsSMILTimeValueSpec* mCreator; 
                                 
                                 
  nsSMILInterval* mBaseInterval; 
                                 
};

#endif
