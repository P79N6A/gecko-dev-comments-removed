





#ifndef mozilla_dom_MediaKeyError_h
#define mozilla_dom_MediaKeyError_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "nsWrapperCache.h"
#include "mozilla/dom/Event.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class MediaKeyError MOZ_FINAL : public Event
{
public:
  NS_FORWARD_TO_EVENT

  MediaKeyError(EventTarget* aOwner, uint32_t aSystemCode);
  ~MediaKeyError();

  virtual JSObject* WrapObjectInternal(JSContext* aCx) MOZ_OVERRIDE;

  uint32_t SystemCode() const;

private:
  uint32_t mSystemCode;
};

} 
} 

#endif
