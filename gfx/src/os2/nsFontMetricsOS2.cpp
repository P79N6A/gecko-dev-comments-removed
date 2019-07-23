




































#include "nsGfxDefs.h"
#include "nsIPref.h"
#include "nsIServiceManager.h"
#include "nsILanguageAtomService.h"
#include "nsICharsetConverterManager.h"
#include "nsLocaleCID.h"
#include "nsILocaleService.h"
#include "nsICharRepresentable.h"
#include "nsISaveAsCharset.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsFontMetricsOS2.h"
#include "nsQuickSort.h"
#include "nsTextFormatter.h"
#include "prmem.h"
#include "plhash.h"
#include "prprf.h"
#include "nsReadableUtils.h"
#include "nsUnicodeRange.h"
#include "nsUnicharUtils.h"

#include "nsOS2Uni.h"
#include <math.h>

#ifdef MOZ_MATHML
  #include <math.h>
#endif

#undef USER_DEFINED
#define USER_DEFINED "x-user-def"

#define NS_REPLACEMENT_CHAR  PRUnichar(0x003F) // question mark


#define NS_MAX_FONT_WEIGHT 900
#define NS_MIN_FONT_WEIGHT 100

#undef CHAR_BUFFER_SIZE
#define CHAR_BUFFER_SIZE 1024



nsTHashtable<GlobalFontEntry>* nsFontMetricsOS2::gGlobalFonts = nsnull;
PRBool        nsFontMetricsOS2::gSubstituteVectorFonts = PR_TRUE;
PLHashTable  *nsFontMetricsOS2::gFamilyNames = nsnull;

static nsIPref           *gPref = nsnull;
static nsIAtom           *gUsersLocale = nsnull;
static nsIAtom           *gSystemLocale = nsnull;
static nsIAtom           *gUserDefined = nsnull;
static int                gInitialized = 0;
static ULONG              gSystemCodePage = 0;
static ULONG              gCurrHashValue = 1;

#ifdef USE_FREETYPE
PRBool nsFontMetricsOS2::gUseFTFunctions = PR_FALSE;
static nsISaveAsCharset* gFontSubstituteConverter = nsnull;

static nsIAtom* gJA = nsnull;
static nsIAtom* gKO = nsnull;
static nsIAtom* gZHTW = nsnull;
static nsIAtom* gZHCN = nsnull;
static nsIAtom* gZHHK = nsnull;
#endif 


enum nsCharset
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
};

static nsCharsetInfo gCharsetInfo[eCharset_COUNT] =
{
  { "DEFAULT",       0,    "" },
  { "ANSI",          1252, "x-western" },
  { "EASTEUROPE",    1250, "x-central-euro" },
  { "RUSSIAN",       1251, "x-cyrillic" },
  { "GREEK",         813,  "el" },
  { "TURKISH",       1254, "tr" },
  { "HEBREW",        1208,  "he" },
  { "ARABIC",        864,  "ar" },
  { "BALTIC",        1257, "x-baltic" },
  { "THAI",          874,  "th" },
  { "SHIFTJIS",      932,  "ja" },
  { "GB2312",        1381, "zh-CN" },
  { "HANGEUL",       949,  "ko" },
  { "CHINESEBIG5",   950,  "zh-TW" },
  { "JOHAB",         1361, "ko-XXX", }
};




    

static void
FreeGlobals(void)
{
  gInitialized = 0;

  NS_IF_RELEASE(gPref);
  NS_IF_RELEASE(gUsersLocale);
  NS_IF_RELEASE(gSystemLocale);
  NS_IF_RELEASE(gUserDefined);
#ifdef USE_FREETYPE
  if (nsFontMetricsOS2::gUseFTFunctions) {
    
    nsFontMetricsOS2FT::pfnFt2EnableFontEngine(FALSE);

    NS_IF_RELEASE(gJA);
    NS_IF_RELEASE(gKO);
    NS_IF_RELEASE(gZHTW);
    NS_IF_RELEASE(gZHCN);
    NS_IF_RELEASE(gZHHK);
  }
#endif 

  if (nsFontMetricsOS2::gGlobalFonts) {
    nsFontMetricsOS2::gGlobalFonts->Clear();
    delete nsFontMetricsOS2::gGlobalFonts;
    nsFontMetricsOS2::gGlobalFonts = nsnull;
  }

  if (nsFontMetricsOS2::gFamilyNames) {
    PL_HashTableDestroy(nsFontMetricsOS2::gFamilyNames);
    nsFontMetricsOS2::gFamilyNames = nsnull;
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

static nsresult
InitGlobals(void)
{
  CallGetService(NS_PREF_CONTRACTID, &gPref);
  if (!gPref) {
    FreeGlobals();
    return NS_ERROR_FAILURE;
  }

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

  UINT cp = ::WinQueryCp(HMQ_CURRENT);
  for (int i = 1; i < eCharset_COUNT; ++i) {
    if (gCharsetInfo[i].mCodePage == cp) {
      gSystemLocale = NS_NewAtom(gCharsetInfo[i].mLangGroup);
      break;
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
#ifdef USE_FREETYPE
  if (nsFontMetricsOS2::gUseFTFunctions) {
    
    nsFontMetricsOS2FT::pfnFt2EnableFontEngine(TRUE);

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
  }
#endif 

  
  
  
  
  ULONG numCP = ::WinQueryCpList((HAB)0, 0, NULL);
  if (numCP > 0) {
     ULONG * pCPList = (ULONG*)nsMemory::Alloc(numCP*sizeof(ULONG));
     if (::WinQueryCpList((HAB)0, numCP, pCPList)) {
        for (PRUint32 i = 0; i < numCP; i++) {
           if (pCPList[i] == 1386) {
              gCharsetInfo[11].mCodePage = 1386;
           } else if (pCPList[i] == 943) {
              gCharsetInfo[10].mCodePage = 943;
           }
        }
     }
     nsMemory::Free(pCPList);
  }

  gSystemCodePage = ::WinQueryCp(HMQ_CURRENT);

  nsresult res;

  if (!nsFontMetricsOS2::gGlobalFonts) {
    res = nsFontMetricsOS2::InitializeGlobalFonts();
    if (NS_FAILED(res)) {
      FreeGlobals();
      return res;
    }
  }

  res = gPref->GetBoolPref("browser.display.substitute_vector_fonts",
                                    &nsFontMetricsOS2::gSubstituteVectorFonts);
  NS_ASSERTION( NS_SUCCEEDED(res), "Could not get pref 'browser.display.substitute_vector_fonts'" );

  
  gFontCleanupObserver = new nsFontCleanupObserver();
  NS_ASSERTION(gFontCleanupObserver, "failed to create observer");
  if (gFontCleanupObserver) {
    
    nsresult rv;
    nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1", &rv));
    if (NS_SUCCEEDED(rv)) {
      rv = observerService->AddObserver(gFontCleanupObserver, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);
    }    
  }
  gInitialized = 1;

  return NS_OK;
}

#ifdef DEBUG_FONT_STRUCT_ALLOCS
unsigned long nsFontMetricsOS2::mRefCount = 0;
#endif

nsFontMetricsOS2::nsFontMetricsOS2()
{
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount++;
  printf("+++ nsFontMetricsOS2 total = %d\n", mRefCount);
#endif
}

nsFontMetricsOS2::~nsFontMetricsOS2()
{
  mFontHandle = nsnull; 
  mUnicodeFont = nsnull; 
  mWesternFont = nsnull; 

  for (PRInt32 i = mLoadedFonts.Count()-1; i >= 0; --i) {
    delete (nsFontOS2*)mLoadedFonts[i];
  }
  mLoadedFonts.Clear();

  if (mDeviceContext) {
    
    mDeviceContext->FontMetricsDeleted(this);
    mDeviceContext = nsnull;
  }
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount--;
  printf("--- nsFontMetricsOS2 total = %d\n", mRefCount);
#endif
}

NS_IMPL_ISUPPORTS1(nsFontMetricsOS2, nsIFontMetrics)

NS_IMETHODIMP
nsFontMetricsOS2::Init( const nsFont &aFont,  nsIAtom* aLangGroup,
                        nsIDeviceContext *aContext)
{
  nsresult res;
  if (!gInitialized) {
    res = InitGlobals();
    if (NS_FAILED(res)) {
      return res;
    }
  }

  mFont = aFont;
  mLangGroup = aLangGroup;

  
  mDeviceContext = (nsDeviceContextOS2 *) aContext;
  return RealizeFont();
}

NS_IMETHODIMP
nsFontMetricsOS2::Destroy()
{
  mDeviceContext = nsnull;
  return NS_OK;
}


static FONTMETRICS* getMetrics( long &lFonts, PCSZ facename, HPS hps)
{
   LONG lWant = 0;
   lFonts = GFX (::GpiQueryFonts(hps, QF_PUBLIC | QF_PRIVATE,
                                 facename, &lWant, 0, 0),
                 GPI_ALTERROR);
   if (!lFonts) {
      return NULL;
   }
   FONTMETRICS* pMetrics = (FONTMETRICS*)nsMemory::Alloc(lFonts * sizeof(FONTMETRICS));

   GFX (::GpiQueryFonts(hps, QF_PUBLIC | QF_PRIVATE, facename,
                        &lFonts, sizeof (FONTMETRICS), pMetrics),
        GPI_ALTERROR);

   return pMetrics;
}





nsFontOS2*
nsFontMetricsOS2::SetFontHandle(HPS aPS, GlobalFontEntry* aEntry,
                                nsMiniMetrics* aMetrics, PRBool aDoFakeEffects)
{
  nsFontOS2* font;
#ifdef USE_FREETYPE
  if (gUseFTFunctions) {
    font = new nsFontOS2FT();
  } else
#endif
  {
    font = new nsFontOS2();
  }

  strcpy(font->mFattrs.szFacename, aMetrics->szFacename);
  if (aMetrics->fsDefn & FM_DEFN_OUTLINE ||
      !mDeviceContext->SupportsRasterFonts()) {
    font->mFattrs.fsFontUse = FATTR_FONTUSE_OUTLINE |
                               FATTR_FONTUSE_TRANSFORMABLE;
  }
  if (aMetrics->fsType & FM_TYPE_MBCS)
    font->mFattrs.fsType |= FATTR_TYPE_MBCS;
  if (aMetrics->fsType & FM_TYPE_DBCS)
    font->mFattrs.fsType |= FATTR_TYPE_DBCS;

  if (aDoFakeEffects) {
    
    if (mFont.weight > NS_FONT_WEIGHT_NORMAL)
      font->mFattrs.fsSelection |= FATTR_SEL_BOLD;
    if (mFont.style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE))
      font->mFattrs.fsSelection |= FATTR_SEL_ITALIC;
  }

#ifdef USE_FREETYPE
  
  
  if (!gUseFTFunctions || aEntry->mCodePage == 65400)
#endif
    font->mFattrs.usCodePage = aEntry->mCodePage;

#ifdef PERF_HASGLYPH_CHAR_MAP
  if (gUseFTFunctions) {
    if (aEntry->mHaveCheckedCharMap == nsnull) {
      aEntry->mHaveCheckedCharMap = (PRUint32*)calloc(UCS2_MAP_LEN, sizeof(PRUint32));
      aEntry->mRepresentableCharMap = (PRUint32*)calloc(UCS2_MAP_LEN, sizeof(PRUint32));
    }
    font->mHaveCheckedCharMap = aEntry->mHaveCheckedCharMap;
    font->mRepresentableCharMap = aEntry->mRepresentableCharMap;
  }
#endif

  float app2dev, reqEmHeight;
  app2dev = mDeviceContext->AppUnitsToDevUnits();
  reqEmHeight = mFont.size * app2dev;

  FATTRS* fattrs = &(font->mFattrs);
  if (fattrs->fsFontUse == 0)  
  {
    long lFonts = 0;
    FONTMETRICS* pMetrics = getMetrics( lFonts, fattrs->szFacename, aPS);

    int reqPointSize = NSTwipsToIntPoints(mFont.size);
    int browserRes = mDeviceContext->GetDPI();

    int minSize = 99, maxSize = 0, curEmHeight = 0;
    for (int i = 0; i < lFonts; i++)
    {
      
      
      
      if (pMetrics[i].sYDeviceRes == browserRes &&
          pMetrics[i].sNominalPointSize / 10 == reqPointSize)
      {
        
        curEmHeight = pMetrics[i].lEmHeight;
        fattrs->lMaxBaselineExt = pMetrics[i].lMaxBaselineExt;
        fattrs->lAveCharWidth = pMetrics[i].lAveCharWidth;
        minSize = maxSize = pMetrics[i].lEmHeight;
        break;
      }
      else
      {
        if (fabs(pMetrics[i].lEmHeight - reqEmHeight) < 
            fabs(curEmHeight - reqEmHeight))
        {
          curEmHeight = pMetrics[i].lEmHeight;
          fattrs->lMaxBaselineExt = pMetrics[i].lMaxBaselineExt;
          fattrs->lAveCharWidth = pMetrics[i].lAveCharWidth;
        }
        else if (fabs(pMetrics[i].lEmHeight - reqEmHeight) == 
                 fabs(curEmHeight - reqEmHeight))
        {
          if ((pMetrics[i].lEmHeight) > curEmHeight)
          {
            curEmHeight = pMetrics[i].lEmHeight;
            fattrs->lMaxBaselineExt = pMetrics[i].lMaxBaselineExt;
            fattrs->lAveCharWidth = pMetrics[i].lAveCharWidth;
          }
        }
      }
      
      
      if (pMetrics[i].lEmHeight > maxSize)
        maxSize = pMetrics[i].lEmHeight;
      if (pMetrics[i].lEmHeight < minSize)
        minSize = pMetrics[i].lEmHeight;
    }

    nsMemory::Free(pMetrics);
    
    
    
    if (reqEmHeight < minSize - 3 ||
        reqEmHeight > maxSize + 3)
    {
      
      
      
      
      nsAutoString alias;
      if (gSubstituteVectorFonts &&
          GetVectorSubstitute(aPS, aEntry->GetKey(), alias))
      {
        strcpy(fattrs->szFacename, NS_LossyConvertUTF16toASCII(alias).get());
        fattrs->fsFontUse = FATTR_FONTUSE_OUTLINE | FATTR_FONTUSE_TRANSFORMABLE;
        fattrs->fsSelection &= ~(FM_SEL_BOLD | FM_SEL_ITALIC);
      }
    }
    
    if (fattrs->fsFontUse == 0) {    
      reqEmHeight = curEmHeight;
    }
  }

   
  if (mFont.decorations & NS_FONT_DECORATION_UNDERLINE) {
    fattrs->fsSelection |= FATTR_SEL_UNDERSCORE;
  }
  if (mFont.decorations & NS_FONT_DECORATION_LINE_THROUGH) {
    fattrs->fsSelection |= FATTR_SEL_STRIKEOUT;
  }

#ifdef USE_FREETYPE
  if (!gUseFTFunctions)
#endif
  {
     
     
     
    const char* langGroup;
    mLangGroup->GetUTF8String(&langGroup);
    for (int j=0; j < eCharset_COUNT; j++)
    {
      if (langGroup[0] == gCharsetInfo[j].mLangGroup[0])
      {
        if (!strcmp(langGroup, gCharsetInfo[j].mLangGroup))
        {
          mConvertCodePage = gCharsetInfo[j].mCodePage;
          break;
        }
      }
    }
  
    
    
    
    if (fattrs->usCodePage != 65400) {
      fattrs->usCodePage = mConvertCodePage;
    }
  }

  
  
  long lFloor = NSToIntFloor(reqEmHeight); 
  font->mCharbox.cx = MAKEFIXED(lFloor, (reqEmHeight - (float)lFloor) * 65536.0f);
  font->mCharbox.cy = font->mCharbox.cx;

  return font;
}




nsFontOS2*
nsFontMetricsOS2::LoadFont(HPS aPS, const nsAString& aFontname)
{
  nsFontOS2* font = nsnull;

   
  PRBool bBold = mFont.weight > NS_FONT_WEIGHT_NORMAL;
  PRBool bItalic = (mFont.style & (NS_FONT_STYLE_ITALIC | NS_FONT_STYLE_OBLIQUE));
  USHORT flags = bBold ? FM_SEL_BOLD : 0;
  flags |= bItalic ? FM_SEL_ITALIC : 0;

  
  nsAutoString fontptr;
  if (mDeviceContext->SupportsRasterFonts() ||
      !GetVectorSubstitute(aPS, aFontname, fontptr))
  {
    fontptr = aFontname;
  }

  GlobalFontEntry* globalEntry = gGlobalFonts->GetEntry(fontptr);
  if (globalEntry) {
    nsMiniMetrics* metrics = globalEntry->mMetrics;
    nsMiniMetrics* plainFont = nsnull;
    while (metrics) {
      if ((metrics->fsSelection & (FM_SEL_ITALIC | FM_SEL_BOLD)) == flags) {
        font = SetFontHandle(aPS, globalEntry, metrics, PR_FALSE);
        break;
      }

      
      if (!plainFont && !(metrics->fsSelection & (FM_SEL_ITALIC | FM_SEL_BOLD)))
        plainFont = metrics;

      metrics = metrics->mNext;
    }

    
    
    
    
    if (!font && plainFont) {
      font = SetFontHandle(aPS, globalEntry, plainFont, PR_TRUE);
    }
  }

  if (!font) {
    
    
    long lFonts = 0;
    nsAutoCharBuffer facename;
    PRInt32 len;
    WideCharToMultiByte(0, PromiseFlatString(aFontname).get(),
                        aFontname.Length(), facename, len);
    FONTMETRICS* pMetrics = getMetrics(lFonts, facename.get(), aPS);

    if (lFonts > 0) {
      nsAutoChar16Buffer familyname;
      MultiByteToWideChar(0, pMetrics[0].szFamilyname,
                          strlen(pMetrics[0].szFamilyname), familyname, len);
      nsAutoString name(familyname.get());
      GlobalFontEntry* globalEntry = gGlobalFonts->GetEntry(name);
      if (globalEntry) {
        
        nsMiniMetrics* metrics = globalEntry->mMetrics;
        while (metrics) {
          if (stricmp(metrics->szFacename, facename.get()) == 0) {
            font = SetFontHandle(aPS, globalEntry, metrics, PR_TRUE);
            break;
          }
          metrics = metrics->mNext;
        }
      }
    }
    nsMemory::Free(pMetrics);
  }

  if (font) {
    mLoadedFonts.AppendElement(font);
  }

  return font;
}

typedef struct nsFontFamilyName
{
  char* mName;
  char* mWinName;
} nsFontFamilyName;

static nsFontFamilyName gBadDBCSFontMapping[] =
{
  { "\xB7\xC2\xCB\xCE\xB3\xA3\xB9\xE6", "IBM Fang Song SC"},
  { "\xBA\xDA\xCC\xE5\xB3\xA3\xB9\xE6", "IBM Hei SC"},
  { "\xBF\xAC\xCC\xE5\xB3\xA3\xB9\xE6", "IBM Kai SC"},
  { "\x5B\x8B\x4F\x53\x5E\x38\x89\xC4", "IBM Song SC"},
  { "\x91\x76\x91\xCC\x8F\xED",         "IBM Song SC"},

  { "@\xB7\xC2\xCB\xCE\xB3\xA3\xB9\xE6", "@IBM Fang Song SC"},
  { "@\xBA\xDA\xCC\xE5\xB3\xA3\xB9\xE6", "@IBM Hei SC"},
  { "@\xBF\xAC\xCC\xE5\xB3\xA3\xB9\xE6", "@IBM Kai SC"},
  { "@\x5B\x8B\x4F\x53\x5E\x38\x89\xC4", "@IBM Song SC"},
  { "@\x91\x76\x91\xCC\x8F\xED",         "@IBM Song SC"},

  { "\xAC\xD1\xFA\x80\xFA\xBD\x8F\x82",  "MOEKai TC"},
  { "\xBC\xD0\xB7\xC7\xB7\xA2\xC5\xE9",  "MOEKai TC"},
  { "\xAC\xD1\xFA\x80\x15\xA7\x8F\x82",  "MOESung TC"},
  { "\xBC\xD0\xB7\xC7\xA7\xBA\xC5\xE9",  "MOESung TC"},
  { "\xCF\xCF\x14\xB6\x8F\x82",          "Hei TC"},
  { "\xA4\xA4\xB6\xC2\xC5\xE9",          "Hei TC"},

  { "@\xAC\xD1\xFA\x80\xFA\xBD\x8F\x82",  "@MOEKai TC"},
  { "@\xBC\xD0\xB7\xC7\xB7\xA2\xC5\xE9",  "@MOEKai TC"},
  { "@\xAC\xD1\xFA\x80\x15\xA7\x8F\x82",  "@MOESong TC"},
  { "@\xBC\xD0\xB7\xC7\xA7\xBA\xC5\xE9",  "@MOESong TC"},
  { "@\xCF\xCF\x14\xB6\x8F\x82",          "@Hei TC"},
  { "@\xA4\xA4\xB6\xC2\xC5\xE9",          "@Hei TC"},
  { nsnull, nsnull }
};

#ifdef DEBUG
PR_STATIC_CALLBACK(PLDHashOperator)
DebugOutputEnumFunc(GlobalFontEntry* aEntry, void* aData)
{
  printf("---------------------------------------------------------------------\n");
  printf(" [[]] %s\n", NS_LossyConvertUTF16toASCII(aEntry->GetKey()).get());
  nsMiniMetrics* metrics = aEntry->mMetrics;
  while (metrics) {
    printf("  %32s", metrics->szFacename);

    if (metrics->fsDefn & FM_DEFN_OUTLINE)
      printf(" : vector");
    else
      printf(" : bitmap");
      
    if (metrics->fsSelection & FM_SEL_BOLD)
      printf(" : bold");
    else
      printf(" :     ");
      
    if (metrics->fsSelection & FM_SEL_ITALIC)
      printf(" : italic");
    else
      printf(" :       ");

    if (metrics->fsType & FM_TYPE_DBCS ||
        metrics->fsType & FM_TYPE_MBCS)
      printf(" : M/DBCS");
    else
      printf(" :       ");

    printf(" : cp=%5d\n", aEntry->mCodePage );
    
    metrics = metrics->mNext;
  }

  return PL_DHASH_NEXT;
}
#endif

nsresult
nsFontMetricsOS2::InitializeGlobalFonts()
{
  gGlobalFonts = new nsTHashtable<GlobalFontEntry>;
  if (!gGlobalFonts->Init(64))
    return NS_ERROR_OUT_OF_MEMORY;

  HPS ps = ::WinGetScreenPS(HWND_DESKTOP);
  LONG lRemFonts = 0, lNumFonts;
  lNumFonts = GFX (::GpiQueryFonts(ps, QF_PUBLIC, NULL, &lRemFonts, 0, 0),
                   GPI_ALTERROR);
  FONTMETRICS* pFontMetrics = (PFONTMETRICS) nsMemory::Alloc(lNumFonts * sizeof(FONTMETRICS));
  lRemFonts = GFX (::GpiQueryFonts(ps, QF_PUBLIC, NULL, &lNumFonts,
                                   sizeof (FONTMETRICS), pFontMetrics),
                   GPI_ALTERROR);
  ::WinReleasePS(ps);

  for (int i = 0; i < lNumFonts; i++) {
    FONTMETRICS* font = &(pFontMetrics[i]);

    
    
    if (strcmp(font->szFamilyname, "Courier") == 0 &&
        !(font->fsDefn & FM_DEFN_OUTLINE)) {
      continue;
    }

    
    
    
    if (strcmp(pFontMetrics[i].szFamilyname, "Roman") == 0 ||
        strcmp(pFontMetrics[i].szFamilyname, "Swiss") == 0) {
      continue;
    }

     
     
     
     
     
     
     
     
     
     
     
     
    char* f;
    if (PL_strcasestr(font->szFacename, "bold") != nsnull ||
        PL_strcasestr(font->szFacename, "italic") != nsnull ||
        PL_strcasestr(font->szFacename, "oblique") != nsnull ||
        PL_strcasestr(font->szFacename, "regular") != nsnull ||
        PL_strcasestr(font->szFacename, "-normal") != nsnull)
    {
      f = NS_STATIC_CAST(char*, font->szFamilyname);
    } else {
      f = NS_STATIC_CAST(char*, font->szFacename);
    }
    nsAutoChar16Buffer fontname;
    PRInt32 len;
    MultiByteToWideChar(0, f, strlen(f), fontname, len);
    nsAutoString fontptr(fontname.get());

    
    
    if (font->fsType & FM_TYPE_DBCS)
    {
      if ((gSystemCodePage != 1386) &&
          (gSystemCodePage != 1381) &&
          (gSystemCodePage != 950))
      {
        for (int i = 0; gBadDBCSFontMapping[i].mName != nsnull; i++) {
          if (strcmp(f, gBadDBCSFontMapping[i].mName) == 0)
          {
            CopyASCIItoUTF16(nsDependentCString(gBadDBCSFontMapping[i].mWinName),
                            fontptr);
            break;
          }
        }
      }
    }

    
    GlobalFontEntry* globalEntry = gGlobalFonts->PutEntry(fontptr);
    if (!globalEntry)
      return NS_ERROR_OUT_OF_MEMORY;

    
    nsMiniMetrics* metrics = new nsMiniMetrics;
    strcpy(metrics->szFacename, font->szFacename);
    metrics->fsType = font->fsType;
    metrics->fsDefn = font->fsDefn;
    metrics->fsSelection = font->fsSelection;
     
     
    if (font->usWeightClass > 5)
      metrics->fsSelection |= FM_SEL_BOLD;

    
    
    if (strncmp(font->szFamilyname, "Lucida", 6) == 0 &&
        PL_strcasestr(font->szFacename, "bold") != nsnull) {
      metrics->fsSelection |= FM_SEL_BOLD;
    }

    
    metrics->mNext = globalEntry->mMetrics;
    globalEntry->mMetrics = metrics;
    globalEntry->mCodePage = font->usCodePage;
  }

#ifdef DEBUG_pedemonte
  gGlobalFonts->EnumerateEntries(DebugOutputEnumFunc, nsnull);
  fflush(stdout);
#endif

  nsMemory::Free(pFontMetrics);

  return NS_OK;
}

nsFontOS2*
nsFontMetricsOS2::FindGlobalFont(HPS aPS, PRUint32 aChar)
{
  nsFontOS2* fh = nsnull;
  nsAutoString fontname;
  if (!IsDBCS())
    fontname.AssignLiteral("Helv");
  else
    fontname.AssignLiteral("Helv Combined");
  fh = LoadFont(aPS, fontname);
  NS_ASSERTION(fh, "Couldn't load default font - BAD things are happening");
  return fh;
}

nsFontOS2*
nsFontMetricsOS2::FindUserDefinedFont(HPS aPS, PRUint32 aChar)
{
  if (mIsUserDefined) {
    
    nsFontOS2* font = LoadFont(aPS, mUserDefined);
    mIsUserDefined = PR_FALSE;
    if (font && font->HasGlyph(aPS, aChar)) {
      return font;
    }
  }
  return nsnull;
}

static nsFontFamilyName gFamilyNameTable[] =
{
#ifdef MOZ_MATHML
  { "-moz-math-text",   "Times New Roman" },
  { "-moz-math-symbol", "Symbol" },
#endif
  { "times",           "Times New Roman" },
  { "times roman",     "Times New Roman" },

  { nsnull, nsnull }
};

static nsFontFamilyName gFamilyNameTableDBCS[] =
{
#ifdef MOZ_MATHML
  { "-moz-math-text",   "Times New Roman" },
  { "-moz-math-symbol", "Symbol" },
#endif
  { "times",           "Times New Roman" },
  { "times roman",     "Times New Roman" },
  { "warpsans",        "WarpSans Combined" },

  { nsnull, nsnull }
};




static PLHashNumber PR_CALLBACK
HashKey(const void* aString)
{
  const nsString* str = (const nsString*)aString;
  return (PLHashNumber) nsCRT::HashCode(str->get());
}

static PRIntn PR_CALLBACK
CompareKeys(const void* aStr1, const void* aStr2)
{
  return nsCRT::strcmp(((const nsString*) aStr1)->get(),
    ((const nsString*) aStr2)->get()) == 0;
}

PR_STATIC_CALLBACK(void*) familyname_AllocTable(void *pool, size_t size)
{
  return nsMemory::Alloc(size);
}

PR_STATIC_CALLBACK(void) familyname_FreeTable(void *pool, void *item)
{
  nsMemory::Free(item);
}

PR_STATIC_CALLBACK(PLHashEntry*) familyname_AllocEntry(void *pool, const void *key)
{
  return PR_NEW(PLHashEntry);
}

PR_STATIC_CALLBACK(void) familyname_FreeEntry(void *pool, PLHashEntry *he, PRUint32 flag)
{
  delete (nsString *) (he->value);

  if (flag == HT_FREE_ENTRY)  {
    delete (nsString *) (he->key);
    nsMemory::Free(he);
  }
}

PLHashAllocOps familyname_HashAllocOps = {
  familyname_AllocTable, familyname_FreeTable,
  familyname_AllocEntry, familyname_FreeEntry
};

PLHashTable*
nsFontMetricsOS2::InitializeFamilyNames(void)
{
  if (!gFamilyNames) {
    gFamilyNames = PL_NewHashTable( 0, HashKey, CompareKeys, nsnull,
                                    &familyname_HashAllocOps, nsnull );
    if (!gFamilyNames) {
      return nsnull;
    }

    nsFontFamilyName* f;
    if (!IsDBCS()) {
      f = gFamilyNameTable;
    } else {
      f = gFamilyNameTableDBCS;
    }

    while (f->mName) {
      nsString* name = new nsString;
      nsString* winName = new nsString;
      if (name && winName) {
        name->AssignWithConversion(f->mName);
        winName->AssignWithConversion(f->mWinName);
        if (PL_HashTableAdd(gFamilyNames, name, (void*) winName)) { 
          ++f;
          continue;
        }
      }
      
      if (name) delete name;
      if (winName) delete winName;
      return nsnull;
    }
  }

  return gFamilyNames;
}

nsFontOS2*
nsFontMetricsOS2::FindLocalFont(HPS aPS, PRUint32 aChar)
{
  if (!gFamilyNames) {
    if (!InitializeFamilyNames()) {
      return nsnull;
    }
  }
  while (mFontsIndex < mFonts.Count()) {
    if (mFontsIndex == mGenericIndex) {
      return nsnull;
    }

    nsString* name = mFonts.StringAt(mFontsIndex++);
    nsAutoString low(*name);
    ToLowerCase(low);
    nsString* winName = (nsString*) PL_HashTableLookup(gFamilyNames, &low);
    if (!winName) {
      winName = name;
    }
#ifdef DEBUG_FONT_SELECTION
    printf(" FindLocalFont(): attempting to load %s\n",
           NS_LossyConvertUTF16toASCII(*winName).get());
#endif
    nsFontOS2* font = LoadFont(aPS, *winName);
    if (font && font->HasGlyph(aPS, aChar)) {
      return font;
    }
  }
  return nsnull;
}

nsFontOS2*
nsFontMetricsOS2::LoadGenericFont(HPS aPS, PRUint32 aChar, const nsAString& aName)
{
  for (int i = mLoadedFonts.Count()-1; i >= 0; --i) {
    
    const nsACString& fontName =
      nsDependentCString(((nsFontOS2*)mLoadedFonts[i])->mFattrs.szFacename);
    if (aName.Equals(NS_ConvertASCIItoUTF16(fontName),
                     nsCaseInsensitiveStringComparator()))
      return nsnull;

  }
#ifdef DEBUG_FONT_SELECTION
  printf(" LoadGenericFont(): attempting to load %s\n",
         NS_LossyConvertUTF16toASCII(aName).get());
#endif
  nsFontOS2* font = LoadFont(aPS, aName);
  if (font && font->HasGlyph(aPS, aChar)) {
    return font;
  }
  return nsnull;
}

struct GenericFontEnumContext
{
  HPS               mPS;        
  PRUint32          mChar;      
  nsFontOS2        *mFont;      
  nsFontMetricsOS2 *mMetrics;   
};

static PRBool
GenericFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void* aData)
{
  GenericFontEnumContext* context = (GenericFontEnumContext*)aData;
  HPS ps = context->mPS;
  PRUint32 ch = context->mChar;
  nsFontMetricsOS2* metrics = context->mMetrics;
  context->mFont = metrics->LoadGenericFont(ps, ch, aFamily);
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

nsFontOS2*
nsFontMetricsOS2::FindGenericFont(HPS aPS, PRUint32 aChar)
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

  
  GenericFontEnumContext context = {aPS, aChar, nsnull, this};
  font.EnumerateFamilies(GenericFontEnumCallback, &context);
  if (context.mFont) { 
    return context.mFont;
  }

  mTriedAllGenerics = 1;
  return nsnull;
}

nsFontOS2*
nsFontMetricsOS2::FindPrefFont(HPS aPS, PRUint32 aChar)
{
  if (mTriedAllPref) {
    
    return nsnull;
  }
  nsFont font("", 0, 0, 0, 0, 0);
  
  
  
  if (gUsersLocale != mLangGroup) {
    nsAutoString langGroup;
    gUsersLocale->ToString(langGroup);
    AppendGenericFontFromPref(font.name, 
                              NS_ConvertUTF16toUTF8(langGroup).get(), 
                              NS_ConvertUTF16toUTF8(mGeneric).get());
  }
  
  
  
  
  if ((gSystemLocale != mLangGroup) && (gSystemLocale != gUsersLocale)) {
    nsAutoString langGroup;
    gSystemLocale->ToString(langGroup);
    AppendGenericFontFromPref(font.name, 
                              NS_ConvertUTF16toUTF8(langGroup).get(), 
                              NS_ConvertUTF16toUTF8(mGeneric).get());
  }

  
  for (int i = 1; i < eCharset_COUNT; ++i) {
    nsIAtom* langGroup = NS_NewAtom(gCharsetInfo[i].mLangGroup); 
    if((gUsersLocale != langGroup) && (gSystemLocale != langGroup)) {
      AppendGenericFontFromPref(font.name, gCharsetInfo[i].mLangGroup, 
                                NS_ConvertUTF16toUTF8(mGeneric).get());
    }
    NS_IF_RELEASE(langGroup);
  }
  GenericFontEnumContext context = {aPS, aChar, nsnull, this};
  font.EnumerateFamilies(GenericFontEnumCallback, &context);
  if (context.mFont) { 
    return context.mFont;
  }
  mTriedAllPref = 1;
  return nsnull;
}


nsFontOS2*
nsFontMetricsOS2::FindFont(HPS aPS, PRUint32 aChar)
{
  nsFontOS2* font = FindUserDefinedFont(aPS, aChar);
  if (!font) {
    font = FindLocalFont(aPS, aChar);
    if (!font) {
      font = FindGenericFont(aPS, aChar);
      if (!font) {
        font = FindPrefFont(aPS, aChar);
        if (!font) {
          font = FindGlobalFont(aPS, aChar);
#ifdef USE_FREETYPE
          if (!font) {
            font = FindSubstituteFont(aPS, aChar);
          }
#endif
        }
      }
    }
  }
#ifdef DEBUG_FONT_SELECTION
  if (font) {
    printf(" FindFont(): found font %s for char 0x%04x\n",
           font->mFattrs.szFacename, aChar);
  }
#endif
  return font;
}

static PRBool
FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
  nsFontMetricsOS2* metrics = (nsFontMetricsOS2*) aData;

#ifdef USE_FREETYPE
  if (!nsFontMetricsOS2::gUseFTFunctions)
#endif
  {
    
    if (aFamily.Find("Arial", IGNORE_CASE) != -1) {
       if (metrics->mConvertCodePage != 1252) {
          if ((metrics->mConvertCodePage == 0) &&
              (gSystemCodePage != 850) &&
              (gSystemCodePage != 437)) {
             return PR_TRUE; 
          }
       }
    }
  }

  metrics->mFonts.AppendString(aFamily);
  if (aGeneric) {
    metrics->mGeneric.Assign(aFamily);
    ToLowerCase(metrics->mGeneric);
    return PR_FALSE; 
  }
  ++metrics->mGenericIndex;

  return PR_TRUE; 
}

PRBool
nsFontMetricsOS2::GetVectorSubstitute(HPS aPS, const nsAString& aFamilyname,
                                      nsAString& aAlias)
{
  if (aFamilyname.EqualsLiteral("Tms Rmn")) {
    aAlias.AssignLiteral("Times New Roman");
  } else if (aFamilyname.EqualsLiteral("Helv")) {
    aAlias.AssignLiteral("Helvetica");
  }

  
  if (!mDeviceContext->SupportsRasterFonts()) {
    if (aFamilyname.EqualsLiteral("System Proportional") ||
        aFamilyname.EqualsLiteral("WarpSans"))
    {
      aAlias.AssignLiteral("Helvetica");
    } else if (aFamilyname.EqualsLiteral("System Monospaced") ||
               aFamilyname.EqualsLiteral("System VIO"))
    {
      aAlias.AssignLiteral("Courier");
    }
  }

  if (aAlias.IsEmpty())
    return PR_FALSE;
  else
    return PR_TRUE;
}

nsresult
nsFontMetricsOS2::RealizeFont()
{
  nsresult  rv;
  HPS       ps = NULL;

  if (mDeviceContext->mPrintDC){
    ps = mDeviceContext->mPrintPS;
  } else {
    HWND win = (HWND)mDeviceContext->mWidget;
    ps = ::WinGetPS(win);
    if (!ps) {
      ps = ::WinGetPS(HWND_DESKTOP);
    }
  }

  mFont.EnumerateFamilies(FontEnumCallback, this);

  nsCAutoString pref;
  nsXPIDLString value;

  
  if (mGeneric.IsEmpty()) {
    const char* langGroup = nsnull;
    mLangGroup->GetUTF8String(&langGroup);
    pref.Assign("font.default.");
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
    
    
    pref.Assign("font.name.");
    pref.AppendWithConversion(mGeneric);
    pref.Append(".x-user-def");
    rv = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));
    if (NS_SUCCEEDED(rv)) {
      mUserDefined.Assign(value);
      mIsUserDefined = 1;
    }
  }

  nsFontOS2* font = FindFont(ps, 'a');
  NS_ASSERTION(font, "FindFont() returned null.  THIS IS BAD!");
  if (!font) {
    if (mDeviceContext->mPrintDC == NULL) {
      ::WinReleasePS(ps);
    }
    return NS_ERROR_FAILURE;
  }

  font->mConvertCodePage = mConvertCodePage;

   
  mFontHandle = font;
  CHK_SUCCESS (::GpiCreateLogFont(ps, 0, 1, &(font->mFattrs)), FONT_MATCH);
  font->SelectIntoPS( ps, 1 );

  FONTMETRICS fm;
  GFX (::GpiQueryFontMetrics(ps, sizeof(fm), &fm), FALSE);
  
  fm.lInternalLeading = (signed short)fm.lInternalLeading;

  float dev2app;
  dev2app = mDeviceContext->DevUnitsToAppUnits();
  
  mMaxAscent  = NSToCoordRound( (fm.lMaxAscender-1) * dev2app );
  mMaxDescent = NSToCoordRound( (fm.lMaxDescender+1) * dev2app );
  mFontHandle->mMaxAscent = mMaxAscent;
  mFontHandle->mMaxDescent = mMaxDescent;

  mInternalLeading = NSToCoordRound( fm.lInternalLeading * dev2app );
  mExternalLeading = NSToCoordRound( fm.lExternalLeading * dev2app );
  
  
  mEmAscent = mMaxAscent - mInternalLeading;
  mEmDescent  = mMaxDescent;

  mMaxHeight  = mMaxAscent + mMaxDescent;
  mEmHeight = mEmAscent + mEmDescent;

  mMaxAdvance = NSToCoordRound( fm.lMaxCharInc * dev2app );
  mXHeight    = NSToCoordRound( fm.lXHeight * dev2app );

  nscoord onePixel = NSToCoordRound(1 * dev2app);

   
  mSuperscriptYOffset = mXHeight;
  mSubscriptYOffset   = NSToCoordRound( mXHeight / 3.0f );

   
   
  mStrikeoutPosition  = NSToCoordRound( mXHeight / 2.0f);
  mStrikeoutSize      = PR_MAX(onePixel, NSToCoordRound(fm.lStrikeoutSize * dev2app));

#if 1
  
  
  float height = fm.lMaxAscender + fm.lMaxDescender;
  mUnderlinePosition = -PR_MAX(onePixel, NSToIntRound(floor(0.1 * height + 0.5) * dev2app));
  mUnderlineSize = PR_MAX(onePixel, NSToIntRound(floor(0.05 * height + 0.5) * dev2app));
#else
  mUnderlinePosition  = -NSToCoordRound( fm.lUnderscorePosition * dev2app );
  mUnderlineSize      = PR_MAX(onePixel, NSToCoordRound(fm.lUnderscoreSize * dev2app));
#endif

  mAveCharWidth       = PR_MAX(1, NSToCoordRound(fm.lAveCharWidth * dev2app));

   
  SIZEL  size;
  GetTextExtentPoint32(ps, " ", 1, &size);
  mSpaceWidth = NSToCoordRound(float(size.cx) * dev2app);

   
  GFX (::GpiSetCharSet (ps, LCID_DEFAULT), FALSE);
  GFX (::GpiDeleteSetId (ps, 1), FALSE);
  if (mDeviceContext->mPrintDC == NULL)
    ::WinReleasePS(ps);

  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetSpaceWidth(nscoord &aSpaceWidth)
{
  aSpaceWidth = mSpaceWidth;
  return NS_OK;
}


NS_IMETHODIMP nsFontMetricsOS2::GetXHeight( nscoord &aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetSuperscriptOffset(nscoord& aResult)
{
  aResult = mSuperscriptYOffset;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetSubscriptOffset(nscoord& aResult)
{
  aResult = mSubscriptYOffset;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mStrikeoutPosition;
  aSize = mStrikeoutSize;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mUnderlinePosition;
  aSize = mUnderlineSize;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetHeight( nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

#ifdef FONT_LEADING_APIS_V2
NS_IMETHODIMP
nsFontMetricsOS2::GetInternalLeading(nscoord &aLeading)
{
  aLeading = mInternalLeading;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetExternalLeading(nscoord &aLeading)
{
  aLeading = mExternalLeading;
  return NS_OK;
}
#else
NS_IMETHODIMP nsFontMetricsOS2::GetLeading( nscoord &aLeading)
{
   aLeading = mInternalLeading;
   return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetNormalLineHeight(nscoord &aHeight)
{
  aHeight = mEmHeight + mInternalLeading;
  return NS_OK;
}
#endif 

NS_IMETHODIMP nsFontMetricsOS2::GetMaxAscent( nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetMaxDescent( nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetMaxAdvance( nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetFontHandle( nsFontHandle &aHandle)
{
  aHandle = mFontHandle;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsOS2::GetLangGroup(nsIAtom** aLangGroup)
{
  NS_ENSURE_ARG_POINTER(aLangGroup);
  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetEmHeight(nscoord &aHeight)
{
  aHeight = mEmHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetEmAscent(nscoord &aAscent)
{
  aAscent = mEmAscent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetEmDescent(nscoord &aDescent)
{
  aDescent = mEmDescent;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetMaxHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsOS2::GetAveCharWidth(nscoord &aAveCharWidth)
{
  aAveCharWidth = mAveCharWidth;
  return NS_OK;
}

nsFontOS2*
nsFontMetricsOS2::LoadUnicodeFont(HPS aPS, const nsAString& aName)
{
#ifdef DEBUG_FONT_SELECTION
  printf(" LoadUnicodeFont(): attempting to load %s\n",
         NS_LossyConvertUTF16toASCII(aName).get());
#endif
  nsFontOS2* font = LoadFont(aPS, aName);
  if (font) {
    return font;
  }
  return nsnull;
}

struct UnicodeFontEnumContext
{
  HPS               mPS;        
  nsFontOS2*        mFont;      
  nsFontMetricsOS2* mMetrics;   
};

static PRBool
UnicodeFontEnumCallback(const nsString& aFamily, PRBool aGeneric, void* aData)
{
  UnicodeFontEnumContext* context = (UnicodeFontEnumContext*)aData;
  HPS ps = context->mPS;
  nsFontMetricsOS2* metrics = context->mMetrics;
  context->mFont = metrics->LoadUnicodeFont(ps, aFamily);
  if (context->mFont) {
    return PR_FALSE; 
  }
  return PR_TRUE; 
}

void
nsFontMetricsOS2::FindUnicodeFont(HPS aPS)
{
  nsresult res;
  nsCAutoString pref, generic;
  nsXPIDLString value;

  generic.Assign(NS_ConvertUTF16toUTF8(mGeneric));

  pref.Assign("font.name.");
  pref.Append(generic);
  pref.Append(".x-unicode");
   
  res = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));
  if (NS_FAILED(res))
    return;

  nsAutoString fontname;
  fontname.Assign(value);

  nsFontOS2* fh = nsnull;
  fh = LoadUnicodeFont(aPS, fontname);
  if (!fh)
  {
     
     
    pref.Assign("font.name-list.");
    pref.Append(generic);
    pref.Append(".x-unicode");

    res = gPref->CopyUnicharPref(pref.get(), getter_Copies(value));
    if (NS_FAILED(res))
      return;

    nsFont font("", 0, 0, 0, 0, 0);
    font.name.Assign(value);

     
    UnicodeFontEnumContext context = {aPS, nsnull, this};
    font.EnumerateFamilies(UnicodeFontEnumCallback, &context);
    fh = context.mFont;
  }

  if (fh) {   
    fh->mFattrs.usCodePage = 1208;
    fh->mConvertCodePage = 1208;
    mUnicodeFont = fh;
#ifdef DEBUG_FONT_SELECTION
    printf(" FindUnicodeFont(): found new Unicode font %s\n",
           mUnicodeFont->mFattrs.szFacename);
#endif
  } else {
    NS_ERROR("Could not find a Unicode font");
  }
  return;
}

void
nsFontMetricsOS2::FindWesternFont()
{
  
  
  nsFontOS2* font = new nsFontOS2();
  font->mFattrs = mFontHandle->mFattrs;
  font->mFattrs.usCodePage = 1252;
  font->mCharbox = mFontHandle->mCharbox;
  font->mMaxAscent = mFontHandle->mMaxAscent;
  font->mMaxDescent = mFontHandle->mMaxDescent;
  font->mConvertCodePage = 1252;
  mLoadedFonts.AppendElement(font);
  mWesternFont = font;
}

#define IS_SPECIAL_WO_ELLIPSE(x) \
                      ((x == 0x20AC) ||  /* euro */  \
                       (x == 0x2022) ||  /* bull */  \
                       (x == 0x201C) ||  /* ldquo */ \
                       (x == 0x201D) ||  /* rdquo */ \
                       (x == 0x2018) ||  /* lsquo */ \
                       (x == 0x2019) ||  /* rsquo */ \
                       (x == 0x2013) ||  /* ndash */ \
                       (x == 0x2014))    /* mdash */

#define IS_SPECIAL(x) \
                       (IS_SPECIAL_WO_ELLIPSE(x) ||  \
                       (x == 0x2026)) /* hellip */

nsresult
nsFontMetricsOS2::ResolveForwards(HPS                  aPS,
                                  const PRUnichar*     aString,
                                  PRUint32             aLength,
                                  nsFontSwitchCallback aFunc, 
                                  void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString;
  const PRUnichar* currChar = firstChar;
  const PRUnichar* lastChar  = aString + aLength;
  PRBool running = PR_TRUE;

  nsFontSwitch fontSwitch;
  fontSwitch.mFont = 0;

  if (mConvertCodePage == 1252)
  {
    while (running && firstChar < lastChar)
    {
      if ((*currChar > 0x00FF) && !IS_SPECIAL(*currChar))
      {
        if (!mUnicodeFont) {
          FindUnicodeFont(aPS);
          if (!mUnicodeFont) {
            mUnicodeFont = FindGlobalFont(aPS, *currChar);
          }
        }
        fontSwitch.mFont = mUnicodeFont;
        while( ++currChar < lastChar ) {
          if (( *currChar <= 0x00FF ) || IS_SPECIAL(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      } else {
        
        fontSwitch.mFont = mFontHandle;
        while( ++currChar < lastChar ) {
          if (( *currChar > 0x00FF ) && !IS_SPECIAL(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      }
      firstChar = currChar;
    }
  }
  else
  {
    while (running && firstChar < lastChar)
    {
      if ((*currChar >= 0x0080 && *currChar <= 0x00FF) || IS_SPECIAL_WO_ELLIPSE(*currChar))
      { 
        if (!mWesternFont) {
          FindWesternFont();
        }
        fontSwitch.mFont = mWesternFont;
        while( ++currChar < lastChar ) {
          if ((*currChar < 0x0080 || *currChar > 0x00FF) && !IS_SPECIAL_WO_ELLIPSE(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      } else {
        
        fontSwitch.mFont = mFontHandle;
        while( ++currChar < lastChar ) {
          if ((*currChar >= 0x0080 && *currChar <= 0x00FF) || IS_SPECIAL_WO_ELLIPSE(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      }
      firstChar = currChar;
    }
  }
  return NS_OK;
}

nsresult
nsFontMetricsOS2::ResolveBackwards(HPS                  aPS,
                                   const PRUnichar*     aString,
                                   PRUint32             aLength,
                                   nsFontSwitchCallback aFunc, 
                                   void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString + aLength - 1;
  const PRUnichar* lastChar  = aString - 1;
  const PRUnichar* currChar  = firstChar;
  PRBool running = PR_TRUE;

  nsFontSwitch fontSwitch;
  fontSwitch.mFont = 0;
  
  if (mConvertCodePage == 1252)
  {
    while (running && firstChar > lastChar)
    {
      if ((*currChar > 0x00FF) && !IS_SPECIAL(*currChar))
      {
        if (!mUnicodeFont) {
          FindUnicodeFont(aPS);
          if (!mUnicodeFont) {
            mUnicodeFont = FindGlobalFont(aPS, *currChar);
          }
        }
        fontSwitch.mFont = mUnicodeFont;
        while( --currChar > lastChar ) {
          if (( *currChar <= 0x00FF ) || IS_SPECIAL(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      }
      else
      {
         
        fontSwitch.mFont = mFontHandle;
        while( --currChar > lastChar ) {
          if (( *currChar > 0x00FF ) && !IS_SPECIAL(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      }
      firstChar = currChar;
    }
  }
  else
  {
    while (running && firstChar > lastChar)
    {
      if ((*currChar >= 0x0080 && *currChar <= 0x00FF) || IS_SPECIAL(*currChar))
      { 
        if (!mWesternFont) {
          FindWesternFont();
        }
        fontSwitch.mFont = mWesternFont;
        while( --currChar > lastChar ) {
          if ((*currChar < 0x0080 || *currChar > 0x00FF) && !IS_SPECIAL(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      }
      else
      {
         
        fontSwitch.mFont = mFontHandle;
        while( --currChar > lastChar ) {
          if ((*currChar >= 0x0080 && *currChar <= 0x00FF) || IS_SPECIAL(*currChar))
            break;
        }
        running = (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
      }
      firstChar = currChar;
    }
  }
  return NS_OK;
}





#ifdef DEBUG_FONT_STRUCT_ALLOCS
unsigned long nsFontOS2::mRefCount = 0;
#endif

nsFontOS2::nsFontOS2(void)
{
  mFattrs.usRecordLength = sizeof(mFattrs);
  mHashMe = gCurrHashValue;
  gCurrHashValue++;
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount++;
  printf("+++ nsFontOS2 total = %d\n", mRefCount);
#endif
}

nsFontOS2::~nsFontOS2(void)
{
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount--;
  printf("--- nsFontOS2 total = %d\n", mRefCount);
#endif
}

void
nsFontOS2::SelectIntoPS( HPS aPS, long aLcid )
{
   GFX (::GpiSetCharSet(aPS, aLcid), FALSE);
   GFX (::GpiSetCharBox(aPS, &mCharbox), FALSE);
}

PRInt32
nsFontOS2::GetWidth(HPS aPS, const char* aString, PRUint32 aLength)
{
  SIZEL size;
  GetTextExtentPoint32(aPS, aString, aLength, &size);
  return size.cx;
}

PRInt32
nsFontOS2::GetWidth(HPS aPS, const PRUnichar* aString, PRUint32 aLength)
{
  nsAutoCharBuffer buffer;
  PRInt32 destLength = aLength;
  WideCharToMultiByte(mConvertCodePage, aString, aLength, buffer, destLength);

  SIZEL size;
  GetTextExtentPoint32(aPS, buffer.get(), destLength, &size);

  return size.cx;
}

void
nsFontOS2::DrawString(HPS aPS, nsDrawingSurfaceOS2* aSurface,
                      PRInt32 aX, PRInt32 aY,
                      const char* aString, PRUint32 aLength, INT* aDx0)
{
  POINTL ptl = { aX, aY };
  aSurface->NS2PM(&ptl, 1);
  ExtTextOut(aPS, ptl.x, ptl.y, 0, NULL, aString, aLength, aDx0);
}

void
nsFontOS2::DrawString(HPS aPS, nsDrawingSurfaceOS2* aSurface,
                      PRInt32 aX, PRInt32 aY,
                      const PRUnichar* aString, PRUint32 aLength)
{
  nsAutoCharBuffer buffer;
  PRInt32 destLength = aLength;
  WideCharToMultiByte(mConvertCodePage, aString, aLength, buffer, destLength);

  POINTL ptl = { aX, aY };
  aSurface->NS2PM (&ptl, 1);

  ExtTextOut(aPS, ptl.x, ptl.y, 0, NULL, buffer.get(), destLength, NULL);
}


#ifdef USE_FREETYPE



Ft2EnableFontEngine nsFontMetricsOS2FT::pfnFt2EnableFontEngine = NULL;

#define IsCJKLangGroupAtom(a)  ((a)==gJA || (a)==gKO || (a)==gZHCN || \
                                (a)==gZHTW || (a) == gZHHK)

nsFontMetricsOS2FT::~nsFontMetricsOS2FT()
{
  mSubstituteFont = nsnull; 
}

nsFontOS2*
nsFontMetricsOS2FT::FindPrefFont(HPS aPS, PRUint32 aChar)
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
  
  
  GenericFontEnumContext context = {aPS, aChar, nsnull, this};
  font.EnumerateFamilies(GenericFontEnumCallback, &context);
  if (context.mFont) { 
    return context.mFont;
  }
  mTriedAllPref = 1;
  return nsnull;
}

struct FindGlobalFontData
{
  HPS               ps;       
  PRUint32          ch;       
  nsFontOS2FT*      font;     
  GlobalFontEntry*  entry;    
};

PR_STATIC_CALLBACK(PLDHashOperator)
FindGlobalFontEnumFunc(GlobalFontEntry* aEntry, void* aData)
{
  FindGlobalFontData* data = NS_STATIC_CAST(FindGlobalFontData*, aData);
  strcpy(data->font->mFattrs.szFacename, aEntry->mMetrics->szFacename);
#ifdef PERF_HASGLYPH_CHAR_MAP
  if (aEntry->mHaveCheckedCharMap == nsnull) {
    aEntry->mHaveCheckedCharMap = (PRUint32*)calloc(UCS2_MAP_LEN, sizeof(PRUint32));
    aEntry->mRepresentableCharMap = (PRUint32*)calloc(UCS2_MAP_LEN, sizeof(PRUint32));
  }
  data->font->mHaveCheckedCharMap = aEntry->mHaveCheckedCharMap;
  data->font->mRepresentableCharMap = aEntry->mRepresentableCharMap;
#endif

  if (data->font->HasGlyph(data->ps, data->ch)) {
    data->entry = aEntry;
    return PL_DHASH_STOP;
  } else {
    return PL_DHASH_NEXT;
  }
}

nsFontOS2*
nsFontMetricsOS2FT::FindGlobalFont(HPS aPS, PRUint32 aChar)
{
  
  if (!gGlobalFonts) {
    if (!InitializeGlobalFonts()) {
      return nsnull;
    }
  }

  FindGlobalFontData data = {aPS, aChar, nsnull, nsnull};
  data.font = new nsFontOS2FT();
  gGlobalFonts->EnumerateEntries(FindGlobalFontEnumFunc, &data);
  delete data.font;
  if (data.entry) {
    return LoadFont(aPS, data.entry->GetKey());
  }
  return nsnull;
}

nsFontOS2*
nsFontMetricsOS2FT::FindSubstituteFont(HPS aPS, PRUint32 c)
{
  









  if (mSubstituteFont) {
    
    
    ((nsFontOS2Substitute*)mSubstituteFont)->SetRepresentable(c);
    return mSubstituteFont;
  }

  
  
  

  
  int i, count = mLoadedFonts.Count();
  for (i = 0; i < count; ++i) {
    nsFontOS2* font = (nsFontOS2*)mLoadedFonts[i];
    if (font->HasGlyph(aPS, NS_REPLACEMENT_CHAR)) {
      nsFontOS2Substitute* subFont = new nsFontOS2Substitute(font);
      mLoadedFonts.AppendElement((nsFontOS2*)subFont);
      subFont->SetRepresentable(c);
      mSubstituteFont = subFont;
      return subFont;
    }
  }

  
  
  
  FindGlobalFontData data = {aPS, NS_REPLACEMENT_CHAR, nsnull, nsnull};
  data.font = new nsFontOS2FT();
  gGlobalFonts->EnumerateEntries(FindGlobalFontEnumFunc, &data);
  delete data.font;
  if (data.entry) {
    nsFontOS2* font = LoadFont(aPS, data.entry->GetKey());
    if (font) {
      nsFontOS2Substitute* subFont = new nsFontOS2Substitute(font);
      
      
      mLoadedFonts.ReplaceElementAt((nsFontOS2*)subFont, mLoadedFonts.Count());
      subFont->SetRepresentable(c);
      mSubstituteFont = subFont;
      return subFont;
    }
  }

  NS_ERROR("Could not provide a substititute font");
  return nsnull;
}

nsFontOS2*
nsFontMetricsOS2FT::LocateFont(HPS aPS, PRUint32 aChar, PRInt32 & aCount)
{
  nsFontOS2* font;
  PRInt32 i;

  
  for (i = 0; i < aCount; ++i) {
    font = (nsFontOS2*)mLoadedFonts[i];
    if (font->HasGlyph(aPS, aChar))
      return font;
  }

  font = FindFont(aPS, aChar);
  aCount = mLoadedFonts.Count(); 
  NS_ASSERTION(font && mLoadedFonts.IndexOf(font) >= 0,
               "Could not find a font");
  return font;
}

nsresult
nsFontMetricsOS2FT::ResolveForwards(HPS                  aPS,
                                    const PRUnichar*     aString,
                                    PRUint32             aLength,
                                    nsFontSwitchCallback aFunc, 
                                    void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString;
  const PRUnichar* currChar = firstChar;
  const PRUnichar* lastChar  = aString + aLength;
  nsFontOS2* currFont;
  nsFontOS2* nextFont;
  PRInt32 count;
  nsFontSwitch fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  count = mLoadedFonts.Count();

  if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
    currFont = LocateFont(aPS, SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
    currChar += 2;
  }
  else {
    currFont = LocateFont(aPS, *currChar, count);
    ++currChar;
  }

  
  
  if (currFont == mLoadedFonts[0]) {
    while (currChar < lastChar && (currFont->HasGlyph(aPS, *currChar)))
      ++currChar;
    fontSwitch.mFont = currFont;
    if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
      return NS_OK;
    if (currChar == lastChar)
      return NS_OK;
    
    firstChar = currChar;
    if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
      currFont = LocateFont(aPS, SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
      currChar += 2;
    }
    else {
      currFont = LocateFont(aPS, *currChar, count);
      ++currChar;
    }
  }

  
  PRInt32 lastCharLen;
  while (currChar < lastChar) {
    if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
      nextFont = LocateFont(aPS, SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
      lastCharLen = 2;
    }
    else {
      nextFont = LocateFont(aPS, *currChar, count);
      lastCharLen = 1;
    }
    if (nextFont != currFont) {
      
      
      fontSwitch.mFont = currFont;
      if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
        return NS_OK;
      
      firstChar = currChar;

      currFont = nextFont; 
    }
    currChar += lastCharLen;
  }

  
  fontSwitch.mFont = currFont;
  (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
  return NS_OK;
}

nsresult
nsFontMetricsOS2FT::ResolveBackwards(HPS                  aPS,
                                     const PRUnichar*     aString,
                                     PRUint32             aLength,
                                     nsFontSwitchCallback aFunc, 
                                     void*                aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString + aLength - 1;
  const PRUnichar* lastChar  = aString - 1;
  const PRUnichar* currChar  = firstChar;
  nsFontOS2* currFont;
  nsFontOS2* nextFont;
  PRInt32 count;
  nsFontSwitch fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  count = mLoadedFonts.Count();

  
  if (NS_IS_LOW_SURROGATE(*currChar) && (currChar-1) > lastChar && NS_IS_HIGH_SURROGATE(*(currChar-1))) {
    currFont = LocateFont(aPS, SURROGATE_TO_UCS4(*(currChar-1), *currChar), count);
    currChar -= 2;
  }
  else {
    currFont = LocateFont(aPS, *currChar, count);
    --currChar;
  }

  
  
  if (currFont == mLoadedFonts[0]) {
    while (currChar > lastChar && (currFont->HasGlyph(aPS, *currChar)))
      --currChar;
    fontSwitch.mFont = currFont;
    if (!(*aFunc)(&fontSwitch, currChar+1, firstChar - currChar, aData))
      return NS_OK;
    if (currChar == lastChar)
      return NS_OK;
    
    firstChar = currChar;
    if (NS_IS_LOW_SURROGATE(*currChar) && (currChar-1) > lastChar && NS_IS_HIGH_SURROGATE(*(currChar-1))) {
      currFont = LocateFont(aPS, SURROGATE_TO_UCS4(*(currChar-1), *currChar), count);
      currChar -= 2;
    }
    else {
      currFont = LocateFont(aPS, *currChar, count);
      --currChar;
    }
  }

  
  PRInt32 lastCharLen;
  PRUint32 codepoint;

  while (currChar > lastChar) {
    if (NS_IS_LOW_SURROGATE(*currChar) && (currChar-1) > lastChar && NS_IS_HIGH_SURROGATE(*(currChar-1))) {
      codepoint =  SURROGATE_TO_UCS4(*(currChar-1), *currChar);
      nextFont = LocateFont(aPS, codepoint, count);
      lastCharLen = 2;
    }
    else {
      codepoint = *currChar;
      nextFont = LocateFont(aPS, codepoint, count);
      lastCharLen = 1;
    }
    if (nextFont != currFont ||
        

        codepoint > 0xFFFF) {
      
      
      fontSwitch.mFont = currFont;
      if (!(*aFunc)(&fontSwitch, currChar+1, firstChar - currChar, aData))
        return NS_OK;
      
      firstChar = currChar;
      currFont = nextFont; 
    }
    currChar -= lastCharLen;
  }

  
  fontSwitch.mFont = currFont;
  (*aFunc)(&fontSwitch, currChar+1, firstChar - currChar, aData);

  return NS_OK;
}





Ft2FontSupportsUnicodeChar1 nsFontOS2FT::pfnFt2FontSupportsUnicodeChar1 = NULL;
#ifdef USE_EXPANDED_FREETYPE_FUNCS
Ft2QueryTextBoxW nsFontOS2FT::pfnFt2QueryTextBoxW = NULL;
Ft2CharStringPosAtW nsFontOS2FT::pfnFt2CharStringPosAtW = NULL;
#endif 

#ifdef DEBUG_FONT_STRUCT_ALLOCS
unsigned long nsFontOS2FT::mRefCount = 0;
#endif

nsFontOS2FT::nsFontOS2FT(void) : nsFontOS2()
{
  mFattrs.usCodePage = 1208;
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount++;
  printf("+++ nsFontOS2FT total = %d\n", mRefCount);
#endif
}

nsFontOS2FT::~nsFontOS2FT(void)
{
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount--;
  printf("--- nsFontOS2FT total = %d\n", mRefCount);
#endif
}

PRBool
nsFontOS2FT::HasGlyph(HPS aPS, PRUint32 aChar)
{
#ifdef PERF_HASGLYPH_CHAR_MAP
  if (IS_IN_BMP(aChar) && IS_REPRESENTABLE(mHaveCheckedCharMap, aChar)) {
    
    return IS_REPRESENTABLE(mRepresentableCharMap, aChar);
  } else
#endif
  {
    
    if (!IS_IN_BMP(aChar)) {
      return PR_FALSE;
    }

    PRBool rc = pfnFt2FontSupportsUnicodeChar1(0, &mFattrs, PR_TRUE, aChar);

#ifdef PERF_HASGLYPH_CHAR_MAP
    
    SET_REPRESENTABLE(mHaveCheckedCharMap, aChar);
    if (rc) {
      SET_REPRESENTABLE(mRepresentableCharMap, aChar);
    }
#endif

    return rc;
  }
}

PRInt32
nsFontOS2FT::GetWidth(HPS aPS, const PRUnichar* aString, PRUint32 aLength)
{
  USHORT rc;
  SIZEL size;

  if (!IsSymbolFont()) {
#ifdef USE_EXPANDED_FREETYPE_FUNCS
    POINTL ptls[5];
    rc = pfnFt2QueryTextBoxW(aPS, aLength, (LPWSTR)aString, 5, ptls);
    size.cx = ptls[TXTBOX_CONCAT].x;
    
#ifdef DEBUG
    if (rc == FALSE) {
      
      
      
      USHORT errorCode = ERRORIDERROR (::WinGetLastError(0));
      if (errorCode != PMERR_FUNCTION_NOT_SUPPORTED) {
        printf("GFX_Err: pfnFt2QueryTextBoxW = 0x%X, 0x%X (%s - %s,  line %d)\n",
               rc, errorCode, __FILE__, __FUNCTION__, __LINE__);
      }
    }
#endif 

    
    
    
    
    if (rc == FALSE)
#endif 
    {
      NS_ConvertUTF16toUTF8 str(Substring(aString, aString + aLength));
      rc = GetTextExtentPoint32(aPS, (const char*)str.get(), str.Length(),
                                &size);
    }
  } else {
    nsAutoCharBuffer buffer;
    PRInt32 destLength = aLength;
    WideCharToMultiByte(1252, aString, aLength, buffer, destLength);
    rc = GetTextExtentPoint32(aPS, buffer.get(), destLength, &size);
  }

  if (rc == TRUE) {
    return size.cx;
  } else {
    return 0;
  }
}

void
nsFontOS2FT::DrawString(HPS aPS, nsDrawingSurfaceOS2* aSurface,
                        PRInt32 aX, PRInt32 aY,
                        const PRUnichar* aString, PRUint32 aLength)
{
  USHORT rc;
  POINTL ptl = { aX, aY };
  aSurface->NS2PM(&ptl, 1);

  if (!IsSymbolFont()) {
#ifdef USE_EXPANDED_FREETYPE_FUNCS
    rc = pfnFt2CharStringPosAtW(aPS, &ptl, NULL, 0, aLength,
                                (LPWSTR)aString, NULL, NULL);

#ifdef DEBUG
    if (rc == GPI_ERROR) {
      
      
      
      USHORT errorCode = ERRORIDERROR (::WinGetLastError(0));
      if (errorCode != PMERR_FUNCTION_NOT_SUPPORTED) {
        printf("GFX_Err: pfnFt2CharStringPosAtW = 0x%X, 0x%X (%s - %s,  line %d)\n",
               rc, errorCode, __FILE__, __FUNCTION__, __LINE__);
      }
    }
#endif 

    
    
    
    
    if (rc == GPI_ERROR)
#endif 
    {
      NS_ConvertUTF16toUTF8 str(Substring(aString, aString + aLength));
      ExtTextOut(aPS, ptl.x, ptl.y, 0, NULL, (const char*)str.get(),
                 str.Length(), NULL);
    }
  } else {
    nsAutoCharBuffer buffer;
    PRInt32 destLength = aLength;
    WideCharToMultiByte(1252, aString, aLength, buffer, destLength);
    ExtTextOut(aPS, ptl.x, ptl.y, 0, NULL, buffer.get(), destLength, NULL);
  }
}




nsFontOS2Substitute::nsFontOS2Substitute(nsFontOS2* aFont)
{
  mHashMe = gCurrHashValue;
  gCurrHashValue++;

  mFattrs = aFont->mFattrs;
  mCharbox = aFont->mCharbox;
  mMaxAscent = aFont->mMaxAscent;
  mMaxDescent = aFont->mMaxDescent;
  mConvertCodePage = aFont->mConvertCodePage;

  memset(mRepresentableCharMap, 0, sizeof(mRepresentableCharMap));
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount++;
  printf("+++ nsFontOS2Substitute total = %d\n", mRefCount);
#endif
}

nsFontOS2Substitute::~nsFontOS2Substitute(void)
{
#ifdef DEBUG_FONT_STRUCT_ALLOCS
  mRefCount--;
  printf("--- nsFontOS2Substitute total = %d\n", mRefCount);
#endif
}

static nsresult
SubstituteChars(const PRUnichar*    aString, 
                PRUint32            aLength,
                nsAutoChar16Buffer& aResult,
                PRUint32*           aCount)
{
  nsresult res;
  if (!gFontSubstituteConverter) {
    CallCreateInstance(NS_SAVEASCHARSET_CONTRACTID, &gFontSubstituteConverter);
    if (gFontSubstituteConverter) {
      res = gFontSubstituteConverter->Init("ISO-8859-1",
                              nsISaveAsCharset::attr_EntityAfterCharsetConv +
                              nsISaveAsCharset::attr_FallbackQuestionMark +
                              nsISaveAsCharset::attr_IgnoreIgnorables,
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
}

PRInt32
nsFontOS2Substitute::GetWidth(HPS aPS, const PRUnichar* aString,
                              PRUint32 aLength)
{
  nsAutoChar16Buffer buffer;
  nsresult rv = SubstituteChars(aString, aLength, buffer, &aLength);
  if (NS_FAILED(rv) || !aLength) return 0;

  SIZEL size;
  PRUnichar* string = buffer.get();
  NS_ConvertUTF16toUTF8 str(Substring(string, string + aLength));
  BOOL rc = GetTextExtentPoint32(aPS, (const char*)str.get(), str.Length(),
                                 &size);
  if (rc == TRUE) {
    return size.cx;
  } else {
    return 0;
  }
}

void
nsFontOS2Substitute::DrawString(HPS aPS, nsDrawingSurfaceOS2* aSurface,
                                PRInt32 aX, PRInt32 aY,
                                const PRUnichar* aString, PRUint32 aLength)
{
  nsAutoChar16Buffer buffer;
  nsresult rv = SubstituteChars(aString, aLength, buffer, &aLength);
  if (NS_FAILED(rv) || !aLength) return;

  POINTL ptl = { aX, aY };
  aSurface->NS2PM(&ptl, 1);
  PRUnichar* string = buffer.get();
  NS_ConvertUTF16toUTF8 str(Substring(string, string + aLength));
  ExtTextOut(aPS, ptl.x, ptl.y, 0, NULL, (const char*)str.get(), str.Length(), NULL);
}
#endif 





nsFontEnumeratorOS2::nsFontEnumeratorOS2()
{
}

NS_IMPL_ISUPPORTS1(nsFontEnumeratorOS2, nsIFontEnumerator)



static int
CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure)
{
  const PRUnichar* str1 = *((const PRUnichar**) aArg1);
  const PRUnichar* str2 = *((const PRUnichar**) aArg2);

  int rc = 9;
  if ((char)str1[0] == '@') {
    str1++;
    rc = nsCRT::strcmp(str1, str2);
    if (rc == 0)
      return 1;
  }
  if ((char)str2[0] == '@') {
    str2++;
    rc = nsCRT::strcmp(str1, str2);
    if (rc == 0)
      return -1;
  }

  if (rc == 9)
    rc = nsCRT::strcmp(str1, str2);
  return rc;
}

NS_IMETHODIMP
nsFontEnumeratorOS2::EnumerateFonts(const char* aLangGroup,
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult)
{
  return EnumerateAllFonts(aCount, aResult);
}

struct EnumerateAllFontsData
{
  PRUnichar** array;      
  int count;              
};

PR_STATIC_CALLBACK(PLDHashOperator)
EnumerateAllFontsCallback(GlobalFontEntry* aEntry, void* aData)
{
  EnumerateAllFontsData* data = NS_STATIC_CAST(EnumerateAllFontsData*, aData);
  data->array[data->count++] = ToNewUnicode(aEntry->GetKey());
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsFontEnumeratorOS2::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG_POINTER(aCount);
  NS_ENSURE_ARG_POINTER(aResult);

  if (!nsFontMetricsOS2::gGlobalFonts) {
    nsresult res = nsFontMetricsOS2::InitializeGlobalFonts();
    if (NS_FAILED(res)) {
      FreeGlobals();
      return res;
    }
  }

  *aCount = nsFontMetricsOS2::gGlobalFonts->Count();
  PRUnichar** array = (PRUnichar**)nsMemory::Alloc(*aCount * sizeof(PRUnichar*));
  NS_ENSURE_TRUE(array, NS_ERROR_OUT_OF_MEMORY);

  EnumerateAllFontsData data = {array, 0};
  nsFontMetricsOS2::gGlobalFonts->EnumerateEntries(EnumerateAllFontsCallback,
                                                  &data);

  NS_QuickSort(array, *aCount, sizeof(PRUnichar*), CompareFontNames, nsnull);

  *aResult = array;

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorOS2::HaveFontFor(const char* aLangGroup, PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aLangGroup);
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;

  
  NS_ASSERTION( 0, "HaveFontFor is not implemented" );

  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsFontEnumeratorOS2::GetDefaultFont(const char *aLangGroup, 
  const char *aGeneric, PRUnichar **aResult)
{
  
  

  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorOS2::UpdateFontList(PRBool *updateFontList)
{
  *updateFontList = PR_FALSE; 
  NS_ASSERTION( 0, "UpdateFontList is not implemented" );
  return NS_ERROR_NOT_IMPLEMENTED;
}

