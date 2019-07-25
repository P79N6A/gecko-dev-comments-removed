









#ifndef VP8_DEC_EC_TYPES_H
#define VP8_DEC_EC_TYPES_H

#define MAX_OVERLAPS 16






typedef struct
{
    int overlap;
    union b_mode_info *bmi;
} OVERLAP_NODE;


typedef struct
{
    
    OVERLAP_NODE overlaps[MAX_OVERLAPS];
} B_OVERLAP;




typedef struct
{
    B_OVERLAP overlaps[16];
} MB_OVERLAP;




typedef struct
{
    MV mv;
    MV_REFERENCE_FRAME ref_frame;
} EC_BLOCK;

#endif 
