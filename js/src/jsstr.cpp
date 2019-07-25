

















































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
#include "jsfun.h"      
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsopcode.h"
#include "jsregexp.h"
#include "jsscope.h"
#include "jsstaticcheck.h"
#include "jsstr.h"
#include "jsbit.h"
#include "jsvector.h"
#include "jsversion.h"

#include "jscntxtinlines.h"
#include "jsinterpinlines.h"
#include "jsobjinlines.h"
#include "jsregexpinlines.h"
#include "jsstrinlines.h"
#include "jsautooplen.h"        

using namespace js;
using namespace js::gc;

JS_STATIC_ASSERT(size_t(JSString::MAX_LENGTH) <= size_t(JSVAL_INT_MAX));
JS_STATIC_ASSERT(JSString::MAX_LENGTH <= JSVAL_INT_MAX);

JS_STATIC_ASSERT(JS_EXTERNAL_STRING_LIMIT == 8);
JSStringFinalizeOp str_finalizers[JS_EXTERNAL_STRING_LIMIT] = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
};

const jschar *
js_GetStringChars(JSContext *cx, JSString *str)
{
    if (!js_MakeStringImmutable(cx, str))
        return NULL;
    return str->flatChars();
}

void
JSString::flatten()
{
    JS_ASSERT(isRope());

    



    JSString *topNode = this;
    while (topNode->isInteriorNode())
        topNode = topNode->interiorNodeParent();

    const size_t length = topNode->length();
    const size_t capacity = topNode->topNodeCapacity();
    jschar *const chars = (jschar *) topNode->topNodeBuffer();

    



    topNode->e.mParent = NULL;
#ifdef DEBUG
    topNode->mLengthAndFlags = JSString::INTERIOR_NODE;
#endif

    









    JSString *str = topNode;
    jschar *pos = chars;
    while (true) {
        
        JS_ASSERT(str->isInteriorNode());
        {
            JSString *next = str->mLeft;
            str->mChars = pos;  
            if (next->isInteriorNode()) {
                str->mLengthAndFlags = 0x200;  
                str = next;
                continue;  
            } else {
                size_t len = next->length();
                PodCopy(pos, next->mChars, len);
                pos += len;
                goto visit_right_child;
            }
        }

      revisit_parent:
        if (str->mLengthAndFlags == 0x200) {
          visit_right_child:
            JSString *next = str->e.mRight;
            if (next->isInteriorNode()) {
                str->mLengthAndFlags = 0x300;
                str = next;
                continue;  
            } else {
                size_t len = next->length();
                PodCopy(pos, next->mChars, len);
                pos += len;
                goto finish_node;
            }
        } else {
            JS_ASSERT(str->mLengthAndFlags == 0x300);
          finish_node:
            JSString *next = str->e.mParent;
            str->finishTraversalConversion(topNode, pos);
            if (!next) {
                JS_ASSERT(pos == chars + length);
                *pos = 0;
                topNode->initFlatExtensible(chars, length, capacity);
                return;
            }
            str = next;
            goto revisit_parent;  
        }
    }
}

#ifdef JS_TRACER

int32 JS_FASTCALL
js_Flatten(JSString* str)
{
    str->flatten();
    return 0;
}
JS_DEFINE_CALLINFO_1(extern, INT32, js_Flatten, STRING, 0, nanojit::ACCSET_STORE_ANY)

#endif 

static JS_ALWAYS_INLINE size_t
RopeAllocSize(const size_t length, size_t *capacity)
{
    static const size_t ROPE_DOUBLING_MAX = 1024 * 1024;

    size_t size;
    size_t minCap = (length + 1) * sizeof(jschar);

    




    if (length > ROPE_DOUBLING_MAX)
        size = minCap + (minCap / 8);
    else
        size = RoundUpPow2(minCap);
    *capacity = (size / sizeof(jschar)) - 1;
    JS_ASSERT(size >= sizeof(JSRopeBufferInfo));
    return size;
}

static JS_ALWAYS_INLINE JSRopeBufferInfo *
ObtainRopeBuffer(JSContext *cx, bool usingLeft, bool usingRight,
                 JSRopeBufferInfo *sourceBuffer, size_t length,
                 JSString *left, JSString *right)
{
    JSRopeBufferInfo *buf;
    size_t capacity;

    





    if (usingLeft)
        left->nullifyTopNodeBuffer();
    if (usingRight)
        right->nullifyTopNodeBuffer();

    



    if (length <= sourceBuffer->capacity) {
        buf = sourceBuffer;
    } else {
        size_t allocSize = RopeAllocSize(length, &capacity);
        cx->free(sourceBuffer);
        buf = (JSRopeBufferInfo *) cx->malloc(allocSize);
        if (!buf)
            return NULL;
        buf->capacity = capacity;
    }
    return buf;
}

static JS_ALWAYS_INLINE JSString *
FinishConcat(JSContext *cx, bool usingLeft, bool usingRight,
             JSString *left, JSString *right, size_t length,
             JSRopeBufferInfo *buf)
{
    JSString *res = js_NewGCString(cx);
    if (!res) {
        cx->free(buf);
        return NULL;
    }
    res->initTopNode(left, right, length, buf);
    if (usingLeft)
        left->convertToInteriorNode(res);
    if (usingRight)
        right->convertToInteriorNode(res);
    return res;
}

JSString * JS_FASTCALL
js_ConcatStrings(JSContext *cx, JSString *left, JSString *right)
{
    size_t length, leftLen, rightLen;
    bool leftRopeTop, rightRopeTop;

    leftLen = left->length();
    if (leftLen == 0)
        return right;
    rightLen = right->length();
    if (rightLen == 0)
        return left;

    length = leftLen + rightLen;

    if (JSShortString::fitsIntoShortString(length)) {
        JSShortString *shortStr = js_NewGCShortString(cx);
        if (!shortStr)
            return NULL;

        jschar *buf = shortStr->init(length);
        js_short_strncpy(buf, left->chars(), leftLen);
        js_short_strncpy(buf + leftLen, right->chars(), rightLen);
        buf[length] = 0;
        return shortStr->header();
    }

    








    if (left->isInteriorNode())
        left->flatten();
    if (right->isInteriorNode())
        right->flatten();

    if (left->isExtensible() && !right->isRope() &&
        left->flatCapacity() >= length) {
        JS_ASSERT(left->isFlat());

        



        jschar *chars = left->chars();
        js_strncpy(chars + leftLen, right->chars(), rightLen);
        chars[length] = 0;
        JSString *res = js_NewString(cx, chars, length);
        if (!res)
            return NULL;
        res->initFlatExtensible(chars, length, left->flatCapacity());
        left->initDependent(res, res->flatChars(), leftLen);
        return res;
    }

    if (length > JSString::MAX_LENGTH) {
        if (JS_ON_TRACE(cx)) {
            if (!CanLeaveTrace(cx))
                return NULL;
            LeaveTrace(cx);
        }
        js_ReportAllocationOverflow(cx);
        return NULL;
    }

    leftRopeTop = left->isTopNode();
    rightRopeTop = right->isTopNode();

    



    if (left == right && leftRopeTop) {
        left->flatten();
        leftRopeTop = false;
        rightRopeTop = false;
        JS_ASSERT(leftLen == left->length());
        JS_ASSERT(rightLen == right->length());
        JS_ASSERT(!left->isTopNode());
        JS_ASSERT(!right->isTopNode());
    }

    



    JSRopeBufferInfo *buf = NULL;

    if (leftRopeTop) {
        
        JSRopeBufferInfo *leftBuf = left->topNodeBuffer();

        
        if (JS_UNLIKELY(rightRopeTop)) {
            JSRopeBufferInfo *rightBuf = right->topNodeBuffer();

            
            if (leftBuf->capacity >= rightBuf->capacity) {
                cx->free(rightBuf);
            } else {
                cx->free(leftBuf);
                leftBuf = rightBuf;
            }
        }

        buf = ObtainRopeBuffer(cx, true, rightRopeTop, leftBuf, length, left, right);
        if (!buf)
            return NULL;
    } else if (JS_UNLIKELY(rightRopeTop)) {
        
        JSRopeBufferInfo *rightBuf = right->topNodeBuffer();

        buf = ObtainRopeBuffer(cx, false, true, rightBuf, length, left, right);
        if (!buf)
            return NULL;
    } else {
        
        size_t capacity;
        size_t allocSize = RopeAllocSize(length, &capacity);
        buf = (JSRopeBufferInfo *) cx->malloc(allocSize);
        if (!buf)
            return NULL;
        buf->capacity = capacity;
    }

    return FinishConcat(cx, leftRopeTop, rightRopeTop, left, right, length, buf);
}

const jschar *
JSString::undepend(JSContext *cx)
{
    size_t n, size;
    jschar *s;

    ensureNotRope();

    if (isDependent()) {
        n = dependentLength();
        size = (n + 1) * sizeof(jschar);
        s = (jschar *) cx->malloc(size);
        if (!s)
            return NULL;

        js_strncpy(s, dependentChars(), n);
        s[n] = 0;
        initFlat(s, n);

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

    return flatChars();
}

JSBool
js_MakeStringImmutable(JSContext *cx, JSString *str)
{
    



    str->ensureNotRope();
    if (!str->ensureNotDependent(cx)) {
        JS_RUNTIME_METER(cx->runtime, badUndependStrings);
        return JS_FALSE;
    }
    str->flatClearExtensible();
    return JS_TRUE;
}

static JSString *
ArgToRootedString(JSContext *cx, uintN argc, Value *vp, uintN arg)
{
    if (arg >= argc)
        return ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    vp += 2 + arg;

    if (vp->isObject() && !DefaultValue(cx, &vp->toObject(), JSTYPE_STRING, vp))
        return NULL;

    JSString *str;
    if (vp->isString()) {
        str = vp->toString();
    } else if (vp->isBoolean()) {
        str = ATOM_TO_STRING(cx->runtime->atomState.booleanAtoms[
                                  (int)vp->toBoolean()]);
    } else if (vp->isNull()) {
        str = ATOM_TO_STRING(cx->runtime->atomState.nullAtom);
    } else if (vp->isUndefined()) {
        str = ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    }
    else {
        str = js_NumberToString(cx, vp->toNumber());
        if (str)
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

static const uint32 OVERLONG_UTF8 = UINT32_MAX;

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
js_str_escape(JSContext *cx, JSObject *obj, uintN argc, Value *argv, Value *rval)
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
        if (!ValueToNumber(cx, argv[1], &d))
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

    str = ArgToRootedString(cx, argc, argv - 2, 0);
    if (!str)
        return JS_FALSE;

    str->getCharsAndLength(chars, length);
    newlength = length;

    
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
            js_ReportAllocationOverflow(cx);
            return JS_FALSE;
        }
    }

    if (newlength >= ~(size_t)0 / sizeof(jschar)) {
        js_ReportAllocationOverflow(cx);
        return JS_FALSE;
    }

    newchars = (jschar *) cx->malloc((newlength + 1) * sizeof(jschar));
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

    str = js_NewString(cx, newchars, newlength);
    if (!str) {
        cx->free(newchars);
        return JS_FALSE;
    }
    rval->setString(str);
    return JS_TRUE;
}
#undef IS_OK

static JSBool
str_escape(JSContext *cx, uintN argc, Value *vp)
{
    JSObject *obj = ComputeThisFromVp(cx, vp);
    return obj && js_str_escape(cx, obj, argc, vp + 2, vp);
}


static JSBool
str_unescape(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    size_t i, ni, length;
    const jschar *chars;
    jschar *newchars;
    jschar ch;

    str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;

    str->getCharsAndLength(chars, length);

    
    newchars = (jschar *) cx->malloc((length + 1) * sizeof(jschar));
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

    str = js_NewString(cx, newchars, ni);
    if (!str) {
        cx->free(newchars);
        return JS_FALSE;
    }
    vp->setString(str);
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

static JSBool
str_getProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSString *str;

    if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
        if (obj->getClass() == &js_StringClass) {
            
            str = obj->getPrimitiveThis().toString();
        } else {
            
            str = js_ValueToString(cx, ObjectValue(*obj));
            if (!str)
                return JS_FALSE;
        }

        vp->setInt32(str->length());
    }

    return JS_TRUE;
}

#define STRING_ELEMENT_ATTRS (JSPROP_ENUMERATE|JSPROP_READONLY|JSPROP_PERMANENT)

static JSBool
str_enumerate(JSContext *cx, JSObject *obj)
{
    JSString *str, *str1;
    size_t i, length;

    str = obj->getPrimitiveThis().toString();

    length = str->length();
    for (i = 0; i < length; i++) {
        str1 = js_NewDependentString(cx, str, i, 1);
        if (!str1)
            return JS_FALSE;
        if (!obj->defineProperty(cx, INT_TO_JSID(i), StringValue(str1),
                                 PropertyStub, PropertyStub,
                                 STRING_ELEMENT_ATTRS)) {
            return JS_FALSE;
        }
    }

    return obj->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom),
                               UndefinedValue(), NULL, NULL,
                               JSPROP_PERMANENT | JSPROP_READONLY | JSPROP_SHARED);
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
        JSString *str1 = JSString::getUnitString(cx, str, size_t(slot));
        if (!str1)
            return JS_FALSE;
        if (!obj->defineProperty(cx, id, StringValue(str1), NULL, NULL,
                                 STRING_ELEMENT_ATTRS)) {
            return JS_FALSE;
        }
        *objp = obj;
    }
    return JS_TRUE;
}

Class js_StringClass = {
    js_String_str,
    JSCLASS_HAS_RESERVED_SLOTS(1) | JSCLASS_NEW_RESOLVE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_String),
    PropertyStub,   
    PropertyStub,   
    str_getProperty,
    PropertyStub,   
    str_enumerate,
    (JSResolveOp)str_resolve,
    ConvertStub
};

#define NORMALIZE_THIS(cx,vp,str)                                             \
    JS_BEGIN_MACRO                                                            \
        if (vp[1].isString()) {                                               \
            str = vp[1].toString();                                           \
        } else {                                                              \
            str = NormalizeThis(cx, vp);                                      \
            if (!str)                                                         \
                return JS_FALSE;                                              \
        }                                                                     \
    JS_END_MACRO

static JSString *
NormalizeThis(JSContext *cx, Value *vp)
{
    if (vp[1].isNullOrUndefined() && !ComputeThisFromVp(cx, vp))
        return NULL;

    








    if (vp[1].isObject()) {
        JSObject *obj = &vp[1].toObject();
        if (obj->getClass() == &js_StringClass) {
            vp[1] = obj->getPrimitiveThis();
            return vp[1].toString();
        }
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
    JSString *str;

    NORMALIZE_THIS(cx, vp, str);
    str = js_QuoteString(cx, str, '"');
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;
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

    const jschar *s;
    size_t k;
    str->getCharsAndLength(s, k);

    size_t n = j + k + 2;
    jschar *t = (jschar *) cx->malloc((n + 1) * sizeof(jschar));
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
        cx->free(t);
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





static JSString *
SubstringTail(JSContext *cx, JSString *str, jsdouble length, jsdouble begin, jsdouble end)
{
    if (begin < 0)
        begin = 0;
    else if (begin > length)
        begin = length;

    if (end < 0)
        end = 0;
    else if (end > length)
        end = length;
    if (end < begin) {
        
        jsdouble tmp = begin;
        begin = end;
        end = tmp;
    }

    return js_NewDependentString(cx, str, (size_t)begin, (size_t)(end - begin));
}

static JSBool
str_substring(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    jsdouble d;
    jsdouble length, begin, end;

    NORMALIZE_THIS(cx, vp, str);
    if (argc != 0) {
        if (!ValueToNumber(cx, vp[2], &d))
            return JS_FALSE;
        length = str->length();
        begin = js_DoubleToInteger(d);
        if (argc == 1 || vp[3].isUndefined()) {
            end = length;
        } else {
            if (!ValueToNumber(cx, vp[3], &d))
                return JS_FALSE;
            end = js_DoubleToInteger(d);
        }

        str = SubstringTail(cx, str, length, begin, end);
        if (!str)
            return JS_FALSE;
    }
    vp->setString(str);
    return JS_TRUE;
}

JSString* JS_FASTCALL
js_toLowerCase(JSContext *cx, JSString *str)
{
    size_t i, n;
    const jschar *s;
    jschar *news;

    str->getCharsAndLength(s, n);
    news = (jschar *) cx->malloc((n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    for (i = 0; i < n; i++)
        news[i] = JS_TOLOWER(s[i]);
    news[n] = 0;
    str = js_NewString(cx, news, n);
    if (!str) {
        cx->free(news);
        return NULL;
    }
    return str;
}

static JSBool
str_toLowerCase(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    NORMALIZE_THIS(cx, vp, str);
    str = js_toLowerCase(cx, str);
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;
}

static JSBool
str_toLocaleLowerCase(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToLowerCase) {
        NORMALIZE_THIS(cx, vp, str);
        return cx->localeCallbacks->localeToLowerCase(cx, str, Jsvalify(vp));
    }
    return str_toLowerCase(cx, 0, vp);
}

JSString* JS_FASTCALL
js_toUpperCase(JSContext *cx, JSString *str)
{
    size_t i, n;
    const jschar *s;
    jschar *news;

    str->getCharsAndLength(s, n);
    news = (jschar *) cx->malloc((n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    for (i = 0; i < n; i++)
        news[i] = JS_TOUPPER(s[i]);
    news[n] = 0;
    str = js_NewString(cx, news, n);
    if (!str) {
        cx->free(news);
        return NULL;
    }
    return str;
}

static JSBool
str_toUpperCase(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    NORMALIZE_THIS(cx, vp, str);
    str = js_toUpperCase(cx, str);
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;
}

static JSBool
str_toLocaleUpperCase(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    



    if (cx->localeCallbacks && cx->localeCallbacks->localeToUpperCase) {
        NORMALIZE_THIS(cx, vp, str);
        return cx->localeCallbacks->localeToUpperCase(cx, str, Jsvalify(vp));
    }
    return str_toUpperCase(cx, 0, vp);
}

static JSBool
str_localeCompare(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str, *thatStr;

    NORMALIZE_THIS(cx, vp, str);
    if (argc == 0) {
        vp->setInt32(0);
    } else {
        thatStr = js_ValueToString(cx, vp[2]);
        if (!thatStr)
            return JS_FALSE;
        if (cx->localeCallbacks && cx->localeCallbacks->localeCompare) {
            vp[2].setString(thatStr);
            return cx->localeCallbacks->localeCompare(cx, str, thatStr, Jsvalify(vp));
        }
        vp->setInt32(js_CompareStrings(str, thatStr));
    }
    return JS_TRUE;
}

JSBool
js_str_charAt(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    jsint i;
    jsdouble d;

    if (vp[1].isString() && argc != 0 && vp[2].isInt32()) {
        str = vp[1].toString();
        i = vp[2].toInt32();
        if ((size_t)i >= str->length())
            goto out_of_range;
    } else {
        NORMALIZE_THIS(cx, vp, str);

        if (argc == 0) {
            d = 0.0;
        } else {
            if (!ValueToNumber(cx, vp[2], &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
        }

        if (d < 0 || str->length() <= d)
            goto out_of_range;
        i = (jsint) d;
    }

    str = JSString::getUnitString(cx, str, size_t(i));
    if (!str)
        return JS_FALSE;
    vp->setString(str);
    return JS_TRUE;

out_of_range:
    vp->setString(cx->runtime->emptyString);
    return JS_TRUE;
}

JSBool
js_str_charCodeAt(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    jsint i;
    jsdouble d;

    if (vp[1].isString() && argc != 0 && vp[2].isInt32()) {
        str = vp[1].toString();
        i = vp[2].toInt32();
        if ((size_t)i >= str->length())
            goto out_of_range;
    } else {
        NORMALIZE_THIS(cx, vp, str);

        if (argc == 0) {
            d = 0.0;
        } else {
            if (!ValueToNumber(cx, vp[2], &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
        }

        if (d < 0 || str->length() <= d)
            goto out_of_range;
        i = (jsint) d;
    }

    vp->setInt32(str->chars()[i]);
    return JS_TRUE;

out_of_range:
    vp->setDouble(js_NaN);
    return JS_TRUE;
}

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

static jsint
RopeMatch(JSString *textstr, const jschar *pat, jsuint patlen)
{
    JS_ASSERT(textstr->isTopNode());

    if (patlen == 0)
        return 0;
    if (textstr->length() < patlen)
        return -1;

    




    Vector<JSString *, 16, SystemAllocPolicy> strs;

    





    size_t textstrlen = textstr->length();
    size_t threshold = textstrlen >> sRopeMatchThresholdRatioLog2;
    JSRopeLeafIterator iter(textstr);
    for (JSString *str = iter.init(); str; str = iter.next()) {
        if (threshold-- == 0 || !strs.append(str))
            return StringMatch(textstr->chars(), textstrlen, pat, patlen);
    }

    
    jsint pos = 0;

    

    for (JSString **outerp = strs.begin(); outerp != strs.end(); ++outerp) {
        
        const jschar *chars;
        size_t len;
        (*outerp)->getCharsAndLength(chars, len);
        jsint matchResult = StringMatch(chars, len, pat, patlen);
        if (matchResult != -1)
            return pos + matchResult;

        
        JSString **innerp = outerp;

        



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
                    if (++innerp == strs.end())
                        return -1;
                    (*innerp)->getCharsAndEnd(tt, ttend);
                }
                if (*pp != *tt)
                    goto break_continue;
            }

            
            return pos + (t - chars) - 1;  

          break_continue:;
        }

        pos += len;
    }

    return -1;
}

static JSBool
str_indexOf(JSContext *cx, uintN argc, Value *vp)
{

    JSString *str;
    NORMALIZE_THIS(cx, vp, str);

    JSString *patstr = ArgToRootedString(cx, argc, vp, 0);
    if (!patstr)
        return JS_FALSE;

    const jschar *text = str->chars();
    jsuint textlen = str->length();
    const jschar *pat = patstr->chars();
    jsuint patlen = patstr->length();

    jsuint start;
    if (argc > 1) {
        if (vp[3].isInt32()) {
            jsint i = vp[3].toInt32();
            if (i <= 0) {
                start = 0;
            } else if (jsuint(i) > textlen) {
                start = 0;
                textlen = 0;
            } else {
                start = i;
                text += start;
                textlen -= start;
            }
        } else {
            jsdouble d;
            if (!ValueToNumber(cx, vp[3], &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
            if (d <= 0) {
                start = 0;
            } else if (d > textlen) {
                start = 0;
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
    JSString *str, *str2;
    const jschar *text, *pat;
    jsint i, j, textlen, patlen;
    jsdouble d;

    NORMALIZE_THIS(cx, vp, str);
    text = str->chars();
    textlen = (jsint) str->length();

    if (argc != 0 && vp[2].isString()) {
        str2 = vp[2].toString();
    } else {
        str2 = ArgToRootedString(cx, argc, vp, 0);
        if (!str2)
            return JS_FALSE;
    }
    pat = str2->chars();
    patlen = (jsint) str2->length();

    i = textlen - patlen; 
    if (i < 0) {
        vp->setInt32(-1);
        return JS_TRUE;
    }

    if (argc > 1) {
        if (vp[3].isInt32()) {
            j = vp[3].toInt32();
            if (j <= 0)
                i = 0;
            else if (j < i)
                i = j;
        } else {
            if (!ValueToNumber(cx, vp[3], &d))
                return JS_FALSE;
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
        return JS_TRUE;
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
            return JS_TRUE;
        }
      break_continue:;
    }

    vp->setInt32(-1);
    return JS_TRUE;
}

static JSBool
js_TrimString(JSContext *cx, Value *vp, JSBool trimLeft, JSBool trimRight)
{
    JSString *str;
    const jschar *chars;
    size_t length, begin, end;

    NORMALIZE_THIS(cx, vp, str);
    str->getCharsAndLength(chars, length);
    begin = 0;
    end = length;

    if (trimLeft) {
        while (begin < length && JS_ISSPACE(chars[begin]))
            ++begin;
    }

    if (trimRight) {
        while (end > begin && JS_ISSPACE(chars[end-1]))
            --end;
    }

    str = js_NewDependentString(cx, str, begin, end - begin);
    if (!str)
        return JS_FALSE;

    vp->setString(str);
    return JS_TRUE;
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
    JSString        *patstr;
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
    JSObject    *reobj_;
    RegExp      *re_;

    explicit RegExpPair(RegExp *re): re_(re) {}
    friend class RegExpGuard;

  public:
    
    JSObject *reobj() const { return reobj_; }
    RegExp &re() const { JS_ASSERT(re_); return *re_; }
};








class RegExpGuard
{
    RegExpGuard(const RegExpGuard &);
    void operator=(const RegExpGuard &);

    JSContext   *cx;
    RegExpPair  rep;
    FlatMatch   fm;

    



    static const size_t MAX_FLAT_PAT_LEN = 256;

    static JSString *flattenPattern(JSContext *cx, JSString *patstr) {
        JSCharBuffer cb(cx);
        if (!cb.reserve(patstr->length()))
            return NULL;

        static const jschar ESCAPE_CHAR = '\\';
        const jschar *chars = patstr->chars();
        size_t len = patstr->length();
        for (const jschar *it = chars; it != chars + len; ++it) {
            if (RegExp::isMetaChar(*it)) {
                if (!cb.append(ESCAPE_CHAR) || !cb.append(*it))
                    return NULL;
            } else {
                if (!cb.append(*it))
                    return NULL;
            }
        }
        return js_NewStringFromCharBuffer(cx, cb);
    }

  public:
    explicit RegExpGuard(JSContext *cx) : cx(cx), rep(NULL) {}

    ~RegExpGuard() {
        if (rep.re_)
            rep.re_->decref(cx);
    }

    
    bool
    init(uintN argc, Value *vp)
    {
        if (argc != 0 && VALUE_IS_REGEXP(cx, vp[2])) {
            rep.reobj_ = &vp[2].toObject();
            rep.re_ = RegExp::extractFrom(rep.reobj_);
            rep.re_->incref(cx);
        } else {
            fm.patstr = ArgToRootedString(cx, argc, vp, 0);
            if (!fm.patstr)
                return false;
        }
        return true;
    }

    






    const FlatMatch *
    tryFlatMatch(JSString *textstr, uintN optarg, uintN argc, bool checkMetaChars = true)
    {
        if (rep.re_)
            return NULL;

        fm.patstr->getCharsAndLength(fm.pat, fm.patlen);

        if (optarg < argc)
            return NULL;

        if (checkMetaChars &&
            (fm.patlen > MAX_FLAT_PAT_LEN || RegExp::hasMetaChars(fm.pat, fm.patlen))) {
            return NULL;
        }

        



        if (textstr->isTopNode()) {
            fm.match_ = RopeMatch(textstr, fm.pat, fm.patlen);
        } else {
            const jschar *text;
            size_t textlen;
            textstr->getCharsAndLength(text, textlen);
            fm.match_ = StringMatch(text, textlen, fm.pat, fm.patlen);
        }
        return &fm;
    }

    
    const RegExpPair *
    normalizeRegExp(bool flat, uintN optarg, uintN argc, Value *vp)
    {
        
        if (rep.re_)
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
                return false;
        } else {
            patstr = fm.patstr;
        }
        JS_ASSERT(patstr);

        rep.re_ = RegExp::createFlagged(cx, patstr, opt);
        if (!rep.re_)
            return NULL;
        rep.reobj_ = NULL;
        return &rep;
    }

#if DEBUG
    bool hasRegExpPair() const { return rep.re_; }
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
DoMatch(JSContext *cx, RegExpStatics *res, Value *vp, JSString *str, const RegExpPair &rep,
        DoMatchCallback callback, void *data, MatchControlFlags flags)
{
    RegExp &re = rep.re();
    if (re.global()) {
        
        bool testGlobal = flags & TEST_GLOBAL_BIT;
        if (rep.reobj())
            rep.reobj()->zeroRegExpLastIndex();
        for (size_t count = 0, i = 0, length = str->length(); i <= length; ++count) {
            if (!re.execute(cx, res, str, &i, testGlobal, vp))
                return false;
            if (!Matched(testGlobal, *vp))
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
        if (!re.execute(cx, res, str, &i, testSingle, vp))
            return false;
        if (callbackOnSingle && Matched(testSingle, *vp) && !callback(cx, res, 0, data))
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

    
    JSObject *obj = js_NewSlowArrayObject(cx);
    if (!obj)
        return false;
    vp->setObject(*obj);

    return obj->defineProperty(cx, INT_TO_JSID(0), StringValue(fm.pattern())) &&
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
        arrayobj = js_NewArrayObject(cx, 0, NULL);
        if (!arrayobj)
            return false;
    }

    Value v;
    if (!res->createLastMatch(cx, &v))
        return false;

    JSAutoResolveFlags rf(cx, JSRESOLVE_QUALIFIED | JSRESOLVE_ASSIGNING);
    return !!arrayobj->setProperty(cx, INT_TO_JSID(count), &v, false);
}

static JSBool
str_match(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    NORMALIZE_THIS(cx, vp, str);

    RegExpGuard g(cx);
    if (!g.init(argc, vp))
        return false;
    if (const FlatMatch *fm = g.tryFlatMatch(str, 1, argc))
        return BuildFlatMatchArray(cx, str, *fm, vp);

    const RegExpPair *rep = g.normalizeRegExp(false, 1, argc, vp);
    if (!rep)
        return false;

    AutoObjectRooter array(cx);
    MatchArgType arg = array.addr();
    RegExpStatics *res = cx->regExpStatics();
    if (!DoMatch(cx, res, vp, str, *rep, MatchCallback, arg, MATCH_ARGS))
        return false;

    
    if (rep->re().global())
        vp->setObjectOrNull(array.object());
    return true;
}

static JSBool
str_search(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    NORMALIZE_THIS(cx, vp, str);

    RegExpGuard g(cx);
    if (!g.init(argc, vp))
        return false;
    if (const FlatMatch *fm = g.tryFlatMatch(str, 1, argc)) {
        vp->setInt32(fm->match());
        return true;
    }
    const RegExpPair *rep = g.normalizeRegExp(false, 1, argc, vp);
    if (!rep)
        return false;

    RegExpStatics *res = cx->regExpStatics();
    size_t i = 0;
    if (!rep->re().execute(cx, res, str, &i, true, vp))
        return false;

    if (vp->isTrue())
        vp->setInt32(res->get(0, 0));
    else
        vp->setInt32(-1);
    return true;
}

struct ReplaceData
{
    ReplaceData(JSContext *cx)
     : g(cx), cb(cx)
    {}

    JSString           *str;           
    RegExpGuard        g;              
    JSObject           *lambda;        
    JSObject           *elembase;      
    JSString           *repstr;        
    jschar             *dollar;        
    jschar             *dollarEnd;     
    jsint              index;          
    jsint              leftIndex;      
    JSSubString        dollarStr;      
    bool               calledBack;     
    InvokeSessionGuard session;        
    InvokeArgsGuard    singleShot;     
    JSCharBuffer       cb;             
};

static bool
InterpretDollar(JSContext *cx, RegExpStatics *res, jschar *dp, jschar *ep, ReplaceData &rdata,
                JSSubString *out, size_t *skip)
{
    JS_ASSERT(*dp == '$');

    
    if (dp + 1 >= ep)
        return false;

    
    jschar dc = dp[1];
    if (JS7_ISDEC(dc)) {
        
        uintN num = JS7_UNDEC(dc);
        if (num > res->getParenCount())
            return false;

        jschar *cp = dp + 2;
        if (cp < ep && (dc = *cp, JS7_ISDEC(dc))) {
            uintN tmp = 10 * num + JS7_UNDEC(dc);
            if (tmp <= res->getParenCount()) {
                cp++;
                num = tmp;
            }
        }
        if (num == 0)
            return false;

        
        num--;
        *skip = cp - dp;
        if (num < res->getParenCount())
            res->getParen(num, out);
        else
            *out = js_EmptySubString;
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

class PreserveRegExpStatics
{
    js::RegExpStatics *const original;
    js::RegExpStatics buffer;

  public:
    explicit PreserveRegExpStatics(RegExpStatics *original)
     : original(original),
       buffer(RegExpStatics::InitBuffer())
    {}

    bool init(JSContext *cx) {
        return original->save(cx, &buffer);
    }

    ~PreserveRegExpStatics() {
        original->restore();
    }
};

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
        if (str->isAtomized()) {
            atom = STRING_TO_ATOM(str);
        } else {
            atom = js_AtomizeString(cx, str, 0);
            if (!atom)
                return false;
        }
        jsid id = ATOM_TO_JSID(atom);

        JSObject *holder;
        JSProperty *prop = NULL;
        if (js_LookupPropertyWithFlags(cx, base, id, JSRESOLVE_QUALIFIED, &holder, &prop) < 0)
            return false;

        
        if (prop && holder == base) {
            Shape *shape = (Shape *) prop;
            if (shape->slot != SHAPE_INVALID_SLOT && shape->hasDefaultGetter()) {
                Value value = base->getSlot(shape->slot);
                if (value.isString()) {
                    rdata.repstr = value.toString();
                    *sizep = rdata.repstr->length();
                    return true;
                }
            }
        }

        



        rdata.elembase = NULL;
    }

    JSObject *lambda = rdata.lambda;
    if (lambda) {
        







        uintN p = res->getParenCount();
        uintN argc = 1 + p + 2;

        InvokeSessionGuard &session = rdata.session;
        if (!session.started()) {
            Value lambdav = ObjectValue(*lambda);
            if (!session.start(cx, lambdav, UndefinedValue(), argc))
                return false;
        }

        PreserveRegExpStatics staticsGuard(res);
        if (!staticsGuard.init(cx))
            return false;

        
        uintN argi = 0;
        if (!res->createLastMatch(cx, &session[argi++]))
            return false;

        for (size_t i = 0; i < res->getParenCount(); ++i) {
            if (!res->createParen(cx, i, &session[argi++]))
                return false;
        }

        
        session[argi++].setInt32(res->get(0, 0));
        session[argi].setString(rdata.str);

        if (!session.invoke(cx))
            return false;

        
        rdata.repstr = ValueToString_TestForStringInline(cx, session.rval());
        if (!rdata.repstr)
            return false;

        *sizep = rdata.repstr->length();
        return true;
    }

    JSString *repstr = rdata.repstr;
    size_t replen = repstr->length();
    for (jschar *dp = rdata.dollar, *ep = rdata.dollarEnd; dp; dp = js_strchr_limit(dp, '$', ep)) {
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
DoReplace(JSContext *cx, RegExpStatics *res, ReplaceData &rdata, jschar *chars)
{
    JSString *repstr = rdata.repstr;
    jschar *cp;
    jschar *bp = cp = repstr->chars();
    for (jschar *dp = rdata.dollar, *ep = rdata.dollarEnd; dp; dp = js_strchr_limit(dp, '$', ep)) {
        size_t len = dp - cp;
        js_strncpy(chars, cp, len);
        chars += len;
        cp = dp;

        JSSubString sub;
        size_t skip;
        if (InterpretDollar(cx, res, dp, ep, rdata, &sub, &skip)) {
            len = sub.length;
            js_strncpy(chars, sub.chars, len);
            chars += len;
            cp += skip;
            dp += skip;
        } else {
            dp++;
        }
    }
    js_strncpy(chars, cp, repstr->length() - (cp - bp));
}

static bool
ReplaceCallback(JSContext *cx, RegExpStatics *res, size_t count, void *p)
{
    ReplaceData &rdata = *static_cast<ReplaceData *>(p);

    rdata.calledBack = true;
    JSString *str = rdata.str;
    size_t leftoff = rdata.leftIndex;
    const jschar *left = str->chars() + leftoff;
    size_t leftlen = res->get(0, 0) - leftoff;
    rdata.leftIndex = res->get(0, 1);

    size_t replen = 0;  
    if (!FindReplaceLength(cx, res, rdata, &replen))
        return false;

    size_t growth = leftlen + replen;
    if (!rdata.cb.growByUninitialized(growth))
        return false;

    jschar *chars = rdata.cb.begin() + rdata.index;
    rdata.index += growth;
    js_strncpy(chars, left, leftlen);
    chars += leftlen;
    DoReplace(cx, res, rdata, chars);
    return true;
}

static bool
BuildFlatReplacement(JSContext *cx, JSString *textstr, JSString *repstr,
                     const FlatMatch &fm, Value *vp)
{
    JSRopeBuilder builder(cx);
    size_t match = fm.match(); 
    size_t matchEnd = match + fm.patternLength();

    if (textstr->isTopNode()) {
        



        JSRopeLeafIterator iter(textstr);
        size_t pos = 0;
        for (JSString *str = iter.init(); str; str = iter.next()) {
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

    vp->setString(builder.getStr());
    return true;
}







static inline bool
BuildDollarReplacement(JSContext *cx, JSString *textstr, JSString *repstr,
                       const jschar *firstDollar, const FlatMatch &fm, Value *vp)
{
    JS_ASSERT(repstr->chars() <= firstDollar && firstDollar < repstr->chars() + repstr->length());
    size_t matchStart = fm.match();
    size_t matchLimit = matchStart + fm.patternLength();
    JSCharBuffer newReplaceChars(cx);

    






    if (!newReplaceChars.reserve(textstr->length() - fm.patternLength() + repstr->length()))
        return false;

    
    JS_ALWAYS_TRUE(newReplaceChars.append(repstr->chars(), firstDollar));

    
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

    JSString *newReplace = js_NewStringFromCharBuffer(cx, newReplaceChars);
    ENSURE(newReplace);

    JS_ASSERT(textstr->length() >= matchLimit);
    JSString *rightSide = js_NewDependentString(cx, textstr, matchLimit,
                                                textstr->length() - matchLimit);
    ENSURE(rightSide);

    JSRopeBuilder builder(cx);
    ENSURE(builder.append(leftSide) &&
           builder.append(newReplace) &&
           builder.append(rightSide));
#undef ENSURE

    vp->setString(builder.getStr());
    return true;
}

static inline bool
str_replace_regexp(JSContext *cx, uintN argc, Value *vp, ReplaceData &rdata)
{
    const RegExpPair *rep = rdata.g.normalizeRegExp(true, 2, argc, vp);
    if (!rep)
        return false;

    rdata.index = 0;
    rdata.leftIndex = 0;
    rdata.calledBack = false;

    RegExpStatics *res = cx->regExpStatics();
    if (!DoMatch(cx, res, vp, rdata.str, *rep, ReplaceCallback, &rdata, REPLACE_ARGS))
        return false;

    if (!rdata.calledBack) {
        
        vp->setString(rdata.str);
        return true;
    }

    JSSubString sub;
    res->getRightContext(&sub);
    if (!rdata.cb.append(sub.chars, sub.length))
        return false;

    JSString *retstr = js_NewStringFromCharBuffer(cx, rdata.cb);
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
    LeaveTrace(cx);

    JSString *matchStr = js_NewDependentString(cx, rdata.str, fm.match(), fm.patternLength());
    if (!matchStr)
        return false;

    
    static const uint32 lambdaArgc = 3;
    if (!cx->stack().pushInvokeArgs(cx, lambdaArgc, &rdata.singleShot))
        return false;

    CallArgs &args = rdata.singleShot;
    args.callee().setObject(*rdata.lambda);
    args.thisv().setUndefined();

    Value *sp = args.argv();
    sp[0].setString(matchStr);
    sp[1].setInt32(fm.match());
    sp[2].setString(rdata.str);

    if (!Invoke(cx, rdata.singleShot, 0))
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

    JSRopeBuilder builder(cx);
    if (!(builder.append(leftSide) &&
          builder.append(repstr) &&
          builder.append(rightSide))) {
        return false;
    }

    vp->setString(builder.getStr());
    return true;
}

JSBool
js::str_replace(JSContext *cx, uintN argc, Value *vp)
{
    ReplaceData rdata(cx);
    NORMALIZE_THIS(cx, vp, rdata.str);
    static const uint32 optarg = 2;

    
    if (argc >= optarg && js_IsCallable(vp[3])) {
        rdata.lambda = &vp[3].toObject();
        rdata.elembase = NULL;
        rdata.repstr = NULL;
        rdata.dollar = rdata.dollarEnd = NULL;

        if (rdata.lambda->isFunction()) {
            JSFunction *fun = rdata.lambda->getFunctionPrivate();
            if (fun->isInterpreted()) {
                







                JSScript *script = fun->u.i.script;
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

        
        if (!js_MakeStringImmutable(cx, rdata.repstr))
            return false;
        rdata.dollarEnd = rdata.repstr->chars() + rdata.repstr->length();
        rdata.dollar = js_strchr_limit(rdata.repstr->chars(), '$',
                                       rdata.dollarEnd);
    }

    if (!rdata.g.init(argc, vp))
        return false;

    









    const FlatMatch *fm = rdata.g.tryFlatMatch(rdata.str, optarg, argc, false);
    if (!fm) {
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










static jsint
find_split(JSContext *cx, RegExpStatics *res, JSString *str, js::RegExp *re, jsint *ip,
           JSSubString *sep)
{
    jsint i;
    size_t length;
    jschar *chars;

    










    i = *ip;
    length = str->length();
    if ((size_t)i > length)
        return -1;

    chars = str->chars();

    




    if (re) {
        size_t index;
        Value rval;

      again:
        
        index = (size_t)i;
        if (!re->execute(cx, res, str, &index, true, &rval))
            return -2;
        if (!rval.isTrue()) {
            
            sep->length = 1;
            return length;
        }
        i = (jsint)index;
        JS_ASSERT(sep);
        res->getLastMatch(sep);
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

    




    jsint match = StringMatch(chars + i, length - i, sep->chars, sep->length);
    return match == -1 ? length : match + i;
}

static JSBool
str_split(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    NORMALIZE_THIS(cx, vp, str);

    if (argc == 0) {
        Value v = StringValue(str);
        JSObject *aobj = js_NewArrayObject(cx, 1, &v);
        if (!aobj)
            return false;
        vp->setObject(*aobj);
        return true;
    }

    RegExp *re;
    JSSubString *sep, tmp;
    if (VALUE_IS_REGEXP(cx, vp[2])) {
        re = static_cast<RegExp *>(vp[2].toObject().getPrivate());
        sep = &tmp;

        
        sep->chars = NULL;
        sep->length = 0;
    } else {
        JSString *str2 = js_ValueToString(cx, vp[2]);
        if (!str2)
            return false;
        vp[2].setString(str2);

        



        str2->getCharsAndLength(tmp.chars, tmp.length);
        sep = &tmp;
        re = NULL;
    }

    
    uint32 limit = 0; 
    bool limited = (argc > 1) && !vp[3].isUndefined();
    if (limited) {
        jsdouble d;
        if (!ValueToNumber(cx, vp[3], &d))
            return false;

        
        limit = js_DoubleToECMAUint32(d);
        if (limit > str->length())
            limit = 1 + str->length();
    }

    AutoValueVector splits(cx);

    RegExpStatics *res = cx->regExpStatics();
    jsint i, j;
    uint32 len = i = 0;
    while ((j = find_split(cx, res, str, re, &i, sep)) >= 0) {
        if (limited && len >= limit)
            break;

        JSString *sub = js_NewDependentString(cx, str, i, size_t(j - i));
        if (!sub || !splits.append(StringValue(sub)))
            return false;
        len++;

        




        if (re && sep->chars) {
            for (uintN num = 0; num < res->getParenCount(); num++) {
                if (limited && len >= limit)
                    break;
                JSSubString parsub;
                res->getParen(num, &parsub);
                sub = js_NewStringCopyN(cx, parsub.chars, parsub.length);
                if (!sub || !splits.append(StringValue(sub)))
                    return false;
                len++;
            }
            sep->chars = NULL;
        }
        i = j + sep->length;
    }

    if (j == -2)
        return false;

    JSObject *aobj = js_NewArrayObject(cx, splits.length(), splits.begin());
    if (!aobj)
        return false;
    vp->setObject(*aobj);
    return true;
}

#if JS_HAS_PERL_SUBSTR
static JSBool
str_substr(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;
    jsdouble d;
    jsdouble length, begin, end;

    NORMALIZE_THIS(cx, vp, str);
    if (argc != 0) {
        if (!ValueToNumber(cx, vp[2], &d))
            return JS_FALSE;
        length = str->length();
        begin = js_DoubleToInteger(d);
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
            if (!ValueToNumber(cx, vp[3], &d))
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
    vp->setString(str);
    return JS_TRUE;
}
#endif 




static JSBool
str_concat(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str, *str2;
    Value *argv;
    uintN i;

    NORMALIZE_THIS(cx, vp, str);

    
    vp->setString(str);

    for (i = 0, argv = vp + 2; i < argc; i++) {
        str2 = js_ValueToString(cx, argv[i]);
        if (!str2)
            return JS_FALSE;
        argv[i].setString(str2);

        str = js_ConcatStrings(cx, str, str2);
        if (!str)
            return JS_FALSE;
        vp->setString(str);
    }

    return JS_TRUE;
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
                      ? JSString::getUnitString(cx, str, begin)
                      : js_NewDependentString(cx, str, begin, length);
                if (!str)
                    return JS_FALSE;
            }
            vp->setString(str);
            return JS_TRUE;
        }
    }

    JSString *str;
    NORMALIZE_THIS(cx, vp, str);

    if (argc != 0) {
        double begin, end, length;

        if (!ValueToNumber(cx, vp[2], &begin))
            return JS_FALSE;
        begin = js_DoubleToInteger(begin);
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
            if (!ValueToNumber(cx, vp[3], &end))
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
    vp->setString(str);
    return JS_TRUE;
}

#if JS_HAS_STR_HTML_HELPERS



static JSBool
tagify(JSContext *cx, const char *begin, JSString *param, const char *end,
       Value *vp)
{
    JSString *str;
    jschar *tagbuf;
    size_t beglen, endlen, parlen, taglen;
    size_t i, j;

    NORMALIZE_THIS(cx, vp, str);

    if (!end)
        end = begin;

    beglen = strlen(begin);
    taglen = 1 + beglen + 1;                            
    parlen = 0; 
    if (param) {
        parlen = param->length();
        taglen += 2 + parlen + 1;                       
    }
    endlen = strlen(end);
    taglen += str->length() + 2 + endlen + 1;    

    if (taglen >= ~(size_t)0 / sizeof(jschar)) {
        js_ReportAllocationOverflow(cx);
        return JS_FALSE;
    }

    tagbuf = (jschar *) cx->malloc((taglen + 1) * sizeof(jschar));
    if (!tagbuf)
        return JS_FALSE;

    j = 0;
    tagbuf[j++] = '<';
    for (i = 0; i < beglen; i++)
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
    for (i = 0; i < endlen; i++)
        tagbuf[j++] = (jschar)end[i];
    tagbuf[j++] = '>';
    JS_ASSERT(j == taglen);
    tagbuf[j] = 0;

    str = js_NewString(cx, tagbuf, taglen);
    if (!str) {
        js_free((char *)tagbuf);
        return JS_FALSE;
    }
    vp->setString(str);
    return JS_TRUE;
}

static JSBool
tagify_value(JSContext *cx, uintN argc, Value *vp,
             const char *begin, const char *end)
{
    JSString *param;

    param = ArgToRootedString(cx, argc, vp, 0);
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
    return JSString::getUnitString(cx, str, size_t(i));
}
#endif

JS_DEFINE_TRCINFO_1(str_concat,
    (3, (extern, STRING_RETRY, js_ConcatStrings, CONTEXT, THIS_STRING, STRING,
         1, nanojit::ACCSET_NONE)))

static const uint16 GENERIC_PRIMITIVE = JSFUN_GENERIC_NATIVE | JSFUN_PRIMITIVE_THIS;

static JSFunctionSpec string_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN("quote",             str_quote,             0,GENERIC_PRIMITIVE),
    JS_FN(js_toSource_str,     str_toSource,          0,JSFUN_PRIMITIVE_THIS),
#endif

    
    JS_FN(js_toString_str,     js_str_toString,       0,JSFUN_PRIMITIVE_THIS),
    JS_FN(js_valueOf_str,      js_str_toString,       0,JSFUN_PRIMITIVE_THIS),
    JS_FN(js_toJSON_str,       js_str_toString,       0,JSFUN_PRIMITIVE_THIS),
    JS_FN("substring",         str_substring,         2,GENERIC_PRIMITIVE),
    JS_FN("toLowerCase",       str_toLowerCase,       0,GENERIC_PRIMITIVE),
    JS_FN("toUpperCase",       str_toUpperCase,       0,GENERIC_PRIMITIVE),
    JS_FN("charAt",            js_str_charAt,         1,GENERIC_PRIMITIVE),
    JS_FN("charCodeAt",        js_str_charCodeAt,     1,GENERIC_PRIMITIVE),
    JS_FN("indexOf",           str_indexOf,           1,GENERIC_PRIMITIVE),
    JS_FN("lastIndexOf",       str_lastIndexOf,       1,GENERIC_PRIMITIVE),
    JS_FN("trim",              str_trim,              0,GENERIC_PRIMITIVE),
    JS_FN("trimLeft",          str_trimLeft,          0,GENERIC_PRIMITIVE),
    JS_FN("trimRight",         str_trimRight,         0,GENERIC_PRIMITIVE),
    JS_FN("toLocaleLowerCase", str_toLocaleLowerCase, 0,GENERIC_PRIMITIVE),
    JS_FN("toLocaleUpperCase", str_toLocaleUpperCase, 0,GENERIC_PRIMITIVE),
    JS_FN("localeCompare",     str_localeCompare,     1,GENERIC_PRIMITIVE),

    
    JS_FN("match",             str_match,             1,GENERIC_PRIMITIVE),
    JS_FN("search",            str_search,            1,GENERIC_PRIMITIVE),
    JS_FN("replace",           str_replace,           2,GENERIC_PRIMITIVE),
    JS_FN("split",             str_split,             2,GENERIC_PRIMITIVE),
#if JS_HAS_PERL_SUBSTR
    JS_FN("substr",            str_substr,            2,GENERIC_PRIMITIVE),
#endif

    
    JS_TN("concat",            str_concat,            1,GENERIC_PRIMITIVE, &str_concat_trcinfo),
    JS_FN("slice",             str_slice,             2,GENERIC_PRIMITIVE),

    
#if JS_HAS_STR_HTML_HELPERS
    JS_FN("bold",              str_bold,              0,JSFUN_PRIMITIVE_THIS),
    JS_FN("italics",           str_italics,           0,JSFUN_PRIMITIVE_THIS),
    JS_FN("fixed",             str_fixed,             0,JSFUN_PRIMITIVE_THIS),
    JS_FN("fontsize",          str_fontsize,          1,JSFUN_PRIMITIVE_THIS),
    JS_FN("fontcolor",         str_fontcolor,         1,JSFUN_PRIMITIVE_THIS),
    JS_FN("link",              str_link,              1,JSFUN_PRIMITIVE_THIS),
    JS_FN("anchor",            str_anchor,            1,JSFUN_PRIMITIVE_THIS),
    JS_FN("strike",            str_strike,            0,JSFUN_PRIMITIVE_THIS),
    JS_FN("small",             str_small,             0,JSFUN_PRIMITIVE_THIS),
    JS_FN("big",               str_big,               0,JSFUN_PRIMITIVE_THIS),
    JS_FN("blink",             str_blink,             0,JSFUN_PRIMITIVE_THIS),
    JS_FN("sup",               str_sup,               0,JSFUN_PRIMITIVE_THIS),
    JS_FN("sub",               str_sub,               0,JSFUN_PRIMITIVE_THIS),
#endif

    JS_FS_END
};








#define R2(n)  R(n),   R((n) + (1 << 0)),    R((n) + (2 << 0)),    R((n) + (3 << 0))
#define R4(n)  R2(n),  R2((n) + (1 << 2)),   R2((n) + (2 << 2)),   R2((n) + (3 << 2))
#define R6(n)  R4(n),  R4((n) + (1 << 4)),   R4((n) + (2 << 4)),   R4((n) + (3 << 4))
#define R8(n)  R6(n),  R6((n) + (1 << 6)),   R6((n) + (2 << 6)),   R6((n) + (3 << 6))
#define R10(n) R8(n),  R8((n) + (1 << 8)),   R8((n) + (2 << 8)),   R8((n) + (3 << 8))
#define R12(n) R10(n), R10((n) + (1 << 10)), R10((n) + (2 << 10)), R10((n) + (3 << 10))

#define R3(n) R2(n), R2((n) + (1 << 2))
#define R7(n) R6(n), R6((n) + (1 << 6))





#define R(c) {                                                                \
    JSString::FLAT | JSString::ATOMIZED | (1 << JSString::FLAGS_LENGTH_SHIFT),\
    { (jschar *)(((char *)(unitStringTable + (c))) +                          \
      offsetof(JSString, mInlineStorage)) },                                  \
    { {(c), 0x00} } }

#ifdef __SUNPRO_CC
#pragma pack(8)
#else
#pragma pack(push, 8)
#endif

const JSString JSString::unitStringTable[]
#ifdef __GNUC__
__attribute__ ((aligned (8)))
#endif
= { R8(0) };

#ifdef __SUNPRO_CC
#pragma pack(0)
#else
#pragma pack(pop)
#endif

#undef R






#define TO_SMALL_CHAR(c) ((c) >= '0' && (c) <= '9' ? (c) - '0' :              \
                          (c) >= 'a' && (c) <= 'z' ? (c) - 'a' + 10 :         \
                          (c) >= 'A' && (c) <= 'Z' ? (c) - 'A' + 36 :         \
                          JSString::INVALID_SMALL_CHAR)

#define R TO_SMALL_CHAR

const JSString::SmallChar JSString::toSmallChar[] = { R7(0) };

#undef R





#define FROM_SMALL_CHAR(c) ((c) + ((c) < 10 ? '0' :      \
                                   (c) < 36 ? 'a' - 10 : \
                                   'A' - 36))
#define R FROM_SMALL_CHAR

const jschar JSString::fromSmallChar[] = { R6(0) };

#undef R






#define R(c) {                                                                \
    JSString::FLAT | JSString::ATOMIZED | (2 << JSString::FLAGS_LENGTH_SHIFT),\
    { (jschar *)(((char *)(length2StringTable + (c))) +                       \
      offsetof(JSString, mInlineStorage)) },                                  \
    { {FROM_SMALL_CHAR((c) >> 6), FROM_SMALL_CHAR((c) & 0x3F), 0x00} } }

#ifdef __SUNPRO_CC
#pragma pack(8)
#else
#pragma pack(push, 8)
#endif

const JSString JSString::length2StringTable[]
#ifdef __GNUC__
__attribute__ ((aligned (8)))
#endif
= { R12(0) };

#ifdef __SUNPRO_CC
#pragma pack(0)
#else
#pragma pack(pop)
#endif

#undef R

#define R(c) FROM_SMALL_CHAR((c) >> 6), FROM_SMALL_CHAR((c) & 0x3f), 0x00

const char JSString::deflatedLength2StringTable[] = { R12(0) };

#undef R








#define R(c) {                                                                \
    JSString::FLAT | JSString::ATOMIZED | (3 << JSString::FLAGS_LENGTH_SHIFT),\
    { (jschar *)(((char *)(hundredStringTable + ((c) - 100))) +               \
      offsetof(JSString, mInlineStorage)) },                                  \
    { {((c) / 100) + '0', ((c) / 10 % 10) + '0', ((c) % 10) + '0', 0x00} } }


JS_STATIC_ASSERT(100 + (1 << 7) + (1 << 4) + (1 << 3) + (1 << 2) == 256);

#ifdef __SUNPRO_CC
#pragma pack(8)
#else
#pragma pack(push, 8)
#endif

const JSString JSString::hundredStringTable[]
#ifdef __GNUC__
__attribute__ ((aligned (8)))
#endif
= { R7(100), 
    R4(100 + (1 << 7)), 
    R3(100 + (1 << 7) + (1 << 4)), 
    R2(100 + (1 << 7) + (1 << 4) + (1 << 3)) 
};

#undef R

#define R(c) ((c) < 10 ? JSString::unitStringTable + ((c) + '0') :            \
              (c) < 100 ? JSString::length2StringTable +                      \
              ((size_t)TO_SMALL_CHAR(((c) / 10) + '0') << 6) +                \
              TO_SMALL_CHAR(((c) % 10) + '0') :                               \
              JSString::hundredStringTable + ((c) - 100))

const JSString *const JSString::intStringTable[] = { R8(0) };

#undef R

#ifdef __SUNPRO_CC
#pragma pack(0)
#else
#pragma pack(pop)
#endif

#define R(c) ((c) / 100) + '0', ((c) / 10 % 10) + '0', ((c) % 10) + '0', 0x00

const char JSString::deflatedIntStringTable[] = {
    R7(100), 
    R4(100 + (1 << 7)), 
    R3(100 + (1 << 7) + (1 << 4)), 
    R2(100 + (1 << 7) + (1 << 4) + (1 << 3)) 
};

#undef R
#undef R2
#undef R4
#undef R6
#undef R8
#undef R10
#undef R12

#undef R3
#undef R7


#define U8(c)   char(((c) >> 6) | 0xc0), char(((c) & 0x3f) | 0x80), 0
#define U(c)    U8(c), U8(c+1), U8(c+2), U8(c+3), U8(c+4), U8(c+5), U8(c+6), U8(c+7)

const char JSString::deflatedUnitStringTable[] = {
    U(0x80), U(0x88), U(0x90), U(0x98), U(0xa0), U(0xa8), U(0xb0), U(0xb8),
    U(0xc0), U(0xc8), U(0xd0), U(0xd8), U(0xe0), U(0xe8), U(0xf0), U(0xf8)
};

#undef U
#undef U8

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
        JSObject *obj = NewBuiltinClassInstance(cx, &js_StringClass);
        if (!obj)
            return false;
        obj->setPrimitiveThis(StringValue(str));
        vp->setObject(*obj);
    } else {
        vp->setString(str);
    }
    return true;
}

static JSBool
str_fromCharCode(JSContext *cx, uintN argc, Value *vp)
{
    Value *argv;
    uintN i;
    jschar *chars;
    JSString *str;

    argv = vp + 2;
    JS_ASSERT(argc <= JS_ARGS_LENGTH_MAX);
    if (argc == 1) {
        uint16_t code;
        if (!ValueToUint16(cx, argv[0], &code))
            return JS_FALSE;
        if (code < UNIT_STRING_LIMIT) {
            str = JSString::unitString(code);
            if (!str)
                return JS_FALSE;
            vp->setString(str);
            return JS_TRUE;
        }
        argv[0].setInt32(code);
    }
    chars = (jschar *) cx->malloc((argc + 1) * sizeof(jschar));
    if (!chars)
        return JS_FALSE;
    for (i = 0; i < argc; i++) {
        uint16_t code;
        if (!ValueToUint16(cx, argv[i], &code)) {
            cx->free(chars);
            return JS_FALSE;
        }
        chars[i] = (jschar)code;
    }
    chars[i] = 0;
    str = js_NewString(cx, chars, argc);
    if (!str) {
        cx->free(chars);
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
    if (c < UNIT_STRING_LIMIT)
        return JSString::unitString(c);
    return js_NewStringCopyN(cx, &c, 1);
}
#endif

JS_DEFINE_TRCINFO_1(str_fromCharCode,
    (2, (static, STRING_RETRY, String_fromCharCode, CONTEXT, INT32, 1, nanojit::ACCSET_NONE)))

static JSFunctionSpec string_static_methods[] = {
    JS_TN("fromCharCode", str_fromCharCode, 1, 0, &str_fromCharCode_trcinfo),
    JS_FS_END
};

JSObject *
js_InitStringClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    
    if (!JS_DefineFunctions(cx, obj, string_functions))
        return NULL;

    proto = js_InitClass(cx, obj, NULL, &js_StringClass, js_String, 1,
                         NULL, string_methods,
                         NULL, string_static_methods);
    if (!proto)
        return NULL;
    proto->setPrimitiveThis(StringValue(cx->runtime->emptyString));
    if (!js_DefineNativeProperty(cx, proto, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom),
                                 UndefinedValue(), NULL, NULL,
                                 JSPROP_READONLY | JSPROP_PERMANENT | JSPROP_SHARED, 0, 0,
                                 NULL)) {
        return JS_FALSE;
    }

    return proto;
}

JSString *
js_NewString(JSContext *cx, jschar *chars, size_t length)
{
    JSString *str;

    if (length > JSString::MAX_LENGTH) {
        if (JS_ON_TRACE(cx)) {
            



            if (!CanLeaveTrace(cx))
                return NULL;

            LeaveTrace(cx);
        }
        js_ReportAllocationOverflow(cx);
        return NULL;
    }

    str = js_NewGCString(cx);
    if (!str)
        return NULL;
    str->initFlat(chars, length);
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

static JS_ALWAYS_INLINE JSString *
NewShortString(JSContext *cx, const jschar *chars, size_t length)
{
    JS_ASSERT(JSShortString::fitsIntoShortString(length));
    JSShortString *str = js_NewGCShortString(cx);
    if (!str)
        return NULL;
    jschar *storage = str->init(length);
    js_short_strncpy(storage, chars, length);
    storage[length] = 0;
    return str->header();
}

static JSString *
NewShortString(JSContext *cx, const char *chars, size_t length)
{
    JS_ASSERT(JSShortString::fitsIntoShortString(length));
    JSShortString *str = js_NewGCShortString(cx);
    if (!str)
        return NULL;
    jschar *storage = str->init(length);

    if (js_CStringsAreUTF8) {
#ifdef DEBUG
        size_t oldLength = length;
#endif
        if (!js_InflateStringToBuffer(cx, chars, length, storage, &length))
            return NULL;
        JS_ASSERT(length <= oldLength);
        storage[length] = 0;
        str->resetLength(length);
    } else {
        size_t n = length;
        jschar *p = storage;
        while (n--)
            *p++ = jschar(*chars++);
        *p = 0;
    }
    return str->header();
}

static const size_t sMinWasteSize = 16;

JSString *
js_NewStringFromCharBuffer(JSContext *cx, JSCharBuffer &cb)
{
    if (cb.empty())
        return ATOM_TO_STRING(cx->runtime->atomState.emptyAtom);

    size_t length = cb.length();

    JS_STATIC_ASSERT(JSShortString::MAX_SHORT_STRING_LENGTH < JSCharBuffer::InlineLength);
    if (JSShortString::fitsIntoShortString(length))
        return NewShortString(cx, cb.begin(), length);

    if (!cb.append('\0'))
        return NULL;

    size_t capacity = cb.capacity();

    jschar *buf = cb.extractRawBuffer();
    if (!buf)
        return NULL;

    
    JS_ASSERT(capacity >= length);
    if (capacity > sMinWasteSize && capacity - length > (length >> 2)) {
        size_t bytes = sizeof(jschar) * (length + 1);
        jschar *tmp = (jschar *)cx->realloc(buf, bytes);
        if (!tmp) {
            cx->free(buf);
            return NULL;
        }
        buf = tmp;
    }

    JSString *str = js_NewString(cx, buf, length);
    if (!str)
        cx->free(buf);
    return str;
}

JSString *
js_NewDependentString(JSContext *cx, JSString *base, size_t start,
                      size_t length)
{
    JSString *ds;

    if (length == 0)
        return cx->runtime->emptyString;

    if (start == 0 && length == base->length())
        return base;

    jschar *chars = base->chars() + start;

    JSString *staticStr = JSString::lookupStaticString(chars, length);
    if (staticStr)
        return staticStr;

    
    while (base->isDependent())
        base = base->dependentBase();

    JS_ASSERT(base->isFlat());

    ds = js_NewGCString(cx);
    if (!ds)
        return NULL;
    ds->initDependent(base, chars, length);
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
    return ds;
}

#ifdef DEBUG
#include <math.h>

void printJSStringStats(JSRuntime *rt)
{
    double mean, sigma;

    mean = JS_MeanAndStdDev(rt->totalStrings, rt->lengthSum,
                            rt->lengthSquaredSum, &sigma);

    fprintf(stderr, "%lu total strings, mean length %g (sigma %g)\n",
            (unsigned long)rt->totalStrings, mean, sigma);

    mean = JS_MeanAndStdDev(rt->totalDependentStrings, rt->strdepLengthSum,
                            rt->strdepLengthSquaredSum, &sigma);

    fprintf(stderr, "%lu total dependent strings, mean length %g (sigma %g)\n",
            (unsigned long)rt->totalDependentStrings, mean, sigma);
}
#endif

JSString *
js_NewStringCopyN(JSContext *cx, const jschar *s, size_t n)
{
    if (JSShortString::fitsIntoShortString(n))
        return NewShortString(cx, s, n);

    jschar *news;
    JSString *str;

    news = (jschar *) cx->malloc((n + 1) * sizeof(jschar));
    if (!news)
        return NULL;
    js_strncpy(news, s, n);
    news[n] = 0;
    str = js_NewString(cx, news, n);
    if (!str)
        cx->free(news);
    return str;
}

JSString *
js_NewStringCopyN(JSContext *cx, const char *s, size_t n)
{
    if (JSShortString::fitsIntoShortString(n))
        return NewShortString(cx, s, n);
    return JS_NewStringCopyN(cx, s, n);
}

JSString *
js_NewStringCopyZ(JSContext *cx, const jschar *s)
{
    size_t n, m;
    jschar *news;
    JSString *str;

    n = js_strlen(s);

    if (JSShortString::fitsIntoShortString(n))
        return NewShortString(cx, s, n);

    m = (n + 1) * sizeof(jschar);
    news = (jschar *) cx->malloc(m);
    if (!news)
        return NULL;
    memcpy(news, s, m);
    str = js_NewString(cx, news, n);
    if (!str)
        cx->free(news);
    return str;
}

JSString *
js_NewStringCopyZ(JSContext *cx, const char *s)
{
    return js_NewStringCopyN(cx, s, strlen(s));
}

JS_FRIEND_API(const char *)
js_ValueToPrintable(JSContext *cx, const Value &v, JSValueToStringFun v2sfun)
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

JSString *
js_ValueToString(JSContext *cx, const Value &arg)
{
    Value v = arg;
    if (v.isObject() && !DefaultValue(cx, &v.toObject(), JSTYPE_STRING, &v))
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
        str = ATOM_TO_STRING(cx->runtime->atomState.nullAtom);
    } else {
        str = ATOM_TO_STRING(cx->runtime->atomState.typeAtoms[JSTYPE_VOID]);
    }
    return str;
}

static inline JSBool
AppendAtom(JSAtom *atom, JSCharBuffer &cb)
{
    JSString *str = ATOM_TO_STRING(atom);
    const jschar *chars;
    size_t length;
    str->getCharsAndLength(chars, length);
    return cb.append(chars, length);
}


JSBool
js_ValueToCharBuffer(JSContext *cx, const Value &arg, JSCharBuffer &cb)
{
    Value v = arg;
    if (v.isObject() && !DefaultValue(cx, &v.toObject(), JSTYPE_STRING, &v))
        return JS_FALSE;

    if (v.isString()) {
        const jschar *chars;
        size_t length;
        v.toString()->getCharsAndLength(chars, length);
        return cb.append(chars, length);
    }
    if (v.isNumber())
        return js_NumberValueToCharBuffer(cx, v, cb);
    if (v.isBoolean())
        return js_BooleanToCharBuffer(cx, v.toBoolean(), cb);
    if (v.isNull())
        return AppendAtom(cx->runtime->atomState.nullAtom, cb);
    JS_ASSERT(v.isUndefined());
    return AppendAtom(cx->runtime->atomState.typeAtoms[JSTYPE_VOID], cb);
}

JS_FRIEND_API(JSString *)
js_ValueToSource(JSContext *cx, const Value &v)
{
    if (v.isUndefined())
        return ATOM_TO_STRING(cx->runtime->atomState.void0Atom);
    if (v.isString())
        return js_QuoteString(cx, v.toString(), '"');
    if (v.isPrimitive()) {
        
        if (v.isDouble() && JSDOUBLE_IS_NEGZERO(v.toDouble())) {
            
            static const jschar js_negzero_ucNstr[] = {'-', '0'};

            return js_NewStringCopyN(cx, js_negzero_ucNstr, 2);
        }
        return js_ValueToString(cx, v);
    }

    JSAtom *atom = cx->runtime->atomState.toSourceAtom;
    AutoValueRooter tvr(cx);
    if (!js_TryMethod(cx, &v.toObject(), atom, 0, NULL, tvr.addr()))
        return NULL;
    return js_ValueToString(cx, tvr.value());
}




uint32
js_HashString(JSString *str)
{
    const jschar *s;
    size_t n;
    uint32 h;

    str->getCharsAndLength(s, n);
    for (h = 0; n; s++, n--)
        h = JS_ROTATE_LEFT32(h, 4) ^ *s;
    return h;
}




JSBool JS_FASTCALL
js_EqualStrings(JSString *str1, JSString *str2)
{
    size_t n;
    const jschar *s1, *s2;

    JS_ASSERT(str1);
    JS_ASSERT(str2);

    
    if (str1 == str2)
        return JS_TRUE;

    n = str1->length();
    if (n != str2->length())
        return JS_FALSE;

    if (n == 0)
        return JS_TRUE;

    s1 = str1->chars(), s2 = str2->chars();
    do {
        if (*s1 != *s2)
            return JS_FALSE;
        ++s1, ++s2;
    } while (--n != 0);

    return JS_TRUE;
}
JS_DEFINE_CALLINFO_2(extern, BOOL, js_EqualStrings, STRING, STRING, 1, nanojit::ACCSET_NONE)

int32 JS_FASTCALL
js_CompareStrings(JSString *str1, JSString *str2)
{
    size_t l1, l2, n, i;
    const jschar *s1, *s2;
    intN cmp;

    JS_ASSERT(str1);
    JS_ASSERT(str2);

    
    if (str1 == str2)
        return 0;

    str1->getCharsAndLength(s1, l1);
    str2->getCharsAndLength(s2, l2);
    n = JS_MIN(l1, l2);
    for (i = 0; i < n; i++) {
        cmp = s1[i] - s2[i];
        if (cmp != 0)
            return cmp;
    }
    return (intN)(l1 - l2);
}
JS_DEFINE_CALLINFO_2(extern, INT32, js_CompareStrings, STRING, STRING, 1, nanojit::ACCSET_NONE)

namespace js {

JSBool
MatchStringAndAscii(JSString *str, const char *asciiBytes)
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

jschar *
js_InflateString(JSContext *cx, const char *bytes, size_t *lengthp)
{
    size_t nbytes, nchars, i;
    jschar *chars;
#ifdef DEBUG
    JSBool ok;
#endif

    nbytes = *lengthp;
    if (js_CStringsAreUTF8) {
        if (!js_InflateStringToBuffer(cx, bytes, nbytes, NULL, &nchars))
            goto bad;
        chars = (jschar *) cx->malloc((nchars + 1) * sizeof (jschar));
        if (!chars)
            goto bad;
#ifdef DEBUG
        ok =
#endif
            js_InflateStringToBuffer(cx, bytes, nbytes, chars, &nchars);
        JS_ASSERT(ok);
    } else {
        nchars = nbytes;
        chars = (jschar *) cx->malloc((nchars + 1) * sizeof(jschar));
        if (!chars)
            goto bad;
        for (i = 0; i < nchars; i++)
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
js_DeflateString(JSContext *cx, const jschar *chars, size_t nchars)
{
    size_t nbytes, i;
    char *bytes;
#ifdef DEBUG
    JSBool ok;
#endif

    if (js_CStringsAreUTF8) {
        nbytes = js_GetDeflatedStringLength(cx, chars, nchars);
        if (nbytes == (size_t) -1)
            return NULL;
        bytes = (char *) (cx ? cx->malloc(nbytes + 1) : js_malloc(nbytes + 1));
        if (!bytes)
            return NULL;
#ifdef DEBUG
        ok =
#endif
            js_DeflateStringToBuffer(cx, chars, nchars, bytes, &nbytes);
        JS_ASSERT(ok);
    } else {
        nbytes = nchars;
        bytes = (char *) (cx ? cx->malloc(nbytes + 1) : js_malloc(nbytes + 1));
        if (!bytes)
            return NULL;
        for (i = 0; i < nbytes; i++)
            bytes[i] = (char) chars[i];
    }
    bytes[nbytes] = 0;
    return bytes;
}

size_t
js_GetDeflatedStringLength(JSContext *cx, const jschar *chars, size_t nchars)
{
    if (!js_CStringsAreUTF8)
        return nchars;

    return js_GetDeflatedUTF8StringLength(cx, chars, nchars);
}




size_t
js_GetDeflatedUTF8StringLength(JSContext *cx, const jschar *chars, size_t nchars)
{
    size_t nbytes;
    const jschar *end;
    uintN c, c2;
    char buffer[10];

    nbytes = nchars;
    for (end = chars + nchars; chars != end; chars++) {
        c = *chars;
        if (c < 0x80)
            continue;
        if (0xD800 <= c && c <= 0xDFFF) {
            
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

JSBool
js_DeflateStringToBuffer(JSContext *cx, const jschar *src, size_t srclen,
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

    return js_DeflateStringToUTF8Buffer(cx, src, srclen, dst, dstlenp);
}

JSBool
js_DeflateStringToUTF8Buffer(JSContext *cx, const jschar *src, size_t srclen,
                             char *dst, size_t *dstlenp)
{
    size_t dstlen, i, origDstlen, utf8Len;
    jschar c, c2;
    uint32 v;
    uint8 utf8buf[6];

    dstlen = *dstlenp;
    origDstlen = dstlen;
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
    size_t dstlen, i;

    if (!js_CStringsAreUTF8) {
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

    return js_InflateUTF8StringToBuffer(cx, src, srclen, dst, dstlenp);
}

JSBool
js_InflateUTF8StringToBuffer(JSContext *cx, const char *src, size_t srclen,
                             jschar *dst, size_t *dstlenp)
{
    size_t dstlen, origDstlen, offset, j, n;
    uint32 v;

    dstlen = dst ? *dstlenp : (size_t) -1;
    origDstlen = dstlen;
    offset = 0;

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

namespace js {

DeflatedStringCache::DeflatedStringCache()
{
#ifdef JS_THREADSAFE
    lock = NULL;
#endif
}

bool
DeflatedStringCache::init()
{
#ifdef JS_THREADSAFE
    JS_ASSERT(!lock);
    lock = JS_NEW_LOCK();
    if (!lock)
        return false;
#endif

    



    return map.init(2048);
}

DeflatedStringCache::~DeflatedStringCache()
{
#ifdef JS_THREADSAFE
    if (lock)
        JS_DESTROY_LOCK(lock);
#endif
}

void
DeflatedStringCache::sweep(JSContext *cx)
{
    



    JS_ACQUIRE_LOCK(lock);

    for (Map::Enum e(map); !e.empty(); e.popFront()) {
        JSString *str = e.front().key;
        if (IsAboutToBeFinalized(str)) {
            char *bytes = e.front().value;
            e.removeFront();

            





            js_free(bytes);
        }
    }

    JS_RELEASE_LOCK(lock);
}

void
DeflatedStringCache::remove(JSString *str)
{
    JS_ACQUIRE_LOCK(lock);

    Map::Ptr p = map.lookup(str);
    if (p) {
        js_free(p->value);
        map.remove(p);
    }

    JS_RELEASE_LOCK(lock);
}

bool
DeflatedStringCache::setBytes(JSContext *cx, JSString *str, char *bytes)
{
    JS_ACQUIRE_LOCK(lock);

    Map::AddPtr p = map.lookupForAdd(str);
    JS_ASSERT(!p);
    bool ok = map.add(p, str, bytes);

    JS_RELEASE_LOCK(lock);

    if (!ok)
        js_ReportOutOfMemory(cx);
    return ok;
}

char *
DeflatedStringCache::getBytes(JSContext *cx, JSString *str)
{
    JS_ACQUIRE_LOCK(lock);
    Map::AddPtr p = map.lookupForAdd(str);
    char *bytes = p ? p->value : NULL;
    JS_RELEASE_LOCK(lock);

    if (bytes)
        return bytes;

    bytes = js_DeflateString(cx, str->chars(), str->length());
    if (!bytes)
        return NULL;

    





    char *bytesToFree = NULL;
    JSBool ok;
#ifdef JS_THREADSAFE
    JS_ACQUIRE_LOCK(lock);
    ok = map.relookupOrAdd(p, str, bytes);
    if (ok && p->value != bytes) {
        
        JS_ASSERT(!strcmp(p->value, bytes));
        bytesToFree = bytes;
        bytes = p->value;
    }
    JS_RELEASE_LOCK(lock);
#else  
    ok = map.add(p, str, bytes);
#endif
    if (!ok) {
        bytesToFree = bytes;
        bytes = NULL;
        if (cx)
            js_ReportOutOfMemory(cx);
    }

    if (bytesToFree) {
        if (cx)
            cx->free(bytesToFree);
        else
            js_free(bytesToFree);
    }
    return bytes;
}

} 

const char *
js_GetStringBytes(JSContext *cx, JSString *str)
{
    JSRuntime *rt;
    char *bytes;

    if (JSString::isUnitString(str)) {
#ifdef IS_LITTLE_ENDIAN
        
        bytes = (char *)str->chars();
#else
        
        bytes = (char *)str->chars() + 1;
#endif
        return ((*bytes & 0x80) && js_CStringsAreUTF8)
               ? JSString::deflatedUnitStringTable + ((*bytes & 0x7f) * 3)
               : bytes;
    }

    



    if (JSString::isLength2String(str))
        return JSString::deflatedLength2StringTable + ((str - JSString::length2StringTable) * 3);

    if (JSString::isHundredString(str)) {
        



        return JSString::deflatedIntStringTable + ((str - JSString::hundredStringTable) * 4);
    }

    if (cx) {
        rt = cx->runtime;
    } else {
        
        rt = GetGCThingRuntime(str);
    }

    return rt->deflatedStringCache->getBytes(cx, str);
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





const bool js_alnum[] = {

 false, false, false, false, false, false, false, false, false, false,
 false, false, false, false, false, false, false, false, false, false,
 false, false, false, false, false, false, false, false, false, false,
 false, false, false, false, false, false, false, false, false, false,
 false, false, false, false, false, false, false, false, true,  true,
 true,  true,  true,  true,  true,  true,  true,  true,  false, false,
 false, false, false, false, false, true,  true,  true,  true,  true,
 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
 true,  false, false, false, false, true,  false, true,  true,  true,
 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
 true,  true,  true,  true,  true,  true,  true,  true,  true,  true,
 true,  true,  true,  false, false, false, false, false
};

#define URI_CHUNK 64U

static inline bool
TransferBufferToString(JSContext *cx, JSCharBuffer &cb, Value *rval)
{
    JSString *str = js_NewStringFromCharBuffer(cx, cb);
    if (!str)
        return false;
    rval->setString(str);
    return true;;
}








static JSBool
Encode(JSContext *cx, JSString *str, const jschar *unescapedSet,
       const jschar *unescapedSet2, Value *rval)
{
    size_t length, j, k, L;
    JSCharBuffer cb(cx);
    const jschar *chars;
    jschar c, c2;
    uint32 v;
    uint8 utf8buf[4];
    jschar hexBuf[4];
    static const char HexDigits[] = "0123456789ABCDEF"; 

    str->getCharsAndLength(chars, length);
    if (length == 0) {
        rval->setString(cx->runtime->emptyString);
        return JS_TRUE;
    }

    
    hexBuf[0] = '%';
    hexBuf[3] = 0;
    for (k = 0; k < length; k++) {
        c = chars[k];
        if (js_strchr(unescapedSet, c) ||
            (unescapedSet2 && js_strchr(unescapedSet2, c))) {
            if (!cb.append(c))
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
                if (!cb.append(hexBuf, 3))
                    return JS_FALSE;
            }
        }
    }

    return TransferBufferToString(cx, cb, rval);
}

static JSBool
Decode(JSContext *cx, JSString *str, const jschar *reservedSet, Value *rval)
{
    size_t length, start, k;
    JSCharBuffer cb(cx);
    const jschar *chars;
    jschar c, H;
    uint32 v;
    jsuint B;
    uint8 octets[4];
    intN j, n;

    str->getCharsAndLength(chars, length);
    if (length == 0) {
        rval->setString(cx->runtime->emptyString);
        return JS_TRUE;
    }

    
    for (k = 0; k < length; k++) {
        c = chars[k];
        if (c == '%') {
            start = k;
            if ((k + 2) >= length)
                goto report_bad_uri;
            if (!JS7_ISHEX(chars[k+1]) || !JS7_ISHEX(chars[k+2]))
                goto report_bad_uri;
            B = JS7_UNHEX(chars[k+1]) * 16 + JS7_UNHEX(chars[k+2]);
            k += 2;
            if (!(B & 0x80)) {
                c = (jschar)B;
            } else {
                n = 1;
                while (B & (0x80 >> n))
                    n++;
                if (n == 1 || n > 4)
                    goto report_bad_uri;
                octets[0] = (uint8)B;
                if (k + 3 * (n - 1) >= length)
                    goto report_bad_uri;
                for (j = 1; j < n; j++) {
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
                v = Utf8ToOneUcs4Char(octets, n);
                if (v >= 0x10000) {
                    v -= 0x10000;
                    if (v > 0xFFFFF)
                        goto report_bad_uri;
                    c = (jschar)((v & 0x3FF) + 0xDC00);
                    H = (jschar)((v >> 10) + 0xD800);
                    if (!cb.append(H))
                        return JS_FALSE;
                } else {
                    c = (jschar)v;
                }
            }
            if (js_strchr(reservedSet, c)) {
                if (!cb.append(chars + start, k - start + 1))
                    return JS_FALSE;
            } else {
                if (!cb.append(c))
                    return JS_FALSE;
            }
        } else {
            if (!cb.append(c))
                return JS_FALSE;
        }
    }

    return TransferBufferToString(cx, cb, rval);

  report_bad_uri:
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_URI);
    

    return JS_FALSE;
}

static JSBool
str_decodeURI(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Decode(cx, str, js_uriReservedPlusPound_ucstr, vp);
}

static JSBool
str_decodeURI_Component(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Decode(cx, str, js_empty_ucstr, vp);
}

static JSBool
str_encodeURI(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    str = ArgToRootedString(cx, argc, vp, 0);
    if (!str)
        return JS_FALSE;
    return Encode(cx, str, js_uriReservedPlusPound_ucstr, js_uriUnescaped_ucstr,
                  vp);
}

static JSBool
str_encodeURI_Component(JSContext *cx, uintN argc, Value *vp)
{
    JSString *str;

    str = ArgToRootedString(cx, argc, vp, 0);
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
    uint32 ucs4Char;
    uint32 minucs4Char;
    
    static const uint32 minucs4Table[] = {
        0x00000080, 0x00000800, 0x00010000
    };

    JS_ASSERT(utf8Length >= 1 && utf8Length <= 4);
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
        if (JS_UNLIKELY(ucs4Char < minucs4Char)) {
            ucs4Char = OVERLONG_UTF8;
        } else if (ucs4Char == 0xFFFE || ucs4Char == 0xFFFF) {
            ucs4Char = 0xFFFD;
        }
    }
    return ucs4Char;
}

namespace js {

size_t
PutEscapedStringImpl(char *buffer, size_t bufferSize, FILE *fp, JSString *str, uint32 quote)
{
    const jschar *chars, *charsEnd;
    size_t n;
    const char *escape;
    char c;
    uintN u, hex, shift;
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

    str->getCharsAndEnd(chars, charsEnd);
    n = 0;
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
