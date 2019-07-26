




#include "nsString.h"
#include "nsBidiUtils.h"
#include "nsMathUtils.h"

#include "gfxTypes.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxGraphiteShaper.h"
#include "gfxFontUtils.h"

#include "graphite2/Font.h"
#include "graphite2/Segment.h"

#include "harfbuzz/hb.h"

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
      mGrFace(nullptr),
      mGrFont(nullptr),
      mUseFontGlyphWidths(false)
{
    mTables.Init();
    mCallbackData.mFont = aFont;
    mCallbackData.mShaper = this;
}

PLDHashOperator
ReleaseTableFunc(const uint32_t& ,
                 gfxGraphiteShaper::TableRec& aData,
                 void* )
{
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
    mTables.Enumerate(ReleaseTableFunc, nullptr);
}

static const void*
GrGetTable(const void* appFaceHandle, unsigned int name, size_t *len)
{
    const gfxGraphiteShaper::CallbackData *cb =
        static_cast<const gfxGraphiteShaper::CallbackData*>(appFaceHandle);
    return cb->mShaper->GetTable(name, len);
}

const void*
gfxGraphiteShaper::GetTable(uint32_t aTag, size_t *aLength)
{
    TableRec tableRec;

    if (!mTables.Get(aTag, &tableRec)) {
        hb_blob_t *blob = mFont->GetFontTable(aTag);
        if (blob) {
            
            
            tableRec.mBlob = blob;
            tableRec.mData = hb_blob_get_data(blob, &tableRec.mLength);
            mTables.Put(aTag, tableRec);
        } else {
            return nullptr;
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

static inline uint32_t
MakeGraphiteLangTag(uint32_t aTag)
{
    uint32_t grLangTag = aTag;
    
    uint32_t mask = 0x000000FF;
    while ((grLangTag & mask) == ' ') {
        grLangTag &= ~mask;
        mask <<= 8;
    }
    return grLangTag;
}

struct GrFontFeatures {
    gr_face        *mFace;
    gr_feature_val *mFeatures;
};

static PLDHashOperator
AddFeature(const uint32_t& aTag, uint32_t& aValue, void *aUserArg)
{
    GrFontFeatures *f = static_cast<GrFontFeatures*>(aUserArg);

    const gr_feature_ref* fref = gr_face_find_fref(f->mFace, aTag);
    if (fref) {
        gr_fref_set_feature_value(fref, aValue, f->mFeatures);
    }
    return PL_DHASH_NEXT;
}

bool
gfxGraphiteShaper::ShapeText(gfxContext      *aContext,
                             const PRUnichar *aText,
                             uint32_t         aOffset,
                             uint32_t         aLength,
                             int32_t          aScript,
                             gfxShapedText   *aShapedText)
{
    
    if (!mFont->SetupCairoFont(aContext)) {
        return false;
    }

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
            mGrFace = nullptr;
            return false;
        }
    }

    gfxFontEntry *entry = mFont->GetFontEntry();
    const gfxFontStyle *style = mFont->GetStyle();
    uint32_t grLang = 0;
    if (style->languageOverride) {
        grLang = MakeGraphiteLangTag(style->languageOverride);
    } else if (entry->mLanguageOverride) {
        grLang = MakeGraphiteLangTag(entry->mLanguageOverride);
    } else {
        nsAutoCString langString;
        style->language->ToUTF8String(langString);
        grLang = GetGraphiteTagForLang(langString);
    }
    gr_feature_val *grFeatures = gr_face_featureval_for_lang(mGrFace, grLang);

    nsDataHashtable<nsUint32HashKey,uint32_t> mergedFeatures;

    if (MergeFontFeatures(style->featureSettings, entry->mFeatureSettings,
                          aShapedText->DisableLigatures(), mergedFeatures)) {
        
        GrFontFeatures f = {mGrFace, grFeatures};
        mergedFeatures.Enumerate(AddFeature, &f);
    }

    gr_segment *seg = gr_make_seg(mGrFont, mGrFace, 0, grFeatures,
                                  gr_utf16, aText, aLength,
                                  aShapedText->IsRightToLeft());

    gr_featureval_destroy(grFeatures);

    if (!seg) {
        return false;
    }

    nsresult rv = SetGlyphsFromSegment(aContext, aShapedText, aOffset, aLength,
                                       aText, seg);

    gr_seg_destroy(seg);

    return NS_SUCCEEDED(rv);
}

#define SMALL_GLYPH_RUN 256 // avoid heap allocation of per-glyph data arrays
                            

struct Cluster {
    uint32_t baseChar;
    uint32_t baseGlyph;
    uint32_t nChars;
    uint32_t nGlyphs;
    Cluster() : baseChar(0), baseGlyph(0), nChars(0), nGlyphs(0) { }
};

nsresult
gfxGraphiteShaper::SetGlyphsFromSegment(gfxContext      *aContext,
                                        gfxShapedText   *aShapedText,
                                        uint32_t         aOffset,
                                        uint32_t         aLength,
                                        const PRUnichar *aText,
                                        gr_segment      *aSegment)
{
    int32_t dev2appUnits = aShapedText->GetAppUnitsPerDevUnit();
    bool rtl = aShapedText->IsRightToLeft();

    uint32_t glyphCount = gr_seg_n_slots(aSegment);

    
    nsAutoTArray<Cluster,SMALL_GLYPH_RUN> clusters;
    nsAutoTArray<uint16_t,SMALL_GLYPH_RUN> gids;
    nsAutoTArray<float,SMALL_GLYPH_RUN> xLocs;
    nsAutoTArray<float,SMALL_GLYPH_RUN> yLocs;

    if (!clusters.SetLength(aLength) ||
        !gids.SetLength(glyphCount) ||
        !xLocs.SetLength(glyphCount) ||
        !yLocs.SetLength(glyphCount))
    {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    uint32_t gIndex = 0; 
    uint32_t cIndex = 0; 
    for (const gr_slot *slot = gr_seg_first_slot(aSegment);
         slot != nullptr;
         slot = gr_slot_next_in_segment(slot), gIndex++)
    {
        uint32_t before = gr_slot_before(slot);
        uint32_t after = gr_slot_after(slot);
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
            NS_ASSERTION(cIndex < aLength - 1, "cIndex at end of word");
            Cluster& c = clusters[cIndex + 1];
            c.baseChar = clusters[cIndex].baseChar + clusters[cIndex].nChars;
            c.nChars = before - c.baseChar;
            c.baseGlyph = gIndex;
            c.nGlyphs = 0;
            ++cIndex;
        }

        
        NS_ASSERTION(cIndex < aLength, "cIndex beyond word length");
        ++clusters[cIndex].nGlyphs;

        
        if (clusters[cIndex].baseChar + clusters[cIndex].nChars < after + 1) {
            clusters[cIndex].nChars = after + 1 - clusters[cIndex].baseChar;
        }
    }

    bool roundX;
    bool roundY;
    aContext->GetRoundOffsetsToPixels(&roundX, &roundY);

    gfxShapedText::CompressedGlyph *charGlyphs =
        aShapedText->GetCharacterGlyphs() + aOffset;

    
    for (uint32_t i = 0; i <= cIndex; ++i) {
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

        
        
        uint32_t offs = gr_cinfo_base(gr_seg_cinfo(aSegment, c.baseChar));
        NS_ASSERTION(offs >= c.baseChar && offs < aLength,
                     "unexpected offset");
        if (c.nGlyphs == 1 && c.nChars == 1 &&
            aShapedText->FilterIfIgnorable(aOffset + offs, aText[offs])) {
            continue;
        }

        uint32_t appAdvance = roundX ? NSToIntRound(adv) * dev2appUnits :
                                       NSToIntRound(adv * dev2appUnits);
        if (c.nGlyphs == 1 &&
            gfxShapedText::CompressedGlyph::IsSimpleGlyphID(gids[c.baseGlyph]) &&
            gfxShapedText::CompressedGlyph::IsSimpleAdvance(appAdvance) &&
            yLocs[c.baseGlyph] == 0)
        {
            charGlyphs[offs].SetSimpleGlyph(appAdvance, gids[c.baseGlyph]);
        } else {
            
            nsAutoTArray<gfxShapedText::DetailedGlyph,8> details;
            float clusterLoc;
            for (uint32_t j = c.baseGlyph; j < c.baseGlyph + c.nGlyphs; ++j) {
                gfxShapedText::DetailedGlyph* d = details.AppendElement();
                d->mGlyphID = gids[j];
                d->mYOffset = roundY ? NSToIntRound(-yLocs[j]) * dev2appUnits :
                              -yLocs[j] * dev2appUnits;
                if (j == c.baseGlyph) {
                    d->mXOffset = 0;
                    d->mAdvance = appAdvance;
                    clusterLoc = xLocs[j];
                } else {
                    float dx = rtl ? (xLocs[j] - clusterLoc) :
                                     (xLocs[j] - clusterLoc - adv);
                    d->mXOffset = roundX ? NSToIntRound(dx) * dev2appUnits :
                                           dx * dev2appUnits;
                    d->mAdvance = 0;
                }
            }
            gfxShapedText::CompressedGlyph g;
            g.SetComplex(charGlyphs[offs].IsClusterStart(),
                         true, details.Length());
            aShapedText->SetGlyphs(aOffset + offs, g, details.Elements());
        }

        for (uint32_t j = c.baseChar + 1; j < c.baseChar + c.nChars; ++j) {
            offs = gr_cinfo_base(gr_seg_cinfo(aSegment, j));
            NS_ASSERTION(offs >= j && offs < aLength,
                         "unexpected offset");
            gfxShapedText::CompressedGlyph &g = charGlyphs[offs];
            NS_ASSERTION(!g.IsSimpleGlyph(), "overwriting a simple glyph");
            g.SetComplex(g.IsClusterStart(), false, 0);
        }
    }

    return NS_OK;
}


#include "gfxLanguageTagList.cpp"

nsTHashtable<nsUint32HashKey> gfxGraphiteShaper::sLanguageTags;

 uint32_t
gfxGraphiteShaper::GetGraphiteTagForLang(const nsCString& aLang)
{
    int len = aLang.Length();
    if (len < 2) {
        return 0;
    }

    
    
    uint32_t grLang = 0;
    for (int i = 0; i < 4; ++i) {
        grLang <<= 8;
        if (i < len) {
            uint8_t ch = aLang[i];
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
        for (const uint32_t *tag = sLanguageTagList; *tag != 0; ++tag) {
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
