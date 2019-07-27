




#ifndef GFX_GRAPHITESHAPER_H
#define GFX_GRAPHITESHAPER_H

#include "gfxFont.h"

struct gr_face;
struct gr_font;
struct gr_segment;

class gfxGraphiteShaper : public gfxFontShaper {
public:
    explicit gfxGraphiteShaper(gfxFont *aFont);
    virtual ~gfxGraphiteShaper();

    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    static void Shutdown();

protected:
    nsresult SetGlyphsFromSegment(gfxContext      *aContext,
                                  gfxShapedText   *aShapedText,
                                  uint32_t         aOffset,
                                  uint32_t         aLength,
                                  const char16_t *aText,
                                  gr_segment      *aSegment);

    static float GrGetAdvance(const void* appFontHandle, uint16_t glyphid);

    gr_face *mGrFace; 
                      
    gr_font *mGrFont; 

    struct CallbackData {
        gfxFont           *mFont;
        gfxGraphiteShaper *mShaper;
        gfxContext        *mContext;
    };

    CallbackData mCallbackData;
    bool mFallbackToSmallCaps; 

    
    static uint32_t GetGraphiteTagForLang(const nsCString& aLang);
    static nsTHashtable<nsUint32HashKey> *sLanguageTags;
};

#endif 
