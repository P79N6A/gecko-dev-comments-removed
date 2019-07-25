






































#ifndef jsarray_h___
#define jsarray_h___



#include "jscntxt.h"
#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsatom.h"
#include "jsobj.h"
#include "jsstr.h"


const uintN MIN_SPARSE_INDEX = 256;

inline uint32
JSObject::getDenseArrayInitializedLength()
{
    JS_ASSERT(isDenseArray());
    return initializedLength;
}

inline void
JSObject::setDenseArrayInitializedLength(uint32 length)
{
    JS_ASSERT(isDenseArray());
    JS_ASSERT(length <= getDenseArrayCapacity());
    initializedLength = length;
}

inline bool
JSObject::isPackedDenseArray()
{
    JS_ASSERT(isDenseArray());
    return flags & PACKED_ARRAY;
}

inline void
JSObject::markDenseArrayNotPacked(JSContext *cx)
{
    JS_ASSERT(isDenseArray());
    if (flags & PACKED_ARRAY) {
        flags ^= PACKED_ARRAY;
        cx->markTypeObjectFlags(getType(), js::types::OBJECT_FLAG_NON_PACKED_ARRAY);
    }
}

inline void
JSObject::backfillDenseArrayHoles(JSContext *cx)
{
    
    ensureDenseArrayInitializedLength(cx, getDenseArrayCapacity(), 0);
}

inline void
JSObject::ensureDenseArrayInitializedLength(JSContext *cx, uint32 index, uint32 extra)
{
    




    JS_ASSERT(index + extra <= capacity);
    if (initializedLength < index) {
        markDenseArrayNotPacked(cx);
        ClearValueRange(slots + initializedLength, index - initializedLength, true);
    }
    if (initializedLength < index + extra)
        initializedLength = index + extra;
}

inline JSObject::EnsureDenseResult
JSObject::ensureDenseArrayElements(JSContext *cx, uintN index, uintN extra)
{
    JS_ASSERT(isDenseArray());

    uintN currentCapacity = numSlots();

    



    JS_ASSERT_IF(!cx->typeInferenceEnabled(), currentCapacity == getDenseArrayInitializedLength());

    uintN requiredCapacity;
    if (extra == 1) {
        
        if (index < currentCapacity) {
            ensureDenseArrayInitializedLength(cx, index, 1);
            return ED_OK;
        }
        requiredCapacity = index + 1;
        if (requiredCapacity == 0) {
            
            return ED_SPARSE;
        }
    } else {
        requiredCapacity = index + extra;
        if (requiredCapacity < index) {
            
            return ED_SPARSE;
        }
        if (requiredCapacity <= currentCapacity) {
            ensureDenseArrayInitializedLength(cx, index, extra);
            return ED_OK;
        }
    }

    



    if (requiredCapacity > MIN_SPARSE_INDEX &&
        willBeSparseDenseArray(requiredCapacity, extra)) {
        return ED_SPARSE;
    }
    if (!growSlots(cx, requiredCapacity))
        return ED_FAILED;

    ensureDenseArrayInitializedLength(cx, index, extra);
    return ED_OK;
}

extern bool
js_StringIsIndex(JSLinearString *str, jsuint *indexp);

inline JSBool
js_IdIsIndex(jsid id, jsuint *indexp)
{
    if (JSID_IS_INT(id)) {
        jsint i;
        i = JSID_TO_INT(id);
        if (i < 0)
            return JS_FALSE;
        *indexp = (jsuint)i;
        return JS_TRUE;
    }

    if (JS_UNLIKELY(!JSID_IS_STRING(id)))
        return JS_FALSE;

    return js_StringIsIndex(JSID_TO_ATOM(id), indexp);
}




















inline JSObject *
js_GetProtoIfDenseArray(JSObject *obj);

extern JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj);

extern bool
js_InitContextBusyArrayTable(JSContext *cx);

namespace js
{


extern JSObject * JS_FASTCALL
NewDenseEmptyArray(JSContext *cx, JSObject *proto=NULL);


extern JSObject * JS_FASTCALL
NewDenseAllocatedArray(JSContext *cx, uint length, JSObject *proto=NULL);






extern JSObject * JS_FASTCALL
NewDenseAllocatedEmptyArray(JSContext *cx, uint length, JSObject *proto=NULL);





extern JSObject * JS_FASTCALL
NewDenseUnallocatedArray(JSContext *cx, uint length, JSObject *proto=NULL);


extern JSObject *
NewDenseCopiedArray(JSContext *cx, uint length, Value *vp, JSObject *proto=NULL);


extern JSObject *
NewSlowEmptyArray(JSContext *cx);

}

extern JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsdouble length);

extern JSBool
js_HasLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);

namespace js {

extern JSBool
array_defineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *value,
                     PropertyOp getter, StrictPropertyOp setter, uintN attrs);

extern JSBool
array_deleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict);





extern bool
GetElements(JSContext *cx, JSObject *aobj, jsuint length, js::Value *vp);

}




typedef JSBool (*JSComparator)(void *arg, const void *a, const void *b,
                               int *result);

enum JSMergeSortElemType {
    JS_SORTING_VALUES,
    JS_SORTING_GENERIC
};











extern bool
js_MergeSort(void *vec, size_t nel, size_t elsize, JSComparator cmp,
             void *arg, void *tmp, JSMergeSortElemType elemType);


namespace js {

extern JSBool
array_sort(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
array_push(JSContext *cx, uintN argc, js::Value *vp);

extern JSBool
array_pop(JSContext *cx, uintN argc, js::Value *vp);

} 

#ifdef DEBUG
extern JSBool
js_ArrayInfo(JSContext *cx, uintN argc, jsval *vp);
#endif

extern JSBool
js_ArrayCompPush(JSContext *cx, JSObject *obj, const js::Value &vp);

















JS_FRIEND_API(JSBool)
js_CoerceArrayToCanvasImageData(JSObject *obj, jsuint offset, jsuint count,
                                JSUint8 *dest);

JSBool
js_PrototypeHasIndexedProperties(JSContext *cx, JSObject *obj);




JSBool
js_GetDenseArrayElementValue(JSContext *cx, JSObject *obj, jsid id,
                             js::Value *vp);


JSBool
js_Array(JSContext *cx, uintN argc, js::Value *vp);











JS_FRIEND_API(JSBool)
js_CloneDensePrimitiveArray(JSContext *cx, JSObject *obj, JSObject **clone);





JS_FRIEND_API(JSBool)
js_IsDensePrimitiveArray(JSObject *obj);

extern JSBool JS_FASTCALL
js_EnsureDenseArrayCapacity(JSContext *cx, JSObject *obj, jsint i);

#endif 
