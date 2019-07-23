













































#define ENABLE_X_FONT_BANNING 1

#include <sys/types.h>
#include "gfx-config.h"
#include "nscore.h"
#include "nsQuickSort.h"
#include "nsFontMetricsXlib.h"
#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsILanguageAtomService.h"
#include "nsISaveAsCharset.h"
#include "nsIPref.h"
#include "nsCOMPtr.h"
#include "nsPrintfCString.h"
#include "nspr.h"
#include "nsHashtable.h"
#include "nsReadableUtils.h"
#include "nsAString.h"
#include "nsXPIDLString.h"
#ifdef MOZ_ENABLE_FREETYPE2
#include "nsFT2FontCatalog.h"
#include "nsFreeType.h"
#endif 
#ifdef USE_X11SHARED_CODE
#include "nsXFontNormal.h"
#endif 
#ifdef USE_AASB
#include "nsX11AlphaBlend.h"
#include "nsXFontAAScaledBitmap.h"
#endif 
#ifdef USE_XPRINT
#include "xprintutil.h"
#endif 
#include "xlibrgb.h"
#include "nsUnicharUtils.h"
#ifdef ENABLE_X_FONT_BANNING
#include <regex.h>
#endif 

#include <X11/Xatom.h>

#define UCS2_NOMAPPING 0XFFFD

#ifdef PR_LOGGING 
static PRLogModuleInfo *FontMetricsXlibLM = PR_NewLogModule("FontMetricsXlib");
#endif 

#ifdef ENABLE_X_FONT_BANNING

#ifndef REG_OK
#define REG_OK (0)
#endif 
#endif 

#undef USER_DEFINED
#define USER_DEFINED "x-user-def"





#define FONT_MAX_FONT_SCALE 2

#undef NOISY_FONTS
#undef REALLY_NOISY_FONTS


static PRBool                 FreeNode(nsHashKey* aKey, void* aData, void* aClosure);
#ifdef MOZ_ENABLE_FREETYPE2
static void                   CharSetNameToCodeRangeBits(const char*, PRUint32*, PRUint32*);
#endif 
static const nsFontCharSetMapXlib *GetCharSetMap(nsFontMetricsXlibContext *aFmctx, const char *aCharSetName);



static PRUint32 gFontDebug = 0 | NS_FONT_DEBUG_FONT_SCAN;

struct nsFontCharSetMapXlib;
struct nsFontFamilyNameXlib;
struct nsFontPropertyNameXlib;
struct nsFontStyleXlib;
struct nsFontWeightXlib;
struct nsFontLangGroupXlib;





class nsFontMetricsXlibContext
{
public:
  nsFontMetricsXlibContext();
  nsresult Init(nsIDeviceContext *aDevice, PRBool printermode);
  ~nsFontMetricsXlibContext();

  XlibRgbHandle                        *mXlibRgbHandle;
#ifdef USE_XPRINT
  
  PRPackedBool                          mPrinterMode;
#endif   
  PRPackedBool                          mAllowDoubleByteSpecialChars;
  PRPackedBool                          mForceOutlineScaledFonts;

  PRPackedBool                          mScaleBitmapFontsWithDevScale;
  float                                 mDevScale;

  nsCOMPtr<nsIPref>                     mPref;

  nsCOMPtr<nsICharsetConverterManager> mCharSetManager;
  nsCOMPtr<nsIUnicodeEncoder>           mUserDefinedConverter;

  nsHashtable                           mAliases;
  nsHashtable                           mCharSetMaps;
  nsHashtable                           mFamilies;
  nsHashtable                           mFFRENodes;
  nsHashtable                           mAFRENodes;

  
  
  nsHashtable                           mCachedFFRESearches;
  nsHashtable                           mSpecialCharSets;
  nsHashtable                           mStretches;
  nsHashtable                           mWeights;
  nsCOMPtr<nsISaveAsCharset>            mFontSubConverter;

  PRBool                                mGlobalListInitalised;
  nsFontNodeArrayXlib                   mGlobalList;

  nsFontCharSetInfoXlib                *mUnknown,
                                       *mSpecial,
                                       *mISO106461;
  
  nsCOMPtr<nsIAtom>                     mUnicode;
  nsCOMPtr<nsIAtom>                     mUserDefined;
  nsCOMPtr<nsIAtom>                     mZHTW;
  nsCOMPtr<nsIAtom>                     mZHHK;
  nsCOMPtr<nsIAtom>                     mZHTWHK; 
  nsCOMPtr<nsIAtom>                     mUsersLocale;
  nsCOMPtr<nsIAtom>                     mWesternLocale;

  const nsFontCharSetMapXlib           *mCharSetMap;
  const nsFontCharSetMapXlib           *mNoneCharSetMap;
  const nsFontCharSetMapXlib           *mSpecialCharSetMap;

  PRUint16                             *mUserDefinedCCMap;
  PRUint16                             *mEmptyCCMap;
  PRUint16                             *mDoubleByteSpecialCharsCCMap;

  

  PRInt32                               mOutlineScaleMinimum;
#ifdef USE_AASB

  PRBool                                mAABitmapScaleEnabled;
  PRBool                                mAABitmapScaleAlways;
  PRInt32                               mAABitmapScaleMinimum;
  double                                mAABitmapOversize;
  double                                mAABitmapUndersize;
#endif 


  PRInt32                               mBitmapScaleMinimum;
  double                                mBitmapOversize;
  double                                mBitmapUndersize;

#ifdef USE_AASB
  PRInt32                               mAntiAliasMinimum;
#endif 
  PRInt32                               mEmbeddedBitmapMaximumHeight;

#ifdef MOZ_ENABLE_FREETYPE2
  PRBool                                mEnableFreeType2;
  PRBool                                mFreeType2Autohinted;
  PRBool                                mFreeType2Unhinted;
#endif 
#ifdef USE_AASB
  PRUint8                               mAATTDarkTextMinValue;
  double                                mAATTDarkTextGain;
#endif 

#ifdef ENABLE_X_FONT_BANNING
  regex_t                              *mFontRejectRegEx,
                                       *mFontAcceptRegEx;
#endif 
};

struct nsFontCharSetInfoXlib
{
  const char*            mCharSet;
  nsFontCharSetConverterXlib Convert;
  PRUint8                mSpecialUnderline;
#ifdef MOZ_ENABLE_FREETYPE2
  PRInt32                mCodeRange1Bits;
  PRInt32                mCodeRange2Bits;
#endif
  PRUint16*              mCCMap;
  nsIUnicodeEncoder*     mConverter;
  nsIAtom*               mLangGroup;
  PRBool                 mInitedSizeInfo;
  PRInt32                mOutlineScaleMin;
  PRInt32                mBitmapScaleMin;
  double                 mBitmapOversize;
  double                 mBitmapUndersize;
#ifdef USE_AASB  
  PRInt32                mAABitmapScaleMin;
  double                 mAABitmapOversize;
  double                 mAABitmapUndersize;
  PRBool                 mAABitmapScaleAlways;
#endif 
};

struct nsFontFamilyXlib
{
  NS_DECL_AND_IMPL_ZEROING_OPERATOR_NEW

  nsFontNodeArrayXlib mNodes;
};

struct nsFontFamilyNameXlib
{
  const char* mName;
  const char* mXName;
};

struct nsFontPropertyNameXlib
{
  const char* mName;
  int         mValue;
};

static void SetCharsetLangGroup(nsFontMetricsXlibContext *aFmctx, nsFontCharSetInfoXlib* aCharSetInfo);

static int SingleByteConvert(nsFontCharSetInfoXlib* aSelf, XFontStruct* aFont,
  const PRUnichar* aSrcBuf, PRInt32 aSrcLen, char* aDestBuf, PRInt32 aDestLen);
static int DoubleByteConvert(nsFontCharSetInfoXlib* aSelf, XFontStruct* aFont,
  const PRUnichar* aSrcBuf, PRInt32 aSrcLen, char* aDestBuf, PRInt32 aDestLen);
static int ISO10646Convert(nsFontCharSetInfoXlib* aSelf, XFontStruct* aFont,
  const PRUnichar* aSrcBuf, PRInt32 aSrcLen, char* aDestBuf, PRInt32 aDestLen);

static nsFontCharSetInfoXlib Unknown = { nsnull };
static nsFontCharSetInfoXlib Special = { nsnull };

#ifdef MOZ_ENABLE_FREETYPE2
static nsFontCharSetInfoXlib USASCII =
  { "us-ascii", SingleByteConvert, 0,
    TT_OS2_CPR1_LATIN1 | TT_OS2_CPR1_MAC_ROMAN,
    TT_OS2_CPR2_CA_FRENCH |  TT_OS2_CPR2_PORTUGESE
    | TT_OS2_CPR2_WE_LATIN1 |  TT_OS2_CPR2_US };
static nsFontCharSetInfoXlib ISO88591 =
  { "ISO-8859-1", SingleByteConvert, 0,
    TT_OS2_CPR1_LATIN1 | TT_OS2_CPR1_MAC_ROMAN,
    TT_OS2_CPR2_CA_FRENCH |  TT_OS2_CPR2_PORTUGESE
    | TT_OS2_CPR2_WE_LATIN1 |  TT_OS2_CPR2_US };
static nsFontCharSetInfoXlib ISO88592 =
  { "ISO-8859-2", SingleByteConvert, 0,
    TT_OS2_CPR1_LATIN2, TT_OS2_CPR2_LATIN2 };
static nsFontCharSetInfoXlib ISO88593 =
  { "ISO-8859-3", SingleByteConvert, 0,
    TT_OS2_CPR1_TURKISH, TT_OS2_CPR2_TURKISH };
static nsFontCharSetInfoXlib ISO88594 =
  { "ISO-8859-4", SingleByteConvert, 0,
    TT_OS2_CPR1_BALTIC, TT_OS2_CPR2_BALTIC };
static nsFontCharSetInfoXlib ISO88595 =
  { "ISO-8859-5", SingleByteConvert, 0,
    TT_OS2_CPR1_CYRILLIC, TT_OS2_CPR2_RUSSIAN | TT_OS2_CPR2_CYRILLIC };
static nsFontCharSetInfoXlib ISO88596 =
  { "ISO-8859-6", SingleByteConvert, 0,
      TT_OS2_CPR1_ARABIC, TT_OS2_CPR2_ARABIC | TT_OS2_CPR2_ARABIC_708 };
static nsFontCharSetInfoXlib ISO885968x =
  { "x-iso-8859-6-8-x", SingleByteConvert, 0,
      TT_OS2_CPR1_ARABIC, TT_OS2_CPR2_ARABIC | TT_OS2_CPR2_ARABIC_708 };
static nsFontCharSetInfoXlib ISO8859616 =
  { "x-iso-8859-6-16", SingleByteConvert, 0,
      TT_OS2_CPR1_ARABIC, TT_OS2_CPR2_ARABIC | TT_OS2_CPR2_ARABIC_708 };
static nsFontCharSetInfoXlib IBM1046 =
  { "x-IBM1046", SingleByteConvert, 0,
      TT_OS2_CPR1_ARABIC, TT_OS2_CPR2_ARABIC | TT_OS2_CPR2_ARABIC_708 };
static nsFontCharSetInfoXlib ISO88597 =
  { "ISO-8859-7", SingleByteConvert, 0,
    TT_OS2_CPR1_GREEK, TT_OS2_CPR2_GREEK | TT_OS2_CPR2_GREEK_437G };
static nsFontCharSetInfoXlib ISO88598 =
  { "ISO-8859-8", SingleByteConvert, 0,
    TT_OS2_CPR1_HEBREW, TT_OS2_CPR2_HEBREW };



static nsFontCharSetInfoXlib ISO88599 =
  { "ISO-8859-9", SingleByteConvert, 0,
    TT_OS2_CPR1_TURKISH, TT_OS2_CPR2_TURKISH };








static nsFontCharSetInfoXlib ISO885913 =
  { "ISO-8859-13", SingleByteConvert, 0,
    TT_OS2_CPR1_BALTIC, TT_OS2_CPR2_BALTIC };
static nsFontCharSetInfoXlib ISO885915 =
  { "ISO-8859-15", SingleByteConvert, 0,
    TT_OS2_CPR1_LATIN2, TT_OS2_CPR2_LATIN2 };
static nsFontCharSetInfoXlib JISX0201 =
  { "jis_0201", SingleByteConvert, 1,
    TT_OS2_CPR1_JAPANESE, 0 };
static nsFontCharSetInfoXlib KOI8R =
  { "KOI8-R", SingleByteConvert, 0,
    TT_OS2_CPR1_CYRILLIC, TT_OS2_CPR2_RUSSIAN | TT_OS2_CPR2_CYRILLIC };
static nsFontCharSetInfoXlib KOI8U =
  { "KOI8-U", SingleByteConvert, 0,
    TT_OS2_CPR1_CYRILLIC, TT_OS2_CPR2_RUSSIAN | TT_OS2_CPR2_CYRILLIC };
static nsFontCharSetInfoXlib TIS6202 =


#ifdef SUNCTL
  { "tis620-2", SingleByteConvert, 0,
    TT_OS2_CPR1_THAI, 0 };
#else
  { "windows-874", SingleByteConvert, 0,
    TT_OS2_CPR1_THAI, 0 };
#endif
static nsFontCharSetInfoXlib TIS620 =
  { "TIS-620", SingleByteConvert, 0,
    TT_OS2_CPR1_THAI, 0 };
static nsFontCharSetInfoXlib ISO885911 =
  { "ISO-8859-11", SingleByteConvert, 0,
    TT_OS2_CPR1_THAI, 0 };
static nsFontCharSetInfoXlib Big5 =
  { "x-x-big5", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };


static nsFontCharSetInfoXlib Big5TWHK =
  { "x-x-big5", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116431 =
  { "x-cns-11643-1", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116432 =
  { "x-cns-11643-2", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116433 =
  { "x-cns-11643-3", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116434 =
  { "x-cns-11643-4", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116435 =
  { "x-cns-11643-5", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116436 =
  { "x-cns-11643-6", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib CNS116437 =
  { "x-cns-11643-7", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib GB2312 =
  { "gb_2312-80", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_SIMP, 0 };
static nsFontCharSetInfoXlib GB18030_0 =
  { "gb18030.2000-0", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_SIMP, 0 };
static nsFontCharSetInfoXlib GB18030_1 =
  { "gb18030.2000-1", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_SIMP, 0 };
static nsFontCharSetInfoXlib GBK =
  { "x-gbk-noascii", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_SIMP, 0 };
static nsFontCharSetInfoXlib HKSCS =
  { "hkscs-1", DoubleByteConvert, 1,
    TT_OS2_CPR1_CHINESE_TRAD, 0 };
static nsFontCharSetInfoXlib JISX0208 =
  { "jis_0208-1983", DoubleByteConvert, 1,
    TT_OS2_CPR1_JAPANESE, 0 };
static nsFontCharSetInfoXlib JISX0212 =
  { "jis_0212-1990", DoubleByteConvert, 1,
    TT_OS2_CPR1_JAPANESE, 0 };
static nsFontCharSetInfoXlib KSC5601 =
  { "ks_c_5601-1987", DoubleByteConvert, 1,
    TT_OS2_CPR1_KO_WANSUNG | TT_OS2_CPR1_KO_JOHAB, 0 };
static nsFontCharSetInfoXlib X11Johab =
  { "x-x11johab", DoubleByteConvert, 1,
    TT_OS2_CPR1_KO_WANSUNG | TT_OS2_CPR1_KO_JOHAB, 0 };
static nsFontCharSetInfoXlib JohabNoAscii =
  { "x-johab-noascii", DoubleByteConvert, 1,
    TT_OS2_CPR1_KO_WANSUNG | TT_OS2_CPR1_KO_JOHAB, 0 };
static nsFontCharSetInfoXlib JamoTTF =
  { "x-koreanjamo-0", DoubleByteConvert, 1,
    TT_OS2_CPR1_KO_WANSUNG | TT_OS2_CPR1_KO_JOHAB, 0 };
static nsFontCharSetInfoXlib TamilTTF =
  { "x-tamilttf-0", DoubleByteConvert, 1,
    0, 0 };
static nsFontCharSetInfoXlib CP1250 =
  { "windows-1250", SingleByteConvert, 0,
    TT_OS2_CPR1_LATIN2, TT_OS2_CPR2_LATIN2 };
static nsFontCharSetInfoXlib CP1251 =
  { "windows-1251", SingleByteConvert, 0,
    TT_OS2_CPR1_CYRILLIC, TT_OS2_CPR2_RUSSIAN };
static nsFontCharSetInfoXlib CP1252 =
  { "windows-1252", SingleByteConvert, 0,
    TT_OS2_CPR1_LATIN1 | TT_OS2_CPR1_MAC_ROMAN,
    TT_OS2_CPR2_CA_FRENCH |  TT_OS2_CPR2_PORTUGESE
    | TT_OS2_CPR2_WE_LATIN1 |  TT_OS2_CPR2_US };
static nsFontCharSetInfoXlib CP1253 =
  { "windows-1253", SingleByteConvert, 0,
    TT_OS2_CPR1_GREEK, TT_OS2_CPR2_GREEK | TT_OS2_CPR2_GREEK_437G };
static nsFontCharSetInfoXlib CP1257 =
  { "windows-1257", SingleByteConvert, 0,
    TT_OS2_CPR1_BALTIC, TT_OS2_CPR2_BALTIC };

#ifdef SUNCTL


static nsFontCharSetInfoXlib SunIndic =
  { "x-sun-unicode-india-0", DoubleByteConvert, 0,
    0, 0 };
#endif 

static nsFontCharSetInfoXlib ISO106461 =
  { nsnull, ISO10646Convert, 1, 0xFFFFFFFF, 0xFFFFFFFF };

static nsFontCharSetInfoXlib AdobeSymbol =
   { "Adobe-Symbol-Encoding", SingleByteConvert, 0,
    TT_OS2_CPR1_SYMBOL, 0 };
static nsFontCharSetInfoXlib AdobeEuro =
  { "x-adobe-euro", SingleByteConvert, 0,
    0, 0 };

#ifdef MOZ_MATHML
static nsFontCharSetInfoXlib CMCMEX =
   { "x-t1-cmex", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib CMCMSY =
   { "x-t1-cmsy", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib CMCMR =
   { "x-t1-cmr", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib CMCMMI =
   { "x-t1-cmmi", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib Mathematica1 =
   { "x-mathematica1", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib Mathematica2 =
   { "x-mathematica2", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib Mathematica3 =
   { "x-mathematica3", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib Mathematica4 =
   { "x-mathematica4", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
static nsFontCharSetInfoXlib Mathematica5 =
   { "x-mathematica5", SingleByteConvert, 0, TT_OS2_CPR1_SYMBOL, 0};
#endif 

#else

static nsFontCharSetInfoXlib USASCII =
  { "us-ascii", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88591 =
  { "ISO-8859-1", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88592 =
  { "ISO-8859-2", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88593 =
  { "ISO-8859-3", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88594 =
  { "ISO-8859-4", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88595 =
  { "ISO-8859-5", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88596 =
  { "ISO-8859-6", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO885968x =
  { "x-iso-8859-6-8-x", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO8859616 =
  { "x-iso-8859-6-16", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib IBM1046 =
  { "x-IBM1046", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88597 =
  { "ISO-8859-7", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO88598 =
  { "ISO-8859-8", SingleByteConvert, 0 };



static nsFontCharSetInfoXlib ISO88599 =
  { "ISO-8859-9", SingleByteConvert, 0 };








static nsFontCharSetInfoXlib ISO885913 =
  { "ISO-8859-13", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO885915 =
  { "ISO-8859-15", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib JISX0201 =
  { "jis_0201", SingleByteConvert, 1 };
static nsFontCharSetInfoXlib KOI8R =
  { "KOI8-R", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib KOI8U =
  { "KOI8-U", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib TIS6202 =


#ifdef SUNCTL
  { "tis620-2", SingleByteConvert, 0 };
#else
  { "windows-874", SingleByteConvert, 0 };
#endif
static nsFontCharSetInfoXlib TIS620 =
  { "TIS-620", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib ISO885911 =
  { "ISO-8859-11", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib Big5 =
  { "x-x-big5", DoubleByteConvert, 1 };


static nsFontCharSetInfoXlib Big5TWHK =
  { "x-x-big5", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116431 =
  { "x-cns-11643-1", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116432 =
  { "x-cns-11643-2", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116433 =
  { "x-cns-11643-3", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116434 =
  { "x-cns-11643-4", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116435 =
  { "x-cns-11643-5", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116436 =
  { "x-cns-11643-6", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib CNS116437 =
  { "x-cns-11643-7", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib GB2312 =
  { "gb_2312-80", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib GB18030_0 =
  { "gb18030.2000-0", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib GB18030_1 =
  { "gb18030.2000-1", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib GBK =
  { "x-gbk-noascii", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib HKSCS =
  { "hkscs-1", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib JISX0208 =
  { "jis_0208-1983", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib JISX0212 =
  { "jis_0212-1990", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib KSC5601 =
  { "ks_c_5601-1987", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib X11Johab =
  { "x-x11johab", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib JohabNoAscii =
  { "x-johab-noascii", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib JamoTTF =
  { "x-koreanjamo-0", DoubleByteConvert, 1 };
static nsFontCharSetInfoXlib TamilTTF =
  { "x-tamilttf-0", DoubleByteConvert, 0 };
static nsFontCharSetInfoXlib CP1250 =
  { "windows-1250", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib CP1251 =
  { "windows-1251", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib CP1252 =
  { "windows-1252", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib CP1253 =
  { "windows-1253", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib CP1257 =
  { "windows-1257", SingleByteConvert, 0 };

#ifdef SUNCTL


static nsFontCharSetInfoXlib SunIndic =
  { "x-sun-unicode-india-0", DoubleByteConvert, 0 };
#endif 

static nsFontCharSetInfoXlib ISO106461 =
  { nsnull, ISO10646Convert, 1};

static nsFontCharSetInfoXlib AdobeSymbol =
   { "Adobe-Symbol-Encoding", SingleByteConvert, 0 };
static nsFontCharSetInfoXlib AdobeEuro =
  { "x-adobe-euro", SingleByteConvert, 0 };
         
#ifdef MOZ_MATHML
static nsFontCharSetInfoXlib CMCMEX =
   { "x-t1-cmex", SingleByteConvert, 0};
static nsFontCharSetInfoXlib CMCMSY =
   { "x-t1-cmsy", SingleByteConvert, 0};
static nsFontCharSetInfoXlib CMCMR =
   { "x-t1-cmr", SingleByteConvert, 0};
static nsFontCharSetInfoXlib CMCMMI =
   { "x-t1-cmmi", SingleByteConvert, 0};
static nsFontCharSetInfoXlib Mathematica1 =
   { "x-mathematica1", SingleByteConvert, 0};
static nsFontCharSetInfoXlib Mathematica2 =
   { "x-mathematica2", SingleByteConvert, 0}; 
static nsFontCharSetInfoXlib Mathematica3 =
   { "x-mathematica3", SingleByteConvert, 0};
static nsFontCharSetInfoXlib Mathematica4 =
   { "x-mathematica4", SingleByteConvert, 0}; 
static nsFontCharSetInfoXlib Mathematica5 =
   { "x-mathematica5", SingleByteConvert, 0};
#endif 
#endif

static nsFontLangGroupXlib FLG_WESTERN = { "x-western",     nsnull };
static nsFontLangGroupXlib FLG_BALTIC  = { "x-baltic",      nsnull };
static nsFontLangGroupXlib FLG_CE      = { "x-central-euro",nsnull };
static nsFontLangGroupXlib FLG_RUSSIAN = { "x-cyrillic",    nsnull };
static nsFontLangGroupXlib FLG_GREEK   = { "el",            nsnull };
static nsFontLangGroupXlib FLG_TURKISH = { "tr",            nsnull };
static nsFontLangGroupXlib FLG_HEBREW  = { "he",            nsnull };
static nsFontLangGroupXlib FLG_ARABIC  = { "ar",            nsnull };
static nsFontLangGroupXlib FLG_THAI    = { "th",            nsnull };
static nsFontLangGroupXlib FLG_ZHCN    = { "zh-CN",         nsnull };
static nsFontLangGroupXlib FLG_ZHTW    = { "zh-TW",         nsnull };
static nsFontLangGroupXlib FLG_ZHHK    = { "zh-HK",         nsnull };
static nsFontLangGroupXlib FLG_ZHTWHK  = { "x-zh-TWHK",     nsnull };
static nsFontLangGroupXlib FLG_JA      = { "ja",            nsnull };
static nsFontLangGroupXlib FLG_KO      = { "ko",            nsnull };
#ifdef SUNCTL
static nsFontLangGroupXlib FLG_INDIC   = { "x-devanagari",  nsnull };
#endif
static nsFontLangGroupXlib FLG_TAMIL   = { "x-tamil",       nsnull };
static nsFontLangGroupXlib FLG_NONE    = { nsnull,          nsnull };



























static const nsFontCharSetMapXlib gConstCharSetMap[] =
{
  { "-ascii",             &FLG_NONE,    &Unknown       },
  { "-ibm pc",            &FLG_NONE,    &Unknown       },
  { "adobe-fontspecific", &FLG_NONE,    &Special       },
  { "ansi-1251",          &FLG_RUSSIAN, &CP1251        },
  
  
  
#ifndef SOLARIS
  { "big5-0",             &FLG_ZHTWHK,  &Big5TWHK      }, 
#else 
  { "big5-0",             &FLG_ZHTW,    &USASCII       }, 
#endif
  { "big5-1",             &FLG_ZHTWHK,  &Big5TWHK      }, 
  { "big5.et-0",          &FLG_ZHTW,    &Big5          },
  { "big5.et.ext-0",      &FLG_ZHTW,    &Big5          },
  { "big5.etext-0",       &FLG_ZHTW,    &Big5          },
  { "big5.hku-0",         &FLG_ZHTW,    &Big5          },
  { "big5.hku-1",         &FLG_ZHTW,    &Big5          },
  { "big5.pc-0",          &FLG_ZHTW,    &Big5          },
  { "big5.shift-0",       &FLG_ZHTW,    &Big5          },
  { "big5hkscs-0",        &FLG_ZHHK,    &HKSCS         },
  { "cns11643.1986-1",    &FLG_ZHTW,    &CNS116431     },
  { "cns11643.1986-2",    &FLG_ZHTW,    &CNS116432     },
  { "cns11643.1992-1",    &FLG_ZHTW,    &CNS116431     },
  { "cns11643.1992.1-0",  &FLG_ZHTW,    &CNS116431     },
  { "cns11643.1992-12",   &FLG_NONE,    &Unknown       },
  { "cns11643.1992.2-0",  &FLG_ZHTW,    &CNS116432     },
  { "cns11643.1992-2",    &FLG_ZHTW,    &CNS116432     },
  { "cns11643.1992-3",    &FLG_ZHTW,    &CNS116433     },
  { "cns11643.1992.3-0",  &FLG_ZHTW,    &CNS116433     },
  { "cns11643.1992.4-0",  &FLG_ZHTW,    &CNS116434     },
  { "cns11643.1992-4",    &FLG_ZHTW,    &CNS116434     },
  { "cns11643.1992.5-0",  &FLG_ZHTW,    &CNS116435     },
  { "cns11643.1992-5",    &FLG_ZHTW,    &CNS116435     },
  { "cns11643.1992.6-0",  &FLG_ZHTW,    &CNS116436     },
  { "cns11643.1992-6",    &FLG_ZHTW,    &CNS116436     },
  { "cns11643.1992.7-0",  &FLG_ZHTW,    &CNS116437     },
  { "cns11643.1992-7",    &FLG_ZHTW,    &CNS116437     },
  { "cns11643-1",         &FLG_ZHTW,    &CNS116431     },
  { "cns11643-2",         &FLG_ZHTW,    &CNS116432     },
  { "cns11643-3",         &FLG_ZHTW,    &CNS116433     },
  { "cns11643-4",         &FLG_ZHTW,    &CNS116434     },
  { "cns11643-5",         &FLG_ZHTW,    &CNS116435     },
  { "cns11643-6",         &FLG_ZHTW,    &CNS116436     },
  { "cns11643-7",         &FLG_ZHTW,    &CNS116437     },
  { "cp1251-1",           &FLG_RUSSIAN, &CP1251        },
  { "dec-dectech",        &FLG_NONE,    &Unknown       },
  { "dtsymbol-1",         &FLG_NONE,    &Unknown       },
  { "fontspecific-0",     &FLG_NONE,    &Unknown       },
  { "gb2312.1980-0",      &FLG_ZHCN,    &GB2312        },
  { "gb2312.1980-1",      &FLG_ZHCN,    &GB2312        },
  { "gb13000.1993-1",     &FLG_ZHCN,    &GBK           },
  { "gb18030.2000-0",     &FLG_ZHCN,    &GB18030_0     },
  { "gb18030.2000-1",     &FLG_ZHCN,    &GB18030_1     },
  { "gbk-0",              &FLG_ZHCN,    &GBK           },
  { "gbk1988.1989-0",     &FLG_ZHCN,    &USASCII       },
  { "hkscs-1",            &FLG_ZHHK,    &HKSCS         },
  { "hp-japanese15",      &FLG_NONE,    &Unknown       },
  { "hp-japaneseeuc",     &FLG_NONE,    &Unknown       },
  { "hp-roman8",          &FLG_NONE,    &Unknown       },
  { "hp-schinese15",      &FLG_NONE,    &Unknown       },
  { "hp-tchinese15",      &FLG_NONE,    &Unknown       },
  { "hp-tchinesebig5",    &FLG_ZHTW,    &Big5          },
  { "hp-wa",              &FLG_NONE,    &Unknown       },
  { "hpbig5-",            &FLG_ZHTW,    &Big5          },
  { "hphkbig5-",          &FLG_ZHHK,    &HKSCS         },
  { "hproc16-",           &FLG_NONE,    &Unknown       },
  { "ibm-1046",           &FLG_ARABIC,  &IBM1046       },
  { "ibm-1252",           &FLG_NONE,    &Unknown       },
  { "ibm-850",            &FLG_NONE,    &Unknown       },
  { "ibm-fontspecific",   &FLG_NONE,    &Unknown       },
  { "ibm-sbdcn",          &FLG_NONE,    &Unknown       },
  { "ibm-sbdtw",          &FLG_NONE,    &Unknown       },
  { "ibm-special",        &FLG_NONE,    &Unknown       },
  { "ibm-udccn",          &FLG_NONE,    &Unknown       },
  { "ibm-udcjp",          &FLG_NONE,    &Unknown       },
  { "ibm-udctw",          &FLG_NONE,    &Unknown       },
  { "iso646.1991-irv",    &FLG_NONE,    &Unknown       },
  { "iso8859-1",          &FLG_WESTERN, &ISO88591      },
  { "iso8859-13",         &FLG_BALTIC,  &ISO885913     },
  { "iso8859-15",         &FLG_WESTERN, &ISO885915     },
  { "iso8859-1@cn",       &FLG_NONE,    &Unknown       },
  { "iso8859-1@kr",       &FLG_NONE,    &Unknown       },
  { "iso8859-1@tw",       &FLG_NONE,    &Unknown       },
  { "iso8859-1@zh",       &FLG_NONE,    &Unknown       },
  { "iso8859-2",          &FLG_CE,      &ISO88592      },
  { "iso8859-3",          &FLG_WESTERN, &ISO88593      },
  { "iso8859-4",          &FLG_BALTIC,  &ISO88594      },
  { "iso8859-5",          &FLG_RUSSIAN, &ISO88595      },
  { "iso8859-6",          &FLG_ARABIC,  &ISO88596      },
  { "iso8859-6.8x",       &FLG_ARABIC,  &ISO885968x    },
  { "iso8859-6.16",       &FLG_ARABIC,  &ISO8859616    },
  { "iso8859-7",          &FLG_GREEK,   &ISO88597      },
  { "iso8859-8",          &FLG_HEBREW,  &ISO88598      },
  { "iso8859-9",          &FLG_TURKISH, &ISO88599      },
  { "iso10646-1",         &FLG_NONE,    &ISO106461     },
  { "jisx0201.1976-0",    &FLG_JA,      &JISX0201      },
  { "jisx0201.1976-1",    &FLG_JA,      &JISX0201      },
  { "jisx0208.1983-0",    &FLG_JA,      &JISX0208      },
  { "jisx0208.1990-0",    &FLG_JA,      &JISX0208      },
  { "jisx0212.1990-0",    &FLG_JA,      &JISX0212      },
  { "koi8-r",             &FLG_RUSSIAN, &KOI8R         },
  { "koi8-u",             &FLG_RUSSIAN, &KOI8U         },
  { "johab-1",            &FLG_KO,      &X11Johab      },
  { "johabs-1",           &FLG_KO,      &X11Johab      },
  { "johabsh-1",          &FLG_KO,      &X11Johab      },
  { "ksc5601.1987-0",     &FLG_KO,      &KSC5601       },
  
  
  { "ksc5601.1987-1",     &FLG_KO,      &KSC5601       },
  { "ksc5601.1992-3",     &FLG_KO,      &JohabNoAscii  },
  { "koreanjamo-0",       &FLG_KO,      &JamoTTF       },
  { "microsoft-cp1250",   &FLG_CE,      &CP1250        },
  { "microsoft-cp1251",   &FLG_RUSSIAN, &CP1251        },
  { "microsoft-cp1252",   &FLG_WESTERN, &CP1252        },
  { "microsoft-cp1253",   &FLG_GREEK,   &CP1253        },
  { "microsoft-cp1257",   &FLG_BALTIC,  &CP1257        },
  { "misc-fontspecific",  &FLG_NONE,    &Unknown       },
  { "sgi-fontspecific",   &FLG_NONE,    &Unknown       },
  { "sun-fontspecific",   &FLG_NONE,    &Unknown       },
  { "sunolcursor-1",      &FLG_NONE,    &Unknown       },
  { "sunolglyph-1",       &FLG_NONE,    &Unknown       },
  { "symbol-fontspecific",&FLG_NONE,    &Special       },
  { "tis620.2529-1",      &FLG_THAI,    &TIS620        },
  { "tis620.2533-0",      &FLG_THAI,    &TIS620        },
  { "tis620.2533-1",      &FLG_THAI,    &TIS620        },
  { "tis620-0",           &FLG_THAI,    &TIS620        },
  { "tis620-2",           &FLG_THAI,    &TIS6202       },
  { "iso8859-11",         &FLG_THAI,    &ISO885911     },
  { "ucs2.cjk-0",         &FLG_NONE,    &ISO106461     },
  { "ucs2.cjk_china-0",   &FLG_ZHCN,    &ISO106461     },
  { "iso10646.2000-cn",   &FLG_ZHCN,    &ISO106461     },  
  { "ucs2.cjk_japan-0",   &FLG_JA,      &ISO106461     },
  { "ucs2.cjk_korea-0",   &FLG_KO,      &ISO106461     },
  { "korean.ucs2-0",      &FLG_KO,      &ISO106461     },  
  { "ucs2.cjk_taiwan-0",  &FLG_ZHTW,    &ISO106461     },
  { "ucs2.thai-0",        &FLG_THAI,    &ISO106461     },
  { "tamilttf-0",         &FLG_TAMIL,   &TamilTTF      },
#ifdef SUNCTL
  { "sun.unicode.india-0",&FLG_INDIC,   &SunIndic      },
#endif 

  { nsnull,               nsnull,       nsnull         }
};

static const nsFontFamilyNameXlib gFamilyNameTable[] =
{
  { "arial",           "helvetica" },
  { "courier new",     "courier" },
  { "times new roman", "times" },

#ifdef MOZ_MATHML
  { "cmex",             "cmex10" },
  { "cmsy",             "cmsy10" },
  { "-moz-math-text",   "times" },
  { "-moz-math-symbol", "symbol" },
#endif 

  { nsnull, nsnull }
};

static const nsFontCharSetMapXlib gConstNoneCharSetMap[] = { { nsnull }, };

static const nsFontCharSetMapXlib gConstSpecialCharSetMap[] =
{
  { "symbol-adobe-fontspecific", &FLG_NONE, &AdobeSymbol  },
  { "euromono-adobe-fontspecific",  &FLG_NONE, &AdobeEuro },
  { "eurosans-adobe-fontspecific",  &FLG_NONE, &AdobeEuro },
  { "euroserif-adobe-fontspecific", &FLG_NONE, &AdobeEuro },

#ifdef MOZ_MATHML
  { "cmex10-adobe-fontspecific", &FLG_NONE, &CMCMEX  },
  { "cmsy10-adobe-fontspecific", &FLG_NONE, &CMCMSY  },
  { "cmr10-adobe-fontspecific",  &FLG_NONE, &CMCMR  },
  { "cmmi10-adobe-fontspecific", &FLG_NONE, &CMCMMI  },

  { "math1-adobe-fontspecific", &FLG_NONE, &Mathematica1 },
  { "math2-adobe-fontspecific", &FLG_NONE, &Mathematica2 },
  { "math3-adobe-fontspecific", &FLG_NONE, &Mathematica3 },
  { "math4-adobe-fontspecific", &FLG_NONE, &Mathematica4 },
  { "math5-adobe-fontspecific", &FLG_NONE, &Mathematica5 },
 
  { "math1mono-adobe-fontspecific", &FLG_NONE, &Mathematica1 },
  { "math2mono-adobe-fontspecific", &FLG_NONE, &Mathematica2 },
  { "math3mono-adobe-fontspecific", &FLG_NONE, &Mathematica3 },
  { "math4mono-adobe-fontspecific", &FLG_NONE, &Mathematica4 },
  { "math5mono-adobe-fontspecific", &FLG_NONE, &Mathematica5 },
#endif 

  { nsnull,                      nsnull        }
};

static const nsFontPropertyNameXlib gStretchNames[] =
{
  { "block",         5 }, 
  { "bold",          7 }, 
  { "double wide",   9 },
  { "medium",        5 },
  { "narrow",        3 },
  { "normal",        5 },
  { "semicondensed", 4 },
  { "wide",          7 },

  { nsnull,          0 }
};

static const nsFontPropertyNameXlib gWeightNames[] =
{
  { "black",    900 },
  { "bold",     700 },
  { "book",     400 },
  { "demi",     600 },
  { "demibold", 600 },
  { "light",    300 },
  { "medium",   400 },
  { "regular",  400 },
  
  { nsnull,     0 }
};

#ifdef DEBUG_copyfontcharsetmap
void DumpFontCharSetMap(const nsFontCharSetMapXlib *fcsm)
{
  printf("fcsm = %lx\n", fcsm);
  printf("{\n");

  for( ; fcsm->mName ; fcsm++ )
  {
#define STRNULL(s) ((s)?(s):("nsnull"))
    printf("  { mName='%s', \tmFontLangGroup=%lx {mFontLangGroupName='%s'}, \tmInfo=%lx {mCharSet='%s'}}\n",
           STRNULL(fcsm->mName),
           fcsm->mFontLangGroup,
           STRNULL(fcsm->mFontLangGroup->mFontLangGroupName),
           fcsm->mInfo,
           STRNULL(fcsm->mInfo->mCharSet)); 
#undef STRNULL
  }
  printf("}\n");
}
#endif 










static
PRBool CopyFontCharSetMapXlib(nsFontMetricsXlibContext *aFmctx)
{
  long                        size1[3],
                              size2[3],
                              size3[3];
  int                         i,
                              j,
                              k,
                              l,
                              count[3];
  char                       *s;
  const nsFontCharSetMapXlib *fcsm[3];
  nsFontCharSetMapXlib       *copy[3];
  nsFontLangGroupXlib        *langgroup;
  nsFontCharSetInfoXlib      *charsetinfo;

  fcsm[0] = gConstCharSetMap;
  fcsm[1] = gConstNoneCharSetMap;
  fcsm[2] = gConstSpecialCharSetMap; 
  
  size1[0] = size2[0] = size3[0] = 0;
  size1[1] = size2[1] = size3[1] = 0;
  size1[2] = size2[2] = size3[2] = 0;







#define MAX_ALIGN_BYTES (sizeof(void *) * 2)
#define ALIGN_PTR(ptr) ((void *)(((PRUptrdiff)(ptr) & ~(MAX_ALIGN_BYTES-1L)) + MAX_ALIGN_BYTES))

  for( l = 0 ; l < 3 ; l++ )
  {  
    
    for( count[l]=0 ; fcsm[l][count[l]].mName ; count[l]++ ) ;

    count[l]++;
    size1[l] = sizeof(nsFontCharSetMapXlib)   * count[l] + MAX_ALIGN_BYTES;
    size2[l] = sizeof(nsFontLangGroupXlib)    * count[l] + MAX_ALIGN_BYTES;
    size3[l] = sizeof(nsFontCharSetInfoXlib)  * count[l] + MAX_ALIGN_BYTES;
    count[l]--;
  }
  
  s = (char *)calloc(1, size1[0]+size2[0]+size3[0]+
                        size1[1]+size2[1]+size3[1]+
                        size1[2]+size2[2]+size3[2]);
  if (!s)
    return PR_FALSE;

  


  copy[0]     = (nsFontCharSetMapXlib *)s;             s += size1[0];
  copy[1]     = (nsFontCharSetMapXlib *)ALIGN_PTR(s);  s += size1[1];
  copy[2]     = (nsFontCharSetMapXlib *)ALIGN_PTR(s);  s += size1[2];
  langgroup   = (nsFontLangGroupXlib  *)ALIGN_PTR(s);  s += size2[0] + size2[1] + size2[2];
  charsetinfo = (nsFontCharSetInfoXlib *)ALIGN_PTR(s);

  for( l = 0 ; l < 3 ; l++ )
  { 
    for( i = 0 ; i < count[l] ; i++ )
    {
      copy[l][i].mName = fcsm[l][i].mName;

      if (!copy[l][i].mFontLangGroup)
      {
        nsFontLangGroupXlib *slot = langgroup++;
        *slot = *fcsm[l][i].mFontLangGroup;
        copy[l][i].mFontLangGroup = slot;

        for( k = 0 ; k < 3 ; k++ )
        {
          for( j = 0 ; j < count[k] ; j++ )
          {
            if ((!copy[k][j].mFontLangGroup) && 
                (fcsm[k][j].mFontLangGroup == fcsm[l][i].mFontLangGroup))
            {
              copy[k][j].mFontLangGroup = slot;
            }
          }
        }
      }

      if (!copy[l][i].mInfo)
      {
        nsFontCharSetInfoXlib *slot = charsetinfo++;

        if (fcsm[l][i].mInfo == &Unknown)
        {
          aFmctx->mUnknown = slot;
        }
        else if (fcsm[l][i].mInfo == &Special)
        {
          aFmctx->mSpecial = slot;
        }
        else if (fcsm[l][i].mInfo == &ISO106461)
        {
          aFmctx->mISO106461 = slot;
        }

        *slot = *fcsm[l][i].mInfo;

        copy[l][i].mInfo = slot;

        for( k = 0 ; k < 3 ; k++ )
        {
          for( j = 0 ; j < count[k] ; j++ )
          {
            if ((!copy[k][j].mInfo) && 
                (fcsm[k][j].mInfo == fcsm[l][i].mInfo))
            {
              copy[k][j].mInfo = slot;
            }
          }
        }
      }
    }
  }
  
  aFmctx->mCharSetMap        = copy[0];
  aFmctx->mNoneCharSetMap    = copy[1];
  aFmctx->mSpecialCharSetMap = copy[2];

#ifdef DEBUG_copyfontcharsetmap
  DumpFontCharSetMap(aFmctx->mCharSetMap);
  DumpFontCharSetMap(aFmctx->mNoneCharSetMap);
  DumpFontCharSetMap(aFmctx->mSpecialCharSetMap);
#endif 
  
  return PR_TRUE; 
}

static char*
atomToName(nsIAtom* aAtom)
{
  const char *namePRU;
  aAtom->GetUTF8String(&namePRU);
  return ToNewCString(nsDependentCString(namePRU));
}






#include "dbyte_special_chars.ccmap"
DEFINE_CCMAP(gDoubleByteSpecialCharsCCMap, const);
	  
static PRBool
FreeCharSetMap(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsFontCharSetMapXlib* charsetMap = (nsFontCharSetMapXlib*) aData;
  NS_IF_RELEASE(charsetMap->mInfo->mConverter);
  NS_IF_RELEASE(charsetMap->mInfo->mLangGroup);
  FreeCCMap(charsetMap->mInfo->mCCMap);

  return PR_TRUE;
}

static PRBool
FreeFamily(nsHashKey* aKey, void* aData, void* aClosure)
{
  delete (nsFontFamilyXlib*) aData;

  return PR_TRUE;
}

static void
FreeStretch(nsFontStretchXlib* aStretch)
{
  PR_smprintf_free(aStretch->mScalable);

  for (PRInt32 count = aStretch->mScaledFonts.Count()-1; count >= 0; --count) {
    nsFontXlib *font = (nsFontXlib*)aStretch->mScaledFonts.ElementAt(count);
    if (font) delete font;
  }
  

  for (int i = 0; i < aStretch->mSizesCount; i++) {
    delete aStretch->mSizes[i];
  }
  delete [] aStretch->mSizes;
  delete aStretch;
}

static void
FreeWeight(nsFontWeightXlib* aWeight)
{
  for (int i = 0; i < 9; i++) {
    if (aWeight->mStretches[i]) {
      for (int j = i + 1; j < 9; j++) {
        if (aWeight->mStretches[j] == aWeight->mStretches[i]) {
          aWeight->mStretches[j] = nsnull;
        }
      }
      FreeStretch(aWeight->mStretches[i]);
    }
  }
  delete aWeight;
}

static void
FreeStyle(nsFontStyleXlib* aStyle)
{
  for (int i = 0; i < 9; i++) {
    if (aStyle->mWeights[i]) {
      for (int j = i + 1; j < 9; j++) {
        if (aStyle->mWeights[j] == aStyle->mWeights[i]) {
          aStyle->mWeights[j] = nsnull;
        }
      }
      FreeWeight(aStyle->mWeights[i]);
    }
  }
  delete aStyle;
}

static 
PRBool
FreeNode(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsFontNodeXlib* node = (nsFontNodeXlib*) aData;
  for (int i = 0; i < 3; i++) {
    if (node->mStyles[i]) {
      for (int j = i + 1; j < 3; j++) {
        if (node->mStyles[j] == node->mStyles[i]) {
          node->mStyles[j] = nsnull;
        }
      }
      FreeStyle(node->mStyles[i]);
    }
  }
  delete node;

  return PR_TRUE;
}

static 
PRBool
FreeNodeArray(nsHashKey* aKey, void* aData, void* aClosure)
{
  nsFontNodeArrayXlib* nodes = (nsFontNodeArrayXlib*) aData;
  delete nodes;

  return PR_TRUE;
}


static nsFontMetricsXlibContext *global_fmctx = nsnull;

nsFontMetricsXlibContext::~nsFontMetricsXlibContext()
{
  PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("# nsFontMetricsXlibContext destroy()\n"));

#ifdef MOZ_ENABLE_FREETYPE2
  nsFreeTypeFreeGlobals();
#endif 

#ifdef ENABLE_X_FONT_BANNING
  if (mFontRejectRegEx) {
    regfree(mFontRejectRegEx);
    delete mFontRejectRegEx;
  }
  
  if (mFontAcceptRegEx) {
    regfree(mFontAcceptRegEx);
    delete mFontAcceptRegEx;
  }  
#endif 

#ifdef USE_AASB
  nsXFontAAScaledBitmap::FreeGlobals();
  nsX11AlphaBlendFreeGlobals();
#endif 

  mCharSetMaps.Reset(FreeCharSetMap, nsnull);
  mFamilies.Reset(FreeFamily, nsnull);
  mCachedFFRESearches.Reset(FreeNodeArray, nsnull); 
  mFFRENodes.Reset(FreeNode, nsnull);
  mAFRENodes.Reset(FreeNode, nsnull);
  mSpecialCharSets.Reset(FreeCharSetMap, nsnull);

  const nsFontCharSetMapXlib* charSetMap;
  for (charSetMap=mCharSetMap; charSetMap->mFontLangGroup; charSetMap++) {
    NS_IF_RELEASE(charSetMap->mFontLangGroup->mFontLangGroupAtom);
  }
  FreeCCMap(mUserDefinedCCMap);
  FreeCCMap(mEmptyCCMap);
  PR_Free(mDoubleByteSpecialCharsCCMap);
  
  
  if (mCharSetMap) {
    free((void *)mCharSetMap);
  }
}





nsresult CreateFontMetricsXlibContext(nsIDeviceContext *aDevice, PRBool aPrintermode, nsFontMetricsXlibContext **aFontMetricsXlibContext)
{
  nsresult                  rv;
  nsFontMetricsXlibContext *fmctx;
  
  *aFontMetricsXlibContext = nsnull;
  
  fmctx = new nsFontMetricsXlibContext();
  if (!fmctx)
    return NS_ERROR_OUT_OF_MEMORY;
  
  rv = fmctx->Init(aDevice, aPrintermode);
  if ((NS_FAILED(rv))) {
    delete fmctx;
    return rv;
  }
  
  *aFontMetricsXlibContext = fmctx;
  
  return rv;
}

void DeleteFontMetricsXlibContext(nsFontMetricsXlibContext *aFontMetricsXlibContext)
{
  if (aFontMetricsXlibContext) {
    delete aFontMetricsXlibContext;
  }
}


nsFontMetricsXlibContext::nsFontMetricsXlibContext()
{
}

nsresult
nsFontMetricsXlibContext::Init(nsIDeviceContext *aDevice, PRBool aPrintermode)
{
  PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("# nsFontMetricsXlibContext new() for device=%p\n", aDevice));

  NS_ENSURE_TRUE(aDevice != nsnull, NS_ERROR_NULL_POINTER);

#ifdef USE_XPRINT
  mPrinterMode = aPrintermode;
#endif 

  mForceOutlineScaledFonts = PR_FALSE;
  mXlibRgbHandle = nsnull;
  mAllowDoubleByteSpecialChars = PR_TRUE;

  

  mDevScale = 0.0f; 
  mScaleBitmapFontsWithDevScale = PR_FALSE;

  mGlobalListInitalised = PR_FALSE;

  mUserDefinedCCMap            = nsnull;
  mEmptyCCMap                  = nsnull;
  mDoubleByteSpecialCharsCCMap = nsnull;

  mCharSetMap        = nsnull;
  mNoneCharSetMap    = nsnull;
  mSpecialCharSetMap = nsnull;
  
  

  mOutlineScaleMinimum = 6;
#ifdef USE_AASB

  mAABitmapScaleEnabled = PR_TRUE;
  mAABitmapScaleAlways = PR_FALSE;
  mAABitmapScaleMinimum = 6;
  mAABitmapOversize = 1.1;
  mAABitmapUndersize = 0.9;
#endif 


  mBitmapScaleMinimum = 10;
  mBitmapOversize = 1.2;
  mBitmapUndersize = 0.8;

#ifdef USE_AASB
  mAntiAliasMinimum = 8;
#endif 
  mEmbeddedBitmapMaximumHeight = 1000000;

#ifdef MOZ_ENABLE_FREETYPE2
  mEnableFreeType2 = PR_TRUE;
  mFreeType2Autohinted = PR_FALSE;
  mFreeType2Unhinted = PR_TRUE;
#endif 
#ifdef USE_AASB
  mAATTDarkTextMinValue = 64;
  mAATTDarkTextGain = 0.8;
#endif 

#ifdef ENABLE_X_FONT_BANNING
  mFontRejectRegEx = nsnull;
  mFontAcceptRegEx = nsnull;
#endif 

  PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("## CopyFontCharSetMapXlib start.\n"));

  if (!CopyFontCharSetMapXlib(this)) {
    PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("## CopyFontCharSetMapXlib FAILED.\n"));
    return NS_ERROR_OUT_OF_MEMORY;
  }
  PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("## CopyFontCharSetMapXlib done.\n"));

#ifdef NS_FONT_DEBUG
  



  const char *debug;
#ifdef USE_XPRINT
  if (mPrinterMode) {
    debug = PR_GetEnv("NS_FONT_DEBUG_XPRINT");
  }
  else
#endif 
  {
    debug = PR_GetEnv("NS_FONT_DEBUG_XLIB");
  }

  if (!debug) {
    debug = PR_GetEnv("NS_FONT_DEBUG");
  }
  
  if (debug) {
    PR_sscanf(debug, "%lX", &gFontDebug);
  }
#endif 

  NS_STATIC_CAST(nsDeviceContextX *, aDevice)->GetXlibRgbHandle(mXlibRgbHandle);

  aDevice->GetCanonicalPixelScale(mDevScale);

  mCharSetManager = do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID);
  if (!mCharSetManager) {
    return NS_ERROR_FAILURE;
  }
    
  mPref = do_GetService(NS_PREF_CONTRACTID);  
  if (!mPref) {
    return NS_ERROR_FAILURE;
  }

  nsCompressedCharMap empty_ccmapObj;
  mEmptyCCMap = empty_ccmapObj.NewCCMap();
  if (!mEmptyCCMap)
    return NS_ERROR_OUT_OF_MEMORY;

  
  PRBool val = PR_TRUE;
  nsresult rv = mPref->GetBoolPref("font.allow_double_byte_special_chars", &val);
  if (NS_SUCCEEDED(rv))
    mAllowDoubleByteSpecialChars = val;

  
  PRUint32 dbmapSize = sizeof(gDoubleByteSpecialCharsCCMapUnion);
  mDoubleByteSpecialCharsCCMap = (PRUint16*)PR_Malloc(dbmapSize);
  if (!mDoubleByteSpecialCharsCCMap)
    return NS_ERROR_OUT_OF_MEMORY;
  memcpy(mDoubleByteSpecialCharsCCMap, gDoubleByteSpecialCharsCCMap, dbmapSize);

  PRInt32 scale_minimum = 0;
  rv = mPref->GetIntPref("font.scale.outline.min", &scale_minimum);
  if (NS_SUCCEEDED(rv)) {
    mOutlineScaleMinimum = scale_minimum;
    SIZE_FONT_PRINTF(("mOutlineScaleMinimum = %d", mOutlineScaleMinimum));
  }

  PRInt32 int_val = 0;
  PRInt32 percent = 0;
#ifdef USE_AASB
  val = PR_TRUE;
  rv = mPref->GetBoolPref("font.scale.aa_bitmap.enable", &val);
  if (NS_SUCCEEDED(rv)) {
    mAABitmapScaleEnabled = val;
    SIZE_FONT_PRINTF(("mAABitmapScaleEnabled = %d", mAABitmapScaleEnabled));
  }

  val = PR_FALSE;
  rv = mPref->GetBoolPref("font.scale.aa_bitmap.always", &val);
  if (NS_SUCCEEDED(rv)) {
    mAABitmapScaleAlways = val;
    SIZE_FONT_PRINTF(("mAABitmapScaleAlways = %d", mAABitmapScaleAlways));
  }

  rv = mPref->GetIntPref("font.scale.aa_bitmap.min", &scale_minimum);
  if (NS_SUCCEEDED(rv)) {
    mAABitmapScaleMinimum = scale_minimum;
    SIZE_FONT_PRINTF(("mAABitmapScaleMinimum = %d", mAABitmapScaleMinimum));
  }

  percent = 0;
  rv = mPref->GetIntPref("font.scale.aa_bitmap.undersize", &percent);
  if ((NS_SUCCEEDED(rv)) && (percent)) {
    mAABitmapUndersize = percent/100.0;
    SIZE_FONT_PRINTF(("mAABitmapUndersize = %g", mAABitmapUndersize));
  }
  percent = 0;
  rv = mPref->GetIntPref("font.scale.aa_bitmap.oversize", &percent);
  if ((NS_SUCCEEDED(rv)) && (percent)) {
    mAABitmapOversize = percent/100.0;
    SIZE_FONT_PRINTF(("mAABitmapOversize = %g", mAABitmapOversize));
  }
  int_val = 0;
  rv = mPref->GetIntPref("font.scale.aa_bitmap.dark_text.min", &int_val);
  if (NS_SUCCEEDED(rv)) {
    gAASBDarkTextMinValue = int_val;
    SIZE_FONT_PRINTF(("gAASBDarkTextMinValue = %d", gAASBDarkTextMinValue));
  }
  nsXPIDLCString str;
  rv = mPref->GetCharPref("font.scale.aa_bitmap.dark_text.gain",
                           getter_Copies(str));
  if (NS_SUCCEEDED(rv)) {
    gAASBDarkTextGain = atof(str.get());
    SIZE_FONT_PRINTF(("gAASBDarkTextGain = %g", gAASBDarkTextGain));
  }
  int_val = 0;
  rv = mPref->GetIntPref("font.scale.aa_bitmap.light_text.min", &int_val);
  if (NS_SUCCEEDED(rv)) {
    gAASBLightTextMinValue = int_val;
    SIZE_FONT_PRINTF(("gAASBLightTextMinValue = %d", gAASBLightTextMinValue));
  }
  rv = mPref->GetCharPref("font.scale.aa_bitmap.light_text.gain",
                           getter_Copies(str));
  if (NS_SUCCEEDED(rv)) {
    gAASBLightTextGain = atof(str.get());
    SIZE_FONT_PRINTF(("gAASBLightTextGain = %g", gAASBLightTextGain));
  }
#endif 

  rv = mPref->GetIntPref("font.scale.bitmap.min", &scale_minimum);
  if (NS_SUCCEEDED(rv)) {
    mBitmapScaleMinimum = scale_minimum;
    SIZE_FONT_PRINTF(("mBitmapScaleMinimum = %d", mBitmapScaleMinimum));
  }
  percent = 0;
  mPref->GetIntPref("font.scale.bitmap.oversize", &percent);
  if (percent) {
    mBitmapOversize = percent/100.0;
    SIZE_FONT_PRINTF(("mBitmapOversize = %g", mBitmapOversize));
  }
  percent = 0;
  mPref->GetIntPref("font.scale.bitmap.undersize", &percent);
  if (percent) {
    mBitmapUndersize = percent/100.0;
    SIZE_FONT_PRINTF(("mBitmapUndersize = %g", mBitmapUndersize));
  }

#ifdef USE_XPRINT
  if (mPrinterMode) {
    mForceOutlineScaledFonts = PR_TRUE;
  }
#endif 

 PRBool force_outline_scaled_fonts = mForceOutlineScaledFonts;
#ifdef USE_XPRINT
  if (mPrinterMode) {
    rv = mPref->GetBoolPref("print.xprint.font.force_outline_scaled_fonts", &force_outline_scaled_fonts);
  }  
  if (!mPrinterMode || NS_FAILED(rv)) {
#endif
    rv = mPref->GetBoolPref("font.x11.force_outline_scaled_fonts", &force_outline_scaled_fonts);
#ifdef USE_XPRINT
  }
#endif 
  if (NS_SUCCEEDED(rv)) {
    mForceOutlineScaledFonts = force_outline_scaled_fonts;
  }

#ifdef MOZ_ENABLE_FREETYPE2
  PRBool enable_freetype2 = PR_TRUE;
  rv = mPref->GetBoolPref("font.FreeType2.enable", &enable_freetype2);
  if (NS_SUCCEEDED(rv)) {
    mEnableFreeType2 = enable_freetype2;
    FREETYPE_FONT_PRINTF(("mEnableFreeType2 = %d", mEnableFreeType2));
  }

  PRBool freetype2_autohinted = PR_FALSE;
  rv = mPref->GetBoolPref("font.FreeType2.autohinted", &freetype2_autohinted);
  if (NS_SUCCEEDED(rv)) {
    mFreeType2Autohinted = freetype2_autohinted;
    FREETYPE_FONT_PRINTF(("mFreeType2Autohinted = %d", mFreeType2Autohinted));
  }

  PRBool freetype2_unhinted = PR_TRUE;
  rv = mPref->GetBoolPref("font.FreeType2.unhinted", &freetype2_unhinted);
  if (NS_SUCCEEDED(rv)) {
    mFreeType2Unhinted = freetype2_unhinted;
    FREETYPE_FONT_PRINTF(("mFreeType2Unhinted = %d", mFreeType2Unhinted));
  }
#endif 

#ifdef USE_AASB
  PRInt32 antialias_minimum = 8;
  rv = mPref->GetIntPref("font.antialias.min", &antialias_minimum);
  if (NS_SUCCEEDED(rv)) {
    mAntiAliasMinimum = antialias_minimum;
    FREETYPE_FONT_PRINTF(("mAntiAliasMinimum = %d", mAntiAliasMinimum));
  }
#endif 

  PRInt32 embedded_bitmaps_maximum = 1000000;
  rv = mPref->GetIntPref("font.embedded_bitmaps.max",&embedded_bitmaps_maximum);
  if (NS_SUCCEEDED(rv)) {
    mEmbeddedBitmapMaximumHeight = embedded_bitmaps_maximum;
    FREETYPE_FONT_PRINTF(("mEmbeddedBitmapMaximumHeight = %d",
                          mEmbeddedBitmapMaximumHeight));
  }
  int_val = 0;
#ifdef USE_AASB
  rv = mPref->GetIntPref("font.scale.tt_bitmap.dark_text.min", &int_val);
  if (NS_SUCCEEDED(rv)) {
    mAATTDarkTextMinValue = int_val;
    SIZE_FONT_PRINTF(("mAATTDarkTextMinValue = %d", mAATTDarkTextMinValue));
  }
  rv = mPref->GetCharPref("font.scale.tt_bitmap.dark_text.gain",
                           getter_Copies(str));
  if (NS_SUCCEEDED(rv)) {
    mAATTDarkTextGain = atof(str.get());
    SIZE_FONT_PRINTF(("mAATTDarkTextGain = %g", mAATTDarkTextGain));
  }
#endif 

#ifdef USE_XPRINT
  if (mPrinterMode) {
    mScaleBitmapFontsWithDevScale = PR_TRUE;
  }
#endif 

 PRBool scale_bitmap_fonts_with_devscale = mScaleBitmapFontsWithDevScale;
#ifdef USE_XPRINT
  if (mPrinterMode) {
    rv = mPref->GetBoolPref("print.xprint.font.scale_bitmap_fonts_with_devscale", &scale_bitmap_fonts_with_devscale);
  }  
  if (!mPrinterMode || NS_FAILED(rv)) {
#endif
    rv = mPref->GetBoolPref("font.x11.scale_bitmap_fonts_with_devscale", &scale_bitmap_fonts_with_devscale);
#ifdef USE_XPRINT
  }
#endif 
  if (NS_SUCCEEDED(rv)) {
    mScaleBitmapFontsWithDevScale = scale_bitmap_fonts_with_devscale;
  }

  const nsFontFamilyNameXlib* f = gFamilyNameTable;
  while (f->mName) {
    nsCStringKey key(f->mName);
    mAliases.Put(&key, (void *)f->mXName);
    f++;
  }

  const nsFontPropertyNameXlib* p = gWeightNames;
  while (p->mName) {
    nsCStringKey key(p->mName);
    mWeights.Put(&key, (void*) p->mValue);
    p++;
  }

  p = gStretchNames;
  while (p->mName) {
    nsCStringKey key(p->mName);
    mStretches.Put(&key, (void*) p->mValue);
    p++;
  }

  const nsFontCharSetMapXlib* charSetMap = mCharSetMap;
  while (charSetMap->mName) {
    nsCStringKey key(charSetMap->mName);
    mCharSetMaps.Put(&key, (void *)charSetMap);
    charSetMap++;
  }

  const nsFontCharSetMapXlib* specialCharSetMap = mSpecialCharSetMap;
  while (specialCharSetMap->mName) {
    nsCStringKey key(specialCharSetMap->mName);
    mSpecialCharSets.Put(&key, (void *)specialCharSetMap);
    specialCharSetMap++;
  }

  mUnicode = do_GetAtom("x-unicode");
  if (!mUnicode) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mUserDefined = do_GetAtom(USER_DEFINED);
  if (!mUserDefined) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mZHTW = do_GetAtom("zh-TW");
  if (!mZHTW) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mZHHK = do_GetAtom("zh-HK");
  if (!mZHHK) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  mZHTWHK = do_GetAtom("x-zh-TWHK");
  if (!mZHTWHK) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsCOMPtr<nsILanguageAtomService> langService;
  langService = do_GetService(NS_LANGUAGEATOMSERVICE_CONTRACTID);
  if (langService) {
    mUsersLocale = langService->GetLocaleLanguageGroup();
  }
  if (!mUsersLocale) {
    mUsersLocale = do_GetAtom("x-western");
  }
  mWesternLocale = do_GetAtom("x-western");
  if (!mUsersLocale) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

#ifdef USE_AASB
  rv = nsX11AlphaBlendInitGlobals(xxlib_rgb_get_display(mXlibRgbHandle));
  if (NS_FAILED(rv) || (!nsX11AlphaBlend::CanAntiAlias())) {
    mAABitmapScaleEnabled = PR_FALSE;
  }

  if (mAABitmapScaleEnabled) {
      mAABitmapScaleEnabled = nsXFontAAScaledBitmap::InitGlobals(xxlib_rgb_get_display(mXlibRgbHandle),
                                                                 xxlib_rgb_get_screen(mXlibRgbHandle));
  }
#endif 
  
#ifdef ENABLE_X_FONT_BANNING
  
  nsXPIDLCString fbpattern;
#ifdef USE_XPRINT
  if (mPrinterMode) {
    rv = mPref->GetCharPref("print.xprint.font.rejectfontpattern", getter_Copies(fbpattern));
  }  
  if (!mPrinterMode || NS_FAILED(rv)) {
#endif
    rv = mPref->GetCharPref("font.x11.rejectfontpattern", getter_Copies(fbpattern));
#ifdef USE_XPRINT
  }
#endif 
  if (NS_SUCCEEDED(rv)) {
    mFontRejectRegEx = new regex_t;
    if (!mFontRejectRegEx) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    if (regcomp(mFontRejectRegEx, fbpattern.get(), REG_EXTENDED|REG_NOSUB) != REG_OK) {
      PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("Invalid rejectfontpattern '%s'\n", fbpattern.get()));
      BANNED_FONT_PRINTF(("Invalid font.x11.rejectfontpattern '%s'", fbpattern.get()));
      delete mFontRejectRegEx;
      mFontRejectRegEx = nsnull;
      
      return NS_ERROR_INVALID_ARG;
    }    
  }

#ifdef USE_XPRINT
  if (mPrinterMode) {
    rv = mPref->GetCharPref("print.xprint.font.acceptfontpattern", getter_Copies(fbpattern));
  }  
  if (!mPrinterMode || NS_FAILED(rv)) {
#endif
    rv = mPref->GetCharPref("font.x11.acceptfontpattern", getter_Copies(fbpattern));
#ifdef USE_XPRINT
  }
#endif 
  if (NS_SUCCEEDED(rv)) {
    mFontAcceptRegEx = new regex_t;
    if (!mFontAcceptRegEx) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    
    
    if (regcomp(mFontAcceptRegEx, fbpattern.get(), REG_EXTENDED|REG_NOSUB) != REG_OK) {
      PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("Invalid acceptfontpattern '%s'\n", fbpattern.get()));
      BANNED_FONT_PRINTF(("Invalid font.x11.acceptfontpattern '%s'", fbpattern.get()));
      delete mFontAcceptRegEx;
      mFontAcceptRegEx = nsnull;
      
      return NS_ERROR_INVALID_ARG;
    }    
  }
#endif

#ifdef MOZ_ENABLE_FREETYPE2
  rv = nsFreeTypeInitGlobals();
  if (NS_FAILED(rv)) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
#endif 

  return NS_OK;
}

#ifdef USE_X11SHARED_CODE
#error not implemented yet
#else
void
nsXFontNormal::DrawText8(Drawable aDrawable, GC aGC,
                         PRInt32 aX, PRInt32 aY,
                         const char *aString, PRUint32 aLength)
{
  XDrawString(mDisplay, aDrawable, aGC, aX, aY, aString, PR_MIN(aLength, 32767));
}

void
nsXFontNormal::DrawText16(Drawable aDrawable, GC aGC,
                          PRInt32 aX, PRInt32 aY,
                          const XChar2b *aString, PRUint32 aLength)
{
  XDrawString16(mDisplay, aDrawable, aGC, aX, aY, aString, PR_MIN(aLength, 32767));
}

PRBool
nsXFontNormal::GetXFontProperty(Atom aAtom, unsigned long *aValue)
{
  NS_ASSERTION(mXFont, "GetXFontProperty called before font loaded");
  if (mXFont==nsnull)
    return PR_FALSE;

  return ::XGetFontProperty(mXFont, aAtom, aValue);
}

XFontStruct *
nsXFontNormal::GetXFontStruct()
{
  NS_ASSERTION(mXFont, "GetXFontStruct called before font loaded");
  return mXFont;
}

PRBool
nsXFontNormal::LoadFont()
{
  if (!mXFont)
    return PR_FALSE;
  mIsSingleByte = (mXFont->min_byte1 == 0) && (mXFont->max_byte1 == 0);
  return PR_TRUE;
}

nsXFontNormal::nsXFontNormal(Display *aDisplay, XFontStruct *aXFont)
{
  mDisplay = aDisplay;
  mXFont   = aXFont;
}

void
nsXFontNormal::TextExtents8(const char *aString, PRUint32 aLength,
                            PRInt32* aLBearing, PRInt32* aRBearing,
                            PRInt32* aWidth, PRInt32* aAscent,
                            PRInt32* aDescent)
{
  XCharStruct overall;
  int direction, font_ascent, font_descent;
  ::XTextExtents(mXFont, aString, aLength,
                 &direction, &font_ascent, &font_descent,
                 &overall);

  *aLBearing = overall.lbearing;
  *aRBearing = overall.rbearing;
  *aWidth    = overall.width;
  *aAscent   = overall.ascent;
  *aDescent  = overall.descent;
}

void
nsXFontNormal::TextExtents16(const XChar2b *aString, PRUint32 aLength,
                            PRInt32* aLBearing, PRInt32* aRBearing,
                            PRInt32* aWidth, PRInt32* aAscent,
                            PRInt32* aDescent)
{
  XCharStruct overall;
  int direction, font_ascent, font_descent;
  ::XTextExtents16(mXFont, aString, aLength,
                   &direction, &font_ascent, &font_descent,
                   &overall);

  *aLBearing = overall.lbearing;
  *aRBearing = overall.rbearing;
  *aWidth    = overall.width;
  *aAscent   = overall.ascent;
  *aDescent  = overall.descent;
}

PRInt32
nsXFontNormal::TextWidth8(const char *aString, PRUint32 aLength)
{
  NS_ASSERTION(mXFont, "TextWidth8 called before font loaded");
  if (mXFont==nsnull)
    return 0;
  PRInt32 width = ::XTextWidth(mXFont, aString, aLength);
  return width;
}

PRInt32
nsXFontNormal::TextWidth16(const XChar2b *aString, PRUint32 aLength)
{
  NS_ASSERTION(mXFont, "TextWidth16 called before font loaded");
  if (mXFont==nsnull)
    return 0;
  PRInt32 width = ::XTextWidth16(mXFont, aString, aLength);
  
  return width;
}

void
nsXFontNormal::UnloadFont()
{
  delete this;
}

nsXFontNormal::~nsXFontNormal()
{
}
#endif 

nsFontMetricsXlib::nsFontMetricsXlib()
  : mFonts() 
  
  
  
  
{
}

nsFontMetricsXlib::~nsFontMetricsXlib()
{
  

  if (mLoadedFonts) {
    PR_Free(mLoadedFonts);
    mLoadedFonts = nsnull;
  }

  if (mSubstituteFont) {
    delete mSubstituteFont;
    mSubstituteFont = nsnull;
  }

  mWesternFont = nsnull;

  if (mDeviceContext) {
    
    mDeviceContext->FontMetricsDeleted(this);
    mDeviceContext = nsnull;
  }
}

NS_IMPL_ISUPPORTS1(nsFontMetricsXlib, nsIFontMetrics)

static PRBool
IsASCIIFontName(const nsString& aName)
{
  PRUint32 len = aName.Length();
  const PRUnichar* str = aName.get();
  for (PRUint32 i = 0; i < len; i++) {
    


    if ((str[i] < 0x20) || (str[i] > 0x7E)) {
      return PR_FALSE;
    }
  }

  return PR_TRUE;
}

static PRBool
FontEnumCallback(const nsString& aFamily, PRBool aGeneric, void *aData)
{
#ifdef REALLY_NOISY_FONTS
  printf("font = '");
  fputs(NS_LossyConvertUTF16toASCII(aFamily).get(), stdout);
  printf("'\n");
#endif

  if (!IsASCIIFontName(aFamily)) {
    return PR_TRUE; 
  }

  nsCAutoString name;
  name.AssignWithConversion(aFamily.get());
  ToLowerCase(name);
  nsFontMetricsXlib* metrics = (nsFontMetricsXlib*) aData;
  metrics->mFonts.AppendCString(name);
  metrics->mFontIsGeneric.AppendElement((void*) aGeneric);
  if (aGeneric) {
    metrics->mGeneric = metrics->mFonts.CStringAt(metrics->mFonts.Count() - 1);
    return PR_FALSE; 
  }

  return PR_TRUE; 
}

NS_IMETHODIMP nsFontMetricsXlib::Init(const nsFont& aFont, nsIAtom* aLangGroup,
  nsIDeviceContext* aContext)
{
  NS_ASSERTION(!(nsnull == aContext), "attempt to init fontmetrics with null device context");

  nsresult res;
  mDocConverterType = nsnull;
  
  mDeviceContext = aContext;
 
  NS_STATIC_CAST(nsDeviceContextX *, mDeviceContext)->GetFontMetricsContext(mFontMetricsContext);
  
  mFont = aFont;
  mLangGroup = aLangGroup;

  float app2dev;
  app2dev = mDeviceContext->AppUnitsToDevUnits();

  mPixelSize = NSToIntRound(app2dev * mFont.size);
  
  
  mPixelSize = PR_MIN(XHeightOfScreen(xxlib_rgb_get_screen(mFontMetricsContext->mXlibRgbHandle)) * FONT_MAX_FONT_SCALE, mPixelSize);

  mStretchIndex = 4; 
  mStyleIndex = mFont.style;

  mFont.EnumerateFamilies(FontEnumCallback, this);
  nsXPIDLCString value;
  const char* langGroup = nsnull;
  mLangGroup->GetUTF8String(&langGroup);
  if (!mGeneric) {
    nsCAutoString name("font.default.");
    name.Append(langGroup);
    mFontMetricsContext->mPref->CopyCharPref(name.get(), getter_Copies(value));
    if (value.get()) {
      mDefaultFont = value.get();
    }
    else {
      mDefaultFont = "serif";
    }
    mGeneric = &mDefaultFont;
  }

  if (mLangGroup) {
    nsCAutoString name("font.min-size.");
    if (mGeneric->Equals("monospace")) {
      name.Append("fixed");
    }
    else {
      name.Append("variable");
    }
    name.Append(char('.'));
    name.Append(langGroup);
    PRInt32 minimum = 0;
    res = mFontMetricsContext->mPref->GetIntPref(name.get(), &minimum);
    if (NS_FAILED(res)) {
      mFontMetricsContext->mPref->GetDefaultIntPref(name.get(), &minimum);
    }
    if (minimum < 0) {
      minimum = 0;
    }
    if (mPixelSize < minimum) {
      mPixelSize = minimum;
    }
  }

  if (mLangGroup.get() == mFontMetricsContext->mUserDefined) {
    if (!mFontMetricsContext->mUserDefinedConverter) {
        nsIUnicodeEncoder *ud_conv;
        res = mFontMetricsContext->mCharSetManager->GetUnicodeEncoderRaw("x-user-defined", &ud_conv);
        if (NS_SUCCEEDED(res)) {
          mFontMetricsContext->mUserDefinedConverter = ud_conv;
          res = mFontMetricsContext->mUserDefinedConverter->SetOutputErrorBehavior(
            mFontMetricsContext->mUserDefinedConverter->kOnError_Replace, nsnull, '?');
          nsCOMPtr<nsICharRepresentable> mapper =
            do_QueryInterface(mFontMetricsContext->mUserDefinedConverter);
          if (mapper) {
            mFontMetricsContext->mUserDefinedCCMap = MapperToCCMap(mapper);
            if (!mFontMetricsContext->mUserDefinedCCMap)
              return NS_ERROR_OUT_OF_MEMORY;          
          }
        }
        else {
          return res;
        }
    }

    nsCAutoString name("font.name.");
    name.Append(*mGeneric);
    name.Append(char('.'));
    name.Append(USER_DEFINED);
    mFontMetricsContext->mPref->CopyCharPref(name.get(), getter_Copies(value));
    if (value.get()) {
      mUserDefined = value.get();
      mIsUserDefined = 1;
    }
  }

  mWesternFont = FindFont('a');
  if (!mWesternFont)
    return NS_ERROR_FAILURE;

  RealizeFont();

  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::Destroy()
{
  mDeviceContext = nsnull;
  mFontMetricsContext = nsnull;
  return NS_OK;
}

void nsFontMetricsXlib::RealizeFont()
{
  float f;
  f = mDeviceContext->DevUnitsToAppUnits();

#ifdef MOZ_ENABLE_FREETYPE2
  if (mWesternFont->IsFreeTypeFont()) {
    nsFreeTypeFont *ft = (nsFreeTypeFont *)mWesternFont;
    if (!ft)
      return;
    
    
    
    
#ifdef MOZ_ENABLE_FREETYPE2
    int lineSpacing = ft->ascent() + ft->descent();
    if (lineSpacing > mWesternFont->mSize) {
      mLeading = nscoord((lineSpacing - mWesternFont->mSize) * f);
    }
    else {
      mLeading = 0;
    }
    mEmHeight = PR_MAX(1, nscoord(mWesternFont->mSize * f));
    mEmAscent = nscoord(ft->ascent() * mWesternFont->mSize * f / lineSpacing);
    mEmDescent = mEmHeight - mEmAscent;

    mMaxHeight  = nscoord((ft->max_ascent() + ft->max_descent()) * f);
    mMaxAscent  = nscoord(ft->max_ascent() * f) ;
    mMaxDescent = nscoord(ft->max_descent() * f);

    mMaxAdvance = nscoord(ft->max_width() * f);

    
    mXHeight = NSToCoordRound((float) ft->ascent()* f * 0.56f);

    PRUnichar space = (PRUnichar)' ';
    mSpaceWidth = NSToCoordRound(ft->GetWidth(&space, 1) * f);

    PRUnichar averageX = (PRUnichar)'x';
    mAveCharWidth = NSToCoordRound(ft->GetWidth(&averageX, 1) * f);

    unsigned long pr = 0;
    if (ft->getXHeight(pr)) {
      mXHeight = nscoord(pr * f);
    }

    float height;
    long val;
    if (ft->underlinePosition(val)) {
      

      mUnderlineOffset = -NSToIntRound(val * f);
    }
    else {
      height = ft->ascent() + ft->descent();
      mUnderlineOffset = -NSToIntRound(PR_MAX (1, floor (0.1 * height + 0.5)) * f);
    }

    if (ft->underline_thickness(pr)) {
      
      mUnderlineSize = nscoord(PR_MAX(f, NSToIntRound(pr * f)));
    }
    else {
      height = ft->ascent() + ft->descent();
      mUnderlineSize = NSToIntRound(PR_MAX(1, floor (0.05 * height + 0.5)) * f);
    }

    if (ft->superscript_y(val)) {
      mSuperscriptOffset = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
      mSuperscriptOffset = mXHeight;
    }

    if (ft->subscript_y(val)) {
      mSubscriptOffset = nscoord(PR_MAX(f, NSToIntRound(val * f)));
    }
    else {
     mSubscriptOffset = mXHeight;
    }

    
    mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0);
    mStrikeoutSize = mUnderlineSize;

    return;
#endif 
  }
#endif 
  nsXFont *xFont = mWesternFont->GetXFont();
  XFontStruct *fontInfo = xFont->GetXFontStruct();
  f = mDeviceContext->DevUnitsToAppUnits();

  nscoord lineSpacing = nscoord((fontInfo->ascent + fontInfo->descent) * f);
  mEmHeight = PR_MAX(1, nscoord(mWesternFont->mSize * f));
  if (lineSpacing > mEmHeight) {
    mLeading = lineSpacing - mEmHeight;
  }
  else {
    mLeading = 0;
  }
  mMaxHeight = nscoord((fontInfo->ascent + fontInfo->descent) * f);
  mMaxAscent = nscoord(fontInfo->ascent * f);
  mMaxDescent = nscoord(fontInfo->descent * f);

  mEmAscent = nscoord(mMaxAscent * mEmHeight / lineSpacing);
  mEmDescent = mEmHeight - mEmAscent;

  mMaxAdvance = nscoord(fontInfo->max_bounds.width * f);

  int rawWidth, rawAverage;
  if ((fontInfo->min_byte1 == 0) && (fontInfo->max_byte1 == 0)) {
    rawWidth = xFont->TextWidth8(" ", 1);
    rawAverage = xFont->TextWidth8("x", 1);
  }
  else {
    XChar2b my16bit_space, my16bit_x;
    my16bit_space.byte1 = '\0';
    my16bit_space.byte2 = ' ';
    my16bit_x.byte1     = 0;
    my16bit_x.byte2     = 'x';
    rawWidth   = xFont->TextWidth16(&my16bit_space, 1);
    rawAverage = xFont->TextWidth16(&my16bit_x,     1);
  }
  mSpaceWidth = NSToCoordRound(rawWidth * f);
  mAveCharWidth = NSToCoordRound(rawAverage * f);

  unsigned long pr = 0;
  if (xFont->GetXFontProperty(XA_X_HEIGHT, &pr) && pr != 0 &&
      pr < 0x00ffffff)  
  {
    mXHeight = nscoord(pr * f);
#ifdef REALLY_NOISY_FONTS
    printf("xHeight=%d\n", mXHeight);
#endif
  }
  else 
  {
    
    mXHeight = NSToCoordRound((float) fontInfo->ascent* f * 0.56f);
  }

  if (xFont->GetXFontProperty(XA_UNDERLINE_POSITION, &pr))
  {
    

    mUnderlineOffset = -NSToIntRound(pr * f);
#ifdef REALLY_NOISY_FONTS
    printf("underlineOffset=%d\n", mUnderlineOffset);
#endif
  }
  else
  {
    
    float height;
    height = fontInfo->ascent + fontInfo->descent;
    mUnderlineOffset = -NSToIntRound(PR_MAX (1, floor (0.1 * height + 0.5)) * f);
  }

  if (xFont->GetXFontProperty(XA_UNDERLINE_THICKNESS, &pr))
  {
    
    mUnderlineSize = nscoord(PR_MAX(f, NSToIntRound(pr * f)));
#ifdef REALLY_NOISY_FONTS
    printf("underlineSize=%d\n", mUnderlineSize);
#endif
  }
  else
  {
    float height;
    height = fontInfo->ascent + fontInfo->descent;
    mUnderlineSize = NSToIntRound(PR_MAX(1, floor (0.05 * height + 0.5)) * f);
  }

  if (xFont->GetXFontProperty(XA_SUPERSCRIPT_Y, &pr))
  {
    mSuperscriptOffset = nscoord(PR_MAX(f, NSToIntRound(pr * f)));
#ifdef REALLY_NOISY_FONTS
    printf("superscriptOffset=%d\n", mSuperscriptOffset);
#endif
  }
  else
  {
    mSuperscriptOffset = mXHeight;
  }

  if (xFont->GetXFontProperty(XA_SUBSCRIPT_Y, &pr))
  {
    mSubscriptOffset = nscoord(PR_MAX(f, NSToIntRound(pr * f)));
#ifdef REALLY_NOISY_FONTS
    printf("subscriptOffset=%d\n", mSubscriptOffset);
#endif
  }
  else
  {
    mSubscriptOffset = mXHeight;
  }

  
  mStrikeoutOffset = NSToCoordRound(mXHeight / 2.0);
  mStrikeoutSize = mUnderlineSize;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetXHeight(nscoord& aResult)
{
  aResult = mXHeight;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetSuperscriptOffset(nscoord& aResult)
{
  aResult = mSuperscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetSubscriptOffset(nscoord& aResult)
{
  aResult = mSubscriptOffset;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetStrikeout(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mStrikeoutOffset;
  aSize = mStrikeoutSize;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetUnderline(nscoord& aOffset, nscoord& aSize)
{
  aOffset = mUnderlineOffset;
  aSize = mUnderlineSize;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

#ifdef FONT_LEADING_APIS_V2
NS_IMETHODIMP  nsFontMetricsXlib::GetInternalLeading(nscoord &aLeading)
{
  aLeading = mLeading;
  return NS_OK;
}
NS_IMETHODIMP  nsFontMetricsXlib::GetExternalLeading(nscoord &aLeading)
{
  aLeading = 0;
  return NS_OK;
}
#else

NS_IMETHODIMP  nsFontMetricsXlib::GetNormalLineHeight(nscoord &aHeight)
{
  aHeight = mEmHeight + mLeading;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetLeading(nscoord &aLeading)
{
  aLeading = mLeading;
  return NS_OK;
}
#endif

NS_IMETHODIMP  nsFontMetricsXlib::GetEmHeight(nscoord &aHeight)
{
  aHeight = mEmHeight;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetEmAscent(nscoord &aAscent)
{
  aAscent = mEmAscent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetEmDescent(nscoord &aDescent)
{
  aDescent = mEmDescent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetMaxHeight(nscoord &aHeight)
{
  aHeight = mMaxHeight;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetMaxAscent(nscoord &aAscent)
{
  aAscent = mMaxAscent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetMaxDescent(nscoord &aDescent)
{
  aDescent = mMaxDescent;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetMaxAdvance(nscoord &aAdvance)
{
  aAdvance = mMaxAdvance;
  return NS_OK;
}

NS_IMETHODIMP nsFontMetricsXlib::GetAveCharWidth(nscoord &aAveCharWidth)
{
  aAveCharWidth = mAveCharWidth;
  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetLangGroup(nsIAtom** aLangGroup)
{
  if (!aLangGroup) {
    return NS_ERROR_NULL_POINTER;
  }

  *aLangGroup = mLangGroup;
  NS_IF_ADDREF(*aLangGroup);

  return NS_OK;
}

NS_IMETHODIMP  nsFontMetricsXlib::GetFontHandle(nsFontHandle &aHandle)
{
  aHandle = (nsFontHandle)mWesternFont;
  return NS_OK;
}

nsFontXlib*
nsFontMetricsXlib::LocateFont(PRUint32 aChar, PRInt32 & aCount)
{
  nsFontXlib *font;
  PRInt32 i;

  
  for (i = 0; i < aCount; ++i) {
    font = (nsFontXlib *)mLoadedFonts[i];
    if (CCMAP_HAS_CHAR(font->mCCMap, aChar))
      return font;
  }

  font = FindFont(aChar);
  aCount = mLoadedFontsCount; 

  return font;
}

nsresult
nsFontMetricsXlib::ResolveForwards(const PRUnichar*         aString,
                                   PRUint32                 aLength,
                                   nsFontSwitchCallbackXlib aFunc, 
                                   void*                    aData)
{
  NS_ASSERTION(aString || !aLength, "invalid call");
  const PRUnichar* firstChar = aString;
  const PRUnichar* currChar = firstChar;
  const PRUnichar* lastChar  = aString + aLength;
  nsFontXlib* currFont;
  nsFontXlib* nextFont;
  PRInt32 count;
  nsFontSwitchXlib fontSwitch;

  if (firstChar == lastChar)
    return NS_OK;

  count = mLoadedFontsCount;

  if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
    currFont = LocateFont(SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
    currChar += 2;
  }
  else {
    currFont = LocateFont(*currChar, count);
    ++currChar;
  }

  
  
  if (currFont == mLoadedFonts[0]) {
    while (currChar < lastChar && CCMAP_HAS_CHAR(currFont->mCCMap,*currChar))
      ++currChar;
    fontSwitch.mFontXlib = currFont;
    if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
      return NS_OK;
    if (currChar == lastChar)
      return NS_OK;
    
    firstChar = currChar;
    if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
      currFont = LocateFont(SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
      currChar += 2;
    }
    else {
      currFont = LocateFont(*currChar, count);
      ++currChar;
    }
  }

  
  PRInt32 lastCharLen;
  while (currChar < lastChar) {
    if (NS_IS_HIGH_SURROGATE(*currChar) && (currChar+1) < lastChar && NS_IS_LOW_SURROGATE(*(currChar+1))) {
      nextFont = LocateFont(SURROGATE_TO_UCS4(*currChar, *(currChar+1)), count);
      lastCharLen = 2;
    }
    else {
      nextFont = LocateFont(*currChar, count);
      lastCharLen = 1;
    }
    if (nextFont != currFont) {
      
      
      fontSwitch.mFontXlib = currFont;
      if (!(*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData))
        return NS_OK;
      
      firstChar = currChar;

      currFont = nextFont; 
    }
    currChar += lastCharLen;
  }

  
  fontSwitch.mFontXlib = currFont;
  (*aFunc)(&fontSwitch, firstChar, currChar - firstChar, aData);
  return NS_OK;
}

NS_IMETHODIMP
nsFontMetricsXlib::GetSpaceWidth(nscoord &aSpaceWidth)
{
  aSpaceWidth = mSpaceWidth;
  return NS_OK;
}






















































































































































static int
SingleByteConvert(nsFontCharSetInfoXlib* aSelf, XFontStruct* aFont,
  const PRUnichar* aSrcBuf, PRInt32 aSrcLen, char* aDestBuf, PRInt32 aDestLen)
{
  int count = 0;
  if (aSelf->mConverter) {
    aSelf->mConverter->Convert(aSrcBuf, &aSrcLen, aDestBuf, &aDestLen);
    count = aDestLen;
  }

  return count;
}




































static int
DoubleByteConvert(nsFontCharSetInfoXlib* aSelf, XFontStruct* aFont,
  const PRUnichar* aSrcBuf, PRInt32 aSrcLen, char* aDestBuf, PRInt32 aDestLen)
{
  int count;
  if (aSelf->mConverter) {
    aSelf->mConverter->Convert(aSrcBuf, &aSrcLen, aDestBuf, &aDestLen);
    count = aDestLen;
    if (count > 0) {
      if ((aDestBuf[0] & 0x80) && (!(aFont->max_byte1 & 0x80))) {
        for (PRInt32 i = 0; i < aDestLen; i++)
          aDestBuf[i] &= 0x7F;
      }
      
      
      else if ((!(aDestBuf[0] & 0x80)) && (aFont->min_byte1 & 0x80)) {
        for (PRInt32 i = 0; i < aDestLen; i++)
          aDestBuf[i] |= 0x80;
      }
    }
  }
  else {
    count = 0;
  }

  return count;
}

static int
ISO10646Convert(nsFontCharSetInfoXlib* aSelf, XFontStruct* aFont,
  const PRUnichar* aSrcBuf, PRInt32 aSrcLen, char* aDestBuf, PRInt32 aDestLen)
{
  aDestLen /= 2;
  if (aSrcLen > aDestLen) {
    aSrcLen = aDestLen;
  }
  if (aSrcLen < 0) {
    aSrcLen = 0;
  }
  XChar2b* dest = (XChar2b*) aDestBuf;
  for (PRInt32 i = 0; i < aSrcLen; i++) {
    dest[i].byte1 = (aSrcBuf[i] >> 8);
    dest[i].byte2 = (aSrcBuf[i] & 0xFF);
  }

  return (int) aSrcLen * 2;
}

#ifdef DEBUG

static void
CheckMap(nsFontMetricsXlibContext *aFmctx, const nsFontCharSetMapXlib* aEntry)
{
  while (aEntry->mName) {
    if (aEntry->mInfo->mCharSet) {
      
        nsresult res;
        
        nsCOMPtr<nsIUnicodeEncoder> converter;
        res = aFmctx->mCharSetManager->GetUnicodeEncoderRaw(aEntry->mInfo->mCharSet,
          getter_AddRefs(converter));
        if (NS_FAILED(res)) {
          printf("=== %s failed (%s)\n", aEntry->mInfo->mCharSet, __FILE__);
        }
    }
    aEntry++;
  }
}

static void
CheckSelf(nsFontMetricsXlibContext *aFmctx)
{
  CheckMap(aFmctx, aFmctx->mCharSetMap);

#ifdef MOZ_MATHML
  
  CheckMap(aFmctx, aFmctx->mSpecialCharSetMap);
#endif 
}

#endif 

static PRBool
SetUpFontCharSetInfo(nsFontMetricsXlibContext *aFmctx, nsFontCharSetInfoXlib* aSelf)
{
#ifdef DEBUG
  static PRBool checkedSelf = PR_FALSE;
  if (!checkedSelf) {
    CheckSelf(aFmctx);
    checkedSelf = PR_TRUE;
  }
#endif 

  nsresult res;
  
  nsIUnicodeEncoder* converter = nsnull;
  res = aFmctx->mCharSetManager->GetUnicodeEncoderRaw(aSelf->mCharSet, &converter);
  if (NS_SUCCEEDED(res)) {
    aSelf->mConverter = converter;
    res = converter->SetOutputErrorBehavior(converter->kOnError_Replace,
                                            nsnull, '?');
    nsCOMPtr<nsICharRepresentable> mapper = do_QueryInterface(converter);
    if (mapper) {
      aSelf->mCCMap = MapperToCCMap(mapper);
      if (aSelf->mCCMap) {
#ifdef DEBUG
        NS_WARNING(nsPrintfCString(256, "SetUpFontCharSetInfo: charset = '%s'", aSelf->mCharSet).get());
#endif 

        










        if ((aSelf->Convert == DoubleByteConvert) 
            && (!aFmctx->mAllowDoubleByteSpecialChars)) {
          PRUint16* ccmap = aSelf->mCCMap;
          PRUint32 page = CCMAP_BEGIN_AT_START_OF_MAP;
          const PRUint16* specialmap = aFmctx->mDoubleByteSpecialCharsCCMap;
          while (NextNonEmptyCCMapPage(specialmap, &page)) {
            PRUint32 pagechar = page;
            for (int i=0; i < CCMAP_BITS_PER_PAGE; i++) {
              if (CCMAP_HAS_CHAR(specialmap, pagechar)) 
                CCMAP_UNSET_CHAR(ccmap, pagechar);
              pagechar++;
            }
          }
        }
        return PR_TRUE;
      }
    }
    else {
      NS_WARNING("cannot get nsICharRepresentable");
    }
  }
  else {
    NS_WARNING("cannot get Unicode converter");
  }

  
  
  
  nsCompressedCharMap empty_ccmapObj;
  aSelf->mCCMap = empty_ccmapObj.NewCCMap();

  
  if (aSelf->mCCMap == nsnull)
    return PR_FALSE;

  return PR_TRUE;
}

#undef DEBUG_DUMP_TREE
#ifdef DEBUG_DUMP_TREE

static char* gDumpStyles[3] = { "normal", "italic", "oblique" };

static PRIntn
DumpCharSet(PLHashEntry* he, PRIntn i, void* arg)
{
  printf("        %s\n", (char*) he->key);
  nsFontCharSetXlib* charSet = (nsFontCharSetXlib*) he->value;
  for (int sizeIndex = 0; sizeIndex < charSet->mSizesCount; sizeIndex++) {
    nsFontXlib* size = &charSet->mSizes[sizeIndex];
    printf("          %d %s\n", size->mSize, size->mName);
  }
  return HT_ENUMERATE_NEXT;
}

static void
DumpFamily(nsFontFamilyXlib* aFamily)
{
  for (int styleIndex = 0; styleIndex < 3; styleIndex++) {
    nsFontStyleXlib* style = aFamily->mStyles[styleIndex];
    if (style) {
      printf("  style: %s\n", gDumpStyles[styleIndex]);
      for (int weightIndex = 0; weightIndex < 8; weightIndex++) {
        nsFontWeightXlib* weight = style->mWeights[weightIndex];
        if (weight) {
          printf("    weight: %d\n", (weightIndex + 1) * 100);
          for (int stretchIndex = 0; stretchIndex < 9; stretchIndex++) {
            nsFontStretchXlib* stretch = weight->mStretches[stretchIndex];
            if (stretch) {
              printf("      stretch: %d\n", stretchIndex + 1);
              PL_HashTableEnumerateEntries(stretch->mCharSets, DumpCharSet,
                nsnull);
            }
          }
        }
      }
    }
  }
}


static PRBool
DumpFamilyEnum(nsHashKey* hashKey, void *aData, void* closure)
{
  printf("family: %s\n",
         NS_LossyConvertUTF16toASCII(*NS_STATIC_CAST(nsString*,he->key)));
  nsFontFamilyXlib* family = (nsFontFamilyXlib*) he->value;
  DumpFamily(family);

  return HT_ENUMERATE_NEXT;
}

static void
DumpTree(void)
{
  aFmctx->mFamilies.Enumerate(DumpFamilyEnum, nsnull);
}
#endif 

struct nsFontSearch
{
  nsFontMetricsXlib *mMetrics;
  PRUnichar          mChar;
  nsFontXlib        *mFont;
};

#if 0
static void
GetUnderlineInfo(nsXFont* aFont, unsigned long* aPositionX2,
  unsigned long* aThickness)
{
  



  if (aFont->GetXFontProperty(XA_UNDERLINE_POSITION, aPositionX2)) {
    *aPositionX2 *= 2;
  }
  else {
    *aPositionX2 = aFont->max_bounds.descent;
  }

  




  if (!xFont->GetXFontProperty(XA_UNDERLINE_THICKNESS, aThickness)) {
    int dir, ascent, descent;
    XCharStruct overall;
    XTextExtents(aFont, "_", 1, &dir, &ascent, &descent, &overall);
    *aThickness = (overall.ascent + overall.descent);
  }
}
#endif 

static PRUint16*
GetMapFor10646Font(XFontStruct* aFont)
{
  if (!aFont->per_char)
    return nsnull;

  nsCompressedCharMap ccmapObj;
  PRInt32 minByte1 = aFont->min_byte1;
  PRInt32 maxByte1 = aFont->max_byte1;
  PRInt32 minByte2 = aFont->min_char_or_byte2;
  PRInt32 maxByte2 = aFont->max_char_or_byte2;
  PRInt32 charsPerRow = maxByte2 - minByte2 + 1;
  for (PRInt32 row = minByte1; row <= maxByte1; row++) {
    PRInt32 offset = (((row - minByte1) * charsPerRow) - minByte2);
    for (PRInt32 cell = minByte2; cell <= maxByte2; cell++) {
      XCharStruct* bounds = &aFont->per_char[offset + cell];
      
      
      if (bounds->ascent ||
          bounds->descent ||
          bounds->lbearing ||
          bounds->rbearing ||
          bounds->width ||
          bounds->attributes) {
        ccmapObj.SetChar((row << 8) | cell);
      }
    }
  }
  PRUint16 *ccmap = ccmapObj.NewCCMap();
  return ccmap;
}

PRBool
nsFontXlib::IsEmptyFont(XFontStruct* xFont)
{
  if (!xFont)
    return PR_TRUE;

  
  
  
  if (xFont->per_char) {
    PRInt32 minByte1 = xFont->min_byte1;
    PRInt32 maxByte1 = xFont->max_byte1;
    PRInt32 minByte2 = xFont->min_char_or_byte2;
    PRInt32 maxByte2 = xFont->max_char_or_byte2;
    PRInt32 charsPerRow = maxByte2 - minByte2 + 1;
    for (PRInt32 row = minByte1; row <= maxByte1; row++) {
      PRInt32 offset = (((row - minByte1) * charsPerRow) - minByte2);
      for (PRInt32 cell = minByte2; cell <= maxByte2; cell++) {
        XCharStruct* bounds = &xFont->per_char[offset + cell];
        if (bounds->ascent || bounds->descent) {
          return PR_FALSE;
        }
      }
    }
  }

  return PR_TRUE;
}

void
nsFontXlib::LoadFont(void)
{
  if (mAlreadyCalledLoadFont)
    return;

  Display *aDisplay = xxlib_rgb_get_display(mFontMetricsContext->mXlibRgbHandle);

#ifdef USE_XPRINT
  if (mFontMetricsContext->mPrinterMode)
  {
    if (XpGetContext(aDisplay) == None)
    {
      
      NS_ERROR("Obtaining font information without a valid print context (XLoadQueryFont()) _before_ XpSetContext()\n");
#ifdef DEBUG
      abort();
#else
      return;
#endif       
    }
  }
#endif 

  mAlreadyCalledLoadFont = PR_TRUE;

  XFontStruct *xlibFont = nsnull;

  NS_ASSERTION(!mFont, "mFont should not be loaded");
#ifdef USE_AASB
  if (mAABaseSize==0)
#endif 
  {
    NS_ASSERTION(!mFontHolder, "mFontHolder should not be loaded");
    xlibFont = ::XLoadQueryFont(aDisplay, mName);
    if(!xlibFont)
    {
#ifdef DEBUG
      printf("nsFontXlib::LoadFont(): loading of font '%s' failed\n", mName);
#endif 
      return;
    }

    mXFont = new nsXFontNormal(aDisplay, xlibFont);
  }
#ifdef USE_AASB
  else {
    NS_ASSERTION(mFontHolder, "mFontHolder should be loaded");
    xlibFont = mFontHolder;
    mXFont = new nsXFontAAScaledBitmap(xxlib_rgb_get_display(mFontMetricsContext->mXlibRgbHandle),
                                       xxlib_rgb_get_screen(mFontMetricsContext->mXlibRgbHandle),
                                       xlibFont, mSize, mAABaseSize);
  }
#endif 

  NS_ASSERTION(mXFont,"failed to load mXFont");
  if (!mXFont)
    return;
  if (!mXFont->LoadFont()) {
    delete mXFont;
    mXFont = nsnull;
    return;
  }

  if (xlibFont) {
    XFontStruct* xFont = mXFont->GetXFontStruct();
    XFontStruct* xFont_with_per_char;
#ifdef USE_AASB
    if (mAABaseSize==0)
#endif 
      xFont_with_per_char = xFont;
#ifdef USE_AASB
    else
      xFont_with_per_char = mFontHolder;
#endif 

    mMaxAscent = xFont->ascent;
    mMaxDescent = xFont->descent;

    if (mCharSetInfo == mFontMetricsContext->mISO106461) {
      mCCMap = GetMapFor10646Font(xFont_with_per_char);
      if (!mCCMap) {
        mXFont->UnloadFont();
        mXFont = nsnull;
        ::XFreeFont(aDisplay, xlibFont);
        mFontHolder = nsnull;
        return;
      }
    }

    
    
    if ((mCharSetInfo == &JISX0201)
        || (mCharSetInfo == &CNS116434)
        || (mCharSetInfo == &CNS116435)
        || (mCharSetInfo == &CNS116436)
        || (mCharSetInfo == &CNS116437)
       ) {
      if (IsEmptyFont(xFont_with_per_char)) {
#ifdef NS_FONT_DEBUG_LOAD_FONT
        if (gFontDebug & NS_FONT_DEBUG_LOAD_FONT) {
          printf("\n");
          printf("***************************************\n");
          printf("invalid font \"%s\", %s %d\n", mName, __FILE__, __LINE__);
          printf("***************************************\n");
          printf("\n");
        }
#endif
        mXFont->UnloadFont();
        mXFont = nsnull;
        ::XFreeFont(aDisplay, xlibFont);
        mFontHolder = nsnull;
        return;
      }
    }
    mFont = xlibFont;

#ifdef NS_FONT_DEBUG_LOAD_FONT
    if (gFontDebug & NS_FONT_DEBUG_LOAD_FONT) {
      printf("loaded %s\n", mName);
    }
#endif

  }

#ifdef NS_FONT_DEBUG_LOAD_FONT
  else if (gFontDebug & NS_FONT_DEBUG_LOAD_FONT) {
    printf("cannot load %s\n", mName);
  }
#endif

}

XFontStruct *
nsFontXlib::GetXFontStruct(void)
{
  return mFont;
}

nsXFont*
nsFontXlib::GetXFont(void)
{
  return mXFont;
}

PRBool
nsFontXlib::GetXFontIs10646(void)
{
  return ((PRBool) (mCharSetInfo == mFontMetricsContext->mISO106461));
}

#ifdef MOZ_ENABLE_FREETYPE2
PRBool
nsFontXlib::IsFreeTypeFont(void)
{
  return PR_FALSE;
}
#endif 

nsFontXlib::nsFontXlib()
{
  MOZ_COUNT_CTOR(nsFontXlib);
}

nsFontXlib::~nsFontXlib()
{
  MOZ_COUNT_DTOR(nsFontXlib);
  if (mXFont) {
    delete mXFont;
  }
  if (mFont 
#ifdef USE_AASB
      && (mAABaseSize==0)
#endif 
      ) {
    XFreeFont(xxlib_rgb_get_display(mFontMetricsContext->mXlibRgbHandle), mFont);
  }
  if (mCharSetInfo == mFontMetricsContext->mISO106461) {
    FreeCCMap(mCCMap);
  }
  if (mName) {
    PR_smprintf_free(mName);
  }
}

class nsFontXlibNormal : public nsFontXlib
{
public:
  nsFontXlibNormal(nsFontMetricsXlibContext *aFontMetricsContext);
  nsFontXlibNormal(nsFontXlib*);
  virtual ~nsFontXlibNormal();

  virtual int GetWidth(const PRUnichar* aString, PRUint32 aLength);
  virtual int DrawString(nsRenderingContextXlib* aContext,
                         nsIDrawingSurfaceXlib* aSurface, nscoord aX,
                         nscoord aY, const PRUnichar* aString,
                         PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                      PRUint32           aLength,
                                      nsBoundingMetrics& aBoundingMetrics);
#endif 
};

nsFontXlibNormal::nsFontXlibNormal(nsFontMetricsXlibContext *aFontMetricsContext)
{
  mFontHolder = nsnull;
  mFontMetricsContext = aFontMetricsContext;
}

nsFontXlibNormal::nsFontXlibNormal(nsFontXlib *aFont)
{
  mFontMetricsContext = aFont->mFontMetricsContext;

#ifdef USE_AASB
  mAABaseSize = aFont->mSize;
#endif 
  mFontHolder = aFont->GetXFontStruct();
  if (!mFontHolder) {
    aFont->LoadFont();
    mFontHolder = aFont->GetXFontStruct();
  }
  NS_ASSERTION(mFontHolder, "font to copy not loaded");
}

nsFontXlibNormal::~nsFontXlibNormal()
{
}

int
nsFontXlibNormal::GetWidth(const PRUnichar* aString, PRUint32 aLength)
{
  if (!mFont) {
    LoadFont();
    if (!mFont) {
      return 0;
    }
  }

  XChar2b buf[512];
  char *p;
  PRInt32 bufLen;
  ENCODER_BUFFER_ALLOC_IF_NEEDED(p, mCharSetInfo->mConverter,
                         aString, aLength, buf, sizeof(buf), bufLen);
  int len = mCharSetInfo->Convert(mCharSetInfo, mXFont->GetXFontStruct(),
                                  aString, aLength, p, bufLen);
  int outWidth;
  if (mXFont->IsSingleByte())
    outWidth = mXFont->TextWidth8(p, len);
  else
    outWidth = mXFont->TextWidth16((const XChar2b*)p, len/2);
  ENCODER_BUFFER_FREE_IF_NEEDED(p, buf);
  return outWidth;
}

int
nsFontXlibNormal::DrawString(nsRenderingContextXlib* aContext,
                             nsIDrawingSurfaceXlib* aSurface,
                             nscoord aX, nscoord aY,
                             const PRUnichar* aString, PRUint32 aLength)
{
  if (!mFont) {
    LoadFont();
    if (!mFont) {
      return 0;
    }
  }

  XChar2b buf[512];
  char *p;
  PRInt32 bufLen;
  ENCODER_BUFFER_ALLOC_IF_NEEDED(p, mCharSetInfo->mConverter,
                         aString, aLength, buf, sizeof(buf), bufLen);
  int len = mCharSetInfo->Convert(mCharSetInfo, mXFont->GetXFontStruct(),
                                  aString, aLength, p, bufLen);
  xGC *gc = aContext->GetGC();
  int outWidth;
  if (mXFont->IsSingleByte()) {
    mXFont->DrawText8(aSurface->GetDrawable(), *gc, aX,
                                          aY + mBaselineAdjust, p, len);
    outWidth = mXFont->TextWidth8(p, len);
  }
  else {
    mXFont->DrawText16(aSurface->GetDrawable(), *gc, aX, aY + mBaselineAdjust,
                       (const XChar2b*)p, len/2);
    outWidth = mXFont->TextWidth16((const XChar2b*)p, len/2);
  }
  gc->Release();
  ENCODER_BUFFER_FREE_IF_NEEDED(p, buf);
  return outWidth;
}

#ifdef MOZ_MATHML


nsresult
nsFontXlibNormal::GetBoundingMetrics(const PRUnichar*   aString,
                                     PRUint32           aLength,
                                     nsBoundingMetrics& aBoundingMetrics)                                 
{
  aBoundingMetrics.Clear();               

  if (!mFont) {
    LoadFont();
    if (!mFont) {
      return NS_ERROR_FAILURE;
    }
  }

  if (aString && 0 < aLength) {
    XFontStruct *fontInfo = mXFont->GetXFontStruct();
    XChar2b buf[512];
    char *p;
    PRInt32 bufLen;
    ENCODER_BUFFER_ALLOC_IF_NEEDED(p, mCharSetInfo->mConverter,
                         aString, aLength, buf, sizeof(buf), bufLen);
    int len = mCharSetInfo->Convert(mCharSetInfo, fontInfo, aString, aLength,
                                    p, bufLen);
    if (mXFont->IsSingleByte()) {
      mXFont->TextExtents8(p, len,
                           &aBoundingMetrics.leftBearing,
                           &aBoundingMetrics.rightBearing,
                           &aBoundingMetrics.width,
                           &aBoundingMetrics.ascent,
                           &aBoundingMetrics.descent);
    }
    else {
      mXFont->TextExtents16((const XChar2b*)p, len,
                           &aBoundingMetrics.leftBearing,
                           &aBoundingMetrics.rightBearing,
                           &aBoundingMetrics.width,
                           &aBoundingMetrics.ascent,
                           &aBoundingMetrics.descent);
    }
    ENCODER_BUFFER_FREE_IF_NEEDED(p, buf);
  }

  return NS_OK;
}
#endif 

class nsFontXlibSubstitute : public nsFontXlib
{
public:
  nsFontXlibSubstitute(nsFontXlib* aFont);
  virtual ~nsFontXlibSubstitute();

  virtual XFontStruct *GetXFontStruct(void);
  virtual nsXFont     *GetXFont(void);
  virtual PRBool       GetXFontIs10646(void);
  virtual int          GetWidth(const PRUnichar* aString, PRUint32 aLength);
  virtual int          DrawString(nsRenderingContextXlib* aContext,
                                  nsIDrawingSurfaceXlib* aSurface, nscoord aX,
                                  nscoord aY, const PRUnichar* aString,
                                  PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                      PRUint32           aLength,
                                      nsBoundingMetrics& aBoundingMetrics);
#endif 
  virtual PRUint32 Convert(const PRUnichar* aSrc, PRUint32 aSrcLen,
                           PRUnichar* aDest, PRUint32 aDestLen);

  nsFontXlib* mSubstituteFont;
};

nsFontXlibSubstitute::nsFontXlibSubstitute(nsFontXlib* aFont)
{
  mSubstituteFont = aFont;
  mFontMetricsContext = aFont->mFontMetricsContext;
}

nsFontXlibSubstitute::~nsFontXlibSubstitute()
{
  
}

PRUint32
nsFontXlibSubstitute::Convert(const PRUnichar* aSrc, PRUint32 aSrcLen,
  PRUnichar* aDest, PRUint32 aDestLen)
{
  nsresult res;
  if (!mFontMetricsContext->mFontSubConverter) {
    mFontMetricsContext->mFontSubConverter = do_CreateInstance(NS_SAVEASCHARSET_CONTRACTID);
    if (mFontMetricsContext->mFontSubConverter) {
      res = mFontMetricsContext->mFontSubConverter->Init("ISO-8859-1",
                             nsISaveAsCharset::attr_FallbackQuestionMark +
                               nsISaveAsCharset::attr_EntityAfterCharsetConv +
                               nsISaveAsCharset::attr_IgnoreIgnorables,
                             nsIEntityConverter::transliterate);
      if (NS_FAILED(res))
        mFontMetricsContext->mFontSubConverter = nsnull; 
    }
  }

  if (mFontMetricsContext->mFontSubConverter) {
    nsAutoString tmp(aSrc, aSrcLen);
    char* conv = nsnull;

    
    res = mFontMetricsContext->mFontSubConverter->Convert(tmp.get(), &conv);
    if (NS_SUCCEEDED(res) && conv) {
      char *p = conv;
      PRUint32 i;
      for (i = 0; i < aDestLen; i++) {
        if (*p) {
          aDest[i] = *p;
        }
        else {
          break;
        }
        p++;
      }
      nsMemory::Free(conv);
      conv = nsnull;
      return i;
    }
  }

  if (aSrcLen > aDestLen) {
    aSrcLen = aDestLen;
  }
  for (PRUint32 i = 0; i < aSrcLen; i++)
    aDest[i] = '?';

  return aSrcLen;
}

int
nsFontXlibSubstitute::GetWidth(const PRUnichar* aString, PRUint32 aLength)
{
  PRUnichar buf[512];
  PRUnichar *p = buf;
  PRUint32 bufLen = sizeof(buf)/sizeof(PRUnichar);
  if ((aLength*2) > bufLen) {
    PRUnichar *tmp;
    tmp = (PRUnichar*)nsMemory::Alloc(sizeof(PRUnichar) * (aLength*2));
    if (tmp) {
      p = tmp;
      bufLen = (aLength*2);
    }
  }
  PRUint32 len = Convert(aString, aLength, p, bufLen);
  int outWidth = mSubstituteFont->GetWidth(p, len);
  if (p != buf)
    nsMemory::Free(p);
  return outWidth;

}

int
nsFontXlibSubstitute::DrawString(nsRenderingContextXlib* aContext,
                                 nsIDrawingSurfaceXlib* aSurface,
                                 nscoord aX, nscoord aY,
                                 const PRUnichar* aString, PRUint32 aLength)
{
  PRUnichar buf[512];
  PRUnichar *p = buf;
  PRUint32 bufLen = sizeof(buf)/sizeof(PRUnichar);
  if ((aLength*2) > bufLen) {
    PRUnichar *tmp;
    tmp = (PRUnichar*)nsMemory::Alloc(sizeof(PRUnichar) * (aLength*2));
    if (tmp) {
      p = tmp;
      bufLen = (aLength*2);
    }
  }
  PRUint32 len = Convert(aString, aLength, p, bufLen);
  int outWidth = mSubstituteFont->DrawString(aContext, aSurface, 
                                             aX, aY, p, len);
  if (p != buf)
    nsMemory::Free(p);
  return outWidth;
}

#ifdef MOZ_MATHML


nsresult
nsFontXlibSubstitute::GetBoundingMetrics(const PRUnichar*   aString,
                                         PRUint32           aLength,
                                         nsBoundingMetrics& aBoundingMetrics)                                 
{
  PRUnichar buf[512];
  PRUnichar *p = buf;
  PRUint32 bufLen = sizeof(buf)/sizeof(PRUnichar);
  if ((aLength*2) > bufLen) {
    PRUnichar *tmp;
    tmp = (PRUnichar*)nsMemory::Alloc(sizeof(PRUnichar) * (aLength*2));
    if (tmp) {
      p = tmp;
      bufLen = (aLength*2);
    }
  }
  PRUint32 len = Convert(aString, aLength, p, bufLen);
  nsresult res = mSubstituteFont->GetBoundingMetrics(p, len, 
                                    aBoundingMetrics);
  if (p != buf)
    nsMemory::Free(p);
  return res;
}
#endif 

XFontStruct * 
nsFontXlibSubstitute::GetXFontStruct(void)
{
  return mSubstituteFont->GetXFontStruct();
}

nsXFont*
nsFontXlibSubstitute::GetXFont(void)
{
  return mSubstituteFont->GetXFont();
}

PRBool
nsFontXlibSubstitute::GetXFontIs10646(void)
{
  return mSubstituteFont->GetXFontIs10646();
}

class nsFontXlibUserDefined : public nsFontXlib
{
public:
  nsFontXlibUserDefined(nsFontMetricsXlibContext *aFontMetricsContext);
  virtual ~nsFontXlibUserDefined();

  virtual PRBool Init(nsFontXlib* aFont);
  virtual int GetWidth(const PRUnichar* aString, PRUint32 aLength);
  virtual int DrawString(nsRenderingContextXlib* aContext,
                         nsIDrawingSurfaceXlib* aSurface, nscoord aX,
                         nscoord aY, const PRUnichar* aString,
                         PRUint32 aLength);
#ifdef MOZ_MATHML
  virtual nsresult GetBoundingMetrics(const PRUnichar*   aString,
                                      PRUint32           aLength,
                                      nsBoundingMetrics& aBoundingMetrics);
#endif 
  virtual PRUint32 Convert(const PRUnichar* aSrc, PRInt32 aSrcLen,
                           char* aDest, PRInt32 aDestLen);
};

nsFontXlibUserDefined::nsFontXlibUserDefined(nsFontMetricsXlibContext *aFontMetricsContext)
{
  mFontMetricsContext = aFontMetricsContext;
}

nsFontXlibUserDefined::~nsFontXlibUserDefined()
{
  
}

PRBool
nsFontXlibUserDefined::Init(nsFontXlib* aFont)
{
  if (!aFont->GetXFont()) {
    aFont->LoadFont();
    if (!aFont->GetXFont()) {
      mCCMap = mFontMetricsContext->mEmptyCCMap;
      return PR_FALSE;
    }
  }
  mXFont = aFont->GetXFont();
  mCCMap = mFontMetricsContext->mUserDefinedCCMap;
  mName = aFont->mName;

  return PR_TRUE;
}

PRUint32
nsFontXlibUserDefined::Convert(const PRUnichar* aSrc, PRInt32 aSrcLen,
  char* aDest, PRInt32 aDestLen)
{
  if (aSrcLen > aDestLen) {
    aSrcLen = aDestLen;
  }
  mFontMetricsContext->mUserDefinedConverter->Convert(aSrc, &aSrcLen, aDest, &aDestLen);

  return aSrcLen;
}

int
nsFontXlibUserDefined::GetWidth(const PRUnichar* aString, PRUint32 aLength)
{
  char buf[1024];
  char *p;
  PRInt32 bufLen;
  ENCODER_BUFFER_ALLOC_IF_NEEDED(p, mFontMetricsContext->mUserDefinedConverter,
                         aString, aLength, buf, sizeof(buf), bufLen);
  PRUint32 len = Convert(aString, aLength, p, bufLen);

  int outWidth;
  if (mXFont->IsSingleByte())
    outWidth = mXFont->TextWidth8(p, len);
  else
    outWidth = mXFont->TextWidth16((const XChar2b*)p, len/2);
  ENCODER_BUFFER_FREE_IF_NEEDED(p, buf);
  return outWidth;
}

int
nsFontXlibUserDefined::DrawString(nsRenderingContextXlib* aContext,
                                  nsIDrawingSurfaceXlib* aSurface,
                                  nscoord aX, nscoord aY,
                                  const PRUnichar* aString, PRUint32 aLength)
{
  char  buf[1024];
  char *p;
  PRInt32 bufLen;
  ENCODER_BUFFER_ALLOC_IF_NEEDED(p, mFontMetricsContext->mUserDefinedConverter,
                         aString, aLength, buf, sizeof(buf), bufLen);
  PRUint32 len = Convert(aString, aLength, p, bufLen);
  xGC *gc = aContext->GetGC();

  int outWidth;
  if (mXFont->IsSingleByte()) {
    mXFont->DrawText8(aSurface->GetDrawable(), *gc, aX,
                                          aY + mBaselineAdjust, p, len);
    outWidth = mXFont->TextWidth8(p, len);
  }
  else {
    mXFont->DrawText16(aSurface->GetDrawable(), *gc, aX, aY + mBaselineAdjust,
                       (const XChar2b*)p, len);
    outWidth = mXFont->TextWidth16((const XChar2b*)p, len/2);
  }
  gc->Release();
  ENCODER_BUFFER_FREE_IF_NEEDED(p, buf);
  return outWidth;
}

#ifdef MOZ_MATHML


nsresult
nsFontXlibUserDefined::GetBoundingMetrics(const PRUnichar*   aString,
                                        PRUint32           aLength,
                                        nsBoundingMetrics& aBoundingMetrics)                                 
{
  aBoundingMetrics.Clear();               

  if (aString && 0 < aLength) {
    char  buf[1024];
    char *p;
    PRInt32 bufLen;
    ENCODER_BUFFER_ALLOC_IF_NEEDED(p, mFontMetricsContext->mUserDefinedConverter,
                         aString, aLength, buf, sizeof(buf), bufLen);
    PRUint32 len = Convert(aString, aLength, p, bufLen);
    if (mXFont->IsSingleByte()) {
      mXFont->TextExtents8(p, len,
                           &aBoundingMetrics.leftBearing,
                           &aBoundingMetrics.rightBearing,
                           &aBoundingMetrics.width,
                           &aBoundingMetrics.ascent,
                           &aBoundingMetrics.descent);
    }
    else {
      mXFont->TextExtents16((const XChar2b*)p, len,
                            &aBoundingMetrics.leftBearing,
                            &aBoundingMetrics.rightBearing,
                            &aBoundingMetrics.width,
                            &aBoundingMetrics.ascent,
                            &aBoundingMetrics.descent);
    }
    ENCODER_BUFFER_FREE_IF_NEEDED(p, buf);
  }

  return NS_OK;
}
#endif 

nsFontXlib*
nsFontMetricsXlib::AddToLoadedFontsList(nsFontXlib* aFont)
{
  if (mLoadedFontsCount == mLoadedFontsAlloc) {
    int newSize;
    if (mLoadedFontsAlloc) {
      newSize = (2 * mLoadedFontsAlloc);
    }
    else {
      newSize = 1;
    }
    nsFontXlib** newPointer = (nsFontXlib**) 
      PR_Realloc(mLoadedFonts, newSize * sizeof(nsFontXlib*));
    if (newPointer) {
      mLoadedFonts = newPointer;
      mLoadedFontsAlloc = newSize;
    }
    else {
      return nsnull;
    }
  }
  mLoadedFonts[mLoadedFontsCount++] = aFont;
  return aFont;
}



#define NOT_FOUND_FONT_SIZE 1000*1000*1000

nsFontXlib*
nsFontMetricsXlib::FindNearestSize(nsFontStretchXlib* aStretch, PRUint16 aSize)
{
  nsFontXlib* font = nsnull;
  if (aStretch->mSizes) {
    nsFontXlib** begin = aStretch->mSizes;
    nsFontXlib** end = &aStretch->mSizes[aStretch->mSizesCount];
    nsFontXlib** s;
    
    for (s = begin; s < end; s++) {
      
      if ((*s)->mSize >= aSize) {
        break;
      }
    }
    
    if (s == end) {
      s--;
    }
    else if (s != begin) {
      
      if (((*s)->mSize - aSize) >= (aSize - (*(s - 1))->mSize)) {
        s--;
      }
    }
    
    font = *s;
  }
  return font;
}

static PRBool
SetFontCharsetInfo(nsFontXlib *aFont, nsFontCharSetInfoXlib* aCharSet,
                   PRUnichar aChar)
{
  if (aCharSet->mCharSet) {
    aFont->mCCMap = aCharSet->mCCMap;
    
    if (CCMAP_HAS_CHAR(aFont->mCCMap, aChar)) {
      aFont->LoadFont();
      if (!aFont->GetXFont()) {
        return PR_FALSE;
      }
    }
  }
  else {
    if (aCharSet == aFont->mFontMetricsContext->mISO106461) {
      aFont->LoadFont();
      if (!aFont->GetXFont()) {
        return PR_FALSE;
      }
    }
  }
  return PR_TRUE;
}

static nsFontXlib*
SetupUserDefinedFont(nsFontMetricsXlibContext *aFmctx, nsFontXlib *aFont)
{
  if (!aFont->mUserDefinedFont) {
    aFont->mUserDefinedFont = new nsFontXlibUserDefined(aFmctx);
    if (!aFont->mUserDefinedFont) {
      return nsnull;
    }
    if (!aFont->mUserDefinedFont->Init(aFont)) {
      return nsnull;
    }
  }
  return aFont->mUserDefinedFont;
}


#ifdef USE_AASB
nsFontXlib*
nsFontMetricsXlib::GetAASBBaseFont(nsFontStretchXlib* aStretch, 
                                   nsFontCharSetInfoXlib* aCharSet)
{
  nsFontXlib* base_aafont;
  PRInt32 scale_size;
  PRUint32 aa_target_size;

  scale_size = PR_MAX(mPixelSize, aCharSet->mAABitmapScaleMin);
  aa_target_size = PR_MAX((scale_size*2), 16);
  base_aafont = FindNearestSize(aStretch, aa_target_size);
  NS_ASSERTION(base_aafont,
             "failed to find a base font for Anti-Aliased bitmap Scaling");
  return base_aafont;
}
#endif 

nsFontXlib*
nsFontMetricsXlib::PickASizeAndLoad(nsFontStretchXlib* aStretch,
  nsFontCharSetInfoXlib* aCharSet, PRUnichar aChar, const char *aName)
{
#ifdef MOZ_ENABLE_FREETYPE2
  if (aStretch->mFreeTypeFaceID) {
    
    nsFreeTypeFont *ftfont = nsFreeTypeFont::NewFont(aStretch->mFreeTypeFaceID,
                                                     mPixelSize,
                                                     aName);
    if (!ftfont) {
      FREETYPE_FONT_PRINTF(("failed to create font"));
      return nsnull;
    }
    
    



    ftfont->mName = PR_smprintf("%s", aName);
    if (!ftfont->mName) {
      FREETYPE_FONT_PRINTF(("failed to create mName"));
      delete ftfont;
      return nsnull;
    }
    SetCharsetLangGroup(mFontMetricsContext, aCharSet);
    ftfont->mSize = mPixelSize;
    ftfont->LoadFont();
    ftfont->mCharSetInfo = mFontMetricsContext->mISO106461;
    
    return AddToLoadedFontsList(ftfont);
  }
#endif 

  PRBool      use_scaled_font               = PR_FALSE;
  PRBool      have_nearly_rightsized_bitmap = PR_FALSE;
#ifdef USE_AASB
  nsFontXlib *base_aafont                   = nsnull;
#endif 

#ifdef USE_XPRINT
#define ALWAYS_USE_SCALED_FONTS_FOR_XPRINT 1
#endif 

#ifdef ALWAYS_USE_SCALED_FONTS_FOR_XPRINT








 if (mFontMetricsContext->mPrinterMode)
 {
   use_scaled_font = PR_TRUE;
 }
#endif 

  PRInt32 bitmap_size = NOT_FOUND_FONT_SIZE;
  PRInt32 scale_size = mPixelSize;
  nsFontXlib* font = FindNearestSize(aStretch, mPixelSize);
  if (font) {
    bitmap_size = font->mSize;
    if (   (bitmap_size >= mPixelSize-(mPixelSize/10))
        && (bitmap_size <= mPixelSize+(mPixelSize/10)))
      
      
      have_nearly_rightsized_bitmap = PR_TRUE;
  }

#ifdef USE_AASB
  
  
  
  if (mFontMetricsContext->mAABitmapScaleEnabled && aCharSet->mAABitmapScaleAlways) {
    base_aafont = GetAASBBaseFont(aStretch, aCharSet);
    if (base_aafont) {
      use_scaled_font = PR_TRUE;
      SIZE_FONT_PRINTF(("anti-aliased bitmap scaled font: %s\n"
            "                    desired=%d, aa-scaled=%d, bitmap=%d, "
            "aa_bitmap=%d",
            aName, mPixelSize, scale_size, bitmap_size, base_aafont->mSize));
    }
  }
#endif 

  
  
  
  
  if (!use_scaled_font && !have_nearly_rightsized_bitmap) {
    
    if (aStretch->mOutlineScaled) {
      scale_size = PR_MAX(mPixelSize, aCharSet->mOutlineScaleMin);

      if (PR_ABS(mPixelSize-scale_size) < PR_ABS(mPixelSize-bitmap_size)) {
        use_scaled_font = PR_TRUE;
        SIZE_FONT_PRINTF(("outline font:______ %s\n"
                  "                    desired=%d, scaled=%d, bitmap=%d", 
                  aStretch->mScalable, mPixelSize, scale_size,
                  (bitmap_size=NOT_FOUND_FONT_SIZE?0:bitmap_size)));
      }
    }
#ifdef USE_AASB
    
    if (!use_scaled_font 
        && (bitmap_size<NOT_FOUND_FONT_SIZE) && mFontMetricsContext->mAABitmapScaleEnabled) {
      
      
      scale_size = PR_MAX(mPixelSize, aCharSet->mAABitmapScaleMin);
      double ratio = (bitmap_size / ((double) mPixelSize));
      if (   (ratio < aCharSet->mAABitmapUndersize)
          || (ratio > aCharSet->mAABitmapOversize)) {
        
        
        
        
        base_aafont = GetAASBBaseFont(aStretch, aCharSet);
        if (base_aafont) {
          use_scaled_font = PR_TRUE;
          SIZE_FONT_PRINTF(("anti-aliased bitmap scaled font: %s\n"
              "                    desired=%d, aa-scaled=%d, bitmap=%d, "
              "aa_bitmap=%d",
              aName, mPixelSize, scale_size, bitmap_size, base_aafont->mSize));
        }
      }
    }
#endif 

    
    if (!use_scaled_font && aStretch->mScalable) {
      scale_size = PR_MAX(mPixelSize, aCharSet->mBitmapScaleMin);
      double ratio = (bitmap_size / ((double) mPixelSize));
      if ((ratio < aCharSet->mBitmapUndersize)
        || (ratio > aCharSet->mBitmapOversize)) {
        if ((PR_ABS(mPixelSize-scale_size) < PR_ABS(mPixelSize-bitmap_size))) {
          use_scaled_font = PR_TRUE;
          SIZE_FONT_PRINTF(("bitmap scaled font: %s\n"
                "                    desired=%d, scaled=%d, bitmap=%d", 
                aStretch->mScalable, mPixelSize, scale_size,
                (bitmap_size=NOT_FOUND_FONT_SIZE?0:bitmap_size)));
        }
      }
    }
  }

  NS_ASSERTION((bitmap_size<NOT_FOUND_FONT_SIZE)||use_scaled_font,
                "did not find font size");
  if (!use_scaled_font) {
    SIZE_FONT_PRINTF(("bitmap font:_______ %s\n" 
                      "                    desired=%d, scaled=%d, bitmap=%d", 
                      aName, mPixelSize, scale_size, bitmap_size));
  }

  if (use_scaled_font
#ifdef ALWAYS_USE_SCALED_FONTS_FOR_XPRINT
   && aStretch->mScalable
#endif 
     ) {
   SIZE_FONT_PRINTF(("scaled font:_______ %s\n"
                     "                    desired=%d, scaled=%d, bitmap=%d",
                     aName, mPixelSize, scale_size, bitmap_size));

    PRInt32 i;
    PRInt32 n = aStretch->mScaledFonts.Count();
    nsFontXlib* p = nsnull;
    for (i = 0; i < n; i++) {
      p = (nsFontXlib*) aStretch->mScaledFonts.ElementAt(i);
      if (p->mSize == scale_size) {
        break;
      }
    }
    if (i == n) {
#ifdef USE_AASB
      if (base_aafont) {
        
        if (!SetFontCharsetInfo(base_aafont, aCharSet, aChar))
          return nsnull;
        if (mIsUserDefined) {
          base_aafont = SetupUserDefinedFont(mFontMetricsContext, base_aafont);
          if (!base_aafont)
            return nsnull;
        }
        font = new nsFontXlibNormal(aFmctx, base_aafont);
      }
      else
#endif 
      {
        font = new nsFontXlibNormal(mFontMetricsContext);
      }

      if (font) {
        



#ifdef USE_AASB
        if (base_aafont) {
          font->mName = PR_smprintf("%s", base_aafont->mName);
          font->mAABaseSize = base_aafont->mSize;
        }
        else
#endif 
        {
          font->mName = PR_smprintf(aStretch->mScalable, scale_size);
#ifdef USE_AASB
          font->mAABaseSize = 0;
#endif 
        }
        if (!font->mName) {
          delete font;
          return nsnull;
        }
        font->mSize = scale_size;
        font->mCharSetInfo = aCharSet;
        aStretch->mScaledFonts.AppendElement(font);
      }
      else {
        return nsnull;
      }
    }
    else {
      font = p;
    }
  }

  if (!SetFontCharsetInfo(font, aCharSet, aChar))
    return nsnull;

  if (mIsUserDefined) {
    font = SetupUserDefinedFont(mFontMetricsContext, font);
    if (!font)
      return nsnull;
  }

  return AddToLoadedFontsList(font);
}

PR_BEGIN_EXTERN_C
static int
CompareSizes(const void* aArg1, const void* aArg2, void *data)
{
  return (*((nsFontXlib**) aArg1))->mSize - (*((nsFontXlib**) aArg2))->mSize;
}
PR_END_EXTERN_C

void
nsFontStretchXlib::SortSizes(void)
{
  NS_QuickSort(mSizes, mSizesCount, sizeof(*mSizes), CompareSizes, NULL);
}

void
nsFontWeightXlib::FillStretchHoles(void)
{
  int i, j;

  for (i = 0; i < 9; i++) {
    if (mStretches[i]) {
      mStretches[i]->SortSizes();
    }
  }

  if (!mStretches[4]) {
    for (i = 5; i < 9; i++) {
      if (mStretches[i]) {
        mStretches[4] = mStretches[i];
        break;
      }
    }
    if (!mStretches[4]) {
      for (i = 3; i >= 0; i--) {
        if (mStretches[i]) {
          mStretches[4] = mStretches[i];
          break;
        }
      }
    }
  }

  for (i = 5; i < 9; i++) {
    if (!mStretches[i]) {
      for (j = i + 1; j < 9; j++) {
        if (mStretches[j]) {
          mStretches[i] = mStretches[j];
          break;
        }
      }
      if (!mStretches[i]) {
        for (j = i - 1; j >= 0; j--) {
          if (mStretches[j]) {
            mStretches[i] = mStretches[j];
            break;
          }
        }
      }
    }
  }
  for (i = 3; i >= 0; i--) {
    if (!mStretches[i]) {
      for (j = i - 1; j >= 0; j--) {
        if (mStretches[j]) {
          mStretches[i] = mStretches[j];
          break;
        }
      }
      if (!mStretches[i]) {
        for (j = i + 1; j < 9; j++) {
          if (mStretches[j]) {
            mStretches[i] = mStretches[j];
            break;
          }
        }
      }
    }
  }
}

void
nsFontStyleXlib::FillWeightHoles(void)
{
  int i, j;

  for (i = 0; i < 9; i++) {
    if (mWeights[i]) {
      mWeights[i]->FillStretchHoles();
    }
  }

  if (!mWeights[3]) {
    for (i = 4; i < 9; i++) {
      if (mWeights[i]) {
        mWeights[3] = mWeights[i];
        break;
      }
    }
    if (!mWeights[3]) {
      for (i = 2; i >= 0; i--) {
        if (mWeights[i]) {
          mWeights[3] = mWeights[i];
          break;
        }
      }
    }
  }

  
  if (!mWeights[4]) {
    mWeights[4] = mWeights[3];
  }
  for (i = 5; i < 9; i++) {
    if (!mWeights[i]) {
      for (j = i + 1; j < 9; j++) {
        if (mWeights[j]) {
          mWeights[i] = mWeights[j];
          break;
        }
      }
      if (!mWeights[i]) {
        for (j = i - 1; j >= 0; j--) {
          if (mWeights[j]) {
            mWeights[i] = mWeights[j];
            break;
          }
        }
      }
    }
  }
  for (i = 2; i >= 0; i--) {
    if (!mWeights[i]) {
      for (j = i - 1; j >= 0; j--) {
        if (mWeights[j]) {
          mWeights[i] = mWeights[j];
          break;
        }
      }
      if (!mWeights[i]) {
        for (j = i + 1; j < 9; j++) {
          if (mWeights[j]) {
            mWeights[i] = mWeights[j];
            break;
          }
        }
      }
    }
  }
}

void
nsFontNodeXlib::FillStyleHoles(void)
{
  if (mHolesFilled) {
    return;
  }
  mHolesFilled = 1;

#ifdef DEBUG_DUMP_TREE
  DumpFamily(this);
#endif

  for (int i = 0; i < 3; i++) {
    if (mStyles[i]) {
      mStyles[i]->FillWeightHoles();
    }
  }

  
  
  if (!mStyles[NS_FONT_STYLE_NORMAL]) {
    if (mStyles[NS_FONT_STYLE_ITALIC]) {
      mStyles[NS_FONT_STYLE_NORMAL] = mStyles[NS_FONT_STYLE_ITALIC];
    }
    else {
      mStyles[NS_FONT_STYLE_NORMAL] = mStyles[NS_FONT_STYLE_OBLIQUE];
    }
  }
  if (!mStyles[NS_FONT_STYLE_ITALIC]) {
    if (mStyles[NS_FONT_STYLE_OBLIQUE]) {
      mStyles[NS_FONT_STYLE_ITALIC] = mStyles[NS_FONT_STYLE_OBLIQUE];
    }
    else {
      mStyles[NS_FONT_STYLE_ITALIC] = mStyles[NS_FONT_STYLE_NORMAL];
    }
  }
  if (!mStyles[NS_FONT_STYLE_OBLIQUE]) {
    if (mStyles[NS_FONT_STYLE_ITALIC]) {
      mStyles[NS_FONT_STYLE_OBLIQUE] = mStyles[NS_FONT_STYLE_ITALIC];
    }
    else {
      mStyles[NS_FONT_STYLE_OBLIQUE] = mStyles[NS_FONT_STYLE_NORMAL];
    }
  }

#ifdef DEBUG_DUMP_TREE
  DumpFamily(this);
#endif
}

static void
SetCharsetLangGroup(nsFontMetricsXlibContext *aFmctx, nsFontCharSetInfoXlib* aCharSetInfo)
{
  if (!aCharSetInfo->mCharSet || aCharSetInfo->mLangGroup)
    return;

  nsresult res;
  
  res = aFmctx->mCharSetManager->GetCharsetLangGroupRaw(aCharSetInfo->mCharSet,
                                                        &aCharSetInfo->mLangGroup);
  if (NS_FAILED(res)) {
    aCharSetInfo->mLangGroup = NS_NewAtom("");
#ifdef NOISY_FONTS
    printf("=== cannot get lang group for %s\n", aCharSetInfo->mCharSet);
#endif
  }
}

#define GET_WEIGHT_INDEX(index, weight) \
  do {                                  \
    (index) = WEIGHT_INDEX(weight);     \
    if ((index) < 0) {                  \
      (index) = 0;                      \
    }                                   \
    else if ((index) > 8) {             \
      (index) = 8;                      \
    }                                   \
  } while (0)

nsFontXlib*
nsFontMetricsXlib::SearchNode(nsFontNodeXlib* aNode, PRUnichar aChar)
{
  if (aNode->mDummy) {
    return nsnull;
  }

  nsFontCharSetInfoXlib* charSetInfo = aNode->mCharSetInfo;

  






  if (charSetInfo->mCharSet) {
    PRUint16* ccmap = charSetInfo->mCCMap;
    if (ccmap) {
      for (int i = 0; i < mLoadedFontsCount; i++) {
        if (mLoadedFonts[i]->mCCMap == ccmap) {
          return nsnull;
        }
      }
    }
    else {
      if (!SetUpFontCharSetInfo(mFontMetricsContext, charSetInfo))
        return nsnull;
    }
  }
  else {
    if ((!mIsUserDefined) && (charSetInfo == mFontMetricsContext->mUnknown)) {
      return nsnull;
    }
  }

  aNode->FillStyleHoles();
  nsFontStyleXlib* style = aNode->mStyles[mStyleIndex];

  nsFontWeightXlib** weights = style->mWeights;
  int weight = mFont.weight;
  int steps = (weight % 100);
  int weightIndex;
  if (steps) {
    if (steps < 10) {
      int base = (weight - steps);
      GET_WEIGHT_INDEX(weightIndex, base);
      while (steps--) {
        nsFontWeightXlib* prev = weights[weightIndex];
        for (weightIndex++; weightIndex < 9; weightIndex++) {
          if (weights[weightIndex] != prev) {
            break;
          }
        }
        if (weightIndex >= 9) {
          weightIndex = 8;
        }
      }
    }
    else if (steps > 90) {
      steps = (100 - steps);
      int base = (weight + steps);
      GET_WEIGHT_INDEX(weightIndex, base);
      while (steps--) {
        nsFontWeightXlib* prev = weights[weightIndex];
        for (weightIndex--; weightIndex >= 0; weightIndex--) {
          if (weights[weightIndex] != prev) {
            break;
          }
        }
        if (weightIndex < 0) {
          weightIndex = 0;
        }
      }
    }
    else {
      GET_WEIGHT_INDEX(weightIndex, weight);
    }
  }
  else {
    GET_WEIGHT_INDEX(weightIndex, weight);
  }

  FIND_FONT_PRINTF(("        load font %s", aNode->mName.get()));
  return PickASizeAndLoad(weights[weightIndex]->mStretches[mStretchIndex],
    charSetInfo, aChar, aNode->mName.get());
}

static void 
SetFontLangGroupInfo(nsFontMetricsXlibContext *aFmctx, const nsFontCharSetMapXlib* aCharSetMap)
{
  nsFontLangGroupXlib *fontLangGroup = aCharSetMap->mFontLangGroup;
  if (!fontLangGroup)
    return;

  
  
  
  const char *langGroup = fontLangGroup->mFontLangGroupName;

  
  
  if (!langGroup)
    langGroup = "";
  if (!fontLangGroup->mFontLangGroupAtom) {
      fontLangGroup->mFontLangGroupAtom = NS_NewAtom(langGroup);
  }

  
  if (fontLangGroup->mFontLangGroupAtom == aFmctx->mZHTWHK) {
    langGroup = "zh-TW";
  }

  
  nsFontCharSetInfoXlib *charSetInfo = aCharSetMap->mInfo;
  if (!charSetInfo->mInitedSizeInfo) {
    charSetInfo->mInitedSizeInfo = PR_TRUE;

    nsCAutoString name;
    nsresult rv;
    name.Assign("font.scale.outline.min.");
    name.Append(langGroup);
    rv = aFmctx->mPref->GetIntPref(name.get(), &charSetInfo->mOutlineScaleMin);
    if (NS_SUCCEEDED(rv))
      SIZE_FONT_PRINTF(("%s = %d", name.get(), charSetInfo->mOutlineScaleMin));
    else
      charSetInfo->mOutlineScaleMin = aFmctx->mOutlineScaleMinimum;

#ifdef USE_AASB
    name.Assign("font.scale.aa_bitmap.min.");
    name.Append(langGroup);
    rv = aFmctx->mPref->GetIntPref(name.get(), &charSetInfo->mAABitmapScaleMin);
    if (NS_SUCCEEDED(rv))
      SIZE_FONT_PRINTF(("%s = %d", name.get(), charSetInfo->mAABitmapScaleMin));
    else
      charSetInfo->mAABitmapScaleMin = aFmctx->mAABitmapScaleMinimum;
#endif 

    name.Assign("font.scale.bitmap.min.");
    name.Append(langGroup);
    rv = aFmctx->mPref->GetIntPref(name.get(), &charSetInfo->mBitmapScaleMin);
    if (NS_SUCCEEDED(rv))
      SIZE_FONT_PRINTF(("%s = %d", name.get(), charSetInfo->mBitmapScaleMin));
    else
      charSetInfo->mBitmapScaleMin = aFmctx->mBitmapScaleMinimum;

    PRInt32 percent = 0;
#ifdef USE_AASB
    name.Assign("font.scale.aa_bitmap.oversize.");
    name.Append(langGroup);
    percent = 0;
    rv = mFontMetricsContext->mPref->GetIntPref(name.get(), &percent);
    if (NS_SUCCEEDED(rv)) {
      charSetInfo->mAABitmapOversize = percent/100.0;
      SIZE_FONT_PRINTF(("%s = %g", name.get(), charSetInfo->mAABitmapOversize));
    }
    else
      charSetInfo->mAABitmapOversize = aFmctx->mAABitmapOversize;

    percent = 0;
    name.Assign("font.scale.aa_bitmap.undersize.");
    name.Append(langGroup);
    rv = mFontMetricsContext->mPref->GetIntPref(name.get(), &percent);
    if (NS_SUCCEEDED(rv)) {
      charSetInfo->mAABitmapUndersize = percent/100.0;
      SIZE_FONT_PRINTF(("%s = %g", name.get(),charSetInfo->mAABitmapUndersize));
    }
    else
      charSetInfo->mAABitmapUndersize = aFmctx->mAABitmapUndersize;

    PRBool val = PR_TRUE;
    name.Assign("font.scale.aa_bitmap.always.");
    name.Append(langGroup);
    rv = mFontMetricsContext->mPref->GetBoolPref(name.get(), &val);
    if (NS_SUCCEEDED(rv)) {
      charSetInfo->mAABitmapScaleAlways = val;
      SIZE_FONT_PRINTF(("%s = %d", name.get(),charSetInfo->mAABitmapScaleAlways));
    }
    else
      charSetInfo->mAABitmapScaleAlways = aFmctx->mAABitmapScaleAlways;
#endif 

    percent = 0;
    name.Assign("font.scale.bitmap.oversize.");
    name.Append(langGroup);
    rv = aFmctx->mPref->GetIntPref(name.get(), &percent);
    if (NS_SUCCEEDED(rv)) {
      charSetInfo->mBitmapOversize = percent/100.0;
      SIZE_FONT_PRINTF(("%s = %g", name.get(), charSetInfo->mBitmapOversize));
    }
    else
      charSetInfo->mBitmapOversize = aFmctx->mBitmapOversize;

    percent = 0;
    name.Assign("font.scale.bitmap.undersize.");
    name.Append(langGroup);
    rv = aFmctx->mPref->GetIntPref(name.get(), &percent);
    if (NS_SUCCEEDED(rv)) {
      charSetInfo->mBitmapUndersize = percent/100.0;
      SIZE_FONT_PRINTF(("%s = %g", name.get(), charSetInfo->mBitmapUndersize));
    }
    else
      charSetInfo->mBitmapUndersize = aFmctx->mBitmapUndersize;
  }
}

static nsFontStyleXlib*
NodeGetStyle(nsFontNodeXlib* aNode, int aStyleIndex)
{
  nsFontStyleXlib* style = aNode->mStyles[aStyleIndex];
  if (!style) {
    style = new nsFontStyleXlib;
    if (!style) {
      return nsnull;
    }
    aNode->mStyles[aStyleIndex] = style;
  }
  return style;
}

static nsFontWeightXlib*
NodeGetWeight(nsFontStyleXlib* aStyle, int aWeightIndex)
{
  nsFontWeightXlib* weight = aStyle->mWeights[aWeightIndex];
  if (!weight) {
    weight = new nsFontWeightXlib;
    if (!weight) {
      return nsnull;
    }
    aStyle->mWeights[aWeightIndex] = weight;
  }
  return weight;
}

static nsFontStretchXlib* 
NodeGetStretch(nsFontWeightXlib* aWeight, int aStretchIndex)
{
  nsFontStretchXlib* stretch = aWeight->mStretches[aStretchIndex];
  if (!stretch) {
    stretch = new nsFontStretchXlib;
    if (!stretch) {
      return nsnull;
    }
    aWeight->mStretches[aStretchIndex] = stretch;
  }
  return stretch;
}

static PRBool
NodeAddScalable(nsFontStretchXlib* aStretch,
                PRBool aOutlineScaled, 
#ifdef USE_XPRINT
                PRBool aPrinterBuiltinFont, int aResX, int aResY,
#endif                 
                const char *aDashFoundry, const char *aFamily, 
                const char *aWeight,      const char * aSlant, 
                const char *aWidth,       const char *aStyle, 
                const char *aSpacing,     const char *aCharSet)
{
  
  
  if ((aStretch->mScalable) && (!aStretch->mOutlineScaled) 
      && (aOutlineScaled)) {
    PR_smprintf_free(aStretch->mScalable);
    aStretch->mScalable = nsnull;
  }
  if (!aStretch->mScalable) {
    aStretch->mOutlineScaled = aOutlineScaled;
#ifdef USE_XPRINT
    


    if (aPrinterBuiltinFont) {
      aStretch->mScalable = 
          PR_smprintf("%s-%s-%s-%s-%s-%s-%%d-*-%d-%d-%s-*-%s", 
          aDashFoundry, aFamily, aWeight, aSlant, aWidth, aStyle, 
          aResX, aResY, aSpacing, aCharSet);      
      if (!aStretch->mScalable)
        return PR_FALSE;
    }
    else 
#endif                 
    if (aOutlineScaled) {
      aStretch->mScalable = 
          PR_smprintf("%s-%s-%s-%s-%s-%s-%%d-*-0-0-%s-*-%s", 
          aDashFoundry, aFamily, aWeight, aSlant, aWidth, aStyle, 
          aSpacing, aCharSet);
      if (!aStretch->mScalable)
        return PR_FALSE;
    }
    else {
      aStretch->mScalable = 
          PR_smprintf("%s-%s-%s-%s-%s-%s-%%d-*-*-*-%s-*-%s", 
          aDashFoundry, aFamily, aWeight, aSlant, aWidth, aStyle, 
          aSpacing, aCharSet);
      if (!aStretch->mScalable)
        return PR_FALSE;
    }
  }
  return PR_TRUE;
}

static PRBool
NodeAddSize(nsFontMetricsXlibContext *aFmctx,
            nsFontStretchXlib* aStretch, 
            int aPixelSize, int aPointSize,
            float scaler,
            int aResX,      int aResY,
            const char *aDashFoundry, const char *aFamily, 
            const char *aWeight,      const char * aSlant, 
            const char *aWidth,       const char *aStyle, 
            const char *aSpacing,     const char *aCharSet,
            nsFontCharSetInfoXlib* aCharSetInfo)
{
  if (scaler!=1.0f)
  {
    aPixelSize = int(float(aPixelSize) * scaler);
    aPointSize = int(float(aPointSize) * scaler);
    aResX = 0;
    aResY = 0;
  }

  PRBool haveSize = PR_FALSE;
  if (aStretch->mSizesCount) {
    nsFontXlib** end = &aStretch->mSizes[aStretch->mSizesCount];
    nsFontXlib** s;
    for (s = aStretch->mSizes; s < end; s++) {
      if ((*s)->mSize == aPixelSize) {
        haveSize = PR_TRUE;
        break;
      }
    }
  }
  if (!haveSize) {
    if (aStretch->mSizesCount == aStretch->mSizesAlloc) {
      int newSize = 2 * (aStretch->mSizesAlloc ? aStretch->mSizesAlloc : 1);
      nsFontXlib** newSizes = new nsFontXlib*[newSize];
      if (!newSizes)
        return PR_FALSE;
      for (int j = aStretch->mSizesAlloc - 1; j >= 0; j--) {
        newSizes[j] = aStretch->mSizes[j];
      }
      aStretch->mSizesAlloc = newSize;
      delete [] aStretch->mSizes;
      aStretch->mSizes = newSizes;
    }
    char *name = PR_smprintf("%s-%s-%s-%s-%s-%s-%d-%d-%d-%d-%s-*-%s", 
                             aDashFoundry, aFamily, aWeight, aSlant, aWidth, aStyle, 
                             aPixelSize, aPointSize, aResX, aResY, aSpacing, aCharSet);  

    if (!name) {
      return PR_FALSE;
    }
    nsFontXlib* size = new nsFontXlibNormal(aFmctx);
    if (!size) {
      return PR_FALSE;
    }
    aStretch->mSizes[aStretch->mSizesCount++] = size;
    size->mName           = name;
    
    size->mSize           = aPixelSize;
    size->mBaselineAdjust = 0;
    size->mCCMap          = nsnull;
    size->mCharSetInfo    = aCharSetInfo;
  }
  return PR_TRUE;
}

static void
GetFontNames(nsFontMetricsXlibContext *aFmctx, const char* aPattern, PRBool aAnyFoundry, PRBool aOnlyOutlineScaledFonts, nsFontNodeArrayXlib* aNodes)
{
  Display *dpy = xxlib_rgb_get_display(aFmctx->mXlibRgbHandle);
  Screen  *scr = xxlib_rgb_get_screen (aFmctx->mXlibRgbHandle);

#ifdef NS_FONT_DEBUG_CALL_TRACE
  if (gFontDebug & NS_FONT_DEBUG_CALL_TRACE) {
    printf("GetFontNames %s\n", aPattern);
  }
#endif

#ifdef MOZ_ENABLE_FREETYPE2
  
  nsFT2FontCatalog::GetFontNames(aFmctx, aPattern, aNodes);
#endif 

  nsCAutoString previousNodeName;
  nsHashtable* node_hash;
  if (aAnyFoundry) {
    NS_ASSERTION(aPattern[1] == '*', "invalid 'anyFoundry' pattern");
    node_hash = &aFmctx->mAFRENodes;
  }
  else {
    node_hash = &aFmctx->mFFRENodes;
  }

#ifdef USE_XPRINT
#ifdef DEBUG
  if (aFmctx->mPrinterMode)
  {
    if (!dpy)
    {
      NS_ERROR("Obtaining font information without having a |Display *|");
      abort(); 
    }
  
    if (XpGetContext(dpy) == None)
    {
      
      NS_ERROR("Obtaining font information without valid print context (XListFonts()) _before_ XpSetContext()");
      abort(); 
    }    
  }               
#endif 
#endif 
  
#ifdef ENABLE_X_FONT_BANNING
  int  screen_xres,
       screen_yres;
  





#ifdef USE_XPRINT
  if (aFmctx->mPrinterMode) {
    Bool success;
    long x_dpi = 0,
         y_dpi = 0;
    success = XpuGetResolution(dpy, XpGetContext(dpy), &x_dpi, &y_dpi);
    NS_ASSERTION(success, "XpuGetResolution(dpy, XpGetContext(dpy), &x_dpi, &y_dpi)!");
    screen_xres = x_dpi;
    screen_yres = y_dpi;
  }
  else
#endif 
  {  
    screen_xres = int(((( double(::XWidthOfScreen(scr)))  * 25.4 / double(::XWidthMMOfScreen(scr)) ))  + 0.5);
    screen_yres = int(((( double(::XHeightOfScreen(scr))) * 25.4 / double(::XHeightMMOfScreen(scr)) )) + 0.5);
  }
#endif 
      
  BANNED_FONT_PRINTF(("Loading font '%s'", aPattern));
  



  int    count;
  char **list = ::XListFonts(dpy, aPattern, INT_MAX, &count);
  if ((!list) || (count < 1)) {
    return;
  }
  for (int i = 0; i < count; i++) {
    char name[256]; 

    



    if ((!list[i]) || (list[i][0] != '-') || (PL_strpbrk(list[i], "*?") != nsnull)) {
      continue;
    }

    strcpy(name, list[i]);
    
    char *p = name + 1;
    PRBool scalable       = PR_FALSE,
           outline_scaled = PR_FALSE;
#ifdef USE_XPRINT
    PRBool builtin_printer_font = PR_FALSE;
#endif 
    int    resX = -1,
           resY = -1;

#ifdef FIND_FIELD
#undef FIND_FIELD
#endif
#define FIND_FIELD(var)           \
  char* var = p;                  \
  while ((*p) && ((*p) != '-')) { \
    p++;                          \
  }                               \
  if (*p) {                       \
    *p++ = 0;                     \
  }                               \
  else {                          \
    continue;                     \
  }

#ifdef SKIP_FIELD
#undef SKIP_FIELD
#endif
#define SKIP_FIELD(var)           \
  while ((*p) && ((*p) != '-')) { \
    p++;                          \
  }                               \
  if (*p) {                       \
    p++;                          \
  }                               \
  else {                          \
    continue;                     \
  }

    FIND_FIELD(foundry);
    
    FIND_FIELD(familyName);
    FIND_FIELD(weightName);
    FIND_FIELD(slant);
    FIND_FIELD(setWidth);
    FIND_FIELD(addStyle);
    FIND_FIELD(pixelSize);
    if (pixelSize[0] == '0') {
      scalable = PR_TRUE;
    }
    FIND_FIELD(pointSize);
    if (pointSize[0] == '0') {
      scalable = PR_TRUE;
    }
    FIND_FIELD(resolutionX);
    resX = atoi(resolutionX);
    NS_ASSERTION(!(resolutionX[0] != '0' && resX == 0), "atoi(resolutionX) failure.");
    if (resolutionX[0] == '0') {
      scalable = PR_TRUE;
    }
    FIND_FIELD(resolutionY);
    resY = atoi(resolutionY);
    NS_ASSERTION(!(resolutionY[0] != '0' && resY == 0), "atoi(resolutionY) failure.");
    if (resolutionY[0] == '0') {
      scalable = PR_TRUE;
    }
    
    if ((pixelSize[0] != '0') || (pointSize[0] != '0')) {
      SCALED_FONT_PRINTF(("bitmap (non-scaled) font: %s", list[i]));
    }
    
    else if ((pixelSize[0] == '0') && (pointSize[0] == '0')
          && (resolutionX[0] != '0') && (resolutionY[0] != '0')) {
      SCALED_FONT_PRINTF(("bitmap scaled font: %s", list[i]));
    }
    
    else if ((pixelSize[0] == '0') && (pointSize[0] == '0')
          && (resolutionX[0] == '0') && (resolutionY[0] == '0')) {
      outline_scaled = PR_TRUE;
      SCALED_FONT_PRINTF(("outline scaled font: %s", list[i]));
    }
    else {
      SCALED_FONT_PRINTF(("unexpected font values: %s", list[i]));
      SCALED_FONT_PRINTF(("      pixelSize[0] = %c", pixelSize[0]));
      SCALED_FONT_PRINTF(("      pointSize[0] = %c", pointSize[0]));
      SCALED_FONT_PRINTF(("    resolutionX[0] = %c", resolutionX[0]));
      SCALED_FONT_PRINTF(("    resolutionY[0] = %c", resolutionY[0]));
      static PRBool already_complained = PR_FALSE;
      
      if (!already_complained) {
        already_complained = PR_TRUE;
        NS_ASSERTION(pixelSize[0] == '0', "font scaler type test failed");
        NS_ASSERTION(pointSize[0] == '0', "font scaler type test failed");
        NS_ASSERTION(resolutionX[0] == '0', "font scaler type test failed");
        NS_ASSERTION(resolutionY[0] == '0', "font scaler type test failed");
      }
    }
    FIND_FIELD(spacing);
    FIND_FIELD(averageWidth);

#ifdef USE_XPRINT
    








    if (aFmctx->mPrinterMode &&
        averageWidth[0] == '0' && 
        resX > screen_xres && 
        resY > screen_yres) {
      builtin_printer_font = PR_TRUE;
      
      outline_scaled = PR_TRUE;

      PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("This may be a built-in printer font '%s'\n", list[i]));
    }
#endif 

    if (averageWidth[0] == '0') {
      scalable = PR_TRUE;





      
#ifndef DISABLE_WORKAROUND_FOR_BUG_103159
      






      if ((pixelSize[0] != '0' || pointSize[0] != 0) && 
          (outline_scaled == PR_FALSE)
#ifdef USE_XPRINT
           && (builtin_printer_font == PR_FALSE)
#endif           
          ) {
        PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("rejecting font '%s' (via hardcoded workaround for bug 103159)\n", list[i]));
        BANNED_FONT_PRINTF(("rejecting font '%s' (via hardcoded workaround for bug 103159)", list[i]));          
        continue;
      }  
#endif 
    }
    char* charSetName = p; 
    if (!*charSetName) {
      continue;
    }
    
    if (aOnlyOutlineScaledFonts && (outline_scaled == PR_FALSE)) {
      continue;
    }

#ifdef ENABLE_X_FONT_BANNING
#define BOOL2STR(b) ((b)?("true"):("false"))    
    if (aFmctx->mFontRejectRegEx || aFmctx->mFontAcceptRegEx) {
      char fmatchbuf[512]; 
           
      sprintf(fmatchbuf, "fname=%s;scalable=%s;outline_scaled=%s;xdisplay=%s;xdpy=%d;ydpy=%d;xdevice=%s",
              list[i], 
              BOOL2STR(scalable), 
              BOOL2STR(outline_scaled),
              XDisplayString(dpy),
              screen_xres,
              screen_yres,
#ifdef USE_XPRINT
              aFmctx->mPrinterMode?("printer"):
#endif 
              ("display")
              );
#undef BOOL2STR
                  
      if (aFmctx->mFontRejectRegEx) {
                
        if (regexec(aFmctx->mFontRejectRegEx, fmatchbuf, 0, nsnull, 0) == REG_OK) {
          PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("rejecting font '%s' (via reject pattern)\n", fmatchbuf));
          BANNED_FONT_PRINTF(("rejecting font '%s' (via reject pattern)", fmatchbuf));
          continue;
        }  
      }

      if (aFmctx->mFontAcceptRegEx) {
        if (regexec(aFmctx->mFontAcceptRegEx, fmatchbuf, 0, nsnull, 0) == REG_NOMATCH) {
          PR_LOG(FontMetricsXlibLM, PR_LOG_DEBUG, ("rejecting font '%s' (via accept pattern)\n", fmatchbuf));
          BANNED_FONT_PRINTF(("rejecting font '%s' (via accept pattern)", fmatchbuf));
          continue;
        }
      }       
    }    
#endif     

    const nsFontCharSetMapXlib *charSetMap = GetCharSetMap(aFmctx, charSetName);
    nsFontCharSetInfoXlib* charSetInfo = charSetMap->mInfo;
    
    if (charSetInfo == aFmctx->mSpecial) {
      nsCAutoString familyCharSetName(familyName);
      familyCharSetName.Append('-');
      familyCharSetName.Append(charSetName);
      nsCStringKey familyCharSetKey(familyCharSetName);
      charSetMap = NS_STATIC_CAST(nsFontCharSetMapXlib*, aFmctx->mSpecialCharSets.Get(&familyCharSetKey));
      if (!charSetMap)
        charSetMap = aFmctx->mNoneCharSetMap;
      charSetInfo = charSetMap->mInfo;
    }
    if (!charSetInfo) {
#ifdef NOISY_FONTS
      printf("cannot find charset %s\n", charSetName);
#endif
      charSetInfo = aFmctx->mUnknown;
    }
    SetCharsetLangGroup(aFmctx, charSetInfo);
    SetFontLangGroupInfo(aFmctx, charSetMap);

    nsCAutoString nodeName;
    if (aAnyFoundry)
      nodeName.Assign('*');
    else
      nodeName.Assign(foundry);
    nodeName.Append('-');
    nodeName.Append(familyName);
    nodeName.Append('-');
    nodeName.Append(charSetName);
    nsCStringKey key(nodeName);
    nsFontNodeXlib* node = (nsFontNodeXlib*) node_hash->Get(&key);
    if (!node) {
      node = new nsFontNodeXlib;
      if (!node) {
        continue;
      }
      node_hash->Put(&key, node);
      node->mName = nodeName;
      node->mCharSetInfo = charSetInfo;
    }

    int found = 0;
    if (nodeName == previousNodeName) {
      found = 1;
    }
    else {
      found = (aNodes->IndexOf(node) >= 0);
    }
    previousNodeName = nodeName;
    if (!found) {
      aNodes->AppendElement(node);
    }

    int styleIndex;
    
    switch (slant[0]) {
    case 'i':
      styleIndex = NS_FONT_STYLE_ITALIC;
      break;
    case 'o':
      styleIndex = NS_FONT_STYLE_OBLIQUE;
      break;
    case 'r':
    default:
      styleIndex = NS_FONT_STYLE_NORMAL;
      break;
    }
    nsFontStyleXlib* style = NodeGetStyle(node, styleIndex);
    if (!style)
      continue;

    nsCStringKey weightKey(weightName);
    int weightNumber = NS_PTR_TO_INT32(aFmctx->mWeights.Get(&weightKey));
    if (!weightNumber) {
#ifdef NOISY_FONTS
      printf("cannot find weight %s\n", weightName);
#endif
      weightNumber = NS_FONT_WEIGHT_NORMAL;
    }
    int weightIndex = WEIGHT_INDEX(weightNumber);
    nsFontWeightXlib* weight = NodeGetWeight(style, weightIndex);
    if (!weight)
      continue;
  
    nsCStringKey setWidthKey(setWidth);
    int stretchIndex = NS_PTR_TO_INT32(aFmctx->mStretches.Get(&setWidthKey));
    if (!stretchIndex) {
#ifdef NOISY_FONTS
      printf("cannot find stretch %s\n", setWidth);
#endif
      stretchIndex = 5;
    }
    stretchIndex--;
    nsFontStretchXlib* stretch = NodeGetStretch(weight, stretchIndex);
    if (!stretch)
      continue;

    if (scalable) {
      if (!NodeAddScalable(stretch, outline_scaled,
#ifdef USE_XPRINT
                           builtin_printer_font, resX, resY, 
#endif 
                           name, familyName, weightName, slant, setWidth, addStyle, spacing, charSetName))
        continue;
    }
  
    
    int pixels,
        points;

    pixels = atoi(pixelSize);
    points = atoi(pointSize);

    if (pixels) {
      if (aFmctx->mScaleBitmapFontsWithDevScale && (aFmctx->mDevScale > 1.0f)) {
        
        if (!NodeAddSize(aFmctx, stretch, pixels, points, aFmctx->mDevScale, resX, resY, name, familyName, weightName, 
                         slant, setWidth, addStyle, spacing, charSetName, charSetInfo))
          continue;

        


        float minScaler = PR_MAX(aFmctx->mDevScale / 2.0f, 1.5f),
              maxScaler = aFmctx->mDevScale * 2.f,
              scaler;
        for( scaler = minScaler ; scaler <= maxScaler ; scaler += 0.5f )
        {
          if (!NodeAddSize(aFmctx, stretch, pixels, points, scaler, resX, resY, name, familyName, weightName, 
                           slant, setWidth, addStyle, spacing, charSetName, charSetInfo))
            break;
        }
        if (scaler <= maxScaler) {
          continue; 
        }
      }
      else
      {
        if (!NodeAddSize(aFmctx, stretch, pixels, points, 1.0f, resX, resY, name, familyName, weightName, 
                         slant, setWidth, addStyle, spacing, charSetName, charSetInfo))
          continue;     
      }
    }
  }
  XFreeFontNames(list);

#ifdef DEBUG_DUMP_TREE
  DumpTree();
#endif
}

static nsresult
GetAllFontNames(nsFontMetricsXlibContext *aFmctx)
{
  if (!aFmctx->mGlobalListInitalised) {
    aFmctx->mGlobalListInitalised = PR_TRUE;
    
    

    

    GetFontNames(aFmctx, "-*", PR_FALSE, PR_FALSE, &aFmctx->mGlobalList);
  }

  return NS_OK;
}

static nsFontFamilyXlib*
FindFamily(nsFontMetricsXlibContext *aFmctx, nsCString* aName)
{
  nsCStringKey key(*aName);
  nsFontFamilyXlib* family = (nsFontFamilyXlib*) aFmctx->mFamilies.Get(&key);
  if (!family) {
    family = new nsFontFamilyXlib();
    if (family) {
      char pattern[256];
      PR_snprintf(pattern, sizeof(pattern), "-*-%s-*-*-*-*-*-*-*-*-*-*-*-*",
        aName->get());
      GetFontNames(aFmctx, pattern, PR_TRUE, aFmctx->mForceOutlineScaledFonts, &family->mNodes);
      aFmctx->mFamilies.Put(&key, family);
    }
  }

  return family;
}

nsresult
nsFontMetricsXlib::FamilyExists(nsFontMetricsXlibContext *aFontMetricsContext, const nsString& aName)
{
  if (!global_fmctx) {
    global_fmctx = aFontMetricsContext;
  }

  if (!IsASCIIFontName(aName)) {
    return NS_ERROR_FAILURE;
  }

  nsCAutoString name;
  name.AssignWithConversion(aName.get());
  ToLowerCase(name);
  nsFontFamilyXlib* family = FindFamily(aFontMetricsContext, &name);
  if (family && family->mNodes.Count()) {
    return NS_OK;
  }

  return NS_ERROR_FAILURE;
}




static void
FFREToXLFDPattern(nsACString &aFFREName, nsACString &oPattern)
{
  PRInt32 charsetHyphen;

  oPattern.Append("-");
  oPattern.Append(aFFREName);
  
  charsetHyphen = oPattern.FindChar('-');
  charsetHyphen = oPattern.FindChar('-', charsetHyphen + 1);
  charsetHyphen = oPattern.FindChar('-', charsetHyphen + 1);
  oPattern.Insert("-*-*-*-*-*-*-*-*-*-*", charsetHyphen);
}




static void
FFRESubstituteCharset(nsACString &aFFREName,
                      const char *aReplacementCharset)
{
  PRInt32 charsetHyphen = aFFREName.FindChar('-');
  charsetHyphen = aFFREName.FindChar('-', charsetHyphen + 1);
  aFFREName.Truncate(charsetHyphen+1);
  aFFREName.Append(aReplacementCharset);
}




static void
FFRESubstituteEncoding(nsACString &aFFREName,
                       const char *aReplacementEncoding)
{
  PRInt32 encodingHyphen;
  
  encodingHyphen = aFFREName.FindChar('-');
  encodingHyphen = aFFREName.FindChar('-', encodingHyphen + 1);
  encodingHyphen = aFFREName.FindChar('-', encodingHyphen + 1);
  aFFREName.Truncate(encodingHyphen+1);
  aFFREName.Append(aReplacementEncoding);
}

nsFontXlib*
nsFontMetricsXlib::TryNodes(nsACString &aFFREName, PRUnichar aChar)
{
  const nsPromiseFlatCString& FFREName = PromiseFlatCString(aFFREName);

  FIND_FONT_PRINTF(("        TryNodes aFFREName = %s", FFREName.get()));
  nsCStringKey key(FFREName);
  PRBool anyFoundry = (FFREName.First() == '*');
  nsFontNodeArrayXlib* nodes = (nsFontNodeArrayXlib*) mFontMetricsContext->mCachedFFRESearches.Get(&key);
  if (!nodes) {
    nsCAutoString pattern;
    FFREToXLFDPattern(aFFREName, pattern);
    nodes = new nsFontNodeArrayXlib;
    if (!nodes)
      return nsnull;
    GetFontNames(mFontMetricsContext, pattern.get(), anyFoundry, mFontMetricsContext->mForceOutlineScaledFonts, nodes);
    mFontMetricsContext->mCachedFFRESearches.Put(&key, nodes);
  }
  int i, cnt = nodes->Count();
  for (i=0; i<cnt; i++) {
    nsFontNodeXlib* node = nodes->GetElement(i);
    nsFontXlib * font;
    font = SearchNode(node, aChar);
    if (font && font->SupportsChar(aChar))
      return font;
  }
  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::TryNode(nsCString* aName, PRUnichar aChar)
{
  FIND_FONT_PRINTF(("        TryNode aName = %s", (*aName).get()));
  
  
  
  if (aName->IsEmpty()) {
    return nsnull;
  }
  nsFontXlib* font;
 
  nsCStringKey key(*aName);
  nsFontNodeXlib* node = (nsFontNodeXlib*) mFontMetricsContext->mFFRENodes.Get(&key);
  if (!node) {
    nsCAutoString pattern;
    FFREToXLFDPattern(*aName, pattern);
    nsFontNodeArrayXlib nodes;
    GetFontNames(mFontMetricsContext, pattern.get(), PR_FALSE, mFontMetricsContext->mForceOutlineScaledFonts, &nodes);
    
    if (nodes.Count() > 0) {
      
      
      NS_ASSERTION((nodes.Count() == 1), "unexpected number of nodes");
      node = nodes.GetElement(0);
    }
    else {
      
      node = new nsFontNodeXlib();
      if (!node) {
        return nsnull;
      }
      mFontMetricsContext->mFFRENodes.Put(&key, node);
      node->mDummy = 1;
    }
  }

  if (node) {
    font = SearchNode(node, aChar);
    if (font && font->SupportsChar(aChar))
      return font;
  }

  
  
  
  if (mIsUserDefined) {
    return nsnull;
  }
  
  
  
  nsCAutoString ffreName(*aName);
  FFRESubstituteEncoding(ffreName, "*");
  FIND_FONT_PRINTF(("        TrySubplane: wild-card the encoding"));
  font = TryNodes(ffreName, aChar);
  if (font) {
    NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
    return font;
  }
  return nsnull;
}

nsFontXlib* 
nsFontMetricsXlib::TryLangGroup(nsIAtom* aLangGroup, nsCString* aName, PRUnichar aChar)
{
  
  
  
  FIND_FONT_PRINTF(("      TryLangGroup lang group = %s, aName = %s", 
                            atomToName(aLangGroup), (*aName).get()));
  if (aName->IsEmpty()) {
    return nsnull;
  }
  nsFontXlib* font = FindLangGroupFont(aLangGroup, aChar, aName);
  return font;
}

nsFontXlib*
nsFontMetricsXlib::TryFamily(nsCString* aName, PRUnichar aChar)
{
  
  
  
  nsFontFamilyXlib* family = FindFamily(mFontMetricsContext, aName);
  if (family) {
    
    nsCAutoString FFREName("*-");
    FFREName.Append(*aName);
    FFREName.Append("-*-*");
    FIND_FONT_PRINTF(("        TryFamily %s with lang group = %s", (*aName).get(),
                                                         atomToName(mLangGroup)));
    nsFontXlib* font = TryLangGroup(mLangGroup, &FFREName, aChar);
    if(font) {
      return font;
    }

    
    nsFontNodeArrayXlib* nodes = &family->mNodes;
    PRInt32 n = nodes->Count();
    for (PRInt32 i = 0; i < n; i++) {
      FIND_FONT_PRINTF(("        TryFamily %s", nodes->GetElement(i)->mName.get()));
      nsFontXlib* font = SearchNode(nodes->GetElement(i), aChar);
      if (font && font->SupportsChar(aChar)) {
        return font;
      }
    }
  }

  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::TryAliases(nsCString* aAlias, PRUnichar aChar)
{
  nsCStringKey key(*aAlias);
  char* name = (char*) mFontMetricsContext->mAliases.Get(&key);
  if (name) {
    nsCAutoString str(name);
    return TryFamily(&str, aChar);
  }

  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::FindUserDefinedFont(PRUnichar aChar)
{
  if (mIsUserDefined) {
    FIND_FONT_PRINTF(("        FindUserDefinedFont"));
    nsFontXlib* font = TryNode(&mUserDefined, aChar);
    mIsUserDefined = PR_FALSE;
    if (font) {
      NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
      return font;
    }
  }

  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::FindStyleSheetSpecificFont(PRUnichar aChar)
{
  FIND_FONT_PRINTF(("    FindStyleSheetSpecificFont"));
  while (mFontsIndex < mFonts.Count()) {
    if (mFontIsGeneric[mFontsIndex]) {
      return nsnull;
    }
    nsCString* familyName = mFonts.CStringAt(mFontsIndex);

    




    const char* str = familyName->get();
    FIND_FONT_PRINTF(("        familyName = %s", str));
    PRUint32 len = familyName->Length();
    int hyphens = 0;
    for (PRUint32 i = 0; i < len; i++) {
      if (str[i] == '-') {
        hyphens++;
      }
    }

    










    nsFontXlib* font;
    if (hyphens == 3) {
      font = TryNode(familyName, aChar);
      if (font) {
        NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
        return font;
      }
    }
    else {
      font = TryFamily(familyName, aChar);
      if (font) {
        NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
        return font;
      }
      font = TryAliases(familyName, aChar);
      if (font) {
        NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
        return font;
      }
    }
    
    mFontsIndex++;
  }

  return nsnull;
}

static void
PrefEnumCallback(const char* aName, void* aClosure)
{
  nsFontSearch* s = (nsFontSearch*) aClosure;
  nsFontMetricsXlibContext *aFmctx = s->mMetrics->mFontMetricsContext;

  if (s->mFont) {
    NS_ASSERTION(s->mFont->SupportsChar(s->mChar), "font supposed to support this char");
    return;
  }
  nsXPIDLCString value;
  aFmctx->mPref->CopyCharPref(aName, getter_Copies(value));
  nsCAutoString name;
  if (value.get()) {
    name = value;
    FIND_FONT_PRINTF(("       PrefEnumCallback"));
    s->mFont = s->mMetrics->TryNode(&name, s->mChar);
    if (s->mFont) {
      NS_ASSERTION(s->mFont->SupportsChar(s->mChar), "font supposed to support this char");
      return;
    }
    s->mFont = s->mMetrics->TryLangGroup(s->mMetrics->mLangGroup, &name, s->mChar);
    if (s->mFont) {
      NS_ASSERTION(s->mFont->SupportsChar(s->mChar), "font supposed to support this char");
      return;
    }
  }
  aFmctx->mPref->CopyDefaultCharPref(aName, getter_Copies(value));
  if (value.get() && (!name.Equals(value))) {
    name = value;
    FIND_FONT_PRINTF(("       PrefEnumCallback:default"));
    s->mFont = s->mMetrics->TryNode(&name, s->mChar);
    if (s->mFont) {
      NS_ASSERTION(s->mFont->SupportsChar(s->mChar), "font supposed to support this char");
      return;
    }
    s->mFont = s->mMetrics->TryLangGroup(s->mMetrics->mLangGroup, &name, s->mChar);
    NS_ASSERTION(s->mFont ? s->mFont->SupportsChar(s->mChar) : 1, "font supposed to support this char");
  }
}

nsFontXlib*
nsFontMetricsXlib::FindStyleSheetGenericFont(PRUnichar aChar)
{
  FIND_FONT_PRINTF(("    FindStyleSheetGenericFont"));
  nsFontXlib* font;

  if (mTriedAllGenerics) {
    return nsnull;
  }

  
  
  
  font = FindLangGroupPrefFont(mLangGroup, aChar);
  if (font) {
    NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
    return font;
  }

  
  
  
  
  
  
  
  
  
if (mFontMetricsContext->mAllowDoubleByteSpecialChars) {
  if (!mDocConverterType) {
    if (mLoadedFontsCount) {
      FIND_FONT_PRINTF(("just use the 1st converter type"));
      nsFontXlib* first_font = mLoadedFonts[0];
      if (first_font->mCharSetInfo) {
        mDocConverterType = first_font->mCharSetInfo->Convert;
        if (mDocConverterType == SingleByteConvert ) {
          FIND_FONT_PRINTF(("single byte converter for %s", atomToName(mLangGroup)));
        }
        else {
          FIND_FONT_PRINTF(("double byte converter for %s", atomToName(mLangGroup)));
        }
      }
    }
    if (!mDocConverterType) {
      mDocConverterType = SingleByteConvert;
    }
    if (mDocConverterType == SingleByteConvert) {
      
      
      

      nsFontXlib* western_font = nsnull;
      if (mLangGroup != mFontMetricsContext->mWesternLocale)
        western_font = FindLangGroupPrefFont(mFontMetricsContext->mWesternLocale, aChar);

      
      
      nsCAutoString symbol_ffre("*-symbol-adobe-fontspecific");
      nsFontXlib* symbol_font = TryNodes(symbol_ffre, 0x0030);

      
      nsCAutoString euro_ffre("*-euro*-adobe-fontspecific");
      nsFontXlib* euro_font = TryNodes(euro_ffre, 0x20AC);

      
      
      
      nsFontXlib* sub_font = FindSubstituteFont(aChar);
      NS_ASSERTION(sub_font, "failed to get a special chars substitute font");
      if (sub_font) {
        sub_font->mCCMap = mFontMetricsContext->mDoubleByteSpecialCharsCCMap;
        AddToLoadedFontsList(sub_font);
      }
      if (western_font && CCMAP_HAS_CHAR(western_font->mCCMap, aChar)) {
        return western_font;
      }
      else if (symbol_font && CCMAP_HAS_CHAR(symbol_font->mCCMap, aChar)) {
        return symbol_font;
      }
      else if (euro_font && CCMAP_HAS_CHAR(euro_font->mCCMap, aChar)) {
        return euro_font;
      }
      else if (sub_font && CCMAP_HAS_CHAR(sub_font->mCCMap, aChar)) {
        FIND_FONT_PRINTF(("      transliterate special chars for single byte docs"));
        return sub_font;
      }
    }
  }
}

  
  
  
  if (mFontMetricsContext->mUsersLocale != mLangGroup) {
    FIND_FONT_PRINTF(("      find font based on user's locale's lang group"));
    font = FindLangGroupPrefFont(mFontMetricsContext->mUsersLocale, aChar);
    if (font) {
      NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
      return font;
    }
  }

  
  
  
  nsCAutoString prefix("font.name.");
  prefix.Append(*mGeneric);
  nsFontSearch search = { this, aChar, nsnull };
  FIND_FONT_PRINTF(("      Search all font prefs for generic"));
  mFontMetricsContext->mPref->EnumerateChildren(prefix.get(), PrefEnumCallback, &search);
  if (search.mFont) {
    NS_ASSERTION(search.mFont->SupportsChar(aChar), "font supposed to support this char");
    return search.mFont;
  }

  
  
  
  
  nsCAutoString allPrefs("font.name.");
  search.mFont = nsnull;
  FIND_FONT_PRINTF(("      Search all font prefs"));
  mFontMetricsContext->mPref->EnumerateChildren(allPrefs.get(), PrefEnumCallback, &search);
  if (search.mFont) {
    NS_ASSERTION(search.mFont->SupportsChar(aChar), "font supposed to support this char");
    return search.mFont;
  }

  mTriedAllGenerics = 1;
  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::FindAnyFont(PRUnichar aChar)
{
  FIND_FONT_PRINTF(("    FindAnyFont"));

  
  
  
  

  


  nsresult res = GetAllFontNames(mFontMetricsContext);
  if (NS_FAILED(res))
    return nsnull;

  PRInt32 n = mFontMetricsContext->mGlobalList.Count();
  for (PRInt32 i = 0; i < n; i++) {
    nsFontXlib* font = SearchNode(mFontMetricsContext->mGlobalList.GetElement(i), aChar);
    if (font && font->SupportsChar(aChar)) {
      
      
      return font;
    }
  }

  
  
  
  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::FindSubstituteFont(PRUnichar aChar)
{
  if (!mSubstituteFont) {
    for (int i = 0; i < mLoadedFontsCount; i++) {
      if (CCMAP_HAS_CHAR(mLoadedFonts[i]->mCCMap, 'a')) {
        mSubstituteFont = new nsFontXlibSubstitute(mLoadedFonts[i]);
        break;
      }
    }
    
    
    
    
    
    
    
    
    
  }
  

  
  
  
  
  
  
  
  

  return mSubstituteFont;
}





nsFontXlib* 
nsFontMetricsXlib::FindLangGroupPrefFont(nsIAtom* aLangGroup, PRUnichar aChar)
{ 
  nsFontXlib* font;
  
  
  
  nsCAutoString prefix("font.name."); 
  prefix.Append(*mGeneric); 
  if (aLangGroup) { 
    
    nsCAutoString pref = prefix;
    pref.Append(char('.'));
    const char* langGroup = nsnull;
    aLangGroup->GetUTF8String(&langGroup);
    pref.Append(langGroup);
    nsXPIDLCString value;
    mFontMetricsContext->mPref->CopyCharPref(pref.get(), getter_Copies(value));
    nsCAutoString str;
    nsCAutoString str_user;
    if (value.get()) {
      str = value.get();
      str_user = value.get();
      FIND_FONT_PRINTF(("      user pref %s = %s", pref.get(), str.get()));
      font = TryNode(&str, aChar);
      if (font) {
        NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
        return font;
      }
      font = TryLangGroup(aLangGroup, &str, aChar);
      if (font) {
        NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
        return font;
      }
    }
    
    mFontMetricsContext->mPref->CopyDefaultCharPref(pref.get(), getter_Copies(value));
    if (value.get()) {
      str = value.get();
      
      if (str != str_user) {
        FIND_FONT_PRINTF(("      default pref %s = %s", pref.get(), str.get()));
        font = TryNode(&str, aChar);
        if (font) {
          NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
          return font;
        }
        font = TryLangGroup(aLangGroup, &str, aChar);
        if (font) {
          NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
          return font;
        }
      }
    }
  }

  
  
  
  FIND_FONT_PRINTF(("      find font based on lang group"));
  font = FindLangGroupFont(aLangGroup, aChar, nsnull);
  if (font) {
    NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
    return font;
  }

  return nsnull;
}

nsFontXlib*
nsFontMetricsXlib::FindLangGroupFont(nsIAtom* aLangGroup, PRUnichar aChar, nsCString *aName)
{
  nsFontXlib* font;

  FIND_FONT_PRINTF(("      lang group = %s", atomToName(aLangGroup)));

  
  const nsFontCharSetMapXlib* charSetMap;
  for (charSetMap=mFontMetricsContext->mCharSetMap; charSetMap->mName; charSetMap++) {
    nsFontLangGroupXlib* fontLangGroup = charSetMap->mFontLangGroup;

    if ((!fontLangGroup) || (!fontLangGroup->mFontLangGroupName)) {
      continue;
    }

    if (!charSetMap->mInfo->mLangGroup) {
      SetCharsetLangGroup(mFontMetricsContext, charSetMap->mInfo);
    }

    if (!fontLangGroup->mFontLangGroupAtom) {
      SetFontLangGroupInfo(mFontMetricsContext, charSetMap);
    }

    
    
    
    if ((aLangGroup != fontLangGroup->mFontLangGroupAtom) &&
        (aLangGroup != charSetMap->mInfo->mLangGroup) &&
        (fontLangGroup->mFontLangGroupAtom != mFontMetricsContext->mZHTWHK || 
        (aLangGroup != mFontMetricsContext->mZHHK && aLangGroup != mFontMetricsContext->mZHTW))) {
      continue;
    }
    
    
    nsCAutoString ffreName;
    if(aName) {
      
      ffreName.Assign(*aName);
      FFRESubstituteCharset(ffreName, charSetMap->mName); 
      FIND_FONT_PRINTF(("      %s ffre = %s", charSetMap->mName, ffreName.get()));
      if(aName->First() == '*') {
         
         font = TryNodes(ffreName, aChar);
      } else {
         font = TryNode(&ffreName, aChar);
      }
      NS_ASSERTION(font ? font->SupportsChar(aChar) : 1, "font supposed to support this char");
    } else {
      
      ffreName.Assign("*-*-*-*");
      FFRESubstituteCharset(ffreName, charSetMap->mName); 
      FIND_FONT_PRINTF(("      %s ffre = %s", charSetMap->mName, ffreName.get()));
      font = TryNodes(ffreName, aChar);
      NS_ASSERTION(font ? font->SupportsChar(aChar) : 1, "font supposed to support this char");
    }
    if (font) {
      NS_ASSERTION(font->SupportsChar(aChar), "font supposed to support this char");
      return font;
    }
  }

  return nsnull;
}
















nsFontXlib*
nsFontMetricsXlib::FindFont(PRUnichar aChar)
{
  FIND_FONT_PRINTF(("\nFindFont(%c/0x%04x)", aChar, aChar));

  
  
  
  if (aChar == UCS2_NOMAPPING) {
    FIND_FONT_PRINTF(("      ignore the 'UCS2_NOMAPPING' character, return mWesternFont"));
    return mWesternFont;
  }

  nsFontXlib* font = FindUserDefinedFont(aChar);
  if (!font) {
    font = FindStyleSheetSpecificFont(aChar);
    if (!font) {
      font = FindStyleSheetGenericFont(aChar);
      if (!font) {
        font = FindAnyFont(aChar);
        if (!font) {
          font = FindSubstituteFont(aChar);
        }
      }
    }
  }

#ifdef NS_FONT_DEBUG_CALL_TRACE
  if (gFontDebug & NS_FONT_DEBUG_CALL_TRACE) {
    printf("FindFont(%04X)[", aChar);
    for (PRInt32 i = 0; i < mFonts.Count(); i++) {
      printf("%s, ", mFonts.CStringAt(i)->get());
    }
    printf("]\nreturns ");
    if (font) {
      printf("%s\n", font->mName ? font->mName : "(substitute)");
    }
    else {
      printf("NULL\n");
    }
  }
#endif

  return font;
}




nsFontEnumeratorXlib::nsFontEnumeratorXlib()
{
}

NS_IMPL_ISUPPORTS1(nsFontEnumeratorXlib, nsIFontEnumerator)

typedef struct EnumerateNodeInfo
{
  PRUnichar** mArray;
  int         mIndex;
  nsIAtom*    mLangGroup;
  nsFontMetricsXlibContext *mFontMetricsContext;
} EnumerateNodeInfo;

static PRIntn
EnumerateNode(void* aElement, void* aData)
{
  nsFontNodeXlib* node = (nsFontNodeXlib*) aElement;
  EnumerateNodeInfo* info = (EnumerateNodeInfo*) aData;
  nsFontMetricsXlibContext *aFmctx = info->mFontMetricsContext;
  if (info->mLangGroup != aFmctx->mUserDefined) {
    if (node->mCharSetInfo == aFmctx->mUnknown) {
      return PR_TRUE; 
    }
    else if (info->mLangGroup != aFmctx->mUnicode) {
      
      
      
      if (node->mCharSetInfo->mLangGroup != info->mLangGroup &&
         (node->mCharSetInfo->mLangGroup != aFmctx->mZHTWHK ||
         (info->mLangGroup != aFmctx->mZHHK && 
          info->mLangGroup != aFmctx->mZHTW))) {
        return PR_TRUE; 
      }
    }
    
    
    
    
    
    
    
    
    
    
    
    
    
    

  }
  PRUnichar** array = info->mArray;
  int j = info->mIndex;
  PRUnichar* str = ToNewUnicode(node->mName);
  if (!str) {
    for (j = j - 1; j >= 0; j--) {
      nsMemory::Free(array[j]);
    }
    info->mIndex = 0;
    return PR_FALSE; 
  }
  array[j] = str;
  info->mIndex++;

  return PR_TRUE; 
}

PR_BEGIN_EXTERN_C
static int
CompareFontNames(const void* aArg1, const void* aArg2, void* aClosure)
{
  const PRUnichar* str1 = *((const PRUnichar**) aArg1);
  const PRUnichar* str2 = *((const PRUnichar**) aArg2);

  

  return nsCRT::strcmp(str1, str2);
}
PR_END_EXTERN_C

static nsresult
EnumFonts(nsFontMetricsXlibContext *aFmctx, nsIAtom* aLangGroup, const char* aGeneric, PRUint32* aCount,
  PRUnichar*** aResult)
{
  nsresult res = GetAllFontNames(aFmctx);
  if (NS_FAILED(res))
    return res;

  PRUnichar** array =
    (PRUnichar**) nsMemory::Alloc(aFmctx->mGlobalList.Count() * sizeof(PRUnichar*));
  if (!array)
    return NS_ERROR_OUT_OF_MEMORY;

  EnumerateNodeInfo info = { array, 0, aLangGroup, aFmctx };
  if (!aFmctx->mGlobalList.EnumerateForwards(EnumerateNode, &info)) {
    nsMemory::Free(array);
    return NS_ERROR_OUT_OF_MEMORY;
  }

  NS_QuickSort(array, info.mIndex, sizeof(PRUnichar*), CompareFontNames,
               nsnull);

  *aCount = info.mIndex;
  if (*aCount) {
    *aResult = array;
  }
  else {
    nsMemory::Free(array);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorXlib::EnumerateAllFonts(PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  return EnumFonts(global_fmctx, nsnull, nsnull, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorXlib::EnumerateFonts(const char* aLangGroup,
  const char* aGeneric, PRUint32* aCount, PRUnichar*** aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;
  NS_ENSURE_ARG_POINTER(aCount);
  *aCount = 0;

  
  
  nsCOMPtr<nsIAtom> langGroup;
  if (aLangGroup && *aLangGroup)
    langGroup = do_GetAtom(aLangGroup);
  const char* generic = nsnull;
  if (aGeneric && *aGeneric)
    generic = aGeneric;

  
  return EnumFonts(global_fmctx, langGroup, generic, aCount, aResult);
}

NS_IMETHODIMP
nsFontEnumeratorXlib::HaveFontFor(const char* aLangGroup, PRBool* aResult)
{
  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = PR_FALSE;
  NS_ENSURE_ARG_POINTER(aLangGroup);

  *aResult = PR_TRUE; 
  
  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorXlib::GetDefaultFont(const char *aLangGroup, 
  const char *aGeneric, PRUnichar **aResult)
{
  
  

  NS_ENSURE_ARG_POINTER(aResult);
  *aResult = nsnull;

  return NS_OK;
}

NS_IMETHODIMP
nsFontEnumeratorXlib::UpdateFontList(PRBool *updateFontList)
{
  *updateFontList = PR_FALSE; 
  return NS_OK;
}

static
const nsFontCharSetMapXlib *GetCharSetMap(nsFontMetricsXlibContext *aFmctx, const char *aCharSetName)
{
  nsCStringKey charSetKey(aCharSetName);
  const nsFontCharSetMapXlib* charSetMap = (const nsFontCharSetMapXlib *) aFmctx->mCharSetMaps.Get(&charSetKey);
  if (!charSetMap)
    charSetMap = aFmctx->mNoneCharSetMap;
  return charSetMap;
}

#ifdef MOZ_ENABLE_FREETYPE2
static
void CharSetNameToCodeRangeBits(const char *aCharset,
                                PRUint32 *aCodeRange1, PRUint32 *aCodeRange2)
{
  nsFontCharSetMapXlib  *charSetMap  = GetCharSetMap(aFmctx, aCharset);
  nsFontCharSetInfoXlib *charSetInfo = charSetMap->mInfo;

  *aCodeRange1 = charSetInfo->mCodeRange1Bits;
  *aCodeRange2 = charSetInfo->mCodeRange2Bits;
}
#endif 
