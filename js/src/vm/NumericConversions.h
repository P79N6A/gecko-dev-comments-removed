





#ifndef NumericConversions_h___
#define NumericConversions_h___

#include "mozilla/FloatingPoint.h"

#include "jscpucfg.h"

#include <math.h>


extern double js_NaN;

namespace js {

namespace detail {

union DoublePun {
    struct {
#if defined(IS_LITTLE_ENDIAN)
        uint32_t lo, hi;
#else
        uint32_t hi, lo;
#endif
    } s;
    uint64_t u64;
    double d;
};

} 


template<size_t width, typename ResultType>
inline ResultType
ToIntWidth(double d)
{
#if defined(__i386__) || defined(__i386) || defined(__x86_64__) || \
    defined(_M_IX86) || defined(_M_X64)
    detail::DoublePun du, duh, twoWidth;
    uint32_t di_h, u_tmp, expon, shift_amount;
    int32_t mask32;

    













    du.d = d;
    di_h = du.s.hi;

    u_tmp = (di_h & 0x7ff00000) - 0x3ff00000;
    if (u_tmp >= ((width + 52) << 20)) {
        
        
        
        return 0;
    }

    if (u_tmp < ((width - 1) << 20)) {
        
        return ResultType(d);
    }

    if (u_tmp > ((width - 1) << 20)) {
        
        
        
        
        
        
        
        
        
        
        
        expon = u_tmp >> 20;
        shift_amount = expon - (width - 11);
        mask32 = 0x80000000;
        if (shift_amount < 32) {
            
            mask32 >>= shift_amount;
            duh.s.hi = du.s.hi & mask32;
            duh.s.lo = 0;
        } else {
            
            mask32 >>= (shift_amount-32);
            duh.s.hi = du.s.hi;
            duh.s.lo = du.s.lo & mask32;
        }
        du.d -= duh.d;
    }

    di_h = du.s.hi;

    
    u_tmp = (di_h & 0x7ff00000);
    if (u_tmp >= (0x3ff00000 + ((width - 1) << 20))) {
        
        expon = u_tmp >> 20;

        
        shift_amount = expon - (0x3ff - 11);
        mask32 = 0x80000000;
        if (shift_amount < 32) {
            
            mask32 >>= shift_amount;
            du.s.hi &= mask32;
            du.s.lo = 0;
        } else {
            
            mask32 >>= (shift_amount-32);
            du.s.lo &= mask32;
        }
        
        twoWidth.s.hi = (0x3ff00000 + (width << 20)) ^ (du.s.hi & 0x80000000);
        twoWidth.s.lo = 0;
        du.d -= twoWidth.d;
    }

    return ResultType(du.d);
#else
    double twoWidth, twoWidthMin1;

    if (!mozilla::IsFinite(d))
        return 0;

    
    ResultType i = (ResultType) d;
    if ((double) i == d)
        return ResultType(i);

    twoWidth = width == 32 ? 4294967296.0 : 18446744073709551616.0;
    twoWidthMin1 = width == 32 ? 2147483648.0 : 9223372036854775808.0;
    d = fmod(d, twoWidth);
    d = (d >= 0) ? floor(d) : ceil(d) + twoWidth;
    return (ResultType) (d >= twoWidthMin1 ? d - twoWidth : d);
#endif
}


inline int32_t
ToInt32(double d)
{
#if defined (__arm__) && defined (__GNUC__)
    int32_t i;
    uint32_t    tmp0;
    uint32_t    tmp1;
    uint32_t    tmp2;
    asm (
    
    
    
    
    

    
    
    
    

    
"   mov     %1, %R4, LSR #20\n"
"   bic     %1, %1, #(1 << 11)\n"  

    
    
"   orr     %R4, %R4, #(1 << 20)\n"

    
    
    
    
    
    
    
    
    
    
    

    
    
    
"   sub     %1, %1, #0xff\n"
"   subs    %1, %1, #0x300\n"
"   bmi     8f\n"

    
    

    
"   subs    %3, %1, #52\n"         
"   bmi     1f\n"

    
    
    
    
    
    
"   bic     %2, %3, #0xff\n"
"   orr     %3, %3, %2, LSR #3\n"
    
    
"   mov     %Q4, %Q4, LSL %3\n"
"   b       2f\n"
"1:\n" 
    
    
"   rsb     %3, %1, #52\n"
"   mov     %Q4, %Q4, LSR %3\n"

    
    
    

"2:\n"
    
    
    
    
    
    
    
"   subs    %3, %1, #31\n"          
"   mov     %1, %R4, LSL #11\n"     
"   bmi     3f\n"

    
    
"   bic     %2, %3, #0xff\n"
"   orr     %3, %3, %2, LSR #3\n"
    
"   mov     %2, %1, LSL %3\n"
"   b       4f\n"
"3:\n" 
    
    
"   rsb     %3, %3, #0\n"          
"   mov     %2, %1, LSR %3\n"      

    
    
    

"4:\n"
    
"   orr     %Q4, %Q4, %2\n"
    
    
    
"   eor     %Q4, %Q4, %R4, ASR #31\n"
"   add     %0, %Q4, %R4, LSR #31\n"
"   b       9f\n"
"8:\n"
    
    
"   mov     %0, #0\n"
"9:\n"
    : "=r" (i), "=&r" (tmp0), "=&r" (tmp1), "=&r" (tmp2), "=&r" (d)
    : "4" (d)
    : "cc"
        );
    return i;
#else
    return ToIntWidth<32, int32_t>(d);
#endif
}


inline uint32_t
ToUint32(double d)
{
    return uint32_t(ToInt32(d));
}


inline int64_t
ToInt64(double d)
{
    return ToIntWidth<64, int64_t>(d);
}


inline uint64_t
ToUint64(double d)
{
    return uint64_t(ToInt64(d));
}


inline double
ToInteger(double d)
{
    if (d == 0)
        return d;

    if (!mozilla::IsFinite(d)) {
        if (mozilla::IsNaN(d))
            return 0;
        return d;
    }

    bool neg = (d < 0);
    d = floor(neg ? -d : d);
    return neg ? -d : d;
}

} 

#endif 
