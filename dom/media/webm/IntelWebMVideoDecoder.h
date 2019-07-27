




#if !defined(IntelWebMVideoDecoder_h_)
#define IntelWebMVideoDecoder_h_

#include <stdint.h>

#include "WebMReader.h"
#include "nsAutoPtr.h"
#include "PlatformDecoderModule.h"
#include "mozilla/Monitor.h"

#include "mp4_demuxer/mp4_demuxer.h"
#include "mp4_demuxer/DecoderData.h"

class MediaTaskQueue;

namespace mozilla {

class VP8Sample;

typedef std::deque<VP8Sample*> VP8SampleQueue;

class IntelWebMVideoDecoder : public WebMVideoDecoder, public MediaDataDecoderCallback
{
public:
  static WebMVideoDecoder* Create(WebMReader* aReader);
  virtual nsresult Init(unsigned int aWidth, unsigned int aHeight) MOZ_OVERRIDE;
  virtual nsresult Flush() MOZ_OVERRIDE;
  virtual void Shutdown() MOZ_OVERRIDE;

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual void Output(MediaData* aSample) MOZ_OVERRIDE;

  virtual void DrainComplete() MOZ_OVERRIDE;

  virtual void InputExhausted() MOZ_OVERRIDE;
  virtual void Error() MOZ_OVERRIDE;

  IntelWebMVideoDecoder(WebMReader* aReader);
  ~IntelWebMVideoDecoder();

private:
  void InitLayersBackendType();

  bool Decode();

  bool Demux(nsAutoPtr<VP8Sample>& aSample, bool* aEOS);

  bool SkipVideoDemuxToNextKeyFrame(int64_t aTimeThreshold, uint32_t& parsed);

  bool IsSupportedVideoMimeType(const char* aMimeType);

  VP8Sample* PopSample();

  nsRefPtr<WebMReader> mReader;
  nsRefPtr<PlatformDecoderModule> mPlatform;
  nsRefPtr<MediaDataDecoder> mMediaDataDecoder;

  
  
  nsRefPtr<FlushableMediaTaskQueue> mTaskQueue;

  
  
  Monitor mMonitor;
  nsAutoPtr<mp4_demuxer::VideoDecoderConfig> mDecoderConfig;

  VP8SampleQueue mSampleQueue;
  nsAutoPtr<VP8Sample> mQueuedVideoSample;
  uint64_t mNumSamplesInput;
  uint64_t mNumSamplesOutput;
  uint64_t mLastReportedNumDecodedFrames;
  uint32_t mDecodeAhead;

  
  bool mInputExhausted;
  bool mDrainComplete;
  bool mError;
  bool mEOS;
  bool mIsFlushing;
};

} 

#endif
