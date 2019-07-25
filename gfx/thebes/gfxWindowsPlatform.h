





































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
#include "gfxContext.h"

#include "nsTArray.h"
#include "nsDataHashtable.h"

#include <windows.h>
#include <objbase.h>



struct DCFromContext {
    DCFromContext(gfxContext *aContext) {
        dc = NULL;
        nsRefPtr<gfxASurface> aSurface = aContext->CurrentSurface();
        NS_ASSERTION(aSurface, "DCFromContext: null surface");
        if (aSurface &&
            (aSurface->GetType() == gfxASurface::SurfaceTypeWin32 ||
             aSurface->GetType() == gfxASurface::SurfaceTypeWin32Printing))
        {
            dc = static_cast<gfxWindowsSurface*>(aSurface.get())->GetDC();
            needsRelease = PR_FALSE;
            SaveDC(dc);
            cairo_scaled_font_t* scaled =
                cairo_get_scaled_font(aContext->GetCairo());
            cairo_win32_scaled_font_select_font(scaled, dc);
        }
        if (!dc) {
            dc = GetDC(NULL);
            SetGraphicsMode(dc, GM_ADVANCED);
            needsRelease = PR_TRUE;
        }
    }

    ~DCFromContext() {
        if (needsRelease) {
            ReleaseDC(NULL, dc);
        } else {
            RestoreDC(dc, -1);
        }
    }

    operator HDC () {
        return dc;
    }

    HDC dc;
    PRBool needsRelease;
};


struct ClearTypeParameterInfo {
    ClearTypeParameterInfo() :
        gamma(-1), pixelStructure(-1), clearTypeLevel(-1), enhancedContrast(-1)
    { }

    nsString    displayName;  
    PRInt32     gamma;
    PRInt32     pixelStructure;
    PRInt32     clearTypeLevel;
    PRInt32     enhancedContrast;
};

class THEBES_API gfxWindowsPlatform : public gfxPlatform {
public:
    gfxWindowsPlatform();
    virtual ~gfxWindowsPlatform();
    static gfxWindowsPlatform *GetPlatform() {
        return (gfxWindowsPlatform*) gfxPlatform::GetPlatform();
    }

    virtual gfxPlatformFontList* CreatePlatformFontList();

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxASurface::gfxContentType contentType);
    virtual mozilla::RefPtr<mozilla::gfx::ScaledFont>
      GetScaledFontForFont(gfxFont *aFont);
    virtual already_AddRefed<gfxASurface>
      GetThebesSurfaceForDrawTarget(mozilla::gfx::DrawTarget *aTarget);

    enum RenderMode {
        
        RENDER_GDI = 0,

        
        RENDER_IMAGE_STRETCH32,

        
        RENDER_IMAGE_STRETCH24,

        
        RENDER_DIRECT2D,

        
        RENDER_MODE_MAX
    };

    RenderMode GetRenderMode() { return mRenderMode; }
    void SetRenderMode(RenderMode rmode) { mRenderMode = rmode; }

    



    void UpdateRenderMode();

    






    void VerifyD2DDevice(PRBool aAttemptForce);

    HDC GetScreenDC() { return mScreenDC; }

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, PRBool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet *aUserFontSet);

    


    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    


    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    


    virtual PRBool IsFontFormatSupported(nsIURI *aFontURI, PRUint32 aFormatFlags);

    
    gfxFontFamily *FindFontFamily(const nsAString& aName);
    gfxFontEntry *FindFontEntry(const nsAString& aName, const gfxFontStyle& aFontStyle);

    PRBool GetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<gfxFontEntry> > *array);
    void SetPrefFontEntries(const nsCString& aLangGroup, nsTArray<nsRefPtr<gfxFontEntry> >& array);

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    
    
    PRBool UseClearTypeForDownloadableFonts();
    PRBool UseClearTypeAlways();

    
    
    enum {
        kWindowsUnknown = 0,
        kWindows2000 = 0x50000,
        kWindowsXP = 0x50001,
        kWindowsServer2003 = 0x50002,
        kWindowsVista = 0x60000,
        kWindows7 = 0x60001
    };

    static PRInt32 WindowsOSVersion(PRInt32 *aBuildNum = nsnull);

    static void GetDLLVersion(const PRUnichar *aDLLPath, nsAString& aVersion);

    
    static void GetCleartypeParams(nsTArray<ClearTypeParameterInfo>& aParams);

    virtual void FontsPrefsChanged(const char *aPref);

    void SetupClearTypeParams();

#ifdef CAIRO_HAS_DWRITE_FONT
    IDWriteFactory *GetDWriteFactory() { return mDWriteFactory; }
    inline PRBool DWriteEnabled() { return mUseDirectWrite; }
    inline DWRITE_MEASURING_MODE DWriteMeasuringMode() { return mMeasuringMode; }
#else
    inline PRBool DWriteEnabled() { return PR_FALSE; }
#endif
#ifdef CAIRO_HAS_D2D_SURFACE
    cairo_device_t *GetD2DDevice() { return mD2DDevice; }
    ID3D10Device1 *GetD3D10Device() { return mD2DDevice ? cairo_d2d_device_get_device(mD2DDevice) : nsnull; }
#endif

    static bool IsOptimus();

protected:
    RenderMode mRenderMode;

    PRBool mUseClearTypeForDownloadableFonts;
    PRBool mUseClearTypeAlways;
    HDC mScreenDC;

private:
    void Init();

    PRBool mUseDirectWrite;
    PRBool mUsingGDIFonts;

#ifdef CAIRO_HAS_DWRITE_FONT
    nsRefPtr<IDWriteFactory> mDWriteFactory;
    DWRITE_MEASURING_MODE mMeasuringMode;
#endif
#ifdef CAIRO_HAS_D2D_SURFACE
    cairo_device_t *mD2DDevice;
#endif

    virtual qcms_profile* GetPlatformCMSOutputProfile();

    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<gfxFontEntry> > > mPrefFonts;
};

#endif 
