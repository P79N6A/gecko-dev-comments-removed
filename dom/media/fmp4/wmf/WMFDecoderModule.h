





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
  CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  bool SupportsMimeType(const nsACString& aMimeType) override;

  virtual void DisableHardwareAcceleration() override
  {
    sDXVAEnabled = false;
  }

  virtual bool SupportsSharedDecoders(const mp4_demuxer::VideoDecoderConfig& aConfig) const override;

  virtual ConversionRequired
  DecoderNeedsConversion(const mp4_demuxer::TrackConfig& aConfig) const override;

  
  
  
  
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
