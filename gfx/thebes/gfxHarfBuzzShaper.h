




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
                           bool             aVertical,
                           gfxShapedText   *aShapedText);

    
    hb_blob_t * GetFontTable(hb_tag_t aTag) const;

    
    hb_codepoint_t GetGlyph(hb_codepoint_t unicode,
                            hb_codepoint_t variation_selector) const;

    
    hb_position_t GetGlyphHAdvance(hb_codepoint_t glyph) const;

    hb_position_t GetGlyphVAdvance(hb_codepoint_t glyph) const;

    void GetGlyphVOrigin(hb_codepoint_t aGlyph,
                         hb_position_t *aX, hb_position_t *aY) const;

    
    static hb_position_t
    HBGetGlyphHAdvance(hb_font_t *font, void *font_data,
                       hb_codepoint_t glyph, void *user_data);

    
    static hb_position_t
    HBGetGlyphVAdvance(hb_font_t *font, void *font_data,
                       hb_codepoint_t glyph, void *user_data);

    static hb_bool_t
    HBGetGlyphHOrigin(hb_font_t *font, void *font_data,
                      hb_codepoint_t glyph,
                      hb_position_t *x, hb_position_t *y,
                      void *user_data);
    static hb_bool_t
    HBGetGlyphVOrigin(hb_font_t *font, void *font_data,
                      hb_codepoint_t glyph,
                      hb_position_t *x, hb_position_t *y,
                      void *user_data);

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
    nsresult SetGlyphsFromRun(gfxContext     *aContext,
                              gfxShapedText  *aShapedText,
                              uint32_t        aOffset,
                              uint32_t        aLength,
                              const char16_t *aText,
                              hb_buffer_t    *aBuffer,
                              bool            aVertical);

    
    
    nscoord GetGlyphPositions(gfxContext *aContext,
                              hb_buffer_t *aBuffer,
                              nsTArray<nsPoint>& aPositions,
                              uint32_t aAppUnitsPerDevUnit);

    bool InitializeVertical();
    bool LoadHmtxTable();

    
    
    hb_face_t         *mHBFace;

    
    hb_font_t         *mHBFont;

    FontCallbackData   mCallbackData;

    
    
    
    

    
    mutable hb_blob_t *mKernTable;

    
    mutable hb_blob_t *mHmtxTable;

    
    mutable hb_blob_t *mVmtxTable;
    mutable hb_blob_t *mVORGTable;

    
    
    
    mutable hb_blob_t *mCmapTable;
    mutable int32_t    mCmapFormat;
    mutable uint32_t   mSubtableOffset;
    mutable uint32_t   mUVSTableOffset;

    
    
    
    
    
    mutable int32_t    mNumLongHMetrics;
    
    mutable int32_t    mNumLongVMetrics;

    
    
    bool mUseFontGetGlyph;
    
    
    bool mUseFontGlyphWidths;

    bool mInitialized;
    bool mVerticalInitialized;
};

#endif 
