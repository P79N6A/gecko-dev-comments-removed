





#if !defined(PlatformDecoderModule_h_)
#define PlatformDecoderModule_h_

#include "MediaDecoderReader.h"
#include "mozilla/layers/LayersTypes.h"
#include "nsTArray.h"
#include "mozilla/RefPtr.h"
#include <queue>

namespace mozilla {
class TrackInfo;
class AudioInfo;
class VideoInfo;
class MediaRawData;

namespace layers {
class ImageContainer;
} 

class MediaDataDecoder;
class MediaDataDecoderCallback;
class FlushableTaskQueue;
class CDMProxy;



















class PlatformDecoderModule {
public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PlatformDecoderModule)

  
  
  static void Init();

  
  
  
  
  
  
  static already_AddRefed<PlatformDecoderModule> Create();
  
  static already_AddRefed<PlatformDecoderModule> CreatePDM();

  
  
  virtual nsresult Startup() { return NS_OK; };

#ifdef MOZ_EME
  
  
  
  
  
  static already_AddRefed<PlatformDecoderModule>
  CreateCDMWrapper(CDMProxy* aProxy);
#endif

  
  
  virtual already_AddRefed<MediaDataDecoder>
  CreateDecoder(const TrackInfo& aConfig,
                FlushableTaskQueue* aTaskQueue,
                MediaDataDecoderCallback* aCallback,
                layers::LayersBackend aLayersBackend = layers::LayersBackend::LAYERS_NONE,
                layers::ImageContainer* aImageContainer = nullptr);

  
  
  
  
  virtual bool SupportsMimeType(const nsACString& aMimeType);

  
  static bool AgnosticMimeType(const nsACString& aMimeType);


  enum ConversionRequired {
    kNeedNone,
    kNeedAVCC,
    kNeedAnnexB,
  };

  
  
  
  virtual ConversionRequired DecoderNeedsConversion(const TrackInfo& aConfig) const = 0;

  virtual void DisableHardwareAcceleration() {}

  virtual bool SupportsSharedDecoders(const VideoInfo& aConfig) const {
    return !AgnosticMimeType(aConfig.mMimeType);
  }

protected:
  PlatformDecoderModule() {}
  virtual ~PlatformDecoderModule() {}

  friend class H264Converter;
  
  
  
  
  
  
  
  
  
  
  
  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const VideoInfo& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) = 0;

  
  static bool sUseBlankDecoder;
  static bool sFFmpegDecoderEnabled;
  static bool sGonkDecoderEnabled;
  static bool sAndroidMCDecoderPreferred;
  static bool sAndroidMCDecoderEnabled;
  static bool sGMPDecoderEnabled;
};



class MediaDataDecoderCallback {
public:
  virtual ~MediaDataDecoderCallback() {}

  
  virtual void Output(MediaData* aData) = 0;

  
  
  virtual void Error() = 0;

  
  
  virtual void InputExhausted() = 0;

  virtual void DrainComplete() = 0;

  virtual void ReleaseMediaResources() {};

  virtual bool OnReaderTaskQueue() = 0;
};


















class MediaDataDecoder {
protected:
  virtual ~MediaDataDecoder() {};

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDataDecoder)

  
  
  
  
  
  
  
  virtual nsresult Init() = 0;

  
  virtual nsresult Input(MediaRawData* aSample) = 0;

  
  
  
  
  
  
  
  virtual nsresult Flush() = 0;

  
  
  
  
  
  
  
  
  
  virtual nsresult Drain() = 0;

  
  
  
  
  
  
  
  virtual nsresult Shutdown() = 0;

  
  virtual bool IsHardwareAccelerated() const { return false; }

  
  
  
  
  virtual nsresult ConfigurationChanged(const TrackInfo& aConfig)
  {
    return NS_OK;
  }
};

} 

#endif
