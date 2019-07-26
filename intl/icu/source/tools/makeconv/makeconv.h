















#ifndef __MAKECONV_H__
#define __MAKECONV_H__

#include "unicode/utypes.h"
#include "ucnv_bld.h"
#include "unewdata.h"
#include "ucm.h"


U_CFUNC UBool VERBOSE;
U_CFUNC UBool SMALL;
U_CFUNC UBool IGNORE_SISO_CHECK;


enum {
    TABLE_NONE,
    TABLE_BASE,
    TABLE_EXT,
    TABLE_BASE_AND_EXT
};


struct NewConverter;
typedef struct NewConverter NewConverter;

struct NewConverter {
    void
    (*close)(NewConverter *cnvData);

    
    UBool
    (*isValid)(NewConverter *cnvData,
               const uint8_t *bytes, int32_t length);

    UBool
    (*addTable)(NewConverter *cnvData, UCMTable *table, UConverterStaticData *staticData);

    uint32_t
    (*write)(NewConverter *cnvData, const UConverterStaticData *staticData,
             UNewDataMemory *pData, int32_t tableType);
};

#endif 
