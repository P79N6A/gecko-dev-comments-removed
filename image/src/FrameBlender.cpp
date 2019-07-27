




#include "FrameBlender.h"

#include "mozilla/MemoryReporting.h"
#include "MainThreadUtils.h"

#include "pixman.h"

namespace mozilla {

using namespace gfx;

namespace image {

FrameBlender::FrameBlender(FrameSequence* aSequenceToUse )
 : mFrames(aSequenceToUse)
 , mAnim(nullptr)
 , mLoopCount(-1)
{
  if (!mFrames) {
    mFrames = new FrameSequence();
  }
}

FrameBlender::~FrameBlender()
{
  delete mAnim;
}

already_AddRefed<FrameSequence>
FrameBlender::GetFrameSequence()
{
  nsRefPtr<FrameSequence> seq(mFrames);
  return seq.forget();
}

already_AddRefed<imgFrame>
FrameBlender::GetFrame(uint32_t framenum) const
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames->GetFrame(0).GetFrame();
  }
  if (mAnim->lastCompositedFrameIndex == int32_t(framenum)) {
    return mAnim->compositingFrame.GetFrame();
  }
  return mFrames->GetFrame(framenum).GetFrame();
}

already_AddRefed<imgFrame>
FrameBlender::RawGetFrame(uint32_t framenum) const
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames->GetFrame(0).GetFrame();
  }
  return mFrames->GetFrame(framenum).GetFrame();
}

uint32_t
FrameBlender::GetNumFrames() const
{
  return mFrames->GetNumFrames();
}

int32_t
FrameBlender::GetTimeoutForFrame(uint32_t framenum) const
{
  nsRefPtr<imgFrame> frame = RawGetFrame(framenum);
  const int32_t timeout = frame->GetRawTimeout();
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (timeout >= 0 && timeout <= 10 && mLoopCount != 0)
    return 100;
  return timeout;
}

void
FrameBlender::SetLoopCount(int32_t aLoopCount)
{
  mLoopCount = aLoopCount;
}

int32_t
FrameBlender::GetLoopCount() const
{
  return mLoopCount;
}

void
FrameBlender::RemoveFrame(uint32_t framenum)
{
  NS_ABORT_IF_FALSE(framenum < GetNumFrames(), "Deleting invalid frame!");

  mFrames->RemoveFrame(framenum);
}

void
FrameBlender::ClearFrames()
{
  
  mFrames = new FrameSequence();
}

void
FrameBlender::InsertFrame(uint32_t framenum, imgFrame* aFrame)
{
  NS_ABORT_IF_FALSE(framenum <= GetNumFrames(), "Inserting invalid frame!");
  mFrames->InsertFrame(framenum, aFrame);
  if (GetNumFrames() > 1) {
    EnsureAnimExists();
  }
}

already_AddRefed<imgFrame>
FrameBlender::SwapFrame(uint32_t framenum, imgFrame* aFrame)
{
  NS_ABORT_IF_FALSE(framenum < GetNumFrames(), "Swapping invalid frame!");

  nsRefPtr<imgFrame> ret;

  
  if (mAnim && mAnim->lastCompositedFrameIndex == int32_t(framenum)) {
    ret = mAnim->compositingFrame.Forget();
    mAnim->lastCompositedFrameIndex = -1;
    nsRefPtr<imgFrame> toDelete(mFrames->SwapFrame(framenum, aFrame));
  } else {
    ret = mFrames->SwapFrame(framenum, aFrame);
  }

  return ret.forget();
}

void
FrameBlender::EnsureAnimExists()
{
  if (!mAnim) {
    
    mAnim = new Anim();

    
    
    MOZ_ASSERT(GetNumFrames() == 2);
  }
}




bool
FrameBlender::DoBlend(nsIntRect* aDirtyRect,
                      uint32_t aPrevFrameIndex,
                      uint32_t aNextFrameIndex)
{
  if (!aDirtyRect) {
    return false;
  }

  const FrameDataPair& prevFrame = mFrames->GetFrame(aPrevFrameIndex);
  const FrameDataPair& nextFrame = mFrames->GetFrame(aNextFrameIndex);
  if (!prevFrame.HasFrameData() || !nextFrame.HasFrameData()) {
    return false;
  }

  int32_t prevFrameDisposalMethod = prevFrame->GetFrameDisposalMethod();
  if (prevFrameDisposalMethod == FrameBlender::kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = FrameBlender::kDisposeClear;

  nsIntRect prevFrameRect = prevFrame->GetRect();
  bool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                          prevFrameRect.width == mSize.width &&
                          prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame &&
      (prevFrameDisposalMethod == FrameBlender::kDisposeClear))
    prevFrameDisposalMethod = FrameBlender::kDisposeClearAll;

  int32_t nextFrameDisposalMethod = nextFrame->GetFrameDisposalMethod();
  nsIntRect nextFrameRect = nextFrame->GetRect();
  bool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                          nextFrameRect.width == mSize.width &&
                          nextFrameRect.height == mSize.height);

  if (!nextFrame->GetIsPaletted()) {
    
    
    if (prevFrameDisposalMethod == FrameBlender::kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return true;
    }

    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious) &&
        !nextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return true;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case FrameBlender::kDisposeNotSpecified:
    case FrameBlender::kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case FrameBlender::kDisposeClearAll:
      
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;

    case FrameBlender::kDisposeClear:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case FrameBlender::kDisposeRestorePrevious:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  
  
  
  
  
  
  if (mAnim->lastCompositedFrameIndex == int32_t(aNextFrameIndex)) {
    return true;
  }

  bool needToBlankComposite = false;

  
  if (!mAnim->compositingFrame) {
    mAnim->compositingFrame.SetFrame(new imgFrame());
    nsresult rv =
      mAnim->compositingFrame->InitForDecoder(mSize, SurfaceFormat::B8G8R8A8);
    if (NS_FAILED(rv)) {
      mAnim->compositingFrame.SetFrame(nullptr);
      return false;
    }
    mAnim->compositingFrame.LockAndGetData();
    needToBlankComposite = true;
  } else if (int32_t(aNextFrameIndex) != mAnim->lastCompositedFrameIndex+1) {

    
    
    needToBlankComposite = true;
  }

  
  
  
  
  
  
  bool doDisposal = true;
  if (!nextFrame->GetHasAlpha() &&
      nextFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious) {
    if (isFullNextFrame) {
      
      
      doDisposal = false;
      
      needToBlankComposite = false;
    } else {
      if ((prevFrameRect.x >= nextFrameRect.x) &&
          (prevFrameRect.y >= nextFrameRect.y) &&
          (prevFrameRect.x + prevFrameRect.width <= nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <= nextFrameRect.y + nextFrameRect.height)) {
        
        
        doDisposal = false;
      }
    }
  }

  if (doDisposal) {
    
    switch (prevFrameDisposalMethod) {
      case FrameBlender::kDisposeClear:
        if (needToBlankComposite) {
          
          
          ClearFrame(mAnim->compositingFrame.GetFrameData(),
                     mAnim->compositingFrame->GetRect());
        } else {
          
          ClearFrame(mAnim->compositingFrame.GetFrameData(),
                     mAnim->compositingFrame->GetRect(),
                     prevFrameRect);
        }
        break;

      case FrameBlender::kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame.GetFrameData(),
                   mAnim->compositingFrame->GetRect());
        break;

      case FrameBlender::kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame.GetFrameData(),
                         mAnim->compositingPrevFrame->GetRect(),
                         mAnim->compositingFrame.GetFrameData(),
                         mAnim->compositingFrame->GetRect());

          
          if (nextFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious)
            mAnim->compositingPrevFrame.SetFrame(nullptr);
        } else {
          ClearFrame(mAnim->compositingFrame.GetFrameData(),
                     mAnim->compositingFrame->GetRect());
        }
        break;

      default:
        
        
        
        
        
        
        if (mAnim->lastCompositedFrameIndex != int32_t(aNextFrameIndex - 1)) {
          if (isFullPrevFrame && !prevFrame->GetIsPaletted()) {
            
            CopyFrameImage(prevFrame.GetFrameData(),
                           prevFrame->GetRect(),
                           mAnim->compositingFrame.GetFrameData(),
                           mAnim->compositingFrame->GetRect());
          } else {
            if (needToBlankComposite) {
              
              if (prevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame.GetFrameData(),
                           mAnim->compositingFrame->GetRect());
              }
            }
            DrawFrameTo(prevFrame.GetFrameData(), prevFrameRect,
                        prevFrame->PaletteDataLength(),
                        prevFrame->GetHasAlpha(),
                        mAnim->compositingFrame.GetFrameData(),
                        mAnim->compositingFrame->GetRect(),
                        FrameBlendMethod(prevFrame->GetBlendMethod()));
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mAnim->compositingFrame.GetFrameData(),
               mAnim->compositingFrame->GetRect());
  }

  
  
  
  if ((nextFrameDisposalMethod == FrameBlender::kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame.SetFrame(new imgFrame());
      nsresult rv =
        mAnim->compositingPrevFrame->InitForDecoder(mSize,
                                                    SurfaceFormat::B8G8R8A8);
      if (NS_FAILED(rv)) {
        mAnim->compositingPrevFrame.SetFrame(nullptr);
        return false;
      }

      mAnim->compositingPrevFrame.LockAndGetData();
    }

    CopyFrameImage(mAnim->compositingFrame.GetFrameData(),
                   mAnim->compositingFrame->GetRect(),
                   mAnim->compositingPrevFrame.GetFrameData(),
                   mAnim->compositingPrevFrame->GetRect());
  }

  
  DrawFrameTo(nextFrame.GetFrameData(), nextFrameRect,
              nextFrame->PaletteDataLength(),
              nextFrame->GetHasAlpha(),
              mAnim->compositingFrame.GetFrameData(),
              mAnim->compositingFrame->GetRect(),
              FrameBlendMethod(nextFrame->GetBlendMethod()));

  
  
  int32_t timeout = nextFrame->GetRawTimeout();
  mAnim->compositingFrame->SetRawTimeout(timeout);

  
  nsresult rv =
    mAnim->compositingFrame->ImageUpdated(mAnim->compositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return false;
  }

  mAnim->lastCompositedFrameIndex = int32_t(aNextFrameIndex);

  return true;
}



void
FrameBlender::ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect)
{
  if (!aFrameData)
    return;

  memset(aFrameData, 0, aFrameRect.width * aFrameRect.height * 4);
}


void
FrameBlender::ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect, const nsIntRect& aRectToClear)
{
  if (!aFrameData || aFrameRect.width <= 0 || aFrameRect.height <= 0 ||
      aRectToClear.width <= 0 || aRectToClear.height <= 0) {
    return;
  }

  nsIntRect toClear = aFrameRect.Intersect(aRectToClear);
  if (toClear.IsEmpty()) {
    return;
  }

  uint32_t bytesPerRow = aFrameRect.width * 4;
  for (int row = toClear.y; row < toClear.y + toClear.height; ++row) {
    memset(aFrameData + toClear.x * 4 + row * bytesPerRow, 0, toClear.width * 4);
  }
}




bool
FrameBlender::CopyFrameImage(const uint8_t *aDataSrc, const nsIntRect& aRectSrc,
                             uint8_t *aDataDest, const nsIntRect& aRectDest)
{
  uint32_t dataLengthSrc = aRectSrc.width * aRectSrc.height * 4;
  uint32_t dataLengthDest = aRectDest.width * aRectDest.height * 4;

  if (!aDataDest || !aDataSrc || dataLengthSrc != dataLengthDest) {
    return false;
  }

  memcpy(aDataDest, aDataSrc, dataLengthDest);

  return true;
}

nsresult
FrameBlender::DrawFrameTo(const uint8_t *aSrcData, const nsIntRect& aSrcRect,
                          uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                          uint8_t *aDstPixels, const nsIntRect& aDstRect,
                          FrameBlender::FrameBlendMethod aBlendMethod)
{
  NS_ENSURE_ARG_POINTER(aSrcData);
  NS_ENSURE_ARG_POINTER(aDstPixels);

  
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("FrameBlender::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  
  if ((aSrcRect.x > aDstRect.width) || (aSrcRect.y > aDstRect.height)) {
    return NS_OK;
  }

  if (aSrcPaletteLength) {
    
    int32_t width = std::min(aSrcRect.width, aDstRect.width - aSrcRect.x);
    int32_t height = std::min(aSrcRect.height, aDstRect.height - aSrcRect.y);

    
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= aDstRect.width) &&
                 (aSrcRect.y + height <= aDstRect.height),
                "FrameBlender::DrawFrameTo: Invalid aSrcRect");

    
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "FrameBlender::DrawFrameTo: source must be smaller than dest");

    
    const uint8_t *srcPixels = aSrcData + aSrcPaletteLength;
    uint32_t *dstPixels = reinterpret_cast<uint32_t*>(aDstPixels);
    const uint32_t *colormap = reinterpret_cast<const uint32_t*>(aSrcData);

    
    dstPixels += aSrcRect.x + (aSrcRect.y * aDstRect.width);
    if (!aSrcHasAlpha) {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += aDstRect.width;
      }
    } else {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          const uint32_t color = colormap[srcPixels[c]];
          if (color)
            dstPixels[c] = color;
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += aDstRect.width;
      }
    }
  } else {
    pixman_image_t* src = pixman_image_create_bits(aSrcHasAlpha ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
                                                   aSrcRect.width,
                                                   aSrcRect.height,
                                                   reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(aSrcData)),
                                                   aSrcRect.width * 4);
    pixman_image_t* dst = pixman_image_create_bits(PIXMAN_a8r8g8b8,
                                                   aDstRect.width,
                                                   aDstRect.height,
                                                   reinterpret_cast<uint32_t*>(aDstPixels),
                                                   aDstRect.width * 4);

    pixman_image_composite32(aBlendMethod == FrameBlender::kBlendSource ? PIXMAN_OP_SRC : PIXMAN_OP_OVER,
                             src,
                             nullptr,
                             dst,
                             0, 0,
                             0, 0,
                             aSrcRect.x, aSrcRect.y,
                             aSrcRect.width, aSrcRect.height);

    pixman_image_unref(src);
    pixman_image_unref(dst);
  }

  return NS_OK;
}

void
FrameBlender::Discard()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  
  ClearFrames();
}

size_t
FrameBlender::SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                      MallocSizeOf aMallocSizeOf) const
{
  size_t n = mFrames->SizeOfDecodedWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);

  if (mAnim) {
    if (mAnim->compositingFrame) {
      n += mAnim->compositingFrame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
    }
    if (mAnim->compositingPrevFrame) {
      n += mAnim->compositingPrevFrame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
    }
  }

  return n;
}

void
FrameBlender::ResetAnimation()
{
  if (mAnim) {
    mAnim->lastCompositedFrameIndex = -1;
  }
}

} 
} 
