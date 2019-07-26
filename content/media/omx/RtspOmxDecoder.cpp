





#include "RtspMediaResource.h"
#include "RtspOmxDecoder.h"
#include "RtspOmxReader.h"
#include "MediaOmxStateMachine.h"

namespace mozilla {

MediaDecoder* RtspOmxDecoder::Clone()
{
  return new RtspOmxDecoder();
}

MediaDecoderStateMachine* RtspOmxDecoder::CreateStateMachine()
{
  return new MediaOmxStateMachine(this, new RtspOmxReader(this),
                                  mResource->IsRealTime());
}

} 

