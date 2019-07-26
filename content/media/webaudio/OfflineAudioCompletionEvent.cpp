





#include "OfflineAudioCompletionEvent.h"
#include "mozilla/dom/OfflineAudioCompletionEventBinding.h"
#include "AudioContext.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(OfflineAudioCompletionEvent, nsDOMEvent,
                                     mRenderedBuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(OfflineAudioCompletionEvent)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEvent)

NS_IMPL_ADDREF_INHERITED(OfflineAudioCompletionEvent, nsDOMEvent)
NS_IMPL_RELEASE_INHERITED(OfflineAudioCompletionEvent, nsDOMEvent)

OfflineAudioCompletionEvent::OfflineAudioCompletionEvent(AudioContext* aOwner,
                                                         nsPresContext* aPresContext,
                                                         WidgetEvent* aEvent)
  : nsDOMEvent(aOwner, aPresContext, aEvent)
{
  SetIsDOMBinding();
}

JSObject*
OfflineAudioCompletionEvent::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return OfflineAudioCompletionEventBinding::Wrap(aCx, aScope, this);
}

}
}

