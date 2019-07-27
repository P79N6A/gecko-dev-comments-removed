





#include "MediaCodecDecoder.h"

#include "MediaCodecReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

MediaDecoder*
MediaCodecDecoder::Clone()
{
  return new MediaCodecDecoder();
}

MediaOmxCommonReader*
MediaCodecDecoder::CreateReader()
{
  return new MediaCodecReader(this);
}

MediaDecoderStateMachine*
MediaCodecDecoder::CreateStateMachine(MediaOmxCommonReader* aReader)
{
  return new MediaDecoderStateMachine(this, aReader);
}

} 
