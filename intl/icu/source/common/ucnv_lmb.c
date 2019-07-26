
























#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "unicode/ucnv_err.h"
#include "unicode/ucnv.h"
#include "unicode/uset.h"
#include "cmemory.h"
#include "cstring.h"
#include "uassert.h"
#include "ucnv_imp.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"

#ifdef EBCDIC_RTL
    #include "ascii_a.h"
#endif

#define LENGTHOF(array) (int32_t)(sizeof(array)/sizeof((array)[0]))









































#define ULMBCS_CHARSIZE_MAX      3





#define ULMBCS_C0END           0x1F   
#define ULMBCS_C1START         0x80   




  
typedef uint8_t ulmbcs_byte_t;








#define ULMBCS_GRP_L1         0x01   /* Latin-1    :ibm-850  */
#define ULMBCS_GRP_GR         0x02   /* Greek      :ibm-851  */
#define ULMBCS_GRP_HE         0x03   /* Hebrew     :ibm-1255 */
#define ULMBCS_GRP_AR         0x04   /* Arabic     :ibm-1256 */
#define ULMBCS_GRP_RU         0x05   /* Cyrillic   :ibm-1251 */
#define ULMBCS_GRP_L2         0x06   /* Latin-2    :ibm-852  */
#define ULMBCS_GRP_TR         0x08   /* Turkish    :ibm-1254 */
#define ULMBCS_GRP_TH         0x0B   /* Thai       :ibm-874  */
#define ULMBCS_GRP_JA         0x10   /* Japanese   :ibm-943  */
#define ULMBCS_GRP_KO         0x11   /* Korean     :ibm-1261 */
#define ULMBCS_GRP_TW         0x12   /* Chinese SC :ibm-950  */
#define ULMBCS_GRP_CN         0x13   /* Chinese TC :ibm-1386 */













#define ULMBCS_DOUBLEOPTGROUP_START  0x10   




















                     
#define ULMBCS_HT    0x09   /* Fixed control char - Horizontal Tab */
#define ULMBCS_LF    0x0A   /* Fixed control char - Line Feed */
#define ULMBCS_CR    0x0D   /* Fixed control char - Carriage Return */




#define ULMBCS_123SYSTEMRANGE  0x19   






#define ULMBCS_GRP_CTRL       0x0F   




#define ULMBCS_CTRLOFFSET      0x20   














#define ULMBCS_GRP_EXCEPT     0x00    




#define ULMBCS_GRP_UNICODE    0x14   






#define ULMBCS_UNICOMPATZERO   0xF6   



#define ULMBCS_UNICODE_SIZE      3    








#define ULMBCS_DEFAULTOPTGROUP 0x1    




















#define ULMBCS_GRP_LAST       0x13   /* last LMBCS group that has a converter */

static const char * const OptGroupByteToCPName[ULMBCS_GRP_LAST + 1] = {
    "lmb-excp", 
    "ibm-850",
    "ibm-851",
    "windows-1255",
    "windows-1256",
    "windows-1251",
    "ibm-852",
    NULL,      
    "windows-1254",
    NULL,      
    NULL,      
    "windows-874",
    NULL,      
    NULL,      
    NULL,      
    NULL,      
    "windows-932",
    "windows-949",
    "windows-950",
    "windows-936"

   
      
};


















#define ULMBCS_AMBIGUOUS_SBCS   0x80   /* could fit in more than one 
                                          LMBCS sbcs native encoding 
                                          (example: most accented latin) */
#define ULMBCS_AMBIGUOUS_MBCS   0x81   /* could fit in more than one 
                                          LMBCS mbcs native encoding 
                                          (example: Unihan) */
#define ULMBCS_AMBIGUOUS_ALL   0x82

#define ULMBCS_AMBIGUOUS_MATCH(agroup, xgroup) \
                  ((((agroup) == ULMBCS_AMBIGUOUS_SBCS) && \
                  (xgroup) < ULMBCS_DOUBLEOPTGROUP_START) || \
                  (((agroup) == ULMBCS_AMBIGUOUS_MBCS) && \
                  (xgroup) >= ULMBCS_DOUBLEOPTGROUP_START)) || \
                  ((agroup) == ULMBCS_AMBIGUOUS_ALL)





static const struct _UniLMBCSGrpMap  
{
   const UChar uniStartRange;
   const UChar uniEndRange;
   const ulmbcs_byte_t  GrpType;
} UniLMBCSGrpMap[]
=
{

    {0x0001, 0x001F,  ULMBCS_GRP_CTRL},
    {0x0080, 0x009F,  ULMBCS_GRP_CTRL},
    {0x00A0, 0x00A6,  ULMBCS_AMBIGUOUS_SBCS},
    {0x00A7, 0x00A8,  ULMBCS_AMBIGUOUS_ALL},
    {0x00A9, 0x00AF,  ULMBCS_AMBIGUOUS_SBCS},
    {0x00B0, 0x00B1,  ULMBCS_AMBIGUOUS_ALL},
    {0x00B2, 0x00B3,  ULMBCS_AMBIGUOUS_SBCS},
    {0x00B4, 0x00B4,  ULMBCS_AMBIGUOUS_ALL},
    {0x00B5, 0x00B5,  ULMBCS_AMBIGUOUS_SBCS},
    {0x00B6, 0x00B6,  ULMBCS_AMBIGUOUS_ALL},
    {0x00B7, 0x00D6,  ULMBCS_AMBIGUOUS_SBCS},
    {0x00D7, 0x00D7,  ULMBCS_AMBIGUOUS_ALL},
    {0x00D8, 0x00F6,  ULMBCS_AMBIGUOUS_SBCS},
    {0x00F7, 0x00F7,  ULMBCS_AMBIGUOUS_ALL},
    {0x00F8, 0x01CD,  ULMBCS_AMBIGUOUS_SBCS},
    {0x01CE, 0x01CE,  ULMBCS_GRP_TW },
    {0x01CF, 0x02B9,  ULMBCS_AMBIGUOUS_SBCS},
    {0x02BA, 0x02BA,  ULMBCS_GRP_CN},
    {0x02BC, 0x02C8,  ULMBCS_AMBIGUOUS_SBCS},
    {0x02C9, 0x02D0,  ULMBCS_AMBIGUOUS_MBCS},
    {0x02D8, 0x02DD,  ULMBCS_AMBIGUOUS_SBCS},
    {0x0384, 0x0390,  ULMBCS_AMBIGUOUS_SBCS},
    {0x0391, 0x03A9,  ULMBCS_AMBIGUOUS_ALL},
    {0x03AA, 0x03B0,  ULMBCS_AMBIGUOUS_SBCS},
    {0x03B1, 0x03C9,  ULMBCS_AMBIGUOUS_ALL},
    {0x03CA, 0x03CE,  ULMBCS_AMBIGUOUS_SBCS},
    {0x0400, 0x0400,  ULMBCS_GRP_RU},
    {0x0401, 0x0401,  ULMBCS_AMBIGUOUS_ALL},
    {0x0402, 0x040F,  ULMBCS_GRP_RU},
    {0x0410, 0x0431,  ULMBCS_AMBIGUOUS_ALL},
    {0x0432, 0x044E,  ULMBCS_GRP_RU},
    {0x044F, 0x044F,  ULMBCS_AMBIGUOUS_ALL},
    {0x0450, 0x0491,  ULMBCS_GRP_RU},
    {0x05B0, 0x05F2,  ULMBCS_GRP_HE},
    {0x060C, 0x06AF,  ULMBCS_GRP_AR},
    {0x0E01, 0x0E5B,  ULMBCS_GRP_TH},
    {0x200C, 0x200F,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2010, 0x2010,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2013, 0x2014,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2015, 0x2015,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2016, 0x2016,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2017, 0x2017,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2018, 0x2019,  ULMBCS_AMBIGUOUS_ALL},
    {0x201A, 0x201B,  ULMBCS_AMBIGUOUS_SBCS},
    {0x201C, 0x201D,  ULMBCS_AMBIGUOUS_ALL},
    {0x201E, 0x201F,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2020, 0x2021,  ULMBCS_AMBIGUOUS_ALL},
    {0x2022, 0x2024,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2025, 0x2025,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2026, 0x2026,  ULMBCS_AMBIGUOUS_ALL},
    {0x2027, 0x2027,  ULMBCS_GRP_TW},
    {0x2030, 0x2030,  ULMBCS_AMBIGUOUS_ALL},
    {0x2031, 0x2031,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2032, 0x2033,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2035, 0x2035,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2039, 0x203A,  ULMBCS_AMBIGUOUS_SBCS},
    {0x203B, 0x203B,  ULMBCS_AMBIGUOUS_MBCS},
    {0x203C, 0x203C,  ULMBCS_GRP_EXCEPT},
    {0x2074, 0x2074,  ULMBCS_GRP_KO},
    {0x207F, 0x207F,  ULMBCS_GRP_EXCEPT},
    {0x2081, 0x2084,  ULMBCS_GRP_KO},
    {0x20A4, 0x20AC,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2103, 0x2109,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2111, 0x2120,  ULMBCS_AMBIGUOUS_SBCS},
    
    {0x2121, 0x2121,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2122, 0x2126,  ULMBCS_AMBIGUOUS_SBCS},
    {0x212B, 0x212B,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2135, 0x2135,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2153, 0x2154,  ULMBCS_GRP_KO},
    {0x215B, 0x215E,  ULMBCS_GRP_EXCEPT},
    {0x2160, 0x2179,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2190, 0x2193,  ULMBCS_AMBIGUOUS_ALL},
    {0x2194, 0x2195,  ULMBCS_GRP_EXCEPT},
    {0x2196, 0x2199,  ULMBCS_AMBIGUOUS_MBCS},
    {0x21A8, 0x21A8,  ULMBCS_GRP_EXCEPT},
    {0x21B8, 0x21B9,  ULMBCS_GRP_CN},
    {0x21D0, 0x21D1,  ULMBCS_GRP_EXCEPT},
    {0x21D2, 0x21D2,  ULMBCS_AMBIGUOUS_MBCS},
    {0x21D3, 0x21D3,  ULMBCS_GRP_EXCEPT},
    {0x21D4, 0x21D4,  ULMBCS_AMBIGUOUS_MBCS},
    {0x21D5, 0x21D5,  ULMBCS_GRP_EXCEPT},
    {0x21E7, 0x21E7,  ULMBCS_GRP_CN},
    {0x2200, 0x2200,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2201, 0x2201,  ULMBCS_GRP_EXCEPT},
    {0x2202, 0x2202,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2203, 0x2203,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2204, 0x2206,  ULMBCS_GRP_EXCEPT},
    {0x2207, 0x2208,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2209, 0x220A,  ULMBCS_GRP_EXCEPT},
    {0x220B, 0x220B,  ULMBCS_AMBIGUOUS_MBCS},
    {0x220F, 0x2215,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2219, 0x2219,  ULMBCS_GRP_EXCEPT},
    {0x221A, 0x221A,  ULMBCS_AMBIGUOUS_MBCS},
    {0x221B, 0x221C,  ULMBCS_GRP_EXCEPT},
    {0x221D, 0x221E,  ULMBCS_AMBIGUOUS_MBCS},
    {0x221F, 0x221F,  ULMBCS_GRP_EXCEPT},
    {0x2220, 0x2220,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2223, 0x222A,  ULMBCS_AMBIGUOUS_MBCS},
    {0x222B, 0x223D,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2245, 0x2248,  ULMBCS_GRP_EXCEPT},
    {0x224C, 0x224C,  ULMBCS_GRP_TW},
    {0x2252, 0x2252,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2260, 0x2261,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2262, 0x2265,  ULMBCS_GRP_EXCEPT},
    {0x2266, 0x226F,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2282, 0x2283,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2284, 0x2285,  ULMBCS_GRP_EXCEPT},
    {0x2286, 0x2287,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2288, 0x2297,  ULMBCS_GRP_EXCEPT},
    {0x2299, 0x22BF,  ULMBCS_AMBIGUOUS_MBCS},
    {0x22C0, 0x22C0,  ULMBCS_GRP_EXCEPT},
    {0x2310, 0x2310,  ULMBCS_GRP_EXCEPT},
    {0x2312, 0x2312,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2318, 0x2321,  ULMBCS_GRP_EXCEPT},
    {0x2318, 0x2321,  ULMBCS_GRP_CN},
    {0x2460, 0x24E9,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2500, 0x2500,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2501, 0x2501,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2502, 0x2502,  ULMBCS_AMBIGUOUS_ALL},
    {0x2503, 0x2503,  ULMBCS_AMBIGUOUS_MBCS},
    {0x2504, 0x2505,  ULMBCS_GRP_TW},
    {0x2506, 0x2665,  ULMBCS_AMBIGUOUS_ALL},
    {0x2666, 0x2666,  ULMBCS_GRP_EXCEPT},
    {0x2667, 0x2669,  ULMBCS_AMBIGUOUS_SBCS},
    {0x266A, 0x266A,  ULMBCS_AMBIGUOUS_ALL},
    {0x266B, 0x266C,  ULMBCS_AMBIGUOUS_SBCS},
    {0x266D, 0x266D,  ULMBCS_AMBIGUOUS_MBCS},
    {0x266E, 0x266E,  ULMBCS_AMBIGUOUS_SBCS},
    {0x266F, 0x266F,  ULMBCS_GRP_JA},
    {0x2670, 0x2E7F,  ULMBCS_AMBIGUOUS_SBCS},
    {0x2E80, 0xF861,  ULMBCS_AMBIGUOUS_MBCS},
    {0xF862, 0xF8FF,  ULMBCS_GRP_EXCEPT},
    {0xF900, 0xFA2D,  ULMBCS_AMBIGUOUS_MBCS},
    {0xFB00, 0xFEFF,  ULMBCS_AMBIGUOUS_SBCS},
    {0xFF01, 0xFFEE,  ULMBCS_AMBIGUOUS_MBCS},
    {0xFFFF, 0xFFFF,  ULMBCS_GRP_UNICODE}
};
   
static ulmbcs_byte_t 
FindLMBCSUniRange(UChar uniChar)
{
   const struct _UniLMBCSGrpMap * pTable = UniLMBCSGrpMap;

   while (uniChar > pTable->uniEndRange) 
   {
      pTable++;
   }

   if (uniChar >= pTable->uniStartRange) 
   {
      return pTable->GrpType;
   }
   return ULMBCS_GRP_UNICODE;
}























static const struct _LocaleLMBCSGrpMap
{
   const char    *LocaleID;
   const ulmbcs_byte_t OptGroup;
} LocaleLMBCSGrpMap[] =
{
    {"ar", ULMBCS_GRP_AR},
    {"be", ULMBCS_GRP_RU},
    {"bg", ULMBCS_GRP_L2},
   
    {"cs", ULMBCS_GRP_L2},
   
   
    {"el", ULMBCS_GRP_GR},
   
   
   
   
   
    {"he", ULMBCS_GRP_HE},
    {"hu", ULMBCS_GRP_L2},
   
   
    {"iw", ULMBCS_GRP_HE},
    {"ja", ULMBCS_GRP_JA},
    {"ko", ULMBCS_GRP_KO},
   
   
    {"mk", ULMBCS_GRP_RU},
   
   
    {"pl", ULMBCS_GRP_L2},
   
    {"ro", ULMBCS_GRP_L2},
    {"ru", ULMBCS_GRP_RU},
    {"sh", ULMBCS_GRP_L2},
    {"sk", ULMBCS_GRP_L2},
    {"sl", ULMBCS_GRP_L2},
    {"sq", ULMBCS_GRP_L2},
    {"sr", ULMBCS_GRP_RU},
   
    {"th", ULMBCS_GRP_TH},
    {"tr", ULMBCS_GRP_TR},
    {"uk", ULMBCS_GRP_RU},
   
    {"zhTW", ULMBCS_GRP_TW},
    {"zh", ULMBCS_GRP_CN},
    {NULL, ULMBCS_GRP_L1}
};


static ulmbcs_byte_t 
FindLMBCSLocale(const char *LocaleID)
{
   const struct _LocaleLMBCSGrpMap *pTable = LocaleLMBCSGrpMap;

   if ((!LocaleID) || (!*LocaleID)) 
   {
      return 0;
   }

   while (pTable->LocaleID)
   {
      if (*pTable->LocaleID == *LocaleID) 
      {
         
         if (uprv_strncmp(pTable->LocaleID, LocaleID, strlen(pTable->LocaleID)) == 0)
            return pTable->OptGroup;
      }
      else
      if (*pTable->LocaleID > *LocaleID) 
         break;
      pTable++;
   }
   return ULMBCS_GRP_L1;
}















typedef struct
  {
    UConverterSharedData *OptGrpConverter[ULMBCS_GRP_LAST+1];    
    uint8_t    OptGroup;                  
    uint8_t    localeConverterIndex;      
  }
UConverterDataLMBCS;

static void _LMBCSClose(UConverter * _this);

#define DECLARE_LMBCS_DATA(n) \
static const UConverterImpl _LMBCSImpl##n={\
    UCNV_LMBCS_##n,\
    NULL,NULL,\
    _LMBCSOpen##n,\
    _LMBCSClose,\
    NULL,\
    _LMBCSToUnicodeWithOffsets,\
    _LMBCSToUnicodeWithOffsets,\
    _LMBCSFromUnicode,\
    _LMBCSFromUnicode,\
    NULL,\
    NULL,\
    NULL,\
    NULL,\
    _LMBCSSafeClone,\
    ucnv_getCompleteUnicodeSet\
};\
static const UConverterStaticData _LMBCSStaticData##n={\
  sizeof(UConverterStaticData),\
 "LMBCS-"  #n,\
    0, UCNV_IBM, UCNV_LMBCS_##n, 1, 3,\
    { 0x3f, 0, 0, 0 },1,FALSE,FALSE,0,0,{0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0} \
};\
const UConverterSharedData _LMBCSData##n={\
    sizeof(UConverterSharedData), ~((uint32_t) 0),\
    NULL, NULL, &_LMBCSStaticData##n, FALSE, &_LMBCSImpl##n, \
    0 \
};

 



#define DEFINE_LMBCS_OPEN(n) \
static void \
   _LMBCSOpen##n(UConverter* _this, UConverterLoadArgs* pArgs, UErrorCode* err) \
{ _LMBCSOpenWorker(_this, pArgs, err, n); }




static void 
_LMBCSOpenWorker(UConverter*  _this,
                 UConverterLoadArgs *pArgs,
                 UErrorCode*  err,
                 ulmbcs_byte_t OptGroup)
{
    UConverterDataLMBCS * extraInfo = _this->extraInfo =
        (UConverterDataLMBCS*)uprv_malloc (sizeof (UConverterDataLMBCS));
    if(extraInfo != NULL)
    {
        UConverterNamePieces stackPieces;
        UConverterLoadArgs stackArgs={ (int32_t)sizeof(UConverterLoadArgs) };
        ulmbcs_byte_t i;

        uprv_memset(extraInfo, 0, sizeof(UConverterDataLMBCS));

        stackArgs.onlyTestIsLoadable = pArgs->onlyTestIsLoadable;

        for (i=0; i <= ULMBCS_GRP_LAST && U_SUCCESS(*err); i++)         
        {
            if(OptGroupByteToCPName[i] != NULL) {
                extraInfo->OptGrpConverter[i] = ucnv_loadSharedData(OptGroupByteToCPName[i], &stackPieces, &stackArgs, err);
            }
        }

        if(U_FAILURE(*err) || pArgs->onlyTestIsLoadable) {
            _LMBCSClose(_this);
            return;
        }
        extraInfo->OptGroup = OptGroup;
        extraInfo->localeConverterIndex = FindLMBCSLocale(pArgs->locale);
    }
    else
    {
        *err = U_MEMORY_ALLOCATION_ERROR;
    }
}

static void 
_LMBCSClose(UConverter *   _this) 
{
    if (_this->extraInfo != NULL)
    {
        ulmbcs_byte_t Ix;
        UConverterDataLMBCS * extraInfo = (UConverterDataLMBCS *) _this->extraInfo;

        for (Ix=0; Ix <= ULMBCS_GRP_LAST; Ix++)
        {
           if (extraInfo->OptGrpConverter[Ix] != NULL)
              ucnv_unloadSharedDataIfReady(extraInfo->OptGrpConverter[Ix]);
        }
        if (!_this->isExtraLocal) {
            uprv_free (_this->extraInfo);
            _this->extraInfo = NULL;
        }
    }
}

typedef struct LMBCSClone {
    UConverter cnv;
    UConverterDataLMBCS lmbcs;
} LMBCSClone;

static UConverter * 
_LMBCSSafeClone(const UConverter *cnv, 
                void *stackBuffer, 
                int32_t *pBufferSize, 
                UErrorCode *status) {
    LMBCSClone *newLMBCS;
    UConverterDataLMBCS *extraInfo;
    int32_t i;

    if(*pBufferSize<=0) {
        *pBufferSize=(int32_t)sizeof(LMBCSClone);
        return NULL;
    }

    extraInfo=(UConverterDataLMBCS *)cnv->extraInfo;
    newLMBCS=(LMBCSClone *)stackBuffer;

    

    uprv_memcpy(&newLMBCS->lmbcs, extraInfo, sizeof(UConverterDataLMBCS));

    
    for(i = 0; i <= ULMBCS_GRP_LAST; ++i) {
        if(extraInfo->OptGrpConverter[i] != NULL) {
            ucnv_incrementRefCount(extraInfo->OptGrpConverter[i]);
        }
    }

    newLMBCS->cnv.extraInfo = &newLMBCS->lmbcs;
    newLMBCS->cnv.isExtraLocal = TRUE;
    return &newLMBCS->cnv;
}



















static size_t
LMBCSConversionWorker (
   UConverterDataLMBCS * extraInfo,    
   ulmbcs_byte_t group,                
   ulmbcs_byte_t  * pStartLMBCS,              
   UChar * pUniChar,                   
   ulmbcs_byte_t * lastConverterIndex, 
   UBool * groups_tried                
)   
{
   ulmbcs_byte_t  * pLMBCS = pStartLMBCS;
   UConverterSharedData * xcnv = extraInfo->OptGrpConverter[group];

   int bytesConverted;
   uint32_t value;
   ulmbcs_byte_t firstByte;

   U_ASSERT(xcnv);
   U_ASSERT(group<ULMBCS_GRP_UNICODE);

   bytesConverted = ucnv_MBCSFromUChar32(xcnv, *pUniChar, &value, FALSE);

   
   if(bytesConverted > 0) {
      firstByte = (ulmbcs_byte_t)(value >> ((bytesConverted - 1) * 8));
   } else {
      
      groups_tried[group] = TRUE;
      return 0;
   }

   *lastConverterIndex = group;

   


   U_ASSERT((firstByte <= ULMBCS_C0END) || (firstByte >= ULMBCS_C1START) || (group == ULMBCS_GRP_EXCEPT));
   
   
   if (group != ULMBCS_GRP_EXCEPT && extraInfo->OptGroup != group)
   {
      *pLMBCS++ = group;
      if (bytesConverted == 1 && group >= ULMBCS_DOUBLEOPTGROUP_START)
      {
         *pLMBCS++ = group;
      }
   }

  
   if ( bytesConverted == 1 && firstByte < 0x20 )
      return 0;


   
   switch(bytesConverted)
   {
   case 4:
      *pLMBCS++ = (ulmbcs_byte_t)(value >> 24);
   case 3: 
      *pLMBCS++ = (ulmbcs_byte_t)(value >> 16);
   case 2: 
      *pLMBCS++ = (ulmbcs_byte_t)(value >> 8);
   case 1: 
      *pLMBCS++ = (ulmbcs_byte_t)value;
   default:
      
      break;
   }

   return (pLMBCS - pStartLMBCS);
}





static size_t 
LMBCSConvertUni(ulmbcs_byte_t * pLMBCS, UChar uniChar)  
{
     
   uint8_t LowCh =   (uint8_t)(uniChar & 0x00FF);
   uint8_t HighCh  = (uint8_t)(uniChar >> 8);

   *pLMBCS++ = ULMBCS_GRP_UNICODE;

   if (LowCh == 0)
   {
      *pLMBCS++ = ULMBCS_UNICOMPATZERO;
      *pLMBCS++ = HighCh;
   }
   else
   {
      *pLMBCS++ = HighCh;
      *pLMBCS++ = LowCh;
   }
   return ULMBCS_UNICODE_SIZE;
}




static void 
_LMBCSFromUnicode(UConverterFromUnicodeArgs*     args,
                  UErrorCode*     err)
{
   ulmbcs_byte_t lastConverterIndex = 0;
   UChar uniChar;
   ulmbcs_byte_t  LMBCS[ULMBCS_CHARSIZE_MAX];
   ulmbcs_byte_t  * pLMBCS;
   int32_t bytes_written;
   UBool groups_tried[ULMBCS_GRP_LAST+1];
   UConverterDataLMBCS * extraInfo = (UConverterDataLMBCS *) args->converter->extraInfo;
   int sourceIndex = 0; 

   






















    
    ulmbcs_byte_t OldConverterIndex = 0;

   while (args->source < args->sourceLimit && !U_FAILURE(*err))
   {
      
      OldConverterIndex = extraInfo->localeConverterIndex;

      if (args->target >= args->targetLimit)
      {
         *err = U_BUFFER_OVERFLOW_ERROR;
         break;
      }
      uniChar = *(args->source);
      bytes_written = 0;
      pLMBCS = LMBCS;

      

      
      
      if((uniChar>=0x80) && (uniChar<=0xff)
       &&(uniChar!=0xB1) &&(uniChar!=0xD7) &&(uniChar!=0xF7)
        &&(uniChar!=0xB0) &&(uniChar!=0xB4) &&(uniChar!=0xB6) &&(uniChar!=0xA7) &&(uniChar!=0xA8))
      {
            extraInfo->localeConverterIndex = ULMBCS_GRP_L1;
      }
      if (((uniChar > ULMBCS_C0END) && (uniChar < ULMBCS_C1START)) ||
          uniChar == 0 || uniChar == ULMBCS_HT || uniChar == ULMBCS_CR || 
          uniChar == ULMBCS_LF || uniChar == ULMBCS_123SYSTEMRANGE 
          )
      {
         *pLMBCS++ = (ulmbcs_byte_t ) uniChar;
         bytes_written = 1;
      }


      if (!bytes_written) 
      {
         
         ulmbcs_byte_t group = FindLMBCSUniRange(uniChar);
         
         if (group == ULMBCS_GRP_UNICODE)  
         {
            pLMBCS += LMBCSConvertUni(pLMBCS,uniChar);
            
            bytes_written = (int32_t)(pLMBCS - LMBCS);
         }
         else if (group == ULMBCS_GRP_CTRL)  
         {
            
            if (uniChar <= ULMBCS_C0END)
            {
               *pLMBCS++ = ULMBCS_GRP_CTRL;
               *pLMBCS++ = (ulmbcs_byte_t)(ULMBCS_CTRLOFFSET + uniChar);
            }
            else if (uniChar >= ULMBCS_C1START && uniChar <= ULMBCS_C1START + ULMBCS_CTRLOFFSET)
            {
               *pLMBCS++ = ULMBCS_GRP_CTRL;
               *pLMBCS++ = (ulmbcs_byte_t ) (uniChar & 0x00FF);
            }
            bytes_written = (int32_t)(pLMBCS - LMBCS);
         }
         else if (group < ULMBCS_GRP_UNICODE)  
         {
            
            bytes_written = (int32_t)LMBCSConversionWorker (
                              extraInfo, group, pLMBCS, &uniChar, 
                              &lastConverterIndex, groups_tried);
         }
         if (!bytes_written)    
         {
            uprv_memset(groups_tried, 0, sizeof(groups_tried));

            
            if ((extraInfo->OptGroup != 1) && (ULMBCS_AMBIGUOUS_MATCH(group, extraInfo->OptGroup)))
            {
                
                

                if(extraInfo->localeConverterIndex < ULMBCS_DOUBLEOPTGROUP_START)
                {
                  bytes_written = LMBCSConversionWorker (extraInfo,
                     ULMBCS_GRP_L1, pLMBCS, &uniChar,
                     &lastConverterIndex, groups_tried);

                  if(!bytes_written)
                  {
                     bytes_written = LMBCSConversionWorker (extraInfo,
                         ULMBCS_GRP_EXCEPT, pLMBCS, &uniChar,
                         &lastConverterIndex, groups_tried);
                  }
                  if(!bytes_written)
                  {
                      bytes_written = LMBCSConversionWorker (extraInfo,
                          extraInfo->localeConverterIndex, pLMBCS, &uniChar,
                          &lastConverterIndex, groups_tried);
                  }
                }
                else
                {
                     bytes_written = LMBCSConversionWorker (extraInfo,
                         extraInfo->localeConverterIndex, pLMBCS, &uniChar,
                         &lastConverterIndex, groups_tried);
                }
            }
            
            if (!bytes_written && (extraInfo->localeConverterIndex) && (ULMBCS_AMBIGUOUS_MATCH(group, extraInfo->localeConverterIndex)))
            {
                bytes_written = (int32_t)LMBCSConversionWorker (extraInfo,
                        extraInfo->localeConverterIndex, pLMBCS, &uniChar, &lastConverterIndex, groups_tried);
            }
            
            if (!bytes_written && (lastConverterIndex) && (ULMBCS_AMBIGUOUS_MATCH(group, lastConverterIndex)))
            {
                bytes_written = (int32_t)LMBCSConversionWorker (extraInfo,
                        lastConverterIndex, pLMBCS, &uniChar, &lastConverterIndex, groups_tried);
            }
            if (!bytes_written)
            {
                
               ulmbcs_byte_t grp_start;
               ulmbcs_byte_t grp_end;  
               ulmbcs_byte_t grp_ix;
               grp_start = (ulmbcs_byte_t)((group == ULMBCS_AMBIGUOUS_MBCS) 
                        ? ULMBCS_DOUBLEOPTGROUP_START 
                        :  ULMBCS_GRP_L1);
               grp_end = (ulmbcs_byte_t)((group == ULMBCS_AMBIGUOUS_MBCS) 
                        ? ULMBCS_GRP_LAST 
                        :  ULMBCS_GRP_TH);
               if(group == ULMBCS_AMBIGUOUS_ALL)
               {
                   grp_start = ULMBCS_GRP_L1;
                   grp_end = ULMBCS_GRP_LAST;
               }
               for (grp_ix = grp_start;
                   grp_ix <= grp_end && !bytes_written; 
                    grp_ix++)
               {
                  if (extraInfo->OptGrpConverter [grp_ix] && !groups_tried [grp_ix])
                  {
                     bytes_written = (int32_t)LMBCSConversionWorker (extraInfo,
                       grp_ix, pLMBCS, &uniChar, 
                       &lastConverterIndex, groups_tried);
                  }
               }
                

               if (!bytes_written && grp_start == ULMBCS_GRP_L1)
               {
                  bytes_written = (int32_t)LMBCSConversionWorker (extraInfo,
                     ULMBCS_GRP_EXCEPT, pLMBCS, &uniChar, 
                     &lastConverterIndex, groups_tried);
               }
            }
            
            if (!bytes_written)
            {

               pLMBCS += LMBCSConvertUni(pLMBCS, uniChar);
               bytes_written = (int32_t)(pLMBCS - LMBCS);
            }
         }
      }
  
      
      args->source++;
      pLMBCS = LMBCS;
      while (args->target < args->targetLimit && bytes_written--)
      {
         *(args->target)++ = *pLMBCS++;
         if (args->offsets)
         {
            *(args->offsets)++ = sourceIndex;
         }
      }
      sourceIndex++;
      if (bytes_written > 0)
      {
         



         uint8_t * pErrorBuffer = args->converter->charErrorBuffer;
         *err = U_BUFFER_OVERFLOW_ERROR;
         args->converter->charErrorBufferLength = (int8_t)bytes_written;
         while (bytes_written--)
         {
            *pErrorBuffer++ = *pLMBCS++;
         }
      }
      
      extraInfo->localeConverterIndex = OldConverterIndex;
   }     
}






static UChar
GetUniFromLMBCSUni(char const ** ppLMBCSin)  
{
   uint8_t  HighCh = *(*ppLMBCSin)++;  
   uint8_t  LowCh  = *(*ppLMBCSin)++;

   if (HighCh == ULMBCS_UNICOMPATZERO ) 
   {
      HighCh = LowCh;
      LowCh = 0; 
   }
   return (UChar)((HighCh << 8) | LowCh);
}









#define CHECK_SOURCE_LIMIT(index) \
     if (args->source+index > args->sourceLimit){\
         *err = U_TRUNCATED_CHAR_FOUND;\
         args->source = args->sourceLimit;\
         return 0xffff;}



static UChar32 
_LMBCSGetNextUCharWorker(UConverterToUnicodeArgs*   args,
                         UErrorCode*   err)
{
    UChar32 uniChar = 0;    
    ulmbcs_byte_t   CurByte; 

    
    if (args->source >= args->sourceLimit)
    {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return 0xffff;
    }
    
    CurByte = *((ulmbcs_byte_t  *) (args->source++));
   
    








   
    

    if(((CurByte > ULMBCS_C0END) && (CurByte < ULMBCS_C1START)) 
    ||  (CurByte == 0) 
    ||  CurByte == ULMBCS_HT || CurByte == ULMBCS_CR 
    ||  CurByte == ULMBCS_LF || CurByte == ULMBCS_123SYSTEMRANGE)
    {
        uniChar = CurByte;
    }
    else  
    {
        UConverterDataLMBCS * extraInfo;
        ulmbcs_byte_t group; 
        UConverterSharedData *cnv; 
        
        if (CurByte == ULMBCS_GRP_CTRL)  
        {
            ulmbcs_byte_t  C0C1byte;
            CHECK_SOURCE_LIMIT(1);
            C0C1byte = *(args->source)++;
            uniChar = (C0C1byte < ULMBCS_C1START) ? C0C1byte - ULMBCS_CTRLOFFSET : C0C1byte;
        }
        else 
        if (CurByte == ULMBCS_GRP_UNICODE) 
        {
            CHECK_SOURCE_LIMIT(2);
     
            
            return GetUniFromLMBCSUni(&(args->source));
        }
        else if (CurByte <= ULMBCS_CTRLOFFSET)  
        {
            group = CurByte;                   
            extraInfo = (UConverterDataLMBCS *) args->converter->extraInfo;
            if (group > ULMBCS_GRP_LAST || (cnv = extraInfo->OptGrpConverter[group]) == NULL)
            {
                
                *err = U_INVALID_CHAR_FOUND;
            }      
            else if (group >= ULMBCS_DOUBLEOPTGROUP_START)    
            {

                CHECK_SOURCE_LIMIT(2);

                
                if (*args->source == group) {
                    
                    ++args->source;
                    uniChar = ucnv_MBCSSimpleGetNextUChar(cnv, args->source, 1, FALSE);
                    ++args->source;
                } else {
                    
                    uniChar = ucnv_MBCSSimpleGetNextUChar(cnv, args->source, 2, FALSE);
                    args->source += 2;
                }
            }
            else {                                  
                CHECK_SOURCE_LIMIT(1);
                CurByte = *(args->source)++;
        
                if (CurByte >= ULMBCS_C1START)
                {
                    uniChar = _MBCS_SINGLE_SIMPLE_GET_NEXT_BMP(cnv, CurByte);
                }
                else
                {
                    


                    char bytes[2];

                    extraInfo = (UConverterDataLMBCS *) args->converter->extraInfo;
                    cnv = extraInfo->OptGrpConverter [ULMBCS_GRP_EXCEPT];  
        
                    
                    bytes[0] = group;
                    bytes[1] = CurByte;
                    uniChar = ucnv_MBCSSimpleGetNextUChar(cnv, bytes, 2, FALSE);
                }
            }
        }
        else if (CurByte >= ULMBCS_C1START) 
        {
            extraInfo = (UConverterDataLMBCS *) args->converter->extraInfo;
            group = extraInfo->OptGroup;
            cnv = extraInfo->OptGrpConverter[group];
            if (group >= ULMBCS_DOUBLEOPTGROUP_START)    
            {
                if (!ucnv_MBCSIsLeadByte(cnv, CurByte))
                {
                    CHECK_SOURCE_LIMIT(0);

                    
                    uniChar = ucnv_MBCSSimpleGetNextUChar(cnv, args->source - 1, 1, FALSE);
                }
                else
                {
                    CHECK_SOURCE_LIMIT(1);
                    
                    uniChar = ucnv_MBCSSimpleGetNextUChar(cnv, args->source - 1, 2, FALSE);
                    ++args->source;
                }
            }
            else                                   
            {
                uniChar = _MBCS_SINGLE_SIMPLE_GET_NEXT_BMP(cnv, CurByte);
            }
        }
    }
    return uniChar;
}





static void 
_LMBCSToUnicodeWithOffsets(UConverterToUnicodeArgs*    args,
                     UErrorCode*    err)
{
   char LMBCS [ULMBCS_CHARSIZE_MAX];
   UChar uniChar;    
   const char * saveSource; 
   const char * pStartLMBCS = args->source;  
   const char * errSource = NULL; 
   int8_t savebytes = 0;

   
   while (U_SUCCESS(*err) && args->sourceLimit > args->source && args->targetLimit > args->target)
   {
      saveSource = args->source; 

      if (args->converter->toULength) 
      {
        const char *saveSourceLimit; 
        size_t size_old = args->converter->toULength;

         
        size_t size_new_maybe_1 = sizeof(LMBCS) - size_old;
        size_t size_new_maybe_2 = args->sourceLimit - args->source;
        size_t size_new = (size_new_maybe_1 < size_new_maybe_2) ? size_new_maybe_1 : size_new_maybe_2;
         
      
        uprv_memcpy(LMBCS, args->converter->toUBytes, size_old);
        uprv_memcpy(LMBCS + size_old, args->source, size_new);
        saveSourceLimit = args->sourceLimit;
        args->source = errSource = LMBCS;
        args->sourceLimit = LMBCS+size_old+size_new;
        savebytes = (int8_t)(size_old+size_new);
        uniChar = (UChar) _LMBCSGetNextUCharWorker(args, err);
        args->source = saveSource + ((args->source - LMBCS) - size_old);
        args->sourceLimit = saveSourceLimit;

        if (*err == U_TRUNCATED_CHAR_FOUND)
        {
            
            args->converter->toULength = savebytes;
            uprv_memcpy(args->converter->toUBytes, LMBCS, savebytes);
            args->source = args->sourceLimit;
            *err = U_ZERO_ERROR;
            return;
         }
         else
         {
            
            args->converter->toULength = 0;
         }
      }
      else
      {
         errSource = saveSource;
         uniChar = (UChar) _LMBCSGetNextUCharWorker(args, err);
         savebytes = (int8_t)(args->source - saveSource);
      }
      if (U_SUCCESS(*err))
      {
         if (uniChar < 0xfffe)
         {
            *(args->target)++ = uniChar;
            if(args->offsets)
            {
               *(args->offsets)++ = (int32_t)(saveSource - pStartLMBCS);
            }
         }
         else if (uniChar == 0xfffe)
         {
            *err = U_INVALID_CHAR_FOUND;
         }
         else 
         {
            *err = U_ILLEGAL_CHAR_FOUND;
         }
      }
   }
   
   if (U_SUCCESS(*err) && args->sourceLimit > args->source && args->targetLimit <= args->target)
   {
      *err = U_BUFFER_OVERFLOW_ERROR;
   }
   else if (U_FAILURE(*err)) 
   {
      
      args->converter->toULength = savebytes;
      if (savebytes > 0) {
         uprv_memcpy(args->converter->toUBytes, errSource, savebytes);
      }
      if (*err == U_TRUNCATED_CHAR_FOUND) {
         *err = U_ZERO_ERROR;
      }
   }
}


DEFINE_LMBCS_OPEN(1)
DEFINE_LMBCS_OPEN(2)
DEFINE_LMBCS_OPEN(3)
DEFINE_LMBCS_OPEN(4)
DEFINE_LMBCS_OPEN(5)
DEFINE_LMBCS_OPEN(6)
DEFINE_LMBCS_OPEN(8)
DEFINE_LMBCS_OPEN(11)
DEFINE_LMBCS_OPEN(16)
DEFINE_LMBCS_OPEN(17)
DEFINE_LMBCS_OPEN(18)
DEFINE_LMBCS_OPEN(19)


DECLARE_LMBCS_DATA(1)
DECLARE_LMBCS_DATA(2)
DECLARE_LMBCS_DATA(3)
DECLARE_LMBCS_DATA(4)
DECLARE_LMBCS_DATA(5)
DECLARE_LMBCS_DATA(6)
DECLARE_LMBCS_DATA(8)
DECLARE_LMBCS_DATA(11)
DECLARE_LMBCS_DATA(16)
DECLARE_LMBCS_DATA(17)
DECLARE_LMBCS_DATA(18)
DECLARE_LMBCS_DATA(19)

#endif 
