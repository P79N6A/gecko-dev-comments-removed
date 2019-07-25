



































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 

#include "gfxDWriteFontList.h"
#include "gfxDWriteFonts.h"
#include "nsUnicharUtils.h"
#include "nsILocaleService.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch2.h"
#include "nsServiceManagerUtils.h"

#include "gfxGDIFontList.h"

#include "nsIWindowsRegKey.h"

#ifdef PR_LOGGING
static PRLogModuleInfo *gFontInitLog = nsnull;
#define LOG(args) PR_LOG(gFontInitLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() (gFontInitLog) && PR_LOG_TEST(gFontInitLog, PR_LOG_DEBUG)
#endif 





static const PRUint32 kDelayBeforeLoadingFonts = 120 * 1000; 
static const PRUint32 kIntervalBetweenLoadingFonts = 2000;   

static __inline void
BuildKeyNameFromFontName(nsAString &aName)
{
    if (aName.Length() >= LF_FACESIZE)
        aName.Truncate(LF_FACESIZE - 1);
    ToLowerCase(aName);
}




gfxDWriteFontFamily::~gfxDWriteFontFamily()
{
}

void
gfxDWriteFontFamily::FindStyleVariations()
{
    HRESULT hr;
    if (mHasStyles) {
        return;
    }
    mHasStyles = PR_TRUE;

    for (UINT32 i = 0; i < mDWFamily->GetFontCount(); i++) {
        nsRefPtr<IDWriteFont> font;
        hr = mDWFamily->GetFont(i, getter_AddRefs(font));
        if (FAILED(hr)) {
            
            NS_WARNING("Failed to get existing font from family.");
            continue;
        }

        if (font->GetSimulations() & DWRITE_FONT_SIMULATIONS_OBLIQUE) {
            
            continue;
        }

        nsRefPtr<IDWriteLocalizedStrings> names;
        hr = font->GetFaceNames(getter_AddRefs(names));
        if (FAILED(hr)) {
            continue;
        }
        
        BOOL exists;
        nsAutoTArray<WCHAR,32> faceName;
        UINT32 englishIdx = 0;
        hr = names->FindLocaleName(L"en-us", &englishIdx, &exists);
        if (FAILED(hr)) {
            continue;
        }

        if (!exists) {
            
            englishIdx = 0;
        }
        UINT32 length;
        hr = names->GetStringLength(englishIdx, &length);
        if (FAILED(hr)) {
            continue;
        }
        if (!faceName.SetLength(length + 1)) {
            
            continue;
        }

        hr = names->GetString(englishIdx, faceName.Elements(), length + 1);
        if (FAILED(hr)) {
            continue;
        }

        nsString fullID(mName);
        fullID.Append(faceName.Elements());

        



        gfxDWriteFontEntry *fe = 
            new gfxDWriteFontEntry(fullID, font);
        fe->SetFamily(this);

        mAvailableFonts.AppendElement(fe);
    }
    if (!mAvailableFonts.Length()) {
        NS_WARNING("Family with no font faces in it.");
    }

    if (mIsBadUnderlineFamily) {
        SetBadUnderlineFonts();
    }
}

void
gfxDWriteFontFamily::LocalizedName(nsAString &aLocalizedName)
{
    aLocalizedName.AssignLiteral("Unknown Font");
    HRESULT hr;
    nsresult rv;
    nsCOMPtr<nsILocaleService> ls = do_GetService(NS_LOCALESERVICE_CONTRACTID,
                                                  &rv);
    nsCOMPtr<nsILocale> locale;
    rv = ls->GetApplicationLocale(getter_AddRefs(locale));
    nsString localeName;
    if (NS_SUCCEEDED(rv)) {
        rv = locale->GetCategory(NS_LITERAL_STRING(NSILOCALE_MESSAGE), 
                                 localeName);
    }
    if (NS_FAILED(rv)) {
        localeName.AssignLiteral("en-us");
    }

    nsRefPtr<IDWriteLocalizedStrings> names;

    hr = mDWFamily->GetFamilyNames(getter_AddRefs(names));
    if (FAILED(hr)) {
        return;
    }
    UINT32 idx = 0;
    BOOL exists;
    hr = names->FindLocaleName(localeName.BeginReading(),
                               &idx,
                               &exists);
    if (FAILED(hr)) {
        return;
    }
    if (!exists) {
        
        hr = names->FindLocaleName(L"en-us", &idx, &exists);
        if (FAILED(hr)) {
            return;
        }
        if (!exists) {
            
            idx = 0;
        }
    }
    nsAutoTArray<WCHAR, 32> famName;
    UINT32 length;
    
    hr = names->GetStringLength(idx, &length);
    if (FAILED(hr)) {
        return;
    }
    
    if (!famName.SetLength(length + 1)) {
        
        return;
    }

    hr = names->GetString(idx, famName.Elements(), length + 1);
    if (FAILED(hr)) {
        return;
    }

    aLocalizedName = nsDependentString(famName.Elements());
}




gfxDWriteFontEntry::~gfxDWriteFontEntry()
{
}

PRBool
gfxDWriteFontEntry::IsSymbolFont()
{
    if (mFont) {
        return mFont->IsSymbolFont();
    } else {
        return PR_FALSE;
    }
}

nsresult
gfxDWriteFontEntry::GetFontTable(PRUint32 aTableTag,
                                 FallibleTArray<PRUint8> &aBuffer)
{
    gfxDWriteFontList *pFontList = gfxDWriteFontList::PlatformFontList();

    if (mFont && pFontList->UseGDIFontTableAccess()) {
        LOGFONTW logfont = { 0 };
        if (!InitLogFont(mFont, &logfont))
            return NS_ERROR_FAILURE;

        AutoDC dc;
        AutoSelectFont font(dc.GetDC(), &logfont);
        if (font.IsValid()) {
            PRInt32 tableSize =
                ::GetFontData(dc.GetDC(), NS_SWAP32(aTableTag), 0, NULL, NULL);
            if (tableSize != GDI_ERROR) {
                if (aBuffer.SetLength(tableSize)) {
                    ::GetFontData(dc.GetDC(), NS_SWAP32(aTableTag), 0,
                                  aBuffer.Elements(), aBuffer.Length());
                    return NS_OK;
                }
                return NS_ERROR_OUT_OF_MEMORY;
            }
        }
        return NS_ERROR_FAILURE;
    }

    HRESULT hr;
    nsresult rv;
    nsRefPtr<IDWriteFontFace> fontFace;

    rv = CreateFontFace(getter_AddRefs(fontFace));

    if (NS_FAILED(rv)) {
        return rv;
    }

    PRUint8 *tableData;
    PRUint32 len;
    void *tableContext = NULL;
    BOOL exists;
    hr = fontFace->TryGetFontTable(NS_SWAP32(aTableTag),
                                   (const void**)&tableData,
                                   &len,
                                   &tableContext,
                                   &exists);

    if (FAILED(hr) || !exists) {
        return NS_ERROR_FAILURE;
    }
    if (!aBuffer.SetLength(len)) {
        return NS_ERROR_OUT_OF_MEMORY;
    }
    memcpy(aBuffer.Elements(), tableData, len);
    if (tableContext) {
        fontFace->ReleaseFontTable(&tableContext);
    }
    return NS_OK;
}

nsresult
gfxDWriteFontEntry::ReadCMAP()
{
    HRESULT hr;
    nsresult rv;

    
    if (mCmapInitialized)
        return NS_OK;
    mCmapInitialized = PR_TRUE;

    
    if (mFont && gfxDWriteFontList::PlatformFontList()->UseGDIFontTableAccess()) {
        const PRUint32 kCmapTag = TRUETYPE_TAG('c','m','a','p');
        AutoFallibleTArray<PRUint8,16384> buffer;

        if (GetFontTable(kCmapTag, buffer) != NS_OK)
            return NS_ERROR_FAILURE;
        PRUint8 *cmap = buffer.Elements();

        PRPackedBool  unicodeFont = PR_FALSE, symbolFont = PR_FALSE;
        rv = gfxFontUtils::ReadCMAP(cmap, buffer.Length(),
                                    mCharacterMap, mUVSOffset,
                                    unicodeFont, symbolFont);
        mHasCmapTable = NS_SUCCEEDED(rv);
        return rv;
    }

    
    nsRefPtr<IDWriteFontFace> fontFace;
    rv = CreateFontFace(getter_AddRefs(fontFace));

    if (NS_FAILED(rv)) {
        return rv;
    }

    PRUint8 *tableData;
    PRUint32 len;
    void *tableContext = NULL;
    BOOL exists;
    hr = fontFace->TryGetFontTable(DWRITE_MAKE_OPENTYPE_TAG('c', 'm', 'a', 'p'),
                                   (const void**)&tableData,
                                   &len,
                                   &tableContext,
                                   &exists);
    if (FAILED(hr)) {
        return NS_ERROR_FAILURE;
    }

    PRPackedBool isSymbol = fontFace->IsSymbolFont();
    PRPackedBool isUnicode = PR_TRUE;
    if (exists) {
        rv = gfxFontUtils::ReadCMAP(tableData,
                                    len,
                                    mCharacterMap,
                                    mUVSOffset,
                                    isUnicode,
                                    isSymbol);
    }
    fontFace->ReleaseFontTable(tableContext);

    mHasCmapTable = NS_SUCCEEDED(rv);
    return rv;
}

gfxFont *
gfxDWriteFontEntry::CreateFontInstance(const gfxFontStyle* aFontStyle,
                                       PRBool aNeedsBold)
{
    return new gfxDWriteFont(this, aFontStyle, aNeedsBold);
}

nsresult
gfxDWriteFontEntry::CreateFontFace(IDWriteFontFace **aFontFace,
                                   DWRITE_FONT_SIMULATIONS aSimulations)
{
    HRESULT hr;
    if (mFont) {
        hr = mFont->CreateFontFace(aFontFace);
    } else if (mFontFile) {
        IDWriteFontFile *fontFile = mFontFile.get();
        hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
            CreateFontFace(mFaceType,
                           1,
                           &fontFile,
                           0,
                           aSimulations,
                           aFontFace);
    }
    if (FAILED(hr)) {
        return NS_ERROR_FAILURE;
    }
    return NS_OK;
}

PRBool
gfxDWriteFontEntry::InitLogFont(IDWriteFont *aFont, LOGFONTW *aLogFont)
{
    HRESULT hr;

    BOOL isInSystemCollection;
    IDWriteGdiInterop *gdi = 
        gfxDWriteFontList::PlatformFontList()->GetGDIInterop();
    hr = gdi->ConvertFontToLOGFONT(aFont, aLogFont, &isInSystemCollection);
    return (FAILED(hr) ? PR_FALSE : PR_TRUE);
}




gfxDWriteFontList::gfxDWriteFontList()
{
    mFontSubstitutes.Init();
}





gfxFontEntry *
gfxDWriteFontList::GetDefaultFont(const gfxFontStyle *aStyle,
                                  PRBool &aNeedsBold)
{
    nsAutoString resolvedName;

    
    if (ResolveFontName(NS_LITERAL_STRING("Arial"), resolvedName)) {
        return FindFontForFamily(resolvedName, aStyle, aNeedsBold);
    }

    
    NONCLIENTMETRICSW ncm;
    ncm.cbSize = sizeof(ncm);
    BOOL status = ::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 
                                          sizeof(ncm), &ncm, 0);
    if (status) {
        if (ResolveFontName(nsDependentString(ncm.lfMessageFont.lfFaceName),
                            resolvedName)) {
            return FindFontForFamily(resolvedName, aStyle, aNeedsBold);
        }
    }

    return nsnull;
}

gfxFontEntry *
gfxDWriteFontList::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
                                   const nsAString& aFullname)
{
    PRBool found;
    gfxFontEntry *lookup;

    
    if (!mFaceNamesInitialized) {
        InitFaceNameLists();
    }

    
    if (!(lookup = mPostscriptNames.GetWeak(aFullname, &found)) &&
        !(lookup = mFullnames.GetWeak(aFullname, &found))) 
    {
        return nsnull;
    }
    gfxFontEntry *fe = 
        new gfxDWriteFontEntry(lookup->Name(),
                               static_cast<gfxDWriteFontEntry*>(lookup)->mFont,
                               aProxyEntry->Weight(),
                               aProxyEntry->Stretch(),
                               aProxyEntry->IsItalic());

    return fe;
}

gfxFontEntry *
gfxDWriteFontList::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry,
                                    const PRUint8 *aFontData,
                                    PRUint32 aLength)
{
    nsresult rv;
    nsAutoString uniqueName;
    rv = gfxFontUtils::MakeUniqueUserFontName(uniqueName);
    if (NS_FAILED(rv)) {
        NS_Free((void*)aFontData);
        return nsnull;
    }

    FallibleTArray<PRUint8> newFontData;

    rv = gfxFontUtils::RenameFont(uniqueName, aFontData, aLength, &newFontData);
    NS_Free((void*)aFontData);

    if (NS_FAILED(rv)) {
        return nsnull;
    }
    
    DWORD numFonts = 0;

    nsRefPtr<IDWriteFontFile> fontFile;
    HRESULT hr;

    






    ffReferenceKey key;
    key.mArray = &newFontData;
    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1");
    if (!uuidgen) {
        return nsnull;
    }

    rv = uuidgen->GenerateUUIDInPlace(&key.mGUID);

    if (NS_FAILED(rv)) {
        return nsnull;
    }

    hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
        CreateCustomFontFileReference(&key,
                                      sizeof(key),
                                      gfxDWriteFontFileLoader::Instance(),
                                      getter_AddRefs(fontFile));

    if (FAILED(hr)) {
        NS_WARNING("Failed to create custom font file reference.");
        return nsnull;
    }

    BOOL isSupported;
    DWRITE_FONT_FILE_TYPE fileType;
    UINT32 numFaces;

    PRUint16 w = (aProxyEntry->mWeight == 0 ? 400 : aProxyEntry->mWeight);
    gfxDWriteFontEntry *entry = 
        new gfxDWriteFontEntry(uniqueName, 
                               fontFile,
                               aProxyEntry->Weight(),
                               aProxyEntry->Stretch(),
                               aProxyEntry->IsItalic());

    fontFile->Analyze(&isSupported, &fileType, &entry->mFaceType, &numFaces);
    if (!isSupported || numFaces > 1) {
        
        delete entry;
        return nsnull;
    }

    return entry;
}

nsresult
gfxDWriteFontList::InitFontList()
{

#ifdef PR_LOGGING
    gFontInitLog = PR_NewLogModule("fontinit");

    LARGE_INTEGER frequency;        
    LARGE_INTEGER t1, t2, t3, t4, t5, t6;           
    double elapsedTime, upTime;
    char nowTime[256], nowDate[256];

    if (LOG_ENABLED()) {    
        GetTimeFormat(LOCALE_INVARIANT, TIME_FORCE24HOURFORMAT, 
                      NULL, NULL, nowTime, 256);
        GetDateFormat(LOCALE_INVARIANT, NULL, NULL, NULL, nowDate, 256);
        upTime = (double) GetTickCount();
        QueryPerformanceFrequency(&frequency);
        QueryPerformanceCounter(&t1);
    }
#endif

    HRESULT hr;
    gfxFontCache *fc = gfxFontCache::GetCache();
    if (fc) {
        fc->AgeAllGenerations();
    }

    nsCOMPtr<nsIPrefBranch2> pref = do_GetService(NS_PREFSERVICE_CONTRACTID);
    nsresult rv;

    rv = pref->GetBoolPref(
             "gfx.font_rendering.directwrite.use_gdi_table_loading", 
             &mGDIFontTableAccess);
    if (NS_FAILED(rv)) {
        mGDIFontTableAccess = PR_FALSE;
    }

    gfxPlatformFontList::InitFontList();

    mFontSubstitutes.Clear();
    mNonExistingFonts.Clear();

#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
        QueryPerformanceCounter(&t2);
    }
#endif

    nsRefPtr<IDWriteFontCollection> systemFonts;
    hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
        GetSystemFontCollection(getter_AddRefs(systemFonts));
    NS_ASSERTION(SUCCEEDED(hr), "GetSystemFontCollection failed!");

    if (FAILED(hr)) {
        return NS_ERROR_FAILURE;
    }

#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
        QueryPerformanceCounter(&t3);
    }
#endif

    hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
        GetGdiInterop(getter_AddRefs(mGDIInterop));
    if (FAILED(hr)) {
        return NS_ERROR_FAILURE;
    }

#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
        QueryPerformanceCounter(&t4);
    }
#endif

    for (UINT32 i = 0; i < systemFonts->GetFontFamilyCount(); i++) {
        nsRefPtr<IDWriteFontFamily> family;
        systemFonts->GetFontFamily(i, getter_AddRefs(family));

        nsRefPtr<IDWriteLocalizedStrings> names;
        hr = family->GetFamilyNames(getter_AddRefs(names));
        if (FAILED(hr)) {
            continue;
        }

        UINT32 englishIdx = 0;

        BOOL exists;
        hr = names->FindLocaleName(L"en-us", &englishIdx, &exists);
        if (FAILED(hr)) {
            continue;
        }
        if (!exists) {
            
            englishIdx = 0;
        }

        nsAutoTArray<WCHAR, 32> enName;
        UINT32 length;
        
        hr = names->GetStringLength(englishIdx, &length);
        if (FAILED(hr)) {
            continue;
        }
        
        if (!enName.SetLength(length + 1)) {
            
            continue;
        }

        hr = names->GetString(englishIdx, enName.Elements(), length + 1);
        if (FAILED(hr)) {
            continue;
        }

        nsAutoString name(enName.Elements());
        BuildKeyNameFromFontName(name);

        nsRefPtr<gfxFontFamily> fam;

        if (mFontFamilies.GetWeak(name)) {
            continue;
        }
        
        nsDependentString familyName(enName.Elements());

        fam = new gfxDWriteFontFamily(familyName, family);
        if (!fam) {
            continue;
        }

        if (mBadUnderlineFamilyNames.Contains(name)) {
            fam->SetBadUnderlineFamily();
        }
        mFontFamilies.Put(name, fam);

        
        PRUint32 nameCount = names->GetCount();
        PRUint32 nameIndex;

        for (nameIndex = 0; nameIndex < nameCount; nameIndex++) {
            UINT32 nameLen;
            nsAutoTArray<WCHAR, 32> localizedName;

            
            if (nameIndex == englishIdx) {
                continue;
            }
            
            hr = names->GetStringLength(nameIndex, &nameLen);
            if (FAILED(hr)) {
                continue;
            }

            if (!localizedName.SetLength(nameLen + 1)) {
                continue;
            }

            hr = names->GetString(nameIndex, localizedName.Elements(), 
                                  nameLen + 1);
            if (FAILED(hr)) {
                continue;
            }

            nsDependentString locName(localizedName.Elements());

            if (!familyName.Equals(locName)) {
                AddOtherFamilyName(fam, locName);
            }

        }

        
        fam->SetOtherFamilyNamesInitialized();
    }

    mOtherFamilyNamesInitialized = PR_TRUE;
    GetFontSubstitutes();

    StartLoader(kDelayBeforeLoadingFonts, kIntervalBetweenLoadingFonts);

#ifdef PR_LOGGING
    if (LOG_ENABLED()) {
        QueryPerformanceCounter(&t5);

        
        nsAutoString dwriteVers;
        gfxWindowsPlatform::GetPlatform()->GetDLLVersion(L"dwrite.dll",
                                                         dwriteVers);
        LOG(("InitFontList\n"));
        LOG(("Start: %s %s\n", nowDate, nowTime));
        LOG(("Uptime: %9.3f s\n", upTime/1000));
        LOG(("dwrite version: %s\n", NS_ConvertUTF16toUTF8(dwriteVers).get()));
        elapsedTime = (t5.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
        LOG(("Total time in InitFontList:    %9.3f ms (families: %d, %s)\n", 
          elapsedTime, systemFonts->GetFontFamilyCount(), 
          (mGDIFontTableAccess ? "gdi table access" : "dwrite table access")));
        elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
        LOG((" --- gfxPlatformFontList init: %9.3f ms\n", elapsedTime));
        elapsedTime = (t3.QuadPart - t2.QuadPart) * 1000.0 / frequency.QuadPart;
        LOG((" --- GetSystemFontCollection:  %9.3f ms\n", elapsedTime));
        elapsedTime = (t4.QuadPart - t3.QuadPart) * 1000.0 / frequency.QuadPart;
        LOG((" --- GdiInterop object:        %9.3f ms\n", elapsedTime));
        elapsedTime = (t5.QuadPart - t4.QuadPart) * 1000.0 / frequency.QuadPart;
        LOG((" --- iterate over families:    %9.3f ms\n", elapsedTime));
    }
#endif

    return NS_OK;
}

static void
RemoveCharsetFromFontSubstitute(nsAString &aName)
{
    PRInt32 comma = aName.FindChar(PRUnichar(','));
    if (comma >= 0)
        aName.Truncate(comma);
}

#define MAX_VALUE_NAME 512
#define MAX_VALUE_DATA 512

nsresult
gfxDWriteFontList::GetFontSubstitutes()
{
    HKEY hKey;
    DWORD i, rv, lenAlias, lenActual, valueType;
    WCHAR aliasName[MAX_VALUE_NAME];
    WCHAR actualName[MAX_VALUE_DATA];

    if (RegOpenKeyExW(HKEY_LOCAL_MACHINE, 
          L"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes",
          0, KEY_READ, &hKey) != ERROR_SUCCESS)
    {
        return NS_ERROR_FAILURE;
    }

    for (i = 0, rv = ERROR_SUCCESS; rv != ERROR_NO_MORE_ITEMS; i++) {
        aliasName[0] = 0;
        lenAlias = sizeof(aliasName);
        actualName[0] = 0;
        lenActual = sizeof(actualName);
        rv = RegEnumValueW(hKey, i, aliasName, &lenAlias, NULL, &valueType, 
                (LPBYTE)actualName, &lenActual);

        if (rv != ERROR_SUCCESS || valueType != REG_SZ || lenAlias == 0) {
            continue;
        }

        if (aliasName[0] == WCHAR('@')) {
            continue;
        }

        nsAutoString substituteName((PRUnichar*) aliasName);
        nsAutoString actualFontName((PRUnichar*) actualName);
        RemoveCharsetFromFontSubstitute(substituteName);
        BuildKeyNameFromFontName(substituteName);
        RemoveCharsetFromFontSubstitute(actualFontName);
        BuildKeyNameFromFontName(actualFontName);
        gfxFontFamily *ff;
        if (!actualFontName.IsEmpty() && 
            (ff = mFontFamilies.GetWeak(actualFontName))) {
            mFontSubstitutes.Put(substituteName, ff);
        } else {
            mNonExistingFonts.AppendElement(substituteName);
        }
    }
    return NS_OK;
}

PRBool
gfxDWriteFontList::GetStandardFamilyName(const nsAString& aFontName,
                                         nsAString& aFamilyName)
{
    gfxFontFamily *family = FindFamily(aFontName);
    if (family) {
        family->LocalizedName(aFamilyName);
        return PR_TRUE;
    }

    return PR_FALSE;
}

PRBool 
gfxDWriteFontList::ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName)
{
    nsAutoString keyName(aFontName);
    BuildKeyNameFromFontName(keyName);

    nsRefPtr<gfxFontFamily> ff;
    if (mFontSubstitutes.Get(keyName, &ff)) {
        aResolvedFontName = ff->Name();
        return PR_TRUE;
    }

    if (mNonExistingFonts.Contains(keyName)) {
        return PR_FALSE;
    }

    return gfxPlatformFontList::ResolveFontName(aFontName, aResolvedFontName);
}
