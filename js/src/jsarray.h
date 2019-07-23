






































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
js_NewArrayObject(JSContext *cx, jsuint length, jsval *vector);


extern JSObject *
js_NewSlowArrayObject(JSContext *cx);

#define JSSLOT_ARRAY_LENGTH            JSSLOT_PRIVATE
#define JSSLOT_ARRAY_COUNT             (JSSLOT_ARRAY_LENGTH + 1)
#define JSSLOT_ARRAY_LOOKUP_HOLDER     (JSSLOT_ARRAY_COUNT + 1)

#define ARRAY_DENSE_LENGTH(obj)                                                \
    (JS_ASSERT(OBJ_IS_DENSE_ARRAY(cx, obj)),                                   \
     (obj)->dslots ? (uint32)(obj)->dslots[-1] : 0)

extern JSBool
js_GetLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);

extern JSBool
js_SetLengthProperty(JSContext *cx, JSObject *obj, jsuint length);

extern JSBool
js_HasLengthProperty(JSContext *cx, JSObject *obj, jsuint *lengthp);








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

JS_END_EXTERN_C

#endif 
