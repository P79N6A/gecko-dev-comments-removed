





#include "RtspMediaResource.h"
#include "RtspOmxDecoder.h"
#include "RtspOmxReader.h"
#include "MediaDecoderStateMachine.h"

namespace mozilla {

MediaDecoder* RtspOmxDecoder::Clone()
{
  return new RtspOmxDecoder();
}

MediaDecoderStateMachine* RtspOmxDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this,
                                      new RtspOmxReader(this),
                                      mResource->IsRealTime());
}

} 

