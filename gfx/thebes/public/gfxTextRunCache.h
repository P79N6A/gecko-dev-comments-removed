




































#ifndef GFX_TEXT_RUN_CACHE_H
#define GFX_TEXT_RUN_CACHE_H

#include "gfxFont.h"







class THEBES_API gfxTextRunCache {
public:
    










    static gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   gfxContext *aRefContext,
                                   PRUint32 aAppUnitsPerDevUnit,
                                   PRUint32 aFlags);

    


    static gfxTextRun *MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   const gfxTextRunFactory::Parameters* aParams,
                                   PRUint32 aFlags);

    










    static gfxTextRun *MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
                                   gfxFontGroup *aFontGroup,
                                   gfxContext *aRefContext,
                                   PRUint32 aAppUnitsPerDevUnit,
                                   PRUint32 aFlags);
    
    



    static void ReleaseTextRun(gfxTextRun *aTextRun);

    class AutoTextRun {
    public:
    	AutoTextRun(gfxTextRun *aTextRun) : mTextRun(aTextRun) {}
    	AutoTextRun() : mTextRun(nsnull) {}
    	AutoTextRun& operator=(gfxTextRun *aTextRun) {
            gfxTextRunCache::ReleaseTextRun(mTextRun);
            mTextRun = aTextRun;
            return *this;
        }
        ~AutoTextRun() {
            gfxTextRunCache::ReleaseTextRun(mTextRun);
        }
        gfxTextRun *get() { return mTextRun; }
        gfxTextRun *operator->() { return mTextRun; }
    private:
        gfxTextRun *mTextRun;
    };

protected:
    friend class gfxPlatform;

    static nsresult Init();
    static void Shutdown();
};

#endif 
