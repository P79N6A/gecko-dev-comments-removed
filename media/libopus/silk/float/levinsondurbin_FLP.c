






























#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "SigProc_FLP.h"


silk_float silk_levinsondurbin_FLP(         
    silk_float          A[],                
    const silk_float    corr[],             
    const opus_int      order               
)
{
    opus_int   i, mHalf, m;
    silk_float min_nrg, nrg, t, km, Atmp1, Atmp2;

    min_nrg = 1e-12f * corr[ 0 ] + 1e-9f;
    nrg = corr[ 0 ];
    nrg = silk_max_float(min_nrg, nrg);
    A[ 0 ] = corr[ 1 ] / nrg;
    nrg -= A[ 0 ] * corr[ 1 ];
    nrg = silk_max_float(min_nrg, nrg);

    for( m = 1; m < order; m++ )
    {
        t = corr[ m + 1 ];
        for( i = 0; i < m; i++ ) {
            t -= A[ i ] * corr[ m - i ];
        }

        
        km = t / nrg;

        
        nrg -= km * t;
        nrg = silk_max_float(min_nrg, nrg);

        mHalf = m >> 1;
        for( i = 0; i < mHalf; i++ ) {
            Atmp1 = A[ i ];
            Atmp2 = A[ m - i - 1 ];
            A[ m - i - 1 ] -= km * Atmp1;
            A[ i ]         -= km * Atmp2;
        }
        if( m & 1 ) {
            A[ mHalf ]     -= km * A[ mHalf ];
        }
        A[ m ] = km;
    }

    
    return nrg;
}

