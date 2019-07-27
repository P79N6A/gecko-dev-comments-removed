




#ifndef GFX_DWRITEFONTLIST_H
#define GFX_DWRITEFONTLIST_H

#include "mozilla/MemoryReporting.h"
#include "gfxDWriteCommon.h"

#include "gfxFont.h"
#include "gfxUserFontSet.h"
#include "cairo-win32.h"

#include "gfxPlatformFontList.h"
#include "gfxPlatform.h"
#include <algorithm>







class gfxDWriteFontEntry;




class gfxDWriteFontFamily : public gfxFontFamily
{
public:
    






    gfxDWriteFontFamily(const nsAString& aName, 
                        IDWriteFontFamily *aFamily)
      : gfxFontFamily(aName), mDWFamily(aFamily), mForceGDIClassic(false) {}
    virtual ~gfxDWriteFontFamily();
    
    virtual void FindStyleVariations(FontInfoData *aFontInfoData = nullptr);

    virtual void LocalizedName(nsAString& aLocalizedName);

    virtual void ReadFaceNames(gfxPlatformFontList *aPlatformFontList,
                               bool aNeedFullnamePostscriptNames);

    void SetForceGDIClassic(bool aForce) { mForceGDIClassic = aForce; }

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

protected:
    
    nsRefPtr<IDWriteFontFamily> mDWFamily;
    bool mForceGDIClassic;
};




class gfxDWriteFontEntry : public gfxFontEntry
{
public:
    





    gfxDWriteFontEntry(const nsAString& aFaceName,
                              IDWriteFont *aFont) 
      : gfxFontEntry(aFaceName), mFont(aFont), mFontFile(nullptr),
        mForceGDIClassic(false)
    {
        mItalic = (aFont->GetStyle() == DWRITE_FONT_STYLE_ITALIC ||
                   aFont->GetStyle() == DWRITE_FONT_STYLE_OBLIQUE);
        mStretch = FontStretchFromDWriteStretch(aFont->GetStretch());
        uint16_t weight = NS_ROUNDUP(aFont->GetWeight() - 50, 100);

        weight = std::max<uint16_t>(100, weight);
        weight = std::min<uint16_t>(900, weight);
        mWeight = weight;

        mIsCJK = UNINITIALIZED_VALUE;
    }

    










    gfxDWriteFontEntry(const nsAString& aFaceName,
                              IDWriteFont *aFont,
                              uint16_t aWeight,
                              int16_t aStretch,
                              bool aItalic)
      : gfxFontEntry(aFaceName), mFont(aFont), mFontFile(nullptr),
        mForceGDIClassic(false)
    {
        mWeight = aWeight;
        mStretch = aStretch;
        mItalic = aItalic;
        mIsUserFont = true;
        mIsLocalUserFont = true;
        mIsCJK = UNINITIALIZED_VALUE;
    }

    








    gfxDWriteFontEntry(const nsAString& aFaceName,
                              IDWriteFontFile *aFontFile,
                              uint16_t aWeight,
                              int16_t aStretch,
                              bool aItalic)
      : gfxFontEntry(aFaceName), mFont(nullptr), mFontFile(aFontFile),
        mForceGDIClassic(false)
    {
        mWeight = aWeight;
        mStretch = aStretch;
        mItalic = aItalic;
        mIsUserFont = true;
        mIsCJK = UNINITIALIZED_VALUE;
    }

    virtual ~gfxDWriteFontEntry();

    virtual bool IsSymbolFont();

    virtual hb_blob_t* GetFontTable(uint32_t aTableTag) MOZ_OVERRIDE;

    nsresult ReadCMAP(FontInfoData *aFontInfoData = nullptr);

    bool IsCJKFont();

    void SetForceGDIClassic(bool aForce) { mForceGDIClassic = aForce; }
    bool GetForceGDIClassic() { return mForceGDIClassic; }

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

protected:
    friend class gfxDWriteFont;
    friend class gfxDWriteFontList;

    virtual nsresult CopyFontTable(uint32_t aTableTag,
                                   FallibleTArray<uint8_t>& aBuffer) MOZ_OVERRIDE;

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle,
                                        bool aNeedsBold);
    
    nsresult CreateFontFace(
        IDWriteFontFace **aFontFace,
        DWRITE_FONT_SIMULATIONS aSimulations = DWRITE_FONT_SIMULATIONS_NONE);

    static bool InitLogFont(IDWriteFont *aFont, LOGFONTW *aLogFont);

    



    nsRefPtr<IDWriteFont> mFont;
    nsRefPtr<IDWriteFontFile> mFontFile;

    
    
    nsRefPtr<IDWriteFontFace> mFontFace;

    DWRITE_FONT_FACE_TYPE mFaceType;

    int8_t mIsCJK;
    bool mForceGDIClassic;
};


class FontFallbackRenderer MOZ_FINAL : public IDWriteTextRenderer
{
public:
    FontFallbackRenderer(IDWriteFactory *aFactory)
        : mRefCount(0)
    {
        HRESULT hr = S_OK;

        hr = aFactory->GetSystemFontCollection(getter_AddRefs(mSystemFonts));
        NS_ASSERTION(SUCCEEDED(hr), "GetSystemFontCollection failed!");
    }

    ~FontFallbackRenderer()
    {}

    
    IFACEMETHOD(DrawGlyphRun)(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_MEASURING_MODE measuringMode,
        DWRITE_GLYPH_RUN const* glyphRun,
        DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
        IUnknown* clientDrawingEffect
        );

    IFACEMETHOD(DrawUnderline)(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_UNDERLINE const* underline,
        IUnknown* clientDrawingEffect
        )
    {
        return E_NOTIMPL;
    }


    IFACEMETHOD(DrawStrikethrough)(
        void* clientDrawingContext,
        FLOAT baselineOriginX,
        FLOAT baselineOriginY,
        DWRITE_STRIKETHROUGH const* strikethrough,
        IUnknown* clientDrawingEffect
        )
    {
        return E_NOTIMPL;
    }


    IFACEMETHOD(DrawInlineObject)(
        void* clientDrawingContext,
        FLOAT originX,
        FLOAT originY,
        IDWriteInlineObject* inlineObject,
        BOOL isSideways,
        BOOL isRightToLeft,
        IUnknown* clientDrawingEffect
        )
    {
        return E_NOTIMPL;
    }

    

    IFACEMETHOD(IsPixelSnappingDisabled)(
        void* clientDrawingContext,
        BOOL* isDisabled
        )
    {
        *isDisabled = FALSE;
        return S_OK;
    }

    IFACEMETHOD(GetCurrentTransform)(
        void* clientDrawingContext,
        DWRITE_MATRIX* transform
        )
    {
        const DWRITE_MATRIX ident = {1.0, 0.0, 0.0, 1.0, 0.0, 0.0};
        *transform = ident;
        return S_OK;
    }

    IFACEMETHOD(GetPixelsPerDip)(
        void* clientDrawingContext,
        FLOAT* pixelsPerDip
        )
    {
        *pixelsPerDip = 1.0f;
        return S_OK;
    }

    

    IFACEMETHOD_(unsigned long, AddRef) ()
    {
        return InterlockedIncrement(&mRefCount);
    }

    IFACEMETHOD_(unsigned long,  Release) ()
    {
        unsigned long newCount = InterlockedDecrement(&mRefCount);
        if (newCount == 0)
        {
            delete this;
            return 0;
        }

        return newCount;
    }

    IFACEMETHOD(QueryInterface) (IID const& riid, void** ppvObject)
    {
        if (__uuidof(IDWriteTextRenderer) == riid) {
            *ppvObject = this;
        } else if (__uuidof(IDWritePixelSnapping) == riid) {
            *ppvObject = this;
        } else if (__uuidof(IUnknown) == riid) {
            *ppvObject = this;
        } else {
            *ppvObject = nullptr;
            return E_FAIL;
        }

        this->AddRef();
        return S_OK;
    }

    const nsString& FallbackFamilyName() { return mFamilyName; }

protected:
    long mRefCount;
    nsRefPtr<IDWriteFontCollection> mSystemFonts;
    nsString mFamilyName;
};



class gfxDWriteFontList : public gfxPlatformFontList {
public:
    gfxDWriteFontList();

    static gfxDWriteFontList* PlatformFontList() {
        return static_cast<gfxDWriteFontList*>(sPlatformFontList);
    }

    
    virtual nsresult InitFontList();

    virtual gfxFontFamily* GetDefaultFont(const gfxFontStyle* aStyle);

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
    
    bool GetStandardFamilyName(const nsAString& aFontName,
                                 nsAString& aFamilyName);

    IDWriteGdiInterop *GetGDIInterop() { return mGDIInterop; }
    bool UseGDIFontTableAccess() { return mGDIFontTableAccess; }

    virtual gfxFontFamily* FindFamily(const nsAString& aFamily,
                                      bool aUseSystemFonts = false);

    virtual void GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray);

    gfxFloat GetForceGDIClassicMaxFontSize() { return mForceGDIClassicMaxFontSize; }

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

private:
    friend class gfxDWriteFontFamily;

    nsresult GetFontSubstitutes();

    void GetDirectWriteSubstitutes();

    
    virtual gfxFontEntry* GlobalFontFallback(const uint32_t aCh,
                                             int32_t aRunScript,
                                             const gfxFontStyle* aMatchStyle,
                                             uint32_t& aCmapCount,
                                             gfxFontFamily** aMatchedFamily);

    virtual bool UsesSystemFallback() { return true; }

    void GetFontsFromCollection(IDWriteFontCollection* aCollection);

#ifdef MOZ_BUNDLED_FONTS
    already_AddRefed<IDWriteFontCollection>
    CreateBundledFontsCollection(IDWriteFactory* aFactory);
#endif

    



    nsTArray<nsString> mNonExistingFonts;

    typedef nsRefPtrHashtable<nsStringHashKey, gfxFontFamily> FontTable;

    



    FontTable mFontSubstitutes;

    bool mInitialized;
    virtual nsresult DelayedInitFontList();

    virtual already_AddRefed<FontInfoData> CreateFontInfoData();

    gfxFloat mForceGDIClassicMaxFontSize;

    
    bool mGDIFontTableAccess;
    nsRefPtr<IDWriteGdiInterop> mGDIInterop;

    nsRefPtr<FontFallbackRenderer> mFallbackRenderer;
    nsRefPtr<IDWriteTextFormat>    mFallbackFormat;

    nsRefPtr<IDWriteFontCollection> mSystemFonts;
#ifdef MOZ_BUNDLED_FONTS
    nsRefPtr<IDWriteFontCollection> mBundledFonts;
#endif
};


#endif 
