


































#include <algorithm>
#include <math.h>

#include "CheckedInt.h"
#include "mozilla/Util.h"

#include "mozilla/gfx/Blur.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

using namespace std;

namespace mozilla {
namespace gfx {













static void
BoxBlurHorizontal(unsigned char* aInput,
                  unsigned char* aOutput,
                  int32_t aLeftLobe,
                  int32_t aRightLobe,
                  int32_t aWidth,
                  int32_t aRows,
                  const IntRect& aSkipRect)
{
    MOZ_ASSERT(aWidth > 0);

    int32_t boxSize = aLeftLobe + aRightLobe + 1;
    bool skipRectCoversWholeRow = 0 >= aSkipRect.x &&
                                  aWidth <= aSkipRect.XMost();

    for (int32_t y = 0; y < aRows; y++) {
        
        
        
        bool inSkipRectY = y >= aSkipRect.y &&
                           y < aSkipRect.YMost();
        if (inSkipRectY && skipRectCoversWholeRow) {
            y = aSkipRect.YMost() - 1;
            continue;
        }

        int32_t alphaSum = 0;
        for (int32_t i = 0; i < boxSize; i++) {
            int32_t pos = i - aLeftLobe;
            
            
            pos = max(pos, 0);
            pos = min(pos, aWidth - 1);
            alphaSum += aInput[aWidth * y + pos];
        }
        for (int32_t x = 0; x < aWidth; x++) {
            
            
            if (inSkipRectY && x >= aSkipRect.x &&
                x < aSkipRect.XMost()) {
                x = aSkipRect.XMost();
                if (x >= aWidth)
                    break;

                
                
                alphaSum = 0;
                for (int32_t i = 0; i < boxSize; i++) {
                    int32_t pos = x + i - aLeftLobe;
                    
                    
                    pos = max(pos, 0);
                    pos = min(pos, aWidth - 1);
                    alphaSum += aInput[aWidth * y + pos];
                }
            }
            int32_t tmp = x - aLeftLobe;
            int32_t last = max(tmp, 0);
            int32_t next = min(tmp + boxSize, aWidth - 1);

            aOutput[aWidth * y + x] = alphaSum / boxSize;

            alphaSum += aInput[aWidth * y + next] -
                        aInput[aWidth * y + last];
        }
    }
}






static void
BoxBlurVertical(unsigned char* aInput,
                unsigned char* aOutput,
                int32_t aTopLobe,
                int32_t aBottomLobe,
                int32_t aWidth,
                int32_t aRows,
                const IntRect& aSkipRect)
{
    MOZ_ASSERT(aRows > 0);

    int32_t boxSize = aTopLobe + aBottomLobe + 1;
    bool skipRectCoversWholeColumn = 0 >= aSkipRect.y &&
                                     aRows <= aSkipRect.YMost();

    for (int32_t x = 0; x < aWidth; x++) {
        bool inSkipRectX = x >= aSkipRect.x &&
                           x < aSkipRect.XMost();
        if (inSkipRectX && skipRectCoversWholeColumn) {
            x = aSkipRect.XMost() - 1;
            continue;
        }

        int32_t alphaSum = 0;
        for (int32_t i = 0; i < boxSize; i++) {
            int32_t pos = i - aTopLobe;
            
            
            pos = max(pos, 0);
            pos = min(pos, aRows - 1);
            alphaSum += aInput[aWidth * pos + x];
        }
        for (int32_t y = 0; y < aRows; y++) {
            if (inSkipRectX && y >= aSkipRect.y &&
                y < aSkipRect.YMost()) {
                y = aSkipRect.YMost();
                if (y >= aRows)
                    break;

                alphaSum = 0;
                for (int32_t i = 0; i < boxSize; i++) {
                    int32_t pos = y + i - aTopLobe;
                    
                    
                    pos = max(pos, 0);
                    pos = min(pos, aRows - 1);
                    alphaSum += aInput[aWidth * pos + x];
                }
            }
            int32_t tmp = y - aTopLobe;
            int32_t last = max(tmp, 0);
            int32_t next = min(tmp + boxSize, aRows - 1);

            aOutput[aWidth * y + x] = alphaSum/boxSize;

            alphaSum += aInput[aWidth * next + x] -
                        aInput[aWidth * last + x];
        }
    }
}

static void ComputeLobes(int32_t aRadius, int32_t aLobes[3][2])
{
    int32_t major, minor, final;

    



    int32_t z = aRadius / 3;
    switch (aRadius % 3) {
    case 0:
        
        major = minor = final = z;
        break;
    case 1:
        
        
        
        
        
        
        
        major = z + 1;
        minor = final = z;
        break;
    case 2:
        
        major = final = z + 1;
        minor = z;
        break;
    default:
        
        MOZ_ASSERT(false);
        major = minor = final = 0;
    }
    MOZ_ASSERT(major + minor + final == aRadius);

    aLobes[0][0] = major;
    aLobes[0][1] = minor;
    aLobes[1][0] = minor;
    aLobes[1][1] = major;
    aLobes[2][0] = final;
    aLobes[2][1] = final;
}

static void
SpreadHorizontal(unsigned char* aInput,
                 unsigned char* aOutput,
                 int32_t aRadius,
                 int32_t aWidth,
                 int32_t aRows,
                 int32_t aStride,
                 const IntRect& aSkipRect)
{
    if (aRadius == 0) {
        memcpy(aOutput, aInput, aStride * aRows);
        return;
    }

    bool skipRectCoversWholeRow = 0 >= aSkipRect.x &&
                                    aWidth <= aSkipRect.XMost();
    for (int32_t y = 0; y < aRows; y++) {
        
        
        
        bool inSkipRectY = y >= aSkipRect.y &&
                             y < aSkipRect.YMost();
        if (inSkipRectY && skipRectCoversWholeRow) {
            y = aSkipRect.YMost() - 1;
            continue;
        }

        for (int32_t x = 0; x < aWidth; x++) {
            
            
            if (inSkipRectY && x >= aSkipRect.x &&
                x < aSkipRect.XMost()) {
                x = aSkipRect.XMost();
                if (x >= aWidth)
                    break;
            }

            int32_t sMin = max(x - aRadius, 0);
            int32_t sMax = min(x + aRadius, aWidth - 1);
            int32_t v = 0;
            for (int32_t s = sMin; s <= sMax; ++s) {
                v = max<int32_t>(v, aInput[aStride * y + s]);
            }
            aOutput[aStride * y + x] = v;
        }
    }
}

static void
SpreadVertical(unsigned char* aInput,
               unsigned char* aOutput,
               int32_t aRadius,
               int32_t aWidth,
               int32_t aRows,
               int32_t aStride,
               const IntRect& aSkipRect)
{
    if (aRadius == 0) {
        memcpy(aOutput, aInput, aStride * aRows);
        return;
    }

    bool skipRectCoversWholeColumn = 0 >= aSkipRect.y &&
                                     aRows <= aSkipRect.YMost();
    for (int32_t x = 0; x < aWidth; x++) {
        bool inSkipRectX = x >= aSkipRect.x &&
                           x < aSkipRect.XMost();
        if (inSkipRectX && skipRectCoversWholeColumn) {
            x = aSkipRect.XMost() - 1;
            continue;
        }

        for (int32_t y = 0; y < aRows; y++) {
            
            
            if (inSkipRectX && y >= aSkipRect.y &&
                y < aSkipRect.YMost()) {
                y = aSkipRect.YMost();
                if (y >= aRows)
                    break;
            }

            int32_t sMin = max(y - aRadius, 0);
            int32_t sMax = min(y + aRadius, aRows - 1);
            int32_t v = 0;
            for (int32_t s = sMin; s <= sMax; ++s) {
                v = max<int32_t>(v, aInput[aStride * s + x]);
            }
            aOutput[aStride * y + x] = v;
        }
    }
}

static CheckedInt<int32_t>
RoundUpToMultipleOf4(int32_t aVal)
{
  CheckedInt<int32_t> val(aVal);

  val += 3;
  val /= 4;
  val *= 4;

  return val;
}

AlphaBoxBlur::AlphaBoxBlur(const Rect& aRect,
                           const IntSize& aSpreadRadius,
                           const IntSize& aBlurRadius,
                           const Rect* aDirtyRect,
                           const Rect* aSkipRect)
 : mSpreadRadius(aSpreadRadius),
   mBlurRadius(aBlurRadius),
   mData(NULL)
{
  Rect rect(aRect);
  rect.Inflate(Size(aBlurRadius + aSpreadRadius));
  rect.RoundOut();

  if (aDirtyRect) {
    
    
    mHasDirtyRect = true;
    mDirtyRect = *aDirtyRect;
    Rect requiredBlurArea = mDirtyRect.Intersect(rect);
    requiredBlurArea.Inflate(Size(aBlurRadius + aSpreadRadius));
    rect = requiredBlurArea.Intersect(rect);
  } else {
    mHasDirtyRect = false;
  }

  if (rect.IsEmpty()) {
    return;
  }

  if (aSkipRect) {
    
    
    
    Rect skipRect = *aSkipRect;
    skipRect.RoundIn();
    skipRect.Deflate(Size(aBlurRadius + aSpreadRadius));
    mSkipRect = IntRect(skipRect.x, skipRect.y, skipRect.width, skipRect.height);

    IntRect shadowIntRect(rect.x, rect.y, rect.width, rect.height);
    mSkipRect.IntersectRect(mSkipRect, shadowIntRect);

    if (mSkipRect.IsEqualInterior(shadowIntRect))
      return;

    mSkipRect -= shadowIntRect.TopLeft();
  } else {
    mSkipRect = IntRect(0, 0, 0, 0);
  }

  mRect = IntRect(rect.x, rect.y, rect.width, rect.height);

  CheckedInt<int32_t> stride = RoundUpToMultipleOf4(mRect.width);
  if (stride.valid()) {
    mStride = stride.value();

    CheckedInt<int32_t> size = CheckedInt<int32_t>(mStride) * mRect.height *
                               sizeof(unsigned char);
    if (size.valid()) {
      mData = static_cast<unsigned char*>(malloc(size.value()));
      memset(mData, 0, size.value());
    }
  }
}

AlphaBoxBlur::~AlphaBoxBlur()
{
  free(mData);
}

unsigned char*
AlphaBoxBlur::GetData()
{
  return mData;
}

IntSize
AlphaBoxBlur::GetSize()
{
  IntSize size(mRect.width, mRect.height);
  return size;
}

int32_t
AlphaBoxBlur::GetStride()
{
  return mStride;
}

IntRect
AlphaBoxBlur::GetRect()
{
  return mRect;
}

Rect*
AlphaBoxBlur::GetDirtyRect()
{
  if (mHasDirtyRect) {
    return &mDirtyRect;
  }

  return NULL;
}

void
AlphaBoxBlur::Blur()
{
  if (!mData) {
    return;
  }

  
  if (mBlurRadius != IntSize(0,0) || mSpreadRadius != IntSize(0,0)) {
    int32_t stride = GetStride();

    
    size_t szB = stride * GetSize().height * sizeof(unsigned char);
    unsigned char* tmpData = static_cast<unsigned char*>(malloc(szB));
    if (!tmpData)
      return; 

    memset(tmpData, 0, szB);

    if (mSpreadRadius.width > 0 || mSpreadRadius.height > 0) {
      SpreadHorizontal(mData, tmpData, mSpreadRadius.width, GetSize().width, GetSize().height, stride, mSkipRect);
      SpreadVertical(tmpData, mData, mSpreadRadius.height, GetSize().width, GetSize().height, stride, mSkipRect);
    }

    if (mBlurRadius.width > 0) {
      int32_t lobes[3][2];
      ComputeLobes(mBlurRadius.width, lobes);
      BoxBlurHorizontal(mData, tmpData, lobes[0][0], lobes[0][1], stride, GetSize().height, mSkipRect);
      BoxBlurHorizontal(tmpData, mData, lobes[1][0], lobes[1][1], stride, GetSize().height, mSkipRect);
      BoxBlurHorizontal(mData, tmpData, lobes[2][0], lobes[2][1], stride, GetSize().height, mSkipRect);
    } else {
      memcpy(tmpData, mData, stride * GetSize().height);
    }

    if (mBlurRadius.height > 0) {
      int32_t lobes[3][2];
      ComputeLobes(mBlurRadius.height, lobes);
      BoxBlurVertical(tmpData, mData, lobes[0][0], lobes[0][1], stride, GetSize().height, mSkipRect);
      BoxBlurVertical(mData, tmpData, lobes[1][0], lobes[1][1], stride, GetSize().height, mSkipRect);
      BoxBlurVertical(tmpData, mData, lobes[2][0], lobes[2][1], stride, GetSize().height, mSkipRect);
    } else {
      memcpy(mData, tmpData, stride * GetSize().height);
    }

    free(tmpData);
  }

}













static const Float GAUSSIAN_SCALE_FACTOR = (3 * sqrt(2 * M_PI) / 4) * 1.5;

IntSize
AlphaBoxBlur::CalculateBlurRadius(const Point& aStd)
{
    IntSize size(static_cast<int32_t>(floor(aStd.x * GAUSSIAN_SCALE_FACTOR + 0.5)),
                 static_cast<int32_t>(floor(aStd.y * GAUSSIAN_SCALE_FACTOR + 0.5)));

    return size;
}

}
}
