







































#ifndef GFX_FT2FONTLIST_H
#define GFX_FT2FONTLIST_H

#ifdef XP_WIN
#include "gfxWindowsPlatform.h"
#include <windows.h>
#endif
#include "gfxPlatformFontList.h"

namespace mozilla {
    namespace dom {
        class FontListEntry;
    };
};
using mozilla::dom::FontListEntry;

class FontNameCache;
typedef struct FT_FaceRec_* FT_Face;

class FT2FontEntry : public gfxFontEntry
{
public:
    FT2FontEntry(const nsAString& aFaceName) :
        gfxFontEntry(aFaceName)
    {
        mFTFace = nsnull;
        mFontFace = nsnull;
        mFTFontIndex = 0;
    }

    ~FT2FontEntry();

    const nsString& GetName() const {
        return Name();
    }

    
    static FT2FontEntry* 
    CreateFontEntry(const gfxProxyFontEntry &aProxyEntry,
                    const PRUint8 *aFontData, PRUint32 aLength);

    
    
    
    static FT2FontEntry*
    CreateFontEntry(const FontListEntry& aFLE);

    
    
    static FT2FontEntry* 
    CreateFontEntry(FT_Face aFace, const char *aFilename, PRUint8 aIndex,
                    const PRUint8 *aFontData = nsnull);
        
        

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle,
                                        bool aNeedsBold);

    cairo_font_face_t *CairoFontFace();
    cairo_scaled_font_t *CreateScaledFont(const gfxFontStyle *aStyle);

    nsresult ReadCMAP();
    nsresult GetFontTable(PRUint32 aTableTag, FallibleTArray<PRUint8>& aBuffer);

    FT_Face mFTFace;
    cairo_font_face_t *mFontFace;

    nsCString mFilename;
    PRUint8 mFTFontIndex;
};

class FT2FontFamily : public gfxFontFamily
{
public:
    FT2FontFamily(const nsAString& aName) :
        gfxFontFamily(aName) { }

    
    void AddFacesToFontList(InfallibleTArray<FontListEntry>* aFontList);
};

class gfxFT2FontList : public gfxPlatformFontList
{
public:
    gfxFT2FontList();

    virtual gfxFontEntry* GetDefaultFont(const gfxFontStyle* aStyle,
                                         bool& aNeedsBold);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const PRUint8 *aFontData,
                                           PRUint32 aLength);

    void GetFontList(InfallibleTArray<FontListEntry>* retValue);

    static gfxFT2FontList* PlatformFontList() {
        return static_cast<gfxFT2FontList*>(gfxPlatformFontList::PlatformFontList());
    }

protected:
    virtual nsresult InitFontList();

    void AppendFaceFromFontListEntry(const FontListEntry& aFLE,
                                     bool isStdFile);

    void AppendFacesFromFontFile(nsCString& aFileName,
                                 bool isStdFile = false,
                                 FontNameCache *aCache = nsnull);

    void AppendFacesFromCachedFaceList(nsCString& aFileName,
                                       bool isStdFile,
                                       nsCString& aFaceList);

    void FindFonts();
};

#endif 
