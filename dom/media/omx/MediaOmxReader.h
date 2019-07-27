




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
  bool mUseParserDuration;
  
  
  bool mIsShutdown;
protected:
  android::sp<android::OmxDecoder> mOmxDecoder;
  android::sp<android::MediaExtractor> mExtractor;
  MP3FrameParser mMP3FrameParser;

  
  
  bool mIsWaitingResources;

  
  
  
  
  virtual nsresult InitOmxDecoder();

  
  
  virtual void EnsureActive();

  
  
  
  void UpdateIsWaitingMediaResources();

public:
  MediaOmxReader(AbstractMediaDecoder* aDecoder);
  ~MediaOmxReader();

  virtual nsresult Init(MediaDecoderReader* aCloneDonor);

  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset);

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

  
  virtual bool IsWaitingMediaResources() override;

  virtual bool IsDormantNeeded() { return true;}
  virtual void ReleaseMediaResources();

  virtual void PreReadMetadata() override;
  virtual nsresult ReadMetadata(MediaInfo* aInfo,
                                MetadataTags** aTags);
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

  int64_t ProcessCachedData(int64_t aOffset, bool aWaitForCompletion);

  already_AddRefed<AbstractMediaDecoder> SafeGetDecoder();
};

} 

#endif
