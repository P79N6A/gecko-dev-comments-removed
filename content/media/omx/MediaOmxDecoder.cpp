





#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

MediaOmxDecoder::MediaOmxDecoder() :
  MediaDecoder()
{
}

MediaDecoder* MediaOmxDecoder::Clone()
{
  return new MediaOmxDecoder();
}

MediaDecoderStateMachine* MediaOmxDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new MediaOmxReader(this));
}

MediaOmxDecoder::~MediaOmxDecoder()
{
}

} 

