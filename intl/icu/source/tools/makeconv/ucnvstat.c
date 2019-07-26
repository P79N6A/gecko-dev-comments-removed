












#include "unicode/utypes.h"
#include "unicode/ucnv.h"
#include "ucnv_bld.h"


static const UConverterStaticData _SBCSStaticData={
    sizeof(UConverterStaticData),
    "SBCS",
    0, UCNV_IBM, UCNV_SBCS, 1, 1,
    { 0x1a, 0, 0, 0 }, 1, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


static const UConverterStaticData _DBCSStaticData={
    sizeof(UConverterStaticData),
    "DBCS",
    0, UCNV_IBM, UCNV_DBCS, 2, 2,
    { 0, 0, 0, 0 },0, FALSE, FALSE, 
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

static const UConverterStaticData _MBCSStaticData={
    sizeof(UConverterStaticData),
    "MBCS",
    0, UCNV_IBM, UCNV_MBCS, 1, 1,
    { 0x1a, 0, 0, 0 }, 1, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};

static const UConverterStaticData _EBCDICStatefulStaticData={
    sizeof(UConverterStaticData),
    "EBCDICStateful",
    0, UCNV_IBM, UCNV_EBCDIC_STATEFUL, 1, 1,
    { 0, 0, 0, 0 },0, FALSE, FALSE,
    0,
    0,
    { 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0 } 
};


const UConverterStaticData *ucnv_converterStaticData[UCNV_NUMBER_OF_SUPPORTED_CONVERTER_TYPES]={
    &_SBCSStaticData, &_DBCSStaticData, &_MBCSStaticData, NULL,
    NULL, NULL, NULL, NULL, NULL, &_EBCDICStatefulStaticData,
    NULL,
     NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

