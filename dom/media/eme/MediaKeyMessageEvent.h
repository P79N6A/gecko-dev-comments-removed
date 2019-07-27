





#ifndef mozilla_dom_MediaKeyMessageEvent_h__
#define mozilla_dom_MediaKeyMessageEvent_h__

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "nsCOMPtr.h"
#include "mozilla/dom/Event.h"
#include "mozilla/dom/TypedArray.h"
#include "js/TypeDecls.h"
#include "mozilla/dom/MediaKeyMessageEventBinding.h"

namespace mozilla {
namespace dom {

struct MediaKeyMessageEventInit;

class MediaKeyMessageEvent MOZ_FINAL : public Event
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(MediaKeyMessageEvent, Event)
protected:
  virtual ~MediaKeyMessageEvent();
  explicit MediaKeyMessageEvent(EventTarget* aOwner);

  MediaKeyMessageType mMessageType;
  JS::Heap<JSObject*> mMessage;

public:
  virtual MediaKeyMessageEvent* AsMediaKeyMessageEvent();

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) MOZ_OVERRIDE;

  static already_AddRefed<MediaKeyMessageEvent>
  Constructor(EventTarget* aOwner,
              MediaKeyMessageType aMessageType,
              const nsTArray<uint8_t>& aMessage);

  static already_AddRefed<MediaKeyMessageEvent>
  Constructor(const GlobalObject& aGlobal,
              const nsAString& aType,
              const MediaKeyMessageEventInit& aEventInitDict,
              ErrorResult& aRv);

  MediaKeyMessageType MessageType() const { return mMessageType; }

  void GetMessage(JSContext* cx,
                  JS::MutableHandle<JSObject*> aMessage,
                  ErrorResult& aRv);

private:
  nsTArray<uint8_t> mRawMessage;
};


} 
} 

#endif 
