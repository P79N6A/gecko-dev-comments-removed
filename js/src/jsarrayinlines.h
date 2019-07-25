






































#ifndef jsarrayinlines_h___
#define jsarrayinlines_h___

#include "jsinferinlines.h"
#include "jsobjinlines.h"

inline void
JSObject::setDenseArrayInitializedLength(uint32 length)
{
    JS_ASSERT(isDenseArray());
    JS_ASSERT(length <= getDenseArrayCapacity());
    initializedLength = length;
}

inline void
JSObject::markDenseArrayNotPacked(JSContext *cx)
{
    JS_ASSERT(isDenseArray());
    if (flags & PACKED_ARRAY) {
        flags ^= PACKED_ARRAY;
        MarkTypeObjectFlags(cx, this, js::types::OBJECT_FLAG_NON_PACKED_ARRAY);
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
        js::ClearValueRange(slots + initializedLength, index - initializedLength, true);
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

#endif 
