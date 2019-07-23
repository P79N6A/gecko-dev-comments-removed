









































#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsILanguageAtomService.h"
#include "nsICharsetConverterManager.h"
#include "nsICharRepresentable.h"
#include "nsISaveAsCharset.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsFontMetricsWin.h"
#include "nsQuickSort.h"
#include "nsTextFormatter.h"
#include "nsIPersistentProperties2.h"
#include "nsNetUtil.h"
#include "prmem.h"
#include "plhash.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include "nsUnicodeRange.h"
#include "nsAutoBuffer.h"

#define DEFAULT_TTF_SYMBOL_ENCODING "windows-1252"
#define IS_RTL_PRESENTATION_FORM(c) ((0xfb1d <= (c)) && ((c)<= 0xfefc))

#define NOT_SETUP 0x33
static PRBool gIsWIN95OR98 = NOT_SETUP;

PRBool IsWin95OrWin98()
{
#ifndef WINCE
  if (NOT_SETUP == gIsWIN95OR98) {
    OSVERSIONINFO os;
    os.dwOSVersionInfoSize = sizeof(os);
    ::GetVersionEx(&os);
    
    if (VER_PLATFORM_WIN32_WINDOWS == os.dwPlatformId) {
      gIsWIN95OR98 = PR_TRUE;
    }
    else {
      gIsWIN95OR98 = PR_FALSE;
    }
  }
  return gIsWIN95OR98;
#else
  return PR_TRUE;
#endif
}

extern PRBool UseAFunctions();

#undef USER_DEFINED
#define USER_DEFINED "x-user-def"


#define NS_REPLACEMENT_CHAR  PRUnichar(0x003F) // question mark





enum eCharset
{
  eCharset_DEFAULT = 0,
  eCharset_ANSI,
  eCharset_EASTEUROPE,
  eCharset_RUSSIAN,
  eCharset_GREEK,
  eCharset_TURKISH,
  eCharset_HEBREW,
  eCharset_ARABIC,
  eCharset_BALTIC,
  eCharset_THAI,
  eCharset_SHIFTJIS,
  eCharset_GB2312,
  eCharset_HANGEUL,
  eCharset_CHINESEBIG5,
  eCharset_JOHAB,
  eCharset_COUNT
};

struct nsCharsetInfo
{
  char*    mName;
  PRUint16 mCodePage;
  char*    mLangGroup;
  PRUint16* (*GenerateMap)(nsCharsetInfo* aSelf);
  PRUint16* mCCMap;
};

static PRUint16* GenerateDefault(nsCharsetInfo* aSelf);
static PRUint16* GenerateSingleByte(nsCharsetInfo* aSelf);
static PRUint16* GenerateMultiByte(nsCharsetInfo* aSelf);
static PRBool    LookupWinFontName(const nsAFlatString& aName,
                                   nsAString& aWinName);


nsVoidArray* nsFontMetricsWin::gGlobalFonts = nsnull;
PLHashTable* nsFontMetricsWin::gFontWeights = nsnull;
PLHashTable* nsFontMetricsWin::gFontMaps = nsnull;
PRUint16* nsFontMetricsWin::gEmptyCCMap = nsnull;

#define NS_MAX_FONT_WEIGHT 900
#define NS_MIN_FONT_WEIGHT 100


static nsIPersistentProperties* gFontEncodingProperties = nsnull;
static nsIPersistentProperties* gFontNameMapProperties = nsnull;
static nsICharsetConverterManager* gCharsetManager = nsnull;
static nsIUnicodeEncoder* gUserDefinedConverter = nsnull;
static nsISaveAsCharset* gFontSubstituteConverter = nsnull;
static nsIPref* gPref = nsnull;

static nsIAtom* gUsersLocale = nsnull;
static nsIAtom* gSystemLocale = nsnull;
static nsIAtom* gUserDefined = nsnull;
static nsIAtom* gJA = nsnull;
static nsIAtom* gKO = nsnull;
static nsIAtom* gZHTW = nsnull;
static nsIAtom* gZHCN = nsnull;
static nsIAtom* gZHHK = nsnull;

static nsString* gCodepageStr = nsnull;

static int gInitialized = 0;
static PRBool gDoingLineheightFixup = PR_FALSE;
static PRUint16* gUserDefinedCCMap = nsnull;





static nsFontWin* gFontForIgnorable = nsnull;

#include "ignorable.x-ccmap"
DEFINE_X_CCMAP(gIgnorableCCMapExt, );

static nsCharsetInfo gCharsetInfo[eCharset_COUNT] =
{
  { "DEFAULT",     0,    "",               GenerateDefault,    nsnull},
  { "ANSI",        1252, "x-western",      GenerateSingleByte, nsnull},
  { "EASTEUROPE",  1250, "x-central-euro", GenerateSingleByte, nsnull},
  { "RUSSIAN",     1251, "x-cyrillic",     GenerateSingleByte, nsnull},
  { "GREEK",       1253, "el",             GenerateSingleByte, nsnull},
  { "TURKISH",     1254, "tr",             GenerateSingleByte, nsnull},
  { "HEBREW",      1255, "he",             GenerateSingleByte, nsnull},
  { "ARABIC",      1256, "ar",             GenerateSingleByte, nsnull},
  { "BALTIC",      1257, "x-baltic",       GenerateSingleByte, nsnull},
  { "THAI",        874,  "th",             GenerateSingleByte, nsnull},
  { "SHIFTJIS",    932,  "ja",             GenerateMultiByte,  nsnull},
  { "GB2312",      936,  "zh-CN",          GenerateMultiByte,  nsnull},
  { "HANGEUL",     949,  "ko",             GenerateMultiByte,  nsnull},
  { "CHINESEBIG5", 950,  "zh-TW",          GenerateMultiByte,  nsnull},
  { "JOHAB",       1361, "ko-XXX",         GenerateMultiByte,  nsnull}
};

static void
FreeGlobals(void)
{
  gInitialized = 0;

  NS_IF_RELEASE(gFontEncodingProperties);
  NS_IF_RELEASE(gFontNameMapProperties);
  NS_IF_RELEASE(gCharsetManager);
  NS_IF_RELEASE(gPref);
  NS_IF_RELEASE(gUsersLocale);
  NS_IF_RELEASE(gSystemLocale);
  NS_IF_RELEASE(gUserDefined);
  NS_IF_RELEASE(gUserDefinedConverter);
  if (gUserDefinedCCMap)
    FreeCCMap(gUserDefinedCCMap);
  NS_IF_RELEASE(gJA);
  NS_IF_RELEASE(gKO);
  NS_IF_RELEASE(gZHTW);
  NS_IF_RELEASE(gZHCN);
  NS_IF_RELEASE(gZHHK);

  
  if (nsFontMetricsWin::gFontMaps) {
    PL_HashTableDestroy(nsFontMetricsWin::gFontMaps);
    nsFontMetricsWin::gFontMaps = nsnull;
    if (nsFontMetricsWin::gEmptyCCMap) {
      FreeCCMap(nsFontMetricsWin::gEmptyCCMap);
      nsFontMetricsWin::gEmptyCCMap = nsnull;
    }
  }

  if (nsFontMetricsWin::gGlobalFonts) {
    for (int i = nsFontMetricsWin::gGlobalFonts->Count()-1; i >= 0; --i) {
      nsGlobalFont* font = (nsGlobalFont*)nsFontMetricsWin::gGlobalFonts->ElementAt(i);
      delete font;
    }
    delete nsFontMetricsWin::gGlobalFonts;
    nsFontMetricsWin::gGlobalFonts = nsnull;
  }

  if (gFontForIgnorable) {
    delete gFontForIgnorable;
    gFontForIgnorable = nsnull;
  }

  if (gCodepageStr) {
    delete gCodepageStr;
    gCodepageStr = nsnull; 
  }

  
  if (nsFontMetricsWin::gFontWeights) {
    PL_HashTableDestroy(nsFontMetricsWin::gFontWeights);
    nsFontMetricsWin::gFontWeights = nsnull;
  }

  
  for (int i = 0; i < eCharset_COUNT; ++i) {
    if (gCharsetInfo[i].mCCMap) {
      FreeCCMap(gCharsetInfo[i].mCCMap);
      gCharsetInfo[i].mCCMap = nsnull;
    }
  }
}

class nsFontCleanupObserver : public nsIObserver {
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  nsFontCleanupObserver() { }
  virtual ~nsFontCleanupObserver() {}
};

NS_IMPL_ISUPPORTS1(nsFontCleanupObserver, nsIObserver)

NS_IMETHODIMP nsFontCleanupObserver::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
  if (!nsCRT::strcmp(aTopic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    FreeGlobals();
  }
  return NS_OK;
}

static nsFontCleanupObserver *gFontCleanupObserver;

#undef CHAR_BUFFER_SIZE
#define CHAR_BUFFER_SIZE 1024

typedef nsAutoBuffer<char, CHAR_BUFFER_SIZE> nsAutoCharBuffer;
typedef nsAutoBuffer<PRUnichar, CHAR_BUFFER_SIZE> nsAutoChar16Buffer;

class nsFontSubset : public nsFontWin
{
public:
  nsFontSubset();
  virtual ~nsFontSubset();

  virtual PRInt32 GetWidth(HDC aDC, const PRUnichar* aString,
                           PRUint32 aLength);
  virtual void DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
                          const PRUnichar* aString, PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult
  GetBoundingMetrics(HDC                aDC, 
                     const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics);
#ifdef NS_DEBUG
  virtual void DumpFontInfo();
#endif 
#endif

  int Load(HDC aDC, nsFontMetricsWinA* aFontMetricsWin, nsFontWinA* aFont);

  
  virtual void Convert(const PRUnichar* aString, PRUint32 aLength,
                       nsAutoCharBuffer& aResult, PRUint32* aResultLength);

  BYTE     mCharset;
  PRUint16 mCodePage;
};

class nsFontSubsetSubstitute : public nsFontSubset
{
public:
  nsFontSubsetSubstitute(PRBool aIsForIgnorable = FALSE);
  virtual ~nsFontSubsetSubstitute();

  virtual PRInt32 GetWidth(HDC aDC, const PRUnichar* aString, PRUint32 aLength);
  virtual void DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
                          const PRUnichar* aString, PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult
  GetBoundingMetrics(HDC                aDC,
                     const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics);
#ifdef NS_DEBUG
  virtual void DumpFontInfo();
#endif 
#endif

  
  virtual void Convert(const PRUnichar* aString, PRUint32 aLength,
                       nsAutoCharBuffer& aResult, PRUint32* aResultLength);
  
  
  virtual PRBool HasGlyph(PRUnichar ch) {return PR_TRUE;};

private:
  PRBool mIsForIgnorable;
};

static nsresult
InitGlobals(void)
{
  CallGetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &gCharsetManager);
  if (!gCharsetManager) {
    FreeGlobals();
    return NS_ERROR_FAILURE;
  }
  CallGetService(NS_PREF_CONTRACTID, &gPref);
  if (!gPref) {
    FreeGlobals();
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  PRInt32 intPref;
  if (NS_SUCCEEDED(gPref->GetIntPref(
      "browser.display.normal_lineheight_calc_control", &intPref)))
    gDoingLineheightFixup = (intPref != 0);

  nsCOMPtr<nsILanguageAtomService> langService;
  langService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);
  if (langService) {
    NS_IF_ADDREF(gUsersLocale = langService->GetLocaleLanguageGroup());
  }
  if (!gUsersLocale) {
    gUsersLocale = NS_NewAtom("x-western");
  }
  if (!gUsersLocale) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  if (!gCodepageStr) {
    gCodepageStr = new nsString(NS_LITERAL_STRING(".cp"));
  }
  if (!gCodepageStr) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  UINT cp = ::GetACP();
  gCodepageStr->AppendInt(cp); 
  
  if (!gSystemLocale) {
    for (int i = 1; i < eCharset_COUNT; ++i) {
      if (gCharsetInfo[i].mCodePage == cp) {
        gSystemLocale = NS_NewAtom(gCharsetInfo[i].mLangGroup);
        break;
      }
    }
  }
  if (!gSystemLocale) {
    gSystemLocale = gUsersLocale;
    NS_ADDREF(gSystemLocale);
  }

  gUserDefined = NS_NewAtom(USER_DEFINED);
  if (!gUserDefined) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  gJA = NS_NewAtom("ja");
  if (!gJA) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  gKO = NS_NewAtom("ko");
  if (!gKO) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  gZHCN = NS_NewAtom("zh-CN");
  if (!gZHCN) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  gZHTW = NS_NewAtom("zh-TW");
  if (!gZHTW) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }
  gZHHK = NS_NewAtom("zh-HK");
  if (!gZHHK) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!UseAFunctions()) {
    gFontForIgnorable = new nsFontWinSubstitute(gIgnorableCCMapExt); 
    if (!gFontForIgnorable) {
      FreeGlobals();
      return NS_ERROR_OUT_OF_MEMORY;
    }
  } else {
    nsFontWinSubstituteA* font = new nsFontWinSubstituteA(gIgnorableCCMapExt);
    gFontForIgnorable = font;
    if (!gFontForIgnorable) {
      FreeGlobals();
      return NS_ERROR_OUT_OF_MEMORY;
    }
    font->mSubsets = (nsFontSubset**)nsMemory::Alloc(sizeof(nsFontSubset*));
    if (!font->mSubsets) {
      FreeGlobals();
      return NS_ERROR_OUT_OF_MEMORY;
    }
    font->mSubsets[0] = nsnull;
    nsFontSubsetSubstitute* subset = new nsFontSubsetSubstitute(TRUE);
    if (!subset) {
      FreeGlobals();
      return NS_ERROR_OUT_OF_MEMORY;
    }
    font->mSubsetsCount = 1;
    font->mSubsets[0] = subset;
  }

  
  gFontCleanupObserver = new nsFontCleanupObserver();
  if (!gFontCleanupObserver) {
    FreeGlobals();
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1", &rv));
  if (NS_SUCCEEDED(rv)) {
    rv = observerService->AddObserver(gFontCleanupObserver, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
  }
  gInitialized = 1;

  return NS_OK;
}

nsFontMetricsWin::nsFontMetricsWin()
{
}
  
nsFontMetricsWin::~nsFontMetricsWin()
{
  mSubstituteFont = nsnull; 
  mFontHandle = nsnull; 

  
  for (PRInt32 i = mLoadedFonts.Count()-1; i > 0; --i) {
    delete (nsFontWin*)mLoadedFonts[i];
  }
  mLoadedFonts.Clear();

  if (mDeviceContext) {
    
    mDeviceContext->FontMetricsDeleted(this);
    mDeviceContext = nsnull;
  }
}

#ifdef LEAK_DEBUG
nsrefcnt
nsFontMetricsWin::AddRef()
{
  NS_PRECONDITION(mRefCnt != 0, "resurrecting a dead object");
  return ++mRefCnt;
}

nsrefcnt
nsFontMetricsWin::Release()
{
  NS_PRECONDITION(mRefCnt != 0, "too many release's");
  if (--mRefCnt == 0) {
    delete this;
  }
  return mRefCnt;
}

NS_IMPL_QUERY_INTERFACE1(nsFontMetricsWin, nsIFontMetrics)
  
#else
NS_IMPL_ISUPPORTS1(nsFontMetricsWin, nsIFontMetrics)
#endif

NS_IMETHODIMP
nsFontMetricsWin::Init(const nsFont& aFont, nsIAtom* aLangGroup,
  nsIDeviceContext *aContext)
{
  nsresult res;
  if (!gInitialized) {
    res = InitGlobals();
    
    NS_ASSERTION(NS_SUCCEEDED(res), "No font at all has been created");
    if (NS_FAILED(res)) {
      return res;
    }
  }

  mFont = aFont;
  mLangGroup = aLangGroup;

  
  mDeviceContext = (nsDeviceContextWin *)aContext;
  return RealizeFont();
}

NS_IMETHODIMP
nsFontMetricsWin::Destroy()
{
  mDeviceContext = nsnull;
  return NS_OK;
}







#define CLIP_TURNOFF_FONTASSOCIATION 0x40

void
nsFontMetricsWin::FillLogFont(LOGFONT* logFont, PRInt32 aWeight,
  PRBool aSizeOnly)
{
  float app2dev;
  app2dev = mDeviceContext->AppUnitsToDevUnits();
  logFont->lfHeight = - NSToIntRound(mFont.size * app2dev);

  if (logFont->lfHeight == 0) {
    logFont->lfHeight = -1;
  }

  
  if (aSizeOnly) return;

  
  logFont->lfWidth          = 0; 
  logFont->lfEscapement     = 0;
  logFont->lfOrientation    = 0;
  logFont->lfUnderline      =
    (mFont.decorations & NS_FONT_DECORATION_UNDERLINE)
    ? TRUE : FALSE;
  logFont->lfStrikeOut      =
    (mFont.decorations & NS_FONT_DECORATION_LINE_THROUGH)
    ? TRUE : FALSE;
#ifndef WINCE
  logFont->lfCharSet        = mIsUserDefined ? ANSI_CHARSET : DEFAULT_CHARSET;
  logFont->lfOutPrecision   = OUT_TT_PRECIS;
  logFont->lfClipPrecision  = CLIP_TURNOFF_FONTASSOCIATION;
#else
  logFont->lfCharSet        = DEFAULT_CHARSET;
  logFont->lfOutPrecision   = OUT_DEFAULT_PRECIS;
  logFont->lfClipPrecision  = CLIP_DEFAULT_PRECIS;
#endif
  logFont->lfQuality        = DEFAULT_QUALITY;
  logFont->lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;
  logFont->lfWeight = aWeight;
  logFont->lfItalic = (mFont.style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE))
    ? TRUE : FALSE;   

#ifdef NS_DEBUG
  
  memset(logFont->lfFaceName, 0, sizeof(logFont->lfFaceName));
#endif
}

#undef CFF
#define CFF  (('C') | ('F' << 8) | ('F' << 16) | (' ' << 24))
#undef CMAP
#define CMAP (('c') | ('m' << 8) | ('a' << 16) | ('p' << 24))
#undef HEAD
#define HEAD (('h') | ('e' << 8) | ('a' << 16) | ('d' << 24))
#undef LOCA
#define LOCA (('l') | ('o' << 8) | ('c' << 16) | ('a' << 24))
#undef NAME
#define NAME (('n') | ('a' << 8) | ('m' << 16) | ('e' << 24))

#ifdef IS_BIG_ENDIAN 
# undef GET_SHORT
# define GET_SHORT(p) (*((PRUint16*)p))
# undef GET_LONG
# define GET_LONG(p)  (*((PRUint32*)p))
#else
# ifdef IS_LITTLE_ENDIAN 
#  undef GET_SHORT
#  define GET_SHORT(p) (((p)[0] << 8) | (p)[1])
#  undef GET_LONG
#  define GET_LONG(p) (((p)[0] << 24) | ((p)[1] << 16) | ((p)[2] << 8) | (p)[3])
# endif
#endif


#define AUTO_FONTDATA_BUFFER_SIZE 16384 /* 16K */

typedef nsAutoBuffer<PRUint8, AUTO_FONTDATA_BUFFER_SIZE> nsAutoFontDataBuffer;

static PRUint16
GetGlyphIndex(PRUint16 segCount, PRUint16* endCode, PRUint16* startCode,
  PRUint16* idRangeOffset, PRUint16* idDelta, PRUint8* end, PRUint16 aChar)
{
  PRUint16 glyphIndex = 0;
  PRUint16 i;
  for (i = 0; i < segCount; ++i) {
    if (endCode[i] >= aChar) {
      break;
    }
  }
  PRUint16 startC = startCode[i];
  if (startC <= aChar) {
    if (idRangeOffset[i]) {
      PRUint16* p =
        (idRangeOffset[i]/2 + (aChar - startC) + &idRangeOffset[i]);
      if ((PRUint8*) p < end) {
        if (*p) {
          glyphIndex = (idDelta[i] + *p) % 65536;
        }
      }
    }
    else {
      glyphIndex = (idDelta[i] + aChar) % 65536;
    }
  }

  return glyphIndex;
}

enum eGetNameError
{
  eGetName_OK = 0,    
  eGetName_GDIError,  
  eGetName_OtherError 
};

static eGetNameError
GetNAME(HDC aDC, nsString* aName, PRBool* aIsSymbolEncoding = nsnull)
{
#ifdef WINCE
  return eGetName_GDIError;
#else

  DWORD len = GetFontData(aDC, NAME, 0, nsnull, 0);
  if (len == GDI_ERROR) {
    TEXTMETRIC metrics;
    if (::GetTextMetrics(aDC, &metrics) == 0) 
      return eGetName_OtherError;
    return (metrics.tmPitchAndFamily & TMPF_TRUETYPE) ?
      eGetName_OtherError : eGetName_GDIError;
  }
  if (!len) {
    return eGetName_OtherError;
  }
  nsAutoFontDataBuffer buffer;
  if (!buffer.EnsureElemCapacity(len)) {
    return eGetName_OtherError;
  }
  PRUint8* buf = buffer.get();

  DWORD newLen = GetFontData(aDC, NAME, 0, buf, len);
  if (newLen != len) {
    return eGetName_OtherError;
  }
  PRUint8* p = buf + 2;
  PRUint16 n = GET_SHORT(p);
  p += 2;
  PRUint16 offset = GET_SHORT(p);
  p += 2;
  PRUint16 i;
  PRUint16 idLength;
  PRUint16 idOffset;
  for (i = 0; i < n; ++i) {
    PRUint16 platform = GET_SHORT(p);
    p += 2;
    PRUint16 encoding = GET_SHORT(p);
    p += 4;
    PRUint16 name = GET_SHORT(p);
    p += 2;
    idLength = GET_SHORT(p);
    p += 2;
    idOffset = GET_SHORT(p);
    p += 2;
    
    if ((platform == 3) && ((encoding == 1) || (!encoding)) && (name == 3)) {
      if (aIsSymbolEncoding) {
        *aIsSymbolEncoding = encoding == 0;
      }
      break;
    }
  }
  if (i == n) {
    return eGetName_OtherError;
  }
  p = buf + offset + idOffset;
  idLength /= 2;
  
  
  
  
  PRUnichar c = '0';
  aName->Append(c);
  for (i = 0; i < idLength; ++i) {
    c = GET_SHORT(p);
    p += 2;
    aName->Append(c);
  }

  return eGetName_OK;
#endif
}

static PLHashNumber
HashKey(const void* aString)
{
  const nsString* str = (const nsString*)aString;
  return (PLHashNumber)
    nsCRT::HashCode(str->get());
}

static PRIntn
CompareKeys(const void* aStr1, const void* aStr2)
{
  return nsCRT::strcmp(((const nsString*) aStr1)->get(),
    ((const nsString*) aStr2)->get()) == 0;
}

static int
GetIndexToLocFormat(HDC aDC)
{
  PRUint16 indexToLocFormat;
  if (GetFontData(aDC, HEAD, 50, &indexToLocFormat, 2) != 2) {
    return -1;
  }
  if (!indexToLocFormat) {
    return 0;
  }
  return 1;
}

static nsresult
GetSpaces(HDC aDC, PRBool* aIsCFFOutline, PRUint32* aMaxGlyph,
  nsAutoFontDataBuffer& aIsSpace)
{
  
  DWORD len = GetFontData(aDC, CFF, 0, nsnull, 0);
  if ((len != GDI_ERROR) && len) {
    *aIsCFFOutline = PR_TRUE;
    return NS_OK;
  }
  *aIsCFFOutline = PR_FALSE;
  int isLong = GetIndexToLocFormat(aDC);
  if (isLong < 0) {
    return NS_ERROR_FAILURE;
  }
  len = GetFontData(aDC, LOCA, 0, nsnull, 0);
  if ((len == GDI_ERROR) || (!len)) {
    return NS_ERROR_FAILURE;
  }
  if (!aIsSpace.EnsureElemCapacity(len)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  PRUint8* buf = aIsSpace.get();
  DWORD newLen = GetFontData(aDC, LOCA, 0, buf, len);
  if (newLen != len) {
    return NS_ERROR_FAILURE;
  }
  if (isLong) {
    DWORD longLen = ((len / 4) - 1);
    *aMaxGlyph = longLen;
    PRUint32* longBuf = (PRUint32*) buf;
    for (PRUint32 i = 0; i < longLen; ++i) {
      if (longBuf[i] == longBuf[i+1]) {
        buf[i] = 1;
      }
      else {
        buf[i] = 0;
      }
    }
  }
  else {
    DWORD shortLen = ((len / 2) - 1);
    *aMaxGlyph = shortLen;
    PRUint16* shortBuf = (PRUint16*) buf;
    for (PRUint16 i = 0; i < shortLen; ++i) {
      if (shortBuf[i] == shortBuf[i+1]) {
        buf[i] = 1;
      }
      else {
        buf[i] = 0;
      }
    }
  }

  return NS_OK;
}



static PRUint8 gBitToCharset[64] =
{
 ANSI_CHARSET,
 EASTEUROPE_CHARSET,
 RUSSIAN_CHARSET,
 GREEK_CHARSET,
 TURKISH_CHARSET,
 HEBREW_CHARSET,
 ARABIC_CHARSET,
 BALTIC_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 THAI_CHARSET,
 SHIFTJIS_CHARSET,
 GB2312_CHARSET,
 HANGEUL_CHARSET,
 CHINESEBIG5_CHARSET,
 JOHAB_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET,
 DEFAULT_CHARSET
};

static eCharset gCharsetToIndex[256] =
{
   eCharset_ANSI,
   eCharset_DEFAULT,
   eCharset_DEFAULT, 
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT, 
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_SHIFTJIS,
   eCharset_HANGEUL,
   eCharset_JOHAB,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_GB2312,
   eCharset_DEFAULT,
   eCharset_CHINESEBIG5,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_GREEK,
   eCharset_TURKISH,
   eCharset_DEFAULT, 
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_HEBREW,
   eCharset_ARABIC,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_BALTIC,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_RUSSIAN,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_THAI,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_EASTEUROPE,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT,
   eCharset_DEFAULT  
};

static PRUint8 gCharsetToBit[eCharset_COUNT] =
{
  -1,  
   0,  
   1,  
   2,  
   3,  
   4,  
   5,  
   6,  
   7,  
  16,  
  17,  
  18,  
  19,  
  20,  
  21   
};






static PRUint8 gBitToUnicodeRange[] = 
{
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeGreek,      
  kRangeGreek,      
  kRangeUnassigned, 
  kRangeCyrillic,   
  kRangeArmenian,   
  kRangeHebrew,     
  kRangeUnassigned, 
  kRangeArabic,     
  kRangeUnassigned, 
  kRangeDevanagari, 
  kRangeBengali,    
  kRangeGurmukhi,   
  kRangeGujarati,   
  kRangeOriya,      
  kRangeTamil,      
  kRangeTelugu,     
  kRangeKannada,    
  kRangeMalayalam,  
  kRangeThai,       
  kRangeLao,        
  kRangeGeorgian,   
  kRangeUnassigned, 
  kRangeKorean,     
  kRangeSetLatin,   
  kRangeGreek,      
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeSetLatin,   
  kRangeMathOperators,          
  kRangeMiscTechnical,          
  kRangeControlOpticalEnclose,  
  kRangeControlOpticalEnclose,  
  kRangeControlOpticalEnclose,  
  kRangeBoxBlockGeometrics,     
  kRangeBoxBlockGeometrics,     
  kRangeBoxBlockGeometrics,     
  kRangeMiscSymbols,            
  kRangeDingbats,               
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeSetCJK,                 
  kRangeKorean,                 
  kRangeSurrogate,              
  kRangeUnassigned,             
  kRangeSetCJK,                 
                                        
                                        
                                        
  kRangePrivate,                
  kRangeSetCJK,                 
  kRangeArabic,                 
  kRangeArabic,                 
  kRangeArabic,                 
  kRangeArabic,                 
  kRangeArabic,                 
  kRangeArabic,                 
  kRangeSetCJK,                 
  kRangeSpecials,               
  kRangeTibetan,                
  kRangeSyriac,                 
  kRangeThaana,                 
  kRangeSinhala,                
  kRangeMyanmar,                
  kRangeEthiopic,               
  kRangeCherokee,               
  kRangeCanadian,               
  kRangeOghamRunic,             
  kRangeOghamRunic,             
  kRangeKhmer,                  
  kRangeMongolian,              
  kRangeBraillePattern,         
  kRangeYi,                     
  kRangeUnassigned,             
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned,
  kRangeUnassigned
};



static nsresult
GetCustomEncoding(const char* aFontName, nsCString& aValue, PRBool* aIsWide)
{
  
  static const char* mspgothic=  
    "\x82\x6c\x82\x72 \x82\x6f\x83\x53\x83\x56\x83\x62\x83\x4e";
  
  if ( (!strcmp(aFontName, "Tahoma" )) ||
       (!strcmp(aFontName, "Arial" )) ||
       (!strcmp(aFontName, "Times New Roman" )) ||
       (!strcmp(aFontName, "Courier New" )) ||
       (!strcmp(aFontName, mspgothic )) )
    return NS_ERROR_NOT_AVAILABLE; 

  
  
  
  
  nsCAutoString name;
  if ( ::GetACP() != 949) 
    name.Assign(NS_LITERAL_CSTRING("encoding.") + nsDependentCString(aFontName) + NS_LITERAL_CSTRING(".ttf"));
  else {
    PRUnichar fname[LF_FACESIZE];
    fname[0] = 0;
    MultiByteToWideChar(CP_ACP, 0, aFontName,
    strlen(aFontName) + 1, fname, sizeof(fname)/sizeof(fname[0]));
    name.Assign(NS_LITERAL_CSTRING("encoding.") + NS_ConvertUTF16toUTF8(fname) + NS_LITERAL_CSTRING(".ttf"));
  }

  name.StripWhitespace();
  ToLowerCase(name);

  
  if (!gFontEncodingProperties)
    NS_LoadPersistentPropertiesFromURISpec(&gFontEncodingProperties,
      NS_LITERAL_CSTRING("resource://gre/res/fonts/fontEncoding.properties"));

  if (gFontEncodingProperties) {
    nsAutoString prop;
    nsresult rv = gFontEncodingProperties->GetStringProperty(name, prop);
    if (NS_SUCCEEDED(rv)) {
      aValue.AssignWithConversion(prop);

      
      
      *aIsWide = StringEndsWith(aValue, NS_LITERAL_CSTRING(".wide"));
      if (*aIsWide) {
        aValue.Truncate(aValue.Length()-5);
      }
    }
    return rv;
  }
  return NS_ERROR_NOT_AVAILABLE;
}




static nsresult
GetConverterCommon(const char* aEncoding, nsIUnicodeEncoder** aConverter)
{
  *aConverter = nsnull;
  nsresult rv;
  rv = gCharsetManager->GetUnicodeEncoderRaw(aEncoding, aConverter);
  if (NS_FAILED(rv)) return rv;
  return (*aConverter)->SetOutputErrorBehavior((*aConverter)->kOnError_Replace, nsnull, '?');
}

static nsresult
GetDefaultConverterForTTFSymbolEncoding(nsIUnicodeEncoder** aConverter)
{
  return GetConverterCommon(DEFAULT_TTF_SYMBOL_ENCODING, aConverter);
}

static nsresult
GetConverter(const char* aFontName, PRBool aNameQuirks,
  nsIUnicodeEncoder** aConverter, PRBool* aIsWide = nsnull)
{
  *aConverter = nsnull;

  if (aNameQuirks) {
#ifdef NS_DEBUG
    
    nsCAutoString value;
    PRBool isWide = PR_FALSE;
    NS_ASSERTION(NS_FAILED(GetCustomEncoding(aFontName, value, &isWide)) || !isWide,
                 "internal error -- shouldn't get here");
#endif
    return GetDefaultConverterForTTFSymbolEncoding(aConverter);
  }

  nsCAutoString value;
  PRBool isWide = PR_FALSE;
  nsresult rv = GetCustomEncoding(aFontName, value, &isWide);
  if (NS_FAILED(rv)) return rv;
  if (aIsWide) {
    *aIsWide = isWide;
  }

  return GetConverterCommon(value.get(), aConverter);
}



static PRUint16*
GetCCMapThroughConverter(const char* aFontName, PRBool aNameQuirks)
{
  
  nsCOMPtr<nsIUnicodeEncoder> converter;
  if (NS_SUCCEEDED(GetConverter(aFontName, aNameQuirks, getter_AddRefs(converter)))) {
    nsCOMPtr<nsICharRepresentable> mapper(do_QueryInterface(converter));
    if (mapper)
      return MapperToCCMap(mapper);
  } 
  return nsnull;
}

static nsresult
ConvertUnicodeToGlyph(const PRUnichar* aSrc,  PRInt32 aSrcLength, 
  PRInt32& aDestLength, nsIUnicodeEncoder* aConverter,
  PRBool aIsWide, nsAutoCharBuffer& aResult)
{
  if (aIsWide && 
      NS_FAILED(aConverter->GetMaxLength(aSrc, aSrcLength, &aDestLength))) {
    return NS_ERROR_UNEXPECTED;
  }

  if (!aResult.EnsureElemCapacity(aDestLength)) return NS_ERROR_OUT_OF_MEMORY;
  char* str = aResult.get();

  aConverter->Convert(aSrc, &aSrcLength, str, &aDestLength);

#ifdef IS_LITTLE_ENDIAN
  
  if (aIsWide) {
    char* pstr = str;
    while (pstr < str + aDestLength) {
      PRUint8 tmp = pstr[0];
      pstr[0] = pstr[1];
      pstr[1] = tmp;
      pstr += 2;  
    }
  }
#endif
  return NS_OK;
}

class nsFontInfo : public PLHashEntry
{
public:
  nsFontInfo(eFontType aFontType, PRUint8 aCharset, PRUint16* aCCMap)
  {
    mType = aFontType;
    mCharset = aCharset;
    mCCMap = aCCMap;
#ifdef MOZ_MATHML
    mCMAP.mData = nsnull;  
    mCMAP.mLength = -1;    
#endif
  };

  eFontType mType;
  PRUint8   mCharset;
  PRUint16* mCCMap;
#ifdef MOZ_MATHML
  
  
  
  
  
  
  nsCharacterMap mCMAP;
#endif
};





PR_STATIC_CALLBACK(void*) fontmap_AllocTable(void *pool, size_t size)
{
  return nsMemory::Alloc(size);
}

PR_STATIC_CALLBACK(void) fontmap_FreeTable(void *pool, void *item)
{
  nsMemory::Free(item);
}

PR_STATIC_CALLBACK(PLHashEntry*) fontmap_AllocEntry(void *pool, const void *key)
{
 return new nsFontInfo(eFontType_Unicode, DEFAULT_CHARSET, nsnull);
}

PR_STATIC_CALLBACK(void) fontmap_FreeEntry(void *pool, PLHashEntry *he, PRUint32 flag)
{
  if (flag == HT_FREE_ENTRY)  {
    nsFontInfo *fontInfo = NS_STATIC_CAST(nsFontInfo *, he);
    if (fontInfo->mCCMap && (fontInfo->mCCMap != nsFontMetricsWin::gEmptyCCMap))
      FreeCCMap(fontInfo->mCCMap); 
#ifdef MOZ_MATHML
    if (fontInfo->mCMAP.mData)
      nsMemory::Free(fontInfo->mCMAP.mData);
#endif
    delete (nsString *) (he->key);
    delete fontInfo;
  }
}

PLHashAllocOps fontmap_HashAllocOps = {
  fontmap_AllocTable, fontmap_FreeTable,
  fontmap_AllocEntry, fontmap_FreeEntry
};







#include "blank_glyph.ccmap"
DEFINE_CCMAP(gCharsWithBlankGlyphCCMap, const);

#define SHOULD_BE_SPACE_CHAR(ch)  (CCMAP_HAS_CHAR(gCharsWithBlankGlyphCCMap,ch))

enum {
  eTTPlatformIDUnicode = 0,
  eTTPlatformIDMacintosh = 1,
  eTTPlatformIDMicrosoft = 3
};
enum {
  eTTMicrosoftEncodingSymbol = 0,
  eTTMicrosoftEncodingUnicode = 1,
  eTTMicrosoftEncodingUCS4 = 10
};


enum {
  eTTFormatUninitialize = -1,
  eTTFormat0ByteEncodingTable = 0,
  eTTFormat2HighbyteMappingThroughTable = 2,
  eTTFormat4SegmentMappingToDeltaValues = 4,
  eTTFormat6TrimmedTableMapping = 6,
  eTTFormat8Mixed16bitAnd32bitCoverage = 8,
  eTTFormat10TrimmedArray = 10,
  eTTFormat12SegmentedCoverage = 12
};

static void 
ReadCMAPTableFormat12(PRUint8* aBuf, PRInt32 len, PRUint32 **aExtMap) 
{
  PRUint8* p = aBuf;
  PRUint32 i;

  p += sizeof(PRUint16); 
  p += sizeof(PRUint16); 
  p += sizeof(PRUint32); 
  p += sizeof(PRUint32); 
  PRUint32 nGroup = GET_LONG(p);
  p += sizeof(PRUint32); 

  PRUint32 plane;
  PRUint32 startCode;
  PRUint32 endCode;
  PRUint32 c;
  for (i = 0; i < nGroup; i++) {
    startCode = GET_LONG(p);
    p += sizeof(PRUint32); 
    endCode = GET_LONG(p);
    p += sizeof(PRUint32); 
    for ( c = startCode; c <= endCode; ++c) {
      plane = c >> 16;
      if (!aExtMap[plane]) {
        aExtMap[plane] = new PRUint32[UCS2_MAP_LEN];
        if (!aExtMap[plane])
          return; 
      }
      ADD_GLYPH(aExtMap[plane], c & 0xffff);
    }
    p += sizeof(PRUint32); 
  }
}


static void 
ReadCMAPTableFormat4(PRUint8* aBuf, PRInt32 aLength, PRUint32* aMap,
  PRBool aIsCFFOutline, PRUint8* aIsSpace, PRUint32 aMaxGlyph)
{
  PRUint8* p = aBuf;
  PRUint8* end = aBuf + aLength;
  PRUint32 i;

  
  while (p < end) {
    PRUint8 tmp = p[0];
    p[0] = p[1];
    p[1] = tmp;
    p += 2; 
  }

  PRUint16* s = (PRUint16*) aBuf;
  PRUint16 segCount = s[3] / 2;
  PRUint16* endCode = &s[7];
  PRUint16* startCode = endCode + segCount + 1;
  PRUint16* idDelta = startCode + segCount;
  PRUint16* idRangeOffset = idDelta + segCount;

  for (i = 0; i < segCount; ++i) {
    if (idRangeOffset[i]) {
      PRUint16 startC = startCode[i];
      PRUint16 endC = endCode[i];
      for (PRUint32 c = startC; c <= endC; ++c) {
        PRUint16* g =
          (idRangeOffset[i]/2 + (c - startC) + &idRangeOffset[i]);
        if ((PRUint8*) g < end) {
          if (*g) {
            PRUint16 glyph = idDelta[i] + *g;
            if (aIsCFFOutline) {
              ADD_GLYPH(aMap, c);
            }
            else if (glyph < aMaxGlyph) {
              if (aIsSpace[glyph]) {
                if (SHOULD_BE_SPACE_CHAR(c)) {
                  ADD_GLYPH(aMap, c);
                } 
              }
              else {
                ADD_GLYPH(aMap, c);
              }
            }
          }
          else {
            
            if (SHOULD_BE_SPACE_CHAR(c))
              ADD_GLYPH(aMap, c);
          }
        }
        else {
          
        }
      }
      
    }
    else {
      PRUint16 endC = endCode[i];
      for (PRUint32 c = startCode[i]; c <= endC; ++c) {
        PRUint16 glyph = idDelta[i] + c;
        if (aIsCFFOutline) {
          ADD_GLYPH(aMap, c);
        }
        else if (glyph < aMaxGlyph) {
          if (aIsSpace[glyph]) {
            if (SHOULD_BE_SPACE_CHAR(c)) {
              ADD_GLYPH(aMap, c);
            }
          }
          else {
            ADD_GLYPH(aMap, c);
          }
        }
      }
      
    }
  }
  
}

PRUint16*
nsFontMetricsWin::GetFontCCMAP(HDC aDC, const char* aShortName,
  PRBool aNameQuirks, eFontType& aFontType, PRUint8& aCharset)
{
  PRUint16 *ccmap = nsnull;

  DWORD len = GetFontData(aDC, CMAP, 0, nsnull, 0);
  if ((len == GDI_ERROR) || (!len)) {
    return nsnull;
  }
  nsAutoFontDataBuffer buffer;
  if (!buffer.EnsureElemCapacity(len)) {
    return nsnull;
  }
  PRUint8* buf = buffer.get();
  DWORD newLen = GetFontData(aDC, CMAP, 0, buf, len);
  if (newLen != len) {
    return nsnull;
  }

  PRUint32 map[UCS2_MAP_LEN];
  memset(map, 0, sizeof(map));
  PRUint8* p = buf + sizeof(PRUint16); 
  PRUint16 n = GET_SHORT(p); 
  p += sizeof(PRUint16); 
  PRUint16 i;
  PRUint32 keepOffset;
  PRUint32 offset;
  PRUint32 keepFormat = eTTFormatUninitialize;

  for (i = 0; i < n; ++i) {
    PRUint16 platformID = GET_SHORT(p); 
    p += sizeof(PRUint16); 
    PRUint16 encodingID = GET_SHORT(p); 
    p += sizeof(PRUint16); 
    offset = GET_LONG(p);  
    p += sizeof(PRUint32); 
    if (platformID == eTTPlatformIDMicrosoft) { 
      if (encodingID == eTTMicrosoftEncodingUnicode) { 
        
        
        
        
        
        ccmap = GetCCMapThroughConverter(aShortName, aNameQuirks);
        if (ccmap) {
          aCharset = DEFAULT_CHARSET;
          aFontType = eFontType_NonUnicode;
          return ccmap;
        }
        PRUint16 format = GET_SHORT(buf+offset);
        if (format == eTTFormat4SegmentMappingToDeltaValues) {
          keepFormat = eTTFormat4SegmentMappingToDeltaValues;
          keepOffset = offset;
        }
      } 
      else if (encodingID == eTTMicrosoftEncodingSymbol) { 
        aCharset = SYMBOL_CHARSET;
        aFontType = eFontType_NonUnicode;
        return GetCCMapThroughConverter(aShortName, aNameQuirks);
      } 
      else if (encodingID == eTTMicrosoftEncodingUCS4) {
        PRUint16 format = GET_SHORT(buf+offset);
        if (format == eTTFormat12SegmentedCoverage) {
          keepFormat = eTTFormat12SegmentedCoverage;
          keepOffset = offset;
          
          break;
        }
      }
    } 
  } 


  if (eTTFormat12SegmentedCoverage == keepFormat) {
    PRUint32* extMap[EXTENDED_UNICODE_PLANES+1];
    extMap[0] = map;
    memset(extMap+1, 0, sizeof(PRUint32*)*EXTENDED_UNICODE_PLANES);
    ReadCMAPTableFormat12(buf+keepOffset, len-keepOffset, extMap);
    ccmap = MapToCCMapExt(map, extMap+1, EXTENDED_UNICODE_PLANES);
    for (i = 1; i <= EXTENDED_UNICODE_PLANES; ++i) {
      if (extMap[i])
        delete [] extMap[i];
    }
    aCharset = DEFAULT_CHARSET;
    aFontType = eFontType_Unicode;
  }
  else if (eTTFormat4SegmentMappingToDeltaValues == keepFormat) {
    PRUint32 maxGlyph;
    nsAutoFontDataBuffer isSpace;
    PRBool isCFFOutline;
    if (NS_SUCCEEDED(GetSpaces(aDC, &isCFFOutline, &maxGlyph, isSpace))) {
      ReadCMAPTableFormat4(buf+keepOffset, len-keepOffset, map, isCFFOutline,
        isSpace.get(), maxGlyph);
      ccmap = MapToCCMap(map);
      aCharset = DEFAULT_CHARSET;
      aFontType = eFontType_Unicode;
    }
  }

  return ccmap;
}







PRUint16*
nsFontMetricsWin::GetCCMAP(HDC aDC, const char* aShortName,
  PRBool* aNameQuirks, eFontType* aFontType, PRUint8* aCharset)
{
  if (!gFontMaps) {
    gFontMaps = PL_NewHashTable(0, HashKey, CompareKeys, nsnull, &fontmap_HashAllocOps,
      nsnull);
    if (!gFontMaps) { 
      return nsnull;
    }
    gEmptyCCMap = CreateEmptyCCMap();
    if (!gEmptyCCMap) {
      PL_HashTableDestroy(gFontMaps);
      gFontMaps = nsnull;
      return nsnull;
    }
  }
  eFontType fontType = aFontType ? *aFontType : eFontType_Unicode;
  PRUint8 charset = DEFAULT_CHARSET;
  nsString* name = new nsString(); 
  if (!name) {
    return nsnull;
  }
  nsFontInfo* info;
  PLHashEntry *he, **hep = NULL; 
  PLHashNumber hash;
  PRBool nameQuirks = aNameQuirks ? *aNameQuirks : PR_FALSE;
  PRBool isSymbolEncoding = PR_FALSE;
  eGetNameError ret = GetNAME(aDC, name, &isSymbolEncoding);
  if (ret == eGetName_OK) {
    
    if (nameQuirks && (isSymbolEncoding || fontType != eFontType_Unicode)) {
      name->SetCharAt(PRUnichar('1'), 0); 
    }
    else {
      nameQuirks = PR_FALSE;
      if (aNameQuirks) {
        *aNameQuirks = PR_FALSE;
      }
    }
    
    hash = HashKey(name);
    hep = PL_HashTableRawLookup(gFontMaps, hash, name);
    he = *hep;
    if (he) {
      
      delete name;
      info = NS_STATIC_CAST(nsFontInfo *, he);
      if (aCharset) {
        *aCharset = info->mCharset;
      }
      if (aFontType) {
        *aFontType = info->mType;
      }
      return info->mCCMap;
    }
  }
  
  else if (ret == eGetName_GDIError) {
    delete name;
    charset = GetTextCharset(aDC);
    if (charset & (~0xFF)) {
      return gEmptyCCMap;
    }
    int j = gCharsetToIndex[charset];
    
    
    if (j == eCharset_DEFAULT) {
      return gEmptyCCMap;
    }
    PRUint16* charsetCCMap = gCharsetInfo[j].mCCMap;
    if (!charsetCCMap) {
      charsetCCMap = gCharsetInfo[j].GenerateMap(&gCharsetInfo[j]);
      if (charsetCCMap)
        gCharsetInfo[j].mCCMap = charsetCCMap;
      else
        return gEmptyCCMap;
    }
    if (aCharset) {
      *aCharset = charset;
    }
    if (aFontType) {
      *aFontType = eFontType_Unicode;
    }
    return charsetCCMap;   
  }
  else {
    
    delete name;
    return gEmptyCCMap;
  }

  if (aFontType)
    fontType = *aFontType;
  if (aCharset)
    charset = *aCharset;
  PRUint16* ccmap = GetFontCCMAP(aDC, aShortName, nameQuirks, fontType, charset);
  if (aFontType)
    *aFontType = fontType; 
  if (aCharset)
    *aCharset = charset;

  if (!ccmap) {
    delete name;
    return gEmptyCCMap;
  }

  
  NS_ASSERTION(hep, "bad code");
  he = PL_HashTableRawAdd(gFontMaps, hep, hash, name, nsnull);
  if (he) {
    info = NS_STATIC_CAST(nsFontInfo*, he);
    he->value = info;    
    info->mType = fontType;
    info->mCharset = charset;
    info->mCCMap = ccmap;
    return ccmap;
  }
  delete name;
  return nsnull;
}









static nsresult
GetGlyphIndices(HDC                 aDC,
                nsCharacterMap**    aCMAP,
                const PRUnichar*    aString, 
                PRUint32            aLength,
                nsAutoChar16Buffer& aResult)
{  
  NS_ASSERTION(aString, "null arg");
  if (!aString)
    return NS_ERROR_NULL_POINTER;

  nsAutoFontDataBuffer buffer;
  PRUint8* buf = nsnull;
  DWORD len = -1;
  
  
  

  if (!aCMAP || 
      !*aCMAP || (*aCMAP)->mLength < 0  
     ) 
  {
    
    
    if (aCMAP && *aCMAP) (*aCMAP)->mLength = 0; 

    len = GetFontData(aDC, CMAP, 0, nsnull, 0);
    if ((len == GDI_ERROR) || (!len)) {
      return NS_ERROR_UNEXPECTED;
    }
    if (!buffer.EnsureElemCapacity(len)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    buf = buffer.get();
    DWORD newLen = GetFontData(aDC, CMAP, 0, buf, len);
    if (newLen != len) {
      return NS_ERROR_UNEXPECTED;
    }
    PRUint8* p = buf + sizeof(PRUint16); 
    PRUint16 n = GET_SHORT(p);
    p += sizeof(PRUint16); 
    PRUint16 i;
    PRUint32 offset;
    for (i = 0; i < n; ++i) {
      PRUint16 platformID = GET_SHORT(p); 
      p += sizeof(PRUint16); 
      PRUint16 encodingID = GET_SHORT(p); 
      p += sizeof(PRUint16); 
      offset = GET_LONG(p);  
      p += sizeof(PRUint32); 
      if (platformID == eTTPlatformIDMicrosoft && 
          encodingID == eTTMicrosoftEncodingUnicode) 
        break;
    }
    if (i == n) {
      NS_WARNING("nsFontMetricsWin::GetGlyphIndices() called for a non-unicode font!");
      return NS_ERROR_UNEXPECTED;
    }
    p = buf + offset;
    PRUint16 format = GET_SHORT(p);
    if (format != eTTFormat4SegmentMappingToDeltaValues) {
      return NS_ERROR_UNEXPECTED;
    }
    PRUint8* end = buf + len;

    
    while (p < end) {
      PRUint8 tmp = p[0];
      p[0] = p[1];
      p[1] = tmp;
      p += 2; 
    }
#ifdef MOZ_MATHML
    
    if (aCMAP) {
      nsAutoString name;
      nsFontInfo* info;
      if (GetNAME(aDC, &name) == eGetName_OK) {
        info = (nsFontInfo*)PL_HashTableLookup(nsFontMetricsWin::gFontMaps, &name);
        if (info) {
          info->mCMAP.mData = (PRUint8*)nsMemory::Alloc(len);
          if (info->mCMAP.mData) {
            memcpy(info->mCMAP.mData, buf, len);
            info->mCMAP.mLength = len;
            *aCMAP = &(info->mCMAP);
          }          
        }
      }
    }
#endif
  }

  
  

  if (aCMAP && *aCMAP) { 
    buf = (*aCMAP)->mData;
    len = (*aCMAP)->mLength;
  }
  if (buf && len > 0) {
    
    PRUint8* p = buf + sizeof(PRUint16); 
    PRUint16 n = GET_SHORT(p);
    p += sizeof(PRUint16); 
    PRUint16 i;
    PRUint32 offset;
    for (i = 0; i < n; ++i) {
      PRUint16 platformID = GET_SHORT(p); 
      p += sizeof(PRUint16); 
      PRUint16 encodingID = GET_SHORT(p); 
      p += sizeof(PRUint16); 
      offset = GET_LONG(p);  
      p += sizeof(PRUint32); 
      if (platformID == eTTPlatformIDMicrosoft && 
          encodingID == eTTMicrosoftEncodingUnicode) 
        break;
    }
    PRUint8* end = buf + len;

    PRUint16* s = (PRUint16*) (buf + offset);
    PRUint16 segCount = s[3] / 2;
    PRUint16* endCode = &s[7];
    PRUint16* startCode = endCode + segCount + 1;
    PRUint16* idDelta = startCode + segCount;
    PRUint16* idRangeOffset = idDelta + segCount;

    if (!aResult.EnsureElemCapacity(aLength)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    PRUnichar* result = aResult.get();
    for (i = 0; i < aLength; ++i) {
      result[i] = GetGlyphIndex(segCount, endCode, startCode,
                                idRangeOffset, idDelta, end, 
                                aString[i]);
    }

    return NS_OK;
  }
  return NS_ERROR_UNEXPECTED;
}







enum eGlyphAgent {
  eGlyphAgent_UNKNOWN = -1,
  eGlyphAgent_UNICODE,
  eGlyphAgent_ANSI
};

class nsGlyphAgent
{
public:
  nsGlyphAgent()
  {
    mState = eGlyphAgent_UNKNOWN;
    
    FIXED zero, one;
    zero.fract = 0; one.fract = 0;
    zero.value = 0; one.value = 1; 
    mMat.eM12 = mMat.eM21 = zero;
    mMat.eM11 = mMat.eM22 = one;    
  }

  ~nsGlyphAgent()
  {}

  eGlyphAgent GetState()
  {
    return mState;
  }

  DWORD GetGlyphMetrics(HDC           aDC,
                        PRUint8       aChar,
                        GLYPHMETRICS* aGlyphMetrics);

  DWORD GetGlyphMetrics(HDC           aDC, 
                        PRUnichar     aChar,
                        PRUint16      aGlyphIndex,
                        GLYPHMETRICS* aGlyphMetrics);
private:
  MAT2 mMat;    

  eGlyphAgent mState; 
                
                
                
};



DWORD
nsGlyphAgent::GetGlyphMetrics(HDC           aDC,
                              PRUint8       aChar,
                              GLYPHMETRICS* aGlyphMetrics)
{
  memset(aGlyphMetrics, 0, sizeof(GLYPHMETRICS)); 
  return GetGlyphOutlineA(aDC, aChar, GGO_METRICS, aGlyphMetrics, 0, nsnull, &mMat);
}




DWORD
nsGlyphAgent::GetGlyphMetrics(HDC           aDC,
                              PRUnichar     aChar,
                              PRUint16      aGlyphIndex,
                              GLYPHMETRICS* aGlyphMetrics)
{
  memset(aGlyphMetrics, 0, sizeof(GLYPHMETRICS)); 
  if (eGlyphAgent_UNKNOWN == mState) { 
    
    DWORD len = GetGlyphOutlineW(aDC, aChar, GGO_METRICS, aGlyphMetrics, 0, nsnull, &mMat);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
      
      mState = eGlyphAgent_ANSI;
    }
    else {
      
      mState = eGlyphAgent_UNICODE;
      return len;
    }
  }

  if (eGlyphAgent_UNICODE == mState) {
    return GetGlyphOutlineW(aDC, aChar, GGO_METRICS, aGlyphMetrics, 0, nsnull, &mMat);
  }

  
  
  
  
  if (0 == aGlyphIndex) { 
    nsAutoChar16Buffer buf;
    if (NS_SUCCEEDED(GetGlyphIndices(aDC, nsnull, &aChar, 1, buf)))
      aGlyphIndex = *(buf.get());
  }
  if (0 < aGlyphIndex) {
    return GetGlyphOutlineA(aDC, aGlyphIndex, GGO_METRICS | GGO_GLYPH_INDEX, aGlyphMetrics, 0, nsnull, &mMat);
  }

  
  
  return GDI_ERROR;
}


nsGlyphAgent gGlyphAgent;

#ifdef MOZ_MATHML



static nsresult 
GetBoundingMetricsCommon(HDC aDC, LONG aOverhangCorrection, const PRUnichar* aString, PRUint32 aLength, 
  nsBoundingMetrics& aBoundingMetrics, PRUnichar* aGlyphStr)
{
  
  nscoord descent;
  GLYPHMETRICS gm;                                                
  DWORD len = gGlyphAgent.GetGlyphMetrics(aDC, aString[0], aGlyphStr[0], &gm);
  if (GDI_ERROR == len) {
    return NS_ERROR_UNEXPECTED;
  }
  
  descent = -(nscoord(gm.gmptGlyphOrigin.y) - nscoord(gm.gmBlackBoxY));
  aBoundingMetrics.leftBearing = gm.gmptGlyphOrigin.x;
  aBoundingMetrics.rightBearing = gm.gmptGlyphOrigin.x + gm.gmBlackBoxX;
  aBoundingMetrics.ascent = gm.gmptGlyphOrigin.y;
  aBoundingMetrics.descent = descent;
  aBoundingMetrics.width = gm.gmCellIncX;

  if (1 < aLength) {
    
    for (PRUint32 i = 1; i < aLength; ++i) {
      len = gGlyphAgent.GetGlyphMetrics(aDC, aString[i], aGlyphStr[i], &gm);
      if (GDI_ERROR == len) {
        return NS_ERROR_UNEXPECTED;
      }
      
      descent = -(nscoord(gm.gmptGlyphOrigin.y) - nscoord(gm.gmBlackBoxY));
      if (aBoundingMetrics.ascent < gm.gmptGlyphOrigin.y)
        aBoundingMetrics.ascent = gm.gmptGlyphOrigin.y;
      if (aBoundingMetrics.descent < descent)
        aBoundingMetrics.descent = descent;
    }
    
    SIZE size;
    ::GetTextExtentPointW(aDC, aString, aLength, &size);
    size.cx -= aOverhangCorrection;
    aBoundingMetrics.width = size.cx;
    aBoundingMetrics.rightBearing = size.cx - gm.gmCellIncX + gm.gmptGlyphOrigin.x + gm.gmBlackBoxX;
  }

  return NS_OK;
}

static nsresult 
GetBoundingMetricsCommonA(HDC aDC, LONG aOverhangCorrection, const char* aString, PRUint32 aLength, 
  nsBoundingMetrics& aBoundingMetrics)
{
  
  nscoord descent;
  GLYPHMETRICS gm;
  DWORD len = gGlyphAgent.GetGlyphMetrics(aDC, PRUint8(aString[0]), &gm);
  if (GDI_ERROR == len) {
    return NS_ERROR_UNEXPECTED;
  }

  
  descent = -(nscoord(gm.gmptGlyphOrigin.y) - nscoord(gm.gmBlackBoxY));
  aBoundingMetrics.leftBearing = gm.gmptGlyphOrigin.x;
  aBoundingMetrics.rightBearing = gm.gmptGlyphOrigin.x + gm.gmBlackBoxX;
  aBoundingMetrics.ascent = gm.gmptGlyphOrigin.y;
  aBoundingMetrics.descent = descent;
  aBoundingMetrics.width = gm.gmCellIncX;

  if (1 < aLength) {
    
    for (PRUint32 i = 1; i < aLength; ++i) {
      len = gGlyphAgent.GetGlyphMetrics(aDC, PRUint8(aString[i]), &gm);
      if (GDI_ERROR == len) {
        return NS_ERROR_UNEXPECTED;
      }
      
      descent = -(nscoord(gm.gmptGlyphOrigin.y) - nscoord(gm.gmBlackBoxY));
      if (aBoundingMetrics.ascent < gm.gmptGlyphOrigin.y)
        aBoundingMetrics.ascent = gm.gmptGlyphOrigin.y;
      if (aBoundingMetrics.descent < descent)
        aBoundingMetrics.descent = descent;
    }
    
    SIZE size;
    ::GetTextExtentPointA(aDC, aString, aLength, &size);
    size.cx -= aOverhangCorrection;
    aBoundingMetrics.width = size.cx;
    aBoundingMetrics.rightBearing = size.cx - gm.gmCellIncX + gm.gmBlackBoxX;
  }

  return NS_OK;
}
#endif





class nsFontWinUnicode : public nsFontWin
{
public:
  nsFontWinUnicode(LOGFONT* aLogFont, HFONT aFont, PRUint16* aCCMap);
  virtual ~nsFontWinUnicode();

  virtual PRInt32 GetWidth(HDC aDC, const PRUnichar* aString, PRUint32 aLength);
  virtual void DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,

    const PRUnichar* aString, PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult
  GetBoundingMetrics(HDC                aDC,
                     const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics);

#ifdef NS_DEBUG
  virtual void DumpFontInfo();
#endif 
#endif

private:
  PRBool mUnderlinedOrStrikeOut;
};












class nsFontWinNonUnicode : public nsFontWin
{
public:
  nsFontWinNonUnicode(LOGFONT* aLogFont, HFONT aFont, PRUint16* aCCMap, nsIUnicodeEncoder* aConverter, PRBool aIsWide = PR_FALSE);
  virtual ~nsFontWinNonUnicode();

  virtual PRInt32 GetWidth(HDC aDC, const PRUnichar* aString, PRUint32 aLength);
  virtual void DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
                          const PRUnichar* aString, PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult
  GetBoundingMetrics(HDC                aDC,
                     const PRUnichar*   aString,
                     PRUint32           aLength,
                     nsBoundingMetrics& aBoundingMetrics);
#ifdef NS_DEBUG
  virtual void DumpFontInfo();
#endif 
#endif

private:
  nsCOMPtr<nsIUnicodeEncoder> mConverter;
  PRBool mIsWide; 
};

void
nsFontMetricsWin::InitMetricsFor(HDC aDC, nsFontWin* aFont)
{
  float dev2app;
  dev2app = mDeviceContext->DevUnitsToAppUnits();

  TEXTMETRIC metrics;
  ::GetTextMetrics(aDC, &metrics);
  aFont->mMaxAscent = NSToCoordRound(metrics.tmAscent * dev2app);
  aFont->mMaxDescent = NSToCoordRound(metrics.tmDescent * dev2app);
  aFont->mOverhangCorrection = 0;
  if (IsWin95OrWin98()) {
    aFont->mOverhangCorrection = metrics.tmOverhang;
    if (metrics.tmOverhang < 3 && metrics.tmItalic &&
        !(metrics.tmPitchAndFamily & (TMPF_VECTOR | TMPF_TRUETYPE | TMPF_DEVICE))) {
      
      
      
      SIZE size;
      ::GetTextExtentPoint32(aDC, " ", 1, &size);
      if (!(metrics.tmPitchAndFamily & TMPF_FIXED_PITCH)) {
        
        
        aFont->mOverhangCorrection = size.cx - metrics.tmAveCharWidth;
      } else {
        SIZE size2;
        ::GetTextExtentPoint32(aDC, "  ", 2, &size2);
        aFont->mOverhangCorrection = size.cx * 2 - size2.cx;
      }
    }
  }
  aFont->mMaxCharWidthMetric = metrics.tmMaxCharWidth;
  aFont->mMaxHeightMetric = metrics.tmHeight;
  aFont->mPitchAndFamily = metrics.tmPitchAndFamily;
}

HFONT
nsFontMetricsWin::CreateFontAdjustHandle(HDC aDC, LOGFONT* aLogFont)
{
  
  

  PRInt32 dummy = 0;
  nscoord baseSize = mFont.size; 
  nscoord size72 = NSIntPointsToTwips(72); 
  mFont.size = size72;
  nscoord baselfHeight = aLogFont->lfHeight;
  FillLogFont(aLogFont, dummy, PR_TRUE);

  HFONT hfont = ::CreateFontIndirect(aLogFont);
  mFont.size = baseSize;
  aLogFont->lfHeight = baselfHeight;

  if (hfont) {
    HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
    char name[sizeof(aLogFont->lfFaceName)];
    if (::GetTextFace(aDC, sizeof(name), name) &&
        !strcmpi(name, aLogFont->lfFaceName)) {
      float dev2app;
      dev2app = mDeviceContext->DevUnitsToAppUnits();

      
      nscoord xheight72;
      OUTLINETEXTMETRIC oMetrics;
      TEXTMETRIC& metrics = oMetrics.otmTextMetrics;
      if (0 < ::GetOutlineTextMetrics(aDC, sizeof(oMetrics), &oMetrics)) {
        xheight72 = NSToCoordRound((float)metrics.tmAscent * dev2app * 0.56f); 
        GLYPHMETRICS gm;
        DWORD len = gGlyphAgent.GetGlyphMetrics(aDC, PRUnichar('x'), 0, &gm);
        if (GDI_ERROR != len && gm.gmptGlyphOrigin.y > 0) {
          xheight72 = NSToCoordRound(gm.gmptGlyphOrigin.y * dev2app);
        }
      }
      else {
        ::GetTextMetrics(aDC, &metrics);
        xheight72 = NSToCoordRound((float)metrics.tmAscent * dev2app * 0.56f); 
      }
      ::SelectObject(aDC, (HGDIOBJ)oldFont);  

      
      float adjust = mFont.sizeAdjust / (float(xheight72) / float(size72));
      mFont.size = NSToCoordRound(float(baseSize) * adjust);
      FillLogFont(aLogFont, dummy, PR_TRUE);

      hfont = ::CreateFontIndirect(aLogFont);

      
      mFont.size = baseSize;
      aLogFont->lfHeight = baselfHeight;
      return hfont;
    }
    ::SelectObject(aDC, (HGDIOBJ)oldFont);  
    ::DeleteObject((HFONT)hfont);  
  }
  return nsnull;
}

HFONT
nsFontMetricsWin::CreateFontHandle(HDC aDC, const nsString& aName, LOGFONT* aLogFont)
{
  PRUint16 weightTable = LookForFontWeightTable(aDC, aName);
  PRInt32 weight = GetFontWeight(mFont.weight, weightTable);

  FillLogFont(aLogFont, weight);
 
  
  
  
  WideCharToMultiByte(CP_ACP, 0, aName.get(), aName.Length() + 1,
                      aLogFont->lfFaceName, sizeof(aLogFont->lfFaceName),
                      nsnull, nsnull);

  if (mFont.sizeAdjust <= 0) {
    
    return ::CreateFontIndirect(aLogFont);
  }
  return CreateFontAdjustHandle(aDC, aLogFont);
}

HFONT
nsFontMetricsWin::CreateFontHandle(HDC aDC, nsGlobalFont* aGlobalFont, LOGFONT* aLogFont)
{
  PRUint16 weightTable = LookForFontWeightTable(aDC, aGlobalFont->name);
  PRInt32 weight = GetFontWeight(mFont.weight, weightTable);

  FillLogFont(aLogFont, weight);
  aLogFont->lfCharSet = aGlobalFont->logFont.lfCharSet;
  aLogFont->lfPitchAndFamily = aGlobalFont->logFont.lfPitchAndFamily;
  strcpy(aLogFont->lfFaceName, aGlobalFont->logFont.lfFaceName);

  if (mFont.sizeAdjust <= 0) {
    
    return ::CreateFontIndirect(aLogFont);
  }
  return CreateFontAdjustHandle(aDC, aLogFont);
}

nsFontWin*
nsFontMetricsWin::LoadFont(HDC aDC, const nsString& aName, PRBool aNameQuirks)
{
  LOGFONT logFont;
  HFONT hfont = CreateFontHandle(aDC, aName, &logFont);
  if (hfont) {
    HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
    char name[sizeof(logFont.lfFaceName)];
    if (::GetTextFace(aDC, sizeof(name), name) &&
        !strcmpi(name, logFont.lfFaceName)) {
      nsFontWin* font = nsnull;
      if (mIsUserDefined) {
#ifndef WINCE
        font = new nsFontWinNonUnicode(&logFont, hfont, gUserDefinedCCMap,
                                       gUserDefinedConverter);
#else
        font = new nsFontWinUnicode(&logFont, hfont, gUserDefinedCCMap);
#endif
    } else {
        eFontType fontType = eFontType_Unicode;
        PRBool nameQuirks = aNameQuirks;
        
        if (nameQuirks) {
          nsCAutoString encoding;
          PRBool isWide = PR_FALSE;
          if (NS_SUCCEEDED(GetCustomEncoding(logFont.lfFaceName, encoding, &isWide))) {
            nameQuirks = !isWide;
            fontType = eFontType_NonUnicode;
          }
        }
        PRUint16* ccmap = GetCCMAP(aDC, logFont.lfFaceName, &nameQuirks,
                                   &fontType, nsnull);
        if (ccmap) {
          if (eFontType_Unicode == fontType) {
            font = new nsFontWinUnicode(&logFont, hfont, ccmap);
          }
          else if (eFontType_NonUnicode == fontType) {
            PRBool isWide = PR_FALSE;
            nsCOMPtr<nsIUnicodeEncoder> converter;
            if (NS_SUCCEEDED(GetConverter(logFont.lfFaceName, nameQuirks,
                  getter_AddRefs(converter), &isWide)))
#ifndef WINCE
               font = new nsFontWinNonUnicode(&logFont, hfont, ccmap, converter, isWide);
#else
               font = new nsFontWinUnicode(&logFont, hfont, ccmap);
#endif
          }
        }
      }

      if (font) {
        InitMetricsFor(aDC, font);
        mLoadedFonts.AppendElement(font);
        ::SelectObject(aDC, (HGDIOBJ)oldFont);  
        return font;
      }
      
      
    }
    ::SelectObject(aDC, (HGDIOBJ)oldFont);
    ::DeleteObject(hfont);
  }
  return nsnull;
}

nsFontWin*
nsFontMetricsWin::LoadGlobalFont(HDC aDC, nsGlobalFont* aGlobalFont)
{
  LOGFONT logFont;
  HFONT hfont = CreateFontHandle(aDC, aGlobalFont, &logFont);
  if (hfont) {
    nsFontWin* font = nsnull;
    if (eFontType_Unicode == aGlobalFont->fonttype) {
      font = new nsFontWinUnicode(&logFont, hfont, aGlobalFont->ccmap);
    }
    else if (eFontType_NonUnicode == aGlobalFont->fonttype) {
      nsCOMPtr<nsIUnicodeEncoder> converter;
      PRBool isWide;
      if (NS_SUCCEEDED(GetConverter(logFont.lfFaceName, PR_FALSE,
            getter_AddRefs(converter), &isWide))) {
#ifndef WINCE
        font = new nsFontWinNonUnicode(&logFont, hfont, aGlobalFont->ccmap, converter, isWide);
#else
        font = new nsFontWinUnicode(&logFont, hfont, aGlobalFont->ccmap);
#endif
      }
    }
    if (font) {
      HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
      InitMetricsFor(aDC, font);
      mLoadedFonts.AppendElement(font);
      ::SelectObject(aDC, (HGDIOBJ)oldFont);
      return font;
    }
    ::DeleteObject((HGDIOBJ)hfont);
  }
  return nsnull;
}

static int CALLBACK 
enumProc(const LOGFONT* logFont, const TEXTMETRIC* metrics,
         DWORD fontType, LPARAM hasFontSig)
{
  
  if (logFont->lfFaceName[0] == '@') {
    return 1;
  }

  for (int i = nsFontMetricsWin::gGlobalFonts->Count()-1; i >= 0; --i) {
    nsGlobalFont* font = (nsGlobalFont*)nsFontMetricsWin::gGlobalFonts->ElementAt(i);
    if (!strcmp(font->logFont.lfFaceName, logFont->lfFaceName)) {
      
      int charsetSigBit = gCharsetToBit[gCharsetToIndex[logFont->lfCharSet]];
      if (charsetSigBit >= 0) {
        DWORD charsetSigAdd = 1 << charsetSigBit;
        font->signature.fsCsb[0] |= charsetSigAdd;
      }

      
      if (fontType & hasFontSig) {
#ifndef WINCE
        memcpy(font->signature.fsUsb, ((NEWTEXTMETRICEX*)metrics)->ntmFontSig.fsUsb, 16);
#else
        memset(font->signature.fsUsb, '\0', 16);
#endif
      }
      return 1;
    }
  }

  
  

  PRUnichar name[LF_FACESIZE];
  name[0] = 0;
  MultiByteToWideChar(CP_ACP, 0, logFont->lfFaceName,
    strlen(logFont->lfFaceName) + 1, name, sizeof(name)/sizeof(name[0]));

  nsGlobalFont* font = new nsGlobalFont;
  if (!font) {
    return 0;
  }
  font->name.Assign(name);
  font->ccmap = nsnull;
  font->logFont = *logFont;
  font->signature.fsCsb[0] = 0;
  font->signature.fsCsb[1] = 0;
  font->fonttype = eFontType_UNKNOWN;
  font->flags = 0;

  if (fontType & hasFontSig) {
    font->flags |= NS_GLOBALFONT_TRUETYPE;
#ifndef WINCE
    
    memcpy(font->signature.fsUsb, ((NEWTEXTMETRICEX*)metrics)->ntmFontSig.fsUsb, 16);
#else
    memset(font->signature.fsUsb, '\0', 16);
#endif
  }

  if (logFont->lfCharSet == SYMBOL_CHARSET) {
    font->flags |= NS_GLOBALFONT_SYMBOL;
  }

  int charsetSigBit = gCharsetToBit[gCharsetToIndex[logFont->lfCharSet]];
  if (charsetSigBit >= 0) {
    DWORD charsetSigAdd = 1 << charsetSigBit;
    font->signature.fsCsb[0] |= charsetSigAdd;
  }

  nsFontMetricsWin::gGlobalFonts->AppendElement(font);
  return 1;
}

static int PR_CALLBACK
CompareGlobalFonts(const void* aArg1, const void* aArg2, void* aClosure)
{
  const nsGlobalFont* font1 = (const nsGlobalFont*)aArg1;
  const nsGlobalFont* font2 = (const nsGlobalFont*)aArg2;

  
  
  
  
  
  
  
  PRInt32 weight1 = 0, weight2 = 0; 
  if (!(font1->flags & NS_GLOBALFONT_TRUETYPE))
    weight1 |= 0x2;
  if (!(font2->flags & NS_GLOBALFONT_TRUETYPE))
    weight2 |= 0x2; 
  if (font1->flags & NS_GLOBALFONT_SYMBOL)
    weight1 |= 0x1;
  if (font2->flags & NS_GLOBALFONT_SYMBOL)
    weight2 |= 0x1;

  return weight1 - weight2;
}

nsVoidArray*
nsFontMetricsWin::InitializeGlobalFonts(HDC aDC)
{
  if (!gGlobalFonts) {
    gGlobalFonts = new nsVoidArray();
    if (!gGlobalFonts) return nsnull;

    LOGFONT logFont;
    logFont.lfCharSet = DEFAULT_CHARSET;
    logFont.lfFaceName[0] = 0;
    logFont.lfPitchAndFamily = 0;

    



    EnumFontFamiliesEx(aDC, &logFont, enumProc, TRUETYPE_FONTTYPE, 0);
    if (gGlobalFonts->Count() == 0)
      EnumFontFamilies(aDC, nsnull, enumProc, 0);

    
    gGlobalFonts->Sort(CompareGlobalFonts, nsnull);
  }

  return gGlobalFonts;
}

int
nsFontMetricsWin::SameAsPreviousMap(int aIndex)
{
  
  nsGlobalFont* font = (nsGlobalFont*)gGlobalFonts->ElementAt(aIndex);
  for (int i = 0; i < aIndex; ++i) {
    nsGlobalFont* tmp = (nsGlobalFont*)gGlobalFonts->ElementAt(i);
    if (tmp->flags & NS_GLOBALFONT_SKIP) {
      continue;
    }
    if (!tmp->ccmap) {
      continue;
    }
    if (tmp->ccmap == font->ccmap) {
      font->flags |= NS_GLOBALFONT_SKIP;
      return 1;
    }

    if (IsSameCCMap(tmp->ccmap, font->ccmap)) {
      font->flags |= NS_GLOBALFONT_SKIP;
      return 1;
    }
  }

  return 0;
}

#ifndef WINCE
static
void BitFromUnicodeRange(PRUint32 range, DWORD* usb)
{
  for (int i = 0, dword = 0; dword < 3; ++dword) {
    for (int bit = 0; bit < sizeof(DWORD) * 8; ++bit, ++i) {
      if (range == gBitToUnicodeRange[i]) {
        usb[dword] |= 1 << bit;
      }
    }
  }
}
#endif

#ifdef DEBUG_emk
static LARGE_INTEGER freq, prev;
#endif

nsFontWin*
nsFontMetricsWin::FindGlobalFont(HDC aDC, PRUint32 c)
{
  
  if (!gGlobalFonts) {
    if (!InitializeGlobalFonts(aDC)) {
      return nsnull;
    }
  }
#ifndef WINCE
  PRUint32 range = (c <= 0xFFFF) ? FindCharUnicodeRange(c) : kRangeSurrogate;
  DWORD usb[4];
  memset(usb, 0, sizeof(usb));
  BitFromUnicodeRange(range, usb);
#endif
  int count = gGlobalFonts->Count();
  for (int i = 0; i < count; ++i) {
    nsGlobalFont* font = (nsGlobalFont*)gGlobalFonts->ElementAt(i);
    if (font->flags & NS_GLOBALFONT_SKIP) {
      continue;
    }
    if (!font->ccmap) {
#ifndef WINCE
      
      if (font->flags & NS_GLOBALFONT_TRUETYPE &&
          !(font->signature.fsUsb[0] & usb[0]) &&
          !(font->signature.fsUsb[1] & usb[1]) &&
          !(font->signature.fsUsb[2] & usb[2])) {
        continue;
      }
#endif
      
      
      HFONT hfont = ::CreateFontIndirect(&font->logFont);
      if (!hfont) {
        continue;
      }
      HFONT oldFont = (HFONT)::SelectObject(aDC, hfont);
      font->ccmap = GetCCMAP(aDC, font->logFont.lfFaceName, 
        nsnull, &font->fonttype, nsnull);
      ::SelectObject(aDC, oldFont);
      ::DeleteObject(hfont);
      if (!font->ccmap || font->ccmap == gEmptyCCMap) {
        font->flags |= NS_GLOBALFONT_SKIP;
        continue;
      }
#ifdef DEBUG_emk
      LARGE_INTEGER now;
      QueryPerformanceCounter(&now);
      printf("CCMAP loaded: %g sec, %s [%08X][%08X][%08X]\n", (now.QuadPart - prev.QuadPart) / (double)freq.QuadPart,
        font->logFont.lfFaceName, font->signature.fsUsb[0], font->signature.fsUsb[1], font->signature.fsUsb[2]);
      prev = now;
#endif
      if (SameAsPreviousMap(i)) {
        continue;
      }
    }
    if (CCMAP_HAS_CHAR_EXT(font->ccmap, c)) {
#ifdef DEBUG_emk
      printf("font found:[%s]\n", font->logFont.lfFaceName);
      printf("U+%04X (%d)[%08X][%08X][%08X]\n", c, range, usb[0], usb[1], usb[2]);
#endif
      return LoadGlobalFont(aDC, font);
    }
  }
#ifdef DEBUG_emk
  printf("U+%04X (%d)[%08X][%08X][%08X]\n", c, range, usb[0], usb[1], usb[2]);
#endif
  return nsnull;
}




nsFontWin*
nsFontMetricsWin::FindSubstituteFont(HDC aDC, PRUint32 c)
{
  









  
  
  
  
  
  
  

  
  
  

  if (mSubstituteFont) {
    
    
    ((nsFontWinSubstitute*)mSubstituteFont)->SetRepresentable(c);
    return mSubstituteFont;
  }

  
  
  

  
  int i, count = mLoadedFonts.Count();
  for (i = 0; i < count; ++i) {
    nsAutoString name;
    nsFontWin* font = (nsFontWin*)mLoadedFonts[i];

    if (!font->mFont)
      continue;

    HFONT oldFont = (HFONT)::SelectObject(aDC, font->mFont);
    eGetNameError res = GetNAME(aDC, &name);
    ::SelectObject(aDC, oldFont);
    if (res == eGetName_OK) { 
      nsFontInfo* info = (nsFontInfo*)PL_HashTableLookup(gFontMaps, &name);
      if (!info || info->mType != eFontType_Unicode) {
        continue;
      }
    }
    else if (res == eGetName_GDIError) { 
      

#ifdef WINCE
      name.AssignWithConversion(font->mName);
      font = LoadSubstituteFont(aDC, name);
      if (font) {
        ((nsFontWinSubstitute*)font)->SetRepresentable(c);
        mSubstituteFont = font;
        return font;
      }
#endif
    }
    else {
      continue;
    }
    if (font->HasGlyph(NS_REPLACEMENT_CHAR)) {
      
      
      
      name.AssignWithConversion(font->mName);
      font = LoadSubstituteFont(aDC, name);
      if (font) {
        ((nsFontWinSubstitute*)font)->SetRepresentable(c);
        mSubstituteFont = font;
        return font;
      }
    }
  }

  
  
  
  count = gGlobalFonts->Count();
  for (i = 0; i < count; ++i) {
    nsGlobalFont* globalFont = (nsGlobalFont*)gGlobalFonts->ElementAt(i);
    if (!globalFont->ccmap || 
        globalFont->flags & NS_GLOBALFONT_SKIP ||
        globalFont->fonttype != eFontType_Unicode) {
      continue;
    }
    if (CCMAP_HAS_CHAR(globalFont->ccmap, NS_REPLACEMENT_CHAR)) {
      nsFontWin* font = LoadSubstituteFont(aDC, globalFont->name);
      if (font) {
        ((nsFontWinSubstitute*)font)->SetRepresentable(c);
        mSubstituteFont = font;
        return font;
      }
    }
  }
  
  NS_ASSERTION(::GetMapMode(aDC) == MM_TEXT, "mapping mode needs to be MM_TEXT");
  nsFontWin* font = LoadSubstituteFont(aDC, EmptyString());
  if (font) {
    ((nsFontWinSubstitute*)font)->SetRepresentable(c);
    mSubstituteFont = font;
    return font;
  }

  NS_ERROR("Could not provide a substititute font");
  return nsnull;
}

nsFontWin*
nsFontMetricsWin::LoadSubstituteFont(HDC aDC, const nsString& aName)
{
  LOGFONT logFont;
  HFONT hfont = !aName.IsEmpty()
    ? CreateFontHandle(aDC, aName, &logFont)
    : (HFONT)::GetStockObject(SYSTEM_FONT);
  if (hfont) {
    
    PRBool displayUnicode = PR_FALSE;
    
    LOGFONT* lfont = !aName.IsEmpty() ? &logFont : nsnull;
    nsFontWinSubstitute* font = new nsFontWinSubstitute(lfont, hfont, nsnull, displayUnicode);
    if (font) {
      HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
      InitMetricsFor(aDC, font);
      mLoadedFonts.AppendElement((nsFontWin*)font);
      ::SelectObject(aDC, (HGDIOBJ)oldFont);
      return font;
    }
    ::DeleteObject((HGDIOBJ)hfont);
  }
  return nsnull;
}

















class nsFontWeightEntry : public PLHashEntry
{
public:
  nsString mFontName;
  PRUint16 mWeightTable; 
};

static PLHashNumber
HashKeyFontWeight(const void* aFontWeightEntry)
{
  const nsString* string = &((const nsFontWeightEntry*) aFontWeightEntry)->mFontName;
  return (PLHashNumber)nsCRT::HashCode(string->get());
}

static PRIntn
CompareKeysFontWeight(const void* aFontWeightEntry1, const void* aFontWeightEntry2)
{
  const nsString* str1 = &((const nsFontWeightEntry*) aFontWeightEntry1)->mFontName;
  const nsString* str2 = &((const nsFontWeightEntry*) aFontWeightEntry2)->mFontName;
  return nsCRT::strcmp(str1->get(), str2->get()) == 0;
}





PR_STATIC_CALLBACK(void*) fontweight_AllocTable(void *pool, size_t size)
{
  return nsMemory::Alloc(size);
}

PR_STATIC_CALLBACK(void) fontweight_FreeTable(void *pool, void *item)
{
  nsMemory::Free(item);
}

PR_STATIC_CALLBACK(PLHashEntry*) fontweight_AllocEntry(void *pool, const void *key)
{
  return new nsFontWeightEntry;
}

PR_STATIC_CALLBACK(void) fontweight_FreeEntry(void *pool, PLHashEntry *he, PRUint32 flag)
{
  if (flag == HT_FREE_ENTRY)  {
    nsFontWeightEntry *fontWeightEntry = NS_STATIC_CAST(nsFontWeightEntry *, he);
    delete fontWeightEntry;
  }
}

PLHashAllocOps fontweight_HashAllocOps = {
  fontweight_AllocTable, fontweight_FreeTable,
  fontweight_AllocEntry, fontweight_FreeEntry
};


void nsFontMetricsWin::SetFontWeight(PRInt32 aWeight, PRUint16* aWeightTable) {
  NS_ASSERTION((aWeight >= 0) && (aWeight <= 9), "Invalid font weight passed");
  *aWeightTable |= 1 << (aWeight - 1);
}


PRBool nsFontMetricsWin::IsFontWeightAvailable(PRInt32 aWeight, PRUint16 aWeightTable) {
  PRInt32 normalizedWeight = aWeight / 100;
  NS_ASSERTION((aWeight >= 100) && (aWeight <= 900), "Invalid font weight passed");
  PRUint16 bitwiseWeight = 1 << (normalizedWeight - 1);
  return (bitwiseWeight & aWeightTable) != 0;
}

typedef struct {
  LOGFONT  mLogFont;
  PRUint16 mWeights;
  int      mFontCount;
} nsFontWeightInfo;

static int CALLBACK
nsFontWeightCallback(const LOGFONT* logFont, const TEXTMETRIC * metrics,
                     DWORD fontType, LPARAM closure)
{

  nsFontWeightInfo* weightInfo = (nsFontWeightInfo*)closure;
  if (metrics) {
      
    if (weightInfo->mFontCount == 0)
      weightInfo->mLogFont = *logFont;
    nsFontMetricsWin::SetFontWeight(metrics->tmWeight / 100,
      &weightInfo->mWeights);
    ++weightInfo->mFontCount;
  }
  return TRUE; 
}

static void
SearchSimulatedFontWeight(HDC aDC, nsFontWeightInfo* aWeightInfo)
{
  int weight, weightCount;

  
  if (aWeightInfo->mFontCount == 0) {
    return;
  }

  
  weightCount = 0;
  for (weight = 100; weight <= 900; weight += 100) {
    if (nsFontMetricsWin::IsFontWeightAvailable(weight, aWeightInfo->mWeights))
      ++weightCount;
  }

  
  
  
  if ((weightCount == 0) || (weightCount > 1)) {
    return;
  }

  
  
  LOGFONT logFont = aWeightInfo->mLogFont;
  for (weight = 100; weight <= 900; weight += 100) {
    logFont.lfWeight = weight;
    HFONT hfont = ::CreateFontIndirect(&logFont);
    HFONT oldfont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);

    TEXTMETRIC metrics;
    GetTextMetrics(aDC, &metrics);
    if (metrics.tmWeight == weight) {



      nsFontMetricsWin::SetFontWeight(weight / 100, &aWeightInfo->mWeights);
    }

    ::SelectObject(aDC, (HGDIOBJ)oldfont);
    ::DeleteObject((HGDIOBJ)hfont);
  }
}

PRUint16 
nsFontMetricsWin::GetFontWeightTable(HDC aDC, const nsString& aFontName)
{
  
  LOGFONT logFont;
  logFont.lfCharSet = DEFAULT_CHARSET;

  
  
  
  WideCharToMultiByte(CP_ACP, 0, aFontName.get(), aFontName.Length() + 1,
                      logFont.lfFaceName, sizeof(logFont.lfFaceName),
                      nsnull, nsnull);

  logFont.lfPitchAndFamily = 0;

  nsFontWeightInfo weightInfo;
  weightInfo.mWeights = 0;
  weightInfo.mFontCount = 0;
  ::EnumFontFamiliesEx(aDC, &logFont, nsFontWeightCallback, (LPARAM)&weightInfo, 0);
  if (weightInfo.mFontCount == 0)
    ::EnumFontFamilies(aDC, logFont.lfFaceName, nsFontWeightCallback, (LPARAM)&weightInfo);
  SearchSimulatedFontWeight(aDC, &weightInfo);

  return weightInfo.mWeights;
}








PRInt32
nsFontMetricsWin::GetClosestWeight(PRInt32 aWeight, PRUint16 aWeightTable)
{
  

  
  if ((aWeight > 0) && IsFontWeightAvailable(aWeight, aWeightTable)) {
    return aWeight;
  }

  

  
  PRBool done = PR_FALSE;
  PRInt32 lighterWeight = 0;
  PRInt32 proposedLighterWeight = PR_MAX(0, aWeight - 100);
  while (!done && (proposedLighterWeight >= 100)) {
    if (IsFontWeightAvailable(proposedLighterWeight, aWeightTable)) {
      lighterWeight = proposedLighterWeight;
      done = PR_TRUE;
    } else {
      proposedLighterWeight -= 100;
    }
  }

  
  done = PR_FALSE;
  PRInt32 darkerWeight = 0;
  PRInt32 proposedDarkerWeight = PR_MIN(aWeight + 100, 900);
  while (!done && (proposedDarkerWeight <= 900)) {
    if (IsFontWeightAvailable(proposedDarkerWeight, aWeightTable)) {
      darkerWeight = proposedDarkerWeight;
      done = PR_TRUE;   
    } else {
      proposedDarkerWeight += 100;
    }
  }

  

  
  
  
  
  
  
  
  

  if (aWeight <= 500) {
    return lighterWeight ? lighterWeight : darkerWeight;
  } 

  
  

  
  
  
  
  return darkerWeight ? darkerWeight : lighterWeight;
}

PRInt32
nsFontMetricsWin::GetBolderWeight(PRInt32 aWeight, PRInt32 aDistance, PRUint16 aWeightTable)
{
  PRInt32 newWeight = aWeight;
  PRInt32 proposedWeight = aWeight + 100; 
  for (PRInt32 j = 0; j < aDistance; ++j) {
    PRBool foundWeight = PR_FALSE;
    while (!foundWeight && (proposedWeight <= NS_MAX_FONT_WEIGHT)) {
      if (IsFontWeightAvailable(proposedWeight, aWeightTable)) {
        newWeight = proposedWeight; 
        foundWeight = PR_TRUE;
      }
      proposedWeight += 100; 
    }
  }
  return newWeight;
}

PRInt32
nsFontMetricsWin::GetLighterWeight(PRInt32 aWeight, PRInt32 aDistance, PRUint16 aWeightTable)
{
  PRInt32 newWeight = aWeight;
  PRInt32 proposedWeight = aWeight - 100; 
  for (PRInt32 j = 0; j < aDistance; ++j) {
    PRBool foundWeight = PR_FALSE;
    while (!foundWeight && (proposedWeight >= NS_MIN_FONT_WEIGHT)) {
      if (IsFontWeightAvailable(proposedWeight, aWeightTable)) {
        newWeight = proposedWeight; 
        foundWeight = PR_TRUE;
      }
      proposedWeight -= 100; 
    }
  }
  return newWeight;
}

PRInt32
nsFontMetricsWin::GetFontWeight(PRInt32 aWeight, PRUint16 aWeightTable)
{
  
  
  PRInt32 remainder = aWeight % 100;
  PRInt32 normalizedWeight = aWeight / 100;
  PRInt32 selectedWeight = 0;

  
  if (remainder == 0) {
    selectedWeight = GetClosestWeight(aWeight, aWeightTable);
  } else {
    NS_ASSERTION((remainder < 10) || (remainder > 90), "Invalid bolder or lighter value");
    if (remainder < 10) {
      PRInt32 weight = GetClosestWeight(normalizedWeight * 100, aWeightTable);
      selectedWeight = GetBolderWeight(weight, remainder, aWeightTable);
    } else {
      
      
      PRInt32 weight = GetClosestWeight((normalizedWeight + 1) * 100, aWeightTable);
      selectedWeight = GetLighterWeight(weight, 100-remainder, aWeightTable);
    }
  }


  return selectedWeight;
}

PRUint16
nsFontMetricsWin::LookForFontWeightTable(HDC aDC, const nsString& aName)
{
  
  if (!gFontWeights) {
    gFontWeights = PL_NewHashTable(0, HashKeyFontWeight, CompareKeysFontWeight, nsnull, &fontweight_HashAllocOps,
      nsnull);
    if (!gFontWeights) {
      return 0;
    }
  }

  
  
  
  nsAutoString low(aName);
  ToLowerCase(low);

   
  nsFontWeightEntry searchEntry;
  searchEntry.mFontName = low;
  searchEntry.mWeightTable = 0;

  nsFontWeightEntry* weightEntry;
  PLHashEntry **hep, *he;
  PLHashNumber hash = HashKeyFontWeight(&searchEntry);
  hep = PL_HashTableRawLookup(gFontWeights, hash, &searchEntry);
  he = *hep;
  if (he) {
    
    weightEntry = NS_STATIC_CAST(nsFontWeightEntry *, he);
    return weightEntry->mWeightTable;
  }

   
  PRUint16 weightTable = GetFontWeightTable(aDC, aName);


   
  he = PL_HashTableRawAdd(gFontWeights, hep, hash, &searchEntry, nsnull);
  if (he) {   
    weightEntry = NS_STATIC_CAST(nsFontWeightEntry*, he);
    weightEntry->mFontName = low;
    weightEntry->mWeightTable = weightTable;
    he->key = weightEntry;
    he->value = weightEntry;
    return weightEntry->mWeightTable;
  }

  return 0;
}



nsFontWin*
nsFontMetricsWin::FindUserDefinedFont(HDC aDC, PRUint32 aChar)
{
  if (mIsUserDefined) {
    
    nsFontWin* font = LoadFont(aDC, mUserDefined);
    mIsUserDefined = PR_FALSE;
    if (font && font->HasGlyph(aChar))
      return font;    
  }
  return nsnull;
}

nsFontWin*
nsFontMetricsWin::FindLocalFont(HDC aDC, PRUint32 aChar)
{
  while (mFontsIndex < mFonts.Count()) {
    if (mFontsIndex == mGenericIndex) {
      return nsnull;
    }

    nsString* name = mFonts.StringAt(mFontsIndex++);
    nsAutoString winName; 
    PRBool found = LookupWinFontName(*name, winName); 
    
    nsFontWin* font = LoadFont(aDC, found ? winName : *name,
                               mFont.familyNameQuirks);
    if (font && font->HasGlyph(aChar)) {
      return font;
    }
  }
  return nsnull;
}

nsFontWin*
nsFontMetricsWin::LoadGenericFont(HDC aDC, PRUint32 aChar, const nsString& aName)
{
  for (int i = mLoadedFonts.Count()-1; i >= 0; --i) {
    
    const nsACString& fontName =
      nsDependentCString(((nsFontWin*)mLoadedFonts[i])->mName);
    if (aName.Equals(NS_ConvertASCIItoUTF16(fontName),
                     nsCaseInsensitiveStringComparator()))
      return nsnull;

  }
  nsFontWin* font = LoadFont(aDC, aName);
  if (font && font->HasGlyph(aChar)) {
    return font;
  }
  return nsnull;
}

struct GenericFontEnumContext
{
  HDC               mDC;
  PRUint32          mChar;
  nsFontWin*        mFont;
  nsFontMetricsWin* mMetrics;
};

static PRBool PR_CALLBACK
GenericFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void* aData)
{
  GenericFontEnumContext* context = (GenericFontEnumContext*)aData;
  HDC dc = context->mDC;
  PRUint32 ch = context->mChar;
  nsFontMetricsWin* metrics = context->mMetrics;
  context->mFont = metrics->LoadGenericFont(dc, ch, aFamily);
  if (context->mFont) {
    return PR_FALSE; 
  }
  return PR_TRUE; 
}

#define MAKE_FONT_PREF_KEY(_pref, _s0, _s1) \
 _pref.Assign(_s0); \
 _pref.Append(_s1);

static void 
AppendGenericFontFromPref(nsString& aFontname,
                          const char* aLangGroup,
                          const char* aGeneric)
{
  nsresult res;
  nsCAutoString pref;
  nsXPIDLString value;
  nsCAutoString generic_dot_langGroup;

  generic_dot_langGroup.Assign(aGeneric);
  generic_dot_langGroup.Append('.');
  generic_dot_langGroup.Append(aLangGroup);

  
  
  MAKE_FONT_PREF_KEY(pref, "font.name.", generic_dot_langGroup);
  res = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));      
  if (NS_SUCCEEDED(res)) {
    if(!aFontname.IsEmpty())
      aFontname.Append((PRUnichar)',');
    aFontname.Append(value);
  }

  
  
  MAKE_FONT_PREF_KEY(pref, "font.name-list.", generic_dot_langGroup);
  res = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));      
  if (NS_SUCCEEDED(res)) {
    if(!aFontname.IsEmpty())
      aFontname.Append((PRUnichar)',');
    aFontname.Append(value);
  }
}

nsFontWin*
nsFontMetricsWin::FindGenericFont(HDC aDC, PRUint32 aChar)
{
  if (mTriedAllGenerics) {
    
    return nsnull;
  }

  
  
  nsFont font("", 0, 0, 0, 0, 0);

  if (mLangGroup) {
    const char* langGroup;
    mLangGroup->GetUTF8String(&langGroup);
  
    
    
    
    
    

    if (!strcmp(langGroup, "x-unicode")) {
      mTriedAllGenerics = 1;
      return nsnull;
    }

    AppendGenericFontFromPref(font.name, langGroup, 
                              NS_ConvertUTF16toUTF8(mGeneric).get());
  }

  
  GenericFontEnumContext context = {aDC, aChar, nsnull, this};
  font.EnumerateFamilies(GenericFontEnumCallback, &context);
  if (context.mFont) { 
    return context.mFont;
  }

#if defined(DEBUG_rbs) || defined(DEBUG_shanjian)
  const char* lang;
  mLangGroup->GetUTF8String(&lang);
  NS_ConvertUTF16toUTF8 generic(mGeneric);
  NS_ConvertUTF16toUTF8 family(mFont.name);
  printf("FindGenericFont missed:U+%04X langGroup:%s generic:%s mFont.name:%s\n", 
         aChar, lang, generic.get(), family.get());
#endif

  mTriedAllGenerics = 1;
  return nsnull;
}

#define IsCJKLangGroupAtom(a)  ((a)==gJA || (a)==gKO || (a)==gZHCN || \
                                (a)==gZHTW || (a) == gZHHK)

nsFontWin*
nsFontMetricsWin::FindPrefFont(HDC aDC, PRUint32 aChar)
{
  if (mTriedAllPref) {
    
    return nsnull;
  }
  nsFont font("", 0, 0, 0, 0, 0);

  
  
  
  
  
  
  
  
  
  

  PRUint32 unicodeRange = FindCharUnicodeRange(aChar);
  if (unicodeRange < kRangeSpecificItemNum) {
    
    AppendGenericFontFromPref(font.name, LangGroupFromUnicodeRange(unicodeRange), 
                              NS_ConvertUTF16toUTF8(mGeneric).get());
  } else if (kRangeSetLatin == unicodeRange) { 
    
    
    
    AppendGenericFontFromPref(font.name, "x-western",
                              NS_ConvertUTF16toUTF8(mGeneric).get());
    AppendGenericFontFromPref(font.name, "x-central-euro",
                              NS_ConvertUTF16toUTF8(mGeneric).get());
  } else if (kRangeSetCJK == unicodeRange) { 
    
    
    
    if ((gUsersLocale != mLangGroup) && IsCJKLangGroupAtom(gUsersLocale)) {
      nsCAutoString usersLocaleLangGroup;
      gUsersLocale->ToUTF8String(usersLocaleLangGroup);
      AppendGenericFontFromPref(font.name, usersLocaleLangGroup.get(), 
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    }
    
    
    if ((gSystemLocale != mLangGroup) && (gSystemLocale != gUsersLocale) && IsCJKLangGroupAtom(gSystemLocale)) {
      nsCAutoString systemLocaleLangGroup;
      gSystemLocale->ToUTF8String(systemLocaleLangGroup);
      AppendGenericFontFromPref(font.name, systemLocaleLangGroup.get(), 
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    }

    
    if (mLangGroup != gJA && gUsersLocale != gJA && gSystemLocale != gJA)
      AppendGenericFontFromPref(font.name, "ja",
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    if (mLangGroup != gZHCN && gUsersLocale != gZHCN && gSystemLocale != gZHCN)
      AppendGenericFontFromPref(font.name, "zh-CN",
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    if (mLangGroup != gZHTW && gUsersLocale != gZHTW && gSystemLocale != gZHTW)
      AppendGenericFontFromPref(font.name, "zh-TW",
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    if (mLangGroup != gZHHK && gUsersLocale != gZHHK && gSystemLocale != gZHHK)
      AppendGenericFontFromPref(font.name, "zh-HK",
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    if (mLangGroup != gKO && gUsersLocale != gKO && gSystemLocale != gKO)
      AppendGenericFontFromPref(font.name, "ko",
                                NS_ConvertUTF16toUTF8(mGeneric).get());
  } 

  
  AppendGenericFontFromPref(font.name, "x-unicode",
                            NS_ConvertUTF16toUTF8(mGeneric).get());
  
  
  GenericFontEnumContext context = {aDC, aChar, nsnull, this};
  font.EnumerateFamilies(GenericFontEnumCallback, &context);
  if (context.mFont) { 
    return context.mFont;
  }
  mTriedAllPref = 1;
  return nsnull;
}

nsFontWin*
nsFontMetricsWin::FindFont(HDC aDC, PRUint32 aChar)
{
#ifdef DEBUG_emk
  LARGE_INTEGER start, end;
  QueryPerformanceFrequency(&freq);
  QueryPerformanceCounter(&start);
#endif
  
  if (mLoadedFonts.Count() < 1)
    mLoadedFonts.AppendElement(gFontForIgnorable);
  if (gFontForIgnorable->HasGlyph(aChar))
      return gFontForIgnorable;

  nsFontWin* font = FindUserDefinedFont(aDC, aChar);
  if (!font) {
    font = FindLocalFont(aDC, aChar);
    if (!font) {
      font = FindGenericFont(aDC, aChar);
      if (!font) {
        font = FindPrefFont(aDC, aChar);
        if (!font) {
#ifdef DEBUG_emk
          QueryPerformanceCounter(&prev);
#endif
          font = FindGlobalFont(aDC, aChar);
          if (!font) {
#ifdef DEBUG_emk
            QueryPerformanceCounter(&end);
            printf("%g sec.\n", (end.QuadPart - start.QuadPart) / (double)freq.QuadPart);
#endif
            font = FindSubstituteFont(aDC, aChar);
          }
        }
      }
    }
  }
  return font;
}

nsFontWin*
nsFontMetricsWin::GetFontFor(HFONT aHFONT)
{
  int count = mLoadedFonts.Count();
  for (int i = 0; i < count; ++i) {
    nsFontWin* font = (nsFontWin*)mLoadedFonts[i];
    if (font->mFont == aHFONT)
      return font;
  }
  NS_ERROR("Cannot find the font that owns the handle");
  return nsnull;
}

static PRBool
FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  nsFontMetricsWin* metrics = (nsFontMetricsWin*) aData;
  metrics->mFonts.AppendString(aFamily);
  if (aGeneric) {
    metrics->mGeneric.Assign(aFamily);
    ToLowerCase(metrics->mGeneric);
    return PR_FALSE; 
  }
  ++metrics->mGenericIndex;

  return PR_TRUE; 
}






nsresult
nsFontMetricsWin::RealizeFont()
{
  nsresult rv;
  HWND win = NULL;
  HDC  dc = NULL;
  HDC  dc1 = NULL;

  if (mDeviceContext->mDC){
    
    
    
    
    dc = mDeviceContext->mDC;
    win = (HWND)mDeviceContext->mWidget;
    dc1 = ::GetDC(win);
  } else {
    
    win = (HWND)mDeviceContext->mWidget;
    dc = ::GetDC(win);
    dc1 = dc;
  }

  mFont.EnumerateFamilies(FontEnumCallback, this); 

  nsCAutoString pref;
  nsXPIDLString value;

  
  if (mGeneric.IsEmpty()) {
    pref.Assign("font.default.");
    const char* langGroup;
    mLangGroup->GetUTF8String(&langGroup);
    pref.Append(langGroup);
    rv = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      mGeneric.Assign(value);
    }
    else {
      mGeneric.AssignLiteral("serif");
    }
  }

  if (mLangGroup.get() == gUserDefined) {
    if (!gUserDefinedConverter) {
      rv = gCharsetManager->GetUnicodeEncoderRaw("x-user-defined", &gUserDefinedConverter);
      if (NS_FAILED(rv)) return rv;
      gUserDefinedConverter->SetOutputErrorBehavior(
        gUserDefinedConverter->kOnError_Replace, nsnull, '?');
      nsCOMPtr<nsICharRepresentable> mapper =
        do_QueryInterface(gUserDefinedConverter);
      if (mapper) {
        gUserDefinedCCMap = MapperToCCMap(mapper);
      }
    }

    
    
    pref.Assign("font.name.");
    pref.AppendWithConversion(mGeneric);
    pref.Append(".x-user-def");
    rv = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      mUserDefined.Assign(value);
      mIsUserDefined = 1;
    }
  }

  nsFontWin* font = FindFont(dc1, 'a');
  NS_ASSERTION(font, "missing font");
  if (!font) {
    ::ReleaseDC(win, mDeviceContext->mDC ? dc1 : dc);
    return NS_ERROR_FAILURE;
  }
  mFontHandle = font->mFont;

  HFONT oldfont = (HFONT)::SelectObject(dc, (HGDIOBJ) mFontHandle);

  
  float dev2app;
  dev2app = mDeviceContext->DevUnitsToAppUnits();
  OUTLINETEXTMETRIC oMetrics;
  TEXTMETRIC& metrics = oMetrics.otmTextMetrics;
  nscoord onePixel = NSToCoordRound(1 * dev2app);
  nscoord descentPos = 0;

  if (0 < ::GetOutlineTextMetrics(dc, sizeof(oMetrics), &oMetrics)) {

    mXHeight = NSToCoordRound((float)metrics.tmAscent * dev2app * 0.56f); 
    if (oMetrics.otmptSuperscriptOffset.y == 0 || oMetrics.otmptSuperscriptOffset.y >= metrics.tmAscent)
      mSuperscriptOffset = mXHeight;     
    else
      mSuperscriptOffset = NSToCoordRound(oMetrics.otmptSuperscriptOffset.y * dev2app);
    if (oMetrics.otmptSubscriptOffset.y == 0 || oMetrics.otmptSubscriptOffset.y >= metrics.tmAscent)
      mSubscriptOffset =  mXHeight;     
    else
      mSubscriptOffset = NSToCoordRound(oMetrics.otmptSubscriptOffset.y * dev2app);

    mStrikeoutSize = PR_MAX(onePixel, NSToCoordRound(oMetrics.otmsStrikeoutSize * dev2app));
    mStrikeoutOffset = NSToCoordRound(oMetrics.otmsStrikeoutPosition * dev2app);
    mUnderlineSize = PR_MAX(onePixel, NSToCoordRound(oMetrics.otmsUnderscoreSize * dev2app));
    if (gDoingLineheightFixup) {
      if(IsCJKLangGroupAtom(mLangGroup.get())) {
        mUnderlineOffset = NSToCoordRound(PR_MIN(oMetrics.otmsUnderscorePosition, 
                                                 oMetrics.otmDescent + oMetrics.otmsUnderscoreSize) 
                                                 * dev2app);
        
        descentPos = NSToCoordRound(oMetrics.otmDescent * dev2app);
      } else {
        mUnderlineOffset = NSToCoordRound(PR_MIN(oMetrics.otmsUnderscorePosition*dev2app, 
                                                 oMetrics.otmDescent*dev2app + mUnderlineSize));
      }
    }
    else
      mUnderlineOffset = NSToCoordRound(oMetrics.otmsUnderscorePosition * dev2app);

    
    GLYPHMETRICS gm;
    DWORD len = gGlyphAgent.GetGlyphMetrics(dc, PRUnichar('x'), 0, &gm);
    if (GDI_ERROR != len && gm.gmptGlyphOrigin.y > 0)
    {
      mXHeight = NSToCoordRound(gm.gmptGlyphOrigin.y * dev2app);
    }
    
  }
  else {
    
    
    ::GetTextMetrics(dc, &metrics);
    mXHeight = NSToCoordRound((float)metrics.tmAscent * dev2app * 0.56f); 
    mSuperscriptOffset = mXHeight;     
    mSubscriptOffset = mXHeight;     

    mStrikeoutSize = onePixel; 
    mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0f); 
    mUnderlineSize = onePixel; 
    mUnderlineOffset = -NSToCoordRound((float)metrics.tmDescent * dev2app * 0.30f); 
  }

  mInternalLeading = NSToCoordRound(metrics.tmInternalLeading * dev2app);
  mExternalLeading = NSToCoordRound(metrics.tmExternalLeading * dev2app);
  mEmHeight = NSToCoordRound((metrics.tmHeight - metrics.tmInternalLeading) *
                             dev2app);
  mEmAscent = NSToCoordRound((metrics.tmAscent - metrics.tmInternalLeading) *
                             dev2app);
  mEmDescent = NSToCoordRound(metrics.tmDescent * dev2app);
  mMaxHeight = NSToCoordRound(metrics.tmHeight * dev2app);
  mMaxAscent = NSToCoordRound(metrics.tmAscent * dev2app);
  mMaxDescent = NSToCoordRound(metrics.tmDescent * dev2app);
  mMaxAdvance = NSToCoordRound(metrics.tmMaxCharWidth * dev2app);
  
  
  mMaxStringLength = (PRInt32)floor(32767.0/metrics.tmMaxCharWidth);
  mMaxStringLength = PR_MAX(1, mMaxStringLength);
  
  mAveCharWidth = PR_MAX(1, NSToCoordRound(metrics.tmAveCharWidth * dev2app));

  if (gDoingLineheightFixup) {
    if (mInternalLeading + mExternalLeading > mUnderlineSize &&
        descentPos < mUnderlineOffset) {
      
      
      
      mUnderlineOffset = descentPos;
      nscoord extra = mUnderlineSize - mUnderlineOffset - mMaxDescent;
      if (extra > 0) {
        mEmDescent += extra;  
        mEmHeight += extra;
        mMaxDescent += extra;
        mMaxHeight += extra;
      }
    } else if (mUnderlineSize - mUnderlineOffset > mMaxDescent) {
      
      
      mUnderlineOffset = mUnderlineSize - mMaxDescent;
    }
  }
  
  SIZE  size;
  ::GetTextExtentPoint32(dc, " ", 1, &size);
  size.cx -= font->mOverhangCorrection;
  mSpaceWidth = NSToCoordRound(size.cx * dev2app);

  ::SelectObject(dc, oldfont);

  ::ReleaseDC(win, mDeviceContext->mDC ? dc1 : dc);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetSpaceWidth(nscoord& aSpaceWidth)
{
  aSpaceWidth = mSpaceWidth;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetXHeight(nscoord& aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetSuperscriptOffset(nscoord& aResult)
{
  aResult = mSuperscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetSubscriptOffset(nscoord& aResult)
{
  aResult = mSubscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mStrikeoutOffset;
  aSize = mStrikeoutSize;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mUnderlineOffset;
  aSize = mUnderlineSize;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

#ifdef FONT_LEADING_APIS_V2
NS_IMETHODIMP
nsFontMetricsWin::GetInternalLeading(nscoord &aLeading)
{
  aLeading = mInternalLeading;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetExternalLeading(nscoord &aLeading)
{
  aLeading = mExternalLeading;
  return NS_OK;
}
#else
NS_IMETHODIMP
nsFontMetricsWin::GetLeading(nscoord &aLeading)
{
  aLeading = mInternalLeading;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetNormalLineHeight(nscoord &aHeight)
{
  aHeight = mEmHeight + mInternalLeading;
  return NS_OK;
}
#endif 

NS_IMETHODIMP
nsFontMetricsWin::GetEmHeight(nscoord &aHeight)
{
  aHeight = mEmHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetEmAscent(nscoord &aAscent)
{
  aAscent = mEmAscent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetEmDescent(nscoord &aDescent)
{
  aDescent = mEmDescent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetMaxHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetMaxAscent(nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetMaxDescent(nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetMaxAdvance(nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetAveCharWidth(nscoord &aAveCharWidth)
{
  aAveCharWidth = mAveCharWidth;
  return NS_OK;
}

PRInt32
nsFontMetricsWin::GetMaxStringLength()
{
  return mMaxStringLength;
}

NS_IMETHODIMP
nsFontMetricsWin::GetLangGroup(nsIAtom** aLangGroup)
{
  NS_ENSURE_ARG_POINTER(aLangGroup);
  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsWin::GetFontHandle(nsFontHandle &aHandle)
{
  aHandle = mFontHandle;
  return NS_OK;
}

nsFontWin*
nsFontMetricsWin::LocateFont(HDC aDC, PRUint32 aChar, PRInt32 & aCount)
{
  nsFontWin *font;
  PRInt32 i;

  
  for (i = 0; i < aCount; ++i) {
    font = (nsFontWin*)mLoadedFonts[i];
    if (font->HasGlyph(aChar))
      return font;
  }

  font = FindFont(aDC, aChar);
  aCount = mLoadedFonts.Count(); 
  NS_ASSERTION(font && mLoadedFonts.IndexOf(font) >= 0,
               "Could not find a font");
  return font;
}

nsresult
nsFontMetricsWin::ResolveForwards(HDC                  aDC,
                                  const PRUnichar*     aString,
                                  PRUint32             aLength,
                                  nsFontSwitchCallback aFunc, 
                                  void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString;
  const PRUnichar* currChar = firstChar;
  const PRUnichar* lastChar  = aString + aLength;
  nsFontWin* currFont;
  nsFontWin* nextFont;
  PRInt32 count;
  nsFontSwitch fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  count = mLoadedFonts.Count();

  if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
    currFont = LocateFont(aDC, SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
    currChar += 2;
  }
  else {
    currFont = LocateFont(aDC, *currChar, count);
    ++currChar;
  }

  
  
  NS_ASSERTION(count > 1, "only one font loaded");
  
  PRUint32 firstFont = count > 1 ? 1 : 0; 
  if (currFont == mLoadedFonts[firstFont]) { 
    while (currChar < lastChar && 
           (currFont->HasGlyph(*currChar)) &&
           !CCMAP_HAS_CHAR_EXT(gIgnorableCCMapExt, *currChar))
      ++currChar;
    fontSwitch.mFontWin = currFont;
    if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
      return NS_OK;
    if (currChar == lastChar)
      return NS_OK;
    
    firstChar = currChar;
    if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
      currFont = LocateFont(aDC, SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
      currChar += 2;
    }
    else {
      currFont = LocateFont(aDC, *currChar, count);
      ++currChar;
    }
  }

  
  PRInt32 lastCharLen;
  while (currChar < lastChar) {
    if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
      nextFont = LocateFont(aDC, SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
      lastCharLen = 2;
    }
    else {
      nextFont = LocateFont(aDC, *currChar, count);
      lastCharLen = 1;
    }
    if (nextFont != currFont) {
      
      
      fontSwitch.mFontWin = currFont;
      if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
        return NS_OK;
      
      firstChar = currChar;

      currFont = nextFont; 
    }
    currChar += lastCharLen;
  }

  
  fontSwitch.mFontWin = currFont;
  (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
  return NS_OK;
}

nsresult
nsFontMetricsWin::ResolveBackwards(HDC                  aDC,
                                   const PRUnichar*     aString,
                                   PRUint32             aLength,
                                   nsFontSwitchCallback aFunc, 
                                   void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString + aLength - 1;
  const PRUnichar* lastChar  = aString - 1;
  const PRUnichar* currChar  = firstChar;
  nsFontWin* currFont;
  nsFontWin* nextFont;
  PRInt32 count;
  nsFontSwitch fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  count = mLoadedFonts.Count();

  
  if (NS_IS_LOW_SURROGATE(*currChar) && (currChar-1) > lastChar && NS_IS_HIGH_SURROGATE(*(currChar-1))) {
    currFont = LocateFont(aDC, SURROGATE_TO_UCS4(*(currChar-1), *currChar), count);
    currChar -= 2;
  }
  else {
    currFont = LocateFont(aDC, *currChar, count);
    --currChar;
  }

  
  
  NS_ASSERTION(count > 1, "only one font loaded");
  
  PRUint32 firstFont = count > 1 ? 1 : 0; 
  if (currFont == mLoadedFonts[firstFont]) {
    while (currChar > lastChar && 
           (currFont->HasGlyph(*currChar)) &&
           !CCMAP_HAS_CHAR_EXT(gIgnorableCCMapExt, *currChar) &&
           !IS_RTL_PRESENTATION_FORM(*currChar))
      --currChar;
    fontSwitch.mFontWin = currFont;
    if (!(*aFunc)(&fontSwitch, currChar+1, firstChar - currChar, aData))
      return NS_OK;
    if (currChar == lastChar)
      return NS_OK;
    
    firstChar = currChar;
    if (NS_IS_LOW_SURROGATE(*currChar) && (currChar-1) > lastChar && NS_IS_HIGH_SURROGATE(*(currChar-1))) {
      currFont = LocateFont(aDC, SURROGATE_TO_UCS4(*(currChar-1), *currChar), count);
      currChar -= 2;
    }
    else {
      currFont = LocateFont(aDC, *currChar, count);
      --currChar;
    }
  }

  
  PRInt32 lastCharLen;
  PRUint32 codepoint;

  while (currChar > lastChar) {
    if (NS_IS_LOW_SURROGATE(*currChar) && (currChar-1) > lastChar && NS_IS_HIGH_SURROGATE(*(currChar-1))) {
      codepoint =  SURROGATE_TO_UCS4(*(currChar-1), *currChar);
      nextFont = LocateFont(aDC, codepoint, count);
      lastCharLen = 2;
    }
    else {
      codepoint = *currChar;
      nextFont = LocateFont(aDC, codepoint, count);
      lastCharLen = 1;
    }
    if (nextFont != currFont ||
        




        codepoint > 0xFFFF ||
        IS_RTL_PRESENTATION_FORM(codepoint)) {
      
      
      fontSwitch.mFontWin = currFont;
      if (!(*aFunc)(&fontSwitch, currChar+1, firstChar - currChar, aData))
        return NS_OK;
      
      firstChar = currChar;
      currFont = nextFont; 
    }
    currChar -= lastCharLen;
  }

  
  fontSwitch.mFontWin = currFont;
  (*aFunc)(&fontSwitch, currChar+1, firstChar - currChar, aData);

  return NS_OK;
}



nsFontWin::nsFontWin(LOGFONT* aLogFont, HFONT aFont, PRUint16* aCCMap)
{
  if (aLogFont) {
    strcpy(mName, aLogFont->lfFaceName);
  }
  mFont = aFont;
  mCCMap = aCCMap;
}

nsFontWin::~nsFontWin()
{
  if (mFont) {
    ::DeleteObject(mFont);
    mFont = nsnull;
  }
}

PRInt32
nsFontWin::GetWidth(HDC aDC, const char* aString, PRUint32 aLength)
{
  SIZE size;
  ::GetTextExtentPoint32(aDC, aString, aLength, &size);
  size.cx -= mOverhangCorrection;
  return size.cx;
}

PRBool
nsFontWin::FillClipRect(PRInt32 aX, PRInt32 aY, UINT aLength, UINT uOptions, RECT& clipRect)
{
  if (!(uOptions & (ETO_CLIPPED | ETO_OPAQUE)) &&
      mOverhangCorrection > 0 && !(mPitchAndFamily & TMPF_FIXED_PITCH)) {
    
    
    
    clipRect.top = aY - mMaxHeightMetric;
    clipRect.bottom = aY + mMaxHeightMetric;
    clipRect.left = aX;
    clipRect.right = aX + mMaxCharWidthMetric * aLength;
    return PR_TRUE;
  }
  return PR_FALSE;
}

static PRBool
NS_ExtTextOutA(HDC aDC, nsFontWin* aFont, PRInt32 aX, PRInt32 aY, UINT uOptions,
  LPCRECT lprc, LPCSTR aString, UINT aLength, INT *lpDx)
{
  RECT clipRect;
  if (!lpDx && !lprc && aFont->FillClipRect(aX, aY, aLength, uOptions, clipRect)) {
    lprc = &clipRect;
    uOptions |= ETO_CLIPPED;
  }
  return ::ExtTextOutA(aDC, aX, aY, uOptions, lprc, aString, aLength, lpDx);
}

static PRBool
NS_ExtTextOutW(HDC aDC, nsFontWin* aFont, PRInt32 aX, PRInt32 aY, UINT uOptions,
  LPCRECT lprc, LPCWSTR aString, UINT aLength, INT *lpDx)
{
  RECT clipRect;
  if (!lpDx && !lprc && aFont->FillClipRect(aX, aY, aLength, uOptions, clipRect)) {
    lprc = &clipRect;
    uOptions |= ETO_CLIPPED;
  }
  return ::ExtTextOutW(aDC, aX, aY, uOptions, lprc, aString, aLength, lpDx);
}

void
nsFontWin::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const char* aString, PRUint32 aLength, INT* lpDx)
{
  NS_ExtTextOutA(aDC, this, aX, aY, 0, NULL, aString, aLength, lpDx);
}

#ifdef MOZ_MATHML
nsresult
nsFontWin::GetBoundingMetrics(HDC                aDC, 
                              const char*        aString,
                              PRUint32           aLength,
                              nsBoundingMetrics& aBoundingMetrics)
{
  return GetBoundingMetricsCommonA(aDC, mOverhangCorrection, aString, aLength, aBoundingMetrics);
}
#endif

#ifdef DEBUG
static void
VerifyFontHasGlyph(nsFontWin* aFont, const PRUnichar* aString, PRInt32 aLength)
{
  const PRUnichar* curr = aString;
  const PRUnichar* last = aString + aLength;
  PRUint32 ch;
  while (curr < last) {
    if (NS_IS_HIGH_SURROGATE(*curr) && (curr+1) < last && 
        NS_IS_LOW_SURROGATE(*(curr+1))) {
      ch = SURROGATE_TO_UCS4(*curr, *(curr+1));
      curr += 2;
    }
    else {
      ch = *curr;
      curr += 1;
    }
    NS_ASSERTION(aFont->HasGlyph(ch), "internal error");
  }
}
#define DEBUG_VERIFY_FONT_HASGLYPH(font, string, length) \
VerifyFontHasGlyph(font, string, length)
#else
#define DEBUG_VERIFY_FONT_HASGLYPH(font, string, length)
#endif

nsFontWinUnicode::nsFontWinUnicode(LOGFONT* aLogFont, HFONT aFont,
  PRUint16* aCCMap) : nsFontWin(aLogFont, aFont, aCCMap)
{



  NS_ASSERTION(aLogFont != NULL, "Null logfont passed to nsFontWinUnicode constructor");
  mUnderlinedOrStrikeOut = aLogFont->lfUnderline || aLogFont->lfStrikeOut;
}

nsFontWinUnicode::~nsFontWinUnicode()
{
}

PRInt32
nsFontWinUnicode::GetWidth(HDC aDC, const PRUnichar* aString, PRUint32 aLength)
{
  DEBUG_VERIFY_FONT_HASGLYPH(this, aString, aLength);
  SIZE size;
  ::GetTextExtentPoint32W(aDC, aString, aLength, &size);
  size.cx -= mOverhangCorrection;
  return size.cx;
}

void
nsFontWinUnicode::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const PRUnichar* aString, PRUint32 aLength)
{
  DEBUG_VERIFY_FONT_HASGLYPH(this, aString, aLength);
  
  
  
  
  
  
                           
  
  
  
  
  if (mUnderlinedOrStrikeOut) {
    if (IsWin95OrWin98()) {
      
      
      SIZE size;
      ::GetTextExtentPoint32W(aDC, aString, aLength, &size);
      size.cx -= mOverhangCorrection;
      RECT clipRect;
      clipRect.top = aY - size.cy;
      clipRect.bottom = aY + size.cy; 
                                      
      clipRect.left = aX;
      clipRect.right = aX + size.cx;
      NS_ExtTextOutW(aDC, this, aX, aY, ETO_CLIPPED, &clipRect, aString, aLength, NULL); 
      return;
    }
  } 

  
  NS_ExtTextOutW(aDC, this, aX, aY, 0, NULL, aString, aLength, NULL);  
}

#ifdef MOZ_MATHML
nsresult
nsFontWinUnicode::GetBoundingMetrics(HDC                aDC, 
                                     const PRUnichar*   aString,
                                     PRUint32           aLength,
                                     nsBoundingMetrics& aBoundingMetrics)
{
  DEBUG_VERIFY_FONT_HASGLYPH(this, aString, aLength);
  aBoundingMetrics.Clear();
  nsAutoChar16Buffer buffer;

  
  
  NS_ASSERTION(gGlyphAgent.GetState() != eGlyphAgent_UNKNOWN, "Glyph agent is not yet initialized");
  if (gGlyphAgent.GetState() != eGlyphAgent_UNICODE) {
    
    
    nsresult rv = GetGlyphIndices(aDC, &mCMAP, aString, aLength, buffer);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return GetBoundingMetricsCommon(aDC, mOverhangCorrection, aString, aLength, aBoundingMetrics, buffer.get());
}

#ifdef NS_DEBUG
void 
nsFontWinUnicode::DumpFontInfo()
{
  printf("FontName: %s @%p\n", mName, this);
  printf("FontType: nsFontWinUnicode\n");
}
#endif 
#endif

nsFontWinNonUnicode::nsFontWinNonUnicode(LOGFONT* aLogFont, HFONT aFont,
  PRUint16* aCCMap, nsIUnicodeEncoder* aConverter, PRBool aIsWide) : nsFontWin(aLogFont, aFont, aCCMap)
{
  mConverter = aConverter;
  mIsWide = aIsWide;
}

nsFontWinNonUnicode::~nsFontWinNonUnicode()
{
}

PRInt32
nsFontWinNonUnicode::GetWidth(HDC aDC, const PRUnichar* aString,
  PRUint32 aLength)
{
  DEBUG_VERIFY_FONT_HASGLYPH(this, aString, aLength);
  nsAutoCharBuffer buffer;

  PRInt32 destLength = aLength;
  if (NS_FAILED(ConvertUnicodeToGlyph(aString, aLength, destLength, 
                mConverter, mIsWide, buffer))) {
    return 0;
  }

  SIZE size;
  if (!mIsWide)
    ::GetTextExtentPoint32A(aDC, buffer.get(), destLength, &size);
  else
    ::GetTextExtentPoint32W(aDC, (const PRUnichar*) buffer.get(), destLength / 2, &size);
  size.cx -= mOverhangCorrection;

  return size.cx;
}

void
nsFontWinNonUnicode::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const PRUnichar* aString, PRUint32 aLength)
{
  DEBUG_VERIFY_FONT_HASGLYPH(this, aString, aLength);
  nsAutoCharBuffer buffer;
  PRInt32 destLength = aLength;

  if (NS_FAILED(ConvertUnicodeToGlyph(aString, aLength, destLength, 
                mConverter, mIsWide, buffer))) {
    return;
  }

  if (!mIsWide)
    NS_ExtTextOutA(aDC, this, aX, aY, 0, NULL, buffer.get(), aLength, NULL);
  else 
    NS_ExtTextOutW(aDC, this, aX, aY, 0, NULL, (const PRUnichar*) buffer.get(), destLength / 2, NULL);
}

#ifdef MOZ_MATHML
nsresult
nsFontWinNonUnicode::GetBoundingMetrics(HDC                aDC, 
                                        const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics)
{
  DEBUG_VERIFY_FONT_HASGLYPH(this, aString, aLength);
  aBoundingMetrics.Clear();
  nsAutoCharBuffer buffer;
  PRInt32 destLength = aLength;
  nsresult rv = NS_OK;

  rv = ConvertUnicodeToGlyph(aString, aLength, destLength, mConverter, 
                             mIsWide, buffer);
  if (NS_FAILED(rv))
    return rv;

  if (mIsWide) {
    nsAutoChar16Buffer buf;
    
    
    NS_ASSERTION(gGlyphAgent.GetState() != eGlyphAgent_UNKNOWN, "Glyph agent is not yet initialized");
    if (gGlyphAgent.GetState() != eGlyphAgent_UNICODE) {
      
      
      rv = GetGlyphIndices(aDC, &mCMAP, (const PRUnichar*)buffer.get(), destLength / 2, buf);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }

    
    
    return  GetBoundingMetricsCommon(aDC, mOverhangCorrection, (const PRUnichar*)buffer.get(), 
              destLength / 2, aBoundingMetrics, buf.get());

  }

  return GetBoundingMetricsCommonA(aDC, mOverhangCorrection, buffer.get(), destLength, 
                                   aBoundingMetrics);
}

#ifdef NS_DEBUG
void 
nsFontWinNonUnicode::DumpFontInfo()
{
  printf("FontName: %s @%p\n", mName, this);
  printf("FontType: nsFontWinNonUnicode\n");
}
#endif 
#endif 

nsFontWinSubstitute::nsFontWinSubstitute(LOGFONT* aLogFont, HFONT aFont,
  PRUint16* aCCMap, PRBool aDisplayUnicode) : nsFontWin(aLogFont, aFont, aCCMap)
{
  mDisplayUnicode = aDisplayUnicode;
  mIsForIgnorable = PR_FALSE; 
  memset(mRepresentableCharMap, 0, sizeof(mRepresentableCharMap));
}

nsFontWinSubstitute::nsFontWinSubstitute(PRUint16 *aCCMap) :
  nsFontWin(NULL, NULL, aCCMap)
{
  mIsForIgnorable = PR_TRUE;
  mDisplayUnicode = PR_FALSE;
  memset(mRepresentableCharMap, 0, sizeof(mRepresentableCharMap));
}

nsFontWinSubstitute::~nsFontWinSubstitute()
{
}

static nsresult
SubstituteChars(PRBool              aDisplayUnicode,
                const PRUnichar*    aString, 
                PRUint32            aLength,
                nsAutoChar16Buffer& aResult,
                PRUint32*           aCount)
{

#ifdef WINCE


  if (!aResult.EnsureElemCapacity(aLength))
    return NS_ERROR_OUT_OF_MEMORY;

  *aCount = aLength;
  memcpy(aResult.get(), aString, aLength * sizeof(PRUnichar));
  return NS_OK;

#else


  nsresult res;
  if (!gFontSubstituteConverter) {
    CallCreateInstance(NS_SAVEASCHARSET_CONTRACTID, &gFontSubstituteConverter);
    if (gFontSubstituteConverter) {
      
      
      
      
      res = gFontSubstituteConverter->Init("ISO-8859-1",
              aDisplayUnicode
              ? nsISaveAsCharset::attr_FallbackHexNCR
              : nsISaveAsCharset::attr_EntityAfterCharsetConv + nsISaveAsCharset::attr_FallbackQuestionMark + nsISaveAsCharset::attr_IgnoreIgnorables,
              nsIEntityConverter::transliterate);
      if (NS_FAILED(res)) {
        NS_RELEASE(gFontSubstituteConverter);
      }
    }
  }

  
  PRUnichar* result; 
  if (gFontSubstituteConverter) {
    nsXPIDLCString conv;
    nsAutoString tmp(aString, aLength); 
    res = gFontSubstituteConverter->Convert(tmp.get(), getter_Copies(conv));
    if (NS_SUCCEEDED(res)) {
      *aCount = conv.Length();
      if (*aCount > 0) {
        if (!aResult.EnsureElemCapacity(*aCount)) {
          return NS_ERROR_OUT_OF_MEMORY;
        }
        result = aResult.get();
        PRUnichar* u = result;
        const char* c = conv.get();
        for (; *c; ++c, ++u) {
          *u = *c;
        }
      }
      return NS_OK;
    }
  }

  
  if (!aResult.EnsureElemCapacity(aLength)) return NS_ERROR_OUT_OF_MEMORY;
  result = aResult.get();
  for (PRUint32 i = 0; i < aLength; i++) {
    result[i] = NS_REPLACEMENT_CHAR;
  }
  *aCount = aLength;
  return NS_OK;
#endif 
}

PRInt32
nsFontWinSubstitute::GetWidth(HDC aDC, const PRUnichar* aString,
  PRUint32 aLength)
{
  if (mIsForIgnorable)
    return 0;
  nsAutoChar16Buffer buffer;
  nsresult rv = SubstituteChars(PR_FALSE, aString, aLength, buffer, &aLength);
  if (NS_FAILED(rv) || !aLength) return 0;

  SIZE size;
  ::GetTextExtentPoint32W(aDC, buffer.get(), aLength, &size);
  size.cx -= mOverhangCorrection;

  return size.cx;
}

void
nsFontWinSubstitute::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const PRUnichar* aString, PRUint32 aLength)
{
  if (mIsForIgnorable)
    return;
  nsAutoChar16Buffer buffer;
  nsresult rv = SubstituteChars(PR_FALSE, aString, aLength, buffer, &aLength);
  if (NS_FAILED(rv) || !aLength) return;

  NS_ExtTextOutW(aDC, this, aX, aY, 0, NULL, buffer.get(), aLength, NULL);
}

#ifdef MOZ_MATHML
nsresult
nsFontWinSubstitute::GetBoundingMetrics(HDC                aDC, 
                                        const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics)
{
  aBoundingMetrics.Clear();
  if (mIsForIgnorable)
    return NS_OK;
  nsAutoChar16Buffer buffer;
  nsresult rv = SubstituteChars(mDisplayUnicode, aString, aLength, buffer, &aLength);
  if (NS_FAILED(rv)) {
    return rv;
  }
  if (!aLength) return NS_OK;
  nsAutoChar16Buffer buf;

  
  
  NS_ASSERTION(gGlyphAgent.GetState() != eGlyphAgent_UNKNOWN, "Glyph agent is not yet initialized");
  if (gGlyphAgent.GetState() != eGlyphAgent_UNICODE) {
    
    
    rv = GetGlyphIndices(aDC, &mCMAP, buffer.get(), aLength, buf);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

  return GetBoundingMetricsCommon(aDC, mOverhangCorrection, buffer.get(), aLength, 
                                  aBoundingMetrics, buf.get());
}

#ifdef NS_DEBUG
void 
nsFontWinSubstitute::DumpFontInfo()
{
  printf("FontName: %s @%p\n", mIsForIgnorable ? "For the ignorable" : mName, this);
  printf("FontType: nsFontWinSubstitute\n");
}
#endif 
#endif

static PRUint16*
GenerateDefault(nsCharsetInfo* aSelf)
{ 
  PRUint32 map[UCS2_MAP_LEN];

  memset(map, 0xff, sizeof(map));
  return MapToCCMap(map);
}

static PRUint16*
GenerateSingleByte(nsCharsetInfo* aSelf)
{ 
  PRUint32 map[UCS2_MAP_LEN];
  PRUint8 mb[256];
  WCHAR wc[256];
  int i;

  memset(map, 0, sizeof(map));
  if (UseAFunctions()) {
    
    for (i = 0; i <= 255; ++i) {
      mb[i] = i;
    }
  } else {
    memset(mb + 128, 0, 160 - 128);
    for (i = 0; i <= 127; ++i) {
      mb[i] = i;
    }
    mb[145] = 145;
    mb[146] = 146;

    if (aSelf->mCodePage == 1250) {
      mb[138] = 138;
      mb[140] = 140;
      mb[141] = 141;
      mb[142] = 142;
      mb[143] = 143;
      mb[154] = 154;
      mb[156] = 156;
      mb[158] = 158;
      mb[159] = 159;
    }

    for (i = 160; i <= 255; ++i) {
      mb[i] = i;
    }

    
    
    
    if (IsWin95OrWin98())
      mb[0xb7] = 0;
  }

  int len = MultiByteToWideChar(aSelf->mCodePage, 0, (char*) mb, 256, wc, 256);
#ifdef NS_DEBUG
  if (len != 256) {
    printf("%s: MultiByteToWideChar returned %d\n", aSelf->mName, len);
  }
#endif
  for (i = 0; i <= 255; ++i) {
    ADD_GLYPH(map, wc[i]);
  }
  return MapToCCMap(map);
}

static PRUint16*
GenerateMultiByte(nsCharsetInfo* aSelf)
{ 
  PRUint32 map[UCS2_MAP_LEN];
  memset(map, 0, sizeof(map));
  for (WCHAR c = 0; c < 0xFFFF; ++c) {
    BOOL defaulted = FALSE;
    WideCharToMultiByte(aSelf->mCodePage, 0, &c, 1, nsnull, 0, nsnull,
      &defaulted);
    if (!defaulted) {
      ADD_GLYPH(map, c);
    }
  }
  return MapToCCMap(map);
}

static int
HaveConverterFor(PRUint8 aCharset)
{
  WCHAR wc = 'a';
  char mb[8];
  if (WideCharToMultiByte(gCharsetInfo[gCharsetToIndex[aCharset]].mCodePage, 0,
                          &wc, 1, mb, sizeof(mb), nsnull, nsnull)) {
    return 1;
  }
  
  for (int i = sizeof(gBitToCharset)-1; i >= 0; --i) {
    if (gBitToCharset[i] == aCharset) {
      gBitToCharset[i] = DEFAULT_CHARSET;
    }
  }

  return 0;
}

nsFontSubset::nsFontSubset()
  : nsFontWin(nsnull, nsnull, nsnull)
{
}

nsFontSubset::~nsFontSubset()
{
}

void
nsFontSubset::Convert(const PRUnichar* aString, PRUint32 aLength,
  nsAutoCharBuffer& aResult, PRUint32* aResultLength)
{
  *aResultLength = 0;
  
  int nb = WideCharToMultiByte(mCodePage, 0, aString, aLength,
                               nsnull, 0, nsnull, nsnull);

  if (!nb || !aResult.EnsureElemCapacity(nb)) return;
  char* buf = aResult.get();
  
  *aResultLength = WideCharToMultiByte(mCodePage, 0, aString, aLength,
                                       buf, nb, nsnull, nsnull);
}

PRInt32
nsFontSubset::GetWidth(HDC aDC, const PRUnichar* aString, PRUint32 aLength)
{
  nsAutoCharBuffer buffer;
  Convert(aString, aLength, buffer, &aLength);
  if (aLength) {
    SIZE size;
    ::GetTextExtentPoint32A(aDC, buffer.get(), aLength, &size);
    size.cx -= mOverhangCorrection;
    return size.cx;
  }
  return 0;
}

void
nsFontSubset::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const PRUnichar* aString, PRUint32 aLength)
{
  nsAutoCharBuffer buffer;
  Convert(aString, aLength, buffer, &aLength);
  if (aLength) {
    NS_ExtTextOutA(aDC, this, aX, aY, 0, NULL, buffer.get(), aLength, NULL);
  }
}

#ifdef MOZ_MATHML
nsresult
nsFontSubset::GetBoundingMetrics(HDC                aDC, 
                                 const PRUnichar*   aString,
                                 PRUint32           aLength,
                                 nsBoundingMetrics& aBoundingMetrics)
{
  aBoundingMetrics.Clear();
  nsAutoCharBuffer buffer;
  Convert(aString, aLength, buffer, &aLength);
  if (aLength) {
    return GetBoundingMetricsCommonA(aDC, mOverhangCorrection, buffer.get(), aLength, 
                                     aBoundingMetrics);
  }
  return NS_OK;
}

#ifdef NS_DEBUG
void 
nsFontSubset::DumpFontInfo()
{
  printf("FontName: %s @%p\n", mName, this);
  printf("FontType: nsFontSubset\n");
}
#endif 
#endif

nsFontSubsetSubstitute::nsFontSubsetSubstitute(PRBool aIsForIgnorable)
  : nsFontSubset(), mIsForIgnorable(aIsForIgnorable)
{
}

nsFontSubsetSubstitute::~nsFontSubsetSubstitute()
{
}

PRInt32
nsFontSubsetSubstitute::GetWidth(HDC aDC, const PRUnichar* aString,
  PRUint32 aLength)
{
  if (mIsForIgnorable)
    return 0;

  return nsFontSubset::GetWidth(aDC, aString, aLength);
}

void
nsFontSubsetSubstitute::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const PRUnichar* aString, PRUint32 aLength)
{
  if (mIsForIgnorable)
    return;

  nsFontSubset::DrawString(aDC, aX, aY, aString, aLength);
}

#ifdef MOZ_MATHML
nsresult
nsFontSubsetSubstitute::GetBoundingMetrics(HDC                aDC, 
                                        const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics)
{
  aBoundingMetrics.Clear();
  if (mIsForIgnorable)
    return NS_OK;

  return nsFontSubset::GetBoundingMetrics(aDC, aString, aLength, 
                                          aBoundingMetrics);
}

#ifdef NS_DEBUG
void 
nsFontSubsetSubstitute::DumpFontInfo()
{
  printf("FontName: %s @%p\n", mIsForIgnorable ? "For the ignorable" : mName, this);
  printf("FontType: nsFontSubsetSubstitute\n");
}
#endif 
#endif

void
nsFontSubsetSubstitute::Convert(const PRUnichar* aString, PRUint32 aLength,
  nsAutoCharBuffer& aResult, PRUint32* aResultLength)
{
  nsAutoChar16Buffer buffer;
  nsresult rv = SubstituteChars(PR_FALSE, aString, aLength, buffer, &aLength);
  if (NS_FAILED(rv)) {
    
    *aResultLength = 0;
    return;
  }
  if (!aLength) {
    
    *(aResult.get()) = '\0';
    *aResultLength = 0;
    return;
  }
  nsFontSubset::Convert(buffer.get(), aLength, aResult, aResultLength);
}

nsFontWinA::nsFontWinA(LOGFONT* aLogFont, HFONT aFont, PRUint16* aCCMap)
  : nsFontWin(aLogFont, aFont, aCCMap)
{
  if (aLogFont) {
    mLogFont = *aLogFont;
  }
}

nsFontWinA::~nsFontWinA()
{
  if (mSubsets) {
    nsFontSubset** subset = mSubsets;
    nsFontSubset** endSubsets = subset + mSubsetsCount;
    while (subset < endSubsets) {
      delete *subset;
      ++subset;
    }
    nsMemory::Free(mSubsets);
    mSubsets = nsnull;
  }
}

int
nsFontWinA::GetSubsets(HDC aDC)
{
  mSubsetsCount = 0;

  FONTSIGNATURE signature;
  if (::GetTextCharsetInfo(aDC, &signature, 0) == DEFAULT_CHARSET) {
    return 0;
  }

  int dword;
  DWORD* array = signature.fsCsb;
  int i = 0;
  for (dword = 0; dword < 2; ++dword) {
    for (int bit = 0; bit < sizeof(DWORD) * 8; ++bit, ++i) {
      if ((array[dword] >> bit) & 1) {
        PRUint8 charset = gBitToCharset[i];
#ifdef DEBUG_FONT_SIGNATURE
        printf("  %02d %s\n", i, gCharsetInfo[gCharsetToIndex[charset]].mName);
#endif
        if ((charset != DEFAULT_CHARSET) && HaveConverterFor(charset)) {
          ++mSubsetsCount;
        }
      }
    }
  }

  if (!mSubsetsCount) {
    return 0;
  }
  mSubsets = (nsFontSubset**) nsMemory::Alloc(mSubsetsCount* sizeof(nsFontSubset*));
  if (!mSubsets) {
    mSubsetsCount = 0;
    return 0;
  }
  memset(mSubsets, 0, mSubsetsCount * sizeof(nsFontSubset*)); 

  int j;
  for (j = 0; j < mSubsetsCount; ++j) {
    mSubsets[j] = new nsFontSubset();
    if (!mSubsets[j]) {
      for (j = j - 1; j >= 0; --j) {
        delete mSubsets[j];
      }
      nsMemory::Free(mSubsets);
      mSubsets = nsnull;
      mSubsetsCount = 0;
      return 0;
    }
  }

  i = j = 0;
  for (dword = 0; dword < 2; ++dword) {
    for (int bit = 0; bit < sizeof(DWORD) * 8; ++bit, ++i) {
      if ((array[dword] >> bit) & 1) {
        PRUint8 charset = gBitToCharset[i];
        if ((charset != DEFAULT_CHARSET) && HaveConverterFor(charset)) {
          mSubsets[j]->mCharset = charset;
          ++j;
        }
      }
    }
  }

  return 1;
}

PRInt32
nsFontWinA::GetWidth(HDC aDC, const PRUnichar* aString, PRUint32 aLength)
{
  NS_ERROR("must call nsFontSubset's GetWidth");
  return 0;
}

void
nsFontWinA::DrawString(HDC aDC, PRInt32 aX, PRInt32 aY,
  const PRUnichar* aString, PRUint32 aLength)
{
  NS_ERROR("must call nsFontSubset's DrawString");
}

nsFontSubset* 
nsFontWinA::FindSubset(HDC aDC, PRUnichar aChar, nsFontMetricsWinA* aFontMetrics)
{
  nsFontSubset **subsetPtr = mSubsets;
  nsFontSubset **endSubset = subsetPtr + mSubsetsCount;

  while (subsetPtr < endSubset) {
    if (!(*subsetPtr)->mCCMap)
      if (!(*subsetPtr)->Load(aDC, aFontMetrics, this))
        continue;
    if ((*subsetPtr)->HasGlyph(aChar)) {
      return *subsetPtr;
    }
    ++subsetPtr;
  }
  return nsnull;
}


#ifdef MOZ_MATHML
nsresult
nsFontWinA::GetBoundingMetrics(HDC                aDC, 
                               const PRUnichar*   aString,
                               PRUint32           aLength,
                               nsBoundingMetrics& aBoundingMetrics)
{
  NS_ERROR("must call nsFontSubset's GetBoundingMetrics");
  return NS_ERROR_FAILURE;
}

#ifdef NS_DEBUG
void 
nsFontWinA::DumpFontInfo()
{
  NS_ERROR("must call nsFontSubset's DumpFontInfo");
}
#endif 
#endif

nsFontWin*
nsFontMetricsWinA::LoadFont(HDC aDC, const nsString& aName, PRBool aNameQuirks)
{
  LOGFONT logFont;
  HFONT hfont = CreateFontHandle(aDC, aName, &logFont);
  if (hfont) {
#ifdef DEBUG_FONT_SIGNATURE
    printf("%s\n", logFont.lfFaceName);
#endif
    HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
    char name[sizeof(logFont.lfFaceName)];
    if (::GetTextFace(aDC, sizeof(name), name) &&
        !strcmpi(name, logFont.lfFaceName)) {
      PRUint16* ccmap = GetCCMAP(aDC, logFont.lfFaceName, 
        nsnull, nsnull, nsnull);
      if (ccmap) {
        nsFontWinA* font = new nsFontWinA(&logFont, hfont, ccmap);
        if (font) {
          if (font->GetSubsets(aDC)) {
            
            
            mLoadedFonts.AppendElement(font);
            ::SelectObject(aDC, (HGDIOBJ)oldFont);
            return font;
          }
          ::SelectObject(aDC, (HGDIOBJ)oldFont);
          delete font; 
          return nsnull;
        }
        
        
      }
    }
    ::SelectObject(aDC, (HGDIOBJ)oldFont);
    ::DeleteObject(hfont);
  }
  return nsnull;
}

nsFontWin*
nsFontMetricsWinA::LoadGlobalFont(HDC aDC, nsGlobalFont* aGlobalFont)
{
  LOGFONT logFont;
  HFONT hfont = CreateFontHandle(aDC, aGlobalFont, &logFont);
  if (hfont) {
    nsFontWinA* font = new nsFontWinA(&logFont, hfont, aGlobalFont->ccmap);
    if (font) {
      HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
      if (font->GetSubsets(aDC)) {
        
        
        mLoadedFonts.AppendElement(font);
        ::SelectObject(aDC, (HGDIOBJ)oldFont);
        return font;
      }
      ::SelectObject(aDC, (HGDIOBJ)oldFont);
      delete font; 
      return nsnull;
    }
    ::DeleteObject(hfont);
  }
  return nsnull;
}

int
nsFontSubset::Load(HDC aDC, nsFontMetricsWinA* aFontMetricsWinA, nsFontWinA* aFont)
{
  LOGFONT* logFont = &aFont->mLogFont;
  logFont->lfCharSet = mCharset;
  
  HFONT hfont = (aFontMetricsWinA->Font().sizeAdjust <= 0) 
    ? ::CreateFontIndirect(logFont)
    : aFontMetricsWinA->CreateFontAdjustHandle(aDC, logFont);
  if (hfont) {
    int i = gCharsetToIndex[mCharset];
    if (!gCharsetInfo[i].mCCMap)
      gCharsetInfo[i].mCCMap = gCharsetInfo[i].GenerateMap(&gCharsetInfo[i]);
    if (!gCharsetInfo[i].mCCMap) {
      ::DeleteObject(hfont);
      return 0;
    }
    mCCMap = gCharsetInfo[i].mCCMap;
    mCodePage = gCharsetInfo[i].mCodePage;
    mFont = hfont;

    
    HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
    aFontMetricsWinA->InitMetricsFor(aDC, this);
    ::SelectObject(aDC, (HGDIOBJ)oldFont);

    return 1;
  }
  return 0;
}

nsFontWin*
nsFontMetricsWinA::GetFontFor(HFONT aHFONT)
{
  int count = mLoadedFonts.Count();
  for (int i = 0; i < count; ++i) {
    nsFontWinA* font = (nsFontWinA*)mLoadedFonts[i];
    nsFontSubset** subset = font->mSubsets;
    nsFontSubset** endSubsets = subset + font->mSubsetsCount;
    while (subset < endSubsets) {
      if ((*subset)->mFont == aHFONT) {
        return *subset;
      }
      ++subset;
    }
  }
  NS_ERROR("Cannot find the font that owns the handle");
  return nsnull;
}

nsFontWin*
nsFontMetricsWinA::FindLocalFont(HDC aDC, PRUint32 aChar)
{
  while (mFontsIndex < mFonts.Count()) {
    if (mFontsIndex == mGenericIndex) {
      return nsnull;
    }
    nsString* name = mFonts.StringAt(mFontsIndex++);
    nsAutoString winName; 
    PRBool found = LookupWinFontName(*name, winName); 
    nsFontWinA* font = (nsFontWinA*)LoadFont(aDC, found ? winName : *name);
    if (font && font->HasGlyph(aChar)) {
      nsFontSubset* subset = font->FindSubset(aDC, (PRUnichar)aChar, this);
      if (subset) 
        return subset;
    }
  }
  return nsnull;
}

nsFontWin*
nsFontMetricsWinA::LoadGenericFont(HDC aDC, PRUint32 aChar, const nsString& aName)
{
  for (int i = mLoadedFonts.Count()-1; i >= 0; --i) {

    if (aName.EqualsIgnoreCase(((nsFontWin*)mLoadedFonts[i])->mName))
      return nsnull;

  }
  nsFontWinA* font = (nsFontWinA*)LoadFont(aDC, aName);
  if (font && font->HasGlyph(aChar)) {
    return font->FindSubset(aDC, (PRUnichar)aChar, this);
  }
  return nsnull;
}

static int
SystemSupportsChar(PRUnichar aChar)
{
  for (int i = 0; i < sizeof(gBitToCharset); ++i) {
    PRUint8 charset = gBitToCharset[i];
    if (charset == DEFAULT_CHARSET) 
      continue;
    if (!HaveConverterFor(charset)) 
      continue;
    int j = gCharsetToIndex[charset];
    if (!gCharsetInfo[j].mCCMap) {
      gCharsetInfo[j].mCCMap = gCharsetInfo[j].GenerateMap(&gCharsetInfo[j]);
      if (!gCharsetInfo[j].mCCMap) 
          return 0;
    }
    if (CCMAP_HAS_CHAR(gCharsetInfo[j].mCCMap, aChar)) 
      return 1;
  }

  return 0;
}

nsFontWin*
nsFontMetricsWinA::FindGlobalFont(HDC aDC, PRUint32 c)
{
  if (!gGlobalFonts) {
    if (!InitializeGlobalFonts(aDC)) {
      return nsnull;
    }
  }
  if (!SystemSupportsChar(c)) {
    return nsnull;
  }
  int count = gGlobalFonts->Count();
  for (int i = 0; i < count; ++i) {
    nsGlobalFont* globalFont = (nsGlobalFont*)gGlobalFonts->ElementAt(i); 
    if (globalFont->flags & NS_GLOBALFONT_SKIP) {
      continue;
    }
    if (!globalFont->ccmap) {
      
      
      HFONT hfont = ::CreateFontIndirect(&globalFont->logFont);
      if (!hfont) {
        continue;
      }
      HFONT oldFont = (HFONT)::SelectObject(aDC, hfont);
      globalFont->ccmap = GetCCMAP(aDC, globalFont->logFont.lfFaceName,
        nsnull, nsnull, nsnull);
      ::SelectObject(aDC, oldFont);
      ::DeleteObject(hfont);
      if (!globalFont->ccmap || globalFont->ccmap == gEmptyCCMap) {
        globalFont->flags |= NS_GLOBALFONT_SKIP;
        continue;
      }
      if (SameAsPreviousMap(i)) {
        continue;
      }
    }
    if (CCMAP_HAS_CHAR(globalFont->ccmap, c)) {
      nsFontWinA* font = (nsFontWinA*)LoadGlobalFont(aDC, globalFont);
      if (!font) {
        
        
        globalFont->flags |= NS_GLOBALFONT_SKIP;
        continue;
      }
      nsFontSubset* subset = font->FindSubset(aDC, (PRUnichar)c, this);
      if (subset) {
        return subset;
      }
      mLoadedFonts.RemoveElement(font);
      delete font;
    }
  }

  return nsnull;
}

nsFontWinSubstituteA::nsFontWinSubstituteA(LOGFONT* aLogFont, HFONT aFont,
  PRUint16* aCCMap) : nsFontWinA(aLogFont, aFont, aCCMap),
  mIsForIgnorable(PR_FALSE)
{
  memset(mRepresentableCharMap, 0, sizeof(mRepresentableCharMap));
}

nsFontWinSubstituteA::nsFontWinSubstituteA(PRUint16* aCCMap) :
  nsFontWinA(NULL, NULL, aCCMap),
  mIsForIgnorable(PR_TRUE)
{
  memset(mRepresentableCharMap, 0, sizeof(mRepresentableCharMap));
}

nsFontWinSubstituteA::~nsFontWinSubstituteA()
{
}

nsFontWin*
nsFontMetricsWinA::FindSubstituteFont(HDC aDC, PRUint32 aChar)
{
  
  
  
  
  

  if (mSubstituteFont) {
    
    
    ((nsFontWinSubstituteA*)mSubstituteFont)->SetRepresentable(aChar);
    return ((nsFontWinA*)mSubstituteFont)->mSubsets[0];
  }

  
  int i, count = mLoadedFonts.Count();
  for (i = 0; i < count; ++i) {
    nsFontWinA* font = (nsFontWinA*)mLoadedFonts[i];
    if (font->HasGlyph(NS_REPLACEMENT_CHAR)) {
      nsFontSubset* subset = font->FindSubset(aDC, NS_REPLACEMENT_CHAR, this);
      if (subset) {
        
        nsAutoString name;
        name.AssignWithConversion(font->mName);
        nsFontWinSubstituteA* substituteFont = (nsFontWinSubstituteA*)LoadSubstituteFont(aDC, name);
        if (substituteFont) {
          nsFontSubset* substituteSubset = substituteFont->mSubsets[0];
          substituteSubset->mCharset = subset->mCharset;
          if (substituteSubset->Load(aDC, this, substituteFont)) {
            substituteFont->SetRepresentable(aChar);
            mSubstituteFont = (nsFontWin*)substituteFont;
            return substituteSubset;
          }
          mLoadedFonts.RemoveElement(substituteFont);
          delete substituteFont;
        }
      }
    }
  }

  
  
  
  count = gGlobalFonts->Count();
  for (i = 0; i < count; ++i) {
    nsGlobalFont* globalFont = (nsGlobalFont*)gGlobalFonts->ElementAt(i);
    if (!globalFont->ccmap || 
        globalFont->flags & NS_GLOBALFONT_SKIP) {
      continue;
    }
    if (CCMAP_HAS_CHAR(globalFont->ccmap, NS_REPLACEMENT_CHAR)) {
      
      BYTE charset = DEFAULT_CHARSET;
      nsFontWinA* font = (nsFontWinA*)LoadGlobalFont(aDC, globalFont);
      if (!font) {
        globalFont->flags |= NS_GLOBALFONT_SKIP;
        continue;
      }
      nsFontSubset* subset = font->FindSubset(aDC, NS_REPLACEMENT_CHAR, this);
      if (subset) {
        charset = subset->mCharset;
      }
      mLoadedFonts.RemoveElement(font);
      delete font;
      if (charset != DEFAULT_CHARSET) {
        
        nsFontWinSubstituteA* substituteFont = (nsFontWinSubstituteA*)LoadSubstituteFont(aDC, globalFont->name);
        if (substituteFont) {
          nsFontSubset* substituteSubset = substituteFont->mSubsets[0];
          substituteSubset->mCharset = charset;
          if (substituteSubset->Load(aDC, this, substituteFont)) {
            substituteFont->SetRepresentable(aChar);
            mSubstituteFont = (nsFontWin*)substituteFont;
            return substituteSubset;
          }
          mLoadedFonts.RemoveElement(substituteFont);
          delete substituteFont;
        }
      }
    }
  }

  
  NS_ERROR("Could not provide a substititute font");
  return nsnull;
}

nsFontWin*
nsFontMetricsWinA::LoadSubstituteFont(HDC aDC, const nsString& aName)
{
  LOGFONT logFont;
  HFONT hfont = CreateFontHandle(aDC, aName, &logFont);
  if (hfont) {
    HFONT oldFont = (HFONT)::SelectObject(aDC, (HGDIOBJ)hfont);
    char name[sizeof(logFont.lfFaceName)];
    if (::GetTextFace(aDC, sizeof(name), name) &&
        !strcmpi(name, logFont.lfFaceName)) {
      nsFontWinSubstituteA* font = new nsFontWinSubstituteA(&logFont, hfont, nsnull);
      if (font) {
        font->mSubsets = (nsFontSubset**)nsMemory::Alloc(sizeof(nsFontSubset*));
        if (font->mSubsets) {
          font->mSubsets[0] = nsnull;
          nsFontSubsetSubstitute* subset = new nsFontSubsetSubstitute();
          if (subset) {
            font->mSubsetsCount = 1;
            font->mSubsets[0] = subset;
            mLoadedFonts.AppendElement((nsFontWin*)font);
            ::SelectObject(aDC, (HGDIOBJ)oldFont);
            return font;
          }
        }
        ::SelectObject(aDC, (HGDIOBJ)oldFont);
        delete font; 
        return nsnull;
      }
    }
    ::SelectObject(aDC, (HGDIOBJ)oldFont);
    ::DeleteObject(hfont);
  }
  return nsnull;
}

nsFontSubset*
nsFontMetricsWinA::LocateFontSubset(HDC aDC, PRUnichar aChar, PRInt32 & aCount, nsFontWinA*& aFont)
{
  nsFontSubset *fontSubset;
  PRInt32 i;

  
  for (i = 0; i < aCount; ++i) {
    aFont = (nsFontWinA*)mLoadedFonts[i];
    if (aFont->HasGlyph(aChar)) {
      fontSubset = aFont->FindSubset(aDC, aChar, this);
      if (fontSubset)
        return fontSubset;
    }
  }

  fontSubset = (nsFontSubset*)FindFont(aDC, aChar);
  aFont = nsnull;   
  aCount = mLoadedFonts.Count(); 
  return fontSubset;
}

nsresult
nsFontMetricsWinA::ResolveForwards(HDC                  aDC,
                                   const PRUnichar*     aString,
                                   PRUint32             aLength,
                                   nsFontSwitchCallback aFunc, 
                                   void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString;
  const PRUnichar* lastChar  = aString + aLength;
  const PRUnichar* currChar  = firstChar;
  nsFontSubset* currSubset;
  nsFontSubset* nextSubset;
  nsFontWinA* currFont;
  PRInt32 count;
  nsFontSwitch fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  
  count = mLoadedFonts.Count();
  currSubset = LocateFontSubset(aDC, *currChar, count, currFont);

  
  
  NS_ASSERTION(count > 1, "only one font loaded");
  
  PRUint32 firstFont = count > 1 ? 1 : 0; 
  if (currFont == mLoadedFonts[firstFont]) { 
    while (++currChar < lastChar && 
           currFont->HasGlyph(*currChar) && currSubset->HasGlyph(*currChar) &&
           !CCMAP_HAS_CHAR_EXT(gIgnorableCCMapExt, *currChar))
      ;

    fontSwitch.mFontWin = currSubset;
    if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
      return NS_OK;
    if (currChar == lastChar)
      return NS_OK;
    
    firstChar = currChar;
    currSubset = LocateFontSubset(aDC, *currChar, count, currFont); 
  }

  while (++currChar < lastChar) {
    nextSubset = LocateFontSubset(aDC, *currChar, count, currFont);
    if (nextSubset != currSubset) {
      
      
      fontSwitch.mFontWin = currSubset;
      if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
        return NS_OK;
      
      firstChar = currChar;
      currSubset = nextSubset; 
    }
  }

  
  fontSwitch.mFontWin = currSubset;
  NS_ASSERTION(currSubset, "invalid font here. ");
  (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);

  return NS_OK;
}

nsresult
nsFontMetricsWinA::ResolveBackwards(HDC                  aDC,
                                    const PRUnichar*     aString,
                                    PRUint32             aLength,
                                    nsFontSwitchCallback aFunc, 
                                    void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString + aLength - 1;
  const PRUnichar* lastChar  = aString - 1;
  const PRUnichar* currChar  = firstChar;
  nsFontSubset* currSubset;
  nsFontSubset* nextSubset;
  nsFontWinA* currFont;
  PRInt32 count;
  nsFontSwitch fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  
  count = mLoadedFonts.Count();
  currSubset = LocateFontSubset(aDC, *currChar, count, currFont);

  while (--currChar < lastChar) {
    nextSubset = LocateFontSubset(aDC, *currChar, count, currFont);
    if (nextSubset != currSubset) {
      
      
      fontSwitch.mFontWin = currSubset;
      if (!(*aFunc)(&fontSwitch, firstChar, firstChar - currChar, aData))
        return NS_OK;
      
      firstChar = currChar;
      currSubset = nextSubset; 
    }
  }

  
  fontSwitch.mFontWin = currSubset;
  (*aFunc)(&fontSwitch, firstChar, firstChar - currChar, aData);

  return NS_OK;
}



nsFontEnumeratorWin::nsFontEnumeratorWin()
{
}

NS_IMPL_ISUPPORTS1(nsFontEnumeratorWin,nsIFontEnumerator)

static int gInitializedFontEnumerator = 0;

static int
InitializeFontEnumerator(void)
{
  gInitializedFontEnumerator = 1;
  if (!nsFontMetricsWin::gGlobalFonts) {
    HDC dc = ::GetDC(nsnull);
    if (!nsFontMetricsWin::InitializeGlobalFonts(dc)) {
      ::ReleaseDC(nsnull, dc);
      return 0;
    }
    ::ReleaseDC(nsnull, dc);
  }

  return 1;
}

static int
CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure)
{
  const PRUnichar* str1 = *((const PRUnichar**) aArg1);
  const PRUnichar* str2 = *((const PRUnichar**) aArg2);

  

  return nsCRT::strcmp(str1, str2);
}

static int
SignatureMatchesLangGroup(FONTSIGNATURE* aSignature,
  const char* aLangGroup)
{
  int dword;

  
  
  const char *langGroup = strcmp(aLangGroup, "zh-HK") ? aLangGroup : "zh-TW";

  
  
  DWORD* array = aSignature->fsCsb;
  int i = 0;
  for (dword = 0; dword < 2; ++dword) {
    for (int bit = 0; bit < sizeof(DWORD) * 8; ++bit) {
      if ((array[dword] >> bit) & 1) {
        if (!strcmp(gCharsetInfo[gCharsetToIndex[gBitToCharset[i]]].mLangGroup,
                    langGroup)) {
          return 1;
        }
      }
      ++i;
    }
  }

  
  
  
  

  
  for (i = eCharset_ANSI; i <= eCharset_CHINESEBIG5; ++i) 
  {
    if (!strcmp(gCharsetInfo[i].mLangGroup, langGroup))
      return 0;
  }

  
  
  

  array = aSignature->fsUsb;
  for (i = 0, dword = 0; dword < 3; ++dword) {
    for (int bit = 0; bit < sizeof(DWORD) * 8; ++bit) {
      if ((array[dword] >> bit) & 1) {
        PRUint8 range = gBitToUnicodeRange[i];
        if (kRangeSpecificItemNum > range &&
            !strcmp(gUnicodeRangeToLangGroupTable[range], langGroup)) {
          return 1;
        }
      }
      ++i;
    }
  }

  return 0;
}

static int
FontMatchesGenericType(nsGlobalFont* aFont, const char* aGeneric)
{
  PRUint8 family = aFont->logFont.lfPitchAndFamily & 0xF0;
  PRUint8 pitch = aFont->logFont.lfPitchAndFamily & 0x0F;

  
  
  if (family == FF_ROMAN && pitch & FIXED_PITCH) {
    return !strcmp(aGeneric, "monospace");
  }

  
  
  if (family == FF_MODERN && pitch & VARIABLE_PITCH) {
    return !strcmp(aGeneric, "sans-serif");
  }

  
  switch (family) {
    case FF_DONTCARE:   return 1;
    case FF_ROMAN:      return !strcmp(aGeneric, "serif");
    case FF_SWISS:      return !strcmp(aGeneric, "sans-serif");
    case FF_MODERN:     return !strcmp(aGeneric, "monospace");
    case FF_SCRIPT:     return !strcmp(aGeneric, "cursive");
    case FF_DECORATIVE: return !strcmp(aGeneric, "fantasy");
  }

  return 0;
}

static nsresult
EnumerateMatchingFonts(const char* aLangGroup,
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult)
{
  
  

  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  *aCount = 0;
  *aResult = nsnull;

  if (aLangGroup && *aLangGroup && 
     (!strcmp(aLangGroup, "x-unicode") ||
      !strcmp(aLangGroup, "x-user-def"))) {
    return EnumerateMatchingFonts(nsnull, nsnull, aCount, aResult);
  }

  if (!gInitializedFontEnumerator) {
    if (!InitializeFontEnumerator()) {
      return NS_ERROR_FAILURE;
    }
  }

  int count = nsFontMetricsWin::gGlobalFonts->Count();
  PRUnichar** array = (PRUnichar**)nsMemory::Alloc(count * sizeof(PRUnichar*));
  if (!array) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  int j = 0;
  for (int i = 0; i < count; ++i) {
    nsGlobalFont* font = (nsGlobalFont*)nsFontMetricsWin::gGlobalFonts->ElementAt(i);
    PRBool accept = PR_TRUE;
    if (aLangGroup && *aLangGroup) {
      accept = SignatureMatchesLangGroup(&font->signature, aLangGroup);
    }
    if (accept && aGeneric && *aGeneric) {
      accept = FontMatchesGenericType(font, aGeneric);
    }
    if (accept) {
      PRUnichar* str = ToNewUnicode(font->name);
      if (!str) {
        for (j = j - 1; j >= 0; --j) {
          nsMemory::Free(array[j]);
        }
        nsMemory::Free(array);
        return NS_ERROR_OUT_OF_MEMORY;
      }
      array[j] = str;
      ++j;
    }
  }

  NS_QuickSort(array, j, sizeof(PRUnichar*), CompareFontNames, nsnull);

  *aCount = j;
  *aResult = array;

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorWin::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
{
  return EnumerateMatchingFonts(nsnull, nsnull, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorWin::EnumerateFonts(const char* aLangGroup,
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult)
{
  return EnumerateMatchingFonts(aLangGroup, aGeneric, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorWin::HaveFontFor(const char* aLangGroup, PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aLangGroup);
  NS_ENSURE_ARG_POINTER(aResult);

  *aResult = PR_FALSE;
  if ((!strcmp(aLangGroup, "x-unicode")) ||
      (!strcmp(aLangGroup, "x-user-def"))) {
    *aResult = PR_TRUE;
    return NS_OK;
  }
  if (!gInitializedFontEnumerator) {
    if (!InitializeFontEnumerator()) {
      return NS_ERROR_FAILURE;
    }
  }
  int count = nsFontMetricsWin::gGlobalFonts->Count();
  for (int i = 0; i < count; ++i) {
    nsGlobalFont* font = (nsGlobalFont*)nsFontMetricsWin::gGlobalFonts->ElementAt(i);
    if (SignatureMatchesLangGroup(&font->signature, aLangGroup)) {
       *aResult = PR_TRUE;
       return NS_OK;
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorWin::GetDefaultFont(const char *aLangGroup, 
  const char *aGeneric, PRUnichar **aResult)
{
  
  

  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorWin::UpdateFontList(PRBool *updateFontList)
{
  PRBool haveFontForLang = PR_FALSE;
  int charsetCounter = 0;
  PRUint16 maskBitBefore = 0;

  
  *updateFontList = PR_FALSE;

  
  for (charsetCounter = 1; charsetCounter < eCharset_COUNT; ++charsetCounter) {
    HaveFontFor(gCharsetInfo[charsetCounter].mLangGroup, &haveFontForLang);
    if (haveFontForLang) {
      maskBitBefore |= PR_BIT(charsetCounter);
      haveFontForLang = PR_FALSE;
    }
  }

  
  if (nsFontMetricsWin::gGlobalFonts) {
    for (int i = nsFontMetricsWin::gGlobalFonts->Count()-1; i >= 0; --i) {
      nsGlobalFont* font = (nsGlobalFont*)nsFontMetricsWin::gGlobalFonts->ElementAt(i);
      delete font;
    }
    delete nsFontMetricsWin::gGlobalFonts;
    nsFontMetricsWin::gGlobalFonts = nsnull;
  }

  
  HDC dc = ::GetDC(nsnull);
  if (!nsFontMetricsWin::InitializeGlobalFonts(dc)) {
    ::ReleaseDC(nsnull, dc);
    return NS_ERROR_FAILURE;
  }
  ::ReleaseDC(nsnull, dc);

  PRUint16 maskBitAfter = 0;
  
  for (charsetCounter = 1; charsetCounter < eCharset_COUNT; ++charsetCounter) {
    HaveFontFor(gCharsetInfo[charsetCounter].mLangGroup, &haveFontForLang);
    if (haveFontForLang) {
      maskBitAfter |= PR_BIT(charsetCounter);
      haveFontForLang = PR_FALSE;
    }
  }

  
  *updateFontList = (maskBitBefore != maskBitAfter);
  return NS_OK;
}




 
PRBool LookupWinFontName(const nsAFlatString& aName, nsAString& aWinName)
{
  
  if (aName.LowerCaseEqualsLiteral("tahoma") ||
      aName.LowerCaseEqualsLiteral("arial") ||
      aName.LowerCaseEqualsLiteral("times new roman") ||
      aName.LowerCaseEqualsLiteral("courier new"))
    return PR_FALSE;

  if (!gFontNameMapProperties)
    NS_LoadPersistentPropertiesFromURISpec(&gFontNameMapProperties,
      NS_LITERAL_CSTRING("resource://gre/res/fonts/fontNameMap.properties"));

  if (!gFontNameMapProperties) {
    NS_WARNING("fontNameMap.properties is not available"); 
    return PR_FALSE;
  }

  nsAutoString name(aName); 
  ToLowerCase(name); 

  NS_ConvertUTF16toUTF8 propKey(name);
  propKey.StripWhitespace(); 

  if (IsASCII(propKey)) {
    
    
    LossyAppendUTF16toASCII(*gCodepageStr, propKey); 
    return NS_SUCCEEDED(gFontNameMapProperties->
                        GetStringProperty(propKey, aWinName));
  }

  
  if (NS_SUCCEEDED(gFontNameMapProperties->
                   GetStringProperty(propKey, aWinName))) {
    if (StringEndsWith(aWinName, *gCodepageStr)) {
      
      return PR_FALSE;
    }
    
    aWinName.Truncate(aWinName.Length() - gCodepageStr->Length());
    return PR_TRUE;
  }
  return PR_FALSE;  
}
