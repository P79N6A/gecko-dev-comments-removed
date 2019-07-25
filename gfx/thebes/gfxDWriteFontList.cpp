




#include "mozilla/Util.h"

#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif 

#include "gfxDWriteFontList.h"
#include "gfxDWriteFonts.h"
#include "nsUnicharUtils.h"
#include "nsILocaleService.h"
#include "nsServiceManagerUtils.h"
#include "nsCharSeparatedTokenizer.h"
#include "mozilla/Preferences.h"
#include "mozilla/Telemetry.h"

#include "gfxGDIFontList.h"

#include "nsIWindowsRegKey.h"

using namespace mozilla;

#define LOG_FONTLIST(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontlist), \
                               PR_LOG_DEBUG, args)
#define LOG_FONTLIST_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontlist), \
                                   PR_LOG_DEBUG)

#define LOG_FONTINIT(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontinit), \
                               PR_LOG_DEBUG, args)
#define LOG_FONTINIT_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontinit), \
                                   PR_LOG_DEBUG)

#define LOG_CMAPDATA_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_cmapdata), \
                                   PR_LOG_DEBUG)





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
    mHasStyles = true;

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
        fullID.Append(NS_LITERAL_STRING(" "));
        fullID.Append(faceName.Elements());

        



        gfxDWriteFontEntry *fe = 
            new gfxDWriteFontEntry(fullID, font);
        fe->SetForceGDIClassic(mForceGDIClassic);
        AddFontEntry(fe);

#ifdef PR_LOGGING
        if (LOG_FONTLIST_ENABLED()) {
            LOG_FONTLIST(("(fontlist) added (%s) to family (%s)"
                 " with style: %s weight: %d stretch: %d",
                 NS_ConvertUTF16toUTF8(fe->Name()).get(),
                 NS_ConvertUTF16toUTF8(Name()).get(),
                 (fe->IsItalic()) ? "italic" : "normal",
                 fe->Weight(), fe->Stretch()));
        }
#endif
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

void
gfxDWriteFontFamily::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                         FontListSizes*    aSizes) const
{
    gfxFontFamily::SizeOfExcludingThis(aMallocSizeOf, aSizes);
    
    
}

void
gfxDWriteFontFamily::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                         FontListSizes*    aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}




gfxDWriteFontEntry::~gfxDWriteFontEntry()
{
}

bool
gfxDWriteFontEntry::IsSymbolFont()
{
    if (mFont) {
        return mFont->IsSymbolFont();
    } else {
        return false;
    }
}

static bool
UsingArabicScriptSystemLocale()
{
    LANGID langid = PRIMARYLANGID(::GetSystemDefaultLangID());
    switch (langid) {
    case LANG_ARABIC:
    case LANG_DARI:
    case LANG_PASHTO:
    case LANG_PERSIAN:
    case LANG_SINDHI:
    case LANG_UIGHUR:
    case LANG_URDU:
        return true;
    default:
        return false;
    }
}

nsresult
gfxDWriteFontEntry::GetFontTable(PRUint32 aTableTag,
                                 FallibleTArray<PRUint8> &aBuffer)
{
    gfxDWriteFontList *pFontList = gfxDWriteFontList::PlatformFontList();

    
    
    
    if (mFont && pFontList->UseGDIFontTableAccess() &&
        !(mItalic && UsingArabicScriptSystemLocale()) &&
        !mFont->IsSymbolFont())
    {
        LOGFONTW logfont = { 0 };
        if (!InitLogFont(mFont, &logfont))
            return NS_ERROR_FAILURE;

        AutoDC dc;
        AutoSelectFont font(dc.GetDC(), &logfont);
        if (font.IsValid()) {
            PRUint32 tableSize =
                ::GetFontData(dc.GetDC(), NS_SWAP32(aTableTag), 0, NULL, 0);
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

    
    if (mCharacterMap) {
        return NS_OK;
    }

    nsRefPtr<gfxCharacterMap> charmap = new gfxCharacterMap();

    
    if (mFont && gfxDWriteFontList::PlatformFontList()->UseGDIFontTableAccess()) {
        PRUint32 kCMAP = TRUETYPE_TAG('c','m','a','p');
    
        AutoFallibleTArray<PRUint8,16384> cmap;
        rv = GetFontTable(kCMAP, cmap);
    
        bool unicodeFont = false, symbolFont = false; 
    
        if (NS_SUCCEEDED(rv)) {
            rv = gfxFontUtils::ReadCMAP(cmap.Elements(), cmap.Length(),
                                        *charmap, mUVSOffset,
                                        unicodeFont, symbolFont);
        }
    } else {
        
        nsRefPtr<IDWriteFontFace> fontFace;
        rv = CreateFontFace(getter_AddRefs(fontFace));
    
        if (NS_SUCCEEDED(rv)) {
            const PRUint32 kCmapTag = DWRITE_MAKE_OPENTYPE_TAG('c', 'm', 'a', 'p');
            PRUint8 *tableData;
            PRUint32 len;
            void *tableContext = NULL;
            BOOL exists;
            hr = fontFace->TryGetFontTable(kCmapTag, (const void**)&tableData,
                                           &len, &tableContext, &exists);

            if (SUCCEEDED(hr)) {
                bool isSymbol = fontFace->IsSymbolFont();
                bool isUnicode = true;
                if (exists) {
                    rv = gfxFontUtils::ReadCMAP(tableData, len, *charmap,
                                                mUVSOffset, isUnicode, 
                                                isSymbol);
                }
                fontFace->ReleaseFontTable(tableContext);
            } else {
                rv = NS_ERROR_FAILURE;
            }
        }
    }

    mHasCmapTable = NS_SUCCEEDED(rv);
    if (mHasCmapTable) {
        gfxPlatformFontList *pfl = gfxPlatformFontList::PlatformFontList();
        mCharacterMap = pfl->FindCharMap(charmap);
    } else {
        
        mCharacterMap = new gfxCharacterMap();
    }

#ifdef PR_LOGGING
    LOG_FONTLIST(("(fontlist-cmap) name: %s, size: %d hash: %8.8x%s\n",
                  NS_ConvertUTF16toUTF8(mName).get(),
                  charmap->SizeOfIncludingThis(moz_malloc_size_of),
                  charmap->mHash, mCharacterMap == charmap ? " new" : ""));
    if (LOG_CMAPDATA_ENABLED()) {
        char prefix[256];
        sprintf(prefix, "(cmapdata) name: %.220s",
                NS_ConvertUTF16toUTF8(mName).get());
        charmap->Dump(prefix, eGfxLog_cmapdata);
    }
#endif

    return rv;
}

gfxFont *
gfxDWriteFontEntry::CreateFontInstance(const gfxFontStyle* aFontStyle,
                                       bool aNeedsBold)
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
        if (SUCCEEDED(hr) && (aSimulations & DWRITE_FONT_SIMULATIONS_BOLD) &&
            !((*aFontFace)->GetSimulations() & DWRITE_FONT_SIMULATIONS_BOLD)) {
            
            
            
            nsRefPtr<IDWriteFontFace> origFace = (*aFontFace);
            (*aFontFace)->Release();
            *aFontFace = NULL;
            UINT32 numberOfFiles = 0;
            hr = origFace->GetFiles(&numberOfFiles, NULL);
            if (FAILED(hr)) {
                return NS_ERROR_FAILURE;
            }
            nsAutoTArray<IDWriteFontFile*,1> files;
            files.AppendElements(numberOfFiles);
            hr = origFace->GetFiles(&numberOfFiles, files.Elements());
            if (FAILED(hr)) {
                return NS_ERROR_FAILURE;
            }
            hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
                CreateFontFace(origFace->GetType(),
                               numberOfFiles,
                               files.Elements(),
                               origFace->GetIndex(),
                               aSimulations,
                               aFontFace);
            for (UINT32 i = 0; i < numberOfFiles; ++i) {
                files[i]->Release();
            }
        }
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

bool
gfxDWriteFontEntry::InitLogFont(IDWriteFont *aFont, LOGFONTW *aLogFont)
{
    HRESULT hr;

    BOOL isInSystemCollection;
    IDWriteGdiInterop *gdi = 
        gfxDWriteFontList::PlatformFontList()->GetGDIInterop();
    hr = gdi->ConvertFontToLOGFONT(aFont, aLogFont, &isInSystemCollection);
    return (FAILED(hr) ? false : true);
}

bool
gfxDWriteFontEntry::IsCJKFont()
{
    if (mIsCJK != UNINITIALIZED_VALUE) {
        return mIsCJK;
    }

    mIsCJK = false;

    const PRUint32 kOS2Tag = TRUETYPE_TAG('O','S','/','2');
    AutoFallibleTArray<PRUint8,128> buffer;
    if (GetFontTable(kOS2Tag, buffer) != NS_OK) {
        return mIsCJK;
    }

    
    
    const PRUint32 CJK_CODEPAGE_BITS =
        (1 << 17) | 
        (1 << 18) | 
        (1 << 19) | 
        (1 << 20) | 
        (1 << 21);  

    if (buffer.Length() >= offsetof(OS2Table, sxHeight)) {
        const OS2Table* os2 =
            reinterpret_cast<const OS2Table*>(buffer.Elements());
        if ((PRUint32(os2->codePageRange1) & CJK_CODEPAGE_BITS) != 0) {
            mIsCJK = true;
        }
    }

    return mIsCJK;
}

void
gfxDWriteFontEntry::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                        FontListSizes*    aSizes) const
{
    gfxFontEntry::SizeOfExcludingThis(aMallocSizeOf, aSizes);
    
    
}

void
gfxDWriteFontEntry::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                        FontListSizes*    aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}




gfxDWriteFontList::gfxDWriteFontList()
    : mInitialized(false), mForceGDIClassicMaxFontSize(0.0)
{
    mFontSubstitutes.Init();
}





gfxFontEntry *
gfxDWriteFontList::GetDefaultFont(const gfxFontStyle *aStyle,
                                  bool &aNeedsBold)
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
    gfxFontEntry *lookup;

    
    if (!mFaceNamesInitialized) {
        InitFaceNameLists();
    }

    
    if (!(lookup = mPostscriptNames.GetWeak(aFullname)) &&
        !(lookup = mFullnames.GetWeak(aFullname))) 
    {
        return nsnull;
    }
    gfxDWriteFontEntry* dwriteLookup = static_cast<gfxDWriteFontEntry*>(lookup);
    gfxDWriteFontEntry *fe =
        new gfxDWriteFontEntry(lookup->Name(),
                               dwriteLookup->mFont,
                               aProxyEntry->Weight(),
                               aProxyEntry->Stretch(),
                               aProxyEntry->IsItalic());
    fe->SetForceGDIClassic(dwriteLookup->GetForceGDIClassic());
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

#ifdef DEBUG_DWRITE_STARTUP

#define LOGREGISTRY(msg) LogRegistryEvent(msg)


static void LogRegistryEvent(const wchar_t *msg)
{
    HKEY dummyKey;
    HRESULT hr;
    wchar_t buf[512];

    wsprintfW(buf, L" log %s", msg);
    hr = RegOpenKeyExW(HKEY_LOCAL_MACHINE, buf, 0, KEY_READ, &dummyKey);
    if (SUCCEEDED(hr)) {
        RegCloseKey(dummyKey);
    }
}
#else

#define LOGREGISTRY(msg)

#endif

nsresult
gfxDWriteFontList::InitFontList()
{
    LOGREGISTRY(L"InitFontList start");

    mInitialized = false;

    LARGE_INTEGER frequency;        
    LARGE_INTEGER t1, t2, t3;           
    double elapsedTime, upTime;
    char nowTime[256], nowDate[256];

    if (LOG_FONTINIT_ENABLED()) {    
        GetTimeFormat(LOCALE_INVARIANT, TIME_FORCE24HOURFORMAT, 
                      NULL, NULL, nowTime, 256);
        GetDateFormat(LOCALE_INVARIANT, 0, NULL, NULL, nowDate, 256);
    }
    upTime = (double) GetTickCount();
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&t1);

    HRESULT hr;
    gfxFontCache *fc = gfxFontCache::GetCache();
    if (fc) {
        fc->AgeAllGenerations();
    }

    mGDIFontTableAccess = Preferences::GetBool("gfx.font_rendering.directwrite.use_gdi_table_loading", false);

    gfxPlatformFontList::InitFontList();

    mFontSubstitutes.Clear();
    mNonExistingFonts.Clear();

    QueryPerformanceCounter(&t2);

    hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
        GetGdiInterop(getter_AddRefs(mGDIInterop));
    if (FAILED(hr)) {
        return NS_ERROR_FAILURE;
    }

    LOGREGISTRY(L"InitFontList end");

    QueryPerformanceCounter(&t3);

    if (LOG_FONTINIT_ENABLED()) {
        
        nsAutoString dwriteVers;
        gfxWindowsPlatform::GetDLLVersion(L"dwrite.dll", dwriteVers);
        LOG_FONTINIT(("InitFontList\n"));
        LOG_FONTINIT(("Start: %s %s\n", nowDate, nowTime));
        LOG_FONTINIT(("Uptime: %9.3f s\n", upTime/1000));
        LOG_FONTINIT(("dwrite version: %s\n", 
                      NS_ConvertUTF16toUTF8(dwriteVers).get()));
    }

    elapsedTime = (t3.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    Telemetry::Accumulate(Telemetry::DWRITEFONT_INITFONTLIST_TOTAL, elapsedTime);
    LOG_FONTINIT(("Total time in InitFontList:    %9.3f ms\n", elapsedTime));
    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    Telemetry::Accumulate(Telemetry::DWRITEFONT_INITFONTLIST_INIT, elapsedTime);
    LOG_FONTINIT((" --- gfxPlatformFontList init: %9.3f ms\n", elapsedTime));
    elapsedTime = (t3.QuadPart - t2.QuadPart) * 1000.0 / frequency.QuadPart;
    Telemetry::Accumulate(Telemetry::DWRITEFONT_INITFONTLIST_GDI, elapsedTime);
    LOG_FONTINIT((" --- GdiInterop object:        %9.3f ms\n", elapsedTime));

    return NS_OK;
}

nsresult
gfxDWriteFontList::DelayedInitFontList()
{
    LOGREGISTRY(L"DelayedInitFontList start");

    LARGE_INTEGER frequency;        
    LARGE_INTEGER t1, t2, t3;           
    double elapsedTime, upTime;
    char nowTime[256], nowDate[256];

    if (LOG_FONTINIT_ENABLED()) {    
        GetTimeFormat(LOCALE_INVARIANT, TIME_FORCE24HOURFORMAT, 
                      NULL, NULL, nowTime, 256);
        GetDateFormat(LOCALE_INVARIANT, 0, NULL, NULL, nowDate, 256);
    }

    upTime = (double) GetTickCount();
    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&t1);

    HRESULT hr;

    LOGREGISTRY(L"calling GetSystemFontCollection");
    nsRefPtr<IDWriteFontCollection> systemFonts;
    hr = gfxWindowsPlatform::GetPlatform()->GetDWriteFactory()->
        GetSystemFontCollection(getter_AddRefs(systemFonts));
    NS_ASSERTION(SUCCEEDED(hr), "GetSystemFontCollection failed!");
    LOGREGISTRY(L"GetSystemFontCollection done");

    if (FAILED(hr)) {
        return NS_ERROR_FAILURE;
    }

    QueryPerformanceCounter(&t2);

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

    mOtherFamilyNamesInitialized = true;
    GetFontSubstitutes();

    
    
    
    
    
    GetDirectWriteSubstitutes();

    
    
    
    

    nsAutoString nameGillSans(L"Gill Sans");
    nsAutoString nameGillSansMT(L"Gill Sans MT");
    BuildKeyNameFromFontName(nameGillSans);
    BuildKeyNameFromFontName(nameGillSansMT);

    gfxFontFamily *gillSansFamily = mFontFamilies.GetWeak(nameGillSans);
    gfxFontFamily *gillSansMTFamily = mFontFamilies.GetWeak(nameGillSansMT);

    if (gillSansFamily && gillSansMTFamily) {
        gillSansFamily->FindStyleVariations();
        nsTArray<nsRefPtr<gfxFontEntry> >& faces = gillSansFamily->GetFontList();
        PRUint32 i;

        bool allUltraBold = true;
        for (i = 0; i < faces.Length(); i++) {
            
            if (faces[i]->Name().Find(NS_LITERAL_STRING("Ultra Bold")) == -1) {
                allUltraBold = false;
                break;
            }
        }

        
        
        if (allUltraBold) {

            
            for (i = 0; i < faces.Length(); i++) {
                gillSansMTFamily->AddFontEntry(faces[i]);

#ifdef PR_LOGGING
                if (LOG_FONTLIST_ENABLED()) {
                    gfxFontEntry *fe = faces[i];
                    LOG_FONTLIST(("(fontlist) moved (%s) to family (%s)"
                         " with style: %s weight: %d stretch: %d",
                         NS_ConvertUTF16toUTF8(fe->Name()).get(),
                         NS_ConvertUTF16toUTF8(gillSansMTFamily->Name()).get(),
                         (fe->IsItalic()) ? "italic" : "normal",
                         fe->Weight(), fe->Stretch()));
                }
#endif
            }

            
            mFontFamilies.Remove(nameGillSans);
        }
    }

    nsAdoptingCString classicFamilies =
        Preferences::GetCString("gfx.font_rendering.cleartype_params.force_gdi_classic_for_families");
    if (classicFamilies) {
        nsCCharSeparatedTokenizer tokenizer(classicFamilies, ',');
        while (tokenizer.hasMoreTokens()) {
            NS_ConvertUTF8toUTF16 name(tokenizer.nextToken());
            BuildKeyNameFromFontName(name);
            gfxFontFamily *family = mFontFamilies.GetWeak(name);
            if (family) {
                static_cast<gfxDWriteFontFamily*>(family)->SetForceGDIClassic(true);
            }
        }
    }
    mForceGDIClassicMaxFontSize =
        Preferences::GetInt("gfx.font_rendering.cleartype_params.force_gdi_classic_max_size",
                            mForceGDIClassicMaxFontSize);

    StartLoader(kDelayBeforeLoadingFonts, kIntervalBetweenLoadingFonts);

    LOGREGISTRY(L"DelayedInitFontList end");

    QueryPerformanceCounter(&t3);

    if (LOG_FONTINIT_ENABLED()) {
        
        nsAutoString dwriteVers;
        gfxWindowsPlatform::GetDLLVersion(L"dwrite.dll", dwriteVers);
        LOG_FONTINIT(("DelayedInitFontList\n"));
        LOG_FONTINIT(("Start: %s %s\n", nowDate, nowTime));
        LOG_FONTINIT(("Uptime: %9.3f s\n", upTime/1000));
        LOG_FONTINIT(("dwrite version: %s\n", 
                      NS_ConvertUTF16toUTF8(dwriteVers).get()));
    }

    elapsedTime = (t3.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    Telemetry::Accumulate(Telemetry::DWRITEFONT_DELAYEDINITFONTLIST_TOTAL, elapsedTime);
    Telemetry::Accumulate(Telemetry::DWRITEFONT_DELAYEDINITFONTLIST_COUNT,
                          systemFonts->GetFontFamilyCount());
    Telemetry::Accumulate(Telemetry::DWRITEFONT_DELAYEDINITFONTLIST_GDI_TABLE, mGDIFontTableAccess);
    LOG_FONTINIT((
       "Total time in DelayedInitFontList:    %9.3f ms (families: %d, %s)\n",
       elapsedTime, systemFonts->GetFontFamilyCount(),
       (mGDIFontTableAccess ? "gdi table access" : "dwrite table access")));

    elapsedTime = (t2.QuadPart - t1.QuadPart) * 1000.0 / frequency.QuadPart;
    Telemetry::Accumulate(Telemetry::DWRITEFONT_DELAYEDINITFONTLIST_COLLECT, elapsedTime);
    LOG_FONTINIT((" --- GetSystemFontCollection:  %9.3f ms\n", elapsedTime));

    elapsedTime = (t3.QuadPart - t2.QuadPart) * 1000.0 / frequency.QuadPart;
    Telemetry::Accumulate(Telemetry::DWRITEFONT_DELAYEDINITFONTLIST_ITERATE, elapsedTime);
    LOG_FONTINIT((" --- iterate over families:    %9.3f ms\n", elapsedTime));

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
        lenAlias = ArrayLength(aliasName);
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

struct FontSubstitution {
    const WCHAR* aliasName;
    const WCHAR* actualName;
};

static const FontSubstitution sDirectWriteSubs[] = {
    { L"MS Sans Serif", L"Microsoft Sans Serif" },
    { L"MS Serif", L"Times New Roman" },
    { L"Courier", L"Courier New" },
    { L"Small Fonts", L"Arial" },
    { L"Roman", L"Times New Roman" },
    { L"Script", L"Mistral" }
};

void
gfxDWriteFontList::GetDirectWriteSubstitutes()
{
    for (PRUint32 i = 0; i < ArrayLength(sDirectWriteSubs); ++i) {
        const FontSubstitution& sub(sDirectWriteSubs[i]);
        nsAutoString substituteName((PRUnichar*)sub.aliasName);
        BuildKeyNameFromFontName(substituteName);
        if (nsnull != mFontFamilies.GetWeak(substituteName)) {
            
            
            continue;
        }
        nsAutoString actualFontName((PRUnichar*)sub.actualName);
        BuildKeyNameFromFontName(actualFontName);
        gfxFontFamily *ff;
        if (nsnull != (ff = mFontFamilies.GetWeak(actualFontName))) {
            mFontSubstitutes.Put(substituteName, ff);
        } else {
            mNonExistingFonts.AppendElement(substituteName);
        }
    }
}

bool
gfxDWriteFontList::GetStandardFamilyName(const nsAString& aFontName,
                                         nsAString& aFamilyName)
{
    gfxFontFamily *family = FindFamily(aFontName);
    if (family) {
        family->LocalizedName(aFamilyName);
        return true;
    }

    return false;
}

gfxFontFamily* gfxDWriteFontList::FindFamily(const nsAString& aFamily)
{
    if (!mInitialized) {
        mInitialized = true;
        DelayedInitFontList();
    }

    return gfxPlatformFontList::FindFamily(aFamily);
}

void
gfxDWriteFontList::GetFontFamilyList(nsTArray<nsRefPtr<gfxFontFamily> >& aFamilyArray)
{
    if (!mInitialized) {
        mInitialized = true;
        DelayedInitFontList();
    }

    return gfxPlatformFontList::GetFontFamilyList(aFamilyArray);
}

bool 
gfxDWriteFontList::ResolveFontName(const nsAString& aFontName,
                                   nsAString& aResolvedFontName)
{
    if (!mInitialized) {
        mInitialized = true;
        DelayedInitFontList();
    }

    nsAutoString keyName(aFontName);
    BuildKeyNameFromFontName(keyName);

    gfxFontFamily *ff = mFontSubstitutes.GetWeak(keyName);
    if (ff) {
        aResolvedFontName = ff->Name();
        return true;
    }

    if (mNonExistingFonts.Contains(keyName)) {
        return false;
    }

    return gfxPlatformFontList::ResolveFontName(aFontName, aResolvedFontName);
}

void
gfxDWriteFontList::SizeOfExcludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                       FontListSizes*    aSizes) const
{
    gfxPlatformFontList::SizeOfExcludingThis(aMallocSizeOf, aSizes);

    aSizes->mFontListSize +=
        mFontSubstitutes.SizeOfExcludingThis(SizeOfFamilyNameEntryExcludingThis,
                                             aMallocSizeOf);

    aSizes->mFontListSize +=
        mNonExistingFonts.SizeOfExcludingThis(aMallocSizeOf);
    for (PRUint32 i = 0; i < mNonExistingFonts.Length(); ++i) {
        aSizes->mFontListSize +=
            mNonExistingFonts[i].SizeOfExcludingThisIfUnshared(aMallocSizeOf);
    }
}

void
gfxDWriteFontList::SizeOfIncludingThis(nsMallocSizeOfFun aMallocSizeOf,
                                       FontListSizes*    aSizes) const
{
    aSizes->mFontListSize += aMallocSizeOf(this);
    SizeOfExcludingThis(aMallocSizeOf, aSizes);
}

static nsresult GetFamilyName(IDWriteFont *aFont, nsString& aFamilyName)
{
    HRESULT hr;
    nsRefPtr<IDWriteFontFamily> family;

    
    aFamilyName.Truncate();

    hr = aFont->GetFontFamily(getter_AddRefs(family));
    if (FAILED(hr)) {
        return hr;
    }

    nsRefPtr<IDWriteLocalizedStrings> familyNames;

    hr = family->GetFamilyNames(getter_AddRefs(familyNames));
    if (FAILED(hr)) {
        return hr;
    }

    UINT32 index = 0;
    BOOL exists = false;

    hr = familyNames->FindLocaleName(L"en-us", &index, &exists);
    if (FAILED(hr)) {
        return hr;
    }

    
    if (!exists) {
        index = 0;
    }

    nsAutoTArray<WCHAR, 32> name;
    UINT32 length;

    hr = familyNames->GetStringLength(index, &length);
    if (FAILED(hr)) {
        return hr;
    }

    if (!name.SetLength(length + 1)) {
        return NS_ERROR_FAILURE;
    }
    hr = familyNames->GetString(index, name.Elements(), length + 1);
    if (FAILED(hr)) {
        return hr;
    }

    aFamilyName.Assign(name.Elements());
    return NS_OK;
}





IFACEMETHODIMP FontFallbackRenderer::DrawGlyphRun(
    void* clientDrawingContext,
    FLOAT baselineOriginX,
    FLOAT baselineOriginY,
    DWRITE_MEASURING_MODE measuringMode,
    DWRITE_GLYPH_RUN const* glyphRun,
    DWRITE_GLYPH_RUN_DESCRIPTION const* glyphRunDescription,
    IUnknown* clientDrawingEffect
    )
{
    if (!mSystemFonts) {
        return E_FAIL;
    }

    HRESULT hr = S_OK;

    nsRefPtr<IDWriteFont> font;
    hr = mSystemFonts->GetFontFromFontFace(glyphRun->fontFace,
                                           getter_AddRefs(font));
    if (FAILED(hr)) {
        return hr;
    }

    
    hr = GetFamilyName(font, mFamilyName);
    if (FAILED(hr)) {
        return hr;
    }

    
    
    if (mFamilyName.EqualsLiteral("Arial")) {
        mFamilyName.Truncate();
        return E_FAIL;
    }
    return hr;
}

gfxFontEntry*
gfxDWriteFontList::GlobalFontFallback(const PRUint32 aCh,
                                      PRInt32 aRunScript,
                                      const gfxFontStyle* aMatchStyle,
                                      PRUint32& aCmapCount)
{
    bool useCmaps = gfxPlatform::GetPlatform()->UseCmapsDuringSystemFallback();

    if (useCmaps) {
        return gfxPlatformFontList::GlobalFontFallback(aCh,
                                                       aRunScript,
                                                       aMatchStyle,
                                                       aCmapCount);
    }

    HRESULT hr;

    nsRefPtr<IDWriteFactory> dwFactory =
        gfxWindowsPlatform::GetPlatform()->GetDWriteFactory();
    if (!dwFactory) {
        return nsnull;
    }

    
    if (!mFallbackRenderer) {
        mFallbackRenderer = new FontFallbackRenderer(dwFactory);
    }

    
    if (!mFallbackFormat) {
        hr = dwFactory->CreateTextFormat(L"Arial", NULL,
                                         DWRITE_FONT_WEIGHT_REGULAR,
                                         DWRITE_FONT_STYLE_NORMAL,
                                         DWRITE_FONT_STRETCH_NORMAL,
                                         72.0f, L"en-us",
                                         getter_AddRefs(mFallbackFormat));
        if (FAILED(hr)) {
            return nsnull;
        }
    }

    
    wchar_t str[16];
    PRUint32 strLen;

    if (IS_IN_BMP(aCh)) {
        str[0] = static_cast<wchar_t> (aCh);
        str[1] = 0;
        strLen = 1;
    } else {
        str[0] = static_cast<wchar_t> (H_SURROGATE(aCh));
        str[1] = static_cast<wchar_t> (L_SURROGATE(aCh));
        str[2] = 0;
        strLen = 2;
    }

    
    nsRefPtr<IDWriteTextLayout> fallbackLayout;

    hr = dwFactory->CreateTextLayout(str, strLen, mFallbackFormat,
                                     200.0f, 200.0f,
                                     getter_AddRefs(fallbackLayout));
    if (FAILED(hr)) {
        return nsnull;
    }

    
    
    hr = fallbackLayout->Draw(NULL, mFallbackRenderer, 50.0f, 50.0f);
    if (FAILED(hr)) {
        return nsnull;
    }

    gfxFontEntry *fontEntry = nsnull;
    bool needsBold;  
    fontEntry = FindFontForFamily(mFallbackRenderer->FallbackFamilyName(),
                                  aMatchStyle, needsBold);
    if (fontEntry && !fontEntry->TestCharacterMap(aCh)) {
        fontEntry = nsnull;
        Telemetry::Accumulate(Telemetry::BAD_FALLBACK_FONT, true);
    }
    return fontEntry;
}
