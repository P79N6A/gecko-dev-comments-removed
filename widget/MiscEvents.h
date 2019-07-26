




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
                            bool aOnlyEnabledCheck = false) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_CONTENT_COMMAND_EVENT),
    mOnlyEnabledCheck(aOnlyEnabledCheck), mSucceeded(false), mIsEnabled(false)
  {
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
                     nsIAtom* aCommand, nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, NS_USER_DEFINED_EVENT, aWidget,
                   NS_COMMAND_EVENT),
    command(aCommand)
  {
    userType = aEventType;
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

  WidgetPluginEvent(bool aIsTrusted, uint32_t aMessage, nsIWidget* aWidget) :
    WidgetGUIEvent(aIsTrusted, aMessage, aWidget, NS_PLUGIN_EVENT),
    retargetToFocusedDocument(false)
  {
  }

  
  
  bool retargetToFocusedDocument;
};

} 

#endif 
