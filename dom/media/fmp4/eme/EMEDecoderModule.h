





#if !defined(EMEDecoderModule_h_)
#define EMEDecoderModule_h_

#include "PlatformDecoderModule.h"
#include "gmp-decryption.h"

namespace mozilla {

class CDMProxy;
class FlushableMediaTaskQueue;

class EMEDecoderModule : public PlatformDecoderModule {
private:
  typedef mp4_demuxer::AudioDecoderConfig AudioDecoderConfig;
  typedef mp4_demuxer::VideoDecoderConfig VideoDecoderConfig;

public:
  EMEDecoderModule(CDMProxy* aProxy,
                   PlatformDecoderModule* aPDM,
                   bool aCDMDecodesAudio,
                   bool aCDMDecodesVideo);

  virtual ~EMEDecoderModule();

  
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

  virtual bool
  DecoderNeedsAVCC(const mp4_demuxer::VideoDecoderConfig& aConfig) MOZ_OVERRIDE;

private:
  nsRefPtr<CDMProxy> mProxy;
  
  nsRefPtr<PlatformDecoderModule> mPDM;
  
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;
  bool mCDMDecodesAudio;
  bool mCDMDecodesVideo;

};

} 

#endif 
