




#ifndef GFX_FT2FONTS_H
#define GFX_FT2FONTS_H

#include "mozilla/MemoryReporting.h"
#include "cairo.h"
#include "gfxTypes.h"
#include "gfxFont.h"
#include "gfxFT2FontBase.h"
#include "gfxContext.h"
#include "gfxFontUtils.h"
#include "gfxUserFontSet.h"

class FT2FontEntry;

class gfxFT2Font : public gfxFT2FontBase {
public: 
    gfxFT2Font(cairo_scaled_font_t *aCairoFont,
               FT2FontEntry *aFontEntry,
               const gfxFontStyle *aFontStyle,
               bool aNeedsBold);
    virtual ~gfxFT2Font ();

    FT2FontEntry *GetFontEntry();

    static already_AddRefed<gfxFT2Font>
    GetOrMakeFont(const nsAString& aName, const gfxFontStyle *aStyle,
                  bool aNeedsBold = false);

    static already_AddRefed<gfxFT2Font>
    GetOrMakeFont(FT2FontEntry *aFontEntry, const gfxFontStyle *aStyle,
                  bool aNeedsBold = false);

    struct CachedGlyphData {
        CachedGlyphData()
            : glyphIndex(0xffffffffU) { }

        CachedGlyphData(uint32_t gid)
            : glyphIndex(gid) { }

        uint32_t glyphIndex;
        int32_t lsbDelta;
        int32_t rsbDelta;
        int32_t xAdvance;
    };

    const CachedGlyphData* GetGlyphDataForChar(uint32_t ch) {
        CharGlyphMapEntryType *entry = mCharGlyphCache.PutEntry(ch);

        if (!entry)
            return nullptr;

        if (entry->mData.glyphIndex == 0xffffffffU) {
            
            FillGlyphDataForChar(ch, &entry->mData);
        }

        return &entry->mData;
    }

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;

#ifdef USE_SKIA
    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions> GetGlyphRenderingOptions();
#endif

protected:
    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    void FillGlyphDataForChar(uint32_t ch, CachedGlyphData *gd);

    void AddRange(const char16_t *aText,
                  uint32_t         aOffset,
                  uint32_t         aLength,
                  gfxShapedText   *aShapedText);

    typedef nsBaseHashtableET<nsUint32HashKey, CachedGlyphData> CharGlyphMapEntryType;
    typedef nsTHashtable<CharGlyphMapEntryType> CharGlyphMap;
    CharGlyphMap mCharGlyphCache;
};

#endif 

