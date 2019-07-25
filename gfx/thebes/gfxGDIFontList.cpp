








































#ifdef MOZ_LOGGING
#define FORCE_PR_LOG
#endif
#include "prlog.h"

#include "gfxGDIFontList.h"
#include "gfxWindowsPlatform.h"
#include "gfxUserFontSet.h"
#include "gfxFontUtils.h"
#include "gfxGDIFont.h"

#include "nsServiceManagerUtils.h"
#include "nsTArray.h"
#include "nsUnicharUtils.h"

#include "nsDirectoryServiceUtils.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsISimpleEnumerator.h"
#include "nsIWindowsRegKey.h"

#include <usp10.h>

#define ROUND(x) floor((x) + 0.5)


#ifndef CLEARTYPE_QUALITY
#define CLEARTYPE_QUALITY 5
#endif

#ifdef PR_LOGGING
#define LOG_FONTLIST(args) PR_LOG(gfxPlatform::GetLog(eGfxLog_fontlist), \
                               PR_LOG_DEBUG, args)
#define LOG_FONTLIST_ENABLED() PR_LOG_TEST( \
                                   gfxPlatform::GetLog(eGfxLog_fontlist), \
                                   PR_LOG_DEBUG)

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






#ifndef __t2embapi__

#define TTLOAD_PRIVATE                  0x00000001
#define LICENSE_PREVIEWPRINT            0x0004
#define E_NONE                          0x0000L

typedef unsigned long( WINAPIV *READEMBEDPROC ) ( void*, void*, const unsigned long );

typedef struct
{
    unsigned short usStructSize;    
    unsigned short usRefStrSize;    
    unsigned short *pusRefStr;      
}TTLOADINFO;

LONG WINAPI TTLoadEmbeddedFont
(
    HANDLE*  phFontReference,           
                                        
    ULONG    ulFlags,                   
    ULONG*   pulPrivStatus,             
    ULONG    ulPrivs,                   
    ULONG*   pulStatus,                 
    READEMBEDPROC lpfnReadFromStream,   
    LPVOID   lpvReadStream,             
    LPWSTR   szWinFamilyName,           
    LPSTR    szMacFamilyName,           
    TTLOADINFO* pTTLoadInfo             
);

#endif 

typedef LONG( WINAPI *TTLoadEmbeddedFontProc ) (HANDLE* phFontReference, ULONG ulFlags, ULONG* pulPrivStatus, ULONG ulPrivs, ULONG* pulStatus, 
                                             READEMBEDPROC lpfnReadFromStream, LPVOID lpvReadStream, LPWSTR szWinFamilyName, 
                                             LPSTR szMacFamilyName, TTLOADINFO* pTTLoadInfo);

typedef LONG( WINAPI *TTDeleteEmbeddedFontProc ) (HANDLE hFontReference, ULONG ulFlags, ULONG* pulStatus);


static TTLoadEmbeddedFontProc TTLoadEmbeddedFontPtr = nsnull;
static TTDeleteEmbeddedFontProc TTDeleteEmbeddedFontPtr = nsnull;

class WinUserFontData : public gfxUserFontData {
public:
    WinUserFontData(HANDLE aFontRef, PRBool aIsEmbedded)
        : mFontRef(aFontRef), mIsEmbedded(aIsEmbedded)
    { }

    virtual ~WinUserFontData()
    {
        if (mIsEmbedded) {
            ULONG pulStatus;
            LONG err;
            err = TTDeleteEmbeddedFontPtr(mFontRef, 0, &pulStatus);
#if DEBUG
            if (err != E_NONE) {
                char buf[256];
                sprintf(buf, "error deleting embedded font handle (%p) - TTDeleteEmbeddedFont returned %8.8x", mFontRef, err);
                NS_ASSERTION(err == E_NONE, buf);
            }
#endif
        } else {
            BOOL success;
            success = RemoveFontMemResourceEx(mFontRef);
#if DEBUG
            if (!success) {
                char buf[256];
                sprintf(buf, "error deleting font handle (%p) - RemoveFontMemResourceEx failed", mFontRef);
                NS_ASSERTION(success, buf);
            }
#endif
        }
    }
    
    HANDLE mFontRef;
    PRPackedBool mIsEmbedded;
};

BYTE 
FontTypeToOutPrecision(PRUint8 fontType)
{
    BYTE ret;
    switch (fontType) {
    case GFX_FONT_TYPE_TT_OPENTYPE:
    case GFX_FONT_TYPE_TRUETYPE:
        ret = OUT_TT_ONLY_PRECIS;
        break;
    case GFX_FONT_TYPE_PS_OPENTYPE:
        ret = OUT_PS_ONLY_PRECIS;
        break;
    case GFX_FONT_TYPE_TYPE1:
        ret = OUT_OUTLINE_PRECIS;
        break;
    case GFX_FONT_TYPE_RASTER:
        ret = OUT_RASTER_PRECIS;
        break;
    case GFX_FONT_TYPE_DEVICE:
        ret = OUT_DEVICE_PRECIS;
        break;
    default:
        ret = OUT_DEFAULT_PRECIS;
    }
    return ret;
}







GDIFontEntry::GDIFontEntry(const nsAString& aFaceName, gfxWindowsFontType aFontType,
                                   PRBool aItalic, PRUint16 aWeight, gfxUserFontData *aUserFontData) : 
    gfxFontEntry(aFaceName), 
    mWindowsFamily(0), mWindowsPitch(0),
    mFontType(aFontType),
    mForceGDI(PR_FALSE), mUnknownCMAP(PR_FALSE),
    mCharset(), mUnicodeRanges()
{
    mUserFontData = aUserFontData;
    mItalic = aItalic;
    mWeight = aWeight;
    if (IsType1())
        mForceGDI = PR_TRUE;
    mIsUserFont = aUserFontData != nsnull;

    InitLogFont(aFaceName, aFontType);
}

nsresult
GDIFontEntry::ReadCMAP()
{
    
    if (mFontType != GFX_FONT_TYPE_PS_OPENTYPE && 
        mFontType != GFX_FONT_TYPE_TT_OPENTYPE &&
        mFontType != GFX_FONT_TYPE_TRUETYPE) 
    {
        return NS_ERROR_FAILURE;
    }

    
    if (mCmapInitialized)
        return NS_OK;
    mCmapInitialized = PR_TRUE;

    const PRUint32 kCmapTag = TRUETYPE_TAG('c','m','a','p');
    AutoFallibleTArray<PRUint8,16384> buffer;
    if (GetFontTable(kCmapTag, buffer) != NS_OK)
        return NS_ERROR_FAILURE;
    PRUint8 *cmap = buffer.Elements();

    PRPackedBool  unicodeFont = PR_FALSE, symbolFont = PR_FALSE;
    nsresult rv = gfxFontUtils::ReadCMAP(cmap, buffer.Length(),
                                         mCharacterMap, mUVSOffset,
                                         unicodeFont, symbolFont);
    mSymbolFont = symbolFont;
    mHasCmapTable = NS_SUCCEEDED(rv);

#ifdef PR_LOGGING
    LOG_FONTLIST(("(fontlist-cmap) name: %s, size: %d\n",
                  NS_ConvertUTF16toUTF8(mName).get(), mCharacterMap.GetSize()));
#endif
    return rv;
}

PRBool
GDIFontEntry::IsSymbolFont()
{
    
    HasCmapTable();
    return mSymbolFont;  
}

gfxFont *
GDIFontEntry::CreateFontInstance(const gfxFontStyle* aFontStyle, PRBool aNeedsBold)
{
    PRBool isXP = (gfxWindowsPlatform::WindowsOSVersion() 
                       < gfxWindowsPlatform::kWindowsVista);

    PRBool useClearType = isXP && !aFontStyle->systemFont &&
        (gfxWindowsPlatform::GetPlatform()->UseClearTypeAlways() ||
         (mIsUserFont && !mIsLocalUserFont &&
          gfxWindowsPlatform::GetPlatform()->UseClearTypeForDownloadableFonts()));

    return new gfxGDIFont(this, aFontStyle, aNeedsBold, 
                          (useClearType ? gfxFont::kAntialiasSubpixel
                                        : gfxFont::kAntialiasDefault));
}

nsresult
GDIFontEntry::GetFontTable(PRUint32 aTableTag,
                           FallibleTArray<PRUint8>& aBuffer)
{
    if (!IsTrueType()) {
        return NS_ERROR_FAILURE;
    }

    AutoDC dc;
    AutoSelectFont font(dc.GetDC(), &mLogFont);
    if (font.IsValid()) {
        PRInt32 tableSize =
            ::GetFontData(dc.GetDC(), NS_SWAP32(aTableTag), 0, NULL, NULL);
        if (tableSize != GDI_ERROR) {
            if (aBuffer.SetLength(tableSize)) {
                ::GetFontData(dc.GetDC(), NS_SWAP32(aTableTag), 0,
                              aBuffer.Elements(), tableSize);
                return NS_OK;
            }
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    return NS_ERROR_FAILURE;
}

void
GDIFontEntry::FillLogFont(LOGFONTW *aLogFont, PRBool aItalic,
                              PRUint16 aWeight, gfxFloat aSize,
                              PRBool aUseCleartype)
{
    memcpy(aLogFont, &mLogFont, sizeof(LOGFONTW));

    aLogFont->lfHeight = (LONG)-ROUND(aSize);

    if (aLogFont->lfHeight == 0)
        aLogFont->lfHeight = -1;

    
    
    
    
    aLogFont->lfItalic         = aItalic;
    aLogFont->lfWeight         = aWeight;
    aLogFont->lfQuality        = (aUseCleartype ? CLEARTYPE_QUALITY : DEFAULT_QUALITY);
}

#define MISSING_GLYPH 0x1F // glyph index returned for missing characters
                           

PRBool 
GDIFontEntry::TestCharacterMap(PRUint32 aCh)
{
    if (ReadCMAP() != NS_OK) {
        
        
        mUnknownCMAP = PR_TRUE;
    }

    if (mUnknownCMAP) {
        if (aCh > 0xFFFF)
            return PR_FALSE;

        
        gfxFontStyle fakeStyle;  
        if (mItalic)
            fakeStyle.style = FONT_STYLE_ITALIC;
        fakeStyle.weight = mWeight * 100;

        nsRefPtr<gfxFont> tempFont = FindOrMakeFont(&fakeStyle, PR_FALSE);
        if (!tempFont || !tempFont->Valid())
            return PR_FALSE;
        gfxGDIFont *font = static_cast<gfxGDIFont*>(tempFont.get());

        HDC dc = GetDC((HWND)nsnull);
        SetGraphicsMode(dc, GM_ADVANCED);
        HFONT hfont = font->GetHFONT();
        HFONT oldFont = (HFONT)SelectObject(dc, hfont);

        PRUnichar str[1] = { (PRUnichar)aCh };
        WORD glyph[1];

        PRBool hasGlyph = PR_FALSE;

        
        
        
        if (IsType1() || mForceGDI) {
            
            
            DWORD ret = GetGlyphIndicesW(dc, str, 1, 
                                         glyph, GGI_MARK_NONEXISTING_GLYPHS);
            if (ret != GDI_ERROR
                && glyph[0] != 0xFFFF
                && (IsType1() || glyph[0] != MISSING_GLYPH))
            {
                hasGlyph = PR_TRUE;
            }
        } else {
            
            
            SCRIPT_CACHE sc = NULL;
            HRESULT rv = ScriptGetCMap(dc, &sc, str, 1, 0, glyph);
            if (rv == S_OK)
                hasGlyph = PR_TRUE;
        }

        SelectObject(dc, oldFont);
        ReleaseDC(NULL, dc);

        if (hasGlyph) {
            mCharacterMap.set(aCh);
            return PR_TRUE;
        }
    } else {
        
        return mCharacterMap.test(aCh);
    }

    return PR_FALSE;
}

void
GDIFontEntry::InitLogFont(const nsAString& aName,
                              gfxWindowsFontType aFontType)
{
#define CLIP_TURNOFF_FONTASSOCIATION 0x40
    
    mLogFont.lfHeight = -1;

    
    mLogFont.lfWidth          = 0;
    mLogFont.lfEscapement     = 0;
    mLogFont.lfOrientation    = 0;
    mLogFont.lfUnderline      = FALSE;
    mLogFont.lfStrikeOut      = FALSE;
    mLogFont.lfCharSet        = DEFAULT_CHARSET;
    mLogFont.lfOutPrecision   = FontTypeToOutPrecision(aFontType);
    mLogFont.lfClipPrecision  = CLIP_TURNOFF_FONTASSOCIATION;
    mLogFont.lfQuality        = DEFAULT_QUALITY;
    mLogFont.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
    
    
    
    
    mLogFont.lfItalic         = mItalic;
    mLogFont.lfWeight         = mWeight;

    int len = NS_MIN<int>(aName.Length(), LF_FACESIZE - 1);
    memcpy(&mLogFont.lfFaceName, aName.get(), len * 2);
    mLogFont.lfFaceName[len] = '\0';
}

GDIFontEntry* 
GDIFontEntry::CreateFontEntry(const nsAString& aName, gfxWindowsFontType aFontType, 
                                  PRBool aItalic, PRUint16 aWeight, 
                                  gfxUserFontData* aUserFontData)
{
    

    GDIFontEntry *fe = new GDIFontEntry(aName, aFontType, aItalic, aWeight,
                                        aUserFontData);

    return fe;
}







int CALLBACK
GDIFontFamily::FamilyAddStylesProc(const ENUMLOGFONTEXW *lpelfe,
                                        const NEWTEXTMETRICEXW *nmetrics,
                                        DWORD fontType, LPARAM data)
{
    const NEWTEXTMETRICW& metrics = nmetrics->ntmTm;
    LOGFONTW logFont = lpelfe->elfLogFont;
    GDIFontFamily *ff = reinterpret_cast<GDIFontFamily*>(data);

    
    logFont.lfWeight = NS_MAX<LONG>(NS_MIN<LONG>(logFont.lfWeight, 900), 100);

    gfxWindowsFontType feType = GDIFontEntry::DetermineFontType(metrics, fontType);

    GDIFontEntry *fe = nsnull;
    for (PRUint32 i = 0; i < ff->mAvailableFonts.Length(); ++i) {
        fe = static_cast<GDIFontEntry*>(ff->mAvailableFonts[i].get());
        if (feType > fe->mFontType) {
            
            ff->mAvailableFonts.RemoveElementAt(i);
            --i;
        } else if (feType < fe->mFontType) {
            
            return 1;
        }
    }

    for (PRUint32 i = 0; i < ff->mAvailableFonts.Length(); ++i) {
        fe = static_cast<GDIFontEntry*>(ff->mAvailableFonts[i].get());
        
        if (fe->mWeight == logFont.lfWeight &&
            fe->mItalic == (logFont.lfItalic == 0xFF)) {
            
            fe->mCharset.set(metrics.tmCharSet);
            return 1; 
        }
    }

    fe = GDIFontEntry::CreateFontEntry(nsDependentString(lpelfe->elfFullName), feType, (logFont.lfItalic == 0xFF),
                                       (PRUint16) (logFont.lfWeight), nsnull);
    if (!fe)
        return 1;

    ff->AddFontEntry(fe);

    
    fe->mCharset.set(metrics.tmCharSet);

    fe->mWindowsFamily = logFont.lfPitchAndFamily & 0xF0;
    fe->mWindowsPitch = logFont.lfPitchAndFamily & 0x0F;

    if (nmetrics->ntmFontSig.fsUsb[0] != 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[1] != 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[2] != 0x00000000 &&
        nmetrics->ntmFontSig.fsUsb[3] != 0x00000000) {

        
        PRUint32 x = 0;
        for (PRUint32 i = 0; i < 4; ++i) {
            DWORD range = nmetrics->ntmFontSig.fsUsb[i];
            for (PRUint32 k = 0; k < 32; ++k) {
                fe->mUnicodeRanges.set(x++, (range & (1 << k)) != 0);
            }
        }
    }

#ifdef PR_LOGGING
    if (LOG_FONTLIST_ENABLED()) {
        LOG_FONTLIST(("(fontlist) added (%s) to family (%s)"
             " with style: %s weight: %d stretch: %d",
             NS_ConvertUTF16toUTF8(fe->Name()).get(), 
             NS_ConvertUTF16toUTF8(ff->Name()).get(), 
             (logFont.lfItalic == 0xff) ? "italic" : "normal",
             logFont.lfWeight, fe->Stretch()));
    }
#endif
    return 1;
}

void
GDIFontFamily::FindStyleVariations()
{
    if (mHasStyles)
        return;
    mHasStyles = PR_TRUE;

    HDC hdc = GetDC(nsnull);
    SetGraphicsMode(hdc, GM_ADVANCED);

    LOGFONTW logFont;
    memset(&logFont, 0, sizeof(LOGFONTW));
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfPitchAndFamily = 0;
    PRUint32 l = NS_MIN<PRUint32>(mName.Length(), LF_FACESIZE - 1);
    memcpy(logFont.lfFaceName,
           nsPromiseFlatString(mName).get(),
           l * sizeof(PRUnichar));
    logFont.lfFaceName[l] = 0;

    EnumFontFamiliesExW(hdc, &logFont,
                        (FONTENUMPROCW)GDIFontFamily::FamilyAddStylesProc,
                        (LPARAM)this, 0);
#ifdef PR_LOGGING
    if (LOG_FONTLIST_ENABLED() && mAvailableFonts.Length() == 0) {
        LOG_FONTLIST(("(fontlist) no styles available in family \"%s\"",
                      NS_ConvertUTF16toUTF8(mName).get()));
    }
#endif

    ReleaseDC(nsnull, hdc);

    if (mIsBadUnderlineFamily)
        SetBadUnderlineFonts();
}







gfxGDIFontList::gfxGDIFontList()
{
    mFontSubstitutes.Init(50);

    InitializeFontEmbeddingProcs();
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
gfxGDIFontList::GetFontSubstitutes()
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
        lenAlias = NS_ARRAY_LENGTH(aliasName);
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

nsresult
gfxGDIFontList::InitFontList()
{
    gfxFontCache *fc = gfxFontCache::GetCache();
    if (fc)
        fc->AgeAllGenerations();

    
    gfxPlatformFontList::InitFontList();
    
    mFontSubstitutes.Clear();
    mNonExistingFonts.Clear();

    
    LOGFONTW logfont;
    memset(&logfont, 0, sizeof(logfont));
    logfont.lfCharSet = DEFAULT_CHARSET;

    AutoDC hdc;
    int result = EnumFontFamiliesExW(hdc.GetDC(), &logfont,
                                     (FONTENUMPROCW)&EnumFontFamExProc,
                                     0, 0);

    GetFontSubstitutes();

    StartLoader(kDelayBeforeLoadingFonts, kIntervalBetweenLoadingFonts);

    return NS_OK;
}

int CALLBACK
gfxGDIFontList::EnumFontFamExProc(ENUMLOGFONTEXW *lpelfe,
                                      NEWTEXTMETRICEXW *lpntme,
                                      DWORD fontType,
                                      LPARAM lParam)
{
    const LOGFONTW& lf = lpelfe->elfLogFont;

    if (lf.lfFaceName[0] == '@') {
        return 1;
    }

    nsAutoString name(lf.lfFaceName);
    BuildKeyNameFromFontName(name);

    gfxGDIFontList *fontList = PlatformFontList();

    if (!fontList->mFontFamilies.GetWeak(name)) {
        nsDependentString faceName(lf.lfFaceName);
        nsRefPtr<gfxFontFamily> family = new GDIFontFamily(faceName);
        fontList->mFontFamilies.Put(name, family);

        
        
        
        
        if (!IsASCII(faceName)) {
            family->ReadOtherFamilyNames(gfxPlatformFontList::PlatformFontList());
        }

        if (fontList->mBadUnderlineFamilyNames.Contains(name))
            family->SetBadUnderlineFamily();
    }

    return 1;
}

gfxFontEntry* 
gfxGDIFontList::LookupLocalFont(const gfxProxyFontEntry *aProxyEntry,
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

    
    PRUint16 w = (aProxyEntry->mWeight == 0 ? 400 : aProxyEntry->mWeight);
    PRBool isCFF = PR_FALSE; 
    
    
    
    
    
    gfxFontEntry *fe = GDIFontEntry::CreateFontEntry(lookup->Name(), 
        gfxWindowsFontType(isCFF ? GFX_FONT_TYPE_PS_OPENTYPE : GFX_FONT_TYPE_TRUETYPE) , 
        PRUint32(aProxyEntry->mItalic ? FONT_STYLE_ITALIC : FONT_STYLE_NORMAL), 
        w, nsnull);
        
    if (!fe)
        return nsnull;

    fe->mIsUserFont = PR_TRUE;
    fe->mIsLocalUserFont = PR_TRUE;
    return fe;
}

void gfxGDIFontList::InitializeFontEmbeddingProcs()
{
    HMODULE fontlib = LoadLibraryW(L"t2embed.dll");
    if (!fontlib)
        return;
    TTLoadEmbeddedFontPtr = (TTLoadEmbeddedFontProc) GetProcAddress(fontlib, "TTLoadEmbeddedFont");
    TTDeleteEmbeddedFontPtr = (TTDeleteEmbeddedFontProc) GetProcAddress(fontlib, "TTDeleteEmbeddedFont");
}



class EOTFontStreamReader {
public:
    EOTFontStreamReader(const PRUint8 *aFontData, PRUint32 aLength, PRUint8 *aEOTHeader, 
                           PRUint32 aEOTHeaderLen, FontDataOverlay *aNameOverlay)
        : mCurrentChunk(0), mChunkOffset(0)
    {
        NS_ASSERTION(aFontData, "null font data ptr passed in");
        NS_ASSERTION(aEOTHeader, "null EOT header ptr passed in");
        NS_ASSERTION(aNameOverlay, "null name overlay struct passed in");

        if (aNameOverlay->overlaySrc) {
            mNumChunks = 4;
            
            mDataChunks[0].mData = aEOTHeader;
            mDataChunks[0].mLength = aEOTHeaderLen;
            
            mDataChunks[1].mData = aFontData;
            mDataChunks[1].mLength = aNameOverlay->overlayDest;
            
            mDataChunks[2].mData = aFontData + aNameOverlay->overlaySrc;
            mDataChunks[2].mLength = aNameOverlay->overlaySrcLen;
            
            mDataChunks[3].mData = aFontData + aNameOverlay->overlayDest + aNameOverlay->overlaySrcLen;
            mDataChunks[3].mLength = aLength - aNameOverlay->overlayDest - aNameOverlay->overlaySrcLen;
        } else {
            mNumChunks = 2;
            
            mDataChunks[0].mData = aEOTHeader;
            mDataChunks[0].mLength = aEOTHeaderLen;
            
            mDataChunks[1].mData = aFontData;
            mDataChunks[1].mLength = aLength;
        }
    }

    ~EOTFontStreamReader() 
    { 

    }

    struct FontDataChunk {
        const PRUint8 *mData;
        PRUint32       mLength;
    };

    PRUint32                mNumChunks;
    FontDataChunk           mDataChunks[4];
    PRUint32                mCurrentChunk;
    PRUint32                mChunkOffset;

    unsigned long Read(void *outBuffer, const unsigned long aBytesToRead)
    {
        PRUint32 bytesLeft = aBytesToRead;  
        PRUint8 *out = static_cast<PRUint8*> (outBuffer);

        while (mCurrentChunk < mNumChunks && bytesLeft) {
            FontDataChunk& currentChunk = mDataChunks[mCurrentChunk];
            PRUint32 bytesToCopy = NS_MIN(bytesLeft, 
                                          currentChunk.mLength - mChunkOffset);
            memcpy(out, currentChunk.mData + mChunkOffset, bytesToCopy);
            bytesLeft -= bytesToCopy;
            mChunkOffset += bytesToCopy;
            out += bytesToCopy;

            NS_ASSERTION(mChunkOffset <= currentChunk.mLength, "oops, buffer overrun");

            if (mChunkOffset == currentChunk.mLength) {
                mCurrentChunk++;
                mChunkOffset = 0;
            }
        }

        return aBytesToRead - bytesLeft;
    }

    static unsigned long ReadEOTStream(void *aReadStream, void *outBuffer, 
                                       const unsigned long aBytesToRead) 
    {
        EOTFontStreamReader *eotReader = 
                               static_cast<EOTFontStreamReader*> (aReadStream);
        return eotReader->Read(outBuffer, aBytesToRead);
    }        
        
};

gfxFontEntry* 
gfxGDIFontList::MakePlatformFont(const gfxProxyFontEntry *aProxyEntry, 
                                 const PRUint8 *aFontData,
                                 PRUint32 aLength)
{
    
    
    
    struct FontDataDeleter {
        FontDataDeleter(const PRUint8 *aFontData)
            : mFontData(aFontData) { }
        ~FontDataDeleter() { NS_Free((void*)mFontData); }
        const PRUint8 *mFontData;
    };
    FontDataDeleter autoDelete(aFontData);

    
    if (!TTLoadEmbeddedFontPtr || !TTDeleteEmbeddedFontPtr)
        return nsnull;

    PRBool hasVertical;
    PRBool isCFF = gfxFontUtils::IsCffFont(aFontData, hasVertical);

    nsresult rv;
    HANDLE fontRef = nsnull;
    PRBool isEmbedded = PR_FALSE;

    nsAutoString uniqueName;
    rv = gfxFontUtils::MakeUniqueUserFontName(uniqueName);
    if (NS_FAILED(rv))
        return nsnull;

    
    if (!isCFF) {
        
        AutoFallibleTArray<PRUint8,2048> eotHeader;
        PRUint8 *buffer;
        PRUint32 eotlen;

        isEmbedded = PR_TRUE;
        PRUint32 nameLen = NS_MIN<PRUint32>(uniqueName.Length(), LF_FACESIZE - 1);
        nsPromiseFlatString fontName(Substring(uniqueName, 0, nameLen));
        
        FontDataOverlay overlayNameData = {0, 0, 0};

        rv = gfxFontUtils::MakeEOTHeader(aFontData, aLength, &eotHeader, 
                                         &overlayNameData);
        if (NS_SUCCEEDED(rv)) {

            
            eotlen = eotHeader.Length();
            buffer = reinterpret_cast<PRUint8*> (eotHeader.Elements());
            
            PRInt32 ret;
            ULONG privStatus, pulStatus;
            EOTFontStreamReader eotReader(aFontData, aLength, buffer, eotlen,
                                          &overlayNameData);

            ret = TTLoadEmbeddedFontPtr(&fontRef, TTLOAD_PRIVATE, &privStatus,
                                       LICENSE_PREVIEWPRINT, &pulStatus,
                                       EOTFontStreamReader::ReadEOTStream,
                                       &eotReader,
                                       (PRUnichar*)(fontName.get()), 0, 0);
            if (ret != E_NONE) {
                fontRef = nsnull;
                char buf[256];
                sprintf(buf, "font (%s) not loaded using TTLoadEmbeddedFont - error %8.8x", NS_ConvertUTF16toUTF8(aProxyEntry->FamilyName()).get(), ret);
                NS_WARNING(buf);
            }
        }
    }

    
    if (fontRef == nsnull) {
        
        FallibleTArray<PRUint8> newFontData;

        isEmbedded = PR_FALSE;
        rv = gfxFontUtils::RenameFont(uniqueName, aFontData, aLength, &newFontData);

        if (NS_FAILED(rv))
            return nsnull;
        
        DWORD numFonts = 0;

        PRUint8 *fontData = reinterpret_cast<PRUint8*> (newFontData.Elements());
        PRUint32 fontLength = newFontData.Length();
        NS_ASSERTION(fontData, "null font data after renaming");

        
        
        
        fontRef = AddFontMemResourceEx(fontData, fontLength, 
                                       0 , &numFonts);
        if (!fontRef)
            return nsnull;

        
        
        
        if (fontRef && numFonts != 1 + !!hasVertical) {
            RemoveFontMemResourceEx(fontRef);
            return nsnull;
        }
    }

    
    WinUserFontData *winUserFontData = new WinUserFontData(fontRef, isEmbedded);
    PRUint16 w = (aProxyEntry->mWeight == 0 ? 400 : aProxyEntry->mWeight);

    GDIFontEntry *fe = GDIFontEntry::CreateFontEntry(uniqueName, 
        gfxWindowsFontType(isCFF ? GFX_FONT_TYPE_PS_OPENTYPE : GFX_FONT_TYPE_TRUETYPE) , 
        PRUint32(aProxyEntry->mItalic ? FONT_STYLE_ITALIC : FONT_STYLE_NORMAL), 
        w, winUserFontData);

    if (!fe)
        return fe;

    fe->mIsUserFont = PR_TRUE;

    
    
    if (isCFF && gfxWindowsPlatform::WindowsOSVersion() 
                 < gfxWindowsPlatform::kWindows7) {
        fe->mForceGDI = PR_TRUE;
    }
 
    return fe;
}

gfxFontEntry*
gfxGDIFontList::GetDefaultFont(const gfxFontStyle* aStyle, PRBool& aNeedsBold)
{
    
    HGDIOBJ hGDI = ::GetStockObject(DEFAULT_GUI_FONT);
    LOGFONTW logFont;
    if (hGDI && ::GetObjectW(hGDI, sizeof(logFont), &logFont)) {
        nsAutoString resolvedName;
        if (ResolveFontName(nsDependentString(logFont.lfFaceName), resolvedName)) {
            return FindFontForFamily(resolvedName, aStyle, aNeedsBold);
        }
    }

    
    NONCLIENTMETRICSW ncm;
    ncm.cbSize = sizeof(ncm);
    BOOL status = ::SystemParametersInfoW(SPI_GETNONCLIENTMETRICS, 
                                          sizeof(ncm), &ncm, 0);
    if (status) {
        nsAutoString resolvedName;
        if (ResolveFontName(nsDependentString(ncm.lfMessageFont.lfFaceName), resolvedName)) {
            return FindFontForFamily(resolvedName, aStyle, aNeedsBold);
        }
    }

    return nsnull;
}


PRBool 
gfxGDIFontList::ResolveFontName(const nsAString& aFontName, nsAString& aResolvedFontName)
{
    nsAutoString keyName(aFontName);
    BuildKeyNameFromFontName(keyName);

    nsRefPtr<gfxFontFamily> ff;
    if (mFontSubstitutes.Get(keyName, &ff)) {
        aResolvedFontName = ff->Name();
        return PR_TRUE;
    }

    if (mNonExistingFonts.Contains(keyName))
        return PR_FALSE;

    if (gfxPlatformFontList::ResolveFontName(aFontName, aResolvedFontName))
        return PR_TRUE;

    return PR_FALSE;
}
