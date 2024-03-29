





#include "mozilla/dom/StorageEvent.h"
#include "mozilla/dom/DOMStorage.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(StorageEvent)

NS_IMPL_ADDREF_INHERITED(StorageEvent, Event)
NS_IMPL_RELEASE_INHERITED(StorageEvent, Event)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(StorageEvent, Event)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mStorageArea)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(StorageEvent, Event)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(StorageEvent, Event)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mStorageArea)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(StorageEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

StorageEvent::StorageEvent(EventTarget* aOwner)
  : Event(aOwner, nullptr, nullptr)
{
}

StorageEvent::~StorageEvent()
{
}

StorageEvent*
StorageEvent::AsStorageEvent()
{
  return this;
}

JSObject*
StorageEvent::WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return StorageEventBinding::Wrap(aCx, this, aGivenProto);
}

already_AddRefed<StorageEvent>
StorageEvent::Constructor(EventTarget* aOwner,
                          const nsAString& aType,
                          const StorageEventInit& aEventInitDict)
{
  nsRefPtr<StorageEvent> e = new StorageEvent(aOwner);

  bool trusted = e->Init(aOwner);
  e->InitEvent(aType, aEventInitDict.mBubbles, aEventInitDict.mCancelable);
  e->mKey = aEventInitDict.mKey;
  e->mOldValue = aEventInitDict.mOldValue;
  e->mNewValue = aEventInitDict.mNewValue;
  e->mUrl = aEventInitDict.mUrl;
  e->mStorageArea = aEventInitDict.mStorageArea;
  e->SetTrusted(trusted);

  return e.forget();
}

already_AddRefed<StorageEvent>
StorageEvent::Constructor(const GlobalObject& aGlobal,
                          const nsAString& aType,
                          const StorageEventInit& aEventInitDict,
                          ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> owner = do_QueryInterface(aGlobal.GetAsSupports());
  return Constructor(owner, aType, aEventInitDict);
}

void
StorageEvent::InitStorageEvent(const nsAString& aType, bool aCanBubble,
                               bool aCancelable, const nsAString& aKey,
                               const nsAString& aOldValue,
                               const nsAString& aNewValue,
                               const nsAString& aURL,
                               DOMStorage* aStorageArea,
                               ErrorResult& aRv)
{
  aRv = InitEvent(aType, aCanBubble, aCancelable);
  if (aRv.Failed()) {
    return;
  }

  mKey = aKey;
  mOldValue = aOldValue;
  mNewValue = aNewValue;
  mUrl = aURL;
  mStorageArea = aStorageArea;
}

} 
} 

nsresult
NS_NewDOMStorageEvent(nsIDOMEvent** aDOMEvent,
                      mozilla::dom::EventTarget* aOwner)
{
  nsRefPtr<mozilla::dom::StorageEvent> e =
    new mozilla::dom::StorageEvent(aOwner);

  e->SetTrusted(e->Init(aOwner));
  e.forget(aDOMEvent);

  return NS_OK;
}

