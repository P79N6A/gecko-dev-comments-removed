





































#include "nsIPref.h"
#include "nsServiceManagerUtils.h"
#include "nsReadableUtils.h"
#include "nsExpirationTracker.h"

#include "gfxFont.h"
#include "gfxPlatform.h"

#include "prtypes.h"
#include "gfxTypes.h"
#include "gfxContext.h"
#include "gfxFontMissingGlyphs.h"
#include "nsMathUtils.h"

#include "cairo.h"
#include "gfxFontTest.h"

#include "nsCRT.h"

gfxFontCache *gfxFontCache::gGlobalCache = nsnull;

nsresult
gfxFontCache::Init()
{
    NS_ASSERTION(!gGlobalCache, "Where did this come from?");
    gGlobalCache = new gfxFontCache();
    return gGlobalCache ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
gfxFontCache::Shutdown()
{
    delete gGlobalCache;
    gGlobalCache = nsnull;
}

PRBool
gfxFontCache::HashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    return aKey->mString.Equals(mFont->GetName()) &&
           aKey->mStyle->Equals(*mFont->GetStyle());
}

already_AddRefed<gfxFont>
gfxFontCache::Lookup(const nsAString &aName,
                     const gfxFontStyle *aStyle)
{
    Key key(aName, aStyle);
    HashEntry *entry = mFonts.GetEntry(key);
    if (!entry)
        return nsnull;

    gfxFont *font = entry->mFont;
    NS_ADDREF(font);
    if (font->GetExpirationState()->IsTracked()) {
        RemoveObject(font);
    }
    return font;
}

void
gfxFontCache::AddNew(gfxFont *aFont)
{
    Key key(aFont->GetName(), aFont->GetStyle());
    HashEntry *entry = mFonts.PutEntry(key);
    if (!entry)
        return;
    if (entry->mFont) {
        
        
        
        if (entry->mFont->GetExpirationState()->IsTracked()) {
            RemoveObject(entry->mFont);
        }
    }
    entry->mFont = aFont;
}

void
gfxFontCache::NotifyReleased(gfxFont *aFont)
{
    nsresult rv = AddObject(aFont);
    if (NS_FAILED(rv)) {
        
        DestroyFont(aFont);
    }
    
    
    
    
    
}

void
gfxFontCache::NotifyExpired(gfxFont *aFont)
{
    RemoveObject(aFont);
    DestroyFont(aFont);
}

void
gfxFontCache::DestroyFont(gfxFont *aFont)
{
    Key key(aFont->GetName(), aFont->GetStyle());
    mFonts.RemoveEntry(key);
    delete aFont;
}

gfxFont::gfxFont(const nsAString &aName, const gfxFontStyle *aFontStyle) :
    mName(aName), mStyle(*aFontStyle)
{
}





static double
ToDeviceUnits(double aAppUnits, double aDevUnitsPerAppUnit)
{
    return aAppUnits*aDevUnitsPerAppUnit;
}

void
gfxFont::Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
              gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aPt,
              Spacing *aSpacing)
{
    if (aStart >= aEnd)
        return;

    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    const double devUnitsPerAppUnit = 1.0/double(appUnitsPerDevUnit);
    PRBool isRTL = aTextRun->IsRightToLeft();
    double direction = aTextRun->GetDirection();
    nsAutoTArray<cairo_glyph_t,200> glyphBuffer;
    PRUint32 i;
    
    double x = aPt->x;
    double y = aPt->y;

    if (aSpacing) {
        x += direction*aSpacing[0].mBefore;
    }
    for (i = aStart; i < aEnd; ++i) {
        const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
        if (glyphData->IsSimpleGlyph()) {
            cairo_glyph_t *glyph = glyphBuffer.AppendElement();
            if (!glyph)
                return;
            glyph->index = glyphData->GetSimpleGlyph();
            double advance = glyphData->GetSimpleAdvance();
            
            
            
            
            
            glyph->x = ToDeviceUnits(x, devUnitsPerAppUnit);
            glyph->y = ToDeviceUnits(y, devUnitsPerAppUnit);
            if (isRTL) {
                glyph->x -= ToDeviceUnits(advance, devUnitsPerAppUnit);
                x -= advance;
            } else {
                x += advance;
            }
        } else if (glyphData->IsComplexCluster()) {
            const gfxTextRun::DetailedGlyph *details = aTextRun->GetDetailedGlyphs(i);
            for (;;) {
                cairo_glyph_t *glyph = glyphBuffer.AppendElement();
                if (!glyph)
                    return;
                glyph->index = details->mGlyphID;
                glyph->x = ToDeviceUnits(x + details->mXOffset, devUnitsPerAppUnit);
                glyph->y = ToDeviceUnits(y + details->mYOffset, devUnitsPerAppUnit);
                double advance = details->mAdvance;
                if (isRTL) {
                    glyph->x -= ToDeviceUnits(advance, devUnitsPerAppUnit);
                }
                x += direction*advance;
                if (details->mIsLastGlyph)
                    break;
                ++details;
            }
        } else if (glyphData->IsMissing()) {
            const gfxTextRun::DetailedGlyph *details = aTextRun->GetDetailedGlyphs(i);
            if (details) {
                double advance = details->mAdvance;
                if (!aDrawToPath) {
                    gfxPoint pt(ToDeviceUnits(x, devUnitsPerAppUnit),
                                ToDeviceUnits(y, devUnitsPerAppUnit));
                    gfxFloat advanceDevUnits = ToDeviceUnits(advance, devUnitsPerAppUnit);
                    if (isRTL) {
                        pt.x -= advanceDevUnits;
                    }
                    gfxFloat height = GetMetrics().maxAscent;
                    gfxRect glyphRect(pt.x, pt.y - height, advanceDevUnits, height);
                    gfxFontMissingGlyphs::DrawMissingGlyph(aContext, glyphRect, details->mGlyphID);
                }
                x += direction*advance;
            }
        }
        
        if (aSpacing) {
            double space = aSpacing[i - aStart].mAfter;
            if (i + 1 < aEnd) {
                space += aSpacing[i + 1 - aStart].mBefore;
            }
            x += direction*space;
        }
    }

    *aPt = gfxPoint(x, y);

    cairo_t *cr = aContext->GetCairo();
    SetupCairoFont(cr);
    if (aDrawToPath) {
        cairo_glyph_path(cr, glyphBuffer.Elements(), glyphBuffer.Length());
    } else {
        cairo_show_glyphs(cr, glyphBuffer.Elements(), glyphBuffer.Length());
    }

    if (gfxFontTestStore::CurrentStore()) {
        gfxFontTestStore::CurrentStore()->AddItem(GetUniqueName(),
                                                  glyphBuffer.Elements(), glyphBuffer.Length());
    }
}

gfxFont::RunMetrics
gfxFont::Measure(gfxTextRun *aTextRun,
                 PRUint32 aStart, PRUint32 aEnd,
                 PRBool aTightBoundingBox,
                 Spacing *aSpacing)
{
    
    
    
    
    PRInt32 advance = 0;
    PRUint32 i;
    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    PRUint32 clusterCount = 0;
    for (i = aStart; i < aEnd; ++i) {
        gfxTextRun::CompressedGlyph g = charGlyphs[i];
        if (g.IsClusterStart()) {
            ++clusterCount;
            if (g.IsSimpleGlyph()) {
                advance += charGlyphs[i].GetSimpleAdvance();
            } else if (g.IsComplexOrMissing()) {
                const gfxTextRun::DetailedGlyph *details = aTextRun->GetDetailedGlyphs(i);
                while (details) {
                    advance += details->mAdvance;
                    if (details->mIsLastGlyph)
                        break;
                    ++details;
                }
            }
        }
    }

    gfxFloat floatAdvance = advance;
    if (aSpacing) {
        for (i = 0; i < aEnd - aStart; ++i) {
            floatAdvance += aSpacing[i].mBefore + aSpacing[i].mAfter;
        }
    }
    RunMetrics metrics;
    const gfxFont::Metrics& fontMetrics = GetMetrics();
    metrics.mAdvanceWidth = floatAdvance;
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    metrics.mAscent = fontMetrics.maxAscent*appUnitsPerDevUnit;
    metrics.mDescent = fontMetrics.maxDescent*appUnitsPerDevUnit;
    metrics.mBoundingBox =
        gfxRect(0, -metrics.mAscent, floatAdvance, metrics.mAscent + metrics.mDescent);
    metrics.mClusterCount = clusterCount;
    return metrics;
}

gfxFontGroup::gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle)
    : mFamilies(aFamilies), mStyle(*aStyle)
{

}

PRBool
gfxFontGroup::ForEachFont(FontCreationCallback fc,
                          void *closure)
{
    return ForEachFontInternal(mFamilies, mStyle.langGroup,
                               PR_TRUE, fc, closure);
}

PRBool
gfxFontGroup::ForEachFont(const nsAString& aFamilies,
                          const nsACString& aLangGroup,
                          FontCreationCallback fc,
                          void *closure)
{
    return ForEachFontInternal(aFamilies, aLangGroup,
                               PR_FALSE, fc, closure);
}

struct ResolveData {
    ResolveData(gfxFontGroup::FontCreationCallback aCallback,
                nsACString& aGenericFamily,
                void *aClosure) :
        mCallback(aCallback),
        mGenericFamily(aGenericFamily),
        mClosure(aClosure) {
    }
    gfxFontGroup::FontCreationCallback mCallback;
    nsCString mGenericFamily;
    void *mClosure;
};

PRBool
gfxFontGroup::ForEachFontInternal(const nsAString& aFamilies,
                                  const nsACString& aLangGroup,
                                  PRBool aResolveGeneric,
                                  FontCreationCallback fc,
                                  void *closure)
{
    const PRUnichar kSingleQuote  = PRUnichar('\'');
    const PRUnichar kDoubleQuote  = PRUnichar('\"');
    const PRUnichar kComma        = PRUnichar(',');

    nsCOMPtr<nsIPref> prefs;
    prefs = do_GetService(NS_PREF_CONTRACTID);

    nsPromiseFlatString families(aFamilies);
    const PRUnichar *p, *p_end;
    families.BeginReading(p);
    families.EndReading(p_end);
    nsAutoString family;
    nsCAutoString lcFamily;
    nsAutoString genericFamily;
    nsCAutoString lang(aLangGroup);
    if (lang.IsEmpty())
        lang.Assign("x-unicode"); 

    while (p < p_end) {
        while (nsCRT::IsAsciiSpace(*p))
            if (++p == p_end)
                return PR_TRUE;

        PRBool generic;
        if (*p == kSingleQuote || *p == kDoubleQuote) {
            
            PRUnichar quoteMark = *p;
            if (++p == p_end)
                return PR_TRUE;
            const PRUnichar *nameStart = p;

            
            while (*p != quoteMark)
                if (++p == p_end)
                    return PR_TRUE;

            family = Substring(nameStart, p);
            generic = PR_FALSE;
            genericFamily.SetIsVoid(PR_TRUE);

            while (++p != p_end && *p != kComma)
                 ;

        } else {
            
            const PRUnichar *nameStart = p;
            while (++p != p_end && *p != kComma)
                 ;

            family = Substring(nameStart, p);
            family.CompressWhitespace(PR_FALSE, PR_TRUE);

            if (aResolveGeneric &&
                (family.LowerCaseEqualsLiteral("serif") ||
                 family.LowerCaseEqualsLiteral("sans-serif") ||
                 family.LowerCaseEqualsLiteral("monospace") ||
                 family.LowerCaseEqualsLiteral("cursive") ||
                 family.LowerCaseEqualsLiteral("fantasy")))
            {
                generic = PR_TRUE;

                ToLowerCase(NS_LossyConvertUTF16toASCII(family), lcFamily);

                nsCAutoString prefName("font.name.");
                prefName.Append(lcFamily);
                prefName.AppendLiteral(".");
                prefName.Append(lang);

                
                
                nsXPIDLString value;
                nsresult rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(value));
                if (NS_SUCCEEDED(rv)) {
                    CopyASCIItoUTF16(lcFamily, genericFamily);
                    family = value;
                }
            } else {
                generic = PR_FALSE;
                genericFamily.SetIsVoid(PR_TRUE);
            }
        }
        
        if (!family.IsEmpty()) {
            NS_LossyConvertUTF16toASCII gf(genericFamily);
            ResolveData data(fc, gf, closure);
            PRBool aborted;
            gfxPlatform *pf = gfxPlatform::GetPlatform();
            nsresult rv = pf->ResolveFontName(family,
                                              gfxFontGroup::FontResolverProc,
                                              &data, aborted);
            if (NS_FAILED(rv) || aborted)
                return PR_FALSE;
        }

        if (generic && aResolveGeneric) {
            nsCAutoString prefName("font.name-list.");
            prefName.Append(lcFamily);
            prefName.AppendLiteral(".");
            prefName.Append(aLangGroup);
            nsXPIDLString value;
            nsresult rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(value));
            if (NS_SUCCEEDED(rv)) {
                ForEachFontInternal(value, lang, PR_FALSE, fc, closure);
            }
        }

        ++p; 
    }

    return PR_TRUE;
}

PRBool
gfxFontGroup::FontResolverProc(const nsAString& aName, void *aClosure)
{
    ResolveData *data = reinterpret_cast<ResolveData*>(aClosure);
    return (data->mCallback)(aName, data->mGenericFamily, data->mClosure);
}

void
gfxFontGroup::FindGenericFontFromStyle(FontCreationCallback fc,
                                       void *closure)
{
    nsCOMPtr<nsIPref> prefs(do_GetService(NS_PREF_CONTRACTID));
    if (!prefs)
        return;

    nsCAutoString prefName;
    nsXPIDLString genericName;
    nsXPIDLString familyName;

    
    prefName.AssignLiteral("font.default.");
    prefName.Append(mStyle.langGroup);
    nsresult rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(genericName));
    if (NS_SUCCEEDED(rv)) {
        prefName.AssignLiteral("font.name.");
        prefName.Append(NS_LossyConvertUTF16toASCII(genericName));
        prefName.AppendLiteral(".");
        prefName.Append(mStyle.langGroup);

        rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(familyName));
        if (NS_SUCCEEDED(rv)) {
            ForEachFontInternal(familyName, mStyle.langGroup,
                                PR_FALSE, fc, closure);
        }

        prefName.AssignLiteral("font.name-list.");
        prefName.Append(NS_LossyConvertUTF16toASCII(genericName));
        prefName.AppendLiteral(".");
        prefName.Append(mStyle.langGroup);

        rv = prefs->CopyUnicharPref(prefName.get(), getter_Copies(familyName));
        if (NS_SUCCEEDED(rv)) {
            ForEachFontInternal(familyName, mStyle.langGroup,
                                PR_FALSE, fc, closure);
        }
    }
}

gfxTextRun *
gfxFontGroup::MakeEmptyTextRun(const Parameters *aParams, PRUint32 aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;
    return new gfxTextRun(aParams, nsnull, 0, this, aFlags);
}

gfxTextRun *
gfxFontGroup::MakeSpaceTextRun(const Parameters *aParams, PRUint32 aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;
    static const PRUint8 space = ' ';

    gfxFont *font = GetFontAt(0);
    PRUint32 spaceGlyph = font->GetSpaceGlyph();
    float spaceWidth = font->GetMetrics().spaceWidth;
    PRUint32 spaceWidthAppUnits = NS_lroundf(spaceWidth*aParams->mAppUnitsPerDevUnit);
    if (!spaceGlyph ||
        !gfxTextRun::CompressedGlyph::IsSimpleGlyphID(spaceGlyph) ||
        !gfxTextRun::CompressedGlyph::IsSimpleAdvance(spaceWidthAppUnits))
        return MakeTextRun(&space, 1, aParams, aFlags);

    nsAutoPtr<gfxTextRun> textRun;
    textRun = new gfxTextRun(aParams, &space, 1, this, aFlags);
    if (!textRun)
        return nsnull;
    if (NS_FAILED(textRun->AddGlyphRun(font, 0)))
        return nsnull;
    gfxTextRun::CompressedGlyph g;
    g.SetSimpleGlyph(spaceWidthAppUnits, spaceGlyph);
    textRun->SetCharacterGlyph(0, g);
    return textRun.forget();
}

gfxFontStyle::gfxFontStyle(PRUint8 aStyle, PRUint8 aVariant,
                           PRUint16 aWeight, PRUint8 aDecoration, gfxFloat aSize,
                           const nsACString& aLangGroup,
                           float aSizeAdjust, PRPackedBool aSystemFont,
                           PRPackedBool aFamilyNameQuirks) :
    style(aStyle), systemFont(aSystemFont), variant(aVariant),
    familyNameQuirks(aFamilyNameQuirks), weight(aWeight),
    decorations(aDecoration), size(PR_MIN(aSize, 5000)), langGroup(aLangGroup), sizeAdjust(aSizeAdjust)
{
    if (weight > 900)
        weight = 900;
    if (weight < 100)
        weight = 100;

    if (langGroup.IsEmpty()) {
        NS_WARNING("empty langgroup");
        langGroup.Assign("x-western");
    }
}

gfxFontStyle::gfxFontStyle(const gfxFontStyle& aStyle) :
    style(aStyle.style), systemFont(aStyle.systemFont), variant(aStyle.variant),
    familyNameQuirks(aStyle.familyNameQuirks), weight(aStyle.weight),
    decorations(aStyle.decorations), size(aStyle.size),
    langGroup(aStyle.langGroup), sizeAdjust(aStyle.sizeAdjust)
{
}

void
gfxFontStyle::ComputeWeightAndOffset(PRInt8 *outBaseWeight, PRInt8 *outOffset) const
{
    PRInt8 baseWeight = (weight + 50) / 100;
    PRInt8 offset = weight - baseWeight * 100;

    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    if (outBaseWeight)
        *outBaseWeight = baseWeight;
    if (outOffset)
        *outOffset = offset;
}

PRBool
gfxTextRun::GlyphRunIterator::NextRun()  {
    if (mNextIndex >= mTextRun->mGlyphRuns.Length())
        return PR_FALSE;
    mGlyphRun = &mTextRun->mGlyphRuns[mNextIndex];
    if (mGlyphRun->mCharacterOffset >= mEndOffset)
        return PR_FALSE;

    mStringStart = PR_MAX(mStartOffset, mGlyphRun->mCharacterOffset);
    PRUint32 last = mNextIndex + 1 < mTextRun->mGlyphRuns.Length()
        ? mTextRun->mGlyphRuns[mNextIndex + 1].mCharacterOffset : mTextRun->mCharacterCount;
    mStringEnd = PR_MIN(mEndOffset, last);

    ++mNextIndex;
    return PR_TRUE;
}

gfxTextRun::gfxTextRun(const gfxTextRunFactory::Parameters *aParams, const void *aText,
                       PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags)
  : mUserData(aParams->mUserData),
    mFontGroup(aFontGroup),
    mAppUnitsPerDevUnit(aParams->mAppUnitsPerDevUnit),
    mFlags(aFlags), mCharacterCount(aLength), mHashCode(0)
{
    NS_ADDREF(mFontGroup);
    if (aParams->mSkipChars) {
        mSkipChars.TakeFrom(aParams->mSkipChars);
    }
    if (aLength > 0) {
        mCharacterGlyphs = new CompressedGlyph[aLength];
        if (mCharacterGlyphs) {
            memset(mCharacterGlyphs, 0, sizeof(CompressedGlyph)*aLength);
        }
    }
    if (mFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
        mText.mSingle = NS_STATIC_CAST(const PRUint8 *, aText);
    } else {
        mText.mDouble = NS_STATIC_CAST(const PRUnichar *, aText);
    }
}

gfxTextRun::~gfxTextRun()
{
    if (!(mFlags & gfxTextRunFactory::TEXT_IS_PERSISTENT)) {
        if (mFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            delete[] mText.mSingle;
        } else {
            delete[] mText.mDouble;
        }
    }
    NS_RELEASE(mFontGroup);
}

gfxTextRun *
gfxTextRun::Clone(const gfxTextRunFactory::Parameters *aParams, const void *aText,
                  PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags)
{
    if (!mCharacterGlyphs)
        return nsnull;

    nsAutoPtr<gfxTextRun> textRun;
    textRun = new gfxTextRun(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun || !textRun->mCharacterGlyphs)
        return nsnull;

    PRUint32 i;
    for (i = 0; i < mGlyphRuns.Length(); ++i) {
        if (NS_FAILED(textRun->AddGlyphRun(mGlyphRuns[i].mFont,
                                           mGlyphRuns[i].mCharacterOffset)))
            return nsnull;
    }

    for (i = 0; i < aLength; ++i) {
        CompressedGlyph g = mCharacterGlyphs[i];
        g.SetCanBreakBefore(PR_FALSE);
        textRun->mCharacterGlyphs[i] = g;
    }

    if (mDetailedGlyphs) {
        for (i = 0; i < aLength; ++i) {
            DetailedGlyph *details = mDetailedGlyphs[i];
            if (details) {
                PRUint32 glyphCount = 1;
                while (!details[glyphCount - 1].mIsLastGlyph) {
                    ++glyphCount;
                }
                DetailedGlyph *dest = textRun->AllocateDetailedGlyphs(i, glyphCount);
                if (!dest)
                    return nsnull;
                memcpy(dest, details, sizeof(DetailedGlyph)*glyphCount);
            }
        }
    }

    return textRun.forget();
}

PRBool
gfxTextRun::SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                   PRPackedBool *aBreakBefore)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Overflow");

    if (!mCharacterGlyphs)
        return PR_TRUE;
    PRUint32 changed = 0;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
        NS_ASSERTION(!aBreakBefore[i] ||
                     mCharacterGlyphs[aStart + i].IsClusterStart(),
                     "Break suggested inside cluster!");
        changed |= mCharacterGlyphs[aStart + i].SetCanBreakBefore(aBreakBefore[i]);
    }
    return changed != 0;
}

PRInt32
gfxTextRun::ComputeClusterAdvance(PRUint32 aClusterOffset)
{
    CompressedGlyph *glyphData = &mCharacterGlyphs[aClusterOffset];
    if (glyphData->IsSimpleGlyph())
        return glyphData->GetSimpleAdvance();

    const DetailedGlyph *details = GetDetailedGlyphs(aClusterOffset);
    if (!details)
        return 0;

    PRInt32 advance = 0;
    while (1) {
        advance += details->mAdvance;
        if (details->mIsLastGlyph)
            return advance;
        ++details;
    }
}

gfxTextRun::LigatureData
gfxTextRun::ComputeLigatureData(PRUint32 aPartOffset, PropertyProvider *aProvider)
{
    LigatureData result;

    PRUint32 ligStart = aPartOffset;
    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    while (charGlyphs[ligStart].IsLigatureContinuation()) {
        do {
            NS_ASSERTION(ligStart > 0, "Ligature at the start of the run??");
            --ligStart;
        } while (!charGlyphs[ligStart].IsClusterStart());
    }
    result.mStartOffset = ligStart;
    result.mLigatureWidth = ComputeClusterAdvance(ligStart);
    result.mPartClusterIndex = PR_UINT32_MAX;

    PRUint32 charIndex = ligStart;
    
    PRUint32 clusterCount = 0;
    while (charIndex < mCharacterCount) {
        if (charIndex == aPartOffset) {
            result.mPartClusterIndex = clusterCount;
        }
        if (mCharacterGlyphs[charIndex].IsClusterStart()) {
            if (charIndex > ligStart &&
                !mCharacterGlyphs[charIndex].IsLigatureContinuation())
                break;
            ++clusterCount;
        }
        ++charIndex;
    }
    result.mClusterCount = clusterCount;
    result.mEndOffset = charIndex;

    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        PropertyProvider::Spacing spacing;
        aProvider->GetSpacing(ligStart, 1, &spacing);
        result.mBeforeSpacing = spacing.mBefore;
        aProvider->GetSpacing(charIndex - 1, 1, &spacing);
        result.mAfterSpacing = spacing.mAfter;
    } else {
        result.mBeforeSpacing = result.mAfterSpacing = 0;
    }

    NS_ASSERTION(result.mPartClusterIndex < PR_UINT32_MAX, "Didn't find cluster part???");
    return result;
}

void
gfxTextRun::GetAdjustedSpacing(PRUint32 aStart, PRUint32 aEnd,
                               PropertyProvider *aProvider,
                               PropertyProvider::Spacing *aSpacing)
{
    if (aStart >= aEnd)
        return;

    aProvider->GetSpacing(aStart, aEnd - aStart, aSpacing);

    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    PRUint32 i;

    if (mFlags & gfxTextRunFactory::TEXT_ABSOLUTE_SPACING) {
        
        
        
        
        
        
        PRInt32 clusterWidth = 0;
        for (i = aStart; i < aEnd; ++i) {
            CompressedGlyph *glyphData = &charGlyphs[i];
            
            if (glyphData->IsSimpleGlyph()) {
                if (i > aStart) {
                    aSpacing[i - 1 - aStart].mAfter -= clusterWidth;
                }
                clusterWidth = glyphData->GetSimpleAdvance();
            } else if (glyphData->IsComplexOrMissing()) {
                if (i > aStart) {
                    aSpacing[i - 1 - aStart].mAfter -= clusterWidth;
                }
                clusterWidth = 0;
                const DetailedGlyph *details = GetDetailedGlyphs(i);
                if (details) {
                    while (1) {
                        clusterWidth += details->mAdvance;
                        if (details->mIsLastGlyph)
                            break;
                        ++details;
                    }
                }
            }
        }
        aSpacing[aEnd - 1 - aStart].mAfter -= clusterWidth;
    }

    
    
    
    
    
    
    gfxFloat accumulatedSpace = 0;
    for (i = aStart; i <= aEnd; ++i) {
        if (i < aEnd && charGlyphs[i].IsLigatureContinuation()) {
            accumulatedSpace += aSpacing[i - aStart].mBefore;
            aSpacing[i - aStart].mBefore = 0;
            NS_ASSERTION(i > aStart, "Ligature continuation at start of spacing run?");
            accumulatedSpace += aSpacing[i - 1 - aStart].mAfter;
            aSpacing[i - 1 - aStart].mAfter = 0;
        } else {
            if (i > aStart) {
                aSpacing[i - 1 - aStart].mAfter += accumulatedSpace;
                accumulatedSpace = 0;
            }
        }
    }
}

PRBool
gfxTextRun::GetAdjustedSpacingArray(PRUint32 aStart, PRUint32 aEnd,
                                    PropertyProvider *aProvider,
                                    nsTArray<PropertyProvider::Spacing> *aSpacing)
{
    if (!aProvider || !(mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING))
        return PR_FALSE;
    if (!aSpacing->AppendElements(aEnd - aStart))
        return PR_FALSE;
    GetAdjustedSpacing(aStart, aEnd, aProvider, aSpacing->Elements());
    return PR_TRUE;
}

void
gfxTextRun::ShrinkToLigatureBoundaries(PRUint32 *aStart, PRUint32 *aEnd)
{
    if (*aStart >= *aEnd)
        return;
  
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    NS_ASSERTION(charGlyphs[*aStart].IsClusterStart(),
                 "Started in the middle of a cluster...");
    NS_ASSERTION(*aEnd == mCharacterCount || charGlyphs[*aEnd].IsClusterStart(),
                 "Ended in the middle of a cluster...");

    if (charGlyphs[*aStart].IsLigatureContinuation()) {
        LigatureData data = ComputeLigatureData(*aStart, nsnull);
        *aStart = PR_MIN(*aEnd, data.mEndOffset);
    }
    if (*aEnd < mCharacterCount && charGlyphs[*aEnd].IsLigatureContinuation()) {
        LigatureData data = ComputeLigatureData(*aEnd, nsnull);
        
        
        
        *aEnd = PR_MAX(*aStart, data.mStartOffset);
    }
}

void
gfxTextRun::DrawGlyphs(gfxFont *aFont, gfxContext *aContext,
                       PRBool aDrawToPath, gfxPoint *aPt,
                       PRUint32 aStart, PRUint32 aEnd,
                       PropertyProvider *aProvider)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    PRBool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider, &spacingBuffer);
    aFont->Draw(this, aStart, aEnd, aContext, aDrawToPath, aPt,
                haveSpacing ? spacingBuffer.Elements() : nsnull);
}

gfxFloat
gfxTextRun::GetPartialLigatureWidth(PRUint32 aStart, PRUint32 aEnd,
                                    PropertyProvider *aProvider)
{
    if (aStart >= aEnd)
        return 0;

    LigatureData data = ComputeLigatureData(aStart, aProvider);
    PRUint32 clusterCount = 0;
    PRUint32 i;
    for (i = aStart; i < aEnd; ++i) {
        if (mCharacterGlyphs[i].IsClusterStart()) {
            ++clusterCount;
        }
    }

    gfxFloat result = gfxFloat(data.mLigatureWidth)*clusterCount/data.mClusterCount;
    if (aStart == data.mStartOffset) {
        result += data.mBeforeSpacing;
    }
    if (aEnd == data.mEndOffset) {
        result += data.mAfterSpacing;
    }
    return result;
}

void
gfxTextRun::DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx, PRUint32 aOffset,
                                const gfxRect *aDirtyRect, gfxPoint *aPt,
                                PropertyProvider *aProvider)
{
    NS_ASSERTION(aDirtyRect, "Cannot draw partial ligatures without a dirty rect");

    if (!mCharacterGlyphs[aOffset].IsClusterStart() || !aDirtyRect)
        return;

    
    LigatureData data = ComputeLigatureData(aOffset, aProvider);
    
    gfxFloat clusterWidth = data.mLigatureWidth/data.mClusterCount;

    gfxFloat direction = GetDirection();
    gfxFloat left = aDirtyRect->X();
    gfxFloat right = aDirtyRect->XMost();
    
    gfxFloat widthBeforeCluster;
    
    gfxFloat afterSpace;
    if (data.mStartOffset < aOffset) {
        
        if (IsRightToLeft()) {
            right = PR_MIN(right, aPt->x);
        } else {
            left = PR_MAX(left, aPt->x);
        }
        widthBeforeCluster = clusterWidth*data.mPartClusterIndex +
            data.mBeforeSpacing;
    } else {
        
        
        widthBeforeCluster = 0;
    }
    if (aOffset < data.mEndOffset) {
        
        gfxFloat endEdge = aPt->x + clusterWidth;
        if (IsRightToLeft()) {
            left = PR_MAX(left, endEdge);
        } else {
            right = PR_MIN(right, endEdge);
        }
        afterSpace = 0;
    } else {
        afterSpace = data.mAfterSpacing;
    }

    aCtx->Save();
    
    
    aCtx->Clip(gfxRect(left/mAppUnitsPerDevUnit,
                       aDirtyRect->Y()/mAppUnitsPerDevUnit,
                       (right - left)/mAppUnitsPerDevUnit,
                       aDirtyRect->Height()/mAppUnitsPerDevUnit));
    gfxPoint pt(aPt->x - direction*widthBeforeCluster, aPt->y);
    DrawGlyphs(aFont, aCtx, PR_FALSE, &pt, data.mStartOffset,
               data.mEndOffset, aProvider);
    aCtx->Restore();

    aPt->x += direction*(clusterWidth + afterSpace);
}

void
gfxTextRun::Draw(gfxContext *aContext, gfxPoint aPt,
                 PRUint32 aStart, PRUint32 aLength, const gfxRect *aDirtyRect,
                 PropertyProvider *aProvider, gfxFloat *aAdvanceWidth)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");

    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    gfxFloat direction = GetDirection();
    gfxPoint pt = aPt;

    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        PRUint32 start = iter.GetStringStart();
        PRUint32 end = iter.GetStringEnd();
        NS_ASSERTION(charGlyphs[start].IsClusterStart(),
                     "Started drawing in the middle of a cluster...");
        NS_ASSERTION(end == mCharacterCount || charGlyphs[end].IsClusterStart(),
                     "Ended drawing in the middle of a cluster...");

        PRUint32 ligatureRunStart = start;
        PRUint32 ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

        PRUint32 i;
        for (i = start; i < ligatureRunStart; ++i) {
            DrawPartialLigature(font, aContext, i, aDirtyRect, &pt, aProvider);
        }

        DrawGlyphs(font, aContext, PR_FALSE, &pt, ligatureRunStart,
                   ligatureRunEnd, aProvider);

        for (i = ligatureRunEnd; i < end; ++i) {
            DrawPartialLigature(font, aContext, i, aDirtyRect, &pt, aProvider);
        }
    }

    if (aAdvanceWidth) {
        *aAdvanceWidth = (pt.x - aPt.x)*direction;
    }
}

void
gfxTextRun::DrawToPath(gfxContext *aContext, gfxPoint aPt,
                       PRUint32 aStart, PRUint32 aLength,
                       PropertyProvider *aProvider, gfxFloat *aAdvanceWidth)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");

    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    gfxFloat direction = GetDirection();
    gfxPoint pt = aPt;

    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        PRUint32 start = iter.GetStringStart();
        PRUint32 end = iter.GetStringEnd();
        NS_ASSERTION(charGlyphs[start].IsClusterStart(),
                     "Started drawing path in the middle of a cluster...");
        NS_ASSERTION(!charGlyphs[start].IsLigatureContinuation(),
                     "Can't draw path starting inside ligature");
        NS_ASSERTION(end == mCharacterCount || charGlyphs[end].IsClusterStart(),
                     "Ended drawing path in the middle of a cluster...");
        NS_ASSERTION(end == mCharacterCount || !charGlyphs[end].IsLigatureContinuation(),
                     "Can't end drawing path inside ligature");

        DrawGlyphs(font, aContext, PR_TRUE, &pt, start, end, aProvider);
    }

    if (aAdvanceWidth) {
        *aAdvanceWidth = (pt.x - aPt.x)*direction;
    }
}


void
gfxTextRun::AccumulateMetricsForRun(gfxFont *aFont, PRUint32 aStart,
                                    PRUint32 aEnd,
                                    PRBool aTight, PropertyProvider *aProvider,
                                    Metrics *aMetrics)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    PRBool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider, &spacingBuffer);
    Metrics metrics = aFont->Measure(this, aStart, aEnd, aTight,
                                     haveSpacing ? spacingBuffer.Elements() : nsnull);
 
    if (IsRightToLeft()) {
        metrics.CombineWith(*aMetrics);
        *aMetrics = metrics;
    } else {
        aMetrics->CombineWith(metrics);
    }
}

void
gfxTextRun::AccumulatePartialLigatureMetrics(gfxFont *aFont,
    PRUint32 aOffset, PRBool aTight, PropertyProvider *aProvider, Metrics *aMetrics)
{
    if (!mCharacterGlyphs[aOffset].IsClusterStart())
        return;

    
    
    LigatureData data = ComputeLigatureData(aOffset, aProvider);

    
    Metrics metrics;
    AccumulateMetricsForRun(aFont, data.mStartOffset, data.mEndOffset,
                            aTight, aProvider, &metrics);
    gfxFloat clusterWidth = data.mLigatureWidth/data.mClusterCount;

    gfxFloat bboxStart;
    if (IsRightToLeft()) {
        bboxStart = metrics.mAdvanceWidth - metrics.mBoundingBox.XMost();
    } else {
        bboxStart = metrics.mBoundingBox.X();
    }
    gfxFloat bboxEnd = bboxStart + metrics.mBoundingBox.size.width;

    gfxFloat widthBeforeCluster;
    gfxFloat totalWidth = clusterWidth;
    if (data.mStartOffset < aOffset) {
        widthBeforeCluster =
            clusterWidth*data.mPartClusterIndex + data.mBeforeSpacing;
        
        bboxStart = PR_MAX(bboxStart, widthBeforeCluster);
    } else {
        
        
        widthBeforeCluster = 0;
        totalWidth += data.mBeforeSpacing;
    }
    if (aOffset < data.mEndOffset) {
        
        gfxFloat endEdge = widthBeforeCluster + clusterWidth;
        bboxEnd = PR_MIN(bboxEnd, endEdge);
    } else {
        totalWidth += data.mAfterSpacing;
    }
    bboxStart -= widthBeforeCluster;
    bboxEnd -= widthBeforeCluster;
    if (IsRightToLeft()) {
        metrics.mBoundingBox.pos.x = metrics.mAdvanceWidth - bboxEnd;
    } else {
        metrics.mBoundingBox.pos.x = bboxStart;
    }
    metrics.mBoundingBox.size.width = bboxEnd - bboxStart;

    
    metrics.mAdvanceWidth = totalWidth;
    metrics.mClusterCount = 1;

    if (IsRightToLeft()) {
        metrics.CombineWith(*aMetrics);
        *aMetrics = metrics;
    } else {
        aMetrics->CombineWith(metrics);
    }
}

gfxTextRun::Metrics
gfxTextRun::MeasureText(PRUint32 aStart, PRUint32 aLength,
                        PRBool aTightBoundingBox,
                        PropertyProvider *aProvider)
{
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");
    NS_ASSERTION(aStart == mCharacterCount || charGlyphs[aStart].IsClusterStart(),
                 "MeasureText called, not starting at cluster boundary");
    NS_ASSERTION(aStart + aLength == mCharacterCount ||
                 charGlyphs[aStart + aLength].IsClusterStart(),
                 "MeasureText called, not ending at cluster boundary");

    Metrics accumulatedMetrics;
    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        PRUint32 ligatureRunStart = iter.GetStringStart();
        PRUint32 ligatureRunEnd = iter.GetStringEnd();
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

        PRUint32 i;
        for (i = iter.GetStringStart(); i < ligatureRunStart; ++i) {
            AccumulatePartialLigatureMetrics(font, i, aTightBoundingBox,
                                             aProvider, &accumulatedMetrics);
        }

        
        
        
        
        
        AccumulateMetricsForRun(font,
            ligatureRunStart, ligatureRunEnd, aTightBoundingBox, aProvider,
            &accumulatedMetrics);

        for (i = ligatureRunEnd; i < iter.GetStringEnd(); ++i) {
            AccumulatePartialLigatureMetrics(font, i, aTightBoundingBox,
                                             aProvider, &accumulatedMetrics);
        }
    }

    return accumulatedMetrics;
}

#define MEASUREMENT_BUFFER_SIZE 100

PRUint32
gfxTextRun::BreakAndMeasureText(PRUint32 aStart, PRUint32 aMaxLength,
                                PRBool aLineBreakBefore, gfxFloat aWidth,
                                PropertyProvider *aProvider,
                                PRBool aSuppressInitialBreak,
                                Metrics *aMetrics, PRBool aTightBoundingBox,
                                PRBool *aUsedHyphenation,
                                PRUint32 *aLastBreak)
{
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    aMaxLength = PR_MIN(aMaxLength, mCharacterCount - aStart);

    NS_ASSERTION(aStart + aMaxLength <= mCharacterCount, "Substring out of range");
    NS_ASSERTION(aStart == mCharacterCount || charGlyphs[aStart].IsClusterStart(),
                 "BreakAndMeasureText called, not starting at cluster boundary");
    NS_ASSERTION(aStart + aMaxLength == mCharacterCount ||
                 charGlyphs[aStart + aMaxLength].IsClusterStart(),
                 "BreakAndMeasureText called, not ending at cluster boundary");

    PRUint32 bufferStart = aStart;
    PRUint32 bufferLength = PR_MIN(aMaxLength, MEASUREMENT_BUFFER_SIZE);
    PropertyProvider::Spacing spacingBuffer[MEASUREMENT_BUFFER_SIZE];
    PRBool haveSpacing = aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING) != 0;
    if (haveSpacing) {
        GetAdjustedSpacing(bufferStart, bufferStart + bufferLength, aProvider,
                           spacingBuffer);
    }
    PRPackedBool hyphenBuffer[MEASUREMENT_BUFFER_SIZE];
    PRBool haveHyphenation = (mFlags & gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS) != 0;
    if (haveHyphenation) {
        aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                        hyphenBuffer);
    }

    gfxFloat width = 0;
    gfxFloat advance = 0;
    PRInt32 lastBreak = -1;
    PRBool aborted = PR_FALSE;
    PRUint32 end = aStart + aMaxLength;
    PRBool lastBreakUsedHyphenation = PR_FALSE;

    PRUint32 ligatureRunStart = aStart;
    PRUint32 ligatureRunEnd = end;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    PRUint32 i;
    for (i = aStart; i < end; ++i) {
        if (i >= bufferStart + bufferLength) {
            
            bufferStart = i;
            bufferLength = PR_MIN(aStart + aMaxLength, i + MEASUREMENT_BUFFER_SIZE) - i;
            if (haveSpacing) {
                GetAdjustedSpacing(bufferStart, bufferStart + bufferLength, aProvider,
                                   spacingBuffer);
            }
            if (haveHyphenation) {
                aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                                hyphenBuffer);
            }
        }

        PRBool lineBreakHere = mCharacterGlyphs[i].CanBreakBefore() &&
            (!aSuppressInitialBreak || i > aStart);
        if (lineBreakHere || (haveHyphenation && hyphenBuffer[i - bufferStart])) {
            gfxFloat hyphenatedAdvance = advance;
            PRBool hyphenation = !lineBreakHere;
            if (hyphenation) {
                hyphenatedAdvance += aProvider->GetHyphenWidth();
            }

            if (lastBreak < 0 || width + hyphenatedAdvance <= aWidth) {
                
                lastBreak = i;
                lastBreakUsedHyphenation = hyphenation;
            }

            width += advance;
            advance = 0;
            if (width > aWidth) {
                
                aborted = PR_TRUE;
                break;
            }
        }
        
        if (i >= ligatureRunStart && i < ligatureRunEnd) {
            CompressedGlyph *glyphData = &charGlyphs[i];
            if (glyphData->IsSimpleGlyph()) {
                advance += glyphData->GetSimpleAdvance();
            } else if (glyphData->IsComplexOrMissing()) {
                const DetailedGlyph *details = GetDetailedGlyphs(i);
                if (details) {
                    while (1) {
                        advance += details->mAdvance;
                        if (details->mIsLastGlyph)
                            break;
                        ++details;
                    }
                }
            }
            if (haveSpacing) {
                PropertyProvider::Spacing *space = &spacingBuffer[i - bufferStart];
                advance += space->mBefore + space->mAfter;
            }
        } else {
            advance += GetPartialLigatureWidth(i, i + 1, aProvider);
        }
    }

    if (!aborted) {
        width += advance;
    }

    
    
    
    
    PRUint32 charsFit;
    if (width <= aWidth) {
        charsFit = aMaxLength;
    } else if (lastBreak >= 0) {
        charsFit = lastBreak - aStart;
    } else {
        charsFit = aMaxLength;
    }

    if (aMetrics) {
        *aMetrics = MeasureText(aStart, charsFit, aTightBoundingBox, aProvider);
    }
    if (aUsedHyphenation) {
        *aUsedHyphenation = lastBreakUsedHyphenation;
    }
    if (aLastBreak && charsFit == aMaxLength) {
        if (lastBreak < 0) {
            *aLastBreak = PR_UINT32_MAX;
        } else {
            *aLastBreak = lastBreak - aStart;
        }
    }

    return charsFit;
}

gfxFloat
gfxTextRun::GetAdvanceWidth(PRUint32 aStart, PRUint32 aLength,
                            PropertyProvider *aProvider)
{
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");
    NS_ASSERTION(aStart == mCharacterCount || charGlyphs[aStart].IsClusterStart(),
                 "GetAdvanceWidth called, not starting at cluster boundary");
    NS_ASSERTION(aStart + aLength == mCharacterCount ||
                 charGlyphs[aStart + aLength].IsClusterStart(),
                 "GetAdvanceWidth called, not ending at cluster boundary");

    gfxFloat result = 0; 

    
    
    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        PRUint32 i;
        nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
        if (spacingBuffer.AppendElements(aLength)) {
            GetAdjustedSpacing(aStart, aStart + aLength, aProvider,
                               spacingBuffer.Elements());
            for (i = 0; i < aLength; ++i) {
                PropertyProvider::Spacing *space = &spacingBuffer[i];
                result += space->mBefore + space->mAfter;
            }
        }
    }

    PRUint32 ligatureRunStart = aStart;
    PRUint32 ligatureRunEnd = aStart + aLength;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    result += GetPartialLigatureWidth(aStart, ligatureRunStart, aProvider) +
              GetPartialLigatureWidth(ligatureRunEnd, aStart + aLength, aProvider);

    PRUint32 i;
    for (i = ligatureRunStart; i < ligatureRunEnd; ++i) {
        CompressedGlyph *glyphData = &charGlyphs[i];
        if (glyphData->IsSimpleGlyph()) {
            result += glyphData->GetSimpleAdvance();
        } else if (glyphData->IsComplexOrMissing()) {
            const DetailedGlyph *details = GetDetailedGlyphs(i);
            if (details) {
                while (1) {
                    result += details->mAdvance;
                    if (details->mIsLastGlyph)
                        break;
                    ++details;
                }
            }
        }
    }

    return result;
}

PRBool
gfxTextRun::SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                          PRBool aLineBreakBefore, PRBool aLineBreakAfter,
                          gfxFloat *aAdvanceWidthDelta)
{
    
    
    if (aAdvanceWidthDelta) {
        *aAdvanceWidthDelta = 0;
    }
    return PR_FALSE;
}

PRUint32
gfxTextRun::FindFirstGlyphRunContaining(PRUint32 aOffset)
{
    NS_ASSERTION(aOffset <= mCharacterCount, "Bad offset looking for glyphrun");
    if (aOffset == mCharacterCount)
        return mGlyphRuns.Length();

    PRUint32 start = 0;
    PRUint32 end = mGlyphRuns.Length();
    while (end - start > 1) {
        PRUint32 mid = (start + end)/2;
        if (mGlyphRuns[mid].mCharacterOffset <= aOffset) {
            start = mid;
        } else {
            end = mid;
        }
    }
    NS_ASSERTION(mGlyphRuns[start].mCharacterOffset <= aOffset,
                 "Hmm, something went wrong, aOffset should have been found");
    return start;
}

nsresult
gfxTextRun::AddGlyphRun(gfxFont *aFont, PRUint32 aUTF16Offset)
{
    NS_ASSERTION(mGlyphRuns.Length() > 0 || aUTF16Offset == 0,
                 "First run doesn't cover the first character?");
    GlyphRun *glyphRun = mGlyphRuns.AppendElement();
    if (!glyphRun)
        return NS_ERROR_OUT_OF_MEMORY;
    glyphRun->mFont = aFont;
    glyphRun->mCharacterOffset = aUTF16Offset;
    return NS_OK;
}

PRUint32
gfxTextRun::CountMissingGlyphs()
{
    PRUint32 i;
    PRUint32 count = 0;
    for (i = 0; i < mCharacterCount; ++i) {
        if (mCharacterGlyphs[i].IsMissing()) {
            ++count;
        }
    }
    return count;
}

gfxTextRun::DetailedGlyph *
gfxTextRun::AllocateDetailedGlyphs(PRUint32 aIndex, PRUint32 aCount)
{
    if (!mCharacterGlyphs)
        return nsnull;

    if (!mDetailedGlyphs) {
        mDetailedGlyphs = new nsAutoArrayPtr<DetailedGlyph>[mCharacterCount];
        if (!mDetailedGlyphs) {
            mCharacterGlyphs[aIndex].SetMissing();
            return nsnull;
        }
    }
    DetailedGlyph *details = new DetailedGlyph[aCount];
    if (!details) {
        mCharacterGlyphs[aIndex].SetMissing();
        return nsnull;
    }
    mDetailedGlyphs[aIndex] = details;
    return details;
}

void
gfxTextRun::SetDetailedGlyphs(PRUint32 aIndex, const DetailedGlyph *aGlyphs,
                              PRUint32 aCount)
{
    NS_ASSERTION(aCount > 0, "Can't set zero detailed glyphs");
    NS_ASSERTION(aGlyphs[aCount - 1].mIsLastGlyph, "Failed to set last glyph flag");

    DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, aCount);
    if (!details)
        return;

    memcpy(details, aGlyphs, sizeof(DetailedGlyph)*aCount);
    mCharacterGlyphs[aIndex].SetComplexCluster();
}
  
void
gfxTextRun::SetMissingGlyph(PRUint32 aIndex, PRUnichar aChar)
{
    DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
    if (!details)
        return;

    details->mIsLastGlyph = PR_TRUE;
    details->mGlyphID = aChar;
    GlyphRun *glyphRun = &mGlyphRuns[FindFirstGlyphRunContaining(aIndex)];
    gfxFloat width = PR_MAX(glyphRun->mFont->GetMetrics().aveCharWidth,
                            gfxFontMissingGlyphs::GetDesiredMinWidth());
    details->mAdvance = PRUint32(width*GetAppUnitsPerDevUnit());
    details->mXOffset = 0;
    details->mYOffset = 0;
    mCharacterGlyphs[aIndex].SetMissing();
}

void
gfxTextRun::RecordSurrogates(const PRUnichar *aString)
{
    if (!(mFlags & gfxTextRunFactory::TEXT_HAS_SURROGATES))
        return;

    
    
    PRUint32 i;
    gfxTextRun::CompressedGlyph g;
    for (i = 0; i < mCharacterCount; ++i) {
        if (NS_IS_LOW_SURROGATE(aString[i])) {
            SetCharacterGlyph(i, g.SetLowSurrogate());
        }
    }
}
