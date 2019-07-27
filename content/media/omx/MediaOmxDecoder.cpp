





#include "MediaOmxDecoder.h"
#include "MediaOmxReader.h"
#include "MediaDecoderStateMachine.h"

using namespace android;

namespace mozilla {

MediaDecoder* MediaOmxDecoder::Clone()
{
  return new MediaOmxDecoder();
}

MediaDecoderStateMachine* MediaOmxDecoder::CreateStateMachine()
{
  mReader = new MediaOmxReader(this);
  mReader->SetAudioChannel(GetAudioChannel());
  return new MediaDecoderStateMachine(this, mReader);
}

} 
