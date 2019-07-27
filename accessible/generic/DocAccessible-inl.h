





#ifndef mozilla_a11y_DocAccessible_inl_h_
#define mozilla_a11y_DocAccessible_inl_h_

#include "DocAccessible.h"
#include "nsAccessibilityService.h"
#include "nsAccessiblePivot.h"
#include "NotificationController.h"
#include "States.h"
#include "nsIScrollableFrame.h"

#ifdef A11Y_LOG
#include "Logging.h"
#endif

namespace mozilla {
namespace a11y {

inline nsIAccessiblePivot*
DocAccessible::VirtualCursor()
{
  if (!mVirtualCursor) {
    mVirtualCursor = new nsAccessiblePivot(this);
    mVirtualCursor->AddObserver(this);
  }
  return mVirtualCursor;
}

inline void
DocAccessible::FireDelayedEvent(AccEvent* aEvent)
{
#ifdef A11Y_LOG
  if (logging::IsEnabled(logging::eDocLoad))
    logging::DocLoadEventFired(aEvent);
#endif

  mNotificationController->QueueEvent(aEvent);
}

inline void
DocAccessible::FireDelayedEvent(uint32_t aEventType, Accessible* aTarget)
{
  nsRefPtr<AccEvent> event = new AccEvent(aEventType, aTarget);
  FireDelayedEvent(event);
}

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
DocAccessible::AddScrollListener()
{
  
  if (!mPresShell->GetRootFrame())
    return;

  mDocFlags |= eScrollInitialized;
  nsIScrollableFrame* sf = mPresShell->GetRootScrollFrameAsScrollable();
  if (sf) {
    sf->AddScrollPositionListener(this);

#ifdef A11Y_LOG
    if (logging::IsEnabled(logging::eDocCreate))
      logging::Text("add scroll listener");
#endif
  }
}

inline void
DocAccessible::RemoveScrollListener()
{
  nsIScrollableFrame* sf = mPresShell->GetRootScrollFrameAsScrollable();
  if (sf)
    sf->RemoveScrollPositionListener(this);
}

inline void
DocAccessible::NotifyOfLoad(uint32_t aLoadEventType)
{
  mLoadState |= eDOMLoaded;
  mLoadEventType = aLoadEventType;

  
  
  if (HasLoadState(eCompletelyLoaded) && IsLoadEventTarget()) {
    nsRefPtr<AccEvent> stateEvent =
      new AccStateChangeEvent(this, states::BUSY, false);
    FireDelayedEvent(stateEvent);
  }
}

inline void
DocAccessible::MaybeNotifyOfValueChange(Accessible* aAccessible)
{
  a11y::role role = aAccessible->Role();
  if (role == roles::ENTRY || role == roles::COMBOBOX)
    FireDelayedEvent(nsIAccessibleEvent::EVENT_VALUE_CHANGE, aAccessible);
}

inline Accessible*
DocAccessible::GetAccessibleEvenIfNotInMapOrContainer(nsINode* aNode) const
{
  Accessible* acc = GetAccessibleEvenIfNotInMap(aNode);
  return acc ? acc : GetContainerAccessible(aNode);
}

} 
} 

#endif
