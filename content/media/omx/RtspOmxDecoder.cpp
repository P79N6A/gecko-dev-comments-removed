





#include "RtspMediaResource.h"
#include "RtspOmxDecoder.h"
#include "RtspOmxReader.h"
#include "MediaOmxStateMachine.h"

namespace mozilla {

MediaDecoder* RtspOmxDecoder::Clone()
{
  return new RtspOmxDecoder();
}

MediaDecoderStateMachine* RtspOmxDecoder::CreateStateMachine()
{
  return new MediaOmxStateMachine(this, new RtspOmxReader(this),
                                  mResource->IsRealTime());
}

void RtspOmxDecoder::ApplyStateToStateMachine(PlayState aState)
{
  MOZ_ASSERT(NS_IsMainThread());
  GetReentrantMonitor().AssertCurrentThreadIn();

  MediaDecoder::ApplyStateToStateMachine(aState);


  
  
  
  
  
  

  RtspMediaResource* rtspResource = mResource->GetRtspPointer();
  MOZ_ASSERT(rtspResource);

  nsIStreamingProtocolController* controller =
    rtspResource->GetMediaStreamController();
  if (mDecoderStateMachine) {
    switch (aState) {
      case PLAY_STATE_PLAYING:
        if (controller) {
          controller->Play();
        }
        break;
      case PLAY_STATE_PAUSED:
        if (controller) {
          controller->Pause();
        }
        break;
      default:
        
        break;
    }
  }
}

} 

