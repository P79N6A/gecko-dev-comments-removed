




#include "MediaDecoderStateMachine.h"
#include "WaveReader.h"
#include "WaveDecoder.h"

namespace mozilla {

MediaDecoderStateMachine* WaveDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new WaveReader(this));
}

} 
