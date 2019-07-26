





#include "nsBuiltinDecoderStateMachine.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"

nsBuiltinDecoderStateMachine* nsOggDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsOggReader(this));
}
