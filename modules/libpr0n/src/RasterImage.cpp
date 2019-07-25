











































#include "nsComponentManagerUtils.h"
#include "imgIContainerObserver.h"
#include "ImageErrors.h"
#include "Decoder.h"
#include "imgIDecoderObserver.h"
#include "RasterImage.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"
#include "nsStringStream.h"
#include "prmem.h"
#include "prlog.h"
#include "prenv.h"
#include "nsTime.h"
#include "ImageLogging.h"

#include "nsPNGDecoder.h"
#include "nsGIFDecoder2.h"
#include "nsJPEGDecoder.h"
#include "nsBMPDecoder.h"
#include "nsICODecoder.h"
#include "nsIconDecoder.h"

#include "gfxContext.h"

using namespace mozilla::imagelib;


#define DECODE_FLAGS_MASK (imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA | imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION)
#define DECODE_FLAGS_DEFAULT 0


#if defined(PR_LOGGING)
static PRLogModuleInfo *gCompressedImageAccountingLog = PR_NewLogModule ("CompressedImageAccounting");
#else
#define gCompressedImageAccountingLog
#endif


static PRUint32 gDecodeBytesAtATime = 200000;
static PRUint32 gMaxMSBeforeYield = 400;
static PRUint32 gMaxBytesForSyncDecode = 150000;

void
RasterImage::SetDecodeBytesAtATime(PRUint32 aBytesAtATime)
{
  gDecodeBytesAtATime = aBytesAtATime;
}
void
RasterImage::SetMaxMSBeforeYield(PRUint32 aMaxMS)
{
  gMaxMSBeforeYield = aMaxMS;
}
void
RasterImage::SetMaxBytesForSyncDecode(PRUint32 aMaxBytes)
{
  gMaxBytesForSyncDecode = aMaxBytes;
}
















#define LOG_CONTAINER_ERROR                      \
  PR_BEGIN_MACRO                                 \
  PR_LOG (gImgLog, PR_LOG_ERROR,                 \
          ("RasterImage: [this=%p] Error "      \
           "detected at line %u for image of "   \
           "type %s\n", this, __LINE__,          \
           mSourceDataMimeType.get()));          \
  PR_END_MACRO

#define CONTAINER_ENSURE_SUCCESS(status)      \
  PR_BEGIN_MACRO                              \
  nsresult _status = status; /* eval once */  \
  if (_status) {                              \
    LOG_CONTAINER_ERROR;                      \
    DoError();                                \
    return _status;                           \
  }                                           \
 PR_END_MACRO

#define CONTAINER_ENSURE_TRUE(arg, rv)  \
  PR_BEGIN_MACRO                        \
  if (!(arg)) {                         \
    LOG_CONTAINER_ERROR;                \
    DoError();                          \
    return rv;                          \
  }                                     \
  PR_END_MACRO



static int num_containers;
static int num_discardable_containers;
static PRInt64 total_source_bytes;
static PRInt64 discardable_source_bytes;


static PRBool
DiscardingEnabled()
{
  static PRBool inited;
  static PRBool enabled;

  if (!inited) {
    inited = PR_TRUE;

    enabled = (PR_GetEnv("MOZ_DISABLE_IMAGE_DISCARD") == nsnull);
  }

  return enabled;
}

namespace mozilla {
namespace imagelib {

#ifndef DEBUG
NS_IMPL_ISUPPORTS4(RasterImage, imgIContainer, nsITimerCallback, nsIProperties,
                   nsISupportsWeakReference)
#else
NS_IMPL_ISUPPORTS5(RasterImage, imgIContainer, nsITimerCallback, nsIProperties,
                   imgIContainerDebug, nsISupportsWeakReference)
#endif


RasterImage::RasterImage(imgStatusTracker* aStatusTracker) :
  Image(aStatusTracker), 
  mSize(0,0),
  mFrameDecodeFlags(DECODE_FLAGS_DEFAULT),
  mAnim(nsnull),
  mLoopCount(-1),
  mObserver(nsnull),
  mLockCount(0),
  mDecoder(nsnull),
  mWorker(nsnull),
  mBytesDecoded(0),
#ifdef DEBUG
  mFramesNotified(0),
#endif
  mHasSize(PR_FALSE),
  mDecodeOnDraw(PR_FALSE),
  mMultipart(PR_FALSE),
  mDiscardable(PR_FALSE),
  mHasSourceData(PR_FALSE),
  mDecoded(PR_FALSE),
  mHasBeenDecoded(PR_FALSE),
  mWorkerPending(PR_FALSE),
  mInDecoder(PR_FALSE),
  mAnimationFinished(PR_FALSE)
{
  
  mDiscardTrackerNode.curr = this;
  mDiscardTrackerNode.prev = mDiscardTrackerNode.next = nsnull;

  
  num_containers++;
}


RasterImage::~RasterImage()
{
  delete mAnim;

  for (unsigned int i = 0; i < mFrames.Length(); ++i)
    delete mFrames[i];

  
  if (mDiscardable) {
    num_discardable_containers--;
    discardable_source_bytes -= mSourceData.Length();

    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: destroying RasterImage %p.  "
             "Total Containers: %d, Discardable containers: %d, "
             "Total source bytes: %lld, Source bytes for discardable containers %lld",
             this,
             num_containers,
             num_discardable_containers,
             total_source_bytes,
             discardable_source_bytes));
  }

  DiscardTracker::Remove(&mDiscardTrackerNode);

  
  if (mDecoder) {
    nsresult rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    if (NS_FAILED(rv))
      NS_WARNING("Failed to shut down decoder in destructor!");
  }

  
  num_containers--;
  total_source_bytes -= mSourceData.Length();
}

nsresult
RasterImage::Init(imgIDecoderObserver *aObserver,
                  const char* aMimeType,
                  const char* aURIString,
                  PRUint32 aFlags)
{
  
  if (mInitialized)
    return NS_ERROR_ILLEGAL_VALUE;

  
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aMimeType);

  
  
  NS_ABORT_IF_FALSE(!(aFlags & INIT_FLAG_MULTIPART) ||
                    (!(aFlags & INIT_FLAG_DISCARDABLE) &&
                     !(aFlags & INIT_FLAG_DECODE_ON_DRAW)),
                    "Can't be discardable or decode-on-draw for multipart");

  
  mObserver = do_GetWeakReference(aObserver);
  mSourceDataMimeType.Assign(aMimeType);
  mURIString.Assign(aURIString);
  mDiscardable = !!(aFlags & INIT_FLAG_DISCARDABLE);
  mDecodeOnDraw = !!(aFlags & INIT_FLAG_DECODE_ON_DRAW);
  mMultipart = !!(aFlags & INIT_FLAG_MULTIPART);

  
  if (mDiscardable) {
    num_discardable_containers++;
    discardable_source_bytes += mSourceData.Length();
  }

  
  
  
  if (mSourceDataMimeType.Length() == 0) {
    mInitialized = PR_TRUE;
    return NS_OK;
  }

  
  
  
  
  
  nsresult rv = InitDecoder( mDecodeOnDraw);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  mInitialized = PR_TRUE;

  return NS_OK;
}





NS_IMETHODIMP
RasterImage::ExtractFrame(PRUint32 aWhichFrame,
                          const nsIntRect &aRegion,
                          PRUint32 aFlags,
                          imgIContainer **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  
  nsRefPtr<RasterImage> img(new RasterImage());
  NS_ENSURE_TRUE(img, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  img->Init(nsnull, "", "", INIT_FLAG_NONE);
  img->SetSize(aRegion.width, aRegion.height);
  img->mDecoded = PR_TRUE; 
  img->mHasBeenDecoded = PR_TRUE;
  img->mFrameDecodeFlags = aFlags & DECODE_FLAGS_MASK;

  if (img->mFrameDecodeFlags != mFrameDecodeFlags) {
    
    
    
    if (!(aFlags & FLAG_SYNC_DECODE))
      return NS_ERROR_NOT_AVAILABLE;
    if (!CanForciblyDiscard() || mDecoder || mAnim)
      return NS_ERROR_NOT_AVAILABLE;
    ForceDiscard();

    mFrameDecodeFlags = img->mFrameDecodeFlags;
  }
  
  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  PRUint32 frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  
  
  nsIntRect framerect = frame->GetRect();
  framerect.IntersectRect(framerect, aRegion);

  if (framerect.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoPtr<imgFrame> subframe;
  rv = frame->Extract(framerect, getter_Transfers(subframe));
  if (NS_FAILED(rv))
    return rv;

  img->mFrames.AppendElement(subframe.forget());

  img->mStatusTracker->RecordLoaded();
  img->mStatusTracker->RecordDecoded();

  *_retval = img.forget().get();

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetWidth(PRInt32 *aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  if (mError) {
    *aWidth = 0;
    return NS_ERROR_FAILURE;
  }

  *aWidth = mSize.width;
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetHeight(PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  if (mError) {
    *aHeight = 0;
    return NS_ERROR_FAILURE;
  }

  *aHeight = mSize.height;
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetType(PRUint16 *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = GetType();
  return NS_OK;
}



NS_IMETHODIMP_(PRUint16)
RasterImage::GetType()
{
  return imgIContainer::TYPE_RASTER;
}

imgFrame*
RasterImage::GetImgFrame(PRUint32 framenum)
{
  nsresult rv = WantDecodedFrames();
  CONTAINER_ENSURE_TRUE(NS_SUCCEEDED(rv), nsnull);

  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames.SafeElementAt(0, nsnull);
  }
  if (mAnim->lastCompositedFrameIndex == PRInt32(framenum))
    return mAnim->compositingFrame;
  return mFrames.SafeElementAt(framenum, nsnull);
}

imgFrame*
RasterImage::GetDrawableImgFrame(PRUint32 framenum)
{
  imgFrame *frame = GetImgFrame(framenum);

  
  
  if (frame && frame->GetCompositingFailed())
    return nsnull;
  return frame;
}

PRUint32
RasterImage::GetCurrentImgFrameIndex() const
{
  if (mAnim)
    return mAnim->currentAnimationFrameIndex;

  return 0;
}

imgFrame*
RasterImage::GetCurrentImgFrame()
{
  return GetImgFrame(GetCurrentImgFrameIndex());
}

imgFrame*
RasterImage::GetCurrentDrawableImgFrame()
{
  return GetDrawableImgFrame(GetCurrentImgFrameIndex());
}



NS_IMETHODIMP
RasterImage::GetCurrentFrameIsOpaque(PRBool *aIsOpaque)
{
  NS_ENSURE_ARG_POINTER(aIsOpaque);

  if (mError)
    return NS_ERROR_FAILURE;

  
  imgFrame *curframe = GetCurrentImgFrame();

  
  if (!curframe)
    *aIsOpaque = PR_FALSE;

  
  else {
    *aIsOpaque = !curframe->GetNeedsBackground();

    
    
    nsIntRect framerect = curframe->GetRect();
    *aIsOpaque = *aIsOpaque && framerect.IsEqualInterior(nsIntRect(0, 0, mSize.width, mSize.height));
  }

  return NS_OK;
}

void
RasterImage::GetCurrentFrameRect(nsIntRect& aRect)
{
  
  imgFrame* curframe = GetCurrentImgFrame();

  
  if (curframe) {
    aRect = curframe->GetRect();
  } else {
    
    
    
    
    
    aRect.MoveTo(0, 0);
    aRect.SizeTo(0, 0);
  }
}

PRUint32
RasterImage::GetCurrentFrameIndex()
{
  return GetCurrentImgFrameIndex();
}

PRUint32
RasterImage::GetNumFrames()
{
  return mFrames.Length();
}



NS_IMETHODIMP
RasterImage::GetAnimated(PRBool *aAnimated)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aAnimated);

  
  if (mAnim) {
    *aAnimated = PR_TRUE;
    return NS_OK;
  }

  
  
  if (!mHasBeenDecoded)
    return NS_ERROR_NOT_AVAILABLE;

  
  *aAnimated = PR_FALSE;

  return NS_OK;
}





NS_IMETHODIMP
RasterImage::CopyFrame(PRUint32 aWhichFrame,
                       PRUint32 aFlags,
                       gfxImageSurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  nsresult rv;

  PRUint32 desiredDecodeFlags = aFlags & DECODE_FLAGS_MASK;
  if (desiredDecodeFlags != mFrameDecodeFlags) {
    
    
    
    if (!(aFlags & FLAG_SYNC_DECODE))
      return NS_ERROR_NOT_AVAILABLE;
    if (!CanForciblyDiscard() || mDecoder || mAnim)
      return NS_ERROR_NOT_AVAILABLE;
    ForceDiscard();

    mFrameDecodeFlags = desiredDecodeFlags;
  }

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  NS_ENSURE_ARG_POINTER(_retval);

  
  
  
  PRUint32 frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxPattern> pattern;
  frame->GetPattern(getter_AddRefs(pattern));
  nsIntRect intframerect = frame->GetRect();
  gfxRect framerect(intframerect.x, intframerect.y, intframerect.width, intframerect.height);

  
  
  nsRefPtr<gfxImageSurface> imgsurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height),
                                                             gfxASurface::ImageFormatARGB32);
  gfxContext ctx(imgsurface);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.SetPattern(pattern);
  ctx.Rectangle(framerect);
  ctx.Fill();

  *_retval = imgsurface.forget().get();
  return NS_OK;
}




NS_IMETHODIMP
RasterImage::GetFrame(PRUint32 aWhichFrame,
                      PRUint32 aFlags,
                      gfxASurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  if (mDecoded) {
    
    
    PRUint32 desiredDecodeFlags = aFlags & DECODE_FLAGS_MASK;
    if (desiredDecodeFlags != mFrameDecodeFlags) {
      
      
      
      if (!(aFlags & FLAG_SYNC_DECODE))
        return NS_ERROR_NOT_AVAILABLE;
      if (!CanForciblyDiscard() || mDecoder || mAnim)
        return NS_ERROR_NOT_AVAILABLE;
  
      ForceDiscard();
  
      mFrameDecodeFlags = desiredDecodeFlags;
    }
  }

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  PRUint32 frameIndex = (aWhichFrame == FRAME_FIRST) ?
                          0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxASurface> framesurf;

  
  
  nsIntRect framerect = frame->GetRect();
  if (framerect.x == 0 && framerect.y == 0 &&
      framerect.width == mSize.width &&
      framerect.height == mSize.height)
    rv = frame->GetSurface(getter_AddRefs(framesurf));

  
  
  if (!framesurf) {
    nsRefPtr<gfxImageSurface> imgsurf;
    rv = CopyFrame(aWhichFrame, aFlags, getter_AddRefs(imgsurf));
    framesurf = imgsurf;
  }

  *_retval = framesurf.forget().get();

  return rv;
}

PRUint32
RasterImage::GetDecodedDataSize()
{
  PRUint32 val = 0;
  for (PRUint32 i = 0; i < mFrames.Length(); ++i) {
    imgFrame *frame = mFrames.SafeElementAt(i, nsnull);
    NS_ABORT_IF_FALSE(frame, "Null frame in frame array!");
    val += frame->EstimateMemoryUsed();
  }

  return val;
}

PRUint32
RasterImage::GetSourceDataSize()
{
  PRUint32 sourceDataSize = mSourceData.Length();
  
  NS_ABORT_IF_FALSE(StoringSourceData() || (sourceDataSize == 0),
                    "Non-zero source data size when we aren't storing it?");
  return sourceDataSize;
}

void
RasterImage::DeleteImgFrame(PRUint32 framenum)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Deleting invalid frame!");

  delete mFrames[framenum];
  mFrames[framenum] = nsnull;
}

nsresult
RasterImage::InternalAddFrameHelper(PRUint32 framenum, imgFrame *aFrame,
                                    PRUint8 **imageData, PRUint32 *imageLength,
                                    PRUint32 **paletteData, PRUint32 *paletteLength)
{
  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(aFrame);

  if (paletteData && paletteLength)
    frame->GetPaletteData(paletteData, paletteLength);

  frame->GetImageData(imageData, imageLength);

  
  
  frame->LockImageData();

  mFrames.InsertElementAt(framenum, frame.forget());

  return NS_OK;
}
                                  
nsresult
RasterImage::InternalAddFrame(PRUint32 framenum,
                              PRInt32 aX, PRInt32 aY,
                              PRInt32 aWidth, PRInt32 aHeight,
                              gfxASurface::gfxImageFormat aFormat,
                              PRUint8 aPaletteDepth,
                              PRUint8 **imageData,
                              PRUint32 *imageLength,
                              PRUint32 **paletteData,
                              PRUint32 *paletteLength)
{
  
  
  
  NS_ABORT_IF_FALSE(mInDecoder, "Only decoders may add frames!");

  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(new imgFrame());
  NS_ENSURE_TRUE(frame, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = frame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (mFrames.Length() > 0) {
    imgFrame *prevframe = mFrames.ElementAt(mFrames.Length() - 1);
    prevframe->UnlockImageData();
  }

  if (mFrames.Length() == 0) {
    return InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength, 
                                  paletteData, paletteLength);
  }

  if (mFrames.Length() == 1) {
    
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    
    
    PRInt32 frameDisposalMethod = mFrames[0]->GetFrameDisposalMethod();
    if (frameDisposalMethod == kDisposeClear ||
        frameDisposalMethod == kDisposeRestorePrevious)
      mAnim->firstFrameRefreshArea = mFrames[0]->GetRect();
  }

  
  
  
  nsIntRect frameRect = frame->GetRect();
  mAnim->firstFrameRefreshArea.UnionRect(mAnim->firstFrameRefreshArea, 
                                         frameRect);
  
  rv = InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength,
                              paletteData, paletteLength);
  
  
  EvaluateAnimation();
  
  return rv;
}

nsresult
RasterImage::AppendFrame(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
                         PRInt32 aHeight,
                         gfxASurface::gfxImageFormat aFormat,
                         PRUint8 **imageData,
                         PRUint32 *imageLength)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);

  return InternalAddFrame(mFrames.Length(), aX, aY, aWidth, aHeight, aFormat, 
                           0, imageData, imageLength,
                           nsnull, 
                           nsnull);
}

nsresult
RasterImage::AppendPalettedFrame(PRInt32 aX, PRInt32 aY,
                                 PRInt32 aWidth, PRInt32 aHeight,
                                 gfxASurface::gfxImageFormat aFormat,
                                 PRUint8 aPaletteDepth,
                                 PRUint8 **imageData,
                                 PRUint32 *imageLength,
                                 PRUint32 **paletteData,
                                 PRUint32 *paletteLength)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  NS_ENSURE_ARG_POINTER(paletteData);
  NS_ENSURE_ARG_POINTER(paletteLength);

  return InternalAddFrame(mFrames.Length(), aX, aY, aWidth, aHeight, aFormat, 
                          aPaletteDepth, imageData, imageLength,
                          paletteData, paletteLength);
}

nsresult
RasterImage::SetSize(PRInt32 aWidth, PRInt32 aHeight)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  if ((aWidth < 0) || (aHeight < 0))
    return NS_ERROR_INVALID_ARG;

  
  if (mHasSize &&
      ((aWidth != mSize.width) || (aHeight != mSize.height))) {

    
    if (!mMultipart)
      NS_WARNING("Image changed size on redecode! This should not happen!");
    else
      NS_WARNING("Multipart channel sent an image of a different size");

    
    
    if (mDecoder)
      mDecoder->PostResizeError();

    DoError();
    return NS_ERROR_UNEXPECTED;
  }

  
  mSize.SizeTo(aWidth, aHeight);
  mHasSize = PR_TRUE;

  return NS_OK;
}

nsresult
RasterImage::EnsureCleanFrame(PRUint32 aFrameNum, PRInt32 aX, PRInt32 aY,
                              PRInt32 aWidth, PRInt32 aHeight,
                              gfxASurface::gfxImageFormat aFormat,
                              PRUint8 **imageData, PRUint32 *imageLength)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  NS_ABORT_IF_FALSE(aFrameNum <= mFrames.Length(), "Invalid frame index!");
  if (aFrameNum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  
  if (aFrameNum == mFrames.Length())
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                             0, imageData, imageLength,
                             nsnull, 
                             nsnull);

  imgFrame *frame = GetImgFrame(aFrameNum);
  if (!frame)
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                             0, imageData, imageLength,
                             nsnull, 
                             nsnull);

  
  nsIntRect rect = frame->GetRect();
  if (rect.x == aX && rect.y == aY && rect.width == aWidth &&
      rect.height == aHeight && frame->GetFormat() == aFormat) {
    
    frame->GetImageData(imageData, imageLength);
    if (*imageData) {
      return NS_OK;
    }
  }

  DeleteImgFrame(aFrameNum);
  return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                           0, imageData, imageLength,
                           nsnull, 
                           nsnull);
}

void
RasterImage::FrameUpdated(PRUint32 aFrameNum, nsIntRect &aUpdatedRect)
{
  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling FrameUpdated on frame that doesn't exist!");

  frame->ImageUpdated(aUpdatedRect);
}

nsresult
RasterImage::SetFrameDisposalMethod(PRUint32 aFrameNum,
                                    PRInt32 aDisposalMethod)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame,
                    "Calling SetFrameDisposalMethod on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetFrameDisposalMethod(aDisposalMethod);

  return NS_OK;
}

nsresult
RasterImage::SetFrameTimeout(PRUint32 aFrameNum, PRInt32 aTimeout)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameTimeout on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetTimeout(aTimeout);

  return NS_OK;
}

nsresult
RasterImage::SetFrameBlendMethod(PRUint32 aFrameNum, PRInt32 aBlendMethod)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameBlendMethod on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetBlendMethod(aBlendMethod);

  return NS_OK;
}

nsresult
RasterImage::SetFrameHasNoAlpha(PRUint32 aFrameNum)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameHasNoAlpha on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetHasNoAlpha();

  return NS_OK;
}

nsresult
RasterImage::DecodingComplete()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  
  mDecoded = PR_TRUE;
  mHasBeenDecoded = PR_TRUE;

  nsresult rv;

  
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(!DiscardingActive(),
                      "We shouldn't have been discardable before this");
    rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  
  if ((mFrames.Length() == 1) && !mMultipart) {
    rv = mFrames[0]->Optimize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



nsresult
RasterImage::StartAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(ShouldAnimate(), "Should not animate!");

  if (!ensureAnimExists())
    return NS_ERROR_OUT_OF_MEMORY;

  NS_ABORT_IF_FALSE(mAnim && !mAnim->timer, "Anim must exist and not have a timer yet");
  
  
  
  PRInt32 timeout = 100;
  imgFrame *currentFrame = GetCurrentImgFrame();
  if (currentFrame) {
    timeout = currentFrame->GetTimeout();
    if (timeout < 0) { 
      mAnimationFinished = PR_TRUE;
      return NS_ERROR_ABORT;
    }
  }
  
  mAnim->timer = do_CreateInstance("@mozilla.org/timer;1");
  NS_ENSURE_TRUE(mAnim->timer, NS_ERROR_OUT_OF_MEMORY);
  mAnim->timer->InitWithCallback(static_cast<nsITimerCallback*>(this),
                                 timeout, nsITimer::TYPE_REPEATING_SLACK);
  
  return NS_OK;
}



nsresult
RasterImage::StopAnimation()
{
  NS_ABORT_IF_FALSE(mAnimating, "Should be animating!");

  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnim->timer) {
    mAnim->timer->Cancel();
    mAnim->timer = nsnull;
  }

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::ResetAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnimationMode == kDontAnimMode || 
      !mAnim || mAnim->currentAnimationFrameIndex == 0)
    return NS_OK;

  mAnimationFinished = PR_FALSE;

  if (mAnimating)
    StopAnimation();

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;

  
  

  
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (mAnimating && observer)
    observer->FrameChanged(this, &(mAnim->firstFrameRefreshArea));

  if (ShouldAnimate()) {
    StartAnimation();
    
    
    
    mAnimating = PR_TRUE;
  }

  return NS_OK;
}

void
RasterImage::SetLoopCount(PRInt32 aLoopCount)
{
  if (mError)
    return;

  
  
  
  
  mLoopCount = aLoopCount;
}

nsresult
RasterImage::AddSourceData(const char *aBuffer, PRUint32 aCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aBuffer);
  nsresult rv = NS_OK;

  
  NS_ABORT_IF_FALSE(mInitialized, "Calling AddSourceData() on uninitialized "
                                  "RasterImage!");

  
  NS_ABORT_IF_FALSE(!mHasSourceData, "Calling AddSourceData() after calling "
                                     "sourceDataComplete()!");

  
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  
  if (!StoringSourceData()) {
    rv = WriteToDecoder(aBuffer, aCount);
    CONTAINER_ENSURE_SUCCESS(rv);

    
    
    
    nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
    mInDecoder = PR_TRUE;
    mDecoder->FlushInvalidations();
    mInDecoder = PR_FALSE;
  }

  
  else {

    
    char *newElem = mSourceData.AppendElements(aBuffer, aCount);
    if (!newElem)
      return NS_ERROR_OUT_OF_MEMORY;

    
    
    if (mDecoder && !mWorkerPending) {
      NS_ABORT_IF_FALSE(mWorker, "We should have a worker here!");
      rv = mWorker->Run();
      CONTAINER_ENSURE_SUCCESS(rv);
    }
  }

  
  total_source_bytes += aCount;
  if (mDiscardable)
    discardable_source_bytes += aCount;
  PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
          ("CompressedImageAccounting: Added compressed data to RasterImage %p (%s). "
           "Total Containers: %d, Discardable containers: %d, "
           "Total source bytes: %lld, Source bytes for discardable containers %lld",
           this,
           mSourceDataMimeType.get(),
           num_containers,
           num_discardable_containers,
           total_source_bytes,
           discardable_source_bytes));

  return NS_OK;
}



static void
get_header_str (char *buf, char *data, PRSize data_len)
{
  int i;
  int n;
  static char hex[] = "0123456789abcdef";

  n = data_len < 4 ? data_len : 4;

  for (i = 0; i < n; i++) {
    buf[i * 2]     = hex[(data[i] >> 4) & 0x0f];
    buf[i * 2 + 1] = hex[data[i] & 0x0f];
  }

  buf[i * 2] = 0;
}

nsresult
RasterImage::SourceDataComplete()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mHasSourceData)
    return NS_OK;
  mHasSourceData = PR_TRUE;

  
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  
  
  
  if (!StoringSourceData()) {
    nsresult rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  
  
  
  if (mDecoder && !mWorkerPending) {
    NS_ABORT_IF_FALSE(mWorker, "We should have a worker here!");
    nsresult rv = mWorker->Run();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  mSourceData.Compact();

  
  if (PR_LOG_TEST(gCompressedImageAccountingLog, PR_LOG_DEBUG)) {
    char buf[9];
    get_header_str(buf, mSourceData.Elements(), mSourceData.Length());
    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: RasterImage::SourceDataComplete() - data "
             "is done for container %p (%s) - header %p is 0x%s (length %d)",
             this,
             mSourceDataMimeType.get(),
             mSourceData.Elements(),
             buf,
             mSourceData.Length()));
  }

  
  if (CanDiscard()) {
    nsresult rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }
  return NS_OK;
}

nsresult
RasterImage::NewSourceData()
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mHasSourceData,
                    "Calling NewSourceData before SourceDataComplete!");
  if (!mHasSourceData)
    return NS_ERROR_ILLEGAL_VALUE;

  
  
  
  NS_ABORT_IF_FALSE(mMultipart, "NewSourceData not supported for multipart");
  if (!mMultipart)
    return NS_ERROR_ILLEGAL_VALUE;

  
  NS_ABORT_IF_FALSE(!StoringSourceData(),
                    "Shouldn't be storing source data for multipart");

  
  
  NS_ABORT_IF_FALSE(!mDecoder, "Shouldn't have a decoder in NewSourceData");

  
  NS_ABORT_IF_FALSE(mDecoded, "Should be decoded in NewSourceData");

  
  mDecoded = PR_FALSE;
  mHasSourceData = PR_FALSE;

  
  
  rv = InitDecoder( false);
  CONTAINER_ENSURE_SUCCESS(rv);

  return NS_OK;
}

nsresult
RasterImage::SetSourceSizeHint(PRUint32 sizeHint)
{
  if (sizeHint && StoringSourceData())
    mSourceData.SetCapacity(sizeHint);
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::Notify(nsITimer *timer)
{
#ifdef DEBUG
  mFramesNotified++;
#endif

  
  
  NS_ABORT_IF_FALSE(mAnim, "Need anim for Notify()");
  NS_ABORT_IF_FALSE(timer, "Need timer for Notify()");
  NS_ABORT_IF_FALSE(mAnim->timer == timer,
                    "RasterImage::Notify() called with incorrect timer");

  if (!mAnimating || !ShouldAnimate())
    return NS_OK;

  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (!observer) {
    
    NS_ABORT_IF_FALSE(mAnimationConsumers == 0,
                      "If no observer, should have no consumers");
    if (mAnimating)
      StopAnimation();
    return NS_OK;
  }

  if (mFrames.Length() == 0)
    return NS_OK;
  
  imgFrame *nextFrame = nsnull;
  PRInt32 previousFrameIndex = mAnim->currentAnimationFrameIndex;
  PRUint32 nextFrameIndex = mAnim->currentAnimationFrameIndex + 1;
  PRInt32 timeout = 0;

  
  
  
  NS_ABORT_IF_FALSE(mDecoder || nextFrameIndex <= mFrames.Length(),
                    "How did we get 2 indicies too far by incrementing?");
  bool haveFullNextFrame = !mDecoder || nextFrameIndex < mDecoder->GetCompleteFrameCount();

  
  NS_ABORT_IF_FALSE(haveFullNextFrame ||
                    (mDecoder && mFrames.Length() > mDecoder->GetCompleteFrameCount()),
                    "What is the next frame supposed to be?");

  
  
  if (haveFullNextFrame) {
    if (mFrames.Length() == nextFrameIndex) {
      

      
      if (mAnimationMode == kLoopOnceAnimMode || mLoopCount == 0) {
        mAnimationFinished = PR_TRUE;
        EvaluateAnimation();
        return NS_OK;
      } else {
        
        
        if (mAnim->compositingFrame && mAnim->lastCompositedFrameIndex == -1)
          mAnim->compositingFrame = nsnull;
      }

      nextFrameIndex = 0;
      if (mLoopCount > 0)
        mLoopCount--;
    }

    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      mAnim->timer->SetDelay(100);
      return NS_OK;
    }
    timeout = nextFrame->GetTimeout();

  } else {
    
    
    mAnim->timer->SetDelay(100);
    return NS_OK;
  }

  if (timeout > 0)
    mAnim->timer->SetDelay(timeout);
  else {
    mAnimationFinished = PR_TRUE;
    EvaluateAnimation();
  }

  nsIntRect dirtyRect;
  imgFrame *frameToUse = nsnull;

  if (nextFrameIndex == 0) {
    frameToUse = nextFrame;
    dirtyRect = mAnim->firstFrameRefreshArea;
  } else {
    imgFrame *prevFrame = mFrames[previousFrameIndex];
    if (!prevFrame)
      return NS_OK;

    
    if (NS_FAILED(DoComposite(&frameToUse, &dirtyRect, prevFrame,
                              nextFrame, nextFrameIndex))) {
      
      NS_WARNING("RasterImage::Notify(): Composing Frame Failed\n");
      nextFrame->SetCompositingFailed(PR_TRUE);
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      return NS_OK;
    } else {
      nextFrame->SetCompositingFailed(PR_FALSE);
    }
  }
  
  mAnim->currentAnimationFrameIndex = nextFrameIndex;
  
  observer->FrameChanged(this, &dirtyRect);
  
  return NS_OK;
}




nsresult
RasterImage::DoComposite(imgFrame** aFrameToUse,
                         nsIntRect* aDirtyRect,
                         imgFrame* aPrevFrame,
                         imgFrame* aNextFrame,
                         PRInt32 aNextFrameIndex)
{
  NS_ENSURE_ARG_POINTER(aDirtyRect);
  NS_ENSURE_ARG_POINTER(aPrevFrame);
  NS_ENSURE_ARG_POINTER(aNextFrame);
  NS_ENSURE_ARG_POINTER(aFrameToUse);

  PRInt32 prevFrameDisposalMethod = aPrevFrame->GetFrameDisposalMethod();
  if (prevFrameDisposalMethod == kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = kDisposeClear;

  nsIntRect prevFrameRect = aPrevFrame->GetRect();
  PRBool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                            prevFrameRect.width == mSize.width &&
                            prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && 
      (prevFrameDisposalMethod == kDisposeClear))
    prevFrameDisposalMethod = kDisposeClearAll;

  PRInt32 nextFrameDisposalMethod = aNextFrame->GetFrameDisposalMethod();
  nsIntRect nextFrameRect = aNextFrame->GetRect();
  PRBool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                            nextFrameRect.width == mSize.width &&
                            nextFrameRect.height == mSize.height);

  if (!aNextFrame->GetIsPaletted()) {
    
    
    if (prevFrameDisposalMethod == kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  
    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != kDisposeRestorePrevious) &&
        !aNextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case kDisposeNotSpecified:
    case kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case kDisposeClearAll:
      
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;

    case kDisposeClear:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case kDisposeRestorePrevious:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  
  
  
  
  
  
  if (mAnim->lastCompositedFrameIndex == aNextFrameIndex) {
    *aFrameToUse = mAnim->compositingFrame;
    return NS_OK;
  }

  PRBool needToBlankComposite = PR_FALSE;

  
  if (!mAnim->compositingFrame) {
    mAnim->compositingFrame = new imgFrame();
    if (!mAnim->compositingFrame) {
      NS_WARNING("Failed to init compositingFrame!\n");
      return NS_ERROR_OUT_OF_MEMORY;
    }
    nsresult rv = mAnim->compositingFrame->Init(0, 0, mSize.width, mSize.height,
                                                gfxASurface::ImageFormatARGB32);
    if (NS_FAILED(rv)) {
      mAnim->compositingFrame = nsnull;
      return rv;
    }
    needToBlankComposite = PR_TRUE;
  } else if (aNextFrameIndex != mAnim->lastCompositedFrameIndex+1) {

    
    
    needToBlankComposite = PR_TRUE;
  }

  
  
  
  
  
  
  PRBool doDisposal = PR_TRUE;
  if (!aNextFrame->GetHasAlpha() &&
      nextFrameDisposalMethod != kDisposeRestorePrevious) {
    if (isFullNextFrame) {
      
      
      doDisposal = PR_FALSE;
      
      needToBlankComposite = PR_FALSE;
    } else {
      if ((prevFrameRect.x >= nextFrameRect.x) &&
          (prevFrameRect.y >= nextFrameRect.y) &&
          (prevFrameRect.x + prevFrameRect.width <= nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <= nextFrameRect.y + nextFrameRect.height)) {
        
        
        doDisposal = PR_FALSE;  
      }
    }      
  }

  if (doDisposal) {
    
    switch (prevFrameDisposalMethod) {
      case kDisposeClear:
        if (needToBlankComposite) {
          
          
          ClearFrame(mAnim->compositingFrame);
        } else {
          
          ClearFrame(mAnim->compositingFrame, prevFrameRect);
        }
        break;
  
      case kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame);
        break;
  
      case kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);
  
          
          if (nextFrameDisposalMethod != kDisposeRestorePrevious)
            mAnim->compositingPrevFrame = nsnull;
        } else {
          ClearFrame(mAnim->compositingFrame);
        }
        break;
      
      default:
        
        
        
        
        
        
        if (mAnim->lastCompositedFrameIndex != aNextFrameIndex - 1) {
          if (isFullPrevFrame && !aPrevFrame->GetIsPaletted()) {
            
            CopyFrameImage(aPrevFrame, mAnim->compositingFrame);
          } else {
            if (needToBlankComposite) {
              
              if (aPrevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame);
              }
            }
            DrawFrameTo(aPrevFrame, mAnim->compositingFrame, prevFrameRect);
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mAnim->compositingFrame);
  }

  
  
  
  if ((nextFrameDisposalMethod == kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame = new imgFrame();
      if (!mAnim->compositingPrevFrame) {
        NS_WARNING("Failed to init compositingPrevFrame!\n");
        return NS_ERROR_OUT_OF_MEMORY;
      }
      nsresult rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                                      gfxASurface::ImageFormatARGB32);
      if (NS_FAILED(rv)) {
        mAnim->compositingPrevFrame = nsnull;
        return rv;
      }
    }

    CopyFrameImage(mAnim->compositingFrame, mAnim->compositingPrevFrame);
  }

  
  DrawFrameTo(aNextFrame, mAnim->compositingFrame, nextFrameRect);

  
  
  PRInt32 timeout = aNextFrame->GetTimeout();
  mAnim->compositingFrame->SetTimeout(timeout);

  
  nsresult rv = mAnim->compositingFrame->ImageUpdated(mAnim->compositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0 &&
      nextFrameDisposalMethod != kDisposeRestorePrevious &&
      !aNextFrame->GetIsPaletted()) {
    
    
    
    
    
    if (CopyFrameImage(mAnim->compositingFrame, aNextFrame)) {
      aPrevFrame->SetFrameDisposalMethod(kDisposeClearAll);
      mAnim->lastCompositedFrameIndex = -1;
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  mAnim->lastCompositedFrameIndex = aNextFrameIndex;
  *aFrameToUse = mAnim->compositingFrame;

  return NS_OK;
}



void
RasterImage::ClearFrame(imgFrame *aFrame)
{
  if (!aFrame)
    return;

  nsresult rv = aFrame->LockImageData();
  if (NS_FAILED(rv))
    return;

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Paint();

  aFrame->UnlockImageData();
}


void
RasterImage::ClearFrame(imgFrame *aFrame, nsIntRect &aRect)
{
  if (!aFrame || aRect.width <= 0 || aRect.height <= 0)
    return;

  nsresult rv = aFrame->LockImageData();
  if (NS_FAILED(rv))
    return;

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Rectangle(gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
  ctx.Fill();

  aFrame->UnlockImageData();
}





PRBool
RasterImage::CopyFrameImage(imgFrame *aSrcFrame,
                            imgFrame *aDstFrame)
{
  PRUint8* aDataSrc;
  PRUint8* aDataDest;
  PRUint32 aDataLengthSrc;
  PRUint32 aDataLengthDest;

  if (!aSrcFrame || !aDstFrame)
    return PR_FALSE;

  if (NS_FAILED(aDstFrame->LockImageData()))
    return PR_FALSE;

  
  aSrcFrame->GetImageData(&aDataSrc, &aDataLengthSrc);
  aDstFrame->GetImageData(&aDataDest, &aDataLengthDest);
  if (!aDataDest || !aDataSrc || aDataLengthDest != aDataLengthSrc) {
    aDstFrame->UnlockImageData();
    return PR_FALSE;
  }
  memcpy(aDataDest, aDataSrc, aDataLengthSrc);
  aDstFrame->UnlockImageData();

  return PR_TRUE;
}








nsresult
RasterImage::DrawFrameTo(imgFrame *aSrc,
                         imgFrame *aDst,
                         nsIntRect& aSrcRect)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aDst);

  nsIntRect dstRect = aDst->GetRect();

  
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("RasterImage::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  
  if ((aSrcRect.x > dstRect.width) || (aSrcRect.y > dstRect.height)) {
    return NS_OK;
  }

  if (aSrc->GetIsPaletted()) {
    
    PRInt32 width = NS_MIN(aSrcRect.width, dstRect.width - aSrcRect.x);
    PRInt32 height = NS_MIN(aSrcRect.height, dstRect.height - aSrcRect.y);

    
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= dstRect.width) &&
                 (aSrcRect.y + height <= dstRect.height),
                "RasterImage::DrawFrameTo: Invalid aSrcRect");

    
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "RasterImage::DrawFrameTo: source must be smaller than dest");

    if (NS_FAILED(aDst->LockImageData()))
      return NS_ERROR_FAILURE;

    
    PRUint32 size;
    PRUint8 *srcPixels;
    PRUint32 *colormap;
    PRUint32 *dstPixels;

    aSrc->GetImageData(&srcPixels, &size);
    aSrc->GetPaletteData(&colormap, &size);
    aDst->GetImageData((PRUint8 **)&dstPixels, &size);
    if (!srcPixels || !dstPixels || !colormap) {
      aDst->UnlockImageData();
      return NS_ERROR_FAILURE;
    }

    
    dstPixels += aSrcRect.x + (aSrcRect.y * dstRect.width);
    if (!aSrc->GetHasAlpha()) {
      for (PRInt32 r = height; r > 0; --r) {
        for (PRInt32 c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    } else {
      for (PRInt32 r = height; r > 0; --r) {
        for (PRInt32 c = 0; c < width; c++) {
          const PRUint32 color = colormap[srcPixels[c]];
          if (color)
            dstPixels[c] = color;
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    }

    aDst->UnlockImageData();
    return NS_OK;
  }

  nsRefPtr<gfxPattern> srcPatt;
  aSrc->GetPattern(getter_AddRefs(srcPatt));

  aDst->LockImageData();
  nsRefPtr<gfxASurface> dstSurf;
  aDst->GetSurface(getter_AddRefs(dstSurf));

  gfxContext dst(dstSurf);
  dst.Translate(gfxPoint(aSrcRect.x, aSrcRect.y));
  dst.Rectangle(gfxRect(0, 0, aSrcRect.width, aSrcRect.height), PR_TRUE);
  
  
  PRInt32 blendMethod = aSrc->GetBlendMethod();
  if (blendMethod == kBlendSource) {
    gfxContext::GraphicsOperator defaultOperator = dst.CurrentOperator();
    dst.SetOperator(gfxContext::OPERATOR_CLEAR);
    dst.Fill();
    dst.SetOperator(defaultOperator);
  }
  dst.SetPattern(srcPatt);
  dst.Paint();

  aDst->UnlockImageData();

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::Get(const char *prop, const nsIID & iid, void * *result)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Get(prop, iid, result);
}

NS_IMETHODIMP
RasterImage::Set(const char *prop, nsISupports *value)
{
  if (!mProperties)
    mProperties = do_CreateInstance("@mozilla.org/properties;1");
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;
  return mProperties->Set(prop, value);
}

NS_IMETHODIMP
RasterImage::Has(const char *prop, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mProperties) {
    *_retval = PR_FALSE;
    return NS_OK;
  }
  return mProperties->Has(prop, _retval);
}

NS_IMETHODIMP
RasterImage::Undefine(const char *prop)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Undefine(prop);
}

NS_IMETHODIMP
RasterImage::GetKeys(PRUint32 *count, char ***keys)
{
  if (!mProperties) {
    *count = 0;
    *keys = nsnull;
    return NS_OK;
  }
  return mProperties->GetKeys(count, keys);
}

void
RasterImage::Discard(bool force)
{
  
  NS_ABORT_IF_FALSE(force ? CanForciblyDiscard() : CanDiscard(), "Asked to discard but can't!");

  
  NS_ABORT_IF_FALSE(!mDecoder, "Asked to discard with open decoder!");

  
  
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  
  int old_frame_count = mFrames.Length();

  
  for (int i = 0; i < old_frame_count; ++i)
    delete mFrames[i];
  mFrames.Clear();

  
  mDecoded = PR_FALSE;

  
  nsCOMPtr<imgIDecoderObserver> observer(do_QueryReferent(mObserver));
  if (observer)
    observer->OnDiscard(nsnull);

  if (force)
    DiscardTracker::Remove(&mDiscardTrackerNode);

  
  PR_LOG(gCompressedImageAccountingLog, PR_LOG_DEBUG,
         ("CompressedImageAccounting: discarded uncompressed image "
          "data from RasterImage %p (%s) - %d frames (cached count: %d); "
          "Total Containers: %d, Discardable containers: %d, "
          "Total source bytes: %lld, Source bytes for discardable containers %lld",
          this,
          mSourceDataMimeType.get(),
          old_frame_count,
          mFrames.Length(),
          num_containers,
          num_discardable_containers,
          total_source_bytes,
          discardable_source_bytes));
}


PRBool
RasterImage::CanDiscard() {
  return (DiscardingEnabled() && 
          mDiscardable &&        
          (mLockCount == 0) &&   
          mHasSourceData &&      
          mDecoded);             
}

PRBool
RasterImage::CanForciblyDiscard() {
  return (mDiscardable &&        
          mHasSourceData &&      
          mDecoded);             
}



PRBool
RasterImage::DiscardingActive() {
  return !!(mDiscardTrackerNode.prev || mDiscardTrackerNode.next);
}



PRBool
RasterImage::StoringSourceData() {
  return (mDecodeOnDraw || mDiscardable);
}




nsresult
RasterImage::InitDecoder(bool aDoSizeDecode)
{
  
  NS_ABORT_IF_FALSE(!mDecoder, "Calling InitDecoder() while already decoding!");
  
  
  NS_ABORT_IF_FALSE(!mDecoded, "Calling InitDecoder() but already decoded!");

  
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Discard Timer active in InitDecoder()!");

  
  eDecoderType type = GetDecoderType(mSourceDataMimeType.get());
  CONTAINER_ENSURE_TRUE(type != eDecoderType_unknown, NS_IMAGELIB_ERROR_NO_DECODER);

  
  switch (type) {
    case eDecoderType_png:
      mDecoder = new nsPNGDecoder();
      break;
    case eDecoderType_gif:
      mDecoder = new nsGIFDecoder2();
      break;
    case eDecoderType_jpeg:
      mDecoder = new nsJPEGDecoder();
      break;
    case eDecoderType_bmp:
      mDecoder = new nsBMPDecoder();
      break;
    case eDecoderType_ico:
      mDecoder = new nsICODecoder();
      break;
    case eDecoderType_icon:
      mDecoder = new nsIconDecoder();
      break;
    default:
      NS_ABORT_IF_FALSE(0, "Shouldn't get here!");
  }

  
  nsCOMPtr<imgIDecoderObserver> observer(do_QueryReferent(mObserver));
  mDecoder->SetSizeDecode(aDoSizeDecode);
  mDecoder->SetDecodeFlags(mFrameDecodeFlags);
  mDecoder->Init(this, observer);
  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  
  mWorker = new imgDecodeWorker(this);
  CONTAINER_ENSURE_TRUE(mWorker, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}









nsresult
RasterImage::ShutdownDecoder(eShutdownIntent aIntent)
{
  
  NS_ABORT_IF_FALSE((aIntent >= 0) || (aIntent < eShutdownIntent_AllCount),
                    "Invalid shutdown intent");

  
  NS_ABORT_IF_FALSE(mDecoder, "Calling ShutdownDecoder() with no active decoder!");

  
  bool wasSizeDecode = mDecoder->IsSizeDecode();

  
  
  
  nsRefPtr<Decoder> decoder = mDecoder;
  mDecoder = nsnull;

  mInDecoder = PR_TRUE;
  decoder->Finish();
  mInDecoder = PR_FALSE;

  nsresult decoderStatus = decoder->GetDecoderError();
  if (NS_FAILED(decoderStatus)) {
    DoError();
    return decoderStatus;
  }

  
  mWorker = nsnull;

  
  
  PRBool failed = PR_FALSE;
  if (wasSizeDecode && !mHasSize)
    failed = PR_TRUE;
  if (!wasSizeDecode && !mDecoded)
    failed = PR_TRUE;
  if ((aIntent == eShutdownIntent_Done) && failed) {
    DoError();
    return NS_ERROR_FAILURE;
  }

  
  mBytesDecoded = 0;

  return NS_OK;
}


nsresult
RasterImage::WriteToDecoder(const char *aBuffer, PRUint32 aCount)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "Trying to write to null decoder!");

  
  
  
  
  
  if (mFrames.Length() > 0) {
    imgFrame *curframe = mFrames.ElementAt(mFrames.Length() - 1);
    curframe->LockImageData();
  }

  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = PR_TRUE;
  mDecoder->Write(aBuffer, aCount);
  mInDecoder = PR_FALSE;

  
  
  if (mFrames.Length() > 0) {
    imgFrame *curframe = mFrames.ElementAt(mFrames.Length() - 1);
    curframe->UnlockImageData();
  }

  if (!mDecoder)
    return NS_ERROR_FAILURE;
    
  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  
  
  mBytesDecoded += aCount;

  return NS_OK;
}







nsresult
RasterImage::WantDecodedFrames()
{
  nsresult rv;

  
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(DiscardingActive(),
                      "Decoded and discardable but discarding not activated!");
    rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  return RequestDecode();
}



NS_IMETHODIMP
RasterImage::RequestDecode()
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (!StoringSourceData())
    return NS_OK;

  
  if (mDecoded)
    return NS_OK;

  
  if (mDecoder && !mDecoder->IsSizeDecode())
    return NS_OK;

  
  
  
  
  
  
  if (mInDecoder) {
    nsRefPtr<imgDecodeRequestor> requestor = new imgDecodeRequestor(this);
    if (!requestor)
      return NS_ERROR_OUT_OF_MEMORY;
    return NS_DispatchToCurrentThread(requestor);
  }


  
  
  if (mDecoder &&
      (mDecoder->IsSizeDecode() || mDecoder->GetDecodeFlags() != mFrameDecodeFlags))
  {
    rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (mWorkerPending)
    return NS_OK;

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;

  
  if (!mDecoded && !mInDecoder && mHasSourceData && (mSourceData.Length() < gMaxBytesForSyncDecode))
    return SyncDecode();

  
  
  
  return mWorker->Dispatch();
}


nsresult
RasterImage::SyncDecode()
{
  nsresult rv;

  
  if (mDecoded)
    return NS_OK;

  
  if (!StoringSourceData())
    return NS_OK;

  
  
  
  
  NS_ABORT_IF_FALSE(!mInDecoder, "Yikes, forcing sync in reentrant call!");

  
  
  if (mDecoder &&
      (mDecoder->IsSizeDecode() || mDecoder->GetDecodeFlags() != mFrameDecodeFlags))
  {
    rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                      mSourceData.Length() - mBytesDecoded);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  
  
  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = PR_TRUE;
  mDecoder->FlushInvalidations();
  mInDecoder = PR_FALSE;

  
  if (mDecoder && IsDecodeFinished()) {
    rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  return mError ? NS_ERROR_FAILURE : NS_OK;
}









NS_IMETHODIMP
RasterImage::Draw(gfxContext *aContext,
                  gfxPattern::GraphicsFilter aFilter,
                  const gfxMatrix &aUserSpaceToImageSpace,
                  const gfxRect &aFill,
                  const nsIntRect &aSubimage,
                  const nsIntSize& ,
                  PRUint32 aFlags)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  
  
  
  if ((aFlags & DECODE_FLAGS_MASK) != DECODE_FLAGS_DEFAULT)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aContext);

  
  if (mFrameDecodeFlags != DECODE_FLAGS_DEFAULT) {
    if (!CanForciblyDiscard())
      return NS_ERROR_NOT_AVAILABLE;
    ForceDiscard();

    mFrameDecodeFlags = DECODE_FLAGS_DEFAULT;
  }

  
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  imgFrame *frame = GetCurrentDrawableImgFrame();
  if (!frame) {
    return NS_OK; 
  }

  nsIntRect framerect = frame->GetRect();
  nsIntMargin padding(framerect.x, framerect.y, 
                      mSize.width - framerect.XMost(),
                      mSize.height - framerect.YMost());

  frame->Draw(aContext, aFilter, aUserSpaceToImageSpace, aFill, padding, aSubimage);

  return NS_OK;
}



nsIFrame*
RasterImage::GetRootLayoutFrame()
{
  return nsnull;
}



NS_IMETHODIMP
RasterImage::LockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  DiscardTracker::Remove(&mDiscardTrackerNode);

  
  mLockCount++;

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::UnlockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mLockCount > 0,
                    "Calling UnlockImage with mLockCount == 0!");
  if (mLockCount == 0)
    return NS_ERROR_ABORT;

  
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Locked, but discarding activated");

  
  mLockCount--;

  
  if (CanDiscard()) {
    nsresult rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  return NS_OK;
}


nsresult
RasterImage::DecodeSomeData(PRUint32 aMaxBytes)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "trying to decode without decoder!");

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;


  
  PRUint32 bytesToDecode = NS_MIN(aMaxBytes,
                                  mSourceData.Length() - mBytesDecoded);
  nsresult rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                               bytesToDecode);

  return rv;
}





PRBool
RasterImage::IsDecodeFinished()
{
  
  NS_ABORT_IF_FALSE(mDecoder, "Can't call IsDecodeFinished() without decoder!");

  
  PRBool decodeFinished = PR_FALSE;

  
  
  NS_ABORT_IF_FALSE(StoringSourceData(),
                    "just shut down on SourceDataComplete!");

  
  if (mDecoder->IsSizeDecode()) {
    if (mHasSize)
      decodeFinished = PR_TRUE;
  }
  else {
    if (mDecoded)
      decodeFinished = PR_TRUE;
  }

  
  
  
  
  
  if (mHasSourceData && (mBytesDecoded == mSourceData.Length()))
    decodeFinished = PR_TRUE;

  return decodeFinished;
}


void
RasterImage::DoError()
{
  
  if (mError)
    return;

  
  if (mDecoder)
    ShutdownDecoder(eShutdownIntent_Error);

  
  mError = PR_TRUE;

  
  LOG_CONTAINER_ERROR;
}



NS_IMETHODIMP
imgDecodeWorker::Run()
{
  nsresult rv;

  
  nsCOMPtr<nsIRunnable> kungFuDeathGrip(this);

  
  nsCOMPtr<imgIContainer> iContainer(do_QueryReferent(mContainer));
  if (!iContainer)
    return NS_OK;
  RasterImage* image = static_cast<RasterImage*>(iContainer.get());

  NS_ABORT_IF_FALSE(image->mInitialized,
                    "Worker active for uninitialized container!");

  
  image->mWorkerPending = PR_FALSE;

  
  
  
  if (image->mError)
    return NS_OK;

  
  
  if (!image->mDecoder)
    return NS_OK;

  nsRefPtr<Decoder> decoderKungFuDeathGrip = image->mDecoder;

  
  
  
  PRUint32 maxBytes = image->mDecoder->IsSizeDecode()
    ? image->mSourceData.Length() : gDecodeBytesAtATime;

  
  PRBool haveMoreData = PR_TRUE;
  nsTime deadline(PR_Now() + 1000 * gMaxMSBeforeYield);

  
  
  
  
  while (haveMoreData && !image->IsDecodeFinished() &&
         (nsTime(PR_Now()) < deadline)) {

    
    rv = image->DecodeSomeData(maxBytes);
    if (NS_FAILED(rv)) {
      image->DoError();
      return rv;
    }

    
    haveMoreData =
      image->mSourceData.Length() > image->mBytesDecoded;
  }

  
  
  
  
  if (!image->mHasBeenDecoded) {
    image->mInDecoder = PR_TRUE;
    image->mDecoder->FlushInvalidations();
    image->mInDecoder = PR_FALSE;
  }

  
  if (image->mDecoder && image->IsDecodeFinished()) {
    rv = image->ShutdownDecoder(RasterImage::eShutdownIntent_Done);
    if (NS_FAILED(rv)) {
      image->DoError();
      return rv;
    }
  }

  
  
  
  if (image->mDecoder && !image->IsDecodeFinished() && haveMoreData)
    return this->Dispatch();

  
  return NS_OK;
}


NS_METHOD imgDecodeWorker::Dispatch()
{
  
  nsCOMPtr<imgIContainer> iContainer(do_QueryReferent(mContainer));
  if (!iContainer)
    return NS_OK;
  RasterImage* image = static_cast<RasterImage*>(iContainer.get());

  
  NS_ABORT_IF_FALSE(!image->mWorkerPending,
                    "Trying to queue up worker with one already pending!");

  
  image->mWorkerPending = PR_TRUE;

  
  return NS_DispatchToCurrentThread(this);
}




NS_METHOD
RasterImage::WriteToRasterImage(nsIInputStream* ,
                                void*          aClosure,
                                const char*    aFromRawSegment,
                                PRUint32       ,
                                PRUint32       aCount,
                                PRUint32*      aWriteCount)
{
  
  RasterImage* image = static_cast<RasterImage*>(aClosure);

  
  
  
  (void) image->AddSourceData(aFromRawSegment, aCount);

  
  *aWriteCount = aCount;

  return NS_OK;
}

PRBool
RasterImage::ShouldAnimate()
{
  return Image::ShouldAnimate() && mFrames.Length() >= 2 &&
         !mAnimationFinished;
}



#ifdef DEBUG
NS_IMETHODIMP
RasterImage::GetFramesNotified(PRUint32 *aFramesNotified)
{
  NS_ENSURE_ARG_POINTER(aFramesNotified);

  *aFramesNotified = mFramesNotified;

  return NS_OK;
}
#endif

} 
} 
