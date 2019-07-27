





#include "MediaKeyError.h"
#include "mozilla/dom/MediaKeyErrorBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {

MediaKeyError::MediaKeyError(EventTarget* aOwner, uint32_t aSystemCode)
  : Event(aOwner, nullptr, nullptr)
  , mSystemCode(aSystemCode)
{
  SetIsDOMBinding();
  InitEvent(NS_LITERAL_STRING("error"), false, false);
}

MediaKeyError::~MediaKeyError()
{
}

uint32_t
MediaKeyError::SystemCode() const
{
  return mSystemCode;
}

JSObject*
MediaKeyError::WrapObject(JSContext* aCx)
{
  return MediaKeyErrorBinding::Wrap(aCx, this);
}


} 
} 
