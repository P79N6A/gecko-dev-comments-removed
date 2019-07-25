






































#include "nsBuiltinDecoderStateMachine.h"
#include "nsWebMReader.h"
#include "nsWebMDecoder.h"

nsDecoderStateMachine* nsWebMDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsWebMReader(this));
}
