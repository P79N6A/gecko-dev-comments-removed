





#ifndef mozilla_dom_StorageEvent_h
#define mozilla_dom_StorageEvent_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/StorageEventBinding.h"


nsresult NS_NewDOMStorageEvent(nsIDOMEvent** aDOMEvent,
                               mozilla::dom::EventTarget* aOwner);

namespace mozilla {
namespace dom {

class DOMStorage;

class StorageEvent : public Event
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(StorageEvent, Event)

  explicit StorageEvent(EventTarget* aOwner);

protected:
  virtual ~StorageEvent();

  nsString mKey;
  nsString mOldValue;
  nsString mNewValue;
  nsString mUrl;
  nsRefPtr<DOMStorage> mStorageArea;

public:
  virtual StorageEvent* AsStorageEvent();

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  static already_AddRefed<StorageEvent>
  Constructor(EventTarget* aOwner, const nsAString& aType,
              const StorageEventInit& aEventInitDict);

  static already_AddRefed<StorageEvent>
  Constructor(const GlobalObject& aGlobal, const nsAString& aType,
              const StorageEventInit& aEventInitDict, ErrorResult& aRv);

  void InitStorageEvent(const nsAString& aType, bool aCanBubble,
                        bool aCancelable, const nsAString& aKey,
                        const nsAString& aOldValue,
                        const nsAString& aNewValue,
                        const nsAString& aURL,
                        DOMStorage* aStorageArea,
                        ErrorResult& aRv);

  void GetKey(nsString& aRetVal) const
  {
    aRetVal = mKey;
  }

  void GetOldValue(nsString& aRetVal) const
  {
    aRetVal = mOldValue;
  }

  void GetNewValue(nsString& aRetVal) const
  {
    aRetVal = mNewValue;
  }

  void GetUrl(nsString& aRetVal) const
  {
    aRetVal = mUrl;
  }

  DOMStorage* GetStorageArea() const
  {
    return mStorageArea;
  }
};

} 
} 

#endif 
