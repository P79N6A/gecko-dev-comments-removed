





#ifndef mozilla_dom_indexeddb_idbevents_h__
#define mozilla_dom_indexeddb_idbevents_h__

#include "js/RootingAPI.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/Nullable.h"

#define IDBVERSIONCHANGEEVENT_IID \
  {0x3b65d4c3, 0x73ad, 0x492e, {0xb1, 0x2d, 0x15, 0xf9, 0xda, 0xc2, 0x08, 0x4b}}

class nsAString;
class nsDependentString;

namespace mozilla {

class ErrorResult;

namespace dom {

class EventTarget;
class GlobalObject;
struct IDBVersionChangeEventInit;

namespace indexedDB {

enum Bubbles {
  eDoesNotBubble,
  eDoesBubble
};

enum Cancelable {
  eNotCancelable,
  eCancelable
};

extern const char16_t* kAbortEventType;
extern const char16_t* kBlockedEventType;
extern const char16_t* kCompleteEventType;
extern const char16_t* kErrorEventType;
extern const char16_t* kSuccessEventType;
extern const char16_t* kUpgradeNeededEventType;
extern const char16_t* kVersionChangeEventType;

already_AddRefed<nsIDOMEvent>
CreateGenericEvent(EventTarget* aOwner,
                   const nsDependentString& aType,
                   Bubbles aBubbles,
                   Cancelable aCancelable);

class IDBVersionChangeEvent final : public Event
{
  uint64_t mOldVersion;
  Nullable<uint64_t> mNewVersion;

public:
  static already_AddRefed<IDBVersionChangeEvent>
  Create(EventTarget* aOwner,
         const nsDependentString& aName,
         uint64_t aOldVersion,
         uint64_t aNewVersion)
  {
    Nullable<uint64_t> newVersion(aNewVersion);
    return CreateInternal(aOwner, aName, aOldVersion, newVersion);
  }

  static already_AddRefed<IDBVersionChangeEvent>
  Create(EventTarget* aOwner,
         const nsDependentString& aName,
         uint64_t aOldVersion)
  {
    Nullable<uint64_t> newVersion(0);
    newVersion.SetNull();
    return CreateInternal(aOwner, aName, aOldVersion, newVersion);
  }

  static already_AddRefed<IDBVersionChangeEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const IDBVersionChangeEventInit& aOptions,
              ErrorResult& aRv);

  uint64_t
  OldVersion() const
  {
    return mOldVersion;
  }

  Nullable<uint64_t>
  GetNewVersion() const
  {
    return mNewVersion;
  }

  NS_DECLARE_STATIC_IID_ACCESSOR(IDBVERSIONCHANGEEVENT_IID)

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_EVENT

  virtual JSObject*
  WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

private:
  IDBVersionChangeEvent(EventTarget* aOwner, uint64_t aOldVersion)
    : Event(aOwner, nullptr, nullptr)
    , mOldVersion(aOldVersion)
  {
  }

  ~IDBVersionChangeEvent()
  { }

  static already_AddRefed<IDBVersionChangeEvent>
  CreateInternal(EventTarget* aOwner,
                 const nsAString& aName,
                 uint64_t aOldVersion,
                 Nullable<uint64_t> aNewVersion);
};

NS_DEFINE_STATIC_IID_ACCESSOR(IDBVersionChangeEvent, IDBVERSIONCHANGEEVENT_IID)

} 
} 
} 

#endif 
