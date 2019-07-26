






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "tables.h"

#ifdef __cplusplus
extern "C"
{
#endif

const opus_uint8 silk_gain_iCDF[ 3 ][ N_LEVELS_QGAIN / 8 ] =
{
{
       224,    112,     44,     15,      3,      2,      1,      0
},
{
       254,    237,    192,    132,     70,     23,      4,      0
},
{
       255,    252,    226,    155,     61,     11,      2,      0
}
};

const opus_uint8 silk_delta_gain_iCDF[ MAX_DELTA_GAIN_QUANT - MIN_DELTA_GAIN_QUANT + 1 ] = {
       250,    245,    234,    203,     71,     50,     42,     38,
        35,     33,     31,     29,     28,     27,     26,     25,
        24,     23,     22,     21,     20,     19,     18,     17,
        16,     15,     14,     13,     12,     11,     10,      9,
         8,      7,      6,      5,      4,      3,      2,      1,
         0
};

#ifdef __cplusplus
}
#endif
