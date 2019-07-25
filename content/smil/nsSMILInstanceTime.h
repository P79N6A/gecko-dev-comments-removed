




































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

  PRBool IsDependent(const nsSMILInstanceTime& aOther) const;

  PRBool SameTimeAndBase(const nsSMILInstanceTime& aOther) const
  {
    return mTime == aOther.mTime && GetBaseTime() == aOther.GetBaseTime();
  }

  
  
  PRUint32 Serial() const { return mSerial; }
  void SetSerial(PRUint32 aIndex) { mSerial = aIndex; }

  NS_INLINE_DECL_REFCOUNTING(nsSMILInstanceTime)

protected:
  void SetBaseInterval(nsSMILInterval* aBaseInterval);
  const nsSMILInstanceTime* GetBaseTime() const;

  nsSMILTimeValue mTime;

  
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
