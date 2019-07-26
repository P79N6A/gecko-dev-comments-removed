




#ifndef GFX_PLATFORM_QT_H
#define GFX_PLATFORM_QT_H

#include "gfxPlatform.h"
#include "nsAutoRef.h"
#include "nsDataHashtable.h"
#include "nsTArray.h"
#ifdef MOZ_X11
#include "X11/Xlib.h"
#endif

class gfxFontconfigUtils;
class QWidget;

class gfxQtPlatform : public gfxPlatform {
public:

    enum RenderMode {
        
        RENDER_QPAINTER = 0,
        
        RENDER_BUFFERED,
        
        RENDER_DIRECT,
        
        RENDER_MODE_MAX
    };

    gfxQtPlatform();
    virtual ~gfxQtPlatform();

    static gfxQtPlatform *GetPlatform() {
        return (gfxQtPlatform*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface> CreateOffscreenSurface(const gfxIntSize& size,
                                                         gfxContentType contentType);

    nsresult GetFontList(nsIAtom *aLangGroup,
                         const nsACString& aGenericFamily,
                         nsTArray<nsString>& aListOfFonts);

    nsresult UpdateFontList();

    nsresult ResolveFontName(const nsAString& aFontName,
                             FontResolverCallback aCallback,
                             void *aClosure, bool& aAborted);

    nsresult GetStandardFamilyName(const nsAString& aFontName, nsAString& aFamilyName);

    gfxFontGroup *CreateFontGroup(const nsAString &aFamilies,
                                  const gfxFontStyle *aStyle,
                                  gfxUserFontSet* aUserFontSet);

    



    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    



    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength);

    



    virtual bool IsFontFormatSupported(nsIURI *aFontURI,
                                         uint32_t aFormatFlags);

    void ClearPrefFonts() { mPrefFonts.Clear(); }

    RenderMode GetRenderMode() { return mRenderMode; }
    void SetRenderMode(RenderMode rmode) { mRenderMode = rmode; }

    static int32_t GetDPI();

    virtual gfxImageFormat GetOffscreenFormat();
#ifdef MOZ_X11
    static Display* GetXDisplay(QWidget* aWindow = 0);
    static Screen* GetXScreen(QWidget* aWindow = 0);
#endif

    virtual int GetScreenDepth() const;

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    virtual qcms_profile *GetPlatformCMSOutputProfile();

    
    nsDataHashtable<nsCStringHashKey, nsTArray<nsRefPtr<gfxFontEntry> > > mPrefFonts;

    RenderMode mRenderMode;
    int mScreenDepth;
};

#endif

