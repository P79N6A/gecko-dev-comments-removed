






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


void silk_bwexpander_FLP(
    silk_float          *ar,                
    const opus_int      d,                  
    const silk_float    chirp               
)
{
    opus_int   i;
    silk_float cfac = chirp;

    for( i = 0; i < d - 1; i++ ) {
        ar[ i ] *=  cfac;
        cfac    *=  chirp;
    }
    ar[ d - 1 ] *=  cfac;
}
