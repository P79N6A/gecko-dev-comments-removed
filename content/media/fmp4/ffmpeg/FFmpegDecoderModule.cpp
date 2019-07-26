





#include "FFmpegRuntimeLinker.h"
#include "FFmpegAACDecoder.h"
#include "FFmpegH264Decoder.h"

#include "FFmpegDecoderModule.h"

namespace mozilla
{

PRLogModuleInfo* GetFFmpegDecoderLog()
{
  static PRLogModuleInfo* sFFmpegDecoderLog = nullptr;
  if (!sFFmpegDecoderLog) {
    sFFmpegDecoderLog = PR_NewLogModule("FFmpegDecoderModule");
  }
  return sFFmpegDecoderLog;
}

bool FFmpegDecoderModule::sFFmpegLinkDone = false;

FFmpegDecoderModule::FFmpegDecoderModule()
{
  MOZ_COUNT_CTOR(FFmpegDecoderModule);
}

FFmpegDecoderModule::~FFmpegDecoderModule() {
  MOZ_COUNT_DTOR(FFmpegDecoderModule);
}

bool
FFmpegDecoderModule::Link()
{
  if (sFFmpegLinkDone) {
    return true;
  }

  if (!FFmpegRuntimeLinker::Link()) {
    NS_WARNING("Failed to link FFmpeg shared libraries.");
    return false;
  }

  sFFmpegLinkDone = true;

  return true;
}

nsresult
FFmpegDecoderModule::Shutdown()
{
  
  return NS_OK;
}

MediaDataDecoder*
FFmpegDecoderModule::CreateH264Decoder(
  const mp4_demuxer::VideoDecoderConfig& aConfig,
  mozilla::layers::LayersBackend aLayersBackend,
  mozilla::layers::ImageContainer* aImageContainer,
  MediaTaskQueue* aVideoTaskQueue, MediaDataDecoderCallback* aCallback)
{
  FFMPEG_LOG("Creating FFmpeg H264 decoder.");
  return new FFmpegH264Decoder(aVideoTaskQueue, aCallback, aConfig,
                               aImageContainer);
}

MediaDataDecoder*
FFmpegDecoderModule::CreateAACDecoder(
  const mp4_demuxer::AudioDecoderConfig& aConfig,
  MediaTaskQueue* aAudioTaskQueue, MediaDataDecoderCallback* aCallback)
{
  FFMPEG_LOG("Creating FFmpeg AAC decoder.");
  return new FFmpegAACDecoder(aAudioTaskQueue, aCallback, aConfig);
}

} 
