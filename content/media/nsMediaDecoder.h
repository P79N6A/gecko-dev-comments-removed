




































#if !defined(nsMediaDecoder_h_)
#define nsMediaDecoder_h_

#include "mozilla/XPCOM.h"

#include "nsIPrincipal.h"
#include "nsSize.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "nsITimer.h"
#include "ImageLayers.h"
#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Mutex.h"
#include "nsIMemoryReporter.h"

class nsHTMLMediaElement;
class nsMediaStream;
class nsIStreamListener;
class nsTimeRanges;




static const PRUint32 FRAMEBUFFER_LENGTH_PER_CHANNEL = 1024;



static const PRUint32 FRAMEBUFFER_LENGTH_MIN = 512;
static const PRUint32 FRAMEBUFFER_LENGTH_MAX = 16384;




class nsMediaDecoder : public nsIObserver
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::Image Image;
  typedef mozilla::ReentrantMonitor ReentrantMonitor;
  typedef mozilla::Mutex Mutex;

  nsMediaDecoder();
  virtual ~nsMediaDecoder();

  
  virtual nsMediaDecoder* Clone() = 0;

  
  
  
  virtual bool Init(nsHTMLMediaElement* aElement);

  
  
  virtual nsMediaStream* GetCurrentStream() = 0;

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;

  
  
  virtual double GetCurrentTime() = 0;

  
  virtual nsresult Seek(double aTime) = 0;

  
  
  
  virtual nsresult PlaybackRateChanged() = 0;

  
  virtual double GetDuration() = 0;

  
  
  
  
  
  
  
  
  virtual void SetInfinite(bool aInfinite) = 0;

  
  virtual bool IsInfinite() = 0;

  
  virtual void Pause() = 0;

  
  virtual void SetVolume(double aVolume) = 0;

  
  
  virtual nsresult Play() = 0;

  
  
  
  
  
  virtual nsresult Load(nsMediaStream* aStream,
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
    
    PRInt64 mTotalBytes;
    
    
    PRInt64 mDownloadPosition;
    
    
    PRInt64 mDecoderPosition;
    
    PRInt64 mPlaybackPosition;
    
    
    
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

    
    
    PRUint32 GetParsedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mParsedFrames;
    }

    
    
    PRUint32 GetDecodedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mDecodedFrames;
    }

    
    
    
    PRUint32 GetPresentedFrames() {
      mozilla::ReentrantMonitorAutoEnter mon(mReentrantMonitor);
      return mPresentedFrames;
    }

    
    
    void NotifyDecodedFrames(PRUint32 aParsed, PRUint32 aDecoded) {
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

    
    
    PRUint32 mParsedFrames;

    
    
    PRUint32 mDecodedFrames;

    
    
    PRUint32 mPresentedFrames;
  };

  
  
  
  class AutoNotifyDecoded {
  public:
    AutoNotifyDecoded(nsMediaDecoder* aDecoder, PRUint32& aParsed, PRUint32& aDecoded)
      : mDecoder(aDecoder), mParsed(aParsed), mDecoded(aDecoded) {}
    ~AutoNotifyDecoded() {
      mDecoder->GetFrameStatistics().NotifyDecodedFrames(mParsed, mDecoded);
    }
  private:
    nsMediaDecoder* mDecoder;
    PRUint32& mParsed;
    PRUint32& mDecoded;
  };

  
  
  
  double GetFrameDelay();

  
  
  
  
  virtual Statistics GetStatistics() = 0;
  
  
  FrameStatistics& GetFrameStatistics() { return mFrameStats; }

  
  
  
  virtual void SetDuration(double aDuration) = 0;

  
  virtual void SetSeekable(bool aSeekable) = 0;

  
  virtual bool IsSeekable() = 0;

  
  virtual nsresult GetSeekable(nsTimeRanges* aSeekable) = 0;

  
  
  virtual void SetEndTime(double aTime) = 0;

  
  virtual void Invalidate();

  
  
  
  
  virtual void Progress(bool aTimer);

  
  
  virtual void FireTimeUpdate();

  
  
  
  
  virtual void NotifySuspendedStatusChanged() = 0;

  
  
  virtual void NotifyBytesDownloaded() = 0;

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus) = 0;

  
  
  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) = 0;

  
  
  virtual void Shutdown();

  
  
  
  
  virtual void Suspend() = 0;

  
  
  
  
  
  
  
  virtual void Resume(bool aForceBuffering) = 0;

  
  
  nsHTMLMediaElement* GetMediaElement();

  
  
  PRUint32 GetFrameBufferLength() { return mFrameBufferLength; };

  
  
  nsresult RequestFrameBufferLength(PRUint32 aLength);

  
  
  
  
  
  virtual void MoveLoadsToBackground()=0;

  
  
  
  ImageContainer* GetImageContainer() { return mImageContainer; }

  
  
  
  void SetVideoData(const gfxIntSize& aSize,
                    Image* aImage,
                    TimeStamp aTarget);

  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered) = 0;

  
  
  bool CanPlayThrough();

  
  
  virtual PRInt64 VideoQueueMemoryInUse() = 0;
  virtual PRInt64 AudioQueueMemoryInUse() = 0;

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  void PinForSeek();

  
  void UnpinForSeek();

  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  nsHTMLMediaElement* mElement;

  PRInt32 mRGBWidth;
  PRInt32 mRGBHeight;

  
  FrameStatistics mFrameStats;

  
  
  TimeStamp mPaintTarget;

  
  
  
  TimeDuration mPaintDelay;

  nsRefPtr<ImageContainer> mImageContainer;

  
  
  TimeStamp mProgressTime;

  
  
  
  
  
  TimeStamp mDataTime;

  
  
  
  
  
  
  
  
  
  Mutex mVideoUpdateLock;

  
  PRUint32 mFrameBufferLength;

  
  
  bool mPinnedForSeek;

  
  
  
  
  bool mSizeChanged;

  
  
  
  
  
  
  bool mImageContainerSizeChanged;

  
  
  
  
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
      sUniqueInstance = nsnull;
    }
  }

  static PRInt64 GetDecodedVideoMemory() {
    DecodersArray& decoders = Decoders();
    PRInt64 result = 0;
    for (size_t i = 0; i < decoders.Length(); ++i) {
      result += decoders[i]->VideoQueueMemoryInUse();
    }
    return result;
  }

  static PRInt64 GetDecodedAudioMemory() {
    DecodersArray& decoders = Decoders();
    PRInt64 result = 0;
    for (size_t i = 0; i < decoders.Length(); ++i) {
      result += decoders[i]->AudioQueueMemoryInUse();
    }
    return result;
  }
};

} 
#endif
