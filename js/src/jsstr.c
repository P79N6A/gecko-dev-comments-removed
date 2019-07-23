

















































#include "jsstddef.h"
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
#include "jsconfig.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsstr.h"

#define JSSTRDEP_RECURSION_LIMIT        100

size_t
js_MinimizeDependentStrings(JSString *str, int level, JSString **basep)
{
    JSString *base;
    size_t start, length;

    JS_ASSERT(JSSTRING_IS_DEPENDENT(str));
    base = JSSTRDEP_BASE(str);
    start = JSSTRDEP_START(str);
    if (JSSTRING_IS_DEPENDENT(base)) {
        if (level < JSSTRDEP_RECURSION_LIMIT) {
            start += js_MinimizeDependentStrings(base, level + 1, &base);
        } else {
            do {
                start += JSSTRDEP_START(base);
                base = JSSTRDEP_BASE(base);
            } while (JSSTRING_IS_DEPENDENT(base));
        }
        if (start == 0) {
            JS_ASSERT(JSSTRING_IS_PREFIX(str));
            JSPREFIX_SET_BASE(str, base);
        } else if (start <= JSSTRDEP_START_MASK) {
            length = JSSTRDEP_LENGTH(str);
            JSSTRDEP_SET_START_AND_LENGTH(str, start, length);
            JSSTRDEP_SET_BASE(str, base);
        }
    }
    *basep = base;
    return start;
}

jschar *
js_GetDependentStringChars(JSString *str)
{
    size_t start;
    JSString *base;

    start = js_MinimizeDependentStrings(str, 0, &base);
    JS_ASSERT(!JSSTRING_IS_DEPENDENT(base));
    JS_ASSERT(start < base->length);
    return base->chars + start;
}

const jschar *
js_GetStringChars(JSContext *cx, JSString *str)
{
    if (JSSTRING_IS_DEPENDENT(str) && !js_UndependString(cx, str))
        return NULL;

    *js_GetGCThingFlags(str) &= ~GCF_MUTABLE;
    return str->chars;
}

JSString *
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right)
{
    size_t rn, ln, lrdist, n;
    jschar *rs, *ls, *s;
    JSDependentString *ldep;    
    JSString *str;

    if (JSSTRING_IS_DEPENDENT(right)) {
        rn = JSSTRDEP_LENGTH(right);
        rs = JSSTRDEP_CHARS(right);
    } else {
        rn = right->length;
        rs = right->chars;
    }
    if (rn == 0)
        return left;

    if (JSSTRING_IS_DEPENDENT(left) ||
        !(*js_GetGCThingFlags(left) & GCF_MUTABLE)) {
        
        ln = JSSTRING_LENGTH(left);
        if (ln == 0)
            return right;
        ls = JSSTRING_CHARS(left);
        s = (jschar *) JS_malloc(cx, (ln + rn + 1) * sizeof(jschar));
        if (!s)
            return NULL;
        js_strncpy(s, ls, ln);
        ldep = NULL;
    } else {
        
        ln = left->length;
        if (ln == 0)
            return right;
        ls = left->chars;
        s = (jschar *) JS_realloc(cx, ls, (ln + rn + 1) * sizeof(jschar));
        if (!s)
            return NULL;

        
        lrdist = (size_t)(rs - ls);
        if (lrdist < ln)
            rs = s + lrdist;
        left->chars = ls = s;
        ldep = JSSTRDEP(left);
    }

    js_strncpy(s + ln, rs, rn);
    n = ln + rn;
    s[n] = 0;
    str = js_NewString(cx, s, n, GCF_MUTABLE);
    if (!str) {
        
        if (!ldep) {
            JS_free(cx, s);
        } else {
            s = (jschar *) JS_realloc(cx, ls, (ln + 1) * sizeof(jschar));
            if (s)
                left->chars = s;
        }
    } else {
        
        if (ldep) {
            JSPREFIX_SET_LENGTH(ldep, ln);
            JSPREFIX_SET_BASE(ldep, str);
#ifdef DEBUG
          {
            JSRuntime *rt = cx->runtime;
            JS_RUNTIME_METER(rt, liveDependentStrings);
            JS_RUNTIME_METER(rt, totalDependentStrings);
            JS_LOCK_RUNTIME_VOID(rt,
                (rt->strdepLengthSum += (double)ln,
                 rt->strdepLengthSquaredSum += (double)ln * (double)ln));
          }
#endif
        }
    }

    return str;
}

const jschar *
js_UndependString(JSContext *cx, JSString *str)
{
    size_t n, size;
    jschar *s;

    if (JSSTRING_IS_DEPENDENT(str)) {
        n = JSSTRDEP_LENGTH(str);
        size = (n + 1) * sizeof(jschar);
        s = (jschar *) JS_malloc(cx, size);
        if (!s)
            return NULL;

        js_strncpy(s, JSSTRDEP_CHARS(str), n);
        s[n] = 0;
        str->length = n;
        str->chars = s;

#ifdef DEBUG
        {
            JSRuntime *rt = cx->runtime;
            JS_RUNTIME_UNMETER(rt, liveDependentStrings);
            JS_RUNTIME_UNMETER(rt, totalDependentStrings);
            JS_LOCK_RUNTIME_VOID(rt,
                (rt->strdepLengthSum -= (double)n,
                 rt->strdepLengthSquaredSum -= (double)n * (double)n));
        }
#endif
    }

    return str->chars;
}




static JSBool
str_decodeURI(JSContext *cx, uintN argc, jsval *vp);

static JSBool
str_decodeURI_Component(JSContext *cx, uintN argc, jsval *vp);

static JSBool
str_encodeURI(JSContext *cx, uintN argc, jsval *vp);

static JSBool
str_encodeURI_Component(JSContext *cx, uintN argc, jsval *vp);

static uint32
Utf8ToOneUcs4Char(const uint8 *utf8Buffer, int utf8Length);















#define URL_XALPHAS     ((uint8) 1)
#define URL_XPALPHAS    ((uint8) 2)
#define URL_PATH        ((uint8) 4)

static const uint8 urlCharType[256] =





    
    {    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       
         0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,       
         0,0,0,0,0,0,0,0,0,0,7,4,0,7,7,4,       
         7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,0,       
         7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,       
         7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,7,       
         0,7,7,7,7,7,7,7,7,7,7,7,7,7,7,7,       
         7,7,7,7,7,7,7,7,7,7,7,0,0,0,0,0,       
         0, };



#define IS_OK(C, mask) (urlCharType[((uint8) (C))] & (mask))


JSBool
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;
    size_t i, ni, length, newlength;
    const jschar *chars;
    jschar *newchars;
    jschar ch;
    jsint mask;
    jsdouble d;
    const char digits[] = {'0', '1', '2', '3', '4', '5', '6', '7',
                           '8', '9', 'A', 'B', 'C', 'D', 'E', 'F' };

    mask = URL_XALPHAS | URL_XPALPHAS | URL_PATH;
    if (argc > 1) {
        if (!js_ValueToNumber(cx, argv[1], &d))
            return JS_FALSE;
        if (!JSDOUBLE_IS_FINITE(d) ||
            (mask = (jsint)d) != d ||
            mask & ~(URL_XALPHAS | URL_XPALPHAS | URL_PATH))
        {
            char numBuf[12];
            JS_snprintf(numBuf, sizeof numBuf, "%lx", (unsigned long) mask);
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_STRING_MASK, numBuf);
            return JS_FALSE;
        }
    }

    str = js_ValueToString(cx, argv[0]);
    if (!str)
        return JS_FALSE;
    argv[0] = STRING_TO_JSVAL(str);

    chars = JSSTRING_CHARS(str);
    length = newlength = JSSTRING_LENGTH(str);

    
    for (i = 0; i < length; i++) {
        if ((ch = chars[i]) < 128 && IS_OK(ch, mask))
            continue;
        if (ch < 256) {
            if (mask == URL_XPALPHAS && ch == ' ')
                continue;   
            newlength += 2; 
        } else {
            newlength += 5; 
        }

        



        if (newlength < length) {
            JS_ReportOutOfMemory(cx);
            return JS_FALSE;
        }
    }

    if (newlength >= ~(size_t)0 / sizeof(jschar)) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    newchars = (jschar *) JS_malloc(cx, (newlength + 1) * sizeof(jschar));
    if (!newchars)
        return JS_FALSE;
    for (i = 0, ni = 0; i < length; i++) {
        if ((ch = chars[i]) < 128 && IS_OK(ch, mask)) {
            newchars[ni++] = ch;
        } else if (ch < 256) {
            if (mask == URL_XPALPHAS && ch == ' ') {
                newchars[ni++] = '+'; 
            } else {
                newchars[ni++] = '%';
                newchars[ni++] = digits[ch >> 4];
                newchars[ni++] = digits[ch & 0xF];
            }
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

    str = js_NewString(cx, newchars, newlength, 0);
    if (!str) {
        JS_free(cx, newchars);
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}
#undef IS_OK

static JSBool
str_escape(JSContext *cx, uintN argc, jsval *vp)
{
    return js_str_escape(cx, JS_THIS_OBJECT(cx, vp), argc, vp + 2, vp);
}


static JSBool
str_unescape(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    size_t i, ni, length;
    const jschar *chars;
    jschar *newchars;
    jschar ch;

    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(str);

    chars = JSSTRING_CHARS(str);
    length = JSSTRING_LENGTH(str);

    
    newchars = (jschar *) JS_malloc(cx, (length + 1) * sizeof(jschar));
    if (!newchars)
        return JS_FALSE;
    ni = i = 0;
    while (i < length) {
        ch = chars[i++];
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

    str = js_NewString(cx, newchars, ni, 0);
    if (!str) {
        JS_free(cx, newchars);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

#if JS_HAS_UNEVAL
static JSBool
str_uneval(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    str = js_ValueToSource(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
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
    JS_FN(js_escape_str,             str_escape,                1,1,0,0),
    JS_FN(js_unescape_str,           str_unescape,              1,1,0,0),
#if JS_HAS_UNEVAL
    JS_FN(js_uneval_str,             str_uneval,                1,1,0,0),
#endif
    JS_FN(js_decodeURI_str,          str_decodeURI,             1,1,0,0),
    JS_FN(js_encodeURI_str,          str_encodeURI,             1,1,0,0),
    JS_FN(js_decodeURIComponent_str, str_decodeURI_Component,   1,1,0,0),
    JS_FN(js_encodeURIComponent_str, str_encodeURI_Component,   1,1,0,0),

    JS_FS_END
};

jschar      js_empty_ucstr[]  = {0};
JSSubString js_EmptySubString = {0, js_empty_ucstr};

enum string_tinyid {
    STRING_LENGTH = -1
};

static JSPropertySpec string_props[] = {
    {js_length_str,     STRING_LENGTH,
                        JSPROP_READONLY|JSPROP_PERMANENT|JSPROP_SHARED, 0,0},
    {0,0,0,0,0}
};

static JSBool
str_getProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsval v;
    JSString *str;
    jsint slot;

    if (!JSVAL_IS_INT(id))
        return JS_TRUE;

    slot = JSVAL_TO_INT(id);
    if (slot == STRING_LENGTH) {
        if (OBJ_GET_CLASS(cx, obj) == &js_StringClass) {
            
            v = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
            JS_ASSERT(JSVAL_IS_STRING(v));
            str = JSVAL_TO_STRING(v);
        } else {
            
            str = js_ValueToString(cx, OBJECT_TO_JSVAL(obj));
            if (!str)
                return JS_FALSE;
        }

        *vp = INT_TO_JSVAL((jsint) JSSTRING_LENGTH(str));
    }
    return JS_TRUE;
}

#define STRING_ELEMENT_ATTRS (JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT)

static JSBool
str_enumerate(JSContext *cx, JSObject *obj)
{
    jsval v;
    JSString *str, *str1;
    size_t i, length;

    v = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    JS_ASSERT(JSVAL_IS_STRING(v));
    str = JSVAL_TO_STRING(v);

    length = JSSTRING_LENGTH(str);
    for (i = 0; i < length; i++) {
        str1 = js_NewDependentString(cx, str, i, 1);
        if (!str1)
            return JS_FALSE;
        if (!OBJ_DEFINE_PROPERTY(cx, obj, INT_TO_JSID(i),
                                 STRING_TO_JSVAL(str1), NULL, NULL,
                                 STRING_ELEMENT_ATTRS, NULL)) {
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
str_resolve(JSContext *cx, JSObject *obj, jsval id, uintN flags,
            JSObject **objp)
{
    jsval v;
    JSString *str, *str1;
    jsint slot;

    if (!JSVAL_IS_INT(id) || (flags & JSRESOLVE_ASSIGNING))
        return JS_TRUE;

    v = OBJ_GET_SLOT(cx, obj, JSSLOT_PRIVATE);
    JS_ASSERT(JSVAL_IS_STRING(v));
    str = JSVAL_TO_STRING(v);

    slot = JSVAL_TO_INT(id);
    if ((size_t)slot < JSSTRING_LENGTH(str)) {
        str1 = js_NewDependentString(cx, str, (size_t)slot, 1);
        if (!str1)
            return JS_FALSE;
        if (!OBJ_DEFINE_PROPERTY(cx, obj, INT_TO_JSID(slot),
                                 STRING_TO_JSVAL(str1), NULL, NULL,
                                 STRING_ELEMENT_ATTRS, NULL)) {
            return JS_FALSE;
        }
        *objp = obj;
    }
    return JS_TRUE;
}

JSClass js_StringClass = {
    js_String_str,
    JSCLASS_HAS_PRIVATE | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_String),
    JS_PropertyStub,   JS_PropertyStub,   str_getProperty,   JS_PropertyStub,
    str_enumerate, (JSResolveOp)str_resolve, JS_ConvertStub, JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

#if JS_HAS_TOSOURCE





static JSBool
str_quote(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    str = js_QuoteString(cx, str, '"');
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
str_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    jsval v;
    JSString *str;
    size_t i, j, k, n;
    char buf[16];
    jschar *s, *t;

    if (!js_GetPrimitiveThis(cx, vp, &js_StringClass, &v))
        return JS_FALSE;
    JS_ASSERT(JSVAL_IS_STRING(v));
    str = js_QuoteString(cx, JSVAL_TO_STRING(v), '"');
    if (!str)
        return JS_FALSE;
    j = JS_snprintf(buf, sizeof buf, "(new %s(", js_StringClass.name);
    s = JSSTRING_CHARS(str);
    k = JSSTRING_LENGTH(str);
    n = j + k + 2;
    t = (jschar *) JS_malloc(cx, (n + 1) * sizeof(jschar));
    if (!t)
        return JS_FALSE;
    for (i = 0; i < j; i++)
        t[i] = buf[i];
    for (j = 0; j < k; i++, j++)
        t[i] = s[j];
    t[i++] = ')';
    t[i++] = ')';
    t[i] = 0;
    str = js_NewString(cx, t, n, 0);
    if (!str) {
        JS_free(cx, t);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

#endif 

static JSBool
str_toString(JSContext *cx, uintN argc, jsval *vp)
{
    return js_GetPrimitiveThis(cx, vp, &js_StringClass, vp);
}




static JSBool
str_substring(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    jsdouble d;
    jsdouble length, begin, end;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    if (argc != 0) {
        if (!js_ValueToNumber(cx, vp[2], &d))
            return JS_FALSE;
        length = JSSTRING_LENGTH(str);
        begin = js_DoubleToInteger(d);
        if (begin < 0)
            begin = 0;
        else if (begin > length)
            begin = length;

        if (argc == 1) {
            end = length;
        } else {
            if (!js_ValueToNumber(cx, vp[3], &d))
                return JS_FALSE;
            end = js_DoubleToInteger(d);
            if (end < 0)
                end = 0;
            else if (end > length)
                end = length;
            if (end < begin) {
                
                jsdouble tmp = begin;
                begin = end;
                end = tmp;
            }
        }

        str = js_NewDependentString(cx, str, (size_t)begin,
                                    (size_t)(end - begin));
        if (!str)
            return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
str_toLowerCase(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    size_t i, n;
    jschar *s, *news;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    n = JSSTRING_LENGTH(str);
    news = (jschar *) JS_malloc(cx, (n + 1) * sizeof(jschar));
    if (!news)
        return JS_FALSE;
    s = JSSTRING_CHARS(str);
    for (i = 0; i < n; i++)
        news[i] = JS_TOLOWER(s[i]);
    news[n] = 0;
    str = js_NewString(cx, news, n, 0);
    if (!str) {
        JS_free(cx, news);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
str_toLocaleLowerCase(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToLowerCase) {
        str = js_ValueToString(cx, vp[1]);
        if (!str)
            return JS_FALSE;
        vp[1] = STRING_TO_JSVAL(str);
        return cx->localeCallbacks->localeToLowerCase(cx, str, vp);
    }
    return str_toLowerCase(cx, 0, vp);
}

static JSBool
str_toUpperCase(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    size_t i, n;
    jschar *s, *news;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    n = JSSTRING_LENGTH(str);
    news = (jschar *) JS_malloc(cx, (n + 1) * sizeof(jschar));
    if (!news)
        return JS_FALSE;
    s = JSSTRING_CHARS(str);
    for (i = 0; i < n; i++)
        news[i] = JS_TOUPPER(s[i]);
    news[n] = 0;
    str = js_NewString(cx, news, n, 0);
    if (!str) {
        JS_free(cx, news);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
str_toLocaleUpperCase(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToUpperCase) {
        str = js_ValueToString(cx, vp[1]);
        if (!str)
            return JS_FALSE;
        vp[1] = STRING_TO_JSVAL(str);
        return cx->localeCallbacks->localeToUpperCase(cx, str, vp);
    }
    return str_toUpperCase(cx, 0, vp);
}

static JSBool
str_localeCompare(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str, *thatStr;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    if (argc == 0) {
        *vp = JSVAL_ZERO;
    } else {
        thatStr = js_ValueToString(cx, vp[2]);
        if (!thatStr)
            return JS_FALSE;
        if (cx->localeCallbacks && cx->localeCallbacks->localeCompare) {
            vp[2] = STRING_TO_JSVAL(thatStr);
            return cx->localeCallbacks->localeCompare(cx, str, thatStr, vp);
        }
        *vp = INT_TO_JSVAL(js_CompareStrings(str, thatStr));
    }
    return JS_TRUE;
}

static JSBool
str_charAt(JSContext *cx, uintN argc, jsval *vp)
{
    jsval t, v;
    JSString *str;
    jsint i;
    jschar c;
    jsdouble d;

    t = vp[1];
    v = vp[2];
    if (JSVAL_IS_STRING(t) && JSVAL_IS_INT(v)) {
        str = JSVAL_TO_STRING(t);
        i = JSVAL_TO_INT(v);
        if ((size_t)i >= JSSTRING_LENGTH(str))
            goto out_of_range;
    } else {
        str = js_ValueToString(cx, t);
        if (!str)
            return JS_FALSE;
        vp[1] = STRING_TO_JSVAL(str);

        if (argc == 0) {
            d = 0.0;
        } else {
            if (!js_ValueToNumber(cx, v, &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
        }

        if (d < 0 || JSSTRING_LENGTH(str) <= d)
            goto out_of_range;
        i = (jsint) d;
    }

    c = JSSTRING_CHARS(str)[i];
    str = (c < UNIT_STRING_LIMIT)
          ? js_GetUnitString(cx, c)
          : js_NewDependentString(cx, str, i, 1);
    if (!str)
        return JS_FALSE;
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;

out_of_range:
    *vp = JS_GetEmptyStringValue(cx);
    return JS_TRUE;
}

static JSBool
str_charCodeAt(JSContext *cx, uintN argc, jsval *vp)
{
    jsval t, v;
    JSString *str;
    jsint i;
    jsdouble d;

    t = vp[1];
    v = vp[2];
    if (JSVAL_IS_STRING(t) && JSVAL_IS_INT(v)) {
        str = JSVAL_TO_STRING(t);
        i = JSVAL_TO_INT(v);
        if ((size_t)i >= JSSTRING_LENGTH(str))
            goto out_of_range;
    } else {
        str = js_ValueToString(cx, t);
        if (!str)
            return JS_FALSE;
        vp[1] = STRING_TO_JSVAL(str);

        if (argc == 0) {
            d = 0.0;
        } else {
            if (!js_ValueToNumber(cx, v, &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
        }

        if (d < 0 || JSSTRING_LENGTH(str) <= d)
            goto out_of_range;
        i = (jsint) d;
    }

    *vp = INT_TO_JSVAL(JSSTRING_CHARS(str)[i]);
    return JS_TRUE;

out_of_range:
    *vp = JS_GetNaNValue(cx);
    return JS_TRUE;
}

jsint
js_BoyerMooreHorspool(const jschar *text, jsint textlen,
                      const jschar *pat, jsint patlen,
                      jsint start)
{
    jsint i, j, k, m;
    uint8 skip[BMH_CHARSET_SIZE];
    jschar c;

    JS_ASSERT(0 < patlen && patlen <= BMH_PATLEN_MAX);
    for (i = 0; i < BMH_CHARSET_SIZE; i++)
        skip[i] = (uint8)patlen;
    m = patlen - 1;
    for (i = 0; i < m; i++) {
        c = pat[i];
        if (c >= BMH_CHARSET_SIZE)
            return BMH_BAD_PATTERN;
        skip[c] = (uint8)(m - i);
    }
    for (k = start + m;
         k < textlen;
         k += ((c = text[k]) >= BMH_CHARSET_SIZE) ? patlen : skip[c]) {
        for (i = k, j = m; ; i--, j--) {
            if (j < 0)
                return i + 1;
            if (text[i] != pat[j])
                break;
        }
    }
    return -1;
}

static JSBool
str_indexOf(JSContext *cx, uintN argc, jsval *vp)
{
    jsval t, v;
    JSString *str, *str2;
    const jschar *text, *pat;
    jsint i, j, index, textlen, patlen;
    jsdouble d;

    t = vp[1];
    v = vp[2];
    if (JSVAL_IS_STRING(t) && JSVAL_IS_STRING(v)) {
        str = JSVAL_TO_STRING(t);
        str2 = JSVAL_TO_STRING(v);
    } else {
        str = js_ValueToString(cx, t);
        if (!str)
            return JS_FALSE;
        vp[1] = STRING_TO_JSVAL(str);

        str2 = js_ValueToString(cx, v);
        if (!str2)
            return JS_FALSE;
        vp[2] = STRING_TO_JSVAL(str2);
    }

    text = JSSTRING_CHARS(str);
    textlen = (jsint) JSSTRING_LENGTH(str);
    pat = JSSTRING_CHARS(str2);
    patlen = (jsint) JSSTRING_LENGTH(str2);

    if (argc > 1) {
        if (!js_ValueToNumber(cx, vp[3], &d))
            return JS_FALSE;
        d = js_DoubleToInteger(d);
        if (d < 0)
            i = 0;
        else if (d > textlen)
            i = textlen;
        else
            i = (jsint)d;
    } else {
        i = 0;
    }
    if (patlen == 0) {
        *vp = INT_TO_JSVAL(i);
        return JS_TRUE;
    }

    
    if ((jsuint)(patlen - 2) <= BMH_PATLEN_MAX - 2 && textlen >= 512) {
        index = js_BoyerMooreHorspool(text, textlen, pat, patlen, i);
        if (index != BMH_BAD_PATTERN)
            goto out;
    }

    index = -1;
    j = 0;
    while (i + j < textlen) {
        if (text[i + j] == pat[j]) {
            if (++j == patlen) {
                index = i;
                break;
            }
        } else {
            i++;
            j = 0;
        }
    }

out:
    *vp = INT_TO_JSVAL(index);
    return JS_TRUE;
}

static JSBool
str_lastIndexOf(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str, *str2;
    const jschar *text, *pat;
    jsint i, j, textlen, patlen;
    jsdouble d;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);
    text = JSSTRING_CHARS(str);
    textlen = (jsint) JSSTRING_LENGTH(str);

    str2 = js_ValueToString(cx, vp[2]);
    if (!str2)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(str2);
    pat = JSSTRING_CHARS(str2);
    patlen = (jsint) JSSTRING_LENGTH(str2);

    if (argc > 1) {
        if (!js_ValueToNumber(cx, vp[3], &d))
            return JS_FALSE;
        if (JSDOUBLE_IS_NaN(d)) {
            i = textlen;
        } else {
            d = js_DoubleToInteger(d);
            if (d < 0)
                i = 0;
            else if (d > textlen)
                i = textlen;
            else
                i = (jsint)d;
        }
    } else {
        i = textlen;
    }

    if (patlen == 0) {
        *vp = INT_TO_JSVAL(i);
        return JS_TRUE;
    }

    j = 0;
    while (i >= 0) {
        
        if (i + j < textlen && text[i + j] == pat[j]) {
            if (++j == patlen)
                break;
        } else {
            i--;
            j = 0;
        }
    }
    *vp = INT_TO_JSVAL(i);
    return JS_TRUE;
}




typedef struct GlobData {
    uintN       flags;          
    uintN       optarg;         
    JSString    *str;           
    JSRegExp    *regexp;        
} GlobData;




#define MODE_MATCH      0x00    /* in: return match array on success */
#define MODE_REPLACE    0x01    /* in: match and replace */
#define MODE_SEARCH     0x02    /* in: search only, return match index or -1 */
#define GET_MODE(f)     ((f) & 0x03)
#define FORCE_FLAT      0x04    /* in: force flat (non-regexp) string match */
#define KEEP_REGEXP     0x08    /* inout: keep GlobData.regexp alive for caller
                                          of match_or_replace; if set on input
                                          but clear on output, regexp ownership
                                          does not pass to caller */
#define GLOBAL_REGEXP   0x10    /* out: regexp had the 'g' flag */

static JSBool
match_or_replace(JSContext *cx,
                 JSBool (*glob)(JSContext *cx, jsint count, GlobData *data),
                 void (*destroy)(JSContext *cx, GlobData *data),
                 GlobData *data, uintN argc, jsval *vp)
{
    JSString *str, *src, *opt;
    JSObject *reobj;
    JSRegExp *re;
    size_t index, length;
    JSBool ok, test;
    jsint count;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);
    data->str = str;

    if (JSVAL_IS_REGEXP(cx, vp[2])) {
        reobj = JSVAL_TO_OBJECT(vp[2]);
        re = (JSRegExp *) JS_GetPrivate(cx, reobj);
    } else {
        src = js_ValueToString(cx, vp[2]);
        if (!src)
            return JS_FALSE;
        if (data->optarg < argc) {
            vp[2] = STRING_TO_JSVAL(src);
            opt = js_ValueToString(cx, vp[2 + data->optarg]);
            if (!opt)
                return JS_FALSE;
        } else {
            opt = NULL;
        }
        re = js_NewRegExpOpt(cx, NULL, src, opt,
                             (data->flags & FORCE_FLAT) != 0);
        if (!re)
            return JS_FALSE;
        reobj = NULL;
    }
    
    data->regexp = re;
    HOLD_REGEXP(cx, re);

    if (re->flags & JSREG_GLOB)
        data->flags |= GLOBAL_REGEXP;
    index = 0;
    if (GET_MODE(data->flags) == MODE_SEARCH) {
        ok = js_ExecuteRegExp(cx, re, str, &index, JS_TRUE, vp);
        if (ok) {
            *vp = (*vp == JSVAL_TRUE)
                  ? INT_TO_JSVAL(cx->regExpStatics.leftContext.length)
                  : INT_TO_JSVAL(-1);
        }
    } else if (data->flags & GLOBAL_REGEXP) {
        if (reobj) {
            
            ok = js_SetLastIndex(cx, reobj, 0);
        } else {
            ok = JS_TRUE;
        }
        if (ok) {
            length = JSSTRING_LENGTH(str);
            for (count = 0; index <= length; count++) {
                ok = js_ExecuteRegExp(cx, re, str, &index, JS_TRUE, vp);
                if (!ok || *vp != JSVAL_TRUE)
                    break;
                ok = glob(cx, count, data);
                if (!ok)
                    break;
                if (cx->regExpStatics.lastMatch.length == 0) {
                    if (index == length)
                        break;
                    index++;
                }
            }
            if (!ok && destroy)
                destroy(cx, data);
        }
    } else {
        if (GET_MODE(data->flags) == MODE_REPLACE) {
            test = JS_TRUE;
        } else {
            





            JSStackFrame *fp;

            
            for (fp = cx->fp; fp && !fp->pc; fp = fp->down)
                JS_ASSERT(!fp->script);

            
            test = JS_FALSE;
            if (fp) {
                JS_ASSERT(*fp->pc == JSOP_CALL || *fp->pc == JSOP_NEW);
                JS_ASSERT(js_CodeSpec[*fp->pc].length == 3);
                switch (fp->pc[3]) {
                  case JSOP_POP:
                  case JSOP_IFEQ:
                  case JSOP_IFNE:
                  case JSOP_IFEQX:
                  case JSOP_IFNEX:
                    test = JS_TRUE;
                    break;
                  default:;
                }
            }
        }
        ok = js_ExecuteRegExp(cx, re, str, &index, test, vp);
    }

    DROP_REGEXP(cx, re);
    if (reobj) {
        
        data->flags &= ~KEEP_REGEXP;
    } else if (!ok || !(data->flags & KEEP_REGEXP)) {
        
        data->regexp = NULL;
        js_DestroyRegExp(cx, re);
    }

    return ok;
}

typedef struct MatchData {
    GlobData    base;
    jsval       *arrayval;      
} MatchData;

static JSBool
match_glob(JSContext *cx, jsint count, GlobData *data)
{
    MatchData *mdata;
    JSObject *arrayobj;
    JSSubString *matchsub;
    JSString *matchstr;
    jsval v;

    mdata = (MatchData *)data;
    arrayobj = JSVAL_TO_OBJECT(*mdata->arrayval);
    if (!arrayobj) {
        arrayobj = js_ConstructObject(cx, &js_ArrayClass, NULL, NULL, 0, NULL);
        if (!arrayobj)
            return JS_FALSE;
        *mdata->arrayval = OBJECT_TO_JSVAL(arrayobj);
    }
    matchsub = &cx->regExpStatics.lastMatch;
    matchstr = js_NewStringCopyN(cx, matchsub->chars, matchsub->length);
    if (!matchstr)
        return JS_FALSE;
    v = STRING_TO_JSVAL(matchstr);
    return js_SetProperty(cx, arrayobj, INT_TO_JSID(count), &v);
}

static JSBool
str_match(JSContext *cx, uintN argc, jsval *vp)
{
    MatchData mdata;
    JSBool ok;

    mdata.base.flags = MODE_MATCH;
    mdata.base.optarg = 1;
    mdata.arrayval = &vp[4];
    *mdata.arrayval = JSVAL_NULL;
    ok = match_or_replace(cx, match_glob, NULL, &mdata.base, argc, vp);
    if (ok && !JSVAL_IS_NULL(*mdata.arrayval))
        *vp = *mdata.arrayval;
    return ok;
}

static JSBool
str_search(JSContext *cx, uintN argc, jsval *vp)
{
    GlobData data;

    data.flags = MODE_SEARCH;
    data.optarg = 1;
    return match_or_replace(cx, NULL, NULL, &data, argc, vp);
}

typedef struct ReplaceData {
    GlobData    base;           
    JSObject    *lambda;        
    JSString    *repstr;        
    jschar      *dollar;        
    jschar      *dollarEnd;     
    jschar      *chars;         
    size_t      length;         
    jsint       index;          
    jsint       leftIndex;      
    JSSubString dollarStr;      
} ReplaceData;

static JSSubString *
interpret_dollar(JSContext *cx, jschar *dp, jschar *ep, ReplaceData *rdata,
                 size_t *skip)
{
    JSRegExpStatics *res;
    jschar dc, *cp;
    uintN num, tmp;

    JS_ASSERT(*dp == '$');

    
    if (dp + 1 >= ep)
        return NULL;

    
    res = &cx->regExpStatics;
    dc = dp[1];
    if (JS7_ISDEC(dc)) {
        
        num = JS7_UNDEC(dc);
        if (num > res->parenCount)
            return NULL;

        cp = dp + 2;
        if (cp < ep && (dc = *cp, JS7_ISDEC(dc))) {
            tmp = 10 * num + JS7_UNDEC(dc);
            if (tmp <= res->parenCount) {
                cp++;
                num = tmp;
            }
        }
        if (num == 0)
            return NULL;

        
        num--;
        *skip = cp - dp;
        return REGEXP_PAREN_SUBSTRING(res, num);
    }

    *skip = 2;
    switch (dc) {
      case '$':
        rdata->dollarStr.chars = dp;
        rdata->dollarStr.length = 1;
        return &rdata->dollarStr;
      case '&':
        return &res->lastMatch;
      case '+':
        return &res->lastParen;
      case '`':
        return &res->leftContext;
      case '\'':
        return &res->rightContext;
    }
    return NULL;
}

static JSBool
find_replen(JSContext *cx, ReplaceData *rdata, size_t *sizep)
{
    JSString *repstr;
    size_t replen, skip;
    jschar *dp, *ep;
    JSSubString *sub;
    JSObject *lambda;

    lambda = rdata->lambda;
    if (lambda) {
        uintN argc, i, j, m, n, p;
        jsval *sp, *oldsp, rval;
        void *mark;
        JSStackFrame *fp;
        JSBool ok;

        





        JSRegExpStatics save = cx->regExpStatics;
        JSBool freeMoreParens = JS_FALSE;

        







        p = rdata->base.regexp->parenCount;
        argc = 1 + p + 2;
        sp = js_AllocStack(cx, 2 + argc, &mark);
        if (!sp)
            return JS_FALSE;

        
        *sp++ = OBJECT_TO_JSVAL(lambda);
        *sp++ = OBJECT_TO_JSVAL(OBJ_GET_PARENT(cx, lambda));

#define PUSH_REGEXP_STATIC(sub)                                               \
    JS_BEGIN_MACRO                                                            \
        JSString *str = js_NewStringCopyN(cx,                                 \
                                          cx->regExpStatics.sub.chars,        \
                                          cx->regExpStatics.sub.length);      \
        if (!str) {                                                           \
            ok = JS_FALSE;                                                    \
            goto lambda_out;                                                  \
        }                                                                     \
        *sp++ = STRING_TO_JSVAL(str);                                         \
    JS_END_MACRO

        
        PUSH_REGEXP_STATIC(lastMatch);
        i = 0;
        m = cx->regExpStatics.parenCount;
        n = JS_MIN(m, 9);
        for (j = 0; i < n; i++, j++)
            PUSH_REGEXP_STATIC(parens[j]);
        for (j = 0; i < m; i++, j++)
            PUSH_REGEXP_STATIC(moreParens[j]);

        




        cx->regExpStatics.moreParens = NULL;
        freeMoreParens = JS_TRUE;

#undef PUSH_REGEXP_STATIC

        
        for (; i < p; i++)
            *sp++ = JSVAL_VOID;

        
        *sp++ = INT_TO_JSVAL((jsint)cx->regExpStatics.leftContext.length);
        *sp++ = STRING_TO_JSVAL(rdata->base.str);

        
        fp = cx->fp;
        oldsp = fp->sp;
        fp->sp = sp;
        ok = js_Invoke(cx, argc, JSINVOKE_INTERNAL);
        rval = fp->sp[-1];
        fp->sp = oldsp;

        if (ok) {
            




            repstr = js_ValueToString(cx, rval);
            if (!repstr) {
                ok = JS_FALSE;
            } else {
                rdata->repstr = repstr;
                *sizep = JSSTRING_LENGTH(repstr);
            }
        }

      lambda_out:
        js_FreeStack(cx, mark);
        if (freeMoreParens)
            JS_free(cx, cx->regExpStatics.moreParens);
        cx->regExpStatics = save;
        return ok;
    }

    repstr = rdata->repstr;
    replen = JSSTRING_LENGTH(repstr);
    for (dp = rdata->dollar, ep = rdata->dollarEnd; dp;
         dp = js_strchr_limit(dp, '$', ep)) {
        sub = interpret_dollar(cx, dp, ep, rdata, &skip);
        if (sub) {
            replen += sub->length - skip;
            dp += skip;
        }
        else
            dp++;
    }
    *sizep = replen;
    return JS_TRUE;
}

static void
do_replace(JSContext *cx, ReplaceData *rdata, jschar *chars)
{
    JSString *repstr;
    jschar *bp, *cp, *dp, *ep;
    size_t len, skip;
    JSSubString *sub;

    repstr = rdata->repstr;
    bp = cp = JSSTRING_CHARS(repstr);
    for (dp = rdata->dollar, ep = rdata->dollarEnd; dp;
         dp = js_strchr_limit(dp, '$', ep)) {
        len = dp - cp;
        js_strncpy(chars, cp, len);
        chars += len;
        cp = dp;
        sub = interpret_dollar(cx, dp, ep, rdata, &skip);
        if (sub) {
            len = sub->length;
            js_strncpy(chars, sub->chars, len);
            chars += len;
            cp += skip;
            dp += skip;
        } else {
            dp++;
        }
    }
    js_strncpy(chars, cp, JSSTRING_LENGTH(repstr) - (cp - bp));
}

static void
replace_destroy(JSContext *cx, GlobData *data)
{
    ReplaceData *rdata;

    rdata = (ReplaceData *)data;
    JS_free(cx, rdata->chars);
    rdata->chars = NULL;
}

static JSBool
replace_glob(JSContext *cx, jsint count, GlobData *data)
{
    ReplaceData *rdata;
    JSString *str;
    size_t leftoff, leftlen, replen, growth;
    const jschar *left;
    jschar *chars;

    rdata = (ReplaceData *)data;
    str = data->str;
    leftoff = rdata->leftIndex;
    left = JSSTRING_CHARS(str) + leftoff;
    leftlen = cx->regExpStatics.lastMatch.chars - left;
    rdata->leftIndex = cx->regExpStatics.lastMatch.chars - JSSTRING_CHARS(str);
    rdata->leftIndex += cx->regExpStatics.lastMatch.length;
    if (!find_replen(cx, rdata, &replen))
        return JS_FALSE;
    growth = leftlen + replen;
    chars = (jschar *)
        (rdata->chars
         ? JS_realloc(cx, rdata->chars, (rdata->length + growth + 1)
                                        * sizeof(jschar))
         : JS_malloc(cx, (growth + 1) * sizeof(jschar)));
    if (!chars)
        return JS_FALSE;
    rdata->chars = chars;
    rdata->length += growth;
    chars += rdata->index;
    rdata->index += growth;
    js_strncpy(chars, left, leftlen);
    chars += leftlen;
    do_replace(cx, rdata, chars);
    return JS_TRUE;
}

static JSBool
str_replace(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *lambda;
    JSString *repstr, *str;
    ReplaceData rdata;
    JSBool ok;
    jschar *chars;
    size_t leftlen, rightlen, length;

    if (JS_TypeOfValue(cx, vp[3]) == JSTYPE_FUNCTION) {
        lambda = JSVAL_TO_OBJECT(vp[3]);
        repstr = NULL;
    } else {
        if (!JS_ConvertValue(cx, vp[3], JSTYPE_STRING, &vp[3]))
            return JS_FALSE;
        repstr = JSVAL_TO_STRING(vp[3]);
        lambda = NULL;
    }

    




    rdata.base.flags = MODE_REPLACE | KEEP_REGEXP | FORCE_FLAT;
    rdata.base.optarg = 2;

    rdata.lambda = lambda;
    rdata.repstr = repstr;
    if (repstr) {
        rdata.dollarEnd = JSSTRING_CHARS(repstr) + JSSTRING_LENGTH(repstr);
        rdata.dollar = js_strchr_limit(JSSTRING_CHARS(repstr), '$',
                                       rdata.dollarEnd);
    } else {
        rdata.dollar = rdata.dollarEnd = NULL;
    }
    rdata.chars = NULL;
    rdata.length = 0;
    rdata.index = 0;
    rdata.leftIndex = 0;

    ok = match_or_replace(cx, replace_glob, replace_destroy, &rdata.base,
                          argc, vp);
    if (!ok)
        return JS_FALSE;

    if (!rdata.chars) {
        if ((rdata.base.flags & GLOBAL_REGEXP) || *vp != JSVAL_TRUE) {
            
            *vp = STRING_TO_JSVAL(rdata.base.str);
            goto out;
        }
        leftlen = cx->regExpStatics.leftContext.length;
        ok = find_replen(cx, &rdata, &length);
        if (!ok)
            goto out;
        length += leftlen;
        chars = (jschar *) JS_malloc(cx, (length + 1) * sizeof(jschar));
        if (!chars) {
            ok = JS_FALSE;
            goto out;
        }
        js_strncpy(chars, cx->regExpStatics.leftContext.chars, leftlen);
        do_replace(cx, &rdata, chars + leftlen);
        rdata.chars = chars;
        rdata.length = length;
    }

    rightlen = cx->regExpStatics.rightContext.length;
    length = rdata.length + rightlen;
    chars = (jschar *)
        JS_realloc(cx, rdata.chars, (length + 1) * sizeof(jschar));
    if (!chars) {
        JS_free(cx, rdata.chars);
        ok = JS_FALSE;
        goto out;
    }
    js_strncpy(chars + rdata.length, cx->regExpStatics.rightContext.chars,
               rightlen);
    chars[length] = 0;

    str = js_NewString(cx, chars, length, 0);
    if (!str) {
        JS_free(cx, chars);
        ok = JS_FALSE;
        goto out;
    }
    *vp = STRING_TO_JSVAL(str);

out:
    
    if (rdata.base.flags & KEEP_REGEXP)
        js_DestroyRegExp(cx, rdata.base.regexp);
    return ok;
}










static jsint
find_split(JSContext *cx, JSString *str, JSRegExp *re, jsint *ip,
           JSSubString *sep)
{
    jsint i, j, k;
    size_t length;
    jschar *chars;

    










    i = *ip;
    length = JSSTRING_LENGTH(str);
    if ((size_t)i > length)
        return -1;

    chars = JSSTRING_CHARS(str);

    




    if (re) {
        size_t index;
        jsval rval;

      again:
        
        index = (size_t)i;
        if (!js_ExecuteRegExp(cx, re, str, &index, JS_TRUE, &rval))
            return -2;
        if (rval != JSVAL_TRUE) {
            
            sep->length = 1;
            return length;
        }
        i = (jsint)index;
        *sep = cx->regExpStatics.lastMatch;
        if (sep->length == 0) {
            




            if (i == *ip) {
                




                if ((size_t)i == length)
                    return -1;
                i++;
                goto again;
            }
            if ((size_t)i == length) {
                




                sep->chars = NULL;
            }
        }
        JS_ASSERT((size_t)i >= sep->length);
        return i - sep->length;
    }

    




    if (sep->length == 0)
        return ((size_t)i == length) ? -1 : i + 1;

    




    j = 0;
    while ((size_t)(k = i + j) < length) {
        if (chars[k] == sep->chars[j]) {
            if ((size_t)++j == sep->length)
                return i;
        } else {
            i++;
            j = 0;
        }
    }
    return k;
}

static JSBool
str_split(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str, *sub;
    JSObject *arrayobj;
    jsval v;
    JSBool ok, limited;
    JSRegExp *re;
    JSSubString *sep, tmp;
    jsdouble d;
    jsint i, j;
    uint32 len, limit;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    arrayobj = js_ConstructObject(cx, &js_ArrayClass, NULL, NULL, 0, NULL);
    if (!arrayobj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(arrayobj);

    if (argc == 0) {
        v = STRING_TO_JSVAL(str);
        ok = JS_SetElement(cx, arrayobj, 0, &v);
    } else {
        if (JSVAL_IS_REGEXP(cx, vp[2])) {
            re = (JSRegExp *) JS_GetPrivate(cx, JSVAL_TO_OBJECT(vp[2]));
            sep = &tmp;

            
            sep->chars = NULL;
            sep->length = 0;
        } else {
            JSString *str2 = js_ValueToString(cx, vp[2]);
            if (!str2)
                return JS_FALSE;
            vp[2] = STRING_TO_JSVAL(str2);

            



            tmp.length = JSSTRING_LENGTH(str2);
            tmp.chars = JSSTRING_CHARS(str2);
            sep = &tmp;
            re = NULL;
        }

        
        limited = (argc > 1) && !JSVAL_IS_VOID(vp[3]);
        limit = 0; 
        if (limited) {
            if (!js_ValueToNumber(cx, vp[3], &d))
                return JS_FALSE;

            
            if (!js_DoubleToECMAUint32(cx, d, &limit))
                return JS_FALSE;
            if (limit > JSSTRING_LENGTH(str))
                limit = 1 + JSSTRING_LENGTH(str);
        }

        len = i = 0;
        while ((j = find_split(cx, str, re, &i, sep)) >= 0) {
            if (limited && len >= limit)
                break;
            sub = js_NewDependentString(cx, str, i, (size_t)(j - i));
            if (!sub)
                return JS_FALSE;
            v = STRING_TO_JSVAL(sub);
            if (!JS_SetElement(cx, arrayobj, len, &v))
                return JS_FALSE;
            len++;

            




            if (re && sep->chars) {
                uintN num;
                JSSubString *parsub;

                for (num = 0; num < cx->regExpStatics.parenCount; num++) {
                    if (limited && len >= limit)
                        break;
                    parsub = REGEXP_PAREN_SUBSTRING(&cx->regExpStatics, num);
                    sub = js_NewStringCopyN(cx, parsub->chars, parsub->length);
                    if (!sub)
                        return JS_FALSE;
                    v = STRING_TO_JSVAL(sub);
                    if (!JS_SetElement(cx, arrayobj, len, &v))
                        return JS_FALSE;
                    len++;
                }
                sep->chars = NULL;
            }
            i = j + sep->length;
        }
        ok = (j != -2);
    }
    return ok;
}

#if JS_HAS_PERL_SUBSTR
static JSBool
str_substr(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    jsdouble d;
    jsdouble length, begin, end;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    if (argc != 0) {
        if (!js_ValueToNumber(cx, vp[2], &d))
            return JS_FALSE;
        length = JSSTRING_LENGTH(str);
        begin = js_DoubleToInteger(d);
        if (begin < 0) {
            begin += length;
            if (begin < 0)
                begin = 0;
        } else if (begin > length) {
            begin = length;
        }

        if (argc == 1) {
            end = length;
        } else {
            if (!js_ValueToNumber(cx, vp[3], &d))
                return JS_FALSE;
            end = js_DoubleToInteger(d);
            if (end < 0)
                end = 0;
            end += begin;
            if (end > length)
                end = length;
        }

        str = js_NewDependentString(cx, str,
                                    (size_t)begin,
                                    (size_t)(end - begin));
        if (!str)
            return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}
#endif 




static JSBool
str_concat(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str, *str2;
    jsval *argv;
    uintN i;

    str = js_ValueToString(cx, vp[1]);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    for (i = 0, argv = vp + 2; i < argc; i++) {
        str2 = js_ValueToString(cx, argv[i]);
        if (!str2)
            return JS_FALSE;
        argv[i] = STRING_TO_JSVAL(str2);

        str = js_ConcatStrings(cx, str, str2);
        if (!str)
            return JS_FALSE;
    }

    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
str_slice(JSContext *cx, uintN argc, jsval *vp)
{
    jsval t, v;
    JSString *str;
    jschar c;

    t = vp[1];
    v = vp[2];
    if (argc == 1 && JSVAL_IS_STRING(t) && JSVAL_IS_INT(v)) {
        size_t begin, end, length;

        str = JSVAL_TO_STRING(t);
        begin = JSVAL_TO_INT(v);
        end = JSSTRING_LENGTH(str);
        if (begin <= end) {
            length = end - begin;
            if (length == 0) {
                str = cx->runtime->emptyString;
            } else {
                str = (length == 1 &&
                       (c = JSSTRING_CHARS(str)[begin]) < UNIT_STRING_LIMIT)
                      ? js_GetUnitString(cx, c)
                      : js_NewDependentString(cx, str, begin, length);
                if (!str)
                    return JS_FALSE;
            }
            *vp = STRING_TO_JSVAL(str);
            return JS_TRUE;
        }
    }

    str = js_ValueToString(cx, t);
    if (!str)
        return JS_FALSE;
    vp[1] = STRING_TO_JSVAL(str);

    if (argc != 0) {
        double begin, end, length;

        if (!js_ValueToNumber(cx, v, &begin))
            return JS_FALSE;
        begin = js_DoubleToInteger(begin);
        length = JSSTRING_LENGTH(str);
        if (begin < 0) {
            begin += length;
            if (begin < 0)
                begin = 0;
        } else if (begin > length) {
            begin = length;
        }

        if (argc == 1) {
            end = length;
        } else {
            if (!js_ValueToNumber(cx, vp[3], &end))
                return JS_FALSE;
            end = js_DoubleToInteger(end);
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
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

#if JS_HAS_STR_HTML_HELPERS



static JSBool
tagify(JSContext *cx, const char *begin, JSString *param, const char *end,
       jsval *vp)
{
    jsval v;
    JSString *str;
    jschar *tagbuf;
    size_t beglen, endlen, parlen, taglen;
    size_t i, j;

    v = vp[1];
    if (!JSVAL_IS_OBJECT(v)) {
        JS_ASSERT(JSVAL_IS_STRING(v));
        str = JSVAL_TO_STRING(v);
    } else {
        str = js_ValueToString(cx, v);
        if (!str)
            return JS_FALSE;
        vp[1] = STRING_TO_JSVAL(str);
    }

    if (!end)
        end = begin;

    beglen = strlen(begin);
    taglen = 1 + beglen + 1;                            
    parlen = 0; 
    if (param) {
        parlen = JSSTRING_LENGTH(param);
        taglen += 2 + parlen + 1;                       
    }
    endlen = strlen(end);
    taglen += JSSTRING_LENGTH(str) + 2 + endlen + 1;    

    if (taglen >= ~(size_t)0 / sizeof(jschar)) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    tagbuf = (jschar *) JS_malloc(cx, (taglen + 1) * sizeof(jschar));
    if (!tagbuf)
        return JS_FALSE;

    j = 0;
    tagbuf[j++] = '<';
    for (i = 0; i < beglen; i++)
        tagbuf[j++] = (jschar)begin[i];
    if (param) {
        tagbuf[j++] = '=';
        tagbuf[j++] = '"';
        js_strncpy(&tagbuf[j], JSSTRING_CHARS(param), parlen);
        j += parlen;
        tagbuf[j++] = '"';
    }
    tagbuf[j++] = '>';
    js_strncpy(&tagbuf[j], JSSTRING_CHARS(str), JSSTRING_LENGTH(str));
    j += JSSTRING_LENGTH(str);
    tagbuf[j++] = '<';
    tagbuf[j++] = '/';
    for (i = 0; i < endlen; i++)
        tagbuf[j++] = (jschar)end[i];
    tagbuf[j++] = '>';
    JS_ASSERT(j == taglen);
    tagbuf[j] = 0;

    str = js_NewString(cx, tagbuf, taglen, 0);
    if (!str) {
        free((char *)tagbuf);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSBool
tagify_value(JSContext *cx, const char *begin, const char *end, jsval *vp)
{
    JSString *param;

    param = js_ValueToString(cx, vp[2]);
    if (!param)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(param);
    return tagify(cx, begin, param, end, vp);
}

static JSBool
str_bold(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "b", NULL, NULL, vp);
}

static JSBool
str_italics(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "i", NULL, NULL, vp);
}

static JSBool
str_fixed(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "tt", NULL, NULL, vp);
}

static JSBool
str_fontsize(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify_value(cx, "font size", "font", vp);
}

static JSBool
str_fontcolor(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify_value(cx, "font color", "font", vp);
}

static JSBool
str_link(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify_value(cx, "a href", "a", vp);
}

static JSBool
str_anchor(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify_value(cx, "a name", "a", vp);
}

static JSBool
str_strike(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "strike", NULL, NULL, vp);
}

static JSBool
str_small(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "small", NULL, NULL, vp);
}

static JSBool
str_big(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "big", NULL, NULL, vp);
}

static JSBool
str_blink(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "blink", NULL, NULL, vp);
}

static JSBool
str_sup(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "sup", NULL, NULL, vp);
}

static JSBool
str_sub(JSContext *cx, uintN argc, jsval *vp)
{
    return tagify(cx, "sub", NULL, NULL, vp);
}
#endif 

#define GENERIC           JSFUN_GENERIC_NATIVE
#define PRIMITIVE         JSFUN_THISP_PRIMITIVE
#define GENERIC_PRIMITIVE (GENERIC | PRIMITIVE)

static JSFunctionSpec string_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN("quote",             str_quote,             0,0,GENERIC_PRIMITIVE,0),
    JS_FN(js_toSource_str,     str_toSource,          0,0,JSFUN_THISP_STRING,0),
#endif

    
    JS_FN(js_toString_str,     str_toString,          0,0,JSFUN_THISP_STRING,0),
    JS_FN(js_valueOf_str,      str_toString,          0,0,JSFUN_THISP_STRING,0),
    JS_FN("substring",         str_substring,         0,2,GENERIC_PRIMITIVE,0),
    JS_FN("toLowerCase",       str_toLowerCase,       0,0,GENERIC_PRIMITIVE,0),
    JS_FN("toUpperCase",       str_toUpperCase,       0,0,GENERIC_PRIMITIVE,0),
    JS_FN("charAt",            str_charAt,            1,1,GENERIC_PRIMITIVE,0),
    JS_FN("charCodeAt",        str_charCodeAt,        1,1,GENERIC_PRIMITIVE,0),
    JS_FN("indexOf",           str_indexOf,           1,1,GENERIC_PRIMITIVE,0),
    JS_FN("lastIndexOf",       str_lastIndexOf,       1,1,GENERIC_PRIMITIVE,0),
    JS_FN("toLocaleLowerCase", str_toLocaleLowerCase, 0,0,GENERIC_PRIMITIVE,0),
    JS_FN("toLocaleUpperCase", str_toLocaleUpperCase, 0,0,GENERIC_PRIMITIVE,0),
    JS_FN("localeCompare",     str_localeCompare,     1,1,GENERIC_PRIMITIVE,0),

    
    JS_FN("match",             str_match,             1,1,GENERIC_PRIMITIVE,2),
    JS_FN("search",            str_search,            1,1,GENERIC_PRIMITIVE,0),
    JS_FN("replace",           str_replace,           2,2,GENERIC_PRIMITIVE,0),
    JS_FN("split",             str_split,             0,2,GENERIC_PRIMITIVE,0),
#if JS_HAS_PERL_SUBSTR
    JS_FN("substr",            str_substr,            0,2,GENERIC_PRIMITIVE,0),
#endif

    
    JS_FN("concat",            str_concat,            0,1,GENERIC_PRIMITIVE,0),
    JS_FN("slice",             str_slice,             1,2,GENERIC_PRIMITIVE,0),

    
#if JS_HAS_STR_HTML_HELPERS
    JS_FN("bold",              str_bold,              0,0,PRIMITIVE,0),
    JS_FN("italics",           str_italics,           0,0,PRIMITIVE,0),
    JS_FN("fixed",             str_fixed,             0,0,PRIMITIVE,0),
    JS_FN("fontsize",          str_fontsize,          1,1,PRIMITIVE,0),
    JS_FN("fontcolor",         str_fontcolor,         1,1,PRIMITIVE,0),
    JS_FN("link",              str_link,              1,1,PRIMITIVE,0),
    JS_FN("anchor",            str_anchor,            1,1,PRIMITIVE,0),
    JS_FN("strike",            str_strike,            0,0,PRIMITIVE,0),
    JS_FN("small",             str_small,             0,0,PRIMITIVE,0),
    JS_FN("big",               str_big,               0,0,PRIMITIVE,0),
    JS_FN("blink",             str_blink,             0,0,PRIMITIVE,0),
    JS_FN("sup",               str_sup,               0,0,PRIMITIVE,0),
    JS_FN("sub",               str_sub,               0,0,PRIMITIVE,0),
#endif

    JS_FS_END
};

static JSBool
String(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    JSString *str;

    if (argc > 0) {
        str = js_ValueToString(cx, argv[0]);
        if (!str)
            return JS_FALSE;
        argv[0] = STRING_TO_JSVAL(str);
    } else {
        str = cx->runtime->emptyString;
    }
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        *rval = STRING_TO_JSVAL(str);
        return JS_TRUE;
    }
    OBJ_SET_SLOT(cx, obj, JSSLOT_PRIVATE, STRING_TO_JSVAL(str));
    return JS_TRUE;
}

static JSBool
str_fromCharCode(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    jschar *chars;
    uintN i;
    uint16 code;
    JSString *str;

    argv = vp + 2;
    JS_ASSERT(argc < ARRAY_INIT_LIMIT);
    chars = (jschar *) JS_malloc(cx, (argc + 1) * sizeof(jschar));
    if (!chars)
        return JS_FALSE;
    for (i = 0; i < argc; i++) {
        if (!js_ValueToUint16(cx, argv[i], &code)) {
            JS_free(cx, chars);
            return JS_FALSE;
        }
        chars[i] = (jschar)code;
    }
    chars[i] = 0;
    str = js_NewString(cx, chars, argc, 0);
    if (!str) {
        JS_free(cx, chars);
        return JS_FALSE;
    }
    *vp = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

static JSFunctionSpec string_static_methods[] = {
    JS_FN("fromCharCode",    str_fromCharCode,       0,1,0,0),
    JS_FS_END
};

JSBool
js_InitRuntimeStringState(JSContext *cx)
{
    JSRuntime *rt;

    rt = cx->runtime;

    
#ifdef JS_THREADSAFE
    JS_ASSERT(!rt->deflatedStringCacheLock);
    rt->deflatedStringCacheLock = JS_NEW_LOCK();
    if (!rt->deflatedStringCacheLock)
        return JS_FALSE;
#endif

    rt->emptyString = ATOM_TO_STRING(rt->atomState.emptyAtom);
    return JS_TRUE;
}

#define UNIT_STRING_SPACE(sp)    ((jschar *) ((sp) + UNIT_STRING_LIMIT))
#define UNIT_STRING_SPACE_RT(rt) UNIT_STRING_SPACE((rt)->unitStrings)

#define IN_UNIT_STRING_SPACE(sp,cp)                                           \
    ((cp) - UNIT_STRING_SPACE(sp) < 2 * UNIT_STRING_LIMIT)
#define IN_UNIT_STRING_SPACE_RT(rt,cp)                                        \
    IN_UNIT_STRING_SPACE((rt)->unitStrings, cp)

JSString *
js_GetUnitString(JSContext *cx, jschar c)
{
#if 1
    JSRuntime *rt;
    JSString **sp, *str;
    jschar *cp, i;

    JS_ASSERT(c < UNIT_STRING_LIMIT);
    rt = cx->runtime;
    if (!rt->unitStrings) {
        sp = (JSString **) calloc(UNIT_STRING_LIMIT * sizeof(JSString *) +
                                  UNIT_STRING_LIMIT * 2 * sizeof(jschar),
                                  1);
        if (!sp) {
            JS_ReportOutOfMemory(cx);
            return NULL;
        }
        cp = UNIT_STRING_SPACE(sp);
        for (i = 0; i < UNIT_STRING_LIMIT; i++) {
            *cp = i;
            cp += 2;
        }
        JS_LOCK_GC(rt);
        if (!rt->unitStrings) {
            rt->unitStrings = sp;
            JS_UNLOCK_GC(rt);
        } else {
            JS_UNLOCK_GC(rt);
            free(sp);
        }
    }
    if (!rt->unitStrings[c]) {
        cp = UNIT_STRING_SPACE_RT(rt);
        str = js_NewString(cx, cp + 2 * c, 1, GCF_LOCK);
        if (!str)
            return NULL;
        JS_LOCK_GC(rt);
        if (!rt->unitStrings[c]) {
            rt->unitStrings[c] = str;
            JS_UNLOCK_GC(rt);
        } else {
            JS_UNLOCK_GC(rt);
            js_UnlockGCThingRT(rt, str);
        }
    }
    return rt->unitStrings[c];
#else
    return js_NewStringCopyN(cx, &c, 1);
#endif
}

void
js_FinishRuntimeStringState(JSContext *cx)
{
    JSRuntime *rt = cx->runtime;

    rt->emptyString = NULL;

    if (rt->unitStrings) {
        jschar c;

        for (c = 0; c < UNIT_STRING_LIMIT; c++) {
            if (rt->unitStrings[c])
                js_UnlockGCThingRT(rt, rt->unitStrings[c]);
        }
    }
}

void
js_FinishDeflatedStringCache(JSRuntime *rt)
{
    if (rt->deflatedStringCache) {
        JS_HashTableDestroy(rt->deflatedStringCache);
        rt->deflatedStringCache = NULL;
    }
#ifdef JS_THREADSAFE
    if (rt->deflatedStringCacheLock) {
        JS_DESTROY_LOCK(rt->deflatedStringCacheLock);
        rt->deflatedStringCacheLock = NULL;
    }
#endif
}

JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    
    if (!JS_DefineFunctions(cx, obj, string_functions))
        return NULL;

    proto = JS_InitClass(cx, obj, NULL, &js_StringClass, String, 1,
                         string_props, string_methods,
                         NULL, string_static_methods);
    if (!proto)
        return NULL;
    OBJ_SET_SLOT(cx, proto, JSSLOT_PRIVATE,
                 STRING_TO_JSVAL(cx->runtime->emptyString));
    return proto;
}

JSString *
js_NewString(JSContext *cx, jschar *chars, size_t length, uintN gcflag)
{
    JSString *str;

    if (length > JSSTRING_LENGTH_MASK) {
        JS_ReportOutOfMemory(cx);
        return NULL;
    }

    str = (JSString *) js_NewGCThing(cx, gcflag | GCX_STRING, sizeof(JSString));
    if (!str)
        return NULL;
    str->length = length;
    str->chars = chars;
#ifdef DEBUG
  {
    JSRuntime *rt = cx->runtime;
    JS_RUNTIME_METER(rt, liveStrings);
    JS_RUNTIME_METER(rt, totalStrings);
    JS_LOCK_RUNTIME_VOID(rt,
        (rt->lengthSum += (double)length,
         rt->lengthSquaredSum += (double)length * (double)length));
  }
#endif
    return str;
}

JSString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start,
                      size_t length)
{
    JSDependentString *ds;

    if (length == 0)
        return cx->runtime->emptyString;

    if (start == 0 && length == JSSTRING_LENGTH(base))
        return base;

    if (start > JSSTRDEP_START_MASK ||
        (start != 0 && length > JSSTRDEP_LENGTH_MASK)) {
        return js_NewStringCopyN(cx, JSSTRING_CHARS(base) + start, length);
    }

    ds = (JSDependentString *)
         js_NewGCThing(cx, GCX_MUTABLE_STRING, sizeof(JSString));
    if (!ds)
        return NULL;
    if (start == 0) {
        JSPREFIX_SET_LENGTH(ds, length);
        JSPREFIX_SET_BASE(ds, base);
    } else {
        JSSTRDEP_SET_START_AND_LENGTH(ds, start, length);
        JSSTRDEP_SET_BASE(ds, base);
    }
#ifdef DEBUG
  {
    JSRuntime *rt = cx->runtime;
    JS_RUNTIME_METER(rt, liveDependentStrings);
    JS_RUNTIME_METER(rt, totalDependentStrings);
    JS_RUNTIME_METER(rt, liveStrings);
    JS_RUNTIME_METER(rt, totalStrings);
    JS_LOCK_RUNTIME_VOID(rt,
        (rt->strdepLengthSum += (double)length,
         rt->strdepLengthSquaredSum += (double)length * (double)length));
    JS_LOCK_RUNTIME_VOID(rt,
        (rt->lengthSum += (double)length,
         rt->lengthSquaredSum += (double)length * (double)length));
  }
#endif
    return (JSString *)ds;
}

#ifdef DEBUG
#include <math.h>

void printJSStringStats(JSRuntime *rt) {
    double mean = 0., var = 0., sigma = 0.;
    jsrefcount count = rt->totalStrings;
    if (count > 0 && rt->lengthSum >= 0) {
        mean = rt->lengthSum / count;
        var = count * rt->lengthSquaredSum - rt->lengthSum * rt->lengthSum;
        if (var < 0.0 || count <= 1)
            var = 0.0;
        else
            var /= count * (count - 1);

        
        sigma = (var != 0.) ? sqrt(var) : 0.;
    }
    fprintf(stderr, "%lu total strings, mean length %g (sigma %g)\n",
            (unsigned long)count, mean, sigma);

    mean = var = sigma = 0.;
    count = rt->totalDependentStrings;
    if (count > 0 && rt->strdepLengthSum >= 0) {
        mean = rt->strdepLengthSum / count;
        var = count * rt->strdepLengthSquaredSum
            - rt->strdepLengthSum * rt->strdepLengthSum;
        if (var < 0.0 || count <= 1)
            var = 0.0;
        else
            var /= count * (count - 1);

        
        sigma = (var != 0.) ? sqrt(var) : 0.;
    }
    fprintf(stderr, "%lu total dependent strings, mean length %g (sigma %g)\n",
            (unsigned long)count, mean, sigma);
}
#endif

JSString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n)
{
    jschar *news;
    JSString *str;

    news = (jschar *) JS_malloc(cx, (n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    js_strncpy(news, s, n);
    news[n] = 0;
    str = js_NewString(cx, news, n, 0);
    if (!str)
        JS_free(cx, news);
    return str;
}

JSString *
js_NewStringCopyZ(JSContext *cx, const jschar *s)
{
    size_t n, m;
    jschar *news;
    JSString *str;

    n = js_strlen(s);
    m = (n + 1) * sizeof(jschar);
    news = (jschar *) JS_malloc(cx, m);
    if (!news)
        return NULL;
    memcpy(news, s, m);
    str = js_NewString(cx, news, n, 0);
    if (!str)
        JS_free(cx, news);
    return str;
}

JS_STATIC_DLL_CALLBACK(JSHashNumber)
js_hash_string_pointer(const void *key)
{
    return (JSHashNumber)JS_PTR_TO_UINT32(key) >> JSVAL_TAGBITS;
}

void
js_PurgeDeflatedStringCache(JSRuntime *rt, JSString *str)
{
    JSHashNumber hash;
    JSHashEntry *he, **hep;

    if (!rt->deflatedStringCache)
        return;

    hash = js_hash_string_pointer(str);
    JS_ACQUIRE_LOCK(rt->deflatedStringCacheLock);
    hep = JS_HashTableRawLookup(rt->deflatedStringCache, hash, str);
    he = *hep;
    if (he) {
#ifdef DEBUG
        rt->deflatedStringCacheBytes -= JSSTRING_LENGTH(str);
#endif
        free(he->value);
        JS_HashTableRawRemove(rt->deflatedStringCache, hep, he);
    }
    JS_RELEASE_LOCK(rt->deflatedStringCacheLock);
}

void
js_FinalizeString(JSContext *cx, JSString *str)
{
    js_FinalizeStringRT(cx->runtime, str);
}

void
js_FinalizeStringRT(JSRuntime *rt, JSString *str)
{
    JSBool valid;

    JS_RUNTIME_UNMETER(rt, liveStrings);
    if (JSSTRING_IS_DEPENDENT(str)) {
        
        JS_ASSERT(JSSTRDEP_BASE(str));
        JS_RUNTIME_UNMETER(rt, liveDependentStrings);
        valid = JS_TRUE;
    } else {
        
        valid = (str->chars != NULL);
        if (valid && !IN_UNIT_STRING_SPACE_RT(rt, str->chars))
            free(str->chars);
    }
    if (valid) {
        js_PurgeDeflatedStringCache(rt, str);
        str->chars = NULL;
    }
    str->length = 0;
}

JS_FRIEND_API(const char *)
js_ValueToPrintable(JSContext *cx, jsval v, JSValueToStringFun v2sfun)
{
    JSString *str;

    str = v2sfun(cx, v);
    if (!str)
        return NULL;
    str = js_QuoteString(cx, str, 0);
    if (!str)
        return NULL;
    return js_GetStringBytes(cx, str);
}

JS_FRIEND_API(JSString *)
js_ValueToString(JSContext *cx, jsval v)
{
    JSObject *obj;
    JSString *str;

    if (JSVAL_IS_OBJECT(v)) {
        obj = JSVAL_TO_OBJECT(v);
        if (!obj)
            return ATOM_TO_STRING(cx->runtime->atomState.nullAtom);
        if (!OBJ_DEFAULT_VALUE(cx, obj, JSTYPE_STRING, &v))
            return NULL;
    }
    if (JSVAL_IS_STRING(v)) {
        str = JSVAL_TO_STRING(v);
    } else if (JSVAL_IS_INT(v)) {
        str = js_NumberToString(cx, JSVAL_TO_INT(v));
    } else if (JSVAL_IS_DOUBLE(v)) {
        str = js_NumberToString(cx, *JSVAL_TO_DOUBLE(v));
    } else if (JSVAL_IS_BOOLEAN(v)) {
        str = js_BooleanToString(cx, JSVAL_TO_BOOLEAN(v));
    } else {
        str = ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    }
    return str;
}

JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, jsval v)
{
    JSTempValueRooter tvr;
    JSString *str;

    if (JSVAL_IS_VOID(v))
        return ATOM_TO_STRING(cx->runtime->atomState.void0Atom);
    if (JSVAL_IS_STRING(v))
        return js_QuoteString(cx, JSVAL_TO_STRING(v), '"');
    if (JSVAL_IS_PRIMITIVE(v)) {
        
        if (JSVAL_IS_DOUBLE(v) && JSDOUBLE_IS_NEGZERO(*JSVAL_TO_DOUBLE(v))) {
            
            static const jschar js_negzero_ucNstr[] = {'-', '0'};

            return js_NewStringCopyN(cx, js_negzero_ucNstr, 2);
        }
        return js_ValueToString(cx, v);
    }

    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_NULL, &tvr);
    if (!js_TryMethod(cx, JSVAL_TO_OBJECT(v),
                      cx->runtime->atomState.toSourceAtom,
                      0, NULL, &tvr.u.value)) {
        str = NULL;
    } else {
        str = js_ValueToString(cx, tvr.u.value);
    }
    JS_POP_TEMP_ROOT(cx, &tvr);
    return str;
}




uint32
js_HashString(JSString *str)
{
    uint32 h;
    const jschar *s;
    size_t n;

    h = 0;
    for (s = JSSTRING_CHARS(str), n = JSSTRING_LENGTH(str); n; s++, n--)
        h = (h >> (JS_HASH_BITS - 4)) ^ (h << 4) ^ *s;
    return h;
}




JSBool
js_EqualStrings(JSString *str1, JSString *str2)
{
    size_t n;
    const jschar *s1, *s2;

    JS_ASSERT(str1);
    JS_ASSERT(str2);

    
    if (str1 == str2)
        return JS_TRUE;

    n = JSSTRING_LENGTH(str1);
    if (n != JSSTRING_LENGTH(str2))
        return JS_FALSE;

    if (n == 0)
        return JS_TRUE;

    s1 = JSSTRING_CHARS(str1), s2 = JSSTRING_CHARS(str2);
    do {
        if (*s1 != *s2)
            return JS_FALSE;
        ++s1, ++s2;
    } while (--n != 0);

    return JS_TRUE;
}

intN
js_CompareStrings(JSString *str1, JSString *str2)
{
    size_t l1, l2, n, i;
    const jschar *s1, *s2;
    intN cmp;

    JS_ASSERT(str1);
    JS_ASSERT(str2);

    
    if (str1 == str2)
        return 0;

    l1 = JSSTRING_LENGTH(str1), l2 = JSSTRING_LENGTH(str2);
    s1 = JSSTRING_CHARS(str1),  s2 = JSSTRING_CHARS(str2);
    n = JS_MIN(l1, l2);
    for (i = 0; i < n; i++) {
        cmp = s1[i] - s2[i];
        if (cmp != 0)
            return cmp;
    }
    return (intN)(l1 - l2);
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

const jschar *
js_SkipWhiteSpace(const jschar *s, const jschar *end)
{
    JS_ASSERT(s <= end);
    while (s != end && JS_ISSPACE(*s))
        s++;
    return s;
}

#ifdef JS_C_STRINGS_ARE_UTF8

jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t *length)
{
    jschar *chars = NULL;
    size_t dstlen = 0;

    if (!js_InflateStringToBuffer(cx, bytes, *length, NULL, &dstlen))
        return NULL;
    chars = (jschar *) JS_malloc(cx, (dstlen + 1) * sizeof (jschar));
    if (!chars)
        return NULL;
    js_InflateStringToBuffer(cx, bytes, *length, chars, &dstlen);
    chars[dstlen] = 0;
    *length = dstlen;
    return chars;
}




char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length)
{
    size_t size;
    char *bytes;

    size = js_GetDeflatedStringLength(cx, chars, length);
    if (size == (size_t)-1)
        return NULL;
    bytes = (char *) (cx ? JS_malloc(cx, size+1) : malloc(size+1));
    if (!bytes)
        return NULL;
    js_DeflateStringToBuffer(cx, chars, length, bytes, &size);
    bytes[size] = 0;
    return bytes;
}




size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars,
                           size_t charsLength)
{
    const jschar *end;
    size_t length;
    uintN c, c2;
    char buffer[10];

    length = charsLength;
    for (end = chars + length; chars != end; chars++) {
        c = *chars;
        if (c < 0x80)
            continue;
        if (0xD800 <= c && c <= 0xDFFF) {
            
            chars++;
            if (c >= 0xDC00 || chars == end)
                goto bad_surrogate;
            c2 = *chars;
            if (c2 < 0xDC00 || c2 > 0xDFFF)
                goto bad_surrogate;
            c = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
        }
        c >>= 11;
        length++;
        while (c) {
            c >>= 5;
            length++;
        }
    }
    return length;

  bad_surrogate:
    if (cx) {
        JS_snprintf(buffer, 10, "0x%x", c);
        JS_ReportErrorFlagsAndNumber(cx, JSREPORT_ERROR, js_GetErrorMessage,
                                     NULL, JSMSG_BAD_SURROGATE_CHAR, buffer);
    }
    return (size_t)-1;
}

JSBool
js_DeflateStringToBuffer(JSContext *cx, const jschar *src, size_t srclen,
                         char *dst, size_t *dstlenp)
{
    size_t i, utf8Len, dstlen = *dstlenp, origDstlen = dstlen;
    jschar c, c2;
    uint32 v;
    uint8 utf8buf[6];

    while (srclen) {
        c = *src++;
        srclen--;
        if ((c >= 0xDC00) && (c <= 0xDFFF))
            goto badSurrogate;
        if (c < 0xD800 || c > 0xDBFF) {
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
        js_GetDeflatedStringLength(cx, src - 1, srclen + 1);
    return JS_FALSE;

bufferTooSmall:
    *dstlenp = (origDstlen - dstlen);
    if (cx) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BUFFER_TOO_SMALL);
    }
    return JS_FALSE;
}

JSBool
js_InflateStringToBuffer(JSContext *cx, const char *src, size_t srclen,
                         jschar *dst, size_t *dstlenp)
{
    uint32 v;
    size_t offset = 0, j, n, dstlen = *dstlenp, origDstlen = dstlen;

    if (!dst)
        dstlen = origDstlen = (size_t) -1;

    while (srclen) {
        v = (uint8) *src;
        n = 1;
        if (v & 0x80) {
            while (v & (0x80 >> n))
                n++;
            if (n > srclen)
                goto bufferTooSmall;
            if (n == 1 || n > 6)
                goto badCharacter;
            for (j = 1; j < n; j++) {
                if ((src[j] & 0xC0) != 0x80)
                    goto badCharacter;
            }
            v = Utf8ToOneUcs4Char((uint8 *)src, n);
            if (v >= 0x10000) {
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
                if (dstlen < 2)
                    goto bufferTooSmall;
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

#else 

JSBool
js_InflateStringToBuffer(JSContext* cx, const char *bytes, size_t length,
                         jschar *chars, size_t* charsLength)
{
    size_t i;

    if (length > *charsLength) {
        for (i = 0; i < *charsLength; i++)
            chars[i] = (unsigned char) bytes[i];
        if (cx) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BUFFER_TOO_SMALL);
        }
        return JS_FALSE;
    }
    for (i = 0; i < length; i++)
        chars[i] = (unsigned char) bytes[i];
    *charsLength = length;
    return JS_TRUE;
}

jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t *bytesLength)
{
    jschar *chars;
    size_t i, length = *bytesLength;

    chars = (jschar *) JS_malloc(cx, (length + 1) * sizeof(jschar));
    if (!chars) {
        *bytesLength = 0;
        return NULL;
    }
    for (i = 0; i < length; i++)
        chars[i] = (unsigned char) bytes[i];
    chars[length] = 0;
    *bytesLength = length;
    return chars;
}

size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars,
                           size_t charsLength)
{
    return charsLength;
}

JSBool
js_DeflateStringToBuffer(JSContext* cx, const jschar *chars, size_t length,
                         char *bytes, size_t* bytesLength)
{
    size_t i;

    if (length > *bytesLength) {
        for (i = 0; i < *bytesLength; i++)
            bytes[i] = (char) chars[i];
        if (cx) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BUFFER_TOO_SMALL);
        }
        return JS_FALSE;
    }
    for (i = 0; i < length; i++)
        bytes[i] = (char) chars[i];
    *bytesLength = length;
    return JS_TRUE;
}




char *
js_DeflateString(JSContext *cx, const jschar *chars, size_t length)
{
    size_t i, size;
    char *bytes;

    size = (length + 1) * sizeof(char);
    bytes = (char *) (cx ? JS_malloc(cx, size) : malloc(size));
    if (!bytes)
        return NULL;

    for (i = 0; i < length; i++)
        bytes[i] = (char) chars[i];

    bytes[length] = 0;
    return bytes;
}

#endif

static JSHashTable *
GetDeflatedStringCache(JSRuntime *rt)
{
    JSHashTable *cache;

    cache = rt->deflatedStringCache;
    if (!cache) {
        cache = JS_NewHashTable(8, js_hash_string_pointer,
                                JS_CompareValues, JS_CompareValues,
                                NULL, NULL);
        rt->deflatedStringCache = cache;
    }
    return cache;
}

JSBool
js_SetStringBytes(JSContext *cx, JSString *str, char *bytes, size_t length)
{
    JSRuntime *rt;
    JSHashTable *cache;
    JSBool ok;
    JSHashNumber hash;
    JSHashEntry **hep;

    rt = cx->runtime;
    JS_ACQUIRE_LOCK(rt->deflatedStringCacheLock);

    cache = GetDeflatedStringCache(rt);
    if (!cache) {
        js_ReportOutOfMemory(cx);
        ok = JS_FALSE;
    } else {
        hash = js_hash_string_pointer(str);
        hep = JS_HashTableRawLookup(cache, hash, str);
        JS_ASSERT(*hep == NULL);
        ok = JS_HashTableRawAdd(cache, hep, hash, str, bytes) != NULL;
#ifdef DEBUG
        if (ok)
            rt->deflatedStringCacheBytes += length;
#endif
    }

    JS_RELEASE_LOCK(rt->deflatedStringCacheLock);
    return ok;
}

const char *
js_GetStringBytes(JSContext *cx, JSString *str)
{
    JSRuntime *rt;
    JSHashTable *cache;
    char *bytes;
    JSHashNumber hash;
    JSHashEntry *he, **hep;

    if (cx) {
        rt = cx->runtime;
    } else {
        
        rt = js_GetGCStringRuntime(str);
    }

#ifdef JS_THREADSAFE
    if (!rt->deflatedStringCacheLock) {
        



        return js_DeflateString(NULL, JSSTRING_CHARS(str),
                                      JSSTRING_LENGTH(str));
    }
#endif

    JS_ACQUIRE_LOCK(rt->deflatedStringCacheLock);

    cache = GetDeflatedStringCache(rt);
    if (!cache) {
        if (cx)
            js_ReportOutOfMemory(cx);
        bytes = NULL;
    } else {
        hash = js_hash_string_pointer(str);
        hep = JS_HashTableRawLookup(cache, hash, str);
        he = *hep;
        if (he) {
            bytes = (char *) he->value;

#ifndef JS_C_STRINGS_ARE_UTF8
            
            JS_ASSERT_IF(*bytes != (char) JSSTRING_CHARS(str)[0],
                         *bytes == '\0' && JSSTRING_LENGTH(str) == 0);
#endif
        } else {
            bytes = js_DeflateString(cx, JSSTRING_CHARS(str),
                                     JSSTRING_LENGTH(str));
            if (bytes) {
                if (JS_HashTableRawAdd(cache, hep, hash, str, bytes)) {
#ifdef DEBUG
                    rt->deflatedStringCacheBytes += JSSTRING_LENGTH(str);
#endif
                } else {
                    if (cx)
                        JS_free(cx, bytes);
                    else
                        free(bytes);
                    bytes = NULL;
                }
            }
        }
    }

    JS_RELEASE_LOCK(rt->deflatedStringCacheLock);
    return bytes;
}
















































const uint8 js_X[] = {
  0,   1,   2,   3,   4,   5,   6,   7,  
  8,   9,  10,  11,  12,  13,  14,  15,  
 16,  17,  18,  19,  20,  21,  22,  23,  
 24,  25,  26,  27,  28,  28,  28,  28,  
 28,  28,  28,  28,  29,  30,  31,  32,  
 33,  34,  35,  36,  37,  38,  39,  40,  
 41,  42,  43,  44,  45,  46,  28,  28,  
 47,  48,  49,  50,  51,  52,  53,  28,  
 28,  28,  54,  55,  56,  57,  58,  59,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 60,  60,  61,  62,  63,  64,  65,  66,  
 67,  68,  69,  70,  71,  72,  73,  74,  
 75,  75,  75,  76,  77,  78,  28,  28,  
 79,  80,  81,  82,  83,  83,  84,  85,  
 86,  85,  28,  28,  87,  88,  89,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 90,  91,  92,  93,  94,  56,  95,  28,  
 96,  97,  98,  99,  83, 100,  83, 101,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56, 102,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 28,  28,  28,  28,  28,  28,  28,  28,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56,  56,  56,  
 56,  56,  56,  56,  56,  56, 103,  28,  
104, 104, 104, 104, 104, 104, 104, 104,  
104, 104, 104, 104, 104, 104, 104, 104,  
104, 104, 104, 104, 104, 104, 104, 104,  
104, 104, 104, 104, 104, 104, 104, 104,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105, 105, 105, 105, 105,  
105, 105, 105, 105,  56,  56,  56,  56,  
106,  28,  28,  28, 107, 108, 109, 110,  
 56,  56,  56,  56, 111, 112, 113, 114,  
115, 116,  56, 117, 118, 119, 120, 121   
};



const uint8 js_Y[] = {
  0,   0,   0,   0,   0,   0,   0,   0,  
  0,   1,   1,   1,   1,   1,   0,   0,  
  0,   0,   0,   0,   0,   0,   0,   0,  
  0,   0,   0,   0,   0,   0,   0,   0,  
  2,   3,   3,   3,   4,   3,   3,   3,  
  5,   6,   3,   7,   3,   8,   3,   3,  
  9,   9,   9,   9,   9,   9,   9,   9,  
  9,   9,   3,   3,   7,   7,   7,   3,  
  3,  10,  10,  10,  10,  10,  10,  10,  
 10,  10,  10,  10,  10,  10,  10,  10,  
 10,  10,  10,  10,  10,  10,  10,  10,  
 10,  10,  10,   5,   3,   6,  11,  12,  
 11,  13,  13,  13,  13,  13,  13,  13,  
 13,  13,  13,  13,  13,  13,  13,  13,  
 13,  13,  13,  13,  13,  13,  13,  13,  
 13,  13,  13,   5,   7,   6,   7,   0,  
  0,   0,   0,   0,   0,   0,   0,   0,  
  0,   0,   0,   0,   0,   0,   0,   0,  
  0,   0,   0,   0,   0,   0,   0,   0,  
  0,   0,   0,   0,   0,   0,   0,   0,  
  2,   3,   4,   4,   4,   4,  15,  15,  
 11,  15,  16,   5,   7,   8,  15,  11,  
 15,   7,  17,  17,  11,  16,  15,   3,  
 11,  18,  16,   6,  19,  19,  19,   3,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  20,  20,  20,   7,  
 20,  20,  20,  20,  20,  20,  20,  16,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  21,  21,  21,   7,  
 21,  21,  21,  21,  21,  21,  21,  22,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 25,  26,  23,  24,  23,  24,  23,  24,  
 16,  23,  24,  23,  24,  23,  24,  23,  
 24,  23,  24,  23,  24,  23,  24,  23,  
 24,  16,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 27,  23,  24,  23,  24,  23,  24,  28,  
 16,  29,  23,  24,  23,  24,  30,  23,  
 24,  31,  31,  23,  24,  16,  32,  32,  
 33,  23,  24,  31,  34,  16,  35,  36,  
 23,  24,  16,  16,  35,  37,  16,  38,  
 23,  24,  23,  24,  23,  24,  38,  23,  
 24,  39,  40,  16,  23,  24,  39,  23,  
 24,  41,  41,  23,  24,  23,  24,  42,  
 23,  24,  16,  40,  23,  24,  40,  40,  
 40,  40,  40,  40,  43,  44,  45,  43,  
 44,  45,  43,  44,  45,  23,  24,  23,  
 24,  23,  24,  23,  24,  23,  24,  23,  
 24,  23,  24,  23,  24,  16,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 16,  43,  44,  45,  23,  24,  46,  46,  
 46,  46,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 16,  16,  16,  47,  48,  16,  49,  49,  
 50,  50,  16,  51,  16,  16,  16,  16,  
 49,  16,  16,  52,  16,  16,  16,  16,  
 53,  54,  16,  16,  16,  16,  16,  54,  
 16,  16,  55,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  16,  16,  56,  16,  16,  16,  16,  
 56,  16,  57,  57,  16,  16,  16,  16,  
 16,  16,  58,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  46,  46,  46,  46,  46,  46,  46,  
 59,  59,  59,  59,  59,  59,  59,  59,  
 59,  11,  11,  59,  59,  59,  59,  59,  
 59,  59,  11,  11,  11,  11,  11,  11,  
 11,  11,  11,  11,  11,  11,  11,  11,  
 59,  59,  11,  11,  11,  11,  11,  11,  
 11,  11,  11,  11,  11,  11,  11,  46,  
 59,  59,  59,  59,  59,  11,  11,  11,  
 11,  11,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 60,  60,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,   3,   3,  46,  46,  
 46,  46,  59,  46,  46,  46,   3,  46,  
 46,  46,  46,  46,  11,  11,  61,   3,  
 62,  62,  62,  46,  63,  46,  64,  64,  
 16,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  46,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  65,  66,  66,  66,  
 16,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  16,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  67,  68,  68,  46,  
 69,  70,  38,  38,  38,  71,  72,  46,  
 46,  46,  38,  46,  38,  46,  38,  46,  
 38,  46,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 73,  74,  16,  40,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  75,  75,  75,  75,  75,  75,  75,  
 75,  75,  75,  75,  75,  46,  75,  75,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 20,  20,  20,  20,  20,  20,  20,  20,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 21,  21,  21,  21,  21,  21,  21,  21,  
 46,  74,  74,  74,  74,  74,  74,  74,  
 74,  74,  74,  74,  74,  46,  74,  74,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  15,  60,  60,  60,  60,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 40,  23,  24,  23,  24,  46,  46,  23,  
 24,  46,  46,  23,  24,  46,  46,  46,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  46,  46,  23,  24,  
 23,  24,  23,  24,  23,  24,  46,  46,  
 23,  24,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  46,  
 46,  59,   3,   3,   3,   3,   3,   3,  
 46,  77,  77,  77,  77,  77,  77,  77,  
 77,  77,  77,  77,  77,  77,  77,  77,  
 77,  77,  77,  77,  77,  77,  77,  77,  
 77,  77,  77,  77,  77,  77,  77,  77,  
 77,  77,  77,  77,  77,  77,  77,  16,  
 46,   3,  46,  46,  46,  46,  46,  46,  
 46,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  46,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  46,  60,  60,  60,   3,  60,  
  3,  60,  60,   3,  60,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  46,  46,  46,  46,  46,  
 40,  40,  40,   3,   3,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,   3,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,   3,  46,  46,  46,   3,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  46,  46,  46,  46,  46,  
 59,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  60,  60,  60,  60,  60,  
 60,  60,  60,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 78,  78,  78,  78,  78,  78,  78,  78,  
 78,  78,   3,   3,   3,   3,  46,  46,  
 60,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 46,  46,  40,  40,  40,  40,  40,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  46,  
 40,  40,  40,  40,   3,  40,  60,  60,  
 60,  60,  60,  60,  60,  79,  79,  60,  
 60,  60,  60,  60,  60,  59,  59,  60,  
 60,  15,  60,  60,  60,  60,  46,  46,  
  9,   9,   9,   9,   9,   9,   9,   9,  
  9,   9,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  60,  60,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  46,  46,  60,  40,  80,  80,  
 80,  60,  60,  60,  60,  60,  60,  60,  
 60,  80,  80,  80,  80,  60,  46,  46,  
 15,  60,  60,  60,  60,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  60,  60,   3,   3,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
  3,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  60,  80,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  46,  40,  
 40,  46,  46,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  46,  46,  46,  40,  40,  
 40,  40,  46,  46,  60,  46,  80,  80,  
 80,  60,  60,  60,  60,  46,  46,  80,  
 80,  46,  46,  80,  80,  60,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  80,  
 46,  46,  46,  46,  40,  40,  46,  40,  
 40,  40,  60,  60,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 40,  40,   4,   4,  82,  82,  82,  82,  
 19,  83,  15,  46,  46,  46,  46,  46,  
 46,  46,  60,  46,  46,  40,  40,  40,  
 40,  40,  40,  46,  46,  46,  46,  40,  
 40,  46,  46,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  46,  40,  40,  46,  
 40,  40,  46,  46,  60,  46,  80,  80,  
 80,  60,  60,  46,  46,  46,  46,  60,  
 60,  46,  46,  60,  60,  60,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  40,  40,  40,  40,  46,  40,  46,  
 46,  46,  46,  46,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 60,  60,  40,  40,  40,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  60,  60,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  46,  40,  46,  40,  
 40,  40,  46,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  46,  40,  40,  40,  
 40,  40,  46,  46,  60,  40,  80,  80,  
 80,  60,  60,  60,  60,  60,  46,  60,  
 60,  80,  46,  80,  80,  60,  46,  46,  
 15,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  46,  46,  46,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  60,  80,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  46,  40,  
 40,  46,  46,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  46,  46,  40,  40,  
 40,  40,  46,  46,  60,  40,  80,  60,  
 80,  60,  60,  60,  46,  46,  46,  80,  
 80,  46,  46,  80,  80,  60,  46,  46,  
 46,  46,  46,  46,  46,  46,  60,  80,  
 46,  46,  46,  46,  40,  40,  46,  40,  
 40,  40,  46,  46,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 15,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  60,  80,  46,  40,  40,  40,  
 40,  40,  40,  46,  46,  46,  40,  40,  
 40,  46,  40,  40,  40,  40,  46,  46,  
 46,  40,  40,  46,  40,  46,  40,  40,  
 46,  46,  46,  40,  40,  46,  46,  46,  
 40,  40,  40,  46,  46,  46,  40,  40,  
 40,  40,  40,  40,  40,  40,  46,  40,  
 40,  40,  46,  46,  46,  46,  80,  80,  
 60,  80,  80,  46,  46,  46,  80,  80,  
 80,  46,  80,  80,  80,  60,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  80,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 84,  19,  19,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  80,  80,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  46,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  60,  60,  
 60,  80,  80,  80,  80,  46,  60,  60,  
 60,  46,  60,  60,  60,  60,  46,  46,  
 46,  46,  46,  46,  46,  60,  60,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  40,  46,  46,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  80,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  46,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  80,  60,  
 80,  80,  80,  80,  80,  46,  60,  80,  
 80,  46,  80,  80,  60,  60,  46,  46,  
 46,  46,  46,  46,  46,  80,  80,  46,  
 46,  46,  46,  46,  46,  46,  40,  46,  
 40,  40,  46,  46,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  80,  80,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  80,  80,  
 80,  60,  60,  60,  46,  46,  80,  80,  
 80,  46,  80,  80,  80,  60,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  80,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  40,  46,  46,  46,  46,  81,  81,  
 81,  81,  81,  81,  81,  81,  81,  81,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,   3,  
 40,  60,  40,  40,  60,  60,  60,  60,  
 60,  60,  60,  46,  46,  46,  46,   4,  
 40,  40,  40,  40,  40,  40,  59,  60,  
 60,  60,  60,  60,  60,  60,  60,  15,  
  9,   9,   9,   9,   9,   9,   9,   9,  
  9,   9,   3,   3,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  40,  40,  46,  40,  46,  46,  40,  
 40,  46,  40,  46,  46,  40,  46,  46,  
 46,  46,  46,  46,  40,  40,  40,  40,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 46,  40,  40,  40,  46,  40,  46,  40,  
 46,  46,  40,  40,  46,  40,  40,   3,  
 40,  60,  40,  40,  60,  60,  60,  60,  
 60,  60,  46,  60,  60,  40,  46,  46,  
 40,  40,  40,  40,  40,  46,  59,  46,  
 60,  60,  60,  60,  60,  60,  46,  46,  
  9,   9,   9,   9,   9,   9,   9,   9,  
  9,   9,  46,  46,  40,  40,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,   3,   3,   3,   3,  
  3,   3,   3,   3,   3,   3,   3,   3,  
  3,   3,   3,  15,  15,  15,  15,  15,  
 60,  60,  15,  15,  15,  15,  15,  15,  
 78,  78,  78,  78,  78,  78,  78,  78,  
 78,  78,  85,  85,  85,  85,  85,  85,  
 85,  85,  85,  85,  15,  60,  15,  60,  
 15,  60,   5,   6,   5,   6,  80,  80,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  46,  46,  
 46,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  80,  
 60,  60,  60,  60,  60,   3,  60,  60,  
 60,  60,  60,  60,  46,  46,  46,  46,  
 60,  60,  60,  60,  60,  60,  46,  60,  
 46,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  60,  46,  46,  
 46,  60,  60,  60,  60,  60,  60,  60,  
 46,  60,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  76,  76,  
 76,  76,  76,  76,  76,  76,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  16,  
 16,  16,  16,  16,  16,  16,  16,  46,  
 46,  46,  46,   3,  46,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  46,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  46,  46,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  46,  46,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  16,  16,  
 16,  16,  16,  16,  46,  46,  46,  46,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  23,  24,  23,  24,  23,  24,  
 23,  24,  46,  46,  46,  46,  46,  46,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 86,  86,  86,  86,  86,  86,  46,  46,  
 87,  87,  87,  87,  87,  87,  46,  46,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 86,  86,  86,  86,  86,  86,  46,  46,  
 87,  87,  87,  87,  87,  87,  46,  46,  
 16,  86,  16,  86,  16,  86,  16,  86,  
 46,  87,  46,  87,  46,  87,  46,  87,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 88,  88,  89,  89,  89,  89,  90,  90,  
 91,  91,  92,  92,  93,  93,  46,  46,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 86,  86,  86,  86,  86,  86,  86,  86,  
 87,  87,  87,  87,  87,  87,  87,  87,  
 86,  86,  16,  94,  16,  46,  16,  16,  
 87,  87,  95,  95,  96,  11,  38,  11,  
 11,  11,  16,  94,  16,  46,  16,  16,  
 97,  97,  97,  97,  96,  11,  11,  11,  
 86,  86,  16,  16,  46,  46,  16,  16,  
 87,  87,  98,  98,  46,  11,  11,  11,  
 86,  86,  16,  16,  16,  99,  16,  16,  
 87,  87, 100, 100, 101,  11,  11,  11,  
 46,  46,  16,  94,  16,  46,  16,  16,  
102, 102, 103, 103,  96,  11,  11,  46,  
  2,   2,   2,   2,   2,   2,   2,   2,  
  2,   2,   2,   2, 104, 104, 104, 104,  
  8,   8,   8,   8,   8,   8,   3,   3,  
  5,   6,   5,   5,   5,   6,   5,   5,  
  3,   3,   3,   3,   3,   3,   3,   3,  
105, 106, 104, 104, 104, 104, 104,  46,  
  3,   3,   3,   3,   3,   3,   3,   3,  
  3,   5,   6,   3,   3,   3,   3,  12,  
 12,   3,   3,   3,   7,   5,   6,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46, 104, 104, 104, 104, 104, 104,  
 17,  46,  46,  46,  17,  17,  17,  17,  
 17,  17,   7,   7,   7,   5,   6,  16,  
107, 107, 107, 107, 107, 107, 107, 107,  
107, 107,   7,   7,   7,   5,   6,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
  4,   4,   4,   4,   4,   4,   4,   4,  
  4,   4,   4,   4,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 60,  60,  60,  60,  60,  60,  60,  60,  
 60,  60,  60,  60,  60,  79,  79,  79,  
 79,  60,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  38,  15,  15,  15,  15,  38,  
 15,  15,  16,  38,  38,  38,  16,  16,  
 38,  38,  38,  16,  15,  38,  15,  15,  
 38,  38,  38,  38,  38,  38,  15,  15,  
 15,  15,  15,  15,  38,  15,  38,  15,  
 38,  15,  38,  38,  38,  38,  16,  16,  
 38,  38,  15,  38,  16,  40,  40,  40,  
 40,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  19,  19,  19,  19,  19,  
 19,  19,  19,  19,  19,  19,  19, 108,  
109, 109, 109, 109, 109, 109, 109, 109,  
109, 109, 109, 109, 110, 110, 110, 110,  
111, 111, 111, 111, 111, 111, 111, 111,  
111, 111, 111, 111, 112, 112, 112, 112,  
113, 113, 113,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
  7,   7,   7,   7,   7,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,   7,  15,   7,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,   7,   7,   7,   7,   7,   7,  
  7,   7,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  46,  15,  15,  15,  15,  15,  15,  
  7,   7,   7,   7,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
  7,   7,  15,  15,  15,  15,  15,  15,  
 15,   5,   6,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
114, 114, 114, 114, 114, 114, 114, 114,  
114, 114, 114, 114, 114, 114, 114, 114,  
114, 114, 114, 114,  82,  82,  82,  82,  
 82,  82,  82,  82,  82,  82,  82,  82,  
 82,  82,  82,  82,  82,  82,  82,  82,  
115, 115, 115, 115, 115, 115, 115, 115,  
115, 115, 115, 115, 115, 115, 115, 115,  
115, 115, 115, 115,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15, 116, 116,  
116, 116, 116, 116, 116, 116, 116, 116,  
116, 116, 116, 116, 116, 116, 116, 116,  
116, 116, 116, 116, 116, 116, 116, 116,  
117, 117, 117, 117, 117, 117, 117, 117,  
117, 117, 117, 117, 117, 117, 117, 117,  
117, 117, 117, 117, 117, 117, 117, 117,  
117, 117, 118,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  46,  46,  46,  46,  
 46,  46,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 46,  15,  15,  15,  15,  46,  15,  15,  
 15,  15,  46,  46,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 46,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  46,  15,  46,  15,  
 15,  15,  15,  46,  46,  46,  15,  46,  
 15,  15,  15,  15,  15,  15,  15,  46,  
 46,  15,  15,  15,  15,  15,  15,  15,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46, 119, 119,  
119, 119, 119, 119, 119, 119, 119, 119,  
114, 114, 114, 114, 114, 114, 114, 114,  
114, 114,  83,  83,  83,  83,  83,  83,  
 83,  83,  83,  83,  15,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 46,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  46,  
  2,   3,   3,   3,  15,  59,   3, 120,  
  5,   6,   5,   6,   5,   6,   5,   6,  
  5,   6,  15,  15,   5,   6,   5,   6,  
  5,   6,   5,   6,   8,   5,   6,   5,  
 15, 121, 121, 121, 121, 121, 121, 121,  
121, 121,  60,  60,  60,  60,  60,  60,  
  8,  59,  59,  59,  59,  59,  15,  15,  
 46,  46,  46,  46,  46,  46,  46,  15,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  46,  46,  
 46,  60,  60,  59,  59,  59,  59,  46,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,   3,  59,  59,  59,  46,  
 46,  46,  46,  46,  46,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  46,  46,  
 46,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  46,  
 15,  15,  85,  85,  85,  85,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  46,  46,  46,  
 85,  85,  85,  85,  85,  85,  85,  85,  
 85,  85,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  46,  46,  46,  15,  
114, 114, 114, 114, 114, 114, 114, 114,  
114, 114,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  46,  46,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  46,  
 46,  46,  46,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  46,  46,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  15,  
 15,  15,  15,  15,  15,  15,  15,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
122, 122, 122, 122, 122, 122, 122, 122,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
123, 123, 123, 123, 123, 123, 123, 123,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 16,  16,  16,  16,  16,  16,  16,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  16,  16,  16,  16,  16,  
 46,  46,  46,  46,  46,  46,  60,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,   7,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  46,  
 40,  40,  40,  40,  40,  46,  40,  46,  
 40,  40,  46,  40,  40,  46,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,   5,   6,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 46,  46,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 60,  60,  60,  60,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
  3,   8,   8,  12,  12,   5,   6,   5,  
  6,   5,   6,   5,   6,   5,   6,   5,  
  6,   5,   6,   5,   6,  46,  46,  46,  
 46,   3,   3,   3,   3,  12,  12,  12,  
  3,   3,   3,  46,   3,   3,   3,   3,  
  8,   5,   6,   5,   6,   5,   6,   3,  
  3,   3,   7,   8,   7,   7,   7,  46,  
  3,   4,   3,   3,  46,  46,  46,  46,  
 40,  40,  40,  46,  40,  46,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  46,  46, 104,  
 46,   3,   3,   3,   4,   3,   3,   3,  
  5,   6,   3,   7,   3,   8,   3,   3,  
  9,   9,   9,   9,   9,   9,   9,   9,  
  9,   9,   3,   3,   7,   7,   7,   3,  
  3,  10,  10,  10,  10,  10,  10,  10,  
 10,  10,  10,  10,  10,  10,  10,  10,  
 10,  10,  10,  10,  10,  10,  10,  10,  
 10,  10,  10,   5,   3,   6,  11,  12,  
 11,  13,  13,  13,  13,  13,  13,  13,  
 13,  13,  13,  13,  13,  13,  13,  13,  
 13,  13,  13,  13,  13,  13,  13,  13,  
 13,  13,  13,   5,   7,   6,   7,  46,  
 46,   3,   5,   6,   3,   3,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 59,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  59,  59,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  40,  
 40,  40,  40,  40,  40,  40,  40,  46,  
 46,  46,  40,  40,  40,  40,  40,  40,  
 46,  46,  40,  40,  40,  40,  40,  40,  
 46,  46,  40,  40,  40,  40,  40,  40,  
 46,  46,  40,  40,  40,  46,  46,  46,  
  4,   4,   7,  11,  15,   4,   4,  46,  
  7,   7,   7,   7,   7,  15,  15,  46,  
 46,  46,  46,  46,  46,  46,  46,  46,  
 46,  46,  46,  46,  46,  15,  46,  46   
};



const uint32 js_A[] = {
0x0001000F,  
0x0004000F,  
0x0004000C,  
0x00000018,  
0x0006001A,  
0x00000015,  
0x00000016,  
0x00000019,  
0x00000014,  
0x00036089,  
0x0827FF81,  
0x0000001B,  
0x00050017,  
0x0817FF82,  
0x0000000C,  
0x0000001C,  
0x00070182,  
0x0000600B,  
0x0000500B,  
0x0000800B,  
0x08270181,  
0x08170182,  
0xE1D70182,  
0x00670181,  
0x00570182,  
0xCE670181,  
0x3A170182,  
0xE1E70181,  
0x4B170182,  
0x34A70181,  
0x33A70181,  
0x33670181,  
0x32A70181,  
0x32E70181,  
0x33E70181,  
0x34E70181,  
0x34670181,  
0x35670181,  
0x00070181,  
0x36A70181,  
0x00070185,  
0x36670181,  
0x36E70181,  
0x00AF0181,  
0x007F0183,  
0x009F0182,  
0x00000000,  
0x34970182,  
0x33970182,  
0x33570182,  
0x32970182,  
0x32D70182,  
0x33D70182,  
0x34570182,  
0x34D70182,  
0x35570182,  
0x36970182,  
0x36570182,  
0x36D70182,  
0x00070084,  
0x00030086,  
0x09A70181,  
0x09670181,  
0x10270181,  
0x0FE70181,  
0x09970182,  
0x09570182,  
0x10170182,  
0x0FD70182,  
0x0F970182,  
0x0E570182,  
0x0BD70182,  
0x0D970182,  
0x15970182,  
0x14170182,  
0x14270181,  
0x0C270181,  
0x0C170182,  
0x00034089,  
0x00000087,  
0x00030088,  
0x00037489,  
0x00005A0B,  
0x00006E0B,  
0x0000740B,  
0x0000000B,  
0xFE170182,  
0xFE270181,  
0xED970182,  
0xEA970182,  
0xE7170182,  
0xE0170182,  
0xE4170182,  
0xE0970182,  
0xFDD70182,  
0xEDA70181,  
0xFDE70181,  
0xEAA70181,  
0xE7270181,  
0xFE570182,  
0xE4270181,  
0xFE670181,  
0xE0270181,  
0xE0A70181,  
0x00010010,  
0x0004000D,  
0x0004000E,  
0x0000400B,  
0x0000440B,  
0x0427438A,  
0x0427818A,  
0x0417638A,  
0x0417818A,  
0x0007818A,  
0x0000420B,  
0x0000720B,  
0x06A0001C,  
0x0690001C,  
0x00006C0B,  
0x0000560B,  
0x0007738A,  
0x0007418A,  
0x00000013,  
0x00000012   
};

const jschar js_uriReservedPlusPound_ucstr[] =
    {';', '/', '?', ':', '@', '&', '=', '+', '$', ',', '#', 0};
const jschar js_uriUnescaped_ucstr[] =
    {'0', '1', '2', '3', '4', '5', '6', '7', '8', '9',
     'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M',
     'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z',
     'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
     'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
     '-', '_', '.', '!', '~', '*', '\'', '(', ')', 0};

#define URI_CHUNK 64U


static JSBool
AddCharsToURI(JSContext *cx, JSString *str, const jschar *chars, size_t length)
{
    size_t total;
    jschar *newchars;

    JS_ASSERT(!JSSTRING_IS_DEPENDENT(str));
    total = str->length + length + 1;
    if (!str->chars ||
        JS_HOWMANY(total, URI_CHUNK) > JS_HOWMANY(str->length + 1, URI_CHUNK)) {
        total = JS_ROUNDUP(total, URI_CHUNK);
        newchars = (jschar *) JS_realloc(cx, str->chars,
                                         total * sizeof(jschar));
        if (!newchars)
            return JS_FALSE;
        str->chars = newchars;
    }
    js_strncpy(str->chars + str->length, chars, length);
    str->length += length;
    str->chars[str->length] = 0;
    return JS_TRUE;
}








static JSBool
Encode(JSContext *cx, JSString *str, const jschar *unescapedSet,
       const jschar *unescapedSet2, jsval *rval)
{
    size_t length, j, k, L;
    jschar *chars, c, c2;
    uint32 v;
    uint8 utf8buf[6];
    jschar hexBuf[4];
    static const char HexDigits[] = "0123456789ABCDEF"; 
    JSString *R;

    length = JSSTRING_LENGTH(str);
    if (length == 0) {
        *rval = STRING_TO_JSVAL(cx->runtime->emptyString);
        return JS_TRUE;
    }

    R = js_NewString(cx, NULL, 0, 0);
    if (!R)
        return JS_FALSE;

    hexBuf[0] = '%';
    hexBuf[3] = 0;
    chars = JSSTRING_CHARS(str);
    for (k = 0; k < length; k++) {
        c = chars[k];
        if (js_strchr(unescapedSet, c) ||
            (unescapedSet2 && js_strchr(unescapedSet2, c))) {
            if (!AddCharsToURI(cx, R, &c, 1))
                return JS_FALSE;
        } else {
            if ((c >= 0xDC00) && (c <= 0xDFFF)) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_URI, NULL);
                return JS_FALSE;
            }
            if (c < 0xD800 || c > 0xDBFF) {
                v = c;
            } else {
                k++;
                if (k == length) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_URI, NULL);
                    return JS_FALSE;
                }
                c2 = chars[k];
                if ((c2 < 0xDC00) || (c2 > 0xDFFF)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_URI, NULL);
                    return JS_FALSE;
                }
                v = ((c - 0xD800) << 10) + (c2 - 0xDC00) + 0x10000;
            }
            L = js_OneUcs4ToUtf8Char(utf8buf, v);
            for (j = 0; j < L; j++) {
                hexBuf[1] = HexDigits[utf8buf[j] >> 4];
                hexBuf[2] = HexDigits[utf8buf[j] & 0xf];
                if (!AddCharsToURI(cx, R, hexBuf, 3))
                    return JS_FALSE;
            }
        }
    }

    




    chars = (jschar *) JS_realloc(cx, R->chars, (R->length+1) * sizeof(jschar));
    if (chars)
        R->chars = chars;
    *rval = STRING_TO_JSVAL(R);
    return JS_TRUE;
}

static JSBool
Decode(JSContext *cx, JSString *str, const jschar *reservedSet, jsval *rval)
{
    size_t length, start, k;
    jschar *chars, c, H;
    uint32 v;
    jsuint B;
    uint8 octets[6];
    JSString *R;
    intN j, n;

    length = JSSTRING_LENGTH(str);
    if (length == 0) {
        *rval = STRING_TO_JSVAL(cx->runtime->emptyString);
        return JS_TRUE;
    }

    R = js_NewString(cx, NULL, 0, 0);
    if (!R)
        return JS_FALSE;

    chars = JSSTRING_CHARS(str);
    for (k = 0; k < length; k++) {
        c = chars[k];
        if (c == '%') {
            start = k;
            if ((k + 2) >= length)
                goto bad;
            if (!JS7_ISHEX(chars[k+1]) || !JS7_ISHEX(chars[k+2]))
                goto bad;
            B = JS7_UNHEX(chars[k+1]) * 16 + JS7_UNHEX(chars[k+2]);
            k += 2;
            if (!(B & 0x80)) {
                c = (jschar)B;
            } else {
                n = 1;
                while (B & (0x80 >> n))
                    n++;
                if (n == 1 || n > 6)
                    goto bad;
                octets[0] = (uint8)B;
                if (k + 3 * (n - 1) >= length)
                    goto bad;
                for (j = 1; j < n; j++) {
                    k++;
                    if (chars[k] != '%')
                        goto bad;
                    if (!JS7_ISHEX(chars[k+1]) || !JS7_ISHEX(chars[k+2]))
                        goto bad;
                    B = JS7_UNHEX(chars[k+1]) * 16 + JS7_UNHEX(chars[k+2]);
                    if ((B & 0xC0) != 0x80)
                        goto bad;
                    k += 2;
                    octets[j] = (char)B;
                }
                v = Utf8ToOneUcs4Char(octets, n);
                if (v >= 0x10000) {
                    v -= 0x10000;
                    if (v > 0xFFFFF)
                        goto bad;
                    c = (jschar)((v & 0x3FF) + 0xDC00);
                    H = (jschar)((v >> 10) + 0xD800);
                    if (!AddCharsToURI(cx, R, &H, 1))
                        return JS_FALSE;
                } else {
                    c = (jschar)v;
                }
            }
            if (js_strchr(reservedSet, c)) {
                if (!AddCharsToURI(cx, R, &chars[start], (k - start + 1)))
                    return JS_FALSE;
            } else {
                if (!AddCharsToURI(cx, R, &c, 1))
                    return JS_FALSE;
            }
        } else {
            if (!AddCharsToURI(cx, R, &c, 1))
                return JS_FALSE;
        }
    }

    




    chars = (jschar *) JS_realloc(cx, R->chars, (R->length+1) * sizeof(jschar));
    if (chars)
        R->chars = chars;
    *rval = STRING_TO_JSVAL(R);
    return JS_TRUE;

bad:
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_URI);
    return JS_FALSE;
}

static JSBool
str_decodeURI(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(str);
    return Decode(cx, str, js_uriReservedPlusPound_ucstr, vp);
}

static JSBool
str_decodeURI_Component(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(str);
    return Decode(cx, str, js_empty_ucstr, vp);
}

static JSBool
str_encodeURI(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(str);
    return Encode(cx, str, js_uriReservedPlusPound_ucstr, js_uriUnescaped_ucstr,
                  vp);
}

static JSBool
str_encodeURI_Component(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    str = js_ValueToString(cx, vp[2]);
    if (!str)
        return JS_FALSE;
    vp[2] = STRING_TO_JSVAL(str);
    return Encode(cx, str, js_uriUnescaped_ucstr, NULL, vp);
}





int
js_OneUcs4ToUtf8Char(uint8 *utf8Buffer, uint32 ucs4Char)
{
    int utf8Length = 1;

    JS_ASSERT(ucs4Char <= 0x7FFFFFFF);
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
    uint32 ucs4Char;
    uint32 minucs4Char;
    
    static const uint32 minucs4Table[] = {
        0x00000080, 0x00000800, 0x0001000, 0x0020000, 0x0400000
    };

    JS_ASSERT(utf8Length >= 1 && utf8Length <= 6);
    if (utf8Length == 1) {
        ucs4Char = *utf8Buffer;
        JS_ASSERT(!(ucs4Char & 0x80));
    } else {
        JS_ASSERT((*utf8Buffer & (0x100 - (1 << (7-utf8Length)))) ==
                  (0x100 - (1 << (8-utf8Length))));
        ucs4Char = *utf8Buffer++ & ((1<<(7-utf8Length))-1);
        minucs4Char = minucs4Table[utf8Length-2];
        while (--utf8Length) {
            JS_ASSERT((*utf8Buffer & 0xC0) == 0x80);
            ucs4Char = ucs4Char<<6 | (*utf8Buffer++ & 0x3F);
        }
        if (ucs4Char < minucs4Char ||
            ucs4Char == 0xFFFE || ucs4Char == 0xFFFF) {
            ucs4Char = 0xFFFD;
        }
    }
    return ucs4Char;
}

#if defined(DEBUG) ||                                                         \
    defined(DUMP_CALL_TABLE) ||                                               \
    defined(DUMP_SCOPE_STATS)

JS_FRIEND_API(size_t)
js_PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp,
                        JSString *str, uint32 quote)
{
    jschar *chars, *charsEnd;
    size_t n;
    char *escape;
    char c;
    uintN u, hex, shift;
    enum {
        STOP, FIRST_QUOTE, LAST_QUOTE, CHARS, ESCAPE_START, ESCAPE_MORE
    } state;

    JS_ASSERT(quote == 0 || quote == '\'' || quote == '"');
    JS_ASSERT_IF(buffer, bufferSize != 0);
    JS_ASSERT_IF(!buffer, bufferSize == 0);
    JS_ASSERT_IF(fp, !buffer);

    chars = JSSTRING_CHARS(str);
    charsEnd = chars + JSSTRING_LENGTH(str);
    n = 0;
    --bufferSize;
    state = FIRST_QUOTE;
    shift = 0;
    hex = 0;
    u = 0;
    c = 0;  

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
                    escape = strchr(js_EscapeMap, (int)u);
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
            if (n == bufferSize)
                break;
            buffer[n] = c;
        } else if (fp) {
            fputc(c, fp);
        }
        n++;
    }
  stop:
    if (buffer)
        buffer[n] = '\0';
    return n;
}

#endif
