




#ifndef mozilla_MiscEvents_h__
#define mozilla_MiscEvents_h__

#include <stdint.h>

#include "mozilla/BasicEvents.h"
#include "nsCOMPtr.h"
#include "nsIAtom.h"
#include "nsITransferable.h"

namespace mozilla {





class WidgetContentCommandEvent : public WidgetGUIEvent
{
public:
  virtual WidgetContentCommandEvent* AsContentCommandEvent() MOZ_OVERRIDE
  {
    return this;
  }

  WidgetContentCommandEvent(bool aIsTrusted, uint32_t aMessage,
                            nsIWidget* aWidget,
                            bool aOnlyEnabledCheck = false)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, eContentCommandEventClass)
    , mOnlyEnabledCheck(aOnlyEnabledCheck)
    , mSucceeded(false)
    , mIsEnabled(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    
    NS_ASSERTION(!IsAllowedToDispatchDOMEvent(),
      "WidgetQueryContentEvent needs to support Duplicate()");
    MOZ_CRASH("WidgetQueryContentEvent doesn't support Duplicate()");
    return nullptr;
  }

  
  nsCOMPtr<nsITransferable> mTransferable; 

  
  
  enum
  {
    eCmdScrollUnit_Line,
    eCmdScrollUnit_Page,
    eCmdScrollUnit_Whole
  };

  struct ScrollInfo
  {
    ScrollInfo() :
      mAmount(0), mUnit(eCmdScrollUnit_Line), mIsHorizontal(false)
    {
    }

    int32_t mAmount;    
    uint8_t mUnit;      
    bool mIsHorizontal; 
  } mScroll;

  bool mOnlyEnabledCheck; 

  bool mSucceeded; 
  bool mIsEnabled; 

  void AssignContentCommandEventData(const WidgetContentCommandEvent& aEvent,
                                     bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    mScroll = aEvent.mScroll;
    mOnlyEnabledCheck = aEvent.mOnlyEnabledCheck;
    mSucceeded = aEvent.mSucceeded;
    mIsEnabled = aEvent.mIsEnabled;
  }
};










class WidgetCommandEvent : public WidgetGUIEvent
{
public:
  virtual WidgetCommandEvent* AsCommandEvent() MOZ_OVERRIDE { return this; }

  WidgetCommandEvent(bool aIsTrusted, nsIAtom* aEventType,
                     nsIAtom* aCommand, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, NS_USER_DEFINED_EVENT, aWidget,
                     eCommandEventClass)
    , command(aCommand)
  {
    userType = aEventType;
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    MOZ_ASSERT(mClass == eCommandEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetCommandEvent* result =
      new WidgetCommandEvent(false, userType, command, nullptr);
    result->AssignCommandEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  nsCOMPtr<nsIAtom> command;

  
  void AssignCommandEventData(const WidgetCommandEvent& aEvent,
                              bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    
  }
};







class WidgetPluginEvent : public WidgetGUIEvent
{
public:
  virtual WidgetPluginEvent* AsPluginEvent() MOZ_OVERRIDE { return this; }

  WidgetPluginEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget)
    : WidgetGUIEvent(aIsTrusted, aMessage, aWidget, ePluginEventClass)
    , retargetToFocusedDocument(false)
  {
  }

  virtual WidgetEvent* Duplicate() const MOZ_OVERRIDE
  {
    
    
    MOZ_ASSERT(mClass == ePluginEventClass,
               "Duplicate() must be overridden by sub class");
    
    WidgetPluginEvent* result = new WidgetPluginEvent(false, message, nullptr);
    result->AssignPluginEventData(*this, true);
    result->mFlags = mFlags;
    return result;
  }

  
  
  bool retargetToFocusedDocument;

  void AssignPluginEventData(const WidgetPluginEvent& aEvent,
                             bool aCopyTargets)
  {
    AssignGUIEventData(aEvent, aCopyTargets);

    retargetToFocusedDocument = aEvent.retargetToFocusedDocument;
  }
};

} 

#endif 
