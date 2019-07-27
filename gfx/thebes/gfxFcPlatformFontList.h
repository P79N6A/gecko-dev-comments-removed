




#ifndef GFXFCPLATFORMFONTLIST_H_
#define GFXFCPLATFORMFONTLIST_H_

#include "gfxFont.h"
#include "gfxFontEntry.h"
#include "gfxFT2FontBase.h"
#include "gfxPlatformFontList.h"
#include "mozilla/mozalloc.h"

#include <fontconfig/fontconfig.h>
#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_TRUETYPE_TABLES_H
#include <cairo.h>
#include <cairo-ft.h>

#include "gfxFontconfigUtils.h" 

template <>
class nsAutoRefTraits<FcObjectSet> : public nsPointerRefTraits<FcObjectSet>
{
public:
    static void Release(FcObjectSet *ptr) { FcObjectSetDestroy(ptr); }
};

template <>
class nsAutoRefTraits<FcConfig> : public nsPointerRefTraits<FcConfig>
{
public:
    static void Release(FcConfig *ptr) { FcConfigDestroy(ptr); }
    static void AddRef(FcConfig *ptr) { FcConfigReference(ptr); }
};








class FTUserFontData {
public:
    NS_INLINE_DECL_REFCOUNTING(FTUserFontData)

    explicit FTUserFontData(FT_Face aFace, const uint8_t* aData)
        : mFace(aFace), mFontData(aData)
    {
    }

    const uint8_t *FontData() const { return mFontData; }

private:
    ~FTUserFontData()
    {
        FT_Done_Face(mFace);
        if (mFontData) {
            NS_Free((void*)mFontData);
        }
    }

    FT_Face        mFace;
    const uint8_t *mFontData;
};

class FTUserFontDataRef {
public:
    explicit FTUserFontDataRef(FTUserFontData *aUserFontData)
        : mUserFontData(aUserFontData)
    {
    }

    static void Destroy(void* aData) {
        FTUserFontDataRef* aUserFontDataRef =
            static_cast<FTUserFontDataRef*>(aData);
        delete aUserFontDataRef;
    }

private:
    nsRefPtr<FTUserFontData> mUserFontData;
};





class gfxFontconfigFontEntry : public gfxFontEntry {
public:
    
    explicit gfxFontconfigFontEntry(const nsAString& aFaceName,
                                    FcPattern* aFontPattern);

    
    
    explicit gfxFontconfigFontEntry(const nsAString& aFaceName,
                                    uint16_t aWeight,
                                    int16_t aStretch,
                                    bool aItalic,
                                    const uint8_t *aData,
                                    FT_Face aFace);

    
    explicit gfxFontconfigFontEntry(const nsAString& aFaceName,
                                    FcPattern* aFontPattern,
                                    uint16_t aWeight,
                                    int16_t aStretch,
                                    bool aItalic);

    FcPattern* GetPattern() { return mFontPattern; }

    bool SupportsLangGroup(nsIAtom *aLangGroup) const override;

    nsresult ReadCMAP(FontInfoData *aFontInfoData = nullptr) override;
    bool TestCharacterMap(uint32_t aCh) override;

    hb_blob_t* GetFontTable(uint32_t aTableTag) override;

    void ForgetHBFace() override;
    void ReleaseGrFace(gr_face* aFace) override;

protected:
    virtual ~gfxFontconfigFontEntry();

    gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle,
                                bool aNeedsBold) override;

    
    cairo_scaled_font_t*
    CreateScaledFont(FcPattern* aRenderPattern,
                     const gfxFontStyle *aStyle,
                     bool aNeedsBold);

    
    virtual nsresult
    CopyFontTable(uint32_t aTableTag,
                  FallibleTArray<uint8_t>& aBuffer) override;

    
    void MaybeReleaseFTFace();

    double GetAspect();

    
    nsCountedRef<FcPattern> mFontPattern;

    
    nsRefPtr<FTUserFontData> mUserFontData;

    
    FT_Face   mFTFace;
    bool      mFTFaceInitialized;
    double    mAspect;

    
    const uint8_t* mFontData;
};

class gfxFontconfigFontFamily : public gfxFontFamily {
public:
    explicit gfxFontconfigFontFamily(const nsAString& aName) :
        gfxFontFamily(aName) { }

    void FindStyleVariations(FontInfoData *aFontInfoData = nullptr) override;

    
    
    void AddFontPattern(FcPattern* aFontPattern);

protected:
    virtual ~gfxFontconfigFontFamily() { }

    nsTArray<nsCountedRef<FcPattern> > mFontPatterns;
};

class gfxFontconfigFont : public gfxFT2FontBase {
public:
    gfxFontconfigFont(cairo_scaled_font_t *aScaledFont,
                      gfxFontEntry *aFontEntry,
                      const gfxFontStyle *aFontStyle,
                      bool aNeedsBold);

#ifdef USE_SKIA
    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
    GetGlyphRenderingOptions(const TextRunDrawParams* aRunParams = nullptr) override;
#endif

protected:
    virtual ~gfxFontconfigFont();
};

class gfxFcPlatformFontList : public gfxPlatformFontList {
public:
    gfxFcPlatformFontList();

    
    nsresult InitFontList() override;

    void GetFontList(nsIAtom *aLangGroup,
                     const nsACString& aGenericFamily,
                     nsTArray<nsString>& aListOfFonts) override;


    gfxFontFamily*
    GetDefaultFont(const gfxFontStyle* aStyle) override;

    gfxFontEntry*
    LookupLocalFont(const nsAString& aFontName, uint16_t aWeight,
                    int16_t aStretch, bool aItalic) override;

    gfxFontEntry*
    MakePlatformFont(const nsAString& aFontName, uint16_t aWeight,
                     int16_t aStretch, bool aItalic,
                     const uint8_t* aFontData,
                     uint32_t aLength) override;

    gfxFontFamily* FindFamily(const nsAString& aFamily,
                              nsIAtom* aLanguage = nullptr,
                              bool aUseSystemFonts = false) override;

    bool GetStandardFamilyName(const nsAString& aFontName,
                               nsAString& aFamilyName) override;

    FcConfig* GetLastConfig() const { return mLastConfig; }

    static FT_Library GetFTLibrary();

protected:
    virtual ~gfxFcPlatformFontList();

    
    void AddFontSetFamilies(FcFontSet* aFontSet);

    
    
    gfxFontFamily* FindGenericFamily(const nsAString& aGeneric,
                                     nsIAtom* aLanguage);

    static void CheckFontUpdates(nsITimer *aTimer, void *aThis);

#ifdef MOZ_BUNDLED_FONTS
    void ActivateBundledFonts();
    nsCString mBundledFontsPath;
    bool mBundledFontsInitialized;
#endif

    
    
    nsBaseHashtable<nsStringHashKey,
                    nsCountedRef<FcPattern>,
                    FcPattern*> mLocalNames;

    
    nsRefPtrHashtable<nsCStringHashKey, gfxFontFamily> mGenericMappings;

    
    nsRefPtrHashtable<nsCStringHashKey, gfxFontFamily> mFcSubstituteCache;

    nsCOMPtr<nsITimer> mCheckFontUpdatesTimer;
    nsCountedRef<FcConfig> mLastConfig;

    static FT_Library sCairoFTLibrary;
};

#endif 
