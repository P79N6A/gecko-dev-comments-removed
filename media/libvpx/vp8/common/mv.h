










#ifndef VP8_COMMON_MV_H_
#define VP8_COMMON_MV_H_
#include "vpx/vpx_integer.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    short row;
    short col;
} MV;

typedef union int_mv
{
    uint32_t  as_int;
    MV        as_mv;
} int_mv;        

#ifdef __cplusplus
}  
#endif

#endif
