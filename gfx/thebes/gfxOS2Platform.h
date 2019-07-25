




































#ifndef GFX_OS2_PLATFORM_H
#define GFX_OS2_PLATFORM_H

#define INCL_GPIBITMAPS
#include <os2.h>

#include "gfxPlatform.h"
#include "gfxOS2Fonts.h"
#include "gfxFontUtils.h"
#include "nsTArray.h"

class gfxFontconfigUtils;

class THEBES_API gfxOS2Platform : public gfxPlatform {

public:
    gfxOS2Platform();
    virtual ~gfxOS2Platform();

    static gfxOS2Platform *GetPlatform() {
        return (gfxOS2Platform*) gfxPlatform::GetPlatform();
    }

    already_AddRefed<gfxASurface>
        CreateOffscreenSurface(const gfxIntSize& size,
                               gfxASurface::gfxContentType contentType);

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
                                  gfxUserFontSet *aUserFontSet);

    
    
    
    
    already_AddRefed<gfxOS2Font> FindFontForChar(PRUint32 aCh, gfxOS2Font *aFont);

    
    bool noFontWithChar(PRUint32 aCh) {
        return mCodepointsWithNoFonts.test(aCh);
    }

protected:
    static gfxFontconfigUtils *sFontconfigUtils;

private:
    
    gfxSparseBitSet mCodepointsWithNoFonts;
};

#endif 
