




#ifndef GFX_FONT_H
#define GFX_FONT_H

#include "gfxTypes.h"
#include "nsString.h"
#include "gfxPoint.h"
#include "gfxFontFamilyList.h"
#include "gfxFontUtils.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "gfxSkipChars.h"
#include "gfxRect.h"
#include "nsExpirationTracker.h"
#include "gfxPlatform.h"
#include "nsIAtom.h"
#include "mozilla/HashFunctions.h"
#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "gfxFontFeatures.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Attributes.h"
#include <algorithm>
#include "DrawMode.h"
#include "nsUnicodeScriptCodes.h"
#include "nsDataHashtable.h"
#include "harfbuzz/hb.h"
#include "mozilla/gfx/2D.h"

typedef struct _cairo_scaled_font cairo_scaled_font_t;
typedef struct gr_face            gr_face;

#ifdef DEBUG
#include <stdio.h>
#endif

class gfxContext;
class gfxTextRun;
class gfxFont;
class gfxFontFamily;
class gfxFontGroup;
class gfxGraphiteShaper;
class gfxHarfBuzzShaper;
class gfxUserFontSet;
class gfxUserFontData;
class gfxShapedText;
class gfxShapedWord;
class gfxSVGGlyphs;
class gfxMathTable;
class gfxTextContextPaint;
class FontInfoData;

class nsILanguageAtomService;

#define FONT_MAX_SIZE                  2000.0

#define NO_FONT_LANGUAGE_OVERRIDE      0

#define SMALL_CAPS_SCALE_FACTOR        0.8

struct FontListSizes;
struct gfxTextRunDrawCallbacks;

namespace mozilla {
namespace gfx {
class GlyphRenderingOptions;
}
}

struct gfxFontStyle {
    gfxFontStyle();
    gfxFontStyle(uint8_t aStyle, uint16_t aWeight, int16_t aStretch,
                 gfxFloat aSize, nsIAtom *aLanguage,
                 float aSizeAdjust, bool aSystemFont,
                 bool aPrinterFont,
                 bool aWeightSynthesis, bool aStyleSynthesis,
                 const nsString& aLanguageOverride);
    gfxFontStyle(const gfxFontStyle& aStyle);

    
    
    
    nsRefPtr<nsIAtom> language;

    
    
    

    
    nsTArray<gfxFontFeature> featureSettings;

    
    
    

    
    nsTArray<gfxAlternateValue> alternateValues;

    
    nsRefPtr<gfxFontFeatureValueSet> featureValueLookup;

    
    gfxFloat size;

    
    
    
    
    float sizeAdjust;

    
    float baselineOffset;

    
    
    
    
    
    
    
    
    
    
    uint32_t languageOverride;

    
    uint16_t weight;

    
    
    int8_t stretch;

    
    
    
    bool systemFont : 1;

    
    bool printerFont : 1;

    
    bool useGrayscaleAntialiasing : 1;

    
    uint8_t style : 2;

    
    bool allowSyntheticWeight : 1;
    bool allowSyntheticStyle : 1;

    
    
    bool noFallbackVariantFeatures : 1;

    
    uint8_t variantCaps;

    
    uint8_t variantSubSuper;

    
    
    gfxFloat GetAdjustedSize(gfxFloat aspect) const {
        NS_ASSERTION(sizeAdjust != 0.0, "Not meant to be called when sizeAdjust = 0");
        gfxFloat adjustedSize = std::max(NS_round(size*(sizeAdjust/aspect)), 1.0);
        return std::min(adjustedSize, FONT_MAX_SIZE);
    }

    PLDHashNumber Hash() const {
        return ((style + (systemFont << 7) +
            (weight << 8)) + uint32_t(size*1000) + uint32_t(sizeAdjust*1000)) ^
            nsISupportsHashKey::HashKey(language);
    }

    int8_t ComputeWeight() const;

    
    
    void AdjustForSubSuperscript(int32_t aAppUnitsPerDevPixel);

    bool Equals(const gfxFontStyle& other) const {
        return
            (*reinterpret_cast<const uint64_t*>(&size) ==
             *reinterpret_cast<const uint64_t*>(&other.size)) &&
            (style == other.style) &&
            (variantCaps == other.variantCaps) &&
            (variantSubSuper == other.variantSubSuper) &&
            (allowSyntheticWeight == other.allowSyntheticWeight) &&
            (allowSyntheticStyle == other.allowSyntheticStyle) &&
            (systemFont == other.systemFont) &&
            (printerFont == other.printerFont) &&
            (useGrayscaleAntialiasing == other.useGrayscaleAntialiasing) &&
            (weight == other.weight) &&
            (stretch == other.stretch) &&
            (language == other.language) &&
            (baselineOffset == other.baselineOffset) &&
            (*reinterpret_cast<const uint32_t*>(&sizeAdjust) ==
             *reinterpret_cast<const uint32_t*>(&other.sizeAdjust)) &&
            (featureSettings == other.featureSettings) &&
            (languageOverride == other.languageOverride) &&
            (alternateValues == other.alternateValues) &&
            (featureValueLookup == other.featureValueLookup);
    }

    static void ParseFontFeatureSettings(const nsString& aFeatureString,
                                         nsTArray<gfxFontFeature>& aFeatures);

    static uint32_t ParseFontLanguageOverride(const nsString& aLangTag);
};

class gfxCharacterMap : public gfxSparseBitSet {
public:
    nsrefcnt AddRef() {
        NS_PRECONDITION(int32_t(mRefCnt) >= 0, "illegal refcnt");
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "gfxCharacterMap", sizeof(*this));
        return mRefCnt;
    }

    nsrefcnt Release() {
        NS_PRECONDITION(0 != mRefCnt, "dup release");
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, "gfxCharacterMap");
        if (mRefCnt == 0) {
            NotifyReleased();
            
            return 0;
        }
        return mRefCnt;
    }

    gfxCharacterMap() :
        mHash(0), mBuildOnTheFly(false), mShared(false)
    { }

    void CalcHash() { mHash = GetChecksum(); }

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const {
        return gfxSparseBitSet::SizeOfExcludingThis(aMallocSizeOf);
    }

    
    uint32_t mHash;

    
    bool mBuildOnTheFly;

    
    bool mShared;

protected:
    void NotifyReleased();

    nsAutoRefCnt mRefCnt;

private:
    gfxCharacterMap(const gfxCharacterMap&);
    gfxCharacterMap& operator=(const gfxCharacterMap&);
};

class gfxFontEntry {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFontEntry)

    explicit gfxFontEntry(const nsAString& aName, bool aIsStandardFace = false);

    
    
    const nsString& Name() const { return mName; }

    
    const nsString& FamilyName() const { return mFamilyName; }

    
    
    
    

    
    
    virtual nsString RealFaceName();

    uint16_t Weight() const { return mWeight; }
    int16_t Stretch() const { return mStretch; }

    bool IsUserFont() const { return mIsDataUserFont || mIsLocalUserFont; }
    bool IsLocalUserFont() const { return mIsLocalUserFont; }
    bool IsFixedPitch() const { return mFixedPitch; }
    bool IsItalic() const { return mItalic; }
    bool IsBold() const { return mWeight >= 600; } 
    bool IgnoreGDEF() const { return mIgnoreGDEF; }
    bool IgnoreGSUB() const { return mIgnoreGSUB; }

    
    
    bool SupportsOpenTypeFeature(int32_t aScript, uint32_t aFeatureTag);
    bool SupportsGraphiteFeature(uint32_t aFeatureTag);

    
    const hb_set_t*
    InputsForOpenTypeFeature(int32_t aScript, uint32_t aFeatureTag);

    virtual bool IsSymbolFont();

    virtual bool HasFontTable(uint32_t aTableTag);

    inline bool HasGraphiteTables() {
        if (!mCheckedForGraphiteTables) {
            CheckForGraphiteTables();
            mCheckedForGraphiteTables = true;
        }
        return mHasGraphiteTables;
    }

    inline bool HasCmapTable() {
        if (!mCharacterMap) {
            ReadCMAP();
            NS_ASSERTION(mCharacterMap, "failed to initialize character map");
        }
        return mHasCmapTable;
    }

    inline bool HasCharacter(uint32_t ch) {
        if (mCharacterMap && mCharacterMap->test(ch)) {
            return true;
        }
        return TestCharacterMap(ch);
    }

    virtual bool SkipDuringSystemFallback() { return false; }
    virtual bool TestCharacterMap(uint32_t aCh);
    nsresult InitializeUVSMap();
    uint16_t GetUVSGlyph(uint32_t aCh, uint32_t aVS);

    
    
    
    
    
    
    virtual nsresult ReadCMAP(FontInfoData *aFontInfoData = nullptr);

    bool TryGetSVGData(gfxFont* aFont);
    bool HasSVGGlyph(uint32_t aGlyphId);
    bool GetSVGGlyphExtents(gfxContext *aContext, uint32_t aGlyphId,
                            gfxRect *aResult);
    bool RenderSVGGlyph(gfxContext *aContext, uint32_t aGlyphId, int aDrawMode,
                        gfxTextContextPaint *aContextPaint);
    
    
    void NotifyGlyphsChanged();

    enum MathConstant {
        
        
        ScriptPercentScaleDown,
        ScriptScriptPercentScaleDown,
        DelimitedSubFormulaMinHeight,
        DisplayOperatorMinHeight,
        MathLeading,
        AxisHeight,
        AccentBaseHeight,
        FlattenedAccentBaseHeight,
        SubscriptShiftDown,
        SubscriptTopMax,
        SubscriptBaselineDropMin,
        SuperscriptShiftUp,
        SuperscriptShiftUpCramped,
        SuperscriptBottomMin,
        SuperscriptBaselineDropMax,
        SubSuperscriptGapMin,
        SuperscriptBottomMaxWithSubscript,
        SpaceAfterScript,
        UpperLimitGapMin,
        UpperLimitBaselineRiseMin,
        LowerLimitGapMin,
        LowerLimitBaselineDropMin,
        StackTopShiftUp,
        StackTopDisplayStyleShiftUp,
        StackBottomShiftDown,
        StackBottomDisplayStyleShiftDown,
        StackGapMin,
        StackDisplayStyleGapMin,
        StretchStackTopShiftUp,
        StretchStackBottomShiftDown,
        StretchStackGapAboveMin,
        StretchStackGapBelowMin,
        FractionNumeratorShiftUp,
        FractionNumeratorDisplayStyleShiftUp,
        FractionDenominatorShiftDown,
        FractionDenominatorDisplayStyleShiftDown,
        FractionNumeratorGapMin,
        FractionNumDisplayStyleGapMin,
        FractionRuleThickness,
        FractionDenominatorGapMin,
        FractionDenomDisplayStyleGapMin,
        SkewedFractionHorizontalGap,
        SkewedFractionVerticalGap,
        OverbarVerticalGap,
        OverbarRuleThickness,
        OverbarExtraAscender,
        UnderbarVerticalGap,
        UnderbarRuleThickness,
        UnderbarExtraDescender,
        RadicalVerticalGap,
        RadicalDisplayStyleVerticalGap,
        RadicalRuleThickness,
        RadicalExtraAscender,
        RadicalKernBeforeDegree,
        RadicalKernAfterDegree,
        RadicalDegreeBottomRaisePercent
    };

    
    
    
    bool     TryGetMathTable();
    gfxFloat GetMathConstant(MathConstant aConstant);
    bool     GetMathItalicsCorrection(uint32_t aGlyphID,
                                      gfxFloat* aItalicCorrection);
    uint32_t GetMathVariantsSize(uint32_t aGlyphID, bool aVertical,
                                 uint16_t aSize);
    bool     GetMathVariantsParts(uint32_t aGlyphID, bool aVertical,
                                  uint32_t aGlyphs[4]);

    bool     TryGetColorGlyphs();
    bool     GetColorLayersInfo(uint32_t aGlyphId,
                                nsTArray<uint16_t>& layerGlyphs,
                                nsTArray<mozilla::gfx::Color>& layerColors);

    virtual bool MatchesGenericFamily(const nsACString& aGeneric) const {
        return true;
    }
    virtual bool SupportsLangGroup(nsIAtom *aLangGroup) const {
        return true;
    }

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    virtual hb_blob_t *GetFontTable(uint32_t aTag);

    
    
    class AutoTable {
    public:
        AutoTable(gfxFontEntry* aFontEntry, uint32_t aTag)
        {
            mBlob = aFontEntry->GetFontTable(aTag);
        }
        ~AutoTable() {
            if (mBlob) {
                hb_blob_destroy(mBlob);
            }
        }
        operator hb_blob_t*() const { return mBlob; }
    private:
        hb_blob_t* mBlob;
        
        AutoTable(const AutoTable&) MOZ_DELETE;
        AutoTable& operator=(const AutoTable&) MOZ_DELETE;
    };

    already_AddRefed<gfxFont> FindOrMakeFont(const gfxFontStyle *aStyle,
                                             bool aNeedsBold);

    
    
    
    
    
    
    bool GetExistingFontTable(uint32_t aTag, hb_blob_t** aBlob);

    
    
    
    
    
    
    
    hb_blob_t *ShareFontTableAndGetBlob(uint32_t aTag,
                                        FallibleTArray<uint8_t>* aTable);

    
    
    
    uint16_t UnitsPerEm();
    enum {
        kMinUPEM = 16,    
        kMaxUPEM = 16384, 
        kInvalidUPEM = uint16_t(-1)
    };

    
    
    

    
    
    
    hb_face_t* GetHBFace();
    virtual void ForgetHBFace();

    
    
    gr_face* GetGrFace();
    virtual void ReleaseGrFace(gr_face* aFace);

    
    void DisconnectSVG();

    
    
    void NotifyFontDestroyed(gfxFont* aFont);

    
    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

    
    struct ScriptRange {
        uint32_t         rangeStart;
        uint32_t         rangeEnd;
        hb_tag_t         tags[3]; 
                                  
    };

    bool SupportsScriptInGSUB(const hb_tag_t* aScriptTags);

    nsString         mName;
    nsString         mFamilyName;

    bool             mItalic      : 1;
    bool             mFixedPitch  : 1;
    bool             mIsValid     : 1;
    bool             mIsBadUnderlineFont : 1;
    bool             mIsUserFontContainer : 1; 
    bool             mIsDataUserFont : 1;      
    bool             mIsLocalUserFont : 1;     
    bool             mStandardFace : 1;
    bool             mSymbolFont  : 1;
    bool             mIgnoreGDEF  : 1;
    bool             mIgnoreGSUB  : 1;
    bool             mSVGInitialized : 1;
    bool             mMathInitialized : 1;
    bool             mHasSpaceFeaturesInitialized : 1;
    bool             mHasSpaceFeatures : 1;
    bool             mHasSpaceFeaturesKerning : 1;
    bool             mHasSpaceFeaturesNonKerning : 1;
    bool             mSkipDefaultFeatureSpaceCheck : 1;
    bool             mHasGraphiteTables : 1;
    bool             mCheckedForGraphiteTables : 1;
    bool             mHasCmapTable : 1;
    bool             mGrFaceInitialized : 1;
    bool             mCheckedForColorGlyph : 1;

    
    
    uint32_t         mDefaultSubSpaceFeatures[(MOZ_NUM_SCRIPT_CODES + 31) / 32];
    uint32_t         mNonDefaultSubSpaceFeatures[(MOZ_NUM_SCRIPT_CODES + 31) / 32];

    uint16_t         mWeight;
    int16_t          mStretch;

    nsRefPtr<gfxCharacterMap> mCharacterMap;
    uint32_t         mUVSOffset;
    nsAutoArrayPtr<uint8_t> mUVSData;
    nsAutoPtr<gfxUserFontData> mUserFontData;
    nsAutoPtr<gfxSVGGlyphs> mSVGGlyphs;
    
    nsTArray<gfxFont*> mFontsUsingSVGGlyphs;
    nsAutoPtr<gfxMathTable> mMathTable;
    nsTArray<gfxFontFeature> mFeatureSettings;
    nsAutoPtr<nsDataHashtable<nsUint32HashKey,bool>> mSupportedFeatures;
    nsAutoPtr<nsDataHashtable<nsUint32HashKey,hb_set_t*>> mFeatureInputs;
    uint32_t         mLanguageOverride;

    
    hb_blob_t*       mCOLR;
    hb_blob_t*       mCPAL;

protected:
    friend class gfxPlatformFontList;
    friend class gfxMacPlatformFontList;
    friend class gfxUserFcFontEntry;
    friend class gfxFontFamily;
    friend class gfxSingleFaceMacFontFamily;

    gfxFontEntry();

    
    virtual ~gfxFontEntry();

    virtual gfxFont *CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold) {
        NS_NOTREACHED("oops, somebody didn't override CreateFontInstance");
        return nullptr;
    }

    virtual void CheckForGraphiteTables();

    
    
    virtual nsresult CopyFontTable(uint32_t aTableTag,
                                   FallibleTArray<uint8_t>& aBuffer) {
        NS_NOTREACHED("forgot to override either GetFontTable or CopyFontTable?");
        return NS_ERROR_FAILURE;
    }

    
    
    
    
    
    
    hb_blob_t* GetTableFromFontData(const void* aFontData, uint32_t aTableTag);

    
    virtual already_AddRefed<gfxCharacterMap>
    GetCMAPFromFontInfo(FontInfoData *aFontInfoData,
                        uint32_t& aUVSOffset,
                        bool& aSymbolFont);

    
    
    uint16_t mUnitsPerEm;

    
    
    
    

    
    
    
    
    
    hb_face_t* mHBFace;

    static hb_blob_t* HBGetTable(hb_face_t *face, uint32_t aTag, void *aUserData);

    
    static void HBFaceDeletedCallback(void *aUserData);

    
    
    
    
    gr_face*   mGrFace;

    
    
    nsDataHashtable<nsPtrHashKey<const void>,void*>* mGrTableMap;

    
    nsrefcnt mGrFaceRefCnt;

    static const void* GrGetTable(const void *aAppFaceHandle,
                                  unsigned int aName,
                                  size_t *aLen);
    static void GrReleaseTable(const void *aAppFaceHandle,
                               const void *aTableBuffer);

private:
    























    class FontTableBlobData;

    










    class FontTableHashEntry : public nsUint32HashKey
    {
    public:
        

        typedef nsUint32HashKey KeyClass;
        typedef KeyClass::KeyType KeyType;
        typedef KeyClass::KeyTypePointer KeyTypePointer;

        explicit FontTableHashEntry(KeyTypePointer aTag)
            : KeyClass(aTag)
            , mSharedBlobData(nullptr)
            , mBlob(nullptr)
        { }

        
        
        
        FontTableHashEntry(FontTableHashEntry&& toMove)
            : KeyClass(mozilla::Move(toMove))
            , mSharedBlobData(mozilla::Move(toMove.mSharedBlobData))
            , mBlob(mozilla::Move(toMove.mBlob))
        {
            toMove.mSharedBlobData = nullptr;
            toMove.mBlob = nullptr;
        }

        ~FontTableHashEntry() { Clear(); }

        

        
        
        
        
        hb_blob_t *
        ShareTableAndGetBlob(FallibleTArray<uint8_t>& aTable,
                             nsTHashtable<FontTableHashEntry> *aHashtable);

        
        
        hb_blob_t *GetBlob() const;

        void Clear();

        size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

    private:
        static void DeleteFontTableBlobData(void *aBlobData);
        
        FontTableHashEntry& operator=(FontTableHashEntry& toCopy);

        FontTableBlobData *mSharedBlobData;
        hb_blob_t *mBlob;
    };

    nsAutoPtr<nsTHashtable<FontTableHashEntry> > mFontTableCache;

    gfxFontEntry(const gfxFontEntry&);
    gfxFontEntry& operator=(const gfxFontEntry&);
};



struct GlobalFontMatch {
    GlobalFontMatch(const uint32_t aCharacter,
                    int32_t aRunScript,
                    const gfxFontStyle *aStyle) :
        mCh(aCharacter), mRunScript(aRunScript), mStyle(aStyle),
        mMatchRank(0), mCount(0), mCmapsTested(0)
        {

        }

    const uint32_t         mCh;          
    int32_t                mRunScript;   
    const gfxFontStyle*    mStyle;       
    int32_t                mMatchRank;   
    nsRefPtr<gfxFontEntry> mBestMatch;   
    nsRefPtr<gfxFontFamily> mMatchedFamily; 
    uint32_t               mCount;       
    uint32_t               mCmapsTested; 
};

class gfxFontFamily {
public:
    NS_INLINE_DECL_REFCOUNTING(gfxFontFamily)

    explicit gfxFontFamily(const nsAString& aName) :
        mName(aName),
        mOtherFamilyNamesInitialized(false),
        mHasOtherFamilyNames(false),
        mFaceNamesInitialized(false),
        mHasStyles(false),
        mIsSimpleFamily(false),
        mIsBadUnderlineFamily(false),
        mFamilyCharacterMapInitialized(false),
        mSkipDefaultFeatureSpaceCheck(false)
        { }

    const nsString& Name() { return mName; }

    virtual void LocalizedName(nsAString& aLocalizedName);
    virtual bool HasOtherFamilyNames();
    
    nsTArray<nsRefPtr<gfxFontEntry> >& GetFontList() { return mAvailableFonts; }
    
    void AddFontEntry(nsRefPtr<gfxFontEntry> aFontEntry) {
        
        
        if (aFontEntry->IsItalic() && !aFontEntry->IsUserFont() &&
            Name().EqualsLiteral("Times New Roman"))
        {
            aFontEntry->mIgnoreGDEF = true;
        }
        if (aFontEntry->mFamilyName.IsEmpty()) {
            aFontEntry->mFamilyName = Name();
        } else {
            MOZ_ASSERT(aFontEntry->mFamilyName.Equals(Name()));
        }
        aFontEntry->mSkipDefaultFeatureSpaceCheck = mSkipDefaultFeatureSpaceCheck;
        mAvailableFonts.AppendElement(aFontEntry);
    }

    
    bool HasStyles() { return mHasStyles; }
    void SetHasStyles(bool aHasStyles) { mHasStyles = aHasStyles; }

    
    
    
    
    
    gfxFontEntry *FindFontForStyle(const gfxFontStyle& aFontStyle, 
                                   bool& aNeedsSyntheticBold);

    
    
    void FindFontForChar(GlobalFontMatch *aMatchData);

    
    void SearchAllFontsForChar(GlobalFontMatch *aMatchData);

    
    virtual void ReadOtherFamilyNames(gfxPlatformFontList *aPlatformFontList);

    
    
    static void ReadOtherFamilyNamesForFace(const nsAString& aFamilyName,
                                            const char *aNameData,
                                            uint32_t aDataLength,
                                            nsTArray<nsString>& aOtherFamilyNames,
                                            bool useFullName);

    
    void SetOtherFamilyNamesInitialized() {
        mOtherFamilyNamesInitialized = true;
    }

    
    
    virtual void ReadFaceNames(gfxPlatformFontList *aPlatformFontList,
                               bool aNeedFullnamePostscriptNames,
                               FontInfoData *aFontInfoData = nullptr);

    
    
    virtual void FindStyleVariations(FontInfoData *aFontInfoData = nullptr) { }

    
    gfxFontEntry* FindFont(const nsAString& aPostscriptName);

    
    void ReadAllCMAPs(FontInfoData *aFontInfoData = nullptr);

    bool TestCharacterMap(uint32_t aCh) {
        if (!mFamilyCharacterMapInitialized) {
            ReadAllCMAPs();
        }
        return mFamilyCharacterMap.test(aCh);
    }

    void ResetCharacterMap() {
        mFamilyCharacterMap.reset();
        mFamilyCharacterMapInitialized = false;
    }

    
    void SetBadUnderlineFamily() {
        mIsBadUnderlineFamily = true;
        if (mHasStyles) {
            SetBadUnderlineFonts();
        }
    }

    bool IsBadUnderlineFamily() const { return mIsBadUnderlineFamily; }

    
    void SortAvailableFonts();

    
    
    
    void CheckForSimpleFamily();

    
    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontListSizes* aSizes) const;

#ifdef DEBUG
    
    bool ContainsFace(gfxFontEntry* aFontEntry);
#endif

    void SetSkipSpaceFeatureCheck(bool aSkipCheck) {
        mSkipDefaultFeatureSpaceCheck = aSkipCheck;
    }

protected:
    
    virtual ~gfxFontFamily()
    {
    }

    
    
    virtual bool FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                       bool anItalic, int16_t aStretch);

    bool ReadOtherFamilyNamesForFace(gfxPlatformFontList *aPlatformFontList,
                                     hb_blob_t           *aNameTable,
                                     bool                 useFullName = false);

    
    void SetBadUnderlineFonts() {
        uint32_t i, numFonts = mAvailableFonts.Length();
        for (i = 0; i < numFonts; i++) {
            if (mAvailableFonts[i]) {
                mAvailableFonts[i]->mIsBadUnderlineFont = true;
            }
        }
    }

    nsString mName;
    nsTArray<nsRefPtr<gfxFontEntry> >  mAvailableFonts;
    gfxSparseBitSet mFamilyCharacterMap;
    bool mOtherFamilyNamesInitialized : 1;
    bool mHasOtherFamilyNames : 1;
    bool mFaceNamesInitialized : 1;
    bool mHasStyles : 1;
    bool mIsSimpleFamily : 1;
    bool mIsBadUnderlineFamily : 1;
    bool mFamilyCharacterMapInitialized : 1;
    bool mSkipDefaultFeatureSpaceCheck : 1;

    enum {
        
        
        kRegularFaceIndex    = 0,
        kBoldFaceIndex       = 1,
        kItalicFaceIndex     = 2,
        kBoldItalicFaceIndex = 3,
        
        kBoldMask   = 0x01,
        kItalicMask = 0x02
    };
};

struct gfxTextRange {
    enum {
        
        kFontGroup      = 0x0001,
        kPrefsFallback  = 0x0002,
        kSystemFallback = 0x0004
    };
    gfxTextRange(uint32_t aStart, uint32_t aEnd,
                 gfxFont* aFont, uint8_t aMatchType)
        : start(aStart),
          end(aEnd),
          font(aFont),
          matchType(aMatchType)
    { }
    uint32_t Length() const { return end - start; }
    uint32_t start, end;
    nsRefPtr<gfxFont> font;
    uint8_t matchType;
};



























struct FontCacheSizes {
    FontCacheSizes()
        : mFontInstances(0), mShapedWords(0)
    { }

    size_t mFontInstances; 
    size_t mShapedWords; 
};

class gfxFontCache MOZ_FINAL : public nsExpirationTracker<gfxFont,3> {
public:
    enum {
        FONT_TIMEOUT_SECONDS = 10,
        SHAPED_WORD_TIMEOUT_SECONDS = 60
    };

    gfxFontCache();
    ~gfxFontCache();

    



    static gfxFontCache* GetCache() {
        return gGlobalCache;
    }

    static nsresult Init();
    
    static void Shutdown();

    
    
    already_AddRefed<gfxFont> Lookup(const gfxFontEntry *aFontEntry,
                                     const gfxFontStyle *aStyle);
    
    
    
    
    void AddNew(gfxFont *aFont);

    
    
    
    void NotifyReleased(gfxFont *aFont);

    
    
    virtual void NotifyExpired(gfxFont *aFont);

    
    
    
    void Flush() {
        mFonts.Clear();
        AgeAllGenerations();
    }

    void FlushShapedWordCaches() {
        mFonts.EnumerateEntries(ClearCachedWordsForFont, nullptr);
    }

    void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                FontCacheSizes* aSizes) const;
    void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                FontCacheSizes* aSizes) const;

protected:
    class MemoryReporter MOZ_FINAL : public nsIMemoryReporter
    {
        ~MemoryReporter() {}
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIMEMORYREPORTER
    };

    
    class Observer MOZ_FINAL
        : public nsIObserver
    {
        ~Observer() {}
    public:
        NS_DECL_ISUPPORTS
        NS_DECL_NSIOBSERVER
    };

    void DestroyFont(gfxFont *aFont);

    static gfxFontCache *gGlobalCache;

    struct Key {
        const gfxFontEntry* mFontEntry;
        const gfxFontStyle* mStyle;
        Key(const gfxFontEntry* aFontEntry, const gfxFontStyle* aStyle)
            : mFontEntry(aFontEntry), mStyle(aStyle) {}
    };

    class HashEntry : public PLDHashEntryHdr {
    public:
        typedef const Key& KeyType;
        typedef const Key* KeyTypePointer;

        
        
        explicit HashEntry(KeyTypePointer aStr) : mFont(nullptr) { }
        HashEntry(const HashEntry& toCopy) : mFont(toCopy.mFont) { }
        ~HashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;
        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }
        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return mozilla::HashGeneric(aKey->mStyle->Hash(), aKey->mFontEntry);
        }
        enum { ALLOW_MEMMOVE = true };

        gfxFont* mFont;
    };

    static size_t AddSizeOfFontEntryExcludingThis(HashEntry* aHashEntry,
                                                  mozilla::MallocSizeOf aMallocSizeOf,
                                                  void* aUserArg);

    nsTHashtable<HashEntry> mFonts;

    static PLDHashOperator ClearCachedWordsForFont(HashEntry* aHashEntry, void*);
    static PLDHashOperator AgeCachedWordsForFont(HashEntry* aHashEntry, void*);
    static void WordCacheExpirationTimerCallback(nsITimer* aTimer, void* aCache);
    nsCOMPtr<nsITimer>      mWordCacheExpirationTimer;
};

class gfxTextPerfMetrics {
public:

    struct TextCounts {
        uint32_t    numContentTextRuns;
        uint32_t    numChromeTextRuns;
        uint32_t    numChars;
        uint32_t    maxTextRunLen;
        uint32_t    wordCacheSpaceRules;
        uint32_t    wordCacheLong;
        uint32_t    wordCacheHit;
        uint32_t    wordCacheMiss;
        uint32_t    fallbackPrefs;
        uint32_t    fallbackSystem;
        uint32_t    textrunConst;
        uint32_t    textrunDestr;
    };

    uint32_t reflowCount;

    
    TextCounts current;

    
    TextCounts cumulative;

    gfxTextPerfMetrics() {
        memset(this, 0, sizeof(gfxTextPerfMetrics));
    }

    
    void Accumulate() {
        if (current.numChars == 0) {
            return;
        }
        cumulative.numContentTextRuns += current.numContentTextRuns;
        cumulative.numChromeTextRuns += current.numChromeTextRuns;
        cumulative.numChars += current.numChars;
        if (current.maxTextRunLen > cumulative.maxTextRunLen) {
            cumulative.maxTextRunLen = current.maxTextRunLen;
        }
        cumulative.wordCacheSpaceRules += current.wordCacheSpaceRules;
        cumulative.wordCacheLong += current.wordCacheLong;
        cumulative.wordCacheHit += current.wordCacheHit;
        cumulative.wordCacheMiss += current.wordCacheMiss;
        cumulative.fallbackPrefs += current.fallbackPrefs;
        cumulative.fallbackSystem += current.fallbackSystem;
        cumulative.textrunConst += current.textrunConst;
        cumulative.textrunDestr += current.textrunDestr;
        memset(&current, 0, sizeof(current));
    }
};

class gfxTextRunFactory {
    NS_INLINE_DECL_REFCOUNTING(gfxTextRunFactory)

public:
    
    
    
    enum {
        CACHE_TEXT_FLAGS    = 0xF0000000,
        USER_TEXT_FLAGS     = 0x0FFF0000,
        PLATFORM_TEXT_FLAGS = 0x0000F000,
        TEXTRUN_TEXT_FLAGS  = 0x00000FFF,
        SETTABLE_FLAGS      = CACHE_TEXT_FLAGS | USER_TEXT_FLAGS,

        



        TEXT_IS_PERSISTENT           = 0x0001,
        


        TEXT_IS_ASCII                = 0x0002,
        


        TEXT_IS_RTL                  = 0x0004,
        



        TEXT_ENABLE_SPACING          = 0x0008,
        



        TEXT_ENABLE_HYPHEN_BREAKS    = 0x0010,
        



        TEXT_IS_8BIT                 = 0x0020,
        






        TEXT_NEED_BOUNDING_BOX       = 0x0040,
        



        TEXT_DISABLE_OPTIONAL_LIGATURES = 0x0080,
        




        TEXT_OPTIMIZE_SPEED          = 0x0100,
        






        TEXT_RUN_SIZE_ACCOUNTED      = 0x0200,
        



        TEXT_HIDE_CONTROL_CHARACTERS = 0x0400,

        




        TEXT_TRAILING_ARABICCHAR = 0x20000000,
        




        TEXT_INCOMING_ARABICCHAR = 0x40000000,

        
        TEXT_USE_MATH_SCRIPT = 0x80000000,

        TEXT_UNUSED_FLAGS = 0x10000000
    };

    


    struct Parameters {
        
        gfxContext   *mContext;
        
        void         *mUserData;
        
        
        
        gfxSkipChars *mSkipChars;
        
        
        uint32_t     *mInitialBreaks;
        uint32_t      mInitialBreakCount;
        
        int32_t       mAppUnitsPerDevUnit;
    };

protected:
    
    virtual ~gfxTextRunFactory() {}
};














class gfxGlyphExtents {
public:
    explicit gfxGlyphExtents(int32_t aAppUnitsPerDevUnit) :
        mAppUnitsPerDevUnit(aAppUnitsPerDevUnit) {
        MOZ_COUNT_CTOR(gfxGlyphExtents);
    }
    ~gfxGlyphExtents();

    enum { INVALID_WIDTH = 0xFFFF };

    void NotifyGlyphsChanged() {
        mTightGlyphExtents.Clear();
    }

    
    
    
    
    uint16_t GetContainedGlyphWidthAppUnits(uint32_t aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID);
    }

    bool IsGlyphKnown(uint32_t aGlyphID) const {
        return mContainedGlyphWidths.Get(aGlyphID) != INVALID_WIDTH ||
            mTightGlyphExtents.GetEntry(aGlyphID) != nullptr;
    }

    bool IsGlyphKnownWithTightExtents(uint32_t aGlyphID) const {
        return mTightGlyphExtents.GetEntry(aGlyphID) != nullptr;
    }

    
    
    
    bool GetTightGlyphExtentsAppUnits(gfxFont *aFont, gfxContext *aContext,
            uint32_t aGlyphID, gfxRect *aExtents);

    void SetContainedGlyphWidthAppUnits(uint32_t aGlyphID, uint16_t aWidth) {
        mContainedGlyphWidths.Set(aGlyphID, aWidth);
    }
    void SetTightGlyphExtents(uint32_t aGlyphID, const gfxRect& aExtentsAppUnits);

    int32_t GetAppUnitsPerDevUnit() { return mAppUnitsPerDevUnit; }

    size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
    size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;

private:
    class HashEntry : public nsUint32HashKey {
    public:
        
        
        explicit HashEntry(KeyTypePointer aPtr) : nsUint32HashKey(aPtr) {}
        HashEntry(const HashEntry& toCopy) : nsUint32HashKey(toCopy) {
          x = toCopy.x; y = toCopy.y; width = toCopy.width; height = toCopy.height;
        }

        float x, y, width, height;
    };

    enum { BLOCK_SIZE_BITS = 7, BLOCK_SIZE = 1 << BLOCK_SIZE_BITS }; 

    class GlyphWidths {
    public:
        void Set(uint32_t aIndex, uint16_t aValue);
        uint16_t Get(uint32_t aIndex) const {
            uint32_t block = aIndex >> BLOCK_SIZE_BITS;
            if (block >= mBlocks.Length())
                return INVALID_WIDTH;
            uintptr_t bits = mBlocks[block];
            if (!bits)
                return INVALID_WIDTH;
            uint32_t indexInBlock = aIndex & (BLOCK_SIZE - 1);
            if (bits & 0x1) {
                if (GetGlyphOffset(bits) != indexInBlock)
                    return INVALID_WIDTH;
                return GetWidth(bits);
            }
            uint16_t *widths = reinterpret_cast<uint16_t *>(bits);
            return widths[indexInBlock];
        }

        uint32_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const;
        
        ~GlyphWidths();

    private:
        static uint32_t GetGlyphOffset(uintptr_t aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return (aBits >> 1) & ((1 << BLOCK_SIZE_BITS) - 1);
        }
        static uint32_t GetWidth(uintptr_t aBits) {
            NS_ASSERTION(aBits & 0x1, "This is really a pointer...");
            return aBits >> (1 + BLOCK_SIZE_BITS);
        }
        static uintptr_t MakeSingle(uint32_t aGlyphOffset, uint16_t aWidth) {
            return (aWidth << (1 + BLOCK_SIZE_BITS)) + (aGlyphOffset << 1) + 1;
        }

        nsTArray<uintptr_t> mBlocks;
    };

    GlyphWidths             mContainedGlyphWidths;
    nsTHashtable<HashEntry> mTightGlyphExtents;
    int32_t                 mAppUnitsPerDevUnit;

private:
    
    gfxGlyphExtents(const gfxGlyphExtents& aOther) MOZ_DELETE;
    gfxGlyphExtents& operator=(const gfxGlyphExtents& aOther) MOZ_DELETE;
};



















class gfxFontShaper {
public:
    explicit gfxFontShaper(gfxFont *aFont)
        : mFont(aFont)
    {
        NS_ASSERTION(aFont, "shaper requires a valid font!");
    }

    virtual ~gfxFontShaper() { }

    
    
    
    virtual bool ShapeText(gfxContext     *aContext,
                           const char16_t *aText,
                           uint32_t        aOffset,
                           uint32_t        aLength,
                           int32_t         aScript,
                           gfxShapedText  *aShapedText) = 0;

    gfxFont *GetFont() const { return mFont; }

    
    static bool
    MergeFontFeatures(const gfxFontStyle *aStyle,
                      const nsTArray<gfxFontFeature>& aFontFeatures,
                      bool aDisableLigatures,
                      const nsAString& aFamilyName,
                      bool aAddSmallCaps,
                      nsDataHashtable<nsUint32HashKey,uint32_t>& aMergedFeatures);

protected:
    
    gfxFont * mFont;
};


class GlyphBufferAzure;
struct TextRunDrawParams;
struct FontDrawParams;

class gfxFont {

    friend class gfxHarfBuzzShaper;
    friend class gfxGraphiteShaper;

public:
    nsrefcnt AddRef(void) {
        NS_PRECONDITION(int32_t(mRefCnt) >= 0, "illegal refcnt");
        if (mExpirationState.IsTracked()) {
            gfxFontCache::GetCache()->RemoveObject(this);
        }
        ++mRefCnt;
        NS_LOG_ADDREF(this, mRefCnt, "gfxFont", sizeof(*this));
        return mRefCnt;
    }
    nsrefcnt Release(void) {
        NS_PRECONDITION(0 != mRefCnt, "dup release");
        --mRefCnt;
        NS_LOG_RELEASE(this, mRefCnt, "gfxFont");
        if (mRefCnt == 0) {
            NotifyReleased();
            
            return 0;
        }
        return mRefCnt;
    }

    int32_t GetRefCount() { return mRefCnt; }

    
    typedef enum {
        kAntialiasDefault,
        kAntialiasNone,
        kAntialiasGrayscale,
        kAntialiasSubpixel
    } AntialiasOption;

protected:
    nsAutoRefCnt mRefCnt;
    cairo_scaled_font_t *mScaledFont;

    void NotifyReleased() {
        gfxFontCache *cache = gfxFontCache::GetCache();
        if (cache) {
            
            
            cache->NotifyReleased(this);
        } else {
            
            delete this;
        }
    }

    gfxFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
            AntialiasOption anAAOption = kAntialiasDefault,
            cairo_scaled_font_t *aScaledFont = nullptr);

public:
    virtual ~gfxFont();

    bool Valid() const {
        return mIsValid;
    }

    
    typedef enum {
        LOOSE_INK_EXTENTS,
            
            
            
        TIGHT_INK_EXTENTS,
            
            
            
            
        TIGHT_HINTED_OUTLINE_EXTENTS
            
            
            
            
            
            
            
            
            
            
            
            
            
    } BoundingBoxType;

    const nsString& GetName() const { return mFontEntry->Name(); }
    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual cairo_scaled_font_t* GetCairoScaledFont() { return mScaledFont; }

    virtual gfxFont* CopyWithAntialiasOption(AntialiasOption anAAOption) {
        
        return nullptr;
    }

    virtual gfxFloat GetAdjustedSize() const {
        return mAdjustedSize > 0.0 ? mAdjustedSize : mStyle.size;
    }

    float FUnitsToDevUnitsFactor() const {
        
        NS_ASSERTION(mFUnitsConvFactor > 0.0f, "mFUnitsConvFactor not valid");
        return mFUnitsConvFactor;
    }

    
    bool FontCanSupportHarfBuzz() {
        return mFontEntry->HasCmapTable();
    }

    
    bool FontCanSupportGraphite() {
        return mFontEntry->HasGraphiteTables();
    }

    
    
    bool SupportsFeature(int32_t aScript, uint32_t aFeatureTag);

    
    
    bool SupportsVariantCaps(int32_t aScript, uint32_t aVariantCaps,
                             bool& aFallbackToSmallCaps,
                             bool& aSyntheticLowerToSmallCaps,
                             bool& aSyntheticUpperToSmallCaps);

    
    
    
    bool SupportsSubSuperscript(uint32_t aSubSuperscript,
                                const uint8_t *aString,
                                uint32_t aLength, int32_t aRunScript);

    bool SupportsSubSuperscript(uint32_t aSubSuperscript,
                                const char16_t *aString,
                                uint32_t aLength, int32_t aRunScript);

    
    
    
    virtual bool ProvidesGetGlyph() const {
        return false;
    }
    
    
    virtual uint32_t GetGlyph(uint32_t unicode, uint32_t variation_selector) {
        return 0;
    }
    
    gfxFloat GetGlyphHAdvance(gfxContext *aCtx, uint16_t aGID);

    
    virtual mozilla::TemporaryRef<mozilla::gfx::GlyphRenderingOptions>
      GetGlyphRenderingOptions() { return nullptr; }

    gfxFloat SynthesizeSpaceWidth(uint32_t aCh);

    
    struct Metrics {
        gfxFloat xHeight;
        gfxFloat strikeoutSize;
        gfxFloat strikeoutOffset;
        gfxFloat underlineSize;
        gfxFloat underlineOffset;

        gfxFloat internalLeading;
        gfxFloat externalLeading;

        gfxFloat emHeight;
        gfxFloat emAscent;
        gfxFloat emDescent;
        gfxFloat maxHeight;
        gfxFloat maxAscent;
        gfxFloat maxDescent;
        gfxFloat maxAdvance;

        gfxFloat aveCharWidth;
        gfxFloat spaceWidth;
        gfxFloat zeroOrAveCharWidth;  
                                      
                                      
    };
    virtual const gfxFont::Metrics& GetMetrics() = 0;

    







    struct Spacing {
        gfxFloat mBefore;
        gfxFloat mAfter;
    };
    


    struct RunMetrics {
        RunMetrics() {
            mAdvanceWidth = mAscent = mDescent = 0.0;
        }

        void CombineWith(const RunMetrics& aOther, bool aOtherIsOnLeft);

        
        
        
        
        gfxFloat mAdvanceWidth;
        
        
        gfxFloat mAscent;  
        gfxFloat mDescent; 
        
        
        
        
        
        
        
        gfxRect  mBoundingBox;
    };

    


























    void Draw(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
              gfxPoint *aPt, const TextRunDrawParams& aRunParams);

    




















    virtual RunMetrics Measure(gfxTextRun *aTextRun,
                               uint32_t aStart, uint32_t aEnd,
                               BoundingBoxType aBoundingBoxType,
                               gfxContext *aContextForTightBoundingBox,
                               Spacing *aSpacing);
    




    bool NotifyLineBreaksChanged(gfxTextRun *aTextRun,
                                   uint32_t aStart, uint32_t aLength)
    { return false; }

    
    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    virtual uint32_t GetSpaceGlyph() = 0;

    gfxGlyphExtents *GetOrCreateGlyphExtents(int32_t aAppUnitsPerDevUnit);

    
    virtual void SetupGlyphExtents(gfxContext *aContext, uint32_t aGlyphID,
                                   bool aNeedTight, gfxGlyphExtents *aExtents);

    
    virtual bool SetupCairoFont(gfxContext *aContext) = 0;

    virtual bool AllowSubpixelAA() { return true; }

    bool IsSyntheticBold() { return mApplySyntheticBold; }

    
    
    
    
    gfxFloat GetSyntheticBoldOffset() {
        gfxFloat size = GetAdjustedSize();
        const gfxFloat threshold = 48.0;
        return size < threshold ? (0.25 + 0.75 * size / threshold) :
                                  (size / threshold);
    }

    gfxFontEntry *GetFontEntry() const { return mFontEntry.get(); }
    bool HasCharacter(uint32_t ch) {
        if (!mIsValid)
            return false;
        return mFontEntry->HasCharacter(ch); 
    }

    uint16_t GetUVSGlyph(uint32_t aCh, uint32_t aVS) {
        if (!mIsValid) {
            return 0;
        }
        return mFontEntry->GetUVSGlyph(aCh, aVS); 
    }

    bool InitFakeSmallCapsRun(gfxContext     *aContext,
                              gfxTextRun     *aTextRun,
                              const uint8_t  *aText,
                              uint32_t        aOffset,
                              uint32_t        aLength,
                              uint8_t         aMatchType,
                              int32_t         aScript,
                              bool            aSyntheticLower,
                              bool            aSyntheticUpper);

    bool InitFakeSmallCapsRun(gfxContext     *aContext,
                              gfxTextRun     *aTextRun,
                              const char16_t *aText,
                              uint32_t        aOffset,
                              uint32_t        aLength,
                              uint8_t         aMatchType,
                              int32_t         aScript,
                              bool            aSyntheticLower,
                              bool            aSyntheticUpper);

    
    
    
    template<typename T>
    bool SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const T *aString,
                             uint32_t aRunStart,
                             uint32_t aRunLength,
                             int32_t aRunScript);

    
    
    template<typename T>
    gfxShapedWord* GetShapedWord(gfxContext *aContext,
                                 const T *aText,
                                 uint32_t aLength,
                                 uint32_t aHash,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags,
                                 gfxTextPerfMetrics *aTextPerf);

    
    
    void InitWordCache() {
        if (!mWordCache) {
            mWordCache = new nsTHashtable<CacheHashEntry>;
        }
    }

    
    
    void AgeCachedWords() {
        if (mWordCache) {
            (void)mWordCache->EnumerateEntries(AgeCacheEntry, this);
        }
    }

    
    void ClearCachedWords() {
        if (mWordCache) {
            mWordCache->Clear();
        }
    }

    
    void NotifyGlyphsChanged();

    virtual void AddSizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;
    virtual void AddSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf,
                                        FontCacheSizes* aSizes) const;

    typedef enum {
        FONT_TYPE_DWRITE,
        FONT_TYPE_GDI,
        FONT_TYPE_FT2,
        FONT_TYPE_MAC,
        FONT_TYPE_OS2,
        FONT_TYPE_CAIRO
    } FontType;

    virtual FontType GetType() const = 0;

    virtual mozilla::TemporaryRef<mozilla::gfx::ScaledFont> GetScaledFont(mozilla::gfx::DrawTarget *aTarget)
    { return gfxPlatform::GetPlatform()->GetScaledFontForFont(aTarget, this); }

    bool KerningDisabled() {
        return mKerningSet && !mKerningEnabled;
    }

    



    class GlyphChangeObserver {
    public:
        virtual ~GlyphChangeObserver()
        {
            if (mFont) {
                mFont->RemoveGlyphChangeObserver(this);
            }
        }
        
        void ForgetFont() { mFont = nullptr; }
        virtual void NotifyGlyphsChanged() = 0;
    protected:
        explicit GlyphChangeObserver(gfxFont *aFont) : mFont(aFont)
        {
            mFont->AddGlyphChangeObserver(this);
        }
        gfxFont* mFont;
    };
    friend class GlyphChangeObserver;

    bool GlyphsMayChange()
    {
        
        return mFontEntry->TryGetSVGData(this);
    }

    static void DestroySingletons() {
        delete sScriptTagToCode;
        delete sDefaultFeatures;
    }

    
    
    
    nscoord GetMathConstant(gfxFontEntry::MathConstant aConstant,
                            uint32_t aAppUnitsPerDevPixel)
    {
        return NSToCoordRound(mFontEntry->GetMathConstant(aConstant) *
                              GetAdjustedSize() * aAppUnitsPerDevPixel);
    }

    
    
    
    float GetMathConstant(gfxFontEntry::MathConstant aConstant)
    {
        return mFontEntry->GetMathConstant(aConstant);
    }

    
    virtual already_AddRefed<gfxFont>
    GetSubSuperscriptFont(int32_t aAppUnitsPerDevPixel);

protected:
    
    
    
    
    void DrawOneGlyph(uint32_t           aGlyphID,
                      double             aAdvance,
                      gfxPoint          *aPt,
                      GlyphBufferAzure&  aBuffer,
                      bool              *aEmittedGlyphs) const;

    
    
    
    bool DrawGlyphs(gfxShapedText            *aShapedText,
                    uint32_t                  aOffset, 
                    uint32_t                  aCount, 
                    gfxPoint                 *aPt,
                    const TextRunDrawParams&  aRunParams,
                    const FontDrawParams&     aFontParams);

    
    
    void CalculateSubSuperSizeAndOffset(int32_t aAppUnitsPerDevPixel,
                                        gfxFloat& aSubSuperSizeRatio,
                                        float& aBaselineOffset);

    
    
    
    
    
    virtual already_AddRefed<gfxFont> GetSmallCapsFont();

    
    
    
    virtual bool ProvidesGlyphWidths() const {
        return false;
    }

    
    
    virtual int32_t GetGlyphWidth(gfxContext *aCtx, uint16_t aGID) {
        return -1;
    }

    void AddGlyphChangeObserver(GlyphChangeObserver *aObserver);
    void RemoveGlyphChangeObserver(GlyphChangeObserver *aObserver);

    
    bool HasSubstitutionRulesWithSpaceLookups(int32_t aRunScript);

    
    bool SpaceMayParticipateInShaping(int32_t aRunScript);

    
    bool ShapeText(gfxContext    *aContext,
                   const uint8_t *aText,
                   uint32_t       aOffset, 
                   uint32_t       aLength,
                   int32_t        aScript,
                   gfxShapedText *aShapedText); 

    
    
    virtual bool ShapeText(gfxContext      *aContext,
                           const char16_t *aText,
                           uint32_t         aOffset,
                           uint32_t         aLength,
                           int32_t          aScript,
                           gfxShapedText   *aShapedText);

    
    
    
    void PostShapingFixup(gfxContext      *aContext,
                          const char16_t *aText,
                          uint32_t         aOffset, 
                          uint32_t         aLength,
                          gfxShapedText   *aShapedText);

    
    
    
    
    
    
    
    template<typename T>
    bool ShapeTextWithoutWordCache(gfxContext *aContext,
                                   const T    *aText,
                                   uint32_t    aOffset,
                                   uint32_t    aLength,
                                   int32_t     aScript,
                                   gfxTextRun *aTextRun);

    
    
    
    
    
    template<typename T>
    bool ShapeFragmentWithoutWordCache(gfxContext *aContext,
                                       const T    *aText,
                                       uint32_t    aOffset,
                                       uint32_t    aLength,
                                       int32_t     aScript,
                                       gfxTextRun *aTextRun);

    void CheckForFeaturesInvolvingSpace();

    
    
    bool HasFeatureSet(uint32_t aFeature, bool& aFeatureOn);

    
    static nsDataHashtable<nsUint32HashKey, int32_t> *sScriptTagToCode;
    static nsTHashtable<nsUint32HashKey>             *sDefaultFeatures;

    nsRefPtr<gfxFontEntry> mFontEntry;

    struct CacheHashKey {
        union {
            const uint8_t   *mSingle;
            const char16_t *mDouble;
        }                mText;
        uint32_t         mLength;
        uint32_t         mFlags;
        int32_t          mScript;
        int32_t          mAppUnitsPerDevUnit;
        PLDHashNumber    mHashKey;
        bool             mTextIs8Bit;

        CacheHashKey(const uint8_t *aText, uint32_t aLength,
                     uint32_t aStringHash,
                     int32_t aScriptCode, int32_t aAppUnitsPerDevUnit,
                     uint32_t aFlags)
            : mLength(aLength),
              mFlags(aFlags),
              mScript(aScriptCode),
              mAppUnitsPerDevUnit(aAppUnitsPerDevUnit),
              mHashKey(aStringHash + aScriptCode +
                  aAppUnitsPerDevUnit * 0x100 + aFlags * 0x10000),
              mTextIs8Bit(true)
        {
            NS_ASSERTION(aFlags & gfxTextRunFactory::TEXT_IS_8BIT,
                         "8-bit flag should have been set");
            mText.mSingle = aText;
        }

        CacheHashKey(const char16_t *aText, uint32_t aLength,
                     uint32_t aStringHash,
                     int32_t aScriptCode, int32_t aAppUnitsPerDevUnit,
                     uint32_t aFlags)
            : mLength(aLength),
              mFlags(aFlags),
              mScript(aScriptCode),
              mAppUnitsPerDevUnit(aAppUnitsPerDevUnit),
              mHashKey(aStringHash + aScriptCode +
                  aAppUnitsPerDevUnit * 0x100 + aFlags * 0x10000),
              mTextIs8Bit(false)
        {
            
            
            
            
            mText.mDouble = aText;
        }
    };

    class CacheHashEntry : public PLDHashEntryHdr {
    public:
        typedef const CacheHashKey &KeyType;
        typedef const CacheHashKey *KeyTypePointer;

        
        
        explicit CacheHashEntry(KeyTypePointer aKey) { }
        CacheHashEntry(const CacheHashEntry& toCopy) { NS_ERROR("Should not be called"); }
        ~CacheHashEntry() { }

        bool KeyEquals(const KeyTypePointer aKey) const;

        static KeyTypePointer KeyToPointer(KeyType aKey) { return &aKey; }

        static PLDHashNumber HashKey(const KeyTypePointer aKey) {
            return aKey->mHashKey;
        }

        size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
        {
            return aMallocSizeOf(mShapedWord.get());
        }

        enum { ALLOW_MEMMOVE = true };

        nsAutoPtr<gfxShapedWord> mShapedWord;
    };

    nsAutoPtr<nsTHashtable<CacheHashEntry> > mWordCache;

    static PLDHashOperator AgeCacheEntry(CacheHashEntry *aEntry, void *aUserData);
    static const uint32_t  kShapedWordCacheMaxAge = 3;

    bool                       mIsValid;

    
    
    bool                       mApplySyntheticBold;

    bool                       mKerningSet;     
    bool                       mKerningEnabled; 

    nsExpirationState          mExpirationState;
    gfxFontStyle               mStyle;
    nsAutoTArray<gfxGlyphExtents*,1> mGlyphExtentsArray;
    nsAutoPtr<nsTHashtable<nsPtrHashKey<GlyphChangeObserver> > > mGlyphChangeObservers;

    gfxFloat                   mAdjustedSize;

    float                      mFUnitsConvFactor; 

    
    AntialiasOption            mAntialiasOption;

    
    
    nsAutoPtr<gfxFont>         mNonAAFont;

    
    
    
    nsAutoPtr<gfxFontShaper>   mHarfBuzzShaper;
    nsAutoPtr<gfxFontShaper>   mGraphiteShaper;

    mozilla::RefPtr<mozilla::gfx::ScaledFont> mAzureScaledFont;

    
    
    
    
    
    
    
    
    bool InitMetricsFromSfntTables(Metrics& aMetrics);

    
    
    void CalculateDerivedMetrics(Metrics& aMetrics);

    
    
    void SanitizeMetrics(gfxFont::Metrics *aMetrics, bool aIsBadUnderlineFont);

    bool RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextContextPaint *aContextPaint) const;
    bool RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextContextPaint *aContextPaint,
                        gfxTextRunDrawCallbacks *aCallbacks,
                        bool& aEmittedGlyphs) const;

    bool RenderColorGlyph(gfxContext* aContext,
                          mozilla::gfx::ScaledFont* scaledFont,
                          mozilla::gfx::GlyphRenderingOptions* renderingOptions,
                          mozilla::gfx::DrawOptions drawOptions,
                          const mozilla::gfx::Point& aPoint,
                          uint32_t aGlyphId) const;

    
    
    
    
    
    
    
    static double CalcXScale(gfxContext *aContext);
};


#define DEFAULT_XHEIGHT_FACTOR 0.56f




















class gfxShapedText
{
public:
    gfxShapedText(uint32_t aLength, uint32_t aFlags,
                  int32_t aAppUnitsPerDevUnit)
        : mLength(aLength)
        , mFlags(aFlags)
        , mAppUnitsPerDevUnit(aAppUnitsPerDevUnit)
    { }

    virtual ~gfxShapedText() { }

    














    class CompressedGlyph {
    public:
        CompressedGlyph() { mValue = 0; }

        enum {
            
            
            
            
            
            FLAG_IS_SIMPLE_GLYPH  = 0x80000000U,

            
            
            
            FLAGS_CAN_BREAK_BEFORE = 0x60000000U,

            FLAGS_CAN_BREAK_SHIFT = 29,
            FLAG_BREAK_TYPE_NONE   = 0,
            FLAG_BREAK_TYPE_NORMAL = 1,
            FLAG_BREAK_TYPE_HYPHEN = 2,

            FLAG_CHAR_IS_SPACE     = 0x10000000U,

            
            ADVANCE_MASK  = 0x0FFF0000U,
            ADVANCE_SHIFT = 16,

            GLYPH_MASK = 0x0000FFFFU,

            
            
            

            
            
            
            
            
            FLAG_NOT_MISSING              = 0x01,
            FLAG_NOT_CLUSTER_START        = 0x02,
            FLAG_NOT_LIGATURE_GROUP_START = 0x04,

            FLAG_CHAR_IS_TAB              = 0x08,
            FLAG_CHAR_IS_NEWLINE          = 0x10,
            FLAG_CHAR_IS_LOW_SURROGATE    = 0x20,
            CHAR_IDENTITY_FLAGS_MASK      = 0x38,

            GLYPH_COUNT_MASK = 0x00FFFF00U,
            GLYPH_COUNT_SHIFT = 8
        };

        
        
        
        

        
        static bool IsSimpleGlyphID(uint32_t aGlyph) {
            return (aGlyph & GLYPH_MASK) == aGlyph;
        }
        
        
        static bool IsSimpleAdvance(uint32_t aAdvance) {
            return (aAdvance & (ADVANCE_MASK >> ADVANCE_SHIFT)) == aAdvance;
        }

        bool IsSimpleGlyph() const { return (mValue & FLAG_IS_SIMPLE_GLYPH) != 0; }
        uint32_t GetSimpleAdvance() const { return (mValue & ADVANCE_MASK) >> ADVANCE_SHIFT; }
        uint32_t GetSimpleGlyph() const { return mValue & GLYPH_MASK; }

        bool IsMissing() const { return (mValue & (FLAG_NOT_MISSING|FLAG_IS_SIMPLE_GLYPH)) == 0; }
        bool IsClusterStart() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) || !(mValue & FLAG_NOT_CLUSTER_START);
        }
        bool IsLigatureGroupStart() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) || !(mValue & FLAG_NOT_LIGATURE_GROUP_START);
        }
        bool IsLigatureContinuation() const {
            return (mValue & FLAG_IS_SIMPLE_GLYPH) == 0 &&
                (mValue & (FLAG_NOT_LIGATURE_GROUP_START | FLAG_NOT_MISSING)) ==
                    (FLAG_NOT_LIGATURE_GROUP_START | FLAG_NOT_MISSING);
        }

        
        
        
        bool CharIsSpace() const {
            return (mValue & FLAG_CHAR_IS_SPACE) != 0;
        }

        bool CharIsTab() const {
            return !IsSimpleGlyph() && (mValue & FLAG_CHAR_IS_TAB) != 0;
        }
        bool CharIsNewline() const {
            return !IsSimpleGlyph() && (mValue & FLAG_CHAR_IS_NEWLINE) != 0;
        }
        bool CharIsLowSurrogate() const {
            return !IsSimpleGlyph() && (mValue & FLAG_CHAR_IS_LOW_SURROGATE) != 0;
        }

        uint32_t CharIdentityFlags() const {
            return IsSimpleGlyph() ? 0 : (mValue & CHAR_IDENTITY_FLAGS_MASK);
        }

        void SetClusterStart(bool aIsClusterStart) {
            NS_ASSERTION(!IsSimpleGlyph(),
                         "can't call SetClusterStart on simple glyphs");
            if (aIsClusterStart) {
                mValue &= ~FLAG_NOT_CLUSTER_START;
            } else {
                mValue |= FLAG_NOT_CLUSTER_START;
            }
        }

        uint8_t CanBreakBefore() const {
            return (mValue & FLAGS_CAN_BREAK_BEFORE) >> FLAGS_CAN_BREAK_SHIFT;
        }
        
        uint32_t SetCanBreakBefore(uint8_t aCanBreakBefore) {
            NS_ASSERTION(aCanBreakBefore <= 2,
                         "Bogus break-before value!");
            uint32_t breakMask = (uint32_t(aCanBreakBefore) << FLAGS_CAN_BREAK_SHIFT);
            uint32_t toggle = breakMask ^ (mValue & FLAGS_CAN_BREAK_BEFORE);
            mValue ^= toggle;
            return toggle;
        }

        CompressedGlyph& SetSimpleGlyph(uint32_t aAdvanceAppUnits, uint32_t aGlyph) {
            NS_ASSERTION(IsSimpleAdvance(aAdvanceAppUnits), "Advance overflow");
            NS_ASSERTION(IsSimpleGlyphID(aGlyph), "Glyph overflow");
            NS_ASSERTION(!CharIdentityFlags(), "Char identity flags lost");
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_CHAR_IS_SPACE)) |
                FLAG_IS_SIMPLE_GLYPH |
                (aAdvanceAppUnits << ADVANCE_SHIFT) | aGlyph;
            return *this;
        }
        CompressedGlyph& SetComplex(bool aClusterStart, bool aLigatureStart,
                uint32_t aGlyphCount) {
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_CHAR_IS_SPACE)) |
                FLAG_NOT_MISSING |
                CharIdentityFlags() |
                (aClusterStart ? 0 : FLAG_NOT_CLUSTER_START) |
                (aLigatureStart ? 0 : FLAG_NOT_LIGATURE_GROUP_START) |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        



        CompressedGlyph& SetMissing(uint32_t aGlyphCount) {
            mValue = (mValue & (FLAGS_CAN_BREAK_BEFORE | FLAG_NOT_CLUSTER_START |
                                FLAG_CHAR_IS_SPACE)) |
                CharIdentityFlags() |
                (aGlyphCount << GLYPH_COUNT_SHIFT);
            return *this;
        }
        uint32_t GetGlyphCount() const {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            return (mValue & GLYPH_COUNT_MASK) >> GLYPH_COUNT_SHIFT;
        }

        void SetIsSpace() {
            mValue |= FLAG_CHAR_IS_SPACE;
        }
        void SetIsTab() {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            mValue |= FLAG_CHAR_IS_TAB;
        }
        void SetIsNewline() {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            mValue |= FLAG_CHAR_IS_NEWLINE;
        }
        void SetIsLowSurrogate() {
            NS_ASSERTION(!IsSimpleGlyph(), "Expected non-simple-glyph");
            mValue |= FLAG_CHAR_IS_LOW_SURROGATE;
        }

    private:
        uint32_t mValue;
    };

    
    
    virtual CompressedGlyph *GetCharacterGlyphs() = 0;

    



    struct DetailedGlyph {
        

        uint32_t mGlyphID;
        


   
        int32_t  mAdvance;
        float    mXOffset, mYOffset;
    };

    void SetGlyphs(uint32_t aCharIndex, CompressedGlyph aGlyph,
                   const DetailedGlyph *aGlyphs);

    void SetMissingGlyph(uint32_t aIndex, uint32_t aChar, gfxFont *aFont);

    void SetIsSpace(uint32_t aIndex) {
        GetCharacterGlyphs()[aIndex].SetIsSpace();
    }

    void SetIsLowSurrogate(uint32_t aIndex) {
        SetGlyphs(aIndex, CompressedGlyph().SetComplex(false, false, 0), nullptr);
        GetCharacterGlyphs()[aIndex].SetIsLowSurrogate();
    }

    bool HasDetailedGlyphs() const {
        return mDetailedGlyphs != nullptr;
    }

    bool IsClusterStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return GetCharacterGlyphs()[aPos].IsClusterStart();
    }

    bool IsLigatureGroupStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return GetCharacterGlyphs()[aPos].IsLigatureGroupStart();
    }

    
    
    
    DetailedGlyph *GetDetailedGlyphs(uint32_t aCharIndex) {
        NS_ASSERTION(GetCharacterGlyphs() && HasDetailedGlyphs() &&
                     !GetCharacterGlyphs()[aCharIndex].IsSimpleGlyph() &&
                     GetCharacterGlyphs()[aCharIndex].GetGlyphCount() > 0,
                     "invalid use of GetDetailedGlyphs; check the caller!");
        return mDetailedGlyphs->Get(aCharIndex);
    }

    void AdjustAdvancesForSyntheticBold(float aSynBoldOffset,
                                        uint32_t aOffset, uint32_t aLength);

    
    
    
    void SetupClusterBoundaries(uint32_t         aOffset,
                                const char16_t *aString,
                                uint32_t         aLength);
    
    
    void SetupClusterBoundaries(uint32_t       aOffset,
                                const uint8_t *aString,
                                uint32_t       aLength);

    uint32_t Flags() const {
        return mFlags;
    }

    bool IsRightToLeft() const {
        return (Flags() & gfxTextRunFactory::TEXT_IS_RTL) != 0;
    }

    float GetDirection() const {
        return IsRightToLeft() ? -1.0f : 1.0f;
    }

    bool DisableLigatures() const {
        return (Flags() & gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES) != 0;
    }

    bool TextIs8Bit() const {
        return (Flags() & gfxTextRunFactory::TEXT_IS_8BIT) != 0;
    }

    int32_t GetAppUnitsPerDevUnit() const {
        return mAppUnitsPerDevUnit;
    }

    uint32_t GetLength() const {
        return mLength;
    }

    bool FilterIfIgnorable(uint32_t aIndex, uint32_t aCh);

protected:
    
    DetailedGlyph *AllocateDetailedGlyphs(uint32_t aCharIndex,
                                          uint32_t aCount);

    
    
    
    
    
    
    class DetailedGlyphStore {
    public:
        DetailedGlyphStore()
            : mLastUsed(0)
        { }

        
        
        
        
        
        
        
        
        
        
        
        
        DetailedGlyph* Get(uint32_t aOffset) {
            NS_ASSERTION(mOffsetToIndex.Length() > 0,
                         "no detailed glyph records!");
            DetailedGlyph* details = mDetails.Elements();
            
            if (mLastUsed < mOffsetToIndex.Length() - 1 &&
                aOffset == mOffsetToIndex[mLastUsed + 1].mOffset) {
                ++mLastUsed;
            } else if (aOffset == mOffsetToIndex[0].mOffset) {
                mLastUsed = 0;
            } else if (aOffset == mOffsetToIndex[mLastUsed].mOffset) {
                
            } else if (mLastUsed > 0 &&
                       aOffset == mOffsetToIndex[mLastUsed - 1].mOffset) {
                --mLastUsed;
            } else {
                mLastUsed =
                    mOffsetToIndex.BinaryIndexOf(aOffset, CompareToOffset());
            }
            NS_ASSERTION(mLastUsed != nsTArray<DGRec>::NoIndex,
                         "detailed glyph record missing!");
            return details + mOffsetToIndex[mLastUsed].mIndex;
        }

        DetailedGlyph* Allocate(uint32_t aOffset, uint32_t aCount) {
            uint32_t detailIndex = mDetails.Length();
            DetailedGlyph *details = mDetails.AppendElements(aCount);
            
            
            
            
            if (mOffsetToIndex.Length() == 0 ||
                aOffset > mOffsetToIndex[mOffsetToIndex.Length() - 1].mOffset) {
                mOffsetToIndex.AppendElement(DGRec(aOffset, detailIndex));
            } else {
                mOffsetToIndex.InsertElementSorted(DGRec(aOffset, detailIndex),
                                                   CompareRecordOffsets());
            }
            return details;
        }

        size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf) {
            return aMallocSizeOf(this) +
                mDetails.SizeOfExcludingThis(aMallocSizeOf) +
                mOffsetToIndex.SizeOfExcludingThis(aMallocSizeOf);
        }

    private:
        struct DGRec {
            DGRec(const uint32_t& aOffset, const uint32_t& aIndex)
                : mOffset(aOffset), mIndex(aIndex) { }
            uint32_t mOffset; 
            uint32_t mIndex;  
        };

        struct CompareToOffset {
            bool Equals(const DGRec& a, const uint32_t& b) const {
                return a.mOffset == b;
            }
            bool LessThan(const DGRec& a, const uint32_t& b) const {
                return a.mOffset < b;
            }
        };

        struct CompareRecordOffsets {
            bool Equals(const DGRec& a, const DGRec& b) const {
                return a.mOffset == b.mOffset;
            }
            bool LessThan(const DGRec& a, const DGRec& b) const {
                return a.mOffset < b.mOffset;
            }
        };

        
        
        
        nsTArray<DetailedGlyph>     mDetails;

        
        
        
        nsTArray<DGRec>             mOffsetToIndex;

        
        
        
        nsTArray<DGRec>::index_type mLastUsed;
    };

    nsAutoPtr<DetailedGlyphStore>   mDetailedGlyphs;

    
    uint32_t                        mLength;

    
    uint32_t                        mFlags;

    int32_t                         mAppUnitsPerDevUnit;
};








class gfxShapedWord : public gfxShapedText
{
public:
    
    
    
    
    
    
    
    
    
    static gfxShapedWord* Create(const uint8_t *aText, uint32_t aLength,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags) {
        NS_ASSERTION(aLength <= gfxPlatform::GetPlatform()->WordCacheCharLimit(),
                     "excessive length for gfxShapedWord!");

        
        
        uint32_t size =
            offsetof(gfxShapedWord, mCharGlyphsStorage) +
            aLength * (sizeof(CompressedGlyph) + sizeof(uint8_t));
        void *storage = moz_malloc(size);
        if (!storage) {
            return nullptr;
        }

        
        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    static gfxShapedWord* Create(const char16_t *aText, uint32_t aLength,
                                 int32_t aRunScript,
                                 int32_t aAppUnitsPerDevUnit,
                                 uint32_t aFlags) {
        NS_ASSERTION(aLength <= gfxPlatform::GetPlatform()->WordCacheCharLimit(),
                     "excessive length for gfxShapedWord!");

        
        
        
        if (aFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            nsAutoCString narrowText;
            LossyAppendUTF16toASCII(nsDependentSubstring(aText, aLength),
                                    narrowText);
            return Create((const uint8_t*)(narrowText.BeginReading()),
                          aLength, aRunScript, aAppUnitsPerDevUnit, aFlags);
        }

        uint32_t size =
            offsetof(gfxShapedWord, mCharGlyphsStorage) +
            aLength * (sizeof(CompressedGlyph) + sizeof(char16_t));
        void *storage = moz_malloc(size);
        if (!storage) {
            return nullptr;
        }

        return new (storage) gfxShapedWord(aText, aLength, aRunScript,
                                           aAppUnitsPerDevUnit, aFlags);
    }

    
    
    void operator delete(void* p) {
        moz_free(p);
    }

    CompressedGlyph *GetCharacterGlyphs() {
        return &mCharGlyphsStorage[0];
    }

    const uint8_t* Text8Bit() const {
        NS_ASSERTION(TextIs8Bit(), "invalid use of Text8Bit()");
        return reinterpret_cast<const uint8_t*>(mCharGlyphsStorage + GetLength());
    }

    const char16_t* TextUnicode() const {
        NS_ASSERTION(!TextIs8Bit(), "invalid use of TextUnicode()");
        return reinterpret_cast<const char16_t*>(mCharGlyphsStorage + GetLength());
    }

    char16_t GetCharAt(uint32_t aOffset) const {
        NS_ASSERTION(aOffset < GetLength(), "aOffset out of range");
        return TextIs8Bit() ?
            char16_t(Text8Bit()[aOffset]) : TextUnicode()[aOffset];
    }

    int32_t Script() const {
        return mScript;
    }

    void ResetAge() {
        mAgeCounter = 0;
    }
    uint32_t IncrementAge() {
        return ++mAgeCounter;
    }

private:
    
    friend class gfxTextRun;

    
    gfxShapedWord(const uint8_t *aText, uint32_t aLength,
                  int32_t aRunScript, int32_t aAppUnitsPerDevUnit,
                  uint32_t aFlags)
        : gfxShapedText(aLength, aFlags | gfxTextRunFactory::TEXT_IS_8BIT,
                        aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharGlyphsStorage, 0, aLength * sizeof(CompressedGlyph));
        uint8_t *text = reinterpret_cast<uint8_t*>(&mCharGlyphsStorage[aLength]);
        memcpy(text, aText, aLength * sizeof(uint8_t));
    }

    gfxShapedWord(const char16_t *aText, uint32_t aLength,
                  int32_t aRunScript, int32_t aAppUnitsPerDevUnit,
                  uint32_t aFlags)
        : gfxShapedText(aLength, aFlags, aAppUnitsPerDevUnit)
        , mScript(aRunScript)
        , mAgeCounter(0)
    {
        memset(mCharGlyphsStorage, 0, aLength * sizeof(CompressedGlyph));
        char16_t *text = reinterpret_cast<char16_t*>(&mCharGlyphsStorage[aLength]);
        memcpy(text, aText, aLength * sizeof(char16_t));
        SetupClusterBoundaries(0, aText, aLength);
    }

    int32_t          mScript;

    uint32_t         mAgeCounter;

    
    
    
    
    
    CompressedGlyph  mCharGlyphsStorage[1];
};





struct gfxTextRunDrawCallbacks {

    







    explicit gfxTextRunDrawCallbacks(bool aShouldPaintSVGGlyphs = false)
      : mShouldPaintSVGGlyphs(aShouldPaintSVGGlyphs)
    {
    }

    




    virtual void NotifyGlyphPathEmitted() = 0;

    


    virtual void NotifyBeforeSVGGlyphPainted() { }

    


    virtual void NotifyAfterSVGGlyphPainted() { }

    bool mShouldPaintSVGGlyphs;
};




















class gfxTextRun : public gfxShapedText {
public:

    
    
    void operator delete(void* p) {
        moz_free(p);
    }

    virtual ~gfxTextRun();

    typedef gfxFont::RunMetrics Metrics;

    

    bool IsClusterStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsClusterStart();
    }
    bool IsLigatureGroupStart(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].IsLigatureGroupStart();
    }
    bool CanBreakLineBefore(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore() ==
            CompressedGlyph::FLAG_BREAK_TYPE_NORMAL;
    }
    bool CanHyphenateBefore(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CanBreakBefore() ==
            CompressedGlyph::FLAG_BREAK_TYPE_HYPHEN;
    }

    bool CharIsSpace(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsSpace();
    }
    bool CharIsTab(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsTab();
    }
    bool CharIsNewline(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsNewline();
    }
    bool CharIsLowSurrogate(uint32_t aPos) {
        NS_ASSERTION(aPos < GetLength(), "aPos out of range");
        return mCharacterGlyphs[aPos].CharIsLowSurrogate();
    }

    uint32_t GetLength() { return mLength; }

    
    
    
    
    

    











    virtual bool SetPotentialLineBreaks(uint32_t aStart, uint32_t aLength,
                                          uint8_t *aBreakBefore,
                                          gfxContext *aRefContext);

    









    class PropertyProvider {
    public:
        
        
        virtual void GetHyphenationBreaks(uint32_t aStart, uint32_t aLength,
                                          bool *aBreakBefore) = 0;

        
        
        
        virtual int8_t GetHyphensOption() = 0;

        
        
        virtual gfxFloat GetHyphenWidth() = 0;

        typedef gfxFont::Spacing Spacing;

        





        virtual void GetSpacing(uint32_t aStart, uint32_t aLength,
                                Spacing *aSpacing) = 0;

        
        
        virtual already_AddRefed<gfxContext> GetContext() = 0;

        
        
        virtual uint32_t GetAppUnitsPerDevUnit() = 0;
    };

    class ClusterIterator {
    public:
        explicit ClusterIterator(gfxTextRun *aTextRun);

        void Reset();

        bool NextCluster();

        uint32_t Position() const {
            return mCurrentChar;
        }

        uint32_t ClusterLength() const;

        gfxFloat ClusterAdvance(PropertyProvider *aProvider) const;

    private:
        gfxTextRun *mTextRun;
        uint32_t    mCurrentChar;
    };

    




















    void Draw(gfxContext *aContext, gfxPoint aPt,
              DrawMode aDrawMode,
              uint32_t aStart, uint32_t aLength,
              PropertyProvider *aProvider,
              gfxFloat *aAdvanceWidth, gfxTextContextPaint *aContextPaint,
              gfxTextRunDrawCallbacks *aCallbacks = nullptr);

    




    Metrics MeasureText(uint32_t aStart, uint32_t aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContextForTightBoundingBox,
                        PropertyProvider *aProvider);

    



    gfxFloat GetAdvanceWidth(uint32_t aStart, uint32_t aLength,
                             PropertyProvider *aProvider);

    


























    virtual bool SetLineBreaks(uint32_t aStart, uint32_t aLength,
                                 bool aLineBreakBefore, bool aLineBreakAfter,
                                 gfxFloat *aAdvanceWidthDelta,
                                 gfxContext *aRefContext);

    

























































    uint32_t BreakAndMeasureText(uint32_t aStart, uint32_t aMaxLength,
                                 bool aLineBreakBefore, gfxFloat aWidth,
                                 PropertyProvider *aProvider,
                                 bool aSuppressInitialBreak,
                                 gfxFloat *aTrimWhitespace,
                                 Metrics *aMetrics,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContextForTightBoundingBox,
                                 bool *aUsedHyphenation,
                                 uint32_t *aLastBreak,
                                 bool aCanWordWrap,
                                 gfxBreakPriority *aBreakPriority);

    




    void SetContext(gfxContext *aContext) {}

    

    gfxFloat GetDirection() const { return (mFlags & gfxTextRunFactory::TEXT_IS_RTL) ? -1.0 : 1.0; }
    void *GetUserData() const { return mUserData; }
    void SetUserData(void *aUserData) { mUserData = aUserData; }
    uint32_t GetFlags() const { return mFlags; }
    void SetFlagBits(uint32_t aFlags) {
      NS_ASSERTION(!(aFlags & ~gfxTextRunFactory::SETTABLE_FLAGS),
                   "Only user flags should be mutable");
      mFlags |= aFlags;
    }
    void ClearFlagBits(uint32_t aFlags) {
      NS_ASSERTION(!(aFlags & ~gfxTextRunFactory::SETTABLE_FLAGS),
                   "Only user flags should be mutable");
      mFlags &= ~aFlags;
    }
    const gfxSkipChars& GetSkipChars() const { return mSkipChars; }
    gfxFontGroup *GetFontGroup() const { return mFontGroup; }


    
    
    static gfxTextRun *Create(const gfxTextRunFactory::Parameters *aParams,
                              uint32_t aLength, gfxFontGroup *aFontGroup,
                              uint32_t aFlags);

    
    struct GlyphRun {
        nsRefPtr<gfxFont> mFont;   
        uint32_t          mCharacterOffset; 
        uint8_t           mMatchType;
    };

    class GlyphRunIterator {
    public:
        GlyphRunIterator(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aLength)
          : mTextRun(aTextRun), mStartOffset(aStart), mEndOffset(aStart + aLength) {
            mNextIndex = mTextRun->FindFirstGlyphRunContaining(aStart);
        }
        bool NextRun();
        GlyphRun *GetGlyphRun() { return mGlyphRun; }
        uint32_t GetStringStart() { return mStringStart; }
        uint32_t GetStringEnd() { return mStringEnd; }
    private:
        gfxTextRun *mTextRun;
        GlyphRun   *mGlyphRun;
        uint32_t    mStringStart;
        uint32_t    mStringEnd;
        uint32_t    mNextIndex;
        uint32_t    mStartOffset;
        uint32_t    mEndOffset;
    };

    class GlyphRunOffsetComparator {
    public:
        bool Equals(const GlyphRun& a,
                      const GlyphRun& b) const
        {
            return a.mCharacterOffset == b.mCharacterOffset;
        }

        bool LessThan(const GlyphRun& a,
                        const GlyphRun& b) const
        {
            return a.mCharacterOffset < b.mCharacterOffset;
        }
    };

    friend class GlyphRunIterator;
    friend class FontSelector;

    
    
    












    nsresult AddGlyphRun(gfxFont *aFont, uint8_t aMatchType,
                         uint32_t aStartCharIndex, bool aForceNewRun);
    void ResetGlyphRuns() { mGlyphRuns.Clear(); }
    void SortGlyphRuns();
    void SanitizeGlyphRuns();

    CompressedGlyph* GetCharacterGlyphs() {
        NS_ASSERTION(mCharacterGlyphs, "failed to initialize mCharacterGlyphs");
        return mCharacterGlyphs;
    }

    
    void ClearGlyphsAndCharacters();

    void SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext, uint32_t aCharIndex);

    
    
    
    
    
    
    
    
    
    
    
    
    
    bool SetSpaceGlyphIfSimple(gfxFont *aFont, gfxContext *aContext,
                               uint32_t aCharIndex, char16_t aSpaceChar);

    
    
    
    
    
    
    void SetIsTab(uint32_t aIndex) {
        CompressedGlyph *g = &mCharacterGlyphs[aIndex];
        if (g->IsSimpleGlyph()) {
            DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
            details->mGlyphID = g->GetSimpleGlyph();
            details->mAdvance = g->GetSimpleAdvance();
            details->mXOffset = details->mYOffset = 0;
            SetGlyphs(aIndex, CompressedGlyph().SetComplex(true, true, 1), details);
        }
        g->SetIsTab();
    }
    void SetIsNewline(uint32_t aIndex) {
        CompressedGlyph *g = &mCharacterGlyphs[aIndex];
        if (g->IsSimpleGlyph()) {
            DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
            details->mGlyphID = g->GetSimpleGlyph();
            details->mAdvance = g->GetSimpleAdvance();
            details->mXOffset = details->mYOffset = 0;
            SetGlyphs(aIndex, CompressedGlyph().SetComplex(true, true, 1), details);
        }
        g->SetIsNewline();
    }
    void SetIsLowSurrogate(uint32_t aIndex) {
        SetGlyphs(aIndex, CompressedGlyph().SetComplex(false, false, 0), nullptr);
        mCharacterGlyphs[aIndex].SetIsLowSurrogate();
    }

    





    void FetchGlyphExtents(gfxContext *aRefContext);

    uint32_t CountMissingGlyphs();
    const GlyphRun *GetGlyphRuns(uint32_t *aNumGlyphRuns) {
        *aNumGlyphRuns = mGlyphRuns.Length();
        return mGlyphRuns.Elements();
    }
    
    
    uint32_t FindFirstGlyphRunContaining(uint32_t aOffset);

    
    void CopyGlyphDataFrom(gfxShapedWord *aSource, uint32_t aStart);

    
    
    void CopyGlyphDataFrom(gfxTextRun *aSource, uint32_t aStart,
                           uint32_t aLength, uint32_t aDest);

    nsExpirationState *GetExpirationState() { return &mExpirationState; }

    
    
    
    
    
    
    void ReleaseFontGroup();

    struct LigatureData {
        
        uint32_t mLigatureStart;
        uint32_t mLigatureEnd;
        
        
        gfxFloat mPartAdvance;
        
        
        
        gfxFloat mPartWidth;
        
        bool mClipBeforePart;
        bool mClipAfterPart;
    };
    
    
    
    virtual size_t SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf)
      MOZ_MUST_OVERRIDE;
    virtual size_t SizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)
      MOZ_MUST_OVERRIDE;

    
    size_t MaybeSizeOfIncludingThis(mozilla::MallocSizeOf aMallocSizeOf)  {
        if (mFlags & gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED) {
            return 0;
        }
        mFlags |= gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED;
        return SizeOfIncludingThis(aMallocSizeOf);
    }
    void ResetSizeOfAccountingFlags() {
        mFlags &= ~gfxTextRunFactory::TEXT_RUN_SIZE_ACCOUNTED;
    }

    
    
    

    enum ShapingState {
        eShapingState_Normal,                 
        eShapingState_ShapingWithFeature,     
        eShapingState_ShapingWithFallback,    
        eShapingState_Aborted,                
        eShapingState_ForceFallbackFeature    
    };

    ShapingState GetShapingState() const { return mShapingState; }
    void SetShapingState(ShapingState aShapingState) {
        mShapingState = aShapingState;
    }

#ifdef DEBUG
    void Dump(FILE* aOutput);
#endif

protected:
    





    gfxTextRun(const gfxTextRunFactory::Parameters *aParams,
               uint32_t aLength, gfxFontGroup *aFontGroup, uint32_t aFlags);

    




    static void* AllocateStorageForTextRun(size_t aSize, uint32_t aLength);

    
    
    CompressedGlyph *mCharacterGlyphs;

private:
    

    
    int32_t GetAdvanceForGlyphs(uint32_t aStart, uint32_t aEnd);

    
    
    
    
    bool GetAdjustedSpacingArray(uint32_t aStart, uint32_t aEnd,
                                   PropertyProvider *aProvider,
                                   uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                   nsTArray<PropertyProvider::Spacing> *aSpacing);

    
    
    

    
    LigatureData ComputeLigatureData(uint32_t aPartStart, uint32_t aPartEnd,
                                     PropertyProvider *aProvider);
    gfxFloat ComputePartialLigatureWidth(uint32_t aPartStart, uint32_t aPartEnd,
                                         PropertyProvider *aProvider);
    void DrawPartialLigature(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                             gfxPoint *aPt, PropertyProvider *aProvider,
                             TextRunDrawParams& aParams);
    
    
    void ShrinkToLigatureBoundaries(uint32_t *aStart, uint32_t *aEnd);
    
    gfxFloat GetPartialLigatureWidth(uint32_t aStart, uint32_t aEnd, PropertyProvider *aProvider);
    void AccumulatePartialLigatureMetrics(gfxFont *aFont,
                                          uint32_t aStart, uint32_t aEnd,
                                          gfxFont::BoundingBoxType aBoundingBoxType,
                                          gfxContext *aRefContext,
                                          PropertyProvider *aProvider,
                                          Metrics *aMetrics);

    
    void AccumulateMetricsForRun(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                                 gfxFont::BoundingBoxType aBoundingBoxType,
                                 gfxContext *aRefContext,
                                 PropertyProvider *aProvider,
                                 uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                 Metrics *aMetrics);

    
    void DrawGlyphs(gfxFont *aFont, uint32_t aStart, uint32_t aEnd,
                    gfxPoint *aPt, PropertyProvider *aProvider,
                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                    TextRunDrawParams& aParams);

    
    
    nsAutoTArray<GlyphRun,1>        mGlyphRuns;

    void             *mUserData;
    gfxFontGroup     *mFontGroup; 
                                  
    gfxSkipChars      mSkipChars;
    nsExpirationState mExpirationState;

    bool              mSkipDrawing; 
                                    
                                    
    bool              mReleasedFontGroup; 
                                          

    
    
    ShapingState      mShapingState;
};

class gfxFontGroup : public gfxTextRunFactory {
public:
    class FamilyFace {
    public:
        FamilyFace() { }

        FamilyFace(gfxFontFamily* aFamily, gfxFont* aFont)
            : mFamily(aFamily), mFont(aFont)
        {
            NS_ASSERTION(aFont, "font pointer must not be null");
            NS_ASSERTION(!aFamily ||
                         aFamily->ContainsFace(aFont->GetFontEntry()),
                         "font is not a member of the given family");
        }

        gfxFontFamily* Family() const { return mFamily.get(); }
        gfxFont* Font() const { return mFont.get(); }

    private:
        nsRefPtr<gfxFontFamily> mFamily;
        nsRefPtr<gfxFont>       mFont;
    };

    static void Shutdown(); 

    gfxFontGroup(const mozilla::FontFamilyList& aFontFamilyList,
                 const gfxFontStyle *aStyle,
                 gfxUserFontSet *aUserFontSet = nullptr);

    virtual ~gfxFontGroup();

    virtual gfxFont *GetFontAt(int32_t i) {
        
        
        
        
        NS_ASSERTION(!mUserFontSet || mCurrGeneration == GetGeneration(),
                     "Whoever was caching this font group should have "
                     "called UpdateFontList on it");
        NS_ASSERTION(mFonts.Length() > uint32_t(i) && mFonts[i].Font(), 
                     "Requesting a font index that doesn't exist");

        return mFonts[i].Font();
    }

    
    
    
    gfxFont *GetFirstMathFont();

    uint32_t FontListLength() const {
        return mFonts.Length();
    }

    const gfxFontStyle *GetStyle() const { return &mStyle; }

    virtual gfxFontGroup *Copy(const gfxFontStyle *aStyle);

    



    static bool IsInvalidChar(uint8_t ch);
    static bool IsInvalidChar(char16_t ch);

    





    virtual gfxTextRun *MakeTextRun(const char16_t *aString, uint32_t aLength,
                                    const Parameters *aParams, uint32_t aFlags);
    





    virtual gfxTextRun *MakeTextRun(const uint8_t *aString, uint32_t aLength,
                                    const Parameters *aParams, uint32_t aFlags);

    



    template<typename T>
    gfxTextRun *MakeTextRun(const T *aString, uint32_t aLength,
                            gfxContext *aRefContext,
                            int32_t aAppUnitsPerDevUnit,
                            uint32_t aFlags)
    {
        gfxTextRunFactory::Parameters params = {
            aRefContext, nullptr, nullptr, nullptr, 0, aAppUnitsPerDevUnit
        };
        return MakeTextRun(aString, aLength, &params, aFlags);
    }

    





    gfxFloat GetHyphenWidth(gfxTextRun::PropertyProvider* aProvider);

    






    gfxTextRun *MakeHyphenTextRun(gfxContext *aCtx,
                                  uint32_t aAppUnitsPerDevUnit);

    



    bool HasFont(const gfxFontEntry *aFontEntry);

    
    
    
    
    
    enum { UNDERLINE_OFFSET_NOT_SET = INT16_MAX };
    virtual gfxFloat GetUnderlineOffset() {
        if (mUnderlineOffset == UNDERLINE_OFFSET_NOT_SET)
            mUnderlineOffset = GetFontAt(0)->GetMetrics().underlineOffset;
        return mUnderlineOffset;
    }

    virtual already_AddRefed<gfxFont>
        FindFontForChar(uint32_t ch, uint32_t prevCh, int32_t aRunScript,
                        gfxFont *aPrevMatchedFont,
                        uint8_t *aMatchType);

    
    virtual already_AddRefed<gfxFont> WhichPrefFontSupportsChar(uint32_t aCh);

    virtual already_AddRefed<gfxFont>
        WhichSystemFontSupportsChar(uint32_t aCh, int32_t aRunScript);

    template<typename T>
    void ComputeRanges(nsTArray<gfxTextRange>& mRanges,
                       const T *aString, uint32_t aLength,
                       int32_t aRunScript);

    gfxUserFontSet* GetUserFontSet();

    
    
    
    
    uint64_t GetGeneration();

    
    gfxTextPerfMetrics *GetTextPerfMetrics() { return mTextPerf; }
    void SetTextPerfMetrics(gfxTextPerfMetrics *aTextPerf) { mTextPerf = aTextPerf; }

    
    void SetUserFontSet(gfxUserFontSet *aUserFontSet);

    
    
    virtual void UpdateFontList();

    bool ShouldSkipDrawing() const {
        return mSkipDrawing;
    }

    class LazyReferenceContextGetter {
    public:
      virtual already_AddRefed<gfxContext> GetRefContext() = 0;
    };
    
    
    
    
    
    gfxTextRun* GetEllipsisTextRun(int32_t aAppUnitsPerDevPixel,
                                   LazyReferenceContextGetter& aRefContextGetter);

    
    static void
    ResolveGenericFontNames(mozilla::FontFamilyType aGenericType,
                            nsIAtom *aLanguage,
                            nsTArray<nsString>& aGenericFamilies);

protected:
    mozilla::FontFamilyList mFamilyList;
    gfxFontStyle mStyle;
    nsTArray<FamilyFace> mFonts;
    gfxFloat mUnderlineOffset;
    gfxFloat mHyphenWidth;

    nsRefPtr<gfxUserFontSet> mUserFontSet;
    uint64_t mCurrGeneration;  

    gfxTextPerfMetrics *mTextPerf;

    
    
    nsAutoPtr<gfxTextRun>   mCachedEllipsisTextRun;

    
    nsRefPtr<gfxFontFamily> mLastPrefFamily;
    nsRefPtr<gfxFont>       mLastPrefFont;
    eFontPrefLang           mLastPrefLang;       
    eFontPrefLang           mPageLang;
    bool                    mLastPrefFirstFont;  

    bool                    mSkipDrawing; 
                                          
                                          

    



    gfxTextRun *MakeEmptyTextRun(const Parameters *aParams, uint32_t aFlags);
    gfxTextRun *MakeSpaceTextRun(const Parameters *aParams, uint32_t aFlags);
    gfxTextRun *MakeBlankTextRun(uint32_t aLength,
                                 const Parameters *aParams, uint32_t aFlags);

    
    void BuildFontList();

    
    
    
    void InitMetricsForBadFont(gfxFont* aBadFont);

    
    
    template<typename T>
    void InitTextRun(gfxContext *aContext,
                     gfxTextRun *aTextRun,
                     const T *aString,
                     uint32_t aLength);

    
    
    template<typename T>
    void InitScriptRun(gfxContext *aContext,
                       gfxTextRun *aTextRun,
                       const T *aString,
                       uint32_t aScriptRunStart,
                       uint32_t aScriptRunEnd,
                       int32_t aRunScript);

    
    
    
    already_AddRefed<gfxFont> TryAllFamilyMembers(gfxFontFamily* aFamily,
                                                  uint32_t aCh);

    

    
    void EnumerateFontList(nsIAtom *aLanguage, void *aClosure = nullptr);

    
    void FindGenericFonts(mozilla::FontFamilyType aGenericType,
                          nsIAtom *aLanguage,
                          void *aClosure);

    
    virtual void FindPlatformFont(const nsAString& aName,
                                  bool aUseFontSet,
                                  void *aClosure);

    static nsILanguageAtomService* gLangService;
};
#endif
