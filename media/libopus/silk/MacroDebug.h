






























#ifndef MACRO_DEBUG_H
#define MACRO_DEBUG_H




#if 0 && defined (_DEBUG) && !defined (silk_MACRO_COUNT)

#undef silk_ADD16
static inline opus_int16 silk_ADD16(opus_int16 a, opus_int16 b){
    opus_int16 ret;

    ret = a + b;
    silk_assert( ret == silk_ADD_SAT16( a, b ));
    return ret;
}

#undef silk_ADD32
static inline opus_int32 silk_ADD32(opus_int32 a, opus_int32 b){
    opus_int32 ret;

    ret = a + b;
    silk_assert( ret == silk_ADD_SAT32( a, b ));
    return ret;
}

#undef silk_ADD64
static inline opus_int64 silk_ADD64(opus_int64 a, opus_int64 b){
    opus_int64 ret;

    ret = a + b;
    silk_assert( ret == silk_ADD_SAT64( a, b ));
    return ret;
}

#undef silk_SUB16
static inline opus_int16 silk_SUB16(opus_int16 a, opus_int16 b){
    opus_int16 ret;

    ret = a - b;
    silk_assert( ret == silk_SUB_SAT16( a, b ));
    return ret;
}

#undef silk_SUB32
static inline opus_int32 silk_SUB32(opus_int32 a, opus_int32 b){
    opus_int32 ret;

    ret = a - b;
    silk_assert( ret == silk_SUB_SAT32( a, b ));
    return ret;
}

#undef silk_SUB64
static inline opus_int64 silk_SUB64(opus_int64 a, opus_int64 b){
    opus_int64 ret;

    ret = a - b;
    silk_assert( ret == silk_SUB_SAT64( a, b ));
    return ret;
}

#undef silk_ADD_SAT16
static inline opus_int16 silk_ADD_SAT16( opus_int16 a16, opus_int16 b16 ) {
    opus_int16 res;
    res = (opus_int16)silk_SAT16( silk_ADD32( (opus_int32)(a16), (b16) ) );
    silk_assert( res == silk_SAT16( (opus_int32)a16 + (opus_int32)b16 ) );
    return res;
}

#undef silk_ADD_SAT32
static inline opus_int32 silk_ADD_SAT32(opus_int32 a32, opus_int32 b32){
    opus_int32 res;
    res =    ((((a32) + (b32)) & 0x80000000) == 0 ?                                   \
            ((((a32) & (b32)) & 0x80000000) != 0 ? silk_int32_MIN : (a32)+(b32)) :    \
            ((((a32) | (b32)) & 0x80000000) == 0 ? silk_int32_MAX : (a32)+(b32)) );
    silk_assert( res == silk_SAT32( (opus_int64)a32 + (opus_int64)b32 ) );
    return res;
}

#undef silk_ADD_SAT64
static inline opus_int64 silk_ADD_SAT64( opus_int64 a64, opus_int64 b64 ) {
    opus_int64 res;
    res =    ((((a64) + (b64)) & 0x8000000000000000LL) == 0 ?                                   \
            ((((a64) & (b64)) & 0x8000000000000000LL) != 0 ? silk_int64_MIN : (a64)+(b64)) :    \
            ((((a64) | (b64)) & 0x8000000000000000LL) == 0 ? silk_int64_MAX : (a64)+(b64)) );
    if( res != a64 + b64 ) {
        
        silk_assert( ( res == silk_int64_MAX && ( ( a64 >> 1 ) + ( b64 >> 1 ) > ( silk_int64_MAX >> 3 ) ) ) ||
                    ( res == silk_int64_MIN && ( ( a64 >> 1 ) + ( b64 >> 1 ) < ( silk_int64_MIN >> 3 ) ) ) );
    } else {
        
        silk_assert( res == a64 + b64 );
    }
    return res;
}

#undef silk_SUB_SAT16
static inline opus_int16 silk_SUB_SAT16( opus_int16 a16, opus_int16 b16 ) {
    opus_int16 res;
    res = (opus_int16)silk_SAT16( silk_SUB32( (opus_int32)(a16), (b16) ) );
    silk_assert( res == silk_SAT16( (opus_int32)a16 - (opus_int32)b16 ) );
    return res;
}

#undef silk_SUB_SAT32
static inline opus_int32 silk_SUB_SAT32( opus_int32 a32, opus_int32 b32 ) {
    opus_int32 res;
    res =     ((((a32)-(b32)) & 0x80000000) == 0 ?                                           \
            (( (a32) & ((b32)^0x80000000) & 0x80000000) ? silk_int32_MIN : (a32)-(b32)) :    \
            ((((a32)^0x80000000) & (b32)  & 0x80000000) ? silk_int32_MAX : (a32)-(b32)) );
    silk_assert( res == silk_SAT32( (opus_int64)a32 - (opus_int64)b32 ) );
    return res;
}

#undef silk_SUB_SAT64
static inline opus_int64 silk_SUB_SAT64( opus_int64 a64, opus_int64 b64 ) {
    opus_int64 res;
    res =    ((((a64)-(b64)) & 0x8000000000000000LL) == 0 ?                                                      \
            (( (a64) & ((b64)^0x8000000000000000LL) & 0x8000000000000000LL) ? silk_int64_MIN : (a64)-(b64)) :    \
            ((((a64)^0x8000000000000000LL) & (b64)  & 0x8000000000000000LL) ? silk_int64_MAX : (a64)-(b64)) );

    if( res != a64 - b64 ) {
        
        silk_assert( ( res == silk_int64_MAX && ( ( a64 >> 1 ) + ( b64 >> 1 ) > ( silk_int64_MAX >> 3 ) ) ) ||
                    ( res == silk_int64_MIN && ( ( a64 >> 1 ) + ( b64 >> 1 ) < ( silk_int64_MIN >> 3 ) ) ) );
    } else {
        
        silk_assert( res == a64 - b64 );
    }
    return res;
}

#undef silk_MUL
static inline opus_int32 silk_MUL(opus_int32 a32, opus_int32 b32){
    opus_int32 ret;
    opus_int64 ret64;
    ret = a32 * b32;
    ret64 = (opus_int64)a32 * (opus_int64)b32;
    silk_assert((opus_int64)ret == ret64 );        
    return ret;
}

#undef silk_MUL_uint
static inline opus_uint32 silk_MUL_uint(opus_uint32 a32, opus_uint32 b32){
    opus_uint32 ret;
    ret = a32 * b32;
    silk_assert((opus_uint64)ret == (opus_uint64)a32 * (opus_uint64)b32);        
    return ret;
}

#undef silk_MLA
static inline opus_int32 silk_MLA(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret;
    ret = a32 + b32 * c32;
    silk_assert((opus_int64)ret == (opus_int64)a32 + (opus_int64)b32 * (opus_int64)c32);    
    return ret;
}

#undef silk_MLA_uint
static inline opus_int32 silk_MLA_uint(opus_uint32 a32, opus_uint32 b32, opus_uint32 c32){
    opus_uint32 ret;
    ret = a32 + b32 * c32;
    silk_assert((opus_int64)ret == (opus_int64)a32 + (opus_int64)b32 * (opus_int64)c32);    
    return ret;
}

#undef silk_SMULWB
static inline opus_int32 silk_SMULWB(opus_int32 a32, opus_int32 b32){
    opus_int32 ret;
    ret = (a32 >> 16) * (opus_int32)((opus_int16)b32) + (((a32 & 0x0000FFFF) * (opus_int32)((opus_int16)b32)) >> 16);
    silk_assert((opus_int64)ret == ((opus_int64)a32 * (opus_int16)b32) >> 16);
    return ret;
}

#undef silk_SMLAWB
static inline opus_int32 silk_SMLAWB(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret;
    ret = silk_ADD32( a32, silk_SMULWB( b32, c32 ) );
    silk_assert(silk_ADD32( a32, silk_SMULWB( b32, c32 ) ) == silk_ADD_SAT32( a32, silk_SMULWB( b32, c32 ) ));
    return ret;
}

#undef silk_SMULWT
static inline opus_int32 silk_SMULWT(opus_int32 a32, opus_int32 b32){
    opus_int32 ret;
    ret = (a32 >> 16) * (b32 >> 16) + (((a32 & 0x0000FFFF) * (b32 >> 16)) >> 16);
    silk_assert((opus_int64)ret == ((opus_int64)a32 * (b32 >> 16)) >> 16);
    return ret;
}

#undef silk_SMLAWT
static inline opus_int32 silk_SMLAWT(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret;
    ret = a32 + ((b32 >> 16) * (c32 >> 16)) + (((b32 & 0x0000FFFF) * ((c32 >> 16)) >> 16));
    silk_assert((opus_int64)ret == (opus_int64)a32 + (((opus_int64)b32 * (c32 >> 16)) >> 16));
    return ret;
}

#undef silk_SMULL
static inline opus_int64 silk_SMULL(opus_int64 a64, opus_int64 b64){
    opus_int64 ret64;
    ret64 = a64 * b64;
    if( b64 != 0 ) {
        silk_assert( a64 == (ret64 / b64) );
    } else if( a64 != 0 ) {
        silk_assert( b64 == (ret64 / a64) );
    }
    return ret64;
}


#undef silk_SMLABB
static inline opus_int32 silk_SMLABB(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret;
    ret = a32 + (opus_int32)((opus_int16)b32) * (opus_int32)((opus_int16)c32);
    silk_assert((opus_int64)ret == (opus_int64)a32 + (opus_int64)b32 * (opus_int16)c32);
    return ret;
}


#undef silk_SMLABT
static inline opus_int32 silk_SMLABT(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret;
    ret = a32 + ((opus_int32)((opus_int16)b32)) * (c32 >> 16);
    silk_assert((opus_int64)ret == (opus_int64)a32 + (opus_int64)b32 * (c32 >> 16));
    return ret;
}


#undef silk_SMLATT
static inline opus_int32 silk_SMLATT(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret;
    ret = a32 + (b32 >> 16) * (c32 >> 16);
    silk_assert((opus_int64)ret == (opus_int64)a32 + (b32 >> 16) * (c32 >> 16));
    return ret;
}

#undef silk_SMULWW
static inline opus_int32 silk_SMULWW(opus_int32 a32, opus_int32 b32){
    opus_int32 ret, tmp1, tmp2;
    opus_int64 ret64;

    ret  = silk_SMULWB( a32, b32 );
    tmp1 = silk_RSHIFT_ROUND( b32, 16 );
    tmp2 = silk_MUL( a32, tmp1 );

    silk_assert( (opus_int64)tmp2 == (opus_int64) a32 * (opus_int64) tmp1 );

    tmp1 = ret;
    ret  = silk_ADD32( tmp1, tmp2 );
    silk_assert( silk_ADD32( tmp1, tmp2 ) == silk_ADD_SAT32( tmp1, tmp2 ) );

    ret64 = silk_RSHIFT64( silk_SMULL( a32, b32 ), 16 );
    silk_assert( (opus_int64)ret == ret64 );

    return ret;
}

#undef silk_SMLAWW
static inline opus_int32 silk_SMLAWW(opus_int32 a32, opus_int32 b32, opus_int32 c32){
    opus_int32 ret, tmp;

    tmp = silk_SMULWW( b32, c32 );
    ret = silk_ADD32( a32, tmp );
    silk_assert( ret == silk_ADD_SAT32( a32, tmp ) );
    return ret;
}


#undef  silk_MLA_ovflw
#define silk_MLA_ovflw(a32, b32, c32)    ((a32) + ((b32) * (c32)))
#undef  silk_SMLABB_ovflw
#define silk_SMLABB_ovflw(a32, b32, c32)    ((a32) + ((opus_int32)((opus_int16)(b32))) * (opus_int32)((opus_int16)(c32)))







#undef silk_DIV32
static inline opus_int32 silk_DIV32(opus_int32 a32, opus_int32 b32){
    silk_assert( b32 != 0 );
    return a32 / b32;
}

#undef silk_DIV32_16
static inline opus_int32 silk_DIV32_16(opus_int32 a32, opus_int32 b32){
    silk_assert( b32 != 0 );
    silk_assert( b32 <= silk_int16_MAX );
    silk_assert( b32 >= silk_int16_MIN );
    return a32 / b32;
}










#undef silk_LSHIFT8
static inline opus_int8 silk_LSHIFT8(opus_int8 a, opus_int32 shift){
    opus_int8 ret;
    ret = a << shift;
    silk_assert(shift >= 0);
    silk_assert(shift < 8);
    silk_assert((opus_int64)ret == ((opus_int64)a) << shift);
    return ret;
}

#undef silk_LSHIFT16
static inline opus_int16 silk_LSHIFT16(opus_int16 a, opus_int32 shift){
    opus_int16 ret;
    ret = a << shift;
    silk_assert(shift >= 0);
    silk_assert(shift < 16);
    silk_assert((opus_int64)ret == ((opus_int64)a) << shift);
    return ret;
}

#undef silk_LSHIFT32
static inline opus_int32 silk_LSHIFT32(opus_int32 a, opus_int32 shift){
    opus_int32 ret;
    ret = a << shift;
    silk_assert(shift >= 0);
    silk_assert(shift < 32);
    silk_assert((opus_int64)ret == ((opus_int64)a) << shift);
    return ret;
}

#undef silk_LSHIFT64
static inline opus_int64 silk_LSHIFT64(opus_int64 a, opus_int shift){
    silk_assert(shift >= 0);
    silk_assert(shift < 64);
    return a << shift;
}

#undef silk_LSHIFT_ovflw
static inline opus_int32 silk_LSHIFT_ovflw(opus_int32 a, opus_int32 shift){
    silk_assert(shift >= 0);            
    return a << shift;
}

#undef silk_LSHIFT_uint
static inline opus_uint32 silk_LSHIFT_uint(opus_uint32 a, opus_int32 shift){
    opus_uint32 ret;
    ret = a << shift;
    silk_assert(shift >= 0);
    silk_assert((opus_int64)ret == ((opus_int64)a) << shift);
    return ret;
}

#undef silk_RSHIFT8
static inline opus_int8 silk_RSHIFT8(opus_int8 a, opus_int32 shift){
    silk_assert(shift >=  0);
    silk_assert(shift < 8);
    return a >> shift;
}

#undef silk_RSHIFT16
static inline opus_int16 silk_RSHIFT16(opus_int16 a, opus_int32 shift){
    silk_assert(shift >=  0);
    silk_assert(shift < 16);
    return a >> shift;
}

#undef silk_RSHIFT32
static inline opus_int32 silk_RSHIFT32(opus_int32 a, opus_int32 shift){
    silk_assert(shift >=  0);
    silk_assert(shift < 32);
    return a >> shift;
}

#undef silk_RSHIFT64
static inline opus_int64 silk_RSHIFT64(opus_int64 a, opus_int64 shift){
    silk_assert(shift >=  0);
    silk_assert(shift <= 63);
    return a >> shift;
}

#undef silk_RSHIFT_uint
static inline opus_uint32 silk_RSHIFT_uint(opus_uint32 a, opus_int32 shift){
    silk_assert(shift >=  0);
    silk_assert(shift <= 32);
    return a >> shift;
}

#undef silk_ADD_LSHIFT
static inline opus_int32 silk_ADD_LSHIFT(opus_int32 a, opus_int32 b, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 31);
    ret = a + (b << shift);
    silk_assert((opus_int64)ret == (opus_int64)a + (((opus_int64)b) << shift));
    return ret;                
}

#undef silk_ADD_LSHIFT32
static inline opus_int32 silk_ADD_LSHIFT32(opus_int32 a, opus_int32 b, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 31);
    ret = a + (b << shift);
    silk_assert((opus_int64)ret == (opus_int64)a + (((opus_int64)b) << shift));
    return ret;                
}

#undef silk_ADD_LSHIFT_uint
static inline opus_uint32 silk_ADD_LSHIFT_uint(opus_uint32 a, opus_uint32 b, opus_int32 shift){
    opus_uint32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 32);
    ret = a + (b << shift);
    silk_assert((opus_int64)ret == (opus_int64)a + (((opus_int64)b) << shift));
    return ret;                
}

#undef silk_ADD_RSHIFT
static inline opus_int32 silk_ADD_RSHIFT(opus_int32 a, opus_int32 b, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 31);
    ret = a + (b >> shift);
    silk_assert((opus_int64)ret == (opus_int64)a + (((opus_int64)b) >> shift));
    return ret;                
}

#undef silk_ADD_RSHIFT32
static inline opus_int32 silk_ADD_RSHIFT32(opus_int32 a, opus_int32 b, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 31);
    ret = a + (b >> shift);
    silk_assert((opus_int64)ret == (opus_int64)a + (((opus_int64)b) >> shift));
    return ret;                
}

#undef silk_ADD_RSHIFT_uint
static inline opus_uint32 silk_ADD_RSHIFT_uint(opus_uint32 a, opus_uint32 b, opus_int32 shift){
    opus_uint32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 32);
    ret = a + (b >> shift);
    silk_assert((opus_int64)ret == (opus_int64)a + (((opus_int64)b) >> shift));
    return ret;                
}

#undef silk_SUB_LSHIFT32
static inline opus_int32 silk_SUB_LSHIFT32(opus_int32 a, opus_int32 b, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 31);
    ret = a - (b << shift);
    silk_assert((opus_int64)ret == (opus_int64)a - (((opus_int64)b) << shift));
    return ret;                
}

#undef silk_SUB_RSHIFT32
static inline opus_int32 silk_SUB_RSHIFT32(opus_int32 a, opus_int32 b, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift >= 0);
    silk_assert(shift <= 31);
    ret = a - (b >> shift);
    silk_assert((opus_int64)ret == (opus_int64)a - (((opus_int64)b) >> shift));
    return ret;                
}

#undef silk_RSHIFT_ROUND
static inline opus_int32 silk_RSHIFT_ROUND(opus_int32 a, opus_int32 shift){
    opus_int32 ret;
    silk_assert(shift > 0);        
    silk_assert(shift < 32);
    ret = shift == 1 ? (a >> 1) + (a & 1) : ((a >> (shift - 1)) + 1) >> 1;
    silk_assert((opus_int64)ret == ((opus_int64)a + ((opus_int64)1 << (shift - 1))) >> shift);
    return ret;
}

#undef silk_RSHIFT_ROUND64
static inline opus_int64 silk_RSHIFT_ROUND64(opus_int64 a, opus_int32 shift){
    opus_int64 ret;
    silk_assert(shift > 0);        
    silk_assert(shift < 64);
    ret = shift == 1 ? (a >> 1) + (a & 1) : ((a >> (shift - 1)) + 1) >> 1;
    return ret;
}








#undef silk_abs_int64
static inline opus_int64 silk_abs_int64(opus_int64 a){
    silk_assert(a != 0x8000000000000000);
    return (((a) >  0)  ? (a) : -(a));            
}

#undef silk_abs_int32
static inline opus_int32 silk_abs_int32(opus_int32 a){
    silk_assert(a != 0x80000000);
    return abs(a);
}

#undef silk_CHECK_FIT8
static inline opus_int8 silk_CHECK_FIT8( opus_int64 a ){
    opus_int8 ret;
    ret = (opus_int8)a;
    silk_assert( (opus_int64)ret == a );
    return( ret );
}

#undef silk_CHECK_FIT16
static inline opus_int16 silk_CHECK_FIT16( opus_int64 a ){
    opus_int16 ret;
    ret = (opus_int16)a;
    silk_assert( (opus_int64)ret == a );
    return( ret );
}

#undef silk_CHECK_FIT32
static inline opus_int32 silk_CHECK_FIT32( opus_int64 a ){
    opus_int32 ret;
    ret = (opus_int32)a;
    silk_assert( (opus_int64)ret == a );
    return( ret );
}








#endif
#endif 
