





#ifndef mozilla_AppleDecoderModule_h
#define mozilla_AppleDecoderModule_h

#include "PlatformDecoderModule.h"

namespace mozilla {

class AppleDecoderModule : public PlatformDecoderModule {
public:
  AppleDecoderModule();
  virtual ~AppleDecoderModule();

  virtual nsresult Startup() MOZ_OVERRIDE;

  
  
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  virtual bool SupportsAudioMimeType(const char* aMimeType) MOZ_OVERRIDE;
  virtual bool
  DecoderNeedsAVCC(const mp4_demuxer::VideoDecoderConfig& aConfig) MOZ_OVERRIDE;

  static void Init();
  static nsresult CanDecode();

private:
  friend class InitTask;
  friend class LinkTask;
  friend class UnlinkTask;

  static bool sInitialized;
  static bool sIsVTAvailable;
  static bool sIsVTHWAvailable;
  static bool sIsVDAAvailable;
  static bool sForceVDA;
};

} 

#endif 
