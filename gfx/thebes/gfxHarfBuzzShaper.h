




































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

    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aText);

    
    hb_blob_t * GetFontTable(hb_tag_t aTag) const;

    
    hb_codepoint_t GetGlyph(hb_codepoint_t unicode,
                            hb_codepoint_t variation_selector) const;

    
    hb_position_t GetGlyphHAdvance(gfxContext *aContext,
                                   hb_codepoint_t glyph) const;

    hb_position_t GetHKerning(PRUint16 aFirstGlyph,
                              PRUint16 aSecondGlyph) const;

protected:
    nsresult SetGlyphsFromRun(gfxContext *aContext,
                              gfxShapedWord *aShapedWord,
                              hb_buffer_t *aBuffer);

    
    
    nscoord GetGlyphPositions(gfxContext *aContext,
                              hb_buffer_t *aBuffer,
                              nsTArray<nsPoint>& aPositions,
                              PRUint32 aAppUnitsPerDevUnit);

    
    hb_face_t         *mHBFace;

    
    
    
    

    
    mutable hb_blob_t *mKernTable;

    
    
    
    
    
    mutable hb_blob_t *mHmtxTable;
    mutable PRInt32    mNumLongMetrics;

    
    
    
    mutable hb_blob_t *mCmapTable;
    mutable PRInt32    mCmapFormat;
    mutable PRUint32   mSubtableOffset;
    mutable PRUint32   mUVSTableOffset;

    
    
    bool mUseFontGetGlyph;
    
    
    bool mUseFontGlyphWidths;
};

#endif 
