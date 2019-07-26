





#ifndef __FFmpegDecoderModule_h__
#define __FFmpegDecoderModule_h__

#include "PlatformDecoderModule.h"

namespace mozilla
{

#ifdef PR_LOGGING
extern PRLogModuleInfo* GetFFmpegDecoderLog();
#define FFMPEG_LOG(...) PR_LOG(GetFFmpegDecoderLog(), PR_LOG_DEBUG, (__VA_ARGS__))
#else
#define FFMPEG_LOG(...)
#endif

class FFmpegDecoderModule : public PlatformDecoderModule
{
public:
  FFmpegDecoderModule();
  virtual ~FFmpegDecoderModule();

  static bool Link();

  virtual nsresult Shutdown() MOZ_OVERRIDE;

  virtual MediaDataDecoder* CreateH264Decoder(
    const mp4_demuxer::VideoDecoderConfig& aConfig,
    mozilla::layers::LayersBackend aLayersBackend,
    mozilla::layers::ImageContainer* aImageContainer,
    MediaTaskQueue* aVideoTaskQueue,
    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  virtual MediaDataDecoder* CreateAACDecoder(
    const mp4_demuxer::AudioDecoderConfig& aConfig,
    MediaTaskQueue* aAudioTaskQueue,
    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

private:
  static bool sFFmpegLinkDone;
};

} 

#endif 
