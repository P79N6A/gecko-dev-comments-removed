





#ifndef AudioProcessingEvent_h_
#define AudioProcessingEvent_h_

#include "AudioBuffer.h"
#include "ScriptProcessorNode.h"
#include "mozilla/dom/Event.h"

namespace mozilla {
namespace dom {

class AudioProcessingEvent final : public Event
{
public:
  AudioProcessingEvent(ScriptProcessorNode* aOwner,
                       nsPresContext* aPresContext,
                       WidgetEvent* aEvent);

  NS_DECL_ISUPPORTS_INHERITED
  NS_FORWARD_TO_EVENT
  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(AudioProcessingEvent, Event)

  virtual JSObject* WrapObjectInternal(JSContext* aCx, JS::Handle<JSObject*> aGivenProto) override;

  void InitEvent(AudioBuffer* aInputBuffer,
                 uint32_t aNumberOfInputChannels,
                 double aPlaybackTime)
  {
    InitEvent(NS_LITERAL_STRING("audioprocess"), false, false);
    mInputBuffer = aInputBuffer;
    mNumberOfInputChannels = aNumberOfInputChannels;
    mPlaybackTime = aPlaybackTime;
  }

  double PlaybackTime() const
  {
    return mPlaybackTime;
  }

  AudioBuffer* GetInputBuffer(ErrorResult& aRv)
  {
    if (!mInputBuffer) {
      mInputBuffer = LazilyCreateBuffer(mNumberOfInputChannels, aRv);
    }
    return mInputBuffer;
  }

  AudioBuffer* GetOutputBuffer(ErrorResult& aRv)
  {
    if (!mOutputBuffer) {
      mOutputBuffer = LazilyCreateBuffer(mNode->NumberOfOutputChannels(), aRv);
    }
    return mOutputBuffer;
  }

  bool HasOutputBuffer() const
  {
    return !!mOutputBuffer;
  }

protected:
  virtual ~AudioProcessingEvent();

private:
  already_AddRefed<AudioBuffer>
  LazilyCreateBuffer(uint32_t aNumberOfChannels, ErrorResult& rv);

private:
  double mPlaybackTime;
  nsRefPtr<AudioBuffer> mInputBuffer;
  nsRefPtr<AudioBuffer> mOutputBuffer;
  nsRefPtr<ScriptProcessorNode> mNode;
  uint32_t mNumberOfInputChannels;
};

}
}

#endif

