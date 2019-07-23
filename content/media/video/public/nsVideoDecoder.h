




































#if !defined(nsVideoDecoder_h___)
#define nsVideoDecoder_h___

#include "nsIObserver.h"
#include "nsSize.h"
#include "prlog.h"
#include "gfxContext.h"
#include "gfxRect.h"
#include "nsITimer.h"

#ifdef PR_LOGGING
extern PRLogModuleInfo* gVideoDecoderLog;
#define LOG(type, msg) PR_LOG(gVideoDecoderLog, type, msg)
#else
#define LOG(type, msg)
#endif

class nsHTMLMediaElement;



class nsVideoDecoder : public nsIObserver
{
 public:
  nsVideoDecoder();
  virtual ~nsVideoDecoder() { }

  
  static nsresult InitLogger();

  
  
  
  virtual PRBool Init();

  
  virtual void GetCurrentURI(nsIURI** aURI) = 0;

  
  virtual nsIPrincipal* GetCurrentPrincipal() = 0;

  
  
  virtual float GetCurrentTime() = 0;

  
  virtual nsresult Seek(float time) = 0;

  
  
  
  virtual nsresult PlaybackRateChanged() = 0;

  
  virtual float GetDuration() = 0;
  
  
  virtual void Pause() = 0;

  
  
  virtual float GetVolume() = 0;

  
  virtual void SetVolume(float volume) = 0;

  
  
  virtual nsIntSize GetVideoSize(nsIntSize defaultSize) = 0;

  
  virtual double GetVideoFramerate() = 0;

  
  
  virtual nsresult Play() = 0;

  
  virtual void Stop() = 0;

  
  
  
  virtual nsresult Load(nsIURI* aURI) = 0;

  
  
  
  
  
  virtual void Paint(gfxContext* aContext, const gfxRect& aRect);

  
  virtual void ResourceLoaded() = 0;

  
  
  virtual PRUint32 GetBytesLoaded() = 0;

  
  
  virtual PRUint32 GetTotalBytes() = 0;

  
  virtual void ElementAvailable(nsHTMLMediaElement* anElement);

  
  virtual void ElementUnavailable();

  
  virtual void Invalidate();

  
  virtual void Progress();

protected:
  
  virtual void Shutdown();

  
  
  nsresult StartInvalidating(double aFramerate);

  
  void StopInvalidating();

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  
  
  
  
  void SetRGBData(PRInt32 aWidth, 
                  PRInt32 aHeight, 
                  double aFramerate, 
                  unsigned char* aRGBBuffer);

protected:
  
  nsCOMPtr<nsITimer> mInvalidateTimer;

  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  nsHTMLMediaElement* mElement;

  
  
  
  nsAutoArrayPtr<unsigned char> mRGB;

  PRInt32 mRGBWidth;
  PRInt32 mRGBHeight;

  
  PRPackedBool mSizeChanged;

  
  
  
  
  
  
  
  
  
  PRLock* mVideoUpdateLock;

  
  
  double mFramerate;
};

#endif
