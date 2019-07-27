





#if !defined(WMFPlatformDecoderModule_h_)
#define WMFPlatformDecoderModule_h_

#include "PlatformDecoderModule.h"

namespace mozilla {

class WMFDecoderModule : public PlatformDecoderModule {
public:
  WMFDecoderModule();
  virtual ~WMFDecoderModule();

  
  virtual nsresult Startup() override;

  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const VideoInfo& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  bool SupportsMimeType(const nsACString& aMimeType) override;

  virtual void DisableHardwareAcceleration() override
  {
    sDXVAEnabled = false;
  }

  virtual bool SupportsSharedDecoders(const VideoInfo& aConfig) const override;

  virtual ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override;

  
  
  
  
  static bool HasAAC();
  static bool HasH264();

  
  static void Init();
private:
  bool ShouldUseDXVA(const VideoInfo& aConfig) const;

  static bool sIsWMFEnabled;
  static bool sDXVAEnabled;
};

} 

#endif
