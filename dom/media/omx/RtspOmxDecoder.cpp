





#include "RtspMediaResource.h"
#include "RtspOmxDecoder.h"
#include "RtspOmxReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

MediaDecoder* RtspOmxDecoder::Clone()
{
  return new RtspOmxDecoder();
}

MediaDecoderStateMachine*
RtspOmxDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this,
                                      new RtspOmxReader(this),
                                      mResource->IsRealTime());
}

void
RtspOmxDecoder::ChangeState(PlayState aState)
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

