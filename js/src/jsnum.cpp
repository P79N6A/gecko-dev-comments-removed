









#include "jsnum.h"

#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"
#include "mozilla/RangedPtr.h"

#include "double-conversion.h"

#ifdef XP_OS2
#define _PC_53  PC_53
#define _MCW_EM MCW_EM
#define _MCW_PC MCW_PC
#endif
#include <locale.h>
#include <math.h>
#include <string.h>

#include "jstypes.h"
#include "jsapi.h"
#include "jsatom.h"
#include "jscntxt.h"
#include "jsdtoa.h"
#include "jsobj.h"
#include "jsstr.h"
#include "vm/GlobalObject.h"
#include "vm/NumericConversions.h"
#include "vm/StringBuffer.h"

#include "jsatominlines.h"
#include "jsstrinlines.h"
#include "vm/NumberObject-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::types;

using mozilla::PodCopy;
using mozilla::RangedPtr;






static bool
ComputeAccurateDecimalInteger(ExclusiveContext *cx,
                              const jschar *start, const jschar *end, double *dp)
{
    size_t length = end - start;
    char *cstr = cx->pod_malloc<char>(length + 1);
    if (!cstr)
        return false;

    for (size_t i = 0; i < length; i++) {
        char c = char(start[i]);
        JS_ASSERT(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
        cstr[i] = c;
    }
    cstr[length] = 0;

    char *estr;
    int err = 0;
    *dp = js_strtod_harder(cx->dtoaState(), cstr, &estr, &err);
    if (err == JS_DTOA_ENOMEM) {
        js_ReportOutOfMemory(cx);
        js_free(cstr);
        return false;
    }
    if (err == JS_DTOA_ERANGE && *dp == HUGE_VAL)
        *dp = js_PositiveInfinity;
    js_free(cstr);
    return true;
}

class BinaryDigitReader
{
    const int base;      
    int digit;           
    int digitMask;       
    const jschar *start; 
    const jschar *end;   

  public:
    BinaryDigitReader(int base, const jschar *start, const jschar *end)
      : base(base), digit(0), digitMask(0), start(start), end(end)
    {
    }

    
    int nextDigit() {
        if (digitMask == 0) {
            if (start == end)
                return -1;

            int c = *start++;
            JS_ASSERT(('0' <= c && c <= '9') || ('a' <= c && c <= 'z') || ('A' <= c && c <= 'Z'));
            if ('0' <= c && c <= '9')
                digit = c - '0';
            else if ('a' <= c && c <= 'z')
                digit = c - 'a' + 10;
            else
                digit = c - 'A' + 10;
            digitMask = base >> 1;
        }

        int bit = (digit & digitMask) != 0;
        digitMask >>= 1;
        return bit;
    }
};










static double
ComputeAccurateBinaryBaseInteger(const jschar *start, const jschar *end, int base)
{
    BinaryDigitReader bdr(base, start, end);

    
    int bit;
    do {
        bit = bdr.nextDigit();
    } while (bit == 0);

    JS_ASSERT(bit == 1); 

    
    double value = 1.0;
    for (int j = 52; j > 0; j--) {
        bit = bdr.nextDigit();
        if (bit < 0)
            return value;
        value = value * 2 + bit;
    }

    
    int bit2 = bdr.nextDigit();
    if (bit2 >= 0) {
        double factor = 2.0;
        int sticky = 0;  
        int bit3;

        while ((bit3 = bdr.nextDigit()) >= 0) {
            sticky |= bit3;
            factor *= 2;
        }
        value += bit2 & (bit | sticky);
        value *= factor;
    }

    return value;
}

double
js::ParseDecimalNumber(const JS::TwoByteChars chars)
{
    MOZ_ASSERT(chars.length() > 0);
    uint64_t dec = 0;
    RangedPtr<jschar> s = chars.start(), end = chars.end();
    do {
        jschar c = *s;
        MOZ_ASSERT('0' <= c && c <= '9');
        uint8_t digit = c - '0';
        uint64_t next = dec * 10 + digit;
        MOZ_ASSERT(next < DOUBLE_INTEGRAL_PRECISION_LIMIT,
                   "next value won't be an integrally-precise double");
        dec = next;
    } while (++s < end);
    return static_cast<double>(dec);
}

bool
js::GetPrefixInteger(ExclusiveContext *cx, const jschar *start, const jschar *end, int base,
                     const jschar **endp, double *dp)
{
    JS_ASSERT(start <= end);
    JS_ASSERT(2 <= base && base <= 36);

    const jschar *s = start;
    double d = 0.0;
    for (; s < end; s++) {
        int digit;
        jschar c = *s;
        if ('0' <= c && c <= '9')
            digit = c - '0';
        else if ('a' <= c && c <= 'z')
            digit = c - 'a' + 10;
        else if ('A' <= c && c <= 'Z')
            digit = c - 'A' + 10;
        else
            break;
        if (digit >= base)
            break;
        d = d * base + digit;
    }

    *endp = s;
    *dp = d;

    
    if (d < DOUBLE_INTEGRAL_PRECISION_LIMIT)
        return true;

    




    if (base == 10)
        return ComputeAccurateDecimalInteger(cx, start, s, dp);
    if ((base & (base - 1)) == 0)
        *dp = ComputeAccurateBinaryBaseInteger(start, s, base);

    return true;
}

static JSBool
num_isNaN(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setBoolean(true);
        return true;
    }

    double x;
    if (!ToNumber(cx, args.handleAt(0), &x))
        return false;

    args.rval().setBoolean(mozilla::IsNaN(x));
    return true;
}

static JSBool
num_isFinite(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setBoolean(false);
        return true;
    }

    double x;
    if (!ToNumber(cx, args.handleAt(0), &x))
        return false;

    args.rval().setBoolean(mozilla::IsFinite(x));
    return true;
}

static JSBool
num_parseFloat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 0) {
        args.rval().setDouble(js_NaN);
        return JS_TRUE;
    }
    JSString *str = ToString<CanGC>(cx, args.handleAt(0));
    if (!str)
        return JS_FALSE;
    const jschar *bp = str->getChars(cx);
    if (!bp)
        return JS_FALSE;
    const jschar *end = bp + str->length();
    const jschar *ep;
    double d;
    if (!js_strtod(cx, bp, end, &ep, &d))
        return JS_FALSE;
    if (ep == bp) {
        args.rval().setDouble(js_NaN);
        return JS_TRUE;
    }
    args.rval().setDouble(d);
    return JS_TRUE;
}


JSBool
js::num_parseInt(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    if (args.length() == 0) {
        args.rval().setDouble(js_NaN);
        return true;
    }

    if (args.length() == 1 ||
        (args[1].isInt32() && (args[1].toInt32() == 0 || args[1].toInt32() == 10))) {
        if (args[0].isInt32()) {
            args.rval().set(args[0]);
            return true;
        }

        










        if (args[0].isDouble()) {
            double d = args[0].toDouble();
            if (1.0e-6 < d && d < 1.0e21) {
                args.rval().setNumber(floor(d));
                return true;
            }
            if (-1.0e21 < d && d < -1.0e-6) {
                args.rval().setNumber(-floor(-d));
                return true;
            }
            if (d == 0.0) {
                args.rval().setInt32(0);
                return true;
            }
        }
    }

    
    RootedString inputString(cx, ToString<CanGC>(cx, args.handleAt(0)));
    if (!inputString)
        return false;
    args[0].setString(inputString);

    
    bool stripPrefix = true;
    int32_t radix;
    if (!args.hasDefined(1)) {
        radix = 10;
    } else {
        if (!ToInt32(cx, args[1], &radix))
            return false;
        if (radix == 0) {
            radix = 10;
        } else {
            if (radix < 2 || radix > 36) {
                args.rval().setDouble(js_NaN);
                return true;
            }
            if (radix != 16)
                stripPrefix = false;
        }
    }

    
    const jschar *s;
    const jschar *end;
    {
        const jschar *ws = inputString->getChars(cx);
        if (!ws)
            return false;
        end = ws + inputString->length();
        s = SkipSpace(ws, end);

        MOZ_ASSERT(ws <= s);
        MOZ_ASSERT(s <= end);
    }

    
    bool negative = (s != end && s[0] == '-');

    
    if (s != end && (s[0] == '-' || s[0] == '+'))
        s++;

    
    if (stripPrefix) {
        if (end - s >= 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X')) {
            s += 2;
            radix = 16;
        }
    }

    
    const jschar *actualEnd;
    double number;
    if (!GetPrefixInteger(cx, s, end, radix, &actualEnd, &number))
        return false;
    if (s == actualEnd)
        args.rval().setNumber(js_NaN);
    else
        args.rval().setNumber(negative ? -number : number);
    return true;
}

static const JSFunctionSpec number_functions[] = {
    JS_FN(js_isNaN_str,         num_isNaN,           1,0),
    JS_FN(js_isFinite_str,      num_isFinite,        1,0),
    JS_FN(js_parseFloat_str,    num_parseFloat,      1,0),
    JS_FN(js_parseInt_str,      num_parseInt,        2,0),
    JS_FS_END
};

Class NumberObject::class_ = {
    js_Number_str,
    JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_HAS_CACHED_PROTO(JSProto_Number),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static JSBool
Number(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    bool isConstructing = args.isConstructing();

    if (args.length() > 0) {
        if (!ToNumber(cx, args.handleAt(0)))
            return false;
        args.rval().set(args[0]);
    } else {
        args.rval().setInt32(0);
    }

    if (!isConstructing)
        return true;

    JSObject *obj = NumberObject::create(cx, args.rval().toNumber());
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

JS_ALWAYS_INLINE bool
IsNumber(const Value &v)
{
    return v.isNumber() || (v.isObject() && v.toObject().is<NumberObject>());
}

inline double
Extract(const Value &v)
{
    if (v.isNumber())
        return v.toNumber();
    return v.toObject().as<NumberObject>().unbox();
}

#if JS_HAS_TOSOURCE
JS_ALWAYS_INLINE bool
num_toSource_impl(JSContext *cx, CallArgs args)
{
    double d = Extract(args.thisv());

    StringBuffer sb(cx);
    if (!sb.append("(new Number(") ||
        !NumberValueToStringBuffer(cx, NumberValue(d), sb) ||
        !sb.append("))"))
    {
        return false;
    }

    JSString *str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static JSBool
num_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_toSource_impl>(cx, args);
}
#endif

ToCStringBuf::ToCStringBuf() :dbuf(NULL)
{
    JS_STATIC_ASSERT(sbufSize >= DTOSTR_STANDARD_BUFFER_SIZE);
}

ToCStringBuf::~ToCStringBuf()
{
    if (dbuf)
        js_free(dbuf);
}

template <AllowGC allowGC>
JSFlatString *
js::Int32ToString(ThreadSafeContext *cx, int32_t si)
{
    uint32_t ui;
    if (si >= 0) {
        if (StaticStrings::hasInt(si))
            return cx->staticStrings().getInt(si);
        ui = si;
    } else {
        ui = uint32_t(-si);
        JS_ASSERT_IF(si == INT32_MIN, ui == uint32_t(INT32_MAX) + 1);
    }

    if (cx->isExclusiveContext()) {
        if (JSFlatString *str = cx->asExclusiveContext()->dtoaCache().lookup(10, si))
            return str;
    }

    JSShortString *str = js_NewGCShortString<allowGC>(cx);
    if (!str)
        return NULL;

    jschar buffer[JSShortString::MAX_SHORT_LENGTH + 1];
    RangedPtr<jschar> end(buffer + JSShortString::MAX_SHORT_LENGTH,
                          buffer, JSShortString::MAX_SHORT_LENGTH + 1);
    *end = '\0';
    RangedPtr<jschar> start = BackfillIndexInCharBuffer(ui, end);
    if (si < 0)
        *--start = '-';

    jschar *dst = str->init(end - start);
    PodCopy(dst, start.get(), end - start + 1);

    if (cx->isExclusiveContext())
        cx->asExclusiveContext()->dtoaCache().cache(10, si, str);
    return str;
}

template JSFlatString *
js::Int32ToString<CanGC>(ThreadSafeContext *cx, int32_t si);

template JSFlatString *
js::Int32ToString<NoGC>(ThreadSafeContext *cx, int32_t si);


static char *
IntToCString(ToCStringBuf *cbuf, int i, int base = 10)
{
    unsigned u = (i < 0) ? -i : i;

    RangedPtr<char> cp(cbuf->sbuf + cbuf->sbufSize - 1, cbuf->sbuf, cbuf->sbufSize);
    *cp = '\0';

    
    switch (base) {
    case 10:
      cp = BackfillIndexInCharBuffer(u, cp);
      break;
    case 16:
      do {
          unsigned newu = u / 16;
          *--cp = "0123456789abcdef"[u - newu * 16];
          u = newu;
      } while (u != 0);
      break;
    default:
      JS_ASSERT(base >= 2 && base <= 36);
      do {
          unsigned newu = u / base;
          *--cp = "0123456789abcdefghijklmnopqrstuvwxyz"[u - newu * base];
          u = newu;
      } while (u != 0);
      break;
    }
    if (i < 0)
        *--cp = '-';

    return cp.get();
}

template <AllowGC allowGC>
static JSString * JS_FASTCALL
js_NumberToStringWithBase(ThreadSafeContext *cx, double d, int base);

JS_ALWAYS_INLINE bool
num_toString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsNumber(args.thisv()));

    double d = Extract(args.thisv());

    int32_t base = 10;
    if (args.hasDefined(0)) {
        double d2;
        if (!ToInteger(cx, args.handleAt(0), &d2))
            return false;

        if (d2 < 2 || d2 > 36) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_RADIX);
            return false;
        }

        base = int32_t(d2);
    }
    JSString *str = js_NumberToStringWithBase<CanGC>(cx, d, base);
    if (!str) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    args.rval().setString(str);
    return true;
}

static JSBool
num_toString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_toString_impl>(cx, args);
}

#if !ENABLE_INTL_API
JS_ALWAYS_INLINE bool
num_toLocaleString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsNumber(args.thisv()));

    double d = Extract(args.thisv());

    Rooted<JSString*> str(cx, js_NumberToStringWithBase<CanGC>(cx, d, 10));
    if (!str) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    



    JSAutoByteString numBytes(cx, str);
    if (!numBytes)
        return false;
    const char *num = numBytes.ptr();
    if (!num)
        return false;

    



    const char *nint = num;
    if (*nint == '-')
        nint++;
    while (*nint >= '0' && *nint <= '9')
        nint++;
    int digits = nint - num;
    const char *end = num + digits;
    if (!digits) {
        args.rval().setString(str);
        return true;
    }

    JSRuntime *rt = cx->runtime();
    size_t thousandsLength = strlen(rt->thousandsSeparator);
    size_t decimalLength = strlen(rt->decimalSeparator);

    
    int buflen = strlen(num);
    if (*nint == '.')
        buflen += decimalLength - 1; 

    const char *numGrouping;
    const char *tmpGroup;
    numGrouping = tmpGroup = rt->numGrouping;
    int remainder = digits;
    if (*num == '-')
        remainder--;

    while (*tmpGroup != CHAR_MAX && *tmpGroup != '\0') {
        if (*tmpGroup >= remainder)
            break;
        buflen += thousandsLength;
        remainder -= *tmpGroup;
        tmpGroup++;
    }

    int nrepeat;
    if (*tmpGroup == '\0' && *numGrouping != '\0') {
        nrepeat = (remainder - 1) / tmpGroup[-1];
        buflen += thousandsLength * nrepeat;
        remainder -= nrepeat * tmpGroup[-1];
    } else {
        nrepeat = 0;
    }
    tmpGroup--;

    char *buf = cx->pod_malloc<char>(buflen + 1);
    if (!buf)
        return false;

    char *tmpDest = buf;
    const char *tmpSrc = num;

    while (*tmpSrc == '-' || remainder--) {
        JS_ASSERT(tmpDest - buf < buflen);
        *tmpDest++ = *tmpSrc++;
    }
    while (tmpSrc < end) {
        JS_ASSERT(tmpDest - buf + ptrdiff_t(thousandsLength) <= buflen);
        strcpy(tmpDest, rt->thousandsSeparator);
        tmpDest += thousandsLength;
        JS_ASSERT(tmpDest - buf + *tmpGroup <= buflen);
        js_memcpy(tmpDest, tmpSrc, *tmpGroup);
        tmpDest += *tmpGroup;
        tmpSrc += *tmpGroup;
        if (--nrepeat < 0)
            tmpGroup--;
    }

    if (*nint == '.') {
        JS_ASSERT(tmpDest - buf + ptrdiff_t(decimalLength) <= buflen);
        strcpy(tmpDest, rt->decimalSeparator);
        tmpDest += decimalLength;
        JS_ASSERT(tmpDest - buf + ptrdiff_t(strlen(nint + 1)) <= buflen);
        strcpy(tmpDest, nint + 1);
    } else {
        JS_ASSERT(tmpDest - buf + ptrdiff_t(strlen(nint)) <= buflen);
        strcpy(tmpDest, nint);
    }

    if (cx->runtime()->localeCallbacks && cx->runtime()->localeCallbacks->localeToUnicode) {
        Rooted<Value> v(cx, StringValue(str));
        bool ok = !!cx->runtime()->localeCallbacks->localeToUnicode(cx, buf, &v);
        if (ok)
            args.rval().set(v);
        js_free(buf);
        return ok;
    }

    str = js_NewStringCopyN<CanGC>(cx, buf, buflen);
    js_free(buf);
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}

JSBool
num_toLocaleString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_toLocaleString_impl>(cx, args);
}
#endif

JS_ALWAYS_INLINE bool
num_valueOf_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsNumber(args.thisv()));
    args.rval().setNumber(Extract(args.thisv()));
    return true;
}

JSBool
js_num_valueOf(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_valueOf_impl>(cx, args);
}

const unsigned MAX_PRECISION = 100;

static bool
ComputePrecisionInRange(JSContext *cx, int minPrecision, int maxPrecision, HandleValue v,
                        int *precision)
{
    double prec;
    if (!ToInteger(cx, v, &prec))
        return false;
    if (minPrecision <= prec && prec <= maxPrecision) {
        *precision = int(prec);
        return true;
    }

    ToCStringBuf cbuf;
    if (char *numStr = NumberToCString(cx, &cbuf, prec, 10))
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_PRECISION_RANGE, numStr);
    return false;
}

static bool
DToStrResult(JSContext *cx, double d, JSDToStrMode mode, int precision, CallArgs args)
{
    char buf[DTOSTR_VARIABLE_BUFFER_SIZE(MAX_PRECISION + 1)];
    char *numStr = js_dtostr(cx->mainThread().dtoaState, buf, sizeof buf, mode, precision, d);
    if (!numStr) {
        JS_ReportOutOfMemory(cx);
        return false;
    }
    JSString *str = js_NewStringCopyZ<CanGC>(cx, numStr);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}





JS_ALWAYS_INLINE bool
num_toFixed_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsNumber(args.thisv()));

    int precision;
    if (args.length() == 0) {
        precision = 0;
    } else {
        if (!ComputePrecisionInRange(cx, -20, MAX_PRECISION, args.handleAt(0), &precision))
            return false;
    }

    return DToStrResult(cx, Extract(args.thisv()), DTOSTR_FIXED, precision, args);
}

JSBool
num_toFixed(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_toFixed_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
num_toExponential_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsNumber(args.thisv()));

    JSDToStrMode mode;
    int precision;
    if (args.length() == 0) {
        mode = DTOSTR_STANDARD_EXPONENTIAL;
        precision = 0;
    } else {
        mode = DTOSTR_EXPONENTIAL;
        if (!ComputePrecisionInRange(cx, 0, MAX_PRECISION, args.handleAt(0), &precision))
            return false;
    }

    return DToStrResult(cx, Extract(args.thisv()), mode, precision + 1, args);
}

JSBool
num_toExponential(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_toExponential_impl>(cx, args);
}

JS_ALWAYS_INLINE bool
num_toPrecision_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsNumber(args.thisv()));

    double d = Extract(args.thisv());

    if (!args.hasDefined(0)) {
        JSString *str = js_NumberToStringWithBase<CanGC>(cx, d, 10);
        if (!str) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
        args.rval().setString(str);
        return true;
    }

    JSDToStrMode mode;
    int precision;
    if (args.length() == 0) {
        mode = DTOSTR_STANDARD;
        precision = 0;
    } else {
        mode = DTOSTR_PRECISION;
        if (!ComputePrecisionInRange(cx, 1, MAX_PRECISION, args.handleAt(0), &precision))
            return false;
    }

    return DToStrResult(cx, d, mode, precision, args);
}

JSBool
num_toPrecision(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsNumber, num_toPrecision_impl>(cx, args);
}

static const JSFunctionSpec number_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,       num_toSource,          0, 0),
#endif
    JS_FN(js_toString_str,       num_toString,          1, 0),
#if ENABLE_INTL_API
         {js_toLocaleString_str, {NULL, NULL},           0,0, "Number_toLocaleString"},
#else
    JS_FN(js_toLocaleString_str, num_toLocaleString,     0,0),
#endif
    JS_FN(js_valueOf_str,        js_num_valueOf,        0, 0),
    JS_FN("toFixed",             num_toFixed,           1, 0),
    JS_FN("toExponential",       num_toExponential,     1, 0),
    JS_FN("toPrecision",         num_toPrecision,       1, 0),
    JS_FS_END
};



static JSBool
Number_isNaN(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1 || !args[0].isDouble()) {
        args.rval().setBoolean(false);
        return true;
    }
    args.rval().setBoolean(mozilla::IsNaN(args[0].toDouble()));
    return true;
}


static JSBool
Number_isFinite(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1 || !args[0].isNumber()) {
        args.rval().setBoolean(false);
        return true;
    }
    args.rval().setBoolean(args[0].isInt32() ||
                           mozilla::IsFinite(args[0].toDouble()));
    return true;
}


static JSBool
Number_isInteger(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1 || !args[0].isNumber()) {
        args.rval().setBoolean(false);
        return true;
    }
    Value val = args[0];
    args.rval().setBoolean(val.isInt32() ||
                           (mozilla::IsFinite(val.toDouble()) &&
                            ToInteger(val.toDouble()) == val.toDouble()));
    return true;
}


static JSBool
Number_toInteger(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        args.rval().setInt32(0);
        return true;
    }
    double asint;
    if (!ToInteger(cx, args.handleAt(0), &asint))
        return false;
    args.rval().setNumber(asint);
    return true;
}


static const JSFunctionSpec number_static_methods[] = {
    JS_FN("isFinite", Number_isFinite, 1, 0),
    JS_FN("isInteger", Number_isInteger, 1, 0),
    JS_FN("isNaN", Number_isNaN, 1, 0),
    JS_FN("toInteger", Number_toInteger, 1, 0),
    JS_FS_END
};



enum nc_slot {
    NC_NaN,
    NC_POSITIVE_INFINITY,
    NC_NEGATIVE_INFINITY,
    NC_MAX_VALUE,
    NC_MIN_VALUE,
    NC_LIMIT
};






static JSConstDoubleSpec number_constants[] = {
    {0,                         "NaN",               0,{0,0,0}},
    {0,                         "POSITIVE_INFINITY", 0,{0,0,0}},
    {0,                         "NEGATIVE_INFINITY", 0,{0,0,0}},
    {1.7976931348623157E+308,   "MAX_VALUE",         0,{0,0,0}},
    {0,                         "MIN_VALUE",         0,{0,0,0}},
    {0,0,0,{0,0,0}}
};

double js_NaN;
double js_PositiveInfinity;
double js_NegativeInfinity;

#if (defined __GNUC__ && defined __i386__) || \
    (defined __SUNPRO_CC && defined __i386)





inline void FIX_FPU() {
    short control;
    asm("fstcw %0" : "=m" (control) : );
    control &= ~0x300; 
    control |= 0x2f3;  
    asm("fldcw %0" : : "m" (control) );
}

#else

#define FIX_FPU() ((void)0)

#endif

bool
js::InitRuntimeNumberState(JSRuntime *rt)
{
    FIX_FPU();

    double d;

    



    d = mozilla::SpecificNaN(0, 0x8000000000000ULL);
    number_constants[NC_NaN].dval = js_NaN = d;
    rt->NaNValue.setDouble(d);

    d = mozilla::PositiveInfinity();
    number_constants[NC_POSITIVE_INFINITY].dval = js_PositiveInfinity = d;
    rt->positiveInfinityValue.setDouble(d);

    d = mozilla::NegativeInfinity();
    number_constants[NC_NEGATIVE_INFINITY].dval = js_NegativeInfinity = d;
    rt->negativeInfinityValue.setDouble(d);

    number_constants[NC_MIN_VALUE].dval = mozilla::MinDoubleValue();

    
    
    
#if !ENABLE_INTL_API
    
    const char *thousandsSeparator, *decimalPoint, *grouping;
#ifdef HAVE_LOCALECONV
    struct lconv *locale = localeconv();
    thousandsSeparator = locale->thousands_sep;
    decimalPoint = locale->decimal_point;
    grouping = locale->grouping;
#else
    thousandsSeparator = getenv("LOCALE_THOUSANDS_SEP");
    decimalPoint = getenv("LOCALE_DECIMAL_POINT");
    grouping = getenv("LOCALE_GROUPING");
#endif
    if (!thousandsSeparator)
        thousandsSeparator = "'";
    if (!decimalPoint)
        decimalPoint = ".";
    if (!grouping)
        grouping = "\3\0";

    



    size_t thousandsSeparatorSize = strlen(thousandsSeparator) + 1;
    size_t decimalPointSize = strlen(decimalPoint) + 1;
    size_t groupingSize = strlen(grouping) + 1;

    char *storage = js_pod_malloc<char>(thousandsSeparatorSize +
                                        decimalPointSize +
                                        groupingSize);
    if (!storage)
        return false;

    js_memcpy(storage, thousandsSeparator, thousandsSeparatorSize);
    rt->thousandsSeparator = storage;
    storage += thousandsSeparatorSize;

    js_memcpy(storage, decimalPoint, decimalPointSize);
    rt->decimalSeparator = storage;
    storage += decimalPointSize;

    js_memcpy(storage, grouping, groupingSize);
    rt->numGrouping = grouping;
#endif
    return true;
}

#if !ENABLE_INTL_API
void
js::FinishRuntimeNumberState(JSRuntime *rt)
{
    



    char *storage = const_cast<char *>(rt->thousandsSeparator);
    js_free(storage);
}
#endif

JSObject *
js_InitNumberClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    
    FIX_FPU();

    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    RootedObject numberProto(cx, global->createBlankPrototype(cx, &NumberObject::class_));
    if (!numberProto)
        return NULL;
    numberProto->as<NumberObject>().setPrimitiveValue(0);

    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, Number, cx->names().Number, 1);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, numberProto))
        return NULL;

    
    if (!JS_DefineConstDoubles(cx, ctor, number_constants))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, ctor, NULL, number_static_methods))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, numberProto, NULL, number_methods))
        return NULL;

    if (!JS_DefineFunctions(cx, global, number_functions))
        return NULL;

    RootedValue valueNaN(cx, cx->runtime()->NaNValue);
    RootedValue valueInfinity(cx, cx->runtime()->positiveInfinityValue);

    
    if (!DefineNativeProperty(cx, global, cx->names().NaN, valueNaN,
                              JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_PERMANENT | JSPROP_READONLY, 0, 0) ||
        !DefineNativeProperty(cx, global, cx->names().Infinity, valueInfinity,
                              JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_PERMANENT | JSPROP_READONLY, 0, 0))
    {
        return NULL;
    }

    if (!DefineConstructorAndPrototype(cx, global, JSProto_Number, ctor, numberProto))
        return NULL;

    return numberProto;
}

static char *
FracNumberToCString(ThreadSafeContext *cx, ToCStringBuf *cbuf, double d, int base = 10)
{
#ifdef DEBUG
    {
        int32_t _;
        JS_ASSERT(!mozilla::DoubleIsInt32(d, &_));
    }
#endif

    char* numStr;
    if (base == 10) {
        






        const double_conversion::DoubleToStringConverter &converter
            = double_conversion::DoubleToStringConverter::EcmaScriptConverter();
        double_conversion::StringBuilder builder(cbuf->sbuf, cbuf->sbufSize);
        converter.ToShortest(d, &builder);
        numStr = builder.Finalize();
    } else {
        numStr = cbuf->dbuf = js_dtobasestr(cx->dtoaState(), base, d);
    }
    return numStr;
}

char *
js::NumberToCString(JSContext *cx, ToCStringBuf *cbuf, double d, int base)
{
    int32_t i;
    return mozilla::DoubleIsInt32(d, &i)
           ? IntToCString(cbuf, i, base)
           : FracNumberToCString(cx, cbuf, d, base);
}

template <AllowGC allowGC>
static JSString * JS_FASTCALL
js_NumberToStringWithBase(ThreadSafeContext *cx, double d, int base)
{
    ToCStringBuf cbuf;
    char *numStr;

    




    if (base < 2 || base > 36)
        return NULL;

    int32_t i;
    if (mozilla::DoubleIsInt32(d, &i)) {
        if (base == 10 && StaticStrings::hasInt(i))
            return cx->staticStrings().getInt(i);
        if (unsigned(i) < unsigned(base)) {
            if (i < 10)
                return cx->staticStrings().getInt(i);
            jschar c = 'a' + i - 10;
            JS_ASSERT(StaticStrings::hasUnit(c));
            return cx->staticStrings().getUnit(c);
        }

        if (cx->isExclusiveContext()) {
            if (JSFlatString *str = cx->asExclusiveContext()->dtoaCache().lookup(base, d))
                return str;
        }

        numStr = IntToCString(&cbuf, i, base);
        JS_ASSERT(!cbuf.dbuf && numStr >= cbuf.sbuf && numStr < cbuf.sbuf + cbuf.sbufSize);
    } else {
        if (cx->isExclusiveContext()) {
            if (JSFlatString *str = cx->asExclusiveContext()->dtoaCache().lookup(base, d))
                return str;
        }

        numStr = FracNumberToCString(cx, &cbuf, d, base);
        if (!numStr) {
            js_ReportOutOfMemory(cx);
            return NULL;
        }
        JS_ASSERT_IF(base == 10,
                     !cbuf.dbuf && numStr >= cbuf.sbuf && numStr < cbuf.sbuf + cbuf.sbufSize);
        JS_ASSERT_IF(base != 10,
                     cbuf.dbuf && cbuf.dbuf == numStr);
    }

    JSFlatString *s = js_NewStringCopyZ<allowGC>(cx, numStr);

    if (cx->isExclusiveContext())
        cx->asExclusiveContext()->dtoaCache().cache(base, d, s);

    return s;
}

template <AllowGC allowGC>
JSString *
js_NumberToString(ThreadSafeContext *cx, double d)
{
    return js_NumberToStringWithBase<allowGC>(cx, d, 10);
}

template JSString *
js_NumberToString<CanGC>(ThreadSafeContext *cx, double d);

template JSString *
js_NumberToString<NoGC>(ThreadSafeContext *cx, double d);

JSFlatString *
js::NumberToString(JSContext *cx, double d)
{
    if (JSString *str = js_NumberToStringWithBase<CanGC>(cx, d, 10))
        return &str->asFlat();
    return NULL;
}

JSFlatString *
js::IndexToString(JSContext *cx, uint32_t index)
{
    if (StaticStrings::hasUint(index))
        return cx->staticStrings().getUint(index);

    JSCompartment *c = cx->compartment();
    if (JSFlatString *str = c->dtoaCache.lookup(10, index))
        return str;

    JSShortString *str = js_NewGCShortString<CanGC>(cx);
    if (!str)
        return NULL;

    jschar buffer[JSShortString::MAX_SHORT_LENGTH + 1];
    RangedPtr<jschar> end(buffer + JSShortString::MAX_SHORT_LENGTH,
                          buffer, JSShortString::MAX_SHORT_LENGTH + 1);
    *end = '\0';
    RangedPtr<jschar> start = BackfillIndexInCharBuffer(index, end);

    jschar *dst = str->init(end - start);
    PodCopy(dst, start.get(), end - start + 1);

    c->dtoaCache.cache(10, index, str);
    return str;
}

bool JS_FASTCALL
js::NumberValueToStringBuffer(JSContext *cx, const Value &v, StringBuffer &sb)
{
    
    ToCStringBuf cbuf;
    const char *cstr;
    if (v.isInt32()) {
        cstr = IntToCString(&cbuf, v.toInt32());
    } else {
        cstr = NumberToCString(cx, &cbuf, v.toDouble());
        if (!cstr) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
    }

    



    size_t cstrlen = strlen(cstr);
    JS_ASSERT(!cbuf.dbuf && cstrlen < cbuf.sbufSize);
    return sb.appendInflated(cstr, cstrlen);
}

bool
js::StringToNumber(ExclusiveContext *cx, JSString *str, double *result)
{
    size_t length = str->length();
    const jschar *chars = str->getChars(NULL);
    if (!chars)
        return false;

    if (length == 1) {
        jschar c = chars[0];
        if ('0' <= c && c <= '9') {
            *result = c - '0';
            return true;
        }
        if (unicode::IsSpace(c)) {
            *result = 0.0;
            return true;
        }
        *result = js_NaN;
        return true;
    }

    const jschar *end = chars + length;
    const jschar *bp = SkipSpace(chars, end);

    
    if (end - bp >= 2 && bp[0] == '0' && (bp[1] == 'x' || bp[1] == 'X')) {
        




        const jschar *endptr;
        double d;
        if (!GetPrefixInteger(cx, bp + 2, end, 16, &endptr, &d) ||
            endptr == bp + 2 ||
            SkipSpace(endptr, end) != end)
        {
            *result = js_NaN;
            return true;
        }
        *result = d;
        return true;
    }

    






    const jschar *ep;
    double d;
    if (!js_strtod(cx, bp, end, &ep, &d) || SkipSpace(ep, end) != end) {
        *result = js_NaN;
        return true;
    }
    *result = d;
    return true;
}

#if defined(_MSC_VER)
# pragma optimize("g", off)
#endif

bool
js::ToNumberSlow(ExclusiveContext *cx, Value v, double *out)
{
#ifdef DEBUG
    









#ifndef _MSC_VER
    {
        SkipRoot skip(cx, &v);
        MaybeCheckStackRoots(cx);
    }
#endif
#endif

    JS_ASSERT(!v.isNumber());
    goto skip_int_double;
    for (;;) {
        if (v.isNumber()) {
            *out = v.toNumber();
            return true;
        }
      skip_int_double:
        if (v.isString())
            return StringToNumber(cx, v.toString(), out);
        if (v.isBoolean()) {
            if (v.toBoolean()) {
                *out = 1.0;
                return true;
            }
            *out = 0.0;
            return true;
        }
        if (v.isNull()) {
            *out = 0.0;
            return true;
        }
        if (v.isUndefined())
            break;

        JS_ASSERT(v.isObject());
        RootedValue v2(cx, v);
        if (!ToPrimitive(cx->asJSContext(), JSTYPE_NUMBER, &v2))
            return false;
        v = v2;
        if (v.isObject())
            break;
    }

    *out = js_NaN;
    return true;
}

JS_PUBLIC_API(bool)
js::ToNumberSlow(JSContext *cx, Value v, double *out)
{
    return ToNumberSlow(static_cast<ExclusiveContext *>(cx), v, out);
}

#if defined(_MSC_VER)
# pragma optimize("", on)
#endif





JS_PUBLIC_API(bool)
js::ToInt64Slow(JSContext *cx, const Value &v, int64_t *out)
{
    JS_ASSERT(!v.isInt32());
    double d;
    if (v.isDouble()) {
        d = v.toDouble();
    } else {
        if (!ToNumberSlow(cx, v, &d))
            return false;
    }
    *out = ToInt64(d);
    return true;
}





JS_PUBLIC_API(bool)
js::ToUint64Slow(JSContext *cx, const Value &v, uint64_t *out)
{
    JS_ASSERT(!v.isInt32());
    double d;
    if (v.isDouble()) {
        d = v.toDouble();
    } else {
        if (!ToNumberSlow(cx, v, &d))
            return false;
    }
    *out = ToUint64(d);
    return true;
}

JS_PUBLIC_API(bool)
js::ToInt32Slow(JSContext *cx, const Value &v, int32_t *out)
{
    JS_ASSERT(!v.isInt32());
    double d;
    if (v.isDouble()) {
        d = v.toDouble();
    } else {
        if (!ToNumberSlow(cx, v, &d))
            return false;
    }
    *out = ToInt32(d);
    return true;
}

JS_PUBLIC_API(bool)
js::ToUint32Slow(JSContext *cx, const Value &v, uint32_t *out)
{
    JS_ASSERT(!v.isInt32());
    double d;
    if (v.isDouble()) {
        d = v.toDouble();
    } else {
        if (!ToNumberSlow(cx, v, &d))
            return false;
    }
    *out = ToUint32(d);
    return true;
}

JS_PUBLIC_API(bool)
js::ToUint16Slow(JSContext *cx, const Value &v, uint16_t *out)
{
    JS_ASSERT(!v.isInt32());
    double d;
    if (v.isDouble()) {
        d = v.toDouble();
    } else if (!ToNumberSlow(cx, v, &d)) {
        return false;
    }

    if (d == 0 || !mozilla::IsFinite(d)) {
        *out = 0;
        return true;
    }

    uint16_t u = (uint16_t) d;
    if ((double)u == d) {
        *out = u;
        return true;
    }

    bool neg = (d < 0);
    d = floor(neg ? -d : d);
    d = neg ? -d : d;
    unsigned m = JS_BIT(16);
    d = fmod(d, (double) m);
    if (d < 0)
        d += m;
    *out = (uint16_t) d;
    return true;
}

JSBool
js_strtod(ExclusiveContext *cx, const jschar *s, const jschar *send,
          const jschar **ep, double *dp)
{
    size_t i;
    char cbuf[32];
    char *cstr, *istr, *estr;
    JSBool negative;
    double d;

    const jschar *s1 = SkipSpace(s, send);
    size_t length = send - s1;

    
    if (length >= sizeof cbuf) {
        cstr = (char *) cx->malloc_(length + 1);
        if (!cstr)
           return JS_FALSE;
    } else {
        cstr = cbuf;
    }

    for (i = 0; i != length; i++) {
        if (s1[i] >> 8)
            break;
        cstr[i] = (char)s1[i];
    }
    cstr[i] = 0;

    istr = cstr;
    if ((negative = (*istr == '-')) != 0 || *istr == '+')
        istr++;
    if (*istr == 'I' && !strncmp(istr, "Infinity", 8)) {
        d = negative ? js_NegativeInfinity : js_PositiveInfinity;
        estr = istr + 8;
    } else {
        int err;
        d = js_strtod_harder(cx->dtoaState(), cstr, &estr, &err);
        if (d == HUGE_VAL)
            d = js_PositiveInfinity;
        else if (d == -HUGE_VAL)
            d = js_NegativeInfinity;
    }

    i = estr - cstr;
    if (cstr != cbuf)
        js_free(cstr);
    *ep = i ? s1 + i : s;
    *dp = d;
    return JS_TRUE;
}
