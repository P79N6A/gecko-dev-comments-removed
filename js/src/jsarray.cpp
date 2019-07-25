












































































#include <stdlib.h>
#include <string.h>
#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h" 
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbit.h"
#include "jsbool.h"
#include "jstracer.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsdbgapi.h" 
#include "jsdtoa.h"
#include "jsfun.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsscope.h"
#include "jsstr.h"
#include "jsstaticcheck.h"
#include "jsvector.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"
#include "jscntxtinlines.h"

using namespace js;


#define MAXINDEX 4294967295u
#define MAXSTR   "4294967295"


#define MIN_SPARSE_INDEX 256

static inline bool
INDEX_TOO_BIG(jsuint index)
{
    return index > JS_BIT(29) - 1;
}

#define INDEX_TOO_SPARSE(array, index)                                         \
    (INDEX_TOO_BIG(index) ||                                                   \
     ((index) > js_DenseArrayCapacity(array) && (index) >= MIN_SPARSE_INDEX && \
      (index) > ((array)->getArrayCount() + 1) * 4))

JS_STATIC_ASSERT(sizeof(JSScopeProperty) > 4 * sizeof(jsval));

#define ENSURE_SLOW_ARRAY(cx, obj)                                             \
    (obj->getClass() == &js_SlowArrayClass || js_MakeArraySlow(cx, obj))

















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
    cp = str->chars();
    if (JS7_ISDEC(*cp) && str->length() < sizeof(MAXSTR)) {
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

static jsuint
ValueIsLength(JSContext *cx, jsval* vp)
{
    jsint i;
    jsuint length;

    if (JSVAL_IS_INT(*vp)) {
        i = JSVAL_TO_INT(*vp);
        if (i < 0)
            goto error;
        return (jsuint) i;
    }

    jsdouble d;
    if (!ValueToNumber(cx, *vp, &d))
        goto error;

    if (JSDOUBLE_IS_NaN(d))
        goto error;
    length = (jsuint) d;
    if (d != (jsdouble) length)
        goto error;
    return length;

  error:
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_BAD_ARRAY_LENGTH);
    *vp = JSVAL_NULL;
    return 0;
}

JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp)
{
    if (obj->isArray()) {
        *lengthp = obj->getArrayLength();
        return true;
    }

    if (obj->isArguments() && !IsOverriddenArgsLength(obj)) {
        *lengthp = GetArgsLength(obj);
        return true;
    }

    AutoValueRooter tvr(cx, JSVAL_NULL);
    if (!obj->getProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.lengthAtom), tvr.addr()))
        return false;

    if (JSVAL_IS_INT(tvr.value())) {
        *lengthp = jsuint(jsint(JSVAL_TO_INT(tvr.value()))); 
        return true;
    }

    JS_STATIC_ASSERT(sizeof(jsuint) == sizeof(uint32_t));
    return ValueToECMAUint32(cx, tvr.value(), (uint32_t *)lengthp);
}

static JSBool
IndexToValue(JSContext *cx, jsdouble index, jsval *vp)
{
    return js_NewWeaklyRootedNumber(cx, index, vp);
}

JSBool JS_FASTCALL
js_IndexToId(JSContext *cx, jsuint index, jsid *idp)
{
    JSString *str;

    if (index <= JSVAL_INT_MAX) {
        *idp = INT_TO_JSID(index);
        return JS_TRUE;
    }
    str = js_NumberToString(cx, index);
    if (!str)
        return JS_FALSE;
    return js_ValueToStringId(cx, STRING_TO_JSVAL(str), idp);
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
        ((clasp = obj->getClass()) == &js_SlowArrayClass ||
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
ResizeSlots(JSContext *cx, JSObject *obj, uint32 oldlen, uint32 newlen,
            bool initializeAllSlots = true)
{
    jsval *slots, *newslots;

    if (newlen == 0) {
        if (obj->dslots) {
            cx->free(obj->dslots - 1);
            obj->dslots = NULL;
        }
        return JS_TRUE;
    }

    if (newlen > MAX_DSLOTS_LENGTH) {
        js_ReportAllocationOverflow(cx);
        return JS_FALSE;
    }

    slots = obj->dslots ? obj->dslots - 1 : NULL;
    newslots = (jsval *) cx->realloc(slots, (size_t(newlen) + 1) * sizeof(jsval));
    if (!newslots)
        return JS_FALSE;

    obj->dslots = newslots + 1;
    js_SetDenseArrayCapacity(obj, newlen);

    if (initializeAllSlots) {
        for (slots = obj->dslots + oldlen; slots < obj->dslots + newlen; slots++)
            *slots = JSVAL_HOLE;
    }

    return JS_TRUE;
}








#define CAPACITY_DOUBLING_MAX    (1024 * 1024)





#define CAPACITY_CHUNK  (1024 * 1024 / sizeof(jsval))

static JSBool
EnsureCapacity(JSContext *cx, JSObject *obj, uint32 newcap,
               bool initializeAllSlots = true)
{
    uint32 oldcap = js_DenseArrayCapacity(obj);

    if (newcap > oldcap) {
        









        uint32 nextsize = (oldcap <= CAPACITY_DOUBLING_MAX)
                          ? oldcap * 2 + 1
                          : oldcap + (oldcap >> 3);

        uint32 actualCapacity = JS_MAX(newcap, nextsize);
        if (actualCapacity >= CAPACITY_CHUNK)
            actualCapacity = JS_ROUNDUP(actualCapacity + 1, CAPACITY_CHUNK) - 1; 
        else if (actualCapacity < ARRAY_CAPACITY_MIN)
            actualCapacity = ARRAY_CAPACITY_MIN;
        if (!ResizeSlots(cx, obj, oldcap, actualCapacity, initializeAllSlots))
            return JS_FALSE;
        if (!initializeAllSlots) {
            


            for (jsval *slots = obj->dslots + newcap;
                 slots < obj->dslots + actualCapacity;
                 slots++) {
                *slots = JSVAL_HOLE;
            }
        }
    }
    return JS_TRUE;
}

static bool
ReallyBigIndexToId(JSContext* cx, jsdouble index, jsid* idp)
{
    AutoValueRooter dval(cx);
    if (!js_NewDoubleInRootedValue(cx, index, dval.addr()) ||
        !js_ValueToStringId(cx, dval.value(), idp)) {
        return JS_FALSE;
    }
    return JS_TRUE;
}

static bool
IndexToId(JSContext* cx, JSObject* obj, jsdouble index, JSBool* hole, jsid* idp,
          JSBool createAtom = JS_FALSE)
{
    if (index <= JSVAL_INT_MAX) {
        *idp = INT_TO_JSID(int(index));
        return JS_TRUE;
    }

    if (index <= jsuint(-1)) {
        if (!BigIndexToId(cx, obj, jsuint(index), createAtom, idp))
            return JS_FALSE;
        if (hole && JSVAL_IS_VOID(*idp))
            *hole = JS_TRUE;
        return JS_TRUE;
    }

    return ReallyBigIndexToId(cx, index, idp);
}







static JSBool
GetArrayElement(JSContext *cx, JSObject *obj, jsdouble index, JSBool *hole,
                jsval *vp)
{
    JS_ASSERT(index >= 0);
    if (obj->isDenseArray() && index < js_DenseArrayCapacity(obj) &&
        (*vp = obj->dslots[jsuint(index)]) != JSVAL_HOLE) {
        *hole = JS_FALSE;
        return JS_TRUE;
    }

    AutoIdRooter idr(cx);

    *hole = JS_FALSE;
    if (!IndexToId(cx, obj, index, hole, idr.addr()))
        return JS_FALSE;
    if (*hole) {
        *vp = JSVAL_VOID;
        return JS_TRUE;
    }

    JSObject *obj2;
    JSProperty *prop;
    if (!obj->lookupProperty(cx, idr.id(), &obj2, &prop))
        return JS_FALSE;
    if (!prop) {
        *hole = JS_TRUE;
        *vp = JSVAL_VOID;
    } else {
        obj2->dropProperty(cx, prop);
        if (!obj->getProperty(cx, idr.id(), vp))
            return JS_FALSE;
        *hole = JS_FALSE;
    }
    return JS_TRUE;
}




static JSBool
SetArrayElement(JSContext *cx, JSObject *obj, jsdouble index, jsval v)
{
    JS_ASSERT(index >= 0);

    if (obj->isDenseArray()) {
        
        if (index <= jsuint(-1)) {
            jsuint idx = jsuint(index);
            if (!INDEX_TOO_SPARSE(obj, idx)) {
                JS_ASSERT(idx + 1 > idx);
                if (!EnsureCapacity(cx, obj, idx + 1))
                    return JS_FALSE;
                if (idx >= obj->getArrayLength())
                    obj->setArrayLength(idx + 1);
                if (obj->dslots[idx] == JSVAL_HOLE)
                    obj->incArrayCountBy(1);
                obj->dslots[idx] = v;
                return JS_TRUE;
            }
        }

        if (!js_MakeArraySlow(cx, obj))
            return JS_FALSE;
    }

    AutoIdRooter idr(cx);

    if (!IndexToId(cx, obj, index, NULL, idr.addr(), JS_TRUE))
        return JS_FALSE;
    JS_ASSERT(!JSVAL_IS_VOID(idr.id()));

    return obj->setProperty(cx, idr.id(), &v);
}

static JSBool
DeleteArrayElement(JSContext *cx, JSObject *obj, jsdouble index)
{
    JS_ASSERT(index >= 0);
    if (obj->isDenseArray()) {
        if (index <= jsuint(-1)) {
            jsuint idx = jsuint(index);
            if (!INDEX_TOO_SPARSE(obj, idx) && idx < js_DenseArrayCapacity(obj)) {
                if (obj->dslots[idx] != JSVAL_HOLE)
                    obj->decArrayCountBy(1);
                obj->dslots[idx] = JSVAL_HOLE;
                return JS_TRUE;
            }
        }
        return JS_TRUE;
    }

    AutoIdRooter idr(cx);

    if (!IndexToId(cx, obj, index, NULL, idr.addr()))
        return JS_FALSE;
    if (JSVAL_IS_VOID(idr.id()))
        return JS_TRUE;

    jsval junk;
    return obj->deleteProperty(cx, idr.id(), &junk);
}





static JSBool
SetOrDeleteArrayElement(JSContext *cx, JSObject *obj, jsdouble index,
                        JSBool hole, jsval v)
{
    if (hole) {
        JS_ASSERT(JSVAL_IS_VOID(v));
        return DeleteArrayElement(cx, obj, index);
    }
    return SetArrayElement(cx, obj, index, v);
}

JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsdouble length)
{
    jsval v;
    jsid id;

    if (!IndexToValue(cx, length, &v))
        return JS_FALSE;
    id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    return obj->setProperty(cx, id, &v);
}

JSBool
js_HasLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp)
{
    JSErrorReporter older = JS_SetErrorReporter(cx, NULL);
    AutoValueRooter tvr(cx, JSVAL_NULL);
    jsid id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
    JSBool ok = obj->getProperty(cx, id, tvr.addr());
    JS_SetErrorReporter(cx, older);
    if (!ok)
        return false;

    *lengthp = ValueIsLength(cx, tvr.addr());
    return !JSVAL_IS_NULL(tvr.value());
}

JSBool
js_IsArrayLike(JSContext *cx, JSObject *obj, JSBool *answerp, jsuint *lengthp)
{
    JSObject *wrappedObj = js_GetWrappedObject(cx, obj);

    *answerp = wrappedObj->isArguments() || wrappedObj->isArray();
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
        if (obj->isArray())
            return IndexToValue(cx, obj->getArrayLength(), vp);
    } while ((obj = obj->getProto()) != NULL);
    return JS_TRUE;
}

static JSBool
array_length_setter(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsuint newlen, oldlen, gap, index;
    jsval junk;

    if (!obj->isArray()) {
        jsid lengthId = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);

        return obj->defineProperty(cx, lengthId, *vp, NULL, NULL, JSPROP_ENUMERATE);
    }

    newlen = ValueIsLength(cx, vp);
    if (JSVAL_IS_NULL(*vp))
        return false;
    oldlen = obj->getArrayLength();

    if (oldlen == newlen)
        return true;

    if (!IndexToValue(cx, newlen, vp))
        return false;

    if (oldlen < newlen) {
        obj->setArrayLength(newlen);
        return true;
    }

    if (obj->isDenseArray()) {
        
        jsuint capacity = js_DenseArrayCapacity(obj);
        if (capacity > newlen && !ResizeSlots(cx, obj, capacity, newlen))
            return false;
    } else if (oldlen - newlen < (1 << 24)) {
        do {
            --oldlen;
            if (!JS_CHECK_OPERATION_LIMIT(cx) || !DeleteArrayElement(cx, obj, oldlen))
                return false;
        } while (oldlen != newlen);
    } else {
        






        JSObject *iter = JS_NewPropertyIterator(cx, obj);
        if (!iter)
            return false;

        
        AutoValueRooter tvr(cx, iter);

        gap = oldlen - newlen;
        for (;;) {
            if (!JS_CHECK_OPERATION_LIMIT(cx) || !JS_NextProperty(cx, iter, &id))
                return false;
            if (JSVAL_IS_VOID(id))
                break;
            if (js_IdIsIndex(id, &index) && index - newlen < gap &&
                !obj->deleteProperty(cx, id, &junk)) {
                return false;
            }
        }
    }

    obj->setArrayLength(newlen);
    return true;
}





static inline bool
IsDenseArrayId(JSContext *cx, JSObject *obj, jsid id)
{
    JS_ASSERT(obj->isDenseArray());

    uint32 i;
    return id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom) ||
           (js_IdIsIndex(id, &i) &&
            obj->getArrayLength() != 0 &&
            i < js_DenseArrayCapacity(obj) &&
            obj->dslots[i] != JSVAL_HOLE);
}

static JSBool
array_lookupProperty(JSContext *cx, JSObject *obj, jsid id, JSObject **objp,
                     JSProperty **propp)
{
    if (!obj->isDenseArray())
        return js_LookupProperty(cx, obj, id, objp, propp);

    if (IsDenseArrayId(cx, obj, id)) {
        *propp = (JSProperty *) id;
        *objp = obj;
        return JS_TRUE;
    }

    JSObject *proto = obj->getProto();
    if (!proto) {
        *objp = NULL;
        *propp = NULL;
        return JS_TRUE;
    }
    return proto->lookupProperty(cx, id, objp, propp);
}

static void
array_dropProperty(JSContext *cx, JSObject *obj, JSProperty *prop)
{
    JS_ASSERT(IsDenseArrayId(cx, obj, (jsid) prop));
}

JSBool
js_GetDenseArrayElementValue(JSContext *cx, JSObject *obj, JSProperty *prop,
                             jsval *vp)
{
    jsid id = (jsid) prop;
    JS_ASSERT(IsDenseArrayId(cx, obj, id));

    uint32 i;
    if (!js_IdIsIndex(id, &i)) {
        JS_ASSERT(id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom));
        return IndexToValue(cx, obj->getArrayLength(), vp);
    }
    *vp = obj->dslots[i];
    return JS_TRUE;
}

static JSBool
array_getProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    uint32 i;

    if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))
        return IndexToValue(cx, obj->getArrayLength(), vp);

    if (id == ATOM_TO_JSID(cx->runtime->atomState.protoAtom)) {
        *vp = OBJECT_TO_JSVAL(obj->getProto());
        return JS_TRUE;
    }

    if (!obj->isDenseArray())
        return js_GetProperty(cx, obj, id, vp);

    if (!js_IdIsIndex(ID_TO_VALUE(id), &i) || i >= js_DenseArrayCapacity(obj) ||
        obj->dslots[i] == JSVAL_HOLE) {
        JSObject *obj2;
        JSProperty *prop;
        JSScopeProperty *sprop;

        JSObject *proto = obj->getProto();
        if (!proto) {
            *vp = JSVAL_VOID;
            return JS_TRUE;
        }

        *vp = JSVAL_VOID;
        if (js_LookupPropertyWithFlags(cx, proto, id, cx->resolveFlags,
                                       &obj2, &prop) < 0)
            return JS_FALSE;

        if (prop) {
            if (obj2->isNative()) {
                sprop = (JSScopeProperty *) prop;
                if (!js_NativeGet(cx, obj, obj2, sprop, JSGET_METHOD_BARRIER, vp))
                    return JS_FALSE;
            }
            obj2->dropProperty(cx, prop);
        }
        return JS_TRUE;
    }

    *vp = obj->dslots[i];
    return JS_TRUE;
}

static JSBool
slowarray_addProperty(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    jsuint index, length;

    if (!js_IdIsIndex(id, &index))
        return JS_TRUE;
    length = obj->getArrayLength();
    if (index >= length)
        obj->setArrayLength(index + 1);
    return JS_TRUE;
}

static JSBool
slowarray_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                    jsval *statep, jsid *idp);

static JSType
array_typeOf(JSContext *cx, JSObject *obj)
{
    return JSTYPE_OBJECT;
}


static JSObjectOps js_SlowArrayObjectOps = {
    NULL,
    js_LookupProperty,      js_DefineProperty,
    js_GetProperty,         js_SetProperty,
    js_GetAttributes,       js_SetAttributes,
    js_DeleteProperty,      js_DefaultValue,
    slowarray_enumerate,    js_CheckAccess,
    array_typeOf,           js_TraceObject,
    NULL,                   NATIVE_DROP_PROPERTY,
    NULL,                   js_Construct,
    js_HasInstance,         js_Clear
};

static JSObjectOps *
slowarray_getObjectOps(JSContext *cx, JSClass *clasp)
{
    return &js_SlowArrayObjectOps;
}

static JSBool
array_setProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
{
    uint32 i;

    if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))
        return array_length_setter(cx, obj, id, vp);

    if (!obj->isDenseArray())
        return js_SetProperty(cx, obj, id, vp);

    if (!js_IdIsIndex(id, &i) || INDEX_TOO_SPARSE(obj, i)) {
        if (!js_MakeArraySlow(cx, obj))
            return JS_FALSE;
        return js_SetProperty(cx, obj, id, vp);
    }

    if (!EnsureCapacity(cx, obj, i + 1))
        return JS_FALSE;

    if (i >= obj->getArrayLength())
        obj->setArrayLength(i + 1);
    if (obj->dslots[i] == JSVAL_HOLE)
        obj->incArrayCountBy(1);
    obj->dslots[i] = *vp;
    return JS_TRUE;
}

JSBool
js_PrototypeHasIndexedProperties(JSContext *cx, JSObject *obj)
{
    




    while ((obj = obj->getProto()) != NULL) {
        




        if (!obj->isNative())
            return JS_TRUE;
        if (obj->scope()->hadIndexedProperties())
            return JS_TRUE;
    }
    return JS_FALSE;
}

#ifdef JS_TRACER

static inline JSBool FASTCALL
dense_grow(JSContext* cx, JSObject* obj, jsint i, jsval v)
{
    


    JS_ASSERT((MAX_DSLOTS_LENGTH > MAX_DSLOTS_LENGTH32) == (sizeof(jsval) != sizeof(uint32)));
    if (MAX_DSLOTS_LENGTH > MAX_DSLOTS_LENGTH32) {
        



        if (i < 0)
            return JS_FALSE;
    }

    


    jsuint u = jsuint(i);
    jsuint capacity = js_DenseArrayCapacity(obj);
    if ((u >= capacity) && (INDEX_TOO_SPARSE(obj, u) || !EnsureCapacity(cx, obj, u + 1)))
        return JS_FALSE;

    if (obj->dslots[u] == JSVAL_HOLE) {
        if (js_PrototypeHasIndexedProperties(cx, obj))
            return JS_FALSE;

        if (u >= obj->getArrayLength())
            obj->setArrayLength(u + 1);
        obj->incArrayCountBy(1);
    }

    obj->dslots[u] = v;
    return JS_TRUE;
}


JSBool FASTCALL
js_Array_dense_setelem(JSContext* cx, JSObject* obj, jsint i, jsval v)
{
    JS_ASSERT(obj->isDenseArray());
    return dense_grow(cx, obj, i, v);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_Array_dense_setelem, CONTEXT, OBJECT, INT32, JSVAL, 0,
                     nanojit::ACC_STORE_ANY)

JSBool FASTCALL
js_Array_dense_setelem_int(JSContext* cx, JSObject* obj, jsint i, int32 j)
{
    JS_ASSERT(obj->isDenseArray());

    jsval v;
    if (JS_LIKELY(INT_FITS_IN_JSVAL(j))) {
        v = INT_TO_JSVAL(j);
    } else {
        jsdouble d = (jsdouble)j;
        if (!js_NewDoubleInRootedValue(cx, d, &v))
            return JS_FALSE;
    }

    return dense_grow(cx, obj, i, v);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_Array_dense_setelem_int, CONTEXT, OBJECT, INT32, INT32, 0,
                     nanojit::ACC_STORE_ANY)

JSBool FASTCALL
js_Array_dense_setelem_double(JSContext* cx, JSObject* obj, jsint i, jsdouble d)
{
    JS_ASSERT(obj->isDenseArray());

    jsval v;
    jsint j;

    if (JS_LIKELY(JSDOUBLE_IS_INT(d, j) && INT_FITS_IN_JSVAL(j))) {
        v = INT_TO_JSVAL(j);
    } else {
        if (!js_NewDoubleInRootedValue(cx, d, &v))
            return JS_FALSE;
    }

    return dense_grow(cx, obj, i, v);
}
JS_DEFINE_CALLINFO_4(extern, BOOL, js_Array_dense_setelem_double, CONTEXT, OBJECT, INT32, DOUBLE,
                     0, nanojit::ACC_STORE_ANY)
#endif

static JSBool
array_defineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                     JSPropertyOp getter, JSPropertyOp setter, uintN attrs)
{
    uint32 i = 0;       
    JSBool isIndex;

    if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))
        return JS_TRUE;

    isIndex = js_IdIsIndex(ID_TO_VALUE(id), &i);
    if (!isIndex || attrs != JSPROP_ENUMERATE || !obj->isDenseArray() || INDEX_TOO_SPARSE(obj, i)) {
        if (!ENSURE_SLOW_ARRAY(cx, obj))
            return JS_FALSE;
        return js_DefineProperty(cx, obj, id, value, getter, setter, attrs);
    }

    return array_setProperty(cx, obj, id, &value);
}

static JSBool
array_getAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                    uintN *attrsp)
{
    *attrsp = id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)
        ? JSPROP_PERMANENT : JSPROP_ENUMERATE;
    return JS_TRUE;
}

static JSBool
array_setAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                    uintN *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_CANT_SET_ARRAY_ATTRS);
    return JS_FALSE;
}

static JSBool
array_deleteProperty(JSContext *cx, JSObject *obj, jsval id, jsval *rval)
{
    uint32 i;

    if (!obj->isDenseArray())
        return js_DeleteProperty(cx, obj, id, rval);

    if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
        *rval = JSVAL_FALSE;
        return JS_TRUE;
    }

    if (js_IdIsIndex(id, &i) && i < js_DenseArrayCapacity(obj) &&
        obj->dslots[i] != JSVAL_HOLE) {
        obj->decArrayCountBy(1);
        obj->dslots[i] = JSVAL_HOLE;
    }

    *rval = JSVAL_TRUE;
    return JS_TRUE;
}




























#define PACKED_UINT_PAIR_BITS           14
#define PACKED_UINT_PAIR_MASK           JS_BITMASK(PACKED_UINT_PAIR_BITS)

#define UINT_PAIR_TO_SPECIAL_JSVAL(i,j)                                \
    (JS_ASSERT((uint32) (i) <= PACKED_UINT_PAIR_MASK),                        \
     JS_ASSERT((uint32) (j) <= PACKED_UINT_PAIR_MASK),                        \
     ((jsval) (i) << (PACKED_UINT_PAIR_BITS + JSVAL_TAGBITS)) |               \
     ((jsval) (j) << (JSVAL_TAGBITS)) |                                       \
     (jsval) JSVAL_SPECIAL)

#define SPECIAL_JSVAL_TO_UINT_PAIR(v,i,j)                              \
    (JS_ASSERT(JSVAL_IS_SPECIAL(v)),                                   \
     (i) = (uint32) ((v) >> (PACKED_UINT_PAIR_BITS + JSVAL_TAGBITS)),         \
     (j) = (uint32) ((v) >> JSVAL_TAGBITS) & PACKED_UINT_PAIR_MASK,           \
     JS_ASSERT((i) <= PACKED_UINT_PAIR_MASK))

JS_STATIC_ASSERT(PACKED_UINT_PAIR_BITS * 2 + JSVAL_TAGBITS <= JS_BITS_PER_WORD);

typedef struct JSIndexIterState {
    uint32          index;
    uint32          length;
    JSBool          hasHoles;

    



    jsbitmap        holes[1];
} JSIndexIterState;

#define INDEX_ITER_TAG      3

JS_STATIC_ASSERT(JSVAL_INT == 1);

static JSBool
array_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                jsval *statep, jsid *idp)
{
    uint32 capacity, i;
    JSIndexIterState *ii;

    switch (enum_op) {
      case JSENUMERATE_INIT:
        JS_ASSERT(obj->isDenseArray());
        capacity = js_DenseArrayCapacity(obj);
        if (idp)
            *idp = INT_TO_JSVAL(obj->getArrayCount());
        ii = NULL;
        for (i = 0; i != capacity; ++i) {
            if (obj->dslots[i] == JSVAL_HOLE) {
                if (!ii) {
                    ii = (JSIndexIterState *)
                         cx->malloc(offsetof(JSIndexIterState, holes) +
                                   JS_BITMAP_SIZE(capacity));
                    if (!ii)
                        return JS_FALSE;
                    ii->hasHoles = JS_TRUE;
                    memset(ii->holes, 0, JS_BITMAP_SIZE(capacity));
                }
                JS_SET_BIT(ii->holes, i);
            }
        }
        if (!ii) {
            
            if (capacity <= PACKED_UINT_PAIR_MASK) {
                *statep = UINT_PAIR_TO_SPECIAL_JSVAL(0, capacity);
                break;
            }
            ii = (JSIndexIterState *)
                 cx->malloc(offsetof(JSIndexIterState, holes));
            if (!ii)
                return JS_FALSE;
            ii->hasHoles = JS_FALSE;
        }
        ii->index = 0;
        ii->length = capacity;
        *statep = (jsval) ii | INDEX_ITER_TAG;
        JS_ASSERT(*statep & JSVAL_INT);
        break;

      case JSENUMERATE_NEXT:
        if (JSVAL_IS_SPECIAL(*statep)) {
            SPECIAL_JSVAL_TO_UINT_PAIR(*statep, i, capacity);
            if (i != capacity) {
                *idp = INT_TO_JSID(i);
                *statep = UINT_PAIR_TO_SPECIAL_JSVAL(i + 1, capacity);
                break;
            }
        } else {
            JS_ASSERT((*statep & INDEX_ITER_TAG) == INDEX_ITER_TAG);
            ii = (JSIndexIterState *) (*statep & ~INDEX_ITER_TAG);
            i = ii->index;
            if (i != ii->length) {
                
                if (ii->hasHoles) {
                    while (JS_TEST_BIT(ii->holes, i) && ++i != ii->length)
                        continue;
                }
                if (i != ii->length) {
                    ii->index = i + 1;
                    return js_IndexToId(cx, i, idp);
                }
            }
        }
        

      case JSENUMERATE_DESTROY:
        if (!JSVAL_IS_SPECIAL(*statep)) {
            JS_ASSERT((*statep & INDEX_ITER_TAG) == INDEX_ITER_TAG);
            ii = (JSIndexIterState *) (*statep & ~INDEX_ITER_TAG);
            cx->free(ii);
        }
        *statep = JSVAL_NULL;
        break;
    }
    return JS_TRUE;
}

static JSBool
slowarray_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                    jsval *statep, jsid *idp)
{
    JSBool ok;

    
    if (enum_op != JSENUMERATE_INIT) {
        if (JSVAL_IS_SPECIAL(*statep) ||
            (*statep & INDEX_ITER_TAG) == INDEX_ITER_TAG) {
            return array_enumerate(cx, obj, enum_op, statep, idp);
        }
        JS_ASSERT((*statep & INDEX_ITER_TAG) == JSVAL_INT);
    }
    ok = js_Enumerate(cx, obj, enum_op, statep, idp);
    JS_ASSERT(*statep == JSVAL_NULL || (*statep & INDEX_ITER_TAG) == JSVAL_INT);
    return ok;
}

static void
array_finalize(JSContext *cx, JSObject *obj)
{
    if (obj->dslots)
        cx->free(obj->dslots - 1);
    obj->dslots = NULL;
}

static void
array_trace(JSTracer *trc, JSObject *obj)
{
    uint32 capacity;
    size_t i;
    jsval v;

    JS_ASSERT(obj->isDenseArray());
    obj->traceProtoAndParent(trc);

    capacity = js_DenseArrayCapacity(obj);
    for (i = 0; i < capacity; i++) {
        v = obj->dslots[i];
        if (JSVAL_IS_TRACEABLE(v)) {
            JS_SET_TRACING_INDEX(trc, "array_dslots", i);
            js_CallGCMarker(trc, JSVAL_TO_TRACEABLE(v), JSVAL_TRACE_KIND(v));
        }
    }
}

extern JSObjectOps js_ArrayObjectOps;

static const JSObjectMap SharedArrayMap(&js_ArrayObjectOps, JSObjectMap::SHAPELESS);

JSObjectOps js_ArrayObjectOps = {
    &SharedArrayMap,
    array_lookupProperty, array_defineProperty,
    array_getProperty,    array_setProperty,
    array_getAttributes,  array_setAttributes,
    array_deleteProperty, js_DefaultValue,
    array_enumerate,      js_CheckAccess,
    array_typeOf,         array_trace,
    NULL,                 array_dropProperty,
    NULL,                 NULL,
    js_HasInstance,       NULL
};

static JSObjectOps *
array_getObjectOps(JSContext *cx, JSClass *clasp)
{
    return &js_ArrayObjectOps;
}

JSClass js_ArrayClass = {
    "Array",
    JSCLASS_HAS_RESERVED_SLOTS(2) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Array) |
    JSCLASS_NEW_ENUMERATE,
    JS_PropertyStub,    JS_PropertyStub,   JS_PropertyStub,   JS_PropertyStub,
    JS_EnumerateStub,   JS_ResolveStub,    js_TryValueOf,     array_finalize,
    array_getObjectOps, NULL,              NULL,              NULL,
    NULL,               NULL,              NULL,              NULL
};

JSClass js_SlowArrayClass = {
    "Array",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_CACHED_PROTO(JSProto_Array),
    slowarray_addProperty, JS_PropertyStub, JS_PropertyStub,  JS_PropertyStub,
    JS_EnumerateStub,      JS_ResolveStub,  js_TryValueOf,    NULL,
    slowarray_getObjectOps, NULL,           NULL,             NULL,
    NULL,                  NULL,            NULL,             NULL
};




JSBool
js_MakeArraySlow(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->getClass() == &js_ArrayClass);

    



    uint32 emptyShape;
    JSObject *arrayProto = obj->getProto();
    if (arrayProto->getClass() == &js_ObjectClass) {
        
        emptyShape = js_GenerateShape(cx, false);
    } else {
        
        JS_ASSERT(arrayProto->getClass() == &js_SlowArrayClass);
        emptyShape = arrayProto->scope()->emptyScope->shape;
    }
    JSScope *scope = JSScope::create(cx, &js_SlowArrayObjectOps, &js_SlowArrayClass, obj,
                                     emptyShape);
    if (!scope)
        return JS_FALSE;

    uint32 capacity = js_DenseArrayCapacity(obj);
    if (capacity) {
        scope->freeslot = obj->numSlots() + JS_INITIAL_NSLOTS;
        obj->dslots[-1] = JS_INITIAL_NSLOTS + capacity;
    } else {
        scope->freeslot = obj->numSlots();
    }

    
    for (uint32 i = 0; i < capacity; i++) {
        jsid id;
        JSScopeProperty *sprop;

        if (!JS_ValueToId(cx, INT_TO_JSVAL(i), &id))
            goto out_bad;

        if (obj->dslots[i] == JSVAL_HOLE) {
            obj->dslots[i] = JSVAL_VOID;
            continue;
        }

        sprop = scope->addDataProperty(cx, id, JS_INITIAL_NSLOTS + i,
                                       JSPROP_ENUMERATE);
        if (!sprop)
            goto out_bad;
    }

    





    JS_ASSERT(js_SlowArrayClass.flags & JSCLASS_HAS_PRIVATE);
    obj->voidDenseArrayCount();

    
    obj->classword ^= (jsuword) &js_ArrayClass;
    obj->classword |= (jsuword) &js_SlowArrayClass;

    obj->map = scope;
    return JS_TRUE;

  out_bad:
    scope->destroy(cx);
    return JS_FALSE;
}


static inline JSBool
BufferToString(JSContext *cx, JSCharBuffer &cb, jsval *rval)
{
    JSString *str = js_NewStringFromCharBuffer(cx, cb);
    if (!str)
        return false;
    *rval = STRING_TO_JSVAL(str);
    return true;
}

#if JS_HAS_TOSOURCE
static JSBool
array_toSource(JSContext *cx, uintN argc, jsval *vp)
{
    JS_CHECK_RECURSION(cx, return false);

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj ||
        (obj->getClass() != &js_SlowArrayClass &&
         !JS_InstanceOf(cx, obj, &js_ArrayClass, vp + 2))) {
        return false;
    }

    
    jschar *sharpchars;
    JSHashEntry *he = js_EnterSharpObject(cx, obj, NULL, &sharpchars);
    if (!he)
        return false;
    bool initiallySharp = IS_SHARP(he);

    
    MUST_FLOW_THROUGH("out");
    bool ok = false;

    



    JSCharBuffer cb(cx);

    
#if JS_HAS_SHARP_VARS
    if (IS_SHARP(he)) {
        JS_ASSERT(sharpchars != 0);
        cb.replaceRawBuffer(sharpchars, js_strlen(sharpchars));
        goto make_string;
    } else if (sharpchars) {
        MAKE_SHARP(he);
        cb.replaceRawBuffer(sharpchars, js_strlen(sharpchars));
    }
#else
    if (IS_SHARP(he)) {
        if (!js_AppendLiteral(cb, "[]"))
            goto out;
        cx->free(sharpchars);
        goto make_string;
    }
#endif

    if (!cb.append('['))
        goto out;

    jsuint length;
    if (!js_GetLengthProperty(cx, obj, &length))
        goto out;

    for (jsuint index = 0; index < length; index++) {
        
        JSBool hole;
        if (!JS_CHECK_OPERATION_LIMIT(cx) ||
            !GetArrayElement(cx, obj, index, &hole, vp)) {
            goto out;
        }

        
        JSString *str;
        if (hole) {
            str = cx->runtime->emptyString;
        } else {
            str = js_ValueToSource(cx, *vp);
            if (!str)
                goto out;
        }
        *vp = STRING_TO_JSVAL(str);
        const jschar *chars;
        size_t charlen;
        str->getCharsAndLength(chars, charlen);

        
        if (!cb.append(chars, charlen))
            goto out;
        if (index + 1 != length) {
            if (!js_AppendLiteral(cb, ", "))
                goto out;
        } else if (hole) {
            if (!cb.append(','))
                goto out;
        }
    }

    
    if (!cb.append(']'))
        goto out;

  make_string:
    if (!BufferToString(cx, cb, vp))
        goto out;

    ok = true;

  out:
    if (!initiallySharp)
        js_LeaveSharpObject(cx, NULL);
    return ok;
}
#endif

static JSBool
array_toString_sub(JSContext *cx, JSObject *obj, JSBool locale,
                   JSString *sepstr, jsval *rval)
{
    JS_CHECK_RECURSION(cx, return false);

    



    typedef js::HashSet<JSObject *> ObjSet;
    ObjSet::AddPtr hashp = cx->busyArrays.lookupForAdd(obj);
    uint32 genBefore;
    if (!hashp) {
        
        if (!cx->busyArrays.add(hashp, obj)) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
        genBefore = cx->busyArrays.generation();
    } else {
        
        *rval = ATOM_KEY(cx->runtime->atomState.emptyAtom);
        return true;
    }

    AutoValueRooter tvr(cx, obj);

    
    MUST_FLOW_THROUGH("out");
    bool ok = false;

    
    static const jschar comma = ',';
    const jschar *sep;
    size_t seplen;
    if (sepstr) {
        sepstr->getCharsAndLength(sep, seplen);
    } else {
        sep = &comma;
        seplen = 1;
    }

    



    JSCharBuffer cb(cx);

    jsuint length;
    if (!js_GetLengthProperty(cx, obj, &length))
        goto out;

    for (jsuint index = 0; index < length; index++) {
        
        JSBool hole;
        if (!JS_CHECK_OPERATION_LIMIT(cx) ||
            !GetArrayElement(cx, obj, index, &hole, rval)) {
            goto out;
        }

        
        if (!(hole || JSVAL_IS_VOID(*rval) || JSVAL_IS_NULL(*rval))) {
            if (locale) {
                
                JSObject *robj;

                if (!js_ValueToObject(cx, *rval, &robj))
                    goto out;
                *rval = OBJECT_TO_JSVAL(robj);
                JSAtom *atom = cx->runtime->atomState.toLocaleStringAtom;
                if (!js_TryMethod(cx, robj, atom, 0, NULL, rval))
                    goto out;
            }

            if (!js_ValueToCharBuffer(cx, *rval, cb))
                goto out;
        }

        
        if (index + 1 != length) {
            if (!cb.append(sep, seplen))
                goto out;
        }
    }

    
    if (!BufferToString(cx, cb, rval))
        goto out;

    ok = true;

  out:
    if (genBefore == cx->busyArrays.generation())
        cx->busyArrays.remove(hashp);
    else
        cx->busyArrays.remove(obj);
    return ok;
}

static JSBool
array_toString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj ||
        (obj->getClass() != &js_SlowArrayClass &&
         !JS_InstanceOf(cx, obj, &js_ArrayClass, vp + 2))) {
        return JS_FALSE;
    }

    return array_toString_sub(cx, obj, JS_FALSE, NULL, vp);
}

static JSBool
array_toLocaleString(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj ||
        (obj->getClass() != &js_SlowArrayClass &&
         !JS_InstanceOf(cx, obj, &js_ArrayClass, vp + 2))) {
        return JS_FALSE;
    }

    



    return array_toString_sub(cx, obj, JS_TRUE, NULL, vp);
}

enum TargetElementsType {
    TargetElementsAllHoles,
    TargetElementsMayContainValues
};

enum SourceVectorType {
    SourceVectorAllValues,
    SourceVectorMayContainHoles
};

static JSBool
InitArrayElements(JSContext *cx, JSObject *obj, jsuint start, jsuint count, jsval *vector,
                  TargetElementsType targetType, SourceVectorType vectorType)
{
    JS_ASSERT(count < MAXINDEX);

    



    if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj) &&
        start <= MAXINDEX - count && !INDEX_TOO_BIG(start + count)) {

#ifdef DEBUG_jwalden
        {
            
            AutoIdRooter idr(cx);
            for (jsuint i = 0; i < count; i++) {
                JS_ASSERT_IF(vectorType == SourceVectorAllValues, vector[i] != JSVAL_HOLE);

                jsdouble index = jsdouble(start) + i;
                if (targetType == TargetElementsAllHoles && index < jsuint(-1)) {
                    JS_ASSERT(ReallyBigIndexToId(cx, index, idr.addr()));
                    JSObject* obj2;
                    JSProperty* prop;
                    JS_ASSERT(obj->lookupProperty(cx, idr.id(), &obj2, &prop));
                    JS_ASSERT(!prop);
                }
            }
        }
#endif

        jsuint newlen = start + count;
        JS_ASSERT(jsdouble(start) + count == jsdouble(newlen));
        if (!EnsureCapacity(cx, obj, newlen))
            return JS_FALSE;

        if (newlen > obj->getArrayLength())
            obj->setArrayLength(newlen);

        JS_ASSERT(count < size_t(-1) / sizeof(jsval));
        if (targetType == TargetElementsMayContainValues) {
            jsuint valueCount = 0;
            for (jsuint i = 0; i < count; i++) {
                 if (obj->dslots[start + i] != JSVAL_HOLE)
                     valueCount++;
            }
            JS_ASSERT(obj->getArrayCount() >= valueCount);
            obj->decArrayCountBy(valueCount);
        }
        memcpy(obj->dslots + start, vector, sizeof(jsval) * count);
        if (vectorType == SourceVectorAllValues) {
            obj->incArrayCountBy(count);
        } else {
            jsuint valueCount = 0;
            for (jsuint i = 0; i < count; i++) {
                 if (obj->dslots[start + i] != JSVAL_HOLE)
                     valueCount++;
            }
            obj->incArrayCountBy(valueCount);
        }
        JS_ASSERT_IF(count != 0, obj->dslots[newlen - 1] != JSVAL_HOLE);
        return JS_TRUE;
    }

    jsval* end = vector + count;
    while (vector != end && start < MAXINDEX) {
        if (!JS_CHECK_OPERATION_LIMIT(cx) ||
            !SetArrayElement(cx, obj, start++, *vector++)) {
            return JS_FALSE;
        }
    }

    if (vector == end)
        return JS_TRUE;

    
    if (obj->isDenseArray() && !ENSURE_SLOW_ARRAY(cx, obj))
        return JS_FALSE;

    JS_ASSERT(start == MAXINDEX);
    jsval tmp[2] = {JSVAL_NULL, JSVAL_NULL};
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(tmp), tmp);
    if (!js_NewDoubleInRootedValue(cx, MAXINDEX, &tmp[0]))
        return JS_FALSE;
    jsdouble *dp = JSVAL_TO_DOUBLE(tmp[0]);
    JS_ASSERT(*dp == MAXINDEX);
    AutoIdRooter idr(cx);
    do {
        tmp[1] = *vector++;
        if (!js_ValueToStringId(cx, tmp[0], idr.addr()) ||
            !obj->setProperty(cx, idr.id(), &tmp[1])) {
            return JS_FALSE;
        }
        *dp += 1;
    } while (vector != end);

    return JS_TRUE;
}

static JSBool
InitArrayObject(JSContext *cx, JSObject *obj, jsuint length, const jsval *vector,
                bool holey = false)
{
    JS_ASSERT(obj->isArray());

    obj->setArrayLength(length);

    if (vector) {
        if (!EnsureCapacity(cx, obj, length))
            return JS_FALSE;

        jsuint count = length;
        if (!holey) {
            memcpy(obj->dslots, vector, length * sizeof (jsval));
        } else {
            for (jsuint i = 0; i < length; i++) {
                if (vector[i] == JSVAL_HOLE)
                    --count;
                obj->dslots[i] = vector[i];
            }
        }
        obj->setArrayCount(count);
    } else {
        obj->setArrayCount(0);
    }
    return JS_TRUE;
}

#ifdef JS_TRACER
static JSString* FASTCALL
Array_p_join(JSContext* cx, JSObject* obj, JSString *str)
{
    AutoValueRooter tvr(cx);
    if (!array_toString_sub(cx, obj, JS_FALSE, str, tvr.addr())) {
        SetBuiltinError(cx);
        return NULL;
    }
    return JSVAL_TO_STRING(tvr.value());
}

static JSString* FASTCALL
Array_p_toString(JSContext* cx, JSObject* obj)
{
    AutoValueRooter tvr(cx);
    if (!array_toString_sub(cx, obj, JS_FALSE, NULL, tvr.addr())) {
        SetBuiltinError(cx);
        return NULL;
    }
    return JSVAL_TO_STRING(tvr.value());
}
#endif




static JSBool
array_join(JSContext *cx, uintN argc, jsval *vp)
{
    JSString *str;
    JSObject *obj;

    if (argc == 0 || JSVAL_IS_VOID(vp[2])) {
        str = NULL;
    } else {
        str = js_ValueToString(cx, vp[2]);
        if (!str)
            return JS_FALSE;
        vp[2] = STRING_TO_JSVAL(str);
    }
    obj = JS_THIS_OBJECT(cx, vp);
    return obj && array_toString_sub(cx, obj, JS_FALSE, str, vp);
}

static JSBool
array_reverse(JSContext *cx, uintN argc, jsval *vp)
{
    jsuint len;
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &len))
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(obj);

    if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj)) {
        
        if (len == 0 || !obj->dslots)
            return JS_TRUE;

        








        if (!EnsureCapacity(cx, obj, len))
            return JS_FALSE;

        jsval* lo = &obj->dslots[0];
        jsval* hi = &obj->dslots[len - 1];
        for (; lo < hi; lo++, hi--) {
             jsval tmp = *lo;
             *lo = *hi;
             *hi = tmp;
        }

        




        return JS_TRUE;
    }

    AutoValueRooter tvr(cx);
    for (jsuint i = 0, half = len / 2; i < half; i++) {
        JSBool hole, hole2;
        if (!JS_CHECK_OPERATION_LIMIT(cx) ||
            !GetArrayElement(cx, obj, i, &hole, tvr.addr()) ||
            !GetArrayElement(cx, obj, len - i - 1, &hole2, vp) ||
            !SetOrDeleteArrayElement(cx, obj, len - i - 1, hole, tvr.value()) ||
            !SetOrDeleteArrayElement(cx, obj, i, hole2, *vp)) {
            return false;
        }
    }
    *vp = OBJECT_TO_JSVAL(obj);
    return true;
}

typedef struct MSortArgs {
    size_t       elsize;
    JSComparator cmp;
    void         *arg;
    JSBool       fastcopy;
} MSortArgs;


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

    
    fastcopy = (elsize == sizeof(jsval) &&
                (((jsuword) src | (jsuword) tmp) & JSVAL_ALIGN) == 0);
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

struct CompareArgs
{
    JSContext       *context;
    jsval           fval;
    InvokeArgsGuard args;

    CompareArgs(JSContext *cx, jsval fval)
      : context(cx), fval(fval)
    {}
};

static JS_REQUIRES_STACK JSBool
sort_compare(void *arg, const void *a, const void *b, int *result)
{
    jsval av = *(const jsval *)a, bv = *(const jsval *)b;
    CompareArgs *ca = (CompareArgs *) arg;
    JSContext *cx = ca->context;

    



    JS_ASSERT(!JSVAL_IS_VOID(av));
    JS_ASSERT(!JSVAL_IS_VOID(bv));

    if (!JS_CHECK_OPERATION_LIMIT(cx))
        return JS_FALSE;

    jsval *invokevp = ca->args.getvp();
    jsval *sp = invokevp;
    *sp++ = ca->fval;
    *sp++ = JSVAL_NULL;
    *sp++ = av;
    *sp++ = bv;

    if (!js_Invoke(cx, ca->args, 0))
        return JS_FALSE;

    jsdouble cmp;
    if (!ValueToNumber(cx, *invokevp, &cmp))
        return JS_FALSE;

    
    *result = 0;
    if (!JSDOUBLE_IS_NaN(cmp) && cmp != 0)
        *result = cmp > 0 ? 1 : -1;

    





    return JS_TRUE;
}

typedef JSBool (JS_REQUIRES_STACK *JSRedComparator)(void*, const void*,
                                                    const void*, int *);

static inline JS_IGNORE_STACK JSComparator
comparator_stack_cast(JSRedComparator func)
{
    return func;
}

static int
sort_compare_strings(void *arg, const void *a, const void *b, int *result)
{
    jsval av = *(const jsval *)a, bv = *(const jsval *)b;

    JS_ASSERT(JSVAL_IS_STRING(av));
    JS_ASSERT(JSVAL_IS_STRING(bv));
    if (!JS_CHECK_OPERATION_LIMIT((JSContext *)arg))
        return JS_FALSE;

    *result = (int) js_CompareStrings(JSVAL_TO_STRING(av), JSVAL_TO_STRING(bv));
    return JS_TRUE;
}






JS_STATIC_ASSERT(JSVAL_NULL == 0);

static JSBool
array_sort(JSContext *cx, uintN argc, jsval *vp)
{
    jsval fval;
    jsuint len, newlen, i, undefs;
    size_t elemsize;
    JSString *str;

    jsval *argv = JS_ARGV(cx, vp);
    if (argc > 0) {
        if (JSVAL_IS_PRIMITIVE(argv[0])) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_SORT_ARG);
            return false;
        }
        fval = argv[0];     
    } else {
        fval = JSVAL_NULL;
    }

    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &len))
        return false;
    if (len == 0) {
        *vp = OBJECT_TO_JSVAL(obj);
        return true;
    }

    





#if JS_BITS_PER_WORD == 32
    if (size_t(len) > size_t(-1) / (2 * sizeof(jsval))) {
        js_ReportAllocationOverflow(cx);
        return false;
    }
#endif

    










    {
        jsval *vec = (jsval *) cx->malloc(2 * size_t(len) * sizeof(jsval));
        if (!vec)
            return false;

        struct AutoFreeVector {
            AutoFreeVector(JSContext *cx, jsval *&vec) : cx(cx), vec(vec) { }
            ~AutoFreeVector() {
                cx->free(vec);
            }
            JSContext * const cx;
            jsval *&vec;
        } free(cx, vec);

        AutoArrayRooter tvr(cx, 0, vec);

        







        undefs = 0;
        newlen = 0;
        bool allStrings = true;
        for (i = 0; i < len; i++) {
            if (!JS_CHECK_OPERATION_LIMIT(cx))
                return false;

            
            JSBool hole;
            vec[newlen] = JSVAL_NULL;
            tvr.changeLength(newlen + 1);
            if (!GetArrayElement(cx, obj, i, &hole, &vec[newlen]))
                return false;

            if (hole)
                continue;

            if (JSVAL_IS_VOID(vec[newlen])) {
                ++undefs;
                continue;
            }

            allStrings = allStrings && JSVAL_IS_STRING(vec[newlen]);

            ++newlen;
        }

        if (newlen == 0)
            return true; 

        






        jsval *mergesort_tmp = vec + newlen;
        PodZero(mergesort_tmp, newlen);
        tvr.changeLength(newlen * 2);

        
        if (fval == JSVAL_NULL) {
            



            if (allStrings) {
                elemsize = sizeof(jsval);
            } else {
                

















#if JS_BITS_PER_WORD == 32
                if (size_t(newlen) > size_t(-1) / (4 * sizeof(jsval))) {
                    js_ReportAllocationOverflow(cx);
                    return false;
                }
#endif

                





                i = newlen;
                do {
                    --i;
                    if (!JS_CHECK_OPERATION_LIMIT(cx))
                        return false;
                    jsval v = vec[i];
                    str = js_ValueToString(cx, v);
                    if (!str)
                        return false;
                    vec[2 * i] = STRING_TO_JSVAL(str);
                    vec[2 * i + 1] = v;
                } while (i != 0);

                JS_ASSERT(tvr.array == vec);
                vec = (jsval *) cx->realloc(vec, 4 * size_t(newlen) * sizeof(jsval));
                if (!vec) {
                    vec = tvr.array;
                    return false;
                }
                mergesort_tmp = vec + 2 * newlen;
                PodZero(mergesort_tmp, newlen * 2);
                tvr.changeArray(vec, newlen * 4);
                elemsize = 2 * sizeof(jsval);
            }
            if (!js_MergeSort(vec, size_t(newlen), elemsize,
                              sort_compare_strings, cx, mergesort_tmp)) {
                return false;
            }
            if (!allStrings) {
                





                i = 0;
                do {
                    vec[i] = vec[2 * i + 1];
                } while (++i != newlen);
            }
        } else {
            LeaveTrace(cx);

            CompareArgs ca(cx, fval);
            if (!cx->stack().pushInvokeArgs(cx, 2, ca.args))
                return false;

            if (!js_MergeSort(vec, size_t(newlen), sizeof(jsval),
                              comparator_stack_cast(sort_compare),
                              &ca, mergesort_tmp)) {
                return false;
            }
        }

        




        tvr.changeLength(newlen);
        if (!InitArrayElements(cx, obj, 0, newlen, vec, TargetElementsMayContainValues,
                               SourceVectorAllValues)) {
            return false;
        }
    }

    
    while (undefs != 0) {
        --undefs;
        if (!JS_CHECK_OPERATION_LIMIT(cx) || !SetArrayElement(cx, obj, newlen++, JSVAL_VOID))
            return false;
    }

    
    while (len > newlen) {
        if (!JS_CHECK_OPERATION_LIMIT(cx) || !DeleteArrayElement(cx, obj, --len))
            return JS_FALSE;
    }
    *vp = OBJECT_TO_JSVAL(obj);
    return true;
}




static JSBool
array_push_slowly(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint length;

    if (!js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (!InitArrayElements(cx, obj, length, argc, argv, TargetElementsMayContainValues,
                           SourceVectorAllValues)) {
        return JS_FALSE;
    }

    
    jsdouble newlength = length + jsdouble(argc);
    if (!IndexToValue(cx, newlength, rval))
        return JS_FALSE;
    return js_SetLengthProperty(cx, obj, newlength);
}

static JSBool
array_push1_dense(JSContext* cx, JSObject* obj, jsval v, jsval *rval)
{
    uint32 length = obj->getArrayLength();
    if (INDEX_TOO_SPARSE(obj, length)) {
        if (!js_MakeArraySlow(cx, obj))
            return JS_FALSE;
        return array_push_slowly(cx, obj, 1, &v, rval);
    }

    if (!EnsureCapacity(cx, obj, length + 1))
        return JS_FALSE;
    obj->setArrayLength(length + 1);

    JS_ASSERT(obj->dslots[length] == JSVAL_HOLE);
    obj->incArrayCountBy(1);
    obj->dslots[length] = v;
    return IndexToValue(cx, obj->getArrayLength(), rval);
}

JSBool JS_FASTCALL
js_ArrayCompPush(JSContext *cx, JSObject *obj, jsval v)
{
    JS_ASSERT(obj->isDenseArray());
    uint32_t length = obj->getArrayLength();
    JS_ASSERT(length <= js_DenseArrayCapacity(obj));

    if (length == js_DenseArrayCapacity(obj)) {
        if (length > JS_ARGS_LENGTH_MAX) {
            JS_ReportErrorNumberUC(cx, js_GetErrorMessage, NULL,
                                   JSMSG_ARRAY_INIT_TOO_BIG);
            return JS_FALSE;
        }

        if (!EnsureCapacity(cx, obj, length + 1))
            return JS_FALSE;
    }
    obj->setArrayLength(length + 1);
    obj->incArrayCountBy(1);
    obj->dslots[length] = v;
    return JS_TRUE;
}
JS_DEFINE_CALLINFO_3(extern, BOOL, js_ArrayCompPush, CONTEXT, OBJECT, JSVAL, 0,
                     nanojit::ACC_STORE_ANY)

#ifdef JS_TRACER
static jsval FASTCALL
Array_p_push1(JSContext* cx, JSObject* obj, jsval v)
{
    AutoValueRooter tvr(cx, v);
    if (obj->isDenseArray()
        ? array_push1_dense(cx, obj, v, tvr.addr())
        : array_push_slowly(cx, obj, 1, tvr.addr(), tvr.addr())) {
        return tvr.value();
    }
    SetBuiltinError(cx);
    return JSVAL_VOID;
}
#endif

static JSBool
array_push(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    
    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;
    if (argc != 1 || !obj->isDenseArray())
        return array_push_slowly(cx, obj, argc, vp + 2, vp);

    return array_push1_dense(cx, obj, vp[2], vp);
}

static JSBool
array_pop_slowly(JSContext *cx, JSObject* obj, jsval *vp)
{
    jsuint index;
    JSBool hole;

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
array_pop_dense(JSContext *cx, JSObject* obj, jsval *vp)
{
    jsuint index;
    JSBool hole;

    index = obj->getArrayLength();
    if (index == 0) {
        *vp = JSVAL_VOID;
        return JS_TRUE;
    }
    index--;
    if (!GetArrayElement(cx, obj, index, &hole, vp))
        return JS_FALSE;
    if (!hole && !DeleteArrayElement(cx, obj, index))
        return JS_FALSE;
    obj->setArrayLength(index);
    return JS_TRUE;
}

#ifdef JS_TRACER
static jsval FASTCALL
Array_p_pop(JSContext* cx, JSObject* obj)
{
    AutoValueRooter tvr(cx);
    if (obj->isDenseArray()
        ? array_pop_dense(cx, obj, tvr.addr())
        : array_pop_slowly(cx, obj, tvr.addr())) {
        return tvr.value();
    }
    SetBuiltinError(cx);
    return JSVAL_VOID;
}
#endif

static JSBool
array_pop(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj)
        return JS_FALSE;
    if (obj->isDenseArray())
        return array_pop_dense(cx, obj, vp);
    return array_pop_slowly(cx, obj, vp);
}

static JSBool
array_shift(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsuint length, i;
    JSBool hole;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (length == 0) {
        *vp = JSVAL_VOID;
    } else {
        length--;

        if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj) &&
            length < js_DenseArrayCapacity(obj)) {
            if (JS_LIKELY(obj->dslots != NULL)) {
                *vp = obj->dslots[0];
                if (*vp == JSVAL_HOLE)
                    *vp = JSVAL_VOID;
                else
                    obj->decArrayCountBy(1);
                memmove(obj->dslots, obj->dslots + 1, length * sizeof(jsval));
                obj->dslots[length] = JSVAL_HOLE;
            } else {
                




                JS_ASSERT(obj->getArrayCount() == 0);
                *vp = JSVAL_VOID;
            }
        } else {
            
            if (!GetArrayElement(cx, obj, 0, &hole, vp))
                return JS_FALSE;

            
            AutoValueRooter tvr(cx);
            for (i = 0; i != length; i++) {
                if (!JS_CHECK_OPERATION_LIMIT(cx) ||
                    !GetArrayElement(cx, obj, i + 1, &hole, tvr.addr()) ||
                    !SetOrDeleteArrayElement(cx, obj, i, hole, tvr.value())) {
                    return JS_FALSE;
                }
            }

            
            if (!hole && !DeleteArrayElement(cx, obj, length))
                return JS_FALSE;
        }
    }
    return js_SetLengthProperty(cx, obj, length);
}

static JSBool
array_unshift(JSContext *cx, uintN argc, jsval *vp)
{
    JSObject *obj;
    jsval *argv;
    jsuint length;
    JSBool hole;
    jsdouble last, newlen;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    newlen = length;
    if (argc > 0) {
        
        argv = JS_ARGV(cx, vp);
        if (length > 0) {
            if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj) &&
                !INDEX_TOO_SPARSE(obj, unsigned(newlen + argc))) {
                JS_ASSERT(newlen + argc == length + argc);
                if (!EnsureCapacity(cx, obj, length + argc))
                    return JS_FALSE;
                memmove(obj->dslots + argc, obj->dslots, length * sizeof(jsval));
                for (uint32 i = 0; i < argc; i++)
                    obj->dslots[i] = JSVAL_HOLE;
            } else {
                last = length;
                jsdouble upperIndex = last + argc;
                AutoValueRooter tvr(cx);
                do {
                    --last, --upperIndex;
                    if (!JS_CHECK_OPERATION_LIMIT(cx) ||
                        !GetArrayElement(cx, obj, last, &hole, tvr.addr()) ||
                        !SetOrDeleteArrayElement(cx, obj, upperIndex, hole, tvr.value())) {
                        return JS_FALSE;
                    }
                } while (last != 0);
            }
        }

        
        if (!InitArrayElements(cx, obj, 0, argc, argv, TargetElementsAllHoles, SourceVectorAllValues))
            return JS_FALSE;

        newlen += argc;
        if (!js_SetLengthProperty(cx, obj, newlen))
            return JS_FALSE;
    }

    
    return IndexToValue(cx, newlen, vp);
}

static JSBool
array_splice(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    JSObject *obj;
    jsuint length, begin, end, count, delta, last;
    JSBool hole;
    JSObject *obj2;

    





    obj2 = js_NewArrayObject(cx, 0, NULL);
    if (!obj2)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(obj2);

    
    if (argc == 0)
        return JS_TRUE;
    argv = JS_ARGV(cx, vp);
    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    
    jsdouble d;
    if (!ValueToNumber(cx, *argv, &d))
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
        if (!ValueToNumber(cx, *argv, &d))
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

    AutoValueRooter tvr(cx, JSVAL_NULL);

    
    if (count > 0) {
        if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj) &&
            !js_PrototypeHasIndexedProperties(cx, obj2) &&
            end <= js_DenseArrayCapacity(obj)) {
            if (!InitArrayObject(cx, obj2, count, obj->dslots + begin,
                                 obj->getArrayCount() != obj->getArrayLength())) {
                return JS_FALSE;
            }
        } else {
            for (last = begin; last < end; last++) {
                if (!JS_CHECK_OPERATION_LIMIT(cx) ||
                    !GetArrayElement(cx, obj, last, &hole, tvr.addr())) {
                    return JS_FALSE;
                }

                
                if (!hole && !SetArrayElement(cx, obj2, last - begin, tvr.value()))
                    return JS_FALSE;
            }

            if (!js_SetLengthProperty(cx, obj2, count))
                return JS_FALSE;
        }
    }

    
    if (argc > count) {
        delta = (jsuint)argc - count;
        last = length;
        if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj) &&
            length <= js_DenseArrayCapacity(obj) &&
            (length == 0 || obj->dslots[length - 1] != JSVAL_HOLE)) {
            if (!EnsureCapacity(cx, obj, length + delta))
                return JS_FALSE;
            
            while (last-- > end) {
                jsval srcval = obj->dslots[last];
                jsval* dest = &obj->dslots[last + delta];
                if (*dest == JSVAL_HOLE && srcval != JSVAL_HOLE)
                    obj->incArrayCountBy(1);
                *dest = srcval;
            }
            obj->setArrayLength(obj->getArrayLength() + delta);
        } else {
            
            while (last-- > end) {
                if (!JS_CHECK_OPERATION_LIMIT(cx) ||
                    !GetArrayElement(cx, obj, last, &hole, tvr.addr()) ||
                    !SetOrDeleteArrayElement(cx, obj, last + delta, hole, tvr.value())) {
                    return JS_FALSE;
                }
            }
        }
        length += delta;
    } else if (argc < count) {
        delta = count - (jsuint)argc;
        if (obj->isDenseArray() && !js_PrototypeHasIndexedProperties(cx, obj) &&
            length <= js_DenseArrayCapacity(obj)) {
            
            for (last = end; last < length; last++) {
                jsval srcval = obj->dslots[last];
                jsval* dest = &obj->dslots[last - delta];
                if (*dest == JSVAL_HOLE && srcval != JSVAL_HOLE)
                    obj->incArrayCountBy(1);
                *dest = srcval;
            }
        } else {
            for (last = end; last < length; last++) {
                if (!JS_CHECK_OPERATION_LIMIT(cx) ||
                    !GetArrayElement(cx, obj, last, &hole, tvr.addr()) ||
                    !SetOrDeleteArrayElement(cx, obj, last - delta, hole, tvr.value())) {
                    return JS_FALSE;
                }
            }
        }
        length -= delta;
    }

    



    return InitArrayElements(cx, obj, begin, argc, argv, TargetElementsMayContainValues,
                             SourceVectorAllValues) &&
           js_SetLengthProperty(cx, obj, length);
}




static JSBool
array_concat(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv, v;
    JSObject *aobj, *nobj;
    jsuint length, alength, slot;
    uintN i;
    JSBool hole;

    
    argv = JS_ARGV(cx, vp) - 1;
    JS_ASSERT(JS_THIS_OBJECT(cx, vp) == JSVAL_TO_OBJECT(argv[0]));

    
    aobj = JS_THIS_OBJECT(cx, vp);
    if (aobj->isDenseArray()) {
        







        length = aobj->getArrayLength();
        jsuint capacity = js_DenseArrayCapacity(aobj);
        nobj = js_NewArrayObject(cx, JS_MIN(length, capacity), aobj->dslots,
                                 aobj->getArrayCount() != length);
        if (!nobj)
            return JS_FALSE;
        nobj->setArrayLength(length);
        *vp = OBJECT_TO_JSVAL(nobj);
        if (argc == 0)
            return JS_TRUE;
        argc--;
        argv++;
    } else {
        nobj = js_NewArrayObject(cx, 0, NULL);
        if (!nobj)
            return JS_FALSE;
        *vp = OBJECT_TO_JSVAL(nobj);
        length = 0;
    }

    AutoValueRooter tvr(cx, JSVAL_NULL);

    
    for (i = 0; i <= argc; i++) {
        if (!JS_CHECK_OPERATION_LIMIT(cx))
            return false;
        v = argv[i];
        if (!JSVAL_IS_PRIMITIVE(v)) {
            JSObject *wobj;

            aobj = JSVAL_TO_OBJECT(v);
            wobj = js_GetWrappedObject(cx, aobj);
            if (wobj->isArray()) {
                jsid id = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
                if (!aobj->getProperty(cx, id, tvr.addr()))
                    return false;
                alength = ValueIsLength(cx, tvr.addr());
                if (JSVAL_IS_NULL(tvr.value()))
                    return false;
                for (slot = 0; slot < alength; slot++) {
                    if (!JS_CHECK_OPERATION_LIMIT(cx) ||
                        !GetArrayElement(cx, aobj, slot, &hole, tvr.addr())) {
                        return false;
                    }

                    



                    if (!hole &&
                        !SetArrayElement(cx, nobj, length+slot, tvr.value())) {
                        return false;
                    }
                }
                length += alength;
                continue;
            }
        }

        if (!SetArrayElement(cx, nobj, length, v))
            return false;
        length++;
    }

    return js_SetLengthProperty(cx, nobj, length);
}

static JSBool
array_slice(JSContext *cx, uintN argc, jsval *vp)
{
    jsval *argv;
    JSObject *nobj, *obj;
    jsuint length, begin, end, slot;
    JSBool hole;

    argv = JS_ARGV(cx, vp);

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    begin = 0;
    end = length;

    if (argc > 0) {
        jsdouble d;
        if (!ValueToNumber(cx, argv[0], &d))
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
            if (!ValueToNumber(cx, argv[1], &d))
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

    if (obj->isDenseArray() && end <= js_DenseArrayCapacity(obj) &&
        !js_PrototypeHasIndexedProperties(cx, obj)) {
        nobj = js_NewArrayObject(cx, end - begin, obj->dslots + begin,
                                 obj->getArrayCount() != obj->getArrayLength());
        if (!nobj)
            return JS_FALSE;
        *vp = OBJECT_TO_JSVAL(nobj);
        return JS_TRUE;
    }

    
    nobj = js_NewArrayObject(cx, 0, NULL);
    if (!nobj)
        return JS_FALSE;
    *vp = OBJECT_TO_JSVAL(nobj);

    AutoValueRooter tvr(cx);
    for (slot = begin; slot < end; slot++) {
        if (!JS_CHECK_OPERATION_LIMIT(cx) ||
            !GetArrayElement(cx, obj, slot, &hole, tvr.addr())) {
            return JS_FALSE;
        }
        if (!hole && !SetArrayElement(cx, nobj, slot - begin, tvr.value()))
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
    jsval tosearch;
    jsint direction;
    JSBool hole;

    obj = JS_THIS_OBJECT(cx, vp);
    if (!obj || !js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;
    if (length == 0)
        goto not_found;

    if (argc <= 1) {
        i = isLast ? length - 1 : 0;
        tosearch = (argc != 0) ? vp[2] : JSVAL_VOID;
    } else {
        jsdouble start;

        tosearch = vp[2];
        if (!ValueToNumber(cx, vp[3], &start))
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
        if (!JS_CHECK_OPERATION_LIMIT(cx) ||
            !GetArrayElement(cx, obj, (jsuint)i, &hole, vp)) {
            return JS_FALSE;
        }
        if (!hole && js_StrictlyEqual(cx, *vp, tosearch))
            return js_NewNumberInRootedValue(cx, i, vp);
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
    JSObject *obj = JS_THIS_OBJECT(cx, vp);
    jsuint length;
    if (!obj || !js_GetLengthProperty(cx, obj, &length))
        return JS_FALSE;

    



    if (argc == 0) {
        js_ReportMissingArg(cx, vp, 0);
        return JS_FALSE;
    }
    jsval *argv = vp + 2;
    JSObject *callable = js_ValueToCallableObject(cx, &argv[0], JSV2F_SEARCH_STACK);
    if (!callable)
        return JS_FALSE;

    



    jsuint newlen;
    JSObject *newarr;
#ifdef __GNUC__ 
    newlen = 0;
    newarr = NULL;
#endif
    jsint start = 0, end = length, step = 1;

    switch (mode) {
      case REDUCE_RIGHT:
        start = length - 1, end = -1, step = -1;
        
      case REDUCE:
        if (length == 0 && argc == 1) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_EMPTY_ARRAY_REDUCE);
            return JS_FALSE;
        }
        if (argc >= 2) {
            *vp = argv[1];
        } else {
            JSBool hole;
            do {
                if (!GetArrayElement(cx, obj, start, &hole, vp))
                    return JS_FALSE;
                start += step;
            } while (hole && start != end);

            if (hole && start == end) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_EMPTY_ARRAY_REDUCE);
                return JS_FALSE;
            }
        }
        break;
      case MAP:
      case FILTER:
        newlen = (mode == MAP) ? length : 0;
        newarr = js_NewArrayObject(cx, newlen, NULL);
        if (!newarr)
            return JS_FALSE;
        *vp = OBJECT_TO_JSVAL(newarr);
        break;
      case SOME:
        *vp = JSVAL_FALSE;
        break;
      case EVERY:
        *vp = JSVAL_TRUE;
        break;
      case FOREACH:
        *vp = JSVAL_VOID;
        break;
    }

    if (length == 0)
        return JS_TRUE;

    JSObject *thisp;
    if (argc > 1 && !REDUCE_MODE(mode)) {
        if (!js_ValueToObject(cx, argv[1], &thisp))
            return JS_FALSE;
        argv[1] = OBJECT_TO_JSVAL(thisp);
    } else {
        thisp = NULL;
    }

    



    LeaveTrace(cx);
    argc = 3 + REDUCE_MODE(mode);

    InvokeArgsGuard args;
    if (!cx->stack().pushInvokeArgs(cx, argc, args))
        return JS_FALSE;

    MUST_FLOW_THROUGH("out");
    JSBool ok = JS_TRUE;
    JSBool cond;
    jsval *invokevp = args.getvp();

    AutoValueRooter tvr(cx);
    for (jsint i = start; i != end; i += step) {
        JSBool hole;
        ok = JS_CHECK_OPERATION_LIMIT(cx) &&
             GetArrayElement(cx, obj, i, &hole, tvr.addr());
        if (!ok)
            goto out;
        if (hole)
            continue;

        





        jsval *sp = invokevp;
        *sp++ = OBJECT_TO_JSVAL(callable);
        *sp++ = OBJECT_TO_JSVAL(thisp);
        if (REDUCE_MODE(mode))
            *sp++ = *vp;
        *sp++ = tvr.value();
        *sp++ = INT_TO_JSVAL(i);
        *sp++ = OBJECT_TO_JSVAL(obj);

        
        ok = js_Invoke(cx, args, 0);
        if (!ok)
            break;

        if (mode > MAP)
            cond = js_ValueToBoolean(*invokevp);
#ifdef __GNUC__ 
        else
            cond = JS_FALSE;
#endif

        switch (mode) {
          case FOREACH:
            break;
          case REDUCE:
          case REDUCE_RIGHT:
            *vp = *invokevp;
            break;
          case MAP:
            ok = SetArrayElement(cx, newarr, i, *invokevp);
            if (!ok)
                goto out;
            break;
          case FILTER:
            if (!cond)
                break;
            
            ok = SetArrayElement(cx, newarr, newlen++, tvr.value());
            if (!ok)
                goto out;
            break;
          case SOME:
            if (cond) {
                *vp = JSVAL_TRUE;
                goto out;
            }
            break;
          case EVERY:
            if (!cond) {
                *vp = JSVAL_FALSE;
                goto out;
            }
            break;
        }
    }

  out:
    if (ok && mode == FILTER)
        ok = js_SetLengthProperty(cx, newarr, newlen);
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

static JSBool
array_isArray(JSContext *cx, uintN argc, jsval *vp)
{
    *vp = BOOLEAN_TO_JSVAL(argc > 0 &&
                           !JSVAL_IS_PRIMITIVE(vp[2]) &&
                           js_GetWrappedObject(cx, JSVAL_TO_OBJECT(vp[2]))->isArray());
    return JS_TRUE;
}

static JSPropertySpec array_props[] = {
    {js_length_str,   -1,   JSPROP_SHARED | JSPROP_PERMANENT,
                            array_length_getter,    array_length_setter},
    {0,0,0,0,0}
};

JS_DEFINE_TRCINFO_1(array_toString,
    (2, (static, STRING_FAIL, Array_p_toString, CONTEXT, THIS,      0, nanojit::ACC_STORE_ANY)))
JS_DEFINE_TRCINFO_1(array_join,
    (3, (static, STRING_FAIL, Array_p_join, CONTEXT, THIS, STRING,  0, nanojit::ACC_STORE_ANY)))
JS_DEFINE_TRCINFO_1(array_push,
    (3, (static, JSVAL_FAIL, Array_p_push1, CONTEXT, THIS, JSVAL,   0, nanojit::ACC_STORE_ANY)))
JS_DEFINE_TRCINFO_1(array_pop,
    (2, (static, JSVAL_FAIL, Array_p_pop, CONTEXT, THIS,            0, nanojit::ACC_STORE_ANY)))

static JSFunctionSpec array_methods[] = {
#if JS_HAS_TOSOURCE
    JS_FN(js_toSource_str,      array_toSource,     0,0),
#endif
    JS_TN(js_toString_str,      array_toString,     0,0, &array_toString_trcinfo),
    JS_FN(js_toLocaleString_str,array_toLocaleString,0,0),

    
    JS_TN("join",               array_join,         1,JSFUN_GENERIC_NATIVE, &array_join_trcinfo),
    JS_FN("reverse",            array_reverse,      0,JSFUN_GENERIC_NATIVE),
    JS_FN("sort",               array_sort,         1,JSFUN_GENERIC_NATIVE),
    JS_TN("push",               array_push,         1,JSFUN_GENERIC_NATIVE, &array_push_trcinfo),
    JS_TN("pop",                array_pop,          0,JSFUN_GENERIC_NATIVE, &array_pop_trcinfo),
    JS_FN("shift",              array_shift,        0,JSFUN_GENERIC_NATIVE),
    JS_FN("unshift",            array_unshift,      1,JSFUN_GENERIC_NATIVE),
    JS_FN("splice",             array_splice,       2,JSFUN_GENERIC_NATIVE),

    
    JS_FN("concat",             array_concat,       1,JSFUN_GENERIC_NATIVE),
    JS_FN("slice",              array_slice,        2,JSFUN_GENERIC_NATIVE),

#if JS_HAS_ARRAY_EXTRAS
    JS_FN("indexOf",            array_indexOf,      1,JSFUN_GENERIC_NATIVE),
    JS_FN("lastIndexOf",        array_lastIndexOf,  1,JSFUN_GENERIC_NATIVE),
    JS_FN("forEach",            array_forEach,      1,JSFUN_GENERIC_NATIVE),
    JS_FN("map",                array_map,          1,JSFUN_GENERIC_NATIVE),
    JS_FN("reduce",             array_reduce,       1,JSFUN_GENERIC_NATIVE),
    JS_FN("reduceRight",        array_reduceRight,  1,JSFUN_GENERIC_NATIVE),
    JS_FN("filter",             array_filter,       1,JSFUN_GENERIC_NATIVE),
    JS_FN("some",               array_some,         1,JSFUN_GENERIC_NATIVE),
    JS_FN("every",              array_every,        1,JSFUN_GENERIC_NATIVE),
#endif

    JS_FS_END
};

static JSFunctionSpec array_static_methods[] = {
    JS_FN("isArray",            array_isArray,      1,0),
    JS_FS_END
};

JSBool
js_Array(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    jsuint length;
    const jsval *vector;

    
    if (!JS_IsConstructing(cx)) {
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
        length = ValueIsLength(cx, &argv[0]);
        if (JSVAL_IS_NULL(argv[0]))
            return JS_FALSE;
        vector = NULL;
    }
    return InitArrayObject(cx, obj, length, vector);
}

JSObject* JS_FASTCALL
js_NewEmptyArray(JSContext* cx, JSObject* proto)
{
    JS_ASSERT(proto->isArray());

    JSObject* obj = js_NewGCObject(cx);
    if (!obj)
        return NULL;

    
    obj->map = const_cast<JSObjectMap *>(&SharedArrayMap);
    obj->classword = jsuword(&js_ArrayClass);
    obj->setProto(proto);
    obj->setParent(proto->getParent());

    obj->setArrayLength(0);
    obj->setArrayCount(0);
    obj->voidArrayUnused();
    obj->dslots = NULL;
    return obj;
}
#ifdef JS_TRACER
JS_DEFINE_CALLINFO_2(extern, OBJECT, js_NewEmptyArray, CONTEXT, OBJECT, 0, nanojit::ACC_STORE_ANY)
#endif

JSObject* JS_FASTCALL
js_NewEmptyArrayWithLength(JSContext* cx, JSObject* proto, int32 len)
{
    if (len < 0)
        return NULL;
    JSObject *obj = js_NewEmptyArray(cx, proto);
    if (!obj)
        return NULL;
    obj->setArrayLength(len);
    return obj;
}
#ifdef JS_TRACER
JS_DEFINE_CALLINFO_3(extern, OBJECT, js_NewEmptyArrayWithLength, CONTEXT, OBJECT, INT32, 0,
                     nanojit::ACC_STORE_ANY)
#endif

JSObject* JS_FASTCALL
js_NewArrayWithSlots(JSContext* cx, JSObject* proto, uint32 len)
{
    JSObject* obj = js_NewEmptyArray(cx, proto);
    if (!obj)
        return NULL;
    obj->setArrayLength(len);
    if (!ResizeSlots(cx, obj, 0, JS_MAX(len, ARRAY_CAPACITY_MIN)))
        return NULL;
    return obj;
}
#ifdef JS_TRACER
JS_DEFINE_CALLINFO_3(extern, OBJECT, js_NewArrayWithSlots, CONTEXT, OBJECT, UINT32, 0,
                     nanojit::ACC_STORE_ANY)
#endif

JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj)
{
    JSObject *proto = JS_InitClass(cx, obj, NULL, &js_ArrayClass, js_Array, 1,
                                   array_props, array_methods, NULL, array_static_methods);

    
    if (!proto || !InitArrayObject(cx, proto, 0, NULL))
        return NULL;
    return proto;
}

JSObject *
js_NewArrayObject(JSContext *cx, jsuint length, const jsval *vector, bool holey)
{
    JSObject *obj = js_NewObject(cx, &js_ArrayClass, NULL, NULL);
    if (!obj)
        return NULL;

    



    JS_ASSERT(obj->getProto());

    {
        AutoValueRooter tvr(cx, obj);
        if (!InitArrayObject(cx, obj, length, vector, holey))
            obj = NULL;
    }

    
    cx->weakRoots.finalizableNewborns[FINALIZE_OBJECT] = obj;
    return obj;
}

JSObject *
js_NewSlowArrayObject(JSContext *cx)
{
    JSObject *obj = js_NewObject(cx, &js_SlowArrayClass, NULL, NULL);
    if (obj)
        obj->setArrayLength(0);
    return obj;
}

#ifdef DEBUG_ARRAYS
JSBool
js_ArrayInfo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
{
    uintN i;
    JSObject *array;

    for (i = 0; i < argc; i++) {
        char *bytes;

        bytes = js_DecompileValueGenerator(cx, JSDVG_SEARCH_STACK, argv[i],
                                           NULL);
        if (!bytes)
            return JS_FALSE;
        if (JSVAL_IS_PRIMITIVE(argv[i]) ||
            !(array = JSVAL_TO_OBJECT(argv[i]))->isArray()) {
            fprintf(stderr, "%s: not array\n", bytes);
            cx->free(bytes);
            continue;
        }
        fprintf(stderr, "%s: %s (len %lu", bytes,
                array->isDenseArray()) ? "dense" : "sparse",
                array->getArrayLength());
        if (array->isDenseArray()) {
            fprintf(stderr, ", count %lu, capacity %lu",
                    array->getArrayCount(),
                    js_DenseArrayCapacity(array));
        }
        fputs(")\n", stderr);
        cx->free(bytes);
    }
    return JS_TRUE;
}
#endif

JS_FRIEND_API(JSBool)
js_CoerceArrayToCanvasImageData(JSObject *obj, jsuint offset, jsuint count,
                                JSUint8 *dest)
{
    uint32 length;

    if (!obj || !obj->isDenseArray())
        return JS_FALSE;

    length = obj->getArrayLength();
    if (length < offset + count)
        return JS_FALSE;

    JSUint8 *dp = dest;
    for (uintN i = offset; i < offset+count; i++) {
        jsval v = obj->dslots[i];
        if (JSVAL_IS_INT(v)) {
            jsint vi = JSVAL_TO_INT(v);
            if (jsuint(vi) > 255)
                vi = (vi < 0) ? 0 : 255;
            *dp++ = JSUint8(vi);
        } else if (JSVAL_IS_DOUBLE(v)) {
            jsdouble vd = *JSVAL_TO_DOUBLE(v);
            if (!(vd >= 0)) 
                *dp++ = 0;
            else if (vd > 255)
                *dp++ = 255;
            else {
                jsdouble toTruncate = vd + 0.5;
                JSUint8 val = JSUint8(toTruncate);

                




                if (val == toTruncate) {
                  







                  *dp++ = (val & ~1);
                } else {
                  *dp++ = val;
                }
            }
        } else {
            return JS_FALSE;
        }
    }

    return JS_TRUE;
}

JS_FRIEND_API(JSObject *)
js_NewArrayObjectWithCapacity(JSContext *cx, jsuint capacity, jsval **vector)
{
    JSObject *obj = js_NewArrayObject(cx, capacity, NULL);
    if (!obj)
        return NULL;

    AutoValueRooter tvr(cx, obj);
    if (!EnsureCapacity(cx, obj, capacity, JS_FALSE))
        obj = NULL;

    
    cx->weakRoots.finalizableNewborns[FINALIZE_OBJECT] = obj;
    if (!obj)
        return NULL;

    obj->setArrayCount(capacity);
    *vector = obj->dslots;
    return obj;
}
