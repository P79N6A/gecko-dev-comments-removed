




#ifndef NS_SMILINSTANCETIME_H_
#define NS_SMILINSTANCETIME_H_

#include "nsSMILTimeValue.h"
#include "nsAutoPtr.h"

class nsSMILInterval;
class nsSMILTimeContainer;
class nsSMILTimeValueSpec;





















class nsSMILInstanceTime MOZ_FINAL
{
public:
  
  
  
  enum nsSMILInstanceTimeSource {
    
    SOURCE_NONE,
    
    SOURCE_DOM,
    
    SOURCE_SYNCBASE,
    
    SOURCE_EVENT
  };

  explicit nsSMILInstanceTime(const nsSMILTimeValue& aTime,
                              nsSMILInstanceTimeSource aSource = SOURCE_NONE,
                              nsSMILTimeValueSpec* aCreator = nullptr,
                              nsSMILInterval* aBaseInterval = nullptr);

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
    MOZ_ASSERT(!IsFixedTime(),
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

  
  
  uint32_t Serial() const { return mSerial; }
  void SetSerial(uint32_t aIndex) { mSerial = aIndex; }

  NS_INLINE_DECL_REFCOUNTING(nsSMILInstanceTime)

private:
  
  ~nsSMILInstanceTime();

  void SetBaseInterval(nsSMILInterval* aBaseInterval);

  nsSMILTimeValue mTime;

  
  enum {
    
    
    
    
    
    kDynamic = 1,

    
    
    
    
    kMayUpdate = 2,

    
    
    
    
    
    kFromDOM = 4,

    
    
    
    kWasDynamicEndpoint = 8
  };
  uint8_t       mFlags;   
  mutable bool  mVisited; 

  
  
  
  
  
  
  
  
  
  
  
  
  
  uint16_t      mFixedEndpointRefCnt;

  uint32_t      mSerial; 
                         
                         

  nsSMILTimeValueSpec* mCreator; 
                                 
                                 
  nsSMILInterval* mBaseInterval; 
                                 
};

#endif
