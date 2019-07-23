







































#include "gfxWindowsPlatform.h"

#include "gfxImageSurface.h"
#include "gfxWindowsSurface.h"

#include "nsUnicharUtils.h"

#include "nsIPref.h"
#include "nsServiceManagerUtils.h"

#include "nsIWindowsRegKey.h"

#include "gfxWindowsFonts.h"

#include <string>













gfxWindowsPlatform::gfxWindowsPlatform()
{
    mFonts.Init(200);
    mFontAliases.Init(20);
    mFontSubstitutes.Init(50);
    UpdateFontList();
}

gfxWindowsPlatform::~gfxWindowsPlatform()
{
}

already_AddRefed<gfxASurface>
gfxWindowsPlatform::CreateOffscreenSurface(const gfxIntSize& size,
                                           gfxASurface::gfxImageFormat imageFormat)
{
    gfxASurface *surf = new gfxWindowsSurface(size, imageFormat);
    NS_IF_ADDREF(surf);
    return surf;
}

int CALLBACK 
gfxWindowsPlatform::FontEnumProc(const ENUMLOGFONTEXW *lpelfe,
                                 const NEWTEXTMETRICEXW *nmetrics,
                                 DWORD fontType, LPARAM data)
{
    gfxWindowsPlatform *thisp = reinterpret_cast<gfxWindowsPlatform*>(data);

    const LOGFONTW& logFont = lpelfe->elfLogFont;
    const NEWTEXTMETRICW& metrics = nmetrics->ntmTm;

#ifdef DEBUG_pavlov
    printf("%s %d %d %d\n", NS_ConvertUTF16toUTF8(nsDependentString(logFont.lfFaceName)).get(),
           logFont.lfCharSet, logFont.lfItalic, logFont.lfWeight);
#endif

    
    if (logFont.lfFaceName[0] == L'@') {
        return 1;
    }

    nsString name(logFont.lfFaceName);
    ToLowerCase(name);

    nsRefPtr<FontEntry> fe;
    if (!thisp->mFonts.Get(name, &fe)) {
        fe = new FontEntry(nsDependentString(logFont.lfFaceName), (PRUint16)fontType);
        thisp->mFonts.Put(name, fe);
    }

    
    fe->mCharset[metrics.tmCharSet] = 1;

    
    fe->mWeightTable.SetWeight(PR_MAX(1, PR_MIN(9, metrics.tmWeight / 100)), PR_TRUE);

    
    fe->mDefaultWeight = metrics.tmWeight;

    fe->mFamily = logFont.lfPitchAndFamily & 0xF0;
    fe->mPitch = logFont.lfPitchAndFamily & 0x0F;

    if (nmetrics->ntmFontSig.fsUsb[0] == 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[1] == 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[2] == 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[3] == 0x00000000) {
        
        fe->mUnicodeFont = PR_FALSE;
    } else {
        fe->mUnicodeFont = PR_TRUE;

        
        PRUint32 x = 0;
        for (PRUint32 i = 0; i < 4; ++i) {
            DWORD range = nmetrics->ntmFontSig.fsUsb[i];
            for (PRUint32 k = 0; k < 32; ++k) {
                fe->mUnicodeRanges[x++] = (range & (1 << k)) != 0;
            }
        }
    }

    return 1;
}

static inline PRUint16
ReadShortAt(const PRUint8 *aBuf, PRUint32 aIndex)
{
    return (aBuf[aIndex] << 8) | aBuf[aIndex + 1];
}

static inline PRUint16
ReadShortAt16(const PRUint16 *aBuf, PRUint32 aIndex)
{
    return (((aBuf[aIndex]&0xFF) << 8) | ((aBuf[aIndex]&0xFF00) >> 8));
}

static inline PRUint32
ReadLongAt(const PRUint8 *aBuf, PRUint32 aIndex)
{
    return ((aBuf[aIndex] << 24) | (aBuf[aIndex + 1] << 16) | (aBuf[aIndex + 2] << 8) | (aBuf[aIndex + 3]));
}

static nsresult
ReadCMAPTableFormat12(PRUint8 *aBuf, PRInt32 aLength, FontEntry *aFontEntry) 
{
    enum {
        OffsetFormat = 0,
        OffsetReserved = 2,
        OffsetTableLength = 4,
        OffsetLanguage = 8,
        OffsetNumberGroups = 12,
        OffsetGroups = 16,

        SizeOfGroup = 12,

        GroupOffsetStartCode = 0,
        GroupOffsetEndCode = 4
    };
    NS_ENSURE_TRUE(aLength >= 16, NS_ERROR_FAILURE);

    NS_ENSURE_TRUE(ReadShortAt(aBuf, OffsetFormat) == 12, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(ReadShortAt(aBuf, OffsetReserved) == 0, NS_ERROR_FAILURE);

    PRUint32 tablelen = ReadLongAt(aBuf, OffsetTableLength);
    NS_ENSURE_TRUE(tablelen <= aLength, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(tablelen >= 16, NS_ERROR_FAILURE);

    NS_ENSURE_TRUE(ReadLongAt(aBuf, OffsetLanguage) == 0, NS_ERROR_FAILURE);

    const PRUint32 numGroups  = ReadLongAt(aBuf, OffsetNumberGroups);
    NS_ENSURE_TRUE(tablelen >= 16 + (12 * numGroups), NS_ERROR_FAILURE);

    const PRUint8 *groups = aBuf + OffsetGroups;
    for (PRUint32 i = 0; i < numGroups; i++, groups += SizeOfGroup) {
        const PRUint32 startCharCode = ReadLongAt(groups, GroupOffsetStartCode);
        const PRUint32 endCharCode = ReadLongAt(groups, GroupOffsetEndCode);
        aFontEntry->mCharacterMap.SetRange(startCharCode, endCharCode);
#ifdef UPDATE_RANGES
        for (PRUint32 c = startCharCode; c <= endCharCode; ++c) {
            PRUint16 b = CharRangeBit(c);
            if (b != NO_RANGE_FOUND)
                aFontEntry->mUnicodeRanges.set(b, true);
        }
#endif
    }

    return NS_OK;
}

static nsresult 
ReadCMAPTableFormat4(PRUint8 *aBuf, PRInt32 aLength, FontEntry *aFontEntry)
{
    enum {
        OffsetFormat = 0,
        OffsetLength = 2,
        OffsetLanguage = 4,
        OffsetSegCountX2 = 6
    };

    NS_ENSURE_TRUE(ReadShortAt(aBuf, OffsetFormat) == 4, NS_ERROR_FAILURE);
    PRUint16 tablelen = ReadShortAt(aBuf, OffsetLength);
    NS_ENSURE_TRUE(tablelen <= aLength, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(tablelen > 16, NS_ERROR_FAILURE);
    NS_ENSURE_TRUE(ReadShortAt(aBuf, OffsetLanguage) == 0, NS_ERROR_FAILURE);

    PRUint16 segCountX2 = ReadShortAt(aBuf, OffsetSegCountX2);
    NS_ENSURE_TRUE(tablelen >= 16 + (segCountX2 * 4), NS_ERROR_FAILURE);

    const PRUint16 segCount = segCountX2 / 2;

    const PRUint16 *endCounts = (PRUint16*)(aBuf + 14);
    const PRUint16 *startCounts = endCounts + 1  + segCount;
    const PRUint16 *idDeltas = startCounts + segCount;
    const PRUint16 *idRangeOffsets = idDeltas + segCount;
    for (PRUint16 i = 0; i < segCount; i++) {
        const PRUint16 endCount = ReadShortAt16(endCounts, i);
        const PRUint16 startCount = ReadShortAt16(startCounts, i);
        const PRUint16 idRangeOffset = ReadShortAt16(idRangeOffsets, i);
        if (idRangeOffset == 0) {
            aFontEntry->mCharacterMap.SetRange(startCount, endCount);
#ifdef UPDATE_RANGES
            for (PRUint32 c = startCount; c <= endCount; c++) {
                PRUint16 b = CharRangeBit(c);
                if (b != NO_RANGE_FOUND)
                    aFontEntry->mUnicodeRanges.set(b, true);
            }
#endif
        } else {
            const PRUint16 idDelta = ReadShortAt16(idDeltas, i);
            for (PRUint32 c = startCount; c <= endCount; ++c) {
                if (c == 0xFFFF)
                    break;

                const PRUint16 *gdata = (idRangeOffset/2 
                                         + (c - startCount)
                                         + &idRangeOffsets[i]);

                NS_ENSURE_TRUE((PRUint8*)gdata > aBuf && (PRUint8*)gdata < aBuf + aLength, NS_ERROR_FAILURE);

                
                if (*gdata != 0) {
                    
                    

                    aFontEntry->mCharacterMap.set(c);
#ifdef UPDATE_RANGES
                    PRUint16 b = CharRangeBit(c);
                    if (b != NO_RANGE_FOUND)
                        aFontEntry->mUnicodeRanges.set(b, true);
#endif
                }
            }
        }
    }

    return NS_OK;
}

static nsresult
ReadCMAP(HDC hdc, FontEntry *aFontEntry)
{
    const PRUint32 kCMAP = (('c') | ('m' << 8) | ('a' << 16) | ('p' << 24));

    DWORD len = GetFontData(hdc, kCMAP, 0, nsnull, 0);
    NS_ENSURE_TRUE(len != GDI_ERROR && len != 0, NS_ERROR_FAILURE);

    nsAutoTArray<PRUint8,16384> buffer;
    if (!buffer.AppendElements(len))
        return NS_ERROR_OUT_OF_MEMORY;
    PRUint8 *buf = buffer.Elements();

    DWORD newLen = GetFontData(hdc, kCMAP, 0, buf, len);
    NS_ENSURE_TRUE(newLen == len, NS_ERROR_FAILURE);

    enum {
        OffsetVersion = 0,
        OffsetNumTables = 2,
        SizeOfHeader = 4,

        TableOffsetPlatformID = 0,
        TableOffsetEncodingID = 2,
        TableOffsetOffset = 4,
        SizeOfTable = 8,

        SubtableOffsetFormat = 0
    };
    enum {
        PlatformIDMicrosoft = 3
    };
    enum {
        EncodingIDSymbol = 0,
        EncodingIDMicrosoft = 1,
        EncodingIDUCS4 = 10
    };

    PRUint16 version = ReadShortAt(buf, OffsetVersion);
    PRUint16 numTables = ReadShortAt(buf, OffsetNumTables);

    
    PRUint32 keepOffset = 0;
    PRUint32 keepFormat = 0;

    PRUint8 *table = buf + SizeOfHeader;
    for (PRUint16 i = 0; i < numTables; ++i, table += SizeOfTable) {
        const PRUint16 platformID = ReadShortAt(table, TableOffsetPlatformID);
        if (platformID != PlatformIDMicrosoft)
            continue;

        const PRUint16 encodingID = ReadShortAt(table, TableOffsetEncodingID);
        const PRUint32 offset = ReadLongAt(table, TableOffsetOffset);

        NS_ASSERTION(offset < newLen, "ugh");
        const PRUint8 *subtable = buf + offset;
        const PRUint16 format = ReadShortAt(subtable, SubtableOffsetFormat);

        if (encodingID == EncodingIDSymbol) {
            aFontEntry->mUnicodeFont = PR_FALSE;
            aFontEntry->mSymbolFont = PR_TRUE;
            keepFormat = format;
            keepOffset = offset;
            break;
        } else if (format == 4 && encodingID == EncodingIDMicrosoft) {
            keepFormat = format;
            keepOffset = offset;
        } else if (format == 12 && encodingID == EncodingIDUCS4) {
            keepFormat = format;
            keepOffset = offset;
            break; 
        }
    }

    nsresult rv = NS_ERROR_FAILURE;

    if (keepFormat == 12)
        rv = ReadCMAPTableFormat12(buf + keepOffset, len - keepOffset, aFontEntry);
    else if (keepFormat == 4)
        rv = ReadCMAPTableFormat4(buf + keepOffset, len - keepOffset, aFontEntry);

    return rv;
}

PLDHashOperator PR_CALLBACK
gfxWindowsPlatform::FontGetCMapDataProc(nsStringHashKey::KeyType aKey,
                                        nsRefPtr<FontEntry>& aFontEntry,
                                        void* userArg)
{
    if (aFontEntry->mFontType != TRUETYPE_FONTTYPE) {
        






        for (PRUint16 ch = 0x21; ch <= 0xFF; ch++)
            aFontEntry->mCharacterMap.set(ch);
        return PL_DHASH_NEXT;
    }

    HDC hdc = GetDC(nsnull);

    LOGFONTW logFont;
    memset(&logFont, 0, sizeof(LOGFONTW));
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = 0;
    PRUint32 l = PR_MIN(aFontEntry->mName.Length(), LF_FACESIZE - 1);
    memcpy(logFont.lfFaceName,
           nsPromiseFlatString(aFontEntry->mName).get(),
           l * sizeof(PRUnichar));
    logFont.lfFaceName[l] = 0;

    HFONT font = CreateFontIndirectW(&logFont);

    if (font) {
        HFONT oldFont = (HFONT)SelectObject(hdc, font);

        nsresult rv = ReadCMAP(hdc, aFontEntry);

        if (NS_FAILED(rv)) {
            aFontEntry->mUnicodeFont = PR_FALSE;
            
        }

        SelectObject(hdc, oldFont);
        DeleteObject(font);
    }

    ReleaseDC(nsnull, hdc);

    return PL_DHASH_NEXT;
}


struct FontListData {
    FontListData(const nsACString& aLangGroup, const nsACString& aGenericFamily, nsStringArray& aListOfFonts) :
        mLangGroup(aLangGroup), mGenericFamily(aGenericFamily), mStringArray(aListOfFonts) {}
    const nsACString& mLangGroup;
    const nsACString& mGenericFamily;
    nsStringArray& mStringArray;
};

PLDHashOperator PR_CALLBACK
gfxWindowsPlatform::HashEnumFunc(nsStringHashKey::KeyType aKey,
                                 nsRefPtr<FontEntry>& aFontEntry,
                                 void* userArg)
{
    FontListData *data = (FontListData*)userArg;

    
    if (aFontEntry->mSymbolFont)
        return PL_DHASH_NEXT;

    if (aFontEntry->SupportsLangGroup(data->mLangGroup) &&
        aFontEntry->MatchesGenericFamily(data->mGenericFamily))
        data->mStringArray.AppendString(aFontEntry->mName);

    return PL_DHASH_NEXT;
}

nsresult
gfxWindowsPlatform::GetFontList(const nsACString& aLangGroup,
                                const nsACString& aGenericFamily,
                                nsStringArray& aListOfFonts)
{
    FontListData data(aLangGroup, aGenericFamily, aListOfFonts);

    mFonts.Enumerate(gfxWindowsPlatform::HashEnumFunc, &data);

    aListOfFonts.Sort();
    aListOfFonts.Compact();

    return NS_OK;
}

static void
RemoveCharsetFromFontSubstitute(nsAString &aName)
{
    PRInt32 comma = aName.FindChar(PRUnichar(','));
    if (comma >= 0)
        aName.Truncate(comma);
}

static void
BuildKeyNameFromFontName(nsAString &aName)
{
    if (aName.Length() >= LF_FACESIZE)
        aName.Truncate(LF_FACESIZE - 1);
    ToLowerCase(aName);
}

nsresult
gfxWindowsPlatform::UpdateFontList()
{
    mFonts.Clear();
    mFontAliases.Clear();
    mNonExistingFonts.Clear();
    mFontSubstitutes.Clear();

    LOGFONTW logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfFaceName[0] = 0;
    logFont.lfPitchAndFamily = 0;

    
    HDC dc = ::GetDC(nsnull);
    EnumFontFamiliesExW(dc, &logFont, (FONTENUMPROCW)gfxWindowsPlatform::FontEnumProc, (LPARAM)this, 0);
    ::ReleaseDC(nsnull, dc);

    
    mFonts.Enumerate(gfxWindowsPlatform::FontGetCMapDataProc, nsnull);

    
    nsCOMPtr<nsIWindowsRegKey> regKey = do_CreateInstance("@mozilla.org/windows-registry-key;1");
    if (!regKey)
        return NS_ERROR_FAILURE;
     NS_NAMED_LITERAL_STRING(kFontSubstitutesKey, "SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\FontSubstitutes");

    nsresult rv = regKey->Open(nsIWindowsRegKey::ROOT_KEY_LOCAL_MACHINE,
                               kFontSubstitutesKey, nsIWindowsRegKey::ACCESS_READ);
    if (NS_FAILED(rv))
        return rv;

    PRUint32 count;
    rv = regKey->GetValueCount(&count);
    if (NS_FAILED(rv) || count == 0)
        return rv;
    for (PRUint32 i = 0; i < count; i++) {
        nsAutoString substituteName;
        rv = regKey->GetValueName(i, substituteName);
        if (NS_FAILED(rv) || substituteName.IsEmpty() ||
            substituteName.CharAt(1) == PRUnichar('@'))
            continue;
        PRUint32 valueType;
        rv = regKey->GetValueType(substituteName, &valueType);
        if (NS_FAILED(rv) || valueType != nsIWindowsRegKey::TYPE_STRING)
            continue;
        nsAutoString actualFontName;
        rv = regKey->ReadStringValue(substituteName, actualFontName);
        if (NS_FAILED(rv))
            continue;

        RemoveCharsetFromFontSubstitute(substituteName);
        BuildKeyNameFromFontName(substituteName);
        RemoveCharsetFromFontSubstitute(actualFontName);
        BuildKeyNameFromFontName(actualFontName);
        nsRefPtr<FontEntry> fe;
        if (!actualFontName.IsEmpty() && mFonts.Get(actualFontName, &fe))
            mFontSubstitutes.Put(substituteName, fe);
        else
            mNonExistingFonts.AppendString(substituteName);
    }

    return NS_OK;
}

struct ResolveData {
    ResolveData(gfxPlatform::FontResolverCallback aCallback,
                gfxWindowsPlatform *aCaller, const nsAString *aFontName,
                void *aClosure) :
        mFoundCount(0), mCallback(aCallback), mCaller(aCaller),
        mFontName(aFontName), mClosure(aClosure) {}
    PRUint32 mFoundCount;
    gfxPlatform::FontResolverCallback mCallback;
    gfxWindowsPlatform *mCaller;
    const nsAString *mFontName;
    void *mClosure;
};

nsresult
gfxWindowsPlatform::ResolveFontName(const nsAString& aFontName,
                                    FontResolverCallback aCallback,
                                    void *aClosure,
                                    PRBool& aAborted)
{
    if (aFontName.IsEmpty())
        return NS_ERROR_FAILURE;

    nsAutoString keyName(aFontName);
    BuildKeyNameFromFontName(keyName);

    nsRefPtr<FontEntry> fe;
    if (mFonts.Get(keyName, &fe) ||
        mFontSubstitutes.Get(keyName, &fe) ||
        mFontAliases.Get(keyName, &fe)) {
        aAborted = !(*aCallback)(fe->mName, aClosure);
        
        return NS_OK;
    }

    if (mNonExistingFonts.IndexOf(keyName) >= 0) {
        aAborted = PR_FALSE;
        return NS_OK;
    }

    LOGFONTW logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = 0;
    PRInt32 len = aFontName.Length();
    if (len >= LF_FACESIZE)
        len = LF_FACESIZE - 1;
    memcpy(logFont.lfFaceName,
           nsPromiseFlatString(aFontName).get(), len * sizeof(PRUnichar));
    logFont.lfFaceName[len] = 0;

    HDC dc = ::GetDC(nsnull);
    ResolveData data(aCallback, this, &keyName, aClosure);
    aAborted =
        !EnumFontFamiliesExW(dc, &logFont,
                             (FONTENUMPROCW)gfxWindowsPlatform::FontResolveProc,
                             (LPARAM)&data, 0);
    if (data.mFoundCount == 0)
        mNonExistingFonts.AppendString(keyName);
    ::ReleaseDC(nsnull, dc);

    return NS_OK;
}

int CALLBACK 
gfxWindowsPlatform::FontResolveProc(const ENUMLOGFONTEXW *lpelfe,
                                    const NEWTEXTMETRICEXW *nmetrics,
                                    DWORD fontType, LPARAM data)
{
    const LOGFONTW& logFont = lpelfe->elfLogFont;
    
    if (logFont.lfFaceName[0] == L'@' || logFont.lfFaceName[0] == 0)
        return 1;

    ResolveData *rData = reinterpret_cast<ResolveData*>(data);

    nsAutoString name(logFont.lfFaceName);

    
    nsRefPtr<FontEntry> fe;
    nsAutoString keyName(name);
    BuildKeyNameFromFontName(keyName);
    if (!rData->mCaller->mFonts.Get(keyName, &fe)) {
        
        
        
        
        NS_WARNING("Cannot find actual font");
        return 1;
    }

    rData->mFoundCount++;
    rData->mCaller->mFontAliases.Put(*(rData->mFontName), fe);

    return (rData->mCallback)(name, rData->mClosure);

    
}

struct FontSearch {
    FontSearch(const PRUnichar *aString, PRUint32 aLength, gfxWindowsFont *aFont) :
        string(aString), length(aLength), fontToMatch(aFont), matchRank(0) {
    }
    const PRUnichar *string;
    const PRUint32 length;
    nsRefPtr<gfxWindowsFont> fontToMatch;
    PRInt32 matchRank;
    nsRefPtr<FontEntry> bestMatch;
};

PLDHashOperator PR_CALLBACK
gfxWindowsPlatform::FindFontForStringProc(nsStringHashKey::KeyType aKey,
                                          nsRefPtr<FontEntry>& aFontEntry,
                                          void* userArg)
{
    
    if (aFontEntry->IsCrappyFont())
        return PL_DHASH_NEXT;

    FontSearch *data = (FontSearch*)userArg;

    PRInt32 rank = 0;

    for (PRUint32 i = 0; i < data->length; ++i) {
        PRUint32 ch = data->string[i];

        if ((i+1 < data->length) && NS_IS_HIGH_SURROGATE(ch) && NS_IS_LOW_SURROGATE(data->string[i+1])) {
            i++;
            ch = SURROGATE_TO_UCS4(ch, data->string[i]);
        }

        if (aFontEntry->mCharacterMap.test(ch)) {
            rank += 20;

            
            
            if (aFontEntry->SupportsRange(CharRangeBit(ch)))
                rank += 1;
        }
    }

    
    if (rank == 0)
        return PL_DHASH_NEXT;


    if (aFontEntry->SupportsLangGroup(data->fontToMatch->GetStyle()->langGroup))
        rank += 10;

    if (data->fontToMatch->GetFontEntry()->mFamily == aFontEntry->mFamily)
        rank += 5;
    if (data->fontToMatch->GetFontEntry()->mFamily == aFontEntry->mPitch)
        rank += 5;

    
    PRInt8 baseWeight, weightDistance;
    data->fontToMatch->GetStyle()->ComputeWeightAndOffset(&baseWeight, &weightDistance);
    PRUint16 targetWeight = (baseWeight * 100) + (weightDistance * 100);
    if (targetWeight == aFontEntry->mDefaultWeight)
        rank += 5;

    if (rank > data->matchRank ||
        (rank == data->matchRank && Compare(aFontEntry->mName, data->bestMatch->mName) > 0)) {
        data->bestMatch = aFontEntry;
        data->matchRank = rank;
    }

    return PL_DHASH_NEXT;
}


FontEntry *
gfxWindowsPlatform::FindFontForString(const PRUnichar *aString, PRUint32 aLength, gfxWindowsFont *aFont)
{
    FontSearch data(aString, aLength, aFont);

    
    mFonts.Enumerate(gfxWindowsPlatform::FindFontForStringProc, &data);

    return data.bestMatch;
}

gfxFontGroup *
gfxWindowsPlatform::CreateFontGroup(const nsAString &aFamilies,
                                    const gfxFontStyle *aStyle)
{
    return new gfxWindowsFontGroup(aFamilies, aStyle);
}

FontEntry *
gfxWindowsPlatform::FindFontEntry(const nsAString& aName)
{
    nsString name(aName);
    ToLowerCase(name);

    nsRefPtr<FontEntry> fe;
    if (!mFonts.Get(name, &fe) &&
        !mFontSubstitutes.Get(name, &fe) &&
        !mFontAliases.Get(name, &fe)) {
        return nsnull;
    }
    return fe.get();
}
