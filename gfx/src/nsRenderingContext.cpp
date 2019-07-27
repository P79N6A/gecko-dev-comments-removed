




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


#define FROM_TWIPS(_x)  ((gfxFloat)((_x)/(mP2A)))
#define FROM_TWIPS_INT(_x)  (NSToIntRound((gfxFloat)((_x)/(mP2A))))
#define TO_TWIPS(_x)    ((nscoord)((_x)*(mP2A)))
#define GFX_RECT_FROM_TWIPS_RECT(_r)   (gfxRect(FROM_TWIPS((_r).x), FROM_TWIPS((_r).y), FROM_TWIPS((_r).width), FROM_TWIPS((_r).height)))



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
nsRenderingContext::Init(nsDeviceContext* aContext,
                         gfxContext *aThebesContext)
{
    mDeviceContext = aContext;
    mThebes = aThebesContext;

    mThebes->SetLineWidth(1.0);
    mP2A = mDeviceContext->AppUnitsPerDevPixel();
}

void
nsRenderingContext::Init(nsDeviceContext* aContext,
                         DrawTarget *aDrawTarget)
{
    Init(aContext, new gfxContext(aDrawTarget));
}





void
nsRenderingContext::IntersectClip(const nsRect& aRect)
{
    mThebes->NewPath();
    gfxRect clipRect(GFX_RECT_FROM_TWIPS_RECT(aRect));
    if (mThebes->UserToDevicePixelSnapped(clipRect, true)) {
        gfxMatrix mat(mThebes->CurrentMatrix());
        mat.Invert();
        clipRect = mat.Transform(clipRect);
        mThebes->Rectangle(clipRect);
    } else {
        mThebes->Rectangle(clipRect);
    }

    mThebes->Clip();
}

void
nsRenderingContext::SetClip(const nsIntRegion& aRegion)
{
    
    
    
    
    

    gfxMatrix mat = mThebes->CurrentMatrix();
    mThebes->IdentityMatrix();

    mThebes->ResetClip();

    mThebes->NewPath();
    nsIntRegionRectIterator iter(aRegion);
    const nsIntRect* rect;
    while ((rect = iter.Next())) {
        mThebes->Rectangle(gfxRect(rect->x, rect->y, rect->width, rect->height),
                           true);
    }
    mThebes->Clip();
    mThebes->SetMatrix(mat);
}

void
nsRenderingContext::SetLineStyle(nsLineStyle aLineStyle)
{
    switch (aLineStyle) {
        case nsLineStyle_kSolid:
            mThebes->SetDash(gfxContext::gfxLineSolid);
            break;
        case nsLineStyle_kDashed:
            mThebes->SetDash(gfxContext::gfxLineDashed);
            break;
        case nsLineStyle_kDotted:
            mThebes->SetDash(gfxContext::gfxLineDotted);
            break;
        case nsLineStyle_kNone:
        default:
            
            NS_ERROR("SetLineStyle: Invalid line style");
            break;
    }
}


void
nsRenderingContext::SetColor(nscolor aColor)
{
    


    mThebes->SetColor(gfxRGBA(aColor));
}





void
nsRenderingContext::DrawLine(const nsPoint& aStartPt, const nsPoint& aEndPt)
{
    DrawLine(aStartPt.x, aStartPt.y, aEndPt.x, aEndPt.y);
}

void
nsRenderingContext::DrawLine(nscoord aX0, nscoord aY0,
                             nscoord aX1, nscoord aY1)
{
    gfxPoint p0 = gfxPoint(FROM_TWIPS(aX0), FROM_TWIPS(aY0));
    gfxPoint p1 = gfxPoint(FROM_TWIPS(aX1), FROM_TWIPS(aY1));

    
    
    gfxMatrix savedMatrix = mThebes->CurrentMatrix();
    if (!savedMatrix.HasNonTranslation()) {
        p0 = mThebes->UserToDevice(p0);
        p1 = mThebes->UserToDevice(p1);

        p0.Round();
        p1.Round();

        mThebes->IdentityMatrix();

        mThebes->NewPath();

        
        if (p0.x == p1.x) {
            mThebes->Line(p0 + gfxPoint(0.5, 0),
                          p1 + gfxPoint(0.5, 0));
        } else if (p0.y == p1.y) {
            mThebes->Line(p0 + gfxPoint(0, 0.5),
                          p1 + gfxPoint(0, 0.5));
        } else {
            mThebes->Line(p0, p1);
        }

        mThebes->Stroke();

        mThebes->SetMatrix(savedMatrix);
    } else {
        mThebes->NewPath();
        mThebes->Line(p0, p1);
        mThebes->Stroke();
    }
}

void
nsRenderingContext::DrawRect(const nsRect& aRect)
{
    mThebes->NewPath();
    mThebes->Rectangle(GFX_RECT_FROM_TWIPS_RECT(aRect), true);
    mThebes->Stroke();
}

void
nsRenderingContext::DrawRect(nscoord aX, nscoord aY,
                             nscoord aWidth, nscoord aHeight)
{
    DrawRect(nsRect(aX, aY, aWidth, aHeight));
}





























#define CAIRO_COORD_MAX (double(0x7fffff))

static bool
ConditionRect(gfxRect& r) {
    
    
    if (r.X() > CAIRO_COORD_MAX || r.Y() > CAIRO_COORD_MAX)
        return false;

    if (r.X() < 0.0) {
        r.width += r.X();
        if (r.width < 0.0)
            return false;
        r.x = 0.0;
    }

    if (r.XMost() > CAIRO_COORD_MAX) {
        r.width = CAIRO_COORD_MAX - r.X();
    }

    if (r.Y() < 0.0) {
        r.height += r.Y();
        if (r.Height() < 0.0)
            return false;

        r.y = 0.0;
    }

    if (r.YMost() > CAIRO_COORD_MAX) {
        r.height = CAIRO_COORD_MAX - r.Y();
    }
    return true;
}

void
nsRenderingContext::FillRect(const nsRect& aRect)
{
    gfxRect r(GFX_RECT_FROM_TWIPS_RECT(aRect));

    
    nscoord bigval = (nscoord)(CAIRO_COORD_MAX*mP2A);
    if (aRect.width > bigval ||
        aRect.height > bigval ||
        aRect.x < -bigval ||
        aRect.x > bigval ||
        aRect.y < -bigval ||
        aRect.y > bigval)
    {
        gfxMatrix mat = mThebes->CurrentMatrix();

        r = mat.Transform(r);

        if (!ConditionRect(r))
            return;

        mThebes->IdentityMatrix();
        mThebes->NewPath();

        mThebes->Rectangle(r, true);
        mThebes->Fill();
        mThebes->SetMatrix(mat);
    }

    mThebes->NewPath();
    mThebes->Rectangle(r, true);
    mThebes->Fill();
}

void
nsRenderingContext::FillRect(nscoord aX, nscoord aY,
                             nscoord aWidth, nscoord aHeight)
{
    FillRect(nsRect(aX, aY, aWidth, aHeight));
}

void
nsRenderingContext::DrawEllipse(nscoord aX, nscoord aY,
                                nscoord aWidth, nscoord aHeight)
{
    mThebes->NewPath();
    mThebes->Ellipse(gfxPoint(FROM_TWIPS(aX) + FROM_TWIPS(aWidth)/2.0,
                              FROM_TWIPS(aY) + FROM_TWIPS(aHeight)/2.0),
                     gfxSize(FROM_TWIPS(aWidth),
                             FROM_TWIPS(aHeight)));
    mThebes->Stroke();
}

void
nsRenderingContext::FillEllipse(const nsRect& aRect)
{
    FillEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
}

void
nsRenderingContext::FillEllipse(nscoord aX, nscoord aY,
                                nscoord aWidth, nscoord aHeight)
{
    mThebes->NewPath();
    mThebes->Ellipse(gfxPoint(FROM_TWIPS(aX) + FROM_TWIPS(aWidth)/2.0,
                              FROM_TWIPS(aY) + FROM_TWIPS(aHeight)/2.0),
                     gfxSize(FROM_TWIPS(aWidth),
                             FROM_TWIPS(aHeight)));
    mThebes->Fill();
}

void
nsRenderingContext::FillPolygon(const nsPoint twPoints[], int32_t aNumPoints)
{
    if (aNumPoints == 0)
        return;

    nsAutoArrayPtr<gfxPoint> pxPoints(new gfxPoint[aNumPoints]);

    for (int i = 0; i < aNumPoints; i++) {
        pxPoints[i].x = FROM_TWIPS(twPoints[i].x);
        pxPoints[i].y = FROM_TWIPS(twPoints[i].y);
    }

    mThebes->NewPath();
    mThebes->Polygon(pxPoints, aNumPoints);
    mThebes->Fill();
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
    if (!mFontMetrics)
        return 1;
    return std::min(mFontMetrics->GetMaxStringLength(), MAX_GFX_TEXT_BUF_SIZE);
}

nscoord
nsRenderingContext::GetWidth(char aC)
{
    if (aC == ' ' && mFontMetrics) {
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
