















#ifndef UCNV_BLD_H
#define UCNV_BLD_H

#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/ucnv_err.h"
#include "unicode/utf16.h"
#include "ucnv_cnv.h"
#include "ucnvmbcs.h"
#include "ucnv_ext.h"
#include "udataswp.h"


#define UCNV_ERROR_BUFFER_LENGTH 32


#define UCNV_MAX_SUBCHAR_LEN 4


#define UCNV_MAX_CHAR_LEN 8


#define UCNV_OPTION_VERSION     0xf
#define UCNV_OPTION_SWAP_LFNL   0x10

#define UCNV_GET_VERSION(cnv) ((cnv)->options&UCNV_OPTION_VERSION)

U_CDECL_BEGIN 




union UConverterTable {
    UConverterMBCSTable mbcs;
};

typedef union UConverterTable UConverterTable;

struct UConverterImpl;
typedef struct UConverterImpl UConverterImpl;


#define UCNV_HAS_SUPPLEMENTARY 1
#define UCNV_HAS_SURROGATES    2

typedef struct UConverterStaticData {   
    uint32_t structSize;                
    
    char name 
      [UCNV_MAX_CONVERTER_NAME_LENGTH]; 

    int32_t codepage;               

    int8_t platform;                
    int8_t conversionType;          

    int8_t minBytesPerChar;         
    int8_t maxBytesPerChar;         

    uint8_t subChar[UCNV_MAX_SUBCHAR_LEN]; 
    int8_t subCharLen;              
    
    uint8_t hasToUnicodeFallback;   
    uint8_t hasFromUnicodeFallback; 
    uint8_t unicodeMask;            
    uint8_t subChar1;               
    uint8_t reserved[19];           
                                    
} UConverterStaticData;





struct UConverterSharedData {
    uint32_t structSize;            
    uint32_t referenceCounter;      

    const void *dataMemory;         
    void *table;                    

    const UConverterStaticData *staticData; 

    UBool                sharedDataCached;   
  

    const UConverterImpl *impl;     

    
    uint32_t toUnicodeStatus;

    













    UConverterMBCSTable mbcs;
};



struct UConverter {
    



    void (U_EXPORT2 *fromUCharErrorBehaviour) (const void *context,
                                     UConverterFromUnicodeArgs *args,
                                     const UChar *codeUnits,
                                     int32_t length,
                                     UChar32 codePoint,
                                     UConverterCallbackReason reason,
                                     UErrorCode *);
    



    void (U_EXPORT2 *fromCharErrorBehaviour) (const void *context,
                                    UConverterToUnicodeArgs *args,
                                    const char *codeUnits,
                                    int32_t length,
                                    UConverterCallbackReason reason,
                                    UErrorCode *);

    



    void *extraInfo;

    const void *fromUContext;
    const void *toUContext;

    








    uint8_t *subChars;

    UConverterSharedData *sharedData;   

    uint32_t options; 

    UBool sharedDataIsCached;  
    UBool isCopyLocal;  
    UBool isExtraLocal; 

    UBool  useFallback;
    int8_t toULength;                   
    uint8_t toUBytes[UCNV_MAX_CHAR_LEN-1];
    uint32_t toUnicodeStatus;           
    int32_t mode;
    uint32_t fromUnicodeStatus;

    








    UChar32 fromUChar32;

    





    int8_t maxBytesPerUChar;

    int8_t subCharLen;                  
    int8_t invalidCharLength;
    int8_t charErrorBufferLength;       

    int8_t invalidUCharLength;
    int8_t UCharErrorBufferLength;      

    uint8_t subChar1;                                   
    UBool useSubChar1;
    char invalidCharBuffer[UCNV_MAX_CHAR_LEN];          
    uint8_t charErrorBuffer[UCNV_ERROR_BUFFER_LENGTH];  
    UChar subUChars[UCNV_MAX_SUBCHAR_LEN/U_SIZEOF_UCHAR]; 

    UChar invalidUCharBuffer[U16_MAX_LENGTH];           
    UChar UCharErrorBuffer[UCNV_ERROR_BUFFER_LENGTH];   

    

    
    UChar32 preFromUFirstCP;                
    UChar preFromU[UCNV_EXT_MAX_UCHARS];
    char preToU[UCNV_EXT_MAX_BYTES];
    int8_t preFromULength, preToULength;    
    int8_t preToUFirstLength;               

    
    UConverterCallbackReason toUCallbackReason; 
};

U_CDECL_END 

#define CONVERTER_FILE_EXTENSION ".cnv"







U_CFUNC uint16_t
ucnv_bld_countAvailableConverters(UErrorCode *pErrorCode);









U_CFUNC const char *
ucnv_bld_getAvailableConverter(uint16_t n, UErrorCode *pErrorCode);





U_CAPI UConverterSharedData *
ucnv_load(UConverterLoadArgs *pArgs, UErrorCode *err);






U_CAPI void
ucnv_unload(UConverterSharedData *sharedData);





U_CAPI int32_t U_EXPORT2
ucnv_swap(const UDataSwapper *ds,
          const void *inData, int32_t length, void *outData,
          UErrorCode *pErrorCode);

#endif

#endif 
