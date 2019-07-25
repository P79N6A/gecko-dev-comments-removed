




































#ifndef GFX_GRAPHITESHAPER_H
#define GFX_GRAPHITESHAPER_H

#include "gfxTypes.h"
#include "gfxFont.h"
#include "nsDataHashtable.h"
#include "nsHashKeys.h"

class gr_face;
class gr_font;
class gr_segment;

class gfxGraphiteShaper : public gfxFontShaper {
public:
    gfxGraphiteShaper(gfxFont *aFont);
    virtual ~gfxGraphiteShaper();

    virtual bool ShapeWord(gfxContext *aContext,
                           gfxShapedWord *aShapedWord,
                           const PRUnichar *aText);

    const void* GetTable(PRUint32 aTag, size_t *aLength);

    static void Shutdown();

    struct CallbackData {
        gfxFont           *mFont;
        gfxGraphiteShaper *mShaper;
        gfxContext        *mContext;
    };

    struct TableRec {
        hb_blob_t  *mBlob;
        const void *mData;
        PRUint32    mLength;
    };

protected:
    nsresult SetGlyphsFromSegment(gfxShapedWord *aShapedWord,
                                  gr_segment *aSegment);

    gr_face *mGrFace;
    gr_font *mGrFont;

    CallbackData mCallbackData;

    nsDataHashtable<nsUint32HashKey,TableRec> mTables;

    
    
    bool mUseFontGlyphWidths;

    
    static PRUint32 GetGraphiteTagForLang(const nsCString& aLang);
    static nsTHashtable<nsUint32HashKey> sLanguageTags;
};

#endif 
