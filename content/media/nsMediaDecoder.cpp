




































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

#if defined(XP_MACOSX)
#include "gfxQuartzImageSurface.h"
#endif


#define PROGRESS_MS 350


#define STALL_MS 3000

#ifdef PR_LOGGING

PRLogModuleInfo* gVideoDecoderLog = nsnull;
#endif

nsMediaDecoder::nsMediaDecoder() :
  mElement(0),
  mRGBWidth(-1),
  mRGBHeight(-1),
  mProgressTime(),
  mDataTime(),
  mVideoUpdateLock(nsnull),
  mFramerate(0.0),
  mAspectRatio(1.0),
  mSizeChanged(PR_FALSE),
  mShuttingDown(PR_FALSE)
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

static PRInt32 ConditionDimension(float aValue, PRInt32 aDefault)
{
  
  if (aValue >= 1.0 && aValue <= 10000.0)
    return PRInt32(NS_round(aValue));
  return aDefault;
}

void nsMediaDecoder::Invalidate()
{
  if (!mElement)
    return;

  nsIFrame* frame = mElement->GetPrimaryFrame();
  
  {
    nsAutoLock lock(mVideoUpdateLock);
    if (mSizeChanged) {
      nsIntSize scaledSize(mRGBWidth, mRGBHeight);
      
      
      if (mAspectRatio > 1.0) {
        
        scaledSize.width =
          ConditionDimension(mAspectRatio*scaledSize.width, scaledSize.width);
      } else {
        
        scaledSize.height =
          ConditionDimension(scaledSize.height/mAspectRatio, scaledSize.height);
      }
      mElement->UpdateMediaSize(scaledSize);

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

  TimeStamp now = TimeStamp::Now();

  if (!aTimer) {
    mDataTime = now;
  }

  
  
  if ((mProgressTime.IsNull() ||
       now - mProgressTime >= TimeDuration::FromMilliseconds(PROGRESS_MS)) &&
      !mDataTime.IsNull() &&
      now - mDataTime <= TimeDuration::FromMilliseconds(PROGRESS_MS)) {
    mElement->DispatchAsyncProgressEvent(NS_LITERAL_STRING("progress"));
    mProgressTime = now;
  }

  if (!mDataTime.IsNull() &&
      now - mDataTime >= TimeDuration::FromMilliseconds(STALL_MS)) {
    mElement->DownloadStalled();
    
    mDataTime = TimeStamp();
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

void nsMediaDecoder::SetRGBData(PRInt32 aWidth, PRInt32 aHeight, float aFramerate,
                                float aAspectRatio, unsigned char* aRGBBuffer)
{
  nsAutoLock lock(mVideoUpdateLock);

  if (mRGBWidth != aWidth || mRGBHeight != aHeight ||
      mAspectRatio != aAspectRatio) {
    mRGBWidth = aWidth;
    mRGBHeight = aHeight;
    mAspectRatio = aAspectRatio;
    mSizeChanged = PR_TRUE;
  }
  mFramerate = aFramerate;
  mRGB = aRGBBuffer;
}

void nsMediaDecoder::Paint(gfxContext* aContext,
                           gfxPattern::GraphicsFilter aFilter,
                           const gfxRect& aRect)
{
  nsAutoLock lock(mVideoUpdateLock);

  if (!mRGB)
    return;

  nsRefPtr<gfxImageSurface> imgSurface =
      new gfxImageSurface(mRGB,
                          gfxIntSize(mRGBWidth, mRGBHeight),
                          mRGBWidth * 4,
                          gfxASurface::ImageFormatRGB24);
  if (!imgSurface)
    return;

  nsRefPtr<gfxASurface> surface(imgSurface);

#if defined(XP_MACOSX)
  nsRefPtr<gfxQuartzImageSurface> quartzSurface =
    new gfxQuartzImageSurface(imgSurface);
  if (!quartzSurface)
    return;

  surface = quartzSurface;
#endif

  nsRefPtr<gfxPattern> pat = new gfxPattern(surface);
  if (!pat)
    return;

  
  pat->SetMatrix(gfxMatrix().Scale(mRGBWidth/aRect.Width(), mRGBHeight/aRect.Height()));

  pat->SetFilter(aFilter);

  
  
  gfxPattern::GraphicsExtend extend = gfxPattern::EXTEND_PAD;

  
  
  nsRefPtr<gfxASurface> target = aContext->CurrentSurface();
  gfxASurface::gfxSurfaceType type = target->GetType();
  if (type == gfxASurface::SurfaceTypeXlib ||
      type == gfxASurface::SurfaceTypeXcb ||
      type == gfxASurface::SurfaceTypeQuartz) {
    extend = gfxPattern::EXTEND_NONE;
  }

  pat->SetExtend(extend);

  
  aContext->NewPath();
  aContext->PixelSnappedRectangleAndSetPattern(aRect, pat);
  aContext->Fill();
}

