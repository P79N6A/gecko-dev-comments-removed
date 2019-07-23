




































#ifndef GFX_TEXT_RUN_WORD_CACHE_H
#define GFX_TEXT_RUN_WORD_CACHE_H

#include "gfxFont.h"





class THEBES_API gfxTextRunWordCache {
public:
    enum { TEXT_IN_CACHE = 0x10000000 };

    








    static gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   const gfxFontGroup::Parameters *aParams,
                                   PRUint32 aFlags);
    








    static gfxTextRun *MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   const gfxFontGroup::Parameters *aParams,
                                   PRUint32 aFlags);

    



    static void RemoveTextRun(gfxTextRun *aTextRun);

protected:
    friend class gfxPlatform;

    static nsresult Init();
    static void Shutdown();
};

#endif
