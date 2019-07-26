





#include "nsMediaOmxDecoder.h"
#include "nsMediaOmxReader.h"
#include "nsBuiltinDecoderStateMachine.h"

nsMediaOmxDecoder::nsMediaOmxDecoder() :
  nsBuiltinDecoder()
{
}

nsMediaDecoder* nsMediaOmxDecoder::Clone()
{
  return new nsMediaOmxDecoder();
}

nsDecoderStateMachine* nsMediaOmxDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsMediaOmxReader(this));
}

nsMediaOmxDecoder::~nsMediaOmxDecoder()
{
}
