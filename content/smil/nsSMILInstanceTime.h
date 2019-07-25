




































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
                             bool aBeginObjectChanged,
                             bool aEndObjectChanged);
  void HandleDeletedInterval();
  void HandleFilteredInterval();

  const nsSMILTimeValue& Time() const { return mTime; }
  const nsSMILTimeValueSpec* GetCreator() const { return mCreator; }

  bool IsDynamic() const { return !!(mFlags & kDynamic); }
  bool IsFixedTime() const { return !(mFlags & kMayUpdate); }
  bool FromDOM() const { return !!(mFlags & kFromDOM); }

  bool ShouldPreserve() const;
  void   UnmarkShouldPreserve();

  void AddRefFixedEndpoint();
  void ReleaseFixedEndpoint();

  void DependentUpdate(const nsSMILTimeValue& aNewTime)
  {
    NS_ABORT_IF_FALSE(!IsFixedTime(),
        "Updating an instance time that is not expected to be updated");
    mTime = aNewTime;
  }

  bool IsDependent() const { return !!mBaseInterval; }
  bool IsDependentOn(const nsSMILInstanceTime& aOther) const;
  const nsSMILInterval* GetBaseInterval() const { return mBaseInterval; }
  const nsSMILInstanceTime* GetBaseTime() const;

  bool SameTimeAndBase(const nsSMILInstanceTime& aOther) const
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
  bool          mVisited; 

  
  
  
  
  
  
  
  
  
  
  
  
  
  PRUint16      mFixedEndpointRefCnt;

  PRUint32      mSerial; 
                         
                         

  nsSMILTimeValueSpec* mCreator; 
                                 
                                 
  nsSMILInterval* mBaseInterval; 
                                 
};

#endif
