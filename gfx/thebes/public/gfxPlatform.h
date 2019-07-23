





































#ifndef GFX_PLATFORM_H
#define GFX_PLATFORM_H

#include "prtypes.h"
#include "nsVoidArray.h"

#include "gfxTypes.h"
#include "gfxASurface.h"

#ifdef XP_OS2
#undef OS2EMX_PLAIN_CHAR
#endif

typedef void* cmsHPROFILE;
typedef void* cmsHTRANSFORM;

class gfxImageSurface;
class gfxFontGroup;
struct gfxFontStyle;

class THEBES_API gfxPlatform {
public:
    




    static gfxPlatform *GetPlatform();

    


    static nsresult Init();

    


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

    


    virtual gfxFontGroup *CreateFontGroup(const nsAString& aFamilies,
                                          const gfxFontStyle *aStyle) = 0;

    
    static PRBool DoesARGBImageDataHaveAlpha(PRUint8* data,
                                             PRUint32 width,
                                             PRUint32 height,
                                             PRUint32 stride);

    void GetPrefFonts(const char *aLangGroup, nsString& array, PRBool aAppendUnicode = PR_TRUE);

    


    static PRBool IsCMSEnabled();

    


    static cmsHPROFILE GetCMSOutputProfile();

    


    static cmsHTRANSFORM GetCMSRGBTransform();

    


    static cmsHTRANSFORM GetCMSRGBATransform();

protected:
    gfxPlatform() { }
    virtual ~gfxPlatform();

private:
    virtual cmsHPROFILE GetPlatformCMSOutputProfile();
};

#endif 
