






































#include "nsRenderingContext.h"


#define FROM_TWIPS(_x)  ((gfxFloat)((_x)/(mP2A)))
#define FROM_TWIPS_INT(_x)  (NSToIntRound((gfxFloat)((_x)/(mP2A))))
#define TO_TWIPS(_x)    ((nscoord)((_x)*(mP2A)))
#define GFX_RECT_FROM_TWIPS_RECT(_r)   (gfxRect(FROM_TWIPS((_r).x), FROM_TWIPS((_r).y), FROM_TWIPS((_r).width), FROM_TWIPS((_r).height)))



#define MAX_GFX_TEXT_BUF_SIZE 8000

static PRInt32 FindSafeLength(const PRUnichar *aString, PRUint32 aLength,
                              PRUint32 aMaxChunkLength)
{
    if (aLength <= aMaxChunkLength)
        return aLength;

    PRInt32 len = aMaxChunkLength;

    
    while (len > 0 && NS_IS_LOW_SURROGATE(aString[len])) {
        len--;
    }
    if (len == 0) {
        
        
        
        
        
        return aMaxChunkLength;
    }
    return len;
}

static PRInt32 FindSafeLength(const char *aString, PRUint32 aLength,
                              PRUint32 aMaxChunkLength)
{
    
    return PR_MIN(aLength, aMaxChunkLength);
}




void
nsRenderingContext::Init(nsIDeviceContext* aContext,
                         gfxASurface *aThebesSurface)
{
    Init(aContext, new gfxContext(aThebesSurface));
}

void
nsRenderingContext::Init(nsIDeviceContext* aContext,
                         gfxContext *aThebesContext)
{
    mDeviceContext = aContext;
    mThebes = aThebesContext;

    mThebes->SetLineWidth(1.0);
    mP2A = mDeviceContext->AppUnitsPerDevPixel();
}

already_AddRefed<nsIDeviceContext>
nsRenderingContext::GetDeviceContext()
{
    NS_IF_ADDREF(mDeviceContext);
    return mDeviceContext.get();
}

void
nsRenderingContext::PushState()
{
    mThebes->Save();
}

void
nsRenderingContext::PopState()
{
    mThebes->Restore();
}





void
nsRenderingContext::SetClipRect(const nsRect& aRect, nsClipCombine aCombine)
{
    if (aCombine == nsClipCombine_kReplace) {
        mThebes->ResetClip();
    } else if (aCombine != nsClipCombine_kIntersect) {
        NS_WARNING("Unexpected usage of SetClipRect");
    }

    mThebes->NewPath();
    gfxRect clipRect(GFX_RECT_FROM_TWIPS_RECT(aRect));
    if (mThebes->UserToDevicePixelSnapped(clipRect, PR_TRUE)) {
        gfxMatrix mat(mThebes->CurrentMatrix());
        mThebes->IdentityMatrix();
        mThebes->Rectangle(clipRect);
        mThebes->SetMatrix(mat);
    } else {
        mThebes->Rectangle(clipRect);
    }

    mThebes->Clip();
}

void
nsRenderingContext::SetClipRegion(const nsIntRegion& aRegion,
                                  nsClipCombine aCombine)
{
    
    
    
    
    NS_ASSERTION(aCombine == nsClipCombine_kReplace,
                 "Unexpected usage of SetClipRegion");

    gfxMatrix mat = mThebes->CurrentMatrix();
    mThebes->IdentityMatrix();

    mThebes->ResetClip();

    mThebes->NewPath();
    nsIntRegionRectIterator iter(aRegion);
    const nsIntRect* rect;
    while ((rect = iter.Next())) {
        mThebes->Rectangle(gfxRect(rect->x, rect->y, rect->width, rect->height),
                           PR_TRUE);
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
nsRenderingContext::Translate(const nsPoint& aPt)
{
    mThebes->Translate(gfxPoint(FROM_TWIPS(aPt.x), FROM_TWIPS(aPt.y)));
}

void
nsRenderingContext::Scale(float aSx, float aSy)
{
    mThebes->Scale(aSx, aSy);
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
    mThebes->Rectangle(GFX_RECT_FROM_TWIPS_RECT(aRect), PR_TRUE);
    mThebes->Stroke();
}

void
nsRenderingContext::DrawRect(nscoord aX, nscoord aY,
                             nscoord aWidth, nscoord aHeight)
{
    DrawRect(nsRect(aX, aY, aWidth, aHeight));
}





























#define CAIRO_COORD_MAX (double(0x7fffff))

static PRBool
ConditionRect(gfxRect& r) {
    
    
    if (r.pos.x > CAIRO_COORD_MAX || r.pos.y > CAIRO_COORD_MAX)
        return PR_FALSE;

    if (r.pos.x < 0.0) {
        r.size.width += r.pos.x;
        if (r.size.width < 0.0)
            return PR_FALSE;
        r.pos.x = 0.0;
    }

    if (r.pos.x + r.size.width > CAIRO_COORD_MAX) {
        r.size.width = CAIRO_COORD_MAX - r.pos.x;
    }

    if (r.pos.y < 0.0) {
        r.size.height += r.pos.y;
        if (r.size.height < 0.0)
            return PR_FALSE;

        r.pos.y = 0.0;
    }

    if (r.pos.y + r.size.height > CAIRO_COORD_MAX) {
        r.size.height = CAIRO_COORD_MAX - r.pos.y;
    }
    return PR_TRUE;
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

        mThebes->Rectangle(r, PR_TRUE);
        mThebes->Fill();
        mThebes->SetMatrix(mat);
    }

    mThebes->NewPath();
    mThebes->Rectangle(r, PR_TRUE);
    mThebes->Fill();
}

void
nsRenderingContext::FillRect(nscoord aX, nscoord aY,
                             nscoord aWidth, nscoord aHeight)
{
    FillRect(nsRect(aX, aY, aWidth, aHeight));
}

void
nsRenderingContext::InvertRect(const nsRect& aRect)
{
    gfxContext::GraphicsOperator lastOp = mThebes->CurrentOperator();

    mThebes->SetOperator(gfxContext::OPERATOR_XOR);
    FillRect(aRect);
    mThebes->SetOperator(lastOp);
}

void
nsRenderingContext::InvertRect(nscoord aX, nscoord aY,
                               nscoord aWidth, nscoord aHeight)
{
    InvertRect(nsRect(aX, aY, aWidth, aHeight));
}

void
nsRenderingContext::DrawEllipse(const nsRect& aRect)
{
    DrawEllipse(aRect.x, aRect.y, aRect.width, aRect.height);
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
nsRenderingContext::FillPolygon(const nsPoint twPoints[], PRInt32 aNumPoints)
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
nsRenderingContext::SetRightToLeftText(PRBool aIsRTL)
{
    mFontMetrics->SetRightToLeftText(aIsRTL);
}

void
nsRenderingContext::SetTextRunRTL(PRBool aIsRTL)
{
    mFontMetrics->SetTextRunRTL(aIsRTL);
}

void
nsRenderingContext::SetFont(const nsFont& aFont, nsIAtom* aLanguage,
                            gfxUserFontSet *aUserFontSet)
{
    nsCOMPtr<nsIFontMetrics> newMetrics;
    mDeviceContext->GetMetricsFor(aFont, aLanguage, aUserFontSet,
                                  *getter_AddRefs(newMetrics));
    mFontMetrics = reinterpret_cast<nsIThebesFontMetrics*>(newMetrics.get());
}

void
nsRenderingContext::SetFont(const nsFont& aFont,
                            gfxUserFontSet *aUserFontSet)
{
    nsCOMPtr<nsIFontMetrics> newMetrics;
    mDeviceContext->GetMetricsFor(aFont, nsnull, aUserFontSet,
                                  *getter_AddRefs(newMetrics));
    mFontMetrics = reinterpret_cast<nsIThebesFontMetrics*>(newMetrics.get());
}

void
nsRenderingContext::SetFont(nsIFontMetrics *aFontMetrics)
{
    mFontMetrics = static_cast<nsIThebesFontMetrics*>(aFontMetrics);
}

already_AddRefed<nsIFontMetrics>
nsRenderingContext::GetFontMetrics()
{
    NS_IF_ADDREF(mFontMetrics);
    return mFontMetrics.get();
}

PRInt32
nsRenderingContext::GetMaxChunkLength()
{
    if (!mFontMetrics)
        return 1;
    return PR_MIN(mFontMetrics->GetMaxStringLength(), MAX_GFX_TEXT_BUF_SIZE);
}

nscoord
nsRenderingContext::GetWidth(char aC)
{
    if (aC == ' ' && mFontMetrics) {
        nscoord width;
        mFontMetrics->GetSpaceWidth(width);
        return width;
    }

    return GetWidth(&aC, 1);
}

nscoord
nsRenderingContext::GetWidth(PRUnichar aC)
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
nsRenderingContext::GetWidth(const char* aString, PRUint32 aLength)
{
    PRUint32 maxChunkLength = GetMaxChunkLength();
    nscoord width = 0;
    while (aLength > 0) {
        PRInt32 len = FindSafeLength(aString, aLength, maxChunkLength);
        width += GetWidthInternal(aString, len);
        aLength -= len;
        aString += len;
    }
    return width;
}

nscoord
nsRenderingContext::GetWidth(const PRUnichar *aString, PRUint32 aLength)
{
    PRUint32 maxChunkLength = GetMaxChunkLength();
    nscoord width = 0;
    while (aLength > 0) {
        PRInt32 len = FindSafeLength(aString, aLength, maxChunkLength);
        width += GetWidthInternal(aString, len);
        aLength -= len;
        aString += len;
    }
    return width;
}

#ifdef MOZ_MATHML
nsBoundingMetrics
nsRenderingContext::GetBoundingMetrics(const PRUnichar* aString,
                                       PRUint32 aLength)
{
    PRUint32 maxChunkLength = GetMaxChunkLength();
    PRInt32 len = FindSafeLength(aString, aLength, maxChunkLength);
    
    
    
    nsBoundingMetrics totalMetrics;
    mFontMetrics->GetBoundingMetrics(aString, len, this, totalMetrics);
    aLength -= len;
    aString += len;

    while (aLength > 0) {
        len = FindSafeLength(aString, aLength, maxChunkLength);
        nsBoundingMetrics metrics;
        mFontMetrics->GetBoundingMetrics(aString, len, this, metrics);
        totalMetrics += metrics;
        aLength -= len;
        aString += len;
    }
    return totalMetrics;
}
#endif

void
nsRenderingContext::DrawString(const char *aString, PRUint32 aLength,
                               nscoord aX, nscoord aY)
{
    PRUint32 maxChunkLength = GetMaxChunkLength();
    while (aLength > 0) {
        PRInt32 len = FindSafeLength(aString, aLength, maxChunkLength);
        mFontMetrics->DrawString(aString, len, aX, aY, nsnull, this);
        aLength -= len;

        if (aLength > 0) {
            nscoord width = GetWidthInternal(aString, len);
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
nsRenderingContext::DrawString(const PRUnichar *aString, PRUint32 aLength,
                               nscoord aX, nscoord aY)
{
    PRUint32 maxChunkLength = GetMaxChunkLength();
    if (aLength <= maxChunkLength) {
        mFontMetrics->DrawString(aString, aLength, aX, aY, this, this);
        return;
    }

    PRBool isRTL = mFontMetrics->GetRightToLeftText();

    
    if (isRTL) {
        aX += GetWidth(aString, aLength);
    }

    while (aLength > 0) {
        PRInt32 len = FindSafeLength(aString, aLength, maxChunkLength);
        nscoord width = GetWidthInternal(aString, len);
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

nscoord
nsRenderingContext::GetWidthInternal(const char* aString, PRUint32 aLength)
{
    if (aLength == 0) {
        return 0;
    }

    nscoord width;
    mFontMetrics->GetWidth(aString, aLength, width, this);
    return width;
}

nscoord
nsRenderingContext::GetWidthInternal(const PRUnichar *aString, PRUint32 aLength)
{
    if (aLength == 0) {
        return 0;
    }

    nscoord width;
    mFontMetrics->GetWidth(aString, aLength, width, nsnull, this);
    return width;
}
