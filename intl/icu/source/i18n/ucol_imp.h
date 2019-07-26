
























#ifndef UCOL_IMP_H
#define UCOL_IMP_H

#include "unicode/utypes.h"
#ifdef __cplusplus
#   include "unicode/utf16.h"
#endif

#define UCA_DATA_TYPE "icu"
#define UCA_DATA_NAME "ucadata"
#define INVC_DATA_TYPE "icu"
#define INVC_DATA_NAME "invuca"





#define U_ICUDATA_COLL U_ICUDATA_NAME U_TREE_SEPARATOR_STRING "coll"

#if !UCONFIG_NO_COLLATION

#ifdef __cplusplus
#include "unicode/normalizer2.h"
#include "unicode/unistr.h"
#endif
#include "unicode/ucol.h"
#include "ucol_data.h"
#include "utrie.h"
#include "cmemory.h"































































































































#define UCA_DATA_FORMAT_0 ((uint8_t)0x55)
#define UCA_DATA_FORMAT_1 ((uint8_t)0x43)
#define UCA_DATA_FORMAT_2 ((uint8_t)0x6f)
#define UCA_DATA_FORMAT_3 ((uint8_t)0x6c)

#define UCA_FORMAT_VERSION_0 ((uint8_t)3)
#define UCA_FORMAT_VERSION_1 0
#define UCA_FORMAT_VERSION_2 ((uint8_t)0)
#define UCA_FORMAT_VERSION_3 ((uint8_t)0)



#define INVUCA_DATA_FORMAT_0 ((uint8_t)0x49)
#define INVUCA_DATA_FORMAT_1 ((uint8_t)0x6E)
#define INVUCA_DATA_FORMAT_2 ((uint8_t)0x76)
#define INVUCA_DATA_FORMAT_3 ((uint8_t)0x43)

#define INVUCA_FORMAT_VERSION_0 ((uint8_t)2)
#define INVUCA_FORMAT_VERSION_1 ((uint8_t)1)
#define INVUCA_FORMAT_VERSION_2 ((uint8_t)0)
#define INVUCA_FORMAT_VERSION_3 ((uint8_t)0)




#define UCOL_MAX_BUFFER 128

#define UCOL_NORMALIZATION_GROWTH 2
#define UCOL_NORMALIZATION_MAX_BUFFER UCOL_MAX_BUFFER*UCOL_NORMALIZATION_GROWTH









#define UCOL_WRITABLE_BUFFER_SIZE 256







#define UCOL_EXPAND_CE_BUFFER_SIZE 64


#define UCOL_EXPAND_CE_BUFFER_EXTEND_SIZE 64





#define UCOL_UNSAFECP_TABLE_SIZE 1056
                                    
                                    
#define UCOL_UNSAFECP_TABLE_MASK 0x1fff





#define UCOL_ITER_NORM  1

#define UCOL_ITER_HASLEN 2

                              
                              
                              
#define UCOL_ITER_INNORMBUF 4

                              
                              
#define UCOL_ITER_ALLOCATED 8
                              
#define UCOL_HIRAGANA_Q     16
                              
                              
#define UCOL_WAS_HIRAGANA   32 
                              
                              
                              
#define UCOL_USE_ITERATOR   64

#define UCOL_FORCE_HAN_IMPLICIT 128

#define NFC_ZERO_CC_BLOCK_LIMIT_  0x300

#ifdef __cplusplus

U_NAMESPACE_BEGIN

typedef struct collIterate : public UMemory {
  const UChar *string; 
  

  const UChar *endp; 
  const UChar *pos; 

  uint32_t *toReturn; 
  uint32_t *CEpos; 

  int32_t *offsetReturn; 
  int32_t *offsetStore;  
  int32_t offsetRepeatCount;  
  int32_t offsetRepeatValue;  

  UnicodeString writableBuffer;
  const UChar *fcdPosition; 
  const UCollator *coll;
  const Normalizer2 *nfd;
  uint8_t   flags;
  uint8_t   origFlags;
  uint32_t *extendCEs; 
  int32_t extendCEsSize; 
  uint32_t CEs[UCOL_EXPAND_CE_BUFFER_SIZE]; 

  int32_t *offsetBuffer;    
  int32_t offsetBufferSize; 

  UCharIterator *iterator;
  

  
  
  void appendOffset(int32_t offset, UErrorCode &errorCode);
} collIterate;

U_NAMESPACE_END

#else

typedef struct collIterate collIterate;

#endif

#define paddedsize(something) ((something)+((((something)%4)!=0)?(4-(something)%4):0))
#define headersize (paddedsize(sizeof(UCATableHeader))+paddedsize(sizeof(UColOptionSet)))





struct collIterateState {
    const UChar *pos; 
    const UChar *returnPos;
    const UChar *fcdPosition; 
    const UChar *bufferaddress; 
    int32_t  buffersize;
    uint8_t   flags;
    uint8_t   origFlags;
    uint32_t   iteratorIndex;
    int32_t    iteratorMove;
};

U_CAPI void U_EXPORT2
uprv_init_collIterate(const UCollator *collator,
                      const UChar *sourceString, int32_t sourceLen,
                      U_NAMESPACE_QUALIFIER collIterate *s, UErrorCode *status);


U_CAPI U_NAMESPACE_QUALIFIER collIterate * U_EXPORT2
uprv_new_collIterate(UErrorCode *status);

U_CAPI void U_EXPORT2
uprv_delete_collIterate(U_NAMESPACE_QUALIFIER collIterate *s);


U_CAPI UBool U_EXPORT2
uprv_collIterateAtEnd(U_NAMESPACE_QUALIFIER collIterate *s);

#ifdef __cplusplus

U_NAMESPACE_BEGIN

struct UCollationPCE;
typedef struct UCollationPCE UCollationPCE;

U_NAMESPACE_END

struct UCollationElements : public icu::UMemory
{
  


        icu::collIterate iteratordata_;
  


        UBool              reset_;
  


        UBool              isWritable;




        icu::UCollationPCE     *pce;
};

#else

struct UCollationElements;
#endif

U_CAPI void U_EXPORT2
uprv_init_pce(const struct UCollationElements *elems);

#define UCOL_LEVELTERMINATOR 1


#define UCOL_PRIMARYORDERMASK 0xffff0000

#define UCOL_SECONDARYORDERMASK 0x0000ff00

#define UCOL_TERTIARYORDERMASK 0x000000ff

#define UCOL_PRIMARYORDERSHIFT 16

#define UCOL_SECONDARYORDERSHIFT 8

#define UCOL_BYTE_SIZE_MASK 0xFF

#define UCOL_CASE_BYTE_START 0x80
#define UCOL_CASE_SHIFT_START 7

#define UCOL_IGNORABLE 0


#define UCOL_PRIMARYORDER(order) (((order) & UCOL_PRIMARYORDERMASK)>> UCOL_PRIMARYORDERSHIFT)
#define UCOL_SECONDARYORDER(order) (((order) & UCOL_SECONDARYORDERMASK)>> UCOL_SECONDARYORDERSHIFT)
#define UCOL_TERTIARYORDER(order) ((order) & UCOL_TERTIARYORDERMASK)





#define UCOL_ISTHAIPREVOWEL(ch) ((((uint32_t)(ch) - 0xe40) <= (0xe44 - 0xe40)) || \
                                 (((uint32_t)(ch) - 0xec0) <= (0xec4 - 0xec0)))




#define UCOL_ISTHAIBASECONSONANT(ch) ((uint32_t)(ch) - 0xe01) <= (0xe2e - 0xe01)

#define UCOL_ISJAMO(ch) ((((uint32_t)(ch) - 0x1100) <= (0x1112 - 0x1100)) || \
                         (((uint32_t)(ch) - 0x1161) <= (0x1175 - 0x1161)) || \
                         (((uint32_t)(ch) - 0x11A8) <= (0x11C2 - 0x11A8)))


#define UCOL_FIRST_HAN 0x4E00
#define UCOL_LAST_HAN  0x9FFF
#define UCOL_FIRST_HAN_A 0x3400
#define UCOL_LAST_HAN_A  0x4DBF
#define UCOL_FIRST_HAN_COMPAT 0xFAE0
#define UCOL_LAST_HAN_COMPAT  0xFA2F


#define UCOL_FIRST_HAN_B       0x20000
#define UCOL_LAST_HAN_B        0x2A6DF


#define UCOL_FIRST_HANGUL 0xAC00
#define UCOL_LAST_HANGUL  0xD7AF


#define UCOL_FIRST_L_JAMO 0x1100
#define UCOL_FIRST_V_JAMO 0x1161
#define UCOL_FIRST_T_JAMO 0x11A8
#define UCOL_LAST_T_JAMO  0x11F9


#if 0


#define init_collIterate(collator, sourceString, sourceLen, s) { \
    (s)->start = (s)->string = (s)->pos = (UChar *)(sourceString); \
    (s)->endp  = (sourceLen) == -1 ? NULL :(UChar *)(sourceString)+(sourceLen); \
    (s)->CEpos = (s)->toReturn = (s)->CEs; \
    (s)->isThai = TRUE; \
    (s)->writableBuffer = (s)->stackWritableBuffer; \
    (s)->writableBufSize = UCOL_WRITABLE_BUFFER_SIZE; \
    (s)->coll = (collator); \
    (s)->fcdPosition = 0;   \
    (s)->flags = 0; \
    if(((collator)->normalizationMode == UCOL_ON)) (s)->flags |= UCOL_ITER_NORM; \
}
#endif













#define UCOL_GETMAXEXPANSION(coll, order, result) {                          \
  const uint32_t *start;                                                     \
  const uint32_t *limit;                                                     \
  const uint32_t *mid;                                                       \
  start = (coll)->endExpansionCE;                                            \
  limit = (coll)->lastEndExpansionCE;                                        \
  while (start < limit - 1) {                                                \
    mid = start + ((limit - start) >> 1);                                    \
    if ((order) <= *mid) {                                                   \
      limit = mid;                                                           \
    }                                                                        \
    else {                                                                   \
      start = mid;                                                           \
    }                                                                        \
  }                                                                          \
  if (*start == order) {                                                     \
    result = *((coll)->expansionCESize + (start - (coll)->endExpansionCE));  \
  }                                                                          \
  else if (*limit == order) {                                                \
         result = *(coll->expansionCESize + (limit - coll->endExpansionCE)); \
       }                                                                     \
       else if ((order & 0xFFFF) == 0x00C0) {                                \
              result = 2;                                                    \
            }                                                                \
            else {                                                           \
              result = 1;                                                    \
            }                                                                \
}

U_CFUNC
uint32_t ucol_prv_getSpecialCE(const UCollator *coll, UChar ch, uint32_t CE,
                               U_NAMESPACE_QUALIFIER collIterate *source, UErrorCode *status);

U_CFUNC
uint32_t ucol_prv_getSpecialPrevCE(const UCollator *coll, UChar ch, uint32_t CE,
                                   U_NAMESPACE_QUALIFIER collIterate *source, UErrorCode *status);
U_CAPI uint32_t U_EXPORT2 ucol_getNextCE(const UCollator *coll,
                                         U_NAMESPACE_QUALIFIER collIterate *collationSource, UErrorCode *status);
U_CFUNC uint32_t U_EXPORT2 ucol_getPrevCE(const UCollator *coll,
                                          U_NAMESPACE_QUALIFIER collIterate *collationSource,
                                          UErrorCode *status);

void *ucol_getABuffer(const UCollator *coll, uint32_t size);

#ifdef __cplusplus

U_NAMESPACE_BEGIN

class CollationKey;
class SortKeyByteSink;

U_NAMESPACE_END


U_CFUNC int32_t
ucol_getCollationKey(const UCollator *coll,
                     const UChar *source, int32_t sourceLength,
                     icu::CollationKey &key,
                     UErrorCode &errorCode);

typedef void U_CALLCONV
SortKeyGenerator(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        icu::SortKeyByteSink &result,
        UErrorCode *status);


U_CFUNC
void U_CALLCONV
ucol_calcSortKey(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        icu::SortKeyByteSink &result,
        UErrorCode *status);

U_CFUNC
void U_CALLCONV
ucol_calcSortKeySimpleTertiary(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        icu::SortKeyByteSink &result,
        UErrorCode *status);

#else

typedef void U_CALLCONV
SortKeyGenerator(const    UCollator    *coll,
        const    UChar        *source,
        int32_t        sourceLength,
        void *result,
        UErrorCode *status);

#endif










U_CFUNC uint8_t* U_EXPORT2 
ucol_cloneRuleData(const UCollator *coll, int32_t *length, UErrorCode *status);





U_CFUNC void U_EXPORT2
ucol_setReqValidLocales(UCollator *coll, char *requestedLocaleToAdopt, char *validLocaleToAdopt, char *actualLocaleToAdopt);

#define UCOL_SPECIAL_FLAG 0xF0000000
#define UCOL_TAG_SHIFT 24
#define UCOL_TAG_MASK 0x0F000000
#define INIT_EXP_TABLE_SIZE 1024
#define UCOL_NOT_FOUND 0xF0000000
#define UCOL_EXPANSION 0xF1000000
#define UCOL_CONTRACTION 0xF2000000
#define UCOL_THAI 0xF3000000
#define UCOL_UNMARKED 0x03
#define UCOL_NEW_TERTIARYORDERMASK 0x0000003f


#define UCOL_PRIMARYMASK    0xFFFF0000


#define UCOL_SECONDARYMASK  0x0000FF00


#define UCOL_TERTIARYMASK   0x000000FF






#define UCOL_NO_MORE_CES            0x00010101
#define UCOL_NO_MORE_CES_PRIMARY    0x00010000
#define UCOL_NO_MORE_CES_SECONDARY  0x00000100
#define UCOL_NO_MORE_CES_TERTIARY   0x00000001

#define isSpecial(CE) ((((CE)&UCOL_SPECIAL_FLAG)>>28)==0xF)

#define UCOL_UPPER_CASE 0x80
#define UCOL_MIXED_CASE 0x40
#define UCOL_LOWER_CASE 0x00

#define UCOL_CONTINUATION_MARKER 0xC0
#define UCOL_REMOVE_CONTINUATION 0xFFFFFF3F

#define isContinuation(CE) (((CE) & UCOL_CONTINUATION_MARKER) == UCOL_CONTINUATION_MARKER)
#define isFlagged(CE) (((CE) & 0x80) == 0x80)
#define isLongPrimary(CE) (((CE) & 0xC0) == 0xC0)

#define getCETag(CE) (((CE)&UCOL_TAG_MASK)>>UCOL_TAG_SHIFT)
#define isContraction(CE) (isSpecial((CE)) && (getCETag((CE)) == CONTRACTION_TAG))
#define isPrefix(CE) (isSpecial((CE)) && (getCETag((CE)) == SPEC_PROC_TAG))
#define constructContractCE(tag, CE) (UCOL_SPECIAL_FLAG | ((tag)<<UCOL_TAG_SHIFT) | ((CE)&0xFFFFFF))
#define constructSpecProcCE(CE) (UCOL_SPECIAL_FLAG | (SPEC_PROC_TAG<<UCOL_TAG_SHIFT) | ((CE)&0xFFFFFF))
#define getContractOffset(CE) ((CE)&0xFFFFFF)
#define getExpansionOffset(CE) (((CE)&0x00FFFFF0)>>4)
#define getExpansionCount(CE) ((CE)&0xF)
#define isCEIgnorable(CE) (((CE) & 0xFFFFFFBF) == 0)


#define inNormBuf(coleiter) ((coleiter)->iteratordata_.flags & UCOL_ITER_INNORMBUF)
#define isFCDPointerNull(coleiter) ((coleiter)->iteratordata_.fcdPosition == NULL)
#define hasExpansion(coleiter) ((coleiter)->iteratordata_.CEpos != (coleiter)->iteratordata_.CEs)
#define getExpansionPrefix(coleiter) ((coleiter)->iteratordata_.toReturn - (coleiter)->iteratordata_.CEs)
#define setExpansionPrefix(coleiter, offset) ((coleiter)->iteratordata_.CEs + offset)
#define getExpansionSuffix(coleiter) ((coleiter)->iteratordata_.CEpos - (coleiter)->iteratordata_.toReturn)
#define setExpansionSuffix(coleiter, offset) ((coleiter)->iteratordata_.toReturn = (coleiter)->iteratordata_.CEpos - leftoverces)





enum {
    UCOL_BYTE_ZERO = 0x00,
    UCOL_BYTE_LEVEL_SEPARATOR = 0x01,
    UCOL_BYTE_SORTKEY_GLUE = 0x02,
    UCOL_BYTE_SHIFT_PREFIX = 0x03,
    UCOL_BYTE_UNSHIFTED_MIN = UCOL_BYTE_SHIFT_PREFIX,
    UCOL_BYTE_FIRST_TAILORED = 0x04,
    UCOL_BYTE_COMMON = 0x05,
    UCOL_BYTE_FIRST_UCA = UCOL_BYTE_COMMON,
    
    UCOL_CODAN_PLACEHOLDER = 0x12,
    UCOL_BYTE_FIRST_NON_LATIN_PRIMARY = 0x5B,
    UCOL_BYTE_UNSHIFTED_MAX = 0xFF
}; 

#if 0
#define UCOL_RESET_TOP_VALUE                0x9F000303
#define UCOL_FIRST_PRIMARY_IGNORABLE        0x00008705
#define UCOL_LAST_PRIMARY_IGNORABLE         0x0000DD05
#define UCOL_LAST_PRIMARY_IGNORABLE_CONT    0x000051C0
#define UCOL_FIRST_SECONDARY_IGNORABLE      0x00000000
#define UCOL_LAST_SECONDARY_IGNORABLE       0x00000500
#define UCOL_FIRST_TERTIARY_IGNORABLE       0x00000000
#define UCOL_LAST_TERTIARY_IGNORABLE        0x00000000
#define UCOL_FIRST_VARIABLE                 0x05070505
#define UCOL_LAST_VARIABLE                  0x179B0505
#define UCOL_FIRST_NON_VARIABLE             0x1A200505
#define UCOL_LAST_NON_VARIABLE              0x7B41058F

#define UCOL_NEXT_TOP_VALUE                 0xE8960303
#define UCOL_NEXT_FIRST_PRIMARY_IGNORABLE   0x00008905
#define UCOL_NEXT_LAST_PRIMARY_IGNORABLE    0x03000303
#define UCOL_NEXT_FIRST_SECONDARY_IGNORABLE 0x00008705
#define UCOL_NEXT_LAST_SECONDARY_IGNORABLE  0x00000500
#define UCOL_NEXT_FIRST_TERTIARY_IGNORABLE  0x00000000
#define UCOL_NEXT_LAST_TERTIARY_IGNORABLE   0x00000000
#define UCOL_NEXT_FIRST_VARIABLE            0x05090505
#define UCOL_NEXT_LAST_VARIABLE             0x1A200505

#define PRIMARY_IMPLICIT_MIN 0xE8000000
#define PRIMARY_IMPLICIT_MAX 0xF0000000
#endif


#define UCOL_PROPORTION2 0.5
#define UCOL_PROPORTION3 0.667


#define UCOL_COMMON_BOT2 UCOL_BYTE_COMMON
#define UCOL_COMMON_TOP2 0x86u
#define UCOL_TOTAL2 (UCOL_COMMON_TOP2-UCOL_COMMON_BOT2-1) 

#define UCOL_FLAG_BIT_MASK_CASE_SW_OFF 0x80
#define UCOL_FLAG_BIT_MASK_CASE_SW_ON 0x40
#define UCOL_COMMON_TOP3_CASE_SW_OFF 0x85
#define UCOL_COMMON_TOP3_CASE_SW_LOWER 0x45
#define UCOL_COMMON_TOP3_CASE_SW_UPPER 0xC5


#define UCOL_COMMON_BOT3 0x05

#define UCOL_COMMON_BOTTOM3_CASE_SW_UPPER 0x86;
#define UCOL_COMMON_BOTTOM3_CASE_SW_LOWER UCOL_COMMON_BOT3;

#define UCOL_TOP_COUNT2  (UCOL_PROPORTION2*UCOL_TOTAL2)
#define UCOL_BOT_COUNT2  (UCOL_TOTAL2-UCOL_TOP_COUNT2)


#define UCOL_COMMON2 UCOL_COMMON_BOT2
#define UCOL_COMMON3_UPPERFIRST 0xC5
#define UCOL_COMMON3_NORMAL UCOL_COMMON_BOT3

#define UCOL_COMMON4 0xFF



#define UCOL_CASE_SWITCH      0xC0
#define UCOL_NO_CASE_SWITCH   0x00

#define UCOL_REMOVE_CASE      0x3F
#define UCOL_KEEP_CASE        0xFF

#define UCOL_CASE_BIT_MASK    0xC0

#define UCOL_TERT_CASE_MASK   0xFF

#define UCOL_ENDOFLATINONERANGE 0xFF
#define UCOL_LATINONETABLELEN   (UCOL_ENDOFLATINONERANGE+50)
#define UCOL_BAIL_OUT_CE      0xFF000000


typedef enum {
    NOT_FOUND_TAG = 0,
    EXPANSION_TAG = 1,       
    CONTRACTION_TAG = 2,     
    THAI_TAG = 3,            
    CHARSET_TAG = 4,         
    SURROGATE_TAG = 5,       
    HANGUL_SYLLABLE_TAG = 6, 
    LEAD_SURROGATE_TAG = 7,  
    TRAIL_SURROGATE_TAG = 8,     
    CJK_IMPLICIT_TAG = 9,    
    IMPLICIT_TAG = 10,
    SPEC_PROC_TAG = 11,
    
    LONG_PRIMARY_TAG = 12,   
                             
                             
                             
    DIGIT_TAG = 13,          
    
    CE_TAGS_COUNT
} UColCETags;








typedef struct {
      uint32_t variableTopValue;
       int32_t frenchCollation;
       int32_t alternateHandling; 
       int32_t caseFirst;         
       int32_t caseLevel;         
       int32_t normalizationMode; 
       int32_t strength;          
       int32_t hiraganaQ;         
       int32_t numericCollation;  
      uint32_t reserved[15];                 
} UColOptionSet;

typedef struct {
  uint32_t UCA_FIRST_TERTIARY_IGNORABLE[2];       
  uint32_t UCA_LAST_TERTIARY_IGNORABLE[2];        
  uint32_t UCA_FIRST_PRIMARY_IGNORABLE[2];        
  uint32_t UCA_FIRST_SECONDARY_IGNORABLE[2];      
  uint32_t UCA_LAST_SECONDARY_IGNORABLE[2];       
  uint32_t UCA_LAST_PRIMARY_IGNORABLE[2];         
  uint32_t UCA_FIRST_VARIABLE[2];                 
  uint32_t UCA_LAST_VARIABLE[2];                  
  uint32_t UCA_FIRST_NON_VARIABLE[2];             
  uint32_t UCA_LAST_NON_VARIABLE[2];              
  uint32_t UCA_RESET_TOP_VALUE[2];                
  uint32_t UCA_FIRST_IMPLICIT[2];
  uint32_t UCA_LAST_IMPLICIT[2]; 
  uint32_t UCA_FIRST_TRAILING[2];
  uint32_t UCA_LAST_TRAILING[2]; 

#if 0
  uint32_t UCA_NEXT_TOP_VALUE[2];                 
  uint32_t UCA_NEXT_FIRST_PRIMARY_IGNORABLE;   
  uint32_t UCA_NEXT_LAST_PRIMARY_IGNORABLE;    
  uint32_t UCA_NEXT_FIRST_SECONDARY_IGNORABLE; 
  uint32_t UCA_NEXT_LAST_SECONDARY_IGNORABLE;  
  uint32_t UCA_NEXT_FIRST_TERTIARY_IGNORABLE;  
  uint32_t UCA_NEXT_LAST_TERTIARY_IGNORABLE;   
  uint32_t UCA_NEXT_FIRST_VARIABLE;            
  uint32_t UCA_NEXT_LAST_VARIABLE;             
#endif

  uint32_t UCA_PRIMARY_TOP_MIN;
  uint32_t UCA_PRIMARY_IMPLICIT_MIN; 
  uint32_t UCA_PRIMARY_IMPLICIT_MAX; 
  uint32_t UCA_PRIMARY_TRAILING_MIN; 
  uint32_t UCA_PRIMARY_TRAILING_MAX; 
  uint32_t UCA_PRIMARY_SPECIAL_MIN; 
  uint32_t UCA_PRIMARY_SPECIAL_MAX; 
} UCAConstants;



#define U_UNKNOWN_STATE 0
#define U_COLLATOR_STATE 0x01
#define U_STATE_LIMIT 0x02



typedef struct {
  
  
  uint8_t sizeLo;
  uint8_t sizeHi;
  
  uint8_t isBigEndian;
  
  uint8_t charsetFamily;
  
  uint8_t icuVersion[4];
  
  uint8_t type;
  
  uint8_t reserved[7];
} UStateStruct;





typedef struct {
  
  uint8_t sizeLo;
  uint8_t sizeHi;
  
  uint8_t containsTailoring;
  
  uint8_t containsUCA;
  
  uint8_t versionInfo[4];

  
  uint8_t charsetName[32];                 
  
  uint8_t locale[32];                      

  
  
  
  uint32_t variableTopValue;
  
  uint32_t  alternateHandling; 
  
  uint32_t  frenchCollation;
  
  uint32_t  caseFirst;         
  
  uint32_t  caseLevel;         
  
  uint32_t  normalizationMode; 
  
  uint32_t  strength;
  
  uint8_t reserved[12];
} UColStateStruct;

#define UCOL_INV_SIZEMASK 0xFFF00000
#define UCOL_INV_OFFSETMASK 0x000FFFFF
#define UCOL_INV_SHIFTVALUE 20

U_CDECL_BEGIN



typedef void U_CALLCONV
ResourceCleaner(UCollator *coll);


struct UCollator {
    UColOptionSet  *options;
    SortKeyGenerator *sortKeyGen;
    uint32_t *latinOneCEs;
    char* actualLocale;
    char* validLocale;
    char* requestedLocale;
    const UChar *rules;
    const UChar *ucaRules;
    const UCollator *UCA;
    const UCATableHeader *image;
    UTrie mapping;
    const uint32_t *latinOneMapping;
    const uint32_t *expansion;
    const UChar    *contractionIndex;
    const uint32_t *contractionCEs;

    const uint32_t *endExpansionCE;    

    const uint32_t *lastEndExpansionCE;
    const uint8_t  *expansionCESize;   



    const uint8_t *unsafeCP;           
    const uint8_t *contrEndCP;         
    UChar          minUnsafeCP;        
    UChar          minContrEndCP;      

    int32_t rulesLength;
    int32_t latinOneTableLen;

    uint32_t variableTopValue;
    UColAttributeValue frenchCollation;
    UColAttributeValue alternateHandling; 
    UColAttributeValue caseFirst;         
    UColAttributeValue caseLevel;         
    UColAttributeValue normalizationMode; 
    UColAttributeValue strength;          
    UColAttributeValue hiraganaQ;         
    UColAttributeValue numericCollation;
    UBool variableTopValueisDefault;
    UBool frenchCollationisDefault;
    UBool alternateHandlingisDefault; 
    UBool caseFirstisDefault;         
    UBool caseLevelisDefault;         
    UBool normalizationModeisDefault; 
    UBool strengthisDefault;          
    UBool hiraganaQisDefault;         
    UBool numericCollationisDefault;
    UBool hasRealData;                
                                      
                                      

    UBool freeOnClose;
    UBool freeOptionsOnClose;
    UBool freeRulesOnClose;
    UBool freeImageOnClose;
    UBool freeDefaultReorderCodesOnClose;
    UBool freeReorderCodesOnClose;
    UBool freeLeadBytePermutationTableOnClose;

    UBool latinOneUse;
    UBool latinOneRegenTable;
    UBool latinOneFailed;

    int8_t tertiaryAddition; 
    uint8_t caseSwitch;
    uint8_t tertiaryCommon;
    uint8_t tertiaryMask;
    uint8_t tertiaryTop; 
    uint8_t tertiaryBottom; 
    uint8_t tertiaryTopCount;
    uint8_t tertiaryBottomCount;

    UVersionInfo dataVersion;               
    int32_t* defaultReorderCodes;
    int32_t defaultReorderCodesLength;
    int32_t* reorderCodes;
    int32_t reorderCodesLength;
    uint8_t* leadBytePermutationTable;
    void  *delegate;  
};

U_CDECL_END




U_CFUNC
UCollator* ucol_initUCA(UErrorCode *status);

U_CFUNC
UCollator* ucol_initCollator(const UCATableHeader *image, UCollator *fillIn, const UCollator *UCA, UErrorCode *status);

U_CFUNC
void ucol_setOptionsFromHeader(UCollator* result, UColOptionSet * opts, UErrorCode *status);

U_CFUNC
UCollator* ucol_open_internal(const char* loc, UErrorCode* status);

#if 0
U_CFUNC
void ucol_putOptionsToHeader(UCollator* result, UColOptionSet * opts, UErrorCode *status);
#endif

U_CFUNC
void ucol_updateInternalState(UCollator *coll, UErrorCode *status);

U_CFUNC uint32_t U_EXPORT2 ucol_getFirstCE(const UCollator *coll, UChar u, UErrorCode *status);
U_CAPI UBool U_EXPORT2 ucol_isTailored(const UCollator *coll, const UChar u, UErrorCode *status);

U_CAPI const InverseUCATableHeader* U_EXPORT2 ucol_initInverseUCA(UErrorCode *status);

U_CAPI void U_EXPORT2 
uprv_uca_initImplicitConstants(UErrorCode *status);

U_CAPI uint32_t U_EXPORT2
uprv_uca_getImplicitFromRaw(UChar32 cp);




U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromImplicit(uint32_t implicit);

U_CAPI UChar32 U_EXPORT2
uprv_uca_getRawFromCodePoint(UChar32 i);

U_CAPI UChar32 U_EXPORT2
uprv_uca_getCodePointFromRaw(UChar32 i);

typedef const UChar* GetCollationRulesFunction(void* context, const char* locale, const char* type, int32_t* pLength, UErrorCode* status);

U_CAPI UCollator* U_EXPORT2
ucol_openRulesForImport( const UChar        *rules,
                         int32_t            rulesLength,
                         UColAttributeValue normalizationMode,
                         UCollationStrength strength,
                         UParseError        *parseError,
                         GetCollationRulesFunction  importFunc,
                         void* context,
                         UErrorCode         *status);

       
U_CFUNC void U_EXPORT2 
ucol_buildPermutationTable(UCollator *coll, UErrorCode *status);

U_CFUNC int U_EXPORT2 
ucol_getLeadBytesForReorderCode(const UCollator *uca, int reorderCode, uint16_t* returnLeadBytes, int returnCapacity);

U_CFUNC int U_EXPORT2 
ucol_getReorderCodesForLeadByte(const UCollator *uca, int leadByte, int16_t* returnReorderCodes, int returnCapacity);

#ifdef __cplusplus








static inline UBool ucol_unsafeCP(UChar c, const UCollator *coll) {
    int32_t  hash;
    uint8_t  htbyte;

    if (c < coll->minUnsafeCP) {
        return FALSE;
    }

    hash = c;
    if (hash >= UCOL_UNSAFECP_TABLE_SIZE*8) {
        if(U16_IS_SURROGATE(c)) {
            
            
            return TRUE;
        }
        hash = (hash & UCOL_UNSAFECP_TABLE_MASK) + 256;
    }
    htbyte = coll->unsafeCP[hash>>3];
    return ((htbyte >> (hash & 7)) & 1);
}
#endif 


void ucol_freeOffsetBuffer(U_NAMESPACE_QUALIFIER collIterate *s); 

#endif 

#endif
