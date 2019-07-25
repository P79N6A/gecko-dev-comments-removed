










#ifndef __INC_MV_H
#define __INC_MV_H
#include "vpx/vpx_integer.h"

typedef struct
{
    short row;
    short col;
} MV;

typedef union
{
    uint32_t  as_int;
    MV        as_mv;
} int_mv;        

#endif
