




#ifndef GFX_PLATFORM_H
#define GFX_PLATFORM_H

#include "prlog.h"
#include "mozilla/gfx/Types.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsAutoPtr.h"

#include "gfxPrefs.h"
#include "gfxTypes.h"
#include "gfxFontFamilyList.h"
#include "gfxBlur.h"
#include "nsRect.h"

#include "qcms.h"

#include "mozilla/RefPtr.h"
#include "GfxInfoCollector.h"

#include "mozilla/layers/CompositorTypes.h"

class gfxASurface;
class gfxFont;
class gfxFontGroup;
struct gfxFontStyle;
class gfxUserFontSet;
class gfxFontEntry;
class gfxPlatformFontList;
class gfxTextRun;
class nsIURI;
class nsIAtom;
class nsIObserver;
class SRGBOverrideObserver;

namespace mozilla {
namespace gl {
class SkiaGLGlue;
}
namespace gfx {
class DrawTarget;
class SourceSurface;
class DataSourceSurface;
class ScaledFont;
class DrawEventRecorder;
class VsyncSource;

inline uint32_t
BackendTypeBit(BackendType b)
{
  return 1 << uint8_t(b);
}
}
}

#define MOZ_PERFORMANCE_WARNING(module, ...) \
  do { \
    if (gfxPrefs::PerfWarnings()) { \
      printf_stderr("[" module "] " __VA_ARGS__); \
    } \
  } while (0)

extern cairo_user_data_key_t kDrawTarget;





enum eFontPrefLang {
    eFontPrefLang_Western     =  0,
    eFontPrefLang_Japanese    =  1,
    eFontPrefLang_ChineseTW   =  2,
    eFontPrefLang_ChineseCN   =  3,
    eFontPrefLang_ChineseHK   =  4,
    eFontPrefLang_Korean      =  5,
    eFontPrefLang_Cyrillic    =  6,
    eFontPrefLang_Greek       =  7,
    eFontPrefLang_Thai        =  8,
    eFontPrefLang_Hebrew      =  9,
    eFontPrefLang_Arabic      = 10,
    eFontPrefLang_Devanagari  = 11,
    eFontPrefLang_Tamil       = 12,
    eFontPrefLang_Armenian    = 13,
    eFontPrefLang_Bengali     = 14,
    eFontPrefLang_Canadian    = 15,
    eFontPrefLang_Ethiopic    = 16,
    eFontPrefLang_Georgian    = 17,
    eFontPrefLang_Gujarati    = 18,
    eFontPrefLang_Gurmukhi    = 19,
    eFontPrefLang_Khmer       = 20,
    eFontPrefLang_Malayalam   = 21,
    eFontPrefLang_Oriya       = 22,
    eFontPrefLang_Telugu      = 23,
    eFontPrefLang_Kannada     = 24,
    eFontPrefLang_Sinhala     = 25,
    eFontPrefLang_Tibetan     = 26,

    eFontPrefLang_Others      = 27, 

    eFontPrefLang_CJKSet      = 28  
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
    
    eGfxLog_cmapdata         = 4,
    
    eGfxLog_textperf         = 5
};


const uint32_t kMaxLenPrefLangList = 32;

#define UNINITIALIZED_VALUE  (-1)

inline const char*
GetBackendName(mozilla::gfx::BackendType aBackend)
{
  switch (aBackend) {
      case mozilla::gfx::BackendType::DIRECT2D:
        return "direct2d";
      case mozilla::gfx::BackendType::COREGRAPHICS_ACCELERATED:
        return "quartz accelerated";
      case mozilla::gfx::BackendType::COREGRAPHICS:
        return "quartz";
      case mozilla::gfx::BackendType::CAIRO:
        return "cairo";
      case mozilla::gfx::BackendType::SKIA:
        return "skia";
      case mozilla::gfx::BackendType::RECORDING:
        return "recording";
      case mozilla::gfx::BackendType::DIRECT2D1_1:
        return "direct2d 1.1";
      case mozilla::gfx::BackendType::NONE:
        return "none";
  }
  MOZ_CRASH("Incomplete switch");
}

enum class DeviceResetReason
{
  OK = 0,
  HUNG,
  REMOVED,
  RESET,
  DRIVER_ERROR,
  INVALID_CALL,
  OUT_OF_MEMORY,
  UNKNOWN
};

class gfxPlatform {
    friend class SRGBOverrideObserver;

public:
    typedef mozilla::gfx::Color Color;
    typedef mozilla::gfx::DataSourceSurface DataSourceSurface;
    typedef mozilla::gfx::DrawTarget DrawTarget;
    typedef mozilla::gfx::IntSize IntSize;
    typedef mozilla::gfx::SourceSurface SourceSurface;

    




    static gfxPlatform *GetPlatform();


    



    static void Shutdown();

    static void InitLayersIPC();
    static void ShutdownLayersIPC();

    



    virtual already_AddRefed<gfxASurface>
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) = 0;

    







    virtual mozilla::TemporaryRef<DrawTarget>
      CreateDrawTargetForSurface(gfxASurface *aSurface, const mozilla::gfx::IntSize& aSize);

    virtual mozilla::TemporaryRef<DrawTarget>
      CreateDrawTargetForUpdateSurface(gfxASurface *aSurface, const mozilla::gfx::IntSize& aSize);

    











    static mozilla::TemporaryRef<SourceSurface>
      GetSourceSurfaceForSurface(mozilla::gfx::DrawTarget *aTarget, gfxASurface *aSurface);

    static void ClearSourceSurfaceForSurface(gfxASurface *aSurface);

    static mozilla::TemporaryRef<DataSourceSurface>
        GetWrappedDataSourceSurface(gfxASurface *aSurface);

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    mozilla::TemporaryRef<DrawTarget>
      CreateOffscreenContentDrawTarget(const mozilla::gfx::IntSize& aSize, mozilla::gfx::SurfaceFormat aFormat);

    mozilla::TemporaryRef<DrawTarget>
      CreateOffscreenCanvasDrawTarget(const mozilla::gfx::IntSize& aSize, mozilla::gfx::SurfaceFormat aFormat);

    virtual mozilla::TemporaryRef<DrawTarget>
      CreateDrawTargetForData(unsigned char* aData, const mozilla::gfx::IntSize& aSize, 
                              int32_t aStride, mozilla::gfx::SurfaceFormat aFormat);

    





    virtual bool CanRenderContentToDataSurface() const {
      return false;
    }

    






    bool SupportsAzureContentForDrawTarget(mozilla::gfx::DrawTarget* aTarget);

    bool SupportsAzureContentForType(mozilla::gfx::BackendType aType) {
      return BackendTypeBit(aType) & mContentBackendBitmask;
    }

    
    
    
    
    
    
    virtual bool HaveChoiceOfHWAndSWCanvas();

    virtual bool UseAcceleratedSkiaCanvas();
    virtual void InitializeSkiaCacheLimits();

    
    
    virtual bool UseProgressivePaint() { return gfxPrefs::ProgressivePaintDoNotUseDirectly(); }

    void GetAzureBackendInfo(mozilla::widget::InfoObject &aObj) {
      aObj.DefineProperty("AzureCanvasBackend", GetBackendName(mPreferredCanvasBackend));
      aObj.DefineProperty("AzureSkiaAccelerated", UseAcceleratedSkiaCanvas());
      aObj.DefineProperty("AzureFallbackCanvasBackend", GetBackendName(mFallbackCanvasBackend));
      aObj.DefineProperty("AzureContentBackend", GetBackendName(mContentBackend));
    }
    void GetApzSupportInfo(mozilla::widget::InfoObject& aObj);

    mozilla::gfx::BackendType GetContentBackend() {
      return mContentBackend;
    }

    mozilla::gfx::BackendType GetPreferredCanvasBackend() {
      return mPreferredCanvasBackend;
    }

    



    virtual void SetupClusterBoundaries(gfxTextRun *aTextRun, const char16_t *aString);

    




    virtual nsresult GetFontList(nsIAtom *aLangGroup,
                                 const nsACString& aGenericFamily,
                                 nsTArray<nsString>& aListOfFonts);

    int GetTileWidth();
    int GetTileHeight();
    void SetTileSize(int aWidth, int aHeight);
    




    void ComputeTileSize();

    


    virtual nsresult UpdateFontList();

    




    virtual gfxPlatformFontList *CreatePlatformFontList() {
        NS_NOTREACHED("oops, this platform doesn't have a gfxPlatformFontList implementation");
        return nullptr;
    }

    



    virtual nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName) = 0;

    


    virtual gfxFontGroup
    *CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                     const gfxFontStyle *aStyle,
                     gfxUserFontSet *aUserFontSet) = 0;
                                          
                                          
    





    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic)
    { return nullptr; }

    







    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength);

    


    bool DownloadableFontsEnabled();

    






    virtual bool FontHintingEnabled() { return true; }

    













    virtual bool RequiresLinearZoom() { return false; }

    


    bool UseCmapsDuringSystemFallback();

    


    bool OpenTypeSVGEnabled();

    


    uint32_t WordCacheCharLimit();

    


    uint32_t WordCacheMaxEntries();

    



    bool UseGraphiteShaping();

    
    virtual bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags) { return false; }

    virtual bool DidRenderingDeviceReset(DeviceResetReason* aResetReason = nullptr) { return false; }

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

    
    
    virtual void GetCommonFallbackFonts(uint32_t , uint32_t ,
                                        int32_t ,
                                        nsTArray<const char*>& )
    {
        
    }

    
    static bool InSafeMode();

    static bool OffMainThreadCompositingEnabled();

    static bool CanUseDirect3D9();
    static bool CanUseDirect3D11();
    static bool CanUseHardwareVideoDecoding();
    static bool CanUseDirect3D11ANGLE();

    




    static bool BufferRotationEnabled();
    static void DisableBufferRotation();

    


    static eCMSMode GetCMSMode();

    









    static int GetRenderingIntent();

    




    static void TransformPixel(const Color& in, Color& out, qcms_transform *transform);

    


    static qcms_profile* GetCMSOutputProfile();

    


    static qcms_profile* GetCMSsRGBProfile();

    


    static qcms_transform* GetCMSRGBTransform();

    


    static qcms_transform* GetCMSInverseRGBTransform();

    


    static qcms_transform* GetCMSRGBATransform();

    virtual void FontsPrefsChanged(const char *aPref);

    int32_t GetBidiNumeralOption();

    



    gfxASurface* ScreenReferenceSurface() { return mScreenReferenceSurface; }
    mozilla::gfx::DrawTarget* ScreenReferenceDrawTarget() { return mScreenReferenceDrawTarget; }

    virtual mozilla::gfx::SurfaceFormat Optimal2DFormatForContent(gfxContentType aContent);

    virtual gfxImageFormat OptimalFormatForContent(gfxContentType aContent);

    virtual gfxImageFormat GetOffscreenFormat()
    { return gfxImageFormat::RGB24; }

    


    static PRLogModuleInfo* GetLog(eGfxLog aWhichLog);

    virtual int GetScreenDepth() const;

    


    mozilla::layers::DiagnosticTypes GetLayerDiagnosticTypes();

    static nsIntRect FrameCounterBounds() {
      int bits = 16;
      int sizeOfBit = 3;
      return nsIntRect(0, 0, bits * sizeOfBit, sizeOfBit);
    }

    mozilla::gl::SkiaGLGlue* GetSkiaGLGlue();
    void PurgeSkiaCache();

    virtual bool IsInGonkEmulator() const { return false; }

    static bool UsesOffMainThreadCompositing();

    bool HasEnoughTotalSystemMemoryForSkiaGL();

    



    virtual mozilla::gfx::VsyncSource* GetHardwareVsync() {
      MOZ_ASSERT(mVsyncSource != nullptr);
      MOZ_ASSERT(XRE_IsParentProcess());
      return mVsyncSource;
    }

    




    static bool IsInLayoutAsapMode();

    


    virtual bool SupportsApzWheelInput() const {
      return false;
    }
    virtual bool SupportsApzTouchInput() const {
      return false;
    }

protected:
    gfxPlatform();
    virtual ~gfxPlatform();

    void AppendCJKPrefLangs(eFontPrefLang aPrefLangs[], uint32_t &aLen,
                            eFontPrefLang aCharLang, eFontPrefLang aPageLang);

    


    virtual already_AddRefed<mozilla::gfx::VsyncSource> CreateHardwareVsyncSource();

    



    mozilla::TemporaryRef<DrawTarget>
      CreateDrawTargetForBackend(mozilla::gfx::BackendType aBackend,
                                 const mozilla::gfx::IntSize& aSize,
                                 mozilla::gfx::SurfaceFormat aFormat);

    





    void InitBackendPrefs(uint32_t aCanvasBitmask, mozilla::gfx::BackendType aCanvasDefault,
                          uint32_t aContentBitmask, mozilla::gfx::BackendType aContentDefault);

    



    static mozilla::gfx::BackendType GetCanvasBackendPref(uint32_t aBackendBitmask);

    



    static mozilla::gfx::BackendType GetContentBackendPref(uint32_t &aBackendBitmask);

    





    static mozilla::gfx::BackendType GetBackendPref(const char* aBackendPrefName,
                                                    uint32_t &aBackendBitmask);
    


    static mozilla::gfx::BackendType BackendTypeForName(const nsCString& aName);

    static mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
      GetScaledFontForFontWithCairoSkia(mozilla::gfx::DrawTarget* aTarget, gfxFont* aFont);

    int8_t  mAllowDownloadableFonts;
    int8_t  mGraphiteShapingEnabled;
    int8_t  mOpenTypeSVGEnabled;

    int8_t  mBidiNumeralOption;

    
    
    int8_t  mFallbackUsesCmaps;

    
    int32_t mWordCacheCharLimit;

    
    int32_t mWordCacheMaxEntries;

    uint32_t mTotalSystemMemory;

    
    nsRefPtr<mozilla::gfx::VsyncSource> mVsyncSource;

    mozilla::RefPtr<mozilla::gfx::DrawTarget> mScreenReferenceDrawTarget;

private:
    


    static void Init();

    static void CreateCMSOutputProfile();

    static void GetCMSOutputProfileData(void *&mem, size_t &size);

    friend void RecordingPrefChanged(const char *aPrefName, void *aClosure);

    virtual void GetPlatformCMSOutputProfile(void *&mem, size_t &size);

    nsRefPtr<gfxASurface> mScreenReferenceSurface;
    nsTArray<uint32_t> mCJKPrefLangs;
    nsCOMPtr<nsIObserver> mSRGBOverrideObserver;
    nsCOMPtr<nsIObserver> mFontPrefsObserver;
    nsCOMPtr<nsIObserver> mMemoryPressureObserver;

    
    mozilla::gfx::BackendType mPreferredCanvasBackend;
    
    mozilla::gfx::BackendType mFallbackCanvasBackend;
    
    mozilla::gfx::BackendType mContentBackend;
    
    uint32_t mContentBackendBitmask;

    int mTileWidth;
    int mTileHeight;

    mozilla::widget::GfxInfoCollector<gfxPlatform> mAzureCanvasBackendCollector;
    mozilla::widget::GfxInfoCollector<gfxPlatform> mApzSupportCollector;

    mozilla::RefPtr<mozilla::gfx::DrawEventRecorder> mRecorder;
    mozilla::RefPtr<mozilla::gl::SkiaGLGlue> mSkiaGlue;
};

#endif 
