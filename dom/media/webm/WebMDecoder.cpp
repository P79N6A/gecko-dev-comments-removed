





#include "mozilla/Preferences.h"
#include "MediaDecoderStateMachine.h"
#include "MediaFormatReader.h"
#include "WebMDemuxer.h"
#include "WebMReader.h"
#include "WebMDecoder.h"

namespace mozilla {

MediaDecoderStateMachine* WebMDecoder::CreateStateMachine()
{
  bool useFormatDecoder =
    Preferences::GetBool("media.format-reader.webm", true);
  nsRefPtr<MediaDecoderReader> reader = useFormatDecoder ?
      static_cast<MediaDecoderReader*>(new MediaFormatReader(this, new WebMDemuxer(GetResource()))) :
      new WebMReader(this);
  return new MediaDecoderStateMachine(this, reader);
}

} 

