




#include "FrameBlender.h"

#include "mozilla/MemoryReporting.h"
#include "mozilla/Move.h"
#include "MainThreadUtils.h"

#include "pixman.h"

namespace mozilla {

using namespace gfx;

namespace image {

FrameBlender::FrameBlender()
 : mAnim(nullptr)
 , mLoopCount(-1)
{
}

FrameBlender::~FrameBlender()
{
  delete mAnim;
}

already_AddRefed<imgFrame>
FrameBlender::GetFrame(uint32_t aFrameNum)
{
  if (mAnim && mAnim->lastCompositedFrameIndex == int32_t(aFrameNum)) {
    nsRefPtr<imgFrame> frame = mAnim->compositingFrame.get();
    return frame.forget();
  }
  return RawGetFrame(aFrameNum);
}

already_AddRefed<imgFrame>
FrameBlender::RawGetFrame(uint32_t aFrameNum)
{
  if (!mAnim) {
    NS_ASSERTION(aFrameNum == 0,
                 "Don't ask for a frame > 0 if we're not animated!");
    aFrameNum = 0;
  }
  if (aFrameNum >= mFrames.Length()) {
    return nullptr;
  }
  nsRefPtr<imgFrame> frame = mFrames[aFrameNum].get();
  return frame.forget();
}

uint32_t
FrameBlender::GetNumFrames() const
{
  return mFrames.Length();
}

int32_t
FrameBlender::GetTimeoutForFrame(uint32_t aFrameNum)
{
  nsRefPtr<imgFrame> frame = RawGetFrame(aFrameNum);
  const int32_t timeout = frame->GetRawTimeout();

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (timeout >= 0 && timeout <= 10 && mLoopCount != 0) {
    return 100;
  }

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
FrameBlender::RemoveFrame(uint32_t aFrameNum)
{
  MOZ_ASSERT(aFrameNum < GetNumFrames(), "Deleting invalid frame!");
  mFrames.RemoveElementAt(aFrameNum);
}

void
FrameBlender::ClearFrames()
{
  mFrames.Clear();
  mFrames.Compact();
}

void
FrameBlender::InsertFrame(uint32_t aFrameNum, RawAccessFrameRef&& aRef)
{
  MOZ_ASSERT(aRef, "Need a reference to a frame");
  MOZ_ASSERT(aFrameNum <= GetNumFrames(), "Inserting invalid frame");

  mFrames.InsertElementAt(aFrameNum, Move(aRef));
  if (GetNumFrames() == 2) {
    MOZ_ASSERT(!mAnim, "Shouldn't have an animation context yet");
    mAnim = new Anim();
  }

  MOZ_ASSERT(GetNumFrames() < 2 || mAnim,
             "If we're animated we should have an animation context now");
}




bool
FrameBlender::DoBlend(nsIntRect* aDirtyRect,
                      uint32_t aPrevFrameIndex,
                      uint32_t aNextFrameIndex)
{
  nsRefPtr<imgFrame> prevFrame = RawGetFrame(aPrevFrameIndex);
  nsRefPtr<imgFrame> nextFrame = RawGetFrame(aNextFrameIndex);

  MOZ_ASSERT(prevFrame && nextFrame, "Should have frames here");

  int32_t prevFrameDisposalMethod = prevFrame->GetFrameDisposalMethod();
  if (prevFrameDisposalMethod == FrameBlender::kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame) {
    prevFrameDisposalMethod = FrameBlender::kDisposeClear;
  }

  nsIntRect prevFrameRect = prevFrame->GetRect();
  bool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                          prevFrameRect.width == mSize.width &&
                          prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame &&
      (prevFrameDisposalMethod == FrameBlender::kDisposeClear)) {
    prevFrameDisposalMethod = FrameBlender::kDisposeClearAll;
  }

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
    nsRefPtr<imgFrame> newFrame = new imgFrame;
    nsresult rv = newFrame->InitForDecoder(mSize, SurfaceFormat::B8G8R8A8);
    if (NS_FAILED(rv)) {
      mAnim->compositingFrame.reset();
      return false;
    }
    mAnim->compositingFrame = newFrame->RawAccessRef();
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
          (prevFrameRect.x + prevFrameRect.width <=
              nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <=
              nextFrameRect.y + nextFrameRect.height)) {
        
        
        doDisposal = false;
      }
    }
  }

  if (doDisposal) {
    
    switch (prevFrameDisposalMethod) {
      case FrameBlender::kDisposeClear:
        if (needToBlankComposite) {
          
          
          ClearFrame(mAnim->compositingFrame->GetRawData(),
                     mAnim->compositingFrame->GetRect());
        } else {
          
          ClearFrame(mAnim->compositingFrame->GetRawData(),
                     mAnim->compositingFrame->GetRect(),
                     prevFrameRect);
        }
        break;

      case FrameBlender::kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame->GetRawData(),
                   mAnim->compositingFrame->GetRect());
        break;

      case FrameBlender::kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame->GetRawData(),
                         mAnim->compositingPrevFrame->GetRect(),
                         mAnim->compositingFrame->GetRawData(),
                         mAnim->compositingFrame->GetRect());

          
          if (nextFrameDisposalMethod !=
              FrameBlender::kDisposeRestorePrevious) {
            mAnim->compositingPrevFrame.reset();
          }
        } else {
          ClearFrame(mAnim->compositingFrame->GetRawData(),
                     mAnim->compositingFrame->GetRect());
        }
        break;

      default:
        
        
        
        
        
        
        
        if (mAnim->lastCompositedFrameIndex != int32_t(aNextFrameIndex - 1)) {
          if (isFullPrevFrame && !prevFrame->GetIsPaletted()) {
            
            CopyFrameImage(prevFrame->GetRawData(),
                           prevFrame->GetRect(),
                           mAnim->compositingFrame->GetRawData(),
                           mAnim->compositingFrame->GetRect());
          } else {
            if (needToBlankComposite) {
              
              if (prevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame->GetRawData(),
                           mAnim->compositingFrame->GetRect());
              }
            }
            DrawFrameTo(prevFrame->GetRawData(), prevFrameRect,
                        prevFrame->PaletteDataLength(),
                        prevFrame->GetHasAlpha(),
                        mAnim->compositingFrame->GetRawData(),
                        mAnim->compositingFrame->GetRect(),
                        FrameBlendMethod(prevFrame->GetBlendMethod()));
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mAnim->compositingFrame->GetRawData(),
               mAnim->compositingFrame->GetRect());
  }

  
  
  
  if ((nextFrameDisposalMethod == FrameBlender::kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != FrameBlender::kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      nsRefPtr<imgFrame> newFrame = new imgFrame;
      nsresult rv = newFrame->InitForDecoder(mSize, SurfaceFormat::B8G8R8A8);
      if (NS_FAILED(rv)) {
        mAnim->compositingPrevFrame.reset();
        return false;
      }

      mAnim->compositingPrevFrame = newFrame->RawAccessRef();
    }

    CopyFrameImage(mAnim->compositingFrame->GetRawData(),
                   mAnim->compositingFrame->GetRect(),
                   mAnim->compositingPrevFrame->GetRawData(),
                   mAnim->compositingPrevFrame->GetRect());
  }

  
  DrawFrameTo(nextFrame->GetRawData(), nextFrameRect,
              nextFrame->PaletteDataLength(),
              nextFrame->GetHasAlpha(),
              mAnim->compositingFrame->GetRawData(),
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
  if (!aFrameData) {
    return;
  }

  memset(aFrameData, 0, aFrameRect.width * aFrameRect.height * 4);
}


void
FrameBlender::ClearFrame(uint8_t* aFrameData, const nsIntRect& aFrameRect,
                         const nsIntRect& aRectToClear)
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
    memset(aFrameData + toClear.x * 4 + row * bytesPerRow, 0,
           toClear.width * 4);
  }
}




bool
FrameBlender::CopyFrameImage(const uint8_t* aDataSrc, const nsIntRect& aRectSrc,
                             uint8_t* aDataDest, const nsIntRect& aRectDest)
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
FrameBlender::DrawFrameTo(const uint8_t* aSrcData, const nsIntRect& aSrcRect,
                          uint32_t aSrcPaletteLength, bool aSrcHasAlpha,
                          uint8_t* aDstPixels, const nsIntRect& aDstRect,
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

    
    const uint8_t* srcPixels = aSrcData + aSrcPaletteLength;
    uint32_t* dstPixels = reinterpret_cast<uint32_t*>(aDstPixels);
    const uint32_t* colormap = reinterpret_cast<const uint32_t*>(aSrcData);

    
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
          if (color) {
            dstPixels[c] = color;
          }
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += aDstRect.width;
      }
    }
  } else {
    pixman_image_t* src =
      pixman_image_create_bits(
          aSrcHasAlpha ? PIXMAN_a8r8g8b8 : PIXMAN_x8r8g8b8,
          aSrcRect.width, aSrcRect.height,
          reinterpret_cast<uint32_t*>(const_cast<uint8_t*>(aSrcData)),
          aSrcRect.width * 4);
    pixman_image_t* dst =
      pixman_image_create_bits(PIXMAN_a8r8g8b8,
                               aDstRect.width,
                               aDstRect.height,
                               reinterpret_cast<uint32_t*>(aDstPixels),
                               aDstRect.width * 4);

    auto op = aBlendMethod == FrameBlender::kBlendSource ? PIXMAN_OP_SRC
                                                         : PIXMAN_OP_OVER;
    pixman_image_composite32(op,
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
FrameBlender::SizeOfDecoded(gfxMemoryLocation aLocation,
                            MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;

  for (uint32_t i = 0; i < mFrames.Length(); ++i) {
    n += mFrames[i]->SizeOfExcludingThis(aLocation, aMallocSizeOf);
  }

  if (mAnim) {
    if (mAnim->compositingFrame) {
      n += mAnim->compositingFrame
                ->SizeOfExcludingThis(aLocation, aMallocSizeOf);
    }
    if (mAnim->compositingPrevFrame) {
      n += mAnim->compositingPrevFrame
                ->SizeOfExcludingThis(aLocation, aMallocSizeOf);
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
