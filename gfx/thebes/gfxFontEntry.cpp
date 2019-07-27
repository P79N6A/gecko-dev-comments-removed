




#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "nsServiceManagerUtils.h"
#include "nsExpirationTracker.h"
#include "nsILanguageAtomService.h"
#include "nsITimer.h"

#include "gfxFontEntry.h"
#include "gfxTextRun.h"
#include "gfxPlatform.h"
#include "nsGkAtoms.h"

#include "gfxTypes.h"
#include "gfxContext.h"
#include "gfxFontConstants.h"
#include "gfxHarfBuzzShaper.h"
#include "gfxUserFontSet.h"
#include "gfxPlatformFontList.h"
#include "nsUnicodeProperties.h"
#include "nsMathUtils.h"
#include "nsBidiUtils.h"
#include "nsUnicodeRange.h"
#include "nsStyleConsts.h"
#include "mozilla/AppUnits.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Likely.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "gfxSVGGlyphs.h"
#include "gfxMathTable.h"
#include "gfx2DGlue.h"

#if defined(XP_MACOSX)
#include "nsCocoaFeatures.h"
#endif

#include "cairo.h"

#include "harfbuzz/hb.h"
#include "harfbuzz/hb-ot.h"
#include "graphite2/Font.h"

#include <algorithm>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::unicode;
using mozilla::services::GetObserverService;

void
gfxCharacterMap::NotifyReleased()
{
    gfxPlatformFontList *fontlist = gfxPlatformFontList::PlatformFontList();
    if (mShared) {
        fontlist->RemoveCmap(this);
    }
    delete this;
}

gfxFontEntry::gfxFontEntry() :
    mItalic(false), mFixedPitch(false),
    mIsValid(true),
    mIsBadUnderlineFont(false),
    mIsUserFontContainer(false),
    mIsDataUserFont(false),
    mIsLocalUserFont(false),
    mStandardFace(false),
    mSymbolFont(false),
    mIgnoreGDEF(false),
    mIgnoreGSUB(false),
    mSVGInitialized(false),
    mMathInitialized(false),
    mHasSpaceFeaturesInitialized(false),
    mHasSpaceFeatures(false),
    mHasSpaceFeaturesKerning(false),
    mHasSpaceFeaturesNonKerning(false),
    mSkipDefaultFeatureSpaceCheck(false),
    mCheckedForGraphiteTables(false),
    mHasCmapTable(false),
    mGrFaceInitialized(false),
    mCheckedForColorGlyph(false),
    mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
    mUVSOffset(0), mUVSData(nullptr),
    mLanguageOverride(NO_FONT_LANGUAGE_OVERRIDE),
    mCOLR(nullptr),
    mCPAL(nullptr),
    mUnitsPerEm(0),
    mHBFace(nullptr),
    mGrFace(nullptr),
    mGrFaceRefCnt(0)
{
    memset(&mDefaultSubSpaceFeatures, 0, sizeof(mDefaultSubSpaceFeatures));
    memset(&mNonDefaultSubSpaceFeatures, 0, sizeof(mNonDefaultSubSpaceFeatures));
}

gfxFontEntry::gfxFontEntry(const nsAString& aName, bool aIsStandardFace) :
    mName(aName), mItalic(false), mFixedPitch(false),
    mIsValid(true),
    mIsBadUnderlineFont(false),
    mIsUserFontContainer(false),
    mIsDataUserFont(false),
    mIsLocalUserFont(false), mStandardFace(aIsStandardFace),
    mSymbolFont(false),
    mIgnoreGDEF(false),
    mIgnoreGSUB(false),
    mSVGInitialized(false),
    mMathInitialized(false),
    mHasSpaceFeaturesInitialized(false),
    mHasSpaceFeatures(false),
    mHasSpaceFeaturesKerning(false),
    mHasSpaceFeaturesNonKerning(false),
    mSkipDefaultFeatureSpaceCheck(false),
    mCheckedForGraphiteTables(false),
    mHasCmapTable(false),
    mGrFaceInitialized(false),
    mCheckedForColorGlyph(false),
    mWeight(500), mStretch(NS_FONT_STRETCH_NORMAL),
    mUVSOffset(0), mUVSData(nullptr),
    mLanguageOverride(NO_FONT_LANGUAGE_OVERRIDE),
    mCOLR(nullptr),
    mCPAL(nullptr),
    mUnitsPerEm(0),
    mHBFace(nullptr),
    mGrFace(nullptr),
    mGrFaceRefCnt(0)
{
    memset(&mDefaultSubSpaceFeatures, 0, sizeof(mDefaultSubSpaceFeatures));
    memset(&mNonDefaultSubSpaceFeatures, 0, sizeof(mNonDefaultSubSpaceFeatures));
}

static PLDHashOperator
DestroyHBSet(const uint32_t& aTag, hb_set_t*& aSet, void *aUserArg)
{
    hb_set_destroy(aSet);
    return PL_DHASH_NEXT;
}

gfxFontEntry::~gfxFontEntry()
{
    if (mCOLR) {
        hb_blob_destroy(mCOLR);
    }

    if (mCPAL) {
        hb_blob_destroy(mCPAL);
    }

    
    
    if (mIsDataUserFont) {
        gfxUserFontSet::UserFontCache::ForgetFont(this);
    }

    if (mFeatureInputs) {
        mFeatureInputs->Enumerate(DestroyHBSet, nullptr);
    }

    
    
    
    MOZ_ASSERT(!mHBFace);
    MOZ_ASSERT(!mGrFaceInitialized);
}

bool gfxFontEntry::IsSymbolFont() 
{
    return mSymbolFont;
}

bool gfxFontEntry::TestCharacterMap(uint32_t aCh)
{
    if (!mCharacterMap) {
        ReadCMAP();
        NS_ASSERTION(mCharacterMap, "failed to initialize character map");
    }
    return mCharacterMap->test(aCh);
}

nsresult gfxFontEntry::InitializeUVSMap()
{
    
    
    if (!mCharacterMap) {
        ReadCMAP();
        NS_ASSERTION(mCharacterMap, "failed to initialize character map");
    }

    if (!mUVSOffset) {
        return NS_ERROR_FAILURE;
    }

    if (!mUVSData) {
        const uint32_t kCmapTag = TRUETYPE_TAG('c','m','a','p');
        AutoTable cmapTable(this, kCmapTag);
        if (!cmapTable) {
            mUVSOffset = 0; 
            return NS_ERROR_FAILURE;
        }

        uint8_t* uvsData;
        unsigned int cmapLen;
        const char* cmapData = hb_blob_get_data(cmapTable, &cmapLen);
        nsresult rv = gfxFontUtils::ReadCMAPTableFormat14(
                          (const uint8_t*)cmapData + mUVSOffset,
                          cmapLen - mUVSOffset, uvsData);

        if (NS_FAILED(rv)) {
            mUVSOffset = 0; 
            return rv;
        }

        mUVSData = uvsData;
    }

    return NS_OK;
}

uint16_t gfxFontEntry::GetUVSGlyph(uint32_t aCh, uint32_t aVS)
{
    InitializeUVSMap();

    if (mUVSData) {
        return gfxFontUtils::MapUVSToGlyphFormat14(mUVSData, aCh, aVS);
    }

    return 0;
}

bool gfxFontEntry::SupportsScriptInGSUB(const hb_tag_t* aScriptTags)
{
    hb_face_t *face = GetHBFace();
    if (!face) {
        return false;
    }

    unsigned int index;
    hb_tag_t     chosenScript;
    bool found =
        hb_ot_layout_table_choose_script(face, TRUETYPE_TAG('G','S','U','B'),
                                         aScriptTags, &index, &chosenScript);
    hb_face_destroy(face);

    return found && chosenScript != TRUETYPE_TAG('D','F','L','T');
}

nsresult gfxFontEntry::ReadCMAP(FontInfoData *aFontInfoData)
{
    NS_ASSERTION(false, "using default no-op implementation of ReadCMAP");
    mCharacterMap = new gfxCharacterMap();
    return NS_OK;
}

nsString
gfxFontEntry::RealFaceName()
{
    AutoTable nameTable(this, TRUETYPE_TAG('n','a','m','e'));
    if (nameTable) {
        nsAutoString name;
        nsresult rv = gfxFontUtils::GetFullNameFromTable(nameTable, name);
        if (NS_SUCCEEDED(rv)) {
            return name;
        }
    }
    return Name();
}

already_AddRefed<gfxFont>
gfxFontEntry::FindOrMakeFont(const gfxFontStyle *aStyle, bool aNeedsBold)
{
    
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(this, aStyle);

    if (!font) {
        gfxFont *newFont = CreateFontInstance(aStyle, aNeedsBold);
        if (!newFont)
            return nullptr;
        if (!newFont->Valid()) {
            delete newFont;
            return nullptr;
        }
        font = newFont;
        gfxFontCache::GetCache()->AddNew(font);
    }
    return font.forget();
}

uint16_t
gfxFontEntry::UnitsPerEm()
{
    if (!mUnitsPerEm) {
        AutoTable headTable(this, TRUETYPE_TAG('h','e','a','d'));
        if (headTable) {
            uint32_t len;
            const HeadTable* head =
                reinterpret_cast<const HeadTable*>(hb_blob_get_data(headTable,
                                                                    &len));
            if (len >= sizeof(HeadTable)) {
                mUnitsPerEm = head->unitsPerEm;
            }
        }

        
        
        if (mUnitsPerEm < kMinUPEM || mUnitsPerEm > kMaxUPEM) {
            mUnitsPerEm = kInvalidUPEM;
        }
    }
    return mUnitsPerEm;
}

bool
gfxFontEntry::HasSVGGlyph(uint32_t aGlyphId)
{
    NS_ASSERTION(mSVGInitialized, "SVG data has not yet been loaded. TryGetSVGData() first.");
    return mSVGGlyphs->HasSVGGlyph(aGlyphId);
}

bool
gfxFontEntry::GetSVGGlyphExtents(gfxContext *aContext, uint32_t aGlyphId,
                                 gfxRect *aResult)
{
    NS_ABORT_IF_FALSE(mSVGInitialized,
                      "SVG data has not yet been loaded. TryGetSVGData() first.");
    NS_ABORT_IF_FALSE(mUnitsPerEm >= kMinUPEM && mUnitsPerEm <= kMaxUPEM,
                      "font has invalid unitsPerEm");

    gfxContextAutoSaveRestore matrixRestore(aContext);
    cairo_matrix_t fontMatrix;
    cairo_get_font_matrix(aContext->GetCairo(), &fontMatrix);

    gfxMatrix svgToAppSpace = *reinterpret_cast<gfxMatrix*>(&fontMatrix);
    svgToAppSpace.Scale(1.0f / mUnitsPerEm, 1.0f / mUnitsPerEm);

    return mSVGGlyphs->GetGlyphExtents(aGlyphId, svgToAppSpace, aResult);
}

bool
gfxFontEntry::RenderSVGGlyph(gfxContext *aContext, uint32_t aGlyphId,
                             int aDrawMode, gfxTextContextPaint *aContextPaint)
{
    NS_ASSERTION(mSVGInitialized, "SVG data has not yet been loaded. TryGetSVGData() first.");
    return mSVGGlyphs->RenderGlyph(aContext, aGlyphId, DrawMode(aDrawMode),
                                   aContextPaint);
}

bool
gfxFontEntry::TryGetSVGData(gfxFont* aFont)
{
    if (!gfxPlatform::GetPlatform()->OpenTypeSVGEnabled()) {
        return false;
    }

    if (!mSVGInitialized) {
        mSVGInitialized = true;

        
        if (UnitsPerEm() == kInvalidUPEM) {
            return false;
        }

        
        
        hb_blob_t *svgTable = GetFontTable(TRUETYPE_TAG('S','V','G',' '));
        if (!svgTable) {
            return false;
        }

        
        
        mSVGGlyphs = new gfxSVGGlyphs(svgTable, this);
    }

    if (!mFontsUsingSVGGlyphs.Contains(aFont)) {
        mFontsUsingSVGGlyphs.AppendElement(aFont);
    }

    return !!mSVGGlyphs;
}

void
gfxFontEntry::NotifyFontDestroyed(gfxFont* aFont)
{
    mFontsUsingSVGGlyphs.RemoveElement(aFont);
}

void
gfxFontEntry::NotifyGlyphsChanged()
{
    for (uint32_t i = 0, count = mFontsUsingSVGGlyphs.Length(); i < count; ++i) {
        gfxFont* font = mFontsUsingSVGGlyphs[i];
        font->NotifyGlyphsChanged();
    }
}

bool
gfxFontEntry::TryGetMathTable()
{
    if (!mMathInitialized) {
        mMathInitialized = true;

        
        if (UnitsPerEm() == kInvalidUPEM) {
            return false;
        }

        
        
        hb_blob_t *mathTable = GetFontTable(TRUETYPE_TAG('M','A','T','H'));
        if (!mathTable) {
            return false;
        }

        
        
        mMathTable = new gfxMathTable(mathTable);
        if (!mMathTable->HasValidHeaders()) {
            mMathTable = nullptr;
            return false;
        }
    }

    return !!mMathTable;
}

gfxFloat
gfxFontEntry::GetMathConstant(gfxFontEntry::MathConstant aConstant)
{
    NS_ASSERTION(mMathTable, "Math data has not yet been loaded. TryGetMathData() first.");
    gfxFloat value = mMathTable->GetMathConstant(aConstant);
    if (aConstant == gfxFontEntry::ScriptPercentScaleDown ||
        aConstant == gfxFontEntry::ScriptScriptPercentScaleDown ||
        aConstant == gfxFontEntry::RadicalDegreeBottomRaisePercent) {
        return value / 100.0;
    }
    return value / mUnitsPerEm;
}

bool
gfxFontEntry::GetMathItalicsCorrection(uint32_t aGlyphID,
                                       gfxFloat* aItalicCorrection)
{
    NS_ASSERTION(mMathTable, "Math data has not yet been loaded. TryGetMathData() first.");
    int16_t italicCorrection;
    if (!mMathTable->GetMathItalicsCorrection(aGlyphID, &italicCorrection)) {
        return false;
    }
    *aItalicCorrection = gfxFloat(italicCorrection) / mUnitsPerEm;
    return true;
}

uint32_t
gfxFontEntry::GetMathVariantsSize(uint32_t aGlyphID, bool aVertical,
                                  uint16_t aSize)
{
    NS_ASSERTION(mMathTable, "Math data has not yet been loaded. TryGetMathData() first.");
    return mMathTable->GetMathVariantsSize(aGlyphID, aVertical, aSize);
}

bool
gfxFontEntry::GetMathVariantsParts(uint32_t aGlyphID, bool aVertical,
                                   uint32_t aGlyphs[4])
{
    NS_ASSERTION(mMathTable, "Math data has not yet been loaded. TryGetMathData() first.");
    return mMathTable->GetMathVariantsParts(aGlyphID, aVertical, aGlyphs);
}

bool
gfxFontEntry::TryGetColorGlyphs()
{
    if (mCheckedForColorGlyph) {
        return (mCOLR && mCPAL);
    }

    mCheckedForColorGlyph = true;

    mCOLR = GetFontTable(TRUETYPE_TAG('C', 'O', 'L', 'R'));
    if (!mCOLR) {
        return false;
    }

    mCPAL = GetFontTable(TRUETYPE_TAG('C', 'P', 'A', 'L'));
    if (!mCPAL) {
        hb_blob_destroy(mCOLR);
        mCOLR = nullptr;
        return false;
    }

    
    if (gfxFontUtils::ValidateColorGlyphs(mCOLR, mCPAL)) {
        return true;
    }

    hb_blob_destroy(mCOLR);
    hb_blob_destroy(mCPAL);
    mCOLR = nullptr;
    mCPAL = nullptr;
    return false;
}







class gfxFontEntry::FontTableBlobData {
public:
    
    explicit FontTableBlobData(FallibleTArray<uint8_t>& aBuffer)
        : mHashtable(nullptr), mHashKey(0)
    {
        MOZ_COUNT_CTOR(FontTableBlobData);
        mTableData.SwapElements(aBuffer);
    }

    ~FontTableBlobData() {
        MOZ_COUNT_DTOR(FontTableBlobData);
        if (mHashtable && mHashKey) {
            mHashtable->RemoveEntry(mHashKey);
        }
    }

    
    const char *GetTable() const
    {
        return reinterpret_cast<const char*>(mTableData.Elements());
    }
    uint32_t GetTableLength() const { return mTableData.Length(); }

    
    
    void ManageHashEntry(nsTHashtable<FontTableHashEntry> *aHashtable,
                         uint32_t aHashKey)
    {
        mHashtable = aHashtable;
        mHashKey = aHashKey;
    }

    
    
    void ForgetHashEntry()
    {
        mHashtable = nullptr;
        mHashKey = 0;
    }

    size_t SizeOfExcludingThis(MallocSizeOf aMallocSizeOf) const {
        return mTableData.SizeOfExcludingThis(aMallocSizeOf);
    }
    size_t SizeOfIncludingThis(MallocSizeOf aMallocSizeOf) const {
        return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
    }

private:
    
    FallibleTArray<uint8_t> mTableData;

    
    
    nsTHashtable<FontTableHashEntry> *mHashtable;
    uint32_t                          mHashKey;

    
    FontTableBlobData(const FontTableBlobData&);
};

hb_blob_t *
gfxFontEntry::FontTableHashEntry::
ShareTableAndGetBlob(FallibleTArray<uint8_t>& aTable,
                     nsTHashtable<FontTableHashEntry> *aHashtable)
{
    Clear();
    
    mSharedBlobData = new FontTableBlobData(aTable);
    mBlob = hb_blob_create(mSharedBlobData->GetTable(),
                           mSharedBlobData->GetTableLength(),
                           HB_MEMORY_MODE_READONLY,
                           mSharedBlobData, DeleteFontTableBlobData);
    if (!mSharedBlobData) {
        
        
        
        return hb_blob_reference(mBlob);
    }

    
    
    mSharedBlobData->ManageHashEntry(aHashtable, GetKey());
    return mBlob;
}

void
gfxFontEntry::FontTableHashEntry::Clear()
{
    
    
    
    if (mSharedBlobData) {
        mSharedBlobData->ForgetHashEntry();
        mSharedBlobData = nullptr;
    } else if (mBlob) {
        hb_blob_destroy(mBlob);
    }
    mBlob = nullptr;
}



 void
gfxFontEntry::FontTableHashEntry::DeleteFontTableBlobData(void *aBlobData)
{
    delete static_cast<FontTableBlobData*>(aBlobData);
}

hb_blob_t *
gfxFontEntry::FontTableHashEntry::GetBlob() const
{
    return hb_blob_reference(mBlob);
}

bool
gfxFontEntry::GetExistingFontTable(uint32_t aTag, hb_blob_t **aBlob)
{
    if (!mFontTableCache) {
        
        
        mFontTableCache = new nsTHashtable<FontTableHashEntry>(8);
    }

    FontTableHashEntry *entry = mFontTableCache->GetEntry(aTag);
    if (!entry) {
        return false;
    }

    *aBlob = entry->GetBlob();
    return true;
}

hb_blob_t *
gfxFontEntry::ShareFontTableAndGetBlob(uint32_t aTag,
                                       FallibleTArray<uint8_t>* aBuffer)
{
    if (MOZ_UNLIKELY(!mFontTableCache)) {
        
        
      mFontTableCache = new nsTHashtable<FontTableHashEntry>(8);
    }

    FontTableHashEntry *entry = mFontTableCache->PutEntry(aTag);
    if (MOZ_UNLIKELY(!entry)) { 
        return nullptr;
    }

    if (!aBuffer) {
        
        entry->Clear();
        return nullptr;
    }

    return entry->ShareTableAndGetBlob(*aBuffer, mFontTableCache);
}

static int
DirEntryCmp(const void* aKey, const void* aItem)
{
    int32_t tag = *static_cast<const int32_t*>(aKey);
    const TableDirEntry* entry = static_cast<const TableDirEntry*>(aItem);
    return tag - int32_t(entry->tag);
}

hb_blob_t*
gfxFontEntry::GetTableFromFontData(const void* aFontData, uint32_t aTableTag)
{
    const SFNTHeader* header =
        reinterpret_cast<const SFNTHeader*>(aFontData);
    const TableDirEntry* dir =
        reinterpret_cast<const TableDirEntry*>(header + 1);
    dir = static_cast<const TableDirEntry*>
        (bsearch(&aTableTag, dir, uint16_t(header->numTables),
                 sizeof(TableDirEntry), DirEntryCmp));
    if (dir) {
        return hb_blob_create(reinterpret_cast<const char*>(aFontData) +
                                  dir->offset, dir->length,
                              HB_MEMORY_MODE_READONLY, nullptr, nullptr);

    }
    return nullptr;
}

already_AddRefed<gfxCharacterMap>
gfxFontEntry::GetCMAPFromFontInfo(FontInfoData *aFontInfoData,
                                  uint32_t& aUVSOffset,
                                  bool& aSymbolFont)
{
    if (!aFontInfoData || !aFontInfoData->mLoadCmaps) {
        return nullptr;
    }

    return aFontInfoData->GetCMAP(mName, aUVSOffset, aSymbolFont);
}

hb_blob_t *
gfxFontEntry::GetFontTable(uint32_t aTag)
{
    hb_blob_t *blob;
    if (GetExistingFontTable(aTag, &blob)) {
        return blob;
    }

    FallibleTArray<uint8_t> buffer;
    bool haveTable = NS_SUCCEEDED(CopyFontTable(aTag, buffer));

    return ShareFontTableAndGetBlob(aTag, haveTable ? &buffer : nullptr);
}



 hb_blob_t *
gfxFontEntry::HBGetTable(hb_face_t *face, uint32_t aTag, void *aUserData)
{
    gfxFontEntry *fontEntry = static_cast<gfxFontEntry*>(aUserData);

    
    
    if (aTag == TRUETYPE_TAG('G','D','E','F') &&
        fontEntry->IgnoreGDEF()) {
        return nullptr;
    }

    
    
    if (aTag == TRUETYPE_TAG('G','S','U','B') &&
        fontEntry->IgnoreGSUB()) {
        return nullptr;
    }

    return fontEntry->GetFontTable(aTag);
}

 void
gfxFontEntry::HBFaceDeletedCallback(void *aUserData)
{
    gfxFontEntry *fe = static_cast<gfxFontEntry*>(aUserData);
    fe->ForgetHBFace();
}

void
gfxFontEntry::ForgetHBFace()
{
    mHBFace = nullptr;
}

hb_face_t*
gfxFontEntry::GetHBFace()
{
    if (!mHBFace) {
        mHBFace = hb_face_create_for_tables(HBGetTable, this,
                                            HBFaceDeletedCallback);
        return mHBFace;
    }
    return hb_face_reference(mHBFace);
}

 const void*
gfxFontEntry::GrGetTable(const void *aAppFaceHandle, unsigned int aName,
                         size_t *aLen)
{
    gfxFontEntry *fontEntry =
        static_cast<gfxFontEntry*>(const_cast<void*>(aAppFaceHandle));
    hb_blob_t *blob = fontEntry->GetFontTable(aName);
    if (blob) {
        unsigned int blobLength;
        const void *tableData = hb_blob_get_data(blob, &blobLength);
        fontEntry->mGrTableMap->Put(tableData, blob);
        *aLen = blobLength;
        return tableData;
    }
    *aLen = 0;
    return nullptr;
}

 void
gfxFontEntry::GrReleaseTable(const void *aAppFaceHandle,
                             const void *aTableBuffer)
{
    gfxFontEntry *fontEntry =
        static_cast<gfxFontEntry*>(const_cast<void*>(aAppFaceHandle));
    void *data;
    if (fontEntry->mGrTableMap->Get(aTableBuffer, &data)) {
        fontEntry->mGrTableMap->Remove(aTableBuffer);
        hb_blob_destroy(static_cast<hb_blob_t*>(data));
    }
}

gr_face*
gfxFontEntry::GetGrFace()
{
    if (!mGrFaceInitialized) {
        gr_face_ops faceOps = {
            sizeof(gr_face_ops),
            GrGetTable,
            GrReleaseTable
        };
        mGrTableMap = new nsDataHashtable<nsPtrHashKey<const void>,void*>;
        mGrFace = gr_make_face_with_ops(this, &faceOps, gr_face_default);
        mGrFaceInitialized = true;
    }
    ++mGrFaceRefCnt;
    return mGrFace;
}

void
gfxFontEntry::ReleaseGrFace(gr_face *aFace)
{
    MOZ_ASSERT(aFace == mGrFace); 
    MOZ_ASSERT(mGrFaceRefCnt > 0);
    if (--mGrFaceRefCnt == 0) {
        gr_face_destroy(mGrFace);
        mGrFace = nullptr;
        mGrFaceInitialized = false;
        delete mGrTableMap;
        mGrTableMap = nullptr;
    }
}

void
gfxFontEntry::DisconnectSVG()
{
    if (mSVGInitialized && mSVGGlyphs) {
        mSVGGlyphs = nullptr;
        mSVGInitialized = false;
    }
}

bool
gfxFontEntry::HasFontTable(uint32_t aTableTag)
{
    AutoTable table(this, aTableTag);
    return table && hb_blob_get_length(table) > 0;
}

void
gfxFontEntry::CheckForGraphiteTables()
{
    mHasGraphiteTables = HasFontTable(TRUETYPE_TAG('S','i','l','f'));
}


#define FEATURE_SCRIPT_MASK 0x000000ff // script index replaces low byte of tag


PR_STATIC_ASSERT(MOZ_NUM_SCRIPT_CODES <= FEATURE_SCRIPT_MASK);


#define SCRIPT_FEATURE(s,tag) (((~FEATURE_SCRIPT_MASK) & (tag)) | \
                               ((FEATURE_SCRIPT_MASK) & (s)))

bool
gfxFontEntry::SupportsOpenTypeFeature(int32_t aScript, uint32_t aFeatureTag)
{
    if (!mSupportedFeatures) {
        mSupportedFeatures = new nsDataHashtable<nsUint32HashKey,bool>();
    }

    
    
    NS_ASSERTION(aFeatureTag == HB_TAG('s','m','c','p') ||
                 aFeatureTag == HB_TAG('c','2','s','c') ||
                 aFeatureTag == HB_TAG('p','c','a','p') ||
                 aFeatureTag == HB_TAG('c','2','p','c') ||
                 aFeatureTag == HB_TAG('s','u','p','s') ||
                 aFeatureTag == HB_TAG('s','u','b','s'),
                 "use of unknown feature tag");

    
    NS_ASSERTION(aScript < FEATURE_SCRIPT_MASK - 1,
                 "need to bump the size of the feature shift");

    uint32_t scriptFeature = SCRIPT_FEATURE(aScript, aFeatureTag);
    bool result;
    if (mSupportedFeatures->Get(scriptFeature, &result)) {
        return result;
    }

    result = false;

    hb_face_t *face = GetHBFace();

    if (hb_ot_layout_has_substitution(face)) {
        hb_script_t hbScript =
            gfxHarfBuzzShaper::GetHBScriptUsedForShaping(aScript);

        
        hb_tag_t scriptTags[4] = {
            HB_TAG_NONE,
            HB_TAG_NONE,
            HB_TAG_NONE,
            HB_TAG_NONE
        };
        hb_ot_tags_from_script(hbScript, &scriptTags[0], &scriptTags[1]);

        
        hb_tag_t* scriptTag = &scriptTags[0];
        while (*scriptTag != HB_TAG_NONE) {
            ++scriptTag;
        }
        *scriptTag = HB_OT_TAG_DEFAULT_SCRIPT;

        
        const hb_tag_t kGSUB = HB_TAG('G','S','U','B');
        scriptTag = &scriptTags[0];
        while (*scriptTag != HB_TAG_NONE) {
            unsigned int scriptIndex;
            if (hb_ot_layout_table_find_script(face, kGSUB, *scriptTag,
                                               &scriptIndex)) {
                if (hb_ot_layout_language_find_feature(face, kGSUB,
                                                       scriptIndex,
                                           HB_OT_LAYOUT_DEFAULT_LANGUAGE_INDEX,
                                                       aFeatureTag, nullptr)) {
                    result = true;
                }
                break;
            }
            ++scriptTag;
        }
    }

    hb_face_destroy(face);

    mSupportedFeatures->Put(scriptFeature, result);

    return result;
}

const hb_set_t*
gfxFontEntry::InputsForOpenTypeFeature(int32_t aScript, uint32_t aFeatureTag)
{
    if (!mFeatureInputs) {
        mFeatureInputs = new nsDataHashtable<nsUint32HashKey,hb_set_t*>();
    }

    NS_ASSERTION(aFeatureTag == HB_TAG('s','u','p','s') ||
                 aFeatureTag == HB_TAG('s','u','b','s'),
                 "use of unknown feature tag");

    uint32_t scriptFeature = SCRIPT_FEATURE(aScript, aFeatureTag);
    hb_set_t *inputGlyphs;
    if (mFeatureInputs->Get(scriptFeature, &inputGlyphs)) {
        return inputGlyphs;
    }

    inputGlyphs = hb_set_create();

    hb_face_t *face = GetHBFace();

    if (hb_ot_layout_has_substitution(face)) {
        hb_script_t hbScript =
            gfxHarfBuzzShaper::GetHBScriptUsedForShaping(aScript);

        
        hb_tag_t scriptTags[4] = {
            HB_TAG_NONE,
            HB_TAG_NONE,
            HB_TAG_NONE,
            HB_TAG_NONE
        };
        hb_ot_tags_from_script(hbScript, &scriptTags[0], &scriptTags[1]);

        
        hb_tag_t* scriptTag = &scriptTags[0];
        while (*scriptTag != HB_TAG_NONE) {
            ++scriptTag;
        }
        *scriptTag = HB_OT_TAG_DEFAULT_SCRIPT;

        const hb_tag_t kGSUB = HB_TAG('G','S','U','B');
        hb_tag_t features[2] = { aFeatureTag, HB_TAG_NONE };
        hb_set_t *featurelookups = hb_set_create();
        hb_ot_layout_collect_lookups(face, kGSUB, scriptTags, nullptr,
                                     features, featurelookups);
        hb_codepoint_t index = -1;
        while (hb_set_next(featurelookups, &index)) {
            hb_ot_layout_lookup_collect_glyphs(face, kGSUB, index,
                                               nullptr, inputGlyphs,
                                               nullptr, nullptr);
        }
    }

    hb_face_destroy(face);

    mFeatureInputs->Put(scriptFeature, inputGlyphs);
    return inputGlyphs;
}

bool
gfxFontEntry::SupportsGraphiteFeature(uint32_t aFeatureTag)
{
    if (!mSupportedFeatures) {
        mSupportedFeatures = new nsDataHashtable<nsUint32HashKey,bool>();
    }

    
    
    NS_ASSERTION(aFeatureTag == HB_TAG('s','m','c','p') ||
                 aFeatureTag == HB_TAG('c','2','s','c') ||
                 aFeatureTag == HB_TAG('p','c','a','p') ||
                 aFeatureTag == HB_TAG('c','2','p','c') ||
                 aFeatureTag == HB_TAG('s','u','p','s') ||
                 aFeatureTag == HB_TAG('s','u','b','s'),
                 "use of unknown feature tag");

    
    uint32_t scriptFeature = SCRIPT_FEATURE(FEATURE_SCRIPT_MASK, aFeatureTag);
    bool result;
    if (mSupportedFeatures->Get(scriptFeature, &result)) {
        return result;
    }

    gr_face* face = GetGrFace();
    result = gr_face_find_fref(face, aFeatureTag) != nullptr;
    ReleaseGrFace(face);

    mSupportedFeatures->Put(scriptFeature, result);

    return result;
}

bool
gfxFontEntry::GetColorLayersInfo(uint32_t aGlyphId,
                            nsTArray<uint16_t>& aLayerGlyphs,
                            nsTArray<mozilla::gfx::Color>& aLayerColors)
{
    return gfxFontUtils::GetColorGlyphLayers(mCOLR,
                                             mCPAL,
                                             aGlyphId,
                                             aLayerGlyphs,
                                             aLayerColors);
}

size_t
gfxFontEntry::FontTableHashEntry::SizeOfExcludingThis(mozilla::MallocSizeOf aMallocSizeOf) const
{
    size_t n = 0;
    if (mBlob) {
        n += aMallocSizeOf(mBlob);
    }
    if (mSharedBlobData) {
        n += mSharedBlobData->SizeOfIncludingThis(aMallocSizeOf);
    }
    return n;
}

void
gfxFontEntry::AddSizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                                     FontListSizes* aSizes) const
{
    aSizes->mFontListSize += mName.SizeOfExcludingThisIfUnshared(aMallocSizeOf);

    
    if (mCharacterMap && mCharacterMap->mBuildOnTheFly) {
        aSizes->mCharMapsSize +=
            mCharacterMap->SizeOfIncludingThis(aMallocSizeOf);
    }
    if (mFontTableCache) {
        aSizes->mFontTableCacheSize +=
            mFontTableCache->SizeOfIncludingThis(aMallocSizeOf);
    }
}

void
gfxFontEntry::AddSizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                                     FontListSizes* aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}









class FontEntryStandardFaceComparator {
  public:
    bool Equals(const nsRefPtr<gfxFontEntry>& a, const nsRefPtr<gfxFontEntry>& b) const {
        return a->mStandardFace == b->mStandardFace;
    }
    bool LessThan(const nsRefPtr<gfxFontEntry>& a, const nsRefPtr<gfxFontEntry>& b) const {
        return (a->mStandardFace == false && b->mStandardFace == true);
    }
};

void
gfxFontFamily::SortAvailableFonts()
{
    mAvailableFonts.Sort(FontEntryStandardFaceComparator());
}

bool
gfxFontFamily::HasOtherFamilyNames()
{
    
    if (!mOtherFamilyNamesInitialized) {
        ReadOtherFamilyNames(gfxPlatformFontList::PlatformFontList());  
    }
    return mHasOtherFamilyNames;
}

gfxFontEntry*
gfxFontFamily::FindFontForStyle(const gfxFontStyle& aFontStyle, 
                                bool& aNeedsSyntheticBold)
{
    if (!mHasStyles)
        FindStyleVariations(); 

    NS_ASSERTION(mAvailableFonts.Length() > 0, "font family with no faces!");

    aNeedsSyntheticBold = false;

    int8_t baseWeight = aFontStyle.ComputeWeight();
    bool wantBold = baseWeight >= 6;

    
    if (mAvailableFonts.Length() == 1) {
        gfxFontEntry *fe = mAvailableFonts[0];
        aNeedsSyntheticBold =
            wantBold && !fe->IsBold() && aFontStyle.allowSyntheticWeight;
        return fe;
    }

    bool wantItalic = (aFontStyle.style &
                       (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) != 0;

    
    
    
    
    
    

    if (mIsSimpleFamily) {
        
        
        
        uint8_t faceIndex = (wantItalic ? kItalicMask : 0) |
                            (wantBold ? kBoldMask : 0);

        
        gfxFontEntry *fe = mAvailableFonts[faceIndex];
        if (fe) {
            
            return fe;
        }

        
        static const uint8_t simpleFallbacks[4][3] = {
            { kBoldFaceIndex, kItalicFaceIndex, kBoldItalicFaceIndex },   
            { kRegularFaceIndex, kBoldItalicFaceIndex, kItalicFaceIndex },
            { kBoldItalicFaceIndex, kRegularFaceIndex, kBoldFaceIndex },  
            { kItalicFaceIndex, kBoldFaceIndex, kRegularFaceIndex }       
        };
        const uint8_t *order = simpleFallbacks[faceIndex];

        for (uint8_t trial = 0; trial < 3; ++trial) {
            
            fe = mAvailableFonts[order[trial]];
            if (fe) {
                aNeedsSyntheticBold =
                    wantBold && !fe->IsBold() &&
                    aFontStyle.allowSyntheticWeight;
                return fe;
            }
        }

        
        NS_NOTREACHED("no face found in simple font family!");
        return nullptr;
    }

    
    
    
    

    gfxFontEntry *weightList[10] = { 0 };
    bool foundWeights = FindWeightsForStyle(weightList, wantItalic, aFontStyle.stretch);
    if (!foundWeights) {
        return nullptr;
    }

    
    int8_t matchBaseWeight = 0;
    int8_t i = baseWeight;

    
    
    if (baseWeight == 4 && !weightList[4]) {
        i = 5; 
    }

    
    int8_t direction = (baseWeight > 5) ? 1 : -1;
    for (; ; i += direction) {
        if (weightList[i]) {
            matchBaseWeight = i;
            break;
        }

        
        
        if (i == 1 || i == 9) {
            i = baseWeight;
            direction = -direction;
        }
    }

    NS_ASSERTION(matchBaseWeight != 0, 
                 "weight mapping should always find at least one font in a family");

    gfxFontEntry *matchFE = weightList[matchBaseWeight];

    NS_ASSERTION(matchFE,
                 "weight mapping should always find at least one font in a family");

    if (!matchFE->IsBold() && baseWeight >= 6 &&
        aFontStyle.allowSyntheticWeight)
    {
        aNeedsSyntheticBold = true;
    }

    return matchFE;
}

void
gfxFontFamily::CheckForSimpleFamily()
{
    
    if (mIsSimpleFamily) {
        return;
    };

    uint32_t count = mAvailableFonts.Length();
    if (count > 4 || count == 0) {
        return; 
                
    }

    if (count == 1) {
        mIsSimpleFamily = true;
        return;
    }

    int16_t firstStretch = mAvailableFonts[0]->Stretch();

    gfxFontEntry *faces[4] = { 0 };
    for (uint8_t i = 0; i < count; ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (fe->Stretch() != firstStretch) {
            return; 
        }
        uint8_t faceIndex = (fe->IsItalic() ? kItalicMask : 0) |
                            (fe->Weight() >= 600 ? kBoldMask : 0);
        if (faces[faceIndex]) {
            return; 
        }
        faces[faceIndex] = fe;
    }

    
    
    mAvailableFonts.SetLength(4);
    for (uint8_t i = 0; i < 4; ++i) {
        if (mAvailableFonts[i].get() != faces[i]) {
            mAvailableFonts[i].swap(faces[i]);
        }
    }

    mIsSimpleFamily = true;
}

#ifdef DEBUG
bool
gfxFontFamily::ContainsFace(gfxFontEntry* aFontEntry) {
    uint32_t i, numFonts = mAvailableFonts.Length();
    for (i = 0; i < numFonts; i++) {
        if (mAvailableFonts[i] == aFontEntry) {
            return true;
        }
        
        if (mAvailableFonts[i] && mAvailableFonts[i]->mIsUserFontContainer) {
            gfxUserFontEntry* ufe =
                static_cast<gfxUserFontEntry*>(mAvailableFonts[i].get());
            if (ufe->GetPlatformFontEntry() == aFontEntry) {
                return true;
            }
        }
    }
    return false;
}
#endif

static inline uint32_t
StyleDistance(gfxFontEntry *aFontEntry,
              bool anItalic, int16_t aStretch)
{
    
    
    

    int32_t distance = 0;
    if (aStretch != aFontEntry->mStretch) {
        
        
        
        if (aStretch > 0) {
            distance = (aFontEntry->mStretch - aStretch) * 2;
        } else {
            distance = (aStretch - aFontEntry->mStretch) * 2;
        }
        
        
        
        
        if (distance < 0) {
            distance = -distance + 10;
        }
    }
    if (aFontEntry->IsItalic() != anItalic) {
        distance += 1;
    }
    return uint32_t(distance);
}

bool
gfxFontFamily::FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                   bool anItalic, int16_t aStretch)
{
    uint32_t foundWeights = 0;
    uint32_t bestMatchDistance = 0xffffffff;

    uint32_t count = mAvailableFonts.Length();
    for (uint32_t i = 0; i < count; i++) {
        
        
        gfxFontEntry *fe = mAvailableFonts[i];
        uint32_t distance = StyleDistance(fe, anItalic, aStretch);
        if (distance <= bestMatchDistance) {
            int8_t wt = fe->mWeight / 100;
            NS_ASSERTION(wt >= 1 && wt < 10, "invalid weight in fontEntry");
            if (!aFontsForWeights[wt]) {
                
                aFontsForWeights[wt] = fe;
                ++foundWeights;
            } else {
                uint32_t prevDistance =
                    StyleDistance(aFontsForWeights[wt], anItalic, aStretch);
                if (prevDistance >= distance) {
                    
                    
                    aFontsForWeights[wt] = fe;
                }
            }
            bestMatchDistance = distance;
        }
    }

    NS_ASSERTION(foundWeights > 0, "Font family containing no faces?");

    if (foundWeights == 1) {
        
        return true;
    }

    
    
    
    for (uint32_t i = 0; i < 10; ++i) {
        if (aFontsForWeights[i] &&
            StyleDistance(aFontsForWeights[i], anItalic, aStretch) > bestMatchDistance)
        {
            aFontsForWeights[i] = 0;
        }
    }

    return (foundWeights > 0);
}


void gfxFontFamily::LocalizedName(nsAString& aLocalizedName)
{
    
    aLocalizedName = mName;
}


static int32_t
CalcStyleMatch(gfxFontEntry *aFontEntry, const gfxFontStyle *aStyle)
{
    int32_t rank = 0;
    if (aStyle) {
         
         bool wantItalic =
             (aStyle->style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) != 0;
         if (aFontEntry->IsItalic() == wantItalic) {
             rank += 10;
         }

        
        rank += 9 - DeprecatedAbs(aFontEntry->Weight() / 100 - aStyle->ComputeWeight());
    } else {
        
        if (!aFontEntry->IsItalic()) {
            rank += 3;
        }
        if (!aFontEntry->IsBold()) {
            rank += 2;
        }
    }

    return rank;
}

#define RANK_MATCHED_CMAP   20

void
gfxFontFamily::FindFontForChar(GlobalFontMatch *aMatchData)
{
    if (mFamilyCharacterMapInitialized && !TestCharacterMap(aMatchData->mCh)) {
        
        
        return;
    }

    bool needsBold;
    gfxFontStyle normal;
    gfxFontEntry *fe = FindFontForStyle(
                  (aMatchData->mStyle == nullptr) ? *aMatchData->mStyle : normal,
                  needsBold);

    if (fe && !fe->SkipDuringSystemFallback()) {
        int32_t rank = 0;

        if (fe->TestCharacterMap(aMatchData->mCh)) {
            rank += RANK_MATCHED_CMAP;
            aMatchData->mCount++;
#ifdef PR_LOGGING
            PRLogModuleInfo *log = gfxPlatform::GetLog(eGfxLog_textrun);

            if (MOZ_UNLIKELY(PR_LOG_TEST(log, PR_LOG_DEBUG))) {
                uint32_t unicodeRange = FindCharUnicodeRange(aMatchData->mCh);
                uint32_t script = GetScriptCode(aMatchData->mCh);
                PR_LOG(log, PR_LOG_DEBUG,\
                       ("(textrun-systemfallback-fonts) char: u+%6.6x "
                        "unicode-range: %d script: %d match: [%s]\n",
                        aMatchData->mCh,
                        unicodeRange, script,
                        NS_ConvertUTF16toUTF8(fe->Name()).get()));
            }
#endif
        }

        aMatchData->mCmapsTested++;
        if (rank == 0) {
            return;
        }

         
         
        rank += CalcStyleMatch(fe, aMatchData->mStyle);

        

        if (rank > aMatchData->mMatchRank
            || (rank == aMatchData->mMatchRank &&
                Compare(fe->Name(), aMatchData->mBestMatch->Name()) > 0))
        {
            aMatchData->mBestMatch = fe;
            aMatchData->mMatchedFamily = this;
            aMatchData->mMatchRank = rank;
        }
    }
}

void
gfxFontFamily::SearchAllFontsForChar(GlobalFontMatch *aMatchData)
{
    uint32_t i, numFonts = mAvailableFonts.Length();
    for (i = 0; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (fe && fe->TestCharacterMap(aMatchData->mCh)) {
            int32_t rank = RANK_MATCHED_CMAP;
            rank += CalcStyleMatch(fe, aMatchData->mStyle);
            if (rank > aMatchData->mMatchRank
                || (rank == aMatchData->mMatchRank &&
                    Compare(fe->Name(), aMatchData->mBestMatch->Name()) > 0))
            {
                aMatchData->mBestMatch = fe;
                aMatchData->mMatchedFamily = this;
                aMatchData->mMatchRank = rank;
            }
        }
    }
}

 void
gfxFontFamily::ReadOtherFamilyNamesForFace(const nsAString& aFamilyName,
                                           const char *aNameData,
                                           uint32_t aDataLength,
                                           nsTArray<nsString>& aOtherFamilyNames,
                                           bool useFullName)
{
    const gfxFontUtils::NameHeader *nameHeader =
        reinterpret_cast<const gfxFontUtils::NameHeader*>(aNameData);

    uint32_t nameCount = nameHeader->count;
    if (nameCount * sizeof(gfxFontUtils::NameRecord) > aDataLength) {
        NS_WARNING("invalid font (name records)");
        return;
    }
    
    const gfxFontUtils::NameRecord *nameRecord =
        reinterpret_cast<const gfxFontUtils::NameRecord*>(aNameData + sizeof(gfxFontUtils::NameHeader));
    uint32_t stringsBase = uint32_t(nameHeader->stringOffset);

    for (uint32_t i = 0; i < nameCount; i++, nameRecord++) {
        uint32_t nameLen = nameRecord->length;
        uint32_t nameOff = nameRecord->offset;  

        if (stringsBase + nameOff + nameLen > aDataLength) {
            NS_WARNING("invalid font (name table strings)");
            return;
        }

        uint16_t nameID = nameRecord->nameID;
        if ((useFullName && nameID == gfxFontUtils::NAME_ID_FULL) ||
            (!useFullName && (nameID == gfxFontUtils::NAME_ID_FAMILY ||
                              nameID == gfxFontUtils::NAME_ID_PREFERRED_FAMILY))) {
            nsAutoString otherFamilyName;
            bool ok = gfxFontUtils::DecodeFontName(aNameData + stringsBase + nameOff,
                                                     nameLen,
                                                     uint32_t(nameRecord->platformID),
                                                     uint32_t(nameRecord->encodingID),
                                                     uint32_t(nameRecord->languageID),
                                                     otherFamilyName);
            
            if (ok && otherFamilyName != aFamilyName) {
                aOtherFamilyNames.AppendElement(otherFamilyName);
            }
        }
    }
}


bool
gfxFontFamily::ReadOtherFamilyNamesForFace(gfxPlatformFontList *aPlatformFontList,
                                           hb_blob_t           *aNameTable,
                                           bool                 useFullName)
{
    uint32_t dataLength;
    const char *nameData = hb_blob_get_data(aNameTable, &dataLength);
    nsAutoTArray<nsString,4> otherFamilyNames;

    ReadOtherFamilyNamesForFace(mName, nameData, dataLength,
                                otherFamilyNames, useFullName);

    uint32_t n = otherFamilyNames.Length();
    for (uint32_t i = 0; i < n; i++) {
        aPlatformFontList->AddOtherFamilyName(this, otherFamilyNames[i]);
    }

    return n != 0;
}

void
gfxFontFamily::ReadOtherFamilyNames(gfxPlatformFontList *aPlatformFontList)
{
    if (mOtherFamilyNamesInitialized) 
        return;
    mOtherFamilyNamesInitialized = true;

    FindStyleVariations();

    
    uint32_t i, numFonts = mAvailableFonts.Length();
    const uint32_t kNAME = TRUETYPE_TAG('n','a','m','e');

    for (i = 0; i < numFonts; ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (!fe) {
            continue;
        }
        gfxFontEntry::AutoTable nameTable(fe, kNAME);
        if (!nameTable) {
            continue;
        }
        mHasOtherFamilyNames = ReadOtherFamilyNamesForFace(aPlatformFontList,
                                                           nameTable);
        break;
    }

    
    
    
    if (!mHasOtherFamilyNames) 
        return;

    
    
    for ( ; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (!fe) {
            continue;
        }
        gfxFontEntry::AutoTable nameTable(fe, kNAME);
        if (!nameTable) {
            continue;
        }
        ReadOtherFamilyNamesForFace(aPlatformFontList, nameTable);
    }
}

void
gfxFontFamily::ReadFaceNames(gfxPlatformFontList *aPlatformFontList, 
                             bool aNeedFullnamePostscriptNames,
                             FontInfoData *aFontInfoData)
{
    
    if (mOtherFamilyNamesInitialized &&
        (mFaceNamesInitialized || !aNeedFullnamePostscriptNames))
        return;

    bool asyncFontLoaderDisabled = false;

#if defined(XP_MACOSX)
    
    if (!nsCocoaFeatures::OnLionOrLater()) {
        asyncFontLoaderDisabled = true;
    }
#endif

    if (!mOtherFamilyNamesInitialized &&
        aFontInfoData &&
        aFontInfoData->mLoadOtherNames &&
        !asyncFontLoaderDisabled)
    {
        nsAutoTArray<nsString,4> otherFamilyNames;
        bool foundOtherNames =
            aFontInfoData->GetOtherFamilyNames(mName, otherFamilyNames);
        if (foundOtherNames) {
            uint32_t i, n = otherFamilyNames.Length();
            for (i = 0; i < n; i++) {
                aPlatformFontList->AddOtherFamilyName(this, otherFamilyNames[i]);
            }
        }
        mOtherFamilyNamesInitialized = true;
    }

    
    if (mOtherFamilyNamesInitialized &&
        (mFaceNamesInitialized || !aNeedFullnamePostscriptNames)) {
        return;
    }

    FindStyleVariations(aFontInfoData);

    
    if (mOtherFamilyNamesInitialized &&
        (mFaceNamesInitialized || !aNeedFullnamePostscriptNames)) {
        return;
    }

    uint32_t i, numFonts = mAvailableFonts.Length();
    const uint32_t kNAME = TRUETYPE_TAG('n','a','m','e');

    bool firstTime = true, readAllFaces = false;
    for (i = 0; i < numFonts; ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (!fe) {
            continue;
        }

        nsAutoString fullname, psname;
        bool foundFaceNames = false;
        if (!mFaceNamesInitialized &&
            aNeedFullnamePostscriptNames &&
            aFontInfoData &&
            aFontInfoData->mLoadFaceNames) {
            aFontInfoData->GetFaceNames(fe->Name(), fullname, psname);
            if (!fullname.IsEmpty()) {
                aPlatformFontList->AddFullname(fe, fullname);
            }
            if (!psname.IsEmpty()) {
                aPlatformFontList->AddPostscriptName(fe, psname);
            }
            foundFaceNames = true;

            
            if (mOtherFamilyNamesInitialized) {
                continue;
            }
        }

        
        gfxFontEntry::AutoTable nameTable(fe, kNAME);
        if (!nameTable) {
            continue;
        }

        if (aNeedFullnamePostscriptNames && !foundFaceNames) {
            if (gfxFontUtils::ReadCanonicalName(
                    nameTable, gfxFontUtils::NAME_ID_FULL, fullname) == NS_OK)
            {
                aPlatformFontList->AddFullname(fe, fullname);
            }

            if (gfxFontUtils::ReadCanonicalName(
                    nameTable, gfxFontUtils::NAME_ID_POSTSCRIPT, psname) == NS_OK)
            {
                aPlatformFontList->AddPostscriptName(fe, psname);
            }
        }

        if (!mOtherFamilyNamesInitialized && (firstTime || readAllFaces)) {
            bool foundOtherName = ReadOtherFamilyNamesForFace(aPlatformFontList,
                                                              nameTable);

            
            
            if (firstTime && foundOtherName) {
                mHasOtherFamilyNames = true;
                readAllFaces = true;
            }
            firstTime = false;
        }

        
        if (!readAllFaces && !aNeedFullnamePostscriptNames) {
            break;
        }
    }

    mFaceNamesInitialized = true;
    mOtherFamilyNamesInitialized = true;
}


gfxFontEntry*
gfxFontFamily::FindFont(const nsAString& aPostscriptName)
{
    
    uint32_t numFonts = mAvailableFonts.Length();
    for (uint32_t i = 0; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i].get();
        if (fe && fe->Name() == aPostscriptName)
            return fe;
    }
    return nullptr;
}

void
gfxFontFamily::ReadAllCMAPs(FontInfoData *aFontInfoData)
{
    FindStyleVariations(aFontInfoData);

    uint32_t i, numFonts = mAvailableFonts.Length();
    for (i = 0; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i];
        
        if (!fe || fe->mIsUserFontContainer) {
            continue;
        }
        fe->ReadCMAP(aFontInfoData);
        mFamilyCharacterMap.Union(*(fe->mCharacterMap));
    }
    mFamilyCharacterMap.Compact();
    mFamilyCharacterMapInitialized = true;
}

void
gfxFontFamily::AddSizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                                      FontListSizes* aSizes) const
{
    aSizes->mFontListSize +=
        mName.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    aSizes->mCharMapsSize +=
        mFamilyCharacterMap.SizeOfExcludingThis(aMallocSizeOf);

    aSizes->mFontListSize +=
        mAvailableFonts.SizeOfExcludingThis(aMallocSizeOf);
    for (uint32_t i = 0; i < mAvailableFonts.Length(); ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (fe) {
            fe->AddSizeOfIncludingThis(aMallocSizeOf, aSizes);
        }
    }
}

void
gfxFontFamily::AddSizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                                      FontListSizes* aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}
