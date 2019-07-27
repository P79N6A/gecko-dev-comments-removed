



#include "MediaDecoderStateMachine.h"
#include "RawReader.h"
#include "RawDecoder.h"

namespace mozilla {

MediaDecoderStateMachine* RawDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new RawReader(this), true);
}

} 

