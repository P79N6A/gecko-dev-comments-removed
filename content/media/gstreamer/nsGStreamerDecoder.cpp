





#include "nsBuiltinDecoderStateMachine.h"
#include "nsGStreamerReader.h"
#include "nsGStreamerDecoder.h"

nsDecoderStateMachine* nsGStreamerDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsGStreamerReader(this));
}
