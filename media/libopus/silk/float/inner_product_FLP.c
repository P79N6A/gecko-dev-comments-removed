






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


double silk_inner_product_FLP(
    const silk_float    *data1,
    const silk_float    *data2,
    opus_int            dataSize
)
{
    opus_int  i, dataSize4;
    double   result;

    
    result = 0.0;
    dataSize4 = dataSize & 0xFFFC;
    for( i = 0; i < dataSize4; i += 4 ) {
        result += data1[ i + 0 ] * (double)data2[ i + 0 ] +
                  data1[ i + 1 ] * (double)data2[ i + 1 ] +
                  data1[ i + 2 ] * (double)data2[ i + 2 ] +
                  data1[ i + 3 ] * (double)data2[ i + 3 ];
    }

    
    for( ; i < dataSize; i++ ) {
        result += data1[ i ] * (double)data2[ i ];
    }

    return result;
}
