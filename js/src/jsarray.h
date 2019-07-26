







#ifndef jsarray_h
#define jsarray_h

#include "jspubtd.h"
#include "jsobj.h"

namespace js {

const uint32_t MAX_ARRAY_INDEX = 4294967294u;
}

inline JSBool
js_IdIsIndex(jsid id, uint32_t *indexp)
{
    if (JSID_IS_INT(id)) {
        int32_t i = JSID_TO_INT(id);
        JS_ASSERT(i >= 0);
        *indexp = (uint32_t)i;
        return true;
    }

    if (JS_UNLIKELY(!JSID_IS_STRING(id)))
        return false;

    return js::StringIsArrayIndex(JSID_TO_ATOM(id), indexp);
}

extern JSObject *
js_InitArrayClass(JSContext *cx, js::HandleObject obj);

extern bool
js_InitContextBusyArrayTable(JSContext *cx);

namespace js {

class ArrayObject;


extern ArrayObject * JS_FASTCALL
NewDenseEmptyArray(JSContext *cx, JSObject *proto = NULL,
                   NewObjectKind newKind = GenericObject);


extern ArrayObject * JS_FASTCALL
NewDenseAllocatedArray(ExclusiveContext *cx, uint32_t length, JSObject *proto = NULL,
                       NewObjectKind newKind = GenericObject);





extern ArrayObject * JS_FASTCALL
NewDenseUnallocatedArray(ExclusiveContext *cx, uint32_t length, JSObject *proto = NULL,
                         NewObjectKind newKind = GenericObject);


extern ArrayObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, HandleObject src, uint32_t elementOffset, JSObject *proto = NULL);


extern ArrayObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, const Value *values, JSObject *proto = NULL,
                    NewObjectKind newKind = GenericObject);






extern bool
WouldDefinePastNonwritableLength(ExclusiveContext *cx,
                                 HandleObject obj, uint32_t index, bool strict,
                                 bool *definesPast);





extern bool
CanonicalizeArrayLengthValue(JSContext *cx, HandleValue v, uint32_t *canonicalized);

extern JSBool
GetLengthProperty(JSContext *cx, HandleObject obj, uint32_t *lengthp);

extern JSBool
SetLengthProperty(JSContext *cx, HandleObject obj, double length);

extern JSBool
ObjectMayHaveExtraIndexedProperties(JSObject *obj);







extern bool
GetElements(JSContext *cx, HandleObject aobj, uint32_t length, js::Value *vp);



extern JSBool
array_sort(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
array_push(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
array_pop(JSContext *cx, unsigned argc, js::Value *vp);

extern JSBool
array_concat(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
array_concat_dense(JSContext *cx, Handle<ArrayObject*> arr1, Handle<ArrayObject*> arr2,
                   Handle<ArrayObject*> result);

extern void
ArrayShiftMoveElements(JSObject *obj);

extern JSBool
array_shift(JSContext *cx, unsigned argc, js::Value *vp);

} 

#ifdef DEBUG
extern JSBool
js_ArrayInfo(JSContext *cx, unsigned argc, js::Value *vp);
#endif








extern JSBool
js_NewbornArrayPush(JSContext *cx, js::HandleObject obj, const js::Value &v);


JSBool
js_Array(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
