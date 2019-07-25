




#include "nsBuiltinDecoderStateMachine.h"
#include "nsWaveReader.h"
#include "nsWaveDecoder.h"

nsDecoderStateMachine* nsWaveDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsWaveReader(this));
}
