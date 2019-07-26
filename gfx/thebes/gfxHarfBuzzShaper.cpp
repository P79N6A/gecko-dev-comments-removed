




#include "nsAlgorithm.h"
#include "nsString.h"
#include "nsBidiUtils.h"
#include "nsMathUtils.h"

#include "gfxTypes.h"

#include "gfxContext.h"
#include "gfxPlatform.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxFontUtils.h"
#include "nsUnicodeProperties.h"
#include "nsUnicodeScriptCodes.h"
#include "nsUnicodeNormalizer.h"

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ot.h"

#include "cairo.h"

#include "nsCRT.h"

#if defined(XP_WIN)
#include "gfxWindowsPlatform.h"
#endif

#define FloatToFixed(f) (65536 * (f))
#define FixedToFloat(f) ((f) * (1.0 / 65536.0))



#define FixedToIntRound(f) ((f) > 0 ?  ((32768 + (f)) >> 16) \
                                    : -((32767 - (f)) >> 16))

using namespace mozilla; 
using namespace mozilla::unicode; 





gfxHarfBuzzShaper::gfxHarfBuzzShaper(gfxFont *aFont)
    : gfxFontShaper(aFont),
      mHBFace(nullptr),
      mKernTable(nullptr),
      mHmtxTable(nullptr),
      mNumLongMetrics(0),
      mCmapTable(nullptr),
      mCmapFormat(-1),
      mSubtableOffset(0),
      mUVSTableOffset(0),
      mUseFontGetGlyph(aFont->ProvidesGetGlyph()),
      mUseFontGlyphWidths(false)
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
HBGetTable(hb_face_t *face, hb_tag_t aTag, void *aUserData)
{
    gfxHarfBuzzShaper *shaper = static_cast<gfxHarfBuzzShaper*>(aUserData);
    gfxFont *font = shaper->GetFont();

    
    
    if (aTag == TRUETYPE_TAG('G','D','E','F') &&
        font->GetFontEntry()->IgnoreGDEF()) {
        return nullptr;
    }

    
    
    if (aTag == TRUETYPE_TAG('G','S','U','B') &&
        font->GetFontEntry()->IgnoreGSUB()) {
        return nullptr;
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
    hb_codepoint_t gid;

    if (mUseFontGetGlyph) {
        gid = mFont->GetGlyph(unicode, variation_selector);
    } else {
        
        NS_ASSERTION(mFont->GetFontEntry()->HasCmapTable(),
                     "we cannot be using this font!");

        NS_ASSERTION(mCmapTable && (mCmapFormat > 0) && (mSubtableOffset > 0),
                     "cmap data not correctly set up, expect disaster");

        const uint8_t* data =
            (const uint8_t*)hb_blob_get_data(mCmapTable, nullptr);

        switch (mCmapFormat) {
        case 4:
            gid = unicode < UNICODE_BMP_LIMIT ?
                gfxFontUtils::MapCharToGlyphFormat4(data + mSubtableOffset,
                                                    unicode) : 0;
            break;
        case 12:
            gid = gfxFontUtils::MapCharToGlyphFormat12(data + mSubtableOffset,
                                                       unicode);
            break;
        default:
            NS_WARNING("unsupported cmap format, glyphs will be missing");
            gid = 0;
            break;
        }

        if (gid && variation_selector && mUVSTableOffset) {
            hb_codepoint_t varGID =
                gfxFontUtils::MapUVSToGlyphFormat14(data + mUVSTableOffset,
                                                    unicode,
                                                    variation_selector);
            if (varGID) {
                gid = varGID;
            }
            
            
        }
    }

    if (!gid) {
        
        if (unicode == 0xA0) {
            gid = mFont->GetSpaceGlyph();
        }
    }

    return gid;
}

static hb_bool_t
HBGetGlyph(hb_font_t *font, void *font_data,
           hb_codepoint_t unicode, hb_codepoint_t variation_selector,
           hb_codepoint_t *glyph,
           void *user_data)
{
    const FontCallbackData *fcd =
        static_cast<const FontCallbackData*>(font_data);
    *glyph = fcd->mShaper->GetGlyph(unicode, variation_selector);
    return *glyph != 0;
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

hb_position_t
gfxHarfBuzzShaper::GetGlyphHAdvance(gfxContext *aContext,
                                    hb_codepoint_t glyph) const
{
    if (mUseFontGlyphWidths) {
        return mFont->GetGlyphWidth(aContext, glyph);
    }

    
    

    NS_ASSERTION((mNumLongMetrics > 0) && mHmtxTable != nullptr,
                 "font is lacking metrics, we shouldn't be here");

    if (glyph >= uint32_t(mNumLongMetrics)) {
        glyph = mNumLongMetrics - 1;
    }

    
    
    
    const HMetrics* hmtx =
        reinterpret_cast<const HMetrics*>(hb_blob_get_data(mHmtxTable, nullptr));
    return FloatToFixed(mFont->FUnitsToDevUnitsFactor() *
                        uint16_t(hmtx->metrics[glyph].advanceWidth));
}

static hb_position_t
HBGetGlyphHAdvance(hb_font_t *font, void *font_data,
                   hb_codepoint_t glyph, void *user_data)
{
    const FontCallbackData *fcd =
        static_cast<const FontCallbackData*>(font_data);
    return fcd->mShaper->GetGlyphHAdvance(fcd->mContext, glyph);
}

static hb_bool_t
HBGetContourPoint(hb_font_t *font, void *font_data,
                  unsigned int point_index, hb_codepoint_t glyph,
                  hb_position_t *x, hb_position_t *y,
                  void *user_data)
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
                 uint32_t aSubtableLen,
                 uint16_t aFirstGlyph,
                 uint16_t aSecondGlyph,
                 int32_t& aValue,
                 bool     aIsOverride = false,
                 bool     aIsMinimum = false)
{
    const KernHeaderFmt0* hdr =
        reinterpret_cast<const KernHeaderFmt0*>(aSubtable);

    const KernPair *lo = reinterpret_cast<const KernPair*>(hdr + 1);
    const KernPair *hi = lo + uint16_t(hdr->nPairs);
    const KernPair *limit = hi;

    if (reinterpret_cast<const char*>(aSubtable) + aSubtableLen <
        reinterpret_cast<const char*>(hi)) {
        
        
        return;
    }

#define KERN_PAIR_KEY(l,r) (uint32_t((uint16_t(l) << 16) + uint16_t(r)))

    uint32_t key = KERN_PAIR_KEY(aFirstGlyph, aSecondGlyph);
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
            aValue = int16_t(lo->value);
        } else if (aIsMinimum) {
            aValue = NS_MAX(aValue, int32_t(lo->value));
        } else {
            aValue += int16_t(lo->value);
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

static int16_t
GetKernValueVersion1Fmt2(const void* aSubtable,
                         uint32_t aSubtableLen,
                         uint16_t aFirstGlyph,
                         uint16_t aSecondGlyph)
{
    if (aSubtableLen < sizeof(KernHeaderVersion1Fmt2)) {
        return 0;
    }

    const char* base = reinterpret_cast<const char*>(aSubtable);
    const char* subtableEnd = base + aSubtableLen;

    const KernHeaderVersion1Fmt2* h =
        reinterpret_cast<const KernHeaderVersion1Fmt2*>(aSubtable);
    uint32_t offset = h->array;

    const KernClassTableHdr* leftClassTable =
        reinterpret_cast<const KernClassTableHdr*>(base +
                                                   uint16_t(h->leftOffsetTable));
    if (reinterpret_cast<const char*>(leftClassTable) +
        sizeof(KernClassTableHdr) > subtableEnd) {
        return 0;
    }
    if (aFirstGlyph >= uint16_t(leftClassTable->firstGlyph)) {
        aFirstGlyph -= uint16_t(leftClassTable->firstGlyph);
        if (aFirstGlyph < uint16_t(leftClassTable->nGlyphs)) {
            if (reinterpret_cast<const char*>(leftClassTable) +
                sizeof(KernClassTableHdr) +
                aFirstGlyph * sizeof(uint16_t) >= subtableEnd) {
                return 0;
            }
            offset = uint16_t(leftClassTable->offsets[aFirstGlyph]);
        }
    }

    const KernClassTableHdr* rightClassTable =
        reinterpret_cast<const KernClassTableHdr*>(base +
                                                   uint16_t(h->rightOffsetTable));
    if (reinterpret_cast<const char*>(rightClassTable) +
        sizeof(KernClassTableHdr) > subtableEnd) {
        return 0;
    }
    if (aSecondGlyph >= uint16_t(rightClassTable->firstGlyph)) {
        aSecondGlyph -= uint16_t(rightClassTable->firstGlyph);
        if (aSecondGlyph < uint16_t(rightClassTable->nGlyphs)) {
            if (reinterpret_cast<const char*>(rightClassTable) +
                sizeof(KernClassTableHdr) +
                aSecondGlyph * sizeof(uint16_t) >= subtableEnd) {
                return 0;
            }
            offset += uint16_t(rightClassTable->offsets[aSecondGlyph]);
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
    uint8_t kernValueCount;
    uint8_t leftClassCount;
    uint8_t rightClassCount;
    uint8_t flags;
};

static int16_t
GetKernValueVersion1Fmt3(const void* aSubtable,
                         uint32_t aSubtableLen,
                         uint16_t aFirstGlyph,
                         uint16_t aSecondGlyph)
{
    
    if (aSubtableLen < sizeof(KernHeaderVersion1Fmt3)) {
        return 0;
    }

    const KernHeaderVersion1Fmt3* hdr =
        reinterpret_cast<const KernHeaderVersion1Fmt3*>(aSubtable);
    if (hdr->flags != 0) {
        return 0;
    }

    uint16_t glyphCount = hdr->glyphCount;

    
    if (sizeof(KernHeaderVersion1Fmt3) +
        hdr->kernValueCount * sizeof(int16_t) +
        glyphCount + glyphCount +
        hdr->leftClassCount * hdr->rightClassCount > aSubtableLen) {
        return 0;
    }
        
    if (aFirstGlyph >= glyphCount || aSecondGlyph >= glyphCount) {
        
        return 0;
    }

    
    const AutoSwap_PRInt16* kernValue =
        reinterpret_cast<const AutoSwap_PRInt16*>(hdr + 1);
    const uint8_t* leftClass =
        reinterpret_cast<const uint8_t*>(kernValue + hdr->kernValueCount);
    const uint8_t* rightClass = leftClass + glyphCount;
    const uint8_t* kernIndex = rightClass + glyphCount;

    uint8_t lc = leftClass[aFirstGlyph];
    uint8_t rc = rightClass[aSecondGlyph];
    if (lc >= hdr->leftClassCount || rc >= hdr->rightClassCount) {
        return 0;
    }

    uint8_t ki = kernIndex[leftClass[aFirstGlyph] * hdr->rightClassCount +
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
gfxHarfBuzzShaper::GetHKerning(uint16_t aFirstGlyph,
                               uint16_t aSecondGlyph) const
{
    
    
    
    uint32_t spaceGlyph = mFont->GetSpaceGlyph();
    if (aFirstGlyph == spaceGlyph || aSecondGlyph == spaceGlyph) {
        return 0;
    }

    if (!mKernTable) {
        mKernTable = mFont->GetFontTable(TRUETYPE_TAG('k','e','r','n'));
        if (!mKernTable) {
            mKernTable = hb_blob_get_empty();
        }
    }

    uint32_t len;
    const char* base = hb_blob_get_data(mKernTable, &len);
    if (len < sizeof(KernTableVersion0)) {
        return 0;
    }
    int32_t value = 0;

    
    
    const KernTableVersion0* kern0 =
        reinterpret_cast<const KernTableVersion0*>(base);
    if (uint16_t(kern0->version) == 0) {
        uint16_t nTables = kern0->nTables;
        uint32_t offs = sizeof(KernTableVersion0);
        for (uint16_t i = 0; i < nTables; ++i) {
            if (offs + sizeof(KernTableSubtableHeaderVersion0) > len) {
                break;
            }
            const KernTableSubtableHeaderVersion0* st0 =
                reinterpret_cast<const KernTableSubtableHeaderVersion0*>
                                (base + offs);
            uint16_t subtableLen = uint16_t(st0->length);
            if (offs + subtableLen > len) {
                break;
            }
            offs += subtableLen;
            uint16_t coverage = st0->coverage;
            if (!(coverage & KERN0_COVERAGE_HORIZONTAL)) {
                
                continue;
            }
            if (coverage &
                (KERN0_COVERAGE_CROSS_STREAM | KERN0_COVERAGE_RESERVED)) {
                
                
                
                continue;
            }
            uint8_t format = (coverage >> 8);
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
        if (uint32_t(kern1->version) == 0x00010000) {
            uint32_t nTables = kern1->nTables;
            uint32_t offs = sizeof(KernTableVersion1);
            for (uint32_t i = 0; i < nTables; ++i) {
                if (offs + sizeof(KernTableSubtableHeaderVersion1) > len) {
                    break;
                }
                const KernTableSubtableHeaderVersion1* st1 =
                    reinterpret_cast<const KernTableSubtableHeaderVersion1*>
                                    (base + offs);
                uint32_t subtableLen = uint32_t(st1->length);
                offs += subtableLen;
                uint16_t coverage = st1->coverage;
                if (coverage &
                    (KERN1_COVERAGE_VERTICAL     |
                     KERN1_COVERAGE_CROSS_STREAM |
                     KERN1_COVERAGE_VARIATION    |
                     KERN1_COVERAGE_RESERVED)) {
                    
                    
                    
                    
                    
                    continue;
                }
                uint8_t format = (coverage & 0xff);
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

    if (value != 0) {
        return FloatToFixed(mFont->FUnitsToDevUnitsFactor() * value);
    }
    return 0;
}

static hb_position_t
HBGetHKerning(hb_font_t *font, void *font_data,
              hb_codepoint_t first_glyph, hb_codepoint_t second_glyph,
              void *user_data)
{
    const FontCallbackData *fcd =
        static_cast<const FontCallbackData*>(font_data);
    return fcd->mShaper->GetHKerning(first_glyph, second_glyph);
}





static hb_codepoint_t
HBGetMirroring(hb_unicode_funcs_t *ufuncs, hb_codepoint_t aCh,
               void *user_data)
{
    return GetMirroredChar(aCh);
}

static hb_unicode_general_category_t
HBGetGeneralCategory(hb_unicode_funcs_t *ufuncs, hb_codepoint_t aCh,
                     void *user_data)
{
    return hb_unicode_general_category_t(GetGeneralCategory(aCh));
}

static hb_script_t
HBGetScript(hb_unicode_funcs_t *ufuncs, hb_codepoint_t aCh, void *user_data)
{
    return hb_script_t(GetScriptTagForCode(GetScriptCode(aCh)));
}

static hb_unicode_combining_class_t
HBGetCombiningClass(hb_unicode_funcs_t *ufuncs, hb_codepoint_t aCh,
                    void *user_data)
{
    return hb_unicode_combining_class_t(GetCombiningClass(aCh));
}

static unsigned int
HBGetEastAsianWidth(hb_unicode_funcs_t *ufuncs, hb_codepoint_t aCh,
                    void *user_data)
{
    return GetEastAsianWidth(aCh);
}



static const PRUnichar sDageshForms[0x05EA - 0x05D0 + 1] = {
    0xFB30, 
    0xFB31, 
    0xFB32, 
    0xFB33, 
    0xFB34, 
    0xFB35, 
    0xFB36, 
    0, 
    0xFB38, 
    0xFB39, 
    0xFB3A, 
    0xFB3B, 
    0xFB3C, 
    0, 
    0xFB3E, 
    0, 
    0xFB40, 
    0xFB41, 
    0, 
    0xFB43, 
    0xFB44, 
    0, 
    0xFB46, 
    0xFB47, 
    0xFB48, 
    0xFB49, 
    0xFB4A 
};

static hb_bool_t
HBUnicodeCompose(hb_unicode_funcs_t *ufuncs,
                 hb_codepoint_t      a,
                 hb_codepoint_t      b,
                 hb_codepoint_t     *ab,
                 void               *user_data)
{
    hb_bool_t found = nsUnicodeNormalizer::Compose(a, b, ab);

    if (!found && (b & 0x1fff80) == 0x0580) {
        
        
        switch (b) {
        case 0x05B4: 
            if (a == 0x05D9) { 
                *ab = 0xFB1D;
                found = true;
            }
            break;
        case 0x05B7: 
            if (a == 0x05F2) { 
                *ab = 0xFB1F;
                found = true;
            } else if (a == 0x05D0) { 
                *ab = 0xFB2E;
                found = true;
            }
            break;
        case 0x05B8: 
            if (a == 0x05D0) { 
                *ab = 0xFB2F;
                found = true;
            }
            break;
        case 0x05B9: 
            if (a == 0x05D5) { 
                *ab = 0xFB4B;
                found = true;
            }
            break;
        case 0x05BC: 
            if (a >= 0x05D0 && a <= 0x05EA) {
                *ab = sDageshForms[a - 0x05D0];
                found = (*ab != 0);
            } else if (a == 0xFB2A) { 
                *ab = 0xFB2C;
                found = true;
            } else if (a == 0xFB2B) { 
                *ab = 0xFB2D;
                found = true;
            }
            break;
        case 0x05BF: 
            switch (a) {
            case 0x05D1: 
                *ab = 0xFB4C;
                found = true;
                break;
            case 0x05DB: 
                *ab = 0xFB4D;
                found = true;
                break;
            case 0x05E4: 
                *ab = 0xFB4E;
                found = true;
                break;
            }
            break;
        case 0x05C1: 
            if (a == 0x05E9) { 
                *ab = 0xFB2A;
                found = true;
            } else if (a == 0xFB49) { 
                *ab = 0xFB2C;
                found = true;
            }
            break;
        case 0x05C2: 
            if (a == 0x05E9) { 
                *ab = 0xFB2B;
                found = true;
            } else if (a == 0xFB49) { 
                *ab = 0xFB2D;
                found = true;
            }
            break;
        }
    }

    return found;
}

static hb_bool_t
HBUnicodeDecompose(hb_unicode_funcs_t *ufuncs,
                   hb_codepoint_t      ab,
                   hb_codepoint_t     *a,
                   hb_codepoint_t     *b,
                   void               *user_data)
{
    return nsUnicodeNormalizer::DecomposeNonRecursively(ab, a, b);
}

static PLDHashOperator
AddFeature(const uint32_t& aTag, uint32_t& aValue, void *aUserArg)
{
    nsTArray<hb_feature_t>* features = static_cast<nsTArray<hb_feature_t>*> (aUserArg);

    hb_feature_t feat = { 0, 0, 0, UINT_MAX };
    feat.tag = aTag;
    feat.value = aValue;
    features->AppendElement(feat);
    return PL_DHASH_NEXT;
}





static hb_font_funcs_t * sHBFontFuncs = nullptr;
static hb_unicode_funcs_t * sHBUnicodeFuncs = nullptr;

bool
gfxHarfBuzzShaper::ShapeText(gfxContext      *aContext,
                             const PRUnichar *aText,
                             uint32_t         aOffset,
                             uint32_t         aLength,
                             int32_t          aScript,
                             gfxShapedText   *aShapedText)
{
    
    if (!mFont->SetupCairoFont(aContext)) {
        return false;
    }

    if (!mHBFace) {

        mUseFontGlyphWidths = mFont->ProvidesGlyphWidths();

        

        if (!sHBFontFuncs) {
            
            
            sHBFontFuncs = hb_font_funcs_create();
            hb_font_funcs_set_glyph_func(sHBFontFuncs, HBGetGlyph,
                                         nullptr, nullptr);
            hb_font_funcs_set_glyph_h_advance_func(sHBFontFuncs,
                                                   HBGetGlyphHAdvance,
                                                   nullptr, nullptr);
            hb_font_funcs_set_glyph_contour_point_func(sHBFontFuncs,
                                                       HBGetContourPoint,
                                                       nullptr, nullptr);
            hb_font_funcs_set_glyph_h_kerning_func(sHBFontFuncs,
                                                   HBGetHKerning,
                                                   nullptr, nullptr);

            sHBUnicodeFuncs =
                hb_unicode_funcs_create(hb_unicode_funcs_get_empty());
            hb_unicode_funcs_set_mirroring_func(sHBUnicodeFuncs,
                                                HBGetMirroring,
                                                nullptr, nullptr);
            hb_unicode_funcs_set_script_func(sHBUnicodeFuncs, HBGetScript,
                                             nullptr, nullptr);
            hb_unicode_funcs_set_general_category_func(sHBUnicodeFuncs,
                                                       HBGetGeneralCategory,
                                                       nullptr, nullptr);
            hb_unicode_funcs_set_combining_class_func(sHBUnicodeFuncs,
                                                      HBGetCombiningClass,
                                                      nullptr, nullptr);
            hb_unicode_funcs_set_eastasian_width_func(sHBUnicodeFuncs,
                                                      HBGetEastAsianWidth,
                                                      nullptr, nullptr);
            hb_unicode_funcs_set_compose_func(sHBUnicodeFuncs,
                                              HBUnicodeCompose,
                                              nullptr, nullptr);
            hb_unicode_funcs_set_decompose_func(sHBUnicodeFuncs,
                                                HBUnicodeDecompose,
                                                nullptr, nullptr);
        }

        mHBFace = hb_face_create_for_tables(HBGetTable, this, nullptr);

        if (!mUseFontGetGlyph) {
            
            mCmapTable = mFont->GetFontTable(TRUETYPE_TAG('c','m','a','p'));
            if (!mCmapTable) {
                NS_WARNING("failed to load cmap, glyphs will be missing");
                return false;
            }
            uint32_t len;
            const uint8_t* data = (const uint8_t*)hb_blob_get_data(mCmapTable, &len);
            bool symbol;
            mCmapFormat = gfxFontUtils::
                FindPreferredSubtable(data, len,
                                      &mSubtableOffset, &mUVSTableOffset,
                                      &symbol);
        }

        if (!mUseFontGlyphWidths) {
            
            
            
            
            hb_blob_t *hheaTable =
                mFont->GetFontTable(TRUETYPE_TAG('h','h','e','a'));
            if (hheaTable) {
                uint32_t len;
                const HMetricsHeader* hhea =
                    reinterpret_cast<const HMetricsHeader*>
                        (hb_blob_get_data(hheaTable, &len));
                if (len >= sizeof(HMetricsHeader)) {
                    mNumLongMetrics = hhea->numberOfHMetrics;
                    if (mNumLongMetrics > 0 &&
                        int16_t(hhea->metricDataFormat) == 0) {
                        
                        
                        
                        mHmtxTable =
                            mFont->GetFontTable(TRUETYPE_TAG('h','m','t','x'));
                        if (hb_blob_get_length(mHmtxTable) <
                            mNumLongMetrics * sizeof(HLongMetric)) {
                            
                            
                            hb_blob_destroy(mHmtxTable);
                            mHmtxTable = nullptr;
                        }
                    }
                }
            }
            hb_blob_destroy(hheaTable);
        }
    }

    if ((!mUseFontGetGlyph && mCmapFormat <= 0) ||
        (!mUseFontGlyphWidths && !mHmtxTable)) {
        
        return false;
    }

    FontCallbackData fcd(this, aContext);
    hb_font_t *font = hb_font_create(mHBFace);
    hb_font_set_funcs(font, sHBFontFuncs, &fcd, nullptr);
    hb_font_set_ppem(font, mFont->GetAdjustedSize(), mFont->GetAdjustedSize());
    uint32_t scale = FloatToFixed(mFont->GetAdjustedSize()); 
    hb_font_set_scale(font, scale, scale);

    nsAutoTArray<hb_feature_t,20> features;

    gfxFontEntry *entry = mFont->GetFontEntry();
    const gfxFontStyle *style = mFont->GetStyle();

    nsDataHashtable<nsUint32HashKey,uint32_t> mergedFeatures;

    if (MergeFontFeatures(style->featureSettings,
                      mFont->GetFontEntry()->mFeatureSettings,
                      aShapedText->DisableLigatures(), mergedFeatures)) {
        
        mergedFeatures.Enumerate(AddFeature, &features);
    }

    bool isRightToLeft = aShapedText->IsRightToLeft();
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buffer, sHBUnicodeFuncs);
    hb_buffer_set_direction(buffer, isRightToLeft ? HB_DIRECTION_RTL :
                                                    HB_DIRECTION_LTR);
    
    
    
    hb_script_t scriptTag = (aScript <= MOZ_SCRIPT_INHERITED) ?
        HB_SCRIPT_LATIN :
        hb_script_t(GetScriptTagForCode(aScript));
    hb_buffer_set_script(buffer, scriptTag);

    hb_language_t language;
    if (style->languageOverride) {
        language = hb_ot_tag_to_language(style->languageOverride);
    } else if (entry->mLanguageOverride) {
        language = hb_ot_tag_to_language(entry->mLanguageOverride);
    } else {
        nsCString langString;
        style->language->ToUTF8String(langString);
        language =
            hb_language_from_string(langString.get(), langString.Length());
    }
    hb_buffer_set_language(buffer, language);

    uint32_t length = aLength;
    hb_buffer_add_utf16(buffer,
                        reinterpret_cast<const uint16_t*>(aText),
                        length, 0, length);

    hb_shape(font, buffer, features.Elements(), features.Length());

    if (isRightToLeft) {
        hb_buffer_reverse(buffer);
    }

    nsresult rv = SetGlyphsFromRun(aContext, aShapedText, aOffset, aLength,
                                   aText, buffer);

    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to store glyphs into gfxShapedWord");
    hb_buffer_destroy(buffer);
    hb_font_destroy(font);

    return NS_SUCCEEDED(rv);
}










static void
GetRoundOffsetsToPixels(gfxContext *aContext,
                        bool *aRoundX, bool *aRoundY)
{
    *aRoundX = false;
    
    
    
    if (aContext->CurrentMatrix().HasNonTranslation()) {
        *aRoundY = false;
        return;
    }

    
    
    *aRoundY = true;

    cairo_t *cr = aContext->GetCairo();
    cairo_scaled_font_t *scaled_font = cairo_get_scaled_font(cr);
    
    cairo_font_options_t *font_options = cairo_font_options_create();
    cairo_scaled_font_get_font_options(scaled_font, font_options);
    cairo_hint_metrics_t hint_metrics =
        cairo_font_options_get_hint_metrics(font_options);
    cairo_font_options_destroy(font_options);

    switch (hint_metrics) {
    case CAIRO_HINT_METRICS_OFF:
        *aRoundY = false;
        return;
    case CAIRO_HINT_METRICS_DEFAULT:
        
        
        
        
        
        switch (cairo_scaled_font_get_type(scaled_font)) {
#if CAIRO_HAS_DWRITE_FONT 
        case CAIRO_FONT_TYPE_DWRITE:
            
            
            
            if (!cairo_dwrite_scaled_font_get_force_GDI_classic(scaled_font) &&
                gfxWindowsPlatform::GetPlatform()->DWriteMeasuringMode() ==
                    DWRITE_MEASURING_MODE_NATURAL) {
                return;
            }
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
    *aRoundX = true;
    return;
}

#define SMALL_GLYPH_RUN 128 // some testing indicates that 90%+ of text runs
                            
                            

nsresult
gfxHarfBuzzShaper::SetGlyphsFromRun(gfxContext      *aContext,
                                    gfxShapedText   *aShapedText,
                                    uint32_t         aOffset,
                                    uint32_t         aLength,
                                    const PRUnichar *aText,
                                    hb_buffer_t     *aBuffer)
{
    uint32_t numGlyphs;
    const hb_glyph_info_t *ginfo = hb_buffer_get_glyph_infos(aBuffer, &numGlyphs);
    if (numGlyphs == 0) {
        return NS_OK;
    }

    nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;

    uint32_t wordLength = aLength;
    static const int32_t NO_GLYPH = -1;
    nsAutoTArray<int32_t,SMALL_GLYPH_RUN> charToGlyphArray;
    if (!charToGlyphArray.SetLength(wordLength)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }

    int32_t *charToGlyph = charToGlyphArray.Elements();
    for (uint32_t offset = 0; offset < wordLength; ++offset) {
        charToGlyph[offset] = NO_GLYPH;
    }

    for (uint32_t i = 0; i < numGlyphs; ++i) {
        uint32_t loc = ginfo[i].cluster;
        if (loc < wordLength) {
            charToGlyph[loc] = i;
        }
    }

    int32_t glyphStart = 0; 
    int32_t charStart = 0; 

    bool roundX;
    bool roundY;
    GetRoundOffsetsToPixels(aContext, &roundX, &roundY);

    int32_t appUnitsPerDevUnit = aShapedText->GetAppUnitsPerDevUnit();
    gfxShapedText::CompressedGlyph *charGlyphs =
        aShapedText->GetCharacterGlyphs() + aOffset;

    
    
    double hb2appUnits = FixedToFloat(aShapedText->GetAppUnitsPerDevUnit());

    
    
    
    
    
    
    
    
    
    
    hb_position_t x_residual = 0;

    
    nscoord yPos = 0;

    const hb_glyph_position_t *posInfo =
        hb_buffer_get_glyph_positions(aBuffer, nullptr);

    while (glyphStart < int32_t(numGlyphs)) {

        int32_t charEnd = ginfo[glyphStart].cluster;
        int32_t glyphEnd = glyphStart;
        int32_t charLimit = wordLength;
        while (charEnd < charLimit) {
            
            
            
            
            
            charEnd += 1;
            while (charEnd != charLimit && charToGlyph[charEnd] == NO_GLYPH) {
                charEnd += 1;
            }

            
            for (int32_t i = charStart; i < charEnd; ++i) {
                if (charToGlyph[i] != NO_GLYPH) {
                    glyphEnd = NS_MAX(glyphEnd, charToGlyph[i] + 1);
                    
                }
            }

            if (glyphEnd == glyphStart + 1) {
                
                
                break;
            }

            if (glyphEnd == glyphStart) {
                
                continue;
            }

            
            
            
            bool allGlyphsAreWithinCluster = true;
            for (int32_t i = glyphStart; i < glyphEnd; ++i) {
                int32_t glyphCharIndex = ginfo[i].cluster;
                if (glyphCharIndex < charStart || glyphCharIndex >= charEnd) {
                    allGlyphsAreWithinCluster = false;
                    break;
                }
            }
            if (allGlyphsAreWithinCluster) {
                break;
            }
        }

        NS_ASSERTION(glyphStart < glyphEnd,
                     "character/glyph clump contains no glyphs!");
        NS_ASSERTION(charStart != charEnd,
                     "character/glyph clump contains no characters!");

        
        
        
        
        int32_t baseCharIndex, endCharIndex;
        while (charEnd < int32_t(wordLength) && charToGlyph[charEnd] == NO_GLYPH)
            charEnd++;
        baseCharIndex = charStart;
        endCharIndex = charEnd;

        
        
        if (baseCharIndex >= int32_t(wordLength)) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }
        
        endCharIndex = NS_MIN<int32_t>(endCharIndex, wordLength);

        
        int32_t glyphsInClump = glyphEnd - glyphStart;

        
        
        
        if (glyphsInClump == 1 && baseCharIndex + 1 == endCharIndex &&
            aShapedText->FilterIfIgnorable(aOffset + baseCharIndex,
                                           aText[baseCharIndex])) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        hb_position_t x_offset = posInfo[glyphStart].x_offset;
        hb_position_t x_advance = posInfo[glyphStart].x_advance;
        nscoord xOffset, advance;
        if (roundX) {
            xOffset =
                appUnitsPerDevUnit * FixedToIntRound(x_offset + x_residual);
            
            hb_position_t width = x_advance - x_offset;
            int intWidth = FixedToIntRound(width);
            x_residual = width - FloatToFixed(intWidth);
            advance = appUnitsPerDevUnit * intWidth + xOffset;
        } else {
            xOffset = floor(hb2appUnits * x_offset + 0.5);
            advance = floor(hb2appUnits * x_advance + 0.5);
        }
        
        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(ginfo[glyphStart].codepoint) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            charGlyphs[baseCharIndex].IsClusterStart() &&
            xOffset == 0 &&
            posInfo[glyphStart].y_offset == 0 && yPos == 0)
        {
            charGlyphs[baseCharIndex].SetSimpleGlyph(advance,
                                                     ginfo[glyphStart].codepoint);
        } else {
            
            
            
            
            while (1) {
                gfxTextRun::DetailedGlyph* details =
                    detailedGlyphs.AppendElement();
                details->mGlyphID = ginfo[glyphStart].codepoint;

                details->mXOffset = xOffset;
                details->mAdvance = advance;

                hb_position_t y_offset = posInfo[glyphStart].y_offset;
                details->mYOffset = yPos -
                    (roundY ? appUnitsPerDevUnit * FixedToIntRound(y_offset)
                     : floor(hb2appUnits * y_offset + 0.5));

                hb_position_t y_advance = posInfo[glyphStart].y_advance;
                if (y_advance != 0) {
                    yPos -=
                        roundY ? appUnitsPerDevUnit * FixedToIntRound(y_advance)
                        : floor(hb2appUnits * y_advance + 0.5);
                }
                if (++glyphStart >= glyphEnd) {
                    break;
                }

                x_offset = posInfo[glyphStart].x_offset;
                x_advance = posInfo[glyphStart].x_advance;
                if (roundX) {
                    xOffset = appUnitsPerDevUnit *
                        FixedToIntRound(x_offset + x_residual);
                    
                    
                    
                    
                    
                    x_advance += x_residual;
                    int intAdvance = FixedToIntRound(x_advance);
                    x_residual = x_advance - FloatToFixed(intAdvance);
                    advance = appUnitsPerDevUnit * intAdvance;
                } else {
                    xOffset = floor(hb2appUnits * x_offset + 0.5);
                    advance = floor(hb2appUnits * x_advance + 0.5);
                }
            }

            gfxShapedText::CompressedGlyph g;
            g.SetComplex(charGlyphs[baseCharIndex].IsClusterStart(),
                         true, detailedGlyphs.Length());
            aShapedText->SetGlyphs(aOffset + baseCharIndex,
                                   g, detailedGlyphs.Elements());

            detailedGlyphs.Clear();
        }

        
        
        while (++baseCharIndex != endCharIndex &&
               baseCharIndex < int32_t(wordLength)) {
            gfxShapedText::CompressedGlyph &g = charGlyphs[baseCharIndex];
            NS_ASSERTION(!g.IsSimpleGlyph(), "overwriting a simple glyph");
            g.SetComplex(g.IsClusterStart(), false, 0);
        }

        glyphStart = glyphEnd;
        charStart = charEnd;
    }

    return NS_OK;
}
