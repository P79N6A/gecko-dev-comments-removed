







#ifndef jsarray_h
#define jsarray_h

#include "jsobj.h"
#include "jspubtd.h"

#include "vm/ArrayObject.h"

namespace js {

const uint32_t MAX_ARRAY_INDEX = 4294967294u;

inline bool
IdIsIndex(jsid id, uint32_t* indexp)
{
    if (JSID_IS_INT(id)) {
        int32_t i = JSID_TO_INT(id);
        MOZ_ASSERT(i >= 0);
        *indexp = (uint32_t)i;
        return true;
    }

    if (MOZ_UNLIKELY(!JSID_IS_STRING(id)))
        return false;

    return js::StringIsArrayIndex(JSID_TO_ATOM(id), indexp);
}

extern JSObject*
InitArrayClass(JSContext* cx, js::HandleObject obj);




extern ArrayObject * JS_FASTCALL
NewDenseEmptyArray(JSContext* cx, HandleObject proto = nullptr,
                   NewObjectKind newKind = GenericObject);





extern ArrayObject * JS_FASTCALL
NewDenseUnallocatedArray(ExclusiveContext* cx, uint32_t length, HandleObject proto = nullptr,
                         NewObjectKind newKind = GenericObject);





extern ArrayObject * JS_FASTCALL
NewDensePartlyAllocatedArray(ExclusiveContext* cx, uint32_t length, HandleObject proto = nullptr,
                             NewObjectKind newKind = GenericObject);


extern ArrayObject * JS_FASTCALL
NewDenseFullyAllocatedArray(ExclusiveContext* cx, uint32_t length, HandleObject proto = nullptr,
                            NewObjectKind newKind = GenericObject);


extern ArrayObject*
NewDenseCopiedArray(ExclusiveContext* cx, uint32_t length, HandleArrayObject src,
                    uint32_t elementOffset, HandleObject proto = nullptr);


extern ArrayObject*
NewDenseCopiedArray(ExclusiveContext* cx, uint32_t length, const Value* values,
                    HandleObject proto = nullptr, NewObjectKind newKind = GenericObject);


extern ArrayObject*
NewDenseFullyAllocatedArrayWithTemplate(JSContext* cx, uint32_t length, JSObject* templateObject);


extern JSObject*
NewDenseCopyOnWriteArray(JSContext* cx, HandleArrayObject templateObject, gc::InitialHeap heap);



extern JSObject*
NewFullyAllocatedArrayTryUseGroup(ExclusiveContext* cx, HandleObjectGroup group, size_t length,
                                  NewObjectKind newKind = GenericObject);

extern JSObject*
NewPartlyAllocatedArrayTryUseGroup(ExclusiveContext* cx, HandleObjectGroup group, size_t length);

extern JSObject*
NewFullyAllocatedArrayTryReuseGroup(JSContext* cx, JSObject* obj, size_t length,
                                    NewObjectKind newKind = GenericObject,
                                    bool forceAnalyze = false);

extern JSObject*
NewPartlyAllocatedArrayTryReuseGroup(JSContext* cx, JSObject* obj, size_t length);

extern JSObject*
NewFullyAllocatedArrayForCallingAllocationSite(JSContext* cx, size_t length,
                                               NewObjectKind newKind = GenericObject,
                                               bool forceAnalyze = false);

extern JSObject*
NewPartlyAllocatedArrayForCallingAllocationSite(JSContext* cx, size_t length);

enum class ShouldUpdateTypes
{
    Update,
    DontUpdate
};

extern JSObject*
NewCopiedArrayTryUseGroup(ExclusiveContext* cx, HandleObjectGroup group,
                          const Value* vp, size_t length,
                          NewObjectKind newKind = GenericObject,
                          ShouldUpdateTypes updateTypes = ShouldUpdateTypes::Update);

extern JSObject*
NewCopiedArrayForCallingAllocationSite(JSContext* cx, const Value* vp, size_t length);






extern bool
WouldDefinePastNonwritableLength(HandleNativeObject obj, uint32_t index);





extern bool
CanonicalizeArrayLengthValue(JSContext* cx, HandleValue v, uint32_t* canonicalized);

extern bool
GetLengthProperty(JSContext* cx, HandleObject obj, uint32_t* lengthp);

extern bool
SetLengthProperty(JSContext* cx, HandleObject obj, double length);

extern bool
ObjectMayHaveExtraIndexedProperties(JSObject* obj);







extern bool
GetElements(JSContext* cx, HandleObject aobj, uint32_t length, js::Value* vp);



extern bool
array_sort(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
array_push(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
array_pop(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
array_splice(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
array_splice_impl(JSContext* cx, unsigned argc, js::Value* vp, bool pop);

extern bool
array_concat(JSContext* cx, unsigned argc, js::Value* vp);

template <bool Locale>
JSString*
ArrayJoin(JSContext* cx, HandleObject obj, HandleLinearString sepstr, uint32_t length);

extern bool
array_concat_dense(JSContext* cx, HandleObject arr1, HandleObject arr2,
                   HandleObject result);

bool
array_join(JSContext* cx, unsigned argc, js::Value* vp);

extern JSString*
array_join_impl(JSContext* cx, HandleValue array, HandleString sep);

extern void
ArrayShiftMoveElements(JSObject* obj);

extern bool
array_shift(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
array_unshift(JSContext* cx, unsigned argc, js::Value* vp);

extern bool
array_slice(JSContext* cx, unsigned argc, js::Value* vp);

extern JSObject*
array_slice_dense(JSContext* cx, HandleObject obj, int32_t begin, int32_t end, HandleObject result);








extern bool
NewbornArrayPush(JSContext* cx, HandleObject obj, const Value& v);

extern JSObject*
ArrayConstructorOneArg(JSContext* cx, HandleObjectGroup group, int32_t lengthInt);

#ifdef DEBUG
extern bool
ArrayInfo(JSContext* cx, unsigned argc, Value* vp);
#endif


extern bool
ArrayConstructor(JSContext* cx, unsigned argc, Value* vp);

} 

#endif
