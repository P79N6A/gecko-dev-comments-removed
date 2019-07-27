





#include "RtspMediaCodecDecoder.h"

#include "MediaDecoderStateMachine.h"
#include "RtspMediaResource.h"
#include "RtspMediaCodecReader.h"

namespace mozilla {

MediaDecoder*
RtspMediaCodecDecoder::Clone()
{
  return new RtspMediaCodecDecoder();
}

MediaOmxCommonReader*
RtspMediaCodecDecoder::CreateReader()
{
  return new RtspMediaCodecReader(this);
}

MediaDecoderStateMachine*
RtspMediaCodecDecoder::CreateStateMachineFromReader(MediaOmxCommonReader* aReader)
{
  return new MediaDecoderStateMachine(this, aReader,
                                      mResource->IsRealTime());
}

void
RtspMediaCodecDecoder::ChangeState(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());

  MediaDecoder::ChangeState(aState);

  
  
  if (mPlayState == PLAY_STATE_ENDED) {
    nsRefPtr<RtspMediaResource> resource = mResource->GetRtspPointer();
    if (resource) {
      nsIStreamingProtocolController* controller =
        resource->GetMediaStreamController();
      if (controller) {
        controller->PlaybackEnded();
      }
    }
  }
}

} 

