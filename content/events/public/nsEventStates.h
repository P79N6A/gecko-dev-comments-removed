




































#ifndef nsEventStates_h__
#define nsEventStates_h__

#include "nsDebug.h"








class nsEventStates
{
public:
  typedef PRUint64 InternalType;

  nsEventStates()
    : mStates(0)
  { }

  
  
  
  
  
  explicit nsEventStates(InternalType aStates)
    : mStates(aStates)
  { }

  nsEventStates(const nsEventStates& aEventStates) {
    mStates = aEventStates.mStates;
  }

  nsEventStates& operator=(const nsEventStates& aEventStates) {
    mStates = aEventStates.mStates;
    return *this;
  }

  nsEventStates operator|(const nsEventStates& aEventStates) const {
    return nsEventStates(mStates | aEventStates.mStates);
  }

  nsEventStates& operator|=(const nsEventStates& aEventStates) {
    mStates |= aEventStates.mStates;
    return *this;
  }

  
  
  
  nsEventStates operator&(const nsEventStates& aEventStates) const {
    return nsEventStates(mStates & aEventStates.mStates);
  }

  nsEventStates& operator&=(const nsEventStates& aEventStates) {
    mStates &= aEventStates.mStates;
    return *this;
  }

  bool operator==(const nsEventStates& aEventStates) const {
    return mStates == aEventStates.mStates;
  }

  bool operator!=(const nsEventStates& aEventStates) const {
    return mStates != aEventStates.mStates;
  }

  nsEventStates operator~() const {
    return nsEventStates(~mStates);
  }

  nsEventStates operator^(const nsEventStates& aEventStates) const {
    return nsEventStates(mStates ^ aEventStates.mStates);
  }

  nsEventStates& operator^=(const nsEventStates& aEventStates) {
    mStates ^= aEventStates.mStates;
    return *this;
  }

  





  bool IsEmpty() const {
    return mStates == 0;
  }

  








  bool HasState(nsEventStates aEventStates) const {
#ifdef DEBUG
    
    
    if ((aEventStates.mStates & (aEventStates.mStates - 1))) {
      NS_ERROR("When calling HasState, "
               "nsEventStates object has to contain only one state!");
    }
#endif 
    return mStates & aEventStates.mStates;
  }

  







  bool HasAtLeastOneOfStates(nsEventStates aEventStates) const {
    return mStates & aEventStates.mStates;
  }

  







  bool HasAllStates(nsEventStates aEventStates) const {
    return (mStates & aEventStates.mStates) == aEventStates.mStates;
  }

  
  
  InternalType GetInternalValue() const {
    return mStates;
  }

private:
  InternalType mStates;
};









#define NS_DEFINE_EVENT_STATE_MACRO(_val)               \
  (nsEventStates(nsEventStates::InternalType(1) << _val))


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
#ifdef MOZ_MATHML
#define NS_EVENT_STATE_INCREMENT_SCRIPT_LEVEL NS_DEFINE_EVENT_STATE_MACRO(24)
#endif

#define NS_EVENT_STATE_HANDLER_BLOCKED NS_DEFINE_EVENT_STATE_MACRO(25)

#define NS_EVENT_STATE_HANDLER_DISABLED NS_DEFINE_EVENT_STATE_MACRO(26)

#define NS_EVENT_STATE_INDETERMINATE NS_DEFINE_EVENT_STATE_MACRO(27)

#define NS_EVENT_STATE_HANDLER_CRASHED NS_DEFINE_EVENT_STATE_MACRO(28)

#define NS_EVENT_STATE_FOCUSRING     NS_DEFINE_EVENT_STATE_MACRO(29)

#define NS_EVENT_STATE_MOZ_PLACEHOLDER NS_DEFINE_EVENT_STATE_MACRO(30)

#define NS_EVENT_STATE_MOZ_SUBMITINVALID NS_DEFINE_EVENT_STATE_MACRO(31)

#define NS_EVENT_STATE_MOZ_UI_INVALID NS_DEFINE_EVENT_STATE_MACRO(32)

#define NS_EVENT_STATE_MOZ_UI_VALID NS_DEFINE_EVENT_STATE_MACRO(33)





#define ESM_MANAGED_STATES (NS_EVENT_STATE_ACTIVE | NS_EVENT_STATE_FOCUS |     \
                            NS_EVENT_STATE_HOVER | NS_EVENT_STATE_DRAGOVER |   \
                            NS_EVENT_STATE_URLTARGET | NS_EVENT_STATE_FOCUSRING)

#define INTRINSIC_STATES (~ESM_MANAGED_STATES)

#endif 

