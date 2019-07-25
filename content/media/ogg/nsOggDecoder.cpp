






































#include "nsOggPlayStateMachine.h"
#include "nsOggDecoder.h"

nsDecoderStateMachine* nsOggDecoder::CreateStateMachine()
{
  return new nsOggPlayStateMachine(this);
}
