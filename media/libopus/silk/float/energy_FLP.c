






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


double silk_energy_FLP(
    const silk_float    *data,
    opus_int            dataSize
)
{
    opus_int  i, dataSize4;
    double   result;

    
    result = 0.0;
    dataSize4 = dataSize & 0xFFFC;
    for( i = 0; i < dataSize4; i += 4 ) {
        result += data[ i + 0 ] * (double)data[ i + 0 ] +
                  data[ i + 1 ] * (double)data[ i + 1 ] +
                  data[ i + 2 ] * (double)data[ i + 2 ] +
                  data[ i + 3 ] * (double)data[ i + 3 ];
    }

    
    for( ; i < dataSize; i++ ) {
        result += data[ i ] * (double)data[ i ];
    }

    silk_assert( result >= 0.0 );
    return result;
}
