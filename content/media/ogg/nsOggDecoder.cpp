





#include "nsBuiltinDecoderStateMachine.h"
#include "nsOggReader.h"
#include "nsOggDecoder.h"

nsDecoderStateMachine* nsOggDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsOggReader(this));
}
