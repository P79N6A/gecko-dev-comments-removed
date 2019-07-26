




#include "mozilla/DebugOnly.h"
#include "mozilla/MathAlgorithms.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "nsServiceManagerUtils.h"
#include "nsReadableUtils.h"
#include "nsExpirationTracker.h"
#include "nsILanguageAtomService.h"
#include "nsITimer.h"

#include "gfxFont.h"
#include "gfxPlatform.h"
#include "nsGkAtoms.h"

#include "gfxTypes.h"
#include "nsAlgorithm.h"
#include "gfxContext.h"
#include "gfxFontMissingGlyphs.h"
#include "gfxUserFontSet.h"
#include "gfxPlatformFontList.h"
#include "gfxScriptItemizer.h"
#include "nsUnicodeProperties.h"
#include "nsMathUtils.h"
#include "nsBidiUtils.h"
#include "nsUnicodeRange.h"
#include "nsStyleConsts.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/Likely.h"
#include "mozilla/Preferences.h"
#include "mozilla/Services.h"
#include "mozilla/Telemetry.h"
#include "gfxSVGGlyphs.h"

#include "cairo.h"
#include "gfxFontTest.h"

#include "harfbuzz/hb.h"

#include "nsCRT.h"
#include "sampler.h"

#include <algorithm>

using namespace mozilla;
using namespace mozilla::gfx;
using namespace mozilla::unicode;
using mozilla::services::GetObserverService;

gfxFontCache *gfxFontCache::gGlobalCache = nullptr;

#ifdef DEBUG_roc
#define DEBUG_TEXT_RUN_STORAGE_METRICS
#endif

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
static uint32_t gTextRunStorageHighWaterMark = 0;
static uint32_t gTextRunStorage = 0;
static uint32_t gFontCount = 0;
static uint32_t gGlyphExtentsCount = 0;
static uint32_t gGlyphExtentsWidthsTotalSize = 0;
static uint32_t gGlyphExtentsSetupEagerSimple = 0;
static uint32_t gGlyphExtentsSetupEagerTight = 0;
static uint32_t gGlyphExtentsSetupLazyTight = 0;
static uint32_t gGlyphExtentsSetupFallBackToTight = 0;
#endif

void
gfxCharacterMap::NotifyReleased()
{
    gfxPlatformFontList *fontlist = gfxPlatformFontList::PlatformFontList();
    if (mShared) {
        fontlist->RemoveCmap(this);
    }
    delete this;
}

gfxFontEntry::~gfxFontEntry()
{
    
    
    if (!mIsProxy && IsUserFont() && !IsLocalUserFont()) {
        gfxUserFontSet::UserFontCache::ForgetFont(this);
    }

    if (mSVGGlyphs) {
        delete mSVGGlyphs;
    }
    delete mUserFontData;
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
        AutoFallibleTArray<uint8_t,16384> buffer;
        if (GetFontTable(kCmapTag, buffer) != NS_OK) {
            mUVSOffset = 0; 
            return NS_ERROR_FAILURE;
        }

        uint8_t* uvsData;
        nsresult rv = gfxFontUtils::ReadCMAPTableFormat14(
                          buffer.Elements() + mUVSOffset,
                          buffer.Length() - mUVSOffset,
                          uvsData);
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

nsresult gfxFontEntry::ReadCMAP()
{
    NS_ASSERTION(false, "using default no-op implementation of ReadCMAP");
    mCharacterMap = new gfxCharacterMap();
    return NS_OK;
}

nsString
gfxFontEntry::RealFaceName()
{
    FallibleTArray<uint8_t> nameTable;
    nsresult rv = GetFontTable(TRUETYPE_TAG('n','a','m','e'), nameTable);
    if (NS_SUCCEEDED(rv)) {
        nsAutoString name;
        rv = gfxFontUtils::GetFullNameFromTable(nameTable, name);
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
    gfxFont *f = nullptr;
    font.swap(f);
    return f;
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
    NS_ABORT_IF_FALSE(mSVGInitialized, "SVG data has not yet been loaded. TryGetSVGData() first.");

    gfxContextAutoSaveRestore matrixRestore(aContext);
    cairo_matrix_t fontMatrix;
    cairo_get_font_matrix(aContext->GetCairo(), &fontMatrix);

    gfxMatrix svgToAppSpace = *reinterpret_cast<gfxMatrix*>(&fontMatrix);
    svgToAppSpace.Scale(1.0f / gfxSVGGlyphs::SVG_UNITS_PER_EM,
                        1.0f / gfxSVGGlyphs::SVG_UNITS_PER_EM);

    return mSVGGlyphs->GetGlyphExtents(aGlyphId, svgToAppSpace, aResult);
}

bool
gfxFontEntry::RenderSVGGlyph(gfxContext *aContext, uint32_t aGlyphId,
                             int aDrawMode, gfxTextObjectPaint *aObjectPaint)
{
    NS_ASSERTION(mSVGInitialized, "SVG data has not yet been loaded. TryGetSVGData() first.");
    return mSVGGlyphs->RenderGlyph(aContext, aGlyphId, gfxFont::DrawMode(aDrawMode),
                                   aObjectPaint);
}

bool
gfxFontEntry::TryGetSVGData()
{
    if (!mSVGInitialized) {
        mSVGInitialized = true;

        bool svgEnabled;
        nsresult rv =
            Preferences::GetBool("gfx.font_rendering.opentype_svg.enabled",
                                 &svgEnabled);

        if (NS_FAILED(rv) || !svgEnabled) {
            return false;
        }

        FallibleTArray<uint8_t> svgTable;
        rv = GetFontTable(TRUETYPE_TAG('S', 'V', 'G', ' '), svgTable);
        if (NS_FAILED(rv)) {
            return false;
        }

        FallibleTArray<uint8_t> cmapTable;
        rv = GetFontTable(TRUETYPE_TAG('c', 'm', 'a', 'p'), cmapTable);
        NS_ENSURE_SUCCESS(rv, false);

        mSVGGlyphs = new gfxSVGGlyphs(svgTable, cmapTable);
    }

    return !!mSVGGlyphs;
}







class gfxFontEntry::FontTableBlobData {
public:
    
    FontTableBlobData(FallibleTArray<uint8_t>& aBuffer)
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

    size_t SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
        return mTableData.SizeOfExcludingThis(aMallocSizeOf);
    }
    size_t SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const {
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
    if (!mFontTableCache.IsInitialized()) {
        
        
        mFontTableCache.Init(10);
    }

    FontTableHashEntry *entry = mFontTableCache.GetEntry(aTag);
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
    if (MOZ_UNLIKELY(!mFontTableCache.IsInitialized())) {
        
        
        mFontTableCache.Init(10);
    }

    FontTableHashEntry *entry = mFontTableCache.PutEntry(aTag);
    if (MOZ_UNLIKELY(!entry)) { 
        return nullptr;
    }

    if (!aBuffer) {
        
        entry->Clear();
        return nullptr;
    }

    return entry->ShareTableAndGetBlob(*aBuffer, &mFontTableCache);
}

void
gfxFontEntry::CheckForGraphiteTables()
{
    AutoFallibleTArray<uint8_t,16384> buffer;
    mHasGraphiteTables =
        NS_SUCCEEDED(GetFontTable(TRUETYPE_TAG('S','i','l','f'), buffer));
}

 size_t
gfxFontEntry::FontTableHashEntry::SizeOfEntryExcludingThis
    (FontTableHashEntry *aEntry,
     nsMallocSizeOfFun   aMallocSizeOf,
     void*               aUserArg)
{
    FontListSizes *sizes = static_cast<FontListSizes*>(aUserArg);
    if (aEntry->mBlob) {
        sizes->mFontTableCacheSize += aMallocSizeOf(aEntry->mBlob);
    }
    if (aEntry->mSharedBlobData) {
        sizes->mFontTableCacheSize +=
            aEntry->mSharedBlobData->SizeOfIncludingThis(aMallocSizeOf);
    }

    
    
    return 0;
}

void
gfxFontEntry::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                  FontListSizes*    aSizes) const
{
    aSizes->mFontListSize += mName.SizeOfExcludingThisIfUnshared(aMallocSizeOf);

    
    if (mCharacterMap && mCharacterMap->mBuildOnTheFly) {
        aSizes->mCharMapsSize +=
            mCharacterMap->SizeOfIncludingThis(aMallocSizeOf);
    }
    aSizes->mFontTableCacheSize +=
        mFontTableCache.SizeOfExcludingThis(
            FontTableHashEntry::SizeOfEntryExcludingThis,
            aMallocSizeOf, aSizes);
}

void
gfxFontEntry::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                  FontListSizes*    aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
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
        aNeedsSyntheticBold = wantBold && !fe->IsBold();
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
                aNeedsSyntheticBold = wantBold && !fe->IsBold();
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

    if (!matchFE->IsBold() && baseWeight >= 6)
    {
        aNeedsSyntheticBold = true;
    }

    return matchFE;
}

void
gfxFontFamily::CheckForSimpleFamily()
{
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

        
        rank += 9 - Abs(aFontEntry->Weight() / 100 - aStyle->ComputeWeight());
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

            if (MOZ_UNLIKELY(log)) {
                uint32_t charRange = gfxFontUtils::CharRangeBit(aMatchData->mCh);
                uint32_t unicodeRange = FindCharUnicodeRange(aMatchData->mCh);
                uint32_t script = GetScriptCode(aMatchData->mCh);
                PR_LOG(log, PR_LOG_DEBUG,\
                       ("(textrun-systemfallback-fonts) char: u+%6.6x "
                        "char-range: %d unicode-range: %d script: %d match: [%s]\n",
                        aMatchData->mCh,
                        charRange, unicodeRange, script,
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


bool
gfxFontFamily::ReadOtherFamilyNamesForFace(gfxPlatformFontList *aPlatformFontList,
                                           FallibleTArray<uint8_t>& aNameTable,
                                           bool useFullName)
{
    const uint8_t *nameData = aNameTable.Elements();
    uint32_t dataLength = aNameTable.Length();
    const gfxFontUtils::NameHeader *nameHeader =
        reinterpret_cast<const gfxFontUtils::NameHeader*>(nameData);

    uint32_t nameCount = nameHeader->count;
    if (nameCount * sizeof(gfxFontUtils::NameRecord) > dataLength) {
        NS_WARNING("invalid font (name records)");
        return false;
    }
    
    const gfxFontUtils::NameRecord *nameRecord =
        reinterpret_cast<const gfxFontUtils::NameRecord*>(nameData + sizeof(gfxFontUtils::NameHeader));
    uint32_t stringsBase = uint32_t(nameHeader->stringOffset);

    bool foundNames = false;
    for (uint32_t i = 0; i < nameCount; i++, nameRecord++) {
        uint32_t nameLen = nameRecord->length;
        uint32_t nameOff = nameRecord->offset;  

        if (stringsBase + nameOff + nameLen > dataLength) {
            NS_WARNING("invalid font (name table strings)");
            return false;
        }

        uint16_t nameID = nameRecord->nameID;
        if ((useFullName && nameID == gfxFontUtils::NAME_ID_FULL) ||
            (!useFullName && (nameID == gfxFontUtils::NAME_ID_FAMILY ||
                              nameID == gfxFontUtils::NAME_ID_PREFERRED_FAMILY))) {
            nsAutoString otherFamilyName;
            bool ok = gfxFontUtils::DecodeFontName(nameData + stringsBase + nameOff,
                                                     nameLen,
                                                     uint32_t(nameRecord->platformID),
                                                     uint32_t(nameRecord->encodingID),
                                                     uint32_t(nameRecord->languageID),
                                                     otherFamilyName);
            
            if (ok && otherFamilyName != mName) {
                aPlatformFontList->AddOtherFamilyName(this, otherFamilyName);
                foundNames = true;
            }
        }
    }

    return foundNames;
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
    AutoFallibleTArray<uint8_t,8192> buffer;

    for (i = 0; i < numFonts; ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (!fe)
            continue;

        if (fe->GetFontTable(kNAME, buffer) != NS_OK)
            continue;

        mHasOtherFamilyNames = ReadOtherFamilyNamesForFace(aPlatformFontList,
                                                           buffer);
        break;
    }

    
    
    
    if (!mHasOtherFamilyNames) 
        return;

    
    
    for ( ; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (!fe)
            continue;

        if (fe->GetFontTable(kNAME, buffer) != NS_OK)
            continue;

        ReadOtherFamilyNamesForFace(aPlatformFontList, buffer);
    }
}

void
gfxFontFamily::ReadFaceNames(gfxPlatformFontList *aPlatformFontList, 
                             bool aNeedFullnamePostscriptNames)
{
    
    if (mOtherFamilyNamesInitialized &&
        (mFaceNamesInitialized || !aNeedFullnamePostscriptNames))
        return;

    FindStyleVariations();

    uint32_t i, numFonts = mAvailableFonts.Length();
    const uint32_t kNAME = TRUETYPE_TAG('n','a','m','e');
    AutoFallibleTArray<uint8_t,8192> buffer;
    nsAutoString fullname, psname;

    bool firstTime = true, readAllFaces = false;
    for (i = 0; i < numFonts; ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (!fe)
            continue;

        if (fe->GetFontTable(kNAME, buffer) != NS_OK)
            continue;

        if (aNeedFullnamePostscriptNames) {
            if (gfxFontUtils::ReadCanonicalName(
                    buffer, gfxFontUtils::NAME_ID_FULL, fullname) == NS_OK)
            {
                aPlatformFontList->AddFullname(fe, fullname);
            }

            if (gfxFontUtils::ReadCanonicalName(
                    buffer, gfxFontUtils::NAME_ID_POSTSCRIPT, psname) == NS_OK)
            {
                aPlatformFontList->AddPostscriptName(fe, psname);
            }
        }

       if (!mOtherFamilyNamesInitialized && (firstTime || readAllFaces)) {
           bool foundOtherName = ReadOtherFamilyNamesForFace(aPlatformFontList,
                                                               buffer);

           
           
           if (firstTime && foundOtherName) {
               mHasOtherFamilyNames = true;
               readAllFaces = true;
           }
           firstTime = false;
       }

       
       if (!readAllFaces && !aNeedFullnamePostscriptNames)
           break;
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
gfxFontFamily::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                   FontListSizes*    aSizes) const
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
            fe->SizeOfIncludingThis(aMallocSizeOf, aSizes);
        }
    }
}

void
gfxFontFamily::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                   FontListSizes*    aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}
 








NS_IMPL_ISUPPORTS1(gfxFontCache::MemoryReporter, nsIMemoryMultiReporter)

NS_MEMORY_REPORTER_MALLOC_SIZEOF_FUN(FontCacheMallocSizeOf)

NS_IMETHODIMP
gfxFontCache::MemoryReporter::GetName(nsACString &aName)
{
    aName.AssignLiteral("font-cache");
    return NS_OK;
}

NS_IMETHODIMP
gfxFontCache::MemoryReporter::CollectReports
    (nsIMemoryMultiReporterCallback* aCb,
     nsISupports* aClosure)
{
    FontCacheSizes sizes;

    gfxFontCache::GetCache()->SizeOfIncludingThis(&FontCacheMallocSizeOf,
                                                  &sizes);

    aCb->Callback(EmptyCString(),
                  NS_LITERAL_CSTRING("explicit/gfx/font-cache"),
                  nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
                  sizes.mFontInstances,
                  NS_LITERAL_CSTRING("Memory used for active font instances."),
                  aClosure);

    aCb->Callback(EmptyCString(),
                  NS_LITERAL_CSTRING("explicit/gfx/font-shaped-words"),
                  nsIMemoryReporter::KIND_HEAP, nsIMemoryReporter::UNITS_BYTES,
                  sizes.mShapedWords,
                  NS_LITERAL_CSTRING("Memory used to cache shaped glyph data."),
                  aClosure);

    return NS_OK;
}

NS_IMETHODIMP
gfxFontCache::MemoryReporter::GetExplicitNonHeap(int64_t* aAmount)
{
    
    *aAmount = 0;
    return NS_OK;
}



class MemoryPressureObserver MOZ_FINAL : public nsIObserver,
                                         public nsSupportsWeakReference
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOBSERVER
};

NS_IMPL_ISUPPORTS2(MemoryPressureObserver, nsIObserver, nsISupportsWeakReference)

NS_IMETHODIMP
MemoryPressureObserver::Observe(nsISupports *aSubject,
                                const char *aTopic,
                                const PRUnichar *someData)
{
    if (!nsCRT::strcmp(aTopic, "memory-pressure")) {
        gfxFontCache *fontCache = gfxFontCache::GetCache();
        if (fontCache) {
            fontCache->FlushShapedWordCaches();
        }
    }
    return NS_OK;
}

nsresult
gfxFontCache::Init()
{
    NS_ASSERTION(!gGlobalCache, "Where did this come from?");
    gGlobalCache = new gfxFontCache();
    if (!gGlobalCache) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    NS_RegisterMemoryMultiReporter(new MemoryReporter);
    return NS_OK;
}

void
gfxFontCache::Shutdown()
{
    delete gGlobalCache;
    gGlobalCache = nullptr;

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    printf("Textrun storage high water mark=%d\n", gTextRunStorageHighWaterMark);
    printf("Total number of fonts=%d\n", gFontCount);
    printf("Total glyph extents allocated=%d (size %d)\n", gGlyphExtentsCount,
            int(gGlyphExtentsCount*sizeof(gfxGlyphExtents)));
    printf("Total glyph extents width-storage size allocated=%d\n", gGlyphExtentsWidthsTotalSize);
    printf("Number of simple glyph extents eagerly requested=%d\n", gGlyphExtentsSetupEagerSimple);
    printf("Number of tight glyph extents eagerly requested=%d\n", gGlyphExtentsSetupEagerTight);
    printf("Number of tight glyph extents lazily requested=%d\n", gGlyphExtentsSetupLazyTight);
    printf("Number of simple glyph extent setups that fell back to tight=%d\n", gGlyphExtentsSetupFallBackToTight);
#endif
}

gfxFontCache::gfxFontCache()
    : nsExpirationTracker<gfxFont,3>(FONT_TIMEOUT_SECONDS * 1000)
{
    mFonts.Init();

    nsCOMPtr<nsIObserverService> obs = GetObserverService();
    if (obs) {
        obs->AddObserver(new MemoryPressureObserver, "memory-pressure", false);
    }

#if 0 
    mWordCacheExpirationTimer = do_CreateInstance("@mozilla.org/timer;1");
    if (mWordCacheExpirationTimer) {
        mWordCacheExpirationTimer->
            InitWithFuncCallback(WordCacheExpirationTimerCallback, this,
                                 SHAPED_WORD_TIMEOUT_SECONDS * 1000,
                                 nsITimer::TYPE_REPEATING_SLACK);
    }
#endif
}

gfxFontCache::~gfxFontCache()
{
    
    
    
    gfxUserFontSet::UserFontCache::Shutdown();

    if (mWordCacheExpirationTimer) {
        mWordCacheExpirationTimer->Cancel();
        mWordCacheExpirationTimer = nullptr;
    }

    
    AgeAllGenerations();
    
    NS_WARN_IF_FALSE(mFonts.Count() == 0,
                     "Fonts still alive while shutting down gfxFontCache");
    
    
    
}

bool
gfxFontCache::HashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    return aKey->mFontEntry == mFont->GetFontEntry() &&
           aKey->mStyle->Equals(*mFont->GetStyle());
}

already_AddRefed<gfxFont>
gfxFontCache::Lookup(const gfxFontEntry *aFontEntry,
                     const gfxFontStyle *aStyle)
{
    Key key(aFontEntry, aStyle);
    HashEntry *entry = mFonts.GetEntry(key);

    Telemetry::Accumulate(Telemetry::FONT_CACHE_HIT, entry != nullptr);
    if (!entry)
        return nullptr;

    gfxFont *font = entry->mFont;
    NS_ADDREF(font);
    return font;
}

void
gfxFontCache::AddNew(gfxFont *aFont)
{
    Key key(aFont->GetFontEntry(), aFont->GetStyle());
    HashEntry *entry = mFonts.PutEntry(key);
    if (!entry)
        return;
    gfxFont *oldFont = entry->mFont;
    entry->mFont = aFont;
    
    
    MOZ_ASSERT(entry == mFonts.GetEntry(key));
    
    
    if (oldFont && oldFont->GetExpirationState()->IsTracked()) {
        
        
        NS_ASSERTION(aFont != oldFont, "new font is tracked for expiry!");
        NotifyExpired(oldFont);
    }
}

void
gfxFontCache::NotifyReleased(gfxFont *aFont)
{
    nsresult rv = AddObject(aFont);
    if (NS_FAILED(rv)) {
        
        DestroyFont(aFont);
    }
    
    
    
    
    
}

void
gfxFontCache::NotifyExpired(gfxFont *aFont)
{
    aFont->ClearCachedWords();
    RemoveObject(aFont);
    DestroyFont(aFont);
}

void
gfxFontCache::DestroyFont(gfxFont *aFont)
{
    Key key(aFont->GetFontEntry(), aFont->GetStyle());
    HashEntry *entry = mFonts.GetEntry(key);
    if (entry && entry->mFont == aFont) {
        mFonts.RemoveEntry(key);
    }
    NS_ASSERTION(aFont->GetRefCount() == 0,
                 "Destroying with non-zero ref count!");
    delete aFont;
}


PLDHashOperator
gfxFontCache::AgeCachedWordsForFont(HashEntry* aHashEntry, void* aUserData)
{
    aHashEntry->mFont->AgeCachedWords();
    return PL_DHASH_NEXT;
}


void
gfxFontCache::WordCacheExpirationTimerCallback(nsITimer* aTimer, void* aCache)
{
    gfxFontCache* cache = static_cast<gfxFontCache*>(aCache);
    cache->mFonts.EnumerateEntries(AgeCachedWordsForFont, nullptr);
}


PLDHashOperator
gfxFontCache::ClearCachedWordsForFont(HashEntry* aHashEntry, void* aUserData)
{
    aHashEntry->mFont->ClearCachedWords();
    return PL_DHASH_NEXT;
}


size_t
gfxFontCache::SizeOfFontEntryExcludingThis(HashEntry*        aHashEntry,
                                           nsMallocSizeOfFun aMallocSizeOf,
                                           void*             aUserArg)
{
    HashEntry *entry = static_cast<HashEntry*>(aHashEntry);
    FontCacheSizes *sizes = static_cast<FontCacheSizes*>(aUserArg);
    entry->mFont->SizeOfExcludingThis(aMallocSizeOf, sizes);

    
    
    return 0;
}

void
gfxFontCache::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                  FontCacheSizes*   aSizes) const
{
    

    mFonts.SizeOfExcludingThis(SizeOfFontEntryExcludingThis,
                               aMallocSizeOf, aSizes);
}

void
gfxFontCache::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                  FontCacheSizes*   aSizes) const
{
    aSizes->mFontInstances += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}

 bool
gfxFontShaper::MergeFontFeatures(
    const nsTArray<gfxFontFeature>& aStyleRuleFeatures,
    const nsTArray<gfxFontFeature>& aFontFeatures,
    bool aDisableLigatures,
    nsDataHashtable<nsUint32HashKey,uint32_t>& aMergedFeatures)
{
    
    if (aStyleRuleFeatures.IsEmpty() &&
        aFontFeatures.IsEmpty() &&
        !aDisableLigatures) {
        return false;
    }

    aMergedFeatures.Init();

    
    
    if (aDisableLigatures) {
        aMergedFeatures.Put(HB_TAG('l','i','g','a'), 0);
        aMergedFeatures.Put(HB_TAG('c','l','i','g'), 0);
    }

    
    uint32_t i, count;

    count = aFontFeatures.Length();
    for (i = 0; i < count; i++) {
        const gfxFontFeature& feature = aFontFeatures.ElementAt(i);
        aMergedFeatures.Put(feature.mTag, feature.mValue);
    }

    
    count = aStyleRuleFeatures.Length();
    for (i = 0; i < count; i++) {
        const gfxFontFeature& feature = aStyleRuleFeatures.ElementAt(i);
        aMergedFeatures.Put(feature.mTag, feature.mValue);
    }

    return aMergedFeatures.Count() != 0;
}

void
gfxFont::RunMetrics::CombineWith(const RunMetrics& aOther, bool aOtherIsOnLeft)
{
    mAscent = std::max(mAscent, aOther.mAscent);
    mDescent = std::max(mDescent, aOther.mDescent);
    if (aOtherIsOnLeft) {
        mBoundingBox =
            (mBoundingBox + gfxPoint(aOther.mAdvanceWidth, 0)).Union(aOther.mBoundingBox);
    } else {
        mBoundingBox =
            mBoundingBox.Union(aOther.mBoundingBox + gfxPoint(mAdvanceWidth, 0));
    }
    mAdvanceWidth += aOther.mAdvanceWidth;
}

gfxFont::gfxFont(gfxFontEntry *aFontEntry, const gfxFontStyle *aFontStyle,
                 AntialiasOption anAAOption, cairo_scaled_font_t *aScaledFont) :
    mScaledFont(aScaledFont),
    mFontEntry(aFontEntry), mIsValid(true),
    mApplySyntheticBold(false),
    mStyle(*aFontStyle),
    mAdjustedSize(0.0),
    mFUnitsConvFactor(0.0f),
    mAntialiasOption(anAAOption)
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    ++gFontCount;
#endif
}

gfxFont::~gfxFont()
{
    uint32_t i, count = mGlyphExtentsArray.Length();
    
    
    
    for (i = 0; i < count; ++i) {
        delete mGlyphExtentsArray[i];
    }
}


PLDHashOperator
gfxFont::AgeCacheEntry(CacheHashEntry *aEntry, void *aUserData)
{
    if (!aEntry->mShapedWord) {
        NS_ASSERTION(aEntry->mShapedWord, "cache entry has no gfxShapedWord!");
        return PL_DHASH_REMOVE;
    }
    if (aEntry->mShapedWord->IncrementAge() == kShapedWordCacheMaxAge) {
        return PL_DHASH_REMOVE;
    }
    return PL_DHASH_NEXT;
}

hb_blob_t *
gfxFont::GetFontTable(uint32_t aTag) {
    hb_blob_t *blob;
    if (mFontEntry->GetExistingFontTable(aTag, &blob))
        return blob;

    FallibleTArray<uint8_t> buffer;
    bool haveTable = NS_SUCCEEDED(mFontEntry->GetFontTable(aTag, buffer));

    return mFontEntry->ShareFontTableAndGetBlob(aTag,
                                                haveTable ? &buffer : nullptr);
}





#define ToDeviceUnits(aAppUnits, aDevUnitsPerAppUnit) \
    (double(aAppUnits)*double(aDevUnitsPerAppUnit))

struct GlyphBuffer {
#define GLYPH_BUFFER_SIZE (2048/sizeof(cairo_glyph_t))
    cairo_glyph_t mGlyphBuffer[GLYPH_BUFFER_SIZE];
    unsigned int mNumGlyphs;

    GlyphBuffer()
        : mNumGlyphs(0) { }

    cairo_glyph_t *AppendGlyph() {
        return &mGlyphBuffer[mNumGlyphs++];
    }

    void Flush(cairo_t *aCR, gfxFont::DrawMode aDrawMode, bool aReverse,
               gfxTextObjectPaint *aObjectPaint,
               const gfxMatrix& aGlobalMatrix, bool aFinish = false) {
        
        
        if ((!aFinish && mNumGlyphs < GLYPH_BUFFER_SIZE) || !mNumGlyphs) {
            return;
        }

        if (aReverse) {
            for (uint32_t i = 0; i < mNumGlyphs/2; ++i) {
                cairo_glyph_t tmp = mGlyphBuffer[i];
                mGlyphBuffer[i] = mGlyphBuffer[mNumGlyphs - 1 - i];
                mGlyphBuffer[mNumGlyphs - 1 - i] = tmp;
            }
        }

        if (aDrawMode == gfxFont::GLYPH_PATH) {
            cairo_glyph_path(aCR, mGlyphBuffer, mNumGlyphs);
        } else {
            if ((aDrawMode & (gfxFont::GLYPH_STROKE | gfxFont::GLYPH_STROKE_UNDERNEATH)) ==
                             (gfxFont::GLYPH_STROKE | gfxFont::GLYPH_STROKE_UNDERNEATH)) {
                FlushStroke(aCR, aObjectPaint, aGlobalMatrix);
            }
            if (aDrawMode & gfxFont::GLYPH_FILL) {
                SAMPLE_LABEL("GlyphBuffer", "cairo_show_glyphs");
                nsRefPtr<gfxPattern> pattern;
                if (aObjectPaint &&
                    !!(pattern = aObjectPaint->GetFillPattern(aGlobalMatrix))) {
                    cairo_save(aCR);
                    cairo_set_source(aCR, pattern->CairoPattern());
                }

                cairo_show_glyphs(aCR, mGlyphBuffer, mNumGlyphs);

                if (pattern) {
                    cairo_restore(aCR);
                }
            }
            if ((aDrawMode & (gfxFont::GLYPH_STROKE | gfxFont::GLYPH_STROKE_UNDERNEATH)) ==
                              gfxFont::GLYPH_STROKE) {
                FlushStroke(aCR, aObjectPaint, aGlobalMatrix);
            }
        }

        mNumGlyphs = 0;
    }

private:
    void FlushStroke(cairo_t *aCR, gfxTextObjectPaint *aObjectPaint,
                     const gfxMatrix& aGlobalMatrix) {
        nsRefPtr<gfxPattern> pattern;
        if (aObjectPaint &&
            !!(pattern = aObjectPaint->GetStrokePattern(aGlobalMatrix))) {
            cairo_save(aCR);
            cairo_set_source(aCR, pattern->CairoPattern());
        }

        cairo_new_path(aCR);
        cairo_glyph_path(aCR, mGlyphBuffer, mNumGlyphs);
        cairo_stroke(aCR);

        if (pattern) {
            cairo_restore(aCR);
        }
    }

#undef GLYPH_BUFFER_SIZE
};

static AntialiasMode Get2DAAMode(gfxFont::AntialiasOption aAAOption) {
  switch (aAAOption) {
  case gfxFont::kAntialiasSubpixel:
    return AA_SUBPIXEL;
  case gfxFont::kAntialiasGrayscale:
    return AA_GRAY;
  case gfxFont::kAntialiasNone:
    return AA_NONE;
  default:
    return AA_DEFAULT;
  }
}

struct GlyphBufferAzure {
#define GLYPH_BUFFER_SIZE (2048/sizeof(Glyph))
    Glyph mGlyphBuffer[GLYPH_BUFFER_SIZE];
    unsigned int mNumGlyphs;

    GlyphBufferAzure()
        : mNumGlyphs(0) { }

    Glyph *AppendGlyph() {
        return &mGlyphBuffer[mNumGlyphs++];
    }

    void Flush(DrawTarget *aDT, gfxTextObjectPaint *aObjectPaint, ScaledFont *aFont,
               gfxFont::DrawMode aDrawMode, bool aReverse, const GlyphRenderingOptions *aOptions,
               gfxContext *aThebesContext, const Matrix *aInvFontMatrix, const DrawOptions &aDrawOptions,
               bool aFinish = false)
    {
        
        if ((!aFinish && mNumGlyphs < GLYPH_BUFFER_SIZE) || !mNumGlyphs) {
            return;
        }

        if (aReverse) {
            Glyph *begin = &mGlyphBuffer[0];
            Glyph *end = &mGlyphBuffer[mNumGlyphs];
            std::reverse(begin, end);
        }
        
        gfx::GlyphBuffer buf;
        buf.mGlyphs = mGlyphBuffer;
        buf.mNumGlyphs = mNumGlyphs;

        gfxContext::AzureState state = aThebesContext->CurrentState();
        if ((aDrawMode & (gfxFont::GLYPH_STROKE | gfxFont::GLYPH_STROKE_UNDERNEATH)) ==
                         (gfxFont::GLYPH_STROKE | gfxFont::GLYPH_STROKE_UNDERNEATH)) {
            FlushStroke(aDT, aObjectPaint, aFont, aThebesContext, buf, state);
        }
        if (aDrawMode & gfxFont::GLYPH_FILL) {
            if (state.pattern || aObjectPaint) {
                Pattern *pat;

                nsRefPtr<gfxPattern> fillPattern;
                if (!aObjectPaint ||
                    !(fillPattern = aObjectPaint->GetFillPattern(aThebesContext->CurrentMatrix()))) {
                    if (state.pattern) {
                        pat = state.pattern->GetPattern(aDT, state.patternTransformChanged ? &state.patternTransform : nullptr);
                    } else {
                        pat = nullptr;
                    }
                } else {
                    pat = fillPattern->GetPattern(aDT);
                }

                if (pat) {
                    Matrix saved;
                    Matrix *mat = nullptr;
                    if (aInvFontMatrix) {
                        
                        
                        

                        
                        
                        if (pat->GetType() == PATTERN_LINEAR_GRADIENT) {
                            mat = &static_cast<LinearGradientPattern*>(pat)->mMatrix;
                        } else if (pat->GetType() == PATTERN_RADIAL_GRADIENT) {
                            mat = &static_cast<RadialGradientPattern*>(pat)->mMatrix;
                        } else if (pat->GetType() == PATTERN_SURFACE) {
                            mat = &static_cast<SurfacePattern*>(pat)->mMatrix;
                        }

                        if (mat) {
                            saved = *mat;
                            *mat = (*mat) * (*aInvFontMatrix);
                        }
                    }

                    aDT->FillGlyphs(aFont, buf, *pat,
                                    aDrawOptions, aOptions);

                    if (mat) {
                        *mat = saved;
                    }
                }
            } else if (state.sourceSurface) {
                aDT->FillGlyphs(aFont, buf, SurfacePattern(state.sourceSurface,
                                                           EXTEND_CLAMP,
                                                           state.surfTransform),
                                aDrawOptions, aOptions);
            } else {
                aDT->FillGlyphs(aFont, buf, ColorPattern(state.color),
                                aDrawOptions, aOptions);
            }
        }
        if (aDrawMode & gfxFont::GLYPH_PATH) {
            aThebesContext->EnsurePathBuilder();
            aFont->CopyGlyphsToBuilder(buf, aThebesContext->mPathBuilder);
        }
        if ((aDrawMode & (gfxFont::GLYPH_STROKE | gfxFont::GLYPH_STROKE_UNDERNEATH)) ==
                          gfxFont::GLYPH_STROKE) {
            FlushStroke(aDT, aObjectPaint, aFont, aThebesContext, buf, state);
        }

        mNumGlyphs = 0;
    }

private:
    void FlushStroke(DrawTarget *aDT, gfxTextObjectPaint *aObjectPaint,
                     ScaledFont *aFont, gfxContext *aThebesContext,
                     gfx::GlyphBuffer& aBuf, gfxContext::AzureState& aState)
    {
        RefPtr<Path> path = aFont->GetPathForGlyphs(aBuf, aDT);
        if (aObjectPaint) {
            nsRefPtr<gfxPattern> strokePattern =
              aObjectPaint->GetStrokePattern(aThebesContext->CurrentMatrix());
            if (strokePattern) {
                aDT->Stroke(path, *strokePattern->GetPattern(aDT), aState.strokeOptions);
            }
        }
    }

#undef GLYPH_BUFFER_SIZE
};







double
gfxFont::CalcXScale(gfxContext *aContext)
{
    
    gfxSize t = aContext->UserToDevice(gfxSize(1.0, 0.0));
    if (t.width == 1.0 && t.height == 0.0) {
        
        return 1.0;
    }

    double m = sqrt(t.width * t.width + t.height * t.height);

    NS_ASSERTION(m != 0.0, "degenerate transform while synthetic bolding");
    if (m == 0.0) {
        return 0.0; 
    }

    
    return 1.0 / m;
}

static gfxFont::DrawMode
ForcePaintingDrawMode(gfxFont::DrawMode aDrawMode)
{
    return aDrawMode == gfxFont::GLYPH_PATH ?
        gfxFont::DrawMode(gfxFont::GLYPH_FILL | gfxFont::GLYPH_STROKE) :
        aDrawMode;
}

void
gfxFont::Draw(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
              gfxContext *aContext, DrawMode aDrawMode, gfxPoint *aPt,
              Spacing *aSpacing, gfxTextObjectPaint *aObjectPaint,
              gfxTextRunDrawCallbacks *aCallbacks)
{
    NS_ASSERTION(aDrawMode == gfxFont::GLYPH_PATH || !(aDrawMode & gfxFont::GLYPH_PATH),
                 "GLYPH_PATH cannot be used with GLYPH_FILL, GLYPH_STROKE or GLYPH_STROKE_UNDERNEATH");

    if (aStart >= aEnd)
        return;

    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    const int32_t appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    const double devUnitsPerAppUnit = 1.0/double(appUnitsPerDevUnit);
    bool isRTL = aTextRun->IsRightToLeft();
    double direction = aTextRun->GetDirection();
    gfxMatrix globalMatrix = aContext->CurrentMatrix();

    bool haveSVGGlyphs = GetFontEntry()->TryGetSVGData();
    nsAutoPtr<gfxTextObjectPaint> objectPaint;
    if (haveSVGGlyphs && !aObjectPaint) {
        
        NS_ASSERTION((aDrawMode & GLYPH_STROKE) == 0, "no pattern supplied for stroking text");
        nsRefPtr<gfxPattern> fillPattern = aContext->GetPattern();
        objectPaint = new SimpleTextObjectPaint(fillPattern, nullptr,
                                                aContext->CurrentMatrix());
        aObjectPaint = objectPaint;
    }

    
    
    double synBoldOnePixelOffset = 0;
    int32_t strikes = 0;
    if (IsSyntheticBold()) {
        double xscale = CalcXScale(aContext);
        synBoldOnePixelOffset = direction * xscale;
        
        strikes = NS_lroundf(GetSyntheticBoldOffset() / xscale);
    }

    uint32_t i;
    
    double x = aPt->x;
    double y = aPt->y;

    cairo_t *cr = aContext->GetCairo();
    RefPtr<DrawTarget> dt = aContext->GetDrawTarget();

    bool paintSVGGlyphs = !aCallbacks || aCallbacks->mShouldPaintSVGGlyphs;
    bool emittedGlyphs = false;

    if (aContext->IsCairo()) {
      bool success = SetupCairoFont(aContext);
      if (MOZ_UNLIKELY(!success))
          return;

      ::GlyphBuffer glyphs;
      cairo_glyph_t *glyph;

      if (aSpacing) {
          x += direction*aSpacing[0].mBefore;
      }
      for (i = aStart; i < aEnd; ++i) {
          const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
          if (glyphData->IsSimpleGlyph()) {
              double advance = glyphData->GetSimpleAdvance();
              double glyphX;
              if (isRTL) {
                  x -= advance;
                  glyphX = x;
              } else {
                  glyphX = x;
                  x += advance;
              }

              if (haveSVGGlyphs) {
                  if (!paintSVGGlyphs) {
                      continue;
                  }
                  gfxPoint point(ToDeviceUnits(glyphX, devUnitsPerAppUnit),
                                 ToDeviceUnits(y, devUnitsPerAppUnit));
                  gfxFont::DrawMode mode = ForcePaintingDrawMode(aDrawMode);
                  if (RenderSVGGlyph(aContext, point, mode,
                                     glyphData->GetSimpleGlyph(), aObjectPaint,
                                     aCallbacks, emittedGlyphs)) {
                      continue;
                  }
              }

              
              
              
              
              
              glyph = glyphs.AppendGlyph();
              glyph->index = glyphData->GetSimpleGlyph();
              glyph->x = ToDeviceUnits(glyphX, devUnitsPerAppUnit);
              glyph->y = ToDeviceUnits(y, devUnitsPerAppUnit);
              glyphs.Flush(cr, aDrawMode, isRTL, aObjectPaint, globalMatrix);
            
              
              
              if (IsSyntheticBold()) {
                  double strikeOffset = synBoldOnePixelOffset;
                  int32_t strikeCount = strikes;
                  do {
                      cairo_glyph_t *doubleglyph;
                      doubleglyph = glyphs.AppendGlyph();
                      doubleglyph->index = glyph->index;
                      doubleglyph->x =
                          ToDeviceUnits(glyphX + strikeOffset * appUnitsPerDevUnit,
                                        devUnitsPerAppUnit);
                      doubleglyph->y = glyph->y;
                      strikeOffset += synBoldOnePixelOffset;
                      glyphs.Flush(cr, aDrawMode, isRTL, aObjectPaint, globalMatrix);
                  } while (--strikeCount > 0);
              }
              emittedGlyphs = true;
          } else {
              uint32_t glyphCount = glyphData->GetGlyphCount();
              if (glyphCount > 0) {
                  const gfxTextRun::DetailedGlyph *details =
                      aTextRun->GetDetailedGlyphs(i);
                  NS_ASSERTION(details, "detailedGlyph should not be missing!");
                  double advance;
                  for (uint32_t j = 0; j < glyphCount; ++j, ++details, x += direction * advance) {
                      advance = details->mAdvance;
                      if (glyphData->IsMissing()) {
                          
                          
                          if (aDrawMode != gfxFont::GLYPH_PATH && advance > 0) {
                              double glyphX = x;
                              if (isRTL) {
                                  glyphX -= advance;
                              }
                              gfxPoint pt(ToDeviceUnits(glyphX, devUnitsPerAppUnit),
                                          ToDeviceUnits(y, devUnitsPerAppUnit));
                              gfxFloat advanceDevUnits = ToDeviceUnits(advance, devUnitsPerAppUnit);
                              gfxFloat height = GetMetrics().maxAscent;
                              gfxRect glyphRect(pt.x, pt.y - height, advanceDevUnits, height);
                              gfxFontMissingGlyphs::DrawMissingGlyph(aContext,
                                                                     glyphRect,
                                                                     details->mGlyphID,
                                                                     appUnitsPerDevUnit);
                          }
                      } else {
                          double glyphX = x + details->mXOffset;
                          if (isRTL) {
                              glyphX -= advance;
                          }

                          gfxPoint point(ToDeviceUnits(glyphX, devUnitsPerAppUnit),
                                         ToDeviceUnits(y, devUnitsPerAppUnit));

                          if (haveSVGGlyphs) {
                              if (!paintSVGGlyphs) {
                                  continue;
                              }
                              gfxFont::DrawMode mode = ForcePaintingDrawMode(aDrawMode);
                              if (RenderSVGGlyph(aContext, point, mode,
                                                  details->mGlyphID,
                                                  aObjectPaint, aCallbacks,
                                                  emittedGlyphs)) {
                                  continue;
                              }
                          }

                          glyph = glyphs.AppendGlyph();
                          glyph->index = details->mGlyphID;
                          glyph->x = ToDeviceUnits(glyphX, devUnitsPerAppUnit);
                          glyph->y = ToDeviceUnits(y + details->mYOffset, devUnitsPerAppUnit);
                          glyphs.Flush(cr, aDrawMode, isRTL, aObjectPaint, globalMatrix);

                          if (IsSyntheticBold()) {
                              double strikeOffset = synBoldOnePixelOffset;
                              int32_t strikeCount = strikes;
                              do {
                                  cairo_glyph_t *doubleglyph;
                                  doubleglyph = glyphs.AppendGlyph();
                                  doubleglyph->index = glyph->index;
                                  doubleglyph->x =
                                      ToDeviceUnits(glyphX + strikeOffset *
                                              appUnitsPerDevUnit,
                                              devUnitsPerAppUnit);
                                  doubleglyph->y = glyph->y;
                                  strikeOffset += synBoldOnePixelOffset;
                                  glyphs.Flush(cr, aDrawMode, isRTL, aObjectPaint, globalMatrix);
                              } while (--strikeCount > 0);
                          }
                          emittedGlyphs = true;
                      }
                  }
              }
          }

          if (aSpacing) {
              double space = aSpacing[i - aStart].mAfter;
              if (i + 1 < aEnd) {
                  space += aSpacing[i + 1 - aStart].mBefore;
              }
              x += direction*space;
          }
      }

      if (gfxFontTestStore::CurrentStore()) {
          



          gfxFontTestStore::CurrentStore()->AddItem(GetName(),
                                                    glyphs.mGlyphBuffer,
                                                    glyphs.mNumGlyphs);
      }

      
      glyphs.Flush(cr, aDrawMode, isRTL, aObjectPaint, globalMatrix, true);
      if (aCallbacks && emittedGlyphs) {
          aCallbacks->NotifyGlyphPathEmitted();
      }

    } else {
      RefPtr<ScaledFont> scaledFont = GetScaledFont(dt);

      if (!scaledFont) {
        return;
      }

      bool oldSubpixelAA = dt->GetPermitSubpixelAA();

      if (!AllowSubpixelAA()) {
          dt->SetPermitSubpixelAA(false);
      }

      GlyphBufferAzure glyphs;
      Glyph *glyph;

      Matrix mat, matInv;
      Matrix oldMat = dt->GetTransform();

      
      
      Matrix *passedInvMatrix = nullptr;

      RefPtr<GlyphRenderingOptions> renderingOptions =
        GetGlyphRenderingOptions();

      DrawOptions drawOptions;
      drawOptions.mAntialiasMode = Get2DAAMode(mAntialiasOption);

      if (mScaledFont) {
        cairo_matrix_t matrix;
        cairo_scaled_font_get_font_matrix(mScaledFont, &matrix);
        if (matrix.xy != 0) {
          
          
          
          
          
          
          mat = ToMatrix(*reinterpret_cast<gfxMatrix*>(&matrix));

          mat._11 = mat._22 = 1.0;
          float adjustedSize = mAdjustedSize > 0 ? mAdjustedSize : GetStyle()->size;
          mat._21 /= adjustedSize;

          dt->SetTransform(mat * oldMat);

          matInv = mat;
          matInv.Invert();

          passedInvMatrix = &matInv;
        }
      }

      if (aSpacing) {
          x += direction*aSpacing[0].mBefore;
      }
      for (i = aStart; i < aEnd; ++i) {
          const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
          if (glyphData->IsSimpleGlyph()) {
              double advance = glyphData->GetSimpleAdvance();
              double glyphX;
              if (isRTL) {
                  x -= advance;
                  glyphX = x;
              } else {
                  glyphX = x;
                  x += advance;
              }

              if (haveSVGGlyphs) {
                  if (!paintSVGGlyphs) {
                      continue;
                  }
                  gfxPoint point(ToDeviceUnits(glyphX, devUnitsPerAppUnit),
                                 ToDeviceUnits(y, devUnitsPerAppUnit));
                  gfxFont::DrawMode mode = ForcePaintingDrawMode(aDrawMode);
                  if (RenderSVGGlyph(aContext, point, mode,
                                     glyphData->GetSimpleGlyph(), aObjectPaint,
                                     aCallbacks, emittedGlyphs)) {
                      continue;
                  }
              }

              
              
              
              
              
              glyph = glyphs.AppendGlyph();
              glyph->mIndex = glyphData->GetSimpleGlyph();
              glyph->mPosition.x = ToDeviceUnits(glyphX, devUnitsPerAppUnit);
              glyph->mPosition.y = ToDeviceUnits(y, devUnitsPerAppUnit);
              glyph->mPosition = matInv * glyph->mPosition;
              glyphs.Flush(dt, aObjectPaint, scaledFont,
                           aDrawMode, isRTL, renderingOptions,
                           aContext, passedInvMatrix,
                           drawOptions);

              
              
              if (IsSyntheticBold()) {
                  double strikeOffset = synBoldOnePixelOffset;
                  int32_t strikeCount = strikes;
                  do {
                      Glyph *doubleglyph;
                      doubleglyph = glyphs.AppendGlyph();
                      doubleglyph->mIndex = glyph->mIndex;
                      doubleglyph->mPosition.x =
                          ToDeviceUnits(glyphX + strikeOffset * appUnitsPerDevUnit,
                                        devUnitsPerAppUnit);
                      doubleglyph->mPosition.y = glyph->mPosition.y;
                      doubleglyph->mPosition = matInv * doubleglyph->mPosition;
                      strikeOffset += synBoldOnePixelOffset;
                      glyphs.Flush(dt, aObjectPaint, scaledFont,
                                   aDrawMode, isRTL, renderingOptions,
                                   aContext, passedInvMatrix,
                                   drawOptions);
                  } while (--strikeCount > 0);
              }
              emittedGlyphs = true;
          } else {
              uint32_t glyphCount = glyphData->GetGlyphCount();
              if (glyphCount > 0) {
                  const gfxTextRun::DetailedGlyph *details =
                      aTextRun->GetDetailedGlyphs(i);
                  NS_ASSERTION(details, "detailedGlyph should not be missing!");
                  double advance;
                  for (uint32_t j = 0; j < glyphCount; ++j, ++details, x += direction * advance) {
                      advance = details->mAdvance;
                      if (glyphData->IsMissing()) {
                          
                          
                          if (aDrawMode != gfxFont::GLYPH_PATH && advance > 0) {
                              double glyphX = x;
                              if (isRTL) {
                                  glyphX -= advance;
                              }
                              gfxPoint pt(ToDeviceUnits(glyphX, devUnitsPerAppUnit),
                                          ToDeviceUnits(y, devUnitsPerAppUnit));
                              gfxFloat advanceDevUnits = ToDeviceUnits(advance, devUnitsPerAppUnit);
                              gfxFloat height = GetMetrics().maxAscent;
                              gfxRect glyphRect(pt.x, pt.y - height, advanceDevUnits, height);
                              gfxFontMissingGlyphs::DrawMissingGlyph(aContext,
                                                                     glyphRect,
                                                                     details->mGlyphID,
                                                                     appUnitsPerDevUnit);
                          }
                      } else {
                          double glyphX = x + details->mXOffset;
                          if (isRTL) {
                              glyphX -= advance;
                          }

                          gfxPoint point(ToDeviceUnits(glyphX, devUnitsPerAppUnit),
                                         ToDeviceUnits(y, devUnitsPerAppUnit));

                          if (haveSVGGlyphs) {
                              if (!paintSVGGlyphs) {
                                  continue;
                              }
                              gfxFont::DrawMode mode = ForcePaintingDrawMode(aDrawMode);
                              if (RenderSVGGlyph(aContext, point, mode,
                                                 details->mGlyphID,
                                                 aObjectPaint, aCallbacks,
                                                 emittedGlyphs)) {
                                  continue;
                              }
                          }

                          glyph = glyphs.AppendGlyph();
                          glyph->mIndex = details->mGlyphID;
                          glyph->mPosition.x = ToDeviceUnits(glyphX, devUnitsPerAppUnit);
                          glyph->mPosition.y = ToDeviceUnits(y + details->mYOffset, devUnitsPerAppUnit);
                          glyph->mPosition = matInv * glyph->mPosition;
                          glyphs.Flush(dt, aObjectPaint, scaledFont, aDrawMode,
                                       isRTL, renderingOptions, aContext, passedInvMatrix,
                                       drawOptions);

                          if (IsSyntheticBold()) {
                              double strikeOffset = synBoldOnePixelOffset;
                              int32_t strikeCount = strikes;
                              do {
                                  Glyph *doubleglyph;
                                  doubleglyph = glyphs.AppendGlyph();
                                  doubleglyph->mIndex = glyph->mIndex;
                                  doubleglyph->mPosition.x =
                                      ToDeviceUnits(glyphX + strikeOffset *
                                                    appUnitsPerDevUnit,
                                                    devUnitsPerAppUnit);
                                  doubleglyph->mPosition.y = glyph->mPosition.y;
                                  strikeOffset += synBoldOnePixelOffset;
                                  doubleglyph->mPosition = matInv * doubleglyph->mPosition;
                                  glyphs.Flush(dt, aObjectPaint, scaledFont,
                                               aDrawMode, isRTL, renderingOptions,
                                               aContext, passedInvMatrix, drawOptions);
                              } while (--strikeCount > 0);
                          }
                          emittedGlyphs = true;
                      }
                  }
              }
          }

          if (aSpacing) {
              double space = aSpacing[i - aStart].mAfter;
              if (i + 1 < aEnd) {
                  space += aSpacing[i + 1 - aStart].mBefore;
              }
              x += direction*space;
          }
      }

      glyphs.Flush(dt, aObjectPaint, scaledFont, aDrawMode, isRTL,
                   renderingOptions, aContext, passedInvMatrix,
                   drawOptions, true);
      if (aCallbacks && emittedGlyphs) {
          aCallbacks->NotifyGlyphPathEmitted();
      }

      dt->SetTransform(oldMat);

      dt->SetPermitSubpixelAA(oldSubpixelAA);
    }

    *aPt = gfxPoint(x, y);
}

bool
gfxFont::RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextObjectPaint *aObjectPaint)
{
    if (!GetFontEntry()->HasSVGGlyph(aGlyphId)) {
        return false;
    }

    const gfxFloat devUnitsPerSVGUnit = GetStyle()->size / gfxSVGGlyphs::SVG_UNITS_PER_EM;
    gfxContextMatrixAutoSaveRestore matrixRestore(aContext);

    aContext->Translate(gfxPoint(aPoint.x, aPoint.y));
    aContext->Scale(devUnitsPerSVGUnit, devUnitsPerSVGUnit);

    aObjectPaint->InitStrokeGeometry(aContext, devUnitsPerSVGUnit);

    return GetFontEntry()->RenderSVGGlyph(aContext, aGlyphId, aDrawMode,
                                          aObjectPaint);
}

bool
gfxFont::RenderSVGGlyph(gfxContext *aContext, gfxPoint aPoint, DrawMode aDrawMode,
                        uint32_t aGlyphId, gfxTextObjectPaint *aObjectPaint,
                        gfxTextRunDrawCallbacks *aCallbacks,
                        bool& aEmittedGlyphs)
{
    if (aCallbacks) {
        if (aEmittedGlyphs) {
            aCallbacks->NotifyGlyphPathEmitted();
            aEmittedGlyphs = false;
        }
        aCallbacks->NotifyBeforeSVGGlyphPainted();
    }
    bool rendered = RenderSVGGlyph(aContext, aPoint, aDrawMode, aGlyphId,
                                   aObjectPaint);
    if (aCallbacks) {
        aCallbacks->NotifyAfterSVGGlyphPainted();
    }
    return rendered;
}

static void
UnionRange(gfxFloat aX, gfxFloat* aDestMin, gfxFloat* aDestMax)
{
    *aDestMin = std::min(*aDestMin, aX);
    *aDestMax = std::max(*aDestMax, aX);
}




static bool
NeedsGlyphExtents(gfxFont *aFont, gfxTextRun *aTextRun)
{
    return (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX) ||
        aFont->GetFontEntry()->IsUserFont();
}

static bool
NeedsGlyphExtents(gfxTextRun *aTextRun)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX)
        return true;
    uint32_t numRuns;
    const gfxTextRun::GlyphRun *glyphRuns = aTextRun->GetGlyphRuns(&numRuns);
    for (uint32_t i = 0; i < numRuns; ++i) {
        if (glyphRuns[i].mFont->GetFontEntry()->IsUserFont())
            return true;
    }
    return false;
}

gfxFont::RunMetrics
gfxFont::Measure(gfxTextRun *aTextRun,
                 uint32_t aStart, uint32_t aEnd,
                 BoundingBoxType aBoundingBoxType,
                 gfxContext *aRefContext,
                 Spacing *aSpacing)
{
    
    
    
    
    if (aBoundingBoxType == TIGHT_HINTED_OUTLINE_EXTENTS &&
        mAntialiasOption != kAntialiasNone) {
        if (!mNonAAFont) {
            mNonAAFont = CopyWithAntialiasOption(kAntialiasNone);
        }
        
        
        if (mNonAAFont) {
            return mNonAAFont->Measure(aTextRun, aStart, aEnd,
                                       TIGHT_HINTED_OUTLINE_EXTENTS,
                                       aRefContext, aSpacing);
        }
    }

    const int32_t appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    
    const gfxFont::Metrics& fontMetrics = GetMetrics();

    RunMetrics metrics;
    metrics.mAscent = fontMetrics.maxAscent*appUnitsPerDevUnit;
    metrics.mDescent = fontMetrics.maxDescent*appUnitsPerDevUnit;
    if (aStart == aEnd) {
        
        metrics.mBoundingBox = gfxRect(0, -metrics.mAscent, 0, metrics.mAscent + metrics.mDescent);
        return metrics;
    }

    gfxFloat advanceMin = 0, advanceMax = 0;
    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    bool isRTL = aTextRun->IsRightToLeft();
    double direction = aTextRun->GetDirection();
    bool needsGlyphExtents = NeedsGlyphExtents(this, aTextRun);
    gfxGlyphExtents *extents =
        (aBoundingBoxType == LOOSE_INK_EXTENTS &&
            !needsGlyphExtents &&
            !aTextRun->HasDetailedGlyphs()) ? nullptr
        : GetOrCreateGlyphExtents(aTextRun->GetAppUnitsPerDevUnit());
    double x = 0;
    if (aSpacing) {
        x += direction*aSpacing[0].mBefore;
    }
    uint32_t i;
    for (i = aStart; i < aEnd; ++i) {
        const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
        if (glyphData->IsSimpleGlyph()) {
            double advance = glyphData->GetSimpleAdvance();
            
            
            if ((aBoundingBoxType != LOOSE_INK_EXTENTS || needsGlyphExtents) &&
                extents) {
                uint32_t glyphIndex = glyphData->GetSimpleGlyph();
                uint16_t extentsWidth = extents->GetContainedGlyphWidthAppUnits(glyphIndex);
                if (extentsWidth != gfxGlyphExtents::INVALID_WIDTH &&
                    aBoundingBoxType == LOOSE_INK_EXTENTS) {
                    UnionRange(x, &advanceMin, &advanceMax);
                    UnionRange(x + direction*extentsWidth, &advanceMin, &advanceMax);
                } else {
                    gfxRect glyphRect;
                    if (!extents->GetTightGlyphExtentsAppUnits(this,
                            aRefContext, glyphIndex, &glyphRect)) {
                        glyphRect = gfxRect(0, metrics.mBoundingBox.Y(),
                            advance, metrics.mBoundingBox.Height());
                    }
                    if (isRTL) {
                        glyphRect -= gfxPoint(advance, 0);
                    }
                    glyphRect += gfxPoint(x, 0);
                    metrics.mBoundingBox = metrics.mBoundingBox.Union(glyphRect);
                }
            }
            x += direction*advance;
        } else {
            uint32_t glyphCount = glyphData->GetGlyphCount();
            if (glyphCount > 0) {
                const gfxTextRun::DetailedGlyph *details =
                    aTextRun->GetDetailedGlyphs(i);
                NS_ASSERTION(details != nullptr,
                             "detaiedGlyph record should not be missing!");
                uint32_t j;
                for (j = 0; j < glyphCount; ++j, ++details) {
                    uint32_t glyphIndex = details->mGlyphID;
                    gfxPoint glyphPt(x + details->mXOffset, details->mYOffset);
                    double advance = details->mAdvance;
                    gfxRect glyphRect;
                    if (glyphData->IsMissing() || !extents ||
                        !extents->GetTightGlyphExtentsAppUnits(this,
                                aRefContext, glyphIndex, &glyphRect)) {
                        
                        
                        glyphRect = gfxRect(0, -metrics.mAscent,
                            advance, metrics.mAscent + metrics.mDescent);
                    }
                    if (isRTL) {
                        glyphRect -= gfxPoint(advance, 0);
                    }
                    glyphRect += gfxPoint(x, 0);
                    metrics.mBoundingBox = metrics.mBoundingBox.Union(glyphRect);
                    x += direction*advance;
                }
            }
        }
        
        if (aSpacing) {
            double space = aSpacing[i - aStart].mAfter;
            if (i + 1 < aEnd) {
                space += aSpacing[i + 1 - aStart].mBefore;
            }
            x += direction*space;
        }
    }

    if (aBoundingBoxType == LOOSE_INK_EXTENTS) {
        UnionRange(x, &advanceMin, &advanceMax);
        gfxRect fontBox(advanceMin, -metrics.mAscent,
                        advanceMax - advanceMin, metrics.mAscent + metrics.mDescent);
        metrics.mBoundingBox = metrics.mBoundingBox.Union(fontBox);
    }
    if (isRTL) {
        metrics.mBoundingBox -= gfxPoint(x, 0);
    }

    metrics.mAdvanceWidth = x*direction;
    return metrics;
}

static bool
IsBoundarySpace(PRUnichar aChar, PRUnichar aNextChar)
{
    return (aChar == ' ' || aChar == 0x00A0) && !IsClusterExtender(aNextChar);
}

static inline uint32_t
HashMix(uint32_t aHash, PRUnichar aCh)
{
    return (aHash >> 28) ^ (aHash << 4) ^ aCh;
}

template<typename T>
gfxShapedWord*
gfxFont::GetShapedWord(gfxContext *aContext,
                       const T    *aText,
                       uint32_t    aLength,
                       uint32_t    aHash,
                       int32_t     aRunScript,
                       int32_t     aAppUnitsPerDevUnit,
                       uint32_t    aFlags)
{
    
    if (mWordCache.Count() > 10000) {
        NS_WARNING("flushing shaped-word cache");
        ClearCachedWords();
    }

    
    CacheHashKey key(aText, aLength, aHash,
                     aRunScript,
                     aAppUnitsPerDevUnit,
                     aFlags);

    CacheHashEntry *entry = mWordCache.PutEntry(key);
    if (!entry) {
        NS_WARNING("failed to create word cache entry - expect missing text");
        return nullptr;
    }
    gfxShapedWord *sw = entry->mShapedWord;

    bool isContent = !mStyle.systemFont;

    if (sw) {
        sw->ResetAge();
        Telemetry::Accumulate((isContent ? Telemetry::WORD_CACHE_HITS_CONTENT :
                                   Telemetry::WORD_CACHE_HITS_CHROME),
                              aLength);
        return sw;
    }

    Telemetry::Accumulate((isContent ? Telemetry::WORD_CACHE_MISSES_CONTENT :
                               Telemetry::WORD_CACHE_MISSES_CHROME),
                          aLength);
    sw = entry->mShapedWord = gfxShapedWord::Create(aText, aLength,
                                                    aRunScript,
                                                    aAppUnitsPerDevUnit,
                                                    aFlags);
    if (!sw) {
        NS_WARNING("failed to create gfxShapedWord - expect missing text");
        return nullptr;
    }

    DebugOnly<bool> ok =
        ShapeText(aContext, aText, 0, aLength, aRunScript, sw);

    NS_WARN_IF_FALSE(ok, "failed to shape word - expect garbled text");

    return sw;
}

bool
gfxFont::CacheHashEntry::KeyEquals(const KeyTypePointer aKey) const
{
    const gfxShapedWord *sw = mShapedWord;
    if (!sw) {
        return false;
    }
    if (sw->GetLength() != aKey->mLength ||
        sw->Flags() != aKey->mFlags ||
        sw->GetAppUnitsPerDevUnit() != aKey->mAppUnitsPerDevUnit ||
        sw->Script() != aKey->mScript) {
        return false;
    }
    if (sw->TextIs8Bit()) {
        if (aKey->mTextIs8Bit) {
            return (0 == memcmp(sw->Text8Bit(), aKey->mText.mSingle,
                                aKey->mLength * sizeof(uint8_t)));
        }
        
        
        
        const uint8_t   *s1 = sw->Text8Bit();
        const PRUnichar *s2 = aKey->mText.mDouble;
        const PRUnichar *s2end = s2 + aKey->mLength;
        while (s2 < s2end) {
            if (*s1++ != *s2++) {
                return false;
            }
        }
        return true;
    }
    NS_ASSERTION((aKey->mFlags & gfxTextRunFactory::TEXT_IS_8BIT) == 0 &&
                 !aKey->mTextIs8Bit, "didn't expect 8-bit text here");
    return (0 == memcmp(sw->TextUnicode(), aKey->mText.mDouble,
                        aKey->mLength * sizeof(PRUnichar)));
}

bool
gfxFont::ShapeText(gfxContext    *aContext,
                   const uint8_t *aText,
                   uint32_t       aOffset,
                   uint32_t       aLength,
                   int32_t        aScript,
                   gfxShapedText *aShapedText,
                   bool           aPreferPlatformShaping)
{
    nsDependentCSubstring ascii((const char*)aText, aLength);
    nsAutoString utf16;
    AppendASCIItoUTF16(ascii, utf16);
    if (utf16.Length() != aLength) {
        return false;
    }
    return ShapeText(aContext, utf16.BeginReading(), aOffset, aLength,
                     aScript, aShapedText, aPreferPlatformShaping);
}

bool
gfxFont::ShapeText(gfxContext      *aContext,
                   const PRUnichar *aText,
                   uint32_t         aOffset,
                   uint32_t         aLength,
                   int32_t          aScript,
                   gfxShapedText   *aShapedText,
                   bool             aPreferPlatformShaping)
{
    bool ok = false;

    if (mGraphiteShaper && gfxPlatform::GetPlatform()->UseGraphiteShaping()) {
        ok = mGraphiteShaper->ShapeText(aContext, aText, aOffset, aLength,
                                        aScript, aShapedText);
    }

    if (!ok && mHarfBuzzShaper && !aPreferPlatformShaping) {
        if (gfxPlatform::GetPlatform()->UseHarfBuzzForScript(aScript)) {
            ok = mHarfBuzzShaper->ShapeText(aContext, aText, aOffset, aLength,
                                            aScript, aShapedText);
        }
    }

    if (!ok) {
        if (!mPlatformShaper) {
            CreatePlatformShaper();
            NS_ASSERTION(mPlatformShaper, "no platform shaper available!");
        }
        if (mPlatformShaper) {
            ok = mPlatformShaper->ShapeText(aContext, aText, aOffset, aLength,
                                            aScript, aShapedText);
        }
    }

    PostShapingFixup(aContext, aText, aOffset, aLength, aShapedText);

    return ok;
}

void
gfxFont::PostShapingFixup(gfxContext      *aContext,
                          const PRUnichar *aText,
                          uint32_t         aOffset,
                          uint32_t         aLength,
                          gfxShapedText   *aShapedText)
{
    if (IsSyntheticBold()) {
        float synBoldOffset =
                GetSyntheticBoldOffset() * CalcXScale(aContext);
        aShapedText->AdjustAdvancesForSyntheticBold(synBoldOffset,
                                                    aOffset, aLength);
    }
}

#define MAX_SHAPING_LENGTH  32760 // slightly less than 32K, trying to avoid
                                  
#define BACKTRACK_LIMIT     16 // backtrack this far looking for a good place
                               

template<typename T>
bool
gfxFont::ShapeFragmentWithoutWordCache(gfxContext *aContext,
                                       const T    *aText,
                                       uint32_t    aOffset,
                                       uint32_t    aLength,
                                       int32_t     aScript,
                                       gfxTextRun *aTextRun)
{
    aTextRun->SetupClusterBoundaries(aOffset, aText, aLength);

    bool ok = true;

    while (ok && aLength > 0) {
        uint32_t fragLen = aLength;

        
        if (fragLen > MAX_SHAPING_LENGTH) {
            fragLen = MAX_SHAPING_LENGTH;

            
            
            if (sizeof(T) == sizeof(PRUnichar)) {
                uint32_t i;
                for (i = 0; i < BACKTRACK_LIMIT; ++i) {
                    if (aTextRun->IsClusterStart(aOffset + fragLen - i)) {
                        fragLen -= i;
                        break;
                    }
                }
                if (i == BACKTRACK_LIMIT) {
                    
                    
                    
                    if (NS_IS_LOW_SURROGATE(aText[fragLen]) &&
                        NS_IS_HIGH_SURROGATE(aText[fragLen - 1])) {
                        --fragLen;
                    }
                }
            }
        }

        ok = ShapeText(aContext, aText, aOffset, fragLen, aScript, aTextRun);

        aText += fragLen;
        aOffset += fragLen;
        aLength -= fragLen;
    }

    return ok;
}

template<typename T>
bool
gfxFont::ShapeTextWithoutWordCache(gfxContext *aContext,
                                   const T    *aText,
                                   uint32_t    aOffset,
                                   uint32_t    aLength,
                                   int32_t     aScript,
                                   gfxTextRun *aTextRun)
{
    uint32_t fragStart = 0;
    bool ok = true;

    for (uint32_t i = 0; i <= aLength && ok; ++i) {
        T ch = (i < aLength) ? aText[i] : '\n';
        bool invalid = gfxFontGroup::IsInvalidChar(ch);
        uint32_t length = i - fragStart;

        
        if (!invalid) {
            continue;
        }

        if (length > 0) {
            ok = ShapeFragmentWithoutWordCache(aContext, aText + fragStart,
                                               aOffset + fragStart, length,
                                               aScript, aTextRun);
        }

        if (i == aLength) {
            break;
        }

        
        
        if (ch == '\t') {
            aTextRun->SetIsTab(aOffset + i);
        } else if (ch == '\n') {
            aTextRun->SetIsNewline(aOffset + i);
        }
        fragStart = i + 1;
    }

    NS_WARN_IF_FALSE(ok, "failed to shape text - expect garbled text");
    return ok;
}

inline static bool IsChar8Bit(uint8_t ) { return true; }
inline static bool IsChar8Bit(PRUnichar aCh) { return aCh < 0x100; }

template<typename T>
bool
gfxFont::SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const T *aString,
                             uint32_t aRunStart,
                             uint32_t aRunLength,
                             int32_t aRunScript)
{
    if (aRunLength == 0) {
        return true;
    }

    InitWordCache();

    
    uint32_t flags = aTextRun->GetFlags() &
        (gfxTextRunFactory::TEXT_IS_RTL |
         gfxTextRunFactory::TEXT_DISABLE_OPTIONAL_LIGATURES);
    if (sizeof(T) == sizeof(uint8_t)) {
        flags |= gfxTextRunFactory::TEXT_IS_8BIT;
    }

    const T *text = aString + aRunStart;
    uint32_t wordStart = 0;
    uint32_t hash = 0;
    bool wordIs8Bit = true;
    int32_t appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();

    T nextCh = text[0];
    for (uint32_t i = 0; i <= aRunLength; ++i) {
        T ch = nextCh;
        nextCh = (i < aRunLength - 1) ? text[i + 1] : '\n';
        bool boundary = IsBoundarySpace(ch, nextCh);
        bool invalid = !boundary && gfxFontGroup::IsInvalidChar(ch);
        uint32_t length = i - wordStart;

        
        
        
        if (!boundary && !invalid) {
            if (!IsChar8Bit(ch)) {
                wordIs8Bit = false;
            }
            
            hash = HashMix(hash, ch);
            continue;
        }

        
        
        
        
        if (length > gfxShapedWord::kMaxLength) {
            bool ok = ShapeFragmentWithoutWordCache(aContext,
                                                    text + wordStart,
                                                    aRunStart + wordStart,
                                                    length,
                                                    aRunScript,
                                                    aTextRun);
            if (!ok) {
                return false;
            }
        } else if (length > 0) {
            uint32_t wordFlags = flags;
            
            
            
            if (sizeof(T) == sizeof(PRUnichar)) {
                if (wordIs8Bit) {
                    wordFlags |= gfxTextRunFactory::TEXT_IS_8BIT;
                }
            }
            gfxShapedWord *sw = GetShapedWord(aContext,
                                              text + wordStart, length,
                                              hash, aRunScript,
                                              appUnitsPerDevUnit,
                                              wordFlags);
            if (sw) {
                aTextRun->CopyGlyphDataFrom(sw, aRunStart + wordStart);
            } else {
                return false; 
            }
        }

        if (boundary) {
            
            if (!aTextRun->SetSpaceGlyphIfSimple(this, aContext,
                                                 aRunStart + i, ch))
            {
                static const uint8_t space = ' ';
                gfxShapedWord *sw =
                    GetShapedWord(aContext,
                                  &space, 1,
                                  HashMix(0, ' '), aRunScript,
                                  appUnitsPerDevUnit,
                                  flags | gfxTextRunFactory::TEXT_IS_8BIT);
                if (sw) {
                    aTextRun->CopyGlyphDataFrom(sw, aRunStart + i);
                } else {
                    return false;
                }
            }
            hash = 0;
            wordStart = i + 1;
            wordIs8Bit = true;
            continue;
        }

        if (i == aRunLength) {
            break;
        }

        NS_ASSERTION(invalid,
                     "how did we get here except via an invalid char?");

        
        
        if (ch == '\t') {
            aTextRun->SetIsTab(aRunStart + i);
        } else if (ch == '\n') {
            aTextRun->SetIsNewline(aRunStart + i);
        }

        hash = 0;
        wordStart = i + 1;
        wordIs8Bit = true;
    }

    return true;
}

gfxGlyphExtents *
gfxFont::GetOrCreateGlyphExtents(int32_t aAppUnitsPerDevUnit) {
    uint32_t i, count = mGlyphExtentsArray.Length();
    for (i = 0; i < count; ++i) {
        if (mGlyphExtentsArray[i]->GetAppUnitsPerDevUnit() == aAppUnitsPerDevUnit)
            return mGlyphExtentsArray[i];
    }
    gfxGlyphExtents *glyphExtents = new gfxGlyphExtents(aAppUnitsPerDevUnit);
    if (glyphExtents) {
        mGlyphExtentsArray.AppendElement(glyphExtents);
        
        
        glyphExtents->SetContainedGlyphWidthAppUnits(GetSpaceGlyph(), 0);
    }
    return glyphExtents;
}

void
gfxFont::SetupGlyphExtents(gfxContext *aContext, uint32_t aGlyphID, bool aNeedTight,
                           gfxGlyphExtents *aExtents)
{
    gfxContextMatrixAutoSaveRestore matrixRestore(aContext);
    aContext->IdentityMatrix();
    cairo_glyph_t glyph;
    glyph.index = aGlyphID;
    glyph.x = 0;
    glyph.y = 0;
    cairo_text_extents_t extents;
    cairo_glyph_extents(aContext->GetCairo(), &glyph, 1, &extents);

    const Metrics& fontMetrics = GetMetrics();
    int32_t appUnitsPerDevUnit = aExtents->GetAppUnitsPerDevUnit();
    if (!aNeedTight && extents.x_bearing >= 0 &&
        extents.y_bearing >= -fontMetrics.maxAscent &&
        extents.height + extents.y_bearing <= fontMetrics.maxDescent) {
        uint32_t appUnitsWidth =
            uint32_t(ceil((extents.x_bearing + extents.width)*appUnitsPerDevUnit));
        if (appUnitsWidth < gfxGlyphExtents::INVALID_WIDTH) {
            aExtents->SetContainedGlyphWidthAppUnits(aGlyphID, uint16_t(appUnitsWidth));
            return;
        }
    }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    if (!aNeedTight) {
        ++gGlyphExtentsSetupFallBackToTight;
    }
#endif

    gfxFloat d2a = appUnitsPerDevUnit;
    gfxRect bounds(extents.x_bearing*d2a, extents.y_bearing*d2a,
                   extents.width*d2a, extents.height*d2a);

    gfxRect svgBounds;
    if (mFontEntry->TryGetSVGData() &&
        mFontEntry->HasSVGGlyph(aGlyphID) &&
        mFontEntry->GetSVGGlyphExtents(aContext, aGlyphID, &svgBounds)) {

        bounds = bounds.Union(gfxRect(svgBounds.x * d2a, svgBounds.y * d2a,
                                      svgBounds.width * d2a, svgBounds.height * d2a));
    }

    aExtents->SetTightGlyphExtents(aGlyphID, bounds);
}






bool
gfxFont::InitMetricsFromSfntTables(Metrics& aMetrics)
{
    mIsValid = false; 

    const uint32_t kHeadTableTag = TRUETYPE_TAG('h','e','a','d');
    const uint32_t kHheaTableTag = TRUETYPE_TAG('h','h','e','a');
    const uint32_t kPostTableTag = TRUETYPE_TAG('p','o','s','t');
    const uint32_t kOS_2TableTag = TRUETYPE_TAG('O','S','/','2');

    if (mFUnitsConvFactor == 0.0) {
        
        
        
        AutoFallibleTArray<uint8_t,sizeof(HeadTable)> headData;
        if (NS_FAILED(mFontEntry->GetFontTable(kHeadTableTag, headData)) ||
            headData.Length() < sizeof(HeadTable)) {
            return false; 
        }
        HeadTable *head = reinterpret_cast<HeadTable*>(headData.Elements());
        uint32_t unitsPerEm = head->unitsPerEm;
        if (!unitsPerEm) {
            return true; 
        }
        mFUnitsConvFactor = mAdjustedSize / unitsPerEm;
    }

    
    AutoFallibleTArray<uint8_t,sizeof(HheaTable)> hheaData;
    if (NS_FAILED(mFontEntry->GetFontTable(kHheaTableTag, hheaData)) ||
        hheaData.Length() < sizeof(HheaTable)) {
        return false; 
    }
    HheaTable *hhea = reinterpret_cast<HheaTable*>(hheaData.Elements());

#define SET_UNSIGNED(field,src) aMetrics.field = uint16_t(src) * mFUnitsConvFactor
#define SET_SIGNED(field,src)   aMetrics.field = int16_t(src) * mFUnitsConvFactor

    SET_UNSIGNED(maxAdvance, hhea->advanceWidthMax);
    SET_SIGNED(maxAscent, hhea->ascender);
    SET_SIGNED(maxDescent, -int16_t(hhea->descender));
    SET_SIGNED(externalLeading, hhea->lineGap);

    
    AutoFallibleTArray<uint8_t,sizeof(PostTable)> postData;
    if (NS_FAILED(mFontEntry->GetFontTable(kPostTableTag, postData))) {
        return true; 
    }
    if (postData.Length() <
        offsetof(PostTable, underlineThickness) + sizeof(uint16_t)) {
        return true; 
    }
    PostTable *post = reinterpret_cast<PostTable*>(postData.Elements());

    SET_SIGNED(underlineOffset, post->underlinePosition);
    SET_UNSIGNED(underlineSize, post->underlineThickness);

    
    
    AutoFallibleTArray<uint8_t,sizeof(OS2Table)> os2data;
    if (NS_SUCCEEDED(mFontEntry->GetFontTable(kOS_2TableTag, os2data))) {
        OS2Table *os2 = reinterpret_cast<OS2Table*>(os2data.Elements());

        if (os2data.Length() >= offsetof(OS2Table, sxHeight) +
                                sizeof(int16_t) &&
            uint16_t(os2->version) >= 2) {
            
            SET_SIGNED(xHeight, os2->sxHeight);
            
            aMetrics.xHeight = Abs(aMetrics.xHeight);
        }
        
        if (os2data.Length() >= offsetof(OS2Table, yStrikeoutPosition) +
                                sizeof(int16_t)) {
            SET_SIGNED(aveCharWidth, os2->xAvgCharWidth);
            SET_SIGNED(subscriptOffset, os2->ySubscriptYOffset);
            SET_SIGNED(superscriptOffset, os2->ySuperscriptYOffset);
            SET_SIGNED(strikeoutSize, os2->yStrikeoutSize);
            SET_SIGNED(strikeoutOffset, os2->yStrikeoutPosition);
        }
    }

    mIsValid = true;

    return true;
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
    return floor(aValue/aFraction + 0.5) * aFraction;
}

void gfxFont::CalculateDerivedMetrics(Metrics& aMetrics)
{
    aMetrics.maxAscent =
        ceil(RoundToNearestMultiple(aMetrics.maxAscent, 1/1024.0));
    aMetrics.maxDescent =
        ceil(RoundToNearestMultiple(aMetrics.maxDescent, 1/1024.0));

    if (aMetrics.xHeight <= 0) {
        
        
        
        aMetrics.xHeight = aMetrics.maxAscent * DEFAULT_XHEIGHT_FACTOR;
    }

    aMetrics.maxHeight = aMetrics.maxAscent + aMetrics.maxDescent;

    if (aMetrics.maxHeight - aMetrics.emHeight > 0.0) {
        aMetrics.internalLeading = aMetrics.maxHeight - aMetrics.emHeight;
    } else {
        aMetrics.internalLeading = 0.0;
    }

    aMetrics.emAscent = aMetrics.maxAscent * aMetrics.emHeight
                            / aMetrics.maxHeight;
    aMetrics.emDescent = aMetrics.emHeight - aMetrics.emAscent;

    if (GetFontEntry()->IsFixedPitch()) {
        
        
        
        aMetrics.maxAdvance = aMetrics.aveCharWidth;
    }

    if (!aMetrics.subscriptOffset) {
        aMetrics.subscriptOffset = aMetrics.xHeight;
    }
    if (!aMetrics.superscriptOffset) {
        aMetrics.superscriptOffset = aMetrics.xHeight;
    }

    if (!aMetrics.strikeoutOffset) {
        aMetrics.strikeoutOffset = aMetrics.xHeight * 0.5;
    }
    if (!aMetrics.strikeoutSize) {
        aMetrics.strikeoutSize = aMetrics.underlineSize;
    }
}

void
gfxFont::SanitizeMetrics(gfxFont::Metrics *aMetrics, bool aIsBadUnderlineFont)
{
    
    
    if (mStyle.size == 0.0) {
        memset(aMetrics, 0, sizeof(gfxFont::Metrics));
        return;
    }

    
    
    
    if (aMetrics->superscriptOffset <= 0 ||
        aMetrics->superscriptOffset >= aMetrics->maxAscent) {
        aMetrics->superscriptOffset = aMetrics->xHeight;
    }
    
    if (aMetrics->subscriptOffset <= 0 ||
        aMetrics->subscriptOffset >= aMetrics->maxAscent) {
        aMetrics->subscriptOffset = aMetrics->xHeight;
    }

    aMetrics->underlineSize = std::max(1.0, aMetrics->underlineSize);
    aMetrics->strikeoutSize = std::max(1.0, aMetrics->strikeoutSize);

    aMetrics->underlineOffset = std::min(aMetrics->underlineOffset, -1.0);

    if (aMetrics->maxAscent < 1.0) {
        
        aMetrics->underlineSize = 0;
        aMetrics->underlineOffset = 0;
        aMetrics->strikeoutSize = 0;
        aMetrics->strikeoutOffset = 0;
        return;
    }

    







    if (!mStyle.systemFont && aIsBadUnderlineFont) {
        
        
        aMetrics->underlineOffset = std::min(aMetrics->underlineOffset, -2.0);

        
        if (aMetrics->internalLeading + aMetrics->externalLeading > aMetrics->underlineSize) {
            aMetrics->underlineOffset = std::min(aMetrics->underlineOffset, -aMetrics->emDescent);
        } else {
            aMetrics->underlineOffset = std::min(aMetrics->underlineOffset,
                                               aMetrics->underlineSize - aMetrics->emDescent);
        }
    }
    
    
    else if (aMetrics->underlineSize - aMetrics->underlineOffset > aMetrics->maxDescent) {
        if (aMetrics->underlineSize > aMetrics->maxDescent)
            aMetrics->underlineSize = std::max(aMetrics->maxDescent, 1.0);
        
        aMetrics->underlineOffset = aMetrics->underlineSize - aMetrics->maxDescent;
    }

    
    
    
    gfxFloat halfOfStrikeoutSize = floor(aMetrics->strikeoutSize / 2.0 + 0.5);
    if (halfOfStrikeoutSize + aMetrics->strikeoutOffset > aMetrics->maxAscent) {
        if (aMetrics->strikeoutSize > aMetrics->maxAscent) {
            aMetrics->strikeoutSize = std::max(aMetrics->maxAscent, 1.0);
            halfOfStrikeoutSize = floor(aMetrics->strikeoutSize / 2.0 + 0.5);
        }
        gfxFloat ascent = floor(aMetrics->maxAscent + 0.5);
        aMetrics->strikeoutOffset = std::max(halfOfStrikeoutSize, ascent / 2.0);
    }

    
    if (aMetrics->underlineSize > aMetrics->maxAscent) {
        aMetrics->underlineSize = aMetrics->maxAscent;
    }
}

gfxFloat
gfxFont::SynthesizeSpaceWidth(uint32_t aCh)
{
    
    
    
    switch (aCh) {
    case 0x2000:                                 
    case 0x2002: return GetAdjustedSize() / 2;   
    case 0x2001:                                 
    case 0x2003: return GetAdjustedSize();       
    case 0x2004: return GetAdjustedSize() / 3;   
    case 0x2005: return GetAdjustedSize() / 4;   
    case 0x2006: return GetAdjustedSize() / 6;   
    case 0x2007: return GetMetrics().zeroOrAveCharWidth; 
    case 0x2008: return GetMetrics().spaceWidth; 
    case 0x2009: return GetAdjustedSize() / 5;   
    case 0x200a: return GetAdjustedSize() / 10;  
    case 0x202f: return GetAdjustedSize() / 5;   
    default: return -1.0;
    }
}

 size_t
gfxFont::WordCacheEntrySizeOfExcludingThis(CacheHashEntry*   aHashEntry,
                                           nsMallocSizeOfFun aMallocSizeOf,
                                           void*             aUserArg)
{
    return aMallocSizeOf(aHashEntry->mShapedWord.get());
}

void
gfxFont::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                             FontCacheSizes*   aSizes) const
{
    for (uint32_t i = 0; i < mGlyphExtentsArray.Length(); ++i) {
        aSizes->mFontInstances +=
            mGlyphExtentsArray[i]->SizeOfIncludingThis(aMallocSizeOf);
    }
    aSizes->mShapedWords +=
        mWordCache.SizeOfExcludingThis(WordCacheEntrySizeOfExcludingThis,
                                       aMallocSizeOf);
}

void
gfxFont::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                             FontCacheSizes*   aSizes) const
{
    aSizes->mFontInstances += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}

gfxGlyphExtents::~gfxGlyphExtents()
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    gGlyphExtentsWidthsTotalSize +=
        mContainedGlyphWidths.SizeOfExcludingThis(&FontCacheMallocSizeOf);
    gGlyphExtentsCount++;
#endif
    MOZ_COUNT_DTOR(gfxGlyphExtents);
}

bool
gfxGlyphExtents::GetTightGlyphExtentsAppUnits(gfxFont *aFont,
    gfxContext *aContext, uint32_t aGlyphID, gfxRect *aExtents)
{
    HashEntry *entry = mTightGlyphExtents.GetEntry(aGlyphID);
    if (!entry) {
        if (!aContext) {
            NS_WARNING("Could not get glyph extents (no aContext)");
            return false;
        }

        if (aFont->SetupCairoFont(aContext)) {
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
            ++gGlyphExtentsSetupLazyTight;
#endif
            aFont->SetupGlyphExtents(aContext, aGlyphID, true, this);
            entry = mTightGlyphExtents.GetEntry(aGlyphID);
        }
        if (!entry) {
            NS_WARNING("Could not get glyph extents");
            return false;
        }
    }

    *aExtents = gfxRect(entry->x, entry->y, entry->width, entry->height);
    return true;
}

gfxGlyphExtents::GlyphWidths::~GlyphWidths()
{
    uint32_t i, count = mBlocks.Length();
    for (i = 0; i < count; ++i) {
        uintptr_t bits = mBlocks[i];
        if (bits && !(bits & 0x1)) {
            delete[] reinterpret_cast<uint16_t *>(bits);
        }
    }
}

uint32_t
gfxGlyphExtents::GlyphWidths::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
    uint32_t i;
    uint32_t size = mBlocks.SizeOfExcludingThis(aMallocSizeOf);
    for (i = 0; i < mBlocks.Length(); ++i) {
        uintptr_t bits = mBlocks[i];
        if (bits && !(bits & 0x1)) {
            size += aMallocSizeOf(reinterpret_cast<void*>(bits));
        }
    }
    return size;
}

void
gfxGlyphExtents::GlyphWidths::Set(uint32_t aGlyphID, uint16_t aWidth)
{
    uint32_t block = aGlyphID >> BLOCK_SIZE_BITS;
    uint32_t len = mBlocks.Length();
    if (block >= len) {
        uintptr_t *elems = mBlocks.AppendElements(block + 1 - len);
        if (!elems)
            return;
        memset(elems, 0, sizeof(uintptr_t)*(block + 1 - len));
    }

    uintptr_t bits = mBlocks[block];
    uint32_t glyphOffset = aGlyphID & (BLOCK_SIZE - 1);
    if (!bits) {
        mBlocks[block] = MakeSingle(glyphOffset, aWidth);
        return;
    }

    uint16_t *newBlock;
    if (bits & 0x1) {
        
        
        newBlock = new uint16_t[BLOCK_SIZE];
        if (!newBlock)
            return;
        uint32_t i;
        for (i = 0; i < BLOCK_SIZE; ++i) {
            newBlock[i] = INVALID_WIDTH;
        }
        newBlock[GetGlyphOffset(bits)] = GetWidth(bits);
        mBlocks[block] = reinterpret_cast<uintptr_t>(newBlock);
    } else {
        newBlock = reinterpret_cast<uint16_t *>(bits);
    }
    newBlock[glyphOffset] = aWidth;
}

void
gfxGlyphExtents::SetTightGlyphExtents(uint32_t aGlyphID, const gfxRect& aExtentsAppUnits)
{
    HashEntry *entry = mTightGlyphExtents.PutEntry(aGlyphID);
    if (!entry)
        return;
    entry->x = aExtentsAppUnits.X();
    entry->y = aExtentsAppUnits.Y();
    entry->width = aExtentsAppUnits.Width();
    entry->height = aExtentsAppUnits.Height();
}

size_t
gfxGlyphExtents::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
    return mContainedGlyphWidths.SizeOfExcludingThis(aMallocSizeOf) +
        mTightGlyphExtents.SizeOfExcludingThis(nullptr, aMallocSizeOf);
}

size_t
gfxGlyphExtents::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf) const
{
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}

gfxFontGroup::gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle, gfxUserFontSet *aUserFontSet)
    : mFamilies(aFamilies), mStyle(*aStyle), mUnderlineOffset(UNDERLINE_OFFSET_NOT_SET)
{
    mUserFontSet = nullptr;
    SetUserFontSet(aUserFontSet);

    mSkipDrawing = false;

    mPageLang = gfxPlatform::GetFontPrefLangFor(mStyle.language);
    BuildFontList();
}

void
gfxFontGroup::BuildFontList()
{


#if defined(XP_MACOSX) || defined(XP_WIN) || defined(ANDROID)
    ForEachFont(FindPlatformFont, this);

    if (mFonts.Length() == 0) {
        bool needsBold;
        gfxPlatformFontList *pfl = gfxPlatformFontList::PlatformFontList();
        gfxFontFamily *defaultFamily = pfl->GetDefaultFont(&mStyle);
        NS_ASSERTION(defaultFamily,
                     "invalid default font returned by GetDefaultFont");

        if (defaultFamily) {
            gfxFontEntry *fe = defaultFamily->FindFontForStyle(mStyle,
                                                               needsBold);
            if (fe) {
                nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&mStyle,
                                                            needsBold);
                if (font) {
                    mFonts.AppendElement(FamilyFace(defaultFamily, font));
                }
            }
        }

        if (mFonts.Length() == 0) {
            
            
            
            
            
            nsAutoTArray<nsRefPtr<gfxFontFamily>,200> families;
            pfl->GetFontFamilyList(families);
            uint32_t count = families.Length();
            for (uint32_t i = 0; i < count; ++i) {
                gfxFontEntry *fe = families[i]->FindFontForStyle(mStyle,
                                                                 needsBold);
                if (fe) {
                    nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&mStyle,
                                                                needsBold);
                    if (font) {
                        mFonts.AppendElement(FamilyFace(families[i], font));
                        break;
                    }
                }
            }
        }

        if (mFonts.Length() == 0) {
            
            
            char msg[256]; 
            sprintf(msg, "unable to find a usable font (%.220s)",
                    NS_ConvertUTF16toUTF8(mFamilies).get());
            NS_RUNTIMEABORT(msg);
        }
    }

    if (!mStyle.systemFont) {
        uint32_t count = mFonts.Length();
        for (uint32_t i = 0; i < count; ++i) {
            gfxFont* font = mFonts[i].Font();
            if (font->GetFontEntry()->mIsBadUnderlineFont) {
                gfxFloat first = mFonts[0].Font()->GetMetrics().underlineOffset;
                gfxFloat bad = font->GetMetrics().underlineOffset;
                mUnderlineOffset = std::min(first, bad);
                break;
            }
        }
    }
#endif
}

bool
gfxFontGroup::FindPlatformFont(const nsAString& aName,
                               const nsACString& aGenericName,
                               bool aUseFontSet,
                               void *aClosure)
{
    gfxFontGroup *fontGroup = static_cast<gfxFontGroup*>(aClosure);
    const gfxFontStyle *fontStyle = fontGroup->GetStyle();

    bool needsBold;
    gfxFontFamily *family = nullptr;
    gfxFontEntry *fe = nullptr;

    if (aUseFontSet) {
        
        
        
        
        gfxUserFontSet *fs = fontGroup->GetUserFontSet();
        if (fs) {
            
            
            
            family = fs->GetFamily(aName);
            if (family) {
                bool waitForUserFont = false;
                fe = fs->FindFontEntry(family, *fontStyle,
                                       needsBold, waitForUserFont);
                if (!fe && waitForUserFont) {
                    fontGroup->mSkipDrawing = true;
                }
            }
        }
    }

    
    if (!family) {
        gfxPlatformFontList *fontList = gfxPlatformFontList::PlatformFontList();
        family = fontList->FindFamily(aName);
        if (family) {
            fe = family->FindFontForStyle(*fontStyle, needsBold);
        }
    }

    
    if (fe && !fontGroup->HasFont(fe)) {
        nsRefPtr<gfxFont> font = fe->FindOrMakeFont(fontStyle, needsBold);
        if (font) {
            fontGroup->mFonts.AppendElement(FamilyFace(family, font));
        }
    }

    return true;
}

bool
gfxFontGroup::HasFont(const gfxFontEntry *aFontEntry)
{
    uint32_t count = mFonts.Length();
    for (uint32_t i = 0; i < count; ++i) {
        if (mFonts[i].Font()->GetFontEntry() == aFontEntry)
            return true;
    }
    return false;
}

gfxFontGroup::~gfxFontGroup() {
    mFonts.Clear();
    SetUserFontSet(nullptr);
}

gfxFontGroup *
gfxFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxFontGroup(mFamilies, aStyle, mUserFontSet);
}

bool 
gfxFontGroup::IsInvalidChar(uint8_t ch)
{
    return ((ch & 0x7f) < 0x20);
}

bool 
gfxFontGroup::IsInvalidChar(PRUnichar ch)
{
    
    if (ch >= ' ' && ch < 0x80) {
        return false;
    }
    
    if (ch <= 0x9f) {
        return true;
    }
    return ((ch & 0xFF00) == 0x2000  &&
         (ch == 0x200B || ch == 0x2028 || ch == 0x2029 ||
          IS_BIDI_CONTROL_CHAR(ch)));
}

bool
gfxFontGroup::ForEachFont(FontCreationCallback fc,
                          void *closure)
{
    return ForEachFontInternal(mFamilies, mStyle.language,
                               true, true, true, fc, closure);
}

bool
gfxFontGroup::ForEachFont(const nsAString& aFamilies,
                          nsIAtom *aLanguage,
                          FontCreationCallback fc,
                          void *closure)
{
    return ForEachFontInternal(aFamilies, aLanguage,
                               false, true, true, fc, closure);
}

struct ResolveData {
    ResolveData(gfxFontGroup::FontCreationCallback aCallback,
                nsACString& aGenericFamily,
                bool aUseFontSet,
                void *aClosure) :
        mCallback(aCallback),
        mGenericFamily(aGenericFamily),
        mUseFontSet(aUseFontSet),
        mClosure(aClosure) {
    }
    gfxFontGroup::FontCreationCallback mCallback;
    nsCString mGenericFamily;
    bool mUseFontSet;
    void *mClosure;
};

bool
gfxFontGroup::ForEachFontInternal(const nsAString& aFamilies,
                                  nsIAtom *aLanguage,
                                  bool aResolveGeneric,
                                  bool aResolveFontName,
                                  bool aUseFontSet,
                                  FontCreationCallback fc,
                                  void *closure)
{
    const PRUnichar kSingleQuote  = PRUnichar('\'');
    const PRUnichar kDoubleQuote  = PRUnichar('\"');
    const PRUnichar kComma        = PRUnichar(',');

    nsIAtom *groupAtom = nullptr;
    nsAutoCString groupString;
    if (aLanguage) {
        if (!gLangService) {
            CallGetService(NS_LANGUAGEATOMSERVICE_CONTRACTID, &gLangService);
        }
        if (gLangService) {
            nsresult rv;
            groupAtom = gLangService->GetLanguageGroup(aLanguage, &rv);
        }
    }
    if (!groupAtom) {
        groupAtom = nsGkAtoms::Unicode;
    }
    groupAtom->ToUTF8String(groupString);

    nsPromiseFlatString families(aFamilies);
    const PRUnichar *p, *p_end;
    families.BeginReading(p);
    families.EndReading(p_end);
    nsAutoString family;
    nsAutoCString lcFamily;
    nsAutoString genericFamily;

    while (p < p_end) {
        while (nsCRT::IsAsciiSpace(*p) || *p == kComma)
            if (++p == p_end)
                return true;

        bool generic;
        if (*p == kSingleQuote || *p == kDoubleQuote) {
            
            PRUnichar quoteMark = *p;
            if (++p == p_end)
                return true;
            const PRUnichar *nameStart = p;

            
            while (*p != quoteMark)
                if (++p == p_end)
                    return true;

            family = Substring(nameStart, p);
            generic = false;
            genericFamily.SetIsVoid(true);

            while (++p != p_end && *p != kComma)
                 ;

        } else {
            
            const PRUnichar *nameStart = p;
            while (++p != p_end && *p != kComma)
                 ;

            family = Substring(nameStart, p);
            family.CompressWhitespace(false, true);

            if (aResolveGeneric &&
                (family.LowerCaseEqualsLiteral("serif") ||
                 family.LowerCaseEqualsLiteral("sans-serif") ||
                 family.LowerCaseEqualsLiteral("monospace") ||
                 family.LowerCaseEqualsLiteral("cursive") ||
                 family.LowerCaseEqualsLiteral("fantasy")))
            {
                generic = true;

                ToLowerCase(NS_LossyConvertUTF16toASCII(family), lcFamily);

                nsAutoCString prefName("font.name.");
                prefName.Append(lcFamily);
                prefName.AppendLiteral(".");
                prefName.Append(groupString);

                nsAdoptingString value = Preferences::GetString(prefName.get());
                if (value) {
                    CopyASCIItoUTF16(lcFamily, genericFamily);
                    family = value;
                }
            } else {
                generic = false;
                genericFamily.SetIsVoid(true);
            }
        }

        NS_LossyConvertUTF16toASCII gf(genericFamily);
        if (generic) {
            ForEachFontInternal(family, groupAtom, false,
                                aResolveFontName, false,
                                fc, closure);
        } else if (!family.IsEmpty()) {
            if (aResolveFontName) {
                ResolveData data(fc, gf, aUseFontSet, closure);
                bool aborted = false, needsBold;
                nsresult rv = NS_OK;
                bool foundFamily = false;
                bool waitForUserFont = false;
                gfxFontEntry *fe = nullptr;
                if (aUseFontSet && mUserFontSet) {
                    gfxFontFamily *fam = mUserFontSet->GetFamily(family);
                    if (fam) {
                        fe = mUserFontSet->FindFontEntry(fam, mStyle,
                                                         needsBold,
                                                         waitForUserFont);
                    }
                }
                if (fe) {
                    gfxFontGroup::FontResolverProc(family, &data);
                } else {
                    if (waitForUserFont) {
                        mSkipDrawing = true;
                    }
                    if (!foundFamily) {
                        gfxPlatform *pf = gfxPlatform::GetPlatform();
                        rv = pf->ResolveFontName(family,
                                                 gfxFontGroup::FontResolverProc,
                                                 &data, aborted);
                    }
                }
                if (NS_FAILED(rv) || aborted)
                    return false;
            }
            else {
                if (!fc(family, gf, aUseFontSet, closure))
                    return false;
            }
        }

        if (generic && aResolveGeneric) {
            nsAutoCString prefName("font.name-list.");
            prefName.Append(lcFamily);
            prefName.AppendLiteral(".");
            prefName.Append(groupString);
            nsAdoptingString value = Preferences::GetString(prefName.get());
            if (value) {
                ForEachFontInternal(value, groupAtom, false,
                                    aResolveFontName, false,
                                    fc, closure);
            }
        }

        ++p; 
    }

    return true;
}

bool
gfxFontGroup::FontResolverProc(const nsAString& aName, void *aClosure)
{
    ResolveData *data = reinterpret_cast<ResolveData*>(aClosure);
    return (data->mCallback)(aName, data->mGenericFamily, data->mUseFontSet,
                             data->mClosure);
}

gfxTextRun *
gfxFontGroup::MakeEmptyTextRun(const Parameters *aParams, uint32_t aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;
    return gfxTextRun::Create(aParams, 0, this, aFlags);
}

gfxTextRun *
gfxFontGroup::MakeSpaceTextRun(const Parameters *aParams, uint32_t aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;

    gfxTextRun *textRun = gfxTextRun::Create(aParams, 1, this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    gfxFont *font = GetFontAt(0);
    if (MOZ_UNLIKELY(GetStyle()->size == 0)) {
        
        
        
        textRun->AddGlyphRun(font, gfxTextRange::kFontGroup, 0, false);
    }
    else {
        textRun->SetSpaceGlyph(font, aParams->mContext, 0);
    }

    
    
    
    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeBlankTextRun(uint32_t aLength,
                               const Parameters *aParams, uint32_t aFlags)
{
    gfxTextRun *textRun =
        gfxTextRun::Create(aParams, aLength, this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    textRun->AddGlyphRun(GetFontAt(0), gfxTextRange::kFontGroup, 0, false);
    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeTextRun(const uint8_t *aString, uint32_t aLength,
                          const Parameters *aParams, uint32_t aFlags)
{
    if (aLength == 0) {
        return MakeEmptyTextRun(aParams, aFlags);
    }
    if (aLength == 1 && aString[0] == ' ') {
        return MakeSpaceTextRun(aParams, aFlags);
    }

    aFlags |= TEXT_IS_8BIT;

    if (GetStyle()->size == 0) {
        
        
        
        return MakeBlankTextRun(aLength, aParams, aFlags);
    }

    gfxTextRun *textRun = gfxTextRun::Create(aParams, aLength,
                                             this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    InitTextRun(aParams->mContext, textRun, aString, aLength);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeTextRun(const PRUnichar *aString, uint32_t aLength,
                          const Parameters *aParams, uint32_t aFlags)
{
    if (aLength == 0) {
        return MakeEmptyTextRun(aParams, aFlags);
    }
    if (aLength == 1 && aString[0] == ' ') {
        return MakeSpaceTextRun(aParams, aFlags);
    }
    if (GetStyle()->size == 0) {
        return MakeBlankTextRun(aLength, aParams, aFlags);
    }

    gfxTextRun *textRun = gfxTextRun::Create(aParams, aLength,
                                             this, aFlags);
    if (!textRun) {
        return nullptr;
    }

    InitTextRun(aParams->mContext, textRun, aString, aLength);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

template<typename T>
void
gfxFontGroup::InitTextRun(gfxContext *aContext,
                          gfxTextRun *aTextRun,
                          const T *aString,
                          uint32_t aLength)
{
    NS_ASSERTION(aLength > 0, "don't call InitTextRun for a zero-length run");

    
    
    int32_t numOption = gfxPlatform::GetPlatform()->GetBidiNumeralOption();
    nsAutoArrayPtr<PRUnichar> transformedString;
    if (numOption != IBMBIDI_NUMERAL_NOMINAL) {
        
        
        
        bool prevIsArabic =
            (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_INCOMING_ARABICCHAR) != 0;
        for (uint32_t i = 0; i < aLength; ++i) {
            PRUnichar origCh = aString[i];
            PRUnichar newCh = HandleNumberInChar(origCh, prevIsArabic, numOption);
            if (newCh != origCh) {
                if (!transformedString) {
                    transformedString = new PRUnichar[aLength];
                    if (sizeof(T) == sizeof(PRUnichar)) {
                        memcpy(transformedString.get(), aString, i * sizeof(PRUnichar));
                    } else {
                        for (uint32_t j = 0; j < i; ++j) {
                            transformedString[j] = aString[j];
                        }
                    }
                }
            }
            if (transformedString) {
                transformedString[i] = newCh;
            }
            prevIsArabic = IS_ARABIC_CHAR(newCh);
        }
    }

    if (sizeof(T) == sizeof(uint8_t) && !transformedString) {
        
        
        InitScriptRun(aContext, aTextRun, aString,
                      0, aLength, MOZ_SCRIPT_LATIN);
    } else {
        const PRUnichar *textPtr;
        if (transformedString) {
            textPtr = transformedString.get();
        } else {
            
            
            textPtr = reinterpret_cast<const PRUnichar*>(aString);
        }

        
        
        gfxScriptItemizer scriptRuns(textPtr, aLength);

#ifdef PR_LOGGING
        PRLogModuleInfo *log = (mStyle.systemFont ?
                                gfxPlatform::GetLog(eGfxLog_textrunui) :
                                gfxPlatform::GetLog(eGfxLog_textrun));
#endif

        uint32_t runStart = 0, runLimit = aLength;
        int32_t runScript = MOZ_SCRIPT_LATIN;
        while (scriptRuns.Next(runStart, runLimit, runScript)) {

#ifdef PR_LOGGING
            if (MOZ_UNLIKELY(log)) {
                nsAutoCString lang;
                mStyle.language->ToUTF8String(lang);
                uint32_t runLen = runLimit - runStart;
                PR_LOG(log, PR_LOG_WARNING,\
                       ("(%s) fontgroup: [%s] lang: %s script: %d len %d "
                        "weight: %d width: %d style: %s "
                        "TEXTRUN [%s] ENDTEXTRUN\n",
                        (mStyle.systemFont ? "textrunui" : "textrun"),
                        NS_ConvertUTF16toUTF8(mFamilies).get(),
                        lang.get(), runScript, runLen,
                        uint32_t(mStyle.weight), uint32_t(mStyle.stretch),
                        (mStyle.style & NS_FONT_STYLE_ITALIC ? "italic" :
                        (mStyle.style & NS_FONT_STYLE_OBLIQUE ? "oblique" :
                                                                "normal")),
                        NS_ConvertUTF16toUTF8(textPtr + runStart, runLen).get()));
            }
#endif

            InitScriptRun(aContext, aTextRun, textPtr,
                          runStart, runLimit, runScript);
        }
    }

    if (sizeof(T) == sizeof(PRUnichar) && aLength > 0) {
        gfxTextRun::CompressedGlyph *glyph = aTextRun->GetCharacterGlyphs();
        if (!glyph->IsSimpleGlyph()) {
            glyph->SetClusterStart(true);
        }
    }

    
    
    
    
    
    
    
    
    
    aTextRun->SanitizeGlyphRuns();

    aTextRun->SortGlyphRuns();
}

template<typename T>
void
gfxFontGroup::InitScriptRun(gfxContext *aContext,
                            gfxTextRun *aTextRun,
                            const T *aString,
                            uint32_t aScriptRunStart,
                            uint32_t aScriptRunEnd,
                            int32_t aRunScript)
{
    NS_ASSERTION(aScriptRunEnd > aScriptRunStart,
                 "don't call InitScriptRun for a zero-length run");

    gfxFont *mainFont = GetFontAt(0);

    uint32_t runStart = aScriptRunStart;
    nsAutoTArray<gfxTextRange,3> fontRanges;
    ComputeRanges(fontRanges, aString + aScriptRunStart,
                  aScriptRunEnd - aScriptRunStart, aRunScript);
    uint32_t numRanges = fontRanges.Length();

    for (uint32_t r = 0; r < numRanges; r++) {
        const gfxTextRange& range = fontRanges[r];
        uint32_t matchedLength = range.Length();
        gfxFont *matchedFont = range.font;

        
        if (matchedFont) {
            aTextRun->AddGlyphRun(matchedFont, range.matchType,
                                  runStart, (matchedLength > 0));
            
            if (!matchedFont->SplitAndInitTextRun(aContext, aTextRun, aString,
                                                  runStart, matchedLength,
                                                  aRunScript)) {
                
                matchedFont = nullptr;
            }
        } else {
            aTextRun->AddGlyphRun(mainFont, gfxTextRange::kFontGroup,
                                  runStart, (matchedLength > 0));
        }

        if (!matchedFont) {
            
            
            
            aTextRun->SetupClusterBoundaries(runStart, aString + runStart,
                                             matchedLength);

            
            
            uint32_t runLimit = runStart + matchedLength;
            for (uint32_t index = runStart; index < runLimit; index++) {
                T ch = aString[index];

                
                
                if (ch == '\n') {
                    aTextRun->SetIsNewline(index);
                    continue;
                }
                if (ch == '\t') {
                    aTextRun->SetIsTab(index);
                    continue;
                }

                
                
                if (sizeof(T) == sizeof(PRUnichar)) {
                    if (NS_IS_HIGH_SURROGATE(ch) &&
                        index + 1 < aScriptRunEnd &&
                        NS_IS_LOW_SURROGATE(aString[index + 1]))
                    {
                        aTextRun->SetMissingGlyph(index,
                                                  SURROGATE_TO_UCS4(ch,
                                                                    aString[index + 1]),
                                                  mainFont);
                        index++;
                        continue;
                    }

                    
                    
                    gfxFloat wid = mainFont->SynthesizeSpaceWidth(ch);
                    if (wid >= 0.0) {
                        nscoord advance =
                            aTextRun->GetAppUnitsPerDevUnit() * floor(wid + 0.5);
                        if (gfxShapedText::CompressedGlyph::IsSimpleAdvance(advance)) {
                            aTextRun->GetCharacterGlyphs()[index].
                                SetSimpleGlyph(advance,
                                               mainFont->GetSpaceGlyph());
                        } else {
                            gfxTextRun::DetailedGlyph detailedGlyph;
                            detailedGlyph.mGlyphID = mainFont->GetSpaceGlyph();
                            detailedGlyph.mAdvance = advance;
                            detailedGlyph.mXOffset = detailedGlyph.mYOffset = 0;
                            gfxShapedText::CompressedGlyph g;
                            g.SetComplex(true, true, 1);
                            aTextRun->SetGlyphs(index,
                                                g, &detailedGlyph);
                        }
                        continue;
                    }
                }

                if (IsInvalidChar(ch)) {
                    
                    continue;
                }

                
                aTextRun->SetMissingGlyph(index, ch, mainFont);
            }
        }

        runStart += matchedLength;
    }
}

already_AddRefed<gfxFont>
gfxFontGroup::TryAllFamilyMembers(gfxFontFamily* aFamily, uint32_t aCh)
{
    if (!aFamily->TestCharacterMap(aCh)) {
        return nullptr;
    }

    
    
    
    GlobalFontMatch matchData(aCh, 0, &mStyle);
    aFamily->SearchAllFontsForChar(&matchData);
    gfxFontEntry *fe = matchData.mBestMatch;
    if (!fe) {
        return nullptr;
    }

    bool needsBold = mStyle.weight >= 600 && !fe->IsBold();
    nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&mStyle, needsBold);
    return font.forget();
}

already_AddRefed<gfxFont>
gfxFontGroup::FindFontForChar(uint32_t aCh, uint32_t aPrevCh,
                              int32_t aRunScript, gfxFont *aPrevMatchedFont,
                              uint8_t *aMatchType)
{
    
    
    uint32_t nextIndex = 0;
    bool isJoinControl = gfxFontUtils::IsJoinControl(aCh);
    bool wasJoinCauser = gfxFontUtils::IsJoinCauser(aPrevCh);
    bool isVarSelector = gfxFontUtils::IsVarSelector(aCh);

    if (!isJoinControl && !wasJoinCauser && !isVarSelector) {
        gfxFont *firstFont = mFonts[0].Font();
        if (firstFont->HasCharacter(aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            firstFont->AddRef();
            return firstFont;
        }
        
        
        nsRefPtr<gfxFont> font = TryAllFamilyMembers(mFonts[0].Family(), aCh);
        if (font) {
            *aMatchType = gfxTextRange::kFontGroup;
            return font.forget();
        }
        
        ++nextIndex;
    }

    if (aPrevMatchedFont) {
        
        
        
        uint8_t category = GetGeneralCategory(aCh);
        if (category == HB_UNICODE_GENERAL_CATEGORY_CONTROL) {
            aPrevMatchedFont->AddRef();
            return aPrevMatchedFont;
        }

        
        
        if (isJoinControl || wasJoinCauser) {
            if (aPrevMatchedFont->HasCharacter(aCh)) {
                aPrevMatchedFont->AddRef();
                return aPrevMatchedFont;
            }
        }
    }

    
    
    
    if (isVarSelector) {
        if (aPrevMatchedFont) {
            aPrevMatchedFont->AddRef();
            return aPrevMatchedFont;
        }
        
        return nullptr;
    }

    
    uint32_t fontListLength = FontListLength();
    for (uint32_t i = nextIndex; i < fontListLength; i++) {
        nsRefPtr<gfxFont> font = mFonts[i].Font();
        if (font->HasCharacter(aCh)) {
            *aMatchType = gfxTextRange::kFontGroup;
            return font.forget();
        }

        font = TryAllFamilyMembers(mFonts[i].Family(), aCh);
        if (font) {
            *aMatchType = gfxTextRange::kFontGroup;
            return font.forget();
        }
    }

    
    if ((aCh >= 0xE000  && aCh <= 0xF8FF) || (aCh >= 0xF0000 && aCh <= 0x10FFFD))
        return nullptr;

    
    nsRefPtr<gfxFont> font = WhichPrefFontSupportsChar(aCh);
    if (font) {
        *aMatchType = gfxTextRange::kPrefsFallback;
        return font.forget();
    }

    
    
    if (aPrevMatchedFont && aPrevMatchedFont->HasCharacter(aCh)) {
        *aMatchType = gfxTextRange::kSystemFallback;
        aPrevMatchedFont->AddRef();
        return aPrevMatchedFont;
    }

    
    if (aRunScript == HB_SCRIPT_UNKNOWN) {
        return nullptr;
    }

    
    
    if (GetGeneralCategory(aCh) ==
            HB_UNICODE_GENERAL_CATEGORY_SPACE_SEPARATOR &&
        GetFontAt(0)->SynthesizeSpaceWidth(aCh) >= 0.0)
    {
        return nullptr;
    }

    
    *aMatchType = gfxTextRange::kSystemFallback;
    font = WhichSystemFontSupportsChar(aCh, aRunScript);
    return font.forget();
}

template<typename T>
void gfxFontGroup::ComputeRanges(nsTArray<gfxTextRange>& aRanges,
                                 const T *aString, uint32_t aLength,
                                 int32_t aRunScript)
{
    NS_ASSERTION(aRanges.Length() == 0, "aRanges must be initially empty");
    NS_ASSERTION(aLength > 0, "don't call ComputeRanges for zero-length text");

    uint32_t prevCh = 0;
    uint8_t matchType = 0;
    int32_t lastRangeIndex = -1;

    
    
    
    gfxFont *prevFont = GetFontAt(0);

    for (uint32_t i = 0; i < aLength; i++) {

        const uint32_t origI = i; 

        
        uint32_t ch = aString[i];

        
        if (sizeof(T) == sizeof(PRUnichar)) {
            if ((i + 1 < aLength) && NS_IS_HIGH_SURROGATE(ch) &&
                                 NS_IS_LOW_SURROGATE(aString[i + 1])) {
                i++;
                ch = SURROGATE_TO_UCS4(ch, aString[i]);
            }
        }

        if (ch == 0xa0) {
            ch = ' ';
        }

        
        nsRefPtr<gfxFont> font =
            FindFontForChar(ch, prevCh, aRunScript, prevFont, &matchType);

        prevCh = ch;

        if (lastRangeIndex == -1) {
            
            aRanges.AppendElement(gfxTextRange(0, 1, font, matchType));
            lastRangeIndex++;
            prevFont = font;
        } else {
            
            gfxTextRange& prevRange = aRanges[lastRangeIndex];
            if (prevRange.font != font || prevRange.matchType != matchType) {
                
                prevRange.end = origI;
                aRanges.AppendElement(gfxTextRange(origI, i + 1,
                                                   font, matchType));
                lastRangeIndex++;

                
                
                
                if (sizeof(T) == sizeof(uint8_t) ||
                    !gfxFontUtils::IsJoinCauser(ch))
                {
                    prevFont = font;
                }
            }
        }
    }

    aRanges[lastRangeIndex].end = aLength;
}

gfxUserFontSet* 
gfxFontGroup::GetUserFontSet()
{
    return mUserFontSet;
}

void 
gfxFontGroup::SetUserFontSet(gfxUserFontSet *aUserFontSet)
{
    NS_IF_RELEASE(mUserFontSet);
    mUserFontSet = aUserFontSet;
    NS_IF_ADDREF(mUserFontSet);
    mCurrGeneration = GetGeneration();
}

uint64_t
gfxFontGroup::GetGeneration()
{
    if (!mUserFontSet)
        return 0;
    return mUserFontSet->GetGeneration();
}

void
gfxFontGroup::UpdateFontList()
{
    if (mUserFontSet && mCurrGeneration != GetGeneration()) {
        
        mFonts.Clear();
        mUnderlineOffset = UNDERLINE_OFFSET_NOT_SET;
        mSkipDrawing = false;

        
#if defined(XP_MACOSX) || defined(XP_WIN) || defined(ANDROID)
        BuildFontList();
#else
        ForEachFont(FindPlatformFont, this);
#endif
        mCurrGeneration = GetGeneration();
    }
}

struct PrefFontCallbackData {
    PrefFontCallbackData(nsTArray<nsRefPtr<gfxFontFamily> >& aFamiliesArray)
        : mPrefFamilies(aFamiliesArray)
    {}

    nsTArray<nsRefPtr<gfxFontFamily> >& mPrefFamilies;

    static bool AddFontFamilyEntry(eFontPrefLang aLang, const nsAString& aName, void *aClosure)
    {
        PrefFontCallbackData *prefFontData = static_cast<PrefFontCallbackData*>(aClosure);

        gfxFontFamily *family = gfxPlatformFontList::PlatformFontList()->FindFamily(aName);
        if (family) {
            prefFontData->mPrefFamilies.AppendElement(family);
        }
        return true;
    }
};

already_AddRefed<gfxFont>
gfxFontGroup::WhichPrefFontSupportsChar(uint32_t aCh)
{
    gfxFont *font;

    
    uint32_t unicodeRange = FindCharUnicodeRange(aCh);
    eFontPrefLang charLang = gfxPlatform::GetPlatform()->GetFontPrefLangFor(unicodeRange);

    
    if (mLastPrefFont && charLang == mLastPrefLang &&
        mLastPrefFirstFont && mLastPrefFont->HasCharacter(aCh)) {
        font = mLastPrefFont;
        NS_ADDREF(font);
        return font;
    }

    
    eFontPrefLang prefLangs[kMaxLenPrefLangList];
    uint32_t i, numLangs = 0;

    gfxPlatform::GetPlatform()->GetLangPrefs(prefLangs, numLangs, charLang, mPageLang);

    for (i = 0; i < numLangs; i++) {
        nsAutoTArray<nsRefPtr<gfxFontFamily>, 5> families;
        eFontPrefLang currentLang = prefLangs[i];

        gfxPlatformFontList *fontList = gfxPlatformFontList::PlatformFontList();

        
        if (!fontList->GetPrefFontFamilyEntries(currentLang, &families)) {
            eFontPrefLang prefLangsToSearch[1] = { currentLang };
            PrefFontCallbackData prefFontData(families);
            gfxPlatform::ForEachPrefFont(prefLangsToSearch, 1, PrefFontCallbackData::AddFontFamilyEntry,
                                           &prefFontData);
            fontList->SetPrefFontFamilyEntries(currentLang, families);
        }

        
        uint32_t  j, numPrefs;
        numPrefs = families.Length();
        for (j = 0; j < numPrefs; j++) {
            
            gfxFontFamily *family = families[j];
            if (!family) continue;

            
            
            
            
            if (family == mLastPrefFamily && mLastPrefFont->HasCharacter(aCh)) {
                font = mLastPrefFont;
                NS_ADDREF(font);
                return font;
            }

            bool needsBold;
            gfxFontEntry *fe = family->FindFontForStyle(mStyle, needsBold);
            
            if (fe && fe->TestCharacterMap(aCh)) {
                nsRefPtr<gfxFont> prefFont = fe->FindOrMakeFont(&mStyle, needsBold);
                if (!prefFont) continue;
                mLastPrefFamily = family;
                mLastPrefFont = prefFont;
                mLastPrefLang = charLang;
                mLastPrefFirstFont = (i == 0 && j == 0);
                return prefFont.forget();
            }

        }
    }

    return nullptr;
}

already_AddRefed<gfxFont>
gfxFontGroup::WhichSystemFontSupportsChar(uint32_t aCh, int32_t aRunScript)
{
    gfxFontEntry *fe = 
        gfxPlatformFontList::PlatformFontList()->
            SystemFindFontForChar(aCh, aRunScript, &mStyle);
    if (fe) {
        bool wantBold = mStyle.ComputeWeight() >= 6;
        nsRefPtr<gfxFont> font =
            fe->FindOrMakeFont(&mStyle, wantBold && !fe->IsBold());
        return font.forget();
    }

    return nullptr;
}

 void
gfxFontGroup::Shutdown()
{
    NS_IF_RELEASE(gLangService);
}

nsILanguageAtomService* gfxFontGroup::gLangService = nullptr;


#define DEFAULT_PIXEL_FONT_SIZE 16.0f

 uint32_t
gfxFontStyle::ParseFontLanguageOverride(const nsString& aLangTag)
{
  if (!aLangTag.Length() || aLangTag.Length() > 4) {
    return NO_FONT_LANGUAGE_OVERRIDE;
  }
  uint32_t index, result = 0;
  for (index = 0; index < aLangTag.Length(); ++index) {
    PRUnichar ch = aLangTag[index];
    if (!nsCRT::IsAscii(ch)) { 
      return NO_FONT_LANGUAGE_OVERRIDE;
    }
    result = (result << 8) + ch;
  }
  while (index++ < 4) {
    result = (result << 8) + 0x20;
  }
  return result;
}

gfxFontStyle::gfxFontStyle() :
    language(nsGkAtoms::x_western),
    size(DEFAULT_PIXEL_FONT_SIZE), sizeAdjust(0.0f),
    languageOverride(NO_FONT_LANGUAGE_OVERRIDE),
    weight(NS_FONT_WEIGHT_NORMAL), stretch(NS_FONT_STRETCH_NORMAL),
    systemFont(true), printerFont(false), 
    style(NS_FONT_STYLE_NORMAL)
{
}

gfxFontStyle::gfxFontStyle(uint8_t aStyle, uint16_t aWeight, int16_t aStretch,
                           gfxFloat aSize, nsIAtom *aLanguage,
                           float aSizeAdjust, bool aSystemFont,
                           bool aPrinterFont,
                           const nsString& aLanguageOverride):
    language(aLanguage),
    size(aSize), sizeAdjust(aSizeAdjust),
    languageOverride(ParseFontLanguageOverride(aLanguageOverride)),
    weight(aWeight), stretch(aStretch),
    systemFont(aSystemFont), printerFont(aPrinterFont),
    style(aStyle)
{
    MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(size));
    MOZ_ASSERT(!MOZ_DOUBLE_IS_NaN(sizeAdjust));

    if (weight > 900)
        weight = 900;
    if (weight < 100)
        weight = 100;

    if (size >= FONT_MAX_SIZE) {
        size = FONT_MAX_SIZE;
        sizeAdjust = 0.0;
    } else if (size < 0.0) {
        NS_WARNING("negative font size");
        size = 0.0;
    }

    if (!language) {
        NS_WARNING("null language");
        language = nsGkAtoms::x_western;
    }
}

gfxFontStyle::gfxFontStyle(const gfxFontStyle& aStyle) :
    language(aStyle.language),
    size(aStyle.size), sizeAdjust(aStyle.sizeAdjust),
    languageOverride(aStyle.languageOverride),
    weight(aStyle.weight), stretch(aStyle.stretch),
    systemFont(aStyle.systemFont), printerFont(aStyle.printerFont),
    style(aStyle.style)
{
    featureSettings.AppendElements(aStyle.featureSettings);
}

int8_t
gfxFontStyle::ComputeWeight() const
{
    int8_t baseWeight = (weight + 50) / 100;

    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    return baseWeight;
}

void
gfxShapedText::SetupClusterBoundaries(uint32_t         aOffset,
                                      const PRUnichar *aString,
                                      uint32_t         aLength)
{
    CompressedGlyph *glyphs = GetCharacterGlyphs() + aOffset;

    gfxTextRun::CompressedGlyph extendCluster;
    extendCluster.SetComplex(false, true, 0);

    ClusterIterator iter(aString, aLength);

    
    
    if (aLength && IsClusterExtender(*aString)) {
        *glyphs = extendCluster;
    }

    while (!iter.AtEnd()) {
        if (*iter == PRUnichar(' ')) {
            glyphs->SetIsSpace();
        }
        
        iter.Next();
        
        aString++;
        glyphs++;
        
        while (aString < iter) {
            *glyphs = extendCluster;
            if (NS_IS_LOW_SURROGATE(*aString)) {
                glyphs->SetIsLowSurrogate();
            }
            glyphs++;
            aString++;
        }
    }
}

void
gfxShapedText::SetupClusterBoundaries(uint32_t       aOffset,
                                      const uint8_t *aString,
                                      uint32_t       aLength)
{
    CompressedGlyph *glyphs = GetCharacterGlyphs() + aOffset;
    const uint8_t *limit = aString + aLength;

    while (aString < limit) {
        if (*aString == uint8_t(' ')) {
            glyphs->SetIsSpace();
        }
        aString++;
        glyphs++;
    }
}

gfxShapedText::DetailedGlyph *
gfxShapedText::AllocateDetailedGlyphs(uint32_t aIndex, uint32_t aCount)
{
    NS_ASSERTION(aIndex < GetLength(), "Index out of range");

    if (!mDetailedGlyphs) {
        mDetailedGlyphs = new DetailedGlyphStore();
    }

    DetailedGlyph *details = mDetailedGlyphs->Allocate(aIndex, aCount);
    if (!details) {
        GetCharacterGlyphs()[aIndex].SetMissing(0);
        return nullptr;
    }

    return details;
}

void
gfxShapedText::SetGlyphs(uint32_t aIndex, CompressedGlyph aGlyph,
                         const DetailedGlyph *aGlyphs)
{
    NS_ASSERTION(!aGlyph.IsSimpleGlyph(), "Simple glyphs not handled here");
    NS_ASSERTION(aIndex > 0 || aGlyph.IsLigatureGroupStart(),
                 "First character can't be a ligature continuation!");

    uint32_t glyphCount = aGlyph.GetGlyphCount();
    if (glyphCount > 0) {
        DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, glyphCount);
        if (!details) {
            return;
        }
        memcpy(details, aGlyphs, sizeof(DetailedGlyph)*glyphCount);
    }
    GetCharacterGlyphs()[aIndex] = aGlyph;
}

#define ZWNJ 0x200C
#define ZWJ  0x200D
static inline bool
IsDefaultIgnorable(uint32_t aChar)
{
    return GetIdentifierModification(aChar) == XIDMOD_DEFAULT_IGNORABLE ||
           aChar == ZWNJ || aChar == ZWJ;
}

void
gfxShapedText::SetMissingGlyph(uint32_t aIndex, uint32_t aChar, gfxFont *aFont)
{
    uint8_t category = GetGeneralCategory(aChar);
    if (category >= HB_UNICODE_GENERAL_CATEGORY_SPACING_MARK &&
        category <= HB_UNICODE_GENERAL_CATEGORY_NON_SPACING_MARK)
    {
        GetCharacterGlyphs()[aIndex].SetComplex(false, true, 0);
    }

    DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
    if (!details) {
        return;
    }

    details->mGlyphID = aChar;
    if (IsDefaultIgnorable(aChar)) {
        
        details->mAdvance = 0;
    } else {
        gfxFloat width =
            std::max(aFont->GetMetrics().aveCharWidth,
                     gfxFontMissingGlyphs::GetDesiredMinWidth(aChar,
                         mAppUnitsPerDevUnit));
        details->mAdvance = uint32_t(width * mAppUnitsPerDevUnit);
    }
    details->mXOffset = 0;
    details->mYOffset = 0;
    GetCharacterGlyphs()[aIndex].SetMissing(1);
}

bool
gfxShapedText::FilterIfIgnorable(uint32_t aIndex, uint32_t aCh)
{
    if (IsDefaultIgnorable(aCh)) {
        DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
        if (details) {
            details->mGlyphID = aCh;
            details->mAdvance = 0;
            details->mXOffset = 0;
            details->mYOffset = 0;
            GetCharacterGlyphs()[aIndex].SetMissing(1);
            return true;
        }
    }
    return false;
}

void
gfxShapedText::AdjustAdvancesForSyntheticBold(float aSynBoldOffset,
                                              uint32_t aOffset,
                                              uint32_t aLength)
{
    uint32_t synAppUnitOffset = aSynBoldOffset * mAppUnitsPerDevUnit;
    CompressedGlyph *charGlyphs = GetCharacterGlyphs();
    for (uint32_t i = aOffset; i < aOffset + aLength; ++i) {
         CompressedGlyph *glyphData = charGlyphs + i;
         if (glyphData->IsSimpleGlyph()) {
             
             int32_t advance = glyphData->GetSimpleAdvance() + synAppUnitOffset;
             if (CompressedGlyph::IsSimpleAdvance(advance)) {
                 glyphData->SetSimpleGlyph(advance, glyphData->GetSimpleGlyph());
             } else {
                 
                 uint32_t glyphIndex = glyphData->GetSimpleGlyph();
                 glyphData->SetComplex(true, true, 1);
                 DetailedGlyph detail = {glyphIndex, advance, 0, 0};
                 SetGlyphs(i, *glyphData, &detail);
             }
         } else {
             
             uint32_t detailedLength = glyphData->GetGlyphCount();
             if (detailedLength) {
                 DetailedGlyph *details = GetDetailedGlyphs(i);
                 if (!details) {
                     continue;
                 }
                 if (IsRightToLeft()) {
                     details[0].mAdvance += synAppUnitOffset;
                 } else {
                     details[detailedLength - 1].mAdvance += synAppUnitOffset;
                 }
             }
         }
    }
}

bool
gfxTextRun::GlyphRunIterator::NextRun()  {
    if (mNextIndex >= mTextRun->mGlyphRuns.Length())
        return false;
    mGlyphRun = &mTextRun->mGlyphRuns[mNextIndex];
    if (mGlyphRun->mCharacterOffset >= mEndOffset)
        return false;

    mStringStart = std::max(mStartOffset, mGlyphRun->mCharacterOffset);
    uint32_t last = mNextIndex + 1 < mTextRun->mGlyphRuns.Length()
        ? mTextRun->mGlyphRuns[mNextIndex + 1].mCharacterOffset : mTextRun->GetLength();
    mStringEnd = std::min(mEndOffset, last);

    ++mNextIndex;
    return true;
}

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
static void
AccountStorageForTextRun(gfxTextRun *aTextRun, int32_t aSign)
{
    
    
    
    
    
    
    uint32_t length = aTextRun->GetLength();
    int32_t bytes = length * sizeof(gfxTextRun::CompressedGlyph);
    bytes += sizeof(gfxTextRun);
    gTextRunStorage += bytes*aSign;
    gTextRunStorageHighWaterMark = std::max(gTextRunStorageHighWaterMark, gTextRunStorage);
}
#endif




void *
gfxTextRun::AllocateStorageForTextRun(size_t aSize, uint32_t aLength)
{
    
    
    void *storage = moz_malloc(aSize + aLength * sizeof(CompressedGlyph));
    if (!storage) {
        NS_WARNING("failed to allocate storage for text run!");
        return nullptr;
    }

    
    memset(reinterpret_cast<char*>(storage) + aSize, 0,
           aLength * sizeof(CompressedGlyph));

    return storage;
}

gfxTextRun *
gfxTextRun::Create(const gfxTextRunFactory::Parameters *aParams,
                   uint32_t aLength, gfxFontGroup *aFontGroup, uint32_t aFlags)
{
    void *storage = AllocateStorageForTextRun(sizeof(gfxTextRun), aLength);
    if (!storage) {
        return nullptr;
    }

    return new (storage) gfxTextRun(aParams, aLength, aFontGroup, aFlags);
}

gfxTextRun::gfxTextRun(const gfxTextRunFactory::Parameters *aParams,
                       uint32_t aLength, gfxFontGroup *aFontGroup, uint32_t aFlags)
    : gfxShapedText(aLength, aFlags, aParams->mAppUnitsPerDevUnit)
    , mUserData(aParams->mUserData)
    , mFontGroup(aFontGroup)
{
    NS_ASSERTION(mAppUnitsPerDevUnit > 0, "Invalid app unit scale");
    MOZ_COUNT_CTOR(gfxTextRun);
    NS_ADDREF(mFontGroup);

    mCharacterGlyphs = reinterpret_cast<CompressedGlyph*>(this + 1);

    if (aParams->mSkipChars) {
        mSkipChars.TakeFrom(aParams->mSkipChars);
    }

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    AccountStorageForTextRun(this, 1);
#endif

    mSkipDrawing = mFontGroup->ShouldSkipDrawing();
}

gfxTextRun::~gfxTextRun()
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    AccountStorageForTextRun(this, -1);
#endif
#ifdef DEBUG
    
    mFlags = 0xFFFFFFFF;
#endif

    NS_RELEASE(mFontGroup);
    MOZ_COUNT_DTOR(gfxTextRun);
}

bool
gfxTextRun::SetPotentialLineBreaks(uint32_t aStart, uint32_t aLength,
                                   uint8_t *aBreakBefore,
                                   gfxContext *aRefContext)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Overflow");

    uint32_t changed = 0;
    uint32_t i;
    CompressedGlyph *charGlyphs = mCharacterGlyphs + aStart;
    for (i = 0; i < aLength; ++i) {
        uint8_t canBreak = aBreakBefore[i];
        if (canBreak && !charGlyphs[i].IsClusterStart()) {
            
            
            NS_WARNING("Break suggested inside cluster!");
            canBreak = CompressedGlyph::FLAG_BREAK_TYPE_NONE;
        }
        changed |= charGlyphs[i].SetCanBreakBefore(canBreak);
    }
    return changed != 0;
}

gfxTextRun::LigatureData
gfxTextRun::ComputeLigatureData(uint32_t aPartStart, uint32_t aPartEnd,
                                PropertyProvider *aProvider)
{
    NS_ASSERTION(aPartStart < aPartEnd, "Computing ligature data for empty range");
    NS_ASSERTION(aPartEnd <= GetLength(), "Character length overflow");
  
    LigatureData result;
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    uint32_t i;
    for (i = aPartStart; !charGlyphs[i].IsLigatureGroupStart(); --i) {
        NS_ASSERTION(i > 0, "Ligature at the start of the run??");
    }
    result.mLigatureStart = i;
    for (i = aPartStart + 1; i < GetLength() && !charGlyphs[i].IsLigatureGroupStart(); ++i) {
    }
    result.mLigatureEnd = i;

    int32_t ligatureWidth =
        GetAdvanceForGlyphs(result.mLigatureStart, result.mLigatureEnd);
    
    uint32_t totalClusterCount = 0;
    uint32_t partClusterIndex = 0;
    uint32_t partClusterCount = 0;
    for (i = result.mLigatureStart; i < result.mLigatureEnd; ++i) {
        
        
        
        if (i == result.mLigatureStart || charGlyphs[i].IsClusterStart()) {
            ++totalClusterCount;
            if (i < aPartStart) {
                ++partClusterIndex;
            } else if (i < aPartEnd) {
                ++partClusterCount;
            }
        }
    }
    NS_ASSERTION(totalClusterCount > 0, "Ligature involving no clusters??");
    result.mPartAdvance = partClusterIndex * (ligatureWidth / totalClusterCount);
    result.mPartWidth = partClusterCount * (ligatureWidth / totalClusterCount);

    
    
    
    if (aPartEnd == result.mLigatureEnd) {
        gfxFloat allParts = totalClusterCount * (ligatureWidth / totalClusterCount);
        result.mPartWidth += ligatureWidth - allParts;
    }

    if (partClusterCount == 0) {
        
        result.mClipBeforePart = result.mClipAfterPart = true;
    } else {
        
        
        
        
        result.mClipBeforePart = partClusterIndex > 0;
        
        
        result.mClipAfterPart = partClusterIndex + partClusterCount < totalClusterCount;
    }

    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        gfxFont::Spacing spacing;
        if (aPartStart == result.mLigatureStart) {
            aProvider->GetSpacing(aPartStart, 1, &spacing);
            result.mPartWidth += spacing.mBefore;
        }
        if (aPartEnd == result.mLigatureEnd) {
            aProvider->GetSpacing(aPartEnd - 1, 1, &spacing);
            result.mPartWidth += spacing.mAfter;
        }
    }

    return result;
}

gfxFloat
gfxTextRun::ComputePartialLigatureWidth(uint32_t aPartStart, uint32_t aPartEnd,
                                        PropertyProvider *aProvider)
{
    if (aPartStart >= aPartEnd)
        return 0;
    LigatureData data = ComputeLigatureData(aPartStart, aPartEnd, aProvider);
    return data.mPartWidth;
}

int32_t
gfxTextRun::GetAdvanceForGlyphs(uint32_t aStart, uint32_t aEnd)
{
    const CompressedGlyph *glyphData = mCharacterGlyphs + aStart;
    int32_t advance = 0;
    uint32_t i;
    for (i = aStart; i < aEnd; ++i, ++glyphData) {
        if (glyphData->IsSimpleGlyph()) {
            advance += glyphData->GetSimpleAdvance();   
        } else {
            uint32_t glyphCount = glyphData->GetGlyphCount();
            if (glyphCount == 0) {
                continue;
            }
            const DetailedGlyph *details = GetDetailedGlyphs(i);
            if (details) {
                uint32_t j;
                for (j = 0; j < glyphCount; ++j, ++details) {
                    advance += details->mAdvance;
                }
            }
        }
    }
    return advance;
}

static void
GetAdjustedSpacing(gfxTextRun *aTextRun, uint32_t aStart, uint32_t aEnd,
                   gfxTextRun::PropertyProvider *aProvider,
                   gfxTextRun::PropertyProvider::Spacing *aSpacing)
{
    if (aStart >= aEnd)
        return;

    aProvider->GetSpacing(aStart, aEnd - aStart, aSpacing);

#ifdef DEBUG
    

    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    uint32_t i;

    for (i = aStart; i < aEnd; ++i) {
        if (!charGlyphs[i].IsLigatureGroupStart()) {
            NS_ASSERTION(i == aStart || aSpacing[i - aStart].mBefore == 0,
                         "Before-spacing inside a ligature!");
            NS_ASSERTION(i - 1 <= aStart || aSpacing[i - 1 - aStart].mAfter == 0,
                         "After-spacing inside a ligature!");
        }
    }
#endif
}

bool
gfxTextRun::GetAdjustedSpacingArray(uint32_t aStart, uint32_t aEnd,
                                    PropertyProvider *aProvider,
                                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                    nsTArray<PropertyProvider::Spacing> *aSpacing)
{
    if (!aProvider || !(mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING))
        return false;
    if (!aSpacing->AppendElements(aEnd - aStart))
        return false;
    memset(aSpacing->Elements(), 0, sizeof(gfxFont::Spacing)*(aSpacingStart - aStart));
    GetAdjustedSpacing(this, aSpacingStart, aSpacingEnd, aProvider,
                       aSpacing->Elements() + aSpacingStart - aStart);
    memset(aSpacing->Elements() + aSpacingEnd - aStart, 0, sizeof(gfxFont::Spacing)*(aEnd - aSpacingEnd));
    return true;
}

void
gfxTextRun::ShrinkToLigatureBoundaries(uint32_t *aStart, uint32_t *aEnd)
{
    if (*aStart >= *aEnd)
        return;
  
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    while (*aStart < *aEnd && !charGlyphs[*aStart].IsLigatureGroupStart()) {
        ++(*aStart);
    }
    if (*aEnd < GetLength()) {
        while (*aEnd > *aStart && !charGlyphs[*aEnd].IsLigatureGroupStart()) {
            --(*aEnd);
        }
    }
}

void
gfxTextRun::DrawGlyphs(gfxFont *aFont, gfxContext *aContext,
                       gfxFont::DrawMode aDrawMode, gfxPoint *aPt,
                       gfxTextObjectPaint *aObjectPaint,
                       uint32_t aStart, uint32_t aEnd,
                       PropertyProvider *aProvider,
                       uint32_t aSpacingStart, uint32_t aSpacingEnd,
                       gfxTextRunDrawCallbacks *aCallbacks)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    bool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider,
        aSpacingStart, aSpacingEnd, &spacingBuffer);
    aFont->Draw(this, aStart, aEnd, aContext, aDrawMode, aPt,
                haveSpacing ? spacingBuffer.Elements() : nullptr, aObjectPaint,
                aCallbacks);
}

static void
ClipPartialLigature(gfxTextRun *aTextRun, gfxFloat *aLeft, gfxFloat *aRight,
                    gfxFloat aXOrigin, gfxTextRun::LigatureData *aLigature)
{
    if (aLigature->mClipBeforePart) {
        if (aTextRun->IsRightToLeft()) {
            *aRight = std::min(*aRight, aXOrigin);
        } else {
            *aLeft = std::max(*aLeft, aXOrigin);
        }
    }
    if (aLigature->mClipAfterPart) {
        gfxFloat endEdge = aXOrigin + aTextRun->GetDirection()*aLigature->mPartWidth;
        if (aTextRun->IsRightToLeft()) {
            *aLeft = std::max(*aLeft, endEdge);
        } else {
            *aRight = std::min(*aRight, endEdge);
        }
    }    
}

void
gfxTextRun::DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx,
                                uint32_t aStart, uint32_t aEnd,
                                gfxPoint *aPt,
                                PropertyProvider *aProvider,
                                gfxTextRunDrawCallbacks *aCallbacks)
{
    if (aStart >= aEnd)
        return;

    
    LigatureData data = ComputeLigatureData(aStart, aEnd, aProvider);
    gfxRect clipExtents = aCtx->GetClipExtents();
    gfxFloat left = clipExtents.X()*mAppUnitsPerDevUnit;
    gfxFloat right = clipExtents.XMost()*mAppUnitsPerDevUnit;
    ClipPartialLigature(this, &left, &right, aPt->x, &data);

    {
      
      
      
      gfxContextPathAutoSaveRestore savePath(aCtx);

      
      
      
      aCtx->Save();
      aCtx->NewPath();
      aCtx->Rectangle(gfxRect(left / mAppUnitsPerDevUnit,
                              clipExtents.Y(),
                              (right - left) / mAppUnitsPerDevUnit,
                              clipExtents.Height()), true);
      aCtx->Clip();
    }

    gfxFloat direction = GetDirection();
    gfxPoint pt(aPt->x - direction*data.mPartAdvance, aPt->y);
    DrawGlyphs(aFont, aCtx,
               aCallbacks ? gfxFont::GLYPH_PATH : gfxFont::GLYPH_FILL, &pt,
               nullptr, data.mLigatureStart, data.mLigatureEnd, aProvider,
               aStart, aEnd, aCallbacks);
    aCtx->Restore();

    aPt->x += direction*data.mPartWidth;
}


static bool
HasSyntheticBold(gfxTextRun *aRun, uint32_t aStart, uint32_t aLength)
{
    gfxTextRun::GlyphRunIterator iter(aRun, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        if (font && font->IsSyntheticBold()) {
            return true;
        }
    }

    return false;
}



static bool
HasNonOpaqueColor(gfxContext *aContext, gfxRGBA& aCurrentColor)
{
    if (aContext->GetDeviceColor(aCurrentColor)) {
        if (aCurrentColor.a < 1.0 && aCurrentColor.a > 0.0) {
            return true;
        }
    }
        
    return false;
}


struct BufferAlphaColor {
    BufferAlphaColor(gfxContext *aContext)
        : mContext(aContext)
    {

    }

    ~BufferAlphaColor() {}

    void PushSolidColor(const gfxRect& aBounds, const gfxRGBA& aAlphaColor, uint32_t appsPerDevUnit)
    {
        mContext->Save();
        mContext->NewPath();
        mContext->Rectangle(gfxRect(aBounds.X() / appsPerDevUnit,
                    aBounds.Y() / appsPerDevUnit,
                    aBounds.Width() / appsPerDevUnit,
                    aBounds.Height() / appsPerDevUnit), true);
        mContext->Clip();
        mContext->SetColor(gfxRGBA(aAlphaColor.r, aAlphaColor.g, aAlphaColor.b));
        mContext->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
        mAlpha = aAlphaColor.a;
    }

    void PopAlpha()
    {
        
        mContext->PopGroupToSource();
        mContext->SetOperator(gfxContext::OPERATOR_OVER);
        mContext->Paint(mAlpha);
        mContext->Restore();
    }

    gfxContext *mContext;
    gfxFloat mAlpha;
};

void
gfxTextRun::Draw(gfxContext *aContext, gfxPoint aPt, gfxFont::DrawMode aDrawMode,
                 uint32_t aStart, uint32_t aLength,
                 PropertyProvider *aProvider, gfxFloat *aAdvanceWidth,
                 gfxTextObjectPaint *aObjectPaint,
                 gfxTextRunDrawCallbacks *aCallbacks)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Substring out of range");
    NS_ASSERTION(aDrawMode == gfxFont::GLYPH_PATH || !(aDrawMode & gfxFont::GLYPH_PATH),
                 "GLYPH_PATH cannot be used with GLYPH_FILL, GLYPH_STROKE or GLYPH_STROKE_UNDERNEATH");
    NS_ASSERTION(aDrawMode == gfxFont::GLYPH_PATH || !aCallbacks, "callback must not be specified unless using GLYPH_PATH");

    gfxFloat direction = GetDirection();

    if (mSkipDrawing) {
        
        
        if (aAdvanceWidth) {
            gfxTextRun::Metrics metrics = MeasureText(aStart, aLength,
                                                      gfxFont::LOOSE_INK_EXTENTS,
                                                      aContext, aProvider);
            *aAdvanceWidth = metrics.mAdvanceWidth * direction;
        }

        
        return;
    }

    gfxPoint pt = aPt;

    
    BufferAlphaColor syntheticBoldBuffer(aContext);
    gfxRGBA currentColor;
    bool needToRestore = false;

    if (aDrawMode == gfxFont::GLYPH_FILL && HasNonOpaqueColor(aContext, currentColor)
                                         && HasSyntheticBold(this, aStart, aLength)) {
        needToRestore = true;
        
        gfxTextRun::Metrics metrics = MeasureText(aStart, aLength, gfxFont::LOOSE_INK_EXTENTS,
                                                  aContext, aProvider);
        metrics.mBoundingBox.MoveBy(aPt);
        syntheticBoldBuffer.PushSolidColor(metrics.mBoundingBox, currentColor, GetAppUnitsPerDevUnit());
    }

    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        uint32_t start = iter.GetStringStart();
        uint32_t end = iter.GetStringEnd();
        uint32_t ligatureRunStart = start;
        uint32_t ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);
        
        bool drawPartial = aDrawMode == gfxFont::GLYPH_FILL ||
                           (aDrawMode == gfxFont::GLYPH_PATH && aCallbacks);

        if (drawPartial) {
            DrawPartialLigature(font, aContext, start, ligatureRunStart, &pt,
                                aProvider, aCallbacks);
        }

        DrawGlyphs(font, aContext, aDrawMode, &pt, aObjectPaint, ligatureRunStart,
                   ligatureRunEnd, aProvider, ligatureRunStart, ligatureRunEnd,
                   aCallbacks);

        if (drawPartial) {
            DrawPartialLigature(font, aContext, ligatureRunEnd, end, &pt,
                                aProvider, aCallbacks);
        }
    }

    
    if (needToRestore) {
        syntheticBoldBuffer.PopAlpha();
    }

    if (aAdvanceWidth) {
        *aAdvanceWidth = (pt.x - aPt.x)*direction;
    }
}

void
gfxTextRun::AccumulateMetricsForRun(gfxFont *aFont,
                                    uint32_t aStart, uint32_t aEnd,
                                    gfxFont::BoundingBoxType aBoundingBoxType,
                                    gfxContext *aRefContext,
                                    PropertyProvider *aProvider,
                                    uint32_t aSpacingStart, uint32_t aSpacingEnd,
                                    Metrics *aMetrics)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    bool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider,
        aSpacingStart, aSpacingEnd, &spacingBuffer);
    Metrics metrics = aFont->Measure(this, aStart, aEnd, aBoundingBoxType, aRefContext,
                                     haveSpacing ? spacingBuffer.Elements() : nullptr);
    aMetrics->CombineWith(metrics, IsRightToLeft());
}

void
gfxTextRun::AccumulatePartialLigatureMetrics(gfxFont *aFont,
    uint32_t aStart, uint32_t aEnd,
    gfxFont::BoundingBoxType aBoundingBoxType, gfxContext *aRefContext,
    PropertyProvider *aProvider, Metrics *aMetrics)
{
    if (aStart >= aEnd)
        return;

    
    
    LigatureData data = ComputeLigatureData(aStart, aEnd, aProvider);

    
    Metrics metrics;
    AccumulateMetricsForRun(aFont, data.mLigatureStart, data.mLigatureEnd,
                            aBoundingBoxType, aRefContext,
                            aProvider, aStart, aEnd, &metrics);

    
    gfxFloat bboxLeft = metrics.mBoundingBox.X();
    gfxFloat bboxRight = metrics.mBoundingBox.XMost();
    
    gfxFloat origin = IsRightToLeft() ? metrics.mAdvanceWidth - data.mPartAdvance : 0;
    ClipPartialLigature(this, &bboxLeft, &bboxRight, origin, &data);
    metrics.mBoundingBox.x = bboxLeft;
    metrics.mBoundingBox.width = bboxRight - bboxLeft;

    
    
    metrics.mBoundingBox.x -=
        IsRightToLeft() ? metrics.mAdvanceWidth - (data.mPartAdvance + data.mPartWidth)
            : data.mPartAdvance;    
    metrics.mAdvanceWidth = data.mPartWidth;

    aMetrics->CombineWith(metrics, IsRightToLeft());
}

gfxTextRun::Metrics
gfxTextRun::MeasureText(uint32_t aStart, uint32_t aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContext,
                        PropertyProvider *aProvider)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Substring out of range");

    Metrics accumulatedMetrics;
    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        uint32_t start = iter.GetStringStart();
        uint32_t end = iter.GetStringEnd();
        uint32_t ligatureRunStart = start;
        uint32_t ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

        AccumulatePartialLigatureMetrics(font, start, ligatureRunStart,
            aBoundingBoxType, aRefContext, aProvider, &accumulatedMetrics);

        
        
        
        
        
        AccumulateMetricsForRun(font,
            ligatureRunStart, ligatureRunEnd, aBoundingBoxType,
            aRefContext, aProvider, ligatureRunStart, ligatureRunEnd,
            &accumulatedMetrics);

        AccumulatePartialLigatureMetrics(font, ligatureRunEnd, end,
            aBoundingBoxType, aRefContext, aProvider, &accumulatedMetrics);
    }

    return accumulatedMetrics;
}

#define MEASUREMENT_BUFFER_SIZE 100

uint32_t
gfxTextRun::BreakAndMeasureText(uint32_t aStart, uint32_t aMaxLength,
                                bool aLineBreakBefore, gfxFloat aWidth,
                                PropertyProvider *aProvider,
                                bool aSuppressInitialBreak,
                                gfxFloat *aTrimWhitespace,
                                Metrics *aMetrics,
                                gfxFont::BoundingBoxType aBoundingBoxType,
                                gfxContext *aRefContext,
                                bool *aUsedHyphenation,
                                uint32_t *aLastBreak,
                                bool aCanWordWrap,
                                gfxBreakPriority *aBreakPriority)
{
    aMaxLength = std::min(aMaxLength, GetLength() - aStart);

    NS_ASSERTION(aStart + aMaxLength <= GetLength(), "Substring out of range");

    uint32_t bufferStart = aStart;
    uint32_t bufferLength = std::min<uint32_t>(aMaxLength, MEASUREMENT_BUFFER_SIZE);
    PropertyProvider::Spacing spacingBuffer[MEASUREMENT_BUFFER_SIZE];
    bool haveSpacing = aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING) != 0;
    if (haveSpacing) {
        GetAdjustedSpacing(this, bufferStart, bufferStart + bufferLength, aProvider,
                           spacingBuffer);
    }
    bool hyphenBuffer[MEASUREMENT_BUFFER_SIZE];
    bool haveHyphenation = aProvider &&
        (aProvider->GetHyphensOption() == NS_STYLE_HYPHENS_AUTO ||
         (aProvider->GetHyphensOption() == NS_STYLE_HYPHENS_MANUAL &&
          (mFlags & gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS) != 0));
    if (haveHyphenation) {
        aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                        hyphenBuffer);
    }

    gfxFloat width = 0;
    gfxFloat advance = 0;
    
    uint32_t trimmableChars = 0;
    
    gfxFloat trimmableAdvance = 0;
    int32_t lastBreak = -1;
    int32_t lastBreakTrimmableChars = -1;
    gfxFloat lastBreakTrimmableAdvance = -1;
    bool aborted = false;
    uint32_t end = aStart + aMaxLength;
    bool lastBreakUsedHyphenation = false;

    uint32_t ligatureRunStart = aStart;
    uint32_t ligatureRunEnd = end;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    uint32_t i;
    for (i = aStart; i < end; ++i) {
        if (i >= bufferStart + bufferLength) {
            
            bufferStart = i;
            bufferLength = std::min(aStart + aMaxLength, i + MEASUREMENT_BUFFER_SIZE) - i;
            if (haveSpacing) {
                GetAdjustedSpacing(this, bufferStart, bufferStart + bufferLength, aProvider,
                                   spacingBuffer);
            }
            if (haveHyphenation) {
                aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                                hyphenBuffer);
            }
        }

        
        
        
        
        if (!aSuppressInitialBreak || i > aStart) {
            bool lineBreakHere = mCharacterGlyphs[i].CanBreakBefore() == 1;
            bool hyphenation = haveHyphenation && hyphenBuffer[i - bufferStart];
            bool wordWrapping =
                aCanWordWrap && mCharacterGlyphs[i].IsClusterStart() &&
                *aBreakPriority <= eWordWrapBreak;

            if (lineBreakHere || hyphenation || wordWrapping) {
                gfxFloat hyphenatedAdvance = advance;
                if (!lineBreakHere && !wordWrapping) {
                    hyphenatedAdvance += aProvider->GetHyphenWidth();
                }
            
                if (lastBreak < 0 || width + hyphenatedAdvance - trimmableAdvance <= aWidth) {
                    
                    lastBreak = i;
                    lastBreakTrimmableChars = trimmableChars;
                    lastBreakTrimmableAdvance = trimmableAdvance;
                    lastBreakUsedHyphenation = !lineBreakHere && !wordWrapping;
                    *aBreakPriority = hyphenation || lineBreakHere ?
                        eNormalBreak : eWordWrapBreak;
                }

                width += advance;
                advance = 0;
                if (width - trimmableAdvance > aWidth) {
                    
                    aborted = true;
                    break;
                }
            }
        }
        
        gfxFloat charAdvance;
        if (i >= ligatureRunStart && i < ligatureRunEnd) {
            charAdvance = GetAdvanceForGlyphs(i, i + 1);
            if (haveSpacing) {
                PropertyProvider::Spacing *space = &spacingBuffer[i - bufferStart];
                charAdvance += space->mBefore + space->mAfter;
            }
        } else {
            charAdvance = ComputePartialLigatureWidth(i, i + 1, aProvider);
        }
        
        advance += charAdvance;
        if (aTrimWhitespace) {
            if (mCharacterGlyphs[i].CharIsSpace()) {
                ++trimmableChars;
                trimmableAdvance += charAdvance;
            } else {
                trimmableAdvance = 0;
                trimmableChars = 0;
            }
        }
    }

    if (!aborted) {
        width += advance;
    }

    
    
    
    
    uint32_t charsFit;
    bool usedHyphenation = false;
    if (width - trimmableAdvance <= aWidth) {
        charsFit = aMaxLength;
    } else if (lastBreak >= 0) {
        charsFit = lastBreak - aStart;
        trimmableChars = lastBreakTrimmableChars;
        trimmableAdvance = lastBreakTrimmableAdvance;
        usedHyphenation = lastBreakUsedHyphenation;
    } else {
        charsFit = aMaxLength;
    }

    if (aMetrics) {
        *aMetrics = MeasureText(aStart, charsFit - trimmableChars,
            aBoundingBoxType, aRefContext, aProvider);
    }
    if (aTrimWhitespace) {
        *aTrimWhitespace = trimmableAdvance;
    }
    if (aUsedHyphenation) {
        *aUsedHyphenation = usedHyphenation;
    }
    if (aLastBreak && charsFit == aMaxLength) {
        if (lastBreak < 0) {
            *aLastBreak = UINT32_MAX;
        } else {
            *aLastBreak = lastBreak - aStart;
        }
    }

    return charsFit;
}

gfxFloat
gfxTextRun::GetAdvanceWidth(uint32_t aStart, uint32_t aLength,
                            PropertyProvider *aProvider)
{
    NS_ASSERTION(aStart + aLength <= GetLength(), "Substring out of range");

    uint32_t ligatureRunStart = aStart;
    uint32_t ligatureRunEnd = aStart + aLength;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    gfxFloat result = ComputePartialLigatureWidth(aStart, ligatureRunStart, aProvider) +
                      ComputePartialLigatureWidth(ligatureRunEnd, aStart + aLength, aProvider);

    
    
    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        uint32_t i;
        nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
        if (spacingBuffer.AppendElements(aLength)) {
            GetAdjustedSpacing(this, ligatureRunStart, ligatureRunEnd, aProvider,
                               spacingBuffer.Elements());
            for (i = 0; i < ligatureRunEnd - ligatureRunStart; ++i) {
                PropertyProvider::Spacing *space = &spacingBuffer[i];
                result += space->mBefore + space->mAfter;
            }
        }
    }

    return result + GetAdvanceForGlyphs(ligatureRunStart, ligatureRunEnd);
}

bool
gfxTextRun::SetLineBreaks(uint32_t aStart, uint32_t aLength,
                          bool aLineBreakBefore, bool aLineBreakAfter,
                          gfxFloat *aAdvanceWidthDelta,
                          gfxContext *aRefContext)
{
    
    
    if (aAdvanceWidthDelta) {
        *aAdvanceWidthDelta = 0;
    }
    return false;
}

uint32_t
gfxTextRun::FindFirstGlyphRunContaining(uint32_t aOffset)
{
    NS_ASSERTION(aOffset <= GetLength(), "Bad offset looking for glyphrun");
    NS_ASSERTION(GetLength() == 0 || mGlyphRuns.Length() > 0,
                 "non-empty text but no glyph runs present!");
    if (aOffset == GetLength())
        return mGlyphRuns.Length();
    uint32_t start = 0;
    uint32_t end = mGlyphRuns.Length();
    while (end - start > 1) {
        uint32_t mid = (start + end)/2;
        if (mGlyphRuns[mid].mCharacterOffset <= aOffset) {
            start = mid;
        } else {
            end = mid;
        }
    }
    NS_ASSERTION(mGlyphRuns[start].mCharacterOffset <= aOffset,
                 "Hmm, something went wrong, aOffset should have been found");
    return start;
}

nsresult
gfxTextRun::AddGlyphRun(gfxFont *aFont, uint8_t aMatchType,
                        uint32_t aUTF16Offset, bool aForceNewRun)
{
    NS_ASSERTION(aFont, "adding glyph run for null font!");
    if (!aFont) {
        return NS_OK;
    }    
    uint32_t numGlyphRuns = mGlyphRuns.Length();
    if (!aForceNewRun && numGlyphRuns > 0) {
        GlyphRun *lastGlyphRun = &mGlyphRuns[numGlyphRuns - 1];

        NS_ASSERTION(lastGlyphRun->mCharacterOffset <= aUTF16Offset,
                     "Glyph runs out of order (and run not forced)");

        
        if (lastGlyphRun->mFont == aFont &&
            lastGlyphRun->mMatchType == aMatchType)
        {
            return NS_OK;
        }

        
        
        if (lastGlyphRun->mCharacterOffset == aUTF16Offset) {

            
            
            
            if (numGlyphRuns > 1 &&
                mGlyphRuns[numGlyphRuns - 2].mFont == aFont &&
                mGlyphRuns[numGlyphRuns - 2].mMatchType == aMatchType)
            {
                mGlyphRuns.TruncateLength(numGlyphRuns - 1);
                return NS_OK;
            }

            lastGlyphRun->mFont = aFont;
            lastGlyphRun->mMatchType = aMatchType;
            return NS_OK;
        }
    }

    NS_ASSERTION(aForceNewRun || numGlyphRuns > 0 || aUTF16Offset == 0,
                 "First run doesn't cover the first character (and run not forced)?");

    GlyphRun *glyphRun = mGlyphRuns.AppendElement();
    if (!glyphRun)
        return NS_ERROR_OUT_OF_MEMORY;
    glyphRun->mFont = aFont;
    glyphRun->mCharacterOffset = aUTF16Offset;
    glyphRun->mMatchType = aMatchType;
    return NS_OK;
}

void
gfxTextRun::SortGlyphRuns()
{
    if (mGlyphRuns.Length() <= 1)
        return;

    nsTArray<GlyphRun> runs(mGlyphRuns);
    GlyphRunOffsetComparator comp;
    runs.Sort(comp);

    
    mGlyphRuns.Clear();
    uint32_t i, count = runs.Length();
    for (i = 0; i < count; ++i) {
        
        
        if (i == 0 || runs[i].mFont != runs[i - 1].mFont) {
            mGlyphRuns.AppendElement(runs[i]);
            
            
            NS_ASSERTION(i == 0 ||
                         runs[i].mCharacterOffset !=
                         runs[i - 1].mCharacterOffset,
                         "Two fonts for the same run, glyph indices may not match the font");
        }
    }
}




void
gfxTextRun::SanitizeGlyphRuns()
{
    if (mGlyphRuns.Length() <= 1)
        return;

    
    
    
    
    int32_t i, lastRunIndex = mGlyphRuns.Length() - 1;
    const CompressedGlyph *charGlyphs = mCharacterGlyphs;
    for (i = lastRunIndex; i >= 0; --i) {
        GlyphRun& run = mGlyphRuns[i];
        while (charGlyphs[run.mCharacterOffset].IsLigatureContinuation() &&
               run.mCharacterOffset < GetLength()) {
            run.mCharacterOffset++;
        }
        
        if ((i < lastRunIndex &&
             run.mCharacterOffset >= mGlyphRuns[i+1].mCharacterOffset) ||
            (i == lastRunIndex && run.mCharacterOffset == GetLength())) {
            mGlyphRuns.RemoveElementAt(i);
            --lastRunIndex;
        }
    }
}

uint32_t
gfxTextRun::CountMissingGlyphs()
{
    uint32_t i;
    uint32_t count = 0;
    for (i = 0; i < GetLength(); ++i) {
        if (mCharacterGlyphs[i].IsMissing()) {
            ++count;
        }
    }
    return count;
}

gfxTextRun::DetailedGlyph *
gfxTextRun::AllocateDetailedGlyphs(uint32_t aIndex, uint32_t aCount)
{
    NS_ASSERTION(aIndex < GetLength(), "Index out of range");

    if (!mDetailedGlyphs) {
        mDetailedGlyphs = new DetailedGlyphStore();
    }

    DetailedGlyph *details = mDetailedGlyphs->Allocate(aIndex, aCount);
    if (!details) {
        mCharacterGlyphs[aIndex].SetMissing(0);
        return nullptr;
    }

    return details;
}

void
gfxTextRun::CopyGlyphDataFrom(gfxShapedWord *aShapedWord, uint32_t aOffset)
{
    uint32_t wordLen = aShapedWord->GetLength();
    NS_ASSERTION(aOffset + wordLen <= GetLength(),
                 "word overruns end of textrun!");

    CompressedGlyph *charGlyphs = GetCharacterGlyphs();
    const CompressedGlyph *wordGlyphs = aShapedWord->GetCharacterGlyphs();
    if (aShapedWord->HasDetailedGlyphs()) {
        for (uint32_t i = 0; i < wordLen; ++i, ++aOffset) {
            const CompressedGlyph& g = wordGlyphs[i];
            if (g.IsSimpleGlyph()) {
                charGlyphs[aOffset] = g;
            } else {
                const DetailedGlyph *details =
                    g.GetGlyphCount() > 0 ?
                        aShapedWord->GetDetailedGlyphs(i) : nullptr;
                SetGlyphs(aOffset, g, details);
            }
        }
    } else {
        memcpy(charGlyphs + aOffset, wordGlyphs,
               wordLen * sizeof(CompressedGlyph));
    }
}

void
gfxTextRun::CopyGlyphDataFrom(gfxTextRun *aSource, uint32_t aStart,
                              uint32_t aLength, uint32_t aDest)
{
    NS_ASSERTION(aStart + aLength <= aSource->GetLength(),
                 "Source substring out of range");
    NS_ASSERTION(aDest + aLength <= GetLength(),
                 "Destination substring out of range");

    if (aSource->mSkipDrawing) {
        mSkipDrawing = true;
    }

    
    const CompressedGlyph *srcGlyphs = aSource->mCharacterGlyphs + aStart;
    CompressedGlyph *dstGlyphs = mCharacterGlyphs + aDest;
    for (uint32_t i = 0; i < aLength; ++i) {
        CompressedGlyph g = srcGlyphs[i];
        g.SetCanBreakBefore(!g.IsClusterStart() ?
            CompressedGlyph::FLAG_BREAK_TYPE_NONE :
            dstGlyphs[i].CanBreakBefore());
        if (!g.IsSimpleGlyph()) {
            uint32_t count = g.GetGlyphCount();
            if (count > 0) {
                DetailedGlyph *dst = AllocateDetailedGlyphs(i + aDest, count);
                if (dst) {
                    DetailedGlyph *src = aSource->GetDetailedGlyphs(i + aStart);
                    if (src) {
                        ::memcpy(dst, src, count * sizeof(DetailedGlyph));
                    } else {
                        g.SetMissing(0);
                    }
                } else {
                    g.SetMissing(0);
                }
            }
        }
        dstGlyphs[i] = g;
    }

    
    GlyphRunIterator iter(aSource, aStart, aLength);
#ifdef DEBUG
    gfxFont *lastFont = nullptr;
#endif
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        NS_ASSERTION(font != lastFont, "Glyphruns not coalesced?");
#ifdef DEBUG
        lastFont = font;
        uint32_t end = iter.GetStringEnd();
#endif
        uint32_t start = iter.GetStringStart();

        
        
        
        
        
        
        
        
        
        NS_WARN_IF_FALSE(aSource->IsClusterStart(start),
                         "Started font run in the middle of a cluster");
        NS_WARN_IF_FALSE(end == aSource->GetLength() || aSource->IsClusterStart(end),
                         "Ended font run in the middle of a cluster");

        nsresult rv = AddGlyphRun(font, iter.GetGlyphRun()->mMatchType,
                                  start - aStart + aDest, false);
        if (NS_FAILED(rv))
            return;
    }
}

void
gfxTextRun::SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext,
                          uint32_t aCharIndex)
{
    if (SetSpaceGlyphIfSimple(aFont, aContext, aCharIndex, ' ')) {
        return;
    }

    aFont->InitWordCache();
    static const uint8_t space = ' ';
    gfxShapedWord *sw = aFont->GetShapedWord(aContext,
                                             &space, 1,
                                             HashMix(0, ' '), 
                                             MOZ_SCRIPT_LATIN,
                                             mAppUnitsPerDevUnit,
                                             gfxTextRunFactory::TEXT_IS_8BIT |
                                             gfxTextRunFactory::TEXT_IS_ASCII |
                                             gfxTextRunFactory::TEXT_IS_PERSISTENT);
    if (sw) {
        AddGlyphRun(aFont, gfxTextRange::kFontGroup, aCharIndex, false);
        CopyGlyphDataFrom(sw, aCharIndex);
    }
}

bool
gfxTextRun::SetSpaceGlyphIfSimple(gfxFont *aFont, gfxContext *aContext,
                                  uint32_t aCharIndex, PRUnichar aSpaceChar)
{
    uint32_t spaceGlyph = aFont->GetSpaceGlyph();
    if (!spaceGlyph || !CompressedGlyph::IsSimpleGlyphID(spaceGlyph)) {
        return false;
    }

    uint32_t spaceWidthAppUnits =
        NS_lroundf(aFont->GetMetrics().spaceWidth * mAppUnitsPerDevUnit);
    if (!CompressedGlyph::IsSimpleAdvance(spaceWidthAppUnits)) {
        return false;
    }

    AddGlyphRun(aFont, gfxTextRange::kFontGroup, aCharIndex, false);
    CompressedGlyph g;
    g.SetSimpleGlyph(spaceWidthAppUnits, spaceGlyph);
    if (aSpaceChar == ' ') {
        g.SetIsSpace();
    }
    GetCharacterGlyphs()[aCharIndex] = g;
    return true;
}

void
gfxTextRun::FetchGlyphExtents(gfxContext *aRefContext)
{
    bool needsGlyphExtents = NeedsGlyphExtents(this);
    if (!needsGlyphExtents && !mDetailedGlyphs)
        return;

    uint32_t i, runCount = mGlyphRuns.Length();
    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    for (i = 0; i < runCount; ++i) {
        const GlyphRun& run = mGlyphRuns[i];
        gfxFont *font = run.mFont;
        uint32_t start = run.mCharacterOffset;
        uint32_t end = i + 1 < runCount ?
            mGlyphRuns[i + 1].mCharacterOffset : GetLength();
        bool fontIsSetup = false;
        uint32_t j;
        gfxGlyphExtents *extents = font->GetOrCreateGlyphExtents(mAppUnitsPerDevUnit);
  
        for (j = start; j < end; ++j) {
            const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[j];
            if (glyphData->IsSimpleGlyph()) {
                
                
                if (needsGlyphExtents) {
                    uint32_t glyphIndex = glyphData->GetSimpleGlyph();
                    if (!extents->IsGlyphKnown(glyphIndex)) {
                        if (!fontIsSetup) {
                            if (!font->SetupCairoFont(aRefContext)) {
                                NS_WARNING("failed to set up font for glyph extents");
                                break;
                            }
                            fontIsSetup = true;
                        }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
                        ++gGlyphExtentsSetupEagerSimple;
#endif
                        font->SetupGlyphExtents(aRefContext, glyphIndex, false, extents);
                    }
                }
            } else if (!glyphData->IsMissing()) {
                uint32_t glyphCount = glyphData->GetGlyphCount();
                if (glyphCount == 0) {
                    continue;
                }
                const gfxTextRun::DetailedGlyph *details = GetDetailedGlyphs(j);
                if (!details) {
                    continue;
                }
                for (uint32_t k = 0; k < glyphCount; ++k, ++details) {
                    uint32_t glyphIndex = details->mGlyphID;
                    if (!extents->IsGlyphKnownWithTightExtents(glyphIndex)) {
                        if (!fontIsSetup) {
                            if (!font->SetupCairoFont(aRefContext)) {
                                NS_WARNING("failed to set up font for glyph extents");
                                break;
                            }
                            fontIsSetup = true;
                        }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
                        ++gGlyphExtentsSetupEagerTight;
#endif
                        font->SetupGlyphExtents(aRefContext, glyphIndex, true, extents);
                    }
                }
            }
        }
    }
}


gfxTextRun::ClusterIterator::ClusterIterator(gfxTextRun *aTextRun)
    : mTextRun(aTextRun), mCurrentChar(uint32_t(-1))
{
}

void
gfxTextRun::ClusterIterator::Reset()
{
    mCurrentChar = uint32_t(-1);
}

bool
gfxTextRun::ClusterIterator::NextCluster()
{
    uint32_t len = mTextRun->GetLength();
    while (++mCurrentChar < len) {
        if (mTextRun->IsClusterStart(mCurrentChar)) {
            return true;
        }
    }

    mCurrentChar = uint32_t(-1);
    return false;
}

uint32_t
gfxTextRun::ClusterIterator::ClusterLength() const
{
    if (mCurrentChar == uint32_t(-1)) {
        return 0;
    }

    uint32_t i = mCurrentChar,
             len = mTextRun->GetLength();
    while (++i < len) {
        if (mTextRun->IsClusterStart(i)) {
            break;
        }
    }

    return i - mCurrentChar;
}

gfxFloat
gfxTextRun::ClusterIterator::ClusterAdvance(PropertyProvider *aProvider) const
{
    if (mCurrentChar == uint32_t(-1)) {
        return 0;
    }

    return mTextRun->GetAdvanceWidth(mCurrentChar, ClusterLength(), aProvider);
}

size_t
gfxTextRun::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
    
    
    size_t total = mGlyphRuns.SizeOfExcludingThis(aMallocSizeOf);

    if (mDetailedGlyphs) {
        total += mDetailedGlyphs->SizeOfIncludingThis(aMallocSizeOf);
    }

    return total;
}

size_t
gfxTextRun::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf)
{
    return aMallocSizeOf(this) + SizeOfExcludingThis(aMallocSizeOf);
}


#ifdef DEBUG
void
gfxTextRun::Dump(FILE* aOutput) {
    if (!aOutput) {
        aOutput = stdout;
    }

    uint32_t i;
    fputc('[', aOutput);
    for (i = 0; i < mGlyphRuns.Length(); ++i) {
        if (i > 0) {
            fputc(',', aOutput);
        }
        gfxFont* font = mGlyphRuns[i].mFont;
        const gfxFontStyle* style = font->GetStyle();
        NS_ConvertUTF16toUTF8 fontName(font->GetName());
        nsAutoCString lang;
        style->language->ToUTF8String(lang);
        fprintf(aOutput, "%d: %s %f/%d/%d/%s", mGlyphRuns[i].mCharacterOffset,
                fontName.get(), style->size,
                style->weight, style->style, lang.get());
    }
    fputc(']', aOutput);
}
#endif
