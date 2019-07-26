





#ifndef mozilla_a11y_DocAccessible_inl_h_
#define mozilla_a11y_DocAccessible_inl_h_

#include "DocAccessible.h"
#include "nsAccessibilityService.h"
#include "NotificationController.h"
#include "States.h"

namespace mozilla {
namespace a11y {

inline void
DocAccessible::BindChildDocument(DocAccessible* aDocument)
{
  mNotificationController->ScheduleChildDocBinding(aDocument);
}

template<class Class, class Arg>
inline void
DocAccessible::HandleNotification(Class* aInstance,
                                  typename TNotification<Class, Arg>::Callback aMethod,
                                  Arg* aArg)
{
  if (mNotificationController) {
    mNotificationController->HandleNotification<Class, Arg>(aInstance,
                                                            aMethod, aArg);
  }
}

inline void
DocAccessible::UpdateText(nsIContent* aTextNode)
{
  NS_ASSERTION(mNotificationController, "The document was shut down!");

  
  if (mNotificationController && HasLoadState(eTreeConstructed))
    mNotificationController->ScheduleTextUpdate(aTextNode);
}

inline void
DocAccessible::NotifyOfLoad(uint32_t aLoadEventType)
{
  mLoadState |= eDOMLoaded;
  mLoadEventType = aLoadEventType;

  
  
  if (HasLoadState(eCompletelyLoaded) && IsLoadEventTarget()) {
    nsRefPtr<AccEvent> stateEvent =
      new AccStateChangeEvent(this, states::BUSY, false);
    FireDelayedAccessibleEvent(stateEvent);
  }
}

inline void
DocAccessible::MaybeNotifyOfValueChange(Accessible* aAccessible)
{
  mozilla::a11y::role role = aAccessible->Role();
  if (role == roles::ENTRY || role == roles::COMBOBOX) {
    nsRefPtr<AccEvent> valueChangeEvent =
      new AccEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible,
                   eAutoDetect, AccEvent::eRemoveDupes);
    FireDelayedAccessibleEvent(valueChangeEvent);
  }
}

} 
} 

#endif
