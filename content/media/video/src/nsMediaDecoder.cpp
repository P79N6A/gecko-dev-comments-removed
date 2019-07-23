




































#include "prlog.h"
#include "prmem.h"
#include "nsIFrame.h"
#include "nsIDocument.h"
#include "nsThreadUtils.h"
#include "nsIDOMHTMLMediaElement.h"
#include "nsNetUtil.h"
#include "nsHTMLMediaElement.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsAutoLock.h"
#include "nsIRenderingContext.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"
#include "nsPresContext.h"
#include "nsMediaDecoder.h"
#include "nsDOMError.h"


#define PROGRESS_MS 350


#define STALL_MS 3000

#ifdef PR_LOGGING

PRLogModuleInfo* gVideoDecoderLog = nsnull;
#endif

nsMediaDecoder::nsMediaDecoder() :
  mElement(0),
  mRGBWidth(-1),
  mRGBHeight(-1),
  mProgressTime(0),
  mDataTime(0),
  mSizeChanged(PR_FALSE),
  mVideoUpdateLock(nsnull),
  mFramerate(0.0),
  mShuttingDown(PR_FALSE),
  mStopping(PR_FALSE)
{
  MOZ_COUNT_CTOR(nsMediaDecoder);
}

nsMediaDecoder::~nsMediaDecoder()
{
  if (mVideoUpdateLock) {
    PR_DestroyLock(mVideoUpdateLock);
    mVideoUpdateLock = nsnull;
  }
  MOZ_COUNT_DTOR(nsMediaDecoder);
}

PRBool nsMediaDecoder::Init(nsHTMLMediaElement* aElement)
{
  mElement = aElement;
  mVideoUpdateLock = PR_NewLock();

  return mVideoUpdateLock != nsnull;
}

void nsMediaDecoder::Shutdown()
{
  StopProgress();
  mElement = nsnull;
}

nsHTMLMediaElement* nsMediaDecoder::GetMediaElement()
{
  return mElement;
}
nsresult nsMediaDecoder::InitLogger() 
{
#ifdef PR_LOGGING
  gVideoDecoderLog = PR_NewLogModule("nsMediaDecoder");
#endif
  return NS_OK;
}

void nsMediaDecoder::Invalidate()
{
  if (!mElement)
    return;

  nsIFrame* frame = mElement->GetPrimaryFrame();
  
  {
    nsAutoLock lock(mVideoUpdateLock);
    if (mSizeChanged) {
      mElement->UpdateMediaSize(nsIntSize(mRGBWidth, mRGBHeight));
      mSizeChanged = PR_FALSE;
      if (frame) {
        nsPresContext* presContext = frame->PresContext();      
        nsIPresShell *presShell = presContext->PresShell();
        presShell->FrameNeedsReflow(frame, 
                                    nsIPresShell::eStyleChange,
                                    NS_FRAME_IS_DIRTY);
      }
    }
  }

  if (frame) {
    nsRect r(nsPoint(0,0), frame->GetSize());
    frame->Invalidate(r);
  }
}

static void ProgressCallback(nsITimer* aTimer, void* aClosure)
{
  nsMediaDecoder* decoder = static_cast<nsMediaDecoder*>(aClosure);
  decoder->Progress(PR_TRUE);
}

void nsMediaDecoder::Progress(PRBool aTimer)
{
  if (!mElement)
    return;

  PRIntervalTime now = PR_IntervalNow();
  if (mProgressTime == 0 ||
      PR_IntervalToMilliseconds(PR_IntervalNow() - mProgressTime) >= PROGRESS_MS) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
    mProgressTime = now;
  }

  
  
  if (aTimer &&
      mDataTime != 0 &&
      PR_IntervalToMilliseconds(now - mDataTime) >= STALL_MS) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("stalled"));
    mDataTime = 0;
  }

  if (!aTimer) {
    mDataTime = now;
  }
}

nsresult nsMediaDecoder::StartProgress()
{
  if (mProgressTimer)
    return NS_OK;

  mProgressTimer = do_CreateInstance("@mozilla.org/timer;1");
  return mProgressTimer->InitWithFuncCallback(ProgressCallback,
                                              this, 
                                              PROGRESS_MS,
                                              nsITimer::TYPE_REPEATING_SLACK);
}

nsresult nsMediaDecoder::StopProgress()
{
  if (!mProgressTimer)
    return NS_OK;

  nsresult rv = mProgressTimer->Cancel();
  mProgressTimer = nsnull;

  return rv;
}

void nsMediaDecoder::SetRGBData(PRInt32 aWidth, PRInt32 aHeight, float aFramerate, unsigned char* aRGBBuffer)
{
  if (mRGBWidth != aWidth || mRGBHeight != aHeight) {
    mRGBWidth = aWidth;
    mRGBHeight = aHeight;
    mSizeChanged = PR_TRUE;
    
    mRGB = nsnull;
  }
  mFramerate = aFramerate;

  if (!mRGB) 
    mRGB = new unsigned char[aWidth * aHeight * 4];
  if (mRGB && aRGBBuffer) {
    memcpy(mRGB.get(), aRGBBuffer, aWidth*aHeight*4);
  }
}

void nsMediaDecoder::Paint(gfxContext* aContext, const gfxRect& aRect)
{
  nsAutoLock lock(mVideoUpdateLock);

  if (!mRGB)
    return;

  
  nsRefPtr<gfxASurface> surface = 
    new gfxImageSurface(mRGB,
                        gfxIntSize(mRGBWidth, mRGBHeight), 
                        mRGBWidth * 4,
                        gfxASurface::ImageFormatARGB32);    

  if (!surface)
    return;

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  if (!pat)
    return;

  
  pat->SetMatrix(gfxMatrix().Scale(mRGBWidth/aRect.Width(), mRGBHeight/aRect.Height()));
  
  
  pat->SetExtend(gfxPattern::EXTEND_PAD);

  
  aContext->NewPath();
  aContext->PixelSnappedRectangleAndSetPattern(aRect, pat);
  aContext->Fill();

#ifdef DEBUG_FRAME_RATE
  {
    
    static float last = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
    float now = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
    static int count = 0;
    count++;
    if (now-last > 10.0) {
      LOG(PR_LOG_DEBUG, ("Paint Frame Rate = %f (should be %f)\n", (float)count / (float)(now-last), mFramerate));
      count = 0;
      last = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
    }
  }   
#endif
}

