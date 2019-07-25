




#if !defined(nsMediaDecoder_h_)
#define nsMediaDecoder_h_

#include "mozilla/ReentrantMonitor.h"
#include "VideoFrameContainer.h"
#include "MediaStreamGraph.h"
#include "nsIObserver.h"

class nsHTMLMediaElement;
class nsIStreamListener;
class nsTimeRanges;
class nsIMemoryReporter;
class nsIPrincipal;
class nsITimer;

namespace mozilla {
class MediaResource;
}




static const uint32_t FRAMEBUFFER_LENGTH_PER_CHANNEL = 1024;



static const uint32_t FRAMEBUFFER_LENGTH_MIN = 512;
static const uint32_t FRAMEBUFFER_LENGTH_MAX = 16384;




class nsMediaDecoder : public nsIObserver
{
public:
  typedef mozilla::MediaResource MediaResource;
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
  typedef mozilla::SourceMediaStream SourceMediaStream;
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::VideoFrameContainer VideoFrameContainer;

  nsMediaDecoder();
  virtual ~nsMediaDecoder();

  
  virtual nsMediaDecoder* Clone() = 0;

  
  
  
  virtual bool Init(nsHTMLMediaElement* aElement);

  
  
  virtual MediaResource* GetResource() = 0;

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;

  
  
  virtual double GetCurrentTime() = 0;

  
  virtual nsresult Seek(double aTime) = 0;

  
  
  
  virtual nsresult PlaybackRateChanged() = 0;

  
  virtual double GetDuration() = 0;

  
  
  
  
  
  
  
  
  virtual void SetInfinite(bool aInfinite) = 0;

  
  virtual bool IsInfinite() = 0;

  
  virtual void Pause() = 0;

  
  virtual void SetVolume(double aVolume) = 0;

  
  
  virtual void SetAudioCaptured(bool aCaptured) = 0;

  
  virtual void AddOutputStream(SourceMediaStream* aStream, bool aFinishWhenEnded) = 0;

  
  
  virtual nsresult Play() = 0;

  
  
  
  
  
  virtual nsresult Load(MediaResource* aResource,
                        nsIStreamListener **aListener,
                        nsMediaDecoder* aCloneDonor) = 0;

  
  virtual void ResourceLoaded() = 0;

  
  virtual void NetworkError() = 0;

  
  
  virtual bool IsSeeking() const = 0;

  
  
  virtual bool IsEnded() const = 0;

  
  
  
  virtual void NotifyAudioAvailableListener() = 0;

  struct Statistics {
    
    double mPlaybackRate;
    
    
    double mDownloadRate;
    
    int64_t mTotalBytes;
    
    
    int64_t mDownloadPosition;
    
    
    int64_t mDecoderPosition;
    
    int64_t mPlaybackPosition;
    
    
    
    bool mDownloadRateReliable;
    
    
    
    bool mPlaybackRateReliable;
  };

  
  
  class FrameStatistics {
  public:
    
    FrameStatistics() :
        mReentrantMonitor("nsMediaDecoder::FrameStats"),
        mParsedFrames(0),
        mDecodedFrames(0),
        mPresentedFrames(0) {}

    
    
    uint32_t GetParsedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mParsedFrames;
    }

    
    
    uint32_t GetDecodedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mDecodedFrames;
    }

    
    
    
    uint32_t GetPresentedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mPresentedFrames;
    }

    
    
    void NotifyDecodedFrames(uint32_t aParsed, uint32_t aDecoded) {
      if (aParsed == 0 && aDecoded == 0)
        return;
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      mParsedFrames += aParsed;
      mDecodedFrames += aDecoded;
    }

    
    
    void NotifyPresentedFrame() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      ++mPresentedFrames;
    }

  private:

    
    ReentrantMonitor mReentrantMonitor;

    
    
    uint32_t mParsedFrames;

    
    
    uint32_t mDecodedFrames;

    
    
    uint32_t mPresentedFrames;
  };

  
  
  
  class AutoNotifyDecoded {
  public:
    AutoNotifyDecoded(nsMediaDecoder* aDecoder, uint32_t& aParsed, uint32_t& aDecoded)
      : mDecoder(aDecoder), mParsed(aParsed), mDecoded(aDecoded) {}
    ~AutoNotifyDecoded() {
      mDecoder->GetFrameStatistics().NotifyDecodedFrames(mParsed, mDecoded);
    }
  private:
    nsMediaDecoder* mDecoder;
    uint32_t& mParsed;
    uint32_t& mDecoded;
  };

  
  
  
  
  virtual Statistics GetStatistics() = 0;
  
  
  FrameStatistics& GetFrameStatistics() { return mFrameStats; }

  
  
  
  virtual void SetDuration(double aDuration) = 0;

  
  virtual void SetSeekable(bool aSeekable) = 0;

  
  virtual bool IsSeekable() = 0;

  
  virtual nsresult GetSeekable(nsTimeRanges* aSeekable) = 0;

  
  
  virtual void SetEndTime(double aTime) = 0;

  
  void Invalidate()
  {
    if (mVideoFrameContainer) {
      mVideoFrameContainer->Invalidate();
    }
  }

  
  
  
  
  virtual void Progress(bool aTimer);

  
  
  virtual void FireTimeUpdate();

  
  
  
  
  virtual void NotifySuspendedStatusChanged() = 0;

  
  
  virtual void NotifyBytesDownloaded() = 0;

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus) = 0;

  
  
  virtual void NotifyPrincipalChanged() = 0;

  
  
  virtual void NotifyDataArrived(const char* aBuffer, uint32_t aLength, int64_t aOffset) = 0;

  
  
  virtual void Shutdown();

  
  
  
  
  virtual void Suspend() = 0;

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering) = 0;

  
  
  nsHTMLMediaElement* GetMediaElement();

  
  
  uint32_t GetFrameBufferLength() { return mFrameBufferLength; }

  
  
  virtual nsresult RequestFrameBufferLength(uint32_t aLength);

  
  
  
  
  
  virtual void MoveLoadsToBackground()=0;

  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered) = 0;

  
  
  bool CanPlayThrough();

  
  
  virtual int64_t VideoQueueMemoryInUse() = 0;
  virtual int64_t AudioQueueMemoryInUse() = 0;

  VideoFrameContainer* GetVideoFrameContainer() { return mVideoFrameContainer; }
  mozilla::layers::ImageContainer* GetImageContainer()
  {
    return mVideoFrameContainer ? mVideoFrameContainer->GetImageContainer() : nullptr;
  }

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  void PinForSeek();

  
  void UnpinForSeek();

  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  nsHTMLMediaElement* mElement;

  
  FrameStatistics mFrameStats;

  nsRefPtr<VideoFrameContainer> mVideoFrameContainer;

  
  
  TimeStamp mProgressTime;

  
  
  
  
  
  TimeStamp mDataTime;

  
  uint32_t mFrameBufferLength;

  
  
  bool mPinnedForSeek;

  
  
  
  
  bool mShuttingDown;
};

namespace mozilla {
class MediaMemoryReporter
{
  MediaMemoryReporter();
  ~MediaMemoryReporter();
  static MediaMemoryReporter* sUniqueInstance;

  static MediaMemoryReporter* UniqueInstance() {
    if (!sUniqueInstance) {
      sUniqueInstance = new MediaMemoryReporter;
    }
    return sUniqueInstance;
  }

  typedef nsTArray<nsMediaDecoder*> DecodersArray;
  static DecodersArray& Decoders() {
    return UniqueInstance()->mDecoders;
  }

  DecodersArray mDecoders;

  nsCOMPtr<nsIMemoryReporter> mMediaDecodedVideoMemory;
  nsCOMPtr<nsIMemoryReporter> mMediaDecodedAudioMemory;

public:
  static void AddMediaDecoder(nsMediaDecoder* aDecoder) {
    Decoders().AppendElement(aDecoder);
  }

  static void RemoveMediaDecoder(nsMediaDecoder* aDecoder) {
    DecodersArray& decoders = Decoders();
    decoders.RemoveElement(aDecoder);
    if (decoders.IsEmpty()) {
      delete sUniqueInstance;
      sUniqueInstance = nullptr;
    }
  }

  static int64_t GetDecodedVideoMemory() {
    DecodersArray& decoders = Decoders();
    int64_t result = 0;
    for (size_t i = 0; i < decoders.Length(); ++i) {
      result += decoders[i]->VideoQueueMemoryInUse();
    }
    return result;
  }

  static int64_t GetDecodedAudioMemory() {
    DecodersArray& decoders = Decoders();
    int64_t result = 0;
    for (size_t i = 0; i < decoders.Length(); ++i) {
      result += decoders[i]->AudioQueueMemoryInUse();
    }
    return result;
  }
};

} 
#endif
