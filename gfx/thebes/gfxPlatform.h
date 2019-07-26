




#ifndef GFX_PLATFORM_H
#define GFX_PLATFORM_H

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


const uint32_t kMaxLenPrefLangList = 32;

#define UNINITIALIZED_VALUE  (-1)

typedef gfxASurface::gfxImageFormat gfxImageFormat;

inline const char*
GetBackendName(mozilla::gfx::BackendType aBackend)
{
  switch (aBackend) {
      case mozilla::gfx::BACKEND_DIRECT2D:
        return "direct2d";
      case mozilla::gfx::BACKEND_COREGRAPHICS_ACCELERATED:
        return "quartz accelerated";
      case mozilla::gfx::BACKEND_COREGRAPHICS:
        return "quartz";
      case mozilla::gfx::BACKEND_CAIRO:
        return "cairo";
      case mozilla::gfx::BACKEND_SKIA:
        return "skia";
      case mozilla::gfx::BACKEND_RECORDING:
        return "recording";
      case mozilla::gfx::BACKEND_NONE:
        return "none";
  }
  MOZ_NOT_REACHED("Incomplete switch");
}

class THEBES_API gfxPlatform {
public:
    




    static gfxPlatform *GetPlatform();


    



    static void Shutdown();

    



    virtual already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                                 gfxASurface::gfxContentType contentType) = 0;

    







    virtual already_AddRefed<gfxASurface>
      CreateOffscreenImageSurface(const gfxIntSize& aSize,
                                  gfxASurface::gfxContentType aContentType);

    virtual already_AddRefed<gfxASurface> OptimizeImage(gfxImageSurface *aSurface,
                                                        gfxASurface::gfxImageFormat format);

    virtual mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateDrawTargetForSurface(gfxASurface *aSurface, const mozilla::gfx::IntSize& aSize);

    







    virtual mozilla::RefPtr<mozilla::gfx::SourceSurface>
      GetSourceSurfaceForSurface(mozilla::gfx::DrawTarget *aTarget, gfxASurface *aSurface);

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    











    virtual already_AddRefed<gfxASurface>
      CreateThebesSurfaceAliasForDrawTarget_hack(mozilla::gfx::DrawTarget *aTarget) {
      
      
      return GetThebesSurfaceForDrawTarget(aTarget);
    }

    virtual already_AddRefed<gfxASurface>
      GetThebesSurfaceForDrawTarget(mozilla::gfx::DrawTarget *aTarget);

    virtual mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateOffscreenDrawTarget(const mozilla::gfx::IntSize& aSize, mozilla::gfx::SurfaceFormat aFormat);

    virtual mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateDrawTargetForData(unsigned char* aData, const mozilla::gfx::IntSize& aSize, 
                              int32_t aStride, mozilla::gfx::SurfaceFormat aFormat);

    bool SupportsAzureContent() {
      return GetContentBackend() != mozilla::gfx::BACKEND_NONE;
    }

    void GetAzureBackendInfo(mozilla::widget::InfoObject &aObj) {
      aObj.DefineProperty("AzureCanvasBackend", GetBackendName(mPreferredCanvasBackend));
      aObj.DefineProperty("AzureFallbackCanvasBackend", GetBackendName(mFallbackCanvasBackend));
      aObj.DefineProperty("AzureContentBackend", GetBackendName(mContentBackend));
    }

    mozilla::gfx::BackendType GetPreferredCanvasBackend() {
      return mPreferredCanvasBackend;
    }

    



    virtual void SetupClusterBoundaries(gfxTextRun *aTextRun, const PRUnichar *aString);

    




    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts);

    


    virtual nsresult UpdateFontList();

    




    virtual gfxPlatformFontList *CreatePlatformFontList() {
        NS_NOTREACHED("oops, this platform doesn't have a gfxPlatformFontList implementation");
        return nullptr;
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
    { return nullptr; }

    







    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength);

    


    bool DownloadableFontsEnabled();

    






    virtual bool FontHintingEnabled() { return true; }

    













    virtual bool RequiresLinearZoom() { return false; }

    bool UsesSubpixelAATextRendering() {
#ifdef MOZ_GFX_OPTIMIZE_MOBILE
	return false;
#endif
	return true;
    }

    


    bool UseCmapsDuringSystemFallback();

    



    bool UseGraphiteShaping();

    




    bool UseHarfBuzzForScript(int32_t aScriptCode);

    
    virtual bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags) { return false; }

    void GetPrefFonts(nsIAtom *aLanguage, nsString& array, bool aAppendUnicode = true);

    
    void GetLangPrefs(eFontPrefLang aPrefLangs[], uint32_t &aLen, eFontPrefLang aCharLang, eFontPrefLang aPageLang);
    
    




    typedef bool (*PrefFontCallback) (eFontPrefLang aLang, const nsAString& aName,
                                        void *aClosure);
    static bool ForEachPrefFont(eFontPrefLang aLangArray[], uint32_t aLangArrayLen,
                                  PrefFontCallback aCallback,
                                  void *aClosure);

    
    static eFontPrefLang GetFontPrefLangFor(const char* aLang);

    
    static eFontPrefLang GetFontPrefLangFor(nsIAtom *aLang);

    
    static const char* GetPrefLangName(eFontPrefLang aLang);
   
    
    static eFontPrefLang GetFontPrefLangFor(uint8_t aUnicodeRange);

    
    static bool IsLangCJK(eFontPrefLang aLang);
    
    
    static void AppendPrefLang(eFontPrefLang aPrefLangs[], uint32_t& aLen, eFontPrefLang aAddLang);

    
    
    virtual void GetCommonFallbackFonts(const uint32_t ,
                                        int32_t ,
                                        nsTArray<const char*>& )
    {
        
    }

    
    static bool UseProgressiveTilePainting();

    
    
    static bool UseLowPrecisionBuffer();

    
    static float GetLowPrecisionResolution();

    
    
    static bool UseReusableTileStore();

    static bool OffMainThreadCompositingEnabled();

    


    static eCMSMode GetCMSMode();

    









    static int GetRenderingIntent();

    




    static void TransformPixel(const gfxRGBA& in, gfxRGBA& out, qcms_transform *transform);

    


    static qcms_profile* GetCMSOutputProfile();

    


    static qcms_profile* GetCMSsRGBProfile();

    


    static qcms_transform* GetCMSRGBTransform();

    


    static qcms_transform* GetCMSInverseRGBTransform();

    


    static qcms_transform* GetCMSRGBATransform();

    virtual void FontsPrefsChanged(const char *aPref);

    void OrientationSyncPrefsObserverChanged();

    int32_t GetBidiNumeralOption();

    



    gfxASurface* ScreenReferenceSurface() { return mScreenReferenceSurface; }

    virtual mozilla::gfx::SurfaceFormat Optimal2DFormatForContent(gfxASurface::gfxContentType aContent);

    virtual gfxImageFormat OptimalFormatForContent(gfxASurface::gfxContentType aContent);

    virtual gfxImageFormat GetOffscreenFormat()
    { return gfxASurface::ImageFormatRGB24; }

    


    static PRLogModuleInfo* GetLog(eGfxLog aWhichLog);

    bool WorkAroundDriverBugs() const { return mWorkAroundDriverBugs; }

    virtual int GetScreenDepth() const;

    bool WidgetUpdateFlashing() const { return mWidgetUpdateFlashing; }

    uint32_t GetOrientationSyncMillis() const;

protected:
    gfxPlatform();
    virtual ~gfxPlatform();

    void AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], uint32_t &aLen, 
                            eFontPrefLang aCharLang, eFontPrefLang aPageLang);

    



    mozilla::RefPtr<mozilla::gfx::DrawTarget>
      CreateDrawTargetForBackend(mozilla::gfx::BackendType aBackend,
                                 const mozilla::gfx::IntSize& aSize,
                                 mozilla::gfx::SurfaceFormat aFormat);

    





    void InitBackendPrefs(uint32_t aCanvasBitmask, uint32_t aContentBitmask);

    



    static mozilla::gfx::BackendType GetCanvasBackendPref(uint32_t aBackendBitmask);

    



    static mozilla::gfx::BackendType GetContentBackendPref(uint32_t aBackendBitmask);

    





    static mozilla::gfx::BackendType GetBackendPref(const char* aEnabledPrefName,
                                                    const char* aBackendPrefName,
                                                    uint32_t aBackendBitmask);
    


    static mozilla::gfx::BackendType BackendTypeForName(const nsCString& aName);

    mozilla::gfx::BackendType GetContentBackend() {
      return mContentBackend;
    }

    int8_t  mAllowDownloadableFonts;
    int8_t  mGraphiteShapingEnabled;

    int8_t  mBidiNumeralOption;

    
    
    int8_t  mFallbackUsesCmaps;

    
    int32_t mUseHarfBuzzScripts;

private:
    


    static void Init();

    virtual qcms_profile* GetPlatformCMSOutputProfile();

    nsRefPtr<gfxASurface> mScreenReferenceSurface;
    nsTArray<uint32_t> mCJKPrefLangs;
    nsCOMPtr<nsIObserver> mSRGBOverrideObserver;
    nsCOMPtr<nsIObserver> mFontPrefsObserver;
    nsCOMPtr<nsIObserver> mOrientationSyncPrefsObserver;

    
    mozilla::gfx::BackendType mPreferredCanvasBackend;
    
    mozilla::gfx::BackendType mFallbackCanvasBackend;
    
    mozilla::gfx::BackendType mContentBackend;

    mozilla::widget::GfxInfoCollector<gfxPlatform> mAzureCanvasBackendCollector;
    bool mWorkAroundDriverBugs;

    mozilla::RefPtr<mozilla::gfx::DrawEventRecorder> mRecorder;
    bool mWidgetUpdateFlashing;
    uint32_t mOrientationSyncMillis;
};

#endif 
