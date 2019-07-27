





#ifndef jsnum_h
#define jsnum_h

#include "mozilla/FloatingPoint.h"
#include "mozilla/Range.h"

#include "NamespaceImports.h"

#include "vm/NumericConversions.h"

namespace js {

class StringBuffer;

extern bool
InitRuntimeNumberState(JSRuntime *rt);

#if !EXPOSE_INTL_API
extern void
FinishRuntimeNumberState(JSRuntime *rt);
#endif

} 


extern JSObject *
js_InitNumberClass(JSContext *cx, js::HandleObject obj);




extern const char js_isNaN_str[];
extern const char js_isFinite_str[];
extern const char js_parseFloat_str[];
extern const char js_parseInt_str[];

class JSAtom;

namespace js {






template <js::AllowGC allowGC>
extern JSString *
NumberToString(js::ThreadSafeContext *cx, double d);

extern JSAtom *
NumberToAtom(js::ExclusiveContext *cx, double d);

template <AllowGC allowGC>
extern JSFlatString *
Int32ToString(ThreadSafeContext *cx, int32_t i);

extern JSAtom *
Int32ToAtom(ExclusiveContext *cx, int32_t si);





extern bool JS_FASTCALL
NumberValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb);


extern JSFlatString *
NumberToString(JSContext *cx, double d);

extern JSFlatString *
IndexToString(JSContext *cx, uint32_t index);






struct ToCStringBuf
{
    




    static const size_t sbufSize = 34;
    char sbuf[sbufSize];
    char *dbuf;

    ToCStringBuf();
    ~ToCStringBuf();
};







extern char *
NumberToCString(JSContext *cx, ToCStringBuf *cbuf, double d, int base = 10);





const double DOUBLE_INTEGRAL_PRECISION_LIMIT = uint64_t(1) << 53;







template <typename CharT>
extern double
ParseDecimalNumber(const mozilla::Range<const CharT> chars);













template <typename CharT>
extern bool
GetPrefixInteger(ThreadSafeContext *cx, const CharT *start, const CharT *end, int base,
                 const CharT **endp, double *dp);






extern bool
GetDecimalInteger(ExclusiveContext *cx, const char16_t *start, const char16_t *end, double *dp);

extern bool
StringToNumber(ThreadSafeContext *cx, JSString *str, double *result);


MOZ_ALWAYS_INLINE bool
ToNumber(JSContext *cx, JS::MutableHandleValue vp)
{
    if (vp.isNumber())
        return true;
    double d;
    extern JS_PUBLIC_API(bool) ToNumberSlow(JSContext *cx, Value v, double *dp);
    if (!ToNumberSlow(cx, vp, &d))
        return false;

    vp.setNumber(d);
    return true;
}

bool
num_parseInt(JSContext *cx, unsigned argc, Value *vp);

}  













template <typename CharT>
extern bool
js_strtod(js::ThreadSafeContext *cx, const CharT *begin, const CharT *end,
          const CharT **dEnd, double *d);

extern bool
js_num_toString(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
js_num_valueOf(JSContext *cx, unsigned argc, js::Value *vp);

namespace js {

static MOZ_ALWAYS_INLINE bool
ValueFitsInInt32(const Value &v, int32_t *pi)
{
    if (v.isInt32()) {
        *pi = v.toInt32();
        return true;
    }
    return v.isDouble() && mozilla::NumberIsInt32(v.toDouble(), pi);
}










static MOZ_ALWAYS_INLINE bool
IsDefinitelyIndex(const Value &v, uint32_t *indexp)
{
    if (v.isInt32() && v.toInt32() >= 0) {
        *indexp = v.toInt32();
        return true;
    }

    int32_t i;
    if (v.isDouble() && mozilla::NumberIsInt32(v.toDouble(), &i) && i >= 0) {
        *indexp = uint32_t(i);
        return true;
    }

    return false;
}


static inline bool
ToInteger(JSContext *cx, HandleValue v, double *dp)
{
    if (v.isInt32()) {
        *dp = v.toInt32();
        return true;
    }
    if (v.isDouble()) {
        *dp = v.toDouble();
    } else {
        extern JS_PUBLIC_API(bool) ToNumberSlow(JSContext *cx, Value v, double *dp);
        if (!ToNumberSlow(cx, v, dp))
            return false;
    }
    *dp = ToInteger(*dp);
    return true;
}







template<typename T>
bool ToLengthClamped(T *cx, HandleValue v, uint32_t *out, bool *overflow);

inline bool
SafeAdd(int32_t one, int32_t two, int32_t *res)
{
    
    
    *res = uint32_t(one) + uint32_t(two);
    int64_t ores = (int64_t)one + (int64_t)two;
    return ores == (int64_t)*res;
}

inline bool
SafeSub(int32_t one, int32_t two, int32_t *res)
{
    *res = uint32_t(one) - uint32_t(two);
    int64_t ores = (int64_t)one - (int64_t)two;
    return ores == (int64_t)*res;
}

inline bool
SafeMul(int32_t one, int32_t two, int32_t *res)
{
    *res = uint32_t(one) * uint32_t(two);
    int64_t ores = (int64_t)one * (int64_t)two;
    return ores == (int64_t)*res;
}

extern bool
ToNumberSlow(ExclusiveContext *cx, Value v, double *dp);



MOZ_ALWAYS_INLINE bool
ToNumber(ExclusiveContext *cx, const Value &v, double *out)
{
    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    return ToNumberSlow(cx, v, out);
}





bool
NonObjectToNumberSlow(ThreadSafeContext *cx, Value v, double *out);

inline bool
NonObjectToNumber(ThreadSafeContext *cx, const Value &v, double *out)
{
    if (v.isNumber()) {
        *out = v.toNumber();
        return true;
    }
    return NonObjectToNumberSlow(cx, v, out);
}

bool
NonObjectToInt32Slow(ThreadSafeContext *cx, const Value &v, int32_t *out);

inline bool
NonObjectToInt32(ThreadSafeContext *cx, const Value &v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
        return true;
    }
    return NonObjectToInt32Slow(cx, v, out);
}

bool
NonObjectToUint32Slow(ThreadSafeContext *cx, const Value &v, uint32_t *out);

MOZ_ALWAYS_INLINE bool
NonObjectToUint32(ThreadSafeContext *cx, const Value &v, uint32_t *out)
{
    if (v.isInt32()) {
        *out = uint32_t(v.toInt32());
        return true;
    }
    return NonObjectToUint32Slow(cx, v, out);
}

void FIX_FPU();

} 

#endif 
