








































#ifdef ANDROID
#include "mozilla/dom/ContentChild.h"
#include "nsXULAppAPI.h"

#include "gfxAndroidPlatform.h"
#include <dirent.h>
#include <android/log.h>
#define ALOG(args...)  __android_log_print(ANDROID_LOG_INFO, "Gecko" , ## args)
#endif

#include "gfxFT2FontList.h"
#include "gfxUserFontSet.h"
#include "gfxFontUtils.h"

#include "ft2build.h"
#include FT_FREETYPE_H
#include "gfxFT2Fonts.h"

#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"

#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsISimpleEnumerator.h"

#include "mozilla/scache/StartupCache.h"
#include <sys/stat.h>

#ifdef XP_WIN
#include "nsIWindowsRegKey.h"
#include <windows.h>
#endif

#ifdef PR_LOGGING
static PRLogModuleInfo *gFontInfoLog = PR_NewLogModule("fontInfoLog");
#endif 

#define LOG(args) PR_LOG(gFontInfoLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gFontInfoLog, PR_LOG_DEBUG)

static __inline void
BuildKeyNameFromFontName(nsAString &aName)
{
#ifdef XP_WIN
    if (aName.Length() >= LF_FACESIZE)
        aName.Truncate(LF_FACESIZE - 1);
#endif
    ToLowerCase(aName);
}

#define CACHE_KEY "font.cached-list"

class FontNameCache {
public:
    FontNameCache()
        : mWriteNeeded(PR_FALSE)
    {
        mOps = (PLDHashTableOps) {
            PL_DHashAllocTable,
            PL_DHashFreeTable,
            StringHash,
            HashMatchEntry,
            MoveEntry,
            PL_DHashClearEntryStub,
            PL_DHashFinalizeStub,
            NULL
        };

        if (!PL_DHashTableInit(&mMap, &mOps, nsnull,
                               sizeof(FNCMapEntry), 0))
        {
            mMap.ops = nsnull;
            LOG(("initializing the map failed"));
        }

        NS_ABORT_IF_FALSE(XRE_GetProcessType() == GeckoProcessType_Default,
                          "StartupCacheFontNameCache should only be used in chrome process");
        mCache = mozilla::scache::StartupCache::GetSingleton();

        Init();
    }

    ~FontNameCache()
    {
        if (!mMap.ops) {
            return;
        }
        if (!mWriteNeeded || !mCache) {
            PL_DHashTableFinish(&mMap);
            return;
        }

        nsCAutoString buf;
        PL_DHashTableEnumerate(&mMap, WriteOutMap, &buf);
        PL_DHashTableFinish(&mMap);
        mCache->PutBuffer(CACHE_KEY, buf.get(), buf.Length() + 1);
    }

    void Init()
    {
        if (!mMap.ops || !mCache) {
            return;
        }
        PRUint32 size;
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
            PRUint32 timestamp = strtoul(beginning, NULL, 10);
            beginning = end + 1;
            if (!(end = strchr(beginning, ';'))) {
                break;
            }
            PRUint32 filesize = strtoul(beginning, NULL, 10);

            FNCMapEntry* mapEntry =
                static_cast<FNCMapEntry*>
                (PL_DHashTableOperate(&mMap, filename.get(), PL_DHASH_ADD));
            if (mapEntry) {
                mapEntry->mFilename.Assign(filename);
                mapEntry->mTimestamp = timestamp;
                mapEntry->mFilesize = filesize;
                mapEntry->mFaces.Assign(faceList);
                
                
                mapEntry->mFileExists = PR_FALSE;
            }

            beginning = end + 1;
            end = strchr(beginning, ';');
        }

        
        free(buf);
    }

    virtual void
    GetInfoForFile(nsCString& aFileName, nsCString& aFaceList,
                   PRUint32 *aTimestamp, PRUint32 *aFilesize)
    {
        if (!mMap.ops) {
            return;
        }
        PLDHashEntryHdr *hdr =
            PL_DHashTableOperate(&mMap, aFileName.get(), PL_DHASH_LOOKUP);
        if (!hdr) {
            return;
        }
        FNCMapEntry* entry = static_cast<FNCMapEntry*>(hdr);
        if (entry && entry->mTimestamp && entry->mFilesize) {
            *aTimestamp = entry->mTimestamp;
            *aFilesize = entry->mFilesize;
            aFaceList.Assign(entry->mFaces);
            
            
            
            entry->mFileExists = PR_TRUE;
        }
    }

    virtual void
    CacheFileInfo(nsCString& aFileName, nsCString& aFaceList,
                  PRUint32 aTimestamp, PRUint32 aFilesize)
    {
        if (!mMap.ops) {
            return;
        }
        FNCMapEntry* entry =
            static_cast<FNCMapEntry*>
            (PL_DHashTableOperate(&mMap, aFileName.get(), PL_DHASH_ADD));
        if (entry) {
            entry->mFilename.Assign(aFileName);
            entry->mTimestamp = aTimestamp;
            entry->mFilesize = aFilesize;
            entry->mFaces.Assign(aFaceList);
            entry->mFileExists = PR_TRUE;
        }
        mWriteNeeded = PR_TRUE;
    }

private:
    mozilla::scache::StartupCache* mCache;
    PLDHashTable mMap;
    PRBool mWriteNeeded;

    PLDHashTableOps mOps;

    static PLDHashOperator WriteOutMap(PLDHashTable *aTable,
                                       PLDHashEntryHdr *aHdr,
                                       PRUint32 aNumber, void *aData)
    {
        FNCMapEntry* entry = static_cast<FNCMapEntry*>(aHdr);
        if (!entry->mFileExists) {
            
            return PL_DHASH_NEXT;
        }

        nsCAutoString* buf = reinterpret_cast<nsCAutoString*>(aData);
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
        PRUint32  mTimestamp;
        PRUint32  mFilesize;
        nsCString mFaces;
        PRBool    mFileExists;
    } FNCMapEntry;

    static PLDHashNumber StringHash(PLDHashTable *table, const void *key)
    {
        return HashString(reinterpret_cast<const char*>(key));
    }

    static PRBool HashMatchEntry(PLDHashTable *table,
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
gfxFT2FontList::AppendFacesFromCachedFaceList(nsCString& aFileName,
                                              PRBool aStdFile,
                                              nsCString& aFaceList)
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
        PRUint32 index = strtoul(beginning, NULL, 10);
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        PRBool italic = (*beginning != '0');
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        PRUint32 weight = strtoul(beginning, NULL, 10);
        beginning = end + 1;
        if (!(end = strchr(beginning, ','))) {
            break;
        }
        PRInt32 stretch = strtol(beginning, NULL, 10);

        FontListEntry fle(familyName, faceName, aFileName,
                          weight, stretch, italic, index);
        AppendFaceFromFontListEntry(fle, aStdFile);

        beginning = end + 1;
        end = strchr(beginning, ',');
    }
}

static void
AppendToFaceList(nsCString& aFaceList,
                 nsAString& aFamilyName, FontEntry* aFontEntry)
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
gfxFT2FontList::AppendFacesFromFontFile(nsCString& aFileName,
                                        PRBool aStdFile,
                                        FontNameCache *aCache)
{
    nsCString faceList;
    PRUint32 filesize = 0, timestamp = 0;
    if (aCache) {
        aCache->GetInfoForFile(aFileName, faceList, &timestamp, &filesize);
    }

    struct stat s;
    int statRetval = stat(aFileName.get(), &s);
    if (!faceList.IsEmpty() && 0 == statRetval &&
        s.st_mtime == timestamp && s.st_size == filesize)
    {
        LOG(("using cached font info for %s", aFileName.get()));
        AppendFacesFromCachedFaceList(aFileName, aStdFile, faceList);
        return;
    }

#ifdef XP_WIN
    FT_Library ftLibrary = gfxWindowsPlatform::GetPlatform()->GetFTLibrary();
#elif defined(ANDROID)
    FT_Library ftLibrary = gfxAndroidPlatform::GetPlatform()->GetFTLibrary();
#endif
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

            FontEntry* fe = FontEntry::CreateFontEntry(face, aFileName.get(), i);
            if (fe) {
                NS_ConvertUTF8toUTF16 name(face->family_name);
                BuildKeyNameFromFontName(name);       
                gfxFontFamily *family = mFontFamilies.GetWeak(name);
                if (!family) {
                    family = new gfxFontFamily(name);
                    mFontFamilies.Put(name, family);
                    if (mBadUnderlineFamilyNames.Contains(name)) {
                        family->SetBadUnderlineFamily();
                    }
                }
                fe->mStandardFace = aStdFile;
                family->AddFontEntry(fe);
                if (family->IsBadUnderlineFamily()) {
                    fe->mIsBadUnderlineFont = PR_TRUE;
                }
                AppendToFaceList(faceList, name, fe);
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
        FT_Done_Face(dummy);
        if (aCache && 0 == statRetval && !faceList.IsEmpty()) {
            aCache->CacheFileInfo(aFileName, faceList, timestamp, filesize);
        }
    }
}




static PLDHashOperator
FinalizeFamilyMemberList(nsStringHashKey::KeyType aKey,
                         nsRefPtr<gfxFontFamily>& aFamily,
                         void* aUserArg)
{
    gfxFontFamily *family = aFamily.get();
    PRBool sortFaces = (aUserArg != nsnull);

    family->SetHasStyles(PR_TRUE);

    if (sortFaces) {
        family->SortAvailableFonts();
    }
    family->CheckForSimpleFamily();

    return PL_DHASH_NEXT;
}

void
gfxFT2FontList::FindFonts()
{
#ifdef XP_WIN
    nsTArray<nsString> searchPaths(3);
    nsTArray<nsString> fontPatterns(3);
    fontPatterns.AppendElement(NS_LITERAL_STRING("\\*.ttf"));
    fontPatterns.AppendElement(NS_LITERAL_STRING("\\*.ttc"));
    fontPatterns.AppendElement(NS_LITERAL_STRING("\\*.otf"));
    wchar_t pathBuf[256];
    SHGetSpecialFolderPathW(0, pathBuf, CSIDL_WINDOWS, 0);
    searchPaths.AppendElement(pathBuf);
    SHGetSpecialFolderPathW(0, pathBuf, CSIDL_FONTS, 0);
    searchPaths.AppendElement(pathBuf);
    nsCOMPtr<nsIFile> resDir;
    NS_GetSpecialDirectory(NS_APP_RES_DIR, getter_AddRefs(resDir));
    if (resDir) {
        resDir->Append(NS_LITERAL_STRING("fonts"));
        nsAutoString resPath;
        resDir->GetPath(resPath);
        searchPaths.AppendElement(resPath);
    }
    WIN32_FIND_DATAW results;
    for (PRUint32 i = 0;  i < searchPaths.Length(); i++) {
        const nsString& path(searchPaths[i]);
        for (PRUint32 j = 0; j < fontPatterns.Length(); j++) { 
            nsAutoString pattern(path);
            pattern.Append(fontPatterns[j]);
            HANDLE handle = FindFirstFileExW(pattern.get(),
                                             FindExInfoStandard,
                                             &results,
                                             FindExSearchNameMatch,
                                             NULL,
                                             0);
            PRBool moreFiles = handle != INVALID_HANDLE_VALUE;
            while (moreFiles) {
                nsAutoString filePath(path);
                filePath.AppendLiteral("\\");
                filePath.Append(results.cFileName);
                AppendFacesFromFontFile(NS_ConvertUTF16toUTF8(filePath));
                moreFiles = FindNextFile(handle, &results);
            }
            if (handle != INVALID_HANDLE_VALUE)
                FindClose(handle);
        }
    }
#elif defined(ANDROID)
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
        for (PRUint32 i = 0, n = fonts.Length(); i < n; ++i) {
            AppendFaceFromFontListEntry(fonts[i], PR_FALSE);
        }
        
        
        
        mFontFamilies.Enumerate(FinalizeFamilyMemberList, nsnull);
        LOG(("got font list from chrome process: %d faces in %d families",
            fonts.Length(), mFontFamilies.Count()));
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
    root.Append("/fonts");

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
        "DroidSansJapanese.ttf",
        "DroidSansFallback.ttf"
    };

    DIR *d = opendir(root.get());
    if (!d) {
        
        NS_RUNTIMEABORT("Could not read the system fonts directory");
    }

    struct dirent *ent = NULL;
    while ((ent = readdir(d)) != NULL) {
        int namelen = strlen(ent->d_name);
        if (namelen <= 4) {
            
            continue;
        }
        const char *ext = ent->d_name + namelen - 4;
        if (strcasecmp(ext, ".ttf") == 0 ||
            strcasecmp(ext, ".otf") == 0 ||
            strcasecmp(ext, ".ttc") == 0)
        {
            bool isStdFont = false;
            for (unsigned int i = 0;
                 i < NS_ARRAY_LENGTH(sStandardFonts) && !isStdFont; i++)
            {
                isStdFont = strcmp(sStandardFonts[i], ent->d_name) == 0;
            }

            nsCString s(root);
            s.Append('/');
            s.Append(ent->d_name, namelen);

            
            
            
            
            AppendFacesFromFontFile(s, isStdFont, &fnc);
        }
    }
    closedir(d);

    
    
    
    mFontFamilies.Enumerate(FinalizeFamilyMemberList, this);
#endif 
}

void
gfxFT2FontList::AppendFaceFromFontListEntry(const FontListEntry& aFLE,
                                            PRBool aStdFile)
{
    FontEntry* fe = FontEntry::CreateFontEntry(aFLE);
    if (fe) {
        fe->mStandardFace = aStdFile;
        nsAutoString name(aFLE.familyName());
        gfxFontFamily *family = mFontFamilies.GetWeak(name);
        if (!family) {
            family = new gfxFontFamily(name);
            mFontFamilies.Put(name, family);
            if (mBadUnderlineFamilyNames.Contains(name)) {
                family->SetBadUnderlineFamily();
            }
        }
        family->AddFontEntry(fe);
        if (family->IsBadUnderlineFamily()) {
            fe->mIsBadUnderlineFont = PR_TRUE;
        }
    }
}

static PLDHashOperator
AddFamilyToFontList(nsStringHashKey::KeyType aKey,
                    nsRefPtr<gfxFontFamily>& aFamily,
                    void* aUserArg)
{
    InfallibleTArray<FontListEntry>* fontlist =
        reinterpret_cast<InfallibleTArray<FontListEntry>*>(aUserArg);

    FontFamily *family = static_cast<FontFamily*>(aFamily.get());
    family->AddFacesToFontList(fontlist);

    return PL_DHASH_NEXT;
}

void
gfxFT2FontList::GetFontList(InfallibleTArray<FontListEntry>* retValue)
{
    mFontFamilies.Enumerate(AddFamilyToFontList, retValue);
}

nsresult
gfxFT2FontList::InitFontList()
{
    
    gfxPlatformFontList::InitFontList();
    
    FindFonts();

    return NS_OK;
}

struct FullFontNameSearch {
    FullFontNameSearch(const nsAString& aFullName)
        : mFullName(aFullName), mFontEntry(nsnull)
    { }

    nsString     mFullName;
    gfxFontEntry *mFontEntry;
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

    
    if (family.Equals(fullNameFamily)) {
        nsTArray<nsRefPtr<gfxFontEntry> >& fontList = aFontFamily->GetFontList();
        int index, len = fontList.Length();
        for (index = 0; index < len; index++) {
            if (fontList[index]->Name().Equals(data->mFullName)) {
                data->mFontEntry = fontList[index];
                return PL_DHASH_STOP;
            }
        }
    }

    return PL_DHASH_NEXT;
}

gfxFontEntry* 
gfxFT2FontList::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                       const nsAString& aFontName)
{
    
    FullFontNameSearch data(aFontName);

    mFontFamilies.Enumerate(FindFullName, &data);

    return data.mFontEntry;
}

gfxFontEntry*
gfxFT2FontList::GetDefaultFont(const gfxFontStyle* aStyle, PRBool& aNeedsBold)
{
#ifdef XP_WIN
    HGDIOBJ hGDI = ::GetStockObject(SYSTEM_FONT);
    LOGFONTW logFont;
    if (hGDI && ::GetObjectW(hGDI, sizeof(logFont), &logFont)) {
        nsAutoString resolvedName;
        if (ResolveFontName(nsDependentString(logFont.lfFaceName), resolvedName)) {
            return FindFontForFamily(resolvedName, aStyle, aNeedsBold);
        }
    }
#elif defined(ANDROID)
    nsAutoString resolvedName;
    if (ResolveFontName(NS_LITERAL_STRING("Droid Sans"), resolvedName))
        return FindFontForFamily(resolvedName, aStyle, aNeedsBold);
#endif
    
    return nsnull;
}

gfxFontEntry*
gfxFT2FontList::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                 const PRUint8 *aFontData,
                                 PRUint32 aLength)
{
    
    
    
    return FontEntry::CreateFontEntry(*aProxyEntry, aFontData, aLength);
}
