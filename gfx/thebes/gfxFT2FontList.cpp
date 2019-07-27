




#include "mozilla/ArrayUtils.h"
#include "mozilla/Base64.h"
#include "mozilla/MemoryReporting.h"

#include "mozilla/dom/ContentChild.h"
#include "gfxAndroidPlatform.h"
#include "mozilla/Omnijar.h"
#include "nsAutoPtr.h"
#include "nsIInputStream.h"
#include "nsNetUtil.h"
#define gfxToolkitPlatform gfxAndroidPlatform

#include "nsXULAppAPI.h"
#include <dirent.h>
#include <android/log.h>
#define ALOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gecko" , ## args)

#include "ft2build.h"
#include FT_FREETYPE_H
#include FT_TRUETYPE_TAGS_H
#include FT_TRUETYPE_TABLES_H
#include "cairo-ft.h"

#include "gfxFT2FontList.h"
#include "gfxFT2Fonts.h"
#include "gfxUserFontSet.h"
#include "gfxFontUtils.h"

#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"

#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsISimpleEnumerator.h"
#include "nsIMemory.h"
#include "gfxFontConstants.h"

#include "mozilla/Preferences.h"
#include "mozilla/scache/StartupCache.h"
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

using namespace mozilla;

#ifdef PR_LOGGING
static PRLogModuleInfo *
GetFontInfoLog()
{
    static PRLogModuleInfo *sLog;
    if (!sLog)
        sLog = PR_NewLogModule("fontInfoLog");
    return sLog;
}
#endif 

#undef LOG
#define LOG(args) PR_LOG(GetFontInfoLog(), PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(GetFontInfoLog(), PR_LOG_DEBUG)

static cairo_user_data_key_t sFTUserFontDataKey;

static __inline void
BuildKeyNameFromFontName(nsAString &aName)
{
    ToLowerCase(aName);
}








class AutoFTFace {
public:
    AutoFTFace(FT2FontEntry* aFontEntry)
        : mFace(nullptr), mFontDataBuf(nullptr), mOwnsFace(false)
    {
        if (aFontEntry->mFTFace) {
            mFace = aFontEntry->mFTFace;
            return;
        }

        NS_ASSERTION(!aFontEntry->mFilename.IsEmpty(),
                     "can't use AutoFTFace for fonts without a filename");
        FT_Library ft = gfxToolkitPlatform::GetPlatform()->GetFTLibrary();

        
        
        
        
        
        
        if (aFontEntry->mFilename[0] != '/') {
            nsRefPtr<nsZipArchive> reader =
                Omnijar::GetReader(Omnijar::Type::GRE);
            nsZipItem *item = reader->GetItem(aFontEntry->mFilename.get());
            NS_ASSERTION(item, "failed to find zip entry");

            uint32_t bufSize = item->RealSize();
            mFontDataBuf = static_cast<uint8_t*>(malloc(bufSize));
            if (mFontDataBuf) {
                nsZipCursor cursor(item, reader, mFontDataBuf, bufSize);
                cursor.Copy(&bufSize);
                NS_ASSERTION(bufSize == item->RealSize(),
                             "error reading bundled font");

                if (FT_Err_Ok != FT_New_Memory_Face(ft, mFontDataBuf, bufSize,
                                                    aFontEntry->mFTFontIndex,
                                                    &mFace)) {
                    NS_WARNING("failed to create freetype face");
                }
            }
        } else {
            if (FT_Err_Ok != FT_New_Face(ft, aFontEntry->mFilename.get(),
                                         aFontEntry->mFTFontIndex, &mFace)) {
                NS_WARNING("failed to create freetype face");
            }
        }
        if (FT_Err_Ok != FT_Select_Charmap(mFace, FT_ENCODING_UNICODE)) {
            NS_WARNING("failed to select Unicode charmap");
        }
        mOwnsFace = true;
    }

    ~AutoFTFace() {
        if (mFace && mOwnsFace) {
            FT_Done_Face(mFace);
            if (mFontDataBuf) {
                free(mFontDataBuf);
            }
        }
    }

    operator FT_Face() { return mFace; }

    
    
    
    FT_Face forget() {
        NS_ASSERTION(mOwnsFace, "can't forget() when we didn't own the face");
        mOwnsFace = false;
        return mFace;
    }

    const uint8_t* FontData() const { return mFontDataBuf; }

private:
    FT_Face  mFace;
    uint8_t* mFontDataBuf; 
                           
                           
                           
    bool     mOwnsFace;
};











cairo_scaled_font_t *
FT2FontEntry::CreateScaledFont(const gfxFontStyle *aStyle)
{
    cairo_font_face_t *cairoFace = CairoFontFace();
    if (!cairoFace) {
        return nullptr;
    }

    cairo_scaled_font_t *scaledFont = nullptr;

    cairo_matrix_t sizeMatrix;
    cairo_matrix_t identityMatrix;

    
    cairo_matrix_init_scale(&sizeMatrix, aStyle->size, aStyle->size);
    cairo_matrix_init_identity(&identityMatrix);

    
    bool needsOblique = !IsItalic() &&
            (aStyle->style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE)) &&
            aStyle->allowSyntheticStyle;

    if (needsOblique) {
        cairo_matrix_t style;
        cairo_matrix_init(&style,
                          1,                
                          0,                
                          -1 * OBLIQUE_SKEW_FACTOR, 
                          1,                
                          0,                
                          0);               
        cairo_matrix_multiply(&sizeMatrix, &sizeMatrix, &style);
    }

    cairo_font_options_t *fontOptions = cairo_font_options_create();

    if (gfxPlatform::GetPlatform()->RequiresLinearZoom()) {
        cairo_font_options_set_hint_metrics(fontOptions, CAIRO_HINT_METRICS_OFF);
    }

    scaledFont = cairo_scaled_font_create(cairoFace,
                                          &sizeMatrix,
                                          &identityMatrix, fontOptions);
    cairo_font_options_destroy(fontOptions);

    NS_ASSERTION(cairo_scaled_font_status(scaledFont) == CAIRO_STATUS_SUCCESS,
                 "Failed to make scaled font");

    return scaledFont;
}

FT2FontEntry::~FT2FontEntry()
{
    
    mFTFace = nullptr;

#ifndef ANDROID
    if (mFontFace) {
        cairo_font_face_destroy(mFontFace);
        mFontFace = nullptr;
    }
#endif
}

gfxFont*
FT2FontEntry::CreateFontInstance(const gfxFontStyle *aFontStyle, bool aNeedsBold)
{
    cairo_scaled_font_t *scaledFont = CreateScaledFont(aFontStyle);
    if (!scaledFont) {
        return nullptr;
    }
    gfxFont *font = new gfxFT2Font(scaledFont, this, aFontStyle, aNeedsBold);
    cairo_scaled_font_destroy(scaledFont);
    return font;
}


FT2FontEntry*
FT2FontEntry::CreateFontEntry(const nsAString& aFontName,
                              uint16_t aWeight,
                              int16_t aStretch,
                              bool aItalic,
                              const uint8_t* aFontData,
                              uint32_t aLength)
{
    
    
    
    FT_Face face;
    FT_Error error =
        FT_New_Memory_Face(gfxToolkitPlatform::GetPlatform()->GetFTLibrary(),
                           aFontData, aLength, 0, &face);
    if (error != FT_Err_Ok) {
        free((void*)aFontData);
        return nullptr;
    }
    if (FT_Err_Ok != FT_Select_Charmap(face, FT_ENCODING_UNICODE)) {
        FT_Done_Face(face);
        free((void*)aFontData);
        return nullptr;
    }
    
    
    FT2FontEntry* fe =
        FT2FontEntry::CreateFontEntry(face, nullptr, 0, aFontName,
                                      aFontData);
    if (fe) {
        fe->mItalic = aItalic;
        fe->mWeight = aWeight;
        fe->mStretch = aStretch;
        fe->mIsDataUserFont = true;
    }
    return fe;
}

class FTUserFontData {
public:
    FTUserFontData(FT_Face aFace, const uint8_t* aData)
        : mFace(aFace), mFontData(aData)
    {
    }

    ~FTUserFontData()
    {
        FT_Done_Face(mFace);
        if (mFontData) {
            free((void*)mFontData);
        }
    }

    const uint8_t *FontData() const { return mFontData; }

private:
    FT_Face        mFace;
    const uint8_t *mFontData;
};

static void
FTFontDestroyFunc(void *data)
{
    FTUserFontData *userFontData = static_cast<FTUserFontData*>(data);
    delete userFontData;
}


FT2FontEntry*
FT2FontEntry::CreateFontEntry(const FontListEntry& aFLE)
{
    FT2FontEntry *fe = new FT2FontEntry(aFLE.faceName());
    fe->mFilename = aFLE.filepath();
    fe->mFTFontIndex = aFLE.index();
    fe->mWeight = aFLE.weight();
    fe->mStretch = aFLE.stretch();
    fe->mItalic = aFLE.italic();
    return fe;
}


static bool
FTFaceIsItalic(FT_Face aFace)
{
    return !!(aFace->style_flags & FT_STYLE_FLAG_ITALIC);
}

static uint16_t
FTFaceGetWeight(FT_Face aFace)
{
    TT_OS2 *os2 = static_cast<TT_OS2*>(FT_Get_Sfnt_Table(aFace, ft_sfnt_os2));
    uint16_t os2weight = 0;
    if (os2 && os2->version != 0xffff) {
        
        
        
        
        if (os2->usWeightClass >= 100 && os2->usWeightClass <= 900) {
            os2weight = os2->usWeightClass;
        } else if (os2->usWeightClass >= 1 && os2->usWeightClass <= 9) {
            os2weight = os2->usWeightClass * 100;
        }
    }

    uint16_t result;
    if (os2weight != 0) {
        result = os2weight;
    } else if (aFace->style_flags & FT_STYLE_FLAG_BOLD) {
        result = 700;
    } else {
        result = 400;
    }

    NS_ASSERTION(result >= 100 && result <= 900, "Invalid weight in font!");

    return result;
}









FT2FontEntry*
FT2FontEntry::CreateFontEntry(FT_Face aFace,
                              const char* aFilename, uint8_t aIndex,
                              const nsAString& aName,
                              const uint8_t* aFontData)
{
    FT2FontEntry *fe = new FT2FontEntry(aName);
    fe->mItalic = FTFaceIsItalic(aFace);
    fe->mWeight = FTFaceGetWeight(aFace);
    fe->mFilename = aFilename;
    fe->mFTFontIndex = aIndex;

    if (aFontData) {
        fe->mFTFace = aFace;
        int flags = gfxPlatform::GetPlatform()->FontHintingEnabled() ?
                    FT_LOAD_DEFAULT :
                    (FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING);
        fe->mFontFace = cairo_ft_font_face_create_for_ft_face(aFace, flags);
        FTUserFontData *userFontData = new FTUserFontData(aFace, aFontData);
        cairo_font_face_set_user_data(fe->mFontFace, &sFTUserFontDataKey,
                                      userFontData, FTFontDestroyFunc);
    }

    return fe;
}



static FT2FontEntry*
CreateNamedFontEntry(FT_Face aFace, const char* aFilename, uint8_t aIndex)
{
    if (!aFace->family_name) {
        return nullptr;
    }
    nsAutoString fontName;
    AppendUTF8toUTF16(aFace->family_name, fontName);
    if (aFace->style_name && strcmp("Regular", aFace->style_name)) {
        fontName.Append(' ');
        AppendUTF8toUTF16(aFace->style_name, fontName);
    }
    return FT2FontEntry::CreateFontEntry(aFace, aFilename, aIndex, fontName);
}

FT2FontEntry*
gfxFT2Font::GetFontEntry()
{
    return static_cast<FT2FontEntry*> (mFontEntry.get());
}

cairo_font_face_t *
FT2FontEntry::CairoFontFace()
{
    if (!mFontFace) {
        AutoFTFace face(this);
        if (!face) {
            return nullptr;
        }
        mFTFace = face.forget();
        int flags = gfxPlatform::GetPlatform()->FontHintingEnabled() ?
                    FT_LOAD_DEFAULT :
                    (FT_LOAD_NO_AUTOHINT | FT_LOAD_NO_HINTING);
        mFontFace = cairo_ft_font_face_create_for_ft_face(face, flags);
        FTUserFontData *userFontData = new FTUserFontData(face, face.FontData());
        cairo_font_face_set_user_data(mFontFace, &sFTUserFontDataKey,
                                      userFontData, FTFontDestroyFunc);
    }
    return mFontFace;
}












nsresult
FT2FontEntry::ReadCMAP(FontInfoData *aFontInfoData)
{
    if (mCharacterMap) {
        return NS_OK;
    }

    nsRefPtr<gfxCharacterMap> charmap = new gfxCharacterMap();

    AutoFallibleTArray<uint8_t,16384> buffer;
    nsresult rv = CopyFontTable(TTAG_cmap, buffer);
    
    if (NS_SUCCEEDED(rv)) {
        bool unicodeFont;
        bool symbolFont;
        rv = gfxFontUtils::ReadCMAP(buffer.Elements(), buffer.Length(),
                                    *charmap, mUVSOffset,
                                    unicodeFont, symbolFont);
    }

    if (NS_SUCCEEDED(rv) && !HasGraphiteTables()) {
        
        
        
        

        
        bool hasGSUB = HasFontTable(TRUETYPE_TAG('G','S','U','B'));

        for (const ScriptRange* sr = gfxPlatformFontList::sComplexScriptRanges;
             sr->rangeStart; sr++) {
            
            if (charmap->TestRange(sr->rangeStart, sr->rangeEnd)) {
                
                if (hasGSUB && SupportsScriptInGSUB(sr->tags)) {
                    continue;
                }
                charmap->ClearRange(sr->rangeStart, sr->rangeEnd);
            }
        }
    }

#ifdef MOZ_WIDGET_ANDROID
    
    
    if (!charmap->test(0x0972) &&
        charmap->test(0x0905) && charmap->test(0x0945)) {
        charmap->set(0x0972);
    }
#endif

    mHasCmapTable = NS_SUCCEEDED(rv);
    if (mHasCmapTable) {
        gfxPlatformFontList *pfl = gfxPlatformFontList::PlatformFontList();
        mCharacterMap = pfl->FindCharMap(charmap);
    } else {
        
        mCharacterMap = new gfxCharacterMap();
    }
    return rv;
}

nsresult
FT2FontEntry::CopyFontTable(uint32_t aTableTag,
                            FallibleTArray<uint8_t>& aBuffer)
{
    AutoFTFace face(this);
    if (!face) {
        return NS_ERROR_FAILURE;
    }

    FT_Error status;
    FT_ULong len = 0;
    status = FT_Load_Sfnt_Table(face, aTableTag, 0, nullptr, &len);
    if (status != FT_Err_Ok || len == 0) {
        return NS_ERROR_FAILURE;
    }

    if (!aBuffer.SetLength(len)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    uint8_t *buf = aBuffer.Elements();
    status = FT_Load_Sfnt_Table(face, aTableTag, 0, buf, &len);
    NS_ENSURE_TRUE(status == FT_Err_Ok, NS_ERROR_FAILURE);

    return NS_OK;
}

hb_blob_t*
FT2FontEntry::GetFontTable(uint32_t aTableTag)
{
    if (mFontFace) {
        
        
        FTUserFontData *userFontData = static_cast<FTUserFontData*>(
            cairo_font_face_get_user_data(mFontFace, &sFTUserFontDataKey));
        if (userFontData && userFontData->FontData()) {
            return GetTableFromFontData(userFontData->FontData(), aTableTag);
        }
    }

    
    
    return gfxFontEntry::GetFontTable(aTableTag);
}

void
FT2FontEntry::AddSizeOfExcludingThis(MallocSizeOf aMallocSizeOf,
                                     FontListSizes* aSizes) const
{
    gfxFontEntry::AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
    aSizes->mFontListSize +=
        mFilename.SizeOfExcludingThisIfUnshared(aMallocSizeOf);
}

void
FT2FontEntry::AddSizeOfIncludingThis(MallocSizeOf aMallocSizeOf,
                                     FontListSizes* aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    AddSizeOfExcludingThis(aMallocSizeOf, aSizes);
}







void
FT2FontFamily::AddFacesToFontList(InfallibleTArray<FontListEntry>* aFontList,
                                  Visibility aVisibility)
{
    for (int i = 0, n = mAvailableFonts.Length(); i < n; ++i) {
        const FT2FontEntry *fe =
            static_cast<const FT2FontEntry*>(mAvailableFonts[i].get());
        if (!fe) {
            continue;
        }
        
        aFontList->AppendElement(FontListEntry(Name(), fe->Name(),
                                               fe->mFilename,
                                               fe->Weight(), fe->Stretch(),
                                               fe->IsItalic(),
                                               fe->mFTFontIndex,
                                               aVisibility == kHidden));
    }
}










#define CACHE_KEY "font.cached-list"

class FontNameCache {
public:
    FontNameCache()
        : mWriteNeeded(false)
    {
        mOps = (PLDHashTableOps) {
            StringHash,
            HashMatchEntry,
            MoveEntry,
            PL_DHashClearEntryStub,
            nullptr
        };

        PL_DHashTableInit(&mMap, &mOps, sizeof(FNCMapEntry), 0);

        MOZ_ASSERT(XRE_GetProcessType() == GeckoProcessType_Default,
                   "StartupCacheFontNameCache should only be used in chrome "
                   "process");
        mCache = mozilla::scache::StartupCache::GetSingleton();

        Init();
    }

    ~FontNameCache()
    {
        if (!mMap.IsInitialized()) {
            return;
        }
        if (!mWriteNeeded || !mCache) {
            PL_DHashTableFinish(&mMap);
            return;
        }

        nsAutoCString buf;
        PL_DHashTableEnumerate(&mMap, WriteOutMap, &buf);
        PL_DHashTableFinish(&mMap);
        mCache->PutBuffer(CACHE_KEY, buf.get(), buf.Length() + 1);
    }

    void Init()
    {
        if (!mMap.IsInitialized() || !mCache) {
            return;
        }
        uint32_t size;
        char* buf;
        if (NS_FAILED(mCache->GetBuffer(CACHE_KEY, &buf, &size))) {
            return;
        }

        LOG(("got: %s from the cache", nsDependentCString(buf, size).get()));

        const char* beginning = buf;
        const char* end = strchr(beginning, ';');
        while (end) {
            nsCString filename(beginning, end - beginning);
            beginning = end + 1;
            if (!(end = strchr(beginning, ';'))) {
                break;
            }
            nsCString faceList(beginning, end - beginning);
            beginning = end + 1;
            if (!(end = strchr(beginning, ';'))) {
                break;
            }
            uint32_t timestamp = strtoul(beginning, nullptr, 10);
            beginning = end + 1;
            if (!(end = strchr(beginning, ';'))) {
                break;
            }
            uint32_t filesize = strtoul(beginning, nullptr, 10);

            FNCMapEntry* mapEntry = static_cast<FNCMapEntry*>
                (PL_DHashTableAdd(&mMap, filename.get(), fallible));
            if (mapEntry) {
                mapEntry->mFilename.Assign(filename);
                mapEntry->mTimestamp = timestamp;
                mapEntry->mFilesize = filesize;
                mapEntry->mFaces.Assign(faceList);
                
                
                mapEntry->mFileExists = false;
            }

            beginning = end + 1;
            end = strchr(beginning, ';');
        }

        
        free(buf);
    }

    virtual void
    GetInfoForFile(const nsCString& aFileName, nsCString& aFaceList,
                   uint32_t *aTimestamp, uint32_t *aFilesize)
    {
        if (!mMap.IsInitialized()) {
            return;
        }
        FNCMapEntry *entry =
            static_cast<FNCMapEntry*>(PL_DHashTableSearch(&mMap,
                                                          aFileName.get()));
        if (entry) {
            *aTimestamp = entry->mTimestamp;
            *aFilesize = entry->mFilesize;
            aFaceList.Assign(entry->mFaces);
            
            
            
            entry->mFileExists = true;
        }
    }

    virtual void
    CacheFileInfo(const nsCString& aFileName, const nsCString& aFaceList,
                  uint32_t aTimestamp, uint32_t aFilesize)
    {
        if (!mMap.IsInitialized()) {
            return;
        }
        FNCMapEntry* entry = static_cast<FNCMapEntry*>
            (PL_DHashTableAdd(&mMap, aFileName.get(), fallible));
        if (entry) {
            entry->mFilename.Assign(aFileName);
            entry->mTimestamp = aTimestamp;
            entry->mFilesize = aFilesize;
            entry->mFaces.Assign(aFaceList);
            entry->mFileExists = true;
        }
        mWriteNeeded = true;
    }

private:
    mozilla::scache::StartupCache* mCache;
    PLDHashTable mMap;
    bool mWriteNeeded;

    PLDHashTableOps mOps;

    static PLDHashOperator WriteOutMap(PLDHashTable *aTable,
                                       PLDHashEntryHdr *aHdr,
                                       uint32_t aNumber, void *aData)
    {
        FNCMapEntry* entry = static_cast<FNCMapEntry*>(aHdr);
        if (!entry->mFileExists) {
            
            return PL_DHASH_NEXT;
        }

        nsAutoCString* buf = reinterpret_cast<nsAutoCString*>(aData);
        buf->Append(entry->mFilename);
        buf->Append(';');
        buf->Append(entry->mFaces);
        buf->Append(';');
        buf->AppendInt(entry->mTimestamp);
        buf->Append(';');
        buf->AppendInt(entry->mFilesize);
        buf->Append(';');
        return PL_DHASH_NEXT;
    }

    typedef struct : public PLDHashEntryHdr {
    public:
        nsCString mFilename;
        uint32_t  mTimestamp;
        uint32_t  mFilesize;
        nsCString mFaces;
        bool      mFileExists;
    } FNCMapEntry;

    static PLDHashNumber StringHash(PLDHashTable *table, const void *key)
    {
        return HashString(reinterpret_cast<const char*>(key));
    }

    static bool HashMatchEntry(PLDHashTable *table,
                                 const PLDHashEntryHdr *aHdr, const void *key)
    {
        const FNCMapEntry* entry =
            static_cast<const FNCMapEntry*>(aHdr);
        return entry->mFilename.Equals(reinterpret_cast<const char*>(key));
    }

    static void MoveEntry(PLDHashTable *table, const PLDHashEntryHdr *aFrom,
                          PLDHashEntryHdr *aTo)
    {
        FNCMapEntry* to = static_cast<FNCMapEntry*>(aTo);
        const FNCMapEntry* from = static_cast<const FNCMapEntry*>(aFrom);
        to->mFilename.Assign(from->mFilename);
        to->mTimestamp = from->mTimestamp;
        to->mFilesize = from->mFilesize;
        to->mFaces.Assign(from->mFaces);
        to->mFileExists = from->mFileExists;
    }
};










gfxFT2FontList::gfxFT2FontList()
{
}

void
gfxFT2FontList::AppendFacesFromCachedFaceList(
    const nsCString& aFileName,
    const nsCString& aFaceList,
    StandardFile aStdFile,
    FT2FontFamily::Visibility aVisibility)
{
    const char *beginning = aFaceList.get();
    const char *end = strchr(beginning, ',');
    while (end) {
        nsString familyName =
            NS_ConvertUTF8toUTF16(beginning, end - beginning);
        ToLowerCase(familyName);
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        nsString faceName =
            NS_ConvertUTF8toUTF16(beginning, end - beginning);
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        uint32_t index = strtoul(beginning, nullptr, 10);
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        bool italic = (*beginning != '0');
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        uint32_t weight = strtoul(beginning, nullptr, 10);
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        int32_t stretch = strtol(beginning, nullptr, 10);

        FontListEntry fle(familyName, faceName, aFileName,
                          weight, stretch, italic, index,
                          aVisibility == FT2FontFamily::kHidden);
        AppendFaceFromFontListEntry(fle, aStdFile);

        beginning = end + 1;
        end = strchr(beginning, ',');
    }
}

static void
AppendToFaceList(nsCString& aFaceList,
                 nsAString& aFamilyName, FT2FontEntry* aFontEntry)
{
    aFaceList.Append(NS_ConvertUTF16toUTF8(aFamilyName));
    aFaceList.Append(',');
    aFaceList.Append(NS_ConvertUTF16toUTF8(aFontEntry->Name()));
    aFaceList.Append(',');
    aFaceList.AppendInt(aFontEntry->mFTFontIndex);
    aFaceList.Append(',');
    aFaceList.Append(aFontEntry->IsItalic() ? '1' : '0');
    aFaceList.Append(',');
    aFaceList.AppendInt(aFontEntry->Weight());
    aFaceList.Append(',');
    aFaceList.AppendInt(aFontEntry->Stretch());
    aFaceList.Append(',');
}

void
FT2FontEntry::CheckForBrokenFont(gfxFontFamily *aFamily)
{
    
    if (aFamily->IsBadUnderlineFamily()) {
        mIsBadUnderlineFont = true;
    }

    
    
    
    if (aFamily->Name().EqualsLiteral("roboto")) {
        mIgnoreGSUB = true;
    }

    
    
    
    else if (aFamily->Name().EqualsLiteral("droid sans arabic")) {
        AutoFTFace face(this);
        if (face) {
            const TT_Header *head = static_cast<const TT_Header*>
                (FT_Get_Sfnt_Table(face, ft_sfnt_head));
            if (head && head->CheckSum_Adjust == 0xe445242) {
                mIgnoreGSUB = true;
            }
        }
    }
}

void
gfxFT2FontList::AppendFacesFromFontFile(const nsCString& aFileName,
                                        FontNameCache *aCache,
                                        StandardFile aStdFile,
                                        FT2FontFamily::Visibility aVisibility)
{
    nsCString faceList;
    uint32_t filesize = 0, timestamp = 0;
    if (aCache) {
        aCache->GetInfoForFile(aFileName, faceList, &timestamp, &filesize);
    }

    struct stat s;
    int statRetval = stat(aFileName.get(), &s);
    if (!faceList.IsEmpty() && 0 == statRetval &&
        s.st_mtime == timestamp && s.st_size == filesize)
    {
        LOG(("using cached font info for %s", aFileName.get()));
        AppendFacesFromCachedFaceList(aFileName, faceList, aStdFile,
                                      aVisibility);
        return;
    }

    FT_Library ftLibrary = gfxAndroidPlatform::GetPlatform()->GetFTLibrary();
    FT_Face dummy;
    if (FT_Err_Ok == FT_New_Face(ftLibrary, aFileName.get(), -1, &dummy)) {
        LOG(("reading font info via FreeType for %s", aFileName.get()));
        nsCString faceList;
        timestamp = s.st_mtime;
        filesize = s.st_size;
        for (FT_Long i = 0; i < dummy->num_faces; i++) {
            FT_Face face;
            if (FT_Err_Ok != FT_New_Face(ftLibrary, aFileName.get(), i, &face)) {
                continue;
            }
            AddFaceToList(aFileName, i, aStdFile, aVisibility, face, faceList);
            FT_Done_Face(face);
        }
        FT_Done_Face(dummy);
        if (aCache && 0 == statRetval && !faceList.IsEmpty()) {
            aCache->CacheFileInfo(aFileName, faceList, timestamp, filesize);
        }
    }
}

#define JAR_LAST_MODIFED_TIME "jar-last-modified-time"

void
gfxFT2FontList::FindFontsInOmnijar(FontNameCache *aCache)
{
    bool jarChanged = false;

    mozilla::scache::StartupCache* cache =
        mozilla::scache::StartupCache::GetSingleton();
    char *cachedModifiedTimeBuf;
    uint32_t longSize;
    int64_t jarModifiedTime;
    if (cache &&
        NS_SUCCEEDED(cache->GetBuffer(JAR_LAST_MODIFED_TIME,
                                      &cachedModifiedTimeBuf,
                                      &longSize)) &&
        longSize == sizeof(int64_t))
    {
        nsCOMPtr<nsIFile> jarFile = Omnijar::GetPath(Omnijar::Type::GRE);
        jarFile->GetLastModifiedTime(&jarModifiedTime);
        if (jarModifiedTime > *(int64_t*)cachedModifiedTimeBuf) {
            jarChanged = true;
        }
    }

    static const char* sJarSearchPaths[] = {
        "res/fonts/*.ttf$",
    };
    nsRefPtr<nsZipArchive> reader = Omnijar::GetReader(Omnijar::Type::GRE);
    for (unsigned i = 0; i < ArrayLength(sJarSearchPaths); i++) {
        nsZipFind* find;
        if (NS_SUCCEEDED(reader->FindInit(sJarSearchPaths[i], &find))) {
            const char* path;
            uint16_t len;
            while (NS_SUCCEEDED(find->FindNext(&path, &len))) {
                nsCString entryName(path, len);
                AppendFacesFromOmnijarEntry(reader, entryName, aCache,
                                            jarChanged);
            }
            delete find;
        }
    }

    if (cache) {
        cache->PutBuffer(JAR_LAST_MODIFED_TIME, (char*)&jarModifiedTime,
                         sizeof(jarModifiedTime));
    }
}



void
gfxFT2FontList::AddFaceToList(const nsCString& aEntryName, uint32_t aIndex,
                              StandardFile aStdFile,
                              FT2FontFamily::Visibility aVisibility,
                              FT_Face aFace,
                              nsCString& aFaceList)
{
    if (FT_Err_Ok != FT_Select_Charmap(aFace, FT_ENCODING_UNICODE)) {
        
        return;
    }

    
    
    FT2FontEntry* fe =
        CreateNamedFontEntry(aFace, aEntryName.get(), aIndex);

    auto& fontFamilies =
        (aVisibility == FT2FontFamily::kHidden) ? mHiddenFontFamilies :
                                                  mFontFamilies;

    if (fe) {
        NS_ConvertUTF8toUTF16 name(aFace->family_name);
        BuildKeyNameFromFontName(name);
        gfxFontFamily *family = fontFamilies.GetWeak(name);
        if (!family) {
            family = new FT2FontFamily(name);
            fontFamilies.Put(name, family);
            if (mSkipSpaceLookupCheckFamilies.Contains(name)) {
                family->SetSkipSpaceFeatureCheck(true);
            }
            if (mBadUnderlineFamilyNames.Contains(name)) {
                family->SetBadUnderlineFamily();
            }
        }
        fe->mStandardFace = (aStdFile == kStandard);
        family->AddFontEntry(fe);

        fe->CheckForBrokenFont(family);

        AppendToFaceList(aFaceList, name, fe);
#ifdef PR_LOGGING
        if (LOG_ENABLED()) {
            LOG(("(fontinit) added (%s) to family (%s)"
                 " with style: %s weight: %d stretch: %d",
                 NS_ConvertUTF16toUTF8(fe->Name()).get(),
                 NS_ConvertUTF16toUTF8(family->Name()).get(),
                 fe->IsItalic() ? "italic" : "normal",
                 fe->Weight(), fe->Stretch()));
        }
#endif
    }
}

void
gfxFT2FontList::AppendFacesFromOmnijarEntry(nsZipArchive* aArchive,
                                            const nsCString& aEntryName,
                                            FontNameCache *aCache,
                                            bool aJarChanged)
{
    nsCString faceList;
    if (aCache && !aJarChanged) {
        uint32_t filesize, timestamp;
        aCache->GetInfoForFile(aEntryName, faceList, &timestamp, &filesize);
        if (faceList.Length() > 0) {
            AppendFacesFromCachedFaceList(aEntryName, faceList);
            return;
        }
    }

    nsZipItem *item = aArchive->GetItem(aEntryName.get());
    NS_ASSERTION(item, "failed to find zip entry");

    uint32_t bufSize = item->RealSize();
    
    
    nsAutoArrayPtr<uint8_t> buf(new (fallible) uint8_t[bufSize]);
    if (!buf) {
        return;
    }

    nsZipCursor cursor(item, aArchive, buf, bufSize);
    uint8_t* data = cursor.Copy(&bufSize);
    NS_ASSERTION(data && bufSize == item->RealSize(),
                 "error reading bundled font");
    if (!data) {
        return;
    }

    FT_Library ftLibrary = gfxAndroidPlatform::GetPlatform()->GetFTLibrary();

    FT_Face dummy;
    if (FT_Err_Ok != FT_New_Memory_Face(ftLibrary, buf, bufSize, 0, &dummy)) {
        return;
    }

    for (FT_Long i = 0; i < dummy->num_faces; i++) {
        FT_Face face;
        if (FT_Err_Ok != FT_New_Memory_Face(ftLibrary, buf, bufSize, i, &face)) {
            continue;
        }
        AddFaceToList(aEntryName, i, kStandard, FT2FontFamily::kVisible,
                      face, faceList);
        FT_Done_Face(face);
    }

    FT_Done_Face(dummy);

    if (aCache && !faceList.IsEmpty()) {
        aCache->CacheFileInfo(aEntryName, faceList, 0, bufSize);
    }
}




static PLDHashOperator
FinalizeFamilyMemberList(nsStringHashKey::KeyType aKey,
                         nsRefPtr<gfxFontFamily>& aFamily,
                         void* aUserArg)
{
    gfxFontFamily *family = aFamily.get();
    bool sortFaces = (aUserArg != nullptr);

    family->SetHasStyles(true);

    if (sortFaces) {
        family->SortAvailableFonts();
    }
    family->CheckForSimpleFamily();

    return PL_DHASH_NEXT;
}

void
gfxFT2FontList::FindFonts()
{
    gfxFontCache *fc = gfxFontCache::GetCache();
    if (fc)
        fc->AgeAllGenerations();
    mPrefFonts.Clear();
    mCodepointsWithNoFonts.reset();

    mCodepointsWithNoFonts.SetRange(0,0x1f);     
    mCodepointsWithNoFonts.SetRange(0x7f,0x9f);  

    if (XRE_GetProcessType() != GeckoProcessType_Default) {
        
        InfallibleTArray<FontListEntry> fonts;
        mozilla::dom::ContentChild::GetSingleton()->SendReadFontList(&fonts);
        for (uint32_t i = 0, n = fonts.Length(); i < n; ++i) {
            
            
            AppendFaceFromFontListEntry(fonts[i], kUnknown);
        }
        
        
        
        mFontFamilies.Enumerate(FinalizeFamilyMemberList, nullptr);
        mHiddenFontFamilies.Enumerate(FinalizeFamilyMemberList, nullptr);
        LOG(("got font list from chrome process: %d faces in %d families "
             "and %d in hidden families",
            fonts.Length(), mFontFamilies.Count(),
            mHiddenFontFamilies.Count()));
        return;
    }

    
    FontNameCache fnc;

    
    
    nsCString root;
    char *androidRoot = PR_GetEnv("ANDROID_ROOT");
    if (androidRoot) {
        root = androidRoot;
    } else {
        root = NS_LITERAL_CSTRING("/system");
    }
    root.AppendLiteral("/fonts");

    FindFontsInDir(root, &fnc, FT2FontFamily::kVisible);

    if (mFontFamilies.Count() == 0) {
        
        NS_RUNTIMEABORT("Could not read the system fonts directory");
    }

#ifdef MOZ_WIDGET_GONK
    
    
    root.AppendLiteral("/hidden");
    FindFontsInDir(root, &fnc, FT2FontFamily::kHidden);
#endif

    
    
    
    bool lowmem;
    nsCOMPtr<nsIMemory> mem = nsMemory::GetGlobalMemoryService();
    if ((NS_SUCCEEDED(mem->IsLowMemoryPlatform(&lowmem)) && !lowmem &&
         Preferences::GetBool("gfx.bundled_fonts.enabled")) ||
        Preferences::GetBool("gfx.bundled_fonts.force-enabled")) {
        FindFontsInOmnijar(&fnc);
    }

    
    nsCOMPtr<nsIFile> localDir;
    nsresult rv = NS_GetSpecialDirectory(NS_APP_USER_PROFILE_LOCAL_50_DIR,
                                         getter_AddRefs(localDir));
    if (NS_SUCCEEDED(rv) &&
        NS_SUCCEEDED(localDir->Append(NS_LITERAL_STRING("fonts")))) {
        nsCString localPath;
        rv = localDir->GetNativePath(localPath);
        if (NS_SUCCEEDED(rv)) {
            FindFontsInDir(localPath, &fnc, FT2FontFamily::kVisible);
        }
    }

    
    
    
    mFontFamilies.Enumerate(FinalizeFamilyMemberList, this);
    mHiddenFontFamilies.Enumerate(FinalizeFamilyMemberList, this);
}

void
gfxFT2FontList::FindFontsInDir(const nsCString& aDir,
                               FontNameCache *aFNC,
                               FT2FontFamily::Visibility aVisibility)
{
    static const char* sStandardFonts[] = {
        "DroidSans.ttf",
        "DroidSans-Bold.ttf",
        "DroidSerif-Regular.ttf",
        "DroidSerif-Bold.ttf",
        "DroidSerif-Italic.ttf",
        "DroidSerif-BoldItalic.ttf",
        "DroidSansMono.ttf",
        "DroidSansArabic.ttf",
        "DroidSansHebrew.ttf",
        "DroidSansThai.ttf",
        "MTLmr3m.ttf",
        "MTLc3m.ttf",
        "NanumGothic.ttf",
        "DroidSansJapanese.ttf",
        "DroidSansFallback.ttf"
    };

    DIR *d = opendir(aDir.get());
    if (!d) {
        return;
    }

    struct dirent *ent = nullptr;
    while ((ent = readdir(d)) != nullptr) {
        const char *ext = strrchr(ent->d_name, '.');
        if (!ext) {
            continue;
        }
        if (strcasecmp(ext, ".ttf") == 0 ||
            strcasecmp(ext, ".otf") == 0 ||
            strcasecmp(ext, ".woff") == 0 ||
            strcasecmp(ext, ".ttc") == 0) {
            bool isStdFont = false;
            for (unsigned int i = 0;
                 i < ArrayLength(sStandardFonts) && !isStdFont; i++) {
                isStdFont = strcmp(sStandardFonts[i], ent->d_name) == 0;
            }

            nsCString s(aDir);
            s.Append('/');
            s.Append(ent->d_name);

            
            
            
            
            AppendFacesFromFontFile(s, aFNC, isStdFont ? kStandard : kUnknown,
                                    aVisibility);
        }
    }

    closedir(d);
}

void
gfxFT2FontList::AppendFaceFromFontListEntry(const FontListEntry& aFLE,
                                            StandardFile aStdFile)
{
    FT2FontEntry* fe = FT2FontEntry::CreateFontEntry(aFLE);
    if (fe) {
        auto& fontFamilies =
            aFLE.isHidden() ? mHiddenFontFamilies : mFontFamilies;
        fe->mStandardFace = (aStdFile == kStandard);
        nsAutoString name(aFLE.familyName());
        gfxFontFamily *family = fontFamilies.GetWeak(name);
        if (!family) {
            family = new FT2FontFamily(name);
            fontFamilies.Put(name, family);
            if (mSkipSpaceLookupCheckFamilies.Contains(name)) {
                family->SetSkipSpaceFeatureCheck(true);
            }
            if (mBadUnderlineFamilyNames.Contains(name)) {
                family->SetBadUnderlineFamily();
            }
        }
        family->AddFontEntry(fe);

        fe->CheckForBrokenFont(family);
    }
}

static PLDHashOperator
AddFamilyToFontList(nsStringHashKey::KeyType aKey,
                    nsRefPtr<gfxFontFamily>& aFamily,
                    void* aUserArg)
{
    InfallibleTArray<FontListEntry>* fontlist =
        reinterpret_cast<InfallibleTArray<FontListEntry>*>(aUserArg);

    FT2FontFamily *family = static_cast<FT2FontFamily*>(aFamily.get());
    family->AddFacesToFontList(fontlist, FT2FontFamily::kVisible);

    return PL_DHASH_NEXT;
}

static PLDHashOperator
AddHiddenFamilyToFontList(nsStringHashKey::KeyType aKey,
                          nsRefPtr<gfxFontFamily>& aFamily,
                          void* aUserArg)
{
    InfallibleTArray<FontListEntry>* fontlist =
        reinterpret_cast<InfallibleTArray<FontListEntry>*>(aUserArg);

    FT2FontFamily *family = static_cast<FT2FontFamily*>(aFamily.get());
    family->AddFacesToFontList(fontlist, FT2FontFamily::kHidden);

    return PL_DHASH_NEXT;
}

void
gfxFT2FontList::GetFontList(InfallibleTArray<FontListEntry>* retValue)
{
    mFontFamilies.Enumerate(AddFamilyToFontList, retValue);
    mHiddenFontFamilies.Enumerate(AddHiddenFamilyToFontList, retValue);
}

static void
LoadSkipSpaceLookupCheck(nsTHashtable<nsStringHashKey>& aSkipSpaceLookupCheck)
{
    nsAutoTArray<nsString, 5> skiplist;
    gfxFontUtils::GetPrefsFontList(
        "font.whitelist.skip_default_features_space_check",
        skiplist);
    uint32_t numFonts = skiplist.Length();
    for (uint32_t i = 0; i < numFonts; i++) {
        ToLowerCase(skiplist[i]);
        aSkipSpaceLookupCheck.PutEntry(skiplist[i]);
    }
}

static PLDHashOperator
PreloadAsUserFontFaces(nsStringHashKey::KeyType aKey,
                       nsRefPtr<gfxFontFamily>& aFamily,
                       void* aUserArg)
{
    gfxFontFamily *family = aFamily.get();

    auto& faces = family->GetFontList();
    size_t count = faces.Length();
    for (size_t i = 0; i < count; ++i) {
        FT2FontEntry* fe = static_cast<FT2FontEntry*>(faces[i].get());
        if (fe->mFTFontIndex != 0) {
            NS_NOTREACHED("don't try to preload a multi-face font");
            continue;
        }

        

        
        int fd = open(fe->mFilename.get(), O_RDONLY);
        if (fd < 0) {
            continue;
        }
        struct stat buf;
        if (fstat(fd, &buf) != 0 || buf.st_size < 12) {
            close(fd);
            continue;
        }
        char* data = static_cast<char*>(
            mmap(0, buf.st_size, PROT_READ, MAP_PRIVATE, fd, 0));
        close(fd);
        if (data == MAP_FAILED) {
            continue;
        }

        
        uint32_t crc = crc32(0, nullptr, 0);
        crc = crc32(crc, (Bytef*)data, buf.st_size);
        munmap(data, buf.st_size);

#if 0
        ALOG("\n**** Preloading family [%s] face [%s] CRC32 [0x%08x]",
             NS_ConvertUTF16toUTF8(family->Name()).get(),
             fe->mFilename.get(),
             crc);
#endif

        fe->mUserFontData = new gfxUserFontData;
        fe->mUserFontData->mRealName = fe->Name();
        fe->mUserFontData->mCRC32 = crc;
        fe->mUserFontData->mLength = buf.st_size;

        
        gfxUserFontSet::UserFontCache::CacheFont(
            fe, gfxUserFontSet::UserFontCache::kPersistent);
    }

    return PL_DHASH_NEXT;
}

nsresult
gfxFT2FontList::InitFontList()
{
    
    gfxPlatformFontList::InitFontList();
    mHiddenFontFamilies.Clear();
    
    LoadSkipSpaceLookupCheck(mSkipSpaceLookupCheckFamilies);

    FindFonts();

    mHiddenFontFamilies.Enumerate(PreloadAsUserFontFaces, this);

    return NS_OK;
}

struct FullFontNameSearch {
    FullFontNameSearch(const nsAString& aFullName)
        : mFullName(aFullName), mFontEntry(nullptr)
    { }

    nsString     mFullName;
    FT2FontEntry *mFontEntry;
};



static PLDHashOperator
FindFullName(nsStringHashKey::KeyType aKey,
             nsRefPtr<gfxFontFamily>& aFontFamily,
             void* userArg)
{
    FullFontNameSearch *data = reinterpret_cast<FullFontNameSearch*>(userArg);

    
    const nsString& family = aFontFamily->Name();

    nsString fullNameFamily;
    data->mFullName.Left(fullNameFamily, family.Length());

    
    if (family.Equals(fullNameFamily, nsCaseInsensitiveStringComparator())) {
        nsTArray<nsRefPtr<gfxFontEntry> >& fontList = aFontFamily->GetFontList();
        int index, len = fontList.Length();
        for (index = 0; index < len; index++) {
            gfxFontEntry* fe = fontList[index];
            if (!fe) {
                continue;
            }
            if (fe->Name().Equals(data->mFullName,
                                  nsCaseInsensitiveStringComparator())) {
                data->mFontEntry = static_cast<FT2FontEntry*>(fe);
                return PL_DHASH_STOP;
            }
        }
    }

    return PL_DHASH_NEXT;
}

gfxFontEntry* 
gfxFT2FontList::LookupLocalFont(const nsAString& aFontName,
                                uint16_t aWeight,
                                int16_t aStretch,
                                bool aItalic)
{
    
    FullFontNameSearch data(aFontName);

    
    
    mFontFamilies.Enumerate(FindFullName, &data);

    if (!data.mFontEntry) {
        return nullptr;
    }

    
    

    
    data.mFontEntry->CairoFontFace();
    if (!data.mFontEntry->mFTFace) {
        return nullptr;
    }

    FT2FontEntry* fe =
        FT2FontEntry::CreateFontEntry(data.mFontEntry->mFTFace,
                                      data.mFontEntry->mFilename.get(),
                                      data.mFontEntry->mFTFontIndex,
                                      data.mFontEntry->Name(), nullptr);
    if (fe) {
        fe->mItalic = aItalic;
        fe->mWeight = aWeight;
        fe->mStretch = aStretch;
        fe->mIsLocalUserFont = true;
    }

    return fe;
}

gfxFontFamily*
gfxFT2FontList::GetDefaultFont(const gfxFontStyle* aStyle)
{
    gfxFontFamily *ff = nullptr;
#ifdef MOZ_WIDGET_GONK
    ff = FindFamily(NS_LITERAL_STRING("Fira Sans"));
#elif defined(MOZ_WIDGET_ANDROID)
    ff = FindFamily(NS_LITERAL_STRING("Roboto"));
    if (!ff) {
        ff = FindFamily(NS_LITERAL_STRING("Droid Sans"));
    }
#endif
    
    return ff;
}

gfxFontEntry*
gfxFT2FontList::MakePlatformFont(const nsAString& aFontName,
                                 uint16_t aWeight,
                                 int16_t aStretch,
                                 bool aItalic,
                                 const uint8_t* aFontData,
                                 uint32_t aLength)
{
    
    
    
    return FT2FontEntry::CreateFontEntry(aFontName, aWeight, aStretch,
                                         aItalic, aFontData, aLength);
}

static PLDHashOperator
AppendFamily(nsStringHashKey::KeyType aKey,
             nsRefPtr<gfxFontFamily>& aFamily,
             void* aUserArg)
{
    nsTArray<nsRefPtr<gfxFontFamily> > * familyArray =
        reinterpret_cast<nsTArray<nsRefPtr<gfxFontFamily>>*>(aUserArg);

    familyArray->AppendElement(aFamily);
    return PL_DHASH_NEXT;
}

void
gfxFT2FontList::GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray)
{
    mFontFamilies.Enumerate(AppendFamily, &aFamilyArray);
    mHiddenFontFamilies.Enumerate(AppendFamily, &aFamilyArray);
}
