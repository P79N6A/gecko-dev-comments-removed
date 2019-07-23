






































#ifndef jsarray_h___
#define jsarray_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"

JS_BEGIN_EXTERN_C

#define ARRAY_CAPACITY_MIN      7

extern JSBool
js_IdIsIndex(jsval id, jsuint *indexp);

extern JSClass js_ArrayClass, js_SlowArrayClass;

static JS_INLINE JSBool
js_IsDenseArray(JSObject *obj)
{
    return STOBJ_GET_CLASS(obj) == &js_ArrayClass;
}

#define OBJ_IS_DENSE_ARRAY(cx, obj) js_IsDenseArray(obj)

#define OBJ_IS_ARRAY(cx,obj)    (OBJ_IS_DENSE_ARRAY(cx, obj) ||               \
                                 OBJ_GET_CLASS(cx, obj) == &js_SlowArrayClass)




















static JS_INLINE JSObject *
js_GetProtoIfDenseArray(JSContext *cx, JSObject *obj)
{
    return OBJ_IS_DENSE_ARRAY(cx, obj) ? OBJ_GET_PROTO(cx, obj) : obj;
}

extern JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj);

extern bool
js_InitContextBusyArrayTable(JSContext *cx);





extern JSObject * JS_FASTCALL
js_NewArrayWithSlots(JSContext* cx, JSObject* proto, uint32 len);

extern JSObject *
js_NewArrayObject(JSContext *cx, jsuint length, jsval *vector,
                  JSBool holey = JS_FALSE);


extern JSObject *
js_NewSlowArrayObject(JSContext *cx);

extern JSBool
js_MakeArraySlow(JSContext *cx, JSObject *obj);

#define JSSLOT_ARRAY_LENGTH            JSSLOT_PRIVATE
#define JSSLOT_ARRAY_COUNT             (JSSLOT_ARRAY_LENGTH + 1)
#define JSSLOT_ARRAY_UNUSED            (JSSLOT_ARRAY_COUNT + 1)

static JS_INLINE uint32
js_DenseArrayCapacity(JSObject *obj)
{
    JS_ASSERT(js_IsDenseArray(obj));
    return obj->dslots ? (uint32) obj->dslots[-1] : 0;
}

static JS_INLINE void
js_SetDenseArrayCapacity(JSObject *obj, uint32 capacity)
{
    JS_ASSERT(js_IsDenseArray(obj));
    JS_ASSERT(obj->dslots);
    obj->dslots[-1] = (jsval) capacity;
}

extern JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsdouble length);

extern JSBool
js_HasLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JSBool JS_FASTCALL
js_IndexToId(JSContext *cx, jsuint index, jsid *idp);








extern JSBool
js_IsArrayLike(JSContext *cx, JSObject *obj, JSBool *answerp, jsuint *lengthp);




typedef JSBool (*JSComparator)(void *arg, const void *a, const void *b,
                               int *result);








extern JSBool
js_MergeSort(void *vec, size_t nel, size_t elsize, JSComparator cmp,
             void *arg, void *tmp);

#ifdef DEBUG_ARRAYS
extern JSBool
js_ArrayInfo(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);
#endif

extern JSBool JS_FASTCALL
js_ArrayCompPush(JSContext *cx, JSObject *obj, jsval v);

















JS_FRIEND_API(JSBool)
js_CoerceArrayToCanvasImageData(JSObject *obj, jsuint offset, jsuint count,
                                JSUint8 *dest);

JSBool
js_PrototypeHasIndexedProperties(JSContext *cx, JSObject *obj);




JSBool
js_GetDenseArrayElementValue(JSContext *cx, JSObject *obj, JSProperty *prop,
                             jsval *vp);


JSBool
js_Array(JSContext* cx, JSObject* obj, uintN argc, jsval* argv, jsval* rval);












JS_FRIEND_API(JSObject *)
js_NewArrayObjectWithCapacity(JSContext *cx, jsuint capacity, jsval **vector);

JS_END_EXTERN_C

#endif 
