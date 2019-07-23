





































#include "nsThebesFontMetrics.h"    
#include "nsFont.h"

#include "nsString.h"
#include <stdio.h>

#include "gfxTextRunCache.h"
#include "gfxPlatform.h"
#include "gfxUserFontSet.h"

NS_IMPL_ISUPPORTS1(nsThebesFontMetrics, nsIFontMetrics)

#include <stdlib.h>

nsThebesFontMetrics::nsThebesFontMetrics()
{
    mFontStyle = nsnull;
    mFontGroup = nsnull;
}

nsThebesFontMetrics::~nsThebesFontMetrics()
{
    if (mDeviceContext)
        mDeviceContext->FontMetricsDeleted(this);
    delete mFontStyle;
    
}

NS_IMETHODIMP
nsThebesFontMetrics::Init(const nsFont& aFont, nsIAtom* aLangGroup,
                          nsIDeviceContext *aContext, 
                          gfxUserFontSet *aUserFontSet)
{
    mFont = aFont;
    mLangGroup = aLangGroup;
    mDeviceContext = (nsThebesDeviceContext*)aContext;
    mP2A = mDeviceContext->AppUnitsPerDevPixel();
    mIsRightToLeft = PR_FALSE;
    mTextRunRTL = PR_FALSE;

    gfxFloat size = gfxFloat(aFont.size) / mP2A;

    nsCString langGroup;
    if (aLangGroup) {
        const char* lg;
        mLangGroup->GetUTF8String(&lg);
        langGroup.Assign(lg);
    }

    PRBool printerFont = mDeviceContext->IsPrinterSurface();
    mFontStyle = new gfxFontStyle(aFont.style, aFont.weight, aFont.stretch,
                                  size, langGroup,
                                  aFont.sizeAdjust, aFont.systemFont,
                                  aFont.familyNameQuirks,
                                  printerFont);

    mFontGroup =
        gfxPlatform::GetPlatform()->CreateFontGroup(aFont.name, mFontStyle, 
                                                    aUserFontSet);
    if (mFontGroup->FontListLength() < 1) 
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::Destroy()
{
    mDeviceContext = nsnull;
    return NS_OK;
}


#define ROUND_TO_TWIPS(x) (nscoord)floor(((x) * mP2A) + 0.5)
#define CEIL_TO_TWIPS(x) (nscoord)NS_ceil((x) * mP2A)

const gfxFont::Metrics& nsThebesFontMetrics::GetMetrics() const
{
    return mFontGroup->GetFontAt(0)->GetMetrics();
}

NS_IMETHODIMP
nsThebesFontMetrics::GetXHeight(nscoord& aResult)
{
    aResult = ROUND_TO_TWIPS(GetMetrics().xHeight);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetSuperscriptOffset(nscoord& aResult)
{
    aResult = ROUND_TO_TWIPS(GetMetrics().superscriptOffset);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetSubscriptOffset(nscoord& aResult)
{
    aResult = ROUND_TO_TWIPS(GetMetrics().subscriptOffset);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
    aOffset = ROUND_TO_TWIPS(GetMetrics().strikeoutOffset);
    aSize = ROUND_TO_TWIPS(GetMetrics().strikeoutSize);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
    aOffset = ROUND_TO_TWIPS(mFontGroup->GetUnderlineOffset());
    aSize = ROUND_TO_TWIPS(GetMetrics().underlineSize);

    return NS_OK;
}






static gfxFloat ComputeMaxDescent(const gfxFont::Metrics& aMetrics,
                                  gfxFontGroup* aFontGroup)
{
    gfxFloat offset = NS_floor(-aFontGroup->GetUnderlineOffset() + 0.5);
    gfxFloat size = NS_round(aMetrics.underlineSize);
    gfxFloat minDescent = NS_floor(offset + size + 0.5);
    return PR_MAX(minDescent, aMetrics.maxDescent);
}

static gfxFloat ComputeMaxAscent(const gfxFont::Metrics& aMetrics)
{
    return NS_floor(aMetrics.maxAscent + 0.5);
}

NS_IMETHODIMP
nsThebesFontMetrics::GetHeight(nscoord &aHeight)
{
    aHeight = CEIL_TO_TWIPS(ComputeMaxAscent(GetMetrics())) +
        CEIL_TO_TWIPS(ComputeMaxDescent(GetMetrics(), mFontGroup));
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetInternalLeading(nscoord &aLeading)
{
    aLeading = ROUND_TO_TWIPS(GetMetrics().internalLeading);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetExternalLeading(nscoord &aLeading)
{
    aLeading = ROUND_TO_TWIPS(GetMetrics().externalLeading);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetEmHeight(nscoord &aHeight)
{
    aHeight = ROUND_TO_TWIPS(GetMetrics().emHeight);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetEmAscent(nscoord &aAscent)
{
    aAscent = ROUND_TO_TWIPS(GetMetrics().emAscent);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetEmDescent(nscoord &aDescent)
{
    aDescent = ROUND_TO_TWIPS(GetMetrics().emDescent);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetMaxHeight(nscoord &aHeight)
{
    aHeight = CEIL_TO_TWIPS(ComputeMaxAscent(GetMetrics())) +
        CEIL_TO_TWIPS(ComputeMaxDescent(GetMetrics(), mFontGroup));
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetMaxAscent(nscoord &aAscent)
{
    aAscent = CEIL_TO_TWIPS(ComputeMaxAscent(GetMetrics()));
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetMaxDescent(nscoord &aDescent)
{
    aDescent = CEIL_TO_TWIPS(ComputeMaxDescent(GetMetrics(), mFontGroup));
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetMaxAdvance(nscoord &aAdvance)
{
    aAdvance = CEIL_TO_TWIPS(GetMetrics().maxAdvance);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetLangGroup(nsIAtom** aLangGroup)
{
    *aLangGroup = mLangGroup;
    NS_IF_ADDREF(*aLangGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetFontHandle(nsFontHandle &aHandle)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetAveCharWidth(nscoord& aAveCharWidth)
{
    
    aAveCharWidth = CEIL_TO_TWIPS(GetMetrics().aveCharWidth);
    return NS_OK;
}

NS_IMETHODIMP
nsThebesFontMetrics::GetSpaceWidth(nscoord& aSpaceCharWidth)
{
    aSpaceCharWidth = CEIL_TO_TWIPS(GetMetrics().spaceWidth);
    return NS_OK;
}

PRInt32
nsThebesFontMetrics::GetMaxStringLength()
{
    const gfxFont::Metrics& m = GetMetrics();
    const double x = 32767.0 / m.maxAdvance;
    PRInt32 len = (PRInt32)floor(x);
    return PR_MAX(1, len);
}

class StubPropertyProvider : public gfxTextRun::PropertyProvider {
public:
    virtual void GetHyphenationBreaks(PRUint32 aStart, PRUint32 aLength,
                                      PRPackedBool* aBreakBefore) {
        NS_ERROR("This shouldn't be called because we never call BreakAndMeasureText");
    }
    virtual gfxFloat GetHyphenWidth() {
        NS_ERROR("This shouldn't be called because we never enable hyphens");
        return 0;
    }
    virtual void GetSpacing(PRUint32 aStart, PRUint32 aLength,
                            Spacing* aSpacing) {
        NS_ERROR("This shouldn't be called because we never enable spacing");
    }
};

nsresult 
nsThebesFontMetrics::GetWidth(const char* aString, PRUint32 aLength, nscoord& aWidth,
                              nsThebesRenderingContext *aContext)
{
    if (aLength == 0) {
        aWidth = 0;
        return NS_OK;
    }

    
    if ((aLength == 1) && (aString[0] == ' '))
        return GetSpaceWidth(aWidth);

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    if (!textRun.get())
        return NS_ERROR_FAILURE;

    aWidth = NSToCoordRound(textRun->GetAdvanceWidth(0, aLength, &provider));

    return NS_OK;
}

nsresult
nsThebesFontMetrics::GetWidth(const PRUnichar* aString, PRUint32 aLength,
                              nscoord& aWidth, PRInt32 *aFontID,
                              nsThebesRenderingContext *aContext)
{
    if (aLength == 0) {
        aWidth = 0;
        return NS_OK;
    }

    
    if ((aLength == 1) && (aString[0] == ' '))
        return GetSpaceWidth(aWidth);

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    if (!textRun.get())
        return NS_ERROR_FAILURE;

    aWidth = NSToCoordRound(textRun->GetAdvanceWidth(0, aLength, &provider));

    return NS_OK;
}


nsresult
nsThebesFontMetrics::GetTextDimensions(const PRUnichar* aString,
                                    PRUint32 aLength,
                                    nsTextDimensions& aDimensions, 
                                    PRInt32* aFontID)
{
    return NS_OK;
}

nsresult
nsThebesFontMetrics::GetTextDimensions(const char*         aString,
                                   PRInt32             aLength,
                                   PRInt32             aAvailWidth,
                                   PRInt32*            aBreaks,
                                   PRInt32             aNumBreaks,
                                   nsTextDimensions&   aDimensions,
                                   PRInt32&            aNumCharsFit,
                                   nsTextDimensions&   aLastWordDimensions,
                                   PRInt32*            aFontID)
{
    return NS_OK;
}
nsresult
nsThebesFontMetrics::GetTextDimensions(const PRUnichar*    aString,
                                   PRInt32             aLength,
                                   PRInt32             aAvailWidth,
                                   PRInt32*            aBreaks,
                                   PRInt32             aNumBreaks,
                                   nsTextDimensions&   aDimensions,
                                   PRInt32&            aNumCharsFit,
                                   nsTextDimensions&   aLastWordDimensions,
                                   PRInt32*            aFontID)
{
    return NS_OK;
}


nsresult
nsThebesFontMetrics::DrawString(const char *aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext)
{
    if (aLength == 0)
        return NS_OK;

    NS_ASSERTION(!aSpacing, "Spacing not supported here");
    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    if (!textRun.get())
        return NS_ERROR_FAILURE;
    gfxPoint pt(aX, aY);
    if (mTextRunRTL) {
        pt.x += textRun->GetAdvanceWidth(0, aLength, &provider);
    }
    textRun->Draw(aContext->ThebesContext(), pt, 0, aLength,
                  nsnull, &provider, nsnull);
    return NS_OK;
}

nsresult
nsThebesFontMetrics::DrawString(const PRUnichar* aString, PRUint32 aLength,
                                nscoord aX, nscoord aY,
                                PRInt32 aFontID,
                                const nscoord* aSpacing,
                                nsThebesRenderingContext *aContext)
{
    if (aLength == 0)
        return NS_OK;

    NS_ASSERTION(!aSpacing, "Spacing not supported here");
    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    if (!textRun.get())
        return NS_ERROR_FAILURE;
    gfxPoint pt(aX, aY);
    if (mTextRunRTL) {
        pt.x += textRun->GetAdvanceWidth(0, aLength, &provider);
    }
    textRun->Draw(aContext->ThebesContext(), pt, 0, aLength,
                  nsnull, &provider, nsnull);
    return NS_OK;
}

#ifdef MOZ_MATHML

static void
GetTextRunBoundingMetrics(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aLength,
                          nsThebesRenderingContext *aContext,
                          nsBoundingMetrics &aBoundingMetrics)
{
    StubPropertyProvider provider;
    gfxTextRun::Metrics theMetrics =
        aTextRun->MeasureText(aStart, aLength, gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS,
                              aContext->ThebesContext(), &provider);
        
        

    aBoundingMetrics.leftBearing = NSToCoordFloor(theMetrics.mBoundingBox.X());
    aBoundingMetrics.rightBearing = NSToCoordCeil(theMetrics.mBoundingBox.XMost());
    aBoundingMetrics.width = NSToCoordRound(theMetrics.mAdvanceWidth);
    aBoundingMetrics.ascent = NSToCoordCeil(- theMetrics.mBoundingBox.Y());
    aBoundingMetrics.descent = NSToCoordCeil(theMetrics.mBoundingBox.YMost());
}

nsresult
nsThebesFontMetrics::GetBoundingMetrics(const char *aString, PRUint32 aLength,
                                        nsThebesRenderingContext *aContext,
                                        nsBoundingMetrics &aBoundingMetrics)
{
    if (aLength == 0) {
        aBoundingMetrics.Clear();
        return NS_OK;
    }

    AutoTextRun textRun(this, aContext, aString, aLength);
    if (!textRun.get())
        return NS_ERROR_FAILURE;

    GetTextRunBoundingMetrics(textRun.get(), 0, aLength, aContext, aBoundingMetrics);
    return NS_OK;
}

nsresult
nsThebesFontMetrics::GetBoundingMetrics(const PRUnichar *aString, PRUint32 aLength,
                                        nsThebesRenderingContext *aContext,
                                        nsBoundingMetrics &aBoundingMetrics)
{
    if (aLength == 0) {
        aBoundingMetrics.Clear();
        return NS_OK;
    }

    AutoTextRun textRun(this, aContext, aString, aLength);
    if (!textRun.get())
        return NS_ERROR_FAILURE;

    GetTextRunBoundingMetrics(textRun.get(), 0, aLength, aContext, aBoundingMetrics);
    return NS_OK;
}

#endif 


nsresult
nsThebesFontMetrics::SetRightToLeftText(PRBool aIsRTL)
{
    mIsRightToLeft = aIsRTL;
    return NS_OK;
}


PRBool
nsThebesFontMetrics::GetRightToLeftText()
{
    return mIsRightToLeft;
}

 gfxUserFontSet*
nsThebesFontMetrics::GetUserFontSet()
{
    return mFontGroup->GetUserFontSet();
}
