





#include "nsBuiltinDecoderStateMachine.h"
#include "nsWebMReader.h"
#include "nsWebMDecoder.h"

nsBuiltinDecoderStateMachine* nsWebMDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsWebMReader(this));
}
