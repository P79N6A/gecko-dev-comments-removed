





#include "OfflineAudioCompletionEvent.h"
#include "mozilla/dom/OfflineAudioCompletionEventBinding.h"
#include "AudioContext.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED_1(OfflineAudioCompletionEvent, Event,
                                     mRenderedBuffer)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(OfflineAudioCompletionEvent)
NS_INTERFACE_MAP_END_INHERITING(Event)

NS_IMPL_ADDREF_INHERITED(OfflineAudioCompletionEvent, Event)
NS_IMPL_RELEASE_INHERITED(OfflineAudioCompletionEvent, Event)

OfflineAudioCompletionEvent::OfflineAudioCompletionEvent(AudioContext* aOwner,
                                                         nsPresContext* aPresContext,
                                                         WidgetEvent* aEvent)
  : Event(aOwner, aPresContext, aEvent)
{
  SetIsDOMBinding();
}

JSObject*
OfflineAudioCompletionEvent::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return OfflineAudioCompletionEventBinding::Wrap(aCx, this);
}

}
}

