






































#ifndef jsarray_h___
#define jsarray_h___



#include "jsprvtd.h"
#include "jspubtd.h"

JS_BEGIN_EXTERN_C


#define ARRAY_INIT_LIMIT        JS_BIT(24)

extern JSBool
js_IdIsIndex(jsval id, jsuint *indexp);

extern JSClass js_ArrayClass, js_SlowArrayClass;

#define OBJ_IS_DENSE_ARRAY(cx,obj)  (OBJ_GET_CLASS(cx, obj) == &js_ArrayClass)

#define OBJ_IS_ARRAY(cx,obj)    (OBJ_IS_DENSE_ARRAY(cx, obj) ||               \
                                 OBJ_GET_CLASS(cx, obj) == &js_SlowArrayClass)

extern JSObject *
js_InitArrayClass(JSContext *cx, JSObject *obj);

extern JSObject *
js_NewArrayObject(JSContext *cx, jsuint length, jsval *vector,
                  JSBool holey = JS_FALSE);


extern JSObject *
js_NewSlowArrayObject(JSContext *cx);

extern JSBool
js_MakeArraySlow(JSContext *cx, JSObject *obj);

#define JSSLOT_ARRAY_LENGTH            JSSLOT_PRIVATE
#define JSSLOT_ARRAY_COUNT             (JSSLOT_ARRAY_LENGTH + 1)
#define JSSLOT_ARRAY_LOOKUP_HOLDER     (JSSLOT_ARRAY_COUNT + 1)

#define ARRAY_DENSE_LENGTH(obj)                                                \
    (JS_ASSERT(OBJ_IS_DENSE_ARRAY(cx, obj)),                                   \
     (obj)->dslots ? (uint32)(obj)->dslots[-1] : 0)

#define ARRAY_SET_DENSE_LENGTH(obj, max)                                       \
    (JS_ASSERT((obj)->dslots), (obj)->dslots[-1] = (jsval)(max))

#define ARRAY_GROWBY 8

extern JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsuint length);

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

extern JSBool
js_array_join(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_array_push_slowly(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval);

extern JSBool
js_array_push1_dense(JSContext *cx, JSObject *obj, jsval v, jsval *rval);

extern JSBool
js_array_push(JSContext *cx, uintN argc, jsval *vp);

extern JSBool
js_array_pop_slowly(JSContext *cx, JSObject* obj, jsval *vp);

extern JSBool
js_array_pop_dense(JSContext *cx, JSObject* obj, jsval *vp);

extern JSBool
js_array_pop(JSContext *cx, uintN argc, jsval *vp);

enum ArrayToStringOp {
    TO_STRING,
    TO_LOCALE_STRING,
    TO_SOURCE
};

extern JSBool
js_array_join_sub(JSContext *cx, JSObject *obj, enum ArrayToStringOp op,
                  JSString *sep, jsval *rval);




























JS_FRIEND_API(JSBool)
js_ArrayToJSUint8Buffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                        JSUint8 *dest);

JS_FRIEND_API(JSBool)
js_ArrayToJSUint16Buffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                         JSUint16 *dest);

JS_FRIEND_API(JSBool)
js_ArrayToJSUint32Buffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                         JSUint32 *dest);

JS_FRIEND_API(JSBool)
js_ArrayToJSInt8Buffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                       JSInt8 *dest);

JS_FRIEND_API(JSBool)
js_ArrayToJSInt16Buffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                        JSInt16 *dest);

JS_FRIEND_API(JSBool)
js_ArrayToJSInt32Buffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                        JSInt32 *dest);

JS_FRIEND_API(JSBool)
js_ArrayToJSDoubleBuffer(JSContext *cx, JSObject *obj, jsuint offset, jsuint count,
                         jsdouble *dest);

JS_END_EXTERN_C

#endif 
