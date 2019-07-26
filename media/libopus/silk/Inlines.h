


































#ifndef SILK_FIX_INLINES_H
#define SILK_FIX_INLINES_H

#ifdef  __cplusplus
extern "C"
{
#endif


static inline opus_int32 silk_CLZ64( opus_int64 in )
{
    opus_int32 in_upper;

    in_upper = (opus_int32)silk_RSHIFT64(in, 32);
    if (in_upper == 0) {
        
        return 32 + silk_CLZ32( (opus_int32) in );
    } else {
        
        return silk_CLZ32( in_upper );
    }
}


static inline void silk_CLZ_FRAC(
    opus_int32 in,            
    opus_int32 *lz,           
    opus_int32 *frac_Q7       
)
{
    opus_int32 lzeros = silk_CLZ32(in);

    * lz = lzeros;
    * frac_Q7 = silk_ROR32(in, 24 - lzeros) & 0x7f;
}




static inline opus_int32 silk_SQRT_APPROX( opus_int32 x )
{
    opus_int32 y, lz, frac_Q7;

    if( x <= 0 ) {
        return 0;
    }

    silk_CLZ_FRAC(x, &lz, &frac_Q7);

    if( lz & 1 ) {
        y = 32768;
    } else {
        y = 46214;        
    }

    
    y >>= silk_RSHIFT(lz, 1);

    
    y = silk_SMLAWB(y, y, silk_SMULBB(213, frac_Q7));

    return y;
}


static inline opus_int32 silk_DIV32_varQ(   
    const opus_int32     a32,               
    const opus_int32     b32,               
    const opus_int       Qres               
)
{
    opus_int   a_headrm, b_headrm, lshift;
    opus_int32 b32_inv, a32_nrm, b32_nrm, result;

    silk_assert( b32 != 0 );
    silk_assert( Qres >= 0 );

    
    a_headrm = silk_CLZ32( silk_abs(a32) ) - 1;
    a32_nrm = silk_LSHIFT(a32, a_headrm);                                       
    b_headrm = silk_CLZ32( silk_abs(b32) ) - 1;
    b32_nrm = silk_LSHIFT(b32, b_headrm);                                       

    
    b32_inv = silk_DIV32_16( silk_int32_MAX >> 2, silk_RSHIFT(b32_nrm, 16) );   

    
    result = silk_SMULWB(a32_nrm, b32_inv);                                     

    
    
    a32_nrm = silk_SUB32_ovflw(a32_nrm, silk_LSHIFT_ovflw( silk_SMMUL(b32_nrm, result), 3 ));  

    
    result = silk_SMLAWB(result, a32_nrm, b32_inv);                             

    
    lshift = 29 + a_headrm - b_headrm - Qres;
    if( lshift < 0 ) {
        return silk_LSHIFT_SAT32(result, -lshift);
    } else {
        if( lshift < 32){
            return silk_RSHIFT(result, lshift);
        } else {
            
            return 0;
        }
    }
}


static inline opus_int32 silk_INVERSE32_varQ(   
    const opus_int32     b32,                   
    const opus_int       Qres                   
)
{
    opus_int   b_headrm, lshift;
    opus_int32 b32_inv, b32_nrm, err_Q32, result;

    silk_assert( b32 != 0 );
    silk_assert( Qres > 0 );

    
    b_headrm = silk_CLZ32( silk_abs(b32) ) - 1;
    b32_nrm = silk_LSHIFT(b32, b_headrm);                                       

    
    b32_inv = silk_DIV32_16( silk_int32_MAX >> 2, silk_RSHIFT(b32_nrm, 16) );   

    
    result = silk_LSHIFT(b32_inv, 16);                                          

    
    err_Q32 = silk_LSHIFT( (1<<29) - silk_SMULWB(b32_nrm, b32_inv), 3 );        

    
    result = silk_SMLAWW(result, err_Q32, b32_inv);                             

    
    lshift = 61 - b_headrm - Qres;
    if( lshift <= 0 ) {
        return silk_LSHIFT_SAT32(result, -lshift);
    } else {
        if( lshift < 32){
            return silk_RSHIFT(result, lshift);
        }else{
            
            return 0;
        }
    }
}

#ifdef  __cplusplus
}
#endif

#endif
