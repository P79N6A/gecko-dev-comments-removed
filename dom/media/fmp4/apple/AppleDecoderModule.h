





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
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override;

  virtual bool SupportsMimeType(const nsACString& aMimeType) override;

  virtual ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override;

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
