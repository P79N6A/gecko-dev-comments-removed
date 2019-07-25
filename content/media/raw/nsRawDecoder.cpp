



































#include "nsBuiltinDecoderStateMachine.h"
#include "nsRawReader.h"
#include "nsRawDecoder.h"

nsDecoderStateMachine* nsRawDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsRawReader(this), true);
}
