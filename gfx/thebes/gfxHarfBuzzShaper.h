




#ifndef GFX_HARFBUZZSHAPER_H
#define GFX_HARFBUZZSHAPER_H

#include "gfxFont.h"

#include "harfbuzz/hb.h"
#include "nsUnicodeProperties.h"

class gfxHarfBuzzShaper : public gfxFontShaper {
public:
    explicit gfxHarfBuzzShaper(gfxFont *aFont);
    virtual ~gfxHarfBuzzShaper();

    



    struct FontCallbackData {
        gfxHarfBuzzShaper *mShaper;
        gfxContext        *mContext;
    };

    bool Initialize();
    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    
    hb_blob_t * GetFontTable(hb_tag_t aTag) const;

    
    hb_codepoint_t GetGlyph(hb_codepoint_t unicode,
                            hb_codepoint_t variation_selector) const;

    
    hb_position_t GetGlyphHAdvance(gfxContext *aContext,
                                   hb_codepoint_t glyph) const;

    
    static hb_position_t
    HBGetGlyphHAdvance(hb_font_t *font, void *font_data,
                       hb_codepoint_t glyph, void *user_data);

    hb_position_t GetHKerning(uint16_t aFirstGlyph,
                              uint16_t aSecondGlyph) const;

    static hb_script_t
    GetHBScriptUsedForShaping(int32_t aScript) {
        
        hb_script_t hbScript;
        if (aScript <= MOZ_SCRIPT_INHERITED) {
            
            
            hbScript = HB_SCRIPT_LATIN;
        } else {
            hbScript =
                hb_script_t(mozilla::unicode::GetScriptTagForCode(aScript));
        }
        return hbScript;
    }

protected:
    nsresult SetGlyphsFromRun(gfxContext      *aContext,
                              gfxShapedText   *aShapedText,
                              uint32_t         aOffset,
                              uint32_t         aLength,
                              const char16_t *aText,
                              hb_buffer_t     *aBuffer);

    
    
    nscoord GetGlyphPositions(gfxContext *aContext,
                              hb_buffer_t *aBuffer,
                              nsTArray<nsPoint>& aPositions,
                              uint32_t aAppUnitsPerDevUnit);

    
    
    hb_face_t         *mHBFace;

    
    hb_font_t         *mHBFont;

    FontCallbackData   mCallbackData;

    
    
    
    

    
    mutable hb_blob_t *mKernTable;

    
    
    
    
    
    mutable hb_blob_t *mHmtxTable;
    mutable int32_t    mNumLongMetrics;

    
    
    
    mutable hb_blob_t *mCmapTable;
    mutable int32_t    mCmapFormat;
    mutable uint32_t   mSubtableOffset;
    mutable uint32_t   mUVSTableOffset;

    
    
    bool mUseFontGetGlyph;
    
    
    bool mUseFontGlyphWidths;

    bool mInitialized;
};

#endif 
