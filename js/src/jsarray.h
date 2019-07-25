






































#ifndef jsarray_h___
#define jsarray_h___



#include "jsprvtd.h"
#include "jspubtd.h"
#include "jsobj.h"

#define ARRAY_CAPACITY_MIN      7

extern JSBool
js_IdIsIndex(jsboxedword idw, jsuint *indexp);

extern js::Class js_ArrayClass, js_SlowArrayClass;

inline bool
JSObject::isDenseArray() const
{
    return getClass() == &js_ArrayClass;
}

inline bool
JSObject::isSlowArray() const
{
    return getClass() == &js_SlowArrayClass;
}

inline bool
JSObject::isArray() const
{
    return isDenseArray() || isSlowArray();
}




















static JS_INLINE JSObject *
js_GetProtoIfDenseArray(JSObject *obj)
{
    return obj->isDenseArray() ? obj->getProto() : obj;
}

extern JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj);

extern bool
js_InitContextBusyArrayTable(JSContext *cx);





extern JSObject * JS_FASTCALL
js_NewArrayWithSlots(JSContext* cx, JSObject* proto, uint32 len);

extern JSObject *
js_NewArrayObject(JSContext *cx, jsuint length, const js::Value *vector, bool holey = false);


extern JSObject *
js_NewSlowArrayObject(JSContext *cx);

extern JSBool
js_MakeArraySlow(JSContext *cx, JSObject *obj);

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

enum JSMergeSortElemType {
    JS_SORTING_VALUES,
    JS_SORTING_GENERIC
};











extern bool
js_MergeSort(void *vec, size_t nel, size_t elsize, JSComparator cmp,
             void *arg, void *tmp, JSMergeSortElemType elemType);

#ifdef DEBUG_ARRAYS
extern JSBool
js_ArrayInfo(JSContext *cx, JSObject *obj, uintN argc, js::Value *argv, js::Value *rval);
#endif

extern JSBool JS_FASTCALL
js_ArrayCompPush(JSContext *cx, JSObject *obj, const js::Value &v);

















JS_FRIEND_API(JSBool)
js_CoerceArrayToCanvasImageData(JSObject *obj, jsuint offset, jsuint count,
                                JSUint8 *dest);

JSBool
js_PrototypeHasIndexedProperties(JSContext *cx, JSObject *obj);




JSBool
js_GetDenseArrayElementValue(JSContext *cx, JSObject *obj, JSProperty *prop,
                             js::Value *vp);


JSBool
js_Array(JSContext* cx, JSObject* obj, uintN argc, js::Value* argv, js::Value* rval);












JS_FRIEND_API(JSObject *)
js_NewArrayObjectWithCapacity(JSContext *cx, jsuint capacity, js::Value **vector);











JS_FRIEND_API(JSBool)
js_CloneDensePrimitiveArray(JSContext *cx, JSObject *obj, JSObject **clone);





JS_FRIEND_API(JSBool)
js_IsDensePrimitiveArray(JSObject *obj);

#endif 
