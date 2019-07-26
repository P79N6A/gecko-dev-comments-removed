















#ifndef __UCNVMBCS_H__
#define __UCNVMBCS_H__

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "ucnv_cnv.h"
#include "ucnv_ext.h"





















































































































































































































enum {
    MBCS_MAX_STATE_COUNT=128
};





enum {
    MBCS_STATE_VALID_DIRECT_16,
    MBCS_STATE_VALID_DIRECT_20,

    MBCS_STATE_FALLBACK_DIRECT_16,
    MBCS_STATE_FALLBACK_DIRECT_20,

    MBCS_STATE_VALID_16,
    MBCS_STATE_VALID_16_PAIR,

    MBCS_STATE_UNASSIGNED,
    MBCS_STATE_ILLEGAL,

    MBCS_STATE_CHANGE_ONLY
};


#define MBCS_ENTRY_TRANSITION(state, offset) (int32_t)(((int32_t)(state)<<24L)|(offset))
#define MBCS_ENTRY_TRANSITION_SET_OFFSET(entry, offset) (int32_t)(((entry)&0xff000000)|(offset))
#define MBCS_ENTRY_TRANSITION_ADD_OFFSET(entry, offset) (int32_t)((entry)+(offset))

#define MBCS_ENTRY_FINAL(state, action, value) (int32_t)(0x80000000|((int32_t)(state)<<24L)|((action)<<20L)|(value))
#define MBCS_ENTRY_SET_FINAL(entry) (int32_t)((entry)|0x80000000)
#define MBCS_ENTRY_FINAL_SET_ACTION(entry, action) (int32_t)(((entry)&0xff0fffff)|((int32_t)(action)<<20L))
#define MBCS_ENTRY_FINAL_SET_VALUE(entry, value) (int32_t)(((entry)&0xfff00000)|(value))
#define MBCS_ENTRY_FINAL_SET_ACTION_VALUE(entry, action, value) (int32_t)(((entry)&0xff000000)|((int32_t)(action)<<20L)|(value))

#define MBCS_ENTRY_SET_STATE(entry, state) (int32_t)(((entry)&0x80ffffff)|((int32_t)(state)<<24L))

#define MBCS_ENTRY_STATE(entry) ((((uint32_t)entry)>>24)&0x7f)

#define MBCS_ENTRY_IS_TRANSITION(entry) ((entry)>=0)
#define MBCS_ENTRY_IS_FINAL(entry) ((entry)<0)

#define MBCS_ENTRY_TRANSITION_STATE(entry) (((uint32_t)entry)>>24)
#define MBCS_ENTRY_TRANSITION_OFFSET(entry) ((entry)&0xffffff)

#define MBCS_ENTRY_FINAL_STATE(entry) ((((uint32_t)entry)>>24)&0x7f)
#define MBCS_ENTRY_FINAL_IS_VALID_DIRECT_16(entry) ((entry)<(int32_t)0x80100000)
#define MBCS_ENTRY_FINAL_ACTION(entry) ((((uint32_t)entry)>>20)&0xf)
#define MBCS_ENTRY_FINAL_VALUE(entry) ((entry)&0xfffff)
#define MBCS_ENTRY_FINAL_VALUE_16(entry) (uint16_t)(entry)

#define IS_ASCII_ROUNDTRIP(b, asciiRoundtrips) (((asciiRoundtrips) & (1<<((b)>>2)))!=0)


#define MBCS_SINGLE_RESULT_FROM_U(table, results, c) (results)[ (table)[ (table)[(c)>>10] +(((c)>>4)&0x3f) ] +((c)&0xf) ]


#define SBCS_RESULT_FROM_LOW_BMP(table, results, c) (results)[ (table)[(c)>>6] +((c)&0x3f) ]


#define SBCS_RESULT_FROM_UTF8(table, results, l, t) (results)[ (table)[l] +(t) ]


#define MBCS_STAGE_2_FROM_U(table, c) ((const uint32_t *)(table))[ (table)[(c)>>10] +(((c)>>4)&0x3f) ]
#define MBCS_FROM_U_IS_ROUNDTRIP(stage2Entry, c) ( ((stage2Entry) & ((uint32_t)1<< (16+((c)&0xf)) )) !=0)

#define MBCS_VALUE_2_FROM_STAGE_2(bytes, stage2Entry, c) ((uint16_t *)(bytes))[16*(uint32_t)(uint16_t)(stage2Entry)+((c)&0xf)]
#define MBCS_VALUE_4_FROM_STAGE_2(bytes, stage2Entry, c) ((uint32_t *)(bytes))[16*(uint32_t)(uint16_t)(stage2Entry)+((c)&0xf)]

#define MBCS_POINTER_3_FROM_STAGE_2(bytes, stage2Entry, c) ((bytes)+(16*(uint32_t)(uint16_t)(stage2Entry)+((c)&0xf))*3)


#define DBCS_RESULT_FROM_MOST_BMP(table, results, c) (results)[ (table)[(c)>>6] +((c)&0x3f) ]


#define DBCS_RESULT_FROM_UTF8(table, results, lt1, t2) (results)[ (table)[lt1] +(t2) ]







enum {
    MBCS_OUTPUT_1,          
    MBCS_OUTPUT_2,          
    MBCS_OUTPUT_3,          
    MBCS_OUTPUT_4,          

    MBCS_OUTPUT_3_EUC=8,    
    MBCS_OUTPUT_4_EUC,      

    MBCS_OUTPUT_2_SISO=12,  
    MBCS_OUTPUT_2_HZ,       

    MBCS_OUTPUT_EXT_ONLY,   

    MBCS_OUTPUT_COUNT,

    MBCS_OUTPUT_DBCS_ONLY=0xdb  
};





typedef struct {
    uint32_t offset;
    UChar32 codePoint;
} _MBCSToUFallback;


enum {
    SBCS_FAST_MAX=0x0fff,               
    SBCS_FAST_LIMIT=SBCS_FAST_MAX+1,    
    MBCS_FAST_MAX=0xd7ff,               
    MBCS_FAST_LIMIT=MBCS_FAST_MAX+1     
};







typedef struct UConverterMBCSTable {
    
    uint8_t countStates, dbcsOnlyState, stateTableOwned;
    uint32_t countToUFallbacks;

    const int32_t (*stateTable)[256];
    int32_t (*swapLFNLStateTable)[256]; 
    const uint16_t *unicodeCodeUnits;
    const _MBCSToUFallback *toUFallbacks;

    
    const uint16_t *fromUnicodeTable;
    const uint16_t *mbcsIndex;              
    uint16_t sbcsIndex[SBCS_FAST_LIMIT>>6]; 
    const uint8_t *fromUnicodeBytes;
    uint8_t *swapLFNLFromUnicodeBytes;      
    uint32_t fromUBytesLength;
    uint8_t outputType, unicodeMask;
    UBool utf8Friendly;                     
    UChar maxFastUChar;                     

    
    uint32_t asciiRoundtrips;

    
    uint8_t *reconstitutedData;

    
    char *swapLFNLName;

    
    struct UConverterSharedData *baseSharedData;
    const int32_t *extIndexes;
} UConverterMBCSTable;

#define UCNV_MBCS_TABLE_INITIALIZER { \
    /* toUnicode */ \
    0, 0, 0, \
    0, \
     \
    NULL, \
    NULL, \
    NULL, \
    NULL, \
     \
    /* fromUnicode */ \
    NULL, \
    NULL, \
    { 0 }, \
    NULL, \
    NULL, \
    0, \
    0, 0, \
    FALSE, \
    0, \
     \
    /* roundtrips */ \
    0, \
     \
    /* reconstituted data that was omitted from the .cnv file */ \
    NULL, \
     \
    /* converter name for swaplfnl */ \
    NULL, \
     \
    /* extension data */ \
    NULL, \
    NULL \
}

enum {
    MBCS_OPT_LENGTH_MASK=0x3f,
    MBCS_OPT_NO_FROM_U=0x40,
    



    MBCS_OPT_INCOMPATIBLE_MASK=0xffc0,
    



    MBCS_OPT_UNKNOWN_INCOMPATIBLE_MASK=0xff80
};

enum {
    MBCS_HEADER_V4_LENGTH=8,
    MBCS_HEADER_V5_MIN_LENGTH=9
};




typedef struct {
    UVersionInfo version;
    uint32_t countStates,
             countToUFallbacks,
             offsetToUCodeUnits,
             offsetFromUTable,
             offsetFromUBytes,
             flags,
             fromUBytesLength;

    
    uint32_t options;

    
    uint32_t fullStage2Length;  
} _MBCSHeader;

#define UCNV_MBCS_HEADER_INITIALIZER { { 0 },  0, 0, 0, 0, 0, 0, 0,  0,  0 }














U_CFUNC UChar32
ucnv_MBCSSimpleGetNextUChar(UConverterSharedData *sharedData,
                        const char *source, int32_t length,
                        UBool useFallback);






U_CFUNC UChar32
ucnv_MBCSSingleSimpleGetNextUChar(UConverterSharedData *sharedData,
                              uint8_t b, UBool useFallback);







#define _MBCS_SINGLE_SIMPLE_GET_NEXT_BMP(sharedData, b) \
    (UChar)MBCS_ENTRY_FINAL_VALUE_16((sharedData)->mbcs.stateTable[0][(uint8_t)(b)])





U_CFUNC UBool
ucnv_MBCSIsLeadByte(UConverterSharedData *sharedData, char byte);


#define _MBCS_IS_LEAD_BYTE(sharedData, byte) \
    (UBool)MBCS_ENTRY_IS_TRANSITION((sharedData)->mbcs.stateTable[0][(uint8_t)(byte)])


















U_CFUNC int32_t
ucnv_MBCSFromUChar32(UConverterSharedData *sharedData,
                 UChar32 c, uint32_t *pValue,
                 UBool useFallback);







U_CFUNC int32_t
ucnv_MBCSSingleFromUChar32(UConverterSharedData *sharedData,
                       UChar32 c,
                       UBool useFallback);





U_CFUNC UConverterType
ucnv_MBCSGetType(const UConverter* converter);

U_CFUNC void 
ucnv_MBCSFromUnicodeWithOffsets(UConverterFromUnicodeArgs *pArgs,
                            UErrorCode *pErrorCode);
U_CFUNC void 
ucnv_MBCSToUnicodeWithOffsets(UConverterToUnicodeArgs *pArgs,
                          UErrorCode *pErrorCode);









U_CFUNC void
ucnv_MBCSGetUnicodeSetForUnicode(const UConverterSharedData *sharedData,
                                 const USetAdder *sa,
                                 UConverterUnicodeSet which,
                                 UErrorCode *pErrorCode);







U_CFUNC void
ucnv_MBCSGetFilteredUnicodeSetForUnicode(const UConverterSharedData *sharedData,
                                         const USetAdder *sa,
                                         UConverterUnicodeSet which,
                                         UConverterSetFilter filter,
                                         UErrorCode *pErrorCode);

#endif

#endif
