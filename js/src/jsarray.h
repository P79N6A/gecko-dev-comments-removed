







#ifndef jsarray_h
#define jsarray_h

#include "jsobj.h"
#include "jspubtd.h"

namespace js {

const uint32_t MAX_ARRAY_INDEX = 4294967294u;
}

inline bool
js_IdIsIndex(jsid id, uint32_t *indexp)
{
    if (JSID_IS_INT(id)) {
        int32_t i = JSID_TO_INT(id);
        JS_ASSERT(i >= 0);
        *indexp = (uint32_t)i;
        return true;
    }

    if (MOZ_UNLIKELY(!JSID_IS_STRING(id)))
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
NewDenseEmptyArray(JSContext *cx, JSObject *proto = nullptr,
                   NewObjectKind newKind = GenericObject);





extern ArrayObject * JS_FASTCALL
NewDenseUnallocatedArray(ExclusiveContext *cx, uint32_t length, JSObject *proto = nullptr,
                         NewObjectKind newKind = GenericObject);





extern ArrayObject * JS_FASTCALL
NewDensePartlyAllocatedArray(ExclusiveContext *cx, uint32_t length, JSObject *proto = nullptr,
                             NewObjectKind newKind = GenericObject);


extern ArrayObject * JS_FASTCALL
NewDenseFullyAllocatedArray(ExclusiveContext *cx, uint32_t length, JSObject *proto = nullptr,
                            NewObjectKind newKind = GenericObject);

enum AllocatingBehaviour {
    NewArray_Unallocating,
    NewArray_PartlyAllocating,
    NewArray_FullyAllocating
};





extern ArrayObject * JS_FASTCALL
NewDenseArray(ExclusiveContext *cx, uint32_t length, HandleTypeObject type,
              AllocatingBehaviour allocating);


extern ArrayObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, HandleObject src, uint32_t elementOffset, JSObject *proto = nullptr);


extern ArrayObject *
NewDenseCopiedArray(JSContext *cx, uint32_t length, const Value *values, JSObject *proto = nullptr,
                    NewObjectKind newKind = GenericObject);


extern ArrayObject *
NewDenseFullyAllocatedArrayWithTemplate(JSContext *cx, uint32_t length, JSObject *templateObject);


extern JSObject *
NewDenseCopyOnWriteArray(JSContext *cx, HandleObject templateObject, gc::InitialHeap heap);






extern bool
WouldDefinePastNonwritableLength(ThreadSafeContext *cx,
                                 HandleObject obj, uint32_t index, bool strict,
                                 bool *definesPast);







template <ExecutionMode mode>
extern bool
CanonicalizeArrayLengthValue(typename ExecutionModeTraits<mode>::ContextType cx,
                             HandleValue v, uint32_t *canonicalized);

extern bool
GetLengthProperty(JSContext *cx, HandleObject obj, uint32_t *lengthp);

extern bool
SetLengthProperty(JSContext *cx, HandleObject obj, double length);

extern bool
ObjectMayHaveExtraIndexedProperties(JSObject *obj);







extern bool
GetElements(JSContext *cx, HandleObject aobj, uint32_t length, js::Value *vp);



extern bool
array_sort(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
array_push(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
array_pop(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
array_splice(JSContext *cx, unsigned argc, js::Value *vp);

extern bool
array_splice_impl(JSContext *cx, unsigned argc, js::Value *vp, bool pop);

extern bool
array_concat(JSContext *cx, unsigned argc, js::Value *vp);

template <bool Locale>
JSString *
ArrayJoin(JSContext *cx, HandleObject obj, HandleLinearString sepstr, uint32_t length);

extern bool
array_concat_dense(JSContext *cx, Handle<ArrayObject*> arr1, Handle<ArrayObject*> arr2,
                   Handle<ArrayObject*> result);

bool
array_join(JSContext *cx, unsigned argc, js::Value *vp);

extern JSString *
array_join_impl(JSContext *cx, HandleValue array, HandleString sep);

extern void
ArrayShiftMoveElements(JSObject *obj);

extern bool
array_shift(JSContext *cx, unsigned argc, js::Value *vp);








extern bool
NewbornArrayPush(JSContext *cx, HandleObject obj, const Value &v);

} 

#ifdef DEBUG
extern bool
js_ArrayInfo(JSContext *cx, unsigned argc, js::Value *vp);
#endif


bool
js_Array(JSContext *cx, unsigned argc, js::Value *vp);

#endif 
