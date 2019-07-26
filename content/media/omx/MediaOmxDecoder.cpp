





#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#include "MediaOmxStateMachine.h"

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
  return new MediaOmxStateMachine(this, new MediaOmxReader(this));
}

MediaOmxDecoder::~MediaOmxDecoder()
{
}

} 

