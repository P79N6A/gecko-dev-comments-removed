




#include "nsBuiltinDecoderStateMachine.h"
#include "nsWaveReader.h"
#include "nsWaveDecoder.h"

nsBuiltinDecoderStateMachine* nsWaveDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsWaveReader(this));
}
