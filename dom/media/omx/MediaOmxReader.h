




#if !defined(MediaOmxReader_h_)
#define MediaOmxReader_h_

#include "MediaOmxCommonReader.h"
#include "MediaResource.h"
#include "MediaDecoderReader.h"
#include "nsMimeTypes.h"
#include "MP3FrameParser.h"
#include "nsRect.h"

#include <ui/GraphicBuffer.h>
#include <stagefright/MediaSource.h>

namespace android {
class OmxDecoder;
class MOZ_EXPORT MediaExtractor;
}

namespace mozilla {

class AbstractMediaDecoder;

class MediaOmxReader : public MediaOmxCommonReader
{
  typedef MediaOmxCommonReader::MediaResourcePromise MediaResourcePromise;

  
  
  Mutex mShutdownMutex;
  nsCString mType;
  bool mHasVideo;
  bool mHasAudio;
  nsIntRect mPicture;
  nsIntSize mInitialFrame;
  int64_t mVideoSeekTimeUs;
  int64_t mAudioSeekTimeUs;
  int64_t mLastParserDuration;
  int32_t mSkipCount;
  
  
  bool mIsShutdown;
  MozPromiseHolder<MediaDecoderReader::MetadataPromise> mMetadataPromise;
  MozPromiseRequestHolder<MediaResourcePromise> mMediaResourceRequest;

  MozPromiseHolder<MediaDecoderReader::SeekPromise> mSeekPromise;
  MozPromiseRequestHolder<MediaDecoderReader::VideoDataPromise> mSeekRequest;
protected:
  android::sp<android::OmxDecoder> mOmxDecoder;
  android::sp<android::MediaExtractor> mExtractor;
  MP3FrameParser mMP3FrameParser;

  
  
  
  
  virtual nsresult InitOmxDecoder();

  
  
  virtual void EnsureActive();

  virtual void HandleResourceAllocated();

public:
  MediaOmxReader(AbstractMediaDecoder* aDecoder);
  ~MediaOmxReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);

protected:
  virtual void NotifyDataArrivedInternal(uint32_t aLength, int64_t aOffset) override;
public:

  virtual nsresult ResetDecode()
  {
    mSeekRequest.DisconnectIfExists();
    mSeekPromise.RejectIfExists(NS_OK, __func__);
    return MediaDecoderReader::ResetDecode();
  }

  virtual bool DecodeAudioData();
  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold);

  virtual bool HasAudio()
  {
    return mHasAudio;
  }

  virtual bool HasVideo()
  {
    return mHasVideo;
  }

  virtual void ReleaseMediaResources();

  virtual nsRefPtr<MediaDecoderReader::MetadataPromise> AsyncReadMetadata() override;

  virtual nsRefPtr<SeekPromise>
  Seek(int64_t aTime, int64_t aEndTime) override;

  virtual bool IsMediaSeekable() override;

  virtual void SetIdle() override;

  virtual nsRefPtr<ShutdownPromise> Shutdown() override;

  android::sp<android::MediaSource> GetAudioOffloadTrack();

  
  
  void ReleaseDecoder();

private:
  class ProcessCachedDataTask;
  class NotifyDataArrivedRunnable;

  bool IsShutdown() {
    MutexAutoLock lock(mShutdownMutex);
    return mIsShutdown;
  }

  int64_t ProcessCachedData(int64_t aOffset);

  already_AddRefed<AbstractMediaDecoder> SafeGetDecoder();
};

} 

#endif
