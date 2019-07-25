




































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
  void HandleFilteredInterval();

  const nsSMILTimeValue& Time() const { return mTime; }
  const nsSMILTimeValueSpec* GetCreator() const { return mCreator; }

  PRBool IsDynamic() const { return !!(mFlags & kDynamic); }
  PRBool IsFixedTime() const { return !(mFlags & kMayUpdate); }
  PRBool FromDOM() const { return !!(mFlags & kFromDOM); }

  PRBool ShouldPreserve() const;
  void   UnmarkShouldPreserve();

  void AddRefFixedEndpoint();
  void ReleaseFixedEndpoint();

  void DependentUpdate(const nsSMILTimeValue& aNewTime)
  {
    NS_ABORT_IF_FALSE(!IsFixedTime(),
        "Updating an instance time that is not expected to be updated");
    mTime = aNewTime;
  }

  PRBool IsDependent() const { return !!mBaseInterval; }
  PRBool IsDependentOn(const nsSMILInstanceTime& aOther) const;
  const nsSMILInterval* GetBaseInterval() const { return mBaseInterval; }
  const nsSMILInstanceTime* GetBaseTime() const;

  PRBool SameTimeAndBase(const nsSMILInstanceTime& aOther) const
  {
    return mTime == aOther.mTime && GetBaseTime() == aOther.GetBaseTime();
  }

  
  
  PRUint32 Serial() const { return mSerial; }
  void SetSerial(PRUint32 aIndex) { mSerial = aIndex; }

  NS_INLINE_DECL_REFCOUNTING(nsSMILInstanceTime)

protected:
  void SetBaseInterval(nsSMILInterval* aBaseInterval);

  nsSMILTimeValue mTime;

  
  enum {
    
    
    
    
    
    kDynamic = 1,

    
    
    
    
    kMayUpdate = 2,

    
    
    
    
    
    kFromDOM = 4,

    
    
    
    kWasDynamicEndpoint = 8
  };
  PRUint8       mFlags;   
  PRPackedBool  mVisited; 

  
  
  
  
  
  
  
  
  
  
  
  
  
  PRUint16      mFixedEndpointRefCnt;

  PRUint32      mSerial; 
                         
                         

  nsSMILTimeValueSpec* mCreator; 
                                 
                                 
  nsSMILInterval* mBaseInterval; 
                                 
};

#endif
