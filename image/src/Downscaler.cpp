





#include "Downscaler.h"

#include <algorithm>
#include <ctime>
#include "gfxPrefs.h"
#include "image_operations.h"
#include "convolver.h"
#include "skia/SkTypes.h"

using std::max;
using std::swap;

namespace mozilla {
namespace image {

Downscaler::Downscaler(const nsIntSize& aTargetSize)
  : mTargetSize(aTargetSize)
  , mOutputBuffer(nullptr)
  , mXFilter(MakeUnique<skia::ConvolutionFilter1D>())
  , mYFilter(MakeUnique<skia::ConvolutionFilter1D>())
  , mWindowCapacity(0)
  , mHasAlpha(true)
{
  MOZ_ASSERT(gfxPrefs::ImageDownscaleDuringDecodeEnabled(),
             "Downscaling even though downscale-during-decode is disabled?");
  MOZ_ASSERT(mTargetSize.width > 0 && mTargetSize.height > 0,
             "Invalid target size");
}

Downscaler::~Downscaler()
{
  ReleaseWindow();
}

void
Downscaler::ReleaseWindow()
{
  if (!mWindow) {
    return;
  }

  for (int32_t i = 0; i < mWindowCapacity; ++i) {
    delete[] mWindow[i];
  }

  mWindow = nullptr;
  mWindowCapacity = 0;
}

nsresult
Downscaler::BeginFrame(const nsIntSize& aOriginalSize,
                       uint8_t* aOutputBuffer,
                       bool aHasAlpha)
{
  MOZ_ASSERT(aOutputBuffer);
  MOZ_ASSERT(mTargetSize != aOriginalSize,
             "Created a downscaler, but not downscaling?");
  MOZ_ASSERT(mTargetSize.width <= aOriginalSize.width,
             "Created a downscaler, but width is larger");
  MOZ_ASSERT(mTargetSize.height <= aOriginalSize.height,
             "Created a downscaler, but height is larger");
  MOZ_ASSERT(aOriginalSize.width > 0 && aOriginalSize.height > 0,
             "Invalid original size");

  mOriginalSize = aOriginalSize;
  mOutputBuffer = aOutputBuffer;
  mHasAlpha = aHasAlpha;

  ResetForNextProgressivePass();
  ReleaseWindow();

  auto resizeMethod = skia::ImageOperations::RESIZE_LANCZOS3;

  skia::resize::ComputeFilters(resizeMethod,
                               mOriginalSize.width, mTargetSize.width,
                               0, mTargetSize.width,
                               mXFilter.get());

  skia::resize::ComputeFilters(resizeMethod,
                               mOriginalSize.height, mTargetSize.height,
                               0, mTargetSize.height,
                               mYFilter.get());

  
  mRowBuffer = MakeUnique<uint8_t[]>(mOriginalSize.width * sizeof(uint32_t));
  if (MOZ_UNLIKELY(!mRowBuffer)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  
  
  mWindowCapacity = mYFilter->max_filter();
  mWindow = MakeUnique<uint8_t*[]>(mWindowCapacity);
  if (MOZ_UNLIKELY(!mWindow)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  bool anyAllocationFailed = false;
  const int rowSize = mTargetSize.width * sizeof(uint32_t);
  for (int32_t i = 0; i < mWindowCapacity; ++i) {
    mWindow[i] = new uint8_t[rowSize];
    anyAllocationFailed = anyAllocationFailed || mWindow[i] == nullptr;
  }

  if (MOZ_UNLIKELY(anyAllocationFailed)) {
    
    
    
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
Downscaler::ResetForNextProgressivePass()
{
  mPrevInvalidatedLine = 0;
  mCurrentOutLine = 0;
  mCurrentInLine = 0;
  mLinesInBuffer = 0;
}

static void
GetFilterOffsetAndLength(UniquePtr<skia::ConvolutionFilter1D>& aFilter,
                         int32_t aOutputImagePosition,
                         int32_t* aFilterOffsetOut,
                         int32_t* aFilterLengthOut)
{
  MOZ_ASSERT(aOutputImagePosition < aFilter->num_values());
  aFilter->FilterForValue(aOutputImagePosition,
                          aFilterOffsetOut,
                          aFilterLengthOut);
}

void
Downscaler::CommitRow()
{
  MOZ_ASSERT(mOutputBuffer, "Should have a current frame");
  MOZ_ASSERT(mCurrentInLine < mOriginalSize.height, "Past end of input");
  MOZ_ASSERT(mCurrentOutLine < mTargetSize.height, "Past end of output");

  int32_t filterOffset = 0;
  int32_t filterLength = 0;
  GetFilterOffsetAndLength(mYFilter, mCurrentOutLine,
                           &filterOffset, &filterLength);

  int32_t inLineToRead = filterOffset + mLinesInBuffer;
  MOZ_ASSERT(mCurrentInLine <= inLineToRead, "Reading past end of input");
  if (mCurrentInLine == inLineToRead) {
    skia::ConvolveHorizontally(mRowBuffer.get(), *mXFilter,
                               mWindow[mLinesInBuffer++], mHasAlpha,
                                true);
  }

  MOZ_ASSERT(mCurrentOutLine < mTargetSize.height,
             "Writing past end of output");

  while (mLinesInBuffer == filterLength) {
    DownscaleInputLine();

    if (mCurrentOutLine == mTargetSize.height) {
      break;  
    }

    GetFilterOffsetAndLength(mYFilter, mCurrentOutLine,
                             &filterOffset, &filterLength);
  }

  mCurrentInLine += 1;
}

bool
Downscaler::HasInvalidation() const
{
  return mCurrentOutLine > mPrevInvalidatedLine;
}

nsIntRect
Downscaler::TakeInvalidRect()
{
  if (MOZ_UNLIKELY(!HasInvalidation())) {
    return nsIntRect();
  }

  nsIntRect invalidRect(0, mPrevInvalidatedLine,
                        mTargetSize.width,
                        mCurrentOutLine - mPrevInvalidatedLine);
  mPrevInvalidatedLine = mCurrentOutLine;
  return invalidRect;
}

void
Downscaler::DownscaleInputLine()
{
  typedef skia::ConvolutionFilter1D::Fixed FilterValue;

  MOZ_ASSERT(mOutputBuffer);
  MOZ_ASSERT(mCurrentOutLine < mTargetSize.height,
             "Writing past end of output");

  int32_t filterOffset = 0;
  int32_t filterLength = 0;
  MOZ_ASSERT(mCurrentOutLine < mYFilter->num_values());
  auto filterValues =
    mYFilter->FilterForValue(mCurrentOutLine, &filterOffset, &filterLength);

  uint8_t* outputLine =
    &mOutputBuffer[mCurrentOutLine * mTargetSize.width * sizeof(uint32_t)];
  skia::ConvolveVertically(static_cast<const FilterValue*>(filterValues),
                           filterLength, mWindow.get(), mXFilter->num_values(),
                           outputLine, mHasAlpha,  true);

  mCurrentOutLine += 1;

  if (mCurrentOutLine == mTargetSize.height) {
    
    return;
  }

  int32_t newFilterOffset = 0;
  int32_t newFilterLength = 0;
  GetFilterOffsetAndLength(mYFilter, mCurrentOutLine,
                           &newFilterOffset, &newFilterLength);

  int diff = newFilterOffset - filterOffset;
  MOZ_ASSERT(diff >= 0, "Moving backwards in the filter?");

  
  mLinesInBuffer -= diff;
  mLinesInBuffer = max(mLinesInBuffer, 0);
  for (int32_t i = 0; i < mLinesInBuffer; ++i) {
    swap(mWindow[i], mWindow[filterLength - mLinesInBuffer + i]);
  }
}

} 
} 
