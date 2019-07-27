





#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#include "MediaDecoderStateMachine.h"

using namespace android;

namespace mozilla {

MediaDecoder*
MediaOmxDecoder::Clone()
{
  return new MediaOmxDecoder();
}

MediaOmxCommonReader*
MediaOmxDecoder::CreateReader()
{
  return new MediaOmxReader(this);
}

MediaDecoderStateMachine*
MediaOmxDecoder::CreateStateMachine(MediaOmxCommonReader* aReader)
{
  return new MediaDecoderStateMachine(this, aReader);
}

} 
