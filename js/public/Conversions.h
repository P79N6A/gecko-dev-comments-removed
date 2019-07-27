







#ifndef js_Conversions_h
#define js_Conversions_h

#include "mozilla/Casting.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/TypeTraits.h"

#include <math.h>

#include "jspubtd.h"

#include "js/RootingAPI.h"
#include "js/Value.h"

struct JSContext;

namespace js {


extern JS_PUBLIC_API(bool)
ToBooleanSlow(JS::HandleValue v);


extern JS_PUBLIC_API(bool)
ToNumberSlow(JSContext* cx, JS::Value v, double* dp);


extern JS_PUBLIC_API(bool)
ToInt8Slow(JSContext *cx, JS::HandleValue v, int8_t *out);


extern JS_PUBLIC_API(bool)
ToInt16Slow(JSContext *cx, JS::HandleValue v, int16_t *out);


extern JS_PUBLIC_API(bool)
ToInt32Slow(JSContext* cx, JS::HandleValue v, int32_t* out);


extern JS_PUBLIC_API(bool)
ToUint32Slow(JSContext* cx, JS::HandleValue v, uint32_t* out);


extern JS_PUBLIC_API(bool)
ToUint16Slow(JSContext* cx, JS::HandleValue v, uint16_t* out);


extern JS_PUBLIC_API(bool)
ToInt64Slow(JSContext* cx, JS::HandleValue v, int64_t* out);


extern JS_PUBLIC_API(bool)
ToUint64Slow(JSContext* cx, JS::HandleValue v, uint64_t* out);


extern JS_PUBLIC_API(JSString*)
ToStringSlow(JSContext* cx, JS::HandleValue v);


extern JS_PUBLIC_API(JSObject*)
ToObjectSlow(JSContext* cx, JS::HandleValue v, bool reportScanStack);

} 

namespace JS {

namespace detail {

#ifdef JS_DEBUG





extern JS_PUBLIC_API(void)
AssertArgumentsAreSane(JSContext* cx, HandleValue v);
#else
inline void AssertArgumentsAreSane(JSContext* cx, HandleValue v)
{}
#endif 

} 









extern JS_PUBLIC_API(bool)
OrdinaryToPrimitive(JSContext* cx, HandleObject obj, JSType type, MutableHandleValue vp);


MOZ_ALWAYS_INLINE bool
ToBoolean(HandleValue v)
{
    if (v.isBoolean())
        return v.toBoolean();
    if (v.isInt32())
        return v.toInt32() != 0;
    if (v.isNullOrUndefined())
        return false;
    if (v.isDouble()) {
        double d = v.toDouble();
        return !mozilla::IsNaN(d) && d != 0;
    }
    if (v.isSymbol())
        return true;

    
    return js::ToBooleanSlow(v);
}


MOZ_ALWAYS_INLINE bool
ToNumber(JSContext* cx, HandleValue v, double* out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    return js::ToNumberSlow(cx, v, out);
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

    return d < 0 ? ceil(d) : floor(d);
}


MOZ_ALWAYS_INLINE bool
ToInt32(JSContext* cx, JS::HandleValue v, int32_t* out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    return js::ToInt32Slow(cx, v, out);
}


MOZ_ALWAYS_INLINE bool
ToUint32(JSContext* cx, HandleValue v, uint32_t* out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = uint32_t(v.toInt32());
        return true;
    }
    return js::ToUint32Slow(cx, v, out);
}


MOZ_ALWAYS_INLINE bool
ToInt16(JSContext *cx, JS::HandleValue v, int16_t *out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = int16_t(v.toInt32());
        return true;
    }
    return js::ToInt16Slow(cx, v, out);
}


MOZ_ALWAYS_INLINE bool
ToUint16(JSContext* cx, HandleValue v, uint16_t* out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = uint16_t(v.toInt32());
        return true;
    }
    return js::ToUint16Slow(cx, v, out);
}


MOZ_ALWAYS_INLINE bool
ToInt8(JSContext *cx, JS::HandleValue v, int8_t *out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = int8_t(v.toInt32());
        return true;
    }
    return js::ToInt8Slow(cx, v, out);
}





MOZ_ALWAYS_INLINE bool
ToInt64(JSContext* cx, HandleValue v, int64_t* out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = int64_t(v.toInt32());
        return true;
    }
    return js::ToInt64Slow(cx, v, out);
}





MOZ_ALWAYS_INLINE bool
ToUint64(JSContext* cx, HandleValue v, uint64_t* out)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isInt32()) {
        *out = uint64_t(v.toInt32());
        return true;
    }
    return js::ToUint64Slow(cx, v, out);
}


MOZ_ALWAYS_INLINE JSString*
ToString(JSContext* cx, HandleValue v)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isString())
        return v.toString();
    return js::ToStringSlow(cx, v);
}


inline JSObject*
ToObject(JSContext* cx, HandleValue v)
{
    detail::AssertArgumentsAreSane(cx, v);

    if (v.isObject())
        return &v.toObject();
    return js::ToObjectSlow(cx, v, false);
}

namespace detail {














template<typename ResultType>
inline ResultType
ToUintWidth(double d)
{
    static_assert(mozilla::IsUnsigned<ResultType>::value,
                  "ResultType must be an unsigned type");

    uint64_t bits = mozilla::BitwiseCast<uint64_t>(d);
    unsigned DoubleExponentShift = mozilla::FloatingPoint<double>::kExponentShift;

    
    
    int_fast16_t exp =
        int_fast16_t((bits & mozilla::FloatingPoint<double>::kExponentBits) >> DoubleExponentShift) -
        int_fast16_t(mozilla::FloatingPoint<double>::kExponentBias);

    
    
    if (exp < 0)
        return 0;

    uint_fast16_t exponent = mozilla::AssertedCast<uint_fast16_t>(exp);

    
    
    
    
    
    
    const size_t ResultWidth = CHAR_BIT * sizeof(ResultType);
    if (exponent >= DoubleExponentShift + ResultWidth)
        return 0;

    
    
    
    static_assert(sizeof(ResultType) <= sizeof(uint64_t),
                  "Left-shifting below would lose upper bits");
    ResultType result = (exponent > DoubleExponentShift)
                        ? ResultType(bits << (exponent - DoubleExponentShift))
                        : ResultType(bits >> (DoubleExponentShift - exponent));

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (exponent < ResultWidth) {
        ResultType implicitOne = ResultType(1) << exponent;
        result &= implicitOne - 1; 
        result += implicitOne; 
    }

    
    return (bits & mozilla::FloatingPoint<double>::kSignBit) ? ~result + 1 : result;
}

template<typename ResultType>
inline ResultType
ToIntWidth(double d)
{
    static_assert(mozilla::IsSigned<ResultType>::value,
                  "ResultType must be a signed type");

    const ResultType MaxValue = (1ULL << (CHAR_BIT * sizeof(ResultType) - 1)) - 1;
    const ResultType MinValue = -MaxValue - 1;

    typedef typename mozilla::MakeUnsigned<ResultType>::Type UnsignedResult;
    UnsignedResult u = ToUintWidth<UnsignedResult>(d);
    if (u <= UnsignedResult(MaxValue))
        return static_cast<ResultType>(u);
    return (MinValue + static_cast<ResultType>(u - MaxValue)) - 1;
}

} 


inline int32_t
ToInt32(double d)
{
    
    
#if defined (__arm__) && defined (__GNUC__) && !defined(__APPLE__)
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
    return detail::ToIntWidth<int32_t>(d);
#endif
}


inline uint32_t
ToUint32(double d)
{
    return detail::ToUintWidth<uint32_t>(d);
}


inline int8_t
ToInt8(double d)
{
    return detail::ToIntWidth<int8_t>(d);
}


inline int16_t
ToInt16(double d)
{
    return detail::ToIntWidth<int16_t>(d);
}


inline int64_t
ToInt64(double d)
{
    return detail::ToIntWidth<int64_t>(d);
}


inline uint64_t
ToUint64(double d)
{
    return detail::ToUintWidth<uint64_t>(d);
}

} 

#endif 
