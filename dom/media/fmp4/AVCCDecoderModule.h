





#ifndef mozilla_AVCCDecoderModule_h
#define mozilla_AVCCDecoderModule_h

#include "PlatformDecoderModule.h"

namespace mozilla {

class AVCCMediaDataDecoder;










class AVCCDecoderModule : public PlatformDecoderModule {
public:
  explicit AVCCDecoderModule(PlatformDecoderModule* aPDM);
  virtual ~AVCCDecoderModule();

  virtual nsresult Startup() override;
  virtual nsresult Shutdown() override;

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

  virtual bool SupportsAudioMimeType(const char* aMimeType) override;
  virtual bool SupportsVideoMimeType(const char* aMimeType) override;

private:
  nsRefPtr<PlatformDecoderModule> mPDM;
};

} 

#endif 
