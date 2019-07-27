





#if !defined(PlatformDecoderModule_h_)
#define PlatformDecoderModule_h_

#include "MediaDecoderReader.h"
#include "mozilla/layers/LayersTypes.h"
#include "nsTArray.h"
#include "mozilla/RefPtr.h"
#include <queue>

namespace mp4_demuxer {
class VideoDecoderConfig;
class AudioDecoderConfig;
class MP4Sample;
}

class nsIThreadPool;

namespace mozilla {

namespace layers {
class ImageContainer;
}

class MediaDataDecoder;
class MediaDataDecoderCallback;
class MediaInputQueue;
class MediaTaskQueue;
class CDMProxy;
typedef int64_t Microseconds;



















class PlatformDecoderModule {
public:
  
  
  static void Init();

  
  
  
  
  
  
  static PlatformDecoderModule* Create();

#ifdef MOZ_EME
  
  
  
  
  
  static PlatformDecoderModule* CreateCDMWrapper(CDMProxy* aProxy,
                                                 bool aHasAudio,
                                                 bool aHasVideo,
                                                 MediaTaskQueue* aTaskQueue);
#endif

  
  
  
  
  
  virtual nsresult Shutdown() = 0;

  
  
  
  
  
  
  
  
  
  
  
  virtual already_AddRefed<MediaDataDecoder>
  CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                    layers::LayersBackend aLayersBackend,
                    layers::ImageContainer* aImageContainer,
                    MediaTaskQueue* aVideoTaskQueue,
                    MediaDataDecoderCallback* aCallback) = 0;

  
  
  
  
  
  
  
  
  
  
  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     MediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) = 0;

  
  
  
  virtual bool SupportsAudioMimeType(const char* aMimeType);

  virtual ~PlatformDecoderModule() {}

protected:
  PlatformDecoderModule() {}
  
  static bool sUseBlankDecoder;
  static bool sFFmpegDecoderEnabled;
  static bool sGonkDecoderEnabled;
};



class MediaDataDecoderCallback {
public:
  virtual ~MediaDataDecoderCallback() {}

  
  
  virtual void Output(MediaData* aData) = 0;

  
  
  virtual void Error() = 0;

  
  
  virtual void InputExhausted() = 0;

  virtual void DrainComplete() = 0;

  virtual void NotifyResourcesStatusChanged() {};

  virtual void ReleaseMediaResources() {};
};













class MediaDataDecoder {
protected:
  virtual ~MediaDataDecoder() {};

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(MediaDataDecoder)

  
  
  
  
  
  
  
  virtual nsresult Init() = 0;

  
  
  
  virtual nsresult Input(mp4_demuxer::MP4Sample* aSample) = 0;

  
  
  
  
  
  
  
  virtual nsresult Flush() = 0;


  
  
  
  
  
  
  
  
  
  virtual nsresult Drain() = 0;

  
  
  
  
  
  
  
  virtual nsresult Shutdown() = 0;

  
  virtual bool IsWaitingMediaResources() {
    return false;
  };
  virtual bool IsDormantNeeded() {
    return false;
  };
  virtual void ReleaseMediaResources() {}
  virtual void ReleaseDecoder() {}
};

} 

#endif
