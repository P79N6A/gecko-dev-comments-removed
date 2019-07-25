






































#ifndef jsnuminlines_h___
#define jsnuminlines_h___

#include "jsstr.h"

namespace js {

template<typename T> struct NumberTraits { };
template<> struct NumberTraits<int32> {
  static JS_ALWAYS_INLINE int32 NaN() { return 0; }
  static JS_ALWAYS_INLINE int32 toSelfType(int32 i) { return i; }
  static JS_ALWAYS_INLINE int32 toSelfType(jsdouble d) { return js_DoubleToECMAUint32(d); }
};
template<> struct NumberTraits<jsdouble> {
  static JS_ALWAYS_INLINE jsdouble NaN() { return js_NaN; }
  static JS_ALWAYS_INLINE jsdouble toSelfType(int32 i) { return i; }
  static JS_ALWAYS_INLINE jsdouble toSelfType(jsdouble d) { return d; }
};

template<typename T>
static JS_ALWAYS_INLINE bool
StringToNumberType(JSContext *cx, JSString *str, T *result)
{
    size_t length = str->length();
    const jschar *chars = str->getChars(NULL);
    if (!chars)
        return false;

    if (length == 1) {
        jschar c = chars[0];
        if ('0' <= c && c <= '9') {
            *result = NumberTraits<T>::toSelfType(T(c - '0'));
            return true;
        }
        if (JS_ISSPACE(c)) {
            *result = NumberTraits<T>::toSelfType(T(0));
            return true;
        }
        *result = NumberTraits<T>::NaN();
        return true;
    }

    const jschar *bp = chars;
    const jschar *end = chars + length;
    bp = js_SkipWhiteSpace(bp, end);

    
    if (end - bp >= 2 && bp[0] == '0' && (bp[1] == 'x' || bp[1] == 'X')) {
        
        const jschar *endptr;
        double d;
        if (!GetPrefixInteger(cx, bp + 2, end, 16, &endptr, &d) ||
            js_SkipWhiteSpace(endptr, end) != end) {
            *result = NumberTraits<T>::NaN();
            return true;
        }
        *result = NumberTraits<T>::toSelfType(d);
        return true;
    }

    






    const jschar *ep;
    double d;
    if (!js_strtod(cx, bp, end, &ep, &d) || js_SkipWhiteSpace(ep, end) != end) {
        *result = NumberTraits<T>::NaN();
        return true;
    }
    *result = NumberTraits<T>::toSelfType(d);
    return true;
}

} 

#endif 
