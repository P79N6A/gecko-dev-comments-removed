





#ifndef mozilla_AppleDecoderModule_h
#define mozilla_AppleDecoderModule_h

#include "PlatformDecoderModule.h"

namespace mozilla {

class AppleDecoderModule : public PlatformDecoderModule {
public:
  AppleDecoderModule();
  virtual ~AppleDecoderModule();

  virtual nsresult Startup() override;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const VideoInfo& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  virtual bool SupportsMimeType(const nsACString& aMimeType) override;

  virtual ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override;

  static void Init();

private:
  static bool sInitialized;
  static bool sIsVTAvailable;
  static bool sIsVTHWAvailable;
  static bool sIsVDAAvailable;
  static bool sForceVDA;
};

} 

#endif 
