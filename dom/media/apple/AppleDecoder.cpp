



#include "AppleDecoder.h"
#include "AppleMP3Reader.h"

#include "MediaDecoderStateMachine.h"

namespace mozilla {

AppleDecoder::AppleDecoder()
  : MediaDecoder()
{
}

MediaDecoder *
AppleDecoder::Clone()
{
  return new AppleDecoder();
}

MediaDecoderStateMachine *
AppleDecoder::CreateStateMachine()
{
  
  return new MediaDecoderStateMachine(this, new AppleMP3Reader(this));
}

} 
