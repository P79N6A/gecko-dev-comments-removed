





#include "NotificationEvent.h"

using namespace mozilla::dom;

BEGIN_WORKERS_NAMESPACE

NotificationEvent::NotificationEvent(EventTarget* aOwner)
  : ExtendableEvent(aOwner)
{
}

NS_IMPL_ADDREF_INHERITED(NotificationEvent, ExtendableEvent)
NS_IMPL_RELEASE_INHERITED(NotificationEvent, ExtendableEvent)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(NotificationEvent)
NS_INTERFACE_MAP_END_INHERITING(ExtendableEvent)

NS_IMPL_CYCLE_COLLECTION_INHERITED(NotificationEvent, ExtendableEvent, mNotification)

END_WORKERS_NAMESPACE
