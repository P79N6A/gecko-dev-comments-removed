





#include "nsBuiltinDecoderStateMachine.h"
#include "nsMediaPluginDecoder.h"
#include "nsMediaPluginReader.h"

nsMediaPluginDecoder::nsMediaPluginDecoder(const nsACString& aType) : mType(aType)
{
}

nsBuiltinDecoderStateMachine* nsMediaPluginDecoder::CreateStateMachine()
{
  return new nsBuiltinDecoderStateMachine(this, new nsMediaPluginReader(this));
}
