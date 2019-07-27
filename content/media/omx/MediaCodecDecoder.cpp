





#include "MediaCodecDecoder.h"
#include "MediaCodecReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

MediaDecoder*
MediaCodecDecoder::Clone()
{
  return new MediaCodecDecoder();
}

MediaDecoderStateMachine*
MediaCodecDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new MediaCodecReader(this));
}

} 
