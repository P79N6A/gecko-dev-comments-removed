




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

class nsIMemoryReporter;






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
      CreateOffscreenSurface(const IntSize& size,
                             gfxContentType contentType) override;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont>
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
#ifdef CAIRO_HAS_D2D_SURFACE
    cairo_device_t *GetD2DDevice() { return mD2DDevice; }
    ID3D10Device1 *GetD3D10Device() { return mD2DDevice ? cairo_d2d_device_get_device(mD2DDevice) : nullptr; }
#endif
    ID3D11Device *GetD3D11Device();
    ID3D11Device *GetD3D11ContentDevice();
    ID3D11Device *GetD3D11MediaDevice();

    
    mozilla::TemporaryRef<ID3D11Device> CreateD3D11DecoderDevice();

    mozilla::layers::ReadbackManagerD3D11* GetReadbackManager();

    static bool IsOptimus();

    bool IsWARP() { return mIsWARP; }

    bool SupportsApzWheelInput() override {
      return true;
    }
    bool SupportsApzTouchInput() override;

    virtual already_AddRefed<mozilla::gfx::VsyncSource> CreateHardwareVsyncSource() override;
    static mozilla::Atomic<size_t> sD3D11MemoryUsed;
    static mozilla::Atomic<size_t> sD3D9MemoryUsed;
    static mozilla::Atomic<size_t> sD3D9SurfaceImageUsed;
    static mozilla::Atomic<size_t> sD3D9SharedTextureUsed;

protected:
    RenderMode mRenderMode;

    int8_t mUseClearTypeForDownloadableFonts;
    int8_t mUseClearTypeAlways;

private:
    void Init();
    void InitD3D11Devices();
    IDXGIAdapter1 *GetDXGIAdapter();

    bool mUseDirectWrite;
    bool mUsingGDIFonts;

#ifdef CAIRO_HAS_DWRITE_FONT
    nsRefPtr<IDWriteFactory> mDWriteFactory;
    nsRefPtr<IDWriteTextAnalyzer> mDWriteAnalyzer;
    nsRefPtr<IDWriteRenderingParams> mRenderingParams[TEXT_RENDERING_COUNT];
    DWRITE_MEASURING_MODE mMeasuringMode;
#endif
#ifdef CAIRO_HAS_D2D_SURFACE
    cairo_device_t *mD2DDevice;
#endif
    mozilla::RefPtr<IDXGIAdapter1> mAdapter;
    nsRefPtr<mozilla::layers::DeviceManagerD3D9> mDeviceManager;
    mozilla::RefPtr<ID3D11Device> mD3D11Device;
    mozilla::RefPtr<ID3D11Device> mD3D11ContentDevice;
    mozilla::RefPtr<ID3D11Device> mD3D11MediaDevice;
    bool mD3D11DeviceInitialized;
    mozilla::RefPtr<mozilla::layers::ReadbackManagerD3D11> mD3D11ReadbackManager;
    bool mIsWARP;
    bool mCanInitMediaDevice;

    virtual void GetPlatformCMSOutputProfile(void* &mem, size_t &size);
};

bool DoesD3D11TextureSharingWork(ID3D11Device *device);
bool DoesD3D11DeviceWork(ID3D11Device *device);

#endif
