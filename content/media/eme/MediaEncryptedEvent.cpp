





#include "MediaEncryptedEvent.h"
#include "mozilla/dom/MediaEncryptedEventBinding.h"
#include "nsContentUtils.h"
#include "jsfriendapi.h"
#include "nsINode.h"
#include "mozilla/dom/MediaKeys.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_CLASS(MediaEncryptedEvent)

NS_IMPL_ADDREF_INHERITED(MediaEncryptedEvent, Event)
NS_IMPL_RELEASE_INHERITED(MediaEncryptedEvent, Event)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(MediaEncryptedEvent, Event)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN_INHERITED(MediaEncryptedEvent, Event)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mInitData)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(MediaEncryptedEvent, Event)
  tmp->mInitData = nullptr;
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(MediaEncryptedEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

MediaEncryptedEvent::MediaEncryptedEvent(EventTarget* aOwner)
  : Event(aOwner, nullptr, nullptr)
{
  mozilla::HoldJSObjects(this);
}

MediaEncryptedEvent::~MediaEncryptedEvent()
{
  mInitData = nullptr;
  mozilla::DropJSObjects(this);
}

JSObject*
MediaEncryptedEvent::WrapObject(JSContext* aCx)
{
  return MediaEncryptedEventBinding::Wrap(aCx, this);
}

already_AddRefed<MediaEncryptedEvent>
MediaEncryptedEvent::Constructor(EventTarget* aOwner,
                                 const nsAString& aInitDataType,
                                 const nsTArray<uint8_t>& aInitData)
{
  nsRefPtr<MediaEncryptedEvent> e = new MediaEncryptedEvent(aOwner);
  e->InitEvent(NS_LITERAL_STRING("encrypted"), false, false);
  e->mInitDataType = aInitDataType;
  e->mRawInitData = aInitData;
  e->SetTrusted(true);
  return e.forget();
}

already_AddRefed<MediaEncryptedEvent>
MediaEncryptedEvent::Constructor(const GlobalObject& aGlobal,
                                 const nsAString& aType,
                                 const MediaKeyNeededEventInit& aEventInitDict,
                                 ErrorResult& aRv)
{
  nsCOMPtr<EventTarget> owner = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<MediaEncryptedEvent> e = new MediaEncryptedEvent(owner);
  bool trusted = e->Init(owner);
  e->InitEvent(aType, aEventInitDict.mBubbles, aEventInitDict.mCancelable);
  e->mInitDataType = aEventInitDict.mInitDataType;
  if (!aEventInitDict.mInitData.IsNull()) {
    const auto& a = aEventInitDict.mInitData.Value();
    a.ComputeLengthAndData();
    e->mInitData = ArrayBuffer::Create(aGlobal.Context(),
                                       a.Length(),
                                       a.Data());
    if (!e->mInitData) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return nullptr;
    }
  }
  e->SetTrusted(trusted);
  return e.forget();
}

void
MediaEncryptedEvent::GetInitDataType(nsString& aRetVal) const
{
  aRetVal = mInitDataType;
}

void
MediaEncryptedEvent::GetInitData(JSContext* cx,
                                 JS::MutableHandle<JSObject*> aData,
                                 ErrorResult& aRv)
{
  if (mRawInitData.Length()) {
    mInitData = ArrayBuffer::Create(cx,
                                    mRawInitData.Length(),
                                    mRawInitData.Elements());
    if (!mInitData) {
      aRv.Throw(NS_ERROR_OUT_OF_MEMORY);
      return;
    }
    mRawInitData.Clear();
  }
  if (mInitData) {
    JS::ExposeObjectToActiveJS(mInitData);
  }
  aData.set(mInitData);
}

} 
} 
