





#ifndef mozilla_AVCCDecoderModule_h
#define mozilla_AVCCDecoderModule_h

#include "PlatformDecoderModule.h"

namespace mozilla {

class AVCCMediaDataDecoder;










class AVCCDecoderModule : public PlatformDecoderModule {
public:
  explicit AVCCDecoderModule(PlatformDecoderModule* aPDM);
  virtual ~AVCCDecoderModule();

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
  virtual bool SupportsVideoMimeType(const char* aMimeType) MOZ_OVERRIDE;

private:
  nsRefPtr<PlatformDecoderModule> mPDM;
};

} 

#endif 
