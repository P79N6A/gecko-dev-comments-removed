







































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "nsIPrefService.h"
#include "nsServiceManagerUtils.h"
#include "nsReadableUtils.h"
#include "nsExpirationTracker.h"
#include "nsILanguageAtomService.h"

#include "gfxFont.h"
#include "gfxPlatform.h"
#include "gfxAtoms.h"

#include "prtypes.h"
#include "gfxTypes.h"
#include "gfxContext.h"
#include "gfxFontMissingGlyphs.h"
#include "gfxUserFontSet.h"
#include "gfxPlatformFontList.h"
#include "gfxScriptItemizer.h"
#include "gfxUnicodeProperties.h"
#include "nsMathUtils.h"
#include "nsBidiUtils.h"
#include "nsUnicodeRange.h"
#include "nsCompressedCharMap.h"

#include "cairo.h"
#include "gfxFontTest.h"

#include "harfbuzz/hb-blob.h"

#include "nsCRT.h"

using namespace mozilla;

gfxFontCache *gfxFontCache::gGlobalCache = nsnull;

#ifdef DEBUG_roc
#define DEBUG_TEXT_RUN_STORAGE_METRICS
#endif

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
static PRUint32 gTextRunStorageHighWaterMark = 0;
static PRUint32 gTextRunStorage = 0;
static PRUint32 gFontCount = 0;
static PRUint32 gGlyphExtentsCount = 0;
static PRUint32 gGlyphExtentsWidthsTotalSize = 0;
static PRUint32 gGlyphExtentsSetupEagerSimple = 0;
static PRUint32 gGlyphExtentsSetupEagerTight = 0;
static PRUint32 gGlyphExtentsSetupLazyTight = 0;
static PRUint32 gGlyphExtentsSetupFallBackToTight = 0;
#endif

gfxFontEntry::~gfxFontEntry() 
{
    if (mUserFontData) {
        delete mUserFontData;
    }
}

PRBool gfxFontEntry::TestCharacterMap(PRUint32 aCh)
{
    if (!mCmapInitialized) {
        ReadCMAP();
    }
    return mCharacterMap.test(aCh);
}

nsresult gfxFontEntry::InitializeUVSMap()
{
    
    
    if (!mCmapInitialized) {
        ReadCMAP();
    }

    if (!mUVSOffset) {
        return NS_ERROR_FAILURE;
    }

    if (!mUVSData) {
        const PRUint32 kCmapTag = TRUETYPE_TAG('c','m','a','p');
        AutoFallibleTArray<PRUint8,16384> buffer;
        if (GetFontTable(kCmapTag, buffer) != NS_OK) {
            mUVSOffset = 0; 
            return NS_ERROR_FAILURE;
        }

        PRUint8* uvsData;
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

PRUint16 gfxFontEntry::GetUVSGlyph(PRUint32 aCh, PRUint32 aVS)
{
    InitializeUVSMap();

    if (mUVSData) {
        return gfxFontUtils::MapUVSToGlyphFormat14(mUVSData, aCh, aVS);
    }

    return 0;
}

nsresult gfxFontEntry::ReadCMAP()
{
    mCmapInitialized = PR_TRUE;
    return NS_OK;
}

const nsString& gfxFontEntry::FamilyName() const
{
    NS_ASSERTION(mFamily, "gfxFontEntry is not a member of a family");
    return mFamily->Name();
}

already_AddRefed<gfxFont>
gfxFontEntry::FindOrMakeFont(const gfxFontStyle *aStyle, PRBool aNeedsBold)
{
    
    nsRefPtr<gfxFont> font = gfxFontCache::GetCache()->Lookup(this, aStyle);

    if (!font) {
        gfxFont *newFont = CreateFontInstance(aStyle, aNeedsBold);
        if (!newFont)
            return nsnull;
        if (!newFont->Valid()) {
            delete newFont;
            return nsnull;
        }
        font = newFont;
        gfxFontCache::GetCache()->AddNew(font);
    }
    gfxFont *f = nsnull;
    font.swap(f);
    return f;
}







class gfxFontEntry::FontTableBlobData {
public:
    
    
    
    FontTableBlobData(FallibleTArray<PRUint8>& aBuffer,
                      FontTableHashEntry *aHashEntry)
        : mHashEntry(aHashEntry), mHashtable()
    {
        MOZ_COUNT_CTOR(FontTableBlobData);
        mTableData.SwapElements(aBuffer);
    }

    ~FontTableBlobData() {
        MOZ_COUNT_DTOR(FontTableBlobData);
        if (mHashEntry) {
            if (mHashtable) {
                mHashtable->RemoveEntry(mHashEntry->GetKey());
            } else {
                mHashEntry->Clear();
            }
        }
    }

    
    const char *GetTable() const
    {
        return reinterpret_cast<const char*>(mTableData.Elements());
    }
    PRUint32 GetTableLength() const { return mTableData.Length(); }

    
    
    void ManageHashEntry(nsTHashtable<FontTableHashEntry> *aHashtable)
    {
        mHashtable = aHashtable;
    }

    
    
    void ForgetHashEntry()
    {
        mHashEntry = nsnull;
    }

private:
    
    FallibleTArray<PRUint8> mTableData;
    
    FontTableHashEntry *mHashEntry;
    
    nsTHashtable<FontTableHashEntry> *mHashtable;

    
    FontTableBlobData(const FontTableBlobData&);
};

void
gfxFontEntry::FontTableHashEntry::SaveTable(FallibleTArray<PRUint8>& aTable)
{
    Clear();
    
    FontTableBlobData *data = new FontTableBlobData(aTable, nsnull);
    mBlob = hb_blob_create(data->GetTable(), data->GetTableLength(),
                           HB_MEMORY_MODE_READONLY,
                           DeleteFontTableBlobData, data);    
}

hb_blob_t *
gfxFontEntry::FontTableHashEntry::
ShareTableAndGetBlob(FallibleTArray<PRUint8>& aTable,
                     nsTHashtable<FontTableHashEntry> *aHashtable)
{
    Clear();
    
    mSharedBlobData = new FontTableBlobData(aTable, this);
    mBlob = hb_blob_create(mSharedBlobData->GetTable(),
                           mSharedBlobData->GetTableLength(),
                           HB_MEMORY_MODE_READONLY,
                           DeleteFontTableBlobData, mSharedBlobData);
    if (!mSharedBlobData) {
        
        
        
        return hb_blob_reference(mBlob);
    }

    
    
    mSharedBlobData->ManageHashEntry(aHashtable);
    return mBlob;
}

void
gfxFontEntry::FontTableHashEntry::Clear()
{
    
    
    
    if (mSharedBlobData) {
        mSharedBlobData->ForgetHashEntry();
        mSharedBlobData = nsnull;
    } else if (mBlob) {
        hb_blob_destroy(mBlob);
    }
    mBlob = nsnull;
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

PRBool
gfxFontEntry::GetExistingFontTable(PRUint32 aTag, hb_blob_t **aBlob)
{
    if (!mFontTableCache.IsInitialized()) {
        
        
        mFontTableCache.Init(10);
    }

    FontTableHashEntry *entry = mFontTableCache.GetEntry(aTag);
    if (!entry) {
        return PR_FALSE;
    }

    *aBlob = entry->GetBlob();
    return PR_TRUE;
}

hb_blob_t *
gfxFontEntry::ShareFontTableAndGetBlob(PRUint32 aTag,
                                       FallibleTArray<PRUint8>* aBuffer)
{
    if (NS_UNLIKELY(!mFontTableCache.IsInitialized())) {
        
        
        mFontTableCache.Init(10);
    }

    FontTableHashEntry *entry = mFontTableCache.PutEntry(aTag);
    if (NS_UNLIKELY(!entry)) { 
        return nsnull;
    }

    if (!aBuffer) {
        
        entry->Clear();
        return nsnull;
    }

    return entry->ShareTableAndGetBlob(*aBuffer, &mFontTableCache);
}

void
gfxFontEntry::PreloadFontTable(PRUint32 aTag, FallibleTArray<PRUint8>& aTable)
{
    if (!mFontTableCache.IsInitialized()) {
        
        
        
        mFontTableCache.Init(3);
    }

    FontTableHashEntry *entry = mFontTableCache.PutEntry(aTag);
    if (NS_UNLIKELY(!entry)) { 
        return;
    }

    
    entry->SaveTable(aTable);
}









class FontEntryStandardFaceComparator {
  public:
    PRBool Equals(const nsRefPtr<gfxFontEntry>& a, const nsRefPtr<gfxFontEntry>& b) const {
        return a->mStandardFace == b->mStandardFace;
    }
    PRBool LessThan(const nsRefPtr<gfxFontEntry>& a, const nsRefPtr<gfxFontEntry>& b) const {
        return (a->mStandardFace == PR_FALSE && b->mStandardFace == PR_TRUE);
    }
};

void
gfxFontFamily::SortAvailableFonts()
{
    mAvailableFonts.Sort(FontEntryStandardFaceComparator());
}

PRBool
gfxFontFamily::HasOtherFamilyNames()
{
    
    if (!mOtherFamilyNamesInitialized) {
        ReadOtherFamilyNames(gfxPlatformFontList::PlatformFontList());  
    }
    return mHasOtherFamilyNames;
}

gfxFontEntry*
gfxFontFamily::FindFontForStyle(const gfxFontStyle& aFontStyle, 
                                PRBool& aNeedsSyntheticBold)
{
    if (!mHasStyles)
        FindStyleVariations(); 

    NS_ASSERTION(mAvailableFonts.Length() > 0, "font family with no faces!");

    aNeedsSyntheticBold = PR_FALSE;

    PRInt8 baseWeight = aFontStyle.ComputeWeight();
    PRBool wantBold = baseWeight >= 6;

    
    if (mAvailableFonts.Length() == 1) {
        gfxFontEntry *fe = mAvailableFonts[0];
        aNeedsSyntheticBold = wantBold && !fe->IsBold();
        return fe;
    }

    PRBool wantItalic = (aFontStyle.style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) != 0;

    
    
    
    
    
    

    if (mIsSimpleFamily) {
        
        
        
        PRUint8 faceIndex = (wantItalic ? kItalicMask : 0) |
                            (wantBold ? kBoldMask : 0);

        
        gfxFontEntry *fe = mAvailableFonts[faceIndex];
        if (fe) {
            
            return fe;
        }

        
        static const PRUint8 simpleFallbacks[4][3] = {
            { kBoldFaceIndex, kItalicFaceIndex, kBoldItalicFaceIndex },   
            { kRegularFaceIndex, kBoldItalicFaceIndex, kItalicFaceIndex },
            { kBoldItalicFaceIndex, kRegularFaceIndex, kBoldFaceIndex },  
            { kItalicFaceIndex, kBoldFaceIndex, kRegularFaceIndex }       
        };
        const PRUint8 *order = simpleFallbacks[faceIndex];

        for (PRUint8 trial = 0; trial < 3; ++trial) {
            
            fe = mAvailableFonts[order[trial]];
            if (fe) {
                aNeedsSyntheticBold = wantBold && !fe->IsBold();
                return fe;
            }
        }

        
        NS_NOTREACHED("no face found in simple font family!");
        return nsnull;
    }

    
    
    
    

    gfxFontEntry *weightList[10] = { 0 };
    PRBool foundWeights = FindWeightsForStyle(weightList, wantItalic, aFontStyle.stretch);
    if (!foundWeights) {
        return nsnull;
    }

    
    PRInt8 matchBaseWeight = 0;
    PRInt8 i = baseWeight;

    
    
    if (baseWeight == 4 && !weightList[4]) {
        i = 5; 
    }

    
    PRInt8 direction = (baseWeight > 5) ? 1 : -1;
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
        aNeedsSyntheticBold = PR_TRUE;
    }

    return matchFE;
}

void
gfxFontFamily::CheckForSimpleFamily()
{
    if (mAvailableFonts.Length() > 4 || mAvailableFonts.Length() == 0) {
        return; 
                
    }

    PRInt16 firstStretch = mAvailableFonts[0]->Stretch();

    gfxFontEntry *faces[4] = { 0 };
    for (PRUint8 i = 0; i < mAvailableFonts.Length(); ++i) {
        gfxFontEntry *fe = mAvailableFonts[i];
        if (fe->Stretch() != firstStretch) {
            return; 
        }
        PRUint8 faceIndex = (fe->IsItalic() ? kItalicMask : 0) |
                            (fe->Weight() >= 600 ? kBoldMask : 0);
        if (faces[faceIndex]) {
            return; 
        }
        faces[faceIndex] = fe;
    }

    
    
    mAvailableFonts.SetLength(4);
    for (PRUint8 i = 0; i < 4; ++i) {
        if (mAvailableFonts[i].get() != faces[i]) {
            mAvailableFonts[i].swap(faces[i]);
        }
    }

    mIsSimpleFamily = PR_TRUE;
}

static inline PRUint32
StyleDistance(gfxFontEntry *aFontEntry,
              PRBool anItalic, PRInt16 aStretch)
{
    
    
    

    
    
    

    return (aFontEntry->IsItalic() != anItalic ? 1 : 0) +
           (aFontEntry->mStretch != aStretch ? 10 : 0);
}

PRBool
gfxFontFamily::FindWeightsForStyle(gfxFontEntry* aFontsForWeights[],
                                   PRBool anItalic, PRInt16 aStretch)
{
    PRUint32 foundWeights = 0;
    PRUint32 bestMatchDistance = 0xffffffff;

    for (PRUint32 i = 0; i < mAvailableFonts.Length(); i++) {
        
        
        gfxFontEntry *fe = mAvailableFonts[i];
        PRUint32 distance = StyleDistance(fe, anItalic, aStretch);
        if (distance <= bestMatchDistance) {
            PRInt8 wt = fe->mWeight / 100;
            NS_ASSERTION(wt >= 1 && wt < 10, "invalid weight in fontEntry");
            if (!aFontsForWeights[wt]) {
                
                aFontsForWeights[wt] = fe;
                ++foundWeights;
            } else {
                PRUint32 prevDistance = StyleDistance(aFontsForWeights[wt], anItalic, aStretch);
                if (prevDistance >= distance) {
                    
                    aFontsForWeights[wt] = fe;
                }
            }
            bestMatchDistance = distance;
        }
    }

    NS_ASSERTION(foundWeights > 0, "Font family containing no faces?");

    if (foundWeights == 1) {
        
        return PR_TRUE;
    }

    
    
    
    for (PRUint32 i = 0; i < 10; ++i) {
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


void
gfxFontFamily::FindFontForChar(FontSearch *aMatchData)
{
    if (!mHasStyles)
        FindStyleVariations();

    
    
    

    
    PRUint32 numFonts = mAvailableFonts.Length();
    for (PRUint32 i = 0; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i];

        
        if (!fe || fe->SkipDuringSystemFallback())
            continue;

        PRInt32 rank = 0;

        if (fe->TestCharacterMap(aMatchData->mCh)) {
            rank += 20;
            aMatchData->mCount++;
#ifdef PR_LOGGING
            PRLogModuleInfo *log = gfxPlatform::GetLog(eGfxLog_textrun);
        
            if (NS_UNLIKELY(log)) {
                PRUint32 charRange = gfxFontUtils::CharRangeBit(aMatchData->mCh);
                PRUint32 unicodeRange = FindCharUnicodeRange(aMatchData->mCh);
                PRUint32 hbscript = gfxUnicodeProperties::GetScriptCode(aMatchData->mCh);
                PR_LOG(log, PR_LOG_DEBUG,\
                       ("(textrun-systemfallback-fonts) char: u+%6.6x "
                        "char-range: %d unicode-range: %d script: %d match: [%s]\n",
                        aMatchData->mCh,
                        charRange, unicodeRange, hbscript,
                        NS_ConvertUTF16toUTF8(fe->Name()).get()));
            }
#endif
        }

        
        if (rank == 0)
            continue;
            
        
        

        if (aMatchData->mFontToMatch) { 
            const gfxFontStyle *style = aMatchData->mFontToMatch->GetStyle();
            
            
            PRBool wantItalic =
                ((style->style & (FONT_STYLE_ITALIC | FONT_STYLE_OBLIQUE)) != 0);
            if (fe->IsItalic() == wantItalic) {
                rank += 5;
            }
            
            
            PRInt32 targetWeight = style->ComputeWeight() * 100;

            PRInt32 entryWeight = fe->Weight();
            if (entryWeight == targetWeight) {
                rank += 5;
            } else {
                PRUint32 diffWeight = abs(entryWeight - targetWeight);
                if (diffWeight <= 100)  
                    rank += 2;
            }
        } else {
            
            if (!fe->IsItalic()) {
                rank += 3;
            }
            if (!fe->IsBold()) {
                rank += 2;
            }
        }
        
        
        
        if (rank > aMatchData->mMatchRank
            || (rank == aMatchData->mMatchRank &&
                Compare(fe->Name(), aMatchData->mBestMatch->Name()) > 0)) 
        {
            aMatchData->mBestMatch = fe;
            aMatchData->mMatchRank = rank;
        }
    }
}


PRBool
gfxFontFamily::ReadOtherFamilyNamesForFace(gfxPlatformFontList *aPlatformFontList,
                                           FallibleTArray<PRUint8>& aNameTable,
                                           PRBool useFullName)
{
    const PRUint8 *nameData = aNameTable.Elements();
    PRUint32 dataLength = aNameTable.Length();
    const gfxFontUtils::NameHeader *nameHeader =
        reinterpret_cast<const gfxFontUtils::NameHeader*>(nameData);

    PRUint32 nameCount = nameHeader->count;
    if (nameCount * sizeof(gfxFontUtils::NameRecord) > dataLength) {
        NS_WARNING("invalid font (name records)");
        return PR_FALSE;
    }
    
    const gfxFontUtils::NameRecord *nameRecord =
        reinterpret_cast<const gfxFontUtils::NameRecord*>(nameData + sizeof(gfxFontUtils::NameHeader));
    PRUint32 stringsBase = PRUint32(nameHeader->stringOffset);

    PRBool foundNames = PR_FALSE;
    for (PRUint32 i = 0; i < nameCount; i++, nameRecord++) {
        PRUint32 nameLen = nameRecord->length;
        PRUint32 nameOff = nameRecord->offset;  

        if (stringsBase + nameOff + nameLen > dataLength) {
            NS_WARNING("invalid font (name table strings)");
            return PR_FALSE;
        }

        PRUint16 nameID = nameRecord->nameID;
        if ((useFullName && nameID == gfxFontUtils::NAME_ID_FULL) ||
            (!useFullName && (nameID == gfxFontUtils::NAME_ID_FAMILY ||
                              nameID == gfxFontUtils::NAME_ID_PREFERRED_FAMILY))) {
            nsAutoString otherFamilyName;
            PRBool ok = gfxFontUtils::DecodeFontName(nameData + stringsBase + nameOff,
                                                     nameLen,
                                                     PRUint32(nameRecord->platformID),
                                                     PRUint32(nameRecord->encodingID),
                                                     PRUint32(nameRecord->languageID),
                                                     otherFamilyName);
            
            if (ok && otherFamilyName != mName) {
                aPlatformFontList->AddOtherFamilyName(this, otherFamilyName);
                foundNames = PR_TRUE;
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
    mOtherFamilyNamesInitialized = PR_TRUE;

    FindStyleVariations();

    
    PRUint32 i, numFonts = mAvailableFonts.Length();
    const PRUint32 kNAME = TRUETYPE_TAG('n','a','m','e');
    AutoFallibleTArray<PRUint8,8192> buffer;

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
                             PRBool aNeedFullnamePostscriptNames)
{
    
    if (mOtherFamilyNamesInitialized &&
        (mFaceNamesInitialized || !aNeedFullnamePostscriptNames))
        return;

    FindStyleVariations();

    PRUint32 i, numFonts = mAvailableFonts.Length();
    const PRUint32 kNAME = TRUETYPE_TAG('n','a','m','e');
    AutoFallibleTArray<PRUint8,8192> buffer;
    nsAutoString fullname, psname;

    PRBool firstTime = PR_TRUE, readAllFaces = PR_FALSE;
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
           PRBool foundOtherName = ReadOtherFamilyNamesForFace(aPlatformFontList,
                                                               buffer);

           
           
           if (firstTime && foundOtherName) {
               mHasOtherFamilyNames = PR_TRUE;
               readAllFaces = PR_TRUE;
           }
           firstTime = PR_FALSE;
       }

       
       if (!readAllFaces && !aNeedFullnamePostscriptNames)
           break;
    }

    mFaceNamesInitialized = PR_TRUE;
    mOtherFamilyNamesInitialized = PR_TRUE;
}


gfxFontEntry*
gfxFontFamily::FindFont(const nsAString& aPostscriptName)
{
    
    PRUint32 numFonts = mAvailableFonts.Length();
    for (PRUint32 i = 0; i < numFonts; i++) {
        gfxFontEntry *fe = mAvailableFonts[i].get();
        if (fe && fe->Name() == aPostscriptName)
            return fe;
    }
    return nsnull;
}


nsresult
gfxFontCache::Init()
{
    NS_ASSERTION(!gGlobalCache, "Where did this come from?");
    gGlobalCache = new gfxFontCache();
    return gGlobalCache ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
gfxFontCache::Shutdown()
{
    delete gGlobalCache;
    gGlobalCache = nsnull;

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

PRBool
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
    if (!entry)
        return nsnull;

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
    RemoveObject(aFont);
    DestroyFont(aFont);
}

void
gfxFontCache::DestroyFont(gfxFont *aFont)
{
    Key key(aFont->GetFontEntry(), aFont->GetStyle());
    HashEntry *entry = mFonts.GetEntry(key);
    if (entry && entry->mFont == aFont)
        mFonts.RemoveEntry(key);
    NS_ASSERTION(aFont->GetRefCount() == 0,
                 "Destroying with non-zero ref count!");
    delete aFont;
}

void
gfxFont::RunMetrics::CombineWith(const RunMetrics& aOther, PRBool aOtherIsOnLeft)
{
    mAscent = PR_MAX(mAscent, aOther.mAscent);
    mDescent = PR_MAX(mDescent, aOther.mDescent);
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
                 AntialiasOption anAAOption) :
    mFontEntry(aFontEntry), mIsValid(PR_TRUE),
    mStyle(*aFontStyle),
    mAdjustedSize(0.0),
    mFUnitsConvFactor(0.0f),
    mSyntheticBoldOffset(0),
    mAntialiasOption(anAAOption),
    mPlatformShaper(nsnull),
    mHarfBuzzShaper(nsnull)
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    ++gFontCount;
#endif
}

gfxFont::~gfxFont()
{
    PRUint32 i;
    
    
    
    for (i = 0; i < mGlyphExtentsArray.Length(); ++i) {
        delete mGlyphExtentsArray[i];
    }
}

hb_blob_t *
gfxFont::GetFontTable(PRUint32 aTag) {
    hb_blob_t *blob;
    if (mFontEntry->GetExistingFontTable(aTag, &blob))
        return blob;

    FallibleTArray<PRUint8> buffer;
    PRBool haveTable = NS_SUCCEEDED(mFontEntry->GetFontTable(aTag, buffer));

    return mFontEntry->ShareFontTableAndGetBlob(aTag,
                                                haveTable ? &buffer : nsnull);
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

    void Flush(cairo_t *aCR, PRBool aDrawToPath, PRBool aReverse,
               PRBool aFinish = PR_FALSE) {
        
        
        if (!aFinish && mNumGlyphs + 2 <= GLYPH_BUFFER_SIZE)
            return;

        if (aReverse) {
            for (PRUint32 i = 0; i < mNumGlyphs/2; ++i) {
                cairo_glyph_t tmp = mGlyphBuffer[i];
                mGlyphBuffer[i] = mGlyphBuffer[mNumGlyphs - 1 - i];
                mGlyphBuffer[mNumGlyphs - 1 - i] = tmp;
            }
        }
        if (aDrawToPath)
            cairo_glyph_path(aCR, mGlyphBuffer, mNumGlyphs);
        else
            cairo_show_glyphs(aCR, mGlyphBuffer, mNumGlyphs);

        mNumGlyphs = 0;
    }
#undef GLYPH_BUFFER_SIZE
};

void
gfxFont::Draw(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
              gfxContext *aContext, PRBool aDrawToPath, gfxPoint *aPt,
              Spacing *aSpacing)
{
    if (aStart >= aEnd)
        return;

    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    const double devUnitsPerAppUnit = 1.0/double(appUnitsPerDevUnit);
    PRBool isRTL = aTextRun->IsRightToLeft();
    double direction = aTextRun->GetDirection();
    
    double synBoldDevUnitOffsetAppUnits =
      direction * (double) mSyntheticBoldOffset * appUnitsPerDevUnit;
    PRUint32 i;
    
    double x = aPt->x;
    double y = aPt->y;

    PRBool success = SetupCairoFont(aContext);
    if (NS_UNLIKELY(!success))
        return;

    GlyphBuffer glyphs;
    cairo_glyph_t *glyph;
    cairo_t *cr = aContext->GetCairo();

    if (aSpacing) {
        x += direction*aSpacing[0].mBefore;
    }
    for (i = aStart; i < aEnd; ++i) {
        const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
        if (glyphData->IsSimpleGlyph()) {
            glyph = glyphs.AppendGlyph();
            glyph->index = glyphData->GetSimpleGlyph();
            double advance = glyphData->GetSimpleAdvance();
            
            
            
            
            
            double glyphX;
            if (isRTL) {
                x -= advance;
                glyphX = x;
            } else {
                glyphX = x;
                x += advance;
            }
            glyph->x = ToDeviceUnits(glyphX, devUnitsPerAppUnit);
            glyph->y = ToDeviceUnits(y, devUnitsPerAppUnit);
            
            
            if (mSyntheticBoldOffset) {
                cairo_glyph_t *doubleglyph;
                doubleglyph = glyphs.AppendGlyph();
                doubleglyph->index = glyph->index;
                doubleglyph->x =
                  ToDeviceUnits(glyphX + synBoldDevUnitOffsetAppUnits,
                                devUnitsPerAppUnit);
                doubleglyph->y = glyph->y;
            }
            
            glyphs.Flush(cr, aDrawToPath, isRTL);
        } else {
            PRUint32 glyphCount = glyphData->GetGlyphCount();
            if (glyphCount > 0) {
                const gfxTextRun::DetailedGlyph *details =
                    aTextRun->GetDetailedGlyphs(i);
                NS_ASSERTION(details, "detailedGlyph should not be missing!");
                for (PRUint32 j = 0; j < glyphCount; ++j, ++details) {
                    double advance = details->mAdvance;
                    if (glyphData->IsMissing()) {
                        
                        
                        if (!aDrawToPath && advance > 0) {
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
                                                                   details->mGlyphID);
                        }
                    } else {
                        glyph = glyphs.AppendGlyph();
                        glyph->index = details->mGlyphID;
                        double glyphX = x + details->mXOffset;
                        if (isRTL) {
                            glyphX -= advance;
                        }
                        glyph->x = ToDeviceUnits(glyphX, devUnitsPerAppUnit);
                        glyph->y = ToDeviceUnits(y + details->mYOffset, devUnitsPerAppUnit);

                        
                        if (mSyntheticBoldOffset) {
                            cairo_glyph_t *doubleglyph;
                            doubleglyph = glyphs.AppendGlyph();
                            doubleglyph->index = glyph->index;
                            doubleglyph->x =
                                ToDeviceUnits(glyphX + synBoldDevUnitOffsetAppUnits,
                                              devUnitsPerAppUnit);
                            doubleglyph->y = glyph->y;
                        }

                        glyphs.Flush(cr, aDrawToPath, isRTL);
                    }
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

    if (gfxFontTestStore::CurrentStore()) {
        



        gfxFontTestStore::CurrentStore()->AddItem(GetName(),
                                                  glyphs.mGlyphBuffer,
                                                  glyphs.mNumGlyphs);
    }

    
    glyphs.Flush(cr, aDrawToPath, isRTL, PR_TRUE);

    *aPt = gfxPoint(x, y);
}

static void
UnionRange(gfxFloat aX, gfxFloat* aDestMin, gfxFloat* aDestMax)
{
    *aDestMin = PR_MIN(*aDestMin, aX);
    *aDestMax = PR_MAX(*aDestMax, aX);
}




static PRBool
NeedsGlyphExtents(gfxFont *aFont, gfxTextRun *aTextRun)
{
    return (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX) ||
        aFont->GetFontEntry()->IsUserFont();
}

static PRBool
NeedsGlyphExtents(gfxTextRun *aTextRun)
{
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX)
        return PR_TRUE;
    PRUint32 numRuns;
    const gfxTextRun::GlyphRun *glyphRuns = aTextRun->GetGlyphRuns(&numRuns);
    for (PRUint32 i = 0; i < numRuns; ++i) {
        if (glyphRuns[i].mFont->GetFontEntry()->IsUserFont())
            return PR_TRUE;
    }
    return PR_FALSE;
}

gfxFont::RunMetrics
gfxFont::Measure(gfxTextRun *aTextRun,
                 PRUint32 aStart, PRUint32 aEnd,
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

    const PRUint32 appUnitsPerDevUnit = aTextRun->GetAppUnitsPerDevUnit();
    
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
    PRBool isRTL = aTextRun->IsRightToLeft();
    double direction = aTextRun->GetDirection();
    PRBool needsGlyphExtents = NeedsGlyphExtents(this, aTextRun);
    gfxGlyphExtents *extents =
        (aBoundingBoxType == LOOSE_INK_EXTENTS &&
            !needsGlyphExtents &&
            !aTextRun->HasDetailedGlyphs()) ? nsnull
        : GetOrCreateGlyphExtents(aTextRun->GetAppUnitsPerDevUnit());
    double x = 0;
    if (aSpacing) {
        x += direction*aSpacing[0].mBefore;
    }
    PRUint32 i;
    for (i = aStart; i < aEnd; ++i) {
        const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[i];
        if (glyphData->IsSimpleGlyph()) {
            double advance = glyphData->GetSimpleAdvance();
            
            
            if ((aBoundingBoxType != LOOSE_INK_EXTENTS || needsGlyphExtents) &&
                extents) {
                PRUint32 glyphIndex = glyphData->GetSimpleGlyph();
                PRUint16 extentsWidth = extents->GetContainedGlyphWidthAppUnits(glyphIndex);
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
            PRUint32 glyphCount = glyphData->GetGlyphCount();
            if (glyphCount > 0) {
                const gfxTextRun::DetailedGlyph *details =
                    aTextRun->GetDetailedGlyphs(i);
                NS_ASSERTION(details != nsnull,
                             "detaiedGlyph record should not be missing!");
                PRUint32 j;
                for (j = 0; j < glyphCount; ++j, ++details) {
                    PRUint32 glyphIndex = details->mGlyphID;
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

#define MAX_SHAPING_LENGTH  32760 // slightly less than 32K, trying to avoid
                                  

#define BACKTRACK_LIMIT  1024 // If we can't find a space or a cluster start
                              
                              
                              

PRBool
gfxFont::SplitAndInitTextRun(gfxContext *aContext,
                             gfxTextRun *aTextRun,
                             const PRUnichar *aString,
                             PRUint32 aRunStart,
                             PRUint32 aRunLength,
                             PRInt32 aRunScript)
{
    PRBool ok;

#ifdef PR_LOGGING
    PRLogModuleInfo *log = (mStyle.systemFont ?
                            gfxPlatform::GetLog(eGfxLog_textrunui) :
                            gfxPlatform::GetLog(eGfxLog_textrun));

    if (NS_UNLIKELY(log)) {
        nsCAutoString lang;
        mStyle.language->ToUTF8String(lang);
        PR_LOG(log, PR_LOG_DEBUG,\
               ("(%s-fontmatching) font: [%s] lang: %s script: %d len: %d "
                "TEXTRUN [%s] ENDTEXTRUN\n",
                (mStyle.systemFont ? "textrunui" : "textrun"),
                NS_ConvertUTF16toUTF8(GetName()).get(),
                lang.get(), aRunScript, aRunLength,
                NS_ConvertUTF16toUTF8(aString + aRunStart, aRunLength).get()));
    }
#endif

    do {
        
        
        
        
        
        
        
        

        PRUint32 thisRunLength;
        ok = PR_FALSE;

        if (aRunLength <= MAX_SHAPING_LENGTH) {
            thisRunLength = aRunLength;
        } else {
            
            PRUint32 offset = aRunStart + MAX_SHAPING_LENGTH;
            PRUint32 clusterStart = 0;
            while (offset > aRunStart + MAX_SHAPING_LENGTH - BACKTRACK_LIMIT) {
                if (aTextRun->IsClusterStart(offset)) {
                    if (!clusterStart) {
                        clusterStart = offset;
                    }
                    if (aString[offset] == ' ' || aString[offset - 1] == ' ') {
                        break;
                    }
                }
                --offset;
            }
            
            if (offset > MAX_SHAPING_LENGTH - BACKTRACK_LIMIT) {
                
                thisRunLength = offset - aRunStart;
            } else if (clusterStart != 0) {
                
                thisRunLength = clusterStart - aRunStart;
            } else {
                
                
                
                
                
                thisRunLength = MAX_SHAPING_LENGTH;
            }
        }

        ok = InitTextRun(aContext, aTextRun, aString,
                         aRunStart, thisRunLength, aRunScript);

        aRunStart += thisRunLength;
        aRunLength -= thisRunLength;
    } while (ok && aRunLength > 0);

    NS_WARN_IF_FALSE(ok, "shaper failed, expect scrambled or missing text");
    return ok;
}

PRBool
gfxFont::InitTextRun(gfxContext *aContext,
                     gfxTextRun *aTextRun,
                     const PRUnichar *aString,
                     PRUint32 aRunStart,
                     PRUint32 aRunLength,
                     PRInt32 aRunScript,
                     PRBool aPreferPlatformShaping)
{
    PRBool ok = PR_FALSE;

    if (mHarfBuzzShaper && !aPreferPlatformShaping) {
        if (gfxPlatform::GetPlatform()->UseHarfBuzzForScript(aRunScript)) {
            ok = mHarfBuzzShaper->InitTextRun(aContext, aTextRun, aString,
                                              aRunStart, aRunLength,
                                              aRunScript);
        }
    }

    if (!ok) {
        if (!mPlatformShaper) {
            CreatePlatformShaper();
            NS_ASSERTION(mPlatformShaper, "no platform shaper available!");
        }
        if (mPlatformShaper) {
            ok = mPlatformShaper->InitTextRun(aContext, aTextRun, aString,
                                              aRunStart, aRunLength,
                                              aRunScript);
        }
    }

    return ok;
}

gfxGlyphExtents *
gfxFont::GetOrCreateGlyphExtents(PRUint32 aAppUnitsPerDevUnit) {
    PRUint32 i;
    for (i = 0; i < mGlyphExtentsArray.Length(); ++i) {
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
gfxFont::SetupGlyphExtents(gfxContext *aContext, PRUint32 aGlyphID, PRBool aNeedTight,
                           gfxGlyphExtents *aExtents)
{
    gfxMatrix matrix = aContext->CurrentMatrix();
    aContext->IdentityMatrix();
    cairo_glyph_t glyph;
    glyph.index = aGlyphID;
    glyph.x = 0;
    glyph.y = 0;
    cairo_text_extents_t extents;
    cairo_glyph_extents(aContext->GetCairo(), &glyph, 1, &extents);
    aContext->SetMatrix(matrix);

    const Metrics& fontMetrics = GetMetrics();
    PRUint32 appUnitsPerDevUnit = aExtents->GetAppUnitsPerDevUnit();
    if (!aNeedTight && extents.x_bearing >= 0 &&
        extents.y_bearing >= -fontMetrics.maxAscent &&
        extents.height + extents.y_bearing <= fontMetrics.maxDescent) {
        PRUint32 appUnitsWidth =
            PRUint32(NS_ceil((extents.x_bearing + extents.width)*appUnitsPerDevUnit));
        if (appUnitsWidth < gfxGlyphExtents::INVALID_WIDTH) {
            aExtents->SetContainedGlyphWidthAppUnits(aGlyphID, PRUint16(appUnitsWidth));
            return;
        }
    }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    if (!aNeedTight) {
        ++gGlyphExtentsSetupFallBackToTight;
    }
#endif

    double d2a = appUnitsPerDevUnit;
    gfxRect bounds(extents.x_bearing*d2a, extents.y_bearing*d2a,
                   extents.width*d2a, extents.height*d2a);
    aExtents->SetTightGlyphExtents(aGlyphID, bounds);
}






PRBool
gfxFont::InitMetricsFromSfntTables(Metrics& aMetrics)
{
    mIsValid = PR_FALSE; 

    const PRUint32 kHeadTableTag = TRUETYPE_TAG('h','e','a','d');
    const PRUint32 kHheaTableTag = TRUETYPE_TAG('h','h','e','a');
    const PRUint32 kPostTableTag = TRUETYPE_TAG('p','o','s','t');
    const PRUint32 kOS_2TableTag = TRUETYPE_TAG('O','S','/','2');

    if (mFUnitsConvFactor == 0.0) {
        
        
        
        AutoFallibleTArray<PRUint8,sizeof(HeadTable)> headData;
        if (NS_FAILED(mFontEntry->GetFontTable(kHeadTableTag, headData)) ||
            headData.Length() < sizeof(HeadTable)) {
            return PR_FALSE; 
        }
        HeadTable *head = reinterpret_cast<HeadTable*>(headData.Elements());
        PRUint32 unitsPerEm = head->unitsPerEm;
        if (!unitsPerEm) {
            return PR_TRUE; 
        }
        mFUnitsConvFactor = mAdjustedSize / unitsPerEm;
    }

    
    AutoFallibleTArray<PRUint8,sizeof(HheaTable)> hheaData;
    if (NS_FAILED(mFontEntry->GetFontTable(kHheaTableTag, hheaData)) ||
        hheaData.Length() < sizeof(HheaTable)) {
        return PR_FALSE; 
    }
    HheaTable *hhea = reinterpret_cast<HheaTable*>(hheaData.Elements());

#define SET_UNSIGNED(field,src) aMetrics.field = PRUint16(src) * mFUnitsConvFactor
#define SET_SIGNED(field,src)   aMetrics.field = PRInt16(src) * mFUnitsConvFactor

    SET_UNSIGNED(maxAdvance, hhea->advanceWidthMax);
    SET_SIGNED(maxAscent, hhea->ascender);
    SET_SIGNED(maxDescent, -PRInt16(hhea->descender));
    SET_SIGNED(externalLeading, hhea->lineGap);

    
    AutoFallibleTArray<PRUint8,sizeof(PostTable)> postData;
    if (NS_FAILED(mFontEntry->GetFontTable(kPostTableTag, postData))) {
        return PR_TRUE; 
    }
    if (postData.Length() <
        offsetof(PostTable, underlineThickness) + sizeof(PRUint16)) {
        return PR_TRUE; 
    }
    PostTable *post = reinterpret_cast<PostTable*>(postData.Elements());

    SET_SIGNED(underlineOffset, post->underlinePosition);
    SET_UNSIGNED(underlineSize, post->underlineThickness);

    
    
    AutoFallibleTArray<PRUint8,sizeof(OS2Table)> os2data;
    if (NS_SUCCEEDED(mFontEntry->GetFontTable(kOS_2TableTag, os2data))) {
        OS2Table *os2 = reinterpret_cast<OS2Table*>(os2data.Elements());

        if (os2data.Length() >= offsetof(OS2Table, sxHeight) +
                                sizeof(PRInt16) &&
            PRUint16(os2->version) >= 2) {
            
            SET_SIGNED(xHeight, os2->sxHeight);
            
            aMetrics.xHeight = PR_ABS(aMetrics.xHeight);
        }
        
        if (os2data.Length() >= offsetof(OS2Table, yStrikeoutPosition) +
                                sizeof(PRInt16)) {
            SET_SIGNED(aveCharWidth, os2->xAvgCharWidth);
            SET_SIGNED(subscriptOffset, os2->ySubscriptYOffset);
            SET_SIGNED(superscriptOffset, os2->ySuperscriptYOffset);
            SET_SIGNED(strikeoutSize, os2->yStrikeoutSize);
            SET_SIGNED(strikeoutOffset, os2->yStrikeoutPosition);
        }
    }

    mIsValid = PR_TRUE;

    return PR_TRUE;
}

static double
RoundToNearestMultiple(double aValue, double aFraction)
{
    return floor(aValue/aFraction + 0.5) * aFraction;
}

void gfxFont::CalculateDerivedMetrics(Metrics& aMetrics)
{
    aMetrics.maxAscent =
        NS_ceil(RoundToNearestMultiple(aMetrics.maxAscent, 1/1024.0));
    aMetrics.maxDescent =
        NS_ceil(RoundToNearestMultiple(aMetrics.maxDescent, 1/1024.0));

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
gfxFont::SanitizeMetrics(gfxFont::Metrics *aMetrics, PRBool aIsBadUnderlineFont)
{
    
    
    if (mStyle.size == 0) {
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

    aMetrics->underlineSize = PR_MAX(1.0, aMetrics->underlineSize);
    aMetrics->strikeoutSize = PR_MAX(1.0, aMetrics->strikeoutSize);

    aMetrics->underlineOffset = PR_MIN(aMetrics->underlineOffset, -1.0);

    if (aMetrics->maxAscent < 1.0) {
        
        aMetrics->underlineSize = 0;
        aMetrics->underlineOffset = 0;
        aMetrics->strikeoutSize = 0;
        aMetrics->strikeoutOffset = 0;
        return;
    }

    







    if (!mStyle.systemFont && aIsBadUnderlineFont) {
        
        
        aMetrics->underlineOffset = PR_MIN(aMetrics->underlineOffset, -2.0);

        
        if (aMetrics->internalLeading + aMetrics->externalLeading > aMetrics->underlineSize) {
            aMetrics->underlineOffset = PR_MIN(aMetrics->underlineOffset, -aMetrics->emDescent);
        } else {
            aMetrics->underlineOffset = PR_MIN(aMetrics->underlineOffset,
                                               aMetrics->underlineSize - aMetrics->emDescent);
        }
    }
    
    
    else if (aMetrics->underlineSize - aMetrics->underlineOffset > aMetrics->maxDescent) {
        if (aMetrics->underlineSize > aMetrics->maxDescent)
            aMetrics->underlineSize = PR_MAX(aMetrics->maxDescent, 1.0);
        
        aMetrics->underlineOffset = aMetrics->underlineSize - aMetrics->maxDescent;
    }

    
    
    
    gfxFloat halfOfStrikeoutSize = NS_floor(aMetrics->strikeoutSize / 2.0 + 0.5);
    if (halfOfStrikeoutSize + aMetrics->strikeoutOffset > aMetrics->maxAscent) {
        if (aMetrics->strikeoutSize > aMetrics->maxAscent) {
            aMetrics->strikeoutSize = PR_MAX(aMetrics->maxAscent, 1.0);
            halfOfStrikeoutSize = NS_floor(aMetrics->strikeoutSize / 2.0 + 0.5);
        }
        gfxFloat ascent = NS_floor(aMetrics->maxAscent + 0.5);
        aMetrics->strikeoutOffset = PR_MAX(halfOfStrikeoutSize, ascent / 2.0);
    }

    
    if (aMetrics->underlineSize > aMetrics->maxAscent) {
        aMetrics->underlineSize = aMetrics->maxAscent;
    }
}

gfxGlyphExtents::~gfxGlyphExtents()
{
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    gGlyphExtentsWidthsTotalSize += mContainedGlyphWidths.ComputeSize();
    gGlyphExtentsCount++;
#endif
    MOZ_COUNT_DTOR(gfxGlyphExtents);
}

PRBool
gfxGlyphExtents::GetTightGlyphExtentsAppUnits(gfxFont *aFont,
    gfxContext *aContext, PRUint32 aGlyphID, gfxRect *aExtents)
{
    HashEntry *entry = mTightGlyphExtents.GetEntry(aGlyphID);
    if (!entry) {
        if (!aContext) {
            NS_WARNING("Could not get glyph extents (no aContext)");
            return PR_FALSE;
        }

        aFont->SetupCairoFont(aContext);
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
        ++gGlyphExtentsSetupLazyTight;
#endif
        aFont->SetupGlyphExtents(aContext, aGlyphID, PR_TRUE, this);
        entry = mTightGlyphExtents.GetEntry(aGlyphID);
        if (!entry) {
            NS_WARNING("Could not get glyph extents");
            return PR_FALSE;
        }
    }

    *aExtents = gfxRect(entry->x, entry->y, entry->width, entry->height);
    return PR_TRUE;
}

gfxGlyphExtents::GlyphWidths::~GlyphWidths()
{
    PRUint32 i;
    for (i = 0; i < mBlocks.Length(); ++i) {
        PtrBits bits = mBlocks[i];
        if (bits && !(bits & 0x1)) {
            delete[] reinterpret_cast<PRUint16 *>(bits);
        }
    }
}

#ifdef DEBUG
PRUint32
gfxGlyphExtents::GlyphWidths::ComputeSize()
{
    PRUint32 i;
    PRUint32 size = mBlocks.Capacity()*sizeof(PtrBits);
    for (i = 0; i < mBlocks.Length(); ++i) {
        PtrBits bits = mBlocks[i];
        if (bits && !(bits & 0x1)) {
            size += BLOCK_SIZE*sizeof(PRUint16);
        }
    }
    return size;
}
#endif

void
gfxGlyphExtents::GlyphWidths::Set(PRUint32 aGlyphID, PRUint16 aWidth)
{
    PRUint32 block = aGlyphID >> BLOCK_SIZE_BITS;
    PRUint32 len = mBlocks.Length();
    if (block >= len) {
        PtrBits *elems = mBlocks.AppendElements(block + 1 - len);
        if (!elems)
            return;
        memset(elems, 0, sizeof(PtrBits)*(block + 1 - len));
    }

    PtrBits bits = mBlocks[block];
    PRUint32 glyphOffset = aGlyphID & (BLOCK_SIZE - 1);
    if (!bits) {
        mBlocks[block] = MakeSingle(glyphOffset, aWidth);
        return;
    }

    PRUint16 *newBlock;
    if (bits & 0x1) {
        
        
        newBlock = new PRUint16[BLOCK_SIZE];
        if (!newBlock)
            return;
        PRUint32 i;
        for (i = 0; i < BLOCK_SIZE; ++i) {
            newBlock[i] = INVALID_WIDTH;
        }
        newBlock[GetGlyphOffset(bits)] = GetWidth(bits);
        mBlocks[block] = reinterpret_cast<PtrBits>(newBlock);
    } else {
        newBlock = reinterpret_cast<PRUint16 *>(bits);
    }
    newBlock[glyphOffset] = aWidth;
}

void
gfxGlyphExtents::SetTightGlyphExtents(PRUint32 aGlyphID, const gfxRect& aExtentsAppUnits)
{
    HashEntry *entry = mTightGlyphExtents.PutEntry(aGlyphID);
    if (!entry)
        return;
    entry->x = aExtentsAppUnits.X();
    entry->y = aExtentsAppUnits.Y();
    entry->width = aExtentsAppUnits.Width();
    entry->height = aExtentsAppUnits.Height();
}

gfxFontGroup::gfxFontGroup(const nsAString& aFamilies, const gfxFontStyle *aStyle, gfxUserFontSet *aUserFontSet)
    : mFamilies(aFamilies), mStyle(*aStyle), mUnderlineOffset(UNDERLINE_OFFSET_NOT_SET)
{
    mUserFontSet = nsnull;
    SetUserFontSet(aUserFontSet);

    mSkipDrawing = PR_FALSE;

    mPageLang = gfxPlatform::GetFontPrefLangFor(mStyle.language);
    BuildFontList();
}

void
gfxFontGroup::BuildFontList()
{


#if defined(XP_MACOSX) || (defined(XP_WIN) && !defined(WINCE)) || defined(ANDROID)
    ForEachFont(FindPlatformFont, this);

    if (mFonts.Length() == 0) {
        PRBool needsBold;
        gfxPlatformFontList *pfl = gfxPlatformFontList::PlatformFontList();
        gfxFontEntry *defaultFont = pfl->GetDefaultFont(&mStyle, needsBold);
        NS_ASSERTION(defaultFont, "invalid default font returned by GetDefaultFont");

        if (defaultFont) {
            nsRefPtr<gfxFont> font = defaultFont->FindOrMakeFont(&mStyle,
                                                                 needsBold);
            if (font) {
                mFonts.AppendElement(font);
            }
        }

        if (mFonts.Length() == 0) {
            
            
            
            
            
            nsAutoTArray<nsRefPtr<gfxFontFamily>,200> families;
            pfl->GetFontFamilyList(families);
            for (PRUint32 i = 0; i < families.Length(); ++i) {
                gfxFontEntry *fe = families[i]->FindFontForStyle(mStyle,
                                                                 needsBold);
                if (fe) {
                    nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&mStyle,
                                                                needsBold);
                    if (font) {
                        mFonts.AppendElement(font);
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
        for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
            gfxFont* font = mFonts[i];
            if (font->GetFontEntry()->mIsBadUnderlineFont) {
                gfxFloat first = mFonts[0]->GetMetrics().underlineOffset;
                gfxFloat bad = font->GetMetrics().underlineOffset;
                mUnderlineOffset = PR_MIN(first, bad);
                break;
            }
        }
    }
#endif
}

PRBool
gfxFontGroup::FindPlatformFont(const nsAString& aName,
                               const nsACString& aGenericName,
                               void *aClosure)
{
    gfxFontGroup *fontGroup = static_cast<gfxFontGroup*>(aClosure);
    const gfxFontStyle *fontStyle = fontGroup->GetStyle();

    PRBool needsBold;
    gfxFontEntry *fe = nsnull;

    
    
    
    
    PRBool foundFamily = PR_FALSE;
    gfxUserFontSet *fs = fontGroup->GetUserFontSet();
    if (fs) {
        
        
        
        PRBool waitForUserFont = PR_FALSE;
        fe = fs->FindFontEntry(aName, *fontStyle, foundFamily,
                               needsBold, waitForUserFont);
        if (!fe && waitForUserFont) {
            fontGroup->mSkipDrawing = PR_TRUE;
        }
    }

    
    if (!foundFamily) {
        fe = gfxPlatformFontList::PlatformFontList()->
            FindFontForFamily(aName, fontStyle, needsBold);
    }

    
    if (fe && !fontGroup->HasFont(fe)) {
        nsRefPtr<gfxFont> font = fe->FindOrMakeFont(fontStyle, needsBold);
        if (font) {
            fontGroup->mFonts.AppendElement(font);
        }
    }

    return PR_TRUE;
}

PRBool
gfxFontGroup::HasFont(const gfxFontEntry *aFontEntry)
{
    for (PRUint32 i = 0; i < mFonts.Length(); ++i) {
        if (mFonts.ElementAt(i)->GetFontEntry() == aFontEntry)
            return PR_TRUE;
    }
    return PR_FALSE;
}

gfxFontGroup::~gfxFontGroup() {
    mFonts.Clear();
    SetUserFontSet(nsnull);
}

gfxFontGroup *
gfxFontGroup::Copy(const gfxFontStyle *aStyle)
{
    return new gfxFontGroup(mFamilies, aStyle, mUserFontSet);
}

PRBool 
gfxFontGroup::IsInvalidChar(PRUnichar ch) {
    if (ch >= 32) {
        return ch == 0x0085 ||
            ((ch & 0xFF00) == 0x2000  &&
             (ch == 0x200B || ch == 0x2028 || ch == 0x2029 ||
              IS_BIDI_CONTROL_CHAR(ch)));
    }
    
    
    
    return ch == 0x0B || ch == '\t' || ch == '\r' || ch == '\n' || ch == '\f' ||
        (ch >= 0x1c && ch <= 0x1f);
}

PRBool
gfxFontGroup::ForEachFont(FontCreationCallback fc,
                          void *closure)
{
    return ForEachFontInternal(mFamilies, mStyle.language,
                               PR_TRUE, PR_TRUE, fc, closure);
}

PRBool
gfxFontGroup::ForEachFont(const nsAString& aFamilies,
                          nsIAtom *aLanguage,
                          FontCreationCallback fc,
                          void *closure)
{
    return ForEachFontInternal(aFamilies, aLanguage,
                               PR_FALSE, PR_TRUE, fc, closure);
}

struct ResolveData {
    ResolveData(gfxFontGroup::FontCreationCallback aCallback,
                nsACString& aGenericFamily,
                void *aClosure) :
        mCallback(aCallback),
        mGenericFamily(aGenericFamily),
        mClosure(aClosure) {
    }
    gfxFontGroup::FontCreationCallback mCallback;
    nsCString mGenericFamily;
    void *mClosure;
};

PRBool
gfxFontGroup::ForEachFontInternal(const nsAString& aFamilies,
                                  nsIAtom *aLanguage,
                                  PRBool aResolveGeneric,
                                  PRBool aResolveFontName,
                                  FontCreationCallback fc,
                                  void *closure)
{
    const PRUnichar kSingleQuote  = PRUnichar('\'');
    const PRUnichar kDoubleQuote  = PRUnichar('\"');
    const PRUnichar kComma        = PRUnichar(',');

    nsIAtom *groupAtom = nsnull;
    nsCAutoString groupString;
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
        groupAtom = gfxAtoms::x_unicode;
    }
    groupAtom->ToUTF8String(groupString);

    nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);

    nsPromiseFlatString families(aFamilies);
    const PRUnichar *p, *p_end;
    families.BeginReading(p);
    families.EndReading(p_end);
    nsAutoString family;
    nsCAutoString lcFamily;
    nsAutoString genericFamily;
    nsXPIDLCString value;

    while (p < p_end) {
        while (nsCRT::IsAsciiSpace(*p) || *p == kComma)
            if (++p == p_end)
                return PR_TRUE;

        PRBool generic;
        if (*p == kSingleQuote || *p == kDoubleQuote) {
            
            PRUnichar quoteMark = *p;
            if (++p == p_end)
                return PR_TRUE;
            const PRUnichar *nameStart = p;

            
            while (*p != quoteMark)
                if (++p == p_end)
                    return PR_TRUE;

            family = Substring(nameStart, p);
            generic = PR_FALSE;
            genericFamily.SetIsVoid(PR_TRUE);

            while (++p != p_end && *p != kComma)
                 ;

        } else {
            
            const PRUnichar *nameStart = p;
            while (++p != p_end && *p != kComma)
                 ;

            family = Substring(nameStart, p);
            family.CompressWhitespace(PR_FALSE, PR_TRUE);

            if (aResolveGeneric &&
                (family.LowerCaseEqualsLiteral("serif") ||
                 family.LowerCaseEqualsLiteral("sans-serif") ||
                 family.LowerCaseEqualsLiteral("monospace") ||
                 family.LowerCaseEqualsLiteral("cursive") ||
                 family.LowerCaseEqualsLiteral("fantasy")))
            {
                generic = PR_TRUE;

                ToLowerCase(NS_LossyConvertUTF16toASCII(family), lcFamily);

                nsCAutoString prefName("font.name.");
                prefName.Append(lcFamily);
                prefName.AppendLiteral(".");
                prefName.Append(groupString);

                
                
                nsresult rv = prefs->GetCharPref(prefName.get(), getter_Copies(value));
                if (NS_SUCCEEDED(rv)) {
                    CopyASCIItoUTF16(lcFamily, genericFamily);
                    CopyUTF8toUTF16(value, family);
                }
            } else {
                generic = PR_FALSE;
                genericFamily.SetIsVoid(PR_TRUE);
            }
        }

        if (generic) {
            ForEachFontInternal(family, groupAtom, PR_FALSE,
                                aResolveFontName, fc, closure);
        } else if (!family.IsEmpty()) {
            NS_LossyConvertUTF16toASCII gf(genericFamily);
            if (aResolveFontName) {
                ResolveData data(fc, gf, closure);
                PRBool aborted = PR_FALSE, needsBold;
                nsresult rv = NS_OK;
                PRBool foundFamily = PR_FALSE;
                PRBool waitForUserFont = PR_FALSE;
                if (mUserFontSet &&
                    mUserFontSet->FindFontEntry(family, mStyle, foundFamily,
                                                needsBold, waitForUserFont))
                {
                    gfxFontGroup::FontResolverProc(family, &data);
                } else {
                    if (waitForUserFont) {
                        mSkipDrawing = PR_TRUE;
                    }
                    if (!foundFamily) {
                        gfxPlatform *pf = gfxPlatform::GetPlatform();
                        rv = pf->ResolveFontName(family,
                                                 gfxFontGroup::FontResolverProc,
                                                 &data, aborted);
                    }
                }
                if (NS_FAILED(rv) || aborted)
                    return PR_FALSE;
            }
            else {
                if (!fc(family, gf, closure))
                    return PR_FALSE;
            }
        }

        if (generic && aResolveGeneric) {
            nsCAutoString prefName("font.name-list.");
            prefName.Append(lcFamily);
            prefName.AppendLiteral(".");
            prefName.Append(groupString);
            nsresult rv = prefs->GetCharPref(prefName.get(), getter_Copies(value));
            if (NS_SUCCEEDED(rv)) {
                ForEachFontInternal(NS_ConvertUTF8toUTF16(value),
                                    groupAtom, PR_FALSE, aResolveFontName,
                                    fc, closure);
            }
        }

        ++p; 
    }

    return PR_TRUE;
}

PRBool
gfxFontGroup::FontResolverProc(const nsAString& aName, void *aClosure)
{
    ResolveData *data = reinterpret_cast<ResolveData*>(aClosure);
    return (data->mCallback)(aName, data->mGenericFamily, data->mClosure);
}

gfxTextRun *
gfxFontGroup::MakeEmptyTextRun(const Parameters *aParams, PRUint32 aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;
    return gfxTextRun::Create(aParams, nsnull, 0, this, aFlags);
}

gfxTextRun *
gfxFontGroup::MakeSpaceTextRun(const Parameters *aParams, PRUint32 aFlags)
{
    aFlags |= TEXT_IS_8BIT | TEXT_IS_ASCII | TEXT_IS_PERSISTENT;
    static const PRUint8 space = ' ';

    nsAutoPtr<gfxTextRun> textRun;
    textRun = gfxTextRun::Create(aParams, &space, 1, this, aFlags);
    if (!textRun)
        return nsnull;

    gfxFont *font = GetFontAt(0);
    if (NS_UNLIKELY(GetStyle()->size == 0)) {
        
        
        
        textRun->AddGlyphRun(font, 0);
    }
    else {
        textRun->SetSpaceGlyph(font, aParams->mContext, 0);
    }
    
    
    
    return textRun.forget();
}

#define UNICODE_LRO 0x202d
#define UNICODE_RLO 0x202e
#define UNICODE_PDF 0x202c

inline void
AppendDirectionalIndicatorStart(PRUint32 aFlags, nsAString& aString)
{
    static const PRUnichar overrides[2] = { UNICODE_LRO, UNICODE_RLO };
    aString.Append(overrides[(aFlags & gfxTextRunFactory::TEXT_IS_RTL) != 0]);    
    aString.Append(' ');
}

inline void
AppendDirectionalIndicatorEnd(PRBool aNeedDirection, nsAString& aString)
{
    
    
    
    
    aString.Append(' ');
    if (!aNeedDirection)
        return;

    aString.Append('.');
    aString.Append(UNICODE_PDF);
}

gfxTextRun *
gfxFontGroup::MakeTextRun(const PRUint8 *aString, PRUint32 aLength,
                          const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    NS_ASSERTION(aFlags & TEXT_IS_8BIT, "should be marked 8bit");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    nsDependentCSubstring cString(reinterpret_cast<const char*>(aString),
                                  reinterpret_cast<const char*>(aString) + aLength);

    nsAutoString utf16;
    AppendASCIItoUTF16(cString, utf16);

    InitTextRun(aParams->mContext, textRun, utf16.get(), utf16.Length());

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

gfxTextRun *
gfxFontGroup::MakeTextRun(const PRUnichar *aString, PRUint32 aLength,
                          const Parameters *aParams, PRUint32 aFlags)
{
    NS_ASSERTION(aLength > 0, "should use MakeEmptyTextRun for zero-length text");
    gfxTextRun *textRun = gfxTextRun::Create(aParams, aString, aLength, this, aFlags);
    if (!textRun)
        return nsnull;

    gfxPlatform::GetPlatform()->SetupClusterBoundaries(textRun, aString);

    InitTextRun(aParams->mContext, textRun, aString, aLength);

    textRun->FetchGlyphExtents(aParams->mContext);

    return textRun;
}

void
gfxFontGroup::InitTextRun(gfxContext *aContext,
                          gfxTextRun *aTextRun,
                          const PRUnichar *aString,
                          PRUint32 aLength)
{
    
    
    gfxScriptItemizer scriptRuns(aString, aLength);

#ifdef PR_LOGGING
    PRLogModuleInfo *log = (mStyle.systemFont ?
                            gfxPlatform::GetLog(eGfxLog_textrunui) :
                            gfxPlatform::GetLog(eGfxLog_textrun));
#endif

    PRUint32 runStart = 0, runLimit = aLength;
    PRInt32 runScript = HB_SCRIPT_LATIN;
    while (scriptRuns.Next(runStart, runLimit, runScript)) {

#ifdef PR_LOGGING
        if (NS_UNLIKELY(log)) {
            nsCAutoString lang;
            mStyle.language->ToUTF8String(lang);
            PRUint32 runLen = runLimit - runStart;
            PR_LOG(log, PR_LOG_DEBUG,\
                   ("(%s) fontgroup: [%s] lang: %s script: %d len %d "
                    "weight: %d width: %d style: %s "
                    "TEXTRUN [%s] ENDTEXTRUN\n",
                    (mStyle.systemFont ? "textrunui" : "textrun"),
                    NS_ConvertUTF16toUTF8(mFamilies).get(),
                    lang.get(), runScript, runLen,
                    PRUint32(mStyle.weight), PRUint32(mStyle.stretch),
                    (mStyle.style & FONT_STYLE_ITALIC ? "italic" :
                    (mStyle.style & FONT_STYLE_OBLIQUE ? "oblique" :
                                                            "normal")),
                    NS_ConvertUTF16toUTF8(aString + runStart, runLen).get()));
        }
#endif

        InitScriptRun(aContext, aTextRun, aString, aLength,
                      runStart, runLimit, runScript);
    }

    aTextRun->SortGlyphRuns();
}

void
gfxFontGroup::InitScriptRun(gfxContext *aContext,
                            gfxTextRun *aTextRun,
                            const PRUnichar *aString,
                            PRUint32 aTotalLength,
                            PRUint32 aScriptRunStart,
                            PRUint32 aScriptRunEnd,
                            PRInt32 aRunScript)
{
    gfxFont *mainFont = mFonts[0].get();

    PRUint32 runStart = aScriptRunStart;
    nsAutoTArray<gfxTextRange,3> fontRanges;
    ComputeRanges(fontRanges, aString,
                  aScriptRunStart, aScriptRunEnd, aRunScript);
    PRUint32 numRanges = fontRanges.Length();

    for (PRUint32 r = 0; r < numRanges; r++) {
        const gfxTextRange& range = fontRanges[r];
        PRUint32 matchedLength = range.Length();
        gfxFont *matchedFont = (range.font ? range.font.get() : nsnull);

        
        aTextRun->AddGlyphRun(matchedFont ? matchedFont : mainFont,
                              runStart, (matchedLength > 0));
        if (matchedFont) {
            
            if (!matchedFont->SplitAndInitTextRun(aContext, aTextRun, aString,
                                                  runStart, matchedLength,
                                                  aRunScript)) {
                
                matchedFont = nsnull;
            }
        }
        if (!matchedFont) {
            for (PRUint32 index = runStart; index < runStart + matchedLength; index++) {
                
                if (NS_IS_HIGH_SURROGATE(aString[index]) &&
                    index + 1 < aScriptRunEnd &&
                    NS_IS_LOW_SURROGATE(aString[index+1])) {
                    aTextRun->SetMissingGlyph(index,
                                              SURROGATE_TO_UCS4(aString[index],
                                                                aString[index+1]));
                    index++;
                } else {
                    aTextRun->SetMissingGlyph(index, aString[index]);
                }
            }
        }

        runStart += matchedLength;
    }

    
    
    
    
    aTextRun->SanitizeGlyphRuns();

}


already_AddRefed<gfxFont>
gfxFontGroup::FindFontForChar(PRUint32 aCh, PRUint32 aPrevCh,
                              PRInt32 aRunScript, gfxFont *aPrevMatchedFont)
{
    nsRefPtr<gfxFont>    selectedFont;

    
    
    if (gfxFontUtils::IsJoinCauser(aCh) || gfxFontUtils::IsJoinCauser(aPrevCh)) {
        if (aPrevMatchedFont && aPrevMatchedFont->HasCharacter(aCh)) {
            selectedFont = aPrevMatchedFont;
            return selectedFont.forget();
        }
    }

    
    
    
    if (gfxFontUtils::IsVarSelector(aCh)) {
        if (aPrevMatchedFont) {
            selectedFont = aPrevMatchedFont;
            return selectedFont.forget();
        }
        
        return nsnull;
    }

    
    for (PRUint32 i = 0; i < FontListLength(); i++) {
        nsRefPtr<gfxFont> font = GetFontAt(i);
        if (font->HasCharacter(aCh))
            return font.forget();
    }

    
    if ((aCh >= 0xE000  && aCh <= 0xF8FF) || (aCh >= 0xF0000 && aCh <= 0x10FFFD))
        return nsnull;

    
    if ((selectedFont = WhichPrefFontSupportsChar(aCh))) {
        return selectedFont.forget();
    }

    
    
    if (!selectedFont && aPrevMatchedFont && aPrevMatchedFont->HasCharacter(aCh)) {
        selectedFont = aPrevMatchedFont;
        return selectedFont.forget();
    }

    
    if (!selectedFont) {
        selectedFont = WhichSystemFontSupportsChar(aCh);
        return selectedFont.forget();
    }

    return nsnull;
}


void gfxFontGroup::ComputeRanges(nsTArray<gfxTextRange>& aRanges,
                                 const PRUnichar *aString,
                                 PRUint32 begin, PRUint32 end,
                                 PRInt32 aRunScript)
{
    const PRUnichar *str = aString + begin;
    PRUint32 len = end - begin;

    aRanges.Clear();

    if (len == 0) {
        return;
    }

    PRUint32 prevCh = 0;
    gfxFont *prevFont = nsnull;

    for (PRUint32 i = 0; i < len; i++) {

        const PRUint32 origI = i; 

        
        PRUint32 ch = str[i];
        if ((i+1 < len) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(str[i+1])) {
            i++;
            ch = SURROGATE_TO_UCS4(ch, str[i]);
        }

        
        nsRefPtr<gfxFont> font =
            FindFontForChar(ch, prevCh, aRunScript, prevFont);

        prevCh = ch;

        if (aRanges.Length() == 0) {
            
            gfxTextRange r(0,1);
            r.font = font;
            aRanges.AppendElement(r);
            prevFont = font;
        } else {
            
            gfxTextRange& prevRange = aRanges[aRanges.Length() - 1];
            if (prevRange.font != font) {
                
                prevRange.end = origI;

                gfxTextRange r(origI, i+1);
                r.font = font;
                aRanges.AppendElement(r);

                
                
                
                if (!gfxFontUtils::IsJoinCauser(ch)) {
                    prevFont = font;
                }
            }
        }
    }
    aRanges[aRanges.Length()-1].end = len;
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

PRUint64
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
        mSkipDrawing = PR_FALSE;

        
#if defined(XP_MACOSX) || (defined(XP_WIN) && !defined(WINCE)) || defined(ANDROID)
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

    static PRBool AddFontFamilyEntry(eFontPrefLang aLang, const nsAString& aName, void *aClosure)
    {
        PrefFontCallbackData *prefFontData = static_cast<PrefFontCallbackData*>(aClosure);

        gfxFontFamily *family = gfxPlatformFontList::PlatformFontList()->FindFamily(aName);
        if (family) {
            prefFontData->mPrefFamilies.AppendElement(family);
        }
        return PR_TRUE;
    }
};

already_AddRefed<gfxFont>
gfxFontGroup::WhichPrefFontSupportsChar(PRUint32 aCh)
{
    gfxFont *font;

    
    if (aCh > 0xFFFF)
        return nsnull;

    
    PRUint32 unicodeRange = FindCharUnicodeRange(aCh);
    eFontPrefLang charLang = gfxPlatform::GetPlatform()->GetFontPrefLangFor(unicodeRange);

    
    if (mLastPrefFont && charLang == mLastPrefLang &&
        mLastPrefFirstFont && mLastPrefFont->HasCharacter(aCh)) {
        font = mLastPrefFont;
        NS_ADDREF(font);
        return font;
    }

    
    eFontPrefLang prefLangs[kMaxLenPrefLangList];
    PRUint32 i, numLangs = 0;

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

        
        PRUint32  j, numPrefs;
        numPrefs = families.Length();
        for (j = 0; j < numPrefs; j++) {
            
            gfxFontFamily *family = families[j];
            if (!family) continue;

            
            
            
            
            if (family == mLastPrefFamily && mLastPrefFont->HasCharacter(aCh)) {
                font = mLastPrefFont;
                NS_ADDREF(font);
                return font;
            }

            PRBool needsBold;
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

    return nsnull;
}

already_AddRefed<gfxFont>
gfxFontGroup::WhichSystemFontSupportsChar(PRUint32 aCh)
{
    gfxFontEntry *fe = 
        gfxPlatformFontList::PlatformFontList()->FindFontForChar(aCh, GetFontAt(0));
    if (fe) {
        nsRefPtr<gfxFont> font = fe->FindOrMakeFont(&mStyle, PR_FALSE); 
        return font.forget();
    }

    return nsnull;
}

 void
gfxFontGroup::Shutdown()
{
    NS_IF_RELEASE(gLangService);
}

nsILanguageAtomService* gfxFontGroup::gLangService = nsnull;


#define DEFAULT_PIXEL_FONT_SIZE 16.0f

 void
gfxFontStyle::ParseFontFeatureSettings(const nsString& aFeatureString,
                                       nsTArray<gfxFontFeature>& aFeatures)
{
  aFeatures.Clear();
  PRUint32 offset = 0;
  while (offset < aFeatureString.Length()) {
    
    while (offset < aFeatureString.Length() &&
           nsCRT::IsAsciiSpace(aFeatureString[offset])) {
      ++offset;
    }
    PRInt32 limit = aFeatureString.FindChar(',', offset);
    if (limit < 0) {
      limit = aFeatureString.Length();
    }
    
    
    if (offset + 6 <= PRUint32(limit) &&
      aFeatureString[offset+4] == '=') {
      gfxFontFeature setting;
      setting.mTag =
        ((aFeatureString[offset] & 0xff) << 24) +
        ((aFeatureString[offset+1] & 0xff) << 16) +
        ((aFeatureString[offset+2] & 0xff) << 8) +
         (aFeatureString[offset+3] & 0xff);
      nsString valString;
      aFeatureString.Mid(valString, offset+5, limit-offset-5);
      PRInt32 rv;
      setting.mValue = valString.ToInteger(&rv);
      if (rv == NS_OK) {
        
        
        aFeatures.InsertElementSorted(setting);
      }
    }
    offset = limit + 1;
  }
}

 PRUint32
gfxFontStyle::ParseFontLanguageOverride(const nsString& aLangTag)
{
  if (!aLangTag.Length() || aLangTag.Length() > 4) {
    return NO_FONT_LANGUAGE_OVERRIDE;
  }
  PRUint32 index, result = 0;
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
    style(FONT_STYLE_NORMAL), systemFont(PR_TRUE), printerFont(PR_FALSE), 
    weight(FONT_WEIGHT_NORMAL),
    stretch(NS_FONT_STRETCH_NORMAL), size(DEFAULT_PIXEL_FONT_SIZE),
    sizeAdjust(0.0f),
    language(gfxAtoms::x_western),
    languageOverride(NO_FONT_LANGUAGE_OVERRIDE)
{
}

gfxFontStyle::gfxFontStyle(PRUint8 aStyle, PRUint16 aWeight, PRInt16 aStretch,
                           gfxFloat aSize, nsIAtom *aLanguage,
                           float aSizeAdjust, PRPackedBool aSystemFont,
                           PRPackedBool aPrinterFont,
                           const nsString& aFeatureSettings,
                           const nsString& aLanguageOverride):
    style(aStyle), systemFont(aSystemFont), printerFont(aPrinterFont),
    weight(aWeight), stretch(aStretch),
    size(aSize), sizeAdjust(aSizeAdjust),
    language(aLanguage),
    languageOverride(ParseFontLanguageOverride(aLanguageOverride))
{
    ParseFontFeatureSettings(aFeatureSettings, featureSettings);

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
        language = gfxAtoms::x_western;
    }
}

gfxFontStyle::gfxFontStyle(const gfxFontStyle& aStyle) :
    style(aStyle.style), systemFont(aStyle.systemFont), printerFont(aStyle.printerFont),
    weight(aStyle.weight),
    stretch(aStyle.stretch), size(aStyle.size),
    sizeAdjust(aStyle.sizeAdjust),
    language(aStyle.language),
    languageOverride(aStyle.languageOverride)
{
    featureSettings.AppendElements(aStyle.featureSettings);
}

PRInt8
gfxFontStyle::ComputeWeight() const
{
    PRInt8 baseWeight = (weight + 50) / 100;

    if (baseWeight < 0)
        baseWeight = 0;
    if (baseWeight > 9)
        baseWeight = 9;

    return baseWeight;
}

PRBool
gfxTextRun::GlyphRunIterator::NextRun()  {
    if (mNextIndex >= mTextRun->mGlyphRuns.Length())
        return PR_FALSE;
    mGlyphRun = &mTextRun->mGlyphRuns[mNextIndex];
    if (mGlyphRun->mCharacterOffset >= mEndOffset)
        return PR_FALSE;

    mStringStart = PR_MAX(mStartOffset, mGlyphRun->mCharacterOffset);
    PRUint32 last = mNextIndex + 1 < mTextRun->mGlyphRuns.Length()
        ? mTextRun->mGlyphRuns[mNextIndex + 1].mCharacterOffset : mTextRun->mCharacterCount;
    mStringEnd = PR_MIN(mEndOffset, last);

    ++mNextIndex;
    return PR_TRUE;
}

#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
static void
AccountStorageForTextRun(gfxTextRun *aTextRun, PRInt32 aSign)
{
    
    
    
    
    
    
    PRUint32 length = aTextRun->GetLength();
    PRInt32 bytes = length * sizeof(gfxTextRun::CompressedGlyph);
    if (aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_PERSISTENT) {
      bytes += length * ((aTextRun->GetFlags() & gfxTextRunFactory::TEXT_IS_8BIT) ? 1 : 2);
      bytes += sizeof(gfxTextRun::CompressedGlyph) - 1;
      bytes &= ~(sizeof(gfxTextRun::CompressedGlyph) - 1);
    }
    bytes += sizeof(gfxTextRun);
    gTextRunStorage += bytes*aSign;
    gTextRunStorageHighWaterMark = PR_MAX(gTextRunStorageHighWaterMark, gTextRunStorage);
}
#endif







gfxTextRun::CompressedGlyph *
gfxTextRun::AllocateStorage(const void*& aText, PRUint32 aLength, PRUint32 aFlags)
{
    
    
    
    

    
    PRUint64 allocCount = aLength;

    
    if (!(aFlags & gfxTextRunFactory::TEXT_IS_PERSISTENT)) {
        
        
        if (aFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            allocCount += (aLength + sizeof(CompressedGlyph)-1)
                          / sizeof(CompressedGlyph);
        } else {
            allocCount += (aLength*sizeof(PRUnichar) + sizeof(CompressedGlyph)-1)
                          / sizeof(CompressedGlyph);
        }
    }

    
    
    CompressedGlyph *storage = new (std::nothrow) CompressedGlyph[allocCount];
    if (!storage) {
        NS_WARNING("failed to allocate glyph/text storage for text run!");
        return nsnull;
    }

    
    if (!(aFlags & gfxTextRunFactory::TEXT_IS_PERSISTENT)) {
        if (aFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
            PRUint8 *newText = reinterpret_cast<PRUint8*>(storage + aLength);
            memcpy(newText, aText, aLength);
            aText = newText;
        } else {
            PRUnichar *newText = reinterpret_cast<PRUnichar*>(storage + aLength);
            memcpy(newText, aText, aLength*sizeof(PRUnichar));
            aText = newText;
        }
    }

    return storage;
}

gfxTextRun *
gfxTextRun::Create(const gfxTextRunFactory::Parameters *aParams, const void *aText,
                   PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags)
{
    CompressedGlyph *glyphStorage = AllocateStorage(aText, aLength, aFlags);
    if (!glyphStorage) {
        return nsnull;
    }

    return new gfxTextRun(aParams, aText, aLength, aFontGroup, aFlags, glyphStorage);
}

gfxTextRun::gfxTextRun(const gfxTextRunFactory::Parameters *aParams, const void *aText,
                       PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags,
                       CompressedGlyph *aGlyphStorage)
  : mCharacterGlyphs(aGlyphStorage),
    mUserData(aParams->mUserData),
    mFontGroup(aFontGroup),
    mAppUnitsPerDevUnit(aParams->mAppUnitsPerDevUnit),
    mFlags(aFlags), mCharacterCount(aLength), mHashCode(0)
{
    NS_ASSERTION(mAppUnitsPerDevUnit != 0, "Invalid app unit scale");
    MOZ_COUNT_CTOR(gfxTextRun);
    NS_ADDREF(mFontGroup);
    if (aParams->mSkipChars) {
        mSkipChars.TakeFrom(aParams->mSkipChars);
    }

    if (mFlags & gfxTextRunFactory::TEXT_IS_8BIT) {
        mText.mSingle = static_cast<const PRUint8 *>(aText);
    } else {
        mText.mDouble = static_cast<const PRUnichar *>(aText);
    }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
    AccountStorageForTextRun(this, 1);
#endif

    mUserFontSetGeneration = mFontGroup->GetGeneration();
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

    
    
    delete [] mCharacterGlyphs;

    NS_RELEASE(mFontGroup);
    MOZ_COUNT_DTOR(gfxTextRun);
}

gfxTextRun *
gfxTextRun::Clone(const gfxTextRunFactory::Parameters *aParams, const void *aText,
                  PRUint32 aLength, gfxFontGroup *aFontGroup, PRUint32 aFlags)
{
    if (!mCharacterGlyphs)
        return nsnull;

    nsAutoPtr<gfxTextRun> textRun;
    textRun = gfxTextRun::Create(aParams, aText, aLength, aFontGroup, aFlags);
    if (!textRun)
        return nsnull;

    textRun->CopyGlyphDataFrom(this, 0, mCharacterCount, 0);
    return textRun.forget();
}

PRBool
gfxTextRun::SetPotentialLineBreaks(PRUint32 aStart, PRUint32 aLength,
                                   PRPackedBool *aBreakBefore,
                                   gfxContext *aRefContext)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Overflow");

    if (!mCharacterGlyphs)
        return PR_TRUE;
    PRUint32 changed = 0;
    PRUint32 i;
    for (i = 0; i < aLength; ++i) {
        PRBool canBreak = aBreakBefore[i];
        if (canBreak && !mCharacterGlyphs[aStart + i].IsClusterStart()) {
            
            
            NS_WARNING("Break suggested inside cluster!");
            canBreak = PR_FALSE;
        }
        changed |= mCharacterGlyphs[aStart + i].SetCanBreakBefore(canBreak);
    }
    return changed != 0;
}

gfxTextRun::LigatureData
gfxTextRun::ComputeLigatureData(PRUint32 aPartStart, PRUint32 aPartEnd,
                                PropertyProvider *aProvider)
{
    NS_ASSERTION(aPartStart < aPartEnd, "Computing ligature data for empty range");
    NS_ASSERTION(aPartEnd <= mCharacterCount, "Character length overflow");
  
    LigatureData result;
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    PRUint32 i;
    for (i = aPartStart; !charGlyphs[i].IsLigatureGroupStart(); --i) {
        NS_ASSERTION(i > 0, "Ligature at the start of the run??");
    }
    result.mLigatureStart = i;
    for (i = aPartStart + 1; i < mCharacterCount && !charGlyphs[i].IsLigatureGroupStart(); ++i) {
    }
    result.mLigatureEnd = i;

    PRInt32 ligatureWidth =
        GetAdvanceForGlyphs(result.mLigatureStart, result.mLigatureEnd);
    
    PRUint32 totalClusterCount = 0;
    PRUint32 partClusterIndex = 0;
    PRUint32 partClusterCount = 0;
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
    result.mPartAdvance = ligatureWidth*partClusterIndex/totalClusterCount;
    result.mPartWidth = ligatureWidth*partClusterCount/totalClusterCount;

    if (partClusterCount == 0) {
        
        result.mClipBeforePart = result.mClipAfterPart = PR_TRUE;
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
gfxTextRun::ComputePartialLigatureWidth(PRUint32 aPartStart, PRUint32 aPartEnd,
                                        PropertyProvider *aProvider)
{
    if (aPartStart >= aPartEnd)
        return 0;
    LigatureData data = ComputeLigatureData(aPartStart, aPartEnd, aProvider);
    return data.mPartWidth;
}

PRInt32
gfxTextRun::GetAdvanceForGlyphs(PRUint32 aStart, PRUint32 aEnd)
{
    const CompressedGlyph *glyphData = mCharacterGlyphs + aStart;
    PRInt32 advance = 0;
    PRUint32 i;
    for (i = aStart; i < aEnd; ++i, ++glyphData) {
        if (glyphData->IsSimpleGlyph()) {
            advance += glyphData->GetSimpleAdvance();   
        } else {
            PRUint32 glyphCount = glyphData->GetGlyphCount();
            if (glyphCount == 0) {
                continue;
            }
            const DetailedGlyph *details = GetDetailedGlyphs(i);
            if (details) {
                PRUint32 j;
                for (j = 0; j < glyphCount; ++j, ++details) {
                    advance += details->mAdvance;
                }
            }
        }
    }
    return advance;
}

static void
GetAdjustedSpacing(gfxTextRun *aTextRun, PRUint32 aStart, PRUint32 aEnd,
                   gfxTextRun::PropertyProvider *aProvider,
                   gfxTextRun::PropertyProvider::Spacing *aSpacing)
{
    if (aStart >= aEnd)
        return;

    aProvider->GetSpacing(aStart, aEnd - aStart, aSpacing);

#ifdef DEBUG
    

    const gfxTextRun::CompressedGlyph *charGlyphs = aTextRun->GetCharacterGlyphs();
    PRUint32 i;

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

PRBool
gfxTextRun::GetAdjustedSpacingArray(PRUint32 aStart, PRUint32 aEnd,
                                    PropertyProvider *aProvider,
                                    PRUint32 aSpacingStart, PRUint32 aSpacingEnd,
                                    nsTArray<PropertyProvider::Spacing> *aSpacing)
{
    if (!aProvider || !(mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING))
        return PR_FALSE;
    if (!aSpacing->AppendElements(aEnd - aStart))
        return PR_FALSE;
    memset(aSpacing->Elements(), 0, sizeof(gfxFont::Spacing)*(aSpacingStart - aStart));
    GetAdjustedSpacing(this, aSpacingStart, aSpacingEnd, aProvider,
                       aSpacing->Elements() + aSpacingStart - aStart);
    memset(aSpacing->Elements() + aSpacingEnd - aStart, 0, sizeof(gfxFont::Spacing)*(aEnd - aSpacingEnd));
    return PR_TRUE;
}

void
gfxTextRun::ShrinkToLigatureBoundaries(PRUint32 *aStart, PRUint32 *aEnd)
{
    if (*aStart >= *aEnd)
        return;
  
    CompressedGlyph *charGlyphs = mCharacterGlyphs;

    while (*aStart < *aEnd && !charGlyphs[*aStart].IsLigatureGroupStart()) {
        ++(*aStart);
    }
    if (*aEnd < mCharacterCount) {
        while (*aEnd > *aStart && !charGlyphs[*aEnd].IsLigatureGroupStart()) {
            --(*aEnd);
        }
    }
}

void
gfxTextRun::DrawGlyphs(gfxFont *aFont, gfxContext *aContext,
                       PRBool aDrawToPath, gfxPoint *aPt,
                       PRUint32 aStart, PRUint32 aEnd,
                       PropertyProvider *aProvider,
                       PRUint32 aSpacingStart, PRUint32 aSpacingEnd)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    PRBool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider,
        aSpacingStart, aSpacingEnd, &spacingBuffer);
    aFont->Draw(this, aStart, aEnd, aContext, aDrawToPath, aPt,
                haveSpacing ? spacingBuffer.Elements() : nsnull);
}

static void
ClipPartialLigature(gfxTextRun *aTextRun, gfxFloat *aLeft, gfxFloat *aRight,
                    gfxFloat aXOrigin, gfxTextRun::LigatureData *aLigature)
{
    if (aLigature->mClipBeforePart) {
        if (aTextRun->IsRightToLeft()) {
            *aRight = PR_MIN(*aRight, aXOrigin);
        } else {
            *aLeft = PR_MAX(*aLeft, aXOrigin);
        }
    }
    if (aLigature->mClipAfterPart) {
        gfxFloat endEdge = aXOrigin + aTextRun->GetDirection()*aLigature->mPartWidth;
        if (aTextRun->IsRightToLeft()) {
            *aLeft = PR_MAX(*aLeft, endEdge);
        } else {
            *aRight = PR_MIN(*aRight, endEdge);
        }
    }    
}

void
gfxTextRun::DrawPartialLigature(gfxFont *aFont, gfxContext *aCtx,
                                PRUint32 aStart, PRUint32 aEnd,
                                gfxPoint *aPt,
                                PropertyProvider *aProvider)
{
    if (aStart >= aEnd)
        return;

    
    
    
    gfxContextPathAutoSaveRestore savePath(aCtx);

    
    LigatureData data = ComputeLigatureData(aStart, aEnd, aProvider);
    gfxRect clipExtents = aCtx->GetClipExtents();
    gfxFloat left = clipExtents.X()*mAppUnitsPerDevUnit;
    gfxFloat right = clipExtents.XMost()*mAppUnitsPerDevUnit;
    ClipPartialLigature(this, &left, &right, aPt->x, &data);

    aCtx->Save();
    aCtx->NewPath();
    
    
    
    aCtx->Rectangle(gfxRect(left/mAppUnitsPerDevUnit,
                            clipExtents.Y(),
                            (right - left)/mAppUnitsPerDevUnit,
                            clipExtents.Height()), PR_TRUE);
    aCtx->Clip();
    gfxFloat direction = GetDirection();
    gfxPoint pt(aPt->x - direction*data.mPartAdvance, aPt->y);
    DrawGlyphs(aFont, aCtx, PR_FALSE, &pt, data.mLigatureStart,
               data.mLigatureEnd, aProvider, aStart, aEnd);
    aCtx->Restore();

    aPt->x += direction*data.mPartWidth;
}


static PRBool
HasSyntheticBold(gfxTextRun *aRun, PRUint32 aStart, PRUint32 aLength)
{
    gfxTextRun::GlyphRunIterator iter(aRun, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        if (font && font->IsSyntheticBold()) {
            return PR_TRUE;
        }
    }

    return PR_FALSE;
}



static PRBool
HasNonOpaqueColor(gfxContext *aContext, gfxRGBA& aCurrentColor)
{
    if (aContext->GetDeviceColor(aCurrentColor)) {
        if (aCurrentColor.a < 1.0 && aCurrentColor.a > 0.0) {
            return PR_TRUE;
        }
    }
        
    return PR_FALSE;
}


struct BufferAlphaColor {
    BufferAlphaColor(gfxContext *aContext)
        : mContext(aContext)
    {

    }

    ~BufferAlphaColor() {}

    void PushSolidColor(const gfxRect& aBounds, const gfxRGBA& aAlphaColor, PRUint32 appsPerDevUnit)
    {
        mContext->Save();
        mContext->NewPath();
        mContext->Rectangle(gfxRect(aBounds.X() / appsPerDevUnit,
                    aBounds.Y() / appsPerDevUnit,
                    aBounds.Width() / appsPerDevUnit,
                    aBounds.Height() / appsPerDevUnit), PR_TRUE);
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
gfxTextRun::AdjustAdvancesForSyntheticBold(PRUint32 aStart, PRUint32 aLength)
{
    const PRUint32 appUnitsPerDevUnit = GetAppUnitsPerDevUnit();
    PRBool isRTL = IsRightToLeft();

    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        if (font->IsSyntheticBold()) {
            PRUint32 synAppUnitOffset = font->GetSyntheticBoldOffset() * appUnitsPerDevUnit;
            PRUint32 start = iter.GetStringStart();
            PRUint32 end = iter.GetStringEnd();
            PRUint32 i;
            
            
            for (i = start; i < end; ++i) {
                gfxTextRun::CompressedGlyph *glyphData = &mCharacterGlyphs[i];
                
                if (glyphData->IsSimpleGlyph()) {
                    
                    PRUint32 advance = glyphData->GetSimpleAdvance() + synAppUnitOffset;
                    if (CompressedGlyph::IsSimpleAdvance(advance)) {
                        glyphData->SetSimpleGlyph(advance, glyphData->GetSimpleGlyph());
                    } else {
                        
                        PRUint32 glyphIndex = glyphData->GetSimpleGlyph();
                        glyphData->SetComplex(PR_TRUE, PR_TRUE, 1);
                        DetailedGlyph detail = {glyphIndex, advance, 0, 0};
                        SetGlyphs(i, *glyphData, &detail);
                    }
                } else {
                    
                    PRUint32 detailedLength = glyphData->GetGlyphCount();
                    if (detailedLength) {
                        gfxTextRun::DetailedGlyph *details = GetDetailedGlyphs(i);
                        if (!details) {
                            continue;
                        }
                        if (isRTL) {
                            details[0].mAdvance += synAppUnitOffset;
                        } else {
                            details[detailedLength - 1].mAdvance += synAppUnitOffset;
                        }
                    }
                }
            }
        }
    }
}

void
gfxTextRun::Draw(gfxContext *aContext, gfxPoint aPt,
                 PRUint32 aStart, PRUint32 aLength,
                 PropertyProvider *aProvider, gfxFloat *aAdvanceWidth)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");

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
    PRBool needToRestore = PR_FALSE;

    if (HasNonOpaqueColor(aContext, currentColor) && HasSyntheticBold(this, aStart, aLength)) {
        needToRestore = PR_TRUE;
        
        gfxTextRun::Metrics metrics = MeasureText(aStart, aLength, gfxFont::LOOSE_INK_EXTENTS,
                                                  aContext, aProvider);
        metrics.mBoundingBox.MoveBy(aPt);
        syntheticBoldBuffer.PushSolidColor(metrics.mBoundingBox, currentColor, GetAppUnitsPerDevUnit());
    }

    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        PRUint32 start = iter.GetStringStart();
        PRUint32 end = iter.GetStringEnd();
        PRUint32 ligatureRunStart = start;
        PRUint32 ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);
        
        DrawPartialLigature(font, aContext, start, ligatureRunStart, &pt, aProvider);
        DrawGlyphs(font, aContext, PR_FALSE, &pt, ligatureRunStart,
                   ligatureRunEnd, aProvider, ligatureRunStart, ligatureRunEnd);
        DrawPartialLigature(font, aContext, ligatureRunEnd, end, &pt, aProvider);
    }

    
    if (needToRestore) {
        syntheticBoldBuffer.PopAlpha();
    }

    if (aAdvanceWidth) {
        *aAdvanceWidth = (pt.x - aPt.x)*direction;
    }
}

void
gfxTextRun::DrawToPath(gfxContext *aContext, gfxPoint aPt,
                       PRUint32 aStart, PRUint32 aLength,
                       PropertyProvider *aProvider, gfxFloat *aAdvanceWidth)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");

    gfxFloat direction = GetDirection();
    gfxPoint pt = aPt;

    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        PRUint32 start = iter.GetStringStart();
        PRUint32 end = iter.GetStringEnd();
        PRUint32 ligatureRunStart = start;
        PRUint32 ligatureRunEnd = end;
        ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);
        NS_ASSERTION(ligatureRunStart == start,
                     "Can't draw path starting inside ligature");
        NS_ASSERTION(ligatureRunEnd == end,
                     "Can't end drawing path inside ligature");

        DrawGlyphs(font, aContext, PR_TRUE, &pt, ligatureRunStart, ligatureRunEnd, aProvider,
            ligatureRunStart, ligatureRunEnd);
    }

    if (aAdvanceWidth) {
        *aAdvanceWidth = (pt.x - aPt.x)*direction;
    }
}

void
gfxTextRun::AccumulateMetricsForRun(gfxFont *aFont,
                                    PRUint32 aStart, PRUint32 aEnd,
                                    gfxFont::BoundingBoxType aBoundingBoxType,
                                    gfxContext *aRefContext,
                                    PropertyProvider *aProvider,
                                    PRUint32 aSpacingStart, PRUint32 aSpacingEnd,
                                    Metrics *aMetrics)
{
    nsAutoTArray<PropertyProvider::Spacing,200> spacingBuffer;
    PRBool haveSpacing = GetAdjustedSpacingArray(aStart, aEnd, aProvider,
        aSpacingStart, aSpacingEnd, &spacingBuffer);
    Metrics metrics = aFont->Measure(this, aStart, aEnd, aBoundingBoxType, aRefContext,
                                     haveSpacing ? spacingBuffer.Elements() : nsnull);
    aMetrics->CombineWith(metrics, IsRightToLeft());
}

void
gfxTextRun::AccumulatePartialLigatureMetrics(gfxFont *aFont,
    PRUint32 aStart, PRUint32 aEnd,
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
    metrics.mBoundingBox.pos.x = bboxLeft;
    metrics.mBoundingBox.size.width = bboxRight - bboxLeft;

    
    
    metrics.mBoundingBox.pos.x -=
        IsRightToLeft() ? metrics.mAdvanceWidth - (data.mPartAdvance + data.mPartWidth)
            : data.mPartAdvance;    
    metrics.mAdvanceWidth = data.mPartWidth;

    aMetrics->CombineWith(metrics, IsRightToLeft());
}

gfxTextRun::Metrics
gfxTextRun::MeasureText(PRUint32 aStart, PRUint32 aLength,
                        gfxFont::BoundingBoxType aBoundingBoxType,
                        gfxContext *aRefContext,
                        PropertyProvider *aProvider)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");

    Metrics accumulatedMetrics;
    GlyphRunIterator iter(this, aStart, aLength);
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        PRUint32 start = iter.GetStringStart();
        PRUint32 end = iter.GetStringEnd();
        PRUint32 ligatureRunStart = start;
        PRUint32 ligatureRunEnd = end;
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

PRUint32
gfxTextRun::BreakAndMeasureText(PRUint32 aStart, PRUint32 aMaxLength,
                                PRBool aLineBreakBefore, gfxFloat aWidth,
                                PropertyProvider *aProvider,
                                PRBool aSuppressInitialBreak,
                                gfxFloat *aTrimWhitespace,
                                Metrics *aMetrics,
                                gfxFont::BoundingBoxType aBoundingBoxType,
                                gfxContext *aRefContext,
                                PRBool *aUsedHyphenation,
                                PRUint32 *aLastBreak,
                                PRBool aCanWordWrap,
                                gfxBreakPriority *aBreakPriority)
{
    aMaxLength = PR_MIN(aMaxLength, mCharacterCount - aStart);

    NS_ASSERTION(aStart + aMaxLength <= mCharacterCount, "Substring out of range");

    PRUint32 bufferStart = aStart;
    PRUint32 bufferLength = PR_MIN(aMaxLength, MEASUREMENT_BUFFER_SIZE);
    PropertyProvider::Spacing spacingBuffer[MEASUREMENT_BUFFER_SIZE];
    PRBool haveSpacing = aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING) != 0;
    if (haveSpacing) {
        GetAdjustedSpacing(this, bufferStart, bufferStart + bufferLength, aProvider,
                           spacingBuffer);
    }
    PRPackedBool hyphenBuffer[MEASUREMENT_BUFFER_SIZE];
    PRBool haveHyphenation = aProvider &&
                             (mFlags & gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS) != 0;
    if (haveHyphenation) {
        aProvider->GetHyphenationBreaks(bufferStart, bufferLength,
                                        hyphenBuffer);
    }

    gfxFloat width = 0;
    gfxFloat advance = 0;
    
    PRUint32 trimmableChars = 0;
    
    gfxFloat trimmableAdvance = 0;
    PRInt32 lastBreak = -1;
    PRInt32 lastBreakTrimmableChars = -1;
    gfxFloat lastBreakTrimmableAdvance = -1;
    PRBool aborted = PR_FALSE;
    PRUint32 end = aStart + aMaxLength;
    PRBool lastBreakUsedHyphenation = PR_FALSE;

    PRUint32 ligatureRunStart = aStart;
    PRUint32 ligatureRunEnd = end;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    PRUint32 i;
    for (i = aStart; i < end; ++i) {
        if (i >= bufferStart + bufferLength) {
            
            bufferStart = i;
            bufferLength = PR_MIN(aStart + aMaxLength, i + MEASUREMENT_BUFFER_SIZE) - i;
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
            PRBool lineBreakHere = mCharacterGlyphs[i].CanBreakBefore();
            PRBool hyphenation = haveHyphenation && hyphenBuffer[i - bufferStart];
            PRBool wordWrapping = aCanWordWrap && *aBreakPriority <= eWordWrapBreak;

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
                    
                    aborted = PR_TRUE;
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
            if (GetChar(i) == ' ') {
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

    
    
    
    
    PRUint32 charsFit;
    PRBool usedHyphenation = PR_FALSE;
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
            *aLastBreak = PR_UINT32_MAX;
        } else {
            *aLastBreak = lastBreak - aStart;
        }
    }

    return charsFit;
}

gfxFloat
gfxTextRun::GetAdvanceWidth(PRUint32 aStart, PRUint32 aLength,
                            PropertyProvider *aProvider)
{
    NS_ASSERTION(aStart + aLength <= mCharacterCount, "Substring out of range");

    PRUint32 ligatureRunStart = aStart;
    PRUint32 ligatureRunEnd = aStart + aLength;
    ShrinkToLigatureBoundaries(&ligatureRunStart, &ligatureRunEnd);

    gfxFloat result = ComputePartialLigatureWidth(aStart, ligatureRunStart, aProvider) +
                      ComputePartialLigatureWidth(ligatureRunEnd, aStart + aLength, aProvider);

    
    
    if (aProvider && (mFlags & gfxTextRunFactory::TEXT_ENABLE_SPACING)) {
        PRUint32 i;
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

PRBool
gfxTextRun::SetLineBreaks(PRUint32 aStart, PRUint32 aLength,
                          PRBool aLineBreakBefore, PRBool aLineBreakAfter,
                          gfxFloat *aAdvanceWidthDelta,
                          gfxContext *aRefContext)
{
    
    
    if (aAdvanceWidthDelta) {
        *aAdvanceWidthDelta = 0;
    }
    return PR_FALSE;
}

PRUint32
gfxTextRun::FindFirstGlyphRunContaining(PRUint32 aOffset)
{
    NS_ASSERTION(aOffset <= mCharacterCount, "Bad offset looking for glyphrun");
    NS_ASSERTION(mCharacterCount == 0 || mGlyphRuns.Length() > 0,
                 "non-empty text but no glyph runs present!");
    if (aOffset == mCharacterCount)
        return mGlyphRuns.Length();
    PRUint32 start = 0;
    PRUint32 end = mGlyphRuns.Length();
    while (end - start > 1) {
        PRUint32 mid = (start + end)/2;
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
gfxTextRun::AddGlyphRun(gfxFont *aFont, PRUint32 aUTF16Offset, PRBool aForceNewRun)
{
    PRUint32 numGlyphRuns = mGlyphRuns.Length();
    if (!aForceNewRun &&
        numGlyphRuns > 0)
    {
        GlyphRun *lastGlyphRun = &mGlyphRuns[numGlyphRuns - 1];

        NS_ASSERTION(lastGlyphRun->mCharacterOffset <= aUTF16Offset,
                     "Glyph runs out of order (and run not forced)");

        
        if (lastGlyphRun->mFont == aFont)
            return NS_OK;

        
        
        if (lastGlyphRun->mCharacterOffset == aUTF16Offset) {

            
            
            
            if (numGlyphRuns > 1 &&
                mGlyphRuns[numGlyphRuns - 2].mFont == aFont)
            {
                mGlyphRuns.TruncateLength(numGlyphRuns - 1);
                return NS_OK;
            }

            lastGlyphRun->mFont = aFont;
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
    PRUint32 i;
    for (i = 0; i < runs.Length(); ++i) {
        
        
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

    
    
    
    
    PRInt32 i, lastRunIndex = mGlyphRuns.Length() - 1;
    for (i = lastRunIndex; i >= 0; --i) {
        GlyphRun& run = mGlyphRuns[i];
        while (mCharacterGlyphs[run.mCharacterOffset].IsLigatureContinuation() &&
               run.mCharacterOffset < mCharacterCount) {
            run.mCharacterOffset++;
        }
        
        if ((i < lastRunIndex &&
             run.mCharacterOffset >= mGlyphRuns[i+1].mCharacterOffset) ||
            (i == lastRunIndex && run.mCharacterOffset == mCharacterCount)) {
            mGlyphRuns.RemoveElementAt(i);
            --lastRunIndex;
        }
    }
}

PRUint32
gfxTextRun::CountMissingGlyphs()
{
    PRUint32 i;
    PRUint32 count = 0;
    for (i = 0; i < mCharacterCount; ++i) {
        if (mCharacterGlyphs[i].IsMissing()) {
            ++count;
        }
    }
    return count;
}

gfxTextRun::DetailedGlyph *
gfxTextRun::AllocateDetailedGlyphs(PRUint32 aIndex, PRUint32 aCount)
{
    NS_ASSERTION(aIndex < mCharacterCount, "Index out of range");

    if (!mCharacterGlyphs) {
        return nsnull;
    }

    if (!mDetailedGlyphs) {
        mDetailedGlyphs = new DetailedGlyphStore();
    }

    DetailedGlyph *details = mDetailedGlyphs->Allocate(aIndex, aCount);
    if (!details) {
        mCharacterGlyphs[aIndex].SetMissing(0);
        return nsnull;
    }

    return details;
}

void
gfxTextRun::SetGlyphs(PRUint32 aIndex, CompressedGlyph aGlyph,
                      const DetailedGlyph *aGlyphs)
{
    NS_ASSERTION(!aGlyph.IsSimpleGlyph(), "Simple glyphs not handled here");
    NS_ASSERTION(aIndex > 0 ||
                 (aGlyph.IsClusterStart() && aGlyph.IsLigatureGroupStart()),
                 "First character must be the start of a cluster and can't be a ligature continuation!");

    PRUint32 glyphCount = aGlyph.GetGlyphCount();
    if (glyphCount > 0) {
        DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, glyphCount);
        if (!details)
            return;
        memcpy(details, aGlyphs, sizeof(DetailedGlyph)*glyphCount);
    }
    mCharacterGlyphs[aIndex] = aGlyph;
}

#include "ignorable.x-ccmap"
DEFINE_X_CCMAP(gIgnorableCCMapExt, const);

static inline PRBool
IsDefaultIgnorable(PRUint32 aChar)
{
    return CCMAP_HAS_CHAR_EXT(gIgnorableCCMapExt, aChar);
}

void
gfxTextRun::SetMissingGlyph(PRUint32 aIndex, PRUint32 aChar)
{
    DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
    if (!details)
        return;

    details->mGlyphID = aChar;
    GlyphRun *glyphRun = &mGlyphRuns[FindFirstGlyphRunContaining(aIndex)];
    if (IsDefaultIgnorable(aChar)) {
        
        details->mAdvance = 0;
    } else {
        gfxFloat width = PR_MAX(glyphRun->mFont->GetMetrics().aveCharWidth,
                                gfxFontMissingGlyphs::GetDesiredMinWidth(aChar));
        details->mAdvance = PRUint32(width*GetAppUnitsPerDevUnit());
    }
    details->mXOffset = 0;
    details->mYOffset = 0;
    mCharacterGlyphs[aIndex].SetMissing(1);
}

PRBool
gfxTextRun::FilterIfIgnorable(PRUint32 aIndex)
{
    PRUint32 ch = GetChar(aIndex);
    if (IsDefaultIgnorable(ch)) {
        DetailedGlyph *details = AllocateDetailedGlyphs(aIndex, 1);
        if (details) {
            details->mGlyphID = ch;
            details->mAdvance = 0;
            details->mXOffset = 0;
            details->mYOffset = 0;
            mCharacterGlyphs[aIndex].SetMissing(1);
            return PR_TRUE;
        }
    }
    return PR_FALSE;
}

void
gfxTextRun::CopyGlyphDataFrom(gfxTextRun *aSource, PRUint32 aStart,
                              PRUint32 aLength, PRUint32 aDest)
{
    NS_ASSERTION(aStart + aLength <= aSource->GetLength(),
                 "Source substring out of range");
    NS_ASSERTION(aDest + aLength <= GetLength(),
                 "Destination substring out of range");

    if (aSource->mSkipDrawing) {
        mSkipDrawing = PR_TRUE;
    }

    
    for (PRUint32 i = 0; i < aLength; ++i) {
        CompressedGlyph g = aSource->mCharacterGlyphs[i + aStart];
        g.SetCanBreakBefore(mCharacterGlyphs[i + aDest].CanBreakBefore());
        if (!g.IsSimpleGlyph()) {
            PRUint32 count = g.GetGlyphCount();
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
        mCharacterGlyphs[i + aDest] = g;
    }

    
    GlyphRunIterator iter(aSource, aStart, aLength);
#ifdef DEBUG
    gfxFont *lastFont = nsnull;
#endif
    while (iter.NextRun()) {
        gfxFont *font = iter.GetGlyphRun()->mFont;
        NS_ASSERTION(font != lastFont, "Glyphruns not coalesced?");
#ifdef DEBUG
        lastFont = font;
        PRUint32 end = iter.GetStringEnd();
#endif
        PRUint32 start = iter.GetStringStart();

        
        
        
        
        
        
        
        
        
        NS_WARN_IF_FALSE(aSource->IsClusterStart(start),
                         "Started font run in the middle of a cluster");
        NS_WARN_IF_FALSE(end == aSource->GetLength() || aSource->IsClusterStart(end),
                         "Ended font run in the middle of a cluster");

        nsresult rv = AddGlyphRun(font, start - aStart + aDest);
        if (NS_FAILED(rv))
            return;
    }
}

void
gfxTextRun::SetSpaceGlyph(gfxFont *aFont, gfxContext *aContext, PRUint32 aCharIndex)
{
    PRUint32 spaceGlyph = aFont->GetSpaceGlyph();
    float spaceWidth = aFont->GetMetrics().spaceWidth;
    PRUint32 spaceWidthAppUnits = NS_lroundf(spaceWidth*mAppUnitsPerDevUnit);
    if (!spaceGlyph ||
        !CompressedGlyph::IsSimpleGlyphID(spaceGlyph) ||
        !CompressedGlyph::IsSimpleAdvance(spaceWidthAppUnits)) {
        gfxTextRunFactory::Parameters params = {
            aContext, nsnull, nsnull, nsnull, 0, mAppUnitsPerDevUnit
        };
        static const PRUint8 space = ' ';
        nsAutoPtr<gfxTextRun> textRun;
        textRun = mFontGroup->MakeTextRun(&space, 1, &params,
            gfxTextRunFactory::TEXT_IS_8BIT | gfxTextRunFactory::TEXT_IS_ASCII |
            gfxTextRunFactory::TEXT_IS_PERSISTENT);
        if (!textRun || !textRun->mCharacterGlyphs)
            return;
        CopyGlyphDataFrom(textRun, 0, 1, aCharIndex);
        return;
    }

    AddGlyphRun(aFont, aCharIndex);
    CompressedGlyph g;
    g.SetSimpleGlyph(spaceWidthAppUnits, spaceGlyph);
    SetSimpleGlyph(aCharIndex, g);
}

void
gfxTextRun::FetchGlyphExtents(gfxContext *aRefContext)
{
    PRBool needsGlyphExtents = NeedsGlyphExtents(this);
    if (!needsGlyphExtents && !mDetailedGlyphs)
        return;

    PRUint32 i;
    CompressedGlyph *charGlyphs = mCharacterGlyphs;
    for (i = 0; i < mGlyphRuns.Length(); ++i) {
        gfxFont *font = mGlyphRuns[i].mFont;
        PRUint32 start = mGlyphRuns[i].mCharacterOffset;
        PRUint32 end = i + 1 < mGlyphRuns.Length()
            ? mGlyphRuns[i + 1].mCharacterOffset : GetLength();
        PRBool fontIsSetup = PR_FALSE;
        PRUint32 j;
        gfxGlyphExtents *extents = font->GetOrCreateGlyphExtents(mAppUnitsPerDevUnit);
  
        for (j = start; j < end; ++j) {
            const gfxTextRun::CompressedGlyph *glyphData = &charGlyphs[j];
            if (glyphData->IsSimpleGlyph()) {
                
                
                if (needsGlyphExtents) {
                    PRUint32 glyphIndex = glyphData->GetSimpleGlyph();
                    if (!extents->IsGlyphKnown(glyphIndex)) {
                        if (!fontIsSetup) {
                            font->SetupCairoFont(aRefContext);
                             fontIsSetup = PR_TRUE;
                        }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
                        ++gGlyphExtentsSetupEagerSimple;
#endif
                        font->SetupGlyphExtents(aRefContext, glyphIndex, PR_FALSE, extents);
                    }
                }
            } else if (!glyphData->IsMissing()) {
                PRUint32 glyphCount = glyphData->GetGlyphCount();
                if (glyphCount == 0) {
                    continue;
                }
                const gfxTextRun::DetailedGlyph *details = GetDetailedGlyphs(j);
                if (!details) {
                    continue;
                }
                for (PRUint32 k = 0; k < glyphCount; ++k, ++details) {
                    PRUint32 glyphIndex = details->mGlyphID;
                    if (!extents->IsGlyphKnownWithTightExtents(glyphIndex)) {
                        if (!fontIsSetup) {
                            font->SetupCairoFont(aRefContext);
                            fontIsSetup = PR_TRUE;
                        }
#ifdef DEBUG_TEXT_RUN_STORAGE_METRICS
                        ++gGlyphExtentsSetupEagerTight;
#endif
                        font->SetupGlyphExtents(aRefContext, glyphIndex, PR_TRUE, extents);
                    }
                }
            }
        }
    }
}


gfxTextRun::ClusterIterator::ClusterIterator(gfxTextRun *aTextRun)
    : mTextRun(aTextRun), mCurrentChar(PRUint32(-1))
{
}

void
gfxTextRun::ClusterIterator::Reset()
{
    mCurrentChar = PRUint32(-1);
}

PRBool
gfxTextRun::ClusterIterator::NextCluster()
{
    while (++mCurrentChar < mTextRun->GetLength()) {
        if (mTextRun->IsClusterStart(mCurrentChar)) {
            return PR_TRUE;
        }
    }

    mCurrentChar = PRUint32(-1);
    return PR_FALSE;
}

PRUint32
gfxTextRun::ClusterIterator::ClusterLength() const
{
    if (mCurrentChar == PRUint32(-1)) {
        return 0;
    }

    PRUint32 i = mCurrentChar;
    while (++i < mTextRun->GetLength()) {
        if (mTextRun->IsClusterStart(i)) {
            break;
        }
    }

    return i - mCurrentChar;
}

gfxFloat
gfxTextRun::ClusterIterator::ClusterAdvance(PropertyProvider *aProvider) const
{
    if (mCurrentChar == PRUint32(-1)) {
        return 0;
    }

    return mTextRun->GetAdvanceWidth(mCurrentChar, ClusterLength(), aProvider);
}


#ifdef DEBUG
void
gfxTextRun::Dump(FILE* aOutput) {
    if (!aOutput) {
        aOutput = stdout;
    }

    PRUint32 i;
    fputc('"', aOutput);
    for (i = 0; i < mCharacterCount; ++i) {
        PRUnichar ch = GetChar(i);
        if (ch >= 32 && ch < 128) {
            fputc(ch, aOutput);
        } else {
            fprintf(aOutput, "\\u%4x", ch);
        }
    }
    fputs("\" [", aOutput);
    for (i = 0; i < mGlyphRuns.Length(); ++i) {
        if (i > 0) {
            fputc(',', aOutput);
        }
        gfxFont* font = mGlyphRuns[i].mFont;
        const gfxFontStyle* style = font->GetStyle();
        NS_ConvertUTF16toUTF8 fontName(font->GetName());
        nsCAutoString lang;
        style->language->ToUTF8String(lang);
        fprintf(aOutput, "%d: %s %f/%d/%d/%s", mGlyphRuns[i].mCharacterOffset,
                fontName.get(), style->size,
                style->weight, style->style, lang.get());
    }
    fputc(']', aOutput);
}
#endif
