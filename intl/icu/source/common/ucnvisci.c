
















#include "unicode/utypes.h"

#if !UCONFIG_NO_CONVERSION && !UCONFIG_NO_LEGACY_CONVERSION

#include "unicode/ucnv.h"
#include "unicode/ucnv_cb.h"
#include "unicode/utf16.h"
#include "cmemory.h"
#include "ucnv_bld.h"
#include "ucnv_cnv.h"
#include "cstring.h"
#include "uassert.h"

#define UCNV_OPTIONS_VERSION_MASK 0xf
#define NUKTA               0x093c
#define HALANT              0x094d
#define ZWNJ                0x200c /* Zero Width Non Joiner */
#define ZWJ                 0x200d /* Zero width Joiner */
#define INVALID_CHAR        0xffff
#define ATR                 0xEF   /* Attribute code */
#define EXT                 0xF0   /* Extension code */
#define DANDA               0x0964
#define DOUBLE_DANDA        0x0965
#define ISCII_NUKTA         0xE9
#define ISCII_HALANT        0xE8
#define ISCII_DANDA         0xEA
#define ISCII_INV           0xD9
#define ISCII_VOWEL_SIGN_E  0xE0
#define INDIC_BLOCK_BEGIN   0x0900
#define INDIC_BLOCK_END     0x0D7F
#define INDIC_RANGE         (INDIC_BLOCK_END - INDIC_BLOCK_BEGIN)
#define VOCALLIC_RR         0x0931
#define LF                  0x0A
#define ASCII_END           0xA0
#define NO_CHAR_MARKER      0xFFFE
#define TELUGU_DELTA        DELTA * TELUGU
#define DEV_ABBR_SIGN       0x0970
#define DEV_ANUDATTA        0x0952
#define EXT_RANGE_BEGIN     0xA1
#define EXT_RANGE_END       0xEE

#define PNJ_DELTA           0x0100
#define PNJ_BINDI           0x0A02
#define PNJ_TIPPI           0x0A70
#define PNJ_SIGN_VIRAMA     0x0A4D
#define PNJ_ADHAK           0x0A71
#define PNJ_HA              0x0A39
#define PNJ_RRA             0x0A5C

typedef enum {
    DEVANAGARI =0,
    BENGALI,
    GURMUKHI,
    GUJARATI,
    ORIYA,
    TAMIL,
    TELUGU,
    KANNADA,
    MALAYALAM,
    DELTA=0x80
}UniLang;





typedef enum {
    DEF = 0x40,
    RMN = 0x41,
    DEV = 0x42,
    BNG = 0x43,
    TML = 0x44,
    TLG = 0x45,
    ASM = 0x46,
    ORI = 0x47,
    KND = 0x48,
    MLM = 0x49,
    GJR = 0x4A,
    PNJ = 0x4B,
    ARB = 0x71,
    PES = 0x72,
    URD = 0x73,
    SND = 0x74,
    KSM = 0x75,
    PST = 0x76
}ISCIILang;

typedef enum {
    DEV_MASK =0x80,
    PNJ_MASK =0x40,
    GJR_MASK =0x20,
    ORI_MASK =0x10,
    BNG_MASK =0x08,
    KND_MASK =0x04,
    MLM_MASK =0x02,
    TML_MASK =0x01,
    ZERO =0x00
}MaskEnum;

#define ISCII_CNV_PREFIX "ISCII,version="

typedef struct {
    UChar contextCharToUnicode;         
    UChar contextCharFromUnicode;       
    uint16_t defDeltaToUnicode;         
    uint16_t currentDeltaFromUnicode;   
    uint16_t currentDeltaToUnicode;     
    MaskEnum currentMaskFromUnicode;    
    MaskEnum currentMaskToUnicode;      
    MaskEnum defMaskToUnicode;          
    UBool isFirstBuffer;                
    UBool resetToDefaultToUnicode;      
    char name[sizeof(ISCII_CNV_PREFIX) + 1];
    UChar32 prevToUnicodeStatus;        
} UConverterDataISCII;

typedef struct LookupDataStruct {
    UniLang uniLang;
    MaskEnum maskEnum;
    ISCIILang isciiLang;
} LookupDataStruct;

static const LookupDataStruct lookupInitialData[]={
    { DEVANAGARI, DEV_MASK,  DEV },
    { BENGALI,    BNG_MASK,  BNG },
    { GURMUKHI,   PNJ_MASK,  PNJ },
    { GUJARATI,   GJR_MASK,  GJR },
    { ORIYA,      ORI_MASK,  ORI },
    { TAMIL,      TML_MASK,  TML },
    { TELUGU,     KND_MASK,  TLG },
    { KANNADA,    KND_MASK,  KND },
    { MALAYALAM,  MLM_MASK,  MLM }
};






static const uint8_t pnjMap[80] = {
    
    0, 0, 0, 0, 0, 2, 0, 2,  0, 0, 0, 0, 0, 0, 0, 0,
    
    0, 0, 0, 0, 0, 3, 3, 3,  3, 3, 3, 3, 3, 3, 3, 3,
    
    3, 3, 3, 3, 3, 3, 3, 3,  3, 0, 3, 3, 3, 3, 3, 3,
    
    3, 0, 0, 0, 0, 3, 3, 0,  3, 3, 0, 0, 0, 0, 0, 2,
    
    0, 2, 2, 0, 0, 0, 0, 0,  0, 0, 0, 0, 0, 0, 0, 0
};

static UBool
isPNJConsonant(UChar32 c) {
    if (c < 0xa00 || 0xa50 <= c) {
        return FALSE;
    } else {
        return (UBool)(pnjMap[c - 0xa00] & 1);
    }
}

static UBool
isPNJBindiTippi(UChar32 c) {
    if (c < 0xa00 || 0xa50 <= c) {
        return FALSE;
    } else {
        return (UBool)(pnjMap[c - 0xa00] >> 1);
    }
}

static void _ISCIIOpen(UConverter *cnv, UConverterLoadArgs *pArgs, UErrorCode *errorCode) {
    if(pArgs->onlyTestIsLoadable) {
        return;
    }

    cnv->extraInfo = uprv_malloc(sizeof(UConverterDataISCII));

    if (cnv->extraInfo != NULL) {
        int32_t len=0;
        UConverterDataISCII *converterData=
                (UConverterDataISCII *) cnv->extraInfo;
        converterData->contextCharToUnicode=NO_CHAR_MARKER;
        cnv->toUnicodeStatus = missingCharMarker;
        converterData->contextCharFromUnicode=0x0000;
        converterData->resetToDefaultToUnicode=FALSE;
        
        if ((pArgs->options & UCNV_OPTIONS_VERSION_MASK) < 9) {
            
            converterData->currentDeltaFromUnicode
                    = converterData->currentDeltaToUnicode
                            = converterData->defDeltaToUnicode = (uint16_t)(lookupInitialData[pArgs->options & UCNV_OPTIONS_VERSION_MASK].uniLang * DELTA);

            converterData->currentMaskFromUnicode
                    = converterData->currentMaskToUnicode
                            = converterData->defMaskToUnicode = lookupInitialData[pArgs->options & UCNV_OPTIONS_VERSION_MASK].maskEnum;
            
            converterData->isFirstBuffer=TRUE;
            (void)uprv_strcpy(converterData->name, ISCII_CNV_PREFIX);
            len = (int32_t)uprv_strlen(converterData->name);
            converterData->name[len]= (char)((pArgs->options & UCNV_OPTIONS_VERSION_MASK) + '0');
            converterData->name[len+1]=0;
            
            converterData->prevToUnicodeStatus = 0x0000;
        } else {
            uprv_free(cnv->extraInfo);
            cnv->extraInfo = NULL;
            *errorCode = U_ILLEGAL_ARGUMENT_ERROR;
        }

    } else {
        *errorCode =U_MEMORY_ALLOCATION_ERROR;
    }
}

static void _ISCIIClose(UConverter *cnv) {
    if (cnv->extraInfo!=NULL) {
        if (!cnv->isExtraLocal) {
            uprv_free(cnv->extraInfo);
        }
        cnv->extraInfo=NULL;
    }
}

static const char* _ISCIIgetName(const UConverter* cnv) {
    if (cnv->extraInfo) {
        UConverterDataISCII* myData= (UConverterDataISCII*)cnv->extraInfo;
        return myData->name;
    }
    return NULL;
}

static void _ISCIIReset(UConverter *cnv, UConverterResetChoice choice) {
    UConverterDataISCII* data =(UConverterDataISCII *) (cnv->extraInfo);
    if (choice<=UCNV_RESET_TO_UNICODE) {
        cnv->toUnicodeStatus = missingCharMarker;
        cnv->mode=0;
        data->currentDeltaToUnicode=data->defDeltaToUnicode;
        data->currentMaskToUnicode = data->defMaskToUnicode;
        data->contextCharToUnicode=NO_CHAR_MARKER;
        data->prevToUnicodeStatus = 0x0000;
    }
    if (choice!=UCNV_RESET_TO_UNICODE) {
        cnv->fromUChar32=0x0000;
        data->contextCharFromUnicode=0x00;
        data->currentMaskFromUnicode=data->defMaskToUnicode;
        data->currentDeltaFromUnicode=data->defDeltaToUnicode;
        data->isFirstBuffer=TRUE;
        data->resetToDefaultToUnicode=FALSE;
    }
}





















static const uint8_t validityTable[128] = {



 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + GJR_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + GJR_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + ZERO     + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + ZERO     + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + ZERO     + ORI_MASK + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + GJR_MASK + ZERO     + BNG_MASK + KND_MASK + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + GJR_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + ZERO     + GJR_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + GJR_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + KND_MASK + ZERO     + ZERO     ,
 ZERO     + ZERO     + ZERO     + ORI_MASK + ZERO     + KND_MASK + ZERO     + ZERO     ,
 ZERO     + ZERO     + ZERO     + ORI_MASK + BNG_MASK + ZERO     + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + ZERO     + ZERO     + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ORI_MASK + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ORI_MASK + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + BNG_MASK + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + GJR_MASK + ORI_MASK + BNG_MASK + KND_MASK + MLM_MASK + TML_MASK ,
 DEV_MASK + PNJ_MASK + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     ,





 ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO     + ZERO
};

static const uint16_t fromUnicodeTable[128]={
    0x00a0 ,
    0x00a1 ,
    0x00a2 ,
    0x00a3 ,
    0xa4e0 ,
    0x00a4 ,
    0x00a5 ,
    0x00a6 ,
    0x00a7 ,
    0x00a8 ,
    0x00a9 ,
    0x00aa ,
    0xA6E9 ,
    0x00ae ,
    0x00ab ,
    0x00ac ,
    0x00ad ,
    0x00b2 ,
    0x00af ,
    0x00b0 ,
    0x00b1 ,
    0x00b3 ,
    0x00b4 ,
    0x00b5 ,
    0x00b6 ,
    0x00b7 ,
    0x00b8 ,
    0x00b9 ,
    0x00ba ,
    0x00bb ,
    0x00bc ,
    0x00bd ,
    0x00be ,
    0x00bf ,
    0x00c0 ,
    0x00c1 ,
    0x00c2 ,
    0x00c3 ,
    0x00c4 ,
    0x00c5 ,
    0x00c6 ,
    0x00c7 ,
    0x00c8 ,
    0x00c9 ,
    0x00ca ,
    0x00cb ,
    0x00cc ,
    0x00cd ,
    0x00cf ,
    0x00d0 ,
    0x00d1 ,
    0x00d2 ,
    0x00d3 ,
    0x00d4 ,
    0x00d5 ,
    0x00d6 ,
    0x00d7 ,
    0x00d8 ,
    0xFFFF ,
    0xFFFF ,
    0x00e9 ,
    0xEAE9 ,
    0x00da ,
    0x00db ,
    0x00dc ,
    0x00dd ,
    0x00de ,
    0x00df ,
    0xDFE9 ,
    0x00e3 ,
    0x00e0 ,
    0x00e1 ,
    0x00e2 ,
    0x00e7 ,
    0x00e4 ,
    0x00e5 ,
    0x00e6 ,
    0x00e8 ,
    0x00ec ,
    0x00ed ,
    0xA1E9 , 
    0xFFFF ,
    0xF0B8 ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xb3e9 ,
    0xb4e9 ,
    0xb5e9 ,
    0xbae9 ,
    0xbfe9 ,
    0xC0E9 ,
    0xc9e9 ,
    0x00ce ,
    0xAAe9 ,
    0xA7E9 ,
    0xDBE9 ,
    0xDCE9 ,
    0x00ea ,
    0xeaea ,
    0x00f1 ,
    0x00f2 ,
    0x00f3 ,
    0x00f4 ,
    0x00f5 ,
    0x00f6 ,
    0x00f7 ,
    0x00f8 ,
    0x00f9 ,
    0x00fa ,
    0xF0BF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
    0xFFFF ,
};
static const uint16_t toUnicodeTable[256]={
    0x0000,
    0x0001,
    0x0002,
    0x0003,
    0x0004,
    0x0005,
    0x0006,
    0x0007,
    0x0008,
    0x0009,
    0x000a,
    0x000b,
    0x000c,
    0x000d,
    0x000e,
    0x000f,
    0x0010,
    0x0011,
    0x0012,
    0x0013,
    0x0014,
    0x0015,
    0x0016,
    0x0017,
    0x0018,
    0x0019,
    0x001a,
    0x001b,
    0x001c,
    0x001d,
    0x001e,
    0x001f,
    0x0020,
    0x0021,
    0x0022,
    0x0023,
    0x0024,
    0x0025,
    0x0026,
    0x0027,
    0x0028,
    0x0029,
    0x002a,
    0x002b,
    0x002c,
    0x002d,
    0x002e,
    0x002f,
    0x0030,
    0x0031,
    0x0032,
    0x0033,
    0x0034,
    0x0035,
    0x0036,
    0x0037,
    0x0038,
    0x0039,
    0x003A,
    0x003B,
    0x003c,
    0x003d,
    0x003e,
    0x003f,
    0x0040,
    0x0041,
    0x0042,
    0x0043,
    0x0044,
    0x0045,
    0x0046,
    0x0047,
    0x0048,
    0x0049,
    0x004a,
    0x004b,
    0x004c,
    0x004d,
    0x004e,
    0x004f,
    0x0050,
    0x0051,
    0x0052,
    0x0053,
    0x0054,
    0x0055,
    0x0056,
    0x0057,
    0x0058,
    0x0059,
    0x005a,
    0x005b,
    0x005c,
    0x005d,
    0x005e,
    0x005f,
    0x0060,
    0x0061,
    0x0062,
    0x0063,
    0x0064,
    0x0065,
    0x0066,
    0x0067,
    0x0068,
    0x0069,
    0x006a,
    0x006b,
    0x006c,
    0x006d,
    0x006e,
    0x006f,
    0x0070,
    0x0071,
    0x0072,
    0x0073,
    0x0074,
    0x0075,
    0x0076,
    0x0077,
    0x0078,
    0x0079,
    0x007a,
    0x007b,
    0x007c,
    0x007d,
    0x007e,
    0x007f,
    0x0080,
    0x0081,
    0x0082,
    0x0083,
    0x0084,
    0x0085,
    0x0086,
    0x0087,
    0x0088,
    0x0089,
    0x008a,
    0x008b,
    0x008c,
    0x008d,
    0x008e,
    0x008f,
    0x0090,
    0x0091,
    0x0092,
    0x0093,
    0x0094,
    0x0095,
    0x0096,
    0x0097,
    0x0098,
    0x0099,
    0x009a,
    0x009b,
    0x009c,
    0x009d,
    0x009e,
    0x009f,
    0x00A0,
    0x0901,
    0x0902,
    0x0903,
    0x0905,
    0x0906,
    0x0907,
    0x0908,
    0x0909,
    0x090a,
    0x090b,
    0x090e,
    0x090f,
    0x0910,
    0x090d,
    0x0912,
    0x0913,
    0x0914,
    0x0911,
    0x0915,
    0x0916,
    0x0917,
    0x0918,
    0x0919,
    0x091a,
    0x091b,
    0x091c,
    0x091d,
    0x091e,
    0x091f,
    0x0920,
    0x0921,
    0x0922,
    0x0923,
    0x0924,
    0x0925,
    0x0926,
    0x0927,
    0x0928,
    0x0929,
    0x092a,
    0x092b,
    0x092c,
    0x092d,
    0x092e,
    0x092f,
    0x095f,
    0x0930,
    0x0931,
    0x0932,
    0x0933,
    0x0934,
    0x0935,
    0x0936,
    0x0937,
    0x0938,
    0x0939,
    0x200D,
    0x093e,
    0x093f,
    0x0940,
    0x0941,
    0x0942,
    0x0943,
    0x0946,
    0x0947,
    0x0948,
    0x0945,
    0x094a,
    0x094b,
    0x094c,
    0x0949,
    0x094d,
    0x093c,
    0x0964,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0x0966,
    0x0967,
    0x0968,
    0x0969,
    0x096a,
    0x096b,
    0x096c,
    0x096d,
    0x096e,
    0x096f,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF,
    0xFFFF 
};

static const uint16_t vowelSignESpecialCases[][2]={
	{ 2     , 0      },
	{ 0xA4 , 0x0904 },
};

static const uint16_t nuktaSpecialCases[][2]={
    { 16    , 0      },
    { 0xA6 , 0x090c },
    { 0xEA , 0x093D },
    { 0xDF , 0x0944 },
    { 0xA1 , 0x0950 },
    { 0xb3 , 0x0958 },
    { 0xb4 , 0x0959 },
    { 0xb5 , 0x095a },
    { 0xba , 0x095b },
    { 0xbf , 0x095c },
    { 0xC0 , 0x095d },
    { 0xc9 , 0x095e },
    { 0xAA , 0x0960 },
    { 0xA7 , 0x0961 },
    { 0xDB , 0x0962 },
    { 0xDC , 0x0963 },
};


#define WRITE_TO_TARGET_FROM_U(args,offsets,source,target,targetLimit,targetByteUnit,err){      \
    int32_t offset = (int32_t)(source - args->source-1);                                        \
      /* write the targetUniChar  to target */                                                  \
    if(target < targetLimit){                                                                   \
        if(targetByteUnit <= 0xFF){                                                             \
            *(target)++ = (uint8_t)(targetByteUnit);                                            \
            if(offsets){                                                                        \
                *(offsets++) = offset;                                                          \
            }                                                                                   \
        }else{                                                                                  \
            if (targetByteUnit > 0xFFFF) {                                                      \
                *(target)++ = (uint8_t)(targetByteUnit>>16);                                    \
                if (offsets) {                                                                  \
                    --offset;                                                                   \
                    *(offsets++) = offset;                                                      \
                }                                                                               \
            }                                                                                   \
            if (!(target < targetLimit)) {                                                      \
                args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =    \
                                (uint8_t)(targetByteUnit >> 8);                                 \
                args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =    \
                                (uint8_t)targetByteUnit;                                        \
                *err = U_BUFFER_OVERFLOW_ERROR;                                                 \
            } else {                                                                            \
                *(target)++ = (uint8_t)(targetByteUnit>>8);                                     \
                if(offsets){                                                                    \
                    *(offsets++) = offset;                                                      \
                }                                                                               \
                if(target < targetLimit){                                                       \
                    *(target)++ = (uint8_t)  targetByteUnit;                                    \
                    if(offsets){                                                                \
                        *(offsets++) = offset                            ;                      \
                    }                                                                           \
                }else{                                                                          \
                    args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =\
                                (uint8_t) (targetByteUnit);                                     \
                    *err = U_BUFFER_OVERFLOW_ERROR;                                             \
                }                                                                               \
            }                                                                                   \
        }                                                                                       \
    }else{                                                                                      \
        if (targetByteUnit & 0xFF0000) {                                                        \
            args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =        \
                        (uint8_t) (targetByteUnit >>16);                                        \
        }                                                                                       \
        if(targetByteUnit & 0xFF00){                                                            \
            args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =        \
                        (uint8_t) (targetByteUnit >>8);                                         \
        }                                                                                       \
        args->converter->charErrorBuffer[args->converter->charErrorBufferLength++] =            \
                        (uint8_t) (targetByteUnit);                                             \
        *err = U_BUFFER_OVERFLOW_ERROR;                                                         \
    }                                                                                           \
}








static void UConverter_fromUnicode_ISCII_OFFSETS_LOGIC(
        UConverterFromUnicodeArgs * args, UErrorCode * err) {
    const UChar *source = args->source;
    const UChar *sourceLimit = args->sourceLimit;
    unsigned char *target = (unsigned char *) args->target;
    unsigned char *targetLimit = (unsigned char *) args->targetLimit;
    int32_t* offsets = args->offsets;
    uint32_t targetByteUnit = 0x0000;
    UChar32 sourceChar = 0x0000;
    UChar32 tempContextFromUnicode = 0x0000;    
    UConverterDataISCII *converterData;
    uint16_t newDelta=0;
    uint16_t range = 0;
    UBool deltaChanged = FALSE;

    if ((args->converter == NULL) || (args->targetLimit < args->target) || (args->sourceLimit < args->source)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }
    
    converterData=(UConverterDataISCII*)args->converter->extraInfo;
    newDelta=converterData->currentDeltaFromUnicode;
    range = (uint16_t)(newDelta/DELTA);

    if ((sourceChar = args->converter->fromUChar32)!=0) {
        goto getTrail;
    }

    
    while (source < sourceLimit) {
        
        if (args->converter->fromUnicodeStatus == LF) {
            targetByteUnit = ATR<<8;
            targetByteUnit += (uint8_t) lookupInitialData[range].isciiLang;
            args->converter->fromUnicodeStatus = 0x0000;
            
            WRITE_TO_TARGET_FROM_U(args,offsets,source,target,targetLimit,targetByteUnit,err);
            if (U_FAILURE(*err)) {
                break;
            }
        }
        
        sourceChar = *source++;
        tempContextFromUnicode = converterData->contextCharFromUnicode;
        
        targetByteUnit = missingCharMarker;
        
        
        if (sourceChar <= ASCII_END) {
            args->converter->fromUnicodeStatus = sourceChar;
            WRITE_TO_TARGET_FROM_U(args,offsets,source,target,targetLimit,sourceChar,err);
            if (U_FAILURE(*err)) {
                break;
            }
            continue;
        }
        switch (sourceChar) {
        case ZWNJ:
            
            if (converterData->contextCharFromUnicode) {
                converterData->contextCharFromUnicode = 0x00;
                targetByteUnit = ISCII_HALANT;
            } else {
                
                converterData->contextCharFromUnicode = 0x00;
                continue;
            }
            break;
        case ZWJ:
            
            if (converterData->contextCharFromUnicode) {
                targetByteUnit = ISCII_NUKTA;
            } else {
                targetByteUnit =ISCII_INV;
            }
            converterData->contextCharFromUnicode = 0x00;
            break;
        default:
            
            if ((uint16_t)(INDIC_BLOCK_END-sourceChar) <= INDIC_RANGE) {
                



                if (sourceChar!= DANDA && sourceChar != DOUBLE_DANDA) {
                    
                    range =(uint16_t)((sourceChar-INDIC_BLOCK_BEGIN)/DELTA);
                    newDelta =(uint16_t)(range*DELTA);

                    
                    if (newDelta!= converterData->currentDeltaFromUnicode || converterData->isFirstBuffer) {
                        converterData->currentDeltaFromUnicode = newDelta;
                        converterData->currentMaskFromUnicode = lookupInitialData[range].maskEnum;
                        deltaChanged =TRUE;
                        converterData->isFirstBuffer=FALSE;
                    }
                    
                    if (converterData->currentDeltaFromUnicode == PNJ_DELTA) { 
                        if (sourceChar == PNJ_TIPPI) {
                            
                            sourceChar = PNJ_BINDI;
                        } else if (sourceChar == PNJ_ADHAK) {
                            
                            converterData->contextCharFromUnicode = PNJ_ADHAK;
                        }
                        
                    }
                    
                    
                    sourceChar -= converterData->currentDeltaFromUnicode;
                }

                
                targetByteUnit=fromUnicodeTable[(uint8_t)sourceChar];

                
                if ((validityTable[(uint8_t)sourceChar] & converterData->currentMaskFromUnicode)==0) {
                    
                    if (converterData->currentDeltaFromUnicode!=(TELUGU_DELTA) || sourceChar!=VOCALLIC_RR) {
                        targetByteUnit=missingCharMarker;
                    }
                }

                if (deltaChanged) {
                    


                    uint32_t temp=0;
                    temp =(uint16_t)(ATR<<8);
                    temp += (uint16_t)((uint8_t) lookupInitialData[range].isciiLang);
                    
                    deltaChanged=FALSE;
                    
                    WRITE_TO_TARGET_FROM_U(args,offsets,source,target,targetLimit,temp,err);
                    if (U_FAILURE(*err)) {
                        break;
                    }
                }
                
                if (converterData->currentDeltaFromUnicode == PNJ_DELTA && (sourceChar + PNJ_DELTA) == PNJ_ADHAK) {
                    continue;
                }
            }
            
            converterData->contextCharFromUnicode = 0x00;
            break;
        }
        if (converterData->currentDeltaFromUnicode == PNJ_DELTA && tempContextFromUnicode == PNJ_ADHAK && isPNJConsonant((sourceChar + PNJ_DELTA))) {
            
            
            converterData->contextCharFromUnicode = 0x0000;
            targetByteUnit = targetByteUnit << 16 | ISCII_HALANT << 8 | targetByteUnit;
            
            WRITE_TO_TARGET_FROM_U(args, offsets, source, target, targetLimit, targetByteUnit,err);
            if (U_FAILURE(*err)) {
                break;
            }
        } else if (targetByteUnit != missingCharMarker) {
            if (targetByteUnit==ISCII_HALANT) {
                converterData->contextCharFromUnicode = (UChar)targetByteUnit;
            }
            
            WRITE_TO_TARGET_FROM_U(args,offsets,source,target,targetLimit,targetByteUnit,err);
            if (U_FAILURE(*err)) {
                break;
            }
        } else {
            
            
            if (U16_IS_SURROGATE(sourceChar)) {
                if (U16_IS_SURROGATE_LEAD(sourceChar)) {
getTrail:
                    
                    if (source < sourceLimit) {
                        
                        UChar trail= (*source);
                        if (U16_IS_TRAIL(trail)) {
                            source++;
                            sourceChar=U16_GET_SUPPLEMENTARY(sourceChar, trail);
                            *err =U_INVALID_CHAR_FOUND;
                            
                            
                        } else {
                            
                            
                            *err=U_ILLEGAL_CHAR_FOUND;
                        }
                    } else {
                        
                        *err = U_ZERO_ERROR;
                    }
                } else {
                    
                    
                    *err=U_ILLEGAL_CHAR_FOUND;
                }
            } else {
                
                *err = U_INVALID_CHAR_FOUND;
            }

            args->converter->fromUChar32=sourceChar;
            break;
        }
    }

    
    args->source = source;
    args->target = (char*)target;
}

static const uint16_t lookupTable[][2]={
    { ZERO,       ZERO     },     
    { ZERO,       ZERO     },     
    { DEVANAGARI, DEV_MASK },
    { BENGALI,    BNG_MASK },
    { TAMIL,      TML_MASK },
    { TELUGU,     KND_MASK },
    { BENGALI,    BNG_MASK },
    { ORIYA,      ORI_MASK },
    { KANNADA,    KND_MASK },
    { MALAYALAM,  MLM_MASK },
    { GUJARATI,   GJR_MASK },
    { GURMUKHI,   PNJ_MASK }
};

#define WRITE_TO_TARGET_TO_U(args,source,target,offsets,offset,targetUniChar,delta, err){\
    /* add offset to current Indic Block */                                              \
    if(targetUniChar>ASCII_END &&                                                        \
           targetUniChar != ZWJ &&                                                       \
           targetUniChar != ZWNJ &&                                                      \
           targetUniChar != DANDA &&                                                     \
           targetUniChar != DOUBLE_DANDA){                                               \
                                                                                         \
           targetUniChar+=(uint16_t)(delta);                                             \
    }                                                                                    \
    /* now write the targetUniChar */                                                    \
    if(target<args->targetLimit){                                                        \
        *(target)++ = (UChar)targetUniChar;                                              \
        if(offsets){                                                                     \
            *(offsets)++ = (int32_t)(offset);                                            \
        }                                                                                \
    }else{                                                                               \
        args->converter->UCharErrorBuffer[args->converter->UCharErrorBufferLength++] =   \
            (UChar)targetUniChar;                                                        \
        *err = U_BUFFER_OVERFLOW_ERROR;                                                  \
    }                                                                                    \
}

#define GET_MAPPING(sourceChar,targetUniChar,data){                                      \
    targetUniChar = toUnicodeTable[(sourceChar)] ;                                       \
    /* is the code point valid in current script? */                                     \
    if(sourceChar> ASCII_END &&                                                          \
            (validityTable[(targetUniChar & 0x7F)] & data->currentMaskToUnicode)==0){    \
        /* Vocallic RR is assigne in ISCII Telugu and Unicode */                         \
        if(data->currentDeltaToUnicode!=(TELUGU_DELTA) ||                                \
                    targetUniChar!=VOCALLIC_RR){                                         \
            targetUniChar=missingCharMarker;                                             \
        }                                                                                \
    }                                                                                    \
}






















static void UConverter_toUnicode_ISCII_OFFSETS_LOGIC(UConverterToUnicodeArgs *args, UErrorCode* err) {
    const char *source = ( char *) args->source;
    UChar *target = args->target;
    const char *sourceLimit = args->sourceLimit;
    const UChar* targetLimit = args->targetLimit;
    uint32_t targetUniChar = 0x0000;
    uint8_t sourceChar = 0x0000;
    UConverterDataISCII* data;
    UChar32* toUnicodeStatus=NULL;
    UChar32 tempTargetUniChar = 0x0000;
    UChar* contextCharToUnicode= NULL;
    UBool found;
    int i; 
    int offset = 0;

    if ((args->converter == NULL) || (target < args->target) || (source < args->source)) {
        *err = U_ILLEGAL_ARGUMENT_ERROR;
        return;
    }

    data = (UConverterDataISCII*)(args->converter->extraInfo);
    contextCharToUnicode = &data->contextCharToUnicode; 
    toUnicodeStatus = (UChar32*)&args->converter->toUnicodeStatus;

    while (U_SUCCESS(*err) && source<sourceLimit) {

        targetUniChar = missingCharMarker;

        if (target < targetLimit) {
            sourceChar = (unsigned char)*(source)++;

            
            if (*contextCharToUnicode==ATR) {

                



                
                if ((uint8_t)(PNJ-sourceChar)<=PNJ-DEV) {
                    data->currentDeltaToUnicode = (uint16_t)(lookupTable[sourceChar & 0x0F][0] * DELTA);
                    data->currentMaskToUnicode = (MaskEnum)lookupTable[sourceChar & 0x0F][1];
                } else if (sourceChar==DEF) {
                    
                    data->currentDeltaToUnicode = data->defDeltaToUnicode;
                    data->currentMaskToUnicode = data->defMaskToUnicode;
                } else {
                    if ((sourceChar >= 0x21 && sourceChar <= 0x3F)) {
                        
                    } else {
                        *err =U_ILLEGAL_CHAR_FOUND;
                        
                        *contextCharToUnicode=NO_CHAR_MARKER;
                        goto CALLBACK;
                    }
                }

                
                *contextCharToUnicode=NO_CHAR_MARKER;

                continue;

            } else if (*contextCharToUnicode==EXT) {
                
                if ((uint8_t) (EXT_RANGE_END - sourceChar) <= (EXT_RANGE_END - EXT_RANGE_BEGIN)) {
                    
                    if (sourceChar==0xBF || sourceChar == 0xB8) {
                        targetUniChar = (sourceChar==0xBF) ? DEV_ABBR_SIGN : DEV_ANUDATTA;
                        
                        
                        if (validityTable[(uint8_t)targetUniChar] & data->currentMaskToUnicode) {
                            *contextCharToUnicode= NO_CHAR_MARKER;

                            
                            if (data->prevToUnicodeStatus) {
                                WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -1),data->prevToUnicodeStatus,0,err);
                                data->prevToUnicodeStatus = 0x0000;
                            }
                            
                            WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -2),targetUniChar,data->currentDeltaToUnicode,err);

                            continue;
                        }
                    }
                    
                    targetUniChar = missingCharMarker;
                    *err= U_INVALID_CHAR_FOUND;
                } else {
                    
                    *contextCharToUnicode= NO_CHAR_MARKER;
                    *err = U_ILLEGAL_CHAR_FOUND;
                }
                goto CALLBACK;
            } else if (*contextCharToUnicode==ISCII_INV) {
                if (sourceChar==ISCII_HALANT) {
                    targetUniChar = 0x0020; 
                } else {
                    targetUniChar = ZWJ;
                }

                
                if (data->prevToUnicodeStatus) {
                    WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -1),data->prevToUnicodeStatus,0,err);
                    data->prevToUnicodeStatus = 0x0000;
                }
                
                WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -2),targetUniChar,data->currentDeltaToUnicode,err);
                
                *contextCharToUnicode=NO_CHAR_MARKER;
            }

            
            switch (sourceChar) {
            case ISCII_INV:
            case EXT: 
            case ATR:
                *contextCharToUnicode = (UChar)sourceChar;

                if (*toUnicodeStatus != missingCharMarker) {
                    
                    if (data->prevToUnicodeStatus) {
                        WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -1),data->prevToUnicodeStatus,0,err);
                        data->prevToUnicodeStatus = 0x0000;
                    }
                    WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -2),*toUnicodeStatus,data->currentDeltaToUnicode,err);
                    *toUnicodeStatus = missingCharMarker;
                }
                continue;
            case ISCII_DANDA:
                
                if (*contextCharToUnicode== ISCII_DANDA) {
                    targetUniChar = DOUBLE_DANDA;
                    
                    *contextCharToUnicode = NO_CHAR_MARKER;
                    *toUnicodeStatus = missingCharMarker;
                } else {
                    GET_MAPPING(sourceChar,targetUniChar,data);
                    *contextCharToUnicode = sourceChar;
                }
                break;
            case ISCII_HALANT:
                
                if (*contextCharToUnicode == ISCII_HALANT) {
                    targetUniChar = ZWNJ;
                    
                    *contextCharToUnicode = NO_CHAR_MARKER;
                } else {
                    GET_MAPPING(sourceChar,targetUniChar,data);
                    *contextCharToUnicode = sourceChar;
                }
                break;
            case 0x0A:
                
            case 0x0D:
                data->resetToDefaultToUnicode = TRUE;
                GET_MAPPING(sourceChar,targetUniChar,data)
                ;
                *contextCharToUnicode = sourceChar;
                break;

            case ISCII_VOWEL_SIGN_E:
                i=1;
                found=FALSE;
                for (; i<vowelSignESpecialCases[0][0]; i++) {
                    U_ASSERT(i<sizeof(vowelSignESpecialCases)/sizeof(vowelSignESpecialCases[0]));
                    if (vowelSignESpecialCases[i][0]==(uint8_t)*contextCharToUnicode) {
                        targetUniChar=vowelSignESpecialCases[i][1];
                        found=TRUE;
                        break;
                    }
                }
                if (found) {
                    
                    if (validityTable[(uint8_t)targetUniChar] & data->currentMaskToUnicode) {
                        
                        *contextCharToUnicode= NO_CHAR_MARKER;
                        *toUnicodeStatus = missingCharMarker;
                        break;
                    }
                }
                GET_MAPPING(sourceChar,targetUniChar,data);
                *contextCharToUnicode = sourceChar;
                break;

            case ISCII_NUKTA:
                
                if (*contextCharToUnicode == ISCII_HALANT) {
                    targetUniChar = ZWJ;
                    
                    *contextCharToUnicode = NO_CHAR_MARKER;
                    break;
                } else if (data->currentDeltaToUnicode == PNJ_DELTA && data->contextCharToUnicode == 0xc0) {
                    
                    if (data->prevToUnicodeStatus) {
                        WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -1),data->prevToUnicodeStatus,0,err);
                        data->prevToUnicodeStatus = 0x0000;
                    }
                    


                    targetUniChar = PNJ_RRA;
                    WRITE_TO_TARGET_TO_U(args, source, target, args->offsets, (source-args->source)-2, targetUniChar, 0, err);
                    if (U_SUCCESS(*err)) {
                        targetUniChar = PNJ_SIGN_VIRAMA;
                        WRITE_TO_TARGET_TO_U(args, source, target, args->offsets, (source-args->source)-2, targetUniChar, 0, err);
                        if (U_SUCCESS(*err)) {
                            targetUniChar = PNJ_HA;
                            WRITE_TO_TARGET_TO_U(args, source, target, args->offsets, (source-args->source)-2, targetUniChar, 0, err);
                        } else {
                            args->converter->UCharErrorBuffer[args->converter->UCharErrorBufferLength++]= PNJ_HA;
                        }
                    } else {
                        args->converter->UCharErrorBuffer[args->converter->UCharErrorBufferLength++]= PNJ_SIGN_VIRAMA;
                        args->converter->UCharErrorBuffer[args->converter->UCharErrorBufferLength++]= PNJ_HA;
                    }
                    *toUnicodeStatus = missingCharMarker;
                    data->contextCharToUnicode = NO_CHAR_MARKER;
                    continue;
                } else {
                    
                    i=1;
                    found =FALSE;
                    for (; i<nuktaSpecialCases[0][0]; i++) {
                        if (nuktaSpecialCases[i][0]==(uint8_t)
                                *contextCharToUnicode) {
                            targetUniChar=nuktaSpecialCases[i][1];
                            found =TRUE;
                            break;
                        }
                    }
                    if (found) {
                        
                        if (validityTable[(uint8_t)targetUniChar] & data->currentMaskToUnicode) {
                            
                            *contextCharToUnicode= NO_CHAR_MARKER;
                            *toUnicodeStatus = missingCharMarker;
                            if (data->currentDeltaToUnicode == PNJ_DELTA) {
                                
                                if (data->prevToUnicodeStatus) {
                                    WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -1),data->prevToUnicodeStatus,0,err);
                                    data->prevToUnicodeStatus = 0x0000;
                                }
                                WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -2),targetUniChar,data->currentDeltaToUnicode,err);
                                continue;
                            }
                            break;
                        }
                        
                    }
                    
                }
            default:GET_MAPPING(sourceChar,targetUniChar,data)
                ;
                *contextCharToUnicode = sourceChar;
                break;
            }

            if (*toUnicodeStatus != missingCharMarker) {
                
                if (data->currentDeltaToUnicode == PNJ_DELTA && data->prevToUnicodeStatus != 0 && isPNJConsonant(data->prevToUnicodeStatus) &&
                        (*toUnicodeStatus + PNJ_DELTA) == PNJ_SIGN_VIRAMA && (targetUniChar + PNJ_DELTA) == data->prevToUnicodeStatus) {
                    
                    offset = (int)(source-args->source - 3);
                    tempTargetUniChar = PNJ_ADHAK; 
                    WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,offset,tempTargetUniChar,0,err);
                    WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,offset,data->prevToUnicodeStatus,0,err);
                    data->prevToUnicodeStatus = 0x0000; 
                    *toUnicodeStatus = missingCharMarker;
                    continue;
                } else {
                    
                    if (data->prevToUnicodeStatus) {
                        WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -1),data->prevToUnicodeStatus,0,err);
                        data->prevToUnicodeStatus = 0x0000;
                    }
                    


                    if (data->currentDeltaToUnicode == PNJ_DELTA && (targetUniChar + PNJ_DELTA) == PNJ_BINDI && isPNJBindiTippi((*toUnicodeStatus + PNJ_DELTA))) {
                        targetUniChar = PNJ_TIPPI - PNJ_DELTA;
                        WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -2),*toUnicodeStatus,PNJ_DELTA,err);
                    } else if (data->currentDeltaToUnicode == PNJ_DELTA && (targetUniChar + PNJ_DELTA) == PNJ_SIGN_VIRAMA && isPNJConsonant((*toUnicodeStatus + PNJ_DELTA))) {
                        
                        data->prevToUnicodeStatus = *toUnicodeStatus + PNJ_DELTA;
                    } else {
                        
                        WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source-args->source -2),*toUnicodeStatus,data->currentDeltaToUnicode,err);
                    }
                }
                *toUnicodeStatus = missingCharMarker;
            }

            if (targetUniChar != missingCharMarker) {
                
                *toUnicodeStatus = (UChar) targetUniChar;
                if (data->resetToDefaultToUnicode==TRUE) {
                    data->currentDeltaToUnicode = data->defDeltaToUnicode;
                    data->currentMaskToUnicode = data->defMaskToUnicode;
                    data->resetToDefaultToUnicode=FALSE;
                }
            } else {

                


                *err = U_INVALID_CHAR_FOUND;
CALLBACK:
                args->converter->toUBytes[0] = (uint8_t) sourceChar;
                args->converter->toULength = 1;
                break;
            }

        } else {
            *err =U_BUFFER_OVERFLOW_ERROR;
            break;
        }
    }

    if (U_SUCCESS(*err) && args->flush && source == sourceLimit) {
        
        UConverter *cnv = args->converter;

        if (*contextCharToUnicode==ATR || *contextCharToUnicode==EXT || *contextCharToUnicode==ISCII_INV) {
            
            cnv->toUBytes[0] = (uint8_t)*contextCharToUnicode;
            cnv->toULength = 1;

            
            *contextCharToUnicode = NO_CHAR_MARKER;
        } else {
            cnv->toULength = 0;
        }

        if (*toUnicodeStatus != missingCharMarker) {
            
            WRITE_TO_TARGET_TO_U(args,source,target,args->offsets,(source - args->source -1),*toUnicodeStatus,data->currentDeltaToUnicode,err);
            *toUnicodeStatus = missingCharMarker;
        }
    }

    args->target = target;
    args->source = source;
}


struct cloneISCIIStruct {
    UConverter cnv;
    UConverterDataISCII mydata;
};

static UConverter *
_ISCII_SafeClone(const UConverter *cnv,
              void *stackBuffer,
              int32_t *pBufferSize,
              UErrorCode *status)
{
    struct cloneISCIIStruct * localClone;
    int32_t bufferSizeNeeded = sizeof(struct cloneISCIIStruct);

    if (U_FAILURE(*status)) {
        return 0;
    }

    if (*pBufferSize == 0) { 
        *pBufferSize = bufferSizeNeeded;
        return 0;
    }

    localClone = (struct cloneISCIIStruct *)stackBuffer;
    

    uprv_memcpy(&localClone->mydata, cnv->extraInfo, sizeof(UConverterDataISCII));
    localClone->cnv.extraInfo = &localClone->mydata;
    localClone->cnv.isExtraLocal = TRUE;

    return &localClone->cnv;
}

static void
_ISCIIGetUnicodeSet(const UConverter *cnv,
                    const USetAdder *sa,
                    UConverterUnicodeSet which,
                    UErrorCode *pErrorCode)
{
    int32_t idx, script;
    uint8_t mask;

    

    sa->addRange(sa->set, 0, ASCII_END);
    for (script = DEVANAGARI; script <= MALAYALAM; script++) {
        mask = (uint8_t)(lookupInitialData[script].maskEnum);
        for (idx = 0; idx < DELTA; idx++) {
            
            if ((validityTable[idx] & mask) || (script==TELUGU && idx==0x31)) {
                sa->add(sa->set, idx + (script * DELTA) + INDIC_BLOCK_BEGIN);
            }
        }
    }
    sa->add(sa->set, DANDA);
    sa->add(sa->set, DOUBLE_DANDA);
    sa->add(sa->set, ZWNJ);
    sa->add(sa->set, ZWJ);
}

static const UConverterImpl _ISCIIImpl={

    UCNV_ISCII,

    NULL,
    NULL,

    _ISCIIOpen,
    _ISCIIClose,
    _ISCIIReset,

    UConverter_toUnicode_ISCII_OFFSETS_LOGIC,
    UConverter_toUnicode_ISCII_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISCII_OFFSETS_LOGIC,
    UConverter_fromUnicode_ISCII_OFFSETS_LOGIC,
    NULL,

    NULL,
    _ISCIIgetName,
    NULL,
    _ISCII_SafeClone,
    _ISCIIGetUnicodeSet
};

static const UConverterStaticData _ISCIIStaticData={
    sizeof(UConverterStaticData),
        "ISCII",
         0,
         UCNV_IBM,
         UCNV_ISCII,
         1,
         4,
        { 0x1a, 0, 0, 0 },
        0x1,
        FALSE,
        FALSE,
        0x0,
        0x0,
        { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 }, 

};

const UConverterSharedData _ISCIIData={
    sizeof(UConverterSharedData),
        ~((uint32_t) 0),
        NULL,
        NULL,
        &_ISCIIStaticData,
        FALSE,
        &_ISCIIImpl,
        0
};

#endif 
