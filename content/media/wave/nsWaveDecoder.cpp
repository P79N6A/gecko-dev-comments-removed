




#include "nsBuiltinDecoderStateMachine.h"
#include "nsWaveReader.h"
#include "nsWaveDecoder.h"

namespace mozilla {

nsBuiltinDecoderStateMachine* nsWaveDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsWaveReader(this));
}

} 
