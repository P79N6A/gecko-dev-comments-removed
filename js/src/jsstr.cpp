
















#include "jsstr.h"

#include "mozilla/Attributes.h"
#include "mozilla/Casting.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"

#include <ctype.h>
#include <string.h>

#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jstypes.h"
#include "jsutil.h"

#include "builtin/Intl.h"
#include "builtin/RegExp.h"
#if ENABLE_INTL_API
#include "unicode/unorm.h"
#endif
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/Opcodes.h"
#include "vm/RegExpObject.h"
#include "vm/RegExpStatics.h"
#include "vm/ScopeObject.h"
#include "vm/StringBuffer.h"

#include "jsinferinlines.h"

#include "vm/Interpreter-inl.h"
#include "vm/String-inl.h"
#include "vm/StringObject-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;
using namespace js::unicode;

using mozilla::CheckedInt;
using mozilla::IsNaN;
using mozilla::IsNegativeZero;
using mozilla::PodCopy;
using mozilla::PodEqual;
using mozilla::SafeCast;

typedef Handle<JSLinearString*> HandleLinearString;

static JSLinearString *
ArgToRootedString(JSContext *cx, CallArgs &args, unsigned argno)
{
    if (argno >= args.length())
        return cx->names().undefined;

    JSString *str = ToString<CanGC>(cx, args[argno]);
    if (!str)
        return nullptr;

    args[argno].setString(str);
    return str->ensureLinear(cx);
}




static bool
str_decodeURI(JSContext *cx, unsigned argc, Value *vp);

static bool
str_decodeURI_Component(JSContext *cx, unsigned argc, Value *vp);

static bool
str_encodeURI(JSContext *cx, unsigned argc, Value *vp);

static bool
str_encodeURI_Component(JSContext *cx, unsigned argc, Value *vp);







static bool
str_escape(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    static const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                                  '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    JSLinearString *str = ArgToRootedString(cx, args, 0);
    if (!str)
        return false;

    size_t length = str->length();
    const jschar *chars = str->chars();

    static const uint8_t shouldPassThrough[256] = {
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
         0,0,0,0,0,0,0,0,0,0,1,1,0,1,1,1,       
         1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,       
         1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,       
         1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,       
         0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,       
         1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,     
    };

    
#ifdef DEBUG
    size_t count = 0;
    for (size_t i = 0; i < sizeof(shouldPassThrough); i++) {
        if (shouldPassThrough[i]) {
            count++;
        }
    }
    JS_ASSERT(count == 69);
#endif


    
    size_t newlength = length;
    for (size_t i = 0; i < length; i++) {
        jschar ch = chars[i];
        if (ch < 128 && shouldPassThrough[ch])
            continue;

        
        newlength += (ch < 256) ? 2 : 5;

        



        if (newlength < length) {
            js_ReportAllocationOverflow(cx);
            return false;
        }
    }

    if (newlength >= ~(size_t)0 / sizeof(jschar)) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    jschar *newchars = cx->pod_malloc<jschar>(newlength + 1);
    if (!newchars)
        return false;
    size_t i, ni;
    for (i = 0, ni = 0; i < length; i++) {
        jschar ch = chars[i];
        if (ch < 128 && shouldPassThrough[ch]) {
            newchars[ni++] = ch;
        } else if (ch < 256) {
            newchars[ni++] = '%';
            newchars[ni++] = digits[ch >> 4];
            newchars[ni++] = digits[ch & 0xF];
        } else {
            newchars[ni++] = '%';
            newchars[ni++] = 'u';
            newchars[ni++] = digits[ch >> 12];
            newchars[ni++] = digits[(ch & 0xF00) >> 8];
            newchars[ni++] = digits[(ch & 0xF0) >> 4];
            newchars[ni++] = digits[ch & 0xF];
        }
    }
    JS_ASSERT(ni == newlength);
    newchars[newlength] = 0;

    JSString *retstr = js_NewString<CanGC>(cx, newchars, newlength);
    if (!retstr) {
        js_free(newchars);
        return false;
    }

    args.rval().setString(retstr);
    return true;
}

static inline bool
Unhex4(const jschar *chars, jschar *result)
{
    jschar a = chars[0],
           b = chars[1],
           c = chars[2],
           d = chars[3];

    if (!(JS7_ISHEX(a) && JS7_ISHEX(b) && JS7_ISHEX(c) && JS7_ISHEX(d)))
        return false;

    *result = (((((JS7_UNHEX(a) << 4) + JS7_UNHEX(b)) << 4) + JS7_UNHEX(c)) << 4) + JS7_UNHEX(d);
    return true;
}

static inline bool
Unhex2(const jschar *chars, jschar *result)
{
    jschar a = chars[0],
           b = chars[1];

    if (!(JS7_ISHEX(a) && JS7_ISHEX(b)))
        return false;

    *result = (JS7_UNHEX(a) << 4) + JS7_UNHEX(b);
    return true;
}


static bool
str_unescape(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    JSLinearString *str = ArgToRootedString(cx, args, 0);
    if (!str)
        return false;

    



    JS_STATIC_ASSERT(JSString::MAX_LENGTH <= INT_MAX);

    
    int length = str->length();
    const jschar *chars = str->chars();

    
    StringBuffer sb(cx);

    




    
    int k = 0;
    bool building = false;

    while (true) {
        
        if (k == length) {
            JSLinearString *result;
            if (building) {
                result = sb.finishString();
                if (!result)
                    return false;
            } else {
                result = str;
            }

            args.rval().setString(result);
            return true;
        }

        
        jschar c = chars[k];

        
        if (c != '%')
            goto step_18;

        
        if (k > length - 6)
            goto step_14;

        
        if (chars[k + 1] != 'u')
            goto step_14;

#define ENSURE_BUILDING                             \
    JS_BEGIN_MACRO                                  \
        if (!building) {                            \
            building = true;                        \
            if (!sb.reserve(length))                \
                return false;                       \
            sb.infallibleAppend(chars, chars + k);  \
        }                                           \
    JS_END_MACRO

        
        if (Unhex4(&chars[k + 2], &c)) {
            ENSURE_BUILDING;
            k += 5;
            goto step_18;
        }

      step_14:
        
        if (k > length - 3)
            goto step_18;

        
        if (Unhex2(&chars[k + 1], &c)) {
            ENSURE_BUILDING;
            k += 2;
        }

      step_18:
        if (building)
            sb.infallibleAppend(c);

        
        k += 1;
    }
#undef ENSURE_BUILDING
}

#if JS_HAS_UNEVAL
static bool
str_uneval(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *str = ValueToSource(cx, args.get(0));
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}
#endif

static const JSFunctionSpec string_functions[] = {
    JS_FN(js_escape_str,             str_escape,                1,0),
    JS_FN(js_unescape_str,           str_unescape,              1,0),
#if JS_HAS_UNEVAL
    JS_FN(js_uneval_str,             str_uneval,                1,0),
#endif
    JS_FN(js_decodeURI_str,          str_decodeURI,             1,0),
    JS_FN(js_encodeURI_str,          str_encodeURI,             1,0),
    JS_FN(js_decodeURIComponent_str, str_decodeURI_Component,   1,0),
    JS_FN(js_encodeURIComponent_str, str_encodeURI_Component,   1,0),

    JS_FS_END
};

const jschar      js_empty_ucstr[]  = {0};
const JSSubString js_EmptySubString = {0, js_empty_ucstr};

static const unsigned STRING_ELEMENT_ATTRS = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

static bool
str_enumerate(JSContext *cx, HandleObject obj)
{
    RootedString str(cx, obj->as<StringObject>().unbox());
    RootedValue value(cx);
    for (size_t i = 0, length = str->length(); i < length; i++) {
        JSString *str1 = js_NewDependentString(cx, str, i, 1);
        if (!str1)
            return false;
        value.setString(str1);
        if (!JSObject::defineElement(cx, obj, i, value,
                                     JS_PropertyStub, JS_StrictPropertyStub,
                                     STRING_ELEMENT_ATTRS))
        {
            return false;
        }
    }

    return true;
}

bool
js::str_resolve(JSContext *cx, HandleObject obj, HandleId id, MutableHandleObject objp)
{
    if (!JSID_IS_INT(id))
        return true;

    RootedString str(cx, obj->as<StringObject>().unbox());

    int32_t slot = JSID_TO_INT(id);
    if ((size_t)slot < str->length()) {
        JSString *str1 = cx->staticStrings().getUnitStringForElement(cx, str, size_t(slot));
        if (!str1)
            return false;
        RootedValue value(cx, StringValue(str1));
        if (!JSObject::defineElement(cx, obj, uint32_t(slot), value, nullptr, nullptr,
                                     STRING_ELEMENT_ATTRS))
        {
            return false;
        }
        objp.set(obj);
    }
    return true;
}

const Class StringObject::class_ = {
    js_String_str,
    JSCLASS_HAS_RESERVED_SLOTS(StringObject::RESERVED_SLOTS) |
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_CACHED_PROTO(JSProto_String),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    str_enumerate,
    (JSResolveOp)str_resolve,
    JS_ConvertStub
};







static MOZ_ALWAYS_INLINE JSString *
ThisToStringForStringProto(JSContext *cx, CallReceiver call)
{
    JS_CHECK_RECURSION(cx, return nullptr);

    if (call.thisv().isString())
        return call.thisv().toString();

    if (call.thisv().isObject()) {
        RootedObject obj(cx, &call.thisv().toObject());
        if (obj->is<StringObject>()) {
            Rooted<jsid> id(cx, NameToId(cx->names().toString));
            if (ClassMethodIsNative(cx, obj, &StringObject::class_, id, js_str_toString)) {
                JSString *str = obj->as<StringObject>().unbox();
                call.setThis(StringValue(str));
                return str;
            }
        }
    } else if (call.thisv().isNullOrUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                             call.thisv().isNull() ? "null" : "undefined", "object");
        return nullptr;
    }

    JSString *str = ToStringSlow<CanGC>(cx, call.thisv());
    if (!str)
        return nullptr;

    call.setThis(StringValue(str));
    return str;
}

MOZ_ALWAYS_INLINE bool
IsString(HandleValue v)
{
    return v.isString() || (v.isObject() && v.toObject().is<StringObject>());
}

#if JS_HAS_TOSOURCE





static bool
str_quote(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;
    str = js_QuoteString(cx, str, '"');
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

MOZ_ALWAYS_INLINE bool
str_toSource_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsString(args.thisv()));

    Rooted<JSString*> str(cx, ToString<CanGC>(cx, args.thisv()));
    if (!str)
        return false;

    str = js_QuoteString(cx, str, '"');
    if (!str)
        return false;

    StringBuffer sb(cx);
    if (!sb.append("(new String(") || !sb.append(str) || !sb.append("))"))
        return false;

    str = sb.finishString();
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static bool
str_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsString, str_toSource_impl>(cx, args);
}

#endif 

MOZ_ALWAYS_INLINE bool
str_toString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsString(args.thisv()));

    args.rval().setString(args.thisv().isString()
                              ? args.thisv().toString()
                              : args.thisv().toObject().as<StringObject>().unbox());
    return true;
}

bool
js_str_toString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsString, str_toString_impl>(cx, args);
}





static MOZ_ALWAYS_INLINE bool
ValueToIntegerRange(JSContext *cx, HandleValue v, int32_t *out)
{
    if (v.isInt32()) {
        *out = v.toInt32();
    } else {
        double d;
        if (!ToInteger(cx, v, &d))
            return false;
        if (d > INT32_MAX)
            *out = INT32_MAX;
        else if (d < INT32_MIN)
            *out = INT32_MIN;
        else
            *out = int32_t(d);
    }

    return true;
}

static JSString *
DoSubstr(JSContext *cx, JSString *str, size_t begin, size_t len)
{
    








    if (str->isRope()) {
        JSRope *rope = &str->asRope();

        
        if (begin + len <= rope->leftChild()->length()) {
            str = rope->leftChild();
            return js_NewDependentString(cx, str, begin, len);
        }

        
        if (begin >= rope->leftChild()->length()) {
            str = rope->rightChild();
            begin -= rope->leftChild()->length();
            return js_NewDependentString(cx, str, begin, len);
        }

        



        JS_ASSERT (begin < rope->leftChild()->length() &&
                   begin + len > rope->leftChild()->length());

        size_t lhsLength = rope->leftChild()->length() - begin;
        size_t rhsLength = begin + len - rope->leftChild()->length();

        Rooted<JSRope *> ropeRoot(cx, rope);
        RootedString lhs(cx, js_NewDependentString(cx, ropeRoot->leftChild(),
                                                   begin, lhsLength));
        if (!lhs)
            return nullptr;

        RootedString rhs(cx, js_NewDependentString(cx, ropeRoot->rightChild(), 0, rhsLength));
        if (!rhs)
            return nullptr;

        return JSRope::new_<CanGC>(cx, lhs, rhs, len);
    }

    return js_NewDependentString(cx, str, begin, len);
}

static bool
str_substring(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    JSString *str = ThisToStringForStringProto(cx, args);
    if (!str)
        return false;

    int32_t length, begin, end;
    if (args.length() > 0) {
        end = length = int32_t(str->length());

        if (args[0].isInt32()) {
            begin = args[0].toInt32();
        } else {
            RootedString strRoot(cx, str);
            if (!ValueToIntegerRange(cx, args[0], &begin))
                return false;
            str = strRoot;
        }

        if (begin < 0)
            begin = 0;
        else if (begin > length)
            begin = length;

        if (args.hasDefined(1)) {
            if (args[1].isInt32()) {
                end = args[1].toInt32();
            } else {
                RootedString strRoot(cx, str);
                if (!ValueToIntegerRange(cx, args[1], &end))
                    return false;
                str = strRoot;
            }

            if (end > length) {
                end = length;
            } else {
                if (end < 0)
                    end = 0;
                if (end < begin) {
                    int32_t tmp = begin;
                    begin = end;
                    end = tmp;
                }
            }
        }

        str = DoSubstr(cx, str, size_t(begin), size_t(end - begin));
        if (!str)
            return false;
    }

    args.rval().setString(str);
    return true;
}

JSString* JS_FASTCALL
js_toLowerCase(JSContext *cx, JSString *str)
{
    size_t n = str->length();
    const jschar *s = str->getChars(cx);
    if (!s)
        return nullptr;

    jschar *news = cx->pod_malloc<jschar>(n + 1);
    if (!news)
        return nullptr;
    for (size_t i = 0; i < n; i++)
        news[i] = unicode::ToLowerCase(s[i]);
    news[n] = 0;
    str = js_NewString<CanGC>(cx, news, n);
    if (!str) {
        js_free(news);
        return nullptr;
    }
    return str;
}

static inline bool
ToLowerCaseHelper(JSContext *cx, CallReceiver call)
{
    RootedString str(cx, ThisToStringForStringProto(cx, call));
    if (!str)
        return false;

    str = js_toLowerCase(cx, str);
    if (!str)
        return false;

    call.rval().setString(str);
    return true;
}

static bool
str_toLowerCase(JSContext *cx, unsigned argc, Value *vp)
{
    return ToLowerCaseHelper(cx, CallArgsFromVp(argc, vp));
}

static bool
str_toLocaleLowerCase(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    



    if (cx->runtime()->localeCallbacks && cx->runtime()->localeCallbacks->localeToLowerCase) {
        RootedString str(cx, ThisToStringForStringProto(cx, args));
        if (!str)
            return false;

        RootedValue result(cx);
        if (!cx->runtime()->localeCallbacks->localeToLowerCase(cx, str, &result))
            return false;

        args.rval().set(result);
        return true;
    }

    return ToLowerCaseHelper(cx, args);
}

JSString* JS_FASTCALL
js_toUpperCase(JSContext *cx, JSString *str)
{
    size_t n = str->length();
    const jschar *s = str->getChars(cx);
    if (!s)
        return nullptr;
    jschar *news = cx->pod_malloc<jschar>(n + 1);
    if (!news)
        return nullptr;
    for (size_t i = 0; i < n; i++)
        news[i] = unicode::ToUpperCase(s[i]);
    news[n] = 0;
    str = js_NewString<CanGC>(cx, news, n);
    if (!str) {
        js_free(news);
        return nullptr;
    }
    return str;
}

static bool
ToUpperCaseHelper(JSContext *cx, CallReceiver call)
{
    RootedString str(cx, ThisToStringForStringProto(cx, call));
    if (!str)
        return false;

    str = js_toUpperCase(cx, str);
    if (!str)
        return false;

    call.rval().setString(str);
    return true;
}

static bool
str_toUpperCase(JSContext *cx, unsigned argc, Value *vp)
{
    return ToUpperCaseHelper(cx, CallArgsFromVp(argc, vp));
}

static bool
str_toLocaleUpperCase(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    



    if (cx->runtime()->localeCallbacks && cx->runtime()->localeCallbacks->localeToUpperCase) {
        RootedString str(cx, ThisToStringForStringProto(cx, args));
        if (!str)
            return false;

        RootedValue result(cx);
        if (!cx->runtime()->localeCallbacks->localeToUpperCase(cx, str, &result))
            return false;

        args.rval().set(result);
        return true;
    }

    return ToUpperCaseHelper(cx, args);
}

#if !EXPOSE_INTL_API
static bool
str_localeCompare(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    RootedString thatStr(cx, ToString<CanGC>(cx, args.get(0)));
    if (!thatStr)
        return false;

    if (cx->runtime()->localeCallbacks && cx->runtime()->localeCallbacks->localeCompare) {
        RootedValue result(cx);
        if (!cx->runtime()->localeCallbacks->localeCompare(cx, str, thatStr, &result))
            return false;

        args.rval().set(result);
        return true;
    }

    int32_t result;
    if (!CompareStrings(cx, str, thatStr, &result))
        return false;

    args.rval().setInt32(result);
    return true;
}
#endif

#if EXPOSE_INTL_API
static const size_t SB_LENGTH = 32;


static bool
str_normalize(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    UNormalizationMode form;
    if (!args.hasDefined(0)) {
        form = UNORM_NFC;
    } else {
        
        Rooted<JSLinearString*> formStr(cx, ArgToRootedString(cx, args, 0));
        if (!formStr)
            return false;

        
        if (formStr == cx->names().NFC) {
            form = UNORM_NFC;
        } else if (formStr == cx->names().NFD) {
            form = UNORM_NFD;
        } else if (formStr == cx->names().NFKC) {
            form = UNORM_NFKC;
        } else if (formStr == cx->names().NFKD) {
            form = UNORM_NFKD;
        } else {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_INVALID_NORMALIZE_FORM);
            return false;
        }
    }

    
    Rooted<JSFlatString*> flatStr(cx, str->ensureFlat(cx));
    if (!flatStr)
        return false;
    const UChar *srcChars = JSCharToUChar(flatStr->chars());
    int32_t srcLen = SafeCast<int32_t>(flatStr->length());
    StringBuffer chars(cx);
    if (!chars.resize(SB_LENGTH))
        return false;
    UErrorCode status = U_ZERO_ERROR;
    int32_t size = unorm_normalize(srcChars, srcLen, form, 0,
                                   JSCharToUChar(chars.begin()), SB_LENGTH,
                                   &status);
    if (status == U_BUFFER_OVERFLOW_ERROR) {
        if (!chars.resize(size))
            return false;
        status = U_ZERO_ERROR;
#ifdef DEBUG
        int32_t finalSize =
#endif
        unorm_normalize(srcChars, srcLen, form, 0,
                        JSCharToUChar(chars.begin()), size,
                        &status);
        MOZ_ASSERT(size == finalSize || U_FAILURE(status), "unorm_normalize behaved inconsistently");
    }
    if (U_FAILURE(status))
        return false;
    
    if (!chars.resize(size))
        return false;
    RootedString ns(cx, chars.finishString());
    if (!ns)
        return false;

    
    args.rval().setString(ns);
    return true;
}
#endif

bool
js_str_charAt(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedString str(cx);
    size_t i;
    if (args.thisv().isString() && args.length() != 0 && args[0].isInt32()) {
        str = args.thisv().toString();
        i = size_t(args[0].toInt32());
        if (i >= str->length())
            goto out_of_range;
    } else {
        str = ThisToStringForStringProto(cx, args);
        if (!str)
            return false;

        double d = 0.0;
        if (args.length() > 0 && !ToInteger(cx, args[0], &d))
            return false;

        if (d < 0 || str->length() <= d)
            goto out_of_range;
        i = size_t(d);
    }

    str = cx->staticStrings().getUnitStringForElement(cx, str, i);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;

  out_of_range:
    args.rval().setString(cx->runtime()->emptyString);
    return true;
}

bool
js_str_charCodeAt(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedString str(cx);
    size_t i;
    if (args.thisv().isString() && args.length() != 0 && args[0].isInt32()) {
        str = args.thisv().toString();
        i = size_t(args[0].toInt32());
        if (i >= str->length())
            goto out_of_range;
    } else {
        str = ThisToStringForStringProto(cx, args);
        if (!str)
            return false;

        double d = 0.0;
        if (args.length() > 0 && !ToInteger(cx, args[0], &d))
            return false;

        if (d < 0 || str->length() <= d)
            goto out_of_range;
        i = size_t(d);
    }

    jschar c;
    if (!str->getChar(cx, i, &c))
        return false;
    args.rval().setInt32(c);
    return true;

out_of_range:
    args.rval().setNaN();
    return true;
}







static const uint32_t sBMHCharSetSize = 256; 
static const uint32_t sBMHPatLenMax   = 255; 
static const int      sBMHBadPattern  = -2;  

int
js_BoyerMooreHorspool(const jschar *text, uint32_t textlen,
                      const jschar *pat, uint32_t patlen)
{
    uint8_t skip[sBMHCharSetSize];

    JS_ASSERT(0 < patlen && patlen <= sBMHPatLenMax);
    for (uint32_t i = 0; i < sBMHCharSetSize; i++)
        skip[i] = (uint8_t)patlen;
    uint32_t m = patlen - 1;
    for (uint32_t i = 0; i < m; i++) {
        jschar c = pat[i];
        if (c >= sBMHCharSetSize)
            return sBMHBadPattern;
        skip[c] = (uint8_t)(m - i);
    }
    jschar c;
    for (uint32_t k = m;
         k < textlen;
         k += ((c = text[k]) >= sBMHCharSetSize) ? patlen : skip[c]) {
        for (uint32_t i = k, j = m; ; i--, j--) {
            if (text[i] != pat[j])
                break;
            if (j == 0)
                return static_cast<int>(i);  
        }
    }
    return -1;
}

struct MemCmp {
    typedef uint32_t Extent;
    static MOZ_ALWAYS_INLINE Extent computeExtent(const jschar *, uint32_t patlen) {
        return (patlen - 1) * sizeof(jschar);
    }
    static MOZ_ALWAYS_INLINE bool match(const jschar *p, const jschar *t, Extent extent) {
        return memcmp(p, t, extent) == 0;
    }
};

struct ManualCmp {
    typedef const jschar *Extent;
    static MOZ_ALWAYS_INLINE Extent computeExtent(const jschar *pat, uint32_t patlen) {
        return pat + patlen;
    }
    static MOZ_ALWAYS_INLINE bool match(const jschar *p, const jschar *t, Extent extent) {
        for (; p != extent; ++p, ++t) {
            if (*p != *t)
                return false;
        }
        return true;
    }
};

template <class InnerMatch>
static int
UnrolledMatch(const jschar *text, uint32_t textlen, const jschar *pat, uint32_t patlen)
{
    JS_ASSERT(patlen > 0 && textlen > 0);
    const jschar *textend = text + textlen - (patlen - 1);
    const jschar p0 = *pat;
    const jschar *const patNext = pat + 1;
    const typename InnerMatch::Extent extent = InnerMatch::computeExtent(pat, patlen);
    uint8_t fixup;

    const jschar *t = text;
    switch ((textend - t) & 7) {
      case 0: if (*t++ == p0) { fixup = 8; goto match; }
      case 7: if (*t++ == p0) { fixup = 7; goto match; }
      case 6: if (*t++ == p0) { fixup = 6; goto match; }
      case 5: if (*t++ == p0) { fixup = 5; goto match; }
      case 4: if (*t++ == p0) { fixup = 4; goto match; }
      case 3: if (*t++ == p0) { fixup = 3; goto match; }
      case 2: if (*t++ == p0) { fixup = 2; goto match; }
      case 1: if (*t++ == p0) { fixup = 1; goto match; }
    }
    while (t != textend) {
      if (t[0] == p0) { t += 1; fixup = 8; goto match; }
      if (t[1] == p0) { t += 2; fixup = 7; goto match; }
      if (t[2] == p0) { t += 3; fixup = 6; goto match; }
      if (t[3] == p0) { t += 4; fixup = 5; goto match; }
      if (t[4] == p0) { t += 5; fixup = 4; goto match; }
      if (t[5] == p0) { t += 6; fixup = 3; goto match; }
      if (t[6] == p0) { t += 7; fixup = 2; goto match; }
      if (t[7] == p0) { t += 8; fixup = 1; goto match; }
        t += 8;
        continue;
        do {
            if (*t++ == p0) {
              match:
                if (!InnerMatch::match(patNext, t, extent))
                    goto failed_match;
                return t - text - 1;
            }
          failed_match:;
        } while (--fixup > 0);
    }
    return -1;
}

static MOZ_ALWAYS_INLINE int
StringMatch(const jschar *text, uint32_t textlen,
            const jschar *pat, uint32_t patlen)
{
    if (patlen == 0)
        return 0;
    if (textlen < patlen)
        return -1;

#if defined(__i386__) || defined(_M_IX86) || defined(__i386)
    



    if (patlen == 1) {
        const jschar p0 = *pat;
        for (const jschar *c = text, *end = text + textlen; c != end; ++c) {
            if (*c == p0)
                return c - text;
        }
        return -1;
    }
#endif

    












    if (textlen >= 512 && patlen >= 11 && patlen <= sBMHPatLenMax) {
        int index = js_BoyerMooreHorspool(text, textlen, pat, patlen);
        if (index != sBMHBadPattern)
            return index;
    }

    





    return
#if !defined(__linux__)
           patlen > 128 ? UnrolledMatch<MemCmp>(text, textlen, pat, patlen)
                        :
#endif
                          UnrolledMatch<ManualCmp>(text, textlen, pat, patlen);
}

static const size_t sRopeMatchThresholdRatioLog2 = 5;

bool
js::StringHasPattern(const jschar *text, uint32_t textlen,
                     const jschar *pat, uint32_t patlen)
{
    return StringMatch(text, textlen, pat, patlen) != -1;
}

int
js::StringFindPattern(const jschar *text, uint32_t textlen,
                      const jschar *pat, uint32_t patlen)
{
    return StringMatch(text, textlen, pat, patlen);
}




class StringSegmentRange
{
    
    
    AutoStringVector stack;
    Rooted<JSLinearString*> cur;

    bool settle(JSString *str) {
        while (str->isRope()) {
            JSRope &rope = str->asRope();
            if (!stack.append(rope.rightChild()))
                return false;
            str = rope.leftChild();
        }
        cur = &str->asLinear();
        return true;
    }

  public:
    explicit StringSegmentRange(JSContext *cx)
      : stack(cx), cur(cx)
    {}

    MOZ_WARN_UNUSED_RESULT bool init(JSString *str) {
        JS_ASSERT(stack.empty());
        return settle(str);
    }

    bool empty() const {
        return cur == nullptr;
    }

    JSLinearString *front() const {
        JS_ASSERT(!cur->isRope());
        return cur;
    }

    MOZ_WARN_UNUSED_RESULT bool popFront() {
        JS_ASSERT(!empty());
        if (stack.empty()) {
            cur = nullptr;
            return true;
        }
        return settle(stack.popCopy());
    }
};






static bool
RopeMatch(JSContext *cx, JSString *textstr, const jschar *pat, uint32_t patlen, int *match)
{
    JS_ASSERT(textstr->isRope());

    if (patlen == 0) {
        *match = 0;
        return true;
    }
    if (textstr->length() < patlen) {
        *match = -1;
        return true;
    }

    




    Vector<JSLinearString *, 16, SystemAllocPolicy> strs;

    





    {
        size_t textstrlen = textstr->length();
        size_t threshold = textstrlen >> sRopeMatchThresholdRatioLog2;
        StringSegmentRange r(cx);
        if (!r.init(textstr))
            return false;
        while (!r.empty()) {
            if (threshold-- == 0 || !strs.append(r.front())) {
                const jschar *chars = textstr->getChars(cx);
                if (!chars)
                    return false;
                *match = StringMatch(chars, textstrlen, pat, patlen);
                return true;
            }
            if (!r.popFront())
                return false;
        }
    }

    
    int pos = 0;

    for (JSLinearString **outerp = strs.begin(); outerp != strs.end(); ++outerp) {
        
        JSLinearString *outer = *outerp;
        const jschar *chars = outer->chars();
        size_t len = outer->length();
        int matchResult = StringMatch(chars, len, pat, patlen);
        if (matchResult != -1) {
            
            *match = pos + matchResult;
            return true;
        }

        
        const jschar *const text = chars + (patlen > len ? 0 : len - patlen + 1);
        const jschar *const textend = chars + len;
        const jschar p0 = *pat;
        const jschar *const p1 = pat + 1;
        const jschar *const patend = pat + patlen;
        for (const jschar *t = text; t != textend; ) {
            if (*t++ != p0)
                continue;
            JSLinearString **innerp = outerp;
            const jschar *ttend = textend;
            for (const jschar *pp = p1, *tt = t; pp != patend; ++pp, ++tt) {
                while (tt == ttend) {
                    if (++innerp == strs.end()) {
                        *match = -1;
                        return true;
                    }
                    JSLinearString *inner = *innerp;
                    tt = inner->chars();
                    ttend = tt + inner->length();
                }
                if (*pp != *tt)
                    goto break_continue;
            }

            
            *match = pos + (t - chars) - 1;  
            return true;

          break_continue:;
        }

        pos += len;
    }

    *match = -1;
    return true;
}


static bool
str_contains(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    Rooted<JSLinearString*> searchStr(cx, ArgToRootedString(cx, args, 0));
    if (!searchStr)
        return false;

    
    uint32_t pos = 0;
    if (args.hasDefined(1)) {
        if (args[1].isInt32()) {
            int i = args[1].toInt32();
            pos = (i < 0) ? 0U : uint32_t(i);
        } else {
            double d;
            if (!ToInteger(cx, args[1], &d))
                return false;
            pos = uint32_t(Min(Max(d, 0.0), double(UINT32_MAX)));
        }
    }

    
    uint32_t textLen = str->length();
    const jschar *textChars = str->getChars(cx);
    if (!textChars)
        return false;

    
    uint32_t start = Min(Max(pos, 0U), textLen);

    
    uint32_t searchLen = searchStr->length();
    const jschar *searchChars = searchStr->chars();

    
    textChars += start;
    textLen -= start;
    int match = StringMatch(textChars, textLen, searchChars, searchLen);
    args.rval().setBoolean(match != -1);
    return true;
}


static bool
str_indexOf(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    Rooted<JSLinearString*> searchStr(cx, ArgToRootedString(cx, args, 0));
    if (!searchStr)
        return false;

    
    uint32_t pos = 0;
    if (args.hasDefined(1)) {
        if (args[1].isInt32()) {
            int i = args[1].toInt32();
            pos = (i < 0) ? 0U : uint32_t(i);
        } else {
            double d;
            if (!ToInteger(cx, args[1], &d))
                return false;
            pos = uint32_t(Min(Max(d, 0.0), double(UINT32_MAX)));
        }
    }

   
    uint32_t textLen = str->length();
    const jschar *textChars = str->getChars(cx);
    if (!textChars)
        return false;

    
    uint32_t start = Min(Max(pos, 0U), textLen);

    
    uint32_t searchLen = searchStr->length();
    const jschar *searchChars = searchStr->chars();

    
    textChars += start;
    textLen -= start;
    int match = StringMatch(textChars, textLen, searchChars, searchLen);
    args.rval().setInt32((match == -1) ? -1 : start + match);
    return true;
}

static bool
str_lastIndexOf(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString textstr(cx, ThisToStringForStringProto(cx, args));
    if (!textstr)
        return false;

    size_t textlen = textstr->length();

    Rooted<JSLinearString*> patstr(cx, ArgToRootedString(cx, args, 0));
    if (!patstr)
        return false;

    size_t patlen = patstr->length();

    int i = textlen - patlen; 
    if (i < 0) {
        args.rval().setInt32(-1);
        return true;
    }

    if (args.length() > 1) {
        if (args[1].isInt32()) {
            int j = args[1].toInt32();
            if (j <= 0)
                i = 0;
            else if (j < i)
                i = j;
        } else {
            double d;
            if (!ToNumber(cx, args[1], &d))
                return false;
            if (!IsNaN(d)) {
                d = ToInteger(d);
                if (d <= 0)
                    i = 0;
                else if (d < i)
                    i = (int)d;
            }
        }
    }

    if (patlen == 0) {
        args.rval().setInt32(i);
        return true;
    }

    const jschar *text = textstr->getChars(cx);
    if (!text)
        return false;

    const jschar *pat = patstr->chars();

    const jschar *t = text + i;
    const jschar *textend = text - 1;
    const jschar p0 = *pat;
    const jschar *patNext = pat + 1;
    const jschar *patEnd = pat + patlen;

    for (; t != textend; --t) {
        if (*t == p0) {
            const jschar *t1 = t + 1;
            for (const jschar *p1 = patNext; p1 != patEnd; ++p1, ++t1) {
                if (*t1 != *p1)
                    goto break_continue;
            }
            args.rval().setInt32(t - text);
            return true;
        }
      break_continue:;
    }

    args.rval().setInt32(-1);
    return true;
}


static bool
str_startsWith(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    if (args.get(0).isObject() && IsObjectWithClass(args[0], ESClass_RegExp, cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INVALID_ARG_TYPE,
                             "first", "", "Regular Expression");
        return false;
    }

    
    Rooted<JSLinearString*> searchStr(cx, ArgToRootedString(cx, args, 0));
    if (!searchStr)
        return false;

    
    uint32_t pos = 0;
    if (args.hasDefined(1)) {
        if (args[1].isInt32()) {
            int i = args[1].toInt32();
            pos = (i < 0) ? 0U : uint32_t(i);
        } else {
            double d;
            if (!ToInteger(cx, args[1], &d))
                return false;
            pos = uint32_t(Min(Max(d, 0.0), double(UINT32_MAX)));
        }
    }

    
    uint32_t textLen = str->length();
    const jschar *textChars = str->getChars(cx);
    if (!textChars)
        return false;

    
    uint32_t start = Min(Max(pos, 0U), textLen);

    
    uint32_t searchLen = searchStr->length();
    const jschar *searchChars = searchStr->chars();

    
    if (searchLen + start < searchLen || searchLen + start > textLen) {
        args.rval().setBoolean(false);
        return true;
    }

    
    args.rval().setBoolean(PodEqual(textChars + start, searchChars, searchLen));
    return true;
}


static bool
str_endsWith(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    if (args.get(0).isObject() && IsObjectWithClass(args[0], ESClass_RegExp, cx)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_INVALID_ARG_TYPE,
                             "first", "", "Regular Expression");
        return false;
    }

    
    Rooted<JSLinearString *> searchStr(cx, ArgToRootedString(cx, args, 0));
    if (!searchStr)
        return false;

    
    uint32_t textLen = str->length();
    const jschar *textChars = str->getChars(cx);
    if (!textChars)
        return false;

    
    uint32_t pos = textLen;
    if (args.hasDefined(1)) {
        if (args[1].isInt32()) {
            int i = args[1].toInt32();
            pos = (i < 0) ? 0U : uint32_t(i);
        } else {
            double d;
            if (!ToInteger(cx, args[1], &d))
                return false;
            pos = uint32_t(Min(Max(d, 0.0), double(UINT32_MAX)));
        }
    }

    
    uint32_t end = Min(Max(pos, 0U), textLen);

    
    uint32_t searchLen = searchStr->length();
    const jschar *searchChars = searchStr->chars();

    
    if (searchLen > end) {
        args.rval().setBoolean(false);
        return true;
    }

    
    uint32_t start = end - searchLen;

    
    args.rval().setBoolean(PodEqual(textChars + start, searchChars, searchLen));
    return true;
}

static bool
js_TrimString(JSContext *cx, Value *vp, bool trimLeft, bool trimRight)
{
    CallReceiver call = CallReceiverFromVp(vp);
    RootedString str(cx, ThisToStringForStringProto(cx, call));
    if (!str)
        return false;
    size_t length = str->length();
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return false;

    size_t begin = 0;
    size_t end = length;

    if (trimLeft) {
        while (begin < length && unicode::IsSpace(chars[begin]))
            ++begin;
    }

    if (trimRight) {
        while (end > begin && unicode::IsSpace(chars[end - 1]))
            --end;
    }

    str = js_NewDependentString(cx, str, begin, end - begin);
    if (!str)
        return false;

    call.rval().setString(str);
    return true;
}

static bool
str_trim(JSContext *cx, unsigned argc, Value *vp)
{
    return js_TrimString(cx, vp, true, true);
}

static bool
str_trimLeft(JSContext *cx, unsigned argc, Value *vp)
{
    return js_TrimString(cx, vp, true, false);
}

static bool
str_trimRight(JSContext *cx, unsigned argc, Value *vp)
{
    return js_TrimString(cx, vp, false, true);
}





namespace {


class FlatMatch
{
    RootedAtom patstr;
    const jschar *pat;
    size_t       patlen;
    int32_t      match_;

    friend class StringRegExpGuard;

  public:
    explicit FlatMatch(JSContext *cx) : patstr(cx) {}
    JSLinearString *pattern() const { return patstr; }
    size_t patternLength() const { return patlen; }

    



    int32_t match() const { return match_; }
};

} 

static inline bool
IsRegExpMetaChar(jschar c)
{
    switch (c) {
      
      case '^': case '$': case '\\': case '.': case '*': case '+':
      case '?': case '(': case ')': case '[': case ']': case '{':
      case '}': case '|':
        return true;
      default:
        return false;
    }
}

static inline bool
HasRegExpMetaChars(const jschar *chars, size_t length)
{
    for (size_t i = 0; i < length; ++i) {
        if (IsRegExpMetaChar(chars[i]))
            return true;
    }
    return false;
}

bool
js::StringHasRegExpMetaChars(const jschar *chars, size_t length)
{
    return HasRegExpMetaChars(chars, length);
}

namespace {







class MOZ_STACK_CLASS StringRegExpGuard
{
    RegExpGuard re_;
    FlatMatch   fm;
    RootedObject obj_;

    



    static const size_t MAX_FLAT_PAT_LEN = 256;

    static JSAtom *
    flattenPattern(JSContext *cx, JSAtom *patstr)
    {
        StringBuffer sb(cx);
        if (!sb.reserve(patstr->length()))
            return nullptr;

        static const jschar ESCAPE_CHAR = '\\';
        const jschar *chars = patstr->chars();
        size_t len = patstr->length();
        for (const jschar *it = chars; it != chars + len; ++it) {
            if (IsRegExpMetaChar(*it)) {
                if (!sb.append(ESCAPE_CHAR) || !sb.append(*it))
                    return nullptr;
            } else {
                if (!sb.append(*it))
                    return nullptr;
            }
        }
        return sb.finishAtom();
    }

  public:
    explicit StringRegExpGuard(JSContext *cx)
      : re_(cx), fm(cx), obj_(cx)
    { }

    
    bool init(JSContext *cx, CallArgs args, bool convertVoid = false)
    {
        if (args.length() != 0 && IsObjectWithClass(args[0], ESClass_RegExp, cx))
            return init(cx, &args[0].toObject());

        if (convertVoid && !args.hasDefined(0)) {
            fm.patstr = cx->runtime()->emptyString;
            return true;
        }

        JSString *arg = ArgToRootedString(cx, args, 0);
        if (!arg)
            return false;

        fm.patstr = AtomizeString(cx, arg);
        if (!fm.patstr)
            return false;

        return true;
    }

    bool init(JSContext *cx, JSObject *regexp) {
        obj_ = regexp;

        JS_ASSERT(ObjectClassIs(obj_, ESClass_RegExp, cx));

        if (!RegExpToShared(cx, obj_, &re_))
            return false;
        return true;
    }

    bool init(JSContext *cx, HandleString pattern) {
        fm.patstr = AtomizeString(cx, pattern);
        if (!fm.patstr)
            return false;
        return true;
    }

    










    const FlatMatch *
    tryFlatMatch(JSContext *cx, JSString *textstr, unsigned optarg, unsigned argc,
                 bool checkMetaChars = true)
    {
        if (re_.initialized())
            return nullptr;

        fm.pat = fm.patstr->chars();
        fm.patlen = fm.patstr->length();

        if (optarg < argc)
            return nullptr;

        if (checkMetaChars &&
            (fm.patlen > MAX_FLAT_PAT_LEN || HasRegExpMetaChars(fm.pat, fm.patlen))) {
            return nullptr;
        }

        



        if (textstr->isRope()) {
            if (!RopeMatch(cx, textstr, fm.pat, fm.patlen, &fm.match_))
                return nullptr;
        } else {
            const jschar *text = textstr->asLinear().chars();
            size_t textlen = textstr->length();
            fm.match_ = StringMatch(text, textlen, fm.pat, fm.patlen);
        }
        return &fm;
    }

    
    bool normalizeRegExp(JSContext *cx, bool flat, unsigned optarg, CallArgs args)
    {
        if (re_.initialized())
            return true;

        
        RootedString opt(cx);
        if (optarg < args.length()) {
            opt = ToString<CanGC>(cx, args[optarg]);
            if (!opt)
                return false;
        } else {
            opt = nullptr;
        }

        Rooted<JSAtom *> patstr(cx);
        if (flat) {
            patstr = flattenPattern(cx, fm.patstr);
            if (!patstr)
                return false;
        } else {
            patstr = fm.patstr;
        }
        JS_ASSERT(patstr);

        return cx->compartment()->regExps.get(cx, patstr, opt, &re_);
    }

    bool zeroLastIndex(JSContext *cx) {
        if (!regExpIsObject())
            return true;

        
        
        if (obj_->is<RegExpObject>() && obj_->nativeLookup(cx, cx->names().lastIndex)->writable()) {
            obj_->as<RegExpObject>().zeroLastIndex();
            return true;
        }

        
        RootedValue zero(cx, Int32Value(0));
        return JSObject::setProperty(cx, obj_, obj_, cx->names().lastIndex, &zero, true);
    }

    RegExpShared &regExp() { return *re_; }

    bool regExpIsObject() { return obj_ != nullptr; }
    HandleObject regExpObject() {
        JS_ASSERT(regExpIsObject());
        return obj_;
    }

  private:
    StringRegExpGuard(const StringRegExpGuard &) MOZ_DELETE;
    void operator=(const StringRegExpGuard &) MOZ_DELETE;
};

} 

static bool
DoMatchLocal(JSContext *cx, CallArgs args, RegExpStatics *res, Handle<JSLinearString*> input,
             RegExpShared &re)
{
    size_t charsLen = input->length();
    const jschar *chars = input->chars();

    size_t i = 0;
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
    RegExpRunStatus status = re.execute(cx, chars, charsLen, &i, matches);
    if (status == RegExpRunStatus_Error)
        return false;

    if (status == RegExpRunStatus_Success_NotFound) {
        args.rval().setNull();
        return true;
    }

    if (!res->updateFromMatchPairs(cx, input, matches))
        return false;

    RootedValue rval(cx);
    if (!CreateRegExpMatchResult(cx, input, matches, &rval))
        return false;

    args.rval().set(rval);
    return true;
}


static bool
DoMatchGlobal(JSContext *cx, CallArgs args, RegExpStatics *res, Handle<JSLinearString*> input,
              StringRegExpGuard &g)
{
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (!g.zeroLastIndex(cx))
        return false;

    
    AutoValueVector elements(cx);

    size_t lastSuccessfulStart = 0;

    
    

    
#ifdef JS_YARR
    MatchPair match;
#else
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
#endif
    size_t charsLen = input->length();
    const jschar *chars = input->chars();
    RegExpShared &re = g.regExp();
    for (size_t searchIndex = 0; searchIndex <= charsLen; ) {
        if (!CheckForInterrupt(cx))
            return false;

        
        size_t nextSearchIndex = searchIndex;
#ifdef JS_YARR
        RegExpRunStatus status = re.executeMatchOnly(cx, chars, charsLen, &nextSearchIndex, match);
#else
        RegExpRunStatus status = re.execute(cx, chars, charsLen, &nextSearchIndex, matches);
#endif
        if (status == RegExpRunStatus_Error)
            return false;

        
        if (status == RegExpRunStatus_Success_NotFound)
            break;

        lastSuccessfulStart = searchIndex;

#ifndef JS_YARR
        MatchPair &match = matches[0];
#endif

        
        searchIndex = match.isEmpty() ? nextSearchIndex + 1 : nextSearchIndex;

        
        JSLinearString *str = js_NewDependentString(cx, input, match.start, match.length());
        if (!str)
            return false;
        if (!elements.append(StringValue(str)))
            return false;
    }

    
    if (elements.empty()) {
        args.rval().setNull();
        return true;
    }

    
    
    
    res->updateLazily(cx, input, &re, lastSuccessfulStart);

    
    JSObject *array = NewDenseCopiedArray(cx, elements.length(), elements.begin());
    if (!array)
        return false;

    args.rval().setObject(*array);
    return true;
}

static bool
BuildFlatMatchArray(JSContext *cx, HandleString textstr, const FlatMatch &fm, CallArgs *args)
{
    if (fm.match() < 0) {
        args->rval().setNull();
        return true;
    }

    
    RootedObject obj(cx, NewDenseEmptyArray(cx));
    if (!obj)
        return false;

    RootedValue patternVal(cx, StringValue(fm.pattern()));
    RootedValue matchVal(cx, Int32Value(fm.match()));
    RootedValue textVal(cx, StringValue(textstr));

    if (!JSObject::defineElement(cx, obj, 0, patternVal) ||
        !JSObject::defineProperty(cx, obj, cx->names().index, matchVal) ||
        !JSObject::defineProperty(cx, obj, cx->names().input, textVal))
    {
        return false;
    }

    args->rval().setObject(*obj);
    return true;
}


bool
js::str_match(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    StringRegExpGuard g(cx);
    if (!g.init(cx, args, true))
        return false;

    
    if (const FlatMatch *fm = g.tryFlatMatch(cx, str, 1, args.length()))
        return BuildFlatMatchArray(cx, str, *fm, &args);

    
    if (cx->isExceptionPending())
        return false;

    
    if (!g.normalizeRegExp(cx, false, 1, args))
        return false;

    RegExpStatics *res = cx->global()->getRegExpStatics(cx);
    if (!res)
        return false;

    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;

    
    if (!g.regExp().global())
        return DoMatchLocal(cx, args, res, linearStr, g.regExp());

    
    return DoMatchGlobal(cx, args, res, linearStr, g);
}

bool
js::str_search(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    StringRegExpGuard g(cx);
    if (!g.init(cx, args, true))
        return false;
    if (const FlatMatch *fm = g.tryFlatMatch(cx, str, 1, args.length())) {
        args.rval().setInt32(fm->match());
        return true;
    }

    if (cx->isExceptionPending())  
        return false;

    if (!g.normalizeRegExp(cx, false, 1, args))
        return false;

    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;

    const jschar *chars = linearStr->chars();
    size_t length = linearStr->length();
    RegExpStatics *res = cx->global()->getRegExpStatics(cx);
    if (!res)
        return false;

    
    size_t i = 0;
#ifdef JS_YARR
    MatchPair match;
    RegExpRunStatus status = g.regExp().executeMatchOnly(cx, chars, length, &i, match);
#else
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
    RegExpRunStatus status = g.regExp().execute(cx, chars, length, &i, matches);
#endif
    if (status == RegExpRunStatus_Error)
        return false;

    if (status == RegExpRunStatus_Success)
        res->updateLazily(cx, linearStr, &g.regExp(), 0);

#ifdef JS_YARR
    JS_ASSERT_IF(status == RegExpRunStatus_Success_NotFound, match.start == -1);
    args.rval().setInt32(match.start);
#else
    args.rval().setInt32(status == RegExpRunStatus_Success_NotFound ? -1 : matches[0].start);
#endif
    return true;
}


class RopeBuilder {
    JSContext *cx;
    RootedString res;

    RopeBuilder(const RopeBuilder &other) MOZ_DELETE;
    void operator=(const RopeBuilder &other) MOZ_DELETE;

  public:
    explicit RopeBuilder(JSContext *cx)
      : cx(cx), res(cx, cx->runtime()->emptyString)
    {}

    inline bool append(HandleString str) {
        res = ConcatStrings<CanGC>(cx, res, str);
        return !!res;
    }

    inline JSString *result() {
        return res;
    }
};

namespace {

struct ReplaceData
{
    explicit ReplaceData(JSContext *cx)
      : str(cx), g(cx), lambda(cx), elembase(cx), repstr(cx),
        fig(cx, NullValue()), sb(cx)
    {}

    inline void setReplacementString(JSLinearString *string) {
        JS_ASSERT(string);
        lambda = nullptr;
        elembase = nullptr;
        repstr = string;

        const jschar *chars = repstr->chars();
        if (const jschar *p = js_strchr_limit(chars, '$', chars + repstr->length())) {
            dollarIndex = p - chars;
            MOZ_ASSERT(dollarIndex < repstr->length());
        } else {
            dollarIndex = UINT32_MAX;
        }
    }

    inline void setReplacementFunction(JSObject *func) {
        JS_ASSERT(func);
        lambda = func;
        elembase = nullptr;
        repstr = nullptr;
        dollarIndex = UINT32_MAX;
    }

    RootedString       str;            
    StringRegExpGuard  g;              
    RootedObject       lambda;         
    RootedObject       elembase;       
    Rooted<JSLinearString*> repstr;    
    uint32_t           dollarIndex;    
    int                leftIndex;      
    JSSubString        dollarStr;      
    bool               calledBack;     
    FastInvokeGuard    fig;            
    StringBuffer       sb;             
};

} 

static bool
ReplaceRegExp(JSContext *cx, RegExpStatics *res, ReplaceData &rdata);

static bool
DoMatchForReplaceLocal(JSContext *cx, RegExpStatics *res, Handle<JSLinearString*> linearStr,
                       RegExpShared &re, ReplaceData &rdata)
{
    size_t charsLen = linearStr->length();
    size_t i = 0;
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
    RegExpRunStatus status = re.execute(cx, linearStr->chars(), charsLen, &i, matches);
    if (status == RegExpRunStatus_Error)
        return false;

    if (status == RegExpRunStatus_Success_NotFound)
        return true;

    if (!res->updateFromMatchPairs(cx, linearStr, matches))
        return false;

    return ReplaceRegExp(cx, res, rdata);
}

static bool
DoMatchForReplaceGlobal(JSContext *cx, RegExpStatics *res, Handle<JSLinearString*> linearStr,
                        RegExpShared &re, ReplaceData &rdata)
{
    size_t charsLen = linearStr->length();
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
    for (size_t count = 0, i = 0; i <= charsLen; ++count) {
        if (!CheckForInterrupt(cx))
            return false;

        RegExpRunStatus status = re.execute(cx, linearStr->chars(), charsLen, &i, matches);
        if (status == RegExpRunStatus_Error)
            return false;

        if (status == RegExpRunStatus_Success_NotFound)
            break;

        if (!res->updateFromMatchPairs(cx, linearStr, matches))
            return false;

        if (!ReplaceRegExp(cx, res, rdata))
            return false;
        if (!res->matched())
            ++i;
    }

    return true;
}

static bool
InterpretDollar(RegExpStatics *res, const jschar *dp, const jschar *ep,
                ReplaceData &rdata, JSSubString *out, size_t *skip)
{
    JS_ASSERT(*dp == '$');

    
    if (dp + 1 >= ep)
        return false;

    
    jschar dc = dp[1];
    if (JS7_ISDEC(dc)) {
        
        unsigned num = JS7_UNDEC(dc);
        if (num > res->getMatches().parenCount())
            return false;

        const jschar *cp = dp + 2;
        if (cp < ep && (dc = *cp, JS7_ISDEC(dc))) {
            unsigned tmp = 10 * num + JS7_UNDEC(dc);
            if (tmp <= res->getMatches().parenCount()) {
                cp++;
                num = tmp;
            }
        }
        if (num == 0)
            return false;

        *skip = cp - dp;

        JS_ASSERT(num <= res->getMatches().parenCount());

        



        res->getParen(num, out);
        return true;
    }

    *skip = 2;
    switch (dc) {
      case '$':
        rdata.dollarStr.chars = dp;
        rdata.dollarStr.length = 1;
        *out = rdata.dollarStr;
        return true;
      case '&':
        res->getLastMatch(out);
        return true;
      case '+':
        res->getLastParen(out);
        return true;
      case '`':
        res->getLeftContext(out);
        return true;
      case '\'':
        res->getRightContext(out);
        return true;
    }
    return false;
}

static bool
FindReplaceLength(JSContext *cx, RegExpStatics *res, ReplaceData &rdata, size_t *sizep)
{
    if (rdata.elembase) {
        





        JS_ASSERT(rdata.lambda);
        JS_ASSERT(!rdata.elembase->getOps()->lookupProperty);
        JS_ASSERT(!rdata.elembase->getOps()->getProperty);

        RootedValue match(cx);
        if (!res->createLastMatch(cx, &match))
            return false;
        JSAtom *atom = ToAtom<CanGC>(cx, match);
        if (!atom)
            return false;

        RootedValue v(cx);
        if (HasDataProperty(cx, rdata.elembase, AtomToId(atom), v.address()) && v.isString()) {
            rdata.repstr = v.toString()->ensureLinear(cx);
            if (!rdata.repstr)
                return false;
            *sizep = rdata.repstr->length();
            return true;
        }

        



        rdata.elembase = nullptr;
    }

    if (rdata.lambda) {
        RootedObject lambda(cx, rdata.lambda);
        PreserveRegExpStatics staticsGuard(cx, res);
        if (!staticsGuard.init(cx))
            return false;

        







        unsigned p = res->getMatches().parenCount();
        unsigned argc = 1 + p + 2;

        InvokeArgs &args = rdata.fig.args();
        if (!args.init(argc))
            return false;

        args.setCallee(ObjectValue(*lambda));
        args.setThis(UndefinedValue());

        
        unsigned argi = 0;
        if (!res->createLastMatch(cx, args[argi++]))
            return false;

        for (size_t i = 0; i < res->getMatches().parenCount(); ++i) {
            if (!res->createParen(cx, i + 1, args[argi++]))
                return false;
        }

        
        args[argi++].setInt32(res->getMatches()[0].start);
        args[argi].setString(rdata.str);

        if (!rdata.fig.invoke(cx))
            return false;

        
        JSString *repstr = ToString<CanGC>(cx, args.rval());
        if (!repstr)
            return false;
        rdata.repstr = repstr->ensureLinear(cx);
        if (!rdata.repstr)
            return false;
        *sizep = rdata.repstr->length();
        return true;
    }

    JSLinearString *repstr = rdata.repstr;
    CheckedInt<uint32_t> replen = repstr->length();
    if (rdata.dollarIndex != UINT32_MAX) {
        MOZ_ASSERT(rdata.dollarIndex < repstr->length());
        const jschar *dp = repstr->chars() + rdata.dollarIndex;
        const jschar *ep = repstr->chars() + repstr->length();
        do {
            JSSubString sub;
            size_t skip;
            if (InterpretDollar(res, dp, ep, rdata, &sub, &skip)) {
                if (sub.length > skip)
                    replen += sub.length - skip;
                else
                    replen -= skip - sub.length;
                dp += skip;
            } else {
                dp++;
            }

            dp = js_strchr_limit(dp, '$', ep);
        } while (dp);
    }

    if (!replen.isValid()) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    *sizep = replen.value();
    return true;
}





static void
DoReplace(RegExpStatics *res, ReplaceData &rdata)
{
    JSLinearString *repstr = rdata.repstr;
    const jschar *bp = repstr->chars();
    const jschar *cp = bp;

    if (rdata.dollarIndex != UINT32_MAX) {
        MOZ_ASSERT(rdata.dollarIndex < repstr->length());
        const jschar *dp = bp + rdata.dollarIndex;
        const jschar *ep = bp + repstr->length();
        do {
            
            size_t len = dp - cp;
            rdata.sb.infallibleAppend(cp, len);
            cp = dp;

            JSSubString sub;
            size_t skip;
            if (InterpretDollar(res, dp, ep, rdata, &sub, &skip)) {
                len = sub.length;
                rdata.sb.infallibleAppend(sub.chars, len);
                cp += skip;
                dp += skip;
            } else {
                dp++;
            }

            dp = js_strchr_limit(dp, '$', ep);
        } while (dp);
    }
    rdata.sb.infallibleAppend(cp, repstr->length() - (cp - bp));
}

static bool
ReplaceRegExp(JSContext *cx, RegExpStatics *res, ReplaceData &rdata)
{

    const MatchPair &match = res->getMatches()[0];
    JS_ASSERT(!match.isUndefined());
    JS_ASSERT(match.limit >= match.start && match.limit >= 0);

    rdata.calledBack = true;
    size_t leftoff = rdata.leftIndex;
    size_t leftlen = match.start - leftoff;
    rdata.leftIndex = match.limit;

    size_t replen = 0;  
    if (!FindReplaceLength(cx, res, rdata, &replen))
        return false;

    CheckedInt<uint32_t> newlen(rdata.sb.length());
    newlen += leftlen;
    newlen += replen;
    if (!newlen.isValid()) {
        js_ReportAllocationOverflow(cx);
        return false;
    }
    if (!rdata.sb.reserve(newlen.value()))
        return false;

    JSLinearString &str = rdata.str->asLinear();  
    const jschar *left = str.chars() + leftoff;

    rdata.sb.infallibleAppend(left, leftlen); 
    DoReplace(res, rdata);
    return true;
}

static bool
BuildFlatReplacement(JSContext *cx, HandleString textstr, HandleString repstr,
                     const FlatMatch &fm, MutableHandleValue rval)
{
    RopeBuilder builder(cx);
    size_t match = fm.match();
    size_t matchEnd = match + fm.patternLength();

    if (textstr->isRope()) {
        



        StringSegmentRange r(cx);
        if (!r.init(textstr))
            return false;
        size_t pos = 0;
        while (!r.empty()) {
            RootedString str(cx, r.front());
            size_t len = str->length();
            size_t strEnd = pos + len;
            if (pos < matchEnd && strEnd > match) {
                



                if (match >= pos) {
                    





                    RootedString leftSide(cx, js_NewDependentString(cx, str, 0, match - pos));
                    if (!leftSide ||
                        !builder.append(leftSide) ||
                        !builder.append(repstr)) {
                        return false;
                    }
                }

                



                if (strEnd > matchEnd) {
                    RootedString rightSide(cx, js_NewDependentString(cx, str, matchEnd - pos,
                                                                     strEnd - matchEnd));
                    if (!rightSide || !builder.append(rightSide))
                        return false;
                }
            } else {
                if (!builder.append(str))
                    return false;
            }
            pos += str->length();
            if (!r.popFront())
                return false;
        }
    } else {
        RootedString leftSide(cx, js_NewDependentString(cx, textstr, 0, match));
        if (!leftSide)
            return false;
        RootedString rightSide(cx);
        rightSide = js_NewDependentString(cx, textstr, match + fm.patternLength(),
                                          textstr->length() - match - fm.patternLength());
        if (!rightSide ||
            !builder.append(leftSide) ||
            !builder.append(repstr) ||
            !builder.append(rightSide)) {
            return false;
        }
    }

    rval.setString(builder.result());
    return true;
}







static inline bool
BuildDollarReplacement(JSContext *cx, JSString *textstrArg, JSLinearString *repstr,
                       uint32_t firstDollarIndex, const FlatMatch &fm, MutableHandleValue rval)
{
    Rooted<JSLinearString*> textstr(cx, textstrArg->ensureLinear(cx));
    if (!textstr)
        return false;

    size_t matchStart = fm.match();
    size_t matchLimit = matchStart + fm.patternLength();

    






    StringBuffer newReplaceChars(cx);
    if (!newReplaceChars.reserve(textstr->length() - fm.patternLength() + repstr->length()))
        return false;

    JS_ASSERT(firstDollarIndex < repstr->length());
    const jschar *firstDollar = repstr->chars() + firstDollarIndex;

    
    newReplaceChars.infallibleAppend(repstr->chars(), firstDollar);

    
    const jschar *textchars = textstr->chars();
    const jschar *repstrLimit = repstr->chars() + repstr->length();
    for (const jschar *it = firstDollar; it < repstrLimit; ++it) {
        if (*it != '$' || it == repstrLimit - 1) {
            if (!newReplaceChars.append(*it))
                return false;
            continue;
        }

        switch (*(it + 1)) {
          case '$': 
            if (!newReplaceChars.append(*it))
                return false;
            break;
          case '&':
            if (!newReplaceChars.append(textchars + matchStart, textchars + matchLimit))
                return false;
            break;
          case '`':
            if (!newReplaceChars.append(textchars, textchars + matchStart))
                return false;
            break;
          case '\'':
            if (!newReplaceChars.append(textchars + matchLimit, textchars + textstr->length()))
                return false;
            break;
          default: 
            if (!newReplaceChars.append(*it))
                return false;
            continue;
        }
        ++it; 
    }

    RootedString leftSide(cx, js_NewDependentString(cx, textstr, 0, matchStart));
    if (!leftSide)
        return false;

    RootedString newReplace(cx, newReplaceChars.finishString());
    if (!newReplace)
        return false;

    JS_ASSERT(textstr->length() >= matchLimit);
    RootedString rightSide(cx, js_NewDependentString(cx, textstr, matchLimit,
                                                     textstr->length() - matchLimit));
    if (!rightSide)
        return false;

    RopeBuilder builder(cx);
    if (!builder.append(leftSide) || !builder.append(newReplace) || !builder.append(rightSide))
        return false;

    rval.setString(builder.result());
    return true;
}

struct StringRange
{
    size_t start;
    size_t length;

    StringRange(size_t s, size_t l)
      : start(s), length(l)
    { }
};

static inline JSFatInlineString *
FlattenSubstrings(JSContext *cx, const jschar *chars,
                  const StringRange *ranges, size_t rangesLen, size_t outputLen)
{
    JS_ASSERT(JSFatInlineString::twoByteLengthFits(outputLen));

    JSFatInlineString *str = js_NewGCFatInlineString<CanGC>(cx);
    if (!str)
        return nullptr;

    jschar *buf = str->initTwoByte(outputLen);
    size_t pos = 0;
    for (size_t i = 0; i < rangesLen; i++) {
        PodCopy(buf + pos, chars + ranges[i].start, ranges[i].length);
        pos += ranges[i].length;
    }
    JS_ASSERT(pos == outputLen);

    buf[outputLen] = 0;
    return str;
}

static JSString *
AppendSubstrings(JSContext *cx, Handle<JSFlatString*> flatStr,
                 const StringRange *ranges, size_t rangesLen)
{
    JS_ASSERT(rangesLen);

    
    if (rangesLen == 1)
        return js_NewDependentString(cx, flatStr, ranges[0].start, ranges[0].length);

    const jschar *chars = flatStr->getChars(cx);
    if (!chars)
        return nullptr;

    
    size_t i = 0;
    RopeBuilder rope(cx);
    RootedString part(cx, nullptr);
    while (i < rangesLen) {

        
        size_t substrLen = 0;
        size_t end = i;
        for (; end < rangesLen; end++) {
            if (substrLen + ranges[end].length > JSFatInlineString::MAX_LENGTH_TWO_BYTE)
                break;
            substrLen += ranges[end].length;
        }

        if (i == end) {
            
            const StringRange &sr = ranges[i++];
            part = js_NewDependentString(cx, flatStr, sr.start, sr.length);
        } else {
            
            part = FlattenSubstrings(cx, chars, ranges + i, end - i, substrLen);
            i = end;
        }

        if (!part)
            return nullptr;

        
        if (!rope.append(part))
            return nullptr;
    }

    return rope.result();
}

static bool
StrReplaceRegexpRemove(JSContext *cx, HandleString str, RegExpShared &re, MutableHandleValue rval)
{
    Rooted<JSFlatString*> flatStr(cx, str->ensureFlat(cx));
    if (!flatStr)
        return false;

    Vector<StringRange, 16, SystemAllocPolicy> ranges;

    size_t charsLen = flatStr->length();

#ifdef JS_YARR
    MatchPair match;
#else
    ScopedMatchPairs matches(&cx->tempLifoAlloc());
#endif
    size_t startIndex = 0; 
    size_t lastIndex = 0;  
    size_t lazyIndex = 0;  

    
    while (startIndex <= charsLen) {
        if (!CheckForInterrupt(cx))
            return false;

#ifdef JS_YARR
        RegExpRunStatus status = re.executeMatchOnly(cx, flatStr->chars(), charsLen, &startIndex, match);
#else
        RegExpRunStatus status = re.execute(cx, flatStr->chars(), charsLen, &startIndex, matches);
#endif
        if (status == RegExpRunStatus_Error)
            return false;
        if (status == RegExpRunStatus_Success_NotFound)
            break;

#ifndef JS_YARR
        MatchPair &match = matches[0];
#endif

        
        if (size_t(match.start) > lastIndex) {
            if (!ranges.append(StringRange(lastIndex, match.start - lastIndex)))
                return false;
        }

        lazyIndex = lastIndex;
        lastIndex = startIndex;

        if (match.isEmpty())
            startIndex++;

        
        if (!re.global())
            break;
    }

    RegExpStatics *res;

    
    if (!lastIndex) {
        if (startIndex > 0) {
            res = cx->global()->getRegExpStatics(cx);
            if (!res)
                return false;
            res->updateLazily(cx, flatStr, &re, lazyIndex);
        }
        rval.setString(str);
        return true;
    }

    
    res = cx->global()->getRegExpStatics(cx);
    if (!res)
        return false;

    res->updateLazily(cx, flatStr, &re, lazyIndex);

    
    if (lastIndex < charsLen) {
        if (!ranges.append(StringRange(lastIndex, charsLen - lastIndex)))
            return false;
    }

    
    if (ranges.empty()) {
        rval.setString(cx->runtime()->emptyString);
        return true;
    }

    JSString *result = AppendSubstrings(cx, flatStr, ranges.begin(), ranges.length());
    if (!result)
        return false;

    rval.setString(result);
    return true;
}

static inline bool
StrReplaceRegExp(JSContext *cx, ReplaceData &rdata, MutableHandleValue rval)
{
    rdata.leftIndex = 0;
    rdata.calledBack = false;

    RegExpStatics *res = cx->global()->getRegExpStatics(cx);
    if (!res)
        return false;

    RegExpShared &re = rdata.g.regExp();

    
    
    
    
    
    
    if (re.global() && !rdata.g.zeroLastIndex(cx))
        return false;

    
    if (rdata.repstr && rdata.repstr->length() == 0) {
        JS_ASSERT(!rdata.lambda && !rdata.elembase && rdata.dollarIndex == UINT32_MAX);
        return StrReplaceRegexpRemove(cx, rdata.str, re, rval);
    }

    Rooted<JSLinearString*> linearStr(cx, rdata.str->ensureLinear(cx));
    if (!linearStr)
        return false;

    if (re.global()) {
        if (!DoMatchForReplaceGlobal(cx, res, linearStr, re, rdata))
            return false;
    } else {
        if (!DoMatchForReplaceLocal(cx, res, linearStr, re, rdata))
            return false;
    }

    if (!rdata.calledBack) {
        
        rval.setString(rdata.str);
        return true;
    }

    JSSubString sub;
    res->getRightContext(&sub);
    if (!rdata.sb.append(sub.chars, sub.length))
        return false;

    JSString *retstr = rdata.sb.finishString();
    if (!retstr)
        return false;

    rval.setString(retstr);
    return true;
}

static inline bool
str_replace_regexp(JSContext *cx, CallArgs args, ReplaceData &rdata)
{
    if (!rdata.g.normalizeRegExp(cx, true, 2, args))
        return false;

    return StrReplaceRegExp(cx, rdata, args.rval());
}

bool
js::str_replace_regexp_raw(JSContext *cx, HandleString string, HandleObject regexp,
                       HandleString replacement, MutableHandleValue rval)
{
    
    if (replacement->length() == 0) {
        StringRegExpGuard guard(cx);
        if (!guard.init(cx, regexp))
            return false;

        RegExpShared &re = guard.regExp();
        return StrReplaceRegexpRemove(cx, string, re, rval);
    }

    ReplaceData rdata(cx);
    rdata.str = string;

    JSLinearString *repl = replacement->ensureLinear(cx);
    if (!repl)
        return false;

    rdata.setReplacementString(repl);

    if (!rdata.g.init(cx, regexp))
        return false;

    return StrReplaceRegExp(cx, rdata, rval);
}

static inline bool
StrReplaceString(JSContext *cx, ReplaceData &rdata, const FlatMatch &fm, MutableHandleValue rval)
{
    



    if (rdata.dollarIndex != UINT32_MAX)
        return BuildDollarReplacement(cx, rdata.str, rdata.repstr, rdata.dollarIndex, fm, rval);
    return BuildFlatReplacement(cx, rdata.str, rdata.repstr, fm, rval);
}

static const uint32_t ReplaceOptArg = 2;

bool
js::str_replace_string_raw(JSContext *cx, HandleString string, HandleString pattern,
                          HandleString replacement, MutableHandleValue rval)
{
    ReplaceData rdata(cx);

    rdata.str = string;
    JSLinearString *repl = replacement->ensureLinear(cx);
    if (!repl)
        return false;
    rdata.setReplacementString(repl);

    if (!rdata.g.init(cx, pattern))
        return false;
    const FlatMatch *fm = rdata.g.tryFlatMatch(cx, rdata.str, ReplaceOptArg, ReplaceOptArg, false);

    if (fm->match() < 0) {
        rval.setString(string);
        return true;
    }

    return StrReplaceString(cx, rdata, *fm, rval);
}

static inline bool
str_replace_flat_lambda(JSContext *cx, CallArgs outerArgs, ReplaceData &rdata, const FlatMatch &fm)
{
    RootedString matchStr(cx, js_NewDependentString(cx, rdata.str, fm.match(), fm.patternLength()));
    if (!matchStr)
        return false;

    
    static const uint32_t lambdaArgc = 3;
    if (!rdata.fig.args().init(lambdaArgc))
        return false;

    CallArgs &args = rdata.fig.args();
    args.setCallee(ObjectValue(*rdata.lambda));
    args.setThis(UndefinedValue());

    Value *sp = args.array();
    sp[0].setString(matchStr);
    sp[1].setInt32(fm.match());
    sp[2].setString(rdata.str);

    if (!rdata.fig.invoke(cx))
        return false;

    RootedString repstr(cx, ToString<CanGC>(cx, args.rval()));
    if (!repstr)
        return false;

    RootedString leftSide(cx, js_NewDependentString(cx, rdata.str, 0, fm.match()));
    if (!leftSide)
        return false;

    size_t matchLimit = fm.match() + fm.patternLength();
    RootedString rightSide(cx, js_NewDependentString(cx, rdata.str, matchLimit,
                                                        rdata.str->length() - matchLimit));
    if (!rightSide)
        return false;

    RopeBuilder builder(cx);
    if (!(builder.append(leftSide) &&
          builder.append(repstr) &&
          builder.append(rightSide))) {
        return false;
    }

    outerArgs.rval().setString(builder.result());
    return true;
}








static bool
LambdaIsGetElem(JSContext *cx, JSObject &lambda, MutableHandleObject pobj)
{
    if (!lambda.is<JSFunction>())
        return true;

    RootedFunction fun(cx, &lambda.as<JSFunction>());
    if (!fun->isInterpreted())
        return true;

    JSScript *script = fun->getOrCreateScript(cx);
    if (!script)
        return false;

    jsbytecode *pc = script->code();

    




    if (JSOp(*pc) != JSOP_GETALIASEDVAR || fun->isHeavyweight())
        return true;
    ScopeCoordinate sc(pc);
    ScopeObject *scope = &fun->environment()->as<ScopeObject>();
    for (unsigned i = 0; i < sc.hops(); ++i)
        scope = &scope->enclosingScope().as<ScopeObject>();
    Value b = scope->aliasedVar(sc);
    pc += JSOP_GETALIASEDVAR_LENGTH;

    
    if (JSOp(*pc) != JSOP_GETARG || GET_ARGNO(pc) != 0)
        return true;
    pc += JSOP_GETARG_LENGTH;

    
    if (JSOp(*pc) != JSOP_GETELEM)
        return true;
    pc += JSOP_GETELEM_LENGTH;

    
    if (JSOp(*pc) != JSOP_RETURN)
        return true;

    
    if (!b.isObject())
        return true;

    JSObject &bobj = b.toObject();
    const Class *clasp = bobj.getClass();
    if (!clasp->isNative() || clasp->ops.lookupProperty || clasp->ops.getProperty)
        return true;

    pobj.set(&bobj);
    return true;
}

bool
js::str_replace(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    ReplaceData rdata(cx);
    rdata.str = ThisToStringForStringProto(cx, args);
    if (!rdata.str)
        return false;

    if (!rdata.g.init(cx, args))
        return false;

    
    if (args.length() >= ReplaceOptArg && js_IsCallable(args[1])) {
        rdata.setReplacementFunction(&args[1].toObject());

        if (!LambdaIsGetElem(cx, *rdata.lambda, &rdata.elembase))
            return false;
    } else {
        JSLinearString *string = ArgToRootedString(cx, args, 1);
        if (!string)
            return false;

        rdata.setReplacementString(string);
    }

    rdata.fig.initFunction(ObjectOrNullValue(rdata.lambda));

    









    const FlatMatch *fm = rdata.g.tryFlatMatch(cx, rdata.str, ReplaceOptArg, args.length(), false);

    if (!fm) {
        if (cx->isExceptionPending())  
            return false;
        return str_replace_regexp(cx, args, rdata);
    }

    if (fm->match() < 0) {
        args.rval().setString(rdata.str);
        return true;
    }

    if (rdata.lambda)
        return str_replace_flat_lambda(cx, args, rdata, *fm);
    return StrReplaceString(cx, rdata, *fm, args.rval());
}

namespace {

class SplitMatchResult {
    size_t endIndex_;
    size_t length_;

  public:
    void setFailure() {
        JS_STATIC_ASSERT(SIZE_MAX > JSString::MAX_LENGTH);
        endIndex_ = SIZE_MAX;
    }
    bool isFailure() const {
        return endIndex_ == SIZE_MAX;
    }
    size_t endIndex() const {
        JS_ASSERT(!isFailure());
        return endIndex_;
    }
    size_t length() const {
        JS_ASSERT(!isFailure());
        return length_;
    }
    void setResult(size_t length, size_t endIndex) {
        length_ = length;
        endIndex_ = endIndex;
    }
};

} 

template<class Matcher>
static ArrayObject *
SplitHelper(JSContext *cx, Handle<JSLinearString*> str, uint32_t limit, const Matcher &splitMatch,
            Handle<TypeObject*> type)
{
    size_t strLength = str->length();
    SplitMatchResult result;

    
    if (strLength == 0) {
        if (!splitMatch(cx, str, 0, &result))
            return nullptr;

        









        if (!result.isFailure())
            return NewDenseEmptyArray(cx);

        RootedValue v(cx, StringValue(str));
        return NewDenseCopiedArray(cx, 1, v.address());
    }

    
    size_t lastEndIndex = 0;
    size_t index = 0;

    
    AutoValueVector splits(cx);

    while (index < strLength) {
        
        if (!splitMatch(cx, str, index, &result))
            return nullptr;

        














        if (result.isFailure())
            break;

        
        size_t sepLength = result.length();
        size_t endIndex = result.endIndex();
        if (sepLength == 0 && endIndex == strLength)
            break;

        
        if (endIndex == lastEndIndex) {
            index++;
            continue;
        }

        
        JS_ASSERT(lastEndIndex < endIndex);
        JS_ASSERT(sepLength <= strLength);
        JS_ASSERT(lastEndIndex + sepLength <= endIndex);

        
        size_t subLength = size_t(endIndex - sepLength - lastEndIndex);
        JSString *sub = js_NewDependentString(cx, str, lastEndIndex, subLength);
        if (!sub || !splits.append(StringValue(sub)))
            return nullptr;

        
        if (splits.length() == limit)
            return NewDenseCopiedArray(cx, splits.length(), splits.begin());

        
        lastEndIndex = endIndex;

        
        if (Matcher::returnsCaptures) {
            RegExpStatics *res = cx->global()->getRegExpStatics(cx);
            if (!res)
                return nullptr;

            const MatchPairs &matches = res->getMatches();
            for (size_t i = 0; i < matches.parenCount(); i++) {
                
                if (!matches[i + 1].isUndefined()) {
                    JSSubString parsub;
                    res->getParen(i + 1, &parsub);
                    sub = js_NewStringCopyN<CanGC>(cx, parsub.chars, parsub.length);
                    if (!sub || !splits.append(StringValue(sub)))
                        return nullptr;
                } else {
                    
                    AddTypePropertyId(cx, type, JSID_VOID, UndefinedValue());
                    if (!splits.append(UndefinedValue()))
                        return nullptr;
                }

                
                if (splits.length() == limit)
                    return NewDenseCopiedArray(cx, splits.length(), splits.begin());
            }
        }

        
        index = lastEndIndex;
    }

    
    JSString *sub = js_NewDependentString(cx, str, lastEndIndex, strLength - lastEndIndex);
    if (!sub || !splits.append(StringValue(sub)))
        return nullptr;

    
    return NewDenseCopiedArray(cx, splits.length(), splits.begin());
}


static ArrayObject *
CharSplitHelper(JSContext *cx, Handle<JSLinearString*> str, uint32_t limit)
{
    size_t strLength = str->length();
    if (strLength == 0)
        return NewDenseEmptyArray(cx);

    js::StaticStrings &staticStrings = cx->staticStrings();
    uint32_t resultlen = (limit < strLength ? limit : strLength);

    AutoValueVector splits(cx);
    if (!splits.reserve(resultlen))
        return nullptr;

    for (size_t i = 0; i < resultlen; ++i) {
        JSString *sub = staticStrings.getUnitStringForElement(cx, str, i);
        if (!sub)
            return nullptr;
        splits.infallibleAppend(StringValue(sub));
    }

    return NewDenseCopiedArray(cx, splits.length(), splits.begin());
}

namespace {








class SplitRegExpMatcher
{
    RegExpShared &re;
    RegExpStatics *res;

  public:
    SplitRegExpMatcher(RegExpShared &re, RegExpStatics *res) : re(re), res(res) {}

    static const bool returnsCaptures = true;

    bool operator()(JSContext *cx, Handle<JSLinearString*> str, size_t index,
                    SplitMatchResult *result) const
    {
        const jschar *chars = str->chars();
        size_t length = str->length();

        ScopedMatchPairs matches(&cx->tempLifoAlloc());
        RegExpRunStatus status = re.execute(cx, chars, length, &index, matches);
        if (status == RegExpRunStatus_Error)
            return false;

        if (status == RegExpRunStatus_Success_NotFound) {
            result->setFailure();
            return true;
        }

        if (!res->updateFromMatchPairs(cx, str, matches))
            return false;

        JSSubString sep;
        res->getLastMatch(&sep);

        result->setResult(sep.length, index);
        return true;
    }
};

class SplitStringMatcher
{
    Rooted<JSLinearString*> sep;

  public:
    SplitStringMatcher(JSContext *cx, HandleLinearString sep)
      : sep(cx, sep)
    {}

    static const bool returnsCaptures = false;

    bool operator()(JSContext *cx, JSLinearString *str, size_t index, SplitMatchResult *res) const
    {
        JS_ASSERT(index == 0 || index < str->length());
        const jschar *chars = str->chars();
        int match = StringMatch(chars + index, str->length() - index,
                                sep->chars(), sep->length());
        if (match == -1)
            res->setFailure();
        else
            res->setResult(sep->length(), index + match + sep->length());
        return true;
    }
};

} 


bool
js::str_split(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    RootedTypeObject type(cx, GetTypeCallerInitObject(cx, JSProto_Array));
    if (!type)
        return false;
    AddTypePropertyId(cx, type, JSID_VOID, Type::StringType());

    
    uint32_t limit;
    if (args.hasDefined(1)) {
        double d;
        if (!ToNumber(cx, args[1], &d))
            return false;
        limit = ToUint32(d);
    } else {
        limit = UINT32_MAX;
    }

    
    RegExpGuard re(cx);
    RootedLinearString sepstr(cx);
    bool sepDefined = args.hasDefined(0);
    if (sepDefined) {
        if (IsObjectWithClass(args[0], ESClass_RegExp, cx)) {
            RootedObject obj(cx, &args[0].toObject());
            if (!RegExpToShared(cx, obj, &re))
                return false;
        } else {
            sepstr = ArgToRootedString(cx, args, 0);
            if (!sepstr)
                return false;
        }
    }

    
    if (limit == 0) {
        JSObject *aobj = NewDenseEmptyArray(cx);
        if (!aobj)
            return false;
        aobj->setType(type);
        args.rval().setObject(*aobj);
        return true;
    }

    
    if (!sepDefined) {
        RootedValue v(cx, StringValue(str));
        JSObject *aobj = NewDenseCopiedArray(cx, 1, v.address());
        if (!aobj)
            return false;
        aobj->setType(type);
        args.rval().setObject(*aobj);
        return true;
    }
    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;

    
    RootedObject aobj(cx);
    if (!re.initialized()) {
        if (sepstr->length() == 0) {
            aobj = CharSplitHelper(cx, linearStr, limit);
        } else {
            SplitStringMatcher matcher(cx, sepstr);
            aobj = SplitHelper(cx, linearStr, limit, matcher, type);
        }
    } else {
        RegExpStatics *res = cx->global()->getRegExpStatics(cx);
        if (!res)
            return false;
        SplitRegExpMatcher matcher(*re, res);
        aobj = SplitHelper(cx, linearStr, limit, matcher, type);
    }
    if (!aobj)
        return false;

    
    aobj->setType(type);
    args.rval().setObject(*aobj);
    return true;
}

JSObject *
js::str_split_string(JSContext *cx, HandleTypeObject type, HandleString str, HandleString sep)
{
    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return nullptr;

    Rooted<JSLinearString*> linearSep(cx, sep->ensureLinear(cx));
    if (!linearSep)
        return nullptr;

    uint32_t limit = UINT32_MAX;

    RootedObject aobj(cx);
    if (linearSep->length() == 0) {
        aobj = CharSplitHelper(cx, linearStr, limit);
    } else {
        SplitStringMatcher matcher(cx, linearSep);
        aobj = SplitHelper(cx, linearStr, limit, matcher, type);
    }

    if (!aobj)
        return nullptr;

    aobj->setType(type);
    return aobj;
}

static bool
str_substr(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    int32_t length, len, begin;
    if (args.length() > 0) {
        length = int32_t(str->length());
        if (!ValueToIntegerRange(cx, args[0], &begin))
            return false;

        if (begin >= length) {
            args.rval().setString(cx->runtime()->emptyString);
            return true;
        }
        if (begin < 0) {
            begin += length; 
            if (begin < 0)
                begin = 0;
        }

        if (args.hasDefined(1)) {
            if (!ValueToIntegerRange(cx, args[1], &len))
                return false;

            if (len <= 0) {
                args.rval().setString(cx->runtime()->emptyString);
                return true;
            }

            if (uint32_t(length) < uint32_t(begin + len))
                len = length - begin;
        } else {
            len = length - begin;
        }

        str = DoSubstr(cx, str, size_t(begin), size_t(len));
        if (!str)
            return false;
    }

    args.rval().setString(str);
    return true;
}




static bool
str_concat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *str = ThisToStringForStringProto(cx, args);
    if (!str)
        return false;

    for (unsigned i = 0; i < args.length(); i++) {
        JSString *argStr = ToString<NoGC>(cx, args[i]);
        if (!argStr) {
            RootedString strRoot(cx, str);
            argStr = ToString<CanGC>(cx, args[i]);
            if (!argStr)
                return false;
            str = strRoot;
        }

        JSString *next = ConcatStrings<NoGC>(cx, str, argStr);
        if (next) {
            str = next;
        } else {
            RootedString strRoot(cx, str), argStrRoot(cx, argStr);
            str = ConcatStrings<CanGC>(cx, strRoot, argStrRoot);
            if (!str)
                return false;
        }
    }

    args.rval().setString(str);
    return true;
}

static bool
str_slice(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 1 && args.thisv().isString() && args[0].isInt32()) {
        JSString *str = args.thisv().toString();
        size_t begin = args[0].toInt32();
        size_t end = str->length();
        if (begin <= end) {
            size_t length = end - begin;
            if (length == 0) {
                str = cx->runtime()->emptyString;
            } else {
                str = (length == 1)
                      ? cx->staticStrings().getUnitStringForElement(cx, str, begin)
                      : js_NewDependentString(cx, str, begin, length);
                if (!str)
                    return false;
            }
            args.rval().setString(str);
            return true;
        }
    }

    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    if (args.length() != 0) {
        double begin, end, length;

        if (!ToInteger(cx, args[0], &begin))
            return false;
        length = str->length();
        if (begin < 0) {
            begin += length;
            if (begin < 0)
                begin = 0;
        } else if (begin > length) {
            begin = length;
        }

        if (args.hasDefined(1)) {
            if (!ToInteger(cx, args[1], &end))
                return false;
            if (end < 0) {
                end += length;
                if (end < 0)
                    end = 0;
            } else if (end > length) {
                end = length;
            }
            if (end < begin)
                end = begin;
        } else {
            end = length;
        }

        str = js_NewDependentString(cx, str,
                                    (size_t)begin,
                                    (size_t)(end - begin));
        if (!str)
            return false;
    }
    args.rval().setString(str);
    return true;
}

static const JSFunctionSpec string_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN("quote",             str_quote,             0,JSFUN_GENERIC_NATIVE),
    JS_FN(js_toSource_str,     str_toSource,          0,0),
#endif

    
    JS_FN(js_toString_str,     js_str_toString,       0,0),
    JS_FN(js_valueOf_str,      js_str_toString,       0,0),
    JS_FN("substring",         str_substring,         2,JSFUN_GENERIC_NATIVE),
    JS_FN("toLowerCase",       str_toLowerCase,       0,JSFUN_GENERIC_NATIVE),
    JS_FN("toUpperCase",       str_toUpperCase,       0,JSFUN_GENERIC_NATIVE),
    JS_FN("charAt",            js_str_charAt,         1,JSFUN_GENERIC_NATIVE),
    JS_FN("charCodeAt",        js_str_charCodeAt,     1,JSFUN_GENERIC_NATIVE),
    JS_SELF_HOSTED_FN("codePointAt", "String_codePointAt", 1,0),
    JS_FN("contains",          str_contains,          1,JSFUN_GENERIC_NATIVE),
    JS_FN("indexOf",           str_indexOf,           1,JSFUN_GENERIC_NATIVE),
    JS_FN("lastIndexOf",       str_lastIndexOf,       1,JSFUN_GENERIC_NATIVE),
    JS_FN("startsWith",        str_startsWith,        1,JSFUN_GENERIC_NATIVE),
    JS_FN("endsWith",          str_endsWith,          1,JSFUN_GENERIC_NATIVE),
    JS_FN("trim",              str_trim,              0,JSFUN_GENERIC_NATIVE),
    JS_FN("trimLeft",          str_trimLeft,          0,JSFUN_GENERIC_NATIVE),
    JS_FN("trimRight",         str_trimRight,         0,JSFUN_GENERIC_NATIVE),
    JS_FN("toLocaleLowerCase", str_toLocaleLowerCase, 0,JSFUN_GENERIC_NATIVE),
    JS_FN("toLocaleUpperCase", str_toLocaleUpperCase, 0,JSFUN_GENERIC_NATIVE),
#if EXPOSE_INTL_API
    JS_SELF_HOSTED_FN("localeCompare", "String_localeCompare", 1,0),
#else
    JS_FN("localeCompare",     str_localeCompare,     1,JSFUN_GENERIC_NATIVE),
#endif
    JS_SELF_HOSTED_FN("repeat", "String_repeat",      1,0),
#if EXPOSE_INTL_API
    JS_FN("normalize",         str_normalize,         0,JSFUN_GENERIC_NATIVE),
#endif

    
    JS_FN("match",             str_match,             1,JSFUN_GENERIC_NATIVE),
    JS_FN("search",            str_search,            1,JSFUN_GENERIC_NATIVE),
    JS_FN("replace",           str_replace,           2,JSFUN_GENERIC_NATIVE),
    JS_FN("split",             str_split,             2,JSFUN_GENERIC_NATIVE),
    JS_FN("substr",            str_substr,            2,JSFUN_GENERIC_NATIVE),

    
    JS_FN("concat",            str_concat,            1,JSFUN_GENERIC_NATIVE),
    JS_FN("slice",             str_slice,             2,JSFUN_GENERIC_NATIVE),

    
    JS_SELF_HOSTED_FN("bold",     "String_bold",       0,0),
    JS_SELF_HOSTED_FN("italics",  "String_italics",    0,0),
    JS_SELF_HOSTED_FN("fixed",    "String_fixed",      0,0),
    JS_SELF_HOSTED_FN("strike",   "String_strike",     0,0),
    JS_SELF_HOSTED_FN("small",    "String_small",      0,0),
    JS_SELF_HOSTED_FN("big",      "String_big",        0,0),
    JS_SELF_HOSTED_FN("blink",    "String_blink",      0,0),
    JS_SELF_HOSTED_FN("sup",      "String_sup",        0,0),
    JS_SELF_HOSTED_FN("sub",      "String_sub",        0,0),
    JS_SELF_HOSTED_FN("anchor",   "String_anchor",     1,0),
    JS_SELF_HOSTED_FN("link",     "String_link",       1,0),
    JS_SELF_HOSTED_FN("fontcolor","String_fontcolor",  1,0),
    JS_SELF_HOSTED_FN("fontsize", "String_fontsize",   1,0),

    JS_SELF_HOSTED_FN("@@iterator", "String_iterator", 0,0),
    JS_FS_END
};

bool
js_String(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedString str(cx);
    if (args.length() > 0) {
        str = ToString<CanGC>(cx, args[0]);
        if (!str)
            return false;
    } else {
        str = cx->runtime()->emptyString;
    }

    if (args.isConstructing()) {
        StringObject *strobj = StringObject::create(cx, str);
        if (!strobj)
            return false;
        args.rval().setObject(*strobj);
        return true;
    }

    args.rval().setString(str);
    return true;
}

bool
js::str_fromCharCode(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    JS_ASSERT(args.length() <= ARGS_LENGTH_MAX);
    if (args.length() == 1) {
        uint16_t code;
        if (!ToUint16(cx, args[0], &code))
            return false;
        if (StaticStrings::hasUnit(code)) {
            args.rval().setString(cx->staticStrings().getUnit(code));
            return true;
        }
        args[0].setInt32(code);
    }
    jschar *chars = cx->pod_malloc<jschar>(args.length() + 1);
    if (!chars)
        return false;
    for (unsigned i = 0; i < args.length(); i++) {
        uint16_t code;
        if (!ToUint16(cx, args[i], &code)) {
            js_free(chars);
            return false;
        }
        chars[i] = (jschar)code;
    }
    chars[args.length()] = 0;
    JSString *str = js_NewString<CanGC>(cx, chars, args.length());
    if (!str) {
        js_free(chars);
        return false;
    }

    args.rval().setString(str);
    return true;
}

static const JSFunctionSpec string_static_methods[] = {
    JS_FN("fromCharCode", js::str_fromCharCode, 1, 0),
    JS_SELF_HOSTED_FN("fromCodePoint", "String_static_fromCodePoint", 0,0),

    
    
#if EXPOSE_INTL_API
    JS_SELF_HOSTED_FN("localeCompare", "String_static_localeCompare", 2,0),
#endif
    JS_FS_END
};

 Shape *
StringObject::assignInitialShape(ExclusiveContext *cx, Handle<StringObject*> obj)
{
    JS_ASSERT(obj->nativeEmpty());

    return obj->addDataProperty(cx, cx->names().length, LENGTH_SLOT,
                                JSPROP_PERMANENT | JSPROP_READONLY);
}

JSObject *
js_InitStringClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());

    Rooted<JSString*> empty(cx, cx->runtime()->emptyString);
    RootedObject proto(cx, global->createBlankPrototype(cx, &StringObject::class_));
    if (!proto || !proto->as<StringObject>().init(cx, empty))
        return nullptr;

    
    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, js_String, cx->names().String, 1);
    if (!ctor)
        return nullptr;

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_String, ctor, proto))
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return nullptr;

    if (!DefinePropertiesAndBrand(cx, proto, nullptr, string_methods) ||
        !DefinePropertiesAndBrand(cx, ctor, nullptr, string_static_methods))
    {
        return nullptr;
    }

    



    if (!JS_DefineFunctions(cx, global, string_functions))
        return nullptr;

    return proto;
}

template <AllowGC allowGC>
JSFlatString *
js_NewString(ThreadSafeContext *cx, jschar *chars, size_t length)
{
    if (length == 1) {
        jschar c = chars[0];
        if (StaticStrings::hasUnit(c)) {
            
            
            js_free(chars);
            return cx->staticStrings().getUnit(c);
        }
    }

    return JSFlatString::new_<allowGC>(cx, chars, length);
}

template JSFlatString *
js_NewString<CanGC>(ThreadSafeContext *cx, jschar *chars, size_t length);

template JSFlatString *
js_NewString<NoGC>(ThreadSafeContext *cx, jschar *chars, size_t length);

JSLinearString *
js_NewDependentString(JSContext *cx, JSString *baseArg, size_t start, size_t length)
{
    if (length == 0)
        return cx->emptyString();

    JSLinearString *base = baseArg->ensureLinear(cx);
    if (!base)
        return nullptr;

    if (start == 0 && length == base->length())
        return base;

    const jschar *chars = base->chars() + start;

    if (JSLinearString *staticStr = cx->staticStrings().lookup(chars, length))
        return staticStr;

    return JSDependentString::new_(cx, base, chars, length);
}

template <AllowGC allowGC>
JSFlatString *
js_NewStringCopyN(ExclusiveContext *cx, const jschar *s, size_t n)
{
    if (JSFatInlineString::twoByteLengthFits(n))
        return NewFatInlineString<allowGC>(cx, TwoByteChars(s, n));

    jschar *news = cx->pod_malloc<jschar>(n + 1);
    if (!news)
        return nullptr;
    js_strncpy(news, s, n);
    news[n] = 0;
    JSFlatString *str = js_NewString<allowGC>(cx, news, n);
    if (!str)
        js_free(news);
    return str;
}

template JSFlatString *
js_NewStringCopyN<CanGC>(ExclusiveContext *cx, const jschar *s, size_t n);

template JSFlatString *
js_NewStringCopyN<NoGC>(ExclusiveContext *cx, const jschar *s, size_t n);

template <AllowGC allowGC>
JSFlatString *
js_NewStringCopyN(ThreadSafeContext *cx, const char *s, size_t n)
{
    if (JSFatInlineString::twoByteLengthFits(n))
        return NewFatInlineString<allowGC>(cx, JS::Latin1Chars(s, n));

    jschar *chars = InflateString(cx, s, &n);
    if (!chars)
        return nullptr;
    JSFlatString *str = js_NewString<allowGC>(cx, chars, n);
    if (!str)
        js_free(chars);
    return str;
}

template JSFlatString *
js_NewStringCopyN<CanGC>(ThreadSafeContext *cx, const char *s, size_t n);

template JSFlatString *
js_NewStringCopyN<NoGC>(ThreadSafeContext *cx, const char *s, size_t n);

template <AllowGC allowGC>
JSFlatString *
js_NewStringCopyZ(ExclusiveContext *cx, const jschar *s)
{
    size_t n = js_strlen(s);
    if (JSFatInlineString::twoByteLengthFits(n))
        return NewFatInlineString<allowGC>(cx, TwoByteChars(s, n));

    size_t m = (n + 1) * sizeof(jschar);
    jschar *news = (jschar *) cx->malloc_(m);
    if (!news)
        return nullptr;
    js_memcpy(news, s, m);
    JSFlatString *str = js_NewString<allowGC>(cx, news, n);
    if (!str)
        js_free(news);
    return str;
}

template JSFlatString *
js_NewStringCopyZ<CanGC>(ExclusiveContext *cx, const jschar *s);

template JSFlatString *
js_NewStringCopyZ<NoGC>(ExclusiveContext *cx, const jschar *s);

template <AllowGC allowGC>
JSFlatString *
js_NewStringCopyZ(ThreadSafeContext *cx, const char *s)
{
    return js_NewStringCopyN<allowGC>(cx, s, strlen(s));
}

template JSFlatString *
js_NewStringCopyZ<CanGC>(ThreadSafeContext *cx, const char *s);

template JSFlatString *
js_NewStringCopyZ<NoGC>(ThreadSafeContext *cx, const char *s);

const char *
js_ValueToPrintable(JSContext *cx, const Value &vArg, JSAutoByteString *bytes, bool asSource)
{
    RootedValue v(cx, vArg);
    JSString *str;
    if (asSource)
        str = ValueToSource(cx, v);
    else
        str = ToString<CanGC>(cx, v);
    if (!str)
        return nullptr;
    str = js_QuoteString(cx, str, 0);
    if (!str)
        return nullptr;
    return bytes->encodeLatin1(cx, str);
}

template <AllowGC allowGC>
JSString *
js::ToStringSlow(ExclusiveContext *cx, typename MaybeRooted<Value, allowGC>::HandleType arg)
{
    
    JS_ASSERT(!arg.isString());

    Value v = arg;
    if (!v.isPrimitive()) {
        if (!cx->shouldBeJSContext() || !allowGC)
            return nullptr;
        RootedValue v2(cx, v);
        if (!ToPrimitive(cx->asJSContext(), JSTYPE_STRING, &v2))
            return nullptr;
        v = v2;
    }

    JSString *str;
    if (v.isString()) {
        str = v.toString();
    } else if (v.isInt32()) {
        str = Int32ToString<allowGC>(cx, v.toInt32());
    } else if (v.isDouble()) {
        str = NumberToString<allowGC>(cx, v.toDouble());
    } else if (v.isBoolean()) {
        str = js_BooleanToString(cx, v.toBoolean());
    } else if (v.isNull()) {
        str = cx->names().null;
    } else {
        str = cx->names().undefined;
    }
    return str;
}

template JSString *
js::ToStringSlow<CanGC>(ExclusiveContext *cx, HandleValue arg);

template JSString *
js::ToStringSlow<NoGC>(ExclusiveContext *cx, Value arg);

JS_PUBLIC_API(JSString *)
js::ToStringSlow(JSContext *cx, HandleValue v)
{
    return ToStringSlow<CanGC>(cx, v);
}

JSString *
js::ValueToSource(JSContext *cx, HandleValue v)
{
    JS_CHECK_RECURSION(cx, return nullptr);
    assertSameCompartment(cx, v);

    if (v.isUndefined())
        return cx->names().void0;
    if (v.isString())
        return StringToSource(cx, v.toString());
    if (v.isPrimitive()) {
        
        if (v.isDouble() && IsNegativeZero(v.toDouble())) {
            
            static const jschar js_negzero_ucNstr[] = {'-', '0'};

            return js_NewStringCopyN<CanGC>(cx, js_negzero_ucNstr, 2);
        }
        return ToString<CanGC>(cx, v);
    }

    RootedValue fval(cx);
    RootedObject obj(cx, &v.toObject());
    if (!JSObject::getProperty(cx, obj, obj, cx->names().toSource, &fval))
        return nullptr;
    if (js_IsCallable(fval)) {
        RootedValue rval(cx);
        if (!Invoke(cx, ObjectValue(*obj), fval, 0, nullptr, &rval))
            return nullptr;
        return ToString<CanGC>(cx, rval);
    }

    return ObjectToSource(cx, obj);
}

JSString *
js::StringToSource(JSContext *cx, JSString *str)
{
    return js_QuoteString(cx, str, '"');
}

static bool
EqualCharsLatin1TwoByte(const Latin1Char *s1, const jschar *s2, size_t len)
{
    for (const Latin1Char *s1end = s1 + len; s1 < s1end; s1++, s2++) {
        if (jschar(*s1) != *s2)
            return false;
    }
    return true;
}

static bool
EqualChars(JSLinearString *str1, JSLinearString *str2)
{
    MOZ_ASSERT(str1->length() == str2->length());

    size_t len = str1->length();

    AutoCheckCannotGC nogc;
    if (str1->hasTwoByteChars()) {
        if (str2->hasTwoByteChars())
            return PodEqual(str1->twoByteChars(nogc), str2->twoByteChars(nogc), len);

        return EqualCharsLatin1TwoByte(str2->latin1Chars(nogc), str1->twoByteChars(nogc), len);
    }

    if (str2->hasLatin1Chars())
        return PodEqual(str1->latin1Chars(nogc), str2->latin1Chars(nogc), len);

    return EqualCharsLatin1TwoByte(str1->latin1Chars(nogc), str2->twoByteChars(nogc), len);
}

bool
js::EqualStrings(JSContext *cx, JSString *str1, JSString *str2, bool *result)
{
    if (str1 == str2) {
        *result = true;
        return true;
    }

    size_t length1 = str1->length();
    if (length1 != str2->length()) {
        *result = false;
        return true;
    }

    JSLinearString *linear1 = str1->ensureLinear(cx);
    if (!linear1)
        return false;
    JSLinearString *linear2 = str2->ensureLinear(cx);
    if (!linear2)
        return false;

    *result = EqualChars(linear1, linear2);
    return true;
}

bool
js::EqualStrings(JSLinearString *str1, JSLinearString *str2)
{
    if (str1 == str2)
        return true;

    size_t length1 = str1->length();
    if (length1 != str2->length())
        return false;

    return EqualChars(str1, str2);
}

static bool
CompareStringsImpl(JSContext *cx, JSString *str1, JSString *str2, int32_t *result)
{
    JS_ASSERT(str1);
    JS_ASSERT(str2);

    if (str1 == str2) {
        *result = 0;
        return true;
    }

    const jschar *s1 = str1->getChars(cx);
    if (!s1)
        return false;

    const jschar *s2 = str2->getChars(cx);
    if (!s2)
        return false;

    *result = CompareChars(s1, str1->length(), s2, str2->length());
    return true;
}

bool
js::CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32_t *result)
{
    return CompareStringsImpl(cx, str1, str2, result);
}

int32_t
js::CompareAtoms(JSAtom *atom1, JSAtom *atom2)
{
    return CompareChars(atom1->chars(), atom1->length(), atom2->chars(), atom2->length());
}

bool
js::StringEqualsAscii(JSLinearString *str, const char *asciiBytes)
{
    size_t length = strlen(asciiBytes);
#ifdef DEBUG
    for (size_t i = 0; i != length; ++i)
        JS_ASSERT(unsigned(asciiBytes[i]) <= 127);
#endif
    if (length != str->length())
        return false;
    const jschar *chars = str->chars();
    for (size_t i = 0; i != length; ++i) {
        if (unsigned(asciiBytes[i]) != unsigned(chars[i]))
            return false;
    }
    return true;
}

size_t
js_strlen(const jschar *s)
{
    const jschar *t;

    for (t = s; *t != 0; t++)
        continue;
    return (size_t)(t - s);
}

int32_t
js_strcmp(const jschar *lhs, const jschar *rhs)
{
    while (true) {
        if (*lhs != *rhs)
            return int32_t(*lhs) - int32_t(*rhs);
        if (*lhs == 0)
            return 0;
        ++lhs, ++rhs;
    }
}

jschar *
js_strdup(js::ThreadSafeContext *cx, const jschar *s)
{
    size_t n = js_strlen(s);
    jschar *ret = cx->pod_malloc<jschar>(n + 1);
    if (!ret)
        return nullptr;
    js_strncpy(ret, s, n);
    ret[n] = '\0';
    return ret;
}

jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit)
{
    while (s < limit) {
        if (*s == c)
            return (jschar *)s;
        s++;
    }
    return nullptr;
}

jschar *
js::InflateString(ThreadSafeContext *cx, const char *bytes, size_t *lengthp)
{
    size_t nchars;
    jschar *chars;
    size_t nbytes = *lengthp;

    nchars = nbytes;
    chars = cx->pod_malloc<jschar>(nchars + 1);
    if (!chars)
        goto bad;
    for (size_t i = 0; i < nchars; i++)
        chars[i] = (unsigned char) bytes[i];
    *lengthp = nchars;
    chars[nchars] = 0;
    return chars;

  bad:
    
    
    *lengthp = 0;
    return nullptr;
}

bool
js::DeflateStringToBuffer(JSContext *maybecx, const jschar *src, size_t srclen,
                          char *dst, size_t *dstlenp)
{
    size_t dstlen = *dstlenp;
    if (srclen > dstlen) {
        for (size_t i = 0; i < dstlen; i++)
            dst[i] = (char) src[i];
        if (maybecx) {
            AutoSuppressGC suppress(maybecx);
            JS_ReportErrorNumber(maybecx, js_GetErrorMessage, nullptr,
                                 JSMSG_BUFFER_TOO_SMALL);
        }
        return false;
    }
    for (size_t i = 0; i < srclen; i++)
        dst[i] = (char) src[i];
    *dstlenp = srclen;
    return true;
}

#define ____ false








const bool js_isidstart[] = {

 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, true, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, ____, ____, ____, ____, true, ____, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, ____, ____, ____, ____, ____
};









const bool js_isident[] = {

 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, true, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, true, true,
 true, true, true, true, true, true, true, true, ____, ____,
 ____, ____, ____, ____, ____, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, ____, ____, ____, ____, true, ____, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, ____, ____, ____, ____, ____
};


const bool js_isspace[] = {

 ____, ____, ____, ____, ____, ____, ____, ____, ____, true,
 true, true, true, true, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, true, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____
};















static const bool js_isUriReservedPlusPound[] = {

 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, true, true, ____, true, ____,
 ____, ____, ____, true, true, ____, ____, true, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, true, true,
 ____, true, ____, true, true, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____
};
















static const bool js_isUriUnescaped[] = {

 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, ____, ____, ____, ____, ____, ____, ____,
 ____, ____, ____, true, ____, ____, ____, ____, ____, true,
 true, true, true, ____, ____, true, true, ____, true, true,
 true, true, true, true, true, true, true, true, ____, ____,
 ____, ____, ____, ____, ____, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, ____, ____, ____, ____, true, ____, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, true, true, true, true, true, true, true,
 true, true, true, ____, ____, ____, true, ____
};

#undef ____

#define URI_CHUNK 64U

static inline bool
TransferBufferToString(StringBuffer &sb, MutableHandleValue rval)
{
    JSString *str = sb.finishString();
    if (!str)
        return false;
    rval.setString(str);
    return true;
}








static bool
Encode(JSContext *cx, Handle<JSLinearString*> str, const bool *unescapedSet,
       const bool *unescapedSet2, MutableHandleValue rval)
{
    static const char HexDigits[] = "0123456789ABCDEF"; 

    size_t length = str->length();
    if (length == 0) {
        rval.setString(cx->runtime()->emptyString);
        return true;
    }

    const jschar *chars = str->chars();
    StringBuffer sb(cx);
    if (!sb.reserve(length))
        return false;
    jschar hexBuf[4];
    hexBuf[0] = '%';
    hexBuf[3] = 0;
    for (size_t k = 0; k < length; k++) {
        jschar c = chars[k];
        if (c < 128 && (unescapedSet[c] || (unescapedSet2 && unescapedSet2[c]))) {
            if (!sb.append(c))
                return false;
        } else {
            if ((c >= 0xDC00) && (c <= 0xDFFF)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_URI, nullptr);
                return false;
            }
            uint32_t v;
            if (c < 0xD800 || c > 0xDBFF) {
                v = c;
            } else {
                k++;
                if (k == length) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_BAD_URI, nullptr);
                    return false;
                }
                jschar c2 = chars[k];
                if ((c2 < 0xDC00) || (c2 > 0xDFFF)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_BAD_URI, nullptr);
                    return false;
                }
                v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
            }
            uint8_t utf8buf[4];
            size_t L = js_OneUcs4ToUtf8Char(utf8buf, v);
            for (size_t j = 0; j < L; j++) {
                hexBuf[1] = HexDigits[utf8buf[j] >> 4];
                hexBuf[2] = HexDigits[utf8buf[j] & 0xf];
                if (!sb.append(hexBuf, 3))
                    return false;
            }
        }
    }

    return TransferBufferToString(sb, rval);
}

static bool
Decode(JSContext *cx, Handle<JSLinearString*> str, const bool *reservedSet, MutableHandleValue rval)
{
    size_t length = str->length();
    if (length == 0) {
        rval.setString(cx->runtime()->emptyString);
        return true;
    }

    const jschar *chars = str->chars();
    StringBuffer sb(cx);
    for (size_t k = 0; k < length; k++) {
        jschar c = chars[k];
        if (c == '%') {
            size_t start = k;
            if ((k + 2) >= length)
                goto report_bad_uri;
            if (!JS7_ISHEX(chars[k+1]) || !JS7_ISHEX(chars[k+2]))
                goto report_bad_uri;
            uint32_t B = JS7_UNHEX(chars[k+1]) * 16 + JS7_UNHEX(chars[k+2]);
            k += 2;
            if (!(B & 0x80)) {
                c = (jschar)B;
            } else {
                int n = 1;
                while (B & (0x80 >> n))
                    n++;
                if (n == 1 || n > 4)
                    goto report_bad_uri;
                uint8_t octets[4];
                octets[0] = (uint8_t)B;
                if (k + 3 * (n - 1) >= length)
                    goto report_bad_uri;
                for (int j = 1; j < n; j++) {
                    k++;
                    if (chars[k] != '%')
                        goto report_bad_uri;
                    if (!JS7_ISHEX(chars[k+1]) || !JS7_ISHEX(chars[k+2]))
                        goto report_bad_uri;
                    B = JS7_UNHEX(chars[k+1]) * 16 + JS7_UNHEX(chars[k+2]);
                    if ((B & 0xC0) != 0x80)
                        goto report_bad_uri;
                    k += 2;
                    octets[j] = (char)B;
                }
                uint32_t v = JS::Utf8ToOneUcs4Char(octets, n);
                if (v >= 0x10000) {
                    v -= 0x10000;
                    if (v > 0xFFFFF)
                        goto report_bad_uri;
                    c = (jschar)((v & 0x3FF) + 0xDC00);
                    jschar H = (jschar)((v >> 10) + 0xD800);
                    if (!sb.append(H))
                        return false;
                } else {
                    c = (jschar)v;
                }
            }
            if (c < 128 && reservedSet && reservedSet[c]) {
                if (!sb.append(chars + start, k - start + 1))
                    return false;
            } else {
                if (!sb.append(c))
                    return false;
            }
        } else {
            if (!sb.append(c))
                return false;
        }
    }

    return TransferBufferToString(sb, rval);

  report_bad_uri:
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_URI);
    

    return false;
}

static bool
str_decodeURI(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<JSLinearString*> str(cx, ArgToRootedString(cx, args, 0));
    if (!str)
        return false;

    return Decode(cx, str, js_isUriReservedPlusPound, args.rval());
}

static bool
str_decodeURI_Component(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<JSLinearString*> str(cx, ArgToRootedString(cx, args, 0));
    if (!str)
        return false;

    return Decode(cx, str, nullptr, args.rval());
}

static bool
str_encodeURI(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<JSLinearString*> str(cx, ArgToRootedString(cx, args, 0));
    if (!str)
        return false;

    return Encode(cx, str, js_isUriUnescaped, js_isUriReservedPlusPound, args.rval());
}

static bool
str_encodeURI_Component(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<JSLinearString*> str(cx, ArgToRootedString(cx, args, 0));
    if (!str)
        return false;

    return Encode(cx, str, js_isUriUnescaped, nullptr, args.rval());
}





int
js_OneUcs4ToUtf8Char(uint8_t *utf8Buffer, uint32_t ucs4Char)
{
    int utf8Length = 1;

    JS_ASSERT(ucs4Char <= 0x10FFFF);
    if (ucs4Char < 0x80) {
        *utf8Buffer = (uint8_t)ucs4Char;
    } else {
        int i;
        uint32_t a = ucs4Char >> 11;
        utf8Length = 2;
        while (a) {
            a >>= 5;
            utf8Length++;
        }
        i = utf8Length;
        while (--i) {
            utf8Buffer[i] = (uint8_t)((ucs4Char & 0x3F) | 0x80);
            ucs4Char >>= 6;
        }
        *utf8Buffer = (uint8_t)(0x100 - (1 << (8-utf8Length)) + ucs4Char);
    }
    return utf8Length;
}

size_t
js::PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp, JSLinearString *str,
                         uint32_t quote)
{
    return PutEscapedStringImpl(buffer, bufferSize, fp, str->chars(),
                                str->length(), quote);
}

size_t
js::PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp, const jschar *chars,
                         size_t length, uint32_t quote)
{
    enum {
        STOP, FIRST_QUOTE, LAST_QUOTE, CHARS, ESCAPE_START, ESCAPE_MORE
    } state;

    JS_ASSERT(quote == 0 || quote == '\'' || quote == '"');
    JS_ASSERT_IF(!buffer, bufferSize == 0);
    JS_ASSERT_IF(fp, !buffer);

    if (bufferSize == 0)
        buffer = nullptr;
    else
        bufferSize--;

    const jschar *charsEnd = chars + length;
    size_t n = 0;
    state = FIRST_QUOTE;
    unsigned shift = 0;
    unsigned hex = 0;
    unsigned u = 0;
    char c = 0;  

    for (;;) {
        switch (state) {
          case STOP:
            goto stop;
          case FIRST_QUOTE:
            state = CHARS;
            goto do_quote;
          case LAST_QUOTE:
            state = STOP;
          do_quote:
            if (quote == 0)
                continue;
            c = (char)quote;
            break;
          case CHARS:
            if (chars == charsEnd) {
                state = LAST_QUOTE;
                continue;
            }
            u = *chars++;
            if (u < ' ') {
                if (u != 0) {
                    const char *escape = strchr(js_EscapeMap, (int)u);
                    if (escape) {
                        u = escape[1];
                        goto do_escape;
                    }
                }
                goto do_hex_escape;
            }
            if (u < 127) {
                if (u == quote || u == '\\')
                    goto do_escape;
                c = (char)u;
            } else if (u < 0x100) {
                goto do_hex_escape;
            } else {
                shift = 16;
                hex = u;
                u = 'u';
                goto do_escape;
            }
            break;
          do_hex_escape:
            shift = 8;
            hex = u;
            u = 'x';
          do_escape:
            c = '\\';
            state = ESCAPE_START;
            break;
          case ESCAPE_START:
            JS_ASSERT(' ' <= u && u < 127);
            c = (char)u;
            state = ESCAPE_MORE;
            break;
          case ESCAPE_MORE:
            if (shift == 0) {
                state = CHARS;
                continue;
            }
            shift -= 4;
            u = 0xF & (hex >> shift);
            c = (char)(u + (u < 10 ? '0' : 'A' - 10));
            break;
        }
        if (buffer) {
            JS_ASSERT(n <= bufferSize);
            if (n != bufferSize) {
                buffer[n] = c;
            } else {
                buffer[n] = '\0';
                buffer = nullptr;
            }
        } else if (fp) {
            if (fputc(c, fp) < 0)
                return size_t(-1);
        }
        n++;
    }
  stop:
    if (buffer)
        buffer[n] = '\0';
    return n;
}
