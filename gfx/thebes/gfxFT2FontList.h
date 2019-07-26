




#ifndef GFX_FT2FONTLIST_H
#define GFX_FT2FONTLIST_H

#include "mozilla/MemoryReporting.h"

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
class nsZipArchive;

class FT2FontEntry : public gfxFontEntry
{
public:
    FT2FontEntry(const nsAString& aFaceName) :
        gfxFontEntry(aFaceName),
        mFTFace(nullptr),
        mFontFace(nullptr),
        mFTFontIndex(0)
    {
    }

    ~FT2FontEntry();

    const nsString& GetName() const {
        return Name();
    }

    
    static FT2FontEntry* 
    CreateFontEntry(const gfxProxyFontEntry &aProxyEntry,
                    const uint8_t *aFontData, uint32_t aLength);

    
    
    
    static FT2FontEntry*
    CreateFontEntry(const FontListEntry& aFLE);

    
    
    
    
    static FT2FontEntry* 
    CreateFontEntry(FT_Face aFace,
                    const char *aFilename, uint8_t aIndex,
                    const nsAString& aName,
                    const uint8_t *aFontData = nullptr);

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle,
                                        bool aNeedsBold);

    
    
    cairo_font_face_t *CairoFontFace();

    
    
    cairo_scaled_font_t *CreateScaledFont(const gfxFontStyle *aStyle);

    nsresult ReadCMAP(FontInfoData *aFontInfoData = nullptr);

    virtual hb_blob_t* GetFontTable(uint32_t aTableTag) MOZ_OVERRIDE;

    virtual nsresult CopyFontTable(uint32_t aTableTag,
                                   FallibleTArray<uint8_t>& aBuffer) MOZ_OVERRIDE;

    
    
    void CheckForBrokenFont(gfxFontFamily *aFamily);

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

    FT_Face mFTFace;
    cairo_font_face_t *mFontFace;

    nsCString mFilename;
    uint8_t   mFTFontIndex;
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

    virtual gfxFontFamily* GetDefaultFont(const gfxFontStyle* aStyle);

    virtual gfxFontEntry* LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                          const nsAString& aFontName);

    virtual gfxFontEntry* MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                           const uint8_t *aFontData,
                                           uint32_t aLength);

    void GetFontList(InfallibleTArray<FontListEntry>* retValue);

    static gfxFT2FontList* PlatformFontList() {
        return static_cast<gfxFT2FontList*>(gfxPlatformFontList::PlatformFontList());
    }

protected:
    virtual nsresult InitFontList();

    void AppendFaceFromFontListEntry(const FontListEntry& aFLE,
                                     bool isStdFile);

    void AppendFacesFromFontFile(const nsCString& aFileName,
                                 bool isStdFile = false,
                                 FontNameCache *aCache = nullptr);

    void AppendFacesFromOmnijarEntry(nsZipArchive *aReader,
                                     const nsCString& aEntryName,
                                     FontNameCache *aCache,
                                     bool aJarChanged);

    void AppendFacesFromCachedFaceList(const nsCString& aFileName,
                                       bool isStdFile,
                                       const nsCString& aFaceList);

    void AddFaceToList(const nsCString& aEntryName, uint32_t aIndex,
                       bool aStdFile, FT_Face aFace, nsCString& aFaceList);

    void FindFonts();

    void FindFontsInOmnijar(FontNameCache *aCache);

#ifdef ANDROID
    void FindFontsInDir(const nsCString& aDir, FontNameCache* aFNC);
#endif

    nsTHashtable<nsStringHashKey> mSkipSpaceLookupCheckFamilies;
};

#endif 
