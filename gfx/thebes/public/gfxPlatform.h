





































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




enum eFontPrefLang {
    eFontPrefLang_Western     =  0,
    eFontPrefLang_CentEuro    =  1,
    eFontPrefLang_Japanese    =  2,
    eFontPrefLang_ChineseTW   =  3,
    eFontPrefLang_ChineseCN   =  4,
    eFontPrefLang_ChineseHK   =  5,
    eFontPrefLang_Korean      =  6,
    eFontPrefLang_Cyrillic    =  7,
    eFontPrefLang_Baltic      =  8,
    eFontPrefLang_Greek       =  9,
    eFontPrefLang_Turkish     = 10,
    eFontPrefLang_Thai        = 11,
    eFontPrefLang_Hebrew      = 12,
    eFontPrefLang_Arabic      = 13,
    eFontPrefLang_Devanagari  = 14,
    eFontPrefLang_Tamil       = 15,
    eFontPrefLang_Armenian    = 16,
    eFontPrefLang_Bengali     = 17,
    eFontPrefLang_Canadian    = 18,
    eFontPrefLang_Ethiopic    = 19,
    eFontPrefLang_Georgian    = 20,
    eFontPrefLang_Gujarati    = 21,
    eFontPrefLang_Gurmukhi    = 22,
    eFontPrefLang_Khmer       = 23,
    eFontPrefLang_Malayalam   = 24,

    eFontPrefLang_LangCount   = 25, 

    eFontPrefLang_Others      = 25, 
    eFontPrefLang_UserDefined = 26,

    eFontPrefLang_CJKSet      = 27, 
    eFontPrefLang_AllCount    = 28
};


const PRUint32 kMaxLenPrefLangList = 30;

class THEBES_API gfxPlatform {
public:
    




    static gfxPlatform *GetPlatform();

    


    static nsresult Init();

    


    static void Shutdown();

    


    static PRBool UseGlitz();

    


    static void SetUseGlitz(PRBool use);

    



    virtual already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                                 gfxASurface::gfxImageFormat imageFormat) = 0;


    virtual already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                        gfxASurface::gfxImageFormat format);

    



    




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

    



    virtual nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) = 0;

    


    virtual gfxFontGroup *CreateFontGroup(const nsAString& aFamilies,
                                          const gfxFontStyle *aStyle) = 0;

    void GetPrefFonts(const char *aLangGroup, nsString& array, PRBool aAppendUnicode = PR_TRUE);

    




    typedef PRBool (*PrefFontCallback) (eFontPrefLang aLang, const nsAString& aName,
                                        void *aClosure);
    static PRBool ForEachPrefFont(eFontPrefLang aLangArray[], PRUint32 aLangArrayLen,
                                  PrefFontCallback aCallback,
                                  void *aClosure);

    
    static eFontPrefLang GetFontPrefLangFor(const char* aLang);

    
    static const char* GetPrefLangName(eFontPrefLang aLang);
   
    
    static PRBool IsLangCJK(eFontPrefLang aLang);
    
    
    static void AppendPrefLang(eFontPrefLang aPrefLangs[], PRUint32& aLen, eFontPrefLang aAddLang);
    
    


    static PRBool IsCMSEnabled();

    


    static cmsHPROFILE GetCMSOutputProfile();

    


    static cmsHTRANSFORM GetCMSRGBTransform();

    


    static cmsHTRANSFORM GetCMSInverseRGBTransform();

    


    static cmsHTRANSFORM GetCMSRGBATransform();

protected:
    gfxPlatform() { }
    virtual ~gfxPlatform();

private:
    virtual cmsHPROFILE GetPlatformCMSOutputProfile();
};

#endif 
