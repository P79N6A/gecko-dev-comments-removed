











































#include "gfxPlatformQt.h"
#include "gfxTypes.h"
#include "gfxQtFonts.h"
#include "qdebug.h"





static int
FFRECountHyphens (const nsAString &aFFREName)
{
    int h = 0;
    PRInt32 hyphen = 0;
    while ((hyphen = aFFREName.FindChar('-', hyphen)) >= 0) {
        ++h;
        ++hyphen;
    }
    return h;
}

PRBool
gfxQtFontGroup::FontCallback (const nsAString& fontName,
                                 const nsACString& genericName,
                                 void *closure)
{
    qDebug(">>>>>>Func:%s::%d, fontname:%s, genName:%s\n", __PRETTY_FUNCTION__, __LINE__, NS_ConvertUTF16toUTF8(fontName).get(), nsCString(genericName).get());
    nsStringArray *sa = static_cast<nsStringArray*>(closure);

    
    if (genericName.Length() && FFRECountHyphens(fontName) >= 3)
        return PR_TRUE;

    if (sa->IndexOf(fontName) < 0) {
        sa->AppendString(fontName);
    }

    return PR_TRUE;
}






static already_AddRefed<gfxQtFont>
GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle)
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(aName, aStyle);
    if (!font) {
        font = new gfxQtFont(aName, aStyle);
        if (!font)
            return nsnull;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return static_cast<gfxQtFont *>(f);
}


gfxQtFontGroup::gfxQtFontGroup (const nsAString& families,
                                const gfxFontStyle *aStyle)
    : gfxFontGroup(families, aStyle)
{
    qDebug("Func:%s::%d, fam:%s\n\tNeed to create font metrics, otherwise - CRASH\n\n", __PRETTY_FUNCTION__, __LINE__, NS_ConvertUTF16toUTF8(families).get());
    nsStringArray familyArray;

    
    
    ForEachFontInternal(families, aStyle->langGroup, PR_TRUE, PR_FALSE,
                        FontCallback, &familyArray);

    
    nsAutoString fcFamilies;
    if (familyArray.Count()) {
        qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
        int i = 0;
        while (1) {
            fcFamilies.Append(*familyArray[i]);
            ++i;
            if (i >= familyArray.Count())
                break;
            fcFamilies.Append(NS_LITERAL_STRING(","));
        }
    }
    else {
        
        
        
        
        
        fcFamilies.Append(NS_LITERAL_STRING("sans-serif"));
    }

    nsRefPtr<gfxQtFont> font = GetOrMakeFont(fcFamilies, &mStyle);
    if (font) {
        mFonts.AppendElement(font);
    }
}

gfxQtFontGroup::~gfxQtFontGroup()
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

gfxFontGroup *
gfxQtFontGroup::Copy(const gfxFontStyle *aStyle)
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
     return new gfxQtFontGroup(mFamilies, aStyle);
}

#if defined(ENABLE_FAST_PATH_8BIT)
PRBool
gfxQtFontGroup::CanTakeFastPath(PRUint32 aFlags)
{
    
    
    
    PRBool speed = aFlags & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED;
    PRBool isRTL = aFlags & gfxTextRunFactory::TEXT_IS_RTL;
    return speed && !isRTL
           
           ;
}
#endif

void
gfxQtFontGroup::InitTextRun(gfxTextRun *aTextRun, const char *aUTF8Text,
                             PRUint32 aUTF8Length, PRUint32 aUTF8HeaderLength,
                             PRBool aTake8BitPath)
{
    qDebug(">>>>>>Func:%s::%d, Text:'%s'\n", __PRETTY_FUNCTION__, __LINE__, aUTF8Text);
#if defined(ENABLE_FAST_PATH_ALWAYS)

#else
#if defined(ENABLE_FAST_PATH_8BIT)
    if (aTake8BitPath && CanTakeFastPath(aTextRun->GetFlags())) {

        if (NS_SUCCEEDED(rv))
            return;
    }
#endif


#endif
}






static PRInt32 AppendDirectionalIndicatorUTF8(PRBool aIsRTL, nsACString& aString)
{
    static const PRUnichar overrides[2][2] =
      { { 0x202d, 0 }, { 0x202e, 0 }}; 
    AppendUTF16toUTF8(overrides[aIsRTL], aString);
    return 3; 
}

gfxTextRun *
gfxQtFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "8bit should have been set");
    gfxTextRun *run = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!run)
        return nsnull;

    PRBool isRTL = run->IsRightToLeft();
    if ((aFlags & TEXT_IS_ASCII) && !isRTL) {
        
        const char *utf8Chars = reinterpret_cast<const char*>(aString);
        InitTextRun(run, utf8Chars, aLength, 0, PR_TRUE);
    } else {
        
        const char *chars = reinterpret_cast<const char*>(aString);
        NS_ConvertASCIItoUTF16 unicodeString(chars, aLength);
        nsCAutoString utf8;
        PRInt32 headerLen = AppendDirectionalIndicatorUTF8(isRTL, utf8);
        AppendUTF16toUTF8(unicodeString, utf8);
        InitTextRun(run, utf8.get(), utf8.Length(), headerLen, PR_TRUE);
    }
    run->FetchGlyphExtents(aParams->mContext);
    return run;
}

gfxTextRun *
gfxQtFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                               const Parameters *aParams, PRUint32 aFlags)
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    gfxTextRun *run = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!run)
        return nsnull;

    run->RecordSurrogates(aString);

    nsCAutoString utf8;
    PRInt32 headerLen = AppendDirectionalIndicatorUTF8(run->IsRightToLeft(), utf8);
    AppendUTF16toUTF8(Substring(aString, aString + aLength), utf8);
    PRBool is8Bit = PR_FALSE;

#if defined(ENABLE_FAST_PATH_8BIT)
    if (CanTakeFastPath(aFlags)) {
        PRUint32 allBits = 0;
        PRUint32 i;
        for (i = 0; i < aLength; ++i) {
            allBits |= aString[i];
        }
        is8Bit = (allBits & 0xFF00) == 0;
    }
#endif
    InitTextRun(run, utf8.get(), utf8.Length(), headerLen, is8Bit);
    run->FetchGlyphExtents(aParams->mContext);
    return run;
}





gfxQtFont::gfxQtFont(const nsAString &aName,
                     const gfxFontStyle *aFontStyle)
    : gfxFont(aName, aFontStyle),
      mQFont(nsnull), mCairoFont(nsnull),
      mHasMetrics(PR_FALSE), mAdjustedSize(0)
{
    qDebug(">>>>>>Func:%s::%d, name:%s\n", __PRETTY_FUNCTION__, __LINE__, NS_ConvertUTF16toUTF8(aName).get());
}

gfxQtFont::~gfxQtFont()
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

const gfxFont::Metrics&
gfxQtFont::GetMetrics()
{
    if (mHasMetrics)
        return mMetrics;

    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
#if 0
    
    

    fprintf (stderr, "Font: %s\n", NS_ConvertUTF16toUTF8(mName).get());
    fprintf (stderr, "    emHeight: %f emAscent: %f emDescent: %f\n", mMetrics.emHeight, mMetrics.emAscent, mMetrics.emDescent);
    fprintf (stderr, "    maxAscent: %f maxDescent: %f\n", mMetrics.maxAscent, mMetrics.maxDescent);
    fprintf (stderr, "    internalLeading: %f externalLeading: %f\n", mMetrics.externalLeading, mMetrics.internalLeading);
    fprintf (stderr, "    spaceWidth: %f aveCharWidth: %f xHeight: %f\n", mMetrics.spaceWidth, mMetrics.aveCharWidth, mMetrics.xHeight);
    fprintf (stderr, "    uOff: %f uSize: %f stOff: %f stSize: %f suOff: %f suSize: %f\n", mMetrics.underlineOffset, mMetrics.underlineSize, mMetrics.strikeoutOffset, mMetrics.strikeoutSize, mMetrics.superscriptOffset, mMetrics.subscriptOffset);
#endif

    mHasMetrics = PR_TRUE;
    return mMetrics;
}


nsString
gfxQtFont::GetUniqueName()
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    nsString result;









    return result;
}

 void
gfxQtFont::Shutdown()
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
}

PRBool
gfxQtFont::SetupCairoFont(gfxContext *aContext)
{
    qDebug(">>>>>>Func:%s::%d\n", __PRETTY_FUNCTION__, __LINE__);
    cairo_t *cr = aContext->GetCairo();
    cairo_matrix_t currentCTM;
    cairo_get_matrix(cr, &currentCTM);

    if (mCairoFont) {
        
        cairo_matrix_t fontCTM;
        cairo_scaled_font_get_ctm(mCairoFont, &fontCTM);
        if (fontCTM.xx != currentCTM.xx || fontCTM.yy != currentCTM.yy ||
            fontCTM.xy != currentCTM.xy || fontCTM.yx != currentCTM.yx) {
            
            cairo_scaled_font_destroy(mCairoFont);
            mCairoFont = nsnull;
        }
    }
    if (!mCairoFont) {
        
    }
    if (cairo_scaled_font_status(mCairoFont) != CAIRO_STATUS_SUCCESS) {
        
        
        return PR_FALSE;
    }
    cairo_set_scaled_font(cr, mCairoFont);
    return PR_TRUE;
}
