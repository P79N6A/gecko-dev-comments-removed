















#ifndef __GENMBCS_H__
#define __GENMBCS_H__

#include "makeconv.h"

enum {
    







    MBCS_STAGE_2_SHIFT=4,
    MBCS_STAGE_2_BLOCK_SIZE=0x40,       
    MBCS_STAGE_2_BLOCK_SIZE_SHIFT=6,    
    MBCS_STAGE_2_BLOCK_MASK=0x3f,       
    MBCS_STAGE_1_SHIFT=10,
    MBCS_STAGE_1_BMP_SIZE=0x40, 
    MBCS_STAGE_1_SIZE=0x440,    
    MBCS_STAGE_2_SIZE=0xfbc0,   
    MBCS_MAX_STAGE_2_TOP=MBCS_STAGE_2_SIZE,
    MBCS_STAGE_2_MAX_BLOCKS=MBCS_STAGE_2_SIZE>>MBCS_STAGE_2_BLOCK_SIZE_SHIFT,

    MBCS_STAGE_2_ALL_UNASSIGNED_INDEX=0, 
    MBCS_STAGE_2_FIRST_ASSIGNED=MBCS_STAGE_2_BLOCK_SIZE, 

    MBCS_STAGE_3_BLOCK_SIZE=16,         
    MBCS_STAGE_3_BLOCK_MASK=0xf,
    MBCS_STAGE_3_FIRST_ASSIGNED=MBCS_STAGE_3_BLOCK_SIZE, 

    MBCS_STAGE_3_GRANULARITY=16,        
    MBCS_STAGE_3_SBCS_SIZE=0x10000,     
    MBCS_STAGE_3_MBCS_SIZE=0x10000*MBCS_STAGE_3_GRANULARITY, 

    















    SBCS_UTF8_MAX=0x1fff,

    










    MBCS_UTF8_MAX=0xd7ff,
    MBCS_UTF8_LIMIT=MBCS_UTF8_MAX+1,    

    MBCS_UTF8_STAGE_SHIFT=6,
    MBCS_UTF8_STAGE_3_BLOCK_SIZE=0x40,  
    MBCS_UTF8_STAGE_3_BLOCK_MASK=0x3f,

    
    MBCS_UTF8_STAGE_SIZE=MBCS_UTF8_LIMIT>>MBCS_UTF8_STAGE_SHIFT, 

    MBCS_FROM_U_EXT_FLAG=0x10,          
    MBCS_FROM_U_EXT_MASK=0x0f,          

    
    MBCS_UTF8_STAGE_3_BLOCKS=MBCS_UTF8_STAGE_3_BLOCK_SIZE/MBCS_STAGE_3_BLOCK_SIZE,

    MBCS_MAX_FALLBACK_COUNT=8192
};

U_CFUNC NewConverter *
MBCSOpen(UCMFile *ucm);

struct MBCSData;
typedef struct MBCSData MBCSData;






U_CFUNC const MBCSData *
MBCSGetDummy(void);


U_CFUNC UBool
MBCSOkForBaseFromUnicode(const MBCSData *mbcsData,
                         const uint8_t *bytes, int32_t length,
                         UChar32 c, int8_t flag);

U_CFUNC NewConverter *
CnvExtOpen(UCMFile *ucm);

#endif 
