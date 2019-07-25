





































#include "prtypes.h"
#include "prmem.h"
#include "nsString.h"
#include "nsBidiUtils.h"
#include "nsMathUtils.h"

#include "gfxTypes.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxFontUtils.h"
#include "gfxUnicodeProperties.h"

#include "harfbuzz/hb-unicode.h"
#include "harfbuzz/hb-ot.h"

#include "cairo.h"

#include "nsUnicodeRange.h"
#include "nsCRT.h"

#define FloatToFixed(f) (65536 * (f))
#define FixedToFloat(f) ((f) * (1.0 / 65536.0))



#define FixedToIntRound(f) ((f) > 0 ?  ((32768 + (f)) >> 16) \
                                    : -((32767 - (f)) >> 16))

using namespace mozilla; 





gfxHarfBuzzShaper::gfxHarfBuzzShaper(gfxFont *aFont)
    : gfxFontShaper(aFont),
      mHBFace(nsnull),
      mKernTable(nsnull),
      mHmtxTable(nsnull),
      mNumLongMetrics(0),
      mCmapTable(nsnull),
      mCmapFormat(-1),
      mSubtableOffset(0),
      mUVSTableOffset(0),
      mUseFontGetGlyph(aFont->ProvidesGetGlyph()),
      mUseFontGlyphWidths(PR_FALSE)
{
}

gfxHarfBuzzShaper::~gfxHarfBuzzShaper()
{
    hb_blob_destroy(mCmapTable);
    hb_blob_destroy(mHmtxTable);
    hb_blob_destroy(mKernTable);
    hb_face_destroy(mHBFace);
}







static hb_blob_t *
HBGetTable(hb_tag_t aTag, void *aUserData)
{
    gfxHarfBuzzShaper *shaper = static_cast<gfxHarfBuzzShaper*>(aUserData);
    gfxFont *font = shaper->GetFont();

    
    
    if (aTag == TRUETYPE_TAG('G','D','E','F') &&
        font->GetFontEntry()->IgnoreGDEF())
    {
        return nsnull;
    }

    return font->GetFontTable(aTag);
}






struct FontCallbackData {
    FontCallbackData(gfxHarfBuzzShaper *aShaper, gfxContext *aContext)
        : mShaper(aShaper), mContext(aContext)
    { }

    gfxHarfBuzzShaper *mShaper;
    gfxContext        *mContext;
};

#define UNICODE_BMP_LIMIT 0x10000

hb_codepoint_t
gfxHarfBuzzShaper::GetGlyph(hb_codepoint_t unicode,
                            hb_codepoint_t variation_selector) const
{
    if (mUseFontGetGlyph) {
        return mFont->GetGlyph(unicode, variation_selector);
    }

    
    NS_ASSERTION(mFont->GetFontEntry()->HasCmapTable(),
                 "we cannot be using this font!");

    NS_ASSERTION(mCmapTable && (mCmapFormat > 0) && (mSubtableOffset > 0),
                 "cmap data not correctly set up, expect disaster");

    const PRUint8* data = (const PRUint8*)hb_blob_lock(mCmapTable);

    hb_codepoint_t gid;
    switch (mCmapFormat) {
    case 4:
        gid = unicode < UNICODE_BMP_LIMIT ?
            gfxFontUtils::MapCharToGlyphFormat4(data + mSubtableOffset, unicode) : 0;
        break;
    case 12:
        gid = gfxFontUtils::MapCharToGlyphFormat12(data + mSubtableOffset, unicode);
        break;
    default:
        NS_WARNING("unsupported cmap format, glyphs will be missing");
        gid = 0;
        break;
    }

    if (gid && variation_selector && mUVSTableOffset) {
        hb_codepoint_t varGID =
            gfxFontUtils::MapUVSToGlyphFormat14(data + mUVSTableOffset,
                                                unicode, variation_selector);
        if (varGID) {
            gid = varGID;
        }
        
        
    }

    hb_blob_unlock(mCmapTable);

    return gid;
}

static hb_codepoint_t
HBGetGlyph(hb_font_t *font, hb_face_t *face, const void *user_data,
           hb_codepoint_t unicode, hb_codepoint_t variation_selector)
{
    const FontCallbackData *fcd =
        static_cast<const FontCallbackData*>(user_data);
    return fcd->mShaper->GetGlyph(unicode, variation_selector);
}

struct HMetricsHeader {
    AutoSwap_PRUint32    tableVersionNumber;
    AutoSwap_PRInt16     ascender;
    AutoSwap_PRInt16     descender;
    AutoSwap_PRInt16     lineGap;
    AutoSwap_PRUint16    advanceWidthMax;
    AutoSwap_PRInt16     minLeftSideBearing;
    AutoSwap_PRInt16     minRightSideBearing;
    AutoSwap_PRInt16     xMaxExtent;
    AutoSwap_PRInt16     caretSlopeRise;
    AutoSwap_PRInt16     caretSlopeRun;
    AutoSwap_PRInt16     caretOffset;
    AutoSwap_PRInt16     reserved[4];
    AutoSwap_PRInt16     metricDataFormat;
    AutoSwap_PRUint16    numberOfHMetrics;
};

struct HLongMetric {
    AutoSwap_PRUint16    advanceWidth;
    AutoSwap_PRInt16     lsb;
};

struct HMetrics {
    HLongMetric          metrics[1]; 


};

void
gfxHarfBuzzShaper::GetGlyphAdvance(gfxContext *aContext,
                                   hb_codepoint_t glyph,
                                   hb_position_t *x_advance,
                                   hb_position_t *y_advance) const
{
    if (mUseFontGlyphWidths) {
        *x_advance = mFont->GetGlyphWidth(aContext, glyph);
        return;
    }

    
    

    NS_ASSERTION((mNumLongMetrics > 0) && mHmtxTable != nsnull,
                 "font is lacking metrics, we shouldn't be here");

    if (glyph >= PRUint32(mNumLongMetrics)) {
        glyph = mNumLongMetrics - 1;
    }

    
    
    
    const HMetrics* hmtx =
        reinterpret_cast<const HMetrics*>(hb_blob_lock(mHmtxTable));
    *x_advance =
        FloatToFixed(mFont->FUnitsToDevUnitsFactor() *
                     PRUint16(hmtx->metrics[glyph].advanceWidth));
    hb_blob_unlock(mHmtxTable);
}

static void
HBGetGlyphAdvance(hb_font_t *font, hb_face_t *face, const void *user_data,
                  hb_codepoint_t glyph,
                  hb_position_t *x_advance, hb_position_t *y_advance)
{
    const FontCallbackData *fcd =
        static_cast<const FontCallbackData*>(user_data);
    fcd->mShaper->GetGlyphAdvance(fcd->mContext, glyph, x_advance, y_advance);
}

static hb_bool_t
HBGetContourPoint(hb_font_t *font, hb_face_t *face, const void *user_data,
                  unsigned int point_index, hb_codepoint_t glyph,
                  hb_position_t *x, hb_position_t *y)
{
    

    return false;
}

struct KernHeaderFmt0 {
    AutoSwap_PRUint16 nPairs;
    AutoSwap_PRUint16 searchRange;
    AutoSwap_PRUint16 entrySelector;
    AutoSwap_PRUint16 rangeShift;
};

struct KernPair {
    AutoSwap_PRUint16 left;
    AutoSwap_PRUint16 right;
    AutoSwap_PRInt16  value;
};









static void
GetKernValueFmt0(const void* aSubtable,
                 PRUint32 aSubtableLen,
                 PRUint16 aFirstGlyph,
                 PRUint16 aSecondGlyph,
                 PRInt32& aValue,
                 PRBool   aIsOverride = PR_FALSE,
                 PRBool   aIsMinimum = PR_FALSE)
{
    const KernHeaderFmt0* hdr =
        reinterpret_cast<const KernHeaderFmt0*>(aSubtable);

    const KernPair *lo = reinterpret_cast<const KernPair*>(hdr + 1);
    const KernPair *hi = lo + PRUint16(hdr->nPairs);
    const KernPair *limit = hi;

    if (reinterpret_cast<const char*>(aSubtable) + aSubtableLen <
        reinterpret_cast<const char*>(hi)) {
        
        
        return;
    }

#define KERN_PAIR_KEY(l,r) (PRUint32((PRUint16(l) << 16) + PRUint16(r)))

    PRUint32 key = KERN_PAIR_KEY(aFirstGlyph, aSecondGlyph);
    while (lo < hi) {
        const KernPair *mid = lo + (hi - lo) / 2;
        if (KERN_PAIR_KEY(mid->left, mid->right) < key) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }

    if (lo < limit && KERN_PAIR_KEY(lo->left, lo->right) == key) {
        if (aIsOverride) {
            aValue = PRInt16(lo->value);
        } else if (aIsMinimum) {
            aValue = PR_MAX(aValue, PRInt16(lo->value));
        } else {
            aValue += PRInt16(lo->value);
        }
    }
}







struct KernHeaderVersion1Fmt2 {
    KernTableSubtableHeaderVersion1 header;
    AutoSwap_PRUint16 rowWidth;
    AutoSwap_PRUint16 leftOffsetTable;
    AutoSwap_PRUint16 rightOffsetTable;
    AutoSwap_PRUint16 array;
};

struct KernClassTableHdr {
    AutoSwap_PRUint16 firstGlyph;
    AutoSwap_PRUint16 nGlyphs;
    AutoSwap_PRUint16 offsets[1]; 
};

static PRInt16
GetKernValueVersion1Fmt2(const void* aSubtable,
                         PRUint32 aSubtableLen,
                         PRUint16 aFirstGlyph,
                         PRUint16 aSecondGlyph)
{
    if (aSubtableLen < sizeof(KernHeaderVersion1Fmt2)) {
        return 0;
    }

    const char* base = reinterpret_cast<const char*>(aSubtable);
    const char* subtableEnd = base + aSubtableLen;

    const KernHeaderVersion1Fmt2* h =
        reinterpret_cast<const KernHeaderVersion1Fmt2*>(aSubtable);
    PRUint32 offset = h->array;

    const KernClassTableHdr* leftClassTable =
        reinterpret_cast<const KernClassTableHdr*>(base +
                                                   PRUint16(h->leftOffsetTable));
    if (reinterpret_cast<const char*>(leftClassTable) +
        sizeof(KernClassTableHdr) > subtableEnd) {
        return 0;
    }
    if (aFirstGlyph >= PRUint16(leftClassTable->firstGlyph)) {
        aFirstGlyph -= PRUint16(leftClassTable->firstGlyph);
        if (aFirstGlyph < PRUint16(leftClassTable->nGlyphs)) {
            if (reinterpret_cast<const char*>(leftClassTable) +
                sizeof(KernClassTableHdr) +
                aFirstGlyph * sizeof(PRUint16) >= subtableEnd) {
                return 0;
            }
            offset = PRUint16(leftClassTable->offsets[aFirstGlyph]);
        }
    }

    const KernClassTableHdr* rightClassTable =
        reinterpret_cast<const KernClassTableHdr*>(base +
                                                   PRUint16(h->rightOffsetTable));
    if (reinterpret_cast<const char*>(rightClassTable) +
        sizeof(KernClassTableHdr) > subtableEnd) {
        return 0;
    }
    if (aSecondGlyph >= PRUint16(rightClassTable->firstGlyph)) {
        aSecondGlyph -= PRUint16(rightClassTable->firstGlyph);
        if (aSecondGlyph < PRUint16(rightClassTable->nGlyphs)) {
            if (reinterpret_cast<const char*>(rightClassTable) +
                sizeof(KernClassTableHdr) +
                aSecondGlyph * sizeof(PRUint16) >= subtableEnd) {
                return 0;
            }
            offset += PRUint16(rightClassTable->offsets[aSecondGlyph]);
        }
    }

    const AutoSwap_PRInt16* pval =
        reinterpret_cast<const AutoSwap_PRInt16*>(base + offset);
    if (reinterpret_cast<const char*>(pval + 1) >= subtableEnd) {
        return 0;
    }
    return *pval;
}







struct KernHeaderVersion1Fmt3 {
    KernTableSubtableHeaderVersion1 header;
    AutoSwap_PRUint16 glyphCount;
    PRUint8 kernValueCount;
    PRUint8 leftClassCount;
    PRUint8 rightClassCount;
    PRUint8 flags;
};

static PRInt16
GetKernValueVersion1Fmt3(const void* aSubtable,
                         PRUint32 aSubtableLen,
                         PRUint16 aFirstGlyph,
                         PRUint16 aSecondGlyph)
{
    
    if (aSubtableLen < sizeof(KernHeaderVersion1Fmt3)) {
        return 0;
    }

    const KernHeaderVersion1Fmt3* hdr =
        reinterpret_cast<const KernHeaderVersion1Fmt3*>(aSubtable);
    if (hdr->flags != 0) {
        return 0;
    }

    PRUint16 glyphCount = hdr->glyphCount;

    
    if (sizeof(KernHeaderVersion1Fmt3) +
        hdr->kernValueCount * sizeof(PRInt16) +
        glyphCount + glyphCount +
        hdr->leftClassCount * hdr->rightClassCount > aSubtableLen) {
        return 0;
    }
        
    if (aFirstGlyph >= glyphCount || aSecondGlyph >= glyphCount) {
        
        return 0;
    }

    
    const AutoSwap_PRInt16* kernValue =
        reinterpret_cast<const AutoSwap_PRInt16*>(hdr + 1);
    const PRUint8* leftClass =
        reinterpret_cast<const PRUint8*>(kernValue + hdr->kernValueCount);
    const PRUint8* rightClass = leftClass + glyphCount;
    const PRUint8* kernIndex = rightClass + glyphCount;

    PRUint8 lc = leftClass[aFirstGlyph];
    PRUint8 rc = rightClass[aSecondGlyph];
    if (lc >= hdr->leftClassCount || rc >= hdr->rightClassCount) {
        return 0;
    }

    PRUint8 ki = kernIndex[leftClass[aFirstGlyph] * hdr->rightClassCount +
                           rightClass[aSecondGlyph]];
    if (ki >= hdr->kernValueCount) {
        return 0;
    }

    return kernValue[ki];
}

#define KERN0_COVERAGE_HORIZONTAL   0x0001
#define KERN0_COVERAGE_MINIMUM      0x0002
#define KERN0_COVERAGE_CROSS_STREAM 0x0004
#define KERN0_COVERAGE_OVERRIDE     0x0008
#define KERN0_COVERAGE_RESERVED     0x00F0

#define KERN1_COVERAGE_VERTICAL     0x8000
#define KERN1_COVERAGE_CROSS_STREAM 0x4000
#define KERN1_COVERAGE_VARIATION    0x2000
#define KERN1_COVERAGE_RESERVED     0x1F00

hb_position_t
gfxHarfBuzzShaper::GetKerning(PRUint16 aFirstGlyph,
                              PRUint16 aSecondGlyph) const
{
    
    
    
    PRUint32 spaceGlyph = mFont->GetSpaceGlyph();
    if (aFirstGlyph == spaceGlyph || aSecondGlyph == spaceGlyph) {
        return 0;
    }

    if (!mKernTable) {
        mKernTable = mFont->GetFontTable(TRUETYPE_TAG('k','e','r','n'));
        if (!mKernTable) {
            mKernTable = hb_blob_create_empty();
        }
    }

    PRUint32 len = hb_blob_get_length(mKernTable);
    if (len < sizeof(KernTableVersion0)) {
        return 0;
    }

    PRInt32 value = 0;
    const char* base = reinterpret_cast<const char*>(hb_blob_lock(mKernTable));

    
    
    const KernTableVersion0* kern0 =
        reinterpret_cast<const KernTableVersion0*>(base);
    if (PRUint16(kern0->version) == 0) {
        PRUint16 nTables = kern0->nTables;
        PRUint32 offs = sizeof(KernTableVersion0);
        for (PRUint16 i = 0; i < nTables; ++i) {
            if (offs + sizeof(KernTableSubtableHeaderVersion0) > len) {
                break;
            }
            const KernTableSubtableHeaderVersion0* st0 =
                reinterpret_cast<const KernTableSubtableHeaderVersion0*>
                                (base + offs);
            PRUint16 subtableLen = PRUint16(st0->length);
            if (offs + subtableLen > len) {
                break;
            }
            offs += subtableLen;
            PRUint16 coverage = st0->coverage;
            if (!(coverage & KERN0_COVERAGE_HORIZONTAL)) {
                
                continue;
            }
            if (coverage &
                (KERN0_COVERAGE_CROSS_STREAM | KERN0_COVERAGE_RESERVED)) {
                
                
                
                continue;
            }
            PRUint8 format = (coverage >> 8);
            switch (format) {
            case 0:
                GetKernValueFmt0(st0 + 1, subtableLen - sizeof(*st0),
                                 aFirstGlyph, aSecondGlyph, value,
                                 (coverage & KERN0_COVERAGE_OVERRIDE) != 0,
                                 (coverage & KERN0_COVERAGE_MINIMUM) != 0);
                break;
            default:
                
                
#if DEBUG
                {
                    char buf[1024];
                    sprintf(buf, "unknown kern subtable in %s: "
                                 "ver 0 format %d\n",
                            NS_ConvertUTF16toUTF8(mFont->GetName()).get(),
                            format);
                    NS_WARNING(buf);
                }
#endif
                break;
            }
        }
    } else {
        
        
        const KernTableVersion1* kern1 =
            reinterpret_cast<const KernTableVersion1*>(base);
        if (PRUint32(kern1->version) == 0x00010000) {
            PRUint32 nTables = kern1->nTables;
            PRUint32 offs = sizeof(KernTableVersion1);
            for (PRUint32 i = 0; i < nTables; ++i) {
                if (offs + sizeof(KernTableSubtableHeaderVersion1) > len) {
                    break;
                }
                const KernTableSubtableHeaderVersion1* st1 =
                    reinterpret_cast<const KernTableSubtableHeaderVersion1*>
                                    (base + offs);
                PRUint32 subtableLen = PRUint32(st1->length);
                offs += subtableLen;
                PRUint16 coverage = st1->coverage;
                if (coverage &
                    (KERN1_COVERAGE_VERTICAL     |
                     KERN1_COVERAGE_CROSS_STREAM |
                     KERN1_COVERAGE_VARIATION    |
                     KERN1_COVERAGE_RESERVED)) {
                    
                    
                    
                    
                    
                    continue;
                }
                PRUint8 format = (coverage & 0xff);
                switch (format) {
                case 0:
                    GetKernValueFmt0(st1 + 1, subtableLen - sizeof(*st1),
                                     aFirstGlyph, aSecondGlyph, value);
                    break;
                case 2:
                    value = GetKernValueVersion1Fmt2(st1, subtableLen,
                                                     aFirstGlyph, aSecondGlyph);
                    break;
                case 3:
                    value = GetKernValueVersion1Fmt3(st1, subtableLen,
                                                     aFirstGlyph, aSecondGlyph);
                    break;
                default:
                    
                    
                    
                    
#if DEBUG
                    {
                        char buf[1024];
                        sprintf(buf, "unknown kern subtable in %s: "
                                     "ver 0 format %d\n",
                                NS_ConvertUTF16toUTF8(mFont->GetName()).get(),
                                format);
                        NS_WARNING(buf);
                    }
#endif
                    break;
                }
            }
        }
    }

    hb_blob_unlock(mKernTable);

    if (value != 0) {
        return FloatToFixed(mFont->FUnitsToDevUnitsFactor() * value);
    }
    return 0;
}

static hb_position_t
HBGetKerning(hb_font_t *font, hb_face_t *face, const void *user_data,
             hb_codepoint_t first_glyph, hb_codepoint_t second_glyph)
{
    const FontCallbackData *fcd =
        static_cast<const FontCallbackData*>(user_data);
    return fcd->mShaper->GetKerning(first_glyph, second_glyph);
}





static hb_codepoint_t
HBGetMirroring(hb_codepoint_t aCh)
{
    return gfxUnicodeProperties::GetMirroredChar(aCh);
}

static hb_category_t
HBGetGeneralCategory(hb_codepoint_t aCh)
{
    return hb_category_t(gfxUnicodeProperties::GetGeneralCategory(aCh));
}

static hb_script_t
HBGetScript(hb_codepoint_t aCh)
{
    return hb_script_t(gfxUnicodeProperties::GetScriptCode(aCh));
}

static unsigned int
HBGetCombiningClass(hb_codepoint_t aCh)
{
    return gfxUnicodeProperties::GetCombiningClass(aCh);
}

static unsigned int
HBGetEastAsianWidth(hb_codepoint_t aCh)
{
    return gfxUnicodeProperties::GetEastAsianWidth(aCh);
}





static hb_font_funcs_t * sHBFontFuncs = nsnull;
static hb_unicode_funcs_t * sHBUnicodeFuncs = nsnull;

PRBool
gfxHarfBuzzShaper::InitTextRun(gfxContext *aContext,
                               gfxTextRun *aTextRun,
                               const PRUnichar *aString,
                               PRUint32 aRunStart,
                               PRUint32 aRunLength,
                               PRInt32 aRunScript)
{
    
    mFont->SetupCairoFont(aContext);

    if (!mHBFace) {

        mUseFontGlyphWidths = mFont->ProvidesGlyphWidths();

        

        if (!sHBFontFuncs) {
            
            
            sHBFontFuncs = hb_font_funcs_create();
            hb_font_funcs_set_glyph_func(sHBFontFuncs, HBGetGlyph);
            hb_font_funcs_set_glyph_advance_func(sHBFontFuncs,
                                                 HBGetGlyphAdvance);
            hb_font_funcs_set_contour_point_func(sHBFontFuncs,
                                                 HBGetContourPoint);
            hb_font_funcs_set_kerning_func(sHBFontFuncs, HBGetKerning);

            sHBUnicodeFuncs = hb_unicode_funcs_create();
            hb_unicode_funcs_set_mirroring_func(sHBUnicodeFuncs,
                                                HBGetMirroring);
            hb_unicode_funcs_set_script_func(sHBUnicodeFuncs, HBGetScript);
            hb_unicode_funcs_set_general_category_func(sHBUnicodeFuncs,
                                                       HBGetGeneralCategory);
            hb_unicode_funcs_set_combining_class_func(sHBUnicodeFuncs,
                                                      HBGetCombiningClass);
            hb_unicode_funcs_set_eastasian_width_func(sHBUnicodeFuncs,
                                                      HBGetEastAsianWidth);
        }

        mHBFace = hb_face_create_for_tables(HBGetTable, nsnull, this);

        if (!mUseFontGetGlyph) {
            
            mCmapTable = mFont->GetFontTable(TRUETYPE_TAG('c','m','a','p'));
            if (!mCmapTable) {
                NS_WARNING("failed to load cmap, glyphs will be missing");
                return PR_FALSE;
            }
            const PRUint8* data = (const PRUint8*)hb_blob_lock(mCmapTable);
            PRBool symbol;
            mCmapFormat = gfxFontUtils::
                FindPreferredSubtable(data, hb_blob_get_length(mCmapTable),
                                      &mSubtableOffset, &mUVSTableOffset,
                                      &symbol);
            hb_blob_unlock(mCmapTable);
        }

        if (!mUseFontGlyphWidths) {
            
            
            
            
            hb_blob_t *hheaTable =
                mFont->GetFontTable(TRUETYPE_TAG('h','h','e','a'));
            if (hheaTable &&
                hb_blob_get_length(hheaTable) >= sizeof(HMetricsHeader)) {
                const HMetricsHeader* hhea =
                    reinterpret_cast<const HMetricsHeader*>
                        (hb_blob_lock(hheaTable));
                mNumLongMetrics = hhea->numberOfHMetrics;
                hb_blob_unlock(hheaTable);

                if (mNumLongMetrics > 0 &&
                    PRInt16(hhea->metricDataFormat) == 0) {
                    
                    
                    
                    mHmtxTable =
                        mFont->GetFontTable(TRUETYPE_TAG('h','m','t','x'));
                    if (hb_blob_get_length(mHmtxTable) <
                        mNumLongMetrics * sizeof(HLongMetric)) {
                        
                        
                        hb_blob_destroy(mHmtxTable);
                        mHmtxTable = nsnull;
                    }
                }
            }
            hb_blob_destroy(hheaTable);
        }
    }

    if ((!mUseFontGetGlyph && mCmapFormat <= 0) ||
        (!mUseFontGlyphWidths && !mHmtxTable)) {
        
        return PR_FALSE;
    }

    FontCallbackData fcd(this, aContext);
    hb_font_t *font = hb_font_create();
    hb_font_set_funcs(font, sHBFontFuncs, nsnull, &fcd);
    hb_font_set_ppem(font, mFont->GetAdjustedSize(), mFont->GetAdjustedSize());
    PRUint32 scale = FloatToFixed(mFont->GetAdjustedSize()); 
    hb_font_set_scale(font, scale, scale);

    
    

    PRBool disableLigatures =
        (aTextRun->GetFlags() &
         gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;

    nsAutoTArray<hb_feature_t,20> features;

    
    
    if (disableLigatures) {
        hb_feature_t ligaOff = { HB_TAG('l','i','g','a'), 0, 0, -1 };
        hb_feature_t cligOff = { HB_TAG('c','l','i','g'), 0, 0, -1 };
        features.AppendElement(ligaOff);
        features.AppendElement(cligOff);
    }

    
    const gfxFontStyle *style = aTextRun->GetFontGroup()->GetStyle();
    const nsTArray<gfxFontFeature> *cssFeatures = &style->featureSettings;
    if (cssFeatures->IsEmpty()) {
        cssFeatures = &mFont->GetFontEntry()->mFeatureSettings;
    }
    for (PRUint32 i = 0; i < cssFeatures->Length(); ++i) {
        PRUint32 j;
        for (j = 0; j < features.Length(); ++j) {
            if (cssFeatures->ElementAt(i).mTag == features[j].tag) {
                features[j].value = cssFeatures->ElementAt(i).mValue;
                break;
            }
        }
        if (j == features.Length()) {
            const gfxFontFeature& f = cssFeatures->ElementAt(i);
            hb_feature_t hbf = { f.mTag, f.mValue, 0, -1 };
            features.AppendElement(hbf);
        }
    }

    hb_buffer_t *buffer = hb_buffer_create(aRunLength);
    hb_buffer_set_unicode_funcs(buffer, sHBUnicodeFuncs);
    hb_buffer_set_direction(buffer,
                            aTextRun->IsRightToLeft() ?
                                HB_DIRECTION_RTL : HB_DIRECTION_LTR);
    
    
    
    hb_buffer_set_script(buffer,
                         aRunScript <= HB_SCRIPT_INHERITED ? HB_SCRIPT_LATIN
                         : hb_script_t(aRunScript));

    hb_language_t language;
    if (style->languageOverride) {
        language = hb_ot_tag_to_language(style->languageOverride);
    } else if (mFont->GetFontEntry()->mLanguageOverride) {
        language =
            hb_ot_tag_to_language(mFont->GetFontEntry()->mLanguageOverride);
    } else {
        nsCString langString;
        style->language->ToUTF8String(langString);
        language = hb_language_from_string(langString.get());
    }
    hb_buffer_set_language(buffer, language);

    hb_buffer_add_utf16(buffer, reinterpret_cast<const uint16_t*>(aString + aRunStart),
                        aRunLength, 0, aRunLength);

    hb_shape(font, mHBFace, buffer, features.Elements(), features.Length());

    if (aTextRun->IsRightToLeft()) {
        hb_buffer_reverse(buffer);
    }

#ifdef DEBUG
    nsresult rv =
#endif
    SetGlyphsFromRun(aContext, aTextRun, buffer, aRunStart, aRunLength);
    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to store glyphs into textrun");
    hb_buffer_destroy(buffer);
    hb_font_destroy(font);

    return PR_TRUE;
}










static void
GetRoundOffsetsToPixels(gfxContext *aContext,
                        PRBool *aRoundX, PRBool *aRoundY)
{
    *aRoundX = PR_FALSE;
    
    
    
    if (aContext->CurrentMatrix().HasNonTranslation()) {
        *aRoundY = PR_FALSE;
        return;
    }

    
    
    *aRoundY = PR_TRUE;

    cairo_t *cr = aContext->GetCairo();
    cairo_scaled_font_t *scaled_font = cairo_get_scaled_font(cr);
    
    cairo_font_options_t *font_options = cairo_font_options_create();
    cairo_scaled_font_get_font_options(scaled_font, font_options);
    cairo_hint_metrics_t hint_metrics =
        cairo_font_options_get_hint_metrics(font_options);
    cairo_font_options_destroy(font_options);

    switch (hint_metrics) {
    case CAIRO_HINT_METRICS_OFF:
        *aRoundY = PR_FALSE;
        return;
    case CAIRO_HINT_METRICS_DEFAULT:
        
        
        
        
        
        switch (cairo_scaled_font_get_type(scaled_font)) {
#if CAIRO_HAS_DWRITE_FONT 
        case CAIRO_FONT_TYPE_DWRITE:
            
            
            return;
#endif
        case CAIRO_FONT_TYPE_QUARTZ:
            
            if (cairo_surface_get_type(cairo_get_target(cr)) ==
                CAIRO_SURFACE_TYPE_QUARTZ) {
                return;
            }
        default:
            break;
        }
        
    case CAIRO_HINT_METRICS_ON:
        break;
    }
    *aRoundX = PR_TRUE;
    return;
}

#define SMALL_GLYPH_RUN 128 // some testing indicates that 90%+ of text runs
                            
                            

nsresult
gfxHarfBuzzShaper::SetGlyphsFromRun(gfxContext *aContext,
                                    gfxTextRun *aTextRun,
                                    hb_buffer_t *aBuffer,
                                    PRUint32 aTextRunOffset,
                                    PRUint32 aRunLength)
{
    PRInt32 numGlyphs = hb_buffer_get_length(aBuffer);
    if (numGlyphs == 0)
        return NS_OK;

    const hb_glyph_info_t *ginfo = hb_buffer_get_glyph_infos(aBuffer);

    nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;

    static const PRInt32 NO_GLYPH = -1;
    nsAutoTArray<PRInt32,SMALL_GLYPH_RUN> charToGlyphArray;
    if (!charToGlyphArray.SetLength(aRunLength))
        return NS_ERROR_OUT_OF_MEMORY;

    PRInt32 *charToGlyph = charToGlyphArray.Elements();
    for (PRUint32 offset = 0; offset < aRunLength; ++offset) {
        charToGlyph[offset] = NO_GLYPH;
    }

    for (PRInt32 i = 0; i < numGlyphs; ++i) {
        PRUint32 loc = ginfo[i].cluster;
        if (loc < aRunLength) {
            charToGlyph[loc] = i;
        }
    }

    PRInt32 glyphStart = 0; 
    PRInt32 charStart = 0; 

    PRBool roundX;
    PRBool roundY;
    GetRoundOffsetsToPixels(aContext, &roundX, &roundY);
    
    PRInt32 dev2appUnits = aTextRun->GetAppUnitsPerDevUnit();
    
    
    double hb2appUnits = FixedToFloat(aTextRun->GetAppUnitsPerDevUnit());

    
    nscoord yPos = 0;

    const hb_glyph_position_t *posInfo = hb_buffer_get_glyph_positions(aBuffer);

    while (glyphStart < numGlyphs) {

        PRBool inOrder = PR_TRUE;
        PRInt32 charEnd = ginfo[glyphStart].cluster;
        PRInt32 glyphEnd = glyphStart;
        PRInt32 charLimit = aRunLength;
        while (charEnd < charLimit) {
            
            
            
            
            
            charEnd += 1;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += 1;
            }

            
            for (PRInt32 i = charStart; i < charEnd; ++i) {
                if (charToGlyph[i] != NO_GLYPH) {
                    glyphEnd = PR_MAX(glyphEnd, charToGlyph[i] + 1);
                    
                }
            }

            if (glyphEnd == glyphStart + 1) {
                
                
                break;
            }

            if (glyphEnd == glyphStart) {
                
                continue;
            }

            
            
            
            PRBool allGlyphsAreWithinCluster = PR_TRUE;
            PRInt32 prevGlyphCharIndex = charStart - 1;
            for (PRInt32 i = glyphStart; i < glyphEnd; ++i) {
                PRInt32 glyphCharIndex = ginfo[i].cluster;
                if (glyphCharIndex < charStart || glyphCharIndex >= charEnd) {
                    allGlyphsAreWithinCluster = PR_FALSE;
                    break;
                }
                if (glyphCharIndex <= prevGlyphCharIndex) {
                    inOrder = PR_FALSE;
                }
                prevGlyphCharIndex = glyphCharIndex;
            }
            if (allGlyphsAreWithinCluster) {
                break;
            }
        }

        NS_ASSERTION(glyphStart < glyphEnd,
                     "character/glyph clump contains no glyphs!");
        NS_ASSERTION(charStart != charEnd,
                     "character/glyph clump contains no characters!");

        
        
        
        
        PRInt32 baseCharIndex, endCharIndex;
        while (charEnd < PRInt32(aRunLength) && charToGlyph[charEnd] == NO_GLYPH)
            charEnd++;
        baseCharIndex = charStart;
        endCharIndex = charEnd;

        
        
        if (baseCharIndex >= PRInt32(aRunLength)) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }
        
        endCharIndex = NS_MIN<PRInt32>(endCharIndex, aRunLength);

        
        PRInt32 glyphsInClump = glyphEnd - glyphStart;

        
        
        
        if (glyphsInClump == 1 && baseCharIndex + 1 == endCharIndex &&
            aTextRun->FilterIfIgnorable(aTextRunOffset + baseCharIndex)) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        
        hb_position_t x_advance = posInfo[glyphStart].x_advance;
        nscoord advance =
            roundX ? dev2appUnits * FixedToIntRound(x_advance)
            : NS_floor(hb2appUnits * x_advance + 0.5);

        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(ginfo[glyphStart].codepoint) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            aTextRun->IsClusterStart(aTextRunOffset + baseCharIndex) &&
            posInfo[glyphStart].x_offset == 0 &&
            posInfo[glyphStart].y_offset == 0 && yPos == 0)
        {
            gfxTextRun::CompressedGlyph g;
            aTextRun->SetSimpleGlyph(aTextRunOffset + baseCharIndex,
                                     g.SetSimpleGlyph(advance,
                                         ginfo[glyphStart].codepoint));
        } else {
            
            
            
            
            while (1) {
                gfxTextRun::DetailedGlyph* details =
                    detailedGlyphs.AppendElement();
                details->mGlyphID = ginfo[glyphStart].codepoint;

                
                
                
                
                hb_position_t x_offset = posInfo[glyphStart].x_offset;
                details->mXOffset =
                    roundX ? dev2appUnits * FixedToIntRound(x_offset)
                    : NS_floor(hb2appUnits * x_offset + 0.5);
                hb_position_t y_offset = posInfo[glyphStart].y_offset;
                details->mYOffset = yPos -
                    (roundY ? dev2appUnits * FixedToIntRound(y_offset)
                     : NS_floor(hb2appUnits * y_offset + 0.5));

                details->mAdvance = advance;
                hb_position_t y_advance = posInfo[glyphStart].y_advance;
                if (y_advance != 0) {
                    yPos -=
                        roundY ? dev2appUnits * FixedToIntRound(y_advance)
                        : NS_floor(hb2appUnits * y_advance + 0.5);
                }
                if (++glyphStart >= glyphEnd) {
                    break;
                }
                x_advance = posInfo[glyphStart].x_advance;
                advance =
                    roundX ? dev2appUnits * FixedToIntRound(x_advance)
                    : NS_floor(hb2appUnits * x_advance + 0.5);
            }

            gfxTextRun::CompressedGlyph g;
            g.SetComplex(aTextRun->IsClusterStart(aTextRunOffset + baseCharIndex),
                         PR_TRUE, detailedGlyphs.Length());
            aTextRun->SetGlyphs(aTextRunOffset + baseCharIndex,
                                g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        
        while (++baseCharIndex != endCharIndex &&
               baseCharIndex < PRInt32(aRunLength)) {
            gfxTextRun::CompressedGlyph g;
            g.SetComplex(inOrder &&
                         aTextRun->IsClusterStart(aTextRunOffset + baseCharIndex),
                         PR_FALSE, 0);
            aTextRun->SetGlyphs(aTextRunOffset + baseCharIndex, g, nsnull);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return NS_OK;
}
