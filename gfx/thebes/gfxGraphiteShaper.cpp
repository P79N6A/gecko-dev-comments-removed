




































#include "prtypes.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"
#include "nsMathUtils.h"
#include "nsHashSets.h"

#include "gfxTypes.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxGraphiteShaper.h"
#include "gfxFontUtils.h"
#include "gfxUnicodeProperties.h"

#include "graphite2/Font.h"
#include "graphite2/Segment.h"

#include "harfbuzz/hb-blob.h"

#include "cairo.h"

#include "nsUnicodeRange.h"
#include "nsCRT.h"

#define FloatToFixed(f) (65536 * (f))
#define FixedToFloat(f) ((f) * (1.0 / 65536.0))



#define FixedToIntRound(f) ((f) > 0 ?  ((32768 + (f)) >> 16) \
                                    : -((32767 - (f)) >> 16))

using namespace mozilla; 





gfxGraphiteShaper::gfxGraphiteShaper(gfxFont *aFont)
    : gfxFontShaper(aFont),
      mGrFace(nsnull),
      mGrFont(nsnull),
      mUseFontGlyphWidths(false)
{
    mTables.Init();
    mCallbackData.mFont = aFont;
    mCallbackData.mShaper = this;
}

PLDHashOperator
ReleaseTableFunc(const PRUint32& ,
                 gfxGraphiteShaper::TableRec& aData,
                 void* )
{
    hb_blob_unlock(aData.mBlob);
    hb_blob_destroy(aData.mBlob);
    return PL_DHASH_REMOVE;
}

gfxGraphiteShaper::~gfxGraphiteShaper()
{
    if (mGrFont) {
        gr_font_destroy(mGrFont);
    }
    if (mGrFace) {
        gr_face_destroy(mGrFace);
    }
    mTables.Enumerate(ReleaseTableFunc, nsnull);
}

static const void*
GrGetTable(const void* appFaceHandle, unsigned int name, size_t *len)
{
    const gfxGraphiteShaper::CallbackData *cb =
        static_cast<const gfxGraphiteShaper::CallbackData*>(appFaceHandle);
    return cb->mShaper->GetTable(name, len);
}

const void*
gfxGraphiteShaper::GetTable(PRUint32 aTag, size_t *aLength)
{
    TableRec tableRec;

    if (!mTables.Get(aTag, &tableRec)) {
        hb_blob_t *blob = mFont->GetFontTable(aTag);
        if (blob) {
            
            
            
            
            tableRec.mBlob = blob;
            tableRec.mData = hb_blob_lock(blob);
            tableRec.mLength = hb_blob_get_length(blob);
            mTables.Put(aTag, tableRec);
        } else {
            return nsnull;
        }
    }

    *aLength = tableRec.mLength;
    return tableRec.mData;
}

static float
GrGetAdvance(const void* appFontHandle, gr_uint16 glyphid)
{
    const gfxGraphiteShaper::CallbackData *cb =
        static_cast<const gfxGraphiteShaper::CallbackData*>(appFontHandle);
    return FixedToFloat(cb->mFont->GetGlyphWidth(cb->mContext, glyphid));
}

static inline PRUint32
MakeGraphiteLangTag(PRUint32 aTag)
{
    PRUint32 grLangTag = aTag;
    
    PRUint32 mask = 0x000000FF;
    while ((grLangTag & mask) == ' ') {
        grLangTag &= ~mask;
        mask <<= 8;
    }
    return grLangTag;
}

bool
gfxGraphiteShaper::InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript)
{
    
    mFont->SetupCairoFont(aContext);

    mCallbackData.mContext = aContext;

    if (!mGrFont) {
        mGrFace = gr_make_face(&mCallbackData, GrGetTable, gr_face_default);
        if (!mGrFace) {
            return false;
        }
        mGrFont = mUseFontGlyphWidths ?
            gr_make_font_with_advance_fn(mFont->GetAdjustedSize(),
                                         &mCallbackData, GrGetAdvance,
                                         mGrFace) :
            gr_make_font(mFont->GetAdjustedSize(), mGrFace);
        if (!mGrFont) {
            gr_face_destroy(mGrFace);
            mGrFace = nsnull;
            return false;
        }
    }

    const gfxFontStyle *style = aTextRun->GetFontGroup()->GetStyle();
    PRUint32 grLang = 0;
    if (style->languageOverride) {
        grLang = MakeGraphiteLangTag(style->languageOverride);
    } else if (mFont->GetFontEntry()->mLanguageOverride) {
        grLang = MakeGraphiteLangTag(mFont->GetFontEntry()->mLanguageOverride);
    } else {
        nsCAutoString langString;
        style->language->ToUTF8String(langString);
        grLang = GetGraphiteTagForLang(langString);
    }
    gr_feature_val *grFeatures = gr_face_featureval_for_lang(mGrFace, grLang);

    bool disableLigatures =
        (aTextRun->GetFlags() &
         gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;
    if (disableLigatures) {
        const gr_feature_ref* fref =
            gr_face_find_fref(mGrFace, TRUETYPE_TAG('l','i','g','a'));
        if (fref) {
            gr_fref_set_feature_value(fref, 0, grFeatures);
        }
    }

    const nsTArray<gfxFontFeature> *features = &style->featureSettings;
    if (features->IsEmpty()) {
        features = &mFont->GetFontEntry()->mFeatureSettings;
    }
    for (PRUint32 i = 0; i < features->Length(); ++i) {
        const gr_feature_ref* fref =
            gr_face_find_fref(mGrFace, (*features)[i].mTag);
        if (fref) {
            gr_fref_set_feature_value(fref, (*features)[i].mValue, grFeatures);
        }
    }

    const PRUnichar *textStart = aString + aRunStart;
    const PRUnichar *textEnd = textStart + aRunLength;
    const void *pError;
    size_t nChars = gr_count_unicode_characters(gr_utf16,
                                                textStart, textEnd,
                                                &pError);
    if (pError != nsnull) {
        return false;
    }

    gr_segment *seg = gr_make_seg(mGrFont, mGrFace, 0, grFeatures,
                                  gr_utf16, textStart, nChars,
                                  aTextRun->IsRightToLeft());
    if (features) {
        gr_featureval_destroy(grFeatures);
    }
    if (!seg) {
        return false;
    }

    nsresult rv = SetGlyphsFromSegment(aTextRun, aRunStart, aRunLength, seg);

    gr_seg_destroy(seg);

    return NS_SUCCEEDED(rv);
}

#define SMALL_GLYPH_RUN 256 // avoid heap allocation of per-glyph data arrays
                            

struct Cluster {
    PRUint32 baseChar;
    PRUint32 baseGlyph;
    PRUint32 nChars;
    PRUint32 nGlyphs;
    Cluster() : baseChar(0), baseGlyph(0), nChars(0), nGlyphs(0) { }
};

nsresult
gfxGraphiteShaper::SetGlyphsFromSegment(gfxTextRun *aTextRun,
                                        PRUint32 aRunStart,
                                        PRUint32 aRunLength,
                                        gr_segment *aSegment)
{
    PRInt32 dev2appUnits = aTextRun->GetAppUnitsPerDevUnit();
    bool rtl = aTextRun->IsRightToLeft();

    PRUint32 glyphCount = gr_seg_n_slots(aSegment);

    
    nsAutoTArray<Cluster,SMALL_GLYPH_RUN> clusters;
    nsAutoTArray<PRUint16,SMALL_GLYPH_RUN> gids;
    nsAutoTArray<float,SMALL_GLYPH_RUN> xLocs;
    nsAutoTArray<float,SMALL_GLYPH_RUN> yLocs;

    if (!clusters.SetLength(aRunLength) ||
        !gids.SetLength(glyphCount) ||
        !xLocs.SetLength(glyphCount) ||
        !yLocs.SetLength(glyphCount))
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    PRUint32 gIndex = 0; 
    PRUint32 cIndex = 0; 
    for (const gr_slot *slot = gr_seg_first_slot(aSegment);
         slot != nsnull;
         slot = gr_slot_next_in_segment(slot), gIndex++)
    {
        PRUint32 before = gr_slot_before(slot);
        PRUint32 after = gr_slot_after(slot);
        gids[gIndex] = gr_slot_gid(slot);
        xLocs[gIndex] = gr_slot_origin_X(slot);
        yLocs[gIndex] = gr_slot_origin_Y(slot);

        
        
        
        while (before < clusters[cIndex].baseChar && cIndex > 0) {
            clusters[cIndex-1].nChars += clusters[cIndex].nChars;
            clusters[cIndex-1].nGlyphs += clusters[cIndex].nGlyphs;
            --cIndex;
        }

        
        
        if (gr_slot_can_insert_before(slot) && clusters[cIndex].nChars &&
            before >= clusters[cIndex].baseChar + clusters[cIndex].nChars)
        {
            NS_ASSERTION(cIndex < aRunLength - 1, "cIndex at end of run");
            Cluster& c = clusters[cIndex + 1];
            c.baseChar = clusters[cIndex].baseChar + clusters[cIndex].nChars;
            c.nChars = before - c.baseChar;
            c.baseGlyph = gIndex;
            c.nGlyphs = 0;
            ++cIndex;
        }

        
        NS_ASSERTION(cIndex < aRunLength, "cIndex beyond valid run length");
        ++clusters[cIndex].nGlyphs;

        
        if (clusters[cIndex].baseChar + clusters[cIndex].nChars < after + 1) {
            clusters[cIndex].nChars = after + 1 - clusters[cIndex].baseChar;
        }
    }

    
    for (PRUint32 i = 0; i <= cIndex; ++i) {
        const Cluster& c = clusters[i];

        float adv; 
        if (rtl) {
            if (i == 0) {
                adv = gr_seg_advance_X(aSegment) - xLocs[c.baseGlyph];
            } else {
                adv = xLocs[clusters[i-1].baseGlyph] - xLocs[c.baseGlyph];
            }
        } else {
            if (i == cIndex) {
                adv = gr_seg_advance_X(aSegment) - xLocs[c.baseGlyph];
            } else {
                adv = xLocs[clusters[i+1].baseGlyph] - xLocs[c.baseGlyph];
            }
        }

        
        
        PRUint32 offs = gr_cinfo_base(gr_seg_cinfo(aSegment, c.baseChar));
        NS_ASSERTION(offs >= c.baseChar && offs < aRunLength,
                     "unexpected offset");
        if (c.nGlyphs == 1 && c.nChars == 1 &&
            aTextRun->FilterIfIgnorable(aRunStart + offs))
        {
            continue;
        }

        PRUint32 appAdvance = adv * dev2appUnits;
        if (c.nGlyphs == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(gids[c.baseGlyph]) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(appAdvance) &&
            yLocs[c.baseGlyph] == 0)
        {
            gfxTextRun::CompressedGlyph g;
            aTextRun->SetSimpleGlyph(aRunStart + offs,
                                     g.SetSimpleGlyph(appAdvance,
                                                      gids[c.baseGlyph]));
        } else {
            
            nsAutoTArray<gfxTextRun::DetailedGlyph,8> details;
            float clusterLoc;
            for (PRUint32 j = c.baseGlyph; j < c.baseGlyph + c.nGlyphs; ++j) {
                gfxTextRun::DetailedGlyph* d = details.AppendElement();
                d->mGlyphID = gids[j];
                d->mYOffset = -yLocs[j] * dev2appUnits;
                if (j == c.baseGlyph) {
                    d->mXOffset = 0;
                    d->mAdvance = appAdvance;
                    clusterLoc = xLocs[j];
                } else {
                    d->mXOffset = (xLocs[j] - clusterLoc - adv) * dev2appUnits;
                    d->mAdvance = 0;
                }
            }
            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aTextRun->IsClusterStart(aRunStart + offs),
                         true, details.Length());
            aTextRun->SetGlyphs(aRunStart + offs, g, details.Elements());
        }

        for (PRUint32 j = c.baseChar + 1; j < c.baseChar + c.nChars; ++j) {
            offs = gr_cinfo_base(gr_seg_cinfo(aSegment, j));
            NS_ASSERTION(offs >= j && offs < aRunLength,
                         "unexpected offset");
            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aTextRun->IsClusterStart(aRunStart + offs),
                         false, 0);
            aTextRun->SetGlyphs(aRunStart + offs, g, nsnull);
        }
    }

    return NS_OK;
}


#include "gfxLanguageTagList.cpp"

nsTHashtable<nsUint32HashKey> gfxGraphiteShaper::sLanguageTags;

 PRUint32
gfxGraphiteShaper::GetGraphiteTagForLang(const nsCString& aLang)
{
    int len = aLang.Length();
    if (len < 2) {
        return 0;
    }

    
    
    PRUint32 grLang = 0;
    for (int i = 0; i < 4; ++i) {
        grLang <<= 8;
        if (i < len) {
            PRUint8 ch = aLang[i];
            if (ch == '-') {
                
                len = i;
                continue;
            }
            if (ch < 'a' || ch > 'z') {
                
                return 0;
            }
            grLang += ch;
        }
    }

    
    if (len < 2 || len > 3) {
        return 0;
    }

    if (!sLanguageTags.IsInitialized()) {
        
        sLanguageTags.Init(ArrayLength(sLanguageTagList));
        for (const PRUint32 *tag = sLanguageTagList; *tag != 0; ++tag) {
            sLanguageTags.PutEntry(*tag);
        }
    }

    
    if (sLanguageTags.GetEntry(grLang)) {
        return grLang;
    }

    return 0;
}

 void
gfxGraphiteShaper::Shutdown()
{
#ifdef NS_FREE_PERMANENT_DATA
    if (sLanguageTags.IsInitialized()) {
        sLanguageTags.Clear();
    }
#endif
}
