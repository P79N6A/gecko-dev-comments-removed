




































#if !defined(nsMediaDecoder_h_)
#define nsMediaDecoder_h_

#include "nsIObserver.h"
#include "nsIPrincipal.h"
#include "nsSize.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "nsITimer.h"
#include "prinrval.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gVideoDecoderLog;
#define LOG(type, msg) PR_LOG(gVideoDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif

class nsHTMLMediaElement;




class nsMediaDecoder : public nsIObserver
{
 public:
  nsMediaDecoder();
  virtual ~nsMediaDecoder();

  
  static nsresult InitLogger();

  
  
  
  virtual PRBool Init(nsHTMLMediaElement* aElement);

  
  virtual void GetCurrentURI(nsIURI** aURI) = 0;

  
  virtual nsIPrincipal* GetCurrentPrincipal() = 0;

  
  
  virtual float GetCurrentTime() = 0;

  
  virtual nsresult Seek(float time) = 0;

  
  
  
  virtual nsresult PlaybackRateChanged() = 0;

  
  virtual float GetDuration() = 0;
  
  
  virtual void Pause() = 0;

  
  virtual void SetVolume(float volume) = 0;

  
  
  virtual nsresult Play() = 0;

  
  virtual void Stop() = 0;

  
  
  
  
  virtual nsresult Load(nsIURI* aURI,
                        nsIChannel* aChannel,
                        nsIStreamListener **aListener) = 0;

  
  
  
  
  
  virtual void Paint(gfxContext* aContext, const gfxRect& aRect);

  
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

  
  
  
  
  virtual Statistics GetStatistics() = 0;

  
  virtual void SetTotalBytes(PRInt64 aBytes) = 0;

  
  
  
  virtual void SetDuration(PRInt64 aDuration) = 0;
 
  
  virtual void SetSeekable(PRBool aSeekable) = 0;

  
  virtual PRBool GetSeekable() = 0;

  
  virtual void Invalidate();

  
  
  
  
  virtual void Progress(PRBool aTimer);

  
  
  
  
  
  
  
  virtual void NotifyDownloadSeeked(PRInt64 aOffsetBytes) = 0;

  
  
  
  
  
  
  virtual void NotifyBytesDownloaded(PRInt64 aBytes) = 0;

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus) = 0;

  
  
  
  virtual void NotifyBytesConsumed(PRInt64 aBytes) = 0;

  
  
  virtual void Shutdown();

  
  
  
  virtual void Suspend() = 0;

  
  
  
  virtual void Resume() = 0;

  
  
  nsHTMLMediaElement* GetMediaElement();

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  
  
  void SetRGBData(PRInt32 aWidth,
                  PRInt32 aHeight,
                  float aFramerate,
                  unsigned char* aRGBBuffer);

  













  class ChannelStatistics {
  public:
    ChannelStatistics() { Reset(); }
    void Reset() {
      mLastStartTime = mAccumulatedTime = 0;
      mAccumulatedBytes = 0;
      mIsStarted = PR_FALSE;
    }
    void Start(PRIntervalTime aNow) {
      if (mIsStarted)
        return;
      mLastStartTime = aNow;
      mIsStarted = PR_TRUE;
    }
    void Stop(PRIntervalTime aNow) {
      if (!mIsStarted)
        return;
      mAccumulatedTime += aNow - mLastStartTime;
      mIsStarted = PR_FALSE;
    }
    void AddBytes(PRInt64 aBytes) {
      if (!mIsStarted) {
        
        
        return;
      }
      mAccumulatedBytes += aBytes;
    }
    double GetRateAtLastStop(PRPackedBool* aReliable) {
      *aReliable = mAccumulatedTime >= PR_TicksPerSecond();
      return double(mAccumulatedBytes)*PR_TicksPerSecond()/mAccumulatedTime;
    }
    double GetRate(PRIntervalTime aNow, PRPackedBool* aReliable) {
      PRIntervalTime time = mAccumulatedTime;
      if (mIsStarted) {
        time += aNow - mLastStartTime;
      }
      *aReliable = time >= PR_TicksPerSecond();
      NS_ASSERTION(time >= 0, "Time wraparound?");
      if (time <= 0)
        return 0.0;
      return double(mAccumulatedBytes)*PR_TicksPerSecond()/time;
    }
  private:
    PRInt64        mAccumulatedBytes;
    PRIntervalTime mAccumulatedTime;
    PRIntervalTime mLastStartTime;
    PRPackedBool   mIsStarted;
  };

protected:
  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  nsHTMLMediaElement* mElement;

  
  
  
  nsAutoArrayPtr<unsigned char> mRGB;

  PRInt32 mRGBWidth;
  PRInt32 mRGBHeight;

  
  
  PRIntervalTime mProgressTime;

  
  
  
  
  
  PRIntervalTime mDataTime;

  
  PRPackedBool mSizeChanged;

  
  
  
  
  
  
  
  
  
  PRLock* mVideoUpdateLock;

  
  
  float mFramerate;

  
  
  
  
  PRPackedBool mShuttingDown;

  
  
  
  
  PRPackedBool mStopping;
};

#endif
