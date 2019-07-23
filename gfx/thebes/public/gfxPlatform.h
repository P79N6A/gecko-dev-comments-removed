





































#ifndef GFX_PLATFORM_H
#define GFX_PLATFORM_H

#include "prtypes.h"
#include "nsVoidArray.h"

#include "gfxTypes.h"
#include "gfxASurface.h"

class gfxImageSurface;

class THEBES_API gfxPlatform {
public:
    




    static gfxPlatform *GetPlatform();

    


    static void Shutdown();

    


    static PRBool UseGlitz();

    


    static void SetUseGlitz(PRBool use);

    





    virtual already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                                 gfxASurface::gfxImageFormat imageFormat) = 0;


    virtual already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface);

    



    




    virtual nsresult GetFontList(const nsACString& aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsStringArray& aListOfFonts);

    


    virtual nsresult UpdateFontList();

    





    typedef PRBool (*FontResolverCallback) (const nsAString& aName,
                                            void *aClosure);
    virtual nsresult ResolveFontName(const nsAString& aFontName,
                                     FontResolverCallback aCallback,
                                     void *aClosure,
                                     PRBool& aAborted) = 0;

    
    static PRBool DoesARGBImageDataHaveAlpha(PRUint8* data,
                                             PRUint32 width,
                                             PRUint32 height,
                                             PRUint32 stride);

    void GetPrefFonts(const char *aLangGroup, nsString& array);

protected:
    gfxPlatform() { }
    virtual ~gfxPlatform();

};

#endif 
