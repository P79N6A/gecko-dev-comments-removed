





#ifndef OfflineAudioCompletionEvent_h_
#define OfflineAudioCompletionEvent_h_

#include "nsDOMEvent.h"
#include "AudioBuffer.h"

namespace mozilla {
namespace dom {

class AudioContext;

class OfflineAudioCompletionEvent : public nsDOMEvent,
                                    public EnableWebAudioCheck
{
public:
  OfflineAudioCompletionEvent(AudioContext* aOwner,
                              nsPresContext* aPresContext,
                              WidgetEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_NSDOMEVENT
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(OfflineAudioCompletionEvent, nsDOMEvent)

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void InitEvent(AudioBuffer* aRenderedBuffer)
  {
    InitEvent(NS_LITERAL_STRING("complete"), false, false);
    mRenderedBuffer = aRenderedBuffer;
  }

  AudioBuffer* RenderedBuffer() const
  {
    return mRenderedBuffer;
  }

private:
  nsRefPtr<AudioBuffer> mRenderedBuffer;
};

}
}

#endif

