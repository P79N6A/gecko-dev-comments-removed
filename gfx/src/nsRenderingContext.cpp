




#include "nsRenderingContext.h"
#include <string.h>                     
#include <algorithm>                    
#include "gfxColor.h"                   
#include "gfxMatrix.h"                  
#include "gfxPoint.h"                   
#include "gfxRect.h"                    
#include "gfxTypes.h"                   
#include "mozilla/gfx/BasePoint.h"      
#include "mozilla/mozalloc.h"           
#include "nsBoundingMetrics.h"          
#include "nsCharTraits.h"               
#include "nsDebug.h"                    
#include "nsPoint.h"                    
#include "nsRect.h"                     
#include "nsRegion.h"                   



#define MAX_GFX_TEXT_BUF_SIZE 8000

static int32_t FindSafeLength(const char16_t *aString, uint32_t aLength,
                              uint32_t aMaxChunkLength)
{
    if (aLength <= aMaxChunkLength)
        return aLength;

    int32_t len = aMaxChunkLength;

    
    while (len > 0 && NS_IS_LOW_SURROGATE(aString[len])) {
        len--;
    }
    if (len == 0) {
        
        
        
        
        
        return aMaxChunkLength;
    }
    return len;
}

static int32_t FindSafeLength(const char *aString, uint32_t aLength,
                              uint32_t aMaxChunkLength)
{
    
    return std::min(aLength, aMaxChunkLength);
}




void
nsRenderingContext::Init(gfxContext *aThebesContext)
{
    mThebes = aThebesContext;
    mThebes->SetLineWidth(1.0);
}

void
nsRenderingContext::Init(DrawTarget *aDrawTarget)
{
    Init(new gfxContext(aDrawTarget));
}






void
nsRenderingContext::SetTextRunRTL(bool aIsRTL)
{
    mFontMetrics->SetTextRunRTL(aIsRTL);
}

void
nsRenderingContext::SetFont(nsFontMetrics *aFontMetrics)
{
    mFontMetrics = aFontMetrics;
}

int32_t
nsRenderingContext::GetMaxChunkLength()
{
    return std::min(mFontMetrics->GetMaxStringLength(), MAX_GFX_TEXT_BUF_SIZE);
}

nscoord
nsRenderingContext::GetWidth(char aC)
{
    if (aC == ' ') {
        return mFontMetrics->SpaceWidth();
    }

    return GetWidth(&aC, 1);
}

nscoord
nsRenderingContext::GetWidth(char16_t aC)
{
    return GetWidth(&aC, 1);
}

nscoord
nsRenderingContext::GetWidth(const nsString& aString)
{
    return GetWidth(aString.get(), aString.Length());
}

nscoord
nsRenderingContext::GetWidth(const char* aString)
{
    return GetWidth(aString, strlen(aString));
}

nscoord
nsRenderingContext::GetWidth(const char* aString, uint32_t aLength)
{
    uint32_t maxChunkLength = GetMaxChunkLength();
    nscoord width = 0;
    while (aLength > 0) {
        int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
        width += mFontMetrics->GetWidth(aString, len, this);
        aLength -= len;
        aString += len;
    }
    return width;
}

nscoord
nsRenderingContext::GetWidth(const char16_t *aString, uint32_t aLength)
{
    uint32_t maxChunkLength = GetMaxChunkLength();
    nscoord width = 0;
    while (aLength > 0) {
        int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
        width += mFontMetrics->GetWidth(aString, len, this);
        aLength -= len;
        aString += len;
    }
    return width;
}

nsBoundingMetrics
nsRenderingContext::GetBoundingMetrics(const char16_t* aString,
                                       uint32_t aLength)
{
    uint32_t maxChunkLength = GetMaxChunkLength();
    int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
    
    
    
    nsBoundingMetrics totalMetrics
        = mFontMetrics->GetBoundingMetrics(aString, len, this);
    aLength -= len;
    aString += len;

    while (aLength > 0) {
        len = FindSafeLength(aString, aLength, maxChunkLength);
        nsBoundingMetrics metrics
            = mFontMetrics->GetBoundingMetrics(aString, len, this);
        totalMetrics += metrics;
        aLength -= len;
        aString += len;
    }
    return totalMetrics;
}

void
nsRenderingContext::DrawString(const char *aString, uint32_t aLength,
                               nscoord aX, nscoord aY)
{
    uint32_t maxChunkLength = GetMaxChunkLength();
    while (aLength > 0) {
        int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
        mFontMetrics->DrawString(aString, len, aX, aY, this);
        aLength -= len;

        if (aLength > 0) {
            nscoord width = mFontMetrics->GetWidth(aString, len, this);
            aX += width;
            aString += len;
        }
    }
}

void
nsRenderingContext::DrawString(const nsString& aString, nscoord aX, nscoord aY)
{
    DrawString(aString.get(), aString.Length(), aX, aY);
}

void
nsRenderingContext::DrawString(const char16_t *aString, uint32_t aLength,
                               nscoord aX, nscoord aY)
{
    uint32_t maxChunkLength = GetMaxChunkLength();
    if (aLength <= maxChunkLength) {
        mFontMetrics->DrawString(aString, aLength, aX, aY, this, this);
        return;
    }

    bool isRTL = mFontMetrics->GetTextRunRTL();

    
    if (isRTL) {
        aX += GetWidth(aString, aLength);
    }

    while (aLength > 0) {
        int32_t len = FindSafeLength(aString, aLength, maxChunkLength);
        nscoord width = mFontMetrics->GetWidth(aString, len, this);
        if (isRTL) {
            aX -= width;
        }
        mFontMetrics->DrawString(aString, len, aX, aY, this, this);
        if (!isRTL) {
            aX += width;
        }
        aLength -= len;
        aString += len;
    }
}
