




































#if !defined(nsMediaDecoder_h_)
#define nsMediaDecoder_h_

#include "nsIObserver.h"
#include "nsIPrincipal.h"
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



class nsMediaDecoder : public nsIObserver
{
 public:
  nsMediaDecoder();
  virtual ~nsMediaDecoder();

  
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

  
  
  virtual nsresult Play() = 0;

  
  virtual void Stop() = 0;

  
  
  
  virtual nsresult Load(nsIURI* aURI) = 0;

  
  
  
  
  
  virtual void Paint(gfxContext* aContext, const gfxRect& aRect);

  
  virtual void ResourceLoaded() = 0;

  
  
  virtual PRBool IsSeeking() const = 0;

  
  
  virtual PRUint64 GetBytesLoaded() = 0;

  
  
  virtual PRInt64 GetTotalBytes() = 0;

  
  virtual void SetTotalBytes(PRInt64 aBytes) = 0;

  
  virtual void ElementAvailable(nsHTMLMediaElement* anElement);

  
  virtual void ElementUnavailable();

  
  virtual void Invalidate();

  
  virtual void Progress();

  
  virtual void UpdateBytesDownloaded(PRUint64 aBytes) = 0;

  
  
  virtual void Shutdown();

protected:

  
  nsresult StartProgress();

  
  nsresult StopProgress();

  
  
  
  void MediaSizeChanged();

  
  
  
  
  
  
  void SetRGBData(PRInt32 aWidth, 
                  PRInt32 aHeight, 
                  float aFramerate, 
                  unsigned char* aRGBBuffer);

protected:
  
  nsCOMPtr<nsITimer> mProgressTimer;

  
  
  
  nsHTMLMediaElement* mElement;

  
  
  
  nsAutoArrayPtr<unsigned char> mRGB;

  PRInt32 mRGBWidth;
  PRInt32 mRGBHeight;

  
  PRPackedBool mSizeChanged;

  
  
  
  
  
  
  
  
  
  PRLock* mVideoUpdateLock;

  
  
  float mFramerate;
};

#endif
