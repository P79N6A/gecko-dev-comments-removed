




#ifndef GFX_FONTENTRY_H
#define GFX_FONTENTRY_H

#include "gfxTypes.h"
#include "nsString.h"
#include "gfxFontFeatures.h"
#include "gfxFontUtils.h"
#include "nsTArray.h"
#include "nsTHashtable.h"
#include "mozilla/HashFunctions.h"
#include "mozilla/MemoryReporting.h"
#include "DrawMode.h"
#include "nsUnicodeScriptCodes.h"
#include "nsDataHashtable.h"
#include "harfbuzz/hb.h"
#include "mozilla/gfx/2D.h"

typedef struct gr_face gr_face;

#ifdef DEBUG
#include <stdio.h>
#endif

struct gfxFontStyle;
class gfxContext;
class gfxFont;
class gfxFontFamily;
class gfxUserFontData;
class gfxSVGGlyphs;
class gfxMathTable;
class gfxTextContextPaint;
class FontInfoData;
struct FontListSizes;
class nsIAtom;

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

#endif
