





#ifndef OfflineAudioCompletionEvent_h_
#define OfflineAudioCompletionEvent_h_

#include "AudioBuffer.h"
#include "mozilla/dom/Event.h"

namespace mozilla {
namespace dom {

class AudioContext;

class OfflineAudioCompletionEvent final : public Event
{
public:
  OfflineAudioCompletionEvent(AudioContext* aOwner,
                              nsPresContext* aPresContext,
                              WidgetEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_EVENT
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(OfflineAudioCompletionEvent, Event)

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void InitEvent(AudioBuffer* aRenderedBuffer)
  {
    InitEvent(NS_LITERAL_STRING("complete"), false, false);
    mRenderedBuffer = aRenderedBuffer;
  }

  AudioBuffer* RenderedBuffer() const
  {
    return mRenderedBuffer;
  }

protected:
  virtual ~OfflineAudioCompletionEvent();

private:
  nsRefPtr<AudioBuffer> mRenderedBuffer;
};

}
}

#endif

