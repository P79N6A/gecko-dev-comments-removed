





































#ifndef GFX_PLATFORM_H
#define GFX_PLATFORM_H

#include "prtypes.h"
#include "nsTArray.h"

#include "nsIObserver.h"

#include "gfxTypes.h"
#include "gfxASurface.h"
#include "gfxColor.h"

#include "qcms.h"
#ifdef XP_OS2
#undef OS2EMX_PLAIN_CHAR
#endif

class gfxImageSurface;
class gfxFont;
class gfxFontGroup;
struct gfxFontStyle;
class gfxUserFontSet;
class gfxFontEntry;
class gfxProxyFontEntry;
class gfxPlatformFontList;
class nsIURI;




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
    eFontPrefLang_Oriya       = 25,
    eFontPrefLang_Telugu      = 26,
    eFontPrefLang_Kannada     = 27,
    eFontPrefLang_Sinhala     = 28,

    eFontPrefLang_LangCount   = 29, 

    eFontPrefLang_Others      = 29, 
    eFontPrefLang_UserDefined = 30,

    eFontPrefLang_CJKSet      = 31, 
    eFontPrefLang_AllCount    = 32
};

enum eCMSMode {
    eCMSMode_Off          = 0,     
    eCMSMode_All          = 1,     
    eCMSMode_TaggedOnly   = 2,     
    eCMSMode_AllCount     = 3
};


const PRUint32 kMaxLenPrefLangList = 32;

class THEBES_API gfxPlatform {
public:
    




    static gfxPlatform *GetPlatform();

    


    static nsresult Init();

    


    static void Shutdown();

    



    virtual already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                                 gfxASurface::gfxImageFormat imageFormat) = 0;


    virtual already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                        gfxASurface::gfxImageFormat format);

    



    




    virtual nsresult GetFontList(const nsACString& aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts);

    


    virtual nsresult UpdateFontList();

    


    virtual gfxPlatformFontList *CreatePlatformFontList() {
        NS_NOTREACHED("oops, this platform doesn't have a gfxPlatformFontList implementation");
        return nsnull;
    }

    





    typedef PRBool (*FontResolverCallback) (const nsAString& aName,
                                            void *aClosure);
    virtual nsresult ResolveFontName(const nsAString& aFontName,
                                     FontResolverCallback aCallback,
                                     void *aClosure,
                                     PRBool& aAborted) = 0;

    



    virtual nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) = 0;

    


    virtual gfxFontGroup *CreateFontGroup(const nsAString& aFamilies,
                                          const gfxFontStyle *aStyle,
                                          gfxUserFontSet *aUserFontSet) = 0;
                                          
                                          
    





    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName)
    { return nsnull; }

    







    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    


    virtual PRBool DownloadableFontsEnabled();

    
    virtual PRBool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags) { return PR_FALSE; }

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
    
    


    static eCMSMode GetCMSMode();

    









    static int GetRenderingIntent();

    




    static void TransformPixel(const gfxRGBA& in, gfxRGBA& out, qcms_transform *transform);

    


    static qcms_profile* GetCMSOutputProfile();

    


    static qcms_profile* GetCMSsRGBProfile();

    


    static qcms_transform* GetCMSRGBTransform();

    


    static qcms_transform* GetCMSInverseRGBTransform();

    


    static qcms_transform* GetCMSRGBATransform();

protected:
    gfxPlatform() { }
    virtual ~gfxPlatform();

private:
    virtual qcms_profile* GetPlatformCMSOutputProfile();

    nsCOMPtr<nsIObserver> overrideObserver;
};

#endif 
