




#include "nsString.h"
#include "gfxContext.h"
#include "gfxFontConstants.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxFontUtils.h"
#include "gfxTextRun.h"
#include "nsUnicodeProperties.h"
#include "nsUnicodeScriptCodes.h"
#include "nsUnicodeNormalizer.h"

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ot.h"

#include <algorithm>

#define FloatToFixed(f) (65536 * (f))
#define FixedToFloat(f) ((f) * (1.0 / 65536.0))



#define FixedToIntRound(f) ((f) > 0 ?  ((32768 + (f)) >> 16) \
                                    : -((32767 - (f)) >> 16))

using namespace mozilla; 
using namespace mozilla::unicode; 





gfxHarfBuzzShaper::gfxHarfBuzzShaper(gfxFont *aFont)
    : gfxFontShaper(aFont),
      mHBFace(aFont->GetFontEntry()->GetHBFace()),
      mHBFont(nullptr),
      mKernTable(nullptr),
      mHmtxTable(nullptr),
      mVmtxTable(nullptr),
      mVORGTable(nullptr),
      mLocaTable(nullptr),
      mGlyfTable(nullptr),
      mCmapTable(nullptr),
      mCmapFormat(-1),
      mSubtableOffset(0),
      mUVSTableOffset(0),
      mNumLongHMetrics(0),
      mNumLongVMetrics(0),
      mUseFontGetGlyph(aFont->ProvidesGetGlyph()),
      mUseFontGlyphWidths(false),
      mInitialized(false),
      mVerticalInitialized(false),
      mLoadedLocaGlyf(false),
      mLocaLongOffsets(false)
{
}

gfxHarfBuzzShaper::~gfxHarfBuzzShaper()
{
    if (mCmapTable) {
        hb_blob_destroy(mCmapTable);
    }
    if (mHmtxTable) {
        hb_blob_destroy(mHmtxTable);
    }
    if (mKernTable) {
        hb_blob_destroy(mKernTable);
    }
    if (mVmtxTable) {
        hb_blob_destroy(mVmtxTable);
    }
    if (mVORGTable) {
        hb_blob_destroy(mVORGTable);
    }
    if (mLocaTable) {
        hb_blob_destroy(mLocaTable);
    }
    if (mGlyfTable) {
        hb_blob_destroy(mGlyfTable);
    }
    if (mHBFont) {
        hb_font_destroy(mHBFont);
    }
    if (mHBFace) {
        hb_face_destroy(mHBFace);
    }
}

#define UNICODE_BMP_LIMIT 0x10000

hb_codepoint_t
gfxHarfBuzzShaper::GetGlyph(hb_codepoint_t unicode,
                            hb_codepoint_t variation_selector) const
{
    hb_codepoint_t gid = 0;

    if (mUseFontGetGlyph) {
        gid = mFont->GetGlyph(unicode, variation_selector);
    } else {
        
        NS_ASSERTION(mFont->GetFontEntry()->HasCmapTable(),
                     "we cannot be using this font!");

        NS_ASSERTION(mCmapTable && (mCmapFormat > 0) && (mSubtableOffset > 0),
                     "cmap data not correctly set up, expect disaster");

        const uint8_t* data =
            (const uint8_t*)hb_blob_get_data(mCmapTable, nullptr);

        if (variation_selector) {
            if (mUVSTableOffset) {
                gid =
                    gfxFontUtils::MapUVSToGlyphFormat14(data + mUVSTableOffset,
                                                        unicode,
                                                        variation_selector);
            }
            if (!gid) {
                uint32_t compat =
                    gfxFontUtils::GetUVSFallback(unicode, variation_selector);
                if (compat) {
                    switch (mCmapFormat) {
                    case 4:
                        if (compat < UNICODE_BMP_LIMIT) {
                            gid = gfxFontUtils::MapCharToGlyphFormat4(data + mSubtableOffset,
                                                                      compat);
                        }
                        break;
                    case 10:
                        gid = gfxFontUtils::MapCharToGlyphFormat10(data + mSubtableOffset,
                                                                   compat);
                        break;
                    case 12:
                        gid = gfxFontUtils::MapCharToGlyphFormat12(data + mSubtableOffset,
                                                                   compat);
                        break;
                    }
                }
            }
            
            
            return gid;
        }

        switch (mCmapFormat) {
        case 4:
            gid = unicode < UNICODE_BMP_LIMIT ?
                gfxFontUtils::MapCharToGlyphFormat4(data + mSubtableOffset,
                                                    unicode) : 0;
            break;
        case 10:
            gid = gfxFontUtils::MapCharToGlyphFormat10(data + mSubtableOffset,
                                                       unicode);
            break;
        case 12:
            gid = gfxFontUtils::MapCharToGlyphFormat12(data + mSubtableOffset,
                                                       unicode);
            break;
        default:
            NS_WARNING("unsupported cmap format, glyphs will be missing");
            break;
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
    const gfxHarfBuzzShaper::FontCallbackData *fcd =
        static_cast<const gfxHarfBuzzShaper::FontCallbackData*>(font_data);
    *glyph = fcd->mShaper->GetGlyph(unicode, variation_selector);
    return *glyph != 0;
}



struct LongMetric {
    AutoSwap_PRUint16    advanceWidth; 
    AutoSwap_PRInt16     lsb;          
};

struct GlyphMetrics {
    LongMetric           metrics[1]; 


};

hb_position_t
gfxHarfBuzzShaper::GetGlyphHAdvance(hb_codepoint_t glyph) const
{
    
    

    NS_ASSERTION((mNumLongHMetrics > 0) && mHmtxTable != nullptr,
                 "font is lacking metrics, we shouldn't be here");

    if (glyph >= uint32_t(mNumLongHMetrics)) {
        glyph = mNumLongHMetrics - 1;
    }

    
    
    
    const GlyphMetrics* metrics =
        reinterpret_cast<const GlyphMetrics*>(hb_blob_get_data(mHmtxTable,
                                                               nullptr));
    return FloatToFixed(mFont->FUnitsToDevUnitsFactor() *
                        uint16_t(metrics->metrics[glyph].advanceWidth));
}

hb_position_t
gfxHarfBuzzShaper::GetGlyphVAdvance(hb_codepoint_t glyph) const
{
    if (!mVmtxTable) {
        
        
        return FloatToFixed(mFont->GetMetrics(gfxFont::eVertical).aveCharWidth);
    }

    NS_ASSERTION(mNumLongVMetrics > 0,
                 "font is lacking metrics, we shouldn't be here");

    if (glyph >= uint32_t(mNumLongVMetrics)) {
        glyph = mNumLongVMetrics - 1;
    }

    
    
    
    const GlyphMetrics* metrics =
        reinterpret_cast<const GlyphMetrics*>(hb_blob_get_data(mVmtxTable,
                                                               nullptr));
    return FloatToFixed(mFont->FUnitsToDevUnitsFactor() *
                        uint16_t(metrics->metrics[glyph].advanceWidth));
}


hb_position_t
gfxHarfBuzzShaper::HBGetGlyphHAdvance(hb_font_t *font, void *font_data,
                                      hb_codepoint_t glyph, void *user_data)
{
    const gfxHarfBuzzShaper::FontCallbackData *fcd =
        static_cast<const gfxHarfBuzzShaper::FontCallbackData*>(font_data);
    gfxFont *gfxfont = fcd->mShaper->GetFont();
    if (gfxfont->ProvidesGlyphWidths()) {
        return gfxfont->GetGlyphWidth(*fcd->mContext->GetDrawTarget(), glyph);
    }
    return fcd->mShaper->GetGlyphHAdvance(glyph);
}


hb_position_t
gfxHarfBuzzShaper::HBGetGlyphVAdvance(hb_font_t *font, void *font_data,
                                      hb_codepoint_t glyph, void *user_data)
{
    const gfxHarfBuzzShaper::FontCallbackData *fcd =
        static_cast<const gfxHarfBuzzShaper::FontCallbackData*>(font_data);
    
    
    
    
    return fcd->mShaper->GetGlyphVAdvance(glyph);
}


hb_bool_t
gfxHarfBuzzShaper::HBGetGlyphHOrigin(hb_font_t *font, void *font_data,
                                     hb_codepoint_t glyph,
                                     hb_position_t *x, hb_position_t *y,
                                     void *user_data)
{
    
    return true;
}

struct VORG {
    AutoSwap_PRUint16 majorVersion;
    AutoSwap_PRUint16 minorVersion;
    AutoSwap_PRInt16  defaultVertOriginY;
    AutoSwap_PRUint16 numVertOriginYMetrics;
};

struct VORGrec {
    AutoSwap_PRUint16 glyphIndex;
    AutoSwap_PRInt16  vertOriginY;
};


hb_bool_t
gfxHarfBuzzShaper::HBGetGlyphVOrigin(hb_font_t *font, void *font_data,
                                     hb_codepoint_t glyph,
                                     hb_position_t *x, hb_position_t *y,
                                     void *user_data)
{
    const gfxHarfBuzzShaper::FontCallbackData *fcd =
        static_cast<const gfxHarfBuzzShaper::FontCallbackData*>(font_data);
    fcd->mShaper->GetGlyphVOrigin(glyph, x, y);
    return true;
}

void
gfxHarfBuzzShaper::GetGlyphVOrigin(hb_codepoint_t aGlyph,
                                   hb_position_t *aX, hb_position_t *aY) const
{
    *aX = -0.5 * GetGlyphHAdvance(aGlyph);

    if (mVORGTable) {
        
        
        const VORG* vorg =
            reinterpret_cast<const VORG*>(hb_blob_get_data(mVORGTable, nullptr));

        const VORGrec *lo = reinterpret_cast<const VORGrec*>(vorg + 1);
        const VORGrec *hi = lo + uint16_t(vorg->numVertOriginYMetrics);
        const VORGrec *limit = hi;
        while (lo < hi) {
            const VORGrec *mid = lo + (hi - lo) / 2;
            if (uint16_t(mid->glyphIndex) < aGlyph) {
                lo = mid + 1;
            } else {
                hi = mid;
            }
        }

        if (lo < limit && uint16_t(lo->glyphIndex) == aGlyph) {
            *aY = -FloatToFixed(GetFont()->FUnitsToDevUnitsFactor() *
                                int16_t(lo->vertOriginY));
        } else {
            *aY = -FloatToFixed(GetFont()->FUnitsToDevUnitsFactor() *
                                int16_t(vorg->defaultVertOriginY));
        }
        return;
    }

    if (mVmtxTable) {
        bool emptyGlyf;
        const Glyf *glyf = FindGlyf(aGlyph, &emptyGlyf);
        if (glyf) {
            if (emptyGlyf) {
                *aY = 0;
                return;
            }

            if (aGlyph >= uint32_t(mNumLongVMetrics)) {
                aGlyph = mNumLongVMetrics - 1;
            }
            const GlyphMetrics* metrics =
                reinterpret_cast<const GlyphMetrics*>
                    (hb_blob_get_data(mVmtxTable, nullptr));
            *aY = -FloatToFixed(mFont->FUnitsToDevUnitsFactor() *
                                (int16_t(metrics->metrics[aGlyph].lsb) +
                                 int16_t(glyf->yMax)));
            return;
        } else {
            
            
            
        }
    }

    

    gfxFontEntry::AutoTable hheaTable(GetFont()->GetFontEntry(),
                                      TRUETYPE_TAG('h','h','e','a'));
    if (hheaTable) {
        uint32_t len;
        const MetricsHeader* hhea =
            reinterpret_cast<const MetricsHeader*>(hb_blob_get_data(hheaTable,
                                                                    &len));
        if (len >= sizeof(MetricsHeader)) {
            *aY = -FloatToFixed(GetFont()->FUnitsToDevUnitsFactor() *
                                int16_t(hhea->ascender));
            return;
        }
    }

    NS_NOTREACHED("we shouldn't be here!");
    *aY = -FloatToFixed(GetFont()->GetAdjustedSize() / 2);
}

static hb_bool_t
HBGetGlyphExtents(hb_font_t *font, void *font_data,
                  hb_codepoint_t glyph,
                  hb_glyph_extents_t *extents,
                  void *user_data)
{
    const gfxHarfBuzzShaper::FontCallbackData *fcd =
        static_cast<const gfxHarfBuzzShaper::FontCallbackData*>(font_data);
    return fcd->mShaper->GetGlyphExtents(glyph, extents);
}





const gfxHarfBuzzShaper::Glyf*
gfxHarfBuzzShaper::FindGlyf(hb_codepoint_t aGlyph, bool *aEmptyGlyf) const
{
    if (!mLoadedLocaGlyf) {
        mLoadedLocaGlyf = true; 
                                
        gfxFontEntry *entry = mFont->GetFontEntry();
        uint32_t len;
        gfxFontEntry::AutoTable headTable(entry,
                                          TRUETYPE_TAG('h','e','a','d'));
        if (!headTable) {
            return nullptr;
        }
        const HeadTable* head =
            reinterpret_cast<const HeadTable*>(hb_blob_get_data(headTable,
                                                                &len));
        if (len < sizeof(HeadTable)) {
            return nullptr;
        }
        mLocaLongOffsets = int16_t(head->indexToLocFormat) > 0;
        mLocaTable = entry->GetFontTable(TRUETYPE_TAG('l','o','c','a'));
        mGlyfTable = entry->GetFontTable(TRUETYPE_TAG('g','l','y','f'));
    }

    if (!mLocaTable || !mGlyfTable) {
        
        return nullptr;
    }

    uint32_t offset; 
    uint32_t len;
    const char* data = hb_blob_get_data(mLocaTable, &len);
    if (mLocaLongOffsets) {
        if ((aGlyph + 1) * sizeof(AutoSwap_PRUint32) > len) {
            return nullptr;
        }
        const AutoSwap_PRUint32* offsets =
            reinterpret_cast<const AutoSwap_PRUint32*>(data);
        offset = offsets[aGlyph];
        *aEmptyGlyf = (offset == uint16_t(offsets[aGlyph + 1]));
    } else {
        if ((aGlyph + 1) * sizeof(AutoSwap_PRUint16) > len) {
            return nullptr;
        }
        const AutoSwap_PRUint16* offsets =
            reinterpret_cast<const AutoSwap_PRUint16*>(data);
        offset = uint16_t(offsets[aGlyph]);
        *aEmptyGlyf = (offset == uint16_t(offsets[aGlyph + 1]));
        offset *= 2;
    }

    data = hb_blob_get_data(mGlyfTable, &len);
    if (offset + sizeof(Glyf) > len) {
        return nullptr;
    }

    return reinterpret_cast<const Glyf*>(data + offset);
}

hb_bool_t
gfxHarfBuzzShaper::GetGlyphExtents(hb_codepoint_t aGlyph,
                                   hb_glyph_extents_t *aExtents) const
{
    bool emptyGlyf;
    const Glyf *glyf = FindGlyf(aGlyph, &emptyGlyf);
    if (!glyf) {
        
        return false;
    }

    if (emptyGlyf) {
        aExtents->x_bearing = 0;
        aExtents->y_bearing = 0;
        aExtents->width = 0;
        aExtents->height = 0;
        return true;
    }

    double f = mFont->FUnitsToDevUnitsFactor();
    aExtents->x_bearing = FloatToFixed(int16_t(glyf->xMin) * f);
    aExtents->width =
        FloatToFixed((int16_t(glyf->xMax) - int16_t(glyf->xMin)) * f);

    
    
    aExtents->y_bearing =
        FloatToFixed(int16_t(glyf->yMax) * f -
                     mFont->GetHorizontalMetrics().emAscent);
    aExtents->height =
        FloatToFixed((int16_t(glyf->yMin) - int16_t(glyf->yMax)) * f);

    return true;
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
            aValue = std::max(aValue, int32_t(lo->value));
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
        mKernTable = mFont->GetFontEntry()->GetFontTable(TRUETYPE_TAG('k','e','r','n'));
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
    const gfxHarfBuzzShaper::FontCallbackData *fcd =
        static_cast<const gfxHarfBuzzShaper::FontCallbackData*>(font_data);
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



static const char16_t sDageshForms[0x05EA - 0x05D0 + 1] = {
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
#ifdef MOZ_WIDGET_ANDROID
    
    
    if (ab == 0x0972) {
        *a = 0x0905;
        *b = 0x0945;
        return true;
    }
#endif
    return nsUnicodeNormalizer::DecomposeNonRecursively(ab, a, b);
}

static PLDHashOperator
AddOpenTypeFeature(const uint32_t& aTag, uint32_t& aValue, void *aUserArg)
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
static const hb_script_t sMathScript =
    hb_ot_tag_to_script(HB_TAG('m','a','t','h'));

bool
gfxHarfBuzzShaper::Initialize()
{
    if (mInitialized) {
        return mHBFont != nullptr;
    }
    mInitialized = true;
    mCallbackData.mShaper = this;

    mUseFontGlyphWidths = mFont->ProvidesGlyphWidths();

    if (!sHBFontFuncs) {
        
        
        sHBFontFuncs = hb_font_funcs_create();
        hb_font_funcs_set_glyph_func(sHBFontFuncs, HBGetGlyph,
                                     nullptr, nullptr);
        hb_font_funcs_set_glyph_h_advance_func(sHBFontFuncs,
                                               HBGetGlyphHAdvance,
                                               nullptr, nullptr);
        hb_font_funcs_set_glyph_v_advance_func(sHBFontFuncs,
                                               HBGetGlyphVAdvance,
                                               nullptr, nullptr);
        hb_font_funcs_set_glyph_h_origin_func(sHBFontFuncs,
                                              HBGetGlyphHOrigin,
                                              nullptr, nullptr);
        hb_font_funcs_set_glyph_v_origin_func(sHBFontFuncs,
                                              HBGetGlyphVOrigin,
                                              nullptr, nullptr);
        hb_font_funcs_set_glyph_extents_func(sHBFontFuncs,
                                             HBGetGlyphExtents,
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

    gfxFontEntry *entry = mFont->GetFontEntry();
    if (!mUseFontGetGlyph) {
        
        mCmapTable = entry->GetFontTable(TRUETYPE_TAG('c','m','a','p'));
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
        if (mCmapFormat <= 0) {
            return false;
        }
    }

    if (!mUseFontGlyphWidths) {
        
        
        if (!LoadHmtxTable()) {
            return false;
        }
    }

    mHBFont = hb_font_create(mHBFace);
    hb_font_set_funcs(mHBFont, sHBFontFuncs, &mCallbackData, nullptr);
    hb_font_set_ppem(mHBFont, mFont->GetAdjustedSize(), mFont->GetAdjustedSize());
    uint32_t scale = FloatToFixed(mFont->GetAdjustedSize()); 
    hb_font_set_scale(mHBFont, scale, scale);

    return true;
}

bool
gfxHarfBuzzShaper::LoadHmtxTable()
{
    
    
    gfxFontEntry *entry = mFont->GetFontEntry();
    gfxFontEntry::AutoTable hheaTable(entry, TRUETYPE_TAG('h','h','e','a'));
    if (hheaTable) {
        uint32_t len;
        const MetricsHeader* hhea =
            reinterpret_cast<const MetricsHeader*>
            (hb_blob_get_data(hheaTable, &len));
        if (len >= sizeof(MetricsHeader)) {
            mNumLongHMetrics = hhea->numOfLongMetrics;
            if (mNumLongHMetrics > 0 &&
                int16_t(hhea->metricDataFormat) == 0) {
                
                
                
                
                mHmtxTable = entry->GetFontTable(TRUETYPE_TAG('h','m','t','x'));
                if (hb_blob_get_length(mHmtxTable) <
                    mNumLongHMetrics * sizeof(LongMetric)) {
                    
                    
                    hb_blob_destroy(mHmtxTable);
                    mHmtxTable = nullptr;
                }
            }
        }
    }
    if (!mHmtxTable) {
        return false;
    }
    return true;
}

bool
gfxHarfBuzzShaper::InitializeVertical()
{
    if (!mHmtxTable) {
        if (!LoadHmtxTable()) {
            return false;
        }
    }

    
    
    gfxFontEntry *entry = mFont->GetFontEntry();
    gfxFontEntry::AutoTable vheaTable(entry, TRUETYPE_TAG('v','h','e','a'));
    if (vheaTable) {
        uint32_t len;
        const MetricsHeader* vhea =
            reinterpret_cast<const MetricsHeader*>
            (hb_blob_get_data(vheaTable, &len));
        if (len >= sizeof(MetricsHeader)) {
            mNumLongVMetrics = vhea->numOfLongMetrics;
            if (mNumLongVMetrics > 0 &&
                int16_t(vhea->metricDataFormat) == 0) {
                mVmtxTable = entry->GetFontTable(TRUETYPE_TAG('v','m','t','x'));
                if (hb_blob_get_length(mVmtxTable) <
                    mNumLongVMetrics * sizeof(LongMetric)) {
                    
                    
                    hb_blob_destroy(mVmtxTable);
                    mVmtxTable = nullptr;
                }
            }
        }
    }

    
    if (entry->HasFontTable(TRUETYPE_TAG('C','F','F',' '))) {
        mVORGTable = entry->GetFontTable(TRUETYPE_TAG('V','O','R','G'));
        if (mVORGTable) {
            uint32_t len;
            const VORG* vorg =
                reinterpret_cast<const VORG*>(hb_blob_get_data(mVORGTable,
                                                               &len));
            if (len < sizeof(VORG) ||
                uint16_t(vorg->majorVersion) != 1 ||
                uint16_t(vorg->minorVersion) != 0 ||
                len < sizeof(VORG) + uint16_t(vorg->numVertOriginYMetrics) *
                              sizeof(VORGrec)) {
                
                
                NS_WARNING("discarding invalid VORG table");
                hb_blob_destroy(mVORGTable);
                mVORGTable = nullptr;
            }
        }
    }

    return true;
}

bool
gfxHarfBuzzShaper::ShapeText(gfxContext      *aContext,
                             const char16_t *aText,
                             uint32_t         aOffset,
                             uint32_t         aLength,
                             int32_t          aScript,
                             bool             aVertical,
                             gfxShapedText   *aShapedText)
{
    
    if (!mFont->SetupCairoFont(aContext)) {
        return false;
    }

    mCallbackData.mContext = aContext;

    if (!Initialize()) {
        return false;
    }

    if (aVertical) {
        if (!InitializeVertical()) {
            return false;
        }
    }

    const gfxFontStyle *style = mFont->GetStyle();

    
    bool addSmallCaps = false;
    if (style->variantCaps != NS_FONT_VARIANT_CAPS_NORMAL) {
        switch (style->variantCaps) {
            case NS_FONT_VARIANT_CAPS_ALLPETITE:
            case NS_FONT_VARIANT_CAPS_PETITECAPS:
                bool synLower, synUpper;
                mFont->SupportsVariantCaps(aScript, style->variantCaps,
                                           addSmallCaps, synLower, synUpper);
                break;
            default:
                break;
        }
    }

    gfxFontEntry *entry = mFont->GetFontEntry();

    
    nsAutoTArray<hb_feature_t,20> features;
    MergeFontFeatures(style,
                      entry->mFeatureSettings,
                      aShapedText->DisableLigatures(),
                      entry->FamilyName(),
                      addSmallCaps,
                      AddOpenTypeFeature,
                      &features);

    bool isRightToLeft = aShapedText->IsRightToLeft();
    hb_buffer_t *buffer = hb_buffer_create();
    hb_buffer_set_unicode_funcs(buffer, sHBUnicodeFuncs);

    hb_buffer_set_direction(buffer,
                            aVertical ? HB_DIRECTION_TTB :
                                        (isRightToLeft ? HB_DIRECTION_RTL :
                                                         HB_DIRECTION_LTR));
    hb_script_t scriptTag;
    if (aShapedText->GetFlags() & gfxTextRunFactory::TEXT_USE_MATH_SCRIPT) {
        scriptTag = sMathScript;
    } else {
        scriptTag = GetHBScriptUsedForShaping(aScript);
    }
    hb_buffer_set_script(buffer, scriptTag);

    hb_language_t language;
    if (style->languageOverride) {
        language = hb_ot_tag_to_language(style->languageOverride);
    } else if (entry->mLanguageOverride) {
        language = hb_ot_tag_to_language(entry->mLanguageOverride);
    } else if (style->explicitLanguage) {
        nsCString langString;
        style->language->ToUTF8String(langString);
        language =
            hb_language_from_string(langString.get(), langString.Length());
    } else {
        language = hb_ot_tag_to_language(HB_OT_TAG_DEFAULT_LANGUAGE);
    }
    hb_buffer_set_language(buffer, language);

    uint32_t length = aLength;
    hb_buffer_add_utf16(buffer,
                        reinterpret_cast<const uint16_t*>(aText),
                        length, 0, length);

    hb_shape(mHBFont, buffer, features.Elements(), features.Length());

    if (isRightToLeft) {
        hb_buffer_reverse(buffer);
    }

    nsresult rv = SetGlyphsFromRun(aContext, aShapedText, aOffset, aLength,
                                   aText, buffer, aVertical);

    NS_WARN_IF_FALSE(NS_SUCCEEDED(rv), "failed to store glyphs into gfxShapedWord");
    hb_buffer_destroy(buffer);

    return NS_SUCCEEDED(rv);
}

#define SMALL_GLYPH_RUN 128 // some testing indicates that 90%+ of text runs
                            
                            

nsresult
gfxHarfBuzzShaper::SetGlyphsFromRun(gfxContext     *aContext,
                                    gfxShapedText  *aShapedText,
                                    uint32_t        aOffset,
                                    uint32_t        aLength,
                                    const char16_t *aText,
                                    hb_buffer_t    *aBuffer,
                                    bool            aVertical)
{
    uint32_t numGlyphs;
    const hb_glyph_info_t *ginfo = hb_buffer_get_glyph_infos(aBuffer, &numGlyphs);
    if (numGlyphs == 0) {
        return NS_OK;
    }

    nsAutoTArray<gfxTextRun::DetailedGlyph,1> detailedGlyphs;

    uint32_t wordLength = aLength;
    static const int32_t NO_GLYPH = -1;
    AutoFallibleTArray<int32_t,SMALL_GLYPH_RUN> charToGlyphArray;
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

    bool roundI;
    bool roundB;
    if (aVertical) {
        aContext->GetRoundOffsetsToPixels(&roundB, &roundI);
    } else {
        aContext->GetRoundOffsetsToPixels(&roundI, &roundB);
    }

    int32_t appUnitsPerDevUnit = aShapedText->GetAppUnitsPerDevUnit();
    gfxShapedText::CompressedGlyph *charGlyphs =
        aShapedText->GetCharacterGlyphs() + aOffset;

    
    
    double hb2appUnits = FixedToFloat(aShapedText->GetAppUnitsPerDevUnit());

    
    
    
    
    
    
    
    
    
    
    hb_position_t residual = 0;

    
    nscoord bPos = 0;

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
                    glyphEnd = std::max(glyphEnd, charToGlyph[i] + 1);
                    
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
        
        endCharIndex = std::min<int32_t>(endCharIndex, wordLength);

        
        int32_t glyphsInClump = glyphEnd - glyphStart;

        
        
        
        if (glyphsInClump == 1 && baseCharIndex + 1 == endCharIndex &&
            aShapedText->FilterIfIgnorable(aOffset + baseCharIndex,
                                           aText[baseCharIndex])) {
            glyphStart = glyphEnd;
            charStart = charEnd;
            continue;
        }

        
        

        hb_position_t i_offset, i_advance; 
        hb_position_t b_offset, b_advance; 
        if (aVertical) {
            i_offset = posInfo[glyphStart].y_offset;
            i_advance = posInfo[glyphStart].y_advance;
            b_offset = posInfo[glyphStart].x_offset;
            b_advance = posInfo[glyphStart].x_advance;
        } else {
            i_offset = posInfo[glyphStart].x_offset;
            i_advance = posInfo[glyphStart].x_advance;
            b_offset = posInfo[glyphStart].y_offset;
            b_advance = posInfo[glyphStart].y_advance;
        }

        nscoord iOffset, advance;
        if (roundI) {
            iOffset =
                appUnitsPerDevUnit * FixedToIntRound(i_offset + residual);
            
            hb_position_t width = i_advance - i_offset;
            int intWidth = FixedToIntRound(width);
            residual = width - FloatToFixed(intWidth);
            advance = appUnitsPerDevUnit * intWidth + iOffset;
        } else {
            iOffset = floor(hb2appUnits * i_offset + 0.5);
            advance = floor(hb2appUnits * i_advance + 0.5);
        }
        
        if (glyphsInClump == 1 &&
            gfxTextRun::CompressedGlyph::IsSimpleGlyphID(ginfo[glyphStart].codepoint) &&
            gfxTextRun::CompressedGlyph::IsSimpleAdvance(advance) &&
            charGlyphs[baseCharIndex].IsClusterStart() &&
            iOffset == 0 && b_offset == 0 &&
            b_advance == 0 && bPos == 0)
        {
            charGlyphs[baseCharIndex].SetSimpleGlyph(advance,
                                                     ginfo[glyphStart].codepoint);
        } else {
            
            
            
            
            while (1) {
                gfxTextRun::DetailedGlyph* details =
                    detailedGlyphs.AppendElement();
                details->mGlyphID = ginfo[glyphStart].codepoint;

                details->mXOffset = iOffset;
                details->mAdvance = advance;

                details->mYOffset = bPos -
                    (roundB ? appUnitsPerDevUnit * FixedToIntRound(b_offset)
                     : floor(hb2appUnits * b_offset + 0.5));

                if (b_advance != 0) {
                    bPos -=
                        roundB ? appUnitsPerDevUnit * FixedToIntRound(b_advance)
                        : floor(hb2appUnits * b_advance + 0.5);
                }
                if (++glyphStart >= glyphEnd) {
                    break;
                }

                if (aVertical) {
                    i_offset = posInfo[glyphStart].y_offset;
                    i_advance = posInfo[glyphStart].y_advance;
                    b_offset = posInfo[glyphStart].x_offset;
                    b_advance = posInfo[glyphStart].x_advance;
                } else {
                    i_offset = posInfo[glyphStart].x_offset;
                    i_advance = posInfo[glyphStart].x_advance;
                    b_offset = posInfo[glyphStart].y_offset;
                    b_advance = posInfo[glyphStart].y_advance;
                }

                if (roundI) {
                    iOffset = appUnitsPerDevUnit *
                        FixedToIntRound(i_offset + residual);
                    
                    
                    
                    
                    
                    i_advance += residual;
                    int intAdvance = FixedToIntRound(i_advance);
                    residual = i_advance - FloatToFixed(intAdvance);
                    advance = appUnitsPerDevUnit * intAdvance;
                } else {
                    iOffset = floor(hb2appUnits * i_offset + 0.5);
                    advance = floor(hb2appUnits * i_advance + 0.5);
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
