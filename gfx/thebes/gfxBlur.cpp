




































#include "gfxBlur.h"

#include "nsMathUtils.h"
#include "nsTArray.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

gfxAlphaBoxBlur::gfxAlphaBoxBlur()
{
}

gfxAlphaBoxBlur::~gfxAlphaBoxBlur()
{
}

gfxContext*
gfxAlphaBoxBlur::Init(const gfxRect& aRect,
                      const gfxIntSize& aSpreadRadius,
                      const gfxIntSize& aBlurRadius,
                      const gfxRect* aDirtyRect,
                      const gfxRect* aSkipRect)
{
    mSpreadRadius = aSpreadRadius;
    mBlurRadius = aBlurRadius;

    gfxRect rect(aRect);
    rect.Inflate(aBlurRadius + aSpreadRadius);
    rect.RoundOut();

    if (aDirtyRect) {
        
        
        mHasDirtyRect = PR_TRUE;
        mDirtyRect = *aDirtyRect;
        gfxRect requiredBlurArea = mDirtyRect.Intersect(rect);
        requiredBlurArea.Inflate(aBlurRadius + aSpreadRadius);
        rect = requiredBlurArea.Intersect(rect);
    } else {
        mHasDirtyRect = PR_FALSE;
    }

    
    
    
    if (rect.IsEmpty())
        return nsnull;

    if (aSkipRect) {
        
        
        
        gfxRect skipRect = *aSkipRect;
        skipRect.RoundIn();
        skipRect.Deflate(aBlurRadius + aSpreadRadius);
        gfxUtils::GfxRectToIntRect(skipRect, &mSkipRect);
        nsIntRect shadowIntRect;
        gfxUtils::GfxRectToIntRect(rect, &shadowIntRect);
        mSkipRect.IntersectRect(mSkipRect, shadowIntRect);
        if (mSkipRect.IsEqualInterior(shadowIntRect))
          return nsnull;

        mSkipRect -= shadowIntRect.TopLeft();
    } else {
        mSkipRect = nsIntRect(0, 0, 0, 0);
    }

    
    
    mImageSurface = new gfxImageSurface(gfxIntSize(static_cast<PRInt32>(rect.Width()), static_cast<PRInt32>(rect.Height())),
                                        gfxASurface::ImageFormatA8);
    if (!mImageSurface || mImageSurface->CairoStatus())
        return nsnull;

    
    
    
    mImageSurface->SetDeviceOffset(-rect.TopLeft());

    mContext = new gfxContext(mImageSurface);

    return mContext;
}













static void
BoxBlurHorizontal(unsigned char* aInput,
                  unsigned char* aOutput,
                  PRInt32 aLeftLobe,
                  PRInt32 aRightLobe,
                  PRInt32 aWidth,
                  PRInt32 aRows,
                  const nsIntRect& aSkipRect)
{
    NS_ASSERTION(aWidth > 0, "Can't handle zero width here");

    PRInt32 boxSize = aLeftLobe + aRightLobe + 1;
    PRBool skipRectCoversWholeRow = 0 >= aSkipRect.x &&
                                    aWidth <= aSkipRect.XMost();

    for (PRInt32 y = 0; y < aRows; y++) {
        
        
        
        PRBool inSkipRectY = y >= aSkipRect.y &&
                             y < aSkipRect.YMost();
        if (inSkipRectY && skipRectCoversWholeRow) {
            y = aSkipRect.YMost() - 1;
            continue;
        }

        PRInt32 alphaSum = 0;
        for (PRInt32 i = 0; i < boxSize; i++) {
            PRInt32 pos = i - aLeftLobe;
            
            
            pos = NS_MAX(pos, 0);
            pos = NS_MIN(pos, aWidth - 1);
            alphaSum += aInput[aWidth * y + pos];
        }
        for (PRInt32 x = 0; x < aWidth; x++) {
            
            
            if (inSkipRectY && x >= aSkipRect.x &&
                x < aSkipRect.XMost()) {
                x = aSkipRect.XMost();
                if (x >= aWidth)
                    break;

                
                
                alphaSum = 0;
                for (PRInt32 i = 0; i < boxSize; i++) {
                    PRInt32 pos = x + i - aLeftLobe;
                    
                    
                    pos = NS_MAX(pos, 0);
                    pos = NS_MIN(pos, aWidth - 1);
                    alphaSum += aInput[aWidth * y + pos];
                }
            }
            PRInt32 tmp = x - aLeftLobe;
            PRInt32 last = NS_MAX(tmp, 0);
            PRInt32 next = NS_MIN(tmp + boxSize, aWidth - 1);

            aOutput[aWidth * y + x] = alphaSum/boxSize;

            alphaSum += aInput[aWidth * y + next] -
                        aInput[aWidth * y + last];
        }
    }
}






static void
BoxBlurVertical(unsigned char* aInput,
                unsigned char* aOutput,
                PRInt32 aTopLobe,
                PRInt32 aBottomLobe,
                PRInt32 aWidth,
                PRInt32 aRows,
                const nsIntRect& aSkipRect)
{
    NS_ASSERTION(aRows > 0, "Can't handle zero rows here");

    PRInt32 boxSize = aTopLobe + aBottomLobe + 1;
    PRBool skipRectCoversWholeColumn = 0 >= aSkipRect.y &&
                                       aRows <= aSkipRect.YMost();

    for (PRInt32 x = 0; x < aWidth; x++) {
        PRBool inSkipRectX = x >= aSkipRect.x &&
                             x < aSkipRect.XMost();
        if (inSkipRectX && skipRectCoversWholeColumn) {
            x = aSkipRect.XMost() - 1;
            continue;
        }

        PRInt32 alphaSum = 0;
        for (PRInt32 i = 0; i < boxSize; i++) {
            PRInt32 pos = i - aTopLobe;
            
            
            pos = NS_MAX(pos, 0);
            pos = NS_MIN(pos, aRows - 1);
            alphaSum += aInput[aWidth * pos + x];
        }
        for (PRInt32 y = 0; y < aRows; y++) {
            if (inSkipRectX && y >= aSkipRect.y &&
                y < aSkipRect.YMost()) {
                y = aSkipRect.YMost();
                if (y >= aRows)
                    break;

                alphaSum = 0;
                for (PRInt32 i = 0; i < boxSize; i++) {
                    PRInt32 pos = y + i - aTopLobe;
                    
                    
                    pos = NS_MAX(pos, 0);
                    pos = NS_MIN(pos, aRows - 1);
                    alphaSum += aInput[aWidth * pos + x];
                }
            }
            PRInt32 tmp = y - aTopLobe;
            PRInt32 last = NS_MAX(tmp, 0);
            PRInt32 next = NS_MIN(tmp + boxSize, aRows - 1);

            aOutput[aWidth * y + x] = alphaSum/boxSize;

            alphaSum += aInput[aWidth * next + x] -
                        aInput[aWidth * last + x];
        }
    }
}

static void ComputeLobes(PRInt32 aRadius, PRInt32 aLobes[3][2])
{
    PRInt32 major, minor, final;

    



    PRInt32 z = aRadius/3;
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
        NS_ERROR("Mathematical impossibility.");
        major = minor = final = 0;
    }
    NS_ASSERTION(major + minor + final == aRadius,
                 "Lobes don't sum to the right length");

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
                 PRInt32 aRadius,
                 PRInt32 aWidth,
                 PRInt32 aRows,
                 PRInt32 aStride,
                 const nsIntRect& aSkipRect)
{
    if (aRadius == 0) {
        memcpy(aOutput, aInput, aStride*aRows);
        return;
    }

    PRBool skipRectCoversWholeRow = 0 >= aSkipRect.x &&
                                    aWidth <= aSkipRect.XMost();
    for (PRInt32 y = 0; y < aRows; y++) {
        
        
        
        PRBool inSkipRectY = y >= aSkipRect.y &&
                             y < aSkipRect.YMost();
        if (inSkipRectY && skipRectCoversWholeRow) {
            y = aSkipRect.YMost() - 1;
            continue;
        }

        for (PRInt32 x = 0; x < aWidth; x++) {
            
            
            if (inSkipRectY && x >= aSkipRect.x &&
                x < aSkipRect.XMost()) {
                x = aSkipRect.XMost();
                if (x >= aWidth)
                    break;
            }

            PRInt32 sMin = PR_MAX(x - aRadius, 0);
            PRInt32 sMax = PR_MIN(x + aRadius, aWidth - 1);
            PRInt32 v = 0;
            for (PRInt32 s = sMin; s <= sMax; ++s) {
                v = PR_MAX(v, aInput[aStride * y + s]);
            }
            aOutput[aStride * y + x] = v;
        }
    }
}

static void
SpreadVertical(unsigned char* aInput,
               unsigned char* aOutput,
               PRInt32 aRadius,
               PRInt32 aWidth,
               PRInt32 aRows,
               PRInt32 aStride,
               const nsIntRect& aSkipRect)
{
    if (aRadius == 0) {
        memcpy(aOutput, aInput, aStride*aRows);
        return;
    }

    PRBool skipRectCoversWholeColumn = 0 >= aSkipRect.y &&
                                       aRows <= aSkipRect.YMost();
    for (PRInt32 x = 0; x < aWidth; x++) {
        PRBool inSkipRectX = x >= aSkipRect.x &&
                             x < aSkipRect.XMost();
        if (inSkipRectX && skipRectCoversWholeColumn) {
            x = aSkipRect.XMost() - 1;
            continue;
        }

        for (PRInt32 y = 0; y < aRows; y++) {
            
            
            if (inSkipRectX && y >= aSkipRect.y &&
                y < aSkipRect.YMost()) {
                y = aSkipRect.YMost();
                if (y >= aRows)
                    break;
            }

            PRInt32 sMin = PR_MAX(y - aRadius, 0);
            PRInt32 sMax = PR_MIN(y + aRadius, aRows - 1);
            PRInt32 v = 0;
            for (PRInt32 s = sMin; s <= sMax; ++s) {
                v = PR_MAX(v, aInput[aStride * s + x]);
            }
            aOutput[aStride * y + x] = v;
        }
    }
}

void
gfxAlphaBoxBlur::Paint(gfxContext* aDestinationCtx, const gfxPoint& offset)
{
    if (!mContext)
        return;

    unsigned char* boxData = mImageSurface->Data();

    
    if (mBlurRadius != gfxIntSize(0,0) || mSpreadRadius != gfxIntSize(0,0)) {
        nsTArray<unsigned char> tempAlphaDataBuf;
        PRSize szB = mImageSurface->GetDataSize();
        if (!tempAlphaDataBuf.SetLength(szB))
           return; 

        unsigned char* tmpData = tempAlphaDataBuf.Elements();
        
        
        
        memset(tmpData, 0, szB);

        PRInt32 stride = mImageSurface->Stride();
        PRInt32 rows = mImageSurface->Height();
        PRInt32 width = mImageSurface->Width();

        if (mSpreadRadius.width > 0 || mSpreadRadius.height > 0) {
            SpreadHorizontal(boxData, tmpData, mSpreadRadius.width, width, rows, stride, mSkipRect);
            SpreadVertical(tmpData, boxData, mSpreadRadius.height, width, rows, stride, mSkipRect);
        }

        if (mBlurRadius.width > 0) {
            PRInt32 lobes[3][2];
            ComputeLobes(mBlurRadius.width, lobes);
            BoxBlurHorizontal(boxData, tmpData, lobes[0][0], lobes[0][1], stride, rows, mSkipRect);
            BoxBlurHorizontal(tmpData, boxData, lobes[1][0], lobes[1][1], stride, rows, mSkipRect);
            BoxBlurHorizontal(boxData, tmpData, lobes[2][0], lobes[2][1], stride, rows, mSkipRect);
        } else {
            memcpy(tmpData, boxData, stride*rows);
        }

        if (mBlurRadius.height > 0) {
            PRInt32 lobes[3][2];
            ComputeLobes(mBlurRadius.height, lobes);
            BoxBlurVertical(tmpData, boxData, lobes[0][0], lobes[0][1], stride, rows, mSkipRect);
            BoxBlurVertical(boxData, tmpData, lobes[1][0], lobes[1][1], stride, rows, mSkipRect);
            BoxBlurVertical(tmpData, boxData, lobes[2][0], lobes[2][1], stride, rows, mSkipRect);
        } else {
            memcpy(boxData, tmpData, stride*rows);
        }
    }

    
    
    if (mHasDirtyRect) {
        aDestinationCtx->Save();
        aDestinationCtx->NewPath();
        aDestinationCtx->Rectangle(mDirtyRect);
        aDestinationCtx->Clip();
        aDestinationCtx->Mask(mImageSurface, offset);
        aDestinationCtx->Restore();
    } else {
        aDestinationCtx->Mask(mImageSurface, offset);
    }
}













static const gfxFloat GAUSSIAN_SCALE_FACTOR = (3 * sqrt(2 * M_PI) / 4) * 1.5;

gfxIntSize gfxAlphaBoxBlur::CalculateBlurRadius(const gfxPoint& aStd)
{
    return gfxIntSize(
        static_cast<PRInt32>(floor(aStd.x * GAUSSIAN_SCALE_FACTOR + 0.5)),
        static_cast<PRInt32>(floor(aStd.y * GAUSSIAN_SCALE_FACTOR + 0.5)));
}
