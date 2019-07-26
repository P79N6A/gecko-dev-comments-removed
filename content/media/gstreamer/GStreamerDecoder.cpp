





#include "MediaDecoderStateMachine.h"
#include "GStreamerReader.h"
#include "GStreamerDecoder.h"
#include "GStreamerFormatHelper.h"

namespace mozilla {

MediaDecoderStateMachine* GStreamerDecoder::CreateStateMachine()
{
  return new MediaDecoderStateMachine(this, new GStreamerReader(this));
}

bool
GStreamerDecoder::CanHandleMediaType(const nsACString& aMIMEType,
                                     const nsAString* aCodecs)
{
  return GStreamerFormatHelper::Instance()->CanHandleMediaType(aMIMEType, aCodecs);
}

} 

