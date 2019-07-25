

















#include "mozilla/Attributes.h"
#include "mozilla/FloatingPoint.h"

#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsprobes.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jsversion.h"

#include "builtin/RegExp.h"
#include "vm/GlobalObject.h"
#include "vm/NumericConversions.h"
#include "vm/RegExpObject.h"
#include "vm/StringBuffer.h"

#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jsstrinlines.h"
#include "jsautooplen.h"        

#include "vm/RegExpObject-inl.h"
#include "vm/RegExpStatics-inl.h"
#include "vm/StringObject-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;
using namespace js::unicode;

static JSLinearString *
ArgToRootedString(JSContext *cx, CallArgs &args, unsigned argno)
{
    if (argno >= args.length())
        return cx->runtime->atomState.typeAtoms[JSTYPE_VOID];

    Value &arg = args[argno];
    JSString *str = ToString(cx, arg);
    if (!str)
        return NULL;

    arg = StringValue(str);
    return str->ensureLinear(cx);
}




static JSBool
str_decodeURI(JSContext *cx, unsigned argc, Value *vp);

static JSBool
str_decodeURI_Component(JSContext *cx, unsigned argc, Value *vp);

static JSBool
str_encodeURI(JSContext *cx, unsigned argc, Value *vp);

static JSBool
str_encodeURI_Component(JSContext *cx, unsigned argc, Value *vp);

static const uint32_t INVALID_UTF8 = UINT32_MAX;

static uint32_t
Utf8ToOneUcs4Char(const uint8_t *utf8Buffer, int utf8Length);







static JSBool
str_escape(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
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

    jschar *newchars = (jschar *) cx->malloc_((newlength + 1) * sizeof(jschar));
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

    JSString *retstr = js_NewString(cx, newchars, newlength);
    if (!retstr) {
        cx->free_(newchars);
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


static JSBool
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
static JSBool
str_uneval(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *str = js_ValueToSource(cx, args.length() != 0 ? args[0] : UndefinedValue());
    if (!str)
        return false;

    args.rval().setString(str);
    return true;
}
#endif

static JSFunctionSpec string_functions[] = {
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

jschar      js_empty_ucstr[]  = {0};
JSSubString js_EmptySubString = {0, js_empty_ucstr};

static const unsigned STRING_ELEMENT_ATTRS = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

static JSBool
str_enumerate(JSContext *cx, HandleObject obj)
{
    RootedString str(cx, obj->asString().unbox());
    RootedValue value(cx);
    for (size_t i = 0, length = str->length(); i < length; i++) {
        JSString *str1 = js_NewDependentString(cx, str, i, 1);
        if (!str1)
            return false;
        value.setString(str1);
        if (!obj->defineElement(cx, i, value,
                                JS_PropertyStub, JS_StrictPropertyStub,
                                STRING_ELEMENT_ATTRS)) {
            return false;
        }
    }

    return true;
}

static JSBool
str_resolve(JSContext *cx, HandleObject obj, HandleId id, unsigned flags,
            MutableHandleObject objp)
{
    if (!JSID_IS_INT(id))
        return JS_TRUE;

    JSString *str = obj->asString().unbox();

    int32_t slot = JSID_TO_INT(id);
    if ((size_t)slot < str->length()) {
        JSString *str1 = cx->runtime->staticStrings.getUnitStringForElement(cx, str, size_t(slot));
        if (!str1)
            return JS_FALSE;
        RootedValue value(cx, StringValue(str1));
        if (!obj->defineElement(cx, uint32_t(slot), value, NULL, NULL,
                                STRING_ELEMENT_ATTRS)) {
            return JS_FALSE;
        }
        objp.set(obj);
    }
    return JS_TRUE;
}

Class js::StringClass = {
    js_String_str,
    JSCLASS_HAS_RESERVED_SLOTS(StringObject::RESERVED_SLOTS) |
    JSCLASS_NEW_RESOLVE | JSCLASS_HAS_CACHED_PROTO(JSProto_String),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    str_enumerate,
    (JSResolveOp)str_resolve,
    JS_ConvertStub
};







static JS_ALWAYS_INLINE JSString *
ThisToStringForStringProto(JSContext *cx, CallReceiver call)
{
    JS_CHECK_RECURSION(cx, return NULL);

    if (call.thisv().isString())
        return call.thisv().toString();

    if (call.thisv().isObject()) {
        RootedObject obj(cx, &call.thisv().toObject());
        if (obj->isString()) {
            Rooted<jsid> id(cx, NameToId(cx->runtime->atomState.toStringAtom));
            if (ClassMethodIsNative(cx, obj, &StringClass, id, js_str_toString)) {
                JSString *str = obj->asString().unbox();
                call.thisv().setString(str);
                return str;
            }
        }
    } else if (call.thisv().isNullOrUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO,
                             call.thisv().isNull() ? "null" : "undefined", "object");
        return NULL;
    }

    JSString *str = ToStringSlow(cx, call.thisv());
    if (!str)
        return NULL;

    call.thisv().setString(str);
    return str;
}

static bool
IsString(const Value &v)
{
    return v.isString() || (v.isObject() && v.toObject().hasClass(&StringClass));
}

#if JS_HAS_TOSOURCE





static JSBool
str_quote(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *str = ThisToStringForStringProto(cx, args);
    if (!str)
        return false;
    str = js_QuoteString(cx, str, '"');
    if (!str)
        return false;
    args.rval().setString(str);
    return true;
}

static bool
str_toSource_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsString(args.thisv()));

    Rooted<JSString*> str(cx, ToString(cx, args.thisv()));
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

static JSBool
str_toSource(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, IsString, str_toSource_impl, args);
}

#endif 

static bool
str_toString_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsString(args.thisv()));

    args.rval().setString(args.thisv().isString()
                              ? args.thisv().toString()
                              : args.thisv().toObject().asString().unbox());
    return true;
}

JSBool
js_str_toString(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod(cx, IsString, str_toString_impl, args);
}





JS_ALWAYS_INLINE bool
ValueToIntegerRange(JSContext *cx, const Value &v, int32_t *out)
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

static JSBool
str_substring(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    int32_t length, begin, end;
    if (args.length() > 0) {
        end = length = int32_t(str->length());

        if (!ValueToIntegerRange(cx, args[0], &begin))
            return false;

        if (begin < 0)
            begin = 0;
        else if (begin > length)
            begin = length;

        if (args.hasDefined(1)) {
            if (!ValueToIntegerRange(cx, args[1], &end))
                return false;

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

        str = js_NewDependentString(cx, str, size_t(begin), size_t(end - begin));
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
        return NULL;

    jschar *news = (jschar *) cx->malloc_((n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    for (size_t i = 0; i < n; i++)
        news[i] = unicode::ToLowerCase(s[i]);
    news[n] = 0;
    str = js_NewString(cx, news, n);
    if (!str) {
        cx->free_(news);
        return NULL;
    }
    return str;
}

static inline bool
ToLowerCaseHelper(JSContext *cx, CallReceiver call)
{
    JSString *str = ThisToStringForStringProto(cx, call);
    if (!str)
        return false;

    str = js_toLowerCase(cx, str);
    if (!str)
        return false;

    call.rval().setString(str);
    return true;
}

static JSBool
str_toLowerCase(JSContext *cx, unsigned argc, Value *vp)
{
    return ToLowerCaseHelper(cx, CallArgsFromVp(argc, vp));
}

static JSBool
str_toLocaleLowerCase(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToLowerCase) {
        RootedString str(cx, ThisToStringForStringProto(cx, args));
        if (!str)
            return false;

        Value result;
        if (!cx->localeCallbacks->localeToLowerCase(cx, str, &result))
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
        return NULL;
    jschar *news = (jschar *) cx->malloc_((n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    for (size_t i = 0; i < n; i++)
        news[i] = unicode::ToUpperCase(s[i]);
    news[n] = 0;
    str = js_NewString(cx, news, n);
    if (!str) {
        cx->free_(news);
        return NULL;
    }
    return str;
}

static JSBool
ToUpperCaseHelper(JSContext *cx, CallReceiver call)
{
    JSString *str = ThisToStringForStringProto(cx, call);
    if (!str)
        return false;

    str = js_toUpperCase(cx, str);
    if (!str)
        return false;

    call.rval().setString(str);
    return true;
}

static JSBool
str_toUpperCase(JSContext *cx, unsigned argc, Value *vp)
{
    return ToUpperCaseHelper(cx, CallArgsFromVp(argc, vp));
}

static JSBool
str_toLocaleUpperCase(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToUpperCase) {
        RootedString str(cx, ThisToStringForStringProto(cx, args));
        if (!str)
            return false;

        Value result;
        if (!cx->localeCallbacks->localeToUpperCase(cx, str, &result))
            return false;

        args.rval().set(result);
        return true;
    }

    return ToUpperCaseHelper(cx, args);
}

static JSBool
str_localeCompare(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    if (args.length() == 0) {
        args.rval().setInt32(0);
    } else {
        RootedString thatStr(cx, ToString(cx, args[0]));
        if (!thatStr)
            return false;

        if (cx->localeCallbacks && cx->localeCallbacks->localeCompare) {
            args[0].setString(thatStr);

            Value result;
            if (!cx->localeCallbacks->localeCompare(cx, str, thatStr, &result))
                return true;

            args.rval().set(result);
            return true;
        }

        int32_t result;
        if (!CompareStrings(cx, str, thatStr, &result))
            return false;

        args.rval().setInt32(result);
    }
    return true;
}

JSBool
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

    str = cx->runtime->staticStrings.getUnitStringForElement(cx, str, i);
    if (!str)
        return false;
    args.rval().setString(str);
    return true;

  out_of_range:
    args.rval().setString(cx->runtime->emptyString);
    return true;
}

JSBool
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

    const jschar *chars;
    chars = str->getChars(cx);
    if (!chars)
        return false;

    args.rval().setInt32(chars[i]);
    return true;

out_of_range:
    args.rval().setDouble(js_NaN);
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
    static JS_ALWAYS_INLINE Extent computeExtent(const jschar *, uint32_t patlen) {
        return (patlen - 1) * sizeof(jschar);
    }
    static JS_ALWAYS_INLINE bool match(const jschar *p, const jschar *t, Extent extent) {
        return memcmp(p, t, extent) == 0;
    }
};

struct ManualCmp {
    typedef const jschar *Extent;
    static JS_ALWAYS_INLINE Extent computeExtent(const jschar *pat, uint32_t patlen) {
        return pat + patlen;
    }
    static JS_ALWAYS_INLINE bool match(const jschar *p, const jschar *t, Extent extent) {
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

static JS_ALWAYS_INLINE int
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


static JSBool
str_contains(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    Rooted<JSLinearString *> patstr(cx, ArgToRootedString(cx, args, 0));
    if (!patstr)
        return false;

    
    uint32_t textlen = str->length();
    const jschar *text = str->getChars(cx);
    if (!text)
        return false;

    if (args.hasDefined(1)) {
        
        double posDouble;
        if (!ToInteger(cx, args[1], &posDouble))
            return false;

        
        text += uint32_t(Min(double(textlen), Max(0.0, posDouble)));
    }

    
    uint32_t patlen = patstr->length();
    const jschar *pat = patstr->chars();

    
    args.rval().setBoolean(StringMatch(text, textlen, pat, patlen) >= 0);
    return true;
}

static JSBool
str_indexOf(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    JSLinearString *patstr = ArgToRootedString(cx, args, 0);
    if (!patstr)
        return false;

    uint32_t textlen = str->length();
    const jschar *text = str->getChars(cx);
    if (!text)
        return false;

    uint32_t patlen = patstr->length();
    const jschar *pat = patstr->chars();

    uint32_t start;
    if (args.length() > 1) {
        if (args[1].isInt32()) {
            int i = args[1].toInt32();
            if (i <= 0) {
                start = 0;
            } else if (uint32_t(i) > textlen) {
                start = textlen;
                textlen = 0;
            } else {
                start = i;
                text += start;
                textlen -= start;
            }
        } else {
            double d;
            if (!ToInteger(cx, args[1], &d))
                return false;
            if (d <= 0) {
                start = 0;
            } else if (d > textlen) {
                start = textlen;
                textlen = 0;
            } else {
                start = (int)d;
                text += start;
                textlen -= start;
            }
        }
    } else {
        start = 0;
    }

    int match = StringMatch(text, textlen, pat, patlen);
    args.rval().setInt32((match == -1) ? -1 : start + match);
    return true;
}

static JSBool
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
            if (!MOZ_DOUBLE_IS_NaN(d)) {
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


static JSBool
str_startsWith(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    Rooted<JSLinearString *> patstr(cx, ArgToRootedString(cx, args, 0));
    if (!patstr)
        return false;

    
    uint32_t textlen = str->length();
    const jschar *text = str->getChars(cx);
    if (!text)
        return false;

    if (args.hasDefined(1)) {
        
        double posDouble;
        if (!ToInteger(cx, args[1], &posDouble))
            return false;

        
        uint32_t position = Min(double(textlen), Max(0.0, posDouble));
        text += position;
        textlen -= position;
    }

    
    uint32_t patlen = patstr->length();
    const jschar *pat = patstr->chars();

    
    args.rval().setBoolean(textlen >= patlen && PodEqual(text, pat, patlen));
    return true;
}


static JSBool
str_endsWith(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    
    Rooted<JSLinearString *> patstr(cx, ArgToRootedString(cx, args, 0));
    if (!patstr)
        return false;

    
    uint32_t textlen = str->length();
    const jschar *text = str->getChars(cx);
    if (!text)
        return false;

    if (args.hasDefined(1)) {
        
        double endPosDouble;
        if (!ToInteger(cx, args[1], &endPosDouble))
            return false;

        
        textlen = Min(double(textlen), Max(0.0, endPosDouble));
    }

    
    uint32_t patlen = patstr->length();
    const jschar *pat = patstr->chars();

    
    args.rval().setBoolean(textlen >= patlen && PodEqual(text + textlen - patlen, pat, patlen));
    return true;
}

static JSBool
js_TrimString(JSContext *cx, Value *vp, JSBool trimLeft, JSBool trimRight)
{
    CallReceiver call = CallReceiverFromVp(vp);
    JSString *str = ThisToStringForStringProto(cx, call);
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

static JSBool
str_trim(JSContext *cx, unsigned argc, Value *vp)
{
    return js_TrimString(cx, vp, JS_TRUE, JS_TRUE);
}

static JSBool
str_trimLeft(JSContext *cx, unsigned argc, Value *vp)
{
    return js_TrimString(cx, vp, JS_TRUE, JS_FALSE);
}

static JSBool
str_trimRight(JSContext *cx, unsigned argc, Value *vp)
{
    return js_TrimString(cx, vp, JS_FALSE, JS_TRUE);
}






class FlatMatch
{
    RootedAtom patstr;
    const jschar *pat;
    size_t       patlen;
    int32_t      match_;

    friend class StringRegExpGuard;

  public:
    FlatMatch(JSContext *cx) : patstr(cx) {}
    JSLinearString *pattern() const { return patstr; }
    size_t patternLength() const { return patlen; }

    



    int32_t match() const { return match_; }
};

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







class StringRegExpGuard
{
    StringRegExpGuard(const StringRegExpGuard &) MOZ_DELETE;
    void operator=(const StringRegExpGuard &) MOZ_DELETE;

    RegExpGuard re_;
    FlatMatch   fm;

    



    static const size_t MAX_FLAT_PAT_LEN = 256;

    static JSAtom *
    flattenPattern(JSContext *cx, JSAtom *patstr)
    {
        StringBuffer sb(cx);
        if (!sb.reserve(patstr->length()))
            return NULL;

        static const jschar ESCAPE_CHAR = '\\';
        const jschar *chars = patstr->chars();
        size_t len = patstr->length();
        for (const jschar *it = chars; it != chars + len; ++it) {
            if (IsRegExpMetaChar(*it)) {
                if (!sb.append(ESCAPE_CHAR) || !sb.append(*it))
                    return NULL;
            } else {
                if (!sb.append(*it))
                    return NULL;
            }
        }
        return sb.finishAtom();
    }

  public:
    StringRegExpGuard(JSContext *cx) : fm(cx) {}

    
    bool init(JSContext *cx, CallArgs args, bool convertVoid = false)
    {
        if (args.length() != 0 && IsObjectWithClass(args[0], ESClass_RegExp, cx)) {
            if (!RegExpToShared(cx, args[0].toObject(), &re_))
                return false;
        } else {
            if (convertVoid && !args.hasDefined(0)) {
                fm.patstr = cx->runtime->emptyString;
                return true;
            }

            JSString *arg = ArgToRootedString(cx, args, 0);
            if (!arg)
                return false;

            fm.patstr = js_AtomizeString(cx, arg);
            if (!fm.patstr)
                return false;
        }
        return true;
    }

    









    const FlatMatch *
    tryFlatMatch(JSContext *cx, JSString *textstr, unsigned optarg, unsigned argc,
                 bool checkMetaChars = true)
    {
        if (re_.initialized())
            return NULL;

        fm.pat = fm.patstr->chars();
        fm.patlen = fm.patstr->length();

        if (optarg < argc)
            return NULL;

        if (checkMetaChars &&
            (fm.patlen > MAX_FLAT_PAT_LEN || HasRegExpMetaChars(fm.pat, fm.patlen))) {
            return NULL;
        }

        



        if (textstr->isRope()) {
            if (!RopeMatch(cx, textstr, fm.pat, fm.patlen, &fm.match_))
                return NULL;
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
            opt = ToString(cx, args[optarg]);
            if (!opt)
                return false;
        } else {
            opt = NULL;
        }

        JSAtom *patstr;
        if (flat) {
            patstr = flattenPattern(cx, fm.patstr);
            if (!patstr)
                return false;
        } else {
            patstr = fm.patstr;
        }
        JS_ASSERT(patstr);

        return cx->compartment->regExps.get(cx, patstr, opt, &re_);
    }

    RegExpShared &regExp() { return *re_; }
};


static JS_ALWAYS_INLINE bool
Matched(RegExpExecType type, const Value &v)
{
    return (type == RegExpTest) ? v.isTrue() : !v.isNull();
}

typedef bool (*DoMatchCallback)(JSContext *cx, RegExpStatics *res, size_t count, void *data);





enum MatchControlFlags {
   TEST_GLOBAL_BIT         = 0x1, 
   TEST_SINGLE_BIT         = 0x2, 
   CALLBACK_ON_SINGLE_BIT  = 0x4, 

   MATCH_ARGS    = TEST_GLOBAL_BIT,
   MATCHALL_ARGS = CALLBACK_ON_SINGLE_BIT,
   REPLACE_ARGS  = TEST_GLOBAL_BIT | TEST_SINGLE_BIT | CALLBACK_ON_SINGLE_BIT
};


static bool
DoMatch(JSContext *cx, RegExpStatics *res, JSString *str, RegExpShared &re,
        DoMatchCallback callback, void *data, MatchControlFlags flags, Value *rval)
{
    Rooted<JSLinearString*> linearStr(cx, str->ensureLinear(cx));
    if (!linearStr)
        return false;

    if (re.global()) {
        RegExpExecType type = (flags & TEST_GLOBAL_BIT) ? RegExpTest : RegExpExec;
        for (size_t count = 0, i = 0, length = str->length(); i <= length; ++count) {
            if (!JS_CHECK_OPERATION_LIMIT(cx))
                return false;

            const jschar *chars = linearStr->chars();
            size_t charsLen = linearStr->length();

            if (!ExecuteRegExp(cx, res, re, linearStr, chars, charsLen, &i, type, rval))
                return false;
            if (!Matched(type, *rval))
                break;
            if (!callback(cx, res, count, data))
                return false;
            if (!res->matched())
                ++i;
        }
    } else {
        const jschar *chars = linearStr->chars();
        size_t charsLen = linearStr->length();

        RegExpExecType type = (flags & TEST_SINGLE_BIT) ? RegExpTest : RegExpExec;
        bool callbackOnSingle = !!(flags & CALLBACK_ON_SINGLE_BIT);
        size_t i = 0;
        if (!ExecuteRegExp(cx, res, re, linearStr, chars, charsLen, &i, type, rval))
            return false;
        if (callbackOnSingle && Matched(type, *rval) && !callback(cx, res, 0, data))
            return false;
    }
    return true;
}

static bool
BuildFlatMatchArray(JSContext *cx, HandleString textstr, const FlatMatch &fm, CallArgs *args)
{
    if (fm.match() < 0) {
        args->rval().setNull();
        return true;
    }

    
    RootedObject obj(cx, NewSlowEmptyArray(cx));
    if (!obj)
        return false;

    RootedValue patternVal(cx, StringValue(fm.pattern()));
    RootedValue matchVal(cx, Int32Value(fm.match()));
    RootedValue textVal(cx, StringValue(textstr));

    if (!obj->defineElement(cx, 0, patternVal) ||
        !obj->defineProperty(cx, cx->runtime->atomState.indexAtom, matchVal) ||
        !obj->defineProperty(cx, cx->runtime->atomState.inputAtom, textVal))
    {
        return false;
    }

    args->rval().setObject(*obj);
    return true;
}

typedef JSObject **MatchArgType;





static bool
MatchCallback(JSContext *cx, RegExpStatics *res, size_t count, void *p)
{
    JS_ASSERT(count <= JSID_INT_MAX);  

    JSObject *&arrayobj = *static_cast<MatchArgType>(p);
    if (!arrayobj) {
        arrayobj = NewDenseEmptyArray(cx);
        if (!arrayobj)
            return false;
    }

    RootedValue v(cx);
    return res->createLastMatch(cx, v.address()) && arrayobj->defineElement(cx, count, v);
}

JSBool
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

    RootedObject array(cx);
    MatchArgType arg = array.address();
    RegExpStatics *res = cx->regExpStatics();
    Value rval;
    if (!DoMatch(cx, res, str, g.regExp(), MatchCallback, arg, MATCH_ARGS, &rval))
        return false;

    if (g.regExp().global())
        args.rval().setObjectOrNull(array);
    else
        args.rval().set(rval);
    return true;
}

JSBool
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

    JSLinearString *linearStr = str->ensureLinear(cx);
    if (!linearStr)
        return false;

    const jschar *chars = linearStr->chars();
    size_t length = linearStr->length();
    RegExpStatics *res = cx->regExpStatics();

    
    size_t i = 0;
    Value result;
    if (!ExecuteRegExp(cx, res, g.regExp(), linearStr, chars, length, &i, RegExpTest, &result))
        return false;

    if (result.isTrue())
        args.rval().setInt32(res->matchStart());
    else
        args.rval().setInt32(-1);
    return true;
}

struct ReplaceData
{
    ReplaceData(JSContext *cx)
      : str(cx), g(cx), lambda(cx), elembase(cx), repstr(cx),
        dollarRoot(cx, &dollar), dollarEndRoot(cx, &dollarEnd), sb(cx)
    {}

    RootedString       str;            
    StringRegExpGuard  g;              
    RootedObject       lambda;         
    RootedObject       elembase;       
    Rooted<JSLinearString*> repstr; 
    const jschar       *dollar;        
    const jschar       *dollarEnd;     
    SkipRoot           dollarRoot;     
    SkipRoot           dollarEndRoot;  
    int                leftIndex;      
    JSSubString        dollarStr;      
    bool               calledBack;     
    InvokeArgsGuard    args;           
    StringBuffer       sb;             
};

static bool
InterpretDollar(JSContext *cx, RegExpStatics *res, const jschar *dp, const jschar *ep,
                ReplaceData &rdata, JSSubString *out, size_t *skip)
{
    JS_ASSERT(*dp == '$');

    
    if (dp + 1 >= ep)
        return false;

    
    jschar dc = dp[1];
    if (JS7_ISDEC(dc)) {
        
        unsigned num = JS7_UNDEC(dc);
        if (num > res->parenCount())
            return false;

        const jschar *cp = dp + 2;
        if (cp < ep && (dc = *cp, JS7_ISDEC(dc))) {
            unsigned tmp = 10 * num + JS7_UNDEC(dc);
            if (tmp <= res->parenCount()) {
                cp++;
                num = tmp;
            }
        }
        if (num == 0)
            return false;

        *skip = cp - dp;

        JS_ASSERT(num <= res->parenCount());

        



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
    RootedObject base(cx, rdata.elembase);
    if (base) {
        





        JS_ASSERT(rdata.lambda);
        JS_ASSERT(!base->getOps()->lookupProperty);
        JS_ASSERT(!base->getOps()->getProperty);

        Value match;
        if (!res->createLastMatch(cx, &match))
            return false;
        JSString *str = match.toString();

        JSAtom *atom;
        if (str->isAtom()) {
            atom = &str->asAtom();
        } else {
            atom = js_AtomizeString(cx, str);
            if (!atom)
                return false;
        }

        Value v;
        if (HasDataProperty(cx, base, AtomToId(atom), &v) && v.isString()) {
            rdata.repstr = v.toString()->ensureLinear(cx);
            if (!rdata.repstr)
                return false;
            *sizep = rdata.repstr->length();
            return true;
        }

        



        rdata.elembase = NULL;
    }

    if (JSObject *lambda = rdata.lambda) {
        PreserveRegExpStatics staticsGuard(cx, res);
        if (!staticsGuard.init(cx))
            return false;

        







        unsigned p = res->parenCount();
        unsigned argc = 1 + p + 2;

        InvokeArgsGuard &args = rdata.args;
        if (!args.pushed() && !cx->stack.pushInvokeArgs(cx, argc, &args))
            return false;

        args.setCallee(ObjectValue(*lambda));
        args.thisv() = UndefinedValue();

        
        unsigned argi = 0;
        if (!res->createLastMatch(cx, &args[argi++]))
            return false;

        for (size_t i = 0; i < res->parenCount(); ++i) {
            if (!res->createParen(cx, i + 1, &args[argi++]))
                return false;
        }

        
        args[argi++].setInt32(res->matchStart());
        args[argi].setString(rdata.str);

        if (!Invoke(cx, args))
            return false;

        
        JSString *repstr = ToString(cx, args.rval());
        if (!repstr)
            return false;
        rdata.repstr = repstr->ensureLinear(cx);
        if (!rdata.repstr)
            return false;
        *sizep = rdata.repstr->length();
        return true;
    }

    JSString *repstr = rdata.repstr;
    size_t replen = repstr->length();
    for (const jschar *dp = rdata.dollar, *ep = rdata.dollarEnd; dp;
         dp = js_strchr_limit(dp, '$', ep)) {
        JSSubString sub;
        size_t skip;
        if (InterpretDollar(cx, res, dp, ep, rdata, &sub, &skip)) {
            replen += sub.length - skip;
            dp += skip;
        } else {
            dp++;
        }
    }
    *sizep = replen;
    return true;
}





static void
DoReplace(JSContext *cx, RegExpStatics *res, ReplaceData &rdata)
{
    JSLinearString *repstr = rdata.repstr;
    const jschar *cp;
    const jschar *bp = cp = repstr->chars();

    const jschar *dp = rdata.dollar;
    const jschar *ep = rdata.dollarEnd;
    for (; dp; dp = js_strchr_limit(dp, '$', ep)) {
        
        size_t len = dp - cp;
        rdata.sb.infallibleAppend(cp, len);
        cp = dp;

        JSSubString sub;
        size_t skip;
        if (InterpretDollar(cx, res, dp, ep, rdata, &sub, &skip)) {
            len = sub.length;
            rdata.sb.infallibleAppend(sub.chars, len);
            cp += skip;
            dp += skip;
        } else {
            dp++;
        }
    }
    rdata.sb.infallibleAppend(cp, repstr->length() - (cp - bp));
}

static bool
ReplaceRegExpCallback(JSContext *cx, RegExpStatics *res, size_t count, void *p)
{
    ReplaceData &rdata = *static_cast<ReplaceData *>(p);

    rdata.calledBack = true;
    size_t leftoff = rdata.leftIndex;
    size_t leftlen = res->matchStart() - leftoff;
    rdata.leftIndex = res->matchLimit();

    size_t replen = 0;  
    if (!FindReplaceLength(cx, res, rdata, &replen))
        return false;

    size_t growth = leftlen + replen;
    if (!rdata.sb.reserve(rdata.sb.length() + growth))
        return false;

    JSLinearString &str = rdata.str->asLinear();  
    const jschar *left = str.chars() + leftoff;

    rdata.sb.infallibleAppend(left, leftlen); 
    DoReplace(cx, res, rdata);
    return true;
}

static bool
BuildFlatReplacement(JSContext *cx, HandleString textstr, HandleString repstr,
                     const FlatMatch &fm, CallArgs *args)
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

    args->rval().setString(builder.result());
    return true;
}







static inline bool
BuildDollarReplacement(JSContext *cx, JSString *textstrArg, JSLinearString *repstr,
                       const jschar *firstDollar, const FlatMatch &fm, CallArgs *args)
{
    Rooted<JSLinearString*> textstr(cx, textstrArg->ensureLinear(cx));
    if (!textstr)
        return false;

    JS_ASSERT(repstr->chars() <= firstDollar && firstDollar < repstr->chars() + repstr->length());
    size_t matchStart = fm.match();
    size_t matchLimit = matchStart + fm.patternLength();

    






    StringBuffer newReplaceChars(cx);
    if (!newReplaceChars.reserve(textstr->length() - fm.patternLength() + repstr->length()))
        return false;

    
    newReplaceChars.infallibleAppend(repstr->chars(), firstDollar);

    
#define ENSURE(__cond) if (!(__cond)) return false;
    const jschar *repstrLimit = repstr->chars() + repstr->length();
    for (const jschar *it = firstDollar; it < repstrLimit; ++it) {
        if (*it != '$' || it == repstrLimit - 1) {
            ENSURE(newReplaceChars.append(*it));
            continue;
        }

        switch (*(it + 1)) {
          case '$': 
            ENSURE(newReplaceChars.append(*it));
            break;
          case '&':
            ENSURE(newReplaceChars.append(textstr->chars() + matchStart,
                                          textstr->chars() + matchLimit));
            break;
          case '`':
            ENSURE(newReplaceChars.append(textstr->chars(), textstr->chars() + matchStart));
            break;
          case '\'':
            ENSURE(newReplaceChars.append(textstr->chars() + matchLimit,
                                          textstr->chars() + textstr->length()));
            break;
          default: 
            ENSURE(newReplaceChars.append(*it));
            continue;
        }
        ++it; 
    }

    RootedString leftSide(cx, js_NewDependentString(cx, textstr, 0, matchStart));
    ENSURE(leftSide);

    RootedString newReplace(cx, newReplaceChars.finishString());
    ENSURE(newReplace);

    JS_ASSERT(textstr->length() >= matchLimit);
    RootedString rightSide(cx, js_NewDependentString(cx, textstr, matchLimit,
                                                        textstr->length() - matchLimit));
    ENSURE(rightSide);

    RopeBuilder builder(cx);
    ENSURE(builder.append(leftSide) &&
           builder.append(newReplace) &&
           builder.append(rightSide));
#undef ENSURE

    args->rval().setString(builder.result());
    return true;
}

static inline bool
str_replace_regexp(JSContext *cx, CallArgs args, ReplaceData &rdata)
{
    if (!rdata.g.normalizeRegExp(cx, true, 2, args))
        return false;

    rdata.leftIndex = 0;
    rdata.calledBack = false;

    RegExpStatics *res = cx->regExpStatics();
    RegExpShared &re = rdata.g.regExp();

    Value tmp;
    if (!DoMatch(cx, res, rdata.str, re, ReplaceRegExpCallback, &rdata, REPLACE_ARGS, &tmp))
        return false;

    if (!rdata.calledBack) {
        
        args.rval().setString(rdata.str);
        return true;
    }

    JSSubString sub;
    res->getRightContext(&sub);
    if (!rdata.sb.append(sub.chars, sub.length))
        return false;

    JSString *retstr = rdata.sb.finishString();
    if (!retstr)
        return false;

    args.rval().setString(retstr);
    return true;
}

static inline bool
str_replace_flat_lambda(JSContext *cx, CallArgs outerArgs, ReplaceData &rdata, const FlatMatch &fm)
{
    JS_ASSERT(fm.match() >= 0);

    JSString *matchStr = js_NewDependentString(cx, rdata.str, fm.match(), fm.patternLength());
    if (!matchStr)
        return false;

    
    static const uint32_t lambdaArgc = 3;
    if (!cx->stack.pushInvokeArgs(cx, lambdaArgc, &rdata.args))
        return false;

    CallArgs &args = rdata.args;
    args.calleev().setObject(*rdata.lambda);
    args.thisv().setUndefined();

    Value *sp = args.array();
    sp[0].setString(matchStr);
    sp[1].setInt32(fm.match());
    sp[2].setString(rdata.str);

    if (!Invoke(cx, rdata.args))
        return false;

    RootedString repstr(cx, ToString(cx, args.rval()));
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

static const uint32_t ReplaceOptArg = 2;








static JSObject *
LambdaIsGetElem(JSObject &lambda, JSContext *cx)
{
    if (!lambda.isFunction())
        return NULL;

    JSFunction *fun = lambda.toFunction();
    if (!fun->isInterpreted())
        return NULL;

    JSScript *script = fun->script();
    jsbytecode *pc = script->code;

    




    if (JSOp(*pc) != JSOP_GETALIASEDVAR || fun->isHeavyweight())
        return NULL;
    ScopeCoordinate sc(pc);
    ScopeObject *scope = &fun->environment()->asScope();
    for (unsigned i = 0; i < sc.hops; ++i)
        scope = &scope->enclosingScope().asScope();
    Value b = scope->aliasedVar(sc);
    pc += JSOP_GETALIASEDVAR_LENGTH;

    
    if (JSOp(*pc) != JSOP_GETARG || GET_SLOTNO(pc) != 0)
        return NULL;
    pc += JSOP_GETARG_LENGTH;

    
    if (JSOp(*pc) != JSOP_GETELEM)
        return NULL;
    pc += JSOP_GETELEM_LENGTH;

    
    if (JSOp(*pc) != JSOP_RETURN)
        return NULL;

    
    if (!b.isObject())
        return NULL;

    JSObject &bobj = b.toObject();
    Class *clasp = bobj.getClass();
    if (!clasp->isNative() || clasp->ops.lookupProperty || clasp->ops.getProperty)
        return NULL;

    return &bobj;
}

JSBool
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
        rdata.lambda = &args[1].toObject();
        rdata.elembase = NULL;
        rdata.repstr = NULL;
        rdata.dollar = rdata.dollarEnd = NULL;

        if (JSObject *base = LambdaIsGetElem(*rdata.lambda, cx))
            rdata.elembase = base;
    } else {
        rdata.lambda = NULL;
        rdata.elembase = NULL;
        rdata.repstr = ArgToRootedString(cx, args, 1);
        if (!rdata.repstr)
            return false;

        
        JSFixedString *fixed = rdata.repstr->ensureFixed(cx);
        if (!fixed)
            return false;
        rdata.dollarEnd = fixed->chars() + fixed->length();
        rdata.dollar = js_strchr_limit(fixed->chars(), '$', rdata.dollarEnd);
    }

    









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

    



    if (rdata.dollar)
        return BuildDollarReplacement(cx, rdata.str, rdata.repstr, rdata.dollar, *fm, &args);

    return BuildFlatReplacement(cx, rdata.str, rdata.repstr, *fm, &args);
}

class SplitMatchResult {
    size_t endIndex_;
    size_t length_;

  public:
    void setFailure() {
        JS_STATIC_ASSERT(SIZE_MAX > JSString::MAX_LENGTH);
        endIndex_ = SIZE_MAX;
    }
    bool isFailure() const {
        return (endIndex_ == SIZE_MAX);
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

template<class Matcher>
static JSObject *
SplitHelper(JSContext *cx, Handle<JSLinearString*> str, uint32_t limit, const Matcher &splitMatch,
            Handle<TypeObject*> type)
{
    size_t strLength = str->length();
    SplitMatchResult result;

    
    if (strLength == 0) {
        if (!splitMatch(cx, str, 0, &result))
            return NULL;

        









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
            return NULL;

        














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
            return NULL;

        
        if (splits.length() == limit)
            return NewDenseCopiedArray(cx, splits.length(), splits.begin());

        
        lastEndIndex = endIndex;

        
        if (Matcher::returnsCaptures) {
            RegExpStatics *res = cx->regExpStatics();
            for (size_t i = 0; i < res->parenCount(); i++) {
                
                if (res->pairIsPresent(i + 1)) {
                    JSSubString parsub;
                    res->getParen(i + 1, &parsub);
                    sub = js_NewStringCopyN(cx, parsub.chars, parsub.length);
                    if (!sub || !splits.append(StringValue(sub)))
                        return NULL;
                } else {
                    
                    AddTypeProperty(cx, type, NULL, UndefinedValue());
                    if (!splits.append(UndefinedValue()))
                        return NULL;
                }

                
                if (splits.length() == limit)
                    return NewDenseCopiedArray(cx, splits.length(), splits.begin());
            }
        }

        
        index = lastEndIndex;
    }

    
    JSString *sub = js_NewDependentString(cx, str, lastEndIndex, strLength - lastEndIndex);
    if (!sub || !splits.append(StringValue(sub)))
        return NULL;

    
    return NewDenseCopiedArray(cx, splits.length(), splits.begin());
}








class SplitRegExpMatcher
{
    RegExpShared &re;
    RegExpStatics *res;

  public:
    SplitRegExpMatcher(RegExpShared &re, RegExpStatics *res) : re(re), res(res) {}

    static const bool returnsCaptures = true;

    bool operator()(JSContext *cx, JSLinearString *str, size_t index,
                    SplitMatchResult *result) const
    {
        Value rval = UndefinedValue();
        const jschar *chars = str->chars();
        size_t length = str->length();
        if (!ExecuteRegExp(cx, res, re, str, chars, length, &index, RegExpTest, &rval))
            return false;
        if (!rval.isTrue()) {
            result->setFailure();
            return true;
        }
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
    SplitStringMatcher(JSContext *cx, JSLinearString *sep)
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


JSBool
js::str_split(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    RootedTypeObject type(cx, GetTypeCallerInitObject(cx, JSProto_Array));
    if (!type)
        return false;
    AddTypeProperty(cx, type, NULL, Type::StringType());

    
    uint32_t limit;
    if (args.hasDefined(1)) {
        double d;
        if (!ToNumber(cx, args[1], &d))
            return false;
        limit = ToUint32(d);
    } else {
        limit = UINT32_MAX;
    }

    
    RegExpGuard re;
    JSLinearString *sepstr = NULL;
    bool sepDefined = args.hasDefined(0);
    if (sepDefined) {
        if (IsObjectWithClass(args[0], ESClass_RegExp, cx)) {
            if (!RegExpToShared(cx, args[0].toObject(), &re))
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
        Value v = StringValue(str);
        JSObject *aobj = NewDenseCopiedArray(cx, 1, &v);
        if (!aobj)
            return false;
        aobj->setType(type);
        args.rval().setObject(*aobj);
        return true;
    }
    Rooted<JSLinearString*> strlin(cx, str->ensureLinear(cx));
    if (!strlin)
        return false;

    
    JSObject *aobj;
    if (!re.initialized()) {
        SplitStringMatcher matcher(cx, sepstr);
        aobj = SplitHelper(cx, strlin, limit, matcher, type);
    } else {
        SplitRegExpMatcher matcher(*re, cx->regExpStatics());
        aobj = SplitHelper(cx, strlin, limit, matcher, type);
    }
    if (!aobj)
        return false;

    
    aobj->setType(type);
    args.rval().setObject(*aobj);
    return true;
}

static JSBool
str_substr(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *str = ThisToStringForStringProto(cx, args);
    if (!str)
        return false;

    int32_t length, len, begin;
    if (args.length() > 0) {
        length = int32_t(str->length());
        if (!ValueToIntegerRange(cx, args[0], &begin))
            return false;

        if (begin >= length) {
            str = cx->runtime->emptyString;
            goto out;
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
                str = cx->runtime->emptyString;
                goto out;
            }

            if (uint32_t(length) < uint32_t(begin + len))
                len = length - begin;
        } else {
            len = length - begin;
        }

        str = js_NewDependentString(cx, str, size_t(begin), size_t(len));
        if (!str)
            return false;
    }

out:
    args.rval().setString(str);
    return true;
}




static JSBool
str_concat(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    RootedString str(cx, ThisToStringForStringProto(cx, args));
    if (!str)
        return false;

    for (unsigned i = 0; i < args.length(); i++) {
        RootedString argStr(cx, ToString(cx, args[i]));
        if (!argStr)
            return false;

        str = js_ConcatStrings(cx, str, argStr);
        if (!str)
            return false;
    }

    args.rval().setString(str);
    return true;
}

static JSBool
str_slice(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() == 1 && args.thisv().isString() && args[0].isInt32()) {
        size_t begin, end, length;

        JSString *str = args.thisv().toString();
        begin = args[0].toInt32();
        end = str->length();
        if (begin <= end) {
            length = end - begin;
            if (length == 0) {
                str = cx->runtime->emptyString;
            } else {
                str = (length == 1)
                      ? cx->runtime->staticStrings.getUnitStringForElement(cx, str, begin)
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

#if JS_HAS_STR_HTML_HELPERS



static bool
tagify(JSContext *cx, const char *begin, JSLinearString *param, const char *end,
       CallReceiver call)
{
    JSString *thisstr = ThisToStringForStringProto(cx, call);
    if (!thisstr)
        return false;

    JSLinearString *str = thisstr->ensureLinear(cx);
    if (!str)
        return false;

    if (!end)
        end = begin;

    size_t beglen = strlen(begin);
    size_t taglen = 1 + beglen + 1;                     
    if (param) {
        size_t numChars = param->length();
        const jschar *parchars = param->chars();
        for (size_t i = 0, parlen = numChars; i < parlen; ++i) {
            if (parchars[i] == '"')
                numChars += 5;                          
        }
        taglen += 2 + numChars + 1;                     
    }
    size_t endlen = strlen(end);
    taglen += str->length() + 2 + endlen + 1;           


    StringBuffer sb(cx);
    if (!sb.reserve(taglen))
        return false;

    sb.infallibleAppend('<');

    MOZ_ALWAYS_TRUE(sb.appendInflated(begin, beglen));

    if (param) {
        sb.infallibleAppend('=');
        sb.infallibleAppend('"');
        const jschar *parchars = param->chars();
        for (size_t i = 0, parlen = param->length(); i < parlen; ++i) {
            if (parchars[i] != '"') {
                sb.infallibleAppend(parchars[i]);
            } else {
                MOZ_ALWAYS_TRUE(sb.append("&quot;"));
            }
        }
        sb.infallibleAppend('"');
    }
    
    sb.infallibleAppend('>');

    MOZ_ALWAYS_TRUE(sb.append(str));

    sb.infallibleAppend('<');
    sb.infallibleAppend('/');

    MOZ_ALWAYS_TRUE(sb.appendInflated(end, endlen));

    sb.infallibleAppend('>');

    JSFixedString *retstr = sb.finishString();
    if (!retstr)
        return false;

    call.rval().setString(retstr);
    return true;
}

static JSBool
tagify_value(JSContext *cx, CallArgs args, const char *begin, const char *end)
{
    JSLinearString *param = ArgToRootedString(cx, args, 0);
    if (!param)
        return false;

    return tagify(cx, begin, param, end, args);
}

static JSBool
str_bold(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "b", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_italics(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "i", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_fixed(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "tt", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_fontsize(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify_value(cx, CallArgsFromVp(argc, vp), "font size", "font");
}

static JSBool
str_fontcolor(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify_value(cx, CallArgsFromVp(argc, vp), "font color", "font");
}

static JSBool
str_link(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify_value(cx, CallArgsFromVp(argc, vp), "a href", "a");
}

static JSBool
str_anchor(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify_value(cx, CallArgsFromVp(argc, vp), "a name", "a");
}

static JSBool
str_strike(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "strike", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_small(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "small", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_big(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "big", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_blink(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "blink", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_sup(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "sup", NULL, NULL, CallReceiverFromVp(vp));
}

static JSBool
str_sub(JSContext *cx, unsigned argc, Value *vp)
{
    return tagify(cx, "sub", NULL, NULL, CallReceiverFromVp(vp));
}
#endif 

static JSFunctionSpec string_methods[] = {
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
    JS_FN("localeCompare",     str_localeCompare,     1,JSFUN_GENERIC_NATIVE),

    
    JS_FN("match",             str_match,             1,JSFUN_GENERIC_NATIVE),
    JS_FN("search",            str_search,            1,JSFUN_GENERIC_NATIVE),
    JS_FN("replace",           str_replace,           2,JSFUN_GENERIC_NATIVE),
    JS_FN("split",             str_split,             2,JSFUN_GENERIC_NATIVE),
    JS_FN("substr",            str_substr,            2,JSFUN_GENERIC_NATIVE),

    
    JS_FN("concat",            str_concat,            1,JSFUN_GENERIC_NATIVE),
    JS_FN("slice",             str_slice,             2,JSFUN_GENERIC_NATIVE),

    
#if JS_HAS_STR_HTML_HELPERS
    JS_FN("bold",              str_bold,              0,0),
    JS_FN("italics",           str_italics,           0,0),
    JS_FN("fixed",             str_fixed,             0,0),
    JS_FN("fontsize",          str_fontsize,          1,0),
    JS_FN("fontcolor",         str_fontcolor,         1,0),
    JS_FN("link",              str_link,              1,0),
    JS_FN("anchor",            str_anchor,            1,0),
    JS_FN("strike",            str_strike,            0,0),
    JS_FN("small",             str_small,             0,0),
    JS_FN("big",               str_big,               0,0),
    JS_FN("blink",             str_blink,             0,0),
    JS_FN("sup",               str_sup,               0,0),
    JS_FN("sub",               str_sub,               0,0),
#endif

    JS_FN("iterator",          JS_ArrayIterator,      0,0),
    JS_FS_END
};

JSBool
js_String(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedString str(cx);
    if (args.length() > 0) {
        str = ToString(cx, args[0]);
        if (!str)
            return false;
    } else {
        str = cx->runtime->emptyString;
    }

    if (IsConstructing(args)) {
        StringObject *strobj = StringObject::create(cx, str);
        if (!strobj)
            return false;
        args.rval().setObject(*strobj);
        return true;
    }

    args.rval().setString(str);
    return true;
}

JSBool
js::str_fromCharCode(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    JS_ASSERT(args.length() <= StackSpace::ARGS_LENGTH_MAX);
    if (args.length() == 1) {
        uint16_t code;
        if (!ToUint16(cx, args[0], &code))
            return JS_FALSE;
        if (StaticStrings::hasUnit(code)) {
            args.rval().setString(cx->runtime->staticStrings.getUnit(code));
            return JS_TRUE;
        }
        args[0].setInt32(code);
    }
    jschar *chars = (jschar *) cx->malloc_((args.length() + 1) * sizeof(jschar));
    if (!chars)
        return JS_FALSE;
    for (unsigned i = 0; i < args.length(); i++) {
        uint16_t code;
        if (!ToUint16(cx, args[i], &code)) {
            cx->free_(chars);
            return JS_FALSE;
        }
        chars[i] = (jschar)code;
    }
    chars[args.length()] = 0;
    JSString *str = js_NewString(cx, chars, args.length());
    if (!str) {
        cx->free_(chars);
        return JS_FALSE;
    }

    args.rval().setString(str);
    return JS_TRUE;
}

static JSFunctionSpec string_static_methods[] = {
    JS_FN("fromCharCode", js::str_fromCharCode, 1, 0),
    JS_FS_END
};

Shape *
StringObject::assignInitialShape(JSContext *cx)
{
    JS_ASSERT(nativeEmpty());

    return addDataProperty(cx, NameToId(cx->runtime->atomState.lengthAtom),
                           LENGTH_SLOT, JSPROP_PERMANENT | JSPROP_READONLY);
}

JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    Rooted<GlobalObject*> global(cx, &obj->asGlobal());

    Rooted<JSString*> empty(cx, cx->runtime->emptyString);
    RootedObject proto(cx, global->createBlankPrototype(cx, &StringClass));
    if (!proto || !proto->asString().init(cx, empty))
        return NULL;

    
    RootedFunction ctor(cx, global->createConstructor(cx, js_String, CLASS_NAME(cx, String), 1));
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, proto, NULL, string_methods) ||
        !DefinePropertiesAndBrand(cx, ctor, NULL, string_static_methods))
    {
        return NULL;
    }

    
    TypeObject *type = proto->getNewType(cx);
    if (!type)
        return NULL;
    AddTypeProperty(cx, type, "length", Type::Int32Type());

    if (!DefineConstructorAndPrototype(cx, global, JSProto_String, ctor, proto))
        return NULL;

    



    if (!JS_DefineFunctions(cx, global, string_functions))
        return NULL;

    return proto;
}

JSFixedString *
js_NewString(JSContext *cx, jschar *chars, size_t length)
{
    JSFixedString *s = JSFixedString::new_(cx, chars, length);
    if (s)
        Probes::createString(cx, s, length);
    return s;
}

static JSInlineString *
NewShortString(JSContext *cx, const char *chars, size_t length)
{
    JS_ASSERT(JSShortString::lengthFits(length));
    JSInlineString *str = JSInlineString::lengthFits(length)
                          ? JSInlineString::new_(cx)
                          : JSShortString::new_(cx);
    if (!str)
        return NULL;

    jschar *storage = str->init(length);
    if (js_CStringsAreUTF8) {
#ifdef DEBUG
        size_t oldLength = length;
#endif
        if (!InflateUTF8StringToBuffer(cx, chars, length, storage, &length))
            return NULL;
        JS_ASSERT(length <= oldLength);
        storage[length] = 0;
        str->resetLength(length);
    } else {
        size_t n = length;
        jschar *p = storage;
        while (n--)
            *p++ = (unsigned char)*chars++;
        *p = 0;
    }
    Probes::createString(cx, str, length);
    return str;
}

JSLinearString *
js_NewDependentString(JSContext *cx, JSString *baseArg, size_t start, size_t length)
{
    if (length == 0)
        return cx->runtime->emptyString;

    JSLinearString *base = baseArg->ensureLinear(cx);
    if (!base)
        return NULL;

    if (start == 0 && length == base->length())
        return base;

    const jschar *chars = base->chars() + start;

    if (JSLinearString *staticStr = cx->runtime->staticStrings.lookup(chars, length))
        return staticStr;

    JSLinearString *s = JSDependentString::new_(cx, base, chars, length);
    Probes::createString(cx, s, length);
    return s;
}

JSFixedString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n)
{
    if (JSShortString::lengthFits(n))
        return NewShortString(cx, s, n);

    jschar *news = (jschar *) cx->malloc_((n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    js_strncpy(news, s, n);
    news[n] = 0;
    JSFixedString *str = js_NewString(cx, news, n);
    if (!str)
        cx->free_(news);
    return str;
}

JSFixedString *
js_NewStringCopyN(JSContext *cx, const char *s, size_t n)
{
    if (JSShortString::lengthFits(n))
        return NewShortString(cx, s, n);

    jschar *chars = InflateString(cx, s, &n);
    if (!chars)
        return NULL;
    JSFixedString *str = js_NewString(cx, chars, n);
    if (!str)
        cx->free_(chars);
    return str;
}

JSFixedString *
js_NewStringCopyZ(JSContext *cx, const jschar *s)
{
    size_t n = js_strlen(s);
    if (JSShortString::lengthFits(n))
        return NewShortString(cx, s, n);

    size_t m = (n + 1) * sizeof(jschar);
    jschar *news = (jschar *) cx->malloc_(m);
    if (!news)
        return NULL;
    js_memcpy(news, s, m);
    JSFixedString *str = js_NewString(cx, news, n);
    if (!str)
        cx->free_(news);
    return str;
}

JSFixedString *
js_NewStringCopyZ(JSContext *cx, const char *s)
{
    return js_NewStringCopyN(cx, s, strlen(s));
}

const char *
js_ValueToPrintable(JSContext *cx, const Value &v, JSAutoByteString *bytes, bool asSource)
{
    JSString *str;

    str = (asSource ? js_ValueToSource : ToString)(cx, v);
    if (!str)
        return NULL;
    str = js_QuoteString(cx, str, 0);
    if (!str)
        return NULL;
    return bytes->encode(cx, str);
}

JSString *
js::ToStringSlow(JSContext *cx, const Value &arg)
{
    
    JS_ASSERT(!arg.isString());

    Value v = arg;
    if (!ToPrimitive(cx, JSTYPE_STRING, &v))
        return NULL;

    JSString *str;
    if (v.isString()) {
        str = v.toString();
    } else if (v.isInt32()) {
        str = Int32ToString(cx, v.toInt32());
    } else if (v.isDouble()) {
        str = js_NumberToString(cx, v.toDouble());
    } else if (v.isBoolean()) {
        str = js_BooleanToString(cx, v.toBoolean());
    } else if (v.isNull()) {
        str = cx->runtime->atomState.nullAtom;
    } else {
        str = cx->runtime->atomState.typeAtoms[JSTYPE_VOID];
    }
    return str;
}

JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, const Value &v)
{
    JS_CHECK_RECURSION(cx, return NULL);

    if (v.isUndefined())
        return cx->runtime->atomState.void0Atom;
    if (v.isString())
        return js_QuoteString(cx, v.toString(), '"');
    if (v.isPrimitive()) {
        
        if (v.isDouble() && MOZ_DOUBLE_IS_NEGATIVE_ZERO(v.toDouble())) {
            
            static const jschar js_negzero_ucNstr[] = {'-', '0'};

            return js_NewStringCopyN(cx, js_negzero_ucNstr, 2);
        }
        return ToString(cx, v);
    }

    Value rval = NullValue();
    RootedValue fval(cx);
    RootedId id(cx, NameToId(cx->runtime->atomState.toSourceAtom));
    Rooted<JSObject*> obj(cx, &v.toObject());
    if (!GetMethod(cx, obj, id, 0, &fval))
        return NULL;
    if (js_IsCallable(fval)) {
        if (!Invoke(cx, ObjectValue(*obj), fval, 0, NULL, &rval))
            return NULL;
    }

    return ToString(cx, rval);
}

namespace js {

bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, bool *result)
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

    *result = PodEqual(linear1->chars(), linear2->chars(), length1);
    return true;
}

bool
EqualStrings(JSLinearString *str1, JSLinearString *str2)
{
    if (str1 == str2)
        return true;

    size_t length1 = str1->length();
    if (length1 != str2->length())
        return false;

    return PodEqual(str1->chars(), str2->chars(), length1);
}

}  

namespace js {

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

    return CompareChars(s1, str1->length(), s2, str2->length(), result);
}

bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32_t *result)
{
    return CompareStringsImpl(cx, str1, str2, result);
}

}  

namespace js {

bool
StringEqualsAscii(JSLinearString *str, const char *asciiBytes)
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

} 

size_t
js_strlen(const jschar *s)
{
    const jschar *t;

    for (t = s; *t != 0; t++)
        continue;
    return (size_t)(t - s);
}

jschar *
js_strchr(const jschar *s, jschar c)
{
    while (*s != 0) {
        if (*s == c)
            return (jschar *)s;
        s++;
    }
    return NULL;
}

jschar *
js_strchr_limit(const jschar *s, jschar c, const jschar *limit)
{
    while (s < limit) {
        if (*s == c)
            return (jschar *)s;
        s++;
    }
    return NULL;
}

namespace js {

jschar *
InflateString(JSContext *cx, const char *bytes, size_t *lengthp, FlationCoding fc)
{
    size_t nchars;
    jschar *chars;
    size_t nbytes = *lengthp;

    
    MaybeCheckStackRoots(cx);

    if (js_CStringsAreUTF8 || fc == CESU8Encoding) {
        if (!InflateUTF8StringToBuffer(cx, bytes, nbytes, NULL, &nchars, fc))
            goto bad;
        chars = (jschar *) cx->malloc_((nchars + 1) * sizeof (jschar));
        if (!chars)
            goto bad;
        JS_ALWAYS_TRUE(InflateUTF8StringToBuffer(cx, bytes, nbytes, chars, &nchars, fc));
    } else {
        nchars = nbytes;
        chars = (jschar *) cx->malloc_((nchars + 1) * sizeof(jschar));
        if (!chars)
            goto bad;
        for (size_t i = 0; i < nchars; i++)
            chars[i] = (unsigned char) bytes[i];
    }
    *lengthp = nchars;
    chars[nchars] = 0;
    return chars;

  bad:
    



    *lengthp = 0;
    return NULL;
}




char *
DeflateString(JSContext *cx, const jschar *chars, size_t nchars)
{
    size_t nbytes, i;
    char *bytes;

    if (js_CStringsAreUTF8) {
        nbytes = GetDeflatedStringLength(cx, chars, nchars);
        if (nbytes == (size_t) -1)
            return NULL;
        bytes = (char *) (cx ? cx->malloc_(nbytes + 1) : OffTheBooks::malloc_(nbytes + 1));
        if (!bytes)
            return NULL;
        JS_ALWAYS_TRUE(DeflateStringToBuffer(cx, chars, nchars, bytes, &nbytes));
    } else {
        nbytes = nchars;
        bytes = (char *) (cx ? cx->malloc_(nbytes + 1) : OffTheBooks::malloc_(nbytes + 1));
        if (!bytes)
            return NULL;
        for (i = 0; i < nbytes; i++)
            bytes[i] = (char) chars[i];
    }
    bytes[nbytes] = 0;
    return bytes;
}

size_t
GetDeflatedStringLength(JSContext *cx, const jschar *chars, size_t nchars)
{
    if (!js_CStringsAreUTF8)
        return nchars;

    return GetDeflatedUTF8StringLength(cx, chars, nchars);
}




size_t
GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars,
                                size_t nchars, FlationCoding fc)
{
    size_t nbytes;
    const jschar *end;
    unsigned c, c2;
    char buffer[10];
    bool useCESU8 = fc == CESU8Encoding;

    nbytes = nchars;
    for (end = chars + nchars; chars != end; chars++) {
        c = *chars;
        if (c < 0x80)
            continue;
        if (0xD800 <= c && c <= 0xDFFF && !useCESU8) {
            
            chars++;

            
            nbytes--;
            if (c >= 0xDC00 || chars == end)
                goto bad_surrogate;
            c2 = *chars;
            if (c2 < 0xDC00 || c2 > 0xDFFF)
                goto bad_surrogate;
            c = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        c >>= 11;
        nbytes++;
        while (c) {
            c >>= 5;
            nbytes++;
        }
    }
    return nbytes;

  bad_surrogate:
    if (cx) {
        JS_snprintf(buffer, 10, "0x%x", c);
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage,
                                     NULL, JSMSG_BAD_SURROGATE_CHAR, buffer);
    }
    return (size_t) -1;
}

bool
DeflateStringToBuffer(JSContext *cx, const jschar *src, size_t srclen,
                          char *dst, size_t *dstlenp)
{
    size_t dstlen, i;

    dstlen = *dstlenp;
    if (!js_CStringsAreUTF8) {
        if (srclen > dstlen) {
            for (i = 0; i < dstlen; i++)
                dst[i] = (char) src[i];
            if (cx) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BUFFER_TOO_SMALL);
            }
            return JS_FALSE;
        }
        for (i = 0; i < srclen; i++)
            dst[i] = (char) src[i];
        *dstlenp = srclen;
        return JS_TRUE;
    }

    return DeflateStringToUTF8Buffer(cx, src, srclen, dst, dstlenp);
}

bool
DeflateStringToUTF8Buffer(JSContext *cx, const jschar *src, size_t srclen,
                              char *dst, size_t *dstlenp, FlationCoding fc)
{
    size_t i, utf8Len;
    jschar c, c2;
    uint32_t v;
    uint8_t utf8buf[6];

    bool useCESU8 = fc == CESU8Encoding;
    size_t dstlen = *dstlenp;
    size_t origDstlen = dstlen;

    while (srclen) {
        c = *src++;
        srclen--;
        if ((c >= 0xDC00) && (c <= 0xDFFF) && !useCESU8)
            goto badSurrogate;
        if (c < 0xD800 || c > 0xDBFF || useCESU8) {
            v = c;
        } else {
            if (srclen < 1)
                goto badSurrogate;
            c2 = *src;
            if ((c2 < 0xDC00) || (c2 > 0xDFFF))
                goto badSurrogate;
            src++;
            srclen--;
            v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        if (v < 0x0080) {
            
            if (dstlen == 0)
                goto bufferTooSmall;
            *dst++ = (char) v;
            utf8Len = 1;
        } else {
            utf8Len = js_OneUcs4ToUtf8Char(utf8buf, v);
            if (utf8Len > dstlen)
                goto bufferTooSmall;
            for (i = 0; i < utf8Len; i++)
                *dst++ = (char) utf8buf[i];
        }
        dstlen -= utf8Len;
    }
    *dstlenp = (origDstlen - dstlen);
    return JS_TRUE;

badSurrogate:
    *dstlenp = (origDstlen - dstlen);
    
    if (cx)
        GetDeflatedStringLength(cx, src - 1, srclen + 1);
    return JS_FALSE;

bufferTooSmall:
    *dstlenp = (origDstlen - dstlen);
    if (cx) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BUFFER_TOO_SMALL);
    }
    return JS_FALSE;
}

bool
InflateStringToBuffer(JSContext *cx, const char *src, size_t srclen,
                          jschar *dst, size_t *dstlenp)
{
    size_t dstlen, i;

    if (js_CStringsAreUTF8)
        return InflateUTF8StringToBuffer(cx, src, srclen, dst, dstlenp);

    if (dst) {
        dstlen = *dstlenp;
        if (srclen > dstlen) {
            for (i = 0; i < dstlen; i++)
                dst[i] = (unsigned char) src[i];
            if (cx) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BUFFER_TOO_SMALL);
            }
            return JS_FALSE;
        }
        for (i = 0; i < srclen; i++)
            dst[i] = (unsigned char) src[i];
    }
    *dstlenp = srclen;
    return JS_TRUE;
}

bool
InflateUTF8StringToBuffer(JSContext *cx, const char *src, size_t srclen,
                              jschar *dst, size_t *dstlenp, FlationCoding fc)
{
    size_t dstlen, origDstlen, offset, j, n;
    uint32_t v;

    dstlen = dst ? *dstlenp : (size_t) -1;
    origDstlen = dstlen;
    offset = 0;
    bool useCESU8 = fc == CESU8Encoding;

    while (srclen) {
        v = (uint8_t) *src;
        n = 1;
        if (v & 0x80) {
            while (v & (0x80 >> n))
                n++;
            if (n > srclen)
                goto bufferTooSmall;
            if (n == 1 || n > 4)
                goto badCharacter;
            for (j = 1; j < n; j++) {
                if ((src[j] & 0xC0) != 0x80)
                    goto badCharacter;
            }
            v = Utf8ToOneUcs4Char((uint8_t *)src, n);
            if (v >= 0x10000 && !useCESU8) {
                v -= 0x10000;
                if (v > 0xFFFFF || dstlen < 2) {
                    *dstlenp = (origDstlen - dstlen);
                    if (cx) {
                        char buffer[10];
                        JS_snprintf(buffer, 10, "0x%x", v + 0x10000);
                        JS_ReportErrorFlagsAndNumber(cx,
                                                     JSREPORT_ERROR,
                                                     js_GetErrorMessage, NULL,
                                                     JSMSG_UTF8_CHAR_TOO_LARGE,
                                                     buffer);
                    }
                    return JS_FALSE;
                }
                if (dst) {
                    *dst++ = (jschar)((v >> 10) + 0xD800);
                    v = (jschar)((v & 0x3FF) + 0xDC00);
                }
                dstlen--;
            }
        }
        if (!dstlen)
            goto bufferTooSmall;
        if (dst)
            *dst++ = (jschar) v;
        dstlen--;
        offset += n;
        src += n;
        srclen -= n;
    }
    *dstlenp = (origDstlen - dstlen);
    return JS_TRUE;

badCharacter:
    *dstlenp = (origDstlen - dstlen);
    if (cx) {
        char buffer[10];
        JS_snprintf(buffer, 10, "%d", offset);
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR,
                                     js_GetErrorMessage, NULL,
                                     JSMSG_MALFORMED_UTF8_CHAR,
                                     buffer);
    }
    return JS_FALSE;

bufferTooSmall:
    *dstlenp = (origDstlen - dstlen);
    if (cx) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BUFFER_TOO_SMALL);
    }
    return JS_FALSE;
}

} 

const jschar js_uriReservedPlusPound_ucstr[] =
    {';', '/', '?', ':', '@', '&', '=', '+', '$', ',', '#', 0};
const jschar js_uriUnescaped_ucstr[] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
     '-', '_', '.', '!', '~', '*', '\'', '(', ')', 0};

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

#undef ____

#define URI_CHUNK 64U

static inline bool
TransferBufferToString(JSContext *cx, StringBuffer &sb, Value *rval)
{
    JSString *str = sb.finishString();
    if (!str)
        return false;
    rval->setString(str);
    return true;
}








static JSBool
Encode(JSContext *cx, JSString *str, const jschar *unescapedSet,
       const jschar *unescapedSet2, Value *rval)
{
    static const char HexDigits[] = "0123456789ABCDEF"; 

    size_t length = str->length();
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return JS_FALSE;

    if (length == 0) {
        rval->setString(cx->runtime->emptyString);
        return JS_TRUE;
    }

    StringBuffer sb(cx);
    jschar hexBuf[4];
    hexBuf[0] = '%';
    hexBuf[3] = 0;
    for (size_t k = 0; k < length; k++) {
        jschar c = chars[k];
        if (js_strchr(unescapedSet, c) ||
            (unescapedSet2 && js_strchr(unescapedSet2, c))) {
            if (!sb.append(c))
                return JS_FALSE;
        } else {
            if ((c >= 0xDC00) && (c <= 0xDFFF)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_URI, NULL);
                return JS_FALSE;
            }
            uint32_t v;
            if (c < 0xD800 || c > 0xDBFF) {
                v = c;
            } else {
                k++;
                if (k == length) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_URI, NULL);
                    return JS_FALSE;
                }
                jschar c2 = chars[k];
                if ((c2 < 0xDC00) || (c2 > 0xDFFF)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_URI, NULL);
                    return JS_FALSE;
                }
                v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
            }
            uint8_t utf8buf[4];
            size_t L = js_OneUcs4ToUtf8Char(utf8buf, v);
            for (size_t j = 0; j < L; j++) {
                hexBuf[1] = HexDigits[utf8buf[j] >> 4];
                hexBuf[2] = HexDigits[utf8buf[j] & 0xf];
                if (!sb.append(hexBuf, 3))
                    return JS_FALSE;
            }
        }
    }

    return TransferBufferToString(cx, sb, rval);
}

static JSBool
Decode(JSContext *cx, JSString *str, const jschar *reservedSet, Value *rval)
{
    size_t length = str->length();
    const jschar *chars = str->getChars(cx);
    if (!chars)
        return JS_FALSE;

    if (length == 0) {
        rval->setString(cx->runtime->emptyString);
        return JS_TRUE;
    }

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
                uint32_t v = Utf8ToOneUcs4Char(octets, n);
                if (v >= 0x10000) {
                    v -= 0x10000;
                    if (v > 0xFFFFF)
                        goto report_bad_uri;
                    c = (jschar)((v & 0x3FF) + 0xDC00);
                    jschar H = (jschar)((v >> 10) + 0xD800);
                    if (!sb.append(H))
                        return JS_FALSE;
                } else {
                    c = (jschar)v;
                }
            }
            if (js_strchr(reservedSet, c)) {
                if (!sb.append(chars + start, k - start + 1))
                    return JS_FALSE;
            } else {
                if (!sb.append(c))
                    return JS_FALSE;
            }
        } else {
            if (!sb.append(c))
                return JS_FALSE;
        }
    }

    return TransferBufferToString(cx, sb, rval);

  report_bad_uri:
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_URI);
    

    return JS_FALSE;
}

static JSBool
str_decodeURI(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSLinearString *str = ArgToRootedString(cx, args, 0);
    if (!str)
        return false;

    Value result;
    if (!Decode(cx, str, js_uriReservedPlusPound_ucstr, &result))
        return false;

    args.rval().set(result);
    return true;
}

static JSBool
str_decodeURI_Component(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSLinearString *str = ArgToRootedString(cx, args, 0);
    if (!str)
        return false;

    Value result;
    if (!Decode(cx, str, js_empty_ucstr, &result))
        return false;

    args.rval().set(result);
    return true;
}

static JSBool
str_encodeURI(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSLinearString *str = ArgToRootedString(cx, args, 0);
    if (!str)
        return false;

    Value result;
    if (!Encode(cx, str, js_uriReservedPlusPound_ucstr, js_uriUnescaped_ucstr, &result))
        return false;

    args.rval().set(result);
    return true;
}

static JSBool
str_encodeURI_Component(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSLinearString *str = ArgToRootedString(cx, args, 0);
    if (!str)
        return false;

    Value result;
    if (!Encode(cx, str, js_uriUnescaped_ucstr, NULL, &result))
        return false;

    args.rval().set(result);
    return true;
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






static uint32_t
Utf8ToOneUcs4Char(const uint8_t *utf8Buffer, int utf8Length)
{
    JS_ASSERT(1 <= utf8Length && utf8Length <= 4);

    if (utf8Length == 1) {
        JS_ASSERT(!(*utf8Buffer & 0x80));
        return *utf8Buffer;
    }

    
    static const uint32_t minucs4Table[] = { 0x80, 0x800, 0x10000 };

    JS_ASSERT((*utf8Buffer & (0x100 - (1 << (7 - utf8Length)))) ==
              (0x100 - (1 << (8 - utf8Length))));
    uint32_t ucs4Char = *utf8Buffer++ & ((1 << (7 - utf8Length)) - 1);
    uint32_t minucs4Char = minucs4Table[utf8Length - 2];
    while (--utf8Length) {
        JS_ASSERT((*utf8Buffer & 0xC0) == 0x80);
        ucs4Char = (ucs4Char << 6) | (*utf8Buffer++ & 0x3F);
    }

    if (JS_UNLIKELY(ucs4Char < minucs4Char || (ucs4Char >= 0xD800 && ucs4Char <= 0xDFFF)))
        return INVALID_UTF8;

    return ucs4Char;
}

namespace js {

size_t
PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp, JSLinearString *str, uint32_t quote)
{
    enum {
        STOP, FIRST_QUOTE, LAST_QUOTE, CHARS, ESCAPE_START, ESCAPE_MORE
    } state;

    JS_ASSERT(quote == 0 || quote == '\'' || quote == '"');
    JS_ASSERT_IF(!buffer, bufferSize == 0);
    JS_ASSERT_IF(fp, !buffer);

    if (bufferSize == 0)
        buffer = NULL;
    else
        bufferSize--;

    const jschar *chars = str->chars();
    const jschar *charsEnd = chars + str->length();
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
                buffer = NULL;
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

} 
