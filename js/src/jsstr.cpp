

















































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsprobes.h"
#include "jsregexp.h"
#include "jsscope.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "jsbit.h"
#include "jsvector.h"
#include "jsversion.h"

#include "jsinferinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsregexpinlines.h"
#include "jsautooplen.h"        

#include "vm/GlobalObject.h"
#include "vm/StringObject-inl.h"
#include "vm/String-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;
using namespace js::unicode;

#ifdef JS_TRACER

JSBool JS_FASTCALL
js_FlattenOnTrace(JSContext *cx, JSString* str)
{
    return !!str->ensureLinear(cx);
}
JS_DEFINE_CALLINFO_2(extern, BOOL, js_FlattenOnTrace, CONTEXT, STRING, 0, nanojit::ACCSET_STORE_ANY)

#endif 

static JSLinearString *
ArgToRootedString(JSContext *cx, uintN argc, Value *vp, uintN arg)
{
    if (arg >= argc)
        return cx->runtime->atomState.typeAtoms[JSTYPE_VOID];
    vp += 2 + arg;

    if (!ToPrimitive(cx, JSTYPE_STRING, vp))
        return NULL;

    JSLinearString *str;
    if (vp->isString()) {
        str = vp->toString()->ensureLinear(cx);
    } else if (vp->isBoolean()) {
        str = cx->runtime->atomState.booleanAtoms[(int)vp->toBoolean()];
    } else if (vp->isNull()) {
        str = cx->runtime->atomState.nullAtom;
    } else if (vp->isUndefined()) {
        str = cx->runtime->atomState.typeAtoms[JSTYPE_VOID];
    }
    else {
        str = NumberToString(cx, vp->toNumber());
        if (!str)
            return NULL;
        vp->setString(str);
    }
    return str;
}




static JSBool
str_decodeURI(JSContext *cx, uintN argc, Value *vp);

static JSBool
str_decodeURI_Component(JSContext *cx, uintN argc, Value *vp);

static JSBool
str_encodeURI(JSContext *cx, uintN argc, Value *vp);

static JSBool
str_encodeURI_Component(JSContext *cx, uintN argc, Value *vp);

static const uint32 INVALID_UTF8 = UINT32_MAX;

static uint32
Utf8ToOneUcs4Char(const uint8 *utf8Buffer, int utf8Length);







static JSBool
str_escape(JSContext *cx, uintN argc, Value *vp)
{
    const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    JSLinearString *str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;

    size_t length = str->length();
    const jschar *chars = str->chars();

    static const uint8 shouldPassThrough[256] = {
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
    int count = 0;
    for (uint i = 0; i < sizeof(shouldPassThrough); i++) {
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
            return JS_FALSE;
        }
    }

    if (newlength >= ~(size_t)0 / sizeof(jschar)) {
        js_ReportAllocationOverflow(cx);
        return JS_FALSE;
    }

    jschar *newchars = (jschar *) cx->malloc_((newlength + 1) * sizeof(jschar));
    if (!newchars)
        return JS_FALSE;
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
        return JS_FALSE;
    }
    vp->setString(retstr);
    return JS_TRUE;
}


static JSBool
str_unescape(JSContext *cx, uintN argc, Value *vp)
{
    JSLinearString *str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return false;

    size_t length = str->length();
    const jschar *chars = str->chars();

    
    jschar *newchars = (jschar *) cx->malloc_((length + 1) * sizeof(jschar));
    if (!newchars)
        return false;
    size_t ni = 0, i = 0;
    while (i < length) {
        jschar ch = chars[i++];
        if (ch == '%') {
            
            if (i + 1 < length &&
                JS7_ISHEX(chars[i]) && JS7_ISHEX(chars[i + 1]))
            {
                ch = JS7_UNHEX(chars[i]) * 16 + JS7_UNHEX(chars[i + 1]);
                i += 2;
            } else if (i + 4 < length && chars[i] == 'u' &&
                       JS7_ISHEX(chars[i + 1]) && JS7_ISHEX(chars[i + 2]) &&
                       JS7_ISHEX(chars[i + 3]) && JS7_ISHEX(chars[i + 4]))
            {
                ch = (((((JS7_UNHEX(chars[i + 1]) << 4)
                        + JS7_UNHEX(chars[i + 2])) << 4)
                      + JS7_UNHEX(chars[i + 3])) << 4)
                    + JS7_UNHEX(chars[i + 4]);
                i += 5;
            }
        }
        newchars[ni++] = ch;
    }
    newchars[ni] = 0;

    JSString *retstr = js_NewString(cx, newchars, ni);
    if (!retstr) {
        cx->free_(newchars);
        return JS_FALSE;
    }
    vp->setString(retstr);
    return JS_TRUE;
}

#if JS_HAS_UNEVAL
static JSBool
str_uneval(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    str = js_ValueToSource(cx, argc != 0 ? vp[2] : UndefinedValue());
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;
}
#endif

const char js_escape_str[] = "escape";
const char js_unescape_str[] = "unescape";
#if JS_HAS_UNEVAL
const char js_uneval_str[] = "uneval";
#endif
const char js_decodeURI_str[] = "decodeURI";
const char js_encodeURI_str[] = "encodeURI";
const char js_decodeURIComponent_str[] = "decodeURIComponent";
const char js_encodeURIComponent_str[] = "encodeURIComponent";

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

static const uintN STRING_ELEMENT_ATTRS = JSPROP_ENUMERATE | JSPROP_READONLY | JSPROP_PERMANENT;

static JSBool
str_enumerate(JSContext *cx, JSObject *obj)
{
    JSString *str = obj->getPrimitiveThis().toString();
    for (size_t i = 0, length = str->length(); i < length; i++) {
        JSString *str1 = js_NewDependentString(cx, str, i, 1);
        if (!str1)
            return false;
        if (!obj->defineElement(cx, i, StringValue(str1),
                                JS_PropertyStub, JS_StrictPropertyStub,
                                STRING_ELEMENT_ATTRS)) {
            return false;
        }
    }

    return true;
}

static JSBool
str_resolve(JSContext *cx, JSObject *obj, jsid id, uintN flags,
            JSObject **objp)
{
    if (!JSID_IS_INT(id))
        return JS_TRUE;

    JSString *str = obj->getPrimitiveThis().toString();

    jsint slot = JSID_TO_INT(id);
    if ((size_t)slot < str->length()) {
        JSString *str1 = cx->runtime->staticStrings.getUnitStringForElement(cx, str, size_t(slot));
        if (!str1)
            return JS_FALSE;
        if (!obj->defineElement(cx, uint32(slot), StringValue(str1), NULL, NULL,
                                STRING_ELEMENT_ATTRS)) {
            return JS_FALSE;
        }
        *objp = obj;
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
ThisToStringForStringProto(JSContext *cx, Value *vp)
{
    JS_CHECK_RECURSION(cx, return NULL);

    if (vp[1].isString())
        return vp[1].toString();

    if (vp[1].isObject()) {
        JSObject *obj = &vp[1].toObject();
        if (obj->isString() &&
            ClassMethodIsNative(cx, obj,
                                &StringClass,
                                ATOM_TO_JSID(cx->runtime->atomState.toStringAtom),
                                js_str_toString))
        {
            vp[1] = obj->getPrimitiveThis();
            return vp[1].toString();
        }
    } else if (vp[1].isNullOrUndefined()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_CONVERT_TO,
                             vp[1].isNull() ? "null" : "undefined", "object");
        return NULL;
    }

    JSString *str = js_ValueToString(cx, vp[1]);
    if (!str)
        return NULL;
    vp[1].setString(str);
    return str;
}

#if JS_HAS_TOSOURCE





static JSBool
str_quote(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;
    str = js_QuoteString(cx, str, '"');
    if (!str)
        return false;
    vp->setString(str);
    return true;
}

static JSBool
str_toSource(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    if (!GetPrimitiveThis(cx, vp, &str))
        return false;

    str = js_QuoteString(cx, str, '"');
    if (!str)
        return false;

    char buf[16];
    size_t j = JS_snprintf(buf, sizeof buf, "(new String(");

    JS::Anchor<JSString *> anchor(str);
    size_t k = str->length();
    const jschar *s = str->getChars(cx);
    if (!s)
        return false;

    size_t n = j + k + 2;
    jschar *t = (jschar *) cx->malloc_((n + 1) * sizeof(jschar));
    if (!t)
        return false;

    size_t i;
    for (i = 0; i < j; i++)
        t[i] = buf[i];
    for (j = 0; j < k; i++, j++)
        t[i] = s[j];
    t[i++] = ')';
    t[i++] = ')';
    t[i] = 0;

    str = js_NewString(cx, t, n);
    if (!str) {
        cx->free_(t);
        return false;
    }
    vp->setString(str);
    return true;
}

#endif 

JSBool
js_str_toString(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    if (!GetPrimitiveThis(cx, vp, &str))
        return false;
    vp->setString(str);
    return true;
}




 
JS_ALWAYS_INLINE bool
ValueToIntegerRange(JSContext *cx, const Value &v, int32 *out)
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
            *out = int32(d);
    }

    return true;
}

static JSBool
str_substring(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    int32 length, begin, end;
    if (argc > 0) {
        end = length = int32(str->length());

        if (!ValueToIntegerRange(cx, vp[2], &begin))
            return false;

        if (begin < 0)
            begin = 0;
        else if (begin > length)
            begin = length;

        if (argc > 1 && !vp[3].isUndefined()) {
            if (!ValueToIntegerRange(cx, vp[3], &end))
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

    vp->setString(str);
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

static JSBool
str_toLowerCase(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;
    str = js_toLowerCase(cx, str);
    if (!str)
        return false;
    vp->setString(str);
    return true;
}

static JSBool
str_toLocaleLowerCase(JSContext *cx, uintN argc, Value *vp)
{
    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToLowerCase) {
        JSString *str = ThisToStringForStringProto(cx, vp);
        if (!str)
            return false;
        return cx->localeCallbacks->localeToLowerCase(cx, str, vp);
    }

    return str_toLowerCase(cx, 0, vp);
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
str_toUpperCase(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;
    str = js_toUpperCase(cx, str);
    if (!str)
        return false;
    vp->setString(str);
    return true;
}

static JSBool
str_toLocaleUpperCase(JSContext *cx, uintN argc, Value *vp)
{
    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToUpperCase) {
        JSString *str = ThisToStringForStringProto(cx, vp);
        if (!str)
            return false;
        return cx->localeCallbacks->localeToUpperCase(cx, str, vp);
    }

    return str_toUpperCase(cx, 0, vp);
}

static JSBool
str_localeCompare(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    if (argc == 0) {
        vp->setInt32(0);
    } else {
        JSString *thatStr = js_ValueToString(cx, vp[2]);
        if (!thatStr)
            return false;
        if (cx->localeCallbacks && cx->localeCallbacks->localeCompare) {
            vp[2].setString(thatStr);
            return cx->localeCallbacks->localeCompare(cx, str, thatStr, vp);
        }
        int32 result;
        if (!CompareStrings(cx, str, thatStr, &result))
            return false;
        vp->setInt32(result);
    }
    return true;
}

JSBool
js_str_charAt(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    jsint i;
    if (vp[1].isString() && argc != 0 && vp[2].isInt32()) {
        str = vp[1].toString();
        i = vp[2].toInt32();
        if ((size_t)i >= str->length())
            goto out_of_range;
    } else {
        str = ThisToStringForStringProto(cx, vp);
        if (!str)
            return false;

        double d = 0.0;
        if (argc > 0 && !ToInteger(cx, vp[2], &d))
            return false;

        if (d < 0 || str->length() <= d)
            goto out_of_range;
        i = (jsint) d;
    }

    str = cx->runtime->staticStrings.getUnitStringForElement(cx, str, size_t(i));
    if (!str)
        return false;
    vp->setString(str);
    return true;

  out_of_range:
    vp->setString(cx->runtime->emptyString);
    return true;
}

JSBool
js_str_charCodeAt(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    jsint i;
    if (vp[1].isString() && argc != 0 && vp[2].isInt32()) {
        str = vp[1].toString();
        i = vp[2].toInt32();
        if ((size_t)i >= str->length())
            goto out_of_range;
    } else {
        str = ThisToStringForStringProto(cx, vp);
        if (!str)
            return false;

        double d = 0.0;
        if (argc > 0 && !ToInteger(cx, vp[2], &d))
            return false;

        if (d < 0 || str->length() <= d)
            goto out_of_range;
        i = (jsint) d;
    }

    const jschar *chars;
    chars = str->getChars(cx);
    if (!chars)
        return false;

    vp->setInt32(chars[i]);
    return true;

out_of_range:
    vp->setDouble(js_NaN);
    return true;
}







static const jsuint sBMHCharSetSize = 256; 
static const jsuint sBMHPatLenMax   = 255; 
static const jsint  sBMHBadPattern  = -2;  

jsint
js_BoyerMooreHorspool(const jschar *text, jsuint textlen,
                      const jschar *pat, jsuint patlen)
{
    uint8 skip[sBMHCharSetSize];

    JS_ASSERT(0 < patlen && patlen <= sBMHPatLenMax);
    for (jsuint i = 0; i < sBMHCharSetSize; i++)
        skip[i] = (uint8)patlen;
    jsuint m = patlen - 1;
    for (jsuint i = 0; i < m; i++) {
        jschar c = pat[i];
        if (c >= sBMHCharSetSize)
            return sBMHBadPattern;
        skip[c] = (uint8)(m - i);
    }
    jschar c;
    for (jsuint k = m;
         k < textlen;
         k += ((c = text[k]) >= sBMHCharSetSize) ? patlen : skip[c]) {
        for (jsuint i = k, j = m; ; i--, j--) {
            if (text[i] != pat[j])
                break;
            if (j == 0)
                return static_cast<jsint>(i);  
        }
    }
    return -1;
}

struct MemCmp {
    typedef jsuint Extent;
    static JS_ALWAYS_INLINE Extent computeExtent(const jschar *, jsuint patlen) {
        return (patlen - 1) * sizeof(jschar);
    }
    static JS_ALWAYS_INLINE bool match(const jschar *p, const jschar *t, Extent extent) {
        return memcmp(p, t, extent) == 0;
    }
};

struct ManualCmp {
    typedef const jschar *Extent;
    static JS_ALWAYS_INLINE Extent computeExtent(const jschar *pat, jsuint patlen) {
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
static jsint
UnrolledMatch(const jschar *text, jsuint textlen, const jschar *pat, jsuint patlen)
{
    JS_ASSERT(patlen > 0 && textlen > 0);
    const jschar *textend = text + textlen - (patlen - 1);
    const jschar p0 = *pat;
    const jschar *const patNext = pat + 1;
    const typename InnerMatch::Extent extent = InnerMatch::computeExtent(pat, patlen);
    uint8 fixup;

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

static JS_ALWAYS_INLINE jsint
StringMatch(const jschar *text, jsuint textlen,
            const jschar *pat, jsuint patlen)
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
        jsint index = js_BoyerMooreHorspool(text, textlen, pat, patlen);
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
RopeMatch(JSContext *cx, JSString *textstr, const jschar *pat, jsuint patlen, jsint *match)
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

    
    jsint pos = 0;

    

    for (JSLinearString **outerp = strs.begin(); outerp != strs.end(); ++outerp) {
        
        JSLinearString *outer = *outerp;
        const jschar *chars = outer->chars();
        size_t len = outer->length();
        jsint matchResult = StringMatch(chars, len, pat, patlen);
        if (matchResult != -1) {
            *match = pos + matchResult;
            return true;
        }

        
        JSLinearString **innerp = outerp;

        



        const jschar *const text = chars + (patlen > len ? 0 : len - patlen + 1);
        const jschar *const textend = chars + len;
        const jschar p0 = *pat;
        const jschar *const p1 = pat + 1;
        const jschar *const patend = pat + patlen;
        for (const jschar *t = text; t != textend; ) {
            if (*t++ != p0)
                continue;
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
str_indexOf(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    JSLinearString *patstr = ArgToRootedString(cx, argc, vp, 0);
    if (!patstr)
        return false;

    jsuint textlen = str->length();
    const jschar *text = str->getChars(cx);
    if (!text)
        return false;

    jsuint patlen = patstr->length();
    const jschar *pat = patstr->chars();

    jsuint start;
    if (argc > 1) {
        if (vp[3].isInt32()) {
            jsint i = vp[3].toInt32();
            if (i <= 0) {
                start = 0;
            } else if (jsuint(i) > textlen) {
                start = textlen;
                textlen = 0;
            } else {
                start = i;
                text += start;
                textlen -= start;
            }
        } else {
            jsdouble d;
            if (!ToInteger(cx, vp[3], &d))
                return false;
            if (d <= 0) {
                start = 0;
            } else if (d > textlen) {
                start = textlen;
                textlen = 0;
            } else {
                start = (jsint)d;
                text += start;
                textlen -= start;
            }
        }
    } else {
        start = 0;
    }

    jsint match = StringMatch(text, textlen, pat, patlen);
    vp->setInt32((match == -1) ? -1 : start + match);
    return true;
}

static JSBool
str_lastIndexOf(JSContext *cx, uintN argc, Value *vp)
{
    JSString *textstr = ThisToStringForStringProto(cx, vp);
    if (!textstr)
        return false;
    size_t textlen = textstr->length();
    const jschar *text = textstr->getChars(cx);
    if (!text)
        return false;

    JSLinearString *patstr = ArgToRootedString(cx, argc, vp, 0);
    if (!patstr)
        return false;

    size_t patlen = patstr->length();
    const jschar *pat = patstr->chars();

    jsint i = textlen - patlen; 
    if (i < 0) {
        vp->setInt32(-1);
        return true;
    }

    if (argc > 1) {
        if (vp[3].isInt32()) {
            jsint j = vp[3].toInt32();
            if (j <= 0)
                i = 0;
            else if (j < i)
                i = j;
        } else {
            double d;
            if (!ToNumber(cx, vp[3], &d))
                return false;
            if (!JSDOUBLE_IS_NaN(d)) {
                d = js_DoubleToInteger(d);
                if (d <= 0)
                    i = 0;
                else if (d < i)
                    i = (jsint)d;
            }
        }
    }

    if (patlen == 0) {
        vp->setInt32(i);
        return true;
    }

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
            vp->setInt32(t - text);
            return true;
        }
      break_continue:;
    }

    vp->setInt32(-1);
    return true;
}

static JSBool
js_TrimString(JSContext *cx, Value *vp, JSBool trimLeft, JSBool trimRight)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
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

    vp->setString(str);
    return true;
}

static JSBool
str_trim(JSContext *cx, uintN argc, Value *vp)
{
    return js_TrimString(cx, vp, JS_TRUE, JS_TRUE);
}

static JSBool
str_trimLeft(JSContext *cx, uintN argc, Value *vp)
{
    return js_TrimString(cx, vp, JS_TRUE, JS_FALSE);
}

static JSBool
str_trimRight(JSContext *cx, uintN argc, Value *vp)
{
    return js_TrimString(cx, vp, JS_FALSE, JS_TRUE);
}






class FlatMatch
{
    JSLinearString  *patstr;
    const jschar    *pat;
    size_t          patlen;
    int32           match_;

    friend class RegExpGuard;

  public:
    FlatMatch() : patstr(NULL) {} 
    JSString *pattern() const { return patstr; }
    size_t patternLength() const { return patlen; }

    



    int32 match() const { return match_; }
};


class RegExpPair
{
    AutoRefCount<RegExp>    re_;
    JSObject                *reobj_;

    explicit RegExpPair(RegExpPair &);

  public:
    explicit RegExpPair(JSContext *cx) : re_(cx) {}

    void reset(JSObject &obj) {
        reobj_ = &obj;
        RegExp *re = RegExp::extractFrom(reobj_);
        JS_ASSERT(re);
        re_.reset(NeedsIncRef<RegExp>(re));
    }

    void reset(AlreadyIncRefed<RegExp> re) {
        reobj_ = NULL;
        re_.reset(re);
    }

    
    JSObject *reobj() const { return reobj_; }
    bool hasRegExp() const { return !re_.null(); }
    RegExp &re() const { JS_ASSERT(hasRegExp()); return *re_; }
};








class RegExpGuard
{
    RegExpGuard(const RegExpGuard &);
    void operator=(const RegExpGuard &);

    JSContext   *cx;
    RegExpPair  rep;
    FlatMatch   fm;

    



    static const size_t MAX_FLAT_PAT_LEN = 256;

    static JSString *flattenPattern(JSContext *cx, JSLinearString *patstr) {
        StringBuffer sb(cx);
        if (!sb.reserve(patstr->length()))
            return NULL;

        static const jschar ESCAPE_CHAR = '\\';
        const jschar *chars = patstr->chars();
        size_t len = patstr->length();
        for (const jschar *it = chars; it != chars + len; ++it) {
            if (RegExp::isMetaChar(*it)) {
                if (!sb.append(ESCAPE_CHAR) || !sb.append(*it))
                    return NULL;
            } else {
                if (!sb.append(*it))
                    return NULL;
            }
        }
        return sb.finishString();
    }

  public:
    explicit RegExpGuard(JSContext *cx) : cx(cx), rep(cx) {}
    ~RegExpGuard() {}

    
    bool
    init(uintN argc, Value *vp, bool convertVoid = false)
    {
        if (argc != 0 && VALUE_IS_REGEXP(cx, vp[2])) {
            rep.reset(vp[2].toObject());
        } else {
            if (convertVoid && (argc == 0 || vp[2].isUndefined())) {
                fm.patstr = cx->runtime->emptyString;
                return true;
            }
            
            fm.patstr = ArgToRootedString(cx, argc, vp, 0);
            if (!fm.patstr)
                return false;
        }
        return true;
    }

    








    const FlatMatch *
    tryFlatMatch(JSContext *cx, JSString *textstr, uintN optarg, uintN argc,
                 bool checkMetaChars = true)
    {
        if (rep.hasRegExp())
            return NULL;

        fm.pat = fm.patstr->chars();
        fm.patlen = fm.patstr->length();

        if (optarg < argc)
            return NULL;

        if (checkMetaChars &&
            (fm.patlen > MAX_FLAT_PAT_LEN || RegExp::hasMetaChars(fm.pat, fm.patlen))) {
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

    
    const RegExpPair *
    normalizeRegExp(bool flat, uintN optarg, uintN argc, Value *vp)
    {
        if (rep.hasRegExp())
            return &rep;

        
        JSString *opt;
        if (optarg < argc) {
            opt = js_ValueToString(cx, vp[2 + optarg]);
            if (!opt)
                return NULL;
        } else {
            opt = NULL;
        }

        JSString *patstr;
        if (flat) {
            patstr = flattenPattern(cx, fm.patstr);
            if (!patstr)
                return NULL;
        } else {
            patstr = fm.patstr;
        }
        JS_ASSERT(patstr);

        AlreadyIncRefed<RegExp> re = RegExp::createFlagged(cx, patstr, opt, NULL);
        if (!re)
            return NULL;
        rep.reset(re);
        return &rep;
    }

#if DEBUG
    bool hasRegExpPair() const { return rep.hasRegExp(); }
#endif
};


static JS_ALWAYS_INLINE bool
Matched(bool test, const Value &v)
{
    return test ? v.isTrue() : !v.isNull();
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
DoMatch(JSContext *cx, RegExpStatics *res, JSString *str, const RegExpPair &rep,
        DoMatchCallback callback, void *data, MatchControlFlags flags, Value *rval)
{
    RegExp &re = rep.re();
    if (re.global()) {
        
        bool testGlobal = flags & TEST_GLOBAL_BIT;
        if (rep.reobj())
            rep.reobj()->zeroRegExpLastIndex();
        for (size_t count = 0, i = 0, length = str->length(); i <= length; ++count) {
            if (!re.execute(cx, res, str, &i, testGlobal, rval))
                return false;
            if (!Matched(testGlobal, *rval))
                break;
            if (!callback(cx, res, count, data))
                return false;
            if (!res->matched())
                ++i;
        }
    } else {
        
        bool testSingle = !!(flags & TEST_SINGLE_BIT),
             callbackOnSingle = !!(flags & CALLBACK_ON_SINGLE_BIT);
        size_t i = 0;
        if (!re.execute(cx, res, str, &i, testSingle, rval))
            return false;
        if (callbackOnSingle && Matched(testSingle, *rval) && !callback(cx, res, 0, data))
            return false;
    }
    return true;
}

static bool
BuildFlatMatchArray(JSContext *cx, JSString *textstr, const FlatMatch &fm, Value *vp)
{
    if (fm.match() < 0) {
        vp->setNull();
        return true;
    }

    
    JSObject *obj = NewSlowEmptyArray(cx);
    if (!obj)
        return false;
    vp->setObject(*obj);

    return obj->defineElement(cx, 0, StringValue(fm.pattern())) &&
           obj->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.indexAtom),
                               Int32Value(fm.match())) &&
           obj->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.inputAtom),
                               StringValue(textstr));
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

    Value v;
    return res->createLastMatch(cx, &v) && arrayobj->defineElement(cx, count, v);
}

static JSBool
str_match(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;
    
    RegExpGuard g(cx);
    if (!g.init(argc, vp, true))
        return false;
    if (const FlatMatch *fm = g.tryFlatMatch(cx, str, 1, argc))
        return BuildFlatMatchArray(cx, str, *fm, vp);
    if (cx->isExceptionPending())  
        return false;

    const RegExpPair *rep = g.normalizeRegExp(false, 1, argc, vp);
    if (!rep)
        return false;

    AutoObjectRooter array(cx);
    MatchArgType arg = array.addr();
    RegExpStatics *res = cx->regExpStatics();
    Value rval;
    if (!DoMatch(cx, res, str, *rep, MatchCallback, arg, MATCH_ARGS, &rval))
        return false;

    if (rep->re().global())
        vp->setObjectOrNull(array.object());
    else
        *vp = rval;
    return true;
}

static JSBool
str_search(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    RegExpGuard g(cx);
    if (!g.init(argc, vp, true))
        return false;
    if (const FlatMatch *fm = g.tryFlatMatch(cx, str, 1, argc)) {
        vp->setInt32(fm->match());
        return true;
    }
    if (cx->isExceptionPending())  
        return false;
    
    const RegExpPair *rep = g.normalizeRegExp(false, 1, argc, vp);
    if (!rep)
        return false;

    RegExpStatics *res = cx->regExpStatics();
    size_t i = 0;
    if (!rep->re().execute(cx, res, str, &i, true, vp))
        return false;

    if (vp->isTrue())
        vp->setInt32(res->matchStart());
    else
        vp->setInt32(-1);
    return true;
}

struct ReplaceData
{
    ReplaceData(JSContext *cx)
     : g(cx), sb(cx)
    {}

    JSString           *str;           
    RegExpGuard        g;              
    JSObject           *lambda;        
    JSObject           *elembase;      
    JSLinearString     *repstr;        
    const jschar       *dollar;        
    const jschar       *dollarEnd;     
    jsint              leftIndex;      
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
        
        uintN num = JS7_UNDEC(dc);
        if (num > res->parenCount())
            return false;

        const jschar *cp = dp + 2;
        if (cp < ep && (dc = *cp, JS7_ISDEC(dc))) {
            uintN tmp = 10 * num + JS7_UNDEC(dc);
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
    JSObject *base = rdata.elembase;
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
        jsid id = ATOM_TO_JSID(atom);

        JSObject *holder;
        JSProperty *prop = NULL;
        if (!LookupPropertyWithFlags(cx, base, id, JSRESOLVE_QUALIFIED, &holder, &prop))
            return false;

        
        if (prop && holder == base) {
            Shape *shape = (Shape *) prop;
            if (shape->hasSlot() && shape->hasDefaultGetter()) {
                Value value = base->getSlot(shape->slot());
                if (value.isString()) {
                    rdata.repstr = value.toString()->ensureLinear(cx);
                    if (!rdata.repstr)
                        return false;
                    *sizep = rdata.repstr->length();
                    return true;
                }
            }
        }

        



        rdata.elembase = NULL;
    }

    JSObject *lambda = rdata.lambda;
    if (lambda) {
        PreserveRegExpStatics staticsGuard(res);
        if (!staticsGuard.init(cx))
            return false;

        







        uintN p = res->parenCount();
        uintN argc = 1 + p + 2;

        InvokeArgsGuard &args = rdata.args;
        if (!args.pushed() && !cx->stack.pushInvokeArgs(cx, argc, &args))
            return false;

        args.calleeHasBeenReset();
        args.calleev() = ObjectValue(*lambda);
        args.thisv() = UndefinedValue();

        
        uintN argi = 0;
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

        
        JSString *repstr = ValueToString_TestForStringInline(cx, args.rval());
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
    JS_ALWAYS_TRUE(rdata.sb.append(cp, repstr->length() - (cp - bp)));
}

static bool
ReplaceRegExpCallback(JSContext *cx, RegExpStatics *res, size_t count, void *p)
{
    ReplaceData &rdata = *static_cast<ReplaceData *>(p);

    rdata.calledBack = true;
    JSLinearString &str = rdata.str->asLinear();  
    size_t leftoff = rdata.leftIndex;
    const jschar *left = str.chars() + leftoff;
    size_t leftlen = res->matchStart() - leftoff;
    rdata.leftIndex = res->matchLimit();

    size_t replen = 0;  
    if (!FindReplaceLength(cx, res, rdata, &replen))
        return false;

    size_t growth = leftlen + replen;
    if (!rdata.sb.reserve(rdata.sb.length() + growth))
        return false;
    rdata.sb.infallibleAppend(left, leftlen); 
    DoReplace(cx, res, rdata);
    return true;
}

static bool
BuildFlatReplacement(JSContext *cx, JSString *textstr, JSString *repstr,
                     const FlatMatch &fm, Value *vp)
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
            JSString *str = r.front();
            size_t len = str->length();
            size_t strEnd = pos + len;
            if (pos < matchEnd && strEnd > match) {
                



                if (match >= pos) {
                    





                    JSString *leftSide = js_NewDependentString(cx, str, 0, match - pos);
                    if (!leftSide ||
                        !builder.append(leftSide) ||
                        !builder.append(repstr)) {
                        return false;
                    }
                }

                



                if (strEnd > matchEnd) {
                    JSString *rightSide = js_NewDependentString(cx, str, matchEnd - pos,
                                                                strEnd - matchEnd);
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
        JSString *leftSide = js_NewDependentString(cx, textstr, 0, match);
        if (!leftSide)
            return false;
        JSString *rightSide = js_NewDependentString(cx, textstr, match + fm.patternLength(),
                                                    textstr->length() - match - fm.patternLength());
        if (!rightSide ||
            !builder.append(leftSide) ||
            !builder.append(repstr) ||
            !builder.append(rightSide)) {
            return false;
        }
    }

    vp->setString(builder.result());
    return true;
}







static inline bool
BuildDollarReplacement(JSContext *cx, JSString *textstrArg, JSLinearString *repstr,
                       const jschar *firstDollar, const FlatMatch &fm, Value *vp)
{
    JSLinearString *textstr = textstrArg->ensureLinear(cx);
    if (!textstr)
        return NULL;

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

    JSString *leftSide = js_NewDependentString(cx, textstr, 0, matchStart);
    ENSURE(leftSide);

    JSString *newReplace = newReplaceChars.finishString();
    ENSURE(newReplace);

    JS_ASSERT(textstr->length() >= matchLimit);
    JSString *rightSide = js_NewDependentString(cx, textstr, matchLimit,
                                                textstr->length() - matchLimit);
    ENSURE(rightSide);

    RopeBuilder builder(cx);
    ENSURE(builder.append(leftSide) &&
           builder.append(newReplace) &&
           builder.append(rightSide));
#undef ENSURE

    vp->setString(builder.result());
    return true;
}

static inline bool
str_replace_regexp(JSContext *cx, uintN argc, Value *vp, ReplaceData &rdata)
{
    const RegExpPair *rep = rdata.g.normalizeRegExp(true, 2, argc, vp);
    if (!rep)
        return false;

    rdata.leftIndex = 0;
    rdata.calledBack = false;

    RegExpStatics *res = cx->regExpStatics();
    Value tmp;
    if (!DoMatch(cx, res, rdata.str, *rep, ReplaceRegExpCallback, &rdata, REPLACE_ARGS, &tmp))
        return false;

    if (!rdata.calledBack) {
        
        vp->setString(rdata.str);
        return true;
    }

    JSSubString sub;
    res->getRightContext(&sub);
    if (!rdata.sb.append(sub.chars, sub.length))
        return false;

    JSString *retstr = rdata.sb.finishString();
    if (!retstr)
        return false;

    vp->setString(retstr);
    return true;
}

static inline bool
str_replace_flat_lambda(JSContext *cx, uintN argc, Value *vp, ReplaceData &rdata,
                        const FlatMatch &fm)
{
    JS_ASSERT(fm.match() >= 0);

    JSString *matchStr = js_NewDependentString(cx, rdata.str, fm.match(), fm.patternLength());
    if (!matchStr)
        return false;

    
    static const uint32 lambdaArgc = 3;
    if (!cx->stack.pushInvokeArgs(cx, lambdaArgc, &rdata.args))
        return false;

    CallArgs &args = rdata.args;
    args.calleev().setObject(*rdata.lambda);
    args.thisv().setUndefined();

    Value *sp = args.argv();
    sp[0].setString(matchStr);
    sp[1].setInt32(fm.match());
    sp[2].setString(rdata.str);

    if (!Invoke(cx, rdata.args))
        return false;

    JSString *repstr = js_ValueToString(cx, args.rval());
    if (!repstr)
        return false;

    JSString *leftSide = js_NewDependentString(cx, rdata.str, 0, fm.match());
    if (!leftSide)
        return false;

    size_t matchLimit = fm.match() + fm.patternLength();
    JSString *rightSide = js_NewDependentString(cx, rdata.str, matchLimit,
                                                rdata.str->length() - matchLimit);
    if (!rightSide)
        return false;

    RopeBuilder builder(cx);
    if (!(builder.append(leftSide) &&
          builder.append(repstr) &&
          builder.append(rightSide))) {
        return false;
    }

    vp->setString(builder.result());
    return true;
}

JSBool
js::str_replace(JSContext *cx, uintN argc, Value *vp)
{
    ReplaceData rdata(cx);
    rdata.str = ThisToStringForStringProto(cx, vp);
    if (!rdata.str)
        return false;
    static const uint32 optarg = 2;

    if (!rdata.g.init(argc, vp))
        return false;

    
    if (argc >= optarg && js_IsCallable(vp[3])) {
        rdata.lambda = &vp[3].toObject();
        rdata.elembase = NULL;
        rdata.repstr = NULL;
        rdata.dollar = rdata.dollarEnd = NULL;

        if (rdata.lambda->isFunction()) {
            JSFunction *fun = rdata.lambda->getFunctionPrivate();
            if (fun->isInterpreted()) {
                







                JSScript *script = fun->script();
                jsbytecode *pc = script->code;

                Value table = UndefinedValue();
                if (JSOp(*pc) == JSOP_GETFCSLOT) {
                    table = rdata.lambda->getFlatClosureUpvar(GET_UINT16(pc));
                    pc += JSOP_GETFCSLOT_LENGTH;
                }

                if (table.isObject() &&
                    JSOp(*pc) == JSOP_GETARG && GET_SLOTNO(pc) == 0 &&
                    JSOp(*(pc + JSOP_GETARG_LENGTH)) == JSOP_GETELEM &&
                    JSOp(*(pc + JSOP_GETARG_LENGTH + JSOP_GETELEM_LENGTH)) == JSOP_RETURN) {
                    Class *clasp = table.toObject().getClass();
                    if (clasp->isNative() &&
                        !clasp->ops.lookupProperty &&
                        !clasp->ops.getProperty) {
                        rdata.elembase = &table.toObject();
                    }
                }
            }
        }
    } else {
        rdata.lambda = NULL;
        rdata.elembase = NULL;
        rdata.repstr = ArgToRootedString(cx, argc, vp, 1);
        if (!rdata.repstr)
            return false;

        
        JSFixedString *fixed = rdata.repstr->ensureFixed(cx);
        if (!fixed)
            return false;
        rdata.dollarEnd = fixed->chars() + fixed->length();
        rdata.dollar = js_strchr_limit(fixed->chars(), '$', rdata.dollarEnd);
    }

    









    const FlatMatch *fm = rdata.g.tryFlatMatch(cx, rdata.str, optarg, argc, false);
    if (!fm) {
        if (cx->isExceptionPending())  
            return false;
        JS_ASSERT_IF(!rdata.g.hasRegExpPair(), argc > optarg);
        return str_replace_regexp(cx, argc, vp, rdata);
    }

    if (fm->match() < 0) {
        vp->setString(rdata.str);
        return true;
    }

    if (rdata.lambda)
        return str_replace_flat_lambda(cx, argc, vp, rdata, *fm);

    



    if (rdata.dollar)
        return BuildDollarReplacement(cx, rdata.str, rdata.repstr, rdata.dollar, *fm, vp);

    return BuildFlatReplacement(cx, rdata.str, rdata.repstr, *fm, vp);
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
SplitHelper(JSContext *cx, JSLinearString *str, uint32 limit, Matcher splitMatch, TypeObject *type)
{
    size_t strLength = str->length();
    SplitMatchResult result;

    
    if (strLength == 0) {
        if (!splitMatch(cx, str, 0, &result))
            return NULL;

        









        if (!result.isFailure())
            return NewDenseEmptyArray(cx);

        Value v = StringValue(str);
        return NewDenseCopiedArray(cx, 1, &v);
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








class SplitRegExpMatcher {
    RegExpStatics *res;
    RegExp *re;

  public:
    static const bool returnsCaptures = true;
    SplitRegExpMatcher(RegExp *re, RegExpStatics *res) : res(res), re(re) {
    }

    inline bool operator()(JSContext *cx, JSLinearString *str, size_t index,
                           SplitMatchResult *result) {
        Value rval
#ifdef __GNUC__ 
            = UndefinedValue()
#endif
        ;
        if (!re->execute(cx, res, str, &index, true, &rval))
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

class SplitStringMatcher {
    const jschar *sepChars;
    size_t sepLength;

  public:
    static const bool returnsCaptures = false;
    SplitStringMatcher(JSLinearString *sep) {
        sepChars = sep->chars();
        sepLength = sep->length();
    }

    inline bool operator()(JSContext *cx, JSLinearString *str, size_t index,
                           SplitMatchResult *res) {
        JS_ASSERT(index == 0 || index < str->length());
        const jschar *chars = str->chars();
        jsint match = StringMatch(chars + index, str->length() - index, sepChars, sepLength);
        if (match == -1)
            res->setFailure();
        else
            res->setResult(sepLength, index + match + sepLength);
        return true;
    }
};


static JSBool
str_split(JSContext *cx, uintN argc, Value *vp)
{
    
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    TypeObject *type = GetTypeCallerInitObject(cx, JSProto_Array);
    if (!type)
        return false;
    AddTypeProperty(cx, type, NULL, Type::StringType());

    
    uint32 limit;
    if (argc > 1 && !vp[3].isUndefined()) {
        jsdouble d;
        if (!ToNumber(cx, vp[3], &d))
            return false;
        limit = js_DoubleToECMAUint32(d);
    } else {
        limit = UINT32_MAX;
    }

    
    RegExp *re = NULL;
    JSLinearString *sepstr = NULL;
    bool sepUndefined = (argc == 0 || vp[2].isUndefined());
    if (!sepUndefined) {
        if (VALUE_IS_REGEXP(cx, vp[2])) {
            re = static_cast<RegExp *>(vp[2].toObject().getPrivate());
        } else {
            JSString *sep = js_ValueToString(cx, vp[2]);
            if (!sep)
                return false;
            vp[2].setString(sep);

            sepstr = sep->ensureLinear(cx);
            if (!sepstr)
                return false;
        }
    }

    
    if (limit == 0) {
        JSObject *aobj = NewDenseEmptyArray(cx);
        if (!aobj)
            return false;
        aobj->setType(type);
        vp->setObject(*aobj);
        return true;
    }

    
    if (sepUndefined) {
        Value v = StringValue(str);
        JSObject *aobj = NewDenseCopiedArray(cx, 1, &v);
        if (!aobj)
            return false;
        aobj->setType(type);
        vp->setObject(*aobj);
        return true;
    }
    JSLinearString *strlin = str->ensureLinear(cx);
    if (!strlin)
        return false;

    
    JSObject *aobj;
    if (re) {
        aobj = SplitHelper(cx, strlin, limit, SplitRegExpMatcher(re, cx->regExpStatics()), type);
    } else {
        
        aobj = SplitHelper(cx, strlin, limit, SplitStringMatcher(sepstr), type);
    }
    if (!aobj)
        return false;

    
    aobj->setType(type);
    vp->setObject(*aobj);
    return true;
}

#if JS_HAS_PERL_SUBSTR
static JSBool
str_substr(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    int32 length, len, begin;
    if (argc > 0) {
        length = int32(str->length());
        if (!ValueToIntegerRange(cx, vp[2], &begin))
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

        if (argc == 1 || vp[3].isUndefined()) {
            len = length - begin;
        } else {
            if (!ValueToIntegerRange(cx, vp[3], &len))  
                return false;

            if (len <= 0) {
                str = cx->runtime->emptyString;
                goto out;
            }

            if (uint32(length) < uint32(begin + len))
                len = length - begin;
        }

        str = js_NewDependentString(cx, str, size_t(begin), size_t(len));
        if (!str)
            return false;
    }

out:
    vp->setString(str);
    return true;
}
#endif 




static JSBool
str_concat(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    Value *argv = JS_ARGV(cx, vp);
    for (uintN i = 0; i < argc; i++) {
        JSString *str2 = js_ValueToString(cx, argv[i]);
        if (!str2)
            return false;

        str = js_ConcatStrings(cx, str, str2);
        if (!str)
            return false;
    }

    JS_SET_RVAL(cx, vp, StringValue(str));
    return true;
}

static JSBool
str_slice(JSContext *cx, uintN argc, Value *vp)
{
    if (argc == 1 && vp[1].isString() && vp[2].isInt32()) {
        size_t begin, end, length;

        JSString *str = vp[1].toString();
        begin = vp[2].toInt32();
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
                    return JS_FALSE;
            }
            vp->setString(str);
            return JS_TRUE;
        }
    }

    JSString *str = ThisToStringForStringProto(cx, vp);
    if (!str)
        return false;

    if (argc != 0) {
        double begin, end, length;

        if (!ToInteger(cx, vp[2], &begin))
            return false;
        length = str->length();
        if (begin < 0) {
            begin += length;
            if (begin < 0)
                begin = 0;
        } else if (begin > length) {
            begin = length;
        }

        if (argc == 1 || vp[3].isUndefined()) {
            end = length;
        } else {
            if (!ToInteger(cx, vp[3], &end))
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
        }

        str = js_NewDependentString(cx, str,
                                    (size_t)begin,
                                    (size_t)(end - begin));
        if (!str)
            return JS_FALSE;
    }
    vp->setString(str);
    return JS_TRUE;
}

#if JS_HAS_STR_HTML_HELPERS



static bool
tagify(JSContext *cx, const char *begin, JSLinearString *param, const char *end,
       Value *vp)
{
    JSString *thisstr = ThisToStringForStringProto(cx, vp);
    if (!thisstr)
        return false;
    JSLinearString *str = thisstr->ensureLinear(cx);
    if (!str)
        return false;

    if (!end)
        end = begin;

    size_t beglen = strlen(begin);
    size_t taglen = 1 + beglen + 1;                     
    size_t parlen = 0; 
    if (param) {
        parlen = param->length();
        taglen += 2 + parlen + 1;                       
    }
    size_t endlen = strlen(end);
    taglen += str->length() + 2 + endlen + 1;           

    if (taglen >= ~(size_t)0 / sizeof(jschar)) {
        js_ReportAllocationOverflow(cx);
        return false;
    }

    jschar *tagbuf = (jschar *) cx->malloc_((taglen + 1) * sizeof(jschar));
    if (!tagbuf)
        return false;

    size_t j = 0;
    tagbuf[j++] = '<';
    for (size_t i = 0; i < beglen; i++)
        tagbuf[j++] = (jschar)begin[i];
    if (param) {
        tagbuf[j++] = '=';
        tagbuf[j++] = '"';
        js_strncpy(&tagbuf[j], param->chars(), parlen);
        j += parlen;
        tagbuf[j++] = '"';
    }
    tagbuf[j++] = '>';

    js_strncpy(&tagbuf[j], str->chars(), str->length());
    j += str->length();
    tagbuf[j++] = '<';
    tagbuf[j++] = '/';
    for (size_t i = 0; i < endlen; i++)
        tagbuf[j++] = (jschar)end[i];
    tagbuf[j++] = '>';
    JS_ASSERT(j == taglen);
    tagbuf[j] = 0;

    JSString *retstr = js_NewString(cx, tagbuf, taglen);
    if (!retstr) {
        Foreground::free_((char *)tagbuf);
        return false;
    }
    vp->setString(retstr);
    return true;
}

static JSBool
tagify_value(JSContext *cx, uintN argc, Value *vp,
             const char *begin, const char *end)
{
    JSLinearString *param = ArgToRootedString(cx, argc, vp, 0);
    if (!param)
        return JS_FALSE;
    return tagify(cx, begin, param, end, vp);
}

static JSBool
str_bold(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "b", NULL, NULL, vp);
}

static JSBool
str_italics(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "i", NULL, NULL, vp);
}

static JSBool
str_fixed(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "tt", NULL, NULL, vp);
}

static JSBool
str_fontsize(JSContext *cx, uintN argc, Value *vp)
{
    return tagify_value(cx, argc, vp, "font size", "font");
}

static JSBool
str_fontcolor(JSContext *cx, uintN argc, Value *vp)
{
    return tagify_value(cx, argc, vp, "font color", "font");
}

static JSBool
str_link(JSContext *cx, uintN argc, Value *vp)
{
    return tagify_value(cx, argc, vp, "a href", "a");
}

static JSBool
str_anchor(JSContext *cx, uintN argc, Value *vp)
{
    return tagify_value(cx, argc, vp, "a name", "a");
}

static JSBool
str_strike(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "strike", NULL, NULL, vp);
}

static JSBool
str_small(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "small", NULL, NULL, vp);
}

static JSBool
str_big(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "big", NULL, NULL, vp);
}

static JSBool
str_blink(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "blink", NULL, NULL, vp);
}

static JSBool
str_sup(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "sup", NULL, NULL, vp);
}

static JSBool
str_sub(JSContext *cx, uintN argc, Value *vp)
{
    return tagify(cx, "sub", NULL, NULL, vp);
}
#endif 

#ifdef JS_TRACER
JSString* FASTCALL
js_String_getelem(JSContext* cx, JSString* str, int32 i)
{
    if ((size_t)i >= str->length())
        return NULL;
    return cx->runtime->staticStrings.getUnitStringForElement(cx, str, size_t(i));
}
#endif

JS_DEFINE_TRCINFO_1(str_concat,
    (3, (extern, STRING_RETRY, js_ConcatStrings, CONTEXT, THIS_STRING, STRING,
         1, nanojit::ACCSET_NONE)))

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
    JS_FN("indexOf",           str_indexOf,           1,JSFUN_GENERIC_NATIVE),
    JS_FN("lastIndexOf",       str_lastIndexOf,       1,JSFUN_GENERIC_NATIVE),
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
#if JS_HAS_PERL_SUBSTR
    JS_FN("substr",            str_substr,            2,JSFUN_GENERIC_NATIVE),
#endif

    
    JS_TN("concat",            str_concat,            1,JSFUN_GENERIC_NATIVE, &str_concat_trcinfo),
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

    JS_FS_END
};

JSBool
js_String(JSContext *cx, uintN argc, Value *vp)
{
    Value *argv = vp + 2;

    JSString *str;
    if (argc > 0) {
        str = js_ValueToString(cx, argv[0]);
        if (!str)
            return false;
    } else {
        str = cx->runtime->emptyString;
    }

    if (IsConstructing(vp)) {
        StringObject *strobj = StringObject::create(cx, str);
        if (!strobj)
            return false;
        vp->setObject(*strobj);
    } else {
        vp->setString(str);
    }
    return true;
}

static JSBool
str_fromCharCode(JSContext *cx, uintN argc, Value *vp)
{
    Value *argv = JS_ARGV(cx, vp);
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);
    if (argc == 1) {
        uint16_t code;
        if (!ValueToUint16(cx, argv[0], &code))
            return JS_FALSE;
        if (StaticStrings::hasUnit(code)) {
            vp->setString(cx->runtime->staticStrings.getUnit(code));
            return JS_TRUE;
        }
        argv[0].setInt32(code);
    }
    jschar *chars = (jschar *) cx->malloc_((argc + 1) * sizeof(jschar));
    if (!chars)
        return JS_FALSE;
    for (uintN i = 0; i < argc; i++) {
        uint16_t code;
        if (!ValueToUint16(cx, argv[i], &code)) {
            cx->free_(chars);
            return JS_FALSE;
        }
        chars[i] = (jschar)code;
    }
    chars[argc] = 0;
    JSString *str = js_NewString(cx, chars, argc);
    if (!str) {
        cx->free_(chars);
        return JS_FALSE;
    }
    vp->setString(str);
    return JS_TRUE;
}

#ifdef JS_TRACER
static JSString* FASTCALL
String_fromCharCode(JSContext* cx, int32 i)
{
    JS_ASSERT(JS_ON_TRACE(cx));
    jschar c = (jschar)i;
    if (StaticStrings::hasUnit(c))
        return cx->runtime->staticStrings.getUnit(c);
    return js_NewStringCopyN(cx, &c, 1);
}
#endif

JS_DEFINE_TRCINFO_1(str_fromCharCode,
    (2, (static, STRING_RETRY, String_fromCharCode, CONTEXT, INT32, 1, nanojit::ACCSET_NONE)))

static JSFunctionSpec string_static_methods[] = {
    JS_TN("fromCharCode", str_fromCharCode, 1, 0, &str_fromCharCode_trcinfo),
    JS_FS_END
};

const Shape *
StringObject::assignInitialShape(JSContext *cx)
{
    JS_ASSERT(!cx->compartment->initialStringShape);
    JS_ASSERT(nativeEmpty());

    return addDataProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom),
                           LENGTH_SLOT, JSPROP_PERMANENT | JSPROP_READONLY);
}

JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = obj->asGlobal();

    JSObject *proto = global->createBlankPrototype(cx, &StringClass);
    if (!proto || !proto->asString()->init(cx, cx->runtime->emptyString))
        return NULL;

    
    JSFunction *ctor = global->createConstructor(cx, js_String, &StringClass,
                                                 CLASS_ATOM(cx, String), 1);
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
    if (!CheckStringLength(cx, length))
        return NULL;

    JSFixedString *s = JSFixedString::new_(cx, chars, length);
    Probes::createString(cx, s, length);
    return s;
}

static JS_ALWAYS_INLINE JSFixedString *
NewShortString(JSContext *cx, const jschar *chars, size_t length)
{
    




    JS_ASSERT(JSShortString::lengthFits(length));
    JSInlineString *str = JSInlineString::lengthFits(length)
                          ? JSInlineString::new_(cx)
                          : JSShortString::new_(cx);
    if (!str)
        return NULL;

    jschar *storage = str->init(length);
    PodCopy(storage, chars, length);
    storage[length] = 0;
    Probes::createString(cx, str, length);
    return str;
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

jschar *
StringBuffer::extractWellSized()
{
    size_t capacity = cb.capacity();
    size_t length = cb.length();

    jschar *buf = cb.extractRawBuffer();
    if (!buf)
        return NULL;

    
    JS_ASSERT(capacity >= length);
    if (length > CharBuffer::sMaxInlineStorage &&
        capacity - length > (length >> 2)) {
        size_t bytes = sizeof(jschar) * (length + 1);
        JSContext *cx = context();
        jschar *tmp = (jschar *)cx->realloc_(buf, bytes);
        if (!tmp) {
            cx->free_(buf);
            return NULL;
        }
        buf = tmp;
    }

    return buf;
}

JSFixedString *
StringBuffer::finishString()
{
    JSContext *cx = context();
    if (cb.empty())
        return cx->runtime->atomState.emptyAtom;

    size_t length = cb.length();
    if (!checkLength(length))
        return NULL;

    JS_STATIC_ASSERT(JSShortString::MAX_SHORT_LENGTH < CharBuffer::InlineLength);
    if (JSShortString::lengthFits(length))
        return NewShortString(cx, cb.begin(), length);

    if (!cb.append('\0'))
        return NULL;

    jschar *buf = extractWellSized();
    if (!buf)
        return NULL;

    JSFixedString *str = js_NewString(cx, buf, length);
    if (!str)
        cx->free_(buf);
    return str;
}

JSAtom *
StringBuffer::finishAtom()
{
    JSContext *cx = context();

    size_t length = cb.length();
    if (length == 0)
        return cx->runtime->atomState.emptyAtom;

    JSAtom *atom = js_AtomizeChars(cx, cb.begin(), length);
    cb.clear();
    return atom;
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
    memcpy(news, s, m);
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

    str = (asSource ? js_ValueToSource : js_ValueToString)(cx, v);
    if (!str)
        return NULL;
    str = js_QuoteString(cx, str, 0);
    if (!str)
        return NULL;
    return bytes->encode(cx, str);
}

JSString *
js_ValueToString(JSContext *cx, const Value &arg)
{
    Value v = arg;
    if (!ToPrimitive(cx, JSTYPE_STRING, &v))
        return NULL;

    JSString *str;
    if (v.isString()) {
        str = v.toString();
    } else if (v.isInt32()) {
        str = js_IntToString(cx, v.toInt32());
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


bool
js::ValueToStringBufferSlow(JSContext *cx, const Value &arg, StringBuffer &sb)
{
    Value v = arg;
    if (!ToPrimitive(cx, JSTYPE_STRING, &v))
        return false;

    if (v.isString())
        return sb.append(v.toString());
    if (v.isNumber())
        return NumberValueToStringBuffer(cx, v, sb);
    if (v.isBoolean())
        return BooleanToStringBuffer(cx, v.toBoolean(), sb);
    if (v.isNull())
        return sb.append(cx->runtime->atomState.nullAtom);
    JS_ASSERT(v.isUndefined());
    return sb.append(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
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
        
        if (v.isDouble() && JSDOUBLE_IS_NEGZERO(v.toDouble())) {
            
            static const jschar js_negzero_ucNstr[] = {'-', '0'};

            return js_NewStringCopyN(cx, js_negzero_ucNstr, 2);
        }
        return js_ValueToString(cx, v);
    }

    Value rval = NullValue();
    Value fval;
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.toSourceAtom);
    if (!js_GetMethod(cx, &v.toObject(), id, JSGET_NO_METHOD_BARRIER, &fval))
        return false;
    if (js_IsCallable(fval)) {
        if (!Invoke(cx, v, fval, 0, NULL, &rval))
            return false;
    }

    return js_ValueToString(cx, rval);
}

namespace js {

bool
EqualStrings(JSContext *cx, JSString *str1, JSString *str2, JSBool *result)
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

JSBool JS_FASTCALL
js_EqualStringsOnTrace(JSContext *cx, JSString *str1, JSString *str2)
{
    JSBool result;
    return EqualStrings(cx, str1, str2, &result) ? result : JS_NEITHER;
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_EqualStringsOnTrace,
                     CONTEXT, STRING, STRING, 1, nanojit::ACCSET_NONE)

namespace js {

static bool
CompareStringsImpl(JSContext *cx, JSString *str1, JSString *str2, int32 *result)
{
    JS_ASSERT(str1);
    JS_ASSERT(str2);

    if (str1 == str2) {
        *result = 0;
        return true;
    }

    size_t l1 = str1->length();
    const jschar *s1 = str1->getChars(cx);
    if (!s1)
        return false;

    size_t l2 = str2->length();
    const jschar *s2 = str2->getChars(cx);
    if (!s2)
        return false;

    size_t n = JS_MIN(l1, l2);
    for (size_t i = 0; i < n; i++) {
        if (int32 cmp = s1[i] - s2[i]) {
            *result = cmp;
            return true;
        }
    }
    *result = (int32)(l1 - l2);
    return true;
}

bool
CompareStrings(JSContext *cx, JSString *str1, JSString *str2, int32 *result)
{
    return CompareStringsImpl(cx, str1, str2, result);
}

}  

int32 JS_FASTCALL
js_CompareStringsOnTrace(JSContext *cx, JSString *str1, JSString *str2)
{
    int32 result;
    if (!CompareStringsImpl(cx, str1, str2, &result))
        return INT32_MIN;
    JS_ASSERT(result != INT32_MIN);
    return result;
}
JS_DEFINE_CALLINFO_3(extern, INT32, js_CompareStringsOnTrace,
                     CONTEXT, STRING, STRING, 1, nanojit::ACCSET_NONE)

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
    uintN c, c2;
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
    uint32 v;
    uint8 utf8buf[6];

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
    uint32 v;

    dstlen = dst ? *dstlenp : (size_t) -1;
    origDstlen = dstlen;
    offset = 0;
    bool useCESU8 = fc == CESU8Encoding;

    while (srclen) {
        v = (uint8) *src;
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
            v = Utf8ToOneUcs4Char((uint8 *)src, n);
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
            uint32 v;
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
            uint8 utf8buf[4];
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
            jsuint B = JS7_UNHEX(chars[k+1]) * 16 + JS7_UNHEX(chars[k+2]);
            k += 2;
            if (!(B & 0x80)) {
                c = (jschar)B;
            } else {
                intN n = 1;
                while (B & (0x80 >> n))
                    n++;
                if (n == 1 || n > 4)
                    goto report_bad_uri;
                uint8 octets[4];
                octets[0] = (uint8)B;
                if (k + 3 * (n - 1) >= length)
                    goto report_bad_uri;
                for (intN j = 1; j < n; j++) {
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
                uint32 v = Utf8ToOneUcs4Char(octets, n);
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
str_decodeURI(JSContext *cx, uintN argc, Value *vp)
{
    JSLinearString *str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Decode(cx, str, js_uriReservedPlusPound_ucstr, vp);
}

static JSBool
str_decodeURI_Component(JSContext *cx, uintN argc, Value *vp)
{
    JSLinearString *str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Decode(cx, str, js_empty_ucstr, vp);
}

static JSBool
str_encodeURI(JSContext *cx, uintN argc, Value *vp)
{
    JSLinearString *str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Encode(cx, str, js_uriReservedPlusPound_ucstr, js_uriUnescaped_ucstr,
                  vp);
}

static JSBool
str_encodeURI_Component(JSContext *cx, uintN argc, Value *vp)
{
    JSLinearString *str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Encode(cx, str, js_uriUnescaped_ucstr, NULL, vp);
}





int
js_OneUcs4ToUtf8Char(uint8 *utf8Buffer, uint32 ucs4Char)
{
    int utf8Length = 1;

    JS_ASSERT(ucs4Char <= 0x10FFFF);
    if (ucs4Char < 0x80) {
        *utf8Buffer = (uint8)ucs4Char;
    } else {
        int i;
        uint32 a = ucs4Char >> 11;
        utf8Length = 2;
        while (a) {
            a >>= 5;
            utf8Length++;
        }
        i = utf8Length;
        while (--i) {
            utf8Buffer[i] = (uint8)((ucs4Char & 0x3F) | 0x80);
            ucs4Char >>= 6;
        }
        *utf8Buffer = (uint8)(0x100 - (1 << (8-utf8Length)) + ucs4Char);
    }
    return utf8Length;
}






static uint32
Utf8ToOneUcs4Char(const uint8 *utf8Buffer, int utf8Length)
{
    JS_ASSERT(1 <= utf8Length && utf8Length <= 4);

    if (utf8Length == 1) {
        JS_ASSERT(!(*utf8Buffer & 0x80));
        return *utf8Buffer;
    }

    
    static const uint32 minucs4Table[] = { 0x80, 0x800, 0x10000 };

    JS_ASSERT((*utf8Buffer & (0x100 - (1 << (7 - utf8Length)))) ==
              (0x100 - (1 << (8 - utf8Length))));
    uint32 ucs4Char = *utf8Buffer++ & ((1 << (7 - utf8Length)) - 1);
    uint32 minucs4Char = minucs4Table[utf8Length - 2];
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
PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp, JSLinearString *str, uint32 quote)
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
    uintN shift = 0;
    uintN hex = 0;
    uintN u = 0;
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
