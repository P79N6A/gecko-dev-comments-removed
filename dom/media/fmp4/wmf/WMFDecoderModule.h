





#if !defined(WMFPlatformDecoderModule_h_)
#define WMFPlatformDecoderModule_h_

#include "PlatformDecoderModule.h"

namespace mozilla {

class WMFDecoderModule : public PlatformDecoderModule {
public:
  WMFDecoderModule();
  virtual ~WMFDecoderModule();

  
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

  bool SupportsVideoMimeType(const char* aMimeType) MOZ_OVERRIDE;
  bool SupportsAudioMimeType(const char* aMimeType) MOZ_OVERRIDE;

  virtual void DisableHardwareAcceleration() MOZ_OVERRIDE
  {
    sDXVAEnabled = false;
  }

  virtual bool SupportsSharedDecoders(const mp4_demuxer::VideoDecoderConfig& aConfig) const MOZ_OVERRIDE;

  
  
  
  
  static bool HasAAC();
  static bool HasH264();

  
  static void Init();
private:
  bool ShouldUseDXVA(const mp4_demuxer::VideoDecoderConfig& aConfig) const;

  static bool sIsWMFEnabled;
  static bool sDXVAEnabled;
};

} 

#endif
