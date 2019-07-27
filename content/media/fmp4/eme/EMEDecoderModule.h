





#if !defined(EMEDecoderModule_h_)
#define EMEDecoderModule_h_

#include "PlatformDecoderModule.h"
#include "gmp-decryption.h"

namespace mozilla {

class CDMProxy;
class MediaTaskQueue;

class EMEDecoderModule : public PlatformDecoderModule {
private:
  typedef mp4_demuxer::AudioDecoderConfig AudioDecoderConfig;
  typedef mp4_demuxer::VideoDecoderConfig VideoDecoderConfig;

public:
  EMEDecoderModule(CDMProxy* aProxy,
                   PlatformDecoderModule* aPDM,
                   bool aCDMDecodesAudio,
                   bool aCDMDecodesVideo,
                   already_AddRefed<MediaTaskQueue> aDecodeTaskQueue);

  virtual ~EMEDecoderModule();

  
  virtual nsresult Shutdown() MOZ_OVERRIDE;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                    layers::LayersBackend aLayersBackend,
                    layers::ImageContainer* aImageContainer,
                    MediaTaskQueue* aVideoTaskQueue,
                    MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     MediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) MOZ_OVERRIDE;

private:
  nsRefPtr<CDMProxy> mProxy;
  
  nsAutoPtr<PlatformDecoderModule> mPDM;
  
  nsRefPtr<MediaTaskQueue> mTaskQueue;
  bool mCDMDecodesAudio;
  bool mCDMDecodesVideo;

};

} 

#endif 
