











































#include "nsComponentManagerUtils.h"
#include "imgIContainerObserver.h"
#include "ImageErrors.h"
#include "imgIDecoder.h"
#include "imgIDecoderObserver.h"
#include "imgContainer.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"
#include "nsStringStream.h"
#include "prmem.h"
#include "prlog.h"
#include "prenv.h"
#include "nsTime.h"
#include "ImageLogging.h"

#include "gfxContext.h"


#if defined(PR_LOGGING)
static PRLogModuleInfo *gCompressedImageAccountingLog = PR_NewLogModule ("CompressedImageAccounting");
#else
#define gCompressedImageAccountingLog
#endif
















#define LOG_CONTAINER_ERROR                      \
  PR_BEGIN_MACRO                                 \
  PR_LOG (gImgLog, PR_LOG_ERROR,                 \
          ("ImgContainer: [this=%p] Error "      \
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

NS_IMPL_ISUPPORTS4(imgContainer, imgIContainer, nsITimerCallback, nsIProperties,
                   nsISupportsWeakReference)


imgContainer::imgContainer() :
  mSize(0,0),
  mHasSize(PR_FALSE),
  mAnim(nsnull),
  mAnimationMode(kNormalAnimMode),
  mLoopCount(-1),
  mObserver(nsnull),
  mDecodeOnDraw(PR_FALSE),
  mMultipart(PR_FALSE),
  mInitialized(PR_FALSE),
  mDiscardable(PR_FALSE),
  mLockCount(0),
  mDiscardTimer(nsnull),
  mHasSourceData(PR_FALSE),
  mDecoded(PR_FALSE),
  mDecoder(nsnull),
  mWorker(nsnull),
  mBytesDecoded(0),
  mDecoderInput(nsnull),
  mDecoderFlags(imgIDecoder::DECODER_FLAG_NONE),
  mWorkerPending(PR_FALSE),
  mInDecoder(PR_FALSE),
  mError(PR_FALSE)
{
  
  num_containers++;
}


imgContainer::~imgContainer()
{
  if (mAnim)
    delete mAnim;

  for (unsigned int i = 0; i < mFrames.Length(); ++i)
    delete mFrames[i];

  
  if (mDiscardable) {
    num_discardable_containers--;
    discardable_source_bytes -= mSourceData.Length();

    PR_LOG (gCompressedImageAccountingLog, PR_LOG_DEBUG,
            ("CompressedImageAccounting: destroying imgContainer %p.  "
             "Total Containers: %d, Discardable containers: %d, "
             "Total source bytes: %lld, Source bytes for discardable containers %lld",
             this,
             num_containers,
             num_discardable_containers,
             total_source_bytes,
             discardable_source_bytes));
  }

  if (mDiscardTimer) {
    mDiscardTimer->Cancel();
    mDiscardTimer = nsnull;
  }

  
  if (mDecoder) {
    nsresult rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    if (NS_FAILED(rv))
      NS_WARNING("Failed to shut down decoder in destructor!");
  }

  
  num_containers--;
  total_source_bytes -= mSourceData.Length();
}




NS_IMETHODIMP imgContainer::Init(imgIDecoderObserver *aObserver,
                                 const char* aMimeType,
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
  mDiscardable = aFlags & INIT_FLAG_DISCARDABLE;
  mDecodeOnDraw = aFlags & INIT_FLAG_DECODE_ON_DRAW;;
  mMultipart = aFlags & INIT_FLAG_MULTIPART;

  
  if (mDiscardable) {
    num_discardable_containers++;
    discardable_source_bytes += mSourceData.Length();
  }

  
  
  
  if (mSourceDataMimeType.Length() == 0) {
    mInitialized = PR_TRUE;
    return NS_OK;
  }

  
  
  
  
  PRUint32 dFlags = imgIDecoder::DECODER_FLAG_NONE;
  if (mDecodeOnDraw)
    dFlags |= imgIDecoder::DECODER_FLAG_HEADERONLY;

  
  nsresult rv = InitDecoder(dFlags);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  mInitialized = PR_TRUE;

  return NS_OK;
}





NS_IMETHODIMP imgContainer::ExtractFrame(PRUint32 aWhichFrame,
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

  
  nsRefPtr<imgContainer> img(new imgContainer());
  NS_ENSURE_TRUE(img, NS_ERROR_OUT_OF_MEMORY);

  
  
  
  img->Init(nsnull, "", INIT_FLAG_NONE);
  img->SetSize(aRegion.width, aRegion.height);
  img->mDecoded = PR_TRUE; 

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  PRUint32 frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetImgFrame(frameIndex);
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

  *_retval = img.forget().get();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetWidth(PRInt32 *aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  if (mError)
    return NS_ERROR_FAILURE;

  *aWidth = mSize.width;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetHeight(PRInt32 *aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  if (mError)
    return NS_ERROR_FAILURE;

  *aHeight = mSize.height;
  return NS_OK;
}

imgFrame *imgContainer::GetImgFrame(PRUint32 framenum)
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

PRUint32 imgContainer::GetCurrentImgFrameIndex() const
{
  if (mAnim)
    return mAnim->currentAnimationFrameIndex;

  return 0;
}

imgFrame *imgContainer::GetCurrentImgFrame()
{
  return GetImgFrame(GetCurrentImgFrameIndex());
}



NS_IMETHODIMP imgContainer::GetCurrentFrameIsOpaque(PRBool *aIsOpaque)
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
    *aIsOpaque = *aIsOpaque && (framerect != nsIntRect(0, 0, mSize.width, mSize.height));
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrameRect(nsIntRect &aRect)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  imgFrame *curframe = GetCurrentImgFrame();

  
  if (curframe)
    aRect = curframe->GetRect();

  
  
  
  
  
  else {
    aRect.MoveTo(0, 0);
    aRect.SizeTo(0, 0);
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetCurrentFrameIndex(PRUint32 *aCurrentFrameIdx)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aCurrentFrameIdx);
  
  *aCurrentFrameIdx = GetCurrentImgFrameIndex();

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetNumFrames(PRUint32 *aNumFrames)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aNumFrames);

  *aNumFrames = mFrames.Length();
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetAnimated(PRBool *aAnimated)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aAnimated);

  *aAnimated = (mAnim != nsnull);
  
  return NS_OK;
}





NS_IMETHODIMP imgContainer::CopyFrame(PRUint32 aWhichFrame,
                                      PRUint32 aFlags,
                                      gfxImageSurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  nsresult rv;

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  NS_ENSURE_ARG_POINTER(_retval);

  
  
  
  PRUint32 frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetImgFrame(frameIndex);
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




NS_IMETHODIMP imgContainer::GetFrame(PRUint32 aWhichFrame,
                                     PRUint32 aFlags,
                                     gfxASurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  PRUint32 frameIndex = (aWhichFrame == FRAME_FIRST) ?
                          0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetImgFrame(frameIndex);
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



NS_IMETHODIMP imgContainer::GetDataSize(PRUint32 *_retval)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(_retval);

  
  *_retval = 0;

  
  *_retval += mSourceData.Length();
  NS_ABORT_IF_FALSE(StoringSourceData() || (*_retval == 0),
                    "Non-zero source data size when we aren't storing it?");

  
  for (PRUint32 i = 0; i < mFrames.Length(); ++i) {
    imgFrame *frame = mFrames.SafeElementAt(i, nsnull);
    NS_ABORT_IF_FALSE(frame, "Null frame in frame array!");
    *_retval += frame->GetImageDataLength();
  }

  return NS_OK;
}

nsresult imgContainer::InternalAddFrameHelper(PRUint32 framenum, imgFrame *aFrame,
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

  mFrames.InsertElementAt(framenum, frame.forget());

  return NS_OK;
}
                                  
nsresult imgContainer::InternalAddFrame(PRUint32 framenum,
                                        PRInt32 aX, PRInt32 aY,
                                        PRInt32 aWidth, PRInt32 aHeight,
                                        gfxASurface::gfxImageFormat aFormat,
                                        PRUint8 aPaletteDepth,
                                        PRUint8 **imageData,
                                        PRUint32 *imageLength,
                                        PRUint32 **paletteData,
                                        PRUint32 *paletteLength)
{
  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(new imgFrame());
  NS_ENSURE_TRUE(frame, NS_ERROR_OUT_OF_MEMORY);

  nsresult rv = frame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  if (mFrames.Length() == 0) {
    return InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength, 
                                  paletteData, paletteLength);
  }

  if (mFrames.Length() == 1) {
    
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    
    
    PRInt32 frameDisposalMethod = mFrames[0]->GetFrameDisposalMethod();
    if (frameDisposalMethod == imgIContainer::kDisposeClear ||
        frameDisposalMethod == imgIContainer::kDisposeRestorePrevious)
      mAnim->firstFrameRefreshArea = mFrames[0]->GetRect();
  }

  
  
  
  nsIntRect frameRect = frame->GetRect();
  mAnim->firstFrameRefreshArea.UnionRect(mAnim->firstFrameRefreshArea, 
                                         frameRect);
  
  rv = InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength,
                              paletteData, paletteLength);
  
  
  
  
  if (mFrames.Length() == 2)
    StartAnimation();
  
  return rv;
}


NS_IMETHODIMP imgContainer::AppendFrame(PRInt32 aX, PRInt32 aY, PRInt32 aWidth,
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


NS_IMETHODIMP imgContainer::AppendPalettedFrame(PRInt32 aX, PRInt32 aY,
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


NS_IMETHODIMP imgContainer::SetSize(PRInt32 aWidth, PRInt32 aHeight)
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

    DoError();
    return NS_ERROR_UNEXPECTED;
  }

  
  mSize.SizeTo(aWidth, aHeight);
  mHasSize = PR_TRUE;

  return NS_OK;
}







NS_IMETHODIMP imgContainer::EnsureCleanFrame(PRUint32 aFrameNum, PRInt32 aX, PRInt32 aY,
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
  if (rect.x != aX || rect.y != aY || rect.width != aWidth || rect.height != aHeight ||
      frame->GetFormat() != aFormat) {
    delete frame;
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                             0, imageData, imageLength,
                             nsnull, 
                             nsnull);
  }

  
  frame->GetImageData(imageData, imageLength);

  return NS_OK;
}




NS_IMETHODIMP imgContainer::FrameUpdated(PRUint32 aFrameNum, nsIntRect &aUpdatedRect)
{
  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling FrameUpdated on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->ImageUpdated(aUpdatedRect);

  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetFrameDisposalMethod(PRUint32 aFrameNum, PRInt32 aDisposalMethod)
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



NS_IMETHODIMP imgContainer::SetFrameTimeout(PRUint32 aFrameNum, PRInt32 aTimeout)
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



NS_IMETHODIMP imgContainer::SetFrameBlendMethod(PRUint32 aFrameNum, PRInt32 aBlendMethod)
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




NS_IMETHODIMP imgContainer::SetFrameHasNoAlpha(PRUint32 aFrameNum)
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



NS_IMETHODIMP imgContainer::EndFrameDecode(PRUint32 aFrameNum)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  if (mAnim)
    mAnim->currentDecodingFrameIndex = aFrameNum;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::DecodingComplete(void)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  
  mDecoded = PR_TRUE;
  if (mAnim)
    mAnim->doneDecoding = PR_TRUE;

  nsresult rv;

  
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(!mDiscardTimer,
                      "We shouldn't have been discardable before this");
    rv = ResetDiscardTimer();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  
  if ((mFrames.Length() == 1) && !mMultipart) {
    rv = mFrames[0]->Optimize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetAnimationMode(PRUint16 *aAnimationMode)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aAnimationMode);
  
  *aAnimationMode = mAnimationMode;
  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetAnimationMode(PRUint16 aAnimationMode)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ASSERTION(aAnimationMode == imgIContainer::kNormalAnimMode ||
               aAnimationMode == imgIContainer::kDontAnimMode ||
               aAnimationMode == imgIContainer::kLoopOnceAnimMode,
               "Wrong Animation Mode is being set!");
  
  switch (mAnimationMode = aAnimationMode) {
    case kDontAnimMode:
      StopAnimation();
      break;
    case kNormalAnimMode:
      if (mLoopCount != 0 || 
          (mAnim && (mAnim->currentAnimationFrameIndex + 1 < mFrames.Length())))
        StartAnimation();
      break;
    case kLoopOnceAnimMode:
      if (mAnim && (mAnim->currentAnimationFrameIndex + 1 < mFrames.Length()))
        StartAnimation();
      break;
  }
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::StartAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnimationMode == kDontAnimMode || 
      (mAnim && (mAnim->timer || mAnim->animating)))
    return NS_OK;
  
  if (mFrames.Length() > 1) {
    if (!ensureAnimExists())
      return NS_ERROR_OUT_OF_MEMORY;
    
    
    
    PRInt32 timeout = 100;
    imgFrame *currentFrame = GetCurrentImgFrame();
    if (currentFrame) {
      timeout = currentFrame->GetTimeout();
      if (timeout <= 0) 
        return NS_OK;
    }
    
    mAnim->timer = do_CreateInstance("@mozilla.org/timer;1");
    NS_ENSURE_TRUE(mAnim->timer, NS_ERROR_OUT_OF_MEMORY);
    
    
    mAnim->animating = PR_TRUE;
    mAnim->timer->InitWithCallback(static_cast<nsITimerCallback*>(this),
                                   timeout, nsITimer::TYPE_REPEATING_SLACK);
  }
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::StopAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnim) {
    mAnim->animating = PR_FALSE;

    if (!mAnim->timer)
      return NS_OK;

    mAnim->timer->Cancel();
    mAnim->timer = nsnull;
  }

  return NS_OK;
}



NS_IMETHODIMP imgContainer::ResetAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnimationMode == kDontAnimMode || 
      !mAnim || mAnim->currentAnimationFrameIndex == 0)
    return NS_OK;

  PRBool oldAnimating = mAnim->animating;

  if (mAnim->animating) {
    nsresult rv = StopAnimation();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;

  
  

  
  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (oldAnimating && observer)
    observer->FrameChanged(this, &(mAnim->firstFrameRefreshArea));

  if (oldAnimating)
    return StartAnimation();
  return NS_OK;
}



NS_IMETHODIMP imgContainer::GetLoopCount(PRInt32 *aLoopCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aLoopCount);
  
  *aLoopCount = mLoopCount;
  
  return NS_OK;
}



NS_IMETHODIMP imgContainer::SetLoopCount(PRInt32 aLoopCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  
  
  mLoopCount = aLoopCount;

  return NS_OK;
}



NS_IMETHODIMP imgContainer::AddSourceData(const char *aBuffer, PRUint32 aCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aBuffer);
  nsresult rv = NS_OK;

  
  NS_ABORT_IF_FALSE(mInitialized, "Calling AddSourceData() on uninitialized "
                                  "imgContainer!");

  
  NS_ABORT_IF_FALSE(!mHasSourceData, "Calling AddSourceData() after calling "
                                     "sourceDataComplete()!");

  
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  
  if (!StoringSourceData()) {
    rv = WriteToDecoder(aBuffer, aCount);
    CONTAINER_ENSURE_SUCCESS(rv);
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
          ("CompressedImageAccounting: Added compressed data to imgContainer %p (%s). "
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



NS_IMETHODIMP imgContainer::SourceDataComplete()
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
            ("CompressedImageAccounting: imgContainer::SourceDataComplete() - data "
             "is done for container %p (%s) - header %p is 0x%s (length %d)",
             this,
             mSourceDataMimeType.get(),
             mSourceData.Elements(),
             buf,
             mSourceData.Length()));
  }

  
  if (CanDiscard()) {
    nsresult rv = ResetDiscardTimer();
    CONTAINER_ENSURE_SUCCESS(rv);
  }
  return NS_OK;
}



NS_IMETHODIMP imgContainer::NewSourceData()
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

  
  
  rv = InitDecoder(imgIDecoder::DECODER_FLAG_NONE);
  CONTAINER_ENSURE_SUCCESS(rv);

  return NS_OK;
}



NS_IMETHODIMP imgContainer::Notify(nsITimer *timer)
{
  
  
  NS_ENSURE_TRUE(mAnim, NS_ERROR_UNEXPECTED);
  NS_ASSERTION(mAnim->timer == timer,
               "imgContainer::Notify() called with incorrect timer");

  if (!mAnim->animating || !mAnim->timer)
    return NS_OK;

  nsCOMPtr<imgIContainerObserver> observer(do_QueryReferent(mObserver));
  if (!observer) {
    
    StopAnimation();
    return NS_OK;
  }

  if (mFrames.Length() == 0)
    return NS_OK;
  
  imgFrame *nextFrame = nsnull;
  PRInt32 previousFrameIndex = mAnim->currentAnimationFrameIndex;
  PRUint32 nextFrameIndex = mAnim->currentAnimationFrameIndex + 1;
  PRInt32 timeout = 0;

  
  
  
  
  if (mAnim->doneDecoding || 
      (nextFrameIndex < mAnim->currentDecodingFrameIndex)) {
    if (mFrames.Length() == nextFrameIndex) {
      

      
      if (mAnimationMode == kLoopOnceAnimMode || mLoopCount == 0) {
        StopAnimation();
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

  } else if (nextFrameIndex == mAnim->currentDecodingFrameIndex) {
    
    
    mAnim->timer->SetDelay(100);
    return NS_OK;
  } else { 
    
    
    NS_WARNING("imgContainer::Notify()  Frame is passed decoded frame");
    nextFrameIndex = mAnim->currentDecodingFrameIndex;
    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      mAnim->timer->SetDelay(100);
      return NS_OK;
    }
    timeout = nextFrame->GetTimeout();
  }

  if (timeout > 0)
    mAnim->timer->SetDelay(timeout);
  else
    StopAnimation();

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
      
      NS_WARNING("imgContainer::Notify(): Composing Frame Failed\n");
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      return NS_OK;
    }
  }
  
  mAnim->currentAnimationFrameIndex = nextFrameIndex;
  
  observer->FrameChanged(this, &dirtyRect);
  
  return NS_OK;
}




nsresult imgContainer::DoComposite(imgFrame** aFrameToUse,
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
  if (prevFrameDisposalMethod == imgIContainer::kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = imgIContainer::kDisposeClear;

  nsIntRect prevFrameRect = aPrevFrame->GetRect();
  PRBool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                            prevFrameRect.width == mSize.width &&
                            prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && 
      (prevFrameDisposalMethod == imgIContainer::kDisposeClear))
    prevFrameDisposalMethod = imgIContainer::kDisposeClearAll;

  PRInt32 nextFrameDisposalMethod = aNextFrame->GetFrameDisposalMethod();
  nsIntRect nextFrameRect = aNextFrame->GetRect();
  PRBool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                            nextFrameRect.width == mSize.width &&
                            nextFrameRect.height == mSize.height);

  if (!aNextFrame->GetIsPaletted()) {
    
    
    if (prevFrameDisposalMethod == imgIContainer::kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  
    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious) &&
        !aNextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case imgIContainer::kDisposeNotSpecified:
    case imgIContainer::kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case imgIContainer::kDisposeClearAll:
      
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;

    case imgIContainer::kDisposeClear:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case imgIContainer::kDisposeRestorePrevious:
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
    NS_ENSURE_SUCCESS(rv, rv);
    needToBlankComposite = PR_TRUE;
  } else if (aNextFrameIndex == 1) {
    
    needToBlankComposite = PR_TRUE;
  }

  
  PRBool doDisposal = PR_TRUE;
  if (!aNextFrame->GetHasAlpha()) {
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
      case imgIContainer::kDisposeClear:
        if (needToBlankComposite) {
          
          
          ClearFrame(mAnim->compositingFrame);
        } else {
          
          ClearFrame(mAnim->compositingFrame, prevFrameRect);
        }
        break;
  
      case imgIContainer::kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame);
        break;
  
      case imgIContainer::kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);
  
          
          if (nextFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious)
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

  
  
  
  if ((nextFrameDisposalMethod == imgIContainer::kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != imgIContainer::kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame = new imgFrame();
      if (!mAnim->compositingPrevFrame) {
        NS_WARNING("Failed to init compositingFrame!\n");
        return NS_ERROR_OUT_OF_MEMORY;
      }
      nsresult rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                                      gfxASurface::ImageFormatARGB32);
      NS_ENSURE_SUCCESS(rv, rv);
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
      !aNextFrame->GetIsPaletted()) {
    
    
    
    
    
    if (CopyFrameImage(mAnim->compositingFrame, aNextFrame)) {
      aPrevFrame->SetFrameDisposalMethod(imgIContainer::kDisposeClearAll);
      mAnim->lastCompositedFrameIndex = -1;
      *aFrameToUse = aNextFrame;
      return NS_OK;
    }
  }

  mAnim->lastCompositedFrameIndex = aNextFrameIndex;
  *aFrameToUse = mAnim->compositingFrame;

  return NS_OK;
}



void imgContainer::ClearFrame(imgFrame *aFrame)
{
  if (!aFrame)
    return;

  aFrame->LockImageData();

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Paint();

  aFrame->UnlockImageData();
}


void imgContainer::ClearFrame(imgFrame *aFrame, nsIntRect &aRect)
{
  if (!aFrame || aRect.width <= 0 || aRect.height <= 0)
    return;

  aFrame->LockImageData();

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Rectangle(gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
  ctx.Fill();

  aFrame->UnlockImageData();
}





PRBool imgContainer::CopyFrameImage(imgFrame *aSrcFrame,
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








nsresult imgContainer::DrawFrameTo(imgFrame *aSrc,
                                   imgFrame *aDst, 
                                   nsIntRect& aSrcRect)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aDst);

  nsIntRect dstRect = aDst->GetRect();

  
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("imgContainer::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  
  if ((aSrcRect.x > dstRect.width) || (aSrcRect.y > dstRect.height)) {
    return NS_OK;
  }

  if (aSrc->GetIsPaletted()) {
    
    PRInt32 width = PR_MIN(aSrcRect.width, dstRect.width - aSrcRect.x);
    PRInt32 height = PR_MIN(aSrcRect.height, dstRect.height - aSrcRect.y);

    
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= dstRect.width) &&
                 (aSrcRect.y + height <= dstRect.height),
                "imgContainer::DrawFrameTo: Invalid aSrcRect");

    
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "imgContainer::DrawFrameTo: source must be smaller than dest");

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
  if (blendMethod == imgIContainer::kBlendSource) {
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



NS_IMETHODIMP imgContainer::Get(const char *prop, const nsIID & iid, void * *result)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Get(prop, iid, result);
}

NS_IMETHODIMP imgContainer::Set(const char *prop, nsISupports *value)
{
  if (!mProperties)
    mProperties = do_CreateInstance("@mozilla.org/properties;1");
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;
  return mProperties->Set(prop, value);
}

NS_IMETHODIMP imgContainer::Has(const char *prop, PRBool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mProperties) {
    *_retval = PR_FALSE;
    return NS_OK;
  }
  return mProperties->Has(prop, _retval);
}

NS_IMETHODIMP imgContainer::Undefine(const char *prop)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Undefine(prop);
}

NS_IMETHODIMP imgContainer::GetKeys(PRUint32 *count, char ***keys)
{
  if (!mProperties) {
    *count = 0;
    *keys = nsnull;
    return NS_OK;
  }
  return mProperties->GetKeys(count, keys);
}

static int
get_discard_timer_ms (void)
{
  
  return 15000; 
}

void
imgContainer::sDiscardTimerCallback(nsITimer *aTimer, void *aClosure)
{
  
  imgContainer *self = (imgContainer *) aClosure;
  NS_ABORT_IF_FALSE(aTimer == self->mDiscardTimer, 
                    "imgContainer::DiscardTimerCallback() got a callback "
                    "for an unknown timer");
  self->mDiscardTimer = nsnull;

  
  NS_ABORT_IF_FALSE(self->CanDiscard(), "Hit discard callback but can't discard!");

  
  NS_ABORT_IF_FALSE(!self->mDecoder, "Discard callback fired with open decoder!");

  
  
  NS_ABORT_IF_FALSE(!self->mAnim, "Discard callback fired for animated image!");

  
  int old_frame_count = self->mFrames.Length();

  
  for (int i = 0; i < old_frame_count; ++i)
    delete self->mFrames[i];
  self->mFrames.Clear();

  
  self->mDecoded = PR_FALSE;

  
  nsCOMPtr<imgIDecoderObserver> observer(do_QueryReferent(self->mObserver));
  if (observer)
    observer->OnDiscard(nsnull);

  
  PR_LOG(gCompressedImageAccountingLog, PR_LOG_DEBUG,
         ("CompressedImageAccounting: discarded uncompressed image "
          "data from imgContainer %p (%s) - %d frames (cached count: %d); "
          "Total Containers: %d, Discardable containers: %d, "
          "Total source bytes: %lld, Source bytes for discardable containers %lld",
          self,
          self->mSourceDataMimeType.get(),
          old_frame_count,
          self->mFrames.Length(),
          num_containers,
          num_discardable_containers,
          total_source_bytes,
          discardable_source_bytes));
}

nsresult
imgContainer::ResetDiscardTimer()
{
  
  NS_ABORT_IF_FALSE(CanDiscard(), "Calling ResetDiscardTimer but can't discard!");

  
  NS_ABORT_IF_FALSE(!mAnim, "Trying to reset discard timer on animated image!");

  
  if (mDiscardTimer) {
    nsresult rv = mDiscardTimer->Cancel();
    CONTAINER_ENSURE_SUCCESS(rv);
    mDiscardTimer = nsnull;
  }

  
  mDiscardTimer = do_CreateInstance("@mozilla.org/timer;1");
  CONTAINER_ENSURE_TRUE(mDiscardTimer, NS_ERROR_OUT_OF_MEMORY);

  
  return mDiscardTimer->InitWithFuncCallback(sDiscardTimerCallback,
                                             (void *) this,
                                             get_discard_timer_ms (),
                                             nsITimer::TYPE_ONE_SHOT);
}


PRBool
imgContainer::CanDiscard() {
  return (DiscardingEnabled() && 
          mDiscardable &&        
          (mLockCount == 0) &&   
          mHasSourceData &&      
          mDecoded);             
}



PRBool
imgContainer::StoringSourceData() {
  return (mDecodeOnDraw || mDiscardable);
}




nsresult
imgContainer::InitDecoder (PRUint32 dFlags)
{
  
  NS_ABORT_IF_FALSE(!mDecoder, "Calling InitDecoder() while already decoding!");
  
  
  NS_ABORT_IF_FALSE(!mDecoded, "Calling InitDecoder() but already decoded!");

  
  NS_ABORT_IF_FALSE(!mDiscardTimer, "Discard Timer active in InitDecoder()!");

  
  nsCAutoString decoderCID(NS_LITERAL_CSTRING("@mozilla.org/image/decoder;3?type=") +
                                              mSourceDataMimeType);
  mDecoder = do_CreateInstance(decoderCID.get());
  CONTAINER_ENSURE_TRUE(mDecoder, NS_IMAGELIB_ERROR_NO_DECODER);

  
  mDecoderFlags = dFlags;

  
  nsCOMPtr<imgIDecoderObserver> observer(do_QueryReferent(mObserver));
  nsresult result = mDecoder->Init(this, observer, dFlags);
  CONTAINER_ENSURE_SUCCESS(result);

  
  
  
  
  
  mDecoderInput = do_CreateInstance("@mozilla.org/io/string-input-stream;1");
  CONTAINER_ENSURE_TRUE(mDecoderInput, NS_ERROR_OUT_OF_MEMORY);

  
  mWorker = new imgDecodeWorker(this);
  CONTAINER_ENSURE_TRUE(mWorker, NS_ERROR_OUT_OF_MEMORY);

  return NS_OK;
}









nsresult
imgContainer::ShutdownDecoder(eShutdownIntent aIntent)
{
  
  NS_ABORT_IF_FALSE((aIntent >= 0) || (aIntent < eShutdownIntent_AllCount),
                    "Invalid shutdown intent");

  
  NS_ABORT_IF_FALSE(mDecoder, "Calling ShutdownDecoder() with no active decoder!");

  nsresult rv;

  
  if ((aIntent == eShutdownIntent_Done) &&
      !(mDecoderFlags && imgIDecoder::DECODER_FLAG_HEADERONLY)) {
    mInDecoder = PR_TRUE;
    rv = mDecoder->Flush();
    mInDecoder = PR_FALSE;

    
    
    if (NS_FAILED(rv)) {
      DoError();
      return rv;
    }
  }

  
  mInDecoder = PR_TRUE;
  PRUint32 closeFlags = (aIntent == eShutdownIntent_Error)
                          ? (PRUint32) imgIDecoder::CLOSE_FLAG_DONTNOTIFY
                          : 0;
  rv = mDecoder->Close(closeFlags);
  mInDecoder = PR_FALSE;

  
  
  mDecoder = nsnull;
  if (NS_FAILED(rv)) {
    DoError();
    return rv;
  }

  
  mDecoderInput = nsnull;

  
  mWorker = nsnull;

  
  
  PRBool failed = PR_FALSE;
  if ((mDecoderFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) && !mHasSize)
    failed = PR_TRUE;
  if (!(mDecoderFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) && !mDecoded)
    failed = PR_TRUE;
  if ((aIntent == eShutdownIntent_Done) && failed) {
    DoError();
    return NS_ERROR_FAILURE;
  }

  
  mDecoderFlags = imgIDecoder::DECODER_FLAG_NONE;

  
  mBytesDecoded = 0;

  return NS_OK;
}



nsresult
imgContainer::WriteToDecoder(const char *aBuffer, PRUint32 aCount)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "Trying to write to null decoder!");

  
  nsresult rv = mDecoderInput->ShareData(aBuffer, aCount);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  mInDecoder = PR_TRUE;
  rv = mDecoder->WriteFrom(mDecoderInput, aCount);
  mInDecoder = PR_FALSE;
  CONTAINER_ENSURE_SUCCESS(rv);

  
  
  mBytesDecoded += aCount;

  return NS_OK;
}







nsresult
imgContainer::WantDecodedFrames()
{
  nsresult rv;

  
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(mDiscardTimer, "Decoded and discardable but timer not set!");
    rv = ResetDiscardTimer();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  return RequestDecode();
}



NS_IMETHODIMP
imgContainer::RequestDecode()
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (!StoringSourceData())
    return NS_OK;

  
  if (mDecoded)
    return NS_OK;

  
  if (mInDecoder) {
    nsRefPtr<imgDecodeRequestor> requestor = new imgDecodeRequestor(this);
    if (!requestor)
      return NS_ERROR_OUT_OF_MEMORY;
    return NS_DispatchToCurrentThread(requestor);
  }


  
  if (mDecoder && (mDecoderFlags & imgIDecoder::DECODER_FLAG_HEADERONLY)) {
    rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder(imgIDecoder::DECODER_FLAG_NONE);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (mWorkerPending)
    return NS_OK;

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;

  
  
  
  return mWorker->Dispatch();
}


nsresult
imgContainer::SyncDecode()
{
  nsresult rv;

  
  if (mDecoded)
    return NS_OK;

  
  if (!StoringSourceData())
    return NS_OK;

  
  
  
  
  
  NS_ABORT_IF_FALSE(!mInDecoder, "Yikes, forcing sync in reentrant call!");
  if (mInDecoder)
    return NS_ERROR_FAILURE;

  
  if (mDecoder && (mDecoderFlags & imgIDecoder::DECODER_FLAG_HEADERONLY)) {
    rv = ShutdownDecoder(eShutdownIntent_Interrupted);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder(imgIDecoder::DECODER_FLAG_NONE);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                      mSourceData.Length() - mBytesDecoded);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  if (IsDecodeFinished()) {
    rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  return NS_OK;
}




 
NS_IMETHODIMP imgContainer::Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter, 
                                 gfxMatrix &aUserSpaceToImageSpace, gfxRect &aFill,
                                 nsIntRect &aSubimage, PRUint32 aFlags)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aContext);

  
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  imgFrame *frame = GetCurrentImgFrame();
  if (!frame) {
    NS_ABORT_IF_FALSE(!mDecoded, "Decoded but frame not available?");
    return NS_OK; 
  }

  nsIntRect framerect = frame->GetRect();
  nsIntMargin padding(framerect.x, framerect.y, 
                      mSize.width - framerect.XMost(),
                      mSize.height - framerect.YMost());

  frame->Draw(aContext, aFilter, aUserSpaceToImageSpace, aFill, padding, aSubimage);

  return NS_OK;
}



NS_IMETHODIMP
imgContainer::LockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mDiscardTimer) {
    mDiscardTimer->Cancel();
    mDiscardTimer = nsnull; 
                            
  }

  
  mLockCount++;

  return NS_OK;
}



NS_IMETHODIMP
imgContainer::UnlockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mLockCount > 0,
                    "Calling UnlockImage with mLockCount == 0!");
  if (mLockCount == 0)
    return NS_ERROR_ABORT;

  
  NS_ABORT_IF_FALSE(!mDiscardTimer, "Locked, but discard timer set!");

  
  mLockCount--;

  
  if (CanDiscard()) {
    nsresult rv = ResetDiscardTimer();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  return NS_OK;
}


nsresult
imgContainer::DecodeSomeData (PRUint32 aMaxBytes)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "trying to decode without decoder!");

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;


  
  PRUint32 bytesToDecode = PR_MIN(aMaxBytes,
                                  mSourceData.Length() - mBytesDecoded);
  nsresult rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                               bytesToDecode);

  return rv;
}



PRBool imgContainer::IsDecodeFinished()
{
  
  PRBool decodeFinished = PR_FALSE;

  
  
  NS_ABORT_IF_FALSE(StoringSourceData(),
                    "just shut down on SourceDataComplete!");

  
  if (mDecoderFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) {
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



void imgContainer::DoError()
{
  
  if (mError)
    return;

  
  if (mDecoder) {

    
    nsCOMPtr<imgIDecoderObserver> observer = do_QueryReferent(mObserver);
    if (observer) {
      observer->OnStopContainer(nsnull, this);
      observer->OnStopDecode(nsnull, NS_ERROR_FAILURE, nsnull);
    }

    
    
    (void) ShutdownDecoder(eShutdownIntent_Error);
  }

  
  mError = PR_TRUE;

  
  LOG_CONTAINER_ERROR;
}


#define DECODE_BYTES_AT_A_TIME 4096
#define MAX_USEC_BEFORE_YIELD (1000 * 5)



NS_IMETHODIMP imgDecodeWorker::Run()
{
  nsresult rv;

  
  nsCOMPtr<nsIRunnable> kungFuDeathGrip(this);

  
  nsCOMPtr<imgIContainer> iContainer(do_QueryReferent(mContainer));
  if (!iContainer)
    return NS_OK;
  imgContainer* container = static_cast<imgContainer*>(iContainer.get());

  NS_ABORT_IF_FALSE(container->mInitialized,
                    "Worker active for uninitialized container!");

  
  container->mWorkerPending = PR_FALSE;

  
  
  
  if (container->mError)
    return NS_OK;

  
  
  if (!container->mDecoder)
    return NS_OK;

  
  
  
  PRUint32 maxBytes =
    (container->mDecoderFlags & imgIDecoder::DECODER_FLAG_HEADERONLY)
    ? container->mSourceData.Length() : DECODE_BYTES_AT_A_TIME;

  
  PRBool haveMoreData = PR_TRUE;
  nsTime deadline(PR_Now() + MAX_USEC_BEFORE_YIELD);

  
  
  
  
  while (haveMoreData && !container->IsDecodeFinished() &&
         (nsTime(PR_Now()) < deadline)) {

    
    rv = container->DecodeSomeData(maxBytes);
    if (NS_FAILED(rv)) {
      container->DoError();
      return rv;
    }

    
    haveMoreData =
      container->mSourceData.Length() > container->mBytesDecoded;
  }

  
  if (container->IsDecodeFinished()) {
    rv = container->ShutdownDecoder(imgContainer::eShutdownIntent_Done);
    if (NS_FAILED(rv)) {
      container->DoError();
      return rv;
    }
  }

  
  
  
  if (!container->IsDecodeFinished() && haveMoreData)
    return this->Dispatch();

  
  return NS_OK;
}


NS_METHOD imgDecodeWorker::Dispatch()
{
  
  nsCOMPtr<imgIContainer> iContainer(do_QueryReferent(mContainer));
  if (!iContainer)
    return NS_OK;
  imgContainer* container = static_cast<imgContainer*>(iContainer.get());

  
  NS_ABORT_IF_FALSE(!container->mWorkerPending,
                    "Trying to queue up worker with one already pending!");

  
  container->mWorkerPending = PR_TRUE;

  
  return NS_DispatchToCurrentThread(this);
}




NS_METHOD
imgContainer::WriteToContainer(nsIInputStream* in, void* closure,
                               const char* fromRawSegment, PRUint32 toOffset,
                               PRUint32 count, PRUint32 *writeCount)
{
  
  imgIContainer *container = static_cast<imgIContainer*>(closure);

  
  
  
  (void) container->AddSourceData(fromRawSegment, count);

  
  *writeCount = count;

  return NS_OK;
}
