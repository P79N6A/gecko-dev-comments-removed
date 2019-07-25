




































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
#include "mozilla/Monitor.h"
#include "mozilla/Mutex.h"

class nsHTMLMediaElement;
class nsMediaStream;
class nsIStreamListener;
class nsTimeRanges;




#define FRAMEBUFFER_LENGTH_PER_CHANNEL 1024



#define FRAMEBUFFER_LENGTH_MIN 512
#define FRAMEBUFFER_LENGTH_MAX 16384


class ShutdownThreadEvent : public nsRunnable 
{
public:
  ShutdownThreadEvent(nsIThread* aThread) : mThread(aThread) {}
  ~ShutdownThreadEvent() {}
  NS_IMETHOD Run() {
    mThread->Shutdown();
    return NS_OK;
  }
private:
  nsCOMPtr<nsIThread> mThread;
};




class nsMediaDecoder : public nsIObserver
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;
  typedef mozilla::layers::ImageContainer ImageContainer;
  typedef mozilla::layers::Image Image;
  typedef mozilla::Monitor Monitor;
  typedef mozilla::Mutex Mutex;

  nsMediaDecoder();
  virtual ~nsMediaDecoder();

  
  virtual nsMediaDecoder* Clone() = 0;

  
  
  
  virtual PRBool Init(nsHTMLMediaElement* aElement);

  
  
  virtual nsMediaStream* GetCurrentStream() = 0;

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;

  
  
  virtual double GetCurrentTime() = 0;

  
  virtual nsresult Seek(double aTime) = 0;

  
  
  
  virtual nsresult PlaybackRateChanged() = 0;

  
  virtual double GetDuration() = 0;

  
  virtual void Pause() = 0;

  
  virtual void SetVolume(double aVolume) = 0;

  
  
  virtual nsresult Play() = 0;

  
  
  
  
  
  virtual nsresult Load(nsMediaStream* aStream,
                        nsIStreamListener **aListener,
                        nsMediaDecoder* aCloneDonor) = 0;

  
  virtual void ResourceLoaded() = 0;

  
  virtual void NetworkError() = 0;

  
  
  virtual PRBool IsSeeking() const = 0;

  
  
  virtual PRBool IsEnded() const = 0;

  struct Statistics {
    
    double mPlaybackRate;
    
    
    double mDownloadRate;
    
    PRInt64 mTotalBytes;
    
    
    PRInt64 mDownloadPosition;
    
    
    PRInt64 mDecoderPosition;
    
    PRInt64 mPlaybackPosition;
    
    
    
    PRPackedBool mDownloadRateReliable;
    
    
    
    PRPackedBool mPlaybackRateReliable;
  };

  
  
  class FrameStatistics {
  public:
    
    FrameStatistics() :
        mMonitor("nsMediaDecoder::FrameStats"),
        mParsedFrames(0),
        mDecodedFrames(0),
        mPresentedFrames(0) {}

    
    
    PRUint32 GetParsedFrames() {
      mozilla::MonitorAutoEnter mon(mMonitor);
      return mParsedFrames;
    }

    
    
    PRUint32 GetDecodedFrames() {
      mozilla::MonitorAutoEnter mon(mMonitor);
      return mDecodedFrames;
    }

    
    
    
    PRUint32 GetPresentedFrames() {
      mozilla::MonitorAutoEnter mon(mMonitor);
      return mPresentedFrames;
    }

    
    
    void NotifyDecodedFrames(PRUint32 aParsed, PRUint32 aDecoded) {
      if (aParsed == 0 && aDecoded == 0)
        return;
      mozilla::MonitorAutoEnter mon(mMonitor);
      mParsedFrames += aParsed;
      mDecodedFrames += aDecoded;
    }

    
    
    void NotifyPresentedFrame() {
      mozilla::MonitorAutoEnter mon(mMonitor);
      ++mPresentedFrames;
    }

  private:

    
    Monitor mMonitor;

    
    
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

  
  virtual void SetSeekable(PRBool aSeekable) = 0;

  
  virtual PRBool GetSeekable() = 0;

  
  virtual void Invalidate();

  
  
  
  
  virtual void Progress(PRBool aTimer);

  
  
  virtual void FireTimeUpdate();

  
  
  
  
  virtual void NotifySuspendedStatusChanged() = 0;

  
  
  virtual void NotifyBytesDownloaded() = 0;

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus) = 0;

  
  
  virtual void NotifyDataArrived(const char* aBuffer, PRUint32 aLength, PRUint32 aOffset) = 0;

  
  
  virtual void Shutdown();

  
  
  
  
  virtual void Suspend() = 0;

  
  
  
  
  
  
  
  virtual void Resume(PRBool aForceBuffering) = 0;

  
  
  nsHTMLMediaElement* GetMediaElement();

  
  
  PRUint32 GetFrameBufferLength() { return mFrameBufferLength; };

  
  
  nsresult RequestFrameBufferLength(PRUint32 aLength);

  
  
  
  
  
  virtual void MoveLoadsToBackground()=0;

  
  
  
  ImageContainer* GetImageContainer() { return mImageContainer; }

  
  
  
  void SetVideoData(const gfxIntSize& aSize,
                    float aPixelAspectRatio,
                    Image* aImage,
                    TimeStamp aTarget);

  
  
  virtual nsresult GetBuffered(nsTimeRanges* aBuffered) = 0;

  
  
  PRBool CanPlayThrough();

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

  
  float mPixelAspectRatio;

  
  PRUint32 mFrameBufferLength;

  
  
  PRPackedBool mPinnedForSeek;

  
  
  
  
  PRPackedBool mSizeChanged;

  
  
  
  
  
  
  PRPackedBool mImageContainerSizeChanged;

  
  
  
  
  PRPackedBool mShuttingDown;
};

#endif
