





#ifndef mozilla_dom_MediaKeyNeededEvent_h__
#define mozilla_dom_MediaKeyNeededEvent_h__

#include "mozilla/dom/MediaEncryptedEventBinding.h"
#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/TypedArray.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingUtils.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class MediaEncryptedEvent MOZ_FINAL : public Event
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(MediaEncryptedEvent, Event)
protected:
  virtual ~MediaEncryptedEvent();
  explicit MediaEncryptedEvent(EventTarget* aOwner);

  nsString mInitDataType;
  JS::Heap<JSObject*> mInitData;

public:

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  static already_AddRefed<MediaEncryptedEvent>
  Constructor(EventTarget* aOwner,
              const nsAString& aInitDataType,
              const nsTArray<uint8_t>& aInitData);

  static already_AddRefed<MediaEncryptedEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const MediaKeyNeededEventInit& aEventInitDict,
              ErrorResult& aRv);

  void GetInitDataType(nsString& aRetVal) const;

  void GetInitData(JSContext* cx,
                   JS::MutableHandle<JSObject*> aData,
                   ErrorResult& aRv);
private:
  nsTArray<uint8_t> mRawInitData;
};

} 
} 

#endif 
