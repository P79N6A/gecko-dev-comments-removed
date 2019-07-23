




































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
                      const gfxIntSize& aBlurRadius,
                      const gfxRect* aDirtyRect)
{
    mBlurRadius = aBlurRadius;

    gfxRect rect(aRect);
    rect.Outset(aBlurRadius.height, aBlurRadius.width,
                aBlurRadius.height, aBlurRadius.width);
    rect.RoundOut();

    if (rect.IsEmpty())
        return nsnull;

    if (aDirtyRect) {
        
        
        mHasDirtyRect = PR_TRUE;
        mDirtyRect = *aDirtyRect;
        gfxRect requiredBlurArea = mDirtyRect.Intersect(rect);
        requiredBlurArea.Outset(aBlurRadius.height, aBlurRadius.width,
                                aBlurRadius.height, aBlurRadius.width);
        rect = requiredBlurArea.Intersect(rect);
    } else {
        mHasDirtyRect = PR_FALSE;
    }

    
    
    mImageSurface = new gfxImageSurface(gfxIntSize(static_cast<PRInt32>(rect.Width()), static_cast<PRInt32>(rect.Height())),
                                        gfxASurface::ImageFormatA8);
    if (!mImageSurface || mImageSurface->CairoStatus())
        return nsnull;

    
    
    
    mImageSurface->SetDeviceOffset(-rect.TopLeft());

    mContext = new gfxContext(mImageSurface);

    return mContext;
}

void
gfxAlphaBoxBlur::PremultiplyAlpha(gfxFloat alpha)
{
    if (!mImageSurface)
        return;

    unsigned char* data = mImageSurface->Data();
    PRInt32 length = mImageSurface->GetDataSize();

    for (PRInt32 i=0; i<length; ++i)
        data[i] = static_cast<unsigned char>(data[i] * alpha);
}











static void
BoxBlurHorizontal(unsigned char* aInput,
                  unsigned char* aOutput,
                  PRInt32 aLeftLobe,
                  PRInt32 aRightLobe,
                  PRInt32 aStride,
                  PRInt32 aRows)
{
    PRInt32 boxSize = aLeftLobe + aRightLobe + 1;

    for (PRInt32 y = 0; y < aRows; y++) {
        PRInt32 alphaSum = 0;
        for (PRInt32 i = 0; i < boxSize; i++) {
            PRInt32 pos = i - aLeftLobe;
            pos = PR_MAX(pos, 0);
            pos = PR_MIN(pos, aStride - 1);
            alphaSum += aInput[aStride * y + pos];
        }
        for (PRInt32 x = 0; x < aStride; x++) {
            PRInt32 tmp = x - aLeftLobe;
            PRInt32 last = PR_MAX(tmp, 0);
            PRInt32 next = PR_MIN(tmp + boxSize, aStride - 1);

            aOutput[aStride * y + x] = alphaSum/boxSize;

            alphaSum += aInput[aStride * y + next] -
                        aInput[aStride * y + last];
        }
    }
}





static void
BoxBlurVertical(unsigned char* aInput,
                unsigned char* aOutput,
                PRInt32 aTopLobe,
                PRInt32 aBottomLobe,
                PRInt32 aStride,
                PRInt32 aRows)
{
    PRInt32 boxSize = aTopLobe + aBottomLobe + 1;

    for (PRInt32 x = 0; x < aStride; x++) {
        PRInt32 alphaSum = 0;
        for (PRInt32 i = 0; i < boxSize; i++) {
            PRInt32 pos = i - aTopLobe;
            pos = PR_MAX(pos, 0);
            pos = PR_MIN(pos, aRows - 1);
            alphaSum += aInput[aStride * pos + x];
        }
        for (PRInt32 y = 0; y < aRows; y++) {
            PRInt32 tmp = y - aTopLobe;
            PRInt32 last = PR_MAX(tmp, 0);
            PRInt32 next = PR_MIN(tmp + boxSize, aRows - 1);

            aOutput[aStride * y + x] = alphaSum/boxSize;

            alphaSum += aInput[aStride * next + x] -
                        aInput[aStride * last + x];
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

void
gfxAlphaBoxBlur::Paint(gfxContext* aDestinationCtx, const gfxPoint& offset)
{
    if (!mContext)
        return;

    unsigned char* boxData = mImageSurface->Data();

    
    if (mBlurRadius.width != 0 || mBlurRadius.height != 0) {
        nsTArray<unsigned char> tempAlphaDataBuf;
        if (!tempAlphaDataBuf.SetLength(mImageSurface->GetDataSize()))
            return; 

        unsigned char* tmpData = tempAlphaDataBuf.Elements();
        PRInt32 stride = mImageSurface->Stride();
        PRInt32 rows = mImageSurface->Height();

        if (mBlurRadius.width > 0) {
            PRInt32 lobes[3][2];
            ComputeLobes(mBlurRadius.width, lobes);
            BoxBlurHorizontal(boxData, tmpData, lobes[0][0], lobes[0][1], stride, rows);
            BoxBlurHorizontal(tmpData, boxData, lobes[1][0], lobes[1][1], stride, rows);
            BoxBlurHorizontal(boxData, tmpData, lobes[2][0], lobes[2][1], stride, rows);
        }

        if (mBlurRadius.height > 0) {
            PRInt32 lobes[3][2];
            ComputeLobes(mBlurRadius.height, lobes);
            BoxBlurVertical(tmpData, boxData, lobes[0][0], lobes[0][1], stride, rows);
            BoxBlurVertical(boxData, tmpData, lobes[1][0], lobes[1][1], stride, rows);
            BoxBlurVertical(tmpData, boxData, lobes[2][0], lobes[2][1], stride, rows);
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


static const gfxFloat GAUSSIAN_SCALE_FACTOR = (3 * sqrt(2 * M_PI) / 4) * (3/2);

gfxIntSize gfxAlphaBoxBlur::CalculateBlurRadius(const gfxPoint& aStd)
{
    return gfxIntSize(
        static_cast<PRInt32>(floor(aStd.x * GAUSSIAN_SCALE_FACTOR + 0.5)),
        static_cast<PRInt32>(floor(aStd.y * GAUSSIAN_SCALE_FACTOR + 0.5)));
}
