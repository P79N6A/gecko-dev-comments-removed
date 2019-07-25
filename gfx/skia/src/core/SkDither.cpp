






#include "SkDither.h"




















#ifdef ENABLE_DITHER_MATRIX_4X4

const uint8_t gDitherMatrix_4Bit_4X4[4][4] = {
    {  0,  8,  2, 10 },
    { 12,  4, 14,  6 },
    {  3, 11,  1,  9 },
    { 15,  7, 13,  5 }
};

const uint8_t gDitherMatrix_3Bit_4X4[4][4] = {
    {  0,  4,  1,  5 },
    {  6,  2,  7,  3 },
    {  1,  5,  0,  4 },
    {  7,  3,  6,  2 }
};

#else   

const uint16_t gDitherMatrix_4Bit_16[4] = {
    0xA280, 0x6E4C, 0x91B3, 0x5D7F
};

const uint16_t gDitherMatrix_3Bit_16[4] = {
    0x5140, 0x3726, 0x4051, 0x2637
};

#endif

