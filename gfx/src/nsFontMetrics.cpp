





































#include "nsFontMetrics.h"
#include "nsBoundingMetrics.h"
#include "nsRenderingContext.h"
#include "nsThebesDeviceContext.h"
#include "gfxTextRunCache.h"

namespace {

class AutoTextRun : public gfxTextRunCache::AutoTextRun {
public:
    AutoTextRun(nsFontMetrics* aMetrics, nsRenderingContext* aRC,
                const char* aString, PRInt32 aLength)
        : gfxTextRunCache::AutoTextRun(gfxTextRunCache::MakeTextRun(
            reinterpret_cast<const PRUint8*>(aString), aLength,
            aMetrics->GetThebesFontGroup(), aRC->ThebesContext(),
            aMetrics->AppUnitsPerDevPixel(),
            ComputeFlags(aMetrics)))
    {}

    AutoTextRun(nsFontMetrics* aMetrics, nsRenderingContext* aRC,
                const PRUnichar* aString, PRInt32 aLength)
        : gfxTextRunCache::AutoTextRun(gfxTextRunCache::MakeTextRun(
            aString, aLength, aMetrics->GetThebesFontGroup(),
            aRC->ThebesContext(),
            aMetrics->AppUnitsPerDevPixel(),
            ComputeFlags(aMetrics)))
    {}

private:
    static PRUint32 ComputeFlags(nsFontMetrics* aMetrics) {
        PRUint32 flags = 0;
        if (aMetrics->GetTextRunRTL()) {
            flags |= gfxTextRunFactory::TEXT_IS_RTL;
        }
        return flags;
    }
};

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

} 

nsFontMetrics::nsFontMetrics()
    : mDeviceContext(nsnull), mP2A(-1), mTextRunRTL(PR_FALSE)
{
}

nsFontMetrics::~nsFontMetrics()
{
    if (mDeviceContext)
        mDeviceContext->FontMetricsDeleted(this);
}

nsresult
nsFontMetrics::Init(const nsFont& aFont, nsIAtom* aLanguage,
                    nsIDeviceContext *aContext,
                    gfxUserFontSet *aUserFontSet)
{
    NS_ABORT_IF_FALSE(mP2A == -1, "already initialized");

    mFont = aFont;
    mLanguage = aLanguage;
    mDeviceContext = (nsThebesDeviceContext*)aContext;
    mP2A = mDeviceContext->AppUnitsPerDevPixel();

    gfxFontStyle style(aFont.style,
                       aFont.weight,
                       aFont.stretch,
                       gfxFloat(aFont.size) / mP2A,
                       aLanguage,
                       aFont.sizeAdjust,
                       aFont.systemFont,
                       mDeviceContext->IsPrinterSurface(),
                       aFont.featureSettings,
                       aFont.languageOverride);

    mFontGroup = gfxPlatform::GetPlatform()->
        CreateFontGroup(aFont.name, &style, aUserFontSet);
    if (mFontGroup->FontListLength() < 1)
        return NS_ERROR_UNEXPECTED;

    return NS_OK;
}

void
nsFontMetrics::Destroy()
{
    mDeviceContext = nsnull;
}


#define ROUND_TO_TWIPS(x) (nscoord)floor(((x) * mP2A) + 0.5)
#define CEIL_TO_TWIPS(x) (nscoord)NS_ceil((x) * mP2A)

const gfxFont::Metrics& nsFontMetrics::GetMetrics() const
{
    return mFontGroup->GetFontAt(0)->GetMetrics();
}

nscoord
nsFontMetrics::XHeight()
{
    return ROUND_TO_TWIPS(GetMetrics().xHeight);
}

nscoord
nsFontMetrics::SuperscriptOffset()
{
    return ROUND_TO_TWIPS(GetMetrics().superscriptOffset);
}

nscoord
nsFontMetrics::SubscriptOffset()
{
    return ROUND_TO_TWIPS(GetMetrics().subscriptOffset);
}

void
nsFontMetrics::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
    aOffset = ROUND_TO_TWIPS(GetMetrics().strikeoutOffset);
    aSize = ROUND_TO_TWIPS(GetMetrics().strikeoutSize);
}

void
nsFontMetrics::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
    aOffset = ROUND_TO_TWIPS(mFontGroup->GetUnderlineOffset());
    aSize = ROUND_TO_TWIPS(GetMetrics().underlineSize);
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

nscoord
nsFontMetrics::InternalLeading()
{
    return ROUND_TO_TWIPS(GetMetrics().internalLeading);
}

nscoord
nsFontMetrics::ExternalLeading()
{
    return ROUND_TO_TWIPS(GetMetrics().externalLeading);
}

nscoord
nsFontMetrics::EmHeight()
{
    return ROUND_TO_TWIPS(GetMetrics().emHeight);
}

nscoord
nsFontMetrics::EmAscent()
{
    return ROUND_TO_TWIPS(GetMetrics().emAscent);
}

nscoord
nsFontMetrics::EmDescent()
{
    return ROUND_TO_TWIPS(GetMetrics().emDescent);
}

nscoord
nsFontMetrics::MaxHeight()
{
    return CEIL_TO_TWIPS(ComputeMaxAscent(GetMetrics())) +
        CEIL_TO_TWIPS(ComputeMaxDescent(GetMetrics(), mFontGroup));
}

nscoord
nsFontMetrics::MaxAscent()
{
    return CEIL_TO_TWIPS(ComputeMaxAscent(GetMetrics()));
}

nscoord
nsFontMetrics::MaxDescent()
{
    return CEIL_TO_TWIPS(ComputeMaxDescent(GetMetrics(), mFontGroup));
}

nscoord
nsFontMetrics::MaxAdvance()
{
    return CEIL_TO_TWIPS(GetMetrics().maxAdvance);
}

nscoord
nsFontMetrics::AveCharWidth()
{
    
    return CEIL_TO_TWIPS(GetMetrics().aveCharWidth);
}

nscoord
nsFontMetrics::SpaceWidth()
{
    return CEIL_TO_TWIPS(GetMetrics().spaceWidth);
}

already_AddRefed<nsIAtom>
nsFontMetrics::GetLanguage()
{
    nsIAtom* result = mLanguage.get();
    NS_IF_ADDREF(result);
    return result;
}

PRInt32
nsFontMetrics::GetMaxStringLength()
{
    const gfxFont::Metrics& m = GetMetrics();
    const double x = 32767.0 / m.maxAdvance;
    PRInt32 len = (PRInt32)floor(x);
    return PR_MAX(1, len);
}

nscoord
nsFontMetrics::GetWidth(const char* aString, PRUint32 aLength,
                        nsRenderingContext *aContext)
{
    if (aLength == 0)
        return 0;

    if (aLength == 1 && aString[0] == ' ')
        return SpaceWidth();

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    return NSToCoordRound(textRun->GetAdvanceWidth(0, aLength, &provider));
}

nscoord
nsFontMetrics::GetWidth(const PRUnichar* aString, PRUint32 aLength,
                        nsRenderingContext *aContext)
{
    if (aLength == 0)
        return 0;

    if (aLength == 1 && aString[0] == ' ')
        return SpaceWidth();

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    return NSToCoordRound(textRun->GetAdvanceWidth(0, aLength, &provider));
}


void
nsFontMetrics::DrawString(const char *aString, PRUint32 aLength,
                          nscoord aX, nscoord aY,
                          nsRenderingContext *aContext)
{
    if (aLength == 0)
        return;

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    gfxPoint pt(aX, aY);
    if (mTextRunRTL) {
        pt.x += textRun->GetAdvanceWidth(0, aLength, &provider);
    }
    textRun->Draw(aContext->ThebesContext(), pt, 0, aLength, &provider, nsnull);
}

void
nsFontMetrics::DrawString(const PRUnichar* aString, PRUint32 aLength,
                          nscoord aX, nscoord aY,
                          nsRenderingContext *aContext,
                          nsRenderingContext *aTextRunConstructionContext)
{
    if (aLength == 0)
        return;

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aTextRunConstructionContext, aString, aLength);
    gfxPoint pt(aX, aY);
    if (mTextRunRTL) {
        pt.x += textRun->GetAdvanceWidth(0, aLength, &provider);
    }
    textRun->Draw(aContext->ThebesContext(), pt, 0, aLength, &provider, nsnull);
}

#ifdef MOZ_MATHML
nsBoundingMetrics
nsFontMetrics::GetBoundingMetrics(const PRUnichar *aString, PRUint32 aLength,
                                  nsRenderingContext *aContext)
{
    if (aLength == 0)
        return nsBoundingMetrics();

    StubPropertyProvider provider;
    AutoTextRun textRun(this, aContext, aString, aLength);
    gfxTextRun::Metrics theMetrics =
        textRun->MeasureText(0, aLength,
                             gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS,
                             aContext->ThebesContext(), &provider);

    nsBoundingMetrics m;
    m.leftBearing  = NSToCoordFloor( theMetrics.mBoundingBox.X());
    m.rightBearing = NSToCoordCeil(  theMetrics.mBoundingBox.XMost());
    m.ascent       = NSToCoordCeil( -theMetrics.mBoundingBox.Y());
    m.descent      = NSToCoordCeil(  theMetrics.mBoundingBox.YMost());
    m.width        = NSToCoordRound( theMetrics.mAdvanceWidth);
    return m;
}
#endif 
