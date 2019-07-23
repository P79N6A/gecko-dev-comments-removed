










































#include "jsstddef.h"
#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jscntxt.h"
#include "jsconfig.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstr.h"


#define MAXINDEX 4294967295u
#define MAXSTR   "4294967295"

















JSBool
js_IdIsIndex(jsval id, jsuint *indexp)
{
    JSString *str;
    jschar *cp;

    if (JSVAL_IS_INT(id)) {
        jsint i;
        i = JSVAL_TO_INT(id);
        if (i < 0)
            return JS_FALSE;
        *indexp = (jsuint)i;
        return JS_TRUE;
    }

    
    if (!JSVAL_IS_STRING(id))
        return JS_FALSE;

    str = JSVAL_TO_STRING(id);
    cp = JSSTRING_CHARS(str);
    if (JS7_ISDEC(*cp) && JSSTRING_LENGTH(str) < sizeof(MAXSTR)) {
        jsuint index = JS7_UNDEC(*cp++);
        jsuint oldIndex = 0;
        jsuint c = 0;
        if (index != 0) {
            while (JS7_ISDEC(*cp)) {
                oldIndex = index;
                c = JS7_UNDEC(*cp);
                index = 10*index + c;
                cp++;
            }
        }

        
        if (*cp == 0 &&
             (oldIndex < (MAXINDEX / 10) ||
              (oldIndex == (MAXINDEX / 10) && c < (MAXINDEX % 10))))
        {
            *indexp = index;
            return JS_TRUE;
        }
    }
    return JS_FALSE;
}

static JSBool
ValueIsLength(JSContext *cx, jsval v, jsuint *lengthp)
{
    jsint i;
    jsdouble d;

    if (JSVAL_IS_INT(v)) {
        i = JSVAL_TO_INT(v);
        if (i < 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_ARRAY_LENGTH);
            return JS_FALSE;
        }
        *lengthp = (jsuint) i;
        return JS_TRUE;
    }

    if (!js_ValueToNumber(cx, v, &d)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return JS_FALSE;
    }
    if (!js_DoubleToECMAUint32(cx, d, (uint32 *)lengthp)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return JS_FALSE;
    }
    if (JSDOUBLE_IS_NaN(d) || d != *lengthp) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return JS_FALSE;
    }
    return JS_TRUE;
}

JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp)
{
    JSTempValueRooter tvr;
    jsid id;
    JSBool ok;
    jsint i;

    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_NULL, &tvr);
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    ok = OBJ_GET_PROPERTY(cx, obj, id, &tvr.u.value);
    if (ok) {
        



        if (JSVAL_IS_INT(tvr.u.value)) {
            i = JSVAL_TO_INT(tvr.u.value);
            *lengthp = (jsuint)i;       
        } else {
            ok = js_ValueToECMAUint32(cx, tvr.u.value, (uint32 *)lengthp);
        }
    }
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

static JSBool
IndexToValue(JSContext *cx, jsuint index, jsval *vp)
{
    if (index <= JSVAL_INT_MAX) {
        *vp = INT_TO_JSVAL(index);
        return JS_TRUE;
    }
    return js_NewDoubleValue(cx, (jsdouble)index, vp);
}

static JSBool
BigIndexToId(JSContext *cx, JSObject *obj, jsuint index, JSBool createAtom,
             jsid *idp)
{
    jschar buf[10], *start;
    JSClass *clasp;
    JSAtom *atom;
    JS_STATIC_ASSERT((jsuint)-1 == 4294967295U);

    JS_ASSERT(index > JSVAL_INT_MAX);

    start = JS_ARRAY_END(buf);
    do {
        --start;
        *start = (jschar)('0' + index % 10);
        index /= 10;
    } while (index != 0);

    





    if (!createAtom &&
        ((clasp = OBJ_GET_CLASS(cx, obj)) == &js_ArrayClass ||
         clasp == &js_ArgumentsClass ||
         clasp == &js_ObjectClass)) {
        atom = js_GetExistingStringAtom(cx, start, JS_ARRAY_END(buf) - start);
        if (!atom) {
            *idp = JSVAL_VOID;
            return JS_TRUE;
        }
    } else {
        atom = js_AtomizeChars(cx, start, JS_ARRAY_END(buf) - start, 0);
        if (!atom)
            return JS_FALSE;
    }

    *idp = ATOM_TO_JSID(atom);
    return JS_TRUE;
}







static JSBool
GetArrayElement(JSContext *cx, JSObject *obj, jsuint index, JSBool *hole,
                jsval *vp)
{
    jsid id;
    JSObject *obj2;
    JSProperty *prop;

    if (index <= JSVAL_INT_MAX) {
        id = INT_TO_JSID(index);
    } else {
        if (!BigIndexToId(cx, obj, index, JS_FALSE, &id))
            return JS_FALSE;
        if (id == JSVAL_VOID) {
            *hole = JS_TRUE;
            *vp = JSVAL_VOID;
            return JS_TRUE;
        }
    }

    if (!OBJ_LOOKUP_PROPERTY(cx, obj, id, &obj2, &prop))
        return JS_FALSE;
    if (!prop) {
        *hole = JS_TRUE;
        *vp = JSVAL_VOID;
    } else {
        OBJ_DROP_PROPERTY(cx, obj2, prop);
        if (!OBJ_GET_PROPERTY(cx, obj, id, vp))
            return JS_FALSE;
        *hole = JS_FALSE;
    }
    return JS_TRUE;
}




static JSBool
SetArrayElement(JSContext *cx, JSObject *obj, jsuint index, jsval v)
{
    jsid id;

    if (index <= JSVAL_INT_MAX) {
        id = INT_TO_JSID(index);
    } else {
        if (!BigIndexToId(cx, obj, index, JS_TRUE, &id))
            return JS_FALSE;
        JS_ASSERT(id != JSVAL_VOID);
    }
    return OBJ_SET_PROPERTY(cx, obj, id, &v);
}

static JSBool
DeleteArrayElement(JSContext *cx, JSObject *obj, jsuint index)
{
    jsid id;
    jsval junk;

    if (index <= JSVAL_INT_MAX) {
        id = INT_TO_JSID(index);
    } else {
        if (!BigIndexToId(cx, obj, index, JS_FALSE, &id))
            return JS_FALSE;
        if (id == JSVAL_VOID)
            return JS_TRUE;
    }
    return OBJ_DELETE_PROPERTY(cx, obj, id, &junk);
}





static JSBool
SetOrDeleteArrayElement(JSContext *cx, JSObject *obj, jsuint index,
                        JSBool hole, jsval v)
{
    if (hole) {
        JS_ASSERT(v == JSVAL_VOID);
        return DeleteArrayElement(cx, obj, index);
    } else {
        return SetArrayElement(cx, obj, index, v);
    }
}

JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsuint length)
{
    jsval v;
    jsid id;

    if (!IndexToValue(cx, length, &v))
        return JS_FALSE;
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    return OBJ_SET_PROPERTY(cx, obj, id, &v);
}

JSBool
js_HasLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp)
{
    JSErrorReporter older;
    JSTempValueRooter tvr;
    jsid id;
    JSBool ok;

    older = JS_SetErrorReporter(cx, NULL);
    JS_PUSH_SINGLE_TEMP_ROOT(cx, JSVAL_NULL, &tvr);
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    ok = OBJ_GET_PROPERTY(cx, obj, id, &tvr.u.value);
    JS_SetErrorReporter(cx, older);
    if (ok)
        ok = ValueIsLength(cx, tvr.u.value, lengthp);
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

JSBool
js_IsArrayLike(JSContext *cx, JSObject *obj, JSBool *answerp, jsuint *lengthp)
{
    JSClass *clasp;

    clasp = OBJ_GET_CLASS(cx, obj);
    *answerp = (clasp == &js_ArgumentsClass || clasp == &js_ArrayClass);
    if (!*answerp) {
        *lengthp = 0;
        return JS_TRUE;
    }
    return js_GetLengthProperty(cx, obj, lengthp);
}













static JSBool
array_length_getter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    do {
        if (OBJ_GET_CLASS(cx, obj) == &js_ArrayClass) {
            *vp = STOBJ_GET_SLOT(obj, JSSLOT_ARRAY_LENGTH);
            break;
        }
    } while ((obj = OBJ_GET_PROTO(cx, obj)) != NULL);
    return JS_TRUE;
}

static JSBool
array_length_setter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsuint newlen, oldlen, gap, index;
    jsval junk;
    JSObject *iter;
    JSTempValueRooter tvr;
    JSBool ok;

    if (OBJ_GET_CLASS(cx, obj) != &js_ArrayClass) {
        jsid lengthId = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);

        return OBJ_DEFINE_PROPERTY(cx, obj, lengthId, *vp, NULL, NULL,
                                   JSPROP_ENUMERATE, NULL);
    }

    if (!ValueIsLength(cx, *vp, &newlen))
        return JS_FALSE;
    if (!js_GetLengthProperty(cx, obj, &oldlen))
        return JS_FALSE;
    if (oldlen > newlen) {
        if (oldlen - newlen < (1 << 24)) {
            do {
                --oldlen;
                if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                    !DeleteArrayElement(cx, obj, oldlen)) {
                    return JS_FALSE;
                }
            } while (oldlen != newlen);
        } else {
            






            iter = JS_NewPropertyIterator(cx, obj);
            if (!iter)
                return JS_FALSE;

            
            JS_PUSH_TEMP_ROOT_OBJECT(cx, iter, &tvr);
            gap = oldlen - newlen;
            for (;;) {
                ok = (JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) &&
                      JS_NextProperty(cx, iter, &id));
                if (!ok)
                    break;
                if (id == JSVAL_VOID)
                    break;
                if (js_IdIsIndex(id, &index) && index - newlen < gap) {
                    ok = OBJ_DELETE_PROPERTY(cx, obj, id, &junk);
                    if (!ok)
                        break;
                }
            }
            JS_POP_TEMP_ROOT(cx, &tvr);
            if (!ok)
                return JS_FALSE;
        }
    }
    if (!IndexToValue(cx, newlen, vp))
        return JS_FALSE;
    STOBJ_SET_SLOT(obj, JSSLOT_ARRAY_LENGTH, *vp);
    return JS_TRUE;
}

static JSBool
array_addProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsuint index, length;

    if (!js_IdIsIndex(id, &index))
        return JS_TRUE;
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (index >= length) {
        length = index + 1;
        return js_SetLengthProperty(cx, obj, length);
    }
    return JS_TRUE;
}

static JSBool
array_convert(JSContext *cx, JSObject *obj, JSType type, jsval *vp)
{
    return js_TryValueOf(cx, obj, type, vp);
}

JSClass js_ArrayClass = {
    "Array",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_Array),
    array_addProperty, JS_PropertyStub,   JS_PropertyStub,   JS_PropertyStub,
    JS_EnumerateStub,  JS_ResolveStub,    array_convert,     JS_FinalizeStub,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

enum ArrayToStringOp {
    TO_STRING,
    TO_LOCALE_STRING,
    TO_SOURCE
};






static JSBool
array_join_sub(JSContext *cx, JSObject *obj, enum ArrayToStringOp op,
               JSString *sep, jsval *rval)
{
    JSBool ok, hole;
    jsuint length, index;
    jschar *chars, *ochars;
    size_t nchars, growth, seplen, tmplen, extratail;
    const jschar *sepstr;
    JSString *str;
    JSHashEntry *he;
    JSAtom *atom;
    int stackDummy;

    if (!JS_CHECK_STACK_SIZE(cx, stackDummy)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_OVER_RECURSED);
        return JS_FALSE;
    }

    ok = js_GetLengthProperty(cx, obj, &length);
    if (!ok)
        return JS_FALSE;

    he = js_EnterSharpObject(cx, obj, NULL, &chars);
    if (!he)
        return JS_FALSE;
#ifdef DEBUG
    growth = (size_t) -1;
#endif

    if (op == TO_SOURCE) {
        if (IS_SHARP(he)) {
#if JS_HAS_SHARP_VARS
            nchars = js_strlen(chars);
#else
            chars[0] = '[';
            chars[1] = ']';
            chars[2] = 0;
            nchars = 2;
#endif
            goto make_string;
        }

        



        extratail = 2;
        growth = (1 + extratail) * sizeof(jschar);
        if (!chars) {
            nchars = 0;
            chars = (jschar *) malloc(growth);
            if (!chars)
                goto done;
        } else {
            MAKE_SHARP(he);
            nchars = js_strlen(chars);
            growth += nchars * sizeof(jschar);
            chars = (jschar *)realloc((ochars = chars), growth);
            if (!chars) {
                free(ochars);
                goto done;
            }
        }
        chars[nchars++] = '[';
        JS_ASSERT(sep == NULL);
        sepstr = NULL;  
        seplen = 2;
    } else {
        





        if (chars)
            JS_free(cx, chars);
        chars = NULL;
        nchars = 0;
        extratail = 1;  

        
        if (IS_BUSY(he) || length == 0) {
            js_LeaveSharpObject(cx, NULL);
            *rval = JS_GetEmptyStringValue(cx);
            return ok;
        }

        
        MAKE_BUSY(he);

        if (sep) {
            sepstr = JSSTRING_CHARS(sep);
            seplen = JSSTRING_LENGTH(sep);
        } else {
            sepstr = NULL;      
            seplen = 1;
        }
    }

    
    for (index = 0; index < length; index++) {
        ok = (JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) &&
              GetArrayElement(cx, obj, index, &hole, rval));
        if (!ok)
            goto done;
        if (hole ||
            (op != TO_SOURCE &&
             (JSVAL_IS_VOID(*rval) || JSVAL_IS_NULL(*rval)))) {
            str = cx->runtime->emptyString;
        } else {
            if (op == TO_LOCALE_STRING) {
                JSObject *robj;

                atom = cx->runtime->atomState.toLocaleStringAtom;
                ok = js_ValueToObject(cx, *rval, &robj);
                if (ok) {
                    
                    *rval = OBJECT_TO_JSVAL(robj);
                    ok = js_TryMethod(cx, robj, atom, 0, NULL, rval);
                }
                if (!ok)
                    goto done;
                str = js_ValueToString(cx, *rval);
            } else if (op == TO_STRING) {
                str = js_ValueToString(cx, *rval);
            } else {
                JS_ASSERT(op == TO_SOURCE);
                str = js_ValueToSource(cx, *rval);
            }
            if (!str) {
                ok = JS_FALSE;
                goto done;
            }
        }

        



        if (index + 1 == length)
            seplen = (hole && op == TO_SOURCE) ? 1 : 0;

        
        tmplen = JSSTRING_LENGTH(str);
        growth = nchars + tmplen + seplen + extratail;
        if (nchars > growth || tmplen > growth ||
            growth > (size_t)-1 / sizeof(jschar)) {
            if (chars) {
                free(chars);
                chars = NULL;
            }
            goto done;
        }
        growth *= sizeof(jschar);
        JS_COUNT_OPERATION(cx, JSOW_ALLOCATION);
        if (!chars) {
            chars = (jschar *) malloc(growth);
            if (!chars)
                goto done;
        } else {
            chars = (jschar *) realloc((ochars = chars), growth);
            if (!chars) {
                free(ochars);
                goto done;
            }
        }

        js_strncpy(&chars[nchars], JSSTRING_CHARS(str), tmplen);
        nchars += tmplen;

        if (seplen) {
            if (sepstr) {
                js_strncpy(&chars[nchars], sepstr, seplen);
            } else {
                JS_ASSERT(seplen == 1 || seplen == 2);
                chars[nchars] = ',';
                if (seplen == 2)
                    chars[nchars + 1] = ' ';
            }
            nchars += seplen;
        }
    }

  done:
    if (op == TO_SOURCE) {
        if (chars)
            chars[nchars++] = ']';
    } else {
        CLEAR_BUSY(he);
    }
    js_LeaveSharpObject(cx, NULL);
    if (!ok) {
        if (chars)
            free(chars);
        return ok;
    }

  make_string:
    if (!chars) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }
    chars[nchars] = 0;
    JS_ASSERT(growth == (size_t)-1 || (nchars + 1) * sizeof(jschar) == growth);
    str = js_NewString(cx, chars, nchars, 0);
    if (!str) {
        free(chars);
        return JS_FALSE;
    }
    *rval = STRING_TO_JSVAL(str);
    return JS_TRUE;
}

#if JS_HAS_TOSOURCE
static JSBool
array_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!JS_InstanceOf(cx, obj, &js_ArrayClass, vp + 2))
        return JS_FALSE;
    return array_join_sub(cx, obj, TO_SOURCE, NULL, vp);
}
#endif

static JSBool
array_toString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!JS_InstanceOf(cx, obj, &js_ArrayClass, vp + 2))
        return JS_FALSE;
    return array_join_sub(cx, obj, TO_STRING, NULL, vp);
}

static JSBool
array_toLocaleString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!JS_InstanceOf(cx, obj, &js_ArrayClass, vp + 2))
        return JS_FALSE;

    



    return array_join_sub(cx, obj, TO_LOCALE_STRING, NULL, vp);
}

static JSBool
InitArrayElements(JSContext *cx, JSObject *obj, jsuint start, jsuint end,
                  jsval *vector)
{
    while (start != end) {
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
            !SetArrayElement(cx, obj, start++, *vector++)) {
            return JS_FALSE;
        }
    }
    return JS_TRUE;
}

static JSBool
InitArrayObject(JSContext *cx, JSObject *obj, jsuint length, jsval *vector)
{
    jsval v;

    JS_ASSERT(OBJ_GET_CLASS(cx, obj) == &js_ArrayClass);
    if (!IndexToValue(cx, length, &v))
        return JS_FALSE;
    STOBJ_SET_SLOT(obj, JSSLOT_ARRAY_LENGTH, v);
    return !vector || InitArrayElements(cx, obj, 0, length, vector);
}




static JSBool
array_join(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;

    if (JSVAL_IS_VOID(vp[2])) {
        str = NULL;
    } else {
        str = js_ValueToString(cx, vp[2]);
        if (!str)
            return JS_FALSE;
        vp[2] = STRING_TO_JSVAL(str);
    }
    return array_join_sub(cx, JS_THIS_OBJECT(cx, vp), TO_STRING, str, vp);
}

static JSBool
array_reverse(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval *argv, *tmproot, *tmproot2;
    jsuint len, half, i;
    JSBool hole, hole2;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &len))
        return JS_FALSE;

    



    argv = JS_ARGV(cx, vp);
    tmproot = argv + argc;
    tmproot2 = argv + argc + 1;
    half = len / 2;
    for (i = 0; i < half; i++) {
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
            !GetArrayElement(cx, obj, i, &hole, tmproot) ||
            !GetArrayElement(cx, obj, len - i - 1, &hole2, tmproot2) ||
            !SetOrDeleteArrayElement(cx, obj, len - i - 1, hole, *tmproot) ||
            !SetOrDeleteArrayElement(cx, obj, i, hole2, *tmproot2)) {
            return JS_FALSE;
        }
    }
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}

typedef struct MSortArgs {
    size_t       elsize;
    JSComparator cmp;
    void         *arg;
    JSBool       fastcopy;
} MSortArgs;

static JSBool
sort_compare(void *arg, const void *a, const void *b, int *result);

static int
sort_compare_strings(void *arg, const void *a, const void *b, int *result);


static JSBool
MergeArrays(MSortArgs *msa, void *src, void *dest, size_t run1, size_t run2)
{
    void *arg, *a, *b, *c;
    size_t elsize, runtotal;
    int cmp_result;
    JSComparator cmp;
    JSBool fastcopy;

    runtotal = run1 + run2;

    elsize = msa->elsize;
    cmp = msa->cmp;
    arg = msa->arg;
    fastcopy = msa->fastcopy;

#define CALL_CMP(a, b) \
    if (!cmp(arg, (a), (b), &cmp_result)) return JS_FALSE;

    
    b = (char *)src + run1 * elsize;
    a = (char *)b - elsize;
    CALL_CMP(a, b);
    if (cmp_result <= 0) {
        memcpy(dest, src, runtotal * elsize);
        return JS_TRUE;
    }

#define COPY_ONE(p,q,n) \
    (fastcopy ? (void)(*(jsval*)(p) = *(jsval*)(q)) : (void)memcpy(p, q, n))

    a = src;
    c = dest;
    for (; runtotal != 0; runtotal--) {
        JSBool from_a = run2 == 0;
        if (!from_a && run1 != 0) {
            CALL_CMP(a,b);
            from_a = cmp_result <= 0;
        }

        if (from_a) {
            COPY_ONE(c, a, elsize);
            run1--;
            a = (char *)a + elsize;
        } else {
            COPY_ONE(c, b, elsize);
            run2--;
            b = (char *)b + elsize;
        }
        c = (char *)c + elsize;
    }
#undef COPY_ONE
#undef CALL_CMP

    return JS_TRUE;
}





JSBool
js_MergeSort(void *src, size_t nel, size_t elsize,
             JSComparator cmp, void *arg, void *tmp)
{
    void *swap, *vec1, *vec2;
    MSortArgs msa;
    size_t i, j, lo, hi, run;
    JSBool fastcopy;
    int cmp_result;

    fastcopy = (cmp == sort_compare || cmp == sort_compare_strings);
#define COPY_ONE(p,q,n) \
    (fastcopy ? (void)(*(jsval*)(p) = *(jsval*)(q)) : (void)memcpy(p, q, n))
#define CALL_CMP(a, b) \
    if (!cmp(arg, (a), (b), &cmp_result)) return JS_FALSE;
#define INS_SORT_INT 4

    



    for (lo = 0; lo < nel; lo += INS_SORT_INT) {
        hi = lo + INS_SORT_INT;
        if (hi >= nel)
            hi = nel;
        for (i = lo + 1; i < hi; i++) {
            vec1 = (char *)src + i * elsize;
            vec2 = (char *)vec1 - elsize;
            for (j = i; j > lo; j--) {
                CALL_CMP(vec2, vec1);
                
                if (cmp_result <= 0) {
                    break;
                }

                
                COPY_ONE(tmp, vec2, elsize);
                COPY_ONE(vec2, vec1, elsize);
                COPY_ONE(vec1, tmp, elsize);
                vec1 = vec2;
                vec2 = (char *)vec1 - elsize;
            }
        }
    }
#undef CALL_CMP
#undef COPY_ONE

    msa.elsize = elsize;
    msa.cmp = cmp;
    msa.arg = arg;
    msa.fastcopy = fastcopy;

    vec1 = src;
    vec2 = tmp;
    for (run = INS_SORT_INT; run < nel; run *= 2) {
        for (lo = 0; lo < nel; lo += 2 * run) {
            hi = lo + run;
            if (hi >= nel) {
                memcpy((char *)vec2 + lo * elsize, (char *)vec1 + lo * elsize,
                       (nel - lo) * elsize);
                break;
            }
            if (!MergeArrays(&msa, (char *)vec1 + lo * elsize,
                             (char *)vec2 + lo * elsize, run,
                             hi + run > nel ? nel - hi : run)) {
                return JS_FALSE;
            }
        }
        swap = vec1;
        vec1 = vec2;
        vec2 = swap;
    }
    if (src != vec1)
        memcpy(src, tmp, nel * elsize);

    return JS_TRUE;
}

typedef struct CompareArgs {
    JSContext   *context;
    jsval       fval;
    jsval       *localroot;     
} CompareArgs;

static JSBool
sort_compare(void *arg, const void *a, const void *b, int *result)
{
    jsval av = *(const jsval *)a, bv = *(const jsval *)b;
    CompareArgs *ca = (CompareArgs *) arg;
    JSContext *cx = ca->context;
    jsval fval;
    JSBool ok;

    



    JS_ASSERT(av != JSVAL_VOID);
    JS_ASSERT(bv != JSVAL_VOID);

    if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP))
        return JS_FALSE;

    *result = 0;
    ok = JS_TRUE;
    fval = ca->fval;
    if (fval == JSVAL_NULL) {
        JSString *astr, *bstr;

        if (av != bv) {
            






            astr = js_ValueToString(cx, av);
            *ca->localroot = STRING_TO_JSVAL(astr);
            if (astr && (bstr = js_ValueToString(cx, bv)))
                *result = js_CompareStrings(astr, bstr);
            else
                ok = JS_FALSE;
        }
    } else {
        jsdouble cmp;
        jsval argv[2];

        argv[0] = av;
        argv[1] = bv;
        ok = js_InternalCall(cx,
                             OBJ_GET_PARENT(cx, JSVAL_TO_OBJECT(fval)),
                             fval, 2, argv, ca->localroot);
        if (ok) {
            ok = js_ValueToNumber(cx, *ca->localroot, &cmp);

            
            if (ok) {
                if (JSDOUBLE_IS_NaN(cmp)) {
                    





                } else if (cmp != 0) {
                    *result = cmp > 0 ? 1 : -1;
                }
            }
        }
    }
    return ok;
}

static int
sort_compare_strings(void *arg, const void *a, const void *b, int *result)
{
    jsval av = *(const jsval *)a, bv = *(const jsval *)b;

    if (!JS_CHECK_OPERATION_LIMIT((JSContext *)arg, JSOW_JUMP))
        return JS_FALSE;

    *result = (int) js_CompareStrings(JSVAL_TO_STRING(av), JSVAL_TO_STRING(bv));
    return JS_TRUE;
}






JS_STATIC_ASSERT(JSVAL_NULL == 0);

static JSBool
array_sort(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv, fval, *vec, *mergesort_tmp;
    JSObject *obj;
    CompareArgs ca;
    jsuint len, newlen, i, undefs;
    JSTempValueRooter tvr;
    JSBool hole, ok;

    



    JSBool all_strings;

    argv = JS_ARGV(cx, vp);
    if (argc > 0) {
        if (JSVAL_IS_PRIMITIVE(argv[0])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_BAD_SORT_ARG);
            return JS_FALSE;
        }
        fval = argv[0];
        all_strings = JS_FALSE; 
    } else {
        fval = JSVAL_NULL;
        all_strings = JS_TRUE;  
    }

    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &len))
        return JS_FALSE;
    if (len == 0) {
        *vp = OBJECT_TO_JSVAL(obj);
        return JS_TRUE;
    }

    




    if (len > (size_t)-1 / (2 * sizeof(jsval))) {
        JS_ReportOutOfMemory(cx);
        return JS_FALSE;
    }

    



    vec = (jsval *) JS_malloc(cx, 2 * (size_t)len * sizeof(jsval));
    if (!vec)
        return JS_FALSE;

    










    JS_PUSH_TEMP_ROOT(cx, 0, vec, &tvr);

    







    undefs = 0;
    newlen = 0;
    for (i = 0; i < len; i++) {
        ok = JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP);
        if (!ok)
            goto out;

        
        vec[newlen] = JSVAL_NULL;
        tvr.count = newlen + 1;
        ok = GetArrayElement(cx, obj, i, &hole, &vec[newlen]);
        if (!ok)
            goto out;

        if (hole)
            continue;

        if (vec[newlen] == JSVAL_VOID) {
            ++undefs;
            continue;
        }

        
        all_strings &= JSVAL_IS_STRING(vec[newlen]);

        ++newlen;
    }

    










    mergesort_tmp = vec + newlen;
    memset(mergesort_tmp, 0, newlen * sizeof(jsval));
    tvr.count = newlen * 2;

    
    if (all_strings) {
        ok = js_MergeSort(vec, (size_t) newlen, sizeof(jsval),
                          sort_compare_strings, cx, mergesort_tmp);
    } else {
        ca.context = cx;
        ca.fval = fval;
        ca.localroot = argv + argc; 
        ok = js_MergeSort(vec, (size_t) newlen, sizeof(jsval),
                          sort_compare, &ca, mergesort_tmp);
    }
    if (!ok)
        goto out;

    ok = InitArrayElements(cx, obj, 0, newlen, vec);
    if (!ok)
        goto out;

  out:
    JS_POP_TEMP_ROOT(cx, &tvr);
    JS_free(cx, vec);
    if (!ok)
        return JS_FALSE;

    
    while (undefs != 0) {
        --undefs;
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
            !SetArrayElement(cx, obj, newlen++, JSVAL_VOID)) {
            return JS_FALSE;
        }
    }

    
    while (len > newlen) {
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
            !DeleteArrayElement(cx, obj, --len)) {
            return JS_FALSE;
        }
    }
    *vp = OBJECT_TO_JSVAL(obj);
    return JS_TRUE;
}




static JSBool
slow_array_push(JSContext *cx, JSObject *obj, uintN argc, jsval *vp)
{
    jsuint length, newlength;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    newlength = length + argc;
    if (!InitArrayElements(cx, obj, length, newlength, vp + 2))
        return JS_FALSE;

    
    if (!IndexToValue(cx, newlength, vp))
        return JS_FALSE;
    return js_SetLengthProperty(cx, obj, newlength);
}

static JSBool
array_push(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval v;

    
    obj = JSVAL_TO_OBJECT(vp[1]);
    if (argc != 1 || OBJ_GET_CLASS(cx, obj) != &js_ArrayClass)
        return slow_array_push(cx, obj, argc, vp);

    
    v = STOBJ_GET_SLOT(obj, JSSLOT_ARRAY_LENGTH);
    if (!(v & JSVAL_INT) || v == INT_TO_JSVAL(JSVAL_INT_MAX))
        return slow_array_push(cx, obj, argc, vp);

    






    if (!js_DefineNativeProperty(cx, obj, INT_JSVAL_TO_JSID(v), vp[2],
                                 NULL, NULL, JSPROP_ENUMERATE, 0, 0,
                                 NULL)) {
        return JS_FALSE;
    }
    v += 2;
    JS_ASSERT(STOBJ_GET_SLOT(obj, JSSLOT_ARRAY_LENGTH) == v);
    *vp = v;
    return JS_TRUE;
}

JSBool
array_pop(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsuint index;
    JSBool hole;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &index))
        return JS_FALSE;
    if (index == 0) {
        *vp = JSVAL_VOID;
    } else {
        index--;

        
        if (!GetArrayElement(cx, obj, index, &hole, vp))
            return JS_FALSE;
        if (!hole && !DeleteArrayElement(cx, obj, index))
            return JS_FALSE;
    }
    return js_SetLengthProperty(cx, obj, index);
}

static JSBool
array_shift(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsuint length, i;
    JSBool hole;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (length == 0) {
        *vp = JSVAL_VOID;
    } else {
        length--;

        
        if (!GetArrayElement(cx, obj, 0, &hole, vp))
            return JS_FALSE;

        



        for (i = 0; i != length; i++) {
            if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                !GetArrayElement(cx, obj, i + 1, &hole, &vp[2]) ||
                !SetOrDeleteArrayElement(cx, obj, i, hole, vp[2])) {
                return JS_FALSE;
            }
        }

        
        if (!hole && !DeleteArrayElement(cx, obj, length))
            return JS_FALSE;
    }
    return js_SetLengthProperty(cx, obj, length);
}

static JSBool
array_unshift(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval *argv, *localroot;
    jsuint length, last;
    JSBool hole;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (argc > 0) {
        
        argv = JS_ARGV(cx, vp);
        if (length > 0) {
            last = length;
            localroot = argv + argc;
            do {
                --last;
                if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                    !GetArrayElement(cx, obj, last, &hole, localroot) ||
                    !SetOrDeleteArrayElement(cx, obj, last + argc, hole,
                                             *localroot)) {
                    return JS_FALSE;
                }
            } while (last != 0);
        }

        
        if (!InitArrayElements(cx, obj, 0, argc, argv))
            return JS_FALSE;

        length += argc;
        if (!js_SetLengthProperty(cx, obj, length))
            return JS_FALSE;
    }

    
    return IndexToValue(cx, length, vp);
}

static JSBool
array_splice(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv, *localroot;
    JSObject *obj;
    jsuint length, begin, end, count, delta, last;
    jsdouble d;
    JSBool hole;
    JSObject *obj2;

    



    if (argc == 0)
        return JS_TRUE;
    argv = JS_ARGV(cx, vp);
    localroot = argv + argc;
    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    
    if (!js_ValueToNumber(cx, *argv, &d))
        return JS_FALSE;
    d = js_DoubleToInteger(d);
    if (d < 0) {
        d += length;
        if (d < 0)
            d = 0;
    } else if (d > length) {
        d = length;
    }
    begin = (jsuint)d; 
    argc--;
    argv++;

    
    delta = length - begin;
    if (argc == 0) {
        count = delta;
        end = length;
    } else {
        if (!js_ValueToNumber(cx, *argv, &d))
            return JS_FALSE;
        d = js_DoubleToInteger(d);
        if (d < 0)
            d = 0;
        else if (d > delta)
            d = delta;
        count = (jsuint)d;
        end = begin + count;
        argc--;
        argv++;
    }


    





    obj2 = js_NewArrayObject(cx, 0, NULL);
    if (!obj2)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(obj2);

    
    if (count > 0) {
        for (last = begin; last < end; last++) {
            if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                !GetArrayElement(cx, obj, last, &hole, localroot)) {
                return JS_FALSE;
            }

            
            if (!hole && !SetArrayElement(cx, obj2, last - begin, *localroot))
                return JS_FALSE;
        }

        if (!js_SetLengthProperty(cx, obj2, end - begin))
            return JS_FALSE;
    }

    
    if (argc > count) {
        delta = (jsuint)argc - count;
        last = length;
        
        while (last-- > end) {
            if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                !GetArrayElement(cx, obj, last, &hole, localroot) ||
                !SetOrDeleteArrayElement(cx, obj, last + delta, hole,
                                         *localroot)) {
                return JS_FALSE;
            }
        }
        length += delta;
    } else if (argc < count) {
        delta = count - (jsuint)argc;
        for (last = end; last < length; last++) {
            if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                !GetArrayElement(cx, obj, last, &hole, localroot) ||
                !SetOrDeleteArrayElement(cx, obj, last - delta, hole,
                                         *localroot)) {
                return JS_FALSE;
            }
        }
        length -= delta;
    }

    
    if (!InitArrayElements(cx, obj, begin, begin + argc, argv))
        return JS_FALSE;

    
    return js_SetLengthProperty(cx, obj, length);
}




static JSBool
array_concat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv, *localroot, v;
    JSObject *nobj, *aobj;
    jsuint length, alength, slot;
    uintN i;
    JSBool hole;

    
    argv = JS_ARGV(cx, vp);
    localroot = argv + argc;

    
    --argv;
    JS_ASSERT(JS_THIS_OBJECT(cx, vp) == JSVAL_TO_OBJECT(argv[0]));

    
    nobj = js_NewArrayObject(cx, 0, NULL);
    if (!nobj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(nobj);

    
    length = 0;
    for (i = 0; i <= argc; i++) {
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP))
            return JS_FALSE;
        v = argv[i];
        if (JSVAL_IS_OBJECT(v)) {
            aobj = JSVAL_TO_OBJECT(v);
            if (aobj && OBJ_GET_CLASS(cx, aobj) == &js_ArrayClass) {
                if (!OBJ_GET_PROPERTY(cx, aobj,
                                      ATOM_TO_JSID(cx->runtime->atomState
                                                   .lengthAtom),
                                      localroot)) {
                    return JS_FALSE;
                }
                if (!ValueIsLength(cx, *localroot, &alength))
                    return JS_FALSE;
                for (slot = 0; slot < alength; slot++) {
                    if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
                        !GetArrayElement(cx, aobj, slot, &hole, localroot)) {
                        return JS_FALSE;
                    }

                    



                    if (!hole &&
                        !SetArrayElement(cx, nobj, length + slot, *localroot)) {
                        return JS_FALSE;
                    }
                }
                length += alength;
                continue;
            }
        }

        if (!SetArrayElement(cx, nobj, length, v))
            return JS_FALSE;
        length++;
    }

    return js_SetLengthProperty(cx, nobj, length);
}

static JSBool
array_slice(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv, *localroot;
    JSObject *nobj, *obj;
    jsuint length, begin, end, slot;
    jsdouble d;
    JSBool hole;

    
    argv = JS_ARGV(cx, vp);
    localroot = argv + argc;

    
    nobj = js_NewArrayObject(cx, 0, NULL);
    if (!nobj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(nobj);

    obj = JS_THIS_OBJECT(cx, vp);
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    begin = 0;
    end = length;

    if (argc > 0) {
        if (!js_ValueToNumber(cx, argv[0], &d))
            return JS_FALSE;
        d = js_DoubleToInteger(d);
        if (d < 0) {
            d += length;
            if (d < 0)
                d = 0;
        } else if (d > length) {
            d = length;
        }
        begin = (jsuint)d;

        if (argc > 1) {
            if (!js_ValueToNumber(cx, argv[1], &d))
                return JS_FALSE;
            d = js_DoubleToInteger(d);
            if (d < 0) {
                d += length;
                if (d < 0)
                    d = 0;
            } else if (d > length) {
                d = length;
            }
            end = (jsuint)d;
        }
    }

    if (begin > end)
        begin = end;

    for (slot = begin; slot < end; slot++) {
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
            !GetArrayElement(cx, obj, slot, &hole, localroot)) {
            return JS_FALSE;
        }
        if (!hole && !SetArrayElement(cx, nobj, slot - begin, *localroot))
            return JS_FALSE;
    }
    return js_SetLengthProperty(cx, nobj, end - begin);
}

#if JS_HAS_ARRAY_EXTRAS

static JSBool
array_indexOfHelper(JSContext *cx, JSBool isLast, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsuint length, i, stop;
    jsint direction;
    JSBool hole;

    obj = JSVAL_TO_OBJECT(vp[1]);
    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (length == 0)
        goto not_found;

    if (argc <= 1) {
        i = isLast ? length - 1 : 0;
    } else {
        jsdouble start;

        if (!js_ValueToNumber(cx, vp[3], &start))
            return JS_FALSE;
        start = js_DoubleToInteger(start);
        if (start < 0) {
            start += length;
            if (start < 0) {
                if (isLast)
                    goto not_found;
                i = 0;
            } else {
                i = (jsuint)start;
            }
        } else if (start >= length) {
            if (!isLast)
                goto not_found;
            i = length - 1;
        } else {
            i = (jsuint)start;
        }
    }

    if (isLast) {
        stop = 0;
        direction = -1;
    } else {
        stop = length - 1;
        direction = 1;
    }

    for (;;) {
        if (!JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) ||
            !GetArrayElement(cx, obj, (jsuint)i, &hole, vp)) {
            return JS_FALSE;
        }
        if (!hole && js_StrictlyEqual(*vp, vp[2]))
            return js_NewNumberValue(cx, i, vp);
        if (i == stop)
            goto not_found;
        i += direction;
    }

  not_found:
    *vp = INT_TO_JSVAL(-1);
    return JS_TRUE;
}

static JSBool
array_indexOf(JSContext *cx, uintN argc, jsval *vp)
{
    return array_indexOfHelper(cx, JS_FALSE, argc, vp);
}

static JSBool
array_lastIndexOf(JSContext *cx, uintN argc, jsval *vp)
{
    return array_indexOfHelper(cx, JS_TRUE, argc, vp);
}


typedef enum ArrayExtraMode {
    FOREACH,
    REDUCE,
    REDUCE_RIGHT,
    MAP,
    FILTER,
    SOME,
    EVERY
} ArrayExtraMode;

#define REDUCE_MODE(mode) ((mode) == REDUCE || (mode) == REDUCE_RIGHT)

static JSBool
array_extra(JSContext *cx, ArrayExtraMode mode, uintN argc, jsval *vp)
{
    enum { ELEM, TEMP, RVAL, NROOTS };
    jsval *argv, roots[NROOTS], *sp, *origsp, *oldsp;
    JSObject *obj;
    JSBool ok, cond, hole;
    jsuint length, newlen;
    JSObject *callable, *thisp, *newarr;
    jsint start, end, step, i;
    JSTempValueRooter tvr;
    void *mark;
    JSStackFrame *fp;

    
    argv = vp + 2;

    obj = JSVAL_TO_OBJECT(vp[1]);
    ok = js_GetLengthProperty(cx, obj, &length);
    if (!ok)
        return JS_FALSE;

    



    callable = js_ValueToCallableObject(cx, &argv[0], JSV2F_SEARCH_STACK);
    if (!callable)
        return JS_FALSE;

    



#ifdef __GNUC__ 
    newlen = 0;
    newarr = NULL;
#endif
    start = 0, end = length, step = 1;
    memset(roots, 0, sizeof roots);
    JS_PUSH_TEMP_ROOT(cx, NROOTS, roots, &tvr);

    switch (mode) {
      case REDUCE_RIGHT:
        start = length - 1, end = -1, step = -1;
        
      case REDUCE:
        if (length == 0 && argc == 1) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_EMPTY_ARRAY_REDUCE);
            ok = JS_FALSE;
            goto early_out;
        }
        if (argc >= 2) {
            roots[RVAL] = argv[1];
        } else {
            do {
                ok = GetArrayElement(cx, obj, start, &hole, &roots[RVAL]);
                if (!ok)
                    goto early_out;
                start += step;
            } while (hole && start != end);

            if (hole && start == end) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_EMPTY_ARRAY_REDUCE);
                ok = JS_FALSE;
                goto early_out;
            }
        }
        break;
      case MAP:
      case FILTER:
        newlen = (mode == MAP) ? length : 0;
        newarr = js_NewArrayObject(cx, newlen, NULL);
        if (!newarr) {
            ok = JS_FALSE;
            goto early_out;
        }
        roots[RVAL] = OBJECT_TO_JSVAL(newarr);
        break;
      case SOME:
        roots[RVAL] = JSVAL_FALSE;
        break;
      case EVERY:
        roots[RVAL] = JSVAL_TRUE;
        break;
      case FOREACH:
        break;
    }

    if (length == 0)
        goto early_out;

    if (argc > 1 && !REDUCE_MODE(mode)) {
        ok = js_ValueToObject(cx, argv[1], &thisp);
        if (!ok)
            goto early_out;
        argv[1] = OBJECT_TO_JSVAL(thisp);
    } else {
        thisp = NULL;
    }

    



    argc = 3 + REDUCE_MODE(mode);
    origsp = js_AllocStack(cx, 2 + argc + 1, &mark);
    if (!origsp) {
        ok = JS_FALSE;
        goto early_out;
    }

    
    fp = cx->fp;
    oldsp = fp->sp;

    for (i = start; i != end; i += step) {
        ok = (JS_CHECK_OPERATION_LIMIT(cx, JSOW_JUMP) &&
              GetArrayElement(cx, obj, i, &hole, &roots[ELEM]));
        if (!ok)
            break;
        if (hole)
            continue;

        





        sp = origsp;
        *sp++ = OBJECT_TO_JSVAL(callable);
        *sp++ = OBJECT_TO_JSVAL(thisp);
        if (REDUCE_MODE(mode))
            *sp++ = roots[RVAL];
        *sp++ = roots[ELEM];
        *sp++ = INT_TO_JSVAL(i);
        *sp++ = OBJECT_TO_JSVAL(obj);

        
        fp->sp = sp;
        ok = js_Invoke(cx, argc, JSINVOKE_INTERNAL);
        roots[TEMP] = fp->sp[-1];
        fp->sp = oldsp;
        if (!ok)
            break;

        if (mode > MAP) {
            if (roots[TEMP] == JSVAL_NULL) {
                cond = JS_FALSE;
            } else if (JSVAL_IS_BOOLEAN(roots[TEMP])) {
                cond = JSVAL_TO_BOOLEAN(roots[TEMP]);
            } else {
                ok = js_ValueToBoolean(cx, roots[TEMP], &cond);
                if (!ok)
                    goto out;
            }
        }

        switch (mode) {
          case FOREACH:
            break;
          case REDUCE:
          case REDUCE_RIGHT:
            roots[RVAL] = roots[TEMP];
            break;
          case MAP:
            ok = SetArrayElement(cx, newarr, i, roots[TEMP]);
            if (!ok)
                goto out;
            break;
          case FILTER:
            if (!cond)
                break;
            
            ok = SetArrayElement(cx, newarr, newlen++, roots[ELEM]);
            if (!ok)
                goto out;
            break;
          case SOME:
            if (cond) {
                roots[RVAL] = JSVAL_TRUE;
                goto out;
            }
            break;
          case EVERY:
            if (!cond) {
                roots[RVAL] = JSVAL_FALSE;
                goto out;
            }
            break;
        }
    }

  out:
    js_FreeStack(cx, mark);
    if (ok && mode == FILTER)
        ok = js_SetLengthProperty(cx, newarr, newlen);
  early_out:
    *vp = roots[RVAL];
    JS_POP_TEMP_ROOT(cx, &tvr);
    return ok;
}

static JSBool
array_forEach(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, FOREACH, argc, vp);
}

static JSBool
array_map(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, MAP, argc, vp);
}

static JSBool
array_reduce(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, REDUCE, argc, vp);
}

static JSBool
array_reduceRight(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, REDUCE_RIGHT, argc, vp);
}

static JSBool
array_filter(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, FILTER, argc, vp);
}

static JSBool
array_some(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, SOME, argc, vp);
}

static JSBool
array_every(JSContext *cx, uintN argc, jsval *vp)
{
    return array_extra(cx, EVERY, argc, vp);
}
#endif

static JSPropertySpec array_props[] = {
    {js_length_str,   -1,   JSPROP_SHARED | JSPROP_PERMANENT,
                            array_length_getter,    array_length_setter},
    {0,0,0,0,0}
};

static JSFunctionSpec array_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,      array_toSource,     0,0,0,0),
#endif
    JS_FN(js_toString_str,      array_toString,     0,0,0,0),
    JS_FN(js_toLocaleString_str,array_toLocaleString,0,0,0,0),

    
    JS_FN("join",               array_join,         1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("reverse",            array_reverse,      0,0,JSFUN_GENERIC_NATIVE,2),
    JS_FN("sort",               array_sort,         0,1,JSFUN_GENERIC_NATIVE,1),
    JS_FN("push",               array_push,         1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("pop",                array_pop,          0,0,JSFUN_GENERIC_NATIVE,0),
    JS_FN("shift",              array_shift,        0,0,JSFUN_GENERIC_NATIVE,1),
    JS_FN("unshift",            array_unshift,      0,1,JSFUN_GENERIC_NATIVE,1),
    JS_FN("splice",             array_splice,       0,2,JSFUN_GENERIC_NATIVE,1),

    
    JS_FN("concat",             array_concat,       0,1,JSFUN_GENERIC_NATIVE,1),
    JS_FN("slice",              array_slice,        0,2,JSFUN_GENERIC_NATIVE,1),

#if JS_HAS_ARRAY_EXTRAS
    JS_FN("indexOf",            array_indexOf,      1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("lastIndexOf",        array_lastIndexOf,  1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("forEach",            array_forEach,      1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("map",                array_map,          1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("reduce",             array_reduce,       1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("reduceRight",        array_reduceRight,  1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("filter",             array_filter,       1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("some",               array_some,         1,1,JSFUN_GENERIC_NATIVE,0),
    JS_FN("every",              array_every,        1,1,JSFUN_GENERIC_NATIVE,0),
#endif

    JS_FS_END
};

static JSBool
Array(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint length;
    jsval *vector;

    
    if (!(cx->fp->flags & JSFRAME_CONSTRUCTING)) {
        obj = js_NewObject(cx, &js_ArrayClass, NULL, NULL);
        if (!obj)
            return JS_FALSE;
        *rval = OBJECT_TO_JSVAL(obj);
    }

    if (argc == 0) {
        length = 0;
        vector = NULL;
    } else if (argc > 1) {
        length = (jsuint) argc;
        vector = argv;
    } else if (!JSVAL_IS_NUMBER(argv[0])) {
        length = 1;
        vector = argv;
    } else {
        if (!ValueIsLength(cx, argv[0], &length))
            return JS_FALSE;
        vector = NULL;
    }
    return InitArrayObject(cx, obj, length, vector);
}

JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto;

    proto = JS_InitClass(cx, obj, NULL, &js_ArrayClass, Array, 1,
                         array_props, array_methods, NULL, NULL);

    
    if (!proto || !InitArrayObject(cx, proto, 0, NULL))
        return NULL;
    return proto;
}

JSObject *
js_NewArrayObject(JSContext *cx, jsuint length, jsval *vector)
{
    JSTempValueRooter tvr;
    JSObject *obj;

    obj = js_NewObject(cx, &js_ArrayClass, NULL, NULL);
    if (!obj)
        return NULL;

    JS_PUSH_TEMP_ROOT_OBJECT(cx, obj, &tvr);
    if (!InitArrayObject(cx, obj, length, vector))
        obj = NULL;
    JS_POP_TEMP_ROOT(cx, &tvr);

    
    cx->weakRoots.newborn[GCX_OBJECT] = (JSGCThing *) obj;
    return obj;
}
