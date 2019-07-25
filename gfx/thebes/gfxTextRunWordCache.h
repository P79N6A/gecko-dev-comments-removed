






































#ifndef GFX_TEXT_RUN_WORD_CACHE_H
#define GFX_TEXT_RUN_WORD_CACHE_H

#include "gfxFont.h"





class THEBES_API gfxTextRunWordCache {
public:
    enum {
      TEXT_IN_CACHE = 0x10000000,

      



      TEXT_TRAILING_ARABICCHAR = 0x20000000,
      




      TEXT_INCOMING_ARABICCHAR = 0x40000000,

      TEXT_UNUSED_FLAGS = 0x80000000
    };

    








    static gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   const gfxFontGroup::Parameters *aParams,
                                   PRUint32 aFlags);
    








    static gfxTextRun *MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   const gfxFontGroup::Parameters *aParams,
                                   PRUint32 aFlags);

    



    static void RemoveTextRun(gfxTextRun *aTextRun);

    



    static void Flush();

    






    static void ComputeStorage(PRUint64 *aTotal);

protected:
    friend class gfxPlatform;

    static nsresult Init();
    static void Shutdown();
};

#endif
