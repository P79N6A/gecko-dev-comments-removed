



#include "nsBuiltinDecoderStateMachine.h"
#include "nsRawReader.h"
#include "nsRawDecoder.h"

nsBuiltinDecoderStateMachine* nsRawDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsRawReader(this), true);
}
