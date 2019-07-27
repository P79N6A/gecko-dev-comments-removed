




#ifndef GFX_WINDOWS_PLATFORM_H
#define GFX_WINDOWS_PLATFORM_H






#include "cairo-win32.h"

#include "gfxFontUtils.h"
#include "gfxWindowsSurface.h"
#include "gfxFont.h"
#ifdef CAIRO_HAS_DWRITE_FONT
#include "gfxDWriteFonts.h"
#endif
#include "gfxPlatform.h"
#include "gfxTelemetry.h"
#include "gfxTypes.h"
#include "mozilla/Attributes.h"
#include "mozilla/Atomics.h"
#include "nsTArray.h"
#include "nsDataHashtable.h"

#include "mozilla/RefPtr.h"

#include <windows.h>
#include <objbase.h>

#ifdef CAIRO_HAS_D2D_SURFACE
#include <dxgi.h>
#endif


#include <d3dcommon.h>

#if !defined(D3D_FEATURE_LEVEL_11_1) 
#define D3D_FEATURE_LEVEL_11_1 static_cast<D3D_FEATURE_LEVEL>(0xb100)
#define D3D_FL9_1_REQ_TEXTURE2D_U_OR_V_DIMENSION 2048
#define D3D_FL9_3_REQ_TEXTURE2D_U_OR_V_DIMENSION 4096
#endif

namespace mozilla {
namespace gfx {
class DrawTarget;
}
namespace layers {
class DeviceManagerD3D9;
class ReadbackManagerD3D11;
}
}
struct IDirect3DDevice9;
struct ID3D11Device;
struct IDXGIAdapter1;






class MOZ_STACK_CLASS DCFromDrawTarget final
{
public:
    DCFromDrawTarget(mozilla::gfx::DrawTarget& aDrawTarget);

    ~DCFromDrawTarget() {
        if (mNeedsRelease) {
            ReleaseDC(nullptr, mDC);
        } else {
            RestoreDC(mDC, -1);
        }
    }

    operator HDC () {
        return mDC;
    }

private:
    HDC mDC;
    bool mNeedsRelease;
};


struct ClearTypeParameterInfo {
    ClearTypeParameterInfo() :
        gamma(-1), pixelStructure(-1), clearTypeLevel(-1), enhancedContrast(-1)
    { }

    nsString    displayName;  
    int32_t     gamma;
    int32_t     pixelStructure;
    int32_t     clearTypeLevel;
    int32_t     enhancedContrast;
};

class gfxWindowsPlatform : public gfxPlatform {
public:
    enum TextRenderingMode {
        TEXT_RENDERING_NO_CLEARTYPE,
        TEXT_RENDERING_NORMAL,
        TEXT_RENDERING_GDI_CLASSIC,
        TEXT_RENDERING_COUNT
    };

    gfxWindowsPlatform();
    virtual ~gfxWindowsPlatform();
    static gfxWindowsPlatform *GetPlatform() {
        return (gfxWindowsPlatform*) gfxPlatform::GetPlatform();
    }

    virtual gfxPlatformFontList* CreatePlatformFontList();

    virtual already_AddRefed<gfxASurface>
      CreateOffscreenSurface(const IntSize& aSize,
                             gfxImageFormat aFormat) override;

    virtual already_AddRefed<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(mozilla::gfx::DrawTarget* aTarget, gfxFont *aFont);

    enum RenderMode {
        
        RENDER_GDI = 0,

        
        RENDER_IMAGE_STRETCH32,

        
        RENDER_IMAGE_STRETCH24,

        
        RENDER_DIRECT2D,

        
        RENDER_MODE_MAX
    };

    int GetScreenDepth() const;

    RenderMode GetRenderMode() { return mRenderMode; }
    void SetRenderMode(RenderMode rmode) { mRenderMode = rmode; }

    



    void UpdateRenderMode();

    






    void VerifyD2DDevice(bool aAttemptForce);

#ifdef CAIRO_HAS_D2D_SURFACE
    HRESULT CreateDevice(nsRefPtr<IDXGIAdapter1> &adapter1, int featureLevelIndex);
#endif

    




    double GetDPIScale();

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    virtual void GetCommonFallbackFonts(uint32_t aCh, uint32_t aNextCh,
                                        int32_t aRunScript,
                                        nsTArray<const char*>& aFontList);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet *aUserFontSet);

    


    virtual gfxFontEntry* LookupLocalFont(const nsAString& aFontName,
                                          uint16_t aWeight,
                                          int16_t aStretch,
                                          bool aItalic);

    


    virtual gfxFontEntry* MakePlatformFont(const nsAString& aFontName,
                                           uint16_t aWeight,
                                           int16_t aStretch,
                                           bool aItalic,
                                           const uint8_t* aFontData,
                                           uint32_t aLength);

    virtual bool CanUseHardwareVideoDecoding() override;

    


    virtual bool IsFontFormatSupported(nsIURI *aFontURI, uint32_t aFormatFlags);

    virtual bool DidRenderingDeviceReset(DeviceResetReason* aResetReason = nullptr);

    
    
    bool UseClearTypeForDownloadableFonts();
    bool UseClearTypeAlways();

    static void GetDLLVersion(char16ptr_t aDLLPath, nsAString& aVersion);

    
    static void GetCleartypeParams(nsTArray<ClearTypeParameterInfo>& aParams);

    virtual void FontsPrefsChanged(const char *aPref);

    void SetupClearTypeParams();

#ifdef CAIRO_HAS_DWRITE_FONT
    IDWriteFactory *GetDWriteFactory() { return mDWriteFactory; }
    inline bool DWriteEnabled() { return mUseDirectWrite; }
    inline DWRITE_MEASURING_MODE DWriteMeasuringMode() { return mMeasuringMode; }
    IDWriteTextAnalyzer *GetDWriteAnalyzer() { return mDWriteAnalyzer; }

    IDWriteRenderingParams *GetRenderingParams(TextRenderingMode aRenderMode)
    { return mRenderingParams[aRenderMode]; }
#else
    inline bool DWriteEnabled() { return false; }
#endif
    void OnDeviceManagerDestroy(mozilla::layers::DeviceManagerD3D9* aDeviceManager);
    mozilla::layers::DeviceManagerD3D9* GetD3D9DeviceManager();
    IDirect3DDevice9* GetD3D9Device();
    ID3D10Device1 *GetD3D10Device() { return mD3D10Device; }
    ID3D11Device *GetD3D11Device();
    ID3D11Device *GetD3D11ContentDevice();
    
    ID3D11Device *GetD3D11ImageBridgeDevice();

    
    already_AddRefed<ID3D11Device> CreateD3D11DecoderDevice();

    mozilla::layers::ReadbackManagerD3D11* GetReadbackManager();

    static bool IsOptimus();

    bool IsWARP() { return mIsWARP; }
    bool DoesD3D11TextureSharingWork() { return mDoesD3D11TextureSharingWork; }

    bool SupportsApzWheelInput() const override {
      return true;
    }
    bool SupportsApzTouchInput() const override;

    
    
    
    mozilla::gfx::FeatureStatus GetD3D11Status() const {
      return mD3D11Status;
    }
    mozilla::gfx::FeatureStatus GetD2DStatus() const {
      return mD2DStatus;
    }
    unsigned GetD3D11Version();
    mozilla::gfx::FeatureStatus GetD2D1Status();

    virtual already_AddRefed<mozilla::gfx::VsyncSource> CreateHardwareVsyncSource() override;
    static mozilla::Atomic<size_t> sD3D11MemoryUsed;
    static mozilla::Atomic<size_t> sD3D9MemoryUsed;
    static mozilla::Atomic<size_t> sD3D9SurfaceImageUsed;
    static mozilla::Atomic<size_t> sD3D9SharedTextureUsed;

protected:
    bool AccelerateLayersByDefault() override {
      return true;
    }
    void GetAcceleratedCompositorBackends(nsTArray<mozilla::layers::LayersBackend>& aBackends);

protected:
    RenderMode mRenderMode;

    int8_t mUseClearTypeForDownloadableFonts;
    int8_t mUseClearTypeAlways;

private:
    void Init();

    void InitD3D11Devices();

    
    enum class D3D11Status {
      Ok,
      TryWARP,
      ForceWARP,
      Blocked
    };
    D3D11Status CheckD3D11Support();
    bool AttemptD3D11DeviceCreation(const nsTArray<D3D_FEATURE_LEVEL>& aFeatureLevels);
    bool AttemptWARPDeviceCreation(const nsTArray<D3D_FEATURE_LEVEL>& aFeatureLevels);
    bool AttemptD3D11ImageBridgeDeviceCreation(const nsTArray<D3D_FEATURE_LEVEL>& aFeatureLevels);
    bool AttemptD3D11ContentDeviceCreation(const nsTArray<D3D_FEATURE_LEVEL>& aFeatureLevels);

    
    mozilla::gfx::FeatureStatus InitD2DSupport();
    void InitDWriteSupport();

    IDXGIAdapter1 *GetDXGIAdapter();
    bool IsDeviceReset(HRESULT hr, DeviceResetReason* aReason);

    bool mUseDirectWrite;
    bool mUsingGDIFonts;

#ifdef CAIRO_HAS_DWRITE_FONT
    nsRefPtr<IDWriteFactory> mDWriteFactory;
    nsRefPtr<IDWriteTextAnalyzer> mDWriteAnalyzer;
    nsRefPtr<IDWriteRenderingParams> mRenderingParams[TEXT_RENDERING_COUNT];
    DWRITE_MEASURING_MODE mMeasuringMode;
#endif
    mozilla::RefPtr<IDXGIAdapter1> mAdapter;
    nsRefPtr<mozilla::layers::DeviceManagerD3D9> mDeviceManager;
    mozilla::RefPtr<ID3D10Device1> mD3D10Device;
    mozilla::RefPtr<ID3D11Device> mD3D11Device;
    mozilla::RefPtr<ID3D11Device> mD3D11ContentDevice;
    mozilla::RefPtr<ID3D11Device> mD3D11ImageBridgeDevice;
    bool mD3D11DeviceInitialized;
    mozilla::RefPtr<mozilla::layers::ReadbackManagerD3D11> mD3D11ReadbackManager;
    bool mIsWARP;
    bool mHasDeviceReset;
    bool mDoesD3D11TextureSharingWork;
    DeviceResetReason mDeviceResetReason;

    mozilla::gfx::FeatureStatus mD3D11Status;
    mozilla::gfx::FeatureStatus mD2DStatus;

    virtual void GetPlatformCMSOutputProfile(void* &mem, size_t &size);
};

#endif
