




































#if !defined(nsMediaDecoder_h_)
#define nsMediaDecoder_h_

#include "mozilla/XPCOM.h"

#include "nsIPrincipal.h"
#include "nsSize.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "nsITimer.h"

class nsHTMLMediaElement;
class nsMediaStream;
class nsIStreamListener;




class nsMediaDecoder : public nsIObserver
{
public:
  typedef mozilla::TimeStamp TimeStamp;
  typedef mozilla::TimeDuration TimeDuration;

  nsMediaDecoder();
  virtual ~nsMediaDecoder();

  
  virtual nsMediaDecoder* Clone() = 0;

  
  
  
  virtual PRBool Init(nsHTMLMediaElement* aElement);

  
  
  virtual nsMediaStream* GetCurrentStream() = 0;

  
  virtual already_AddRefed<nsIPrincipal> GetCurrentPrincipal() = 0;

  
  
  virtual float GetCurrentTime() = 0;

  
  virtual nsresult Seek(float time) = 0;

  
  
  
  virtual nsresult PlaybackRateChanged() = 0;

  
  virtual float GetDuration() = 0;
  
  
  virtual void Pause() = 0;

  
  virtual void SetVolume(float volume) = 0;

  
  
  virtual nsresult Play() = 0;

  
  
  
  
  
  virtual nsresult Load(nsMediaStream* aStream,
                        nsIStreamListener **aListener) = 0;

  
  
  
  
  
  virtual void Paint(gfxContext* aContext,
                     gfxPattern::GraphicsFilter aFilter,
                     const gfxRect& aRect);

  
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

  
  
  
  virtual void SetDuration(PRInt64 aDuration) = 0;
 
  
  virtual void SetSeekable(PRBool aSeekable) = 0;

  
  virtual PRBool GetSeekable() = 0;

  
  virtual void Invalidate();

  
  
  
  
  virtual void Progress(PRBool aTimer);

  
  
  
  
  virtual void NotifySuspendedStatusChanged() = 0;
  
  
  
  virtual void NotifyBytesDownloaded() = 0;

  
  
  
  virtual void NotifyDownloadEnded(nsresult aStatus) = 0;

  
  
  virtual void Shutdown();

  
  
  
  
  virtual void Suspend() = 0;

  
  
  
  
  
  virtual void Resume() = 0;

  
  
  nsHTMLMediaElement* GetMediaElement();

  
  
  
  
  
  virtual void MoveLoadsToBackground()=0;

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  
  
  
  void SetRGBData(PRInt32 aWidth,
                  PRInt32 aHeight,
                  float aFramerate,
                  float aAspectRatio,
                  unsigned char* aRGBBuffer);

protected:
  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  nsHTMLMediaElement* mElement;

  
  
  
  nsAutoArrayPtr<unsigned char> mRGB;

  PRInt32 mRGBWidth;
  PRInt32 mRGBHeight;

  
  
  TimeStamp mProgressTime;

  
  
  
  
  
  TimeStamp mDataTime;

  
  
  
  
  
  
  
  
  
  PRLock* mVideoUpdateLock;

  
  
  float mFramerate;

  
  float mAspectRatio;

  
  PRPackedBool mSizeChanged;

  
  
  
  
  PRPackedBool mShuttingDown;
};

#endif
