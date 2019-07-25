





































#ifndef GFX_PLATFORM_H
#define GFX_PLATFORM_H

#include "prtypes.h"
#include "prlog.h"
#include "nsTArray.h"
#include "nsStringGlue.h"
#include "nsIObserver.h"

#include "gfxTypes.h"
#include "gfxASurface.h"
#include "gfxColor.h"

#include "qcms.h"

#include "gfx2DGlue.h"
#include "mozilla/RefPtr.h"
#include "GfxInfoCollector.h"

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
class gfxTextRun;
class nsIURI;
class nsIAtom;

extern mozilla::gfx::UserDataKey kThebesSurfaceKey;
void DestroyThebesSurface(void *data);

extern cairo_user_data_key_t kDrawTarget;





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
    eFontPrefLang_Tibetan     = 29,

    eFontPrefLang_LangCount   = 30, 

    eFontPrefLang_Others      = 30, 
    eFontPrefLang_UserDefined = 31,

    eFontPrefLang_CJKSet      = 32, 
    eFontPrefLang_AllCount    = 33
};

enum eCMSMode {
    eCMSMode_Off          = 0,     
    eCMSMode_All          = 1,     
    eCMSMode_TaggedOnly   = 2,     
    eCMSMode_AllCount     = 3
};

enum eGfxLog {
    
    eGfxLog_fontlist         = 0,
    
    eGfxLog_fontinit         = 1,
    
    eGfxLog_textrun          = 2,
    
    eGfxLog_textrunui        = 3,
    
    eGfxLog_cmapdata         = 4
};


const PRUint32 kMaxLenPrefLangList = 32;

#define UNINITIALIZED_VALUE  (-1)

typedef gfxASurface::gfxImageFormat gfxImageFormat;

inline const char*
GetBackendName(mozilla::gfx::BackendType aBackend)
{
  switch (aBackend) {
      case mozilla::gfx::BACKEND_DIRECT2D:
        return "direct2d";
      case mozilla::gfx::BACKEND_COREGRAPHICS:
        return "quartz";
      case mozilla::gfx::BACKEND_CAIRO:
        return "cairo";
      case mozilla::gfx::BACKEND_SKIA:
        return "skia";
      case mozilla::gfx::BACKEND_NONE:
        return "none";
  }
  MOZ_NOT_REACHED("Incomplet switch");
}

class THEBES_API gfxPlatform {
public:
    




    static gfxPlatform *GetPlatform();


    



    static void Shutdown();

    



    virtual already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                                 gfxASurface::gfxContentType contentType) = 0;


    virtual already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                        gfxASurface::gfxImageFormat format);

    virtual mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateDrawTargetForSurface(gfxASurface *aSurface);

    virtual mozilla::RefPtr<mozilla::gfx::SourceSurface>
      GetSourceSurfaceForSurface(mozilla::gfx::DrawTarget *aTarget, gfxASurface *aSurface);

    virtual mozilla::RefPtr<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(gfxFont *aFont);

    virtual already_AddRefed<gfxASurface>
      GetThebesSurfaceForDrawTarget(mozilla::gfx::DrawTarget *aTarget);

    virtual mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateOffscreenDrawTarget(const mozilla::gfx::IntSize& aSize, mozilla::gfx::SurfaceFormat aFormat);

    virtual mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateDrawTargetForData(unsigned char* aData, const mozilla::gfx::IntSize& aSize, 
                              int32_t aStride, mozilla::gfx::SurfaceFormat aFormat);

    virtual bool SupportsAzure(mozilla::gfx::BackendType& aBackend) { return false; }

    void GetAzureBackendInfo(mozilla::widget::InfoObject &aObj) {
      mozilla::gfx::BackendType backend;
      if (SupportsAzure(backend)) {
        aObj.DefineProperty("AzureBackend", GetBackendName(backend)); 
      }
    }

    



    virtual void SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString);

    




    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts);

    


    virtual nsresult UpdateFontList();

    




    virtual gfxPlatformFontList *CreatePlatformFontList() {
        NS_NOTREACHED("oops, this platform doesn't have a gfxPlatformFontList implementation");
        return nsnull;
    }

    





    typedef bool (*FontResolverCallback) (const nsAString& aName,
                                            void *aClosure);
    virtual nsresult ResolveFontName(const nsAString& aFontName,
                                     FontResolverCallback aCallback,
                                     void *aClosure,
                                     bool& aAborted) = 0;

    



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

    


    bool DownloadableFontsEnabled();

    


    bool SanitizeDownloadedFonts();

    






    virtual bool FontHintingEnabled() { return true; }

    


    bool UseCmapsDuringSystemFallback();

#ifdef MOZ_GRAPHITE
    



    bool UseGraphiteShaping();
#endif

    




    bool UseHarfBuzzForScript(PRInt32 aScriptCode);

    
    virtual bool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags) { return false; }

    void GetPrefFonts(nsIAtom *aLanguage, nsString& array, bool aAppendUnicode = true);

    
    void GetLangPrefs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang);
    
    




    typedef bool (*PrefFontCallback) (eFontPrefLang aLang, const nsAString& aName,
                                        void *aClosure);
    static bool ForEachPrefFont(eFontPrefLang aLangArray[], PRUint32 aLangArrayLen,
                                  PrefFontCallback aCallback,
                                  void *aClosure);

    
    static eFontPrefLang GetFontPrefLangFor(const char* aLang);

    
    static eFontPrefLang GetFontPrefLangFor(nsIAtom *aLang);

    
    static const char* GetPrefLangName(eFontPrefLang aLang);
   
    
    static eFontPrefLang GetFontPrefLangFor(PRUint8 aUnicodeRange);

    
    static bool IsLangCJK(eFontPrefLang aLang);
    
    
    static void AppendPrefLang(eFontPrefLang aPrefLangs[], PRUint32& aLen, eFontPrefLang aAddLang);

    
    
    virtual void GetCommonFallbackFonts(const PRUint32 ,
                                        PRInt32 ,
                                        nsTArray<const char*>& )
    {
        
    }

    
    static bool UseAzureContentDrawing();
    
    


    static eCMSMode GetCMSMode();

    









    static int GetRenderingIntent();

    




    static void TransformPixel(const gfxRGBA& in, gfxRGBA& out, qcms_transform *transform);

    


    static qcms_profile* GetCMSOutputProfile();

    


    static qcms_profile* GetCMSsRGBProfile();

    


    static qcms_transform* GetCMSRGBTransform();

    


    static qcms_transform* GetCMSInverseRGBTransform();

    


    static qcms_transform* GetCMSRGBATransform();

    virtual void FontsPrefsChanged(const char *aPref);

    PRInt32 GetBidiNumeralOption();

    



    gfxASurface* ScreenReferenceSurface() { return mScreenReferenceSurface; }

    virtual gfxImageFormat GetOffscreenFormat()
    { return gfxASurface::FormatFromContent(gfxASurface::CONTENT_COLOR); }

    


    static PRLogModuleInfo* GetLog(eGfxLog aWhichLog);

protected:
    gfxPlatform();
    virtual ~gfxPlatform();

    void AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], PRUint32 &aLen, 
                            eFontPrefLang aCharLang, eFontPrefLang aPageLang);
                                               
    PRInt8  mAllowDownloadableFonts;
    PRInt8  mDownloadableFontsSanitize;
#ifdef MOZ_GRAPHITE
    PRInt8  mGraphiteShapingEnabled;
#endif

    PRInt8  mBidiNumeralOption;

    
    
    PRInt8  mFallbackUsesCmaps;

    
    PRInt32 mUseHarfBuzzScripts;

    
    mozilla::gfx::BackendType mPreferredDrawTargetBackend;

private:
    


    static void Init();

    virtual qcms_profile* GetPlatformCMSOutputProfile();

    nsRefPtr<gfxASurface> mScreenReferenceSurface;
    nsTArray<PRUint32> mCJKPrefLangs;
    nsCOMPtr<nsIObserver> mSRGBOverrideObserver;
    nsCOMPtr<nsIObserver> mFontPrefsObserver;
    mozilla::widget::GfxInfoCollector<gfxPlatform> mAzureBackendCollector;
};

#endif 
