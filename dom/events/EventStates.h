




#ifndef mozilla_EventStates_h_
#define mozilla_EventStates_h_

#include "mozilla/Attributes.h"
#include "nsDebug.h"

namespace mozilla {








class EventStates
{
public:
  typedef uint64_t InternalType;

  MOZ_CONSTEXPR EventStates()
    : mStates(0)
  {
  }

  
  
  
  
  
  explicit MOZ_CONSTEXPR EventStates(InternalType aStates)
    : mStates(aStates)
  {
  }

  EventStates MOZ_CONSTEXPR operator|(const EventStates& aEventStates) const
  {
    return EventStates(mStates | aEventStates.mStates);
  }

  EventStates& operator|=(const EventStates& aEventStates)
  {
    mStates |= aEventStates.mStates;
    return *this;
  }

  
  
  
  EventStates MOZ_CONSTEXPR operator&(const EventStates& aEventStates) const
  {
    return EventStates(mStates & aEventStates.mStates);
  }

  EventStates& operator&=(const EventStates& aEventStates)
  {
    mStates &= aEventStates.mStates;
    return *this;
  }

  bool operator==(const EventStates& aEventStates) const
  {
    return mStates == aEventStates.mStates;
  }

  bool operator!=(const EventStates& aEventStates) const
  {
    return mStates != aEventStates.mStates;
  }

  EventStates operator~() const
  {
    return EventStates(~mStates);
  }

  EventStates operator^(const EventStates& aEventStates) const
  {
    return EventStates(mStates ^ aEventStates.mStates);
  }

  EventStates& operator^=(const EventStates& aEventStates)
  {
    mStates ^= aEventStates.mStates;
    return *this;
  }

  





  bool IsEmpty() const
  {
    return mStates == 0;
  }

  








  bool HasState(EventStates aEventStates) const
  {
#ifdef DEBUG
    
    
    if ((aEventStates.mStates & (aEventStates.mStates - 1))) {
      NS_ERROR("When calling HasState, "
               "EventStates object has to contain only one state!");
    }
#endif 
    return mStates & aEventStates.mStates;
  }

  







  bool HasAtLeastOneOfStates(EventStates aEventStates) const
  {
    return mStates & aEventStates.mStates;
  }

  







  bool HasAllStates(EventStates aEventStates) const
  {
    return (mStates & aEventStates.mStates) == aEventStates.mStates;
  }

  
  
  InternalType GetInternalValue() const {
    return mStates;
  }

private:
  InternalType mStates;
};

} 









#define NS_DEFINE_EVENT_STATE_MACRO(_val)               \
  (mozilla::EventStates(mozilla::EventStates::InternalType(1) << _val))


#define NS_EVENT_STATE_ACTIVE        NS_DEFINE_EVENT_STATE_MACRO(0)

#define NS_EVENT_STATE_FOCUS         NS_DEFINE_EVENT_STATE_MACRO(1)

#define NS_EVENT_STATE_HOVER         NS_DEFINE_EVENT_STATE_MACRO(2)

#define NS_EVENT_STATE_DRAGOVER      NS_DEFINE_EVENT_STATE_MACRO(3)

#define NS_EVENT_STATE_URLTARGET     NS_DEFINE_EVENT_STATE_MACRO(4)

#define NS_EVENT_STATE_CHECKED       NS_DEFINE_EVENT_STATE_MACRO(5)

#define NS_EVENT_STATE_ENABLED       NS_DEFINE_EVENT_STATE_MACRO(6)

#define NS_EVENT_STATE_DISABLED      NS_DEFINE_EVENT_STATE_MACRO(7)

#define NS_EVENT_STATE_REQUIRED      NS_DEFINE_EVENT_STATE_MACRO(8)

#define NS_EVENT_STATE_OPTIONAL      NS_DEFINE_EVENT_STATE_MACRO(9)

#define NS_EVENT_STATE_VISITED       NS_DEFINE_EVENT_STATE_MACRO(10)

#define NS_EVENT_STATE_UNVISITED     NS_DEFINE_EVENT_STATE_MACRO(11)

#define NS_EVENT_STATE_VALID         NS_DEFINE_EVENT_STATE_MACRO(12)

#define NS_EVENT_STATE_INVALID       NS_DEFINE_EVENT_STATE_MACRO(13)

#define NS_EVENT_STATE_INRANGE       NS_DEFINE_EVENT_STATE_MACRO(14)

#define NS_EVENT_STATE_OUTOFRANGE    NS_DEFINE_EVENT_STATE_MACRO(15)


#define NS_EVENT_STATE_MOZ_READONLY  NS_DEFINE_EVENT_STATE_MACRO(16)

#define NS_EVENT_STATE_MOZ_READWRITE NS_DEFINE_EVENT_STATE_MACRO(17)

#define NS_EVENT_STATE_DEFAULT       NS_DEFINE_EVENT_STATE_MACRO(18)

#define NS_EVENT_STATE_BROKEN        NS_DEFINE_EVENT_STATE_MACRO(19)

#define NS_EVENT_STATE_USERDISABLED  NS_DEFINE_EVENT_STATE_MACRO(20)

#define NS_EVENT_STATE_SUPPRESSED    NS_DEFINE_EVENT_STATE_MACRO(21)


#define NS_EVENT_STATE_LOADING       NS_DEFINE_EVENT_STATE_MACRO(22)

#define NS_EVENT_STATE_TYPE_UNSUPPORTED NS_DEFINE_EVENT_STATE_MACRO(23)
#define NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL NS_DEFINE_EVENT_STATE_MACRO(24)

#define NS_EVENT_STATE_HANDLER_BLOCKED NS_DEFINE_EVENT_STATE_MACRO(25)

#define NS_EVENT_STATE_HANDLER_DISABLED NS_DEFINE_EVENT_STATE_MACRO(26)

#define NS_EVENT_STATE_INDETERMINATE NS_DEFINE_EVENT_STATE_MACRO(27)

#define NS_EVENT_STATE_HANDLER_CRASHED NS_DEFINE_EVENT_STATE_MACRO(28)

#define NS_EVENT_STATE_FOCUSRING     NS_DEFINE_EVENT_STATE_MACRO(29)

#define NS_EVENT_STATE_MOZ_SUBMITINVALID NS_DEFINE_EVENT_STATE_MACRO(30)

#define NS_EVENT_STATE_MOZ_UI_INVALID NS_DEFINE_EVENT_STATE_MACRO(31)

#define NS_EVENT_STATE_MOZ_UI_VALID NS_DEFINE_EVENT_STATE_MACRO(32)


#define NS_EVENT_STATE_FULL_SCREEN   NS_DEFINE_EVENT_STATE_MACRO(33)

#define NS_EVENT_STATE_FULL_SCREEN_ANCESTOR   NS_DEFINE_EVENT_STATE_MACRO(34)

#define NS_EVENT_STATE_TYPE_CLICK_TO_PLAY NS_DEFINE_EVENT_STATE_MACRO(35)

#define NS_EVENT_STATE_OPTIMUM NS_DEFINE_EVENT_STATE_MACRO(36)

#define NS_EVENT_STATE_SUB_OPTIMUM NS_DEFINE_EVENT_STATE_MACRO(37)

#define NS_EVENT_STATE_SUB_SUB_OPTIMUM NS_DEFINE_EVENT_STATE_MACRO(38)

#define NS_EVENT_STATE_VULNERABLE_UPDATABLE NS_DEFINE_EVENT_STATE_MACRO(39)

#define NS_EVENT_STATE_VULNERABLE_NO_UPDATE NS_DEFINE_EVENT_STATE_MACRO(40)

#define NS_EVENT_STATE_TYPE_UNSUPPORTED_PLATFORM NS_DEFINE_EVENT_STATE_MACRO(41)

#define NS_EVENT_STATE_LTR NS_DEFINE_EVENT_STATE_MACRO(42)

#define NS_EVENT_STATE_RTL NS_DEFINE_EVENT_STATE_MACRO(43)

#define NS_EVENT_STATE_TYPE_PLAY_PREVIEW NS_DEFINE_EVENT_STATE_MACRO(44)

#define NS_EVENT_STATE_DEVTOOLS_HIGHLIGHTED NS_DEFINE_EVENT_STATE_MACRO(45)


#define NS_EVENT_STATE_IGNORE NS_DEFINE_EVENT_STATE_MACRO(63)





#define DIRECTION_STATES (NS_EVENT_STATE_LTR | NS_EVENT_STATE_RTL)

#define ESM_MANAGED_STATES (NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_FOCUS |     \
                            NS_EVENT_STATE_HOVER | NS_EVENT_STATE_DRAGOVER |   \
                            NS_EVENT_STATE_URLTARGET | NS_EVENT_STATE_FOCUSRING | \
                            NS_EVENT_STATE_FULL_SCREEN | NS_EVENT_STATE_FULL_SCREEN_ANCESTOR)

#define INTRINSIC_STATES (~ESM_MANAGED_STATES)

#endif 

