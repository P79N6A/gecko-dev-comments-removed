





#include "MediaDecoderStateMachine.h"
#include "OggReader.h"
#include "OggDecoder.h"

namespace mozilla {

MediaDecoderStateMachine* OggDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new OggReader(this));
}

} 
