





#include "MediaDecoderStateMachine.h"
#include "WebMReader.h"
#include "WebMDecoder.h"

namespace mozilla {

MediaDecoderStateMachine* WebMDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new WebMReader(this));
}

} 

