




































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
#include "nsVideoDecoder.h"

#ifdef PR_LOGGING

PRLogModuleInfo* gVideoDecoderLog = nsnull;
#endif

nsVideoDecoder::nsVideoDecoder() :
  mElement(0),
  mRGBWidth(-1),
  mRGBHeight(-1),
  mSizeChanged(PR_FALSE),
  mVideoUpdateLock(nsnull),
  mFramerate(0.0)
{
}

PRBool nsVideoDecoder::Init()
{
  mVideoUpdateLock = PR_NewLock();

  return mVideoUpdateLock != nsnull;
}

void nsVideoDecoder::Shutdown()
{
  if (mVideoUpdateLock) {
    PR_DestroyLock(mVideoUpdateLock);
    mVideoUpdateLock = nsnull;
  }
}


nsresult nsVideoDecoder::InitLogger() 
{
#ifdef PR_LOGGING
  gVideoDecoderLog = PR_NewLogModule("nsVideoDecoder");
#endif
  return NS_OK;
}

static void InvalidateCallback(nsITimer* aTimer, void* aClosure)
{
  nsVideoDecoder* decoder = static_cast<nsVideoDecoder*>(aClosure);
  decoder->Invalidate();
}

nsresult nsVideoDecoder::StartInvalidating(double aFramerate)
{
  nsresult rv = NS_OK;

  if (!mInvalidateTimer && aFramerate > 0.0) {
    mInvalidateTimer = do_CreateInstance("@mozilla.org/timer;1");
    rv = mInvalidateTimer->InitWithFuncCallback(InvalidateCallback, 
                                                this, 
                                                static_cast<PRInt32>(1000.0/aFramerate), 
                                                nsITimer::TYPE_REPEATING_PRECISE);
  }
  return rv;
}

void nsVideoDecoder::StopInvalidating()
{
  if (mInvalidateTimer) {
    mInvalidateTimer->Cancel();
    mInvalidateTimer = nsnull;
  }
}

void nsVideoDecoder::Invalidate()
{
  if (!mElement)
    return;

  nsIFrame* frame = mElement->GetPrimaryFrame();
  if (!frame)
    return;
  
  {
    nsAutoLock lock(mVideoUpdateLock);
    if (mSizeChanged) {
      mSizeChanged = PR_FALSE;
      nsPresContext* presContext = frame->PresContext();      
      nsIPresShell *presShell = presContext->PresShell();
      presShell->FrameNeedsReflow(frame, 
                                  nsIPresShell::eStyleChange,
                                  NS_FRAME_IS_DIRTY);
    }
  }
  nsRect r(nsPoint(0,0), frame->GetSize());
  frame->Invalidate(r);
}

static void ProgressCallback(nsITimer* aTimer, void* aClosure)
{
  nsVideoDecoder* decoder = static_cast<nsVideoDecoder*>(aClosure);
  decoder->Progress();
}

void nsVideoDecoder::Progress()
{
  if (!mElement)
    return;

  mElement->DispatchProgressEvent(NS_LITERAL_STRING("progress"));
}

nsresult nsVideoDecoder::StartProgress()
{
  nsresult rv = NS_OK;

  if (!mProgressTimer) {
    mProgressTimer = do_CreateInstance("@mozilla.org/timer;1");
    rv = mProgressTimer->InitWithFuncCallback(ProgressCallback, 
                                              this, 
                                              350, 
                                              nsITimer::TYPE_REPEATING_PRECISE);
  }
  return rv;
}

nsresult nsVideoDecoder::StopProgress()
{
  nsresult rv = NS_OK;
  if (mProgressTimer) {
    rv = mProgressTimer->Cancel();
    mProgressTimer = nsnull;
  }
  return rv;
}

void nsVideoDecoder::SetRGBData(PRInt32 aWidth, PRInt32 aHeight, double aFramerate, unsigned char* aRGBBuffer)
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

void nsVideoDecoder::Paint(gfxContext* aContext, const gfxRect& aRect)
{
  nsAutoLock lock(mVideoUpdateLock);

  if (!mRGB)
    return;

  if (mFramerate > 0.0) {
    StartInvalidating(mFramerate);
  }

  
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

  
  aContext->NewPath();
  aContext->PixelSnappedRectangleAndSetPattern(aRect, pat);
  aContext->Fill();

#ifdef DEBUG_FRAME_RATE
  {
    
    static double last = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
    double now = double(PR_IntervalToMilliseconds(PR_IntervalNow()))/1000.0;
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

void nsVideoDecoder::ElementAvailable(nsHTMLMediaElement* anElement)
{
  mElement = anElement;
}

void nsVideoDecoder::ElementUnavailable()
{
  mElement = nsnull;
}

