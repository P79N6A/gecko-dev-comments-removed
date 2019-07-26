





#include "nsMediaOmxDecoder.h"
#include "nsMediaOmxReader.h"
#include "nsBuiltinDecoderStateMachine.h"

namespace mozilla {

nsMediaOmxDecoder::nsMediaOmxDecoder() :
  nsBuiltinDecoder()
{
}

nsBuiltinDecoder* nsMediaOmxDecoder::Clone()
{
  return new nsMediaOmxDecoder();
}

nsBuiltinDecoderStateMachine* nsMediaOmxDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsMediaOmxReader(this));
}

nsMediaOmxDecoder::~nsMediaOmxDecoder()
{
}

} 

