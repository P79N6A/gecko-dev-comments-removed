





#include "MediaOmxCommonReader.h"

#include <stagefright/MediaSource.h>

#include "AbstractMediaDecoder.h"
#include "AudioChannelService.h"
#include "MediaStreamSource.h"

#ifdef MOZ_AUDIO_OFFLOAD
#include <stagefright/Utils.h>
#include <cutils/properties.h>
#include <stagefright/MetaData.h>
#endif

using namespace android;

namespace mozilla {

#ifdef PR_LOGGING
extern PRLogModuleInfo* gMediaDecoderLog;
#define DECODER_LOG(type, msg) PR_LOG(gMediaDecoderLog, type, msg)
#else
#define DECODER_LOG(type, msg)
#endif

MediaOmxCommonReader::MediaOmxCommonReader(AbstractMediaDecoder *aDecoder)
  : MediaDecoderReader(aDecoder)
{
#ifdef PR_LOGGING
  if (!gMediaDecoderLog) {
    gMediaDecoderLog = PR_NewLogModule("MediaDecoder");
  }
#endif

  mAudioChannel = dom::AudioChannelService::GetDefaultAudioChannel();
}

#ifdef MOZ_AUDIO_OFFLOAD
void MediaOmxCommonReader::CheckAudioOffload()
{
  NS_ASSERTION(mDecoder->OnDecodeThread(), "Should be on decode thread.");

  char offloadProp[128];
  property_get("audio.offload.disable", offloadProp, "0");
  bool offloadDisable =  atoi(offloadProp) != 0;
  if (offloadDisable) {
    return;
  }

  sp<MediaSource> audioOffloadTrack = GetAudioOffloadTrack();
  sp<MetaData> meta = audioOffloadTrack.get()
      ? audioOffloadTrack->getFormat() : nullptr;

  
  bool hasNoVideo = !HasVideo();
  bool isNotStreaming
      = mDecoder->GetResource()->IsDataCachedToEndOfResource(0);

  
  
  bool isTypeMusic = mAudioChannel == dom::AudioChannel::Content;

  DECODER_LOG(PR_LOG_DEBUG, ("%s meta %p, no video %d, no streaming %d,"
      " channel type %d", __FUNCTION__, meta.get(), hasNoVideo,
      isNotStreaming, mAudioChannel));

  if ((meta.get()) && hasNoVideo && isNotStreaming && isTypeMusic &&
      canOffloadStream(meta, false, false, AUDIO_STREAM_MUSIC)) {
    DECODER_LOG(PR_LOG_DEBUG, ("Can offload this audio stream"));
    mDecoder->SetPlatformCanOffloadAudio(true);
  }
}
#endif

} 
