




#include "FrameBlender.h"

#include "mozilla/MemoryReporting.h"
#include "RasterImage.h"
#include "imgFrame.h"

#define PIXMAN_DONT_DEFINE_STDINT
#include "pixman.h"

using namespace mozilla;
using namespace mozilla::image;

namespace mozilla {
namespace image {

FrameBlender::FrameBlender()
 : mAnim(nullptr)
{}

FrameBlender::~FrameBlender()
{
  ClearFrames();

  delete mAnim;
}

imgFrame*
FrameBlender::GetFrame(uint32_t framenum) const
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames.SafeElementAt(0, nullptr);
  }
  if (mAnim->lastCompositedFrameIndex == int32_t(framenum))
    return mAnim->compositingFrame;
  return mFrames.SafeElementAt(framenum, nullptr);
}

imgFrame*
FrameBlender::RawGetFrame(uint32_t framenum) const
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames.SafeElementAt(0, nullptr);
  }

  return mFrames.SafeElementAt(framenum, nullptr);
}

uint32_t
FrameBlender::GetNumFrames() const
{
  return mFrames.Length();
}

void
FrameBlender::RemoveFrame(uint32_t framenum)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Deleting invalid frame!");

  delete mFrames[framenum];
  mFrames[framenum] = nullptr;
  mFrames.RemoveElementAt(framenum);
}

void
FrameBlender::ClearFrames()
{
  for (uint32_t i = 0; i < mFrames.Length(); ++i) {
    delete mFrames[i];
  }
  mFrames.Clear();
}

void
FrameBlender::InsertFrame(uint32_t framenum, imgFrame* aFrame)
{
  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Inserting invalid frame!");
  mFrames.InsertElementAt(framenum, aFrame);
  if (GetNumFrames() > 1) {
    EnsureAnimExists();
  }
}

imgFrame*
FrameBlender::SwapFrame(uint32_t framenum, imgFrame* aFrame)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Swapping invalid frame!");
  imgFrame* ret = mFrames.SafeElementAt(framenum, nullptr);
  mFrames.RemoveElementAt(framenum);
  if (aFrame) {
    mFrames.InsertElementAt(framenum, aFrame);
  }

  return ret;
}




bool
FrameBlender::DoBlend(nsIntRect* aDirtyRect,
                      uint32_t aPrevFrameIndex,
                      uint32_t aNextFrameIndex)
{
  if (!aDirtyRect) {
    return false;
  }

  imgFrame* prevFrame = mFrames[aPrevFrameIndex];
  imgFrame* nextFrame = mFrames[aNextFrameIndex];
  if (!prevFrame || !nextFrame) {
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
    mAnim->compositingFrame = new imgFrame();
    nsresult rv = mAnim->compositingFrame->Init(0, 0, mSize.width, mSize.height,
                                                gfxASurface::ImageFormatARGB32);
    if (NS_FAILED(rv)) {
      mAnim->compositingFrame = nullptr;
      return false;
    }
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
          
          
          ClearFrame(mAnim->compositingFrame);
        } else {
          
          ClearFrame(mAnim->compositingFrame, prevFrameRect);
        }
        break;

      case FrameBlender::kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame);
        break;

      case FrameBlender::kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);

          
          if (nextFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious)
            mAnim->compositingPrevFrame = nullptr;
        } else {
          ClearFrame(mAnim->compositingFrame);
        }
        break;

      default:
        
        
        
        
        
        
        if (mAnim->lastCompositedFrameIndex != int32_t(aNextFrameIndex - 1)) {
          if (isFullPrevFrame && !prevFrame->GetIsPaletted()) {
            
            CopyFrameImage(prevFrame, mAnim->compositingFrame);
          } else {
            if (needToBlankComposite) {
              
              if (prevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame);
              }
            }
            DrawFrameTo(prevFrame, mAnim->compositingFrame, prevFrameRect);
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mAnim->compositingFrame);
  }

  
  
  
  if ((nextFrameDisposalMethod == FrameBlender::kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame = new imgFrame();
      nsresult rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                                      gfxASurface::ImageFormatARGB32);
      if (NS_FAILED(rv)) {
        mAnim->compositingPrevFrame = nullptr;
        return false;
      }
    }

    CopyFrameImage(mAnim->compositingFrame, mAnim->compositingPrevFrame);
  }

  
  DrawFrameTo(nextFrame, mAnim->compositingFrame, nextFrameRect);

  
  
  int32_t timeout = nextFrame->GetTimeout();
  mAnim->compositingFrame->SetTimeout(timeout);

  
  nsresult rv = mAnim->compositingFrame->ImageUpdated(mAnim->compositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return false;
  }

  
  
  
  
  if (isFullNextFrame &&
      nextFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious &&
      !nextFrame->GetIsPaletted()) {
    
    
    
    
    
    if (CopyFrameImage(mAnim->compositingFrame, nextFrame)) {
      prevFrame->SetFrameDisposalMethod(FrameBlender::kDisposeClearAll);
      mAnim->compositingFrame = nullptr;
      mAnim->lastCompositedFrameIndex = -1;
      return true;
    }
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
FrameBlender::ClearFrame(imgFrame* aFrame)
{
  AutoFrameLocker lock(aFrame);
  if (lock.Succeeded()) {
    ClearFrame(aFrame->GetImageData(), aFrame->GetRect());
  }
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
  for (int row = toClear.y; row < toClear.height; ++row) {
    memset(aFrameData + toClear.x * 4 + row * bytesPerRow, 0, toClear.width * 4);
  }
}

void
FrameBlender::ClearFrame(imgFrame* aFrame, const nsIntRect& aRectToClear)
{
  AutoFrameLocker lock(aFrame);
  if (lock.Succeeded()) {
    ClearFrame(aFrame->GetImageData(), aFrame->GetRect(), aRectToClear);
  }
}




bool
FrameBlender::CopyFrameImage(uint8_t *aDataSrc, const nsIntRect& aRectSrc,
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

bool
FrameBlender::CopyFrameImage(imgFrame* aSrc, imgFrame* aDst)
{
  AutoFrameLocker srclock(aSrc);
  AutoFrameLocker dstlock(aDst);
  if (!srclock.Succeeded() || !dstlock.Succeeded()) {
    return false;
  }

  return CopyFrameImage(aSrc->GetImageData(), aSrc->GetRect(),
                        aDst->GetImageData(), aDst->GetRect());
}

nsresult
FrameBlender::DrawFrameTo(uint8_t *aSrcData, const nsIntRect& aSrcRect,
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

    
    uint8_t *srcPixels = aSrcData + aSrcPaletteLength;
    uint32_t *dstPixels = reinterpret_cast<uint32_t*>(aDstPixels);
    uint32_t *colormap = reinterpret_cast<uint32_t*>(aSrcData);

    
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
                                                   reinterpret_cast<uint32_t*>(aSrcData),
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
                             aDstRect.width, aDstRect.height);

    pixman_image_unref(src);
    pixman_image_unref(dst);
  }

  return NS_OK;
}

nsresult
FrameBlender::DrawFrameTo(imgFrame* aSrc, imgFrame* aDst, const nsIntRect& aSrcRect)
{
  AutoFrameLocker srclock(aSrc);
  AutoFrameLocker dstlock(aDst);
  if (!srclock.Succeeded() || !dstlock.Succeeded()) {
    return NS_ERROR_FAILURE;
  }

  if (aSrc->GetIsPaletted()) {
    return DrawFrameTo(reinterpret_cast<uint8_t*>(aSrc->GetPaletteData()),
                       aSrcRect, aSrc->PaletteDataLength(),
                       aSrc->GetHasAlpha(), aDst->GetImageData(),
                       aDst->GetRect(),
                       FrameBlendMethod(aSrc->GetBlendMethod()));
  }

  return DrawFrameTo(aSrc->GetImageData(), aSrcRect,
                     0, aSrc->GetHasAlpha(),
                     aDst->GetImageData(), aDst->GetRect(),
                     FrameBlendMethod(aSrc->GetBlendMethod()));
}

void
FrameBlender::Discard()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  
  for (uint32_t i = 0; i < mFrames.Length(); ++i)
    delete mFrames[i];
  mFrames.Clear();
}

size_t
FrameBlender::SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MemoryLocation aLocation,
                                                      MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  for (uint32_t i = 0; i < mFrames.Length(); ++i) {
    imgFrame* frame = mFrames.SafeElementAt(i, nullptr);
    NS_ABORT_IF_FALSE(frame, "Null frame in frame array!");
    n += frame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

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
