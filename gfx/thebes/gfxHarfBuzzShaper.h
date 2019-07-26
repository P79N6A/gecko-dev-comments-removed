




#ifndef GFX_HARFBUZZSHAPER_H
#define GFX_HARFBUZZSHAPER_H

#include "gfxTypes.h"
#include "gfxFont.h"
#include "nsDataHashtable.h"
#include "nsPoint.h"

#include "harfbuzz/hb.h"

class gfxHarfBuzzShaper : public gfxFontShaper {
public:
    gfxHarfBuzzShaper(gfxFont *aFont);
    virtual ~gfxHarfBuzzShaper();

    virtual bool ShapeText(gfxContext      *aContext,
                           const PRUnichar *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    
    hb_blob_t * GetFontTable(hb_tag_t aTag) const;

    
    hb_codepoint_t GetGlyph(hb_codepoint_t unicode,
                            hb_codepoint_t variation_selector) const;

    
    hb_position_t GetGlyphHAdvance(gfxContext *aContext,
                                   hb_codepoint_t glyph) const;

    hb_position_t GetHKerning(uint16_t aFirstGlyph,
                              uint16_t aSecondGlyph) const;

protected:
    nsresult SetGlyphsFromRun(gfxContext      *aContext,
                              gfxShapedText   *aShapedText,
                              uint32_t         aOffset,
                              uint32_t         aLength,
                              const PRUnichar *aText,
                              hb_buffer_t     *aBuffer);

    
    
    nscoord GetGlyphPositions(gfxContext *aContext,
                              hb_buffer_t *aBuffer,
                              nsTArray<nsPoint>& aPositions,
                              uint32_t aAppUnitsPerDevUnit);

    
    hb_face_t         *mHBFace;

    
    
    
    

    
    mutable hb_blob_t *mKernTable;

    
    
    
    
    
    mutable hb_blob_t *mHmtxTable;
    mutable int32_t    mNumLongMetrics;

    
    
    
    mutable hb_blob_t *mCmapTable;
    mutable int32_t    mCmapFormat;
    mutable uint32_t   mSubtableOffset;
    mutable uint32_t   mUVSTableOffset;

    
    
    bool mUseFontGetGlyph;
    
    
    bool mUseFontGlyphWidths;
};

#endif 
