






































#ifndef jsarrayinlines_h___
#define jsarrayinlines_h___

#include "jsinferinlines.h"
#include "jsobjinlines.h"

inline void
JSObject::markDenseArrayNotPacked(JSContext *cx)
{
    JS_ASSERT(isDenseArray());
    MarkTypeObjectFlags(cx, this, js::types::OBJECT_FLAG_NON_PACKED_ARRAY);
}

inline void
JSObject::ensureDenseArrayInitializedLength(JSContext *cx, uint32_t index, uint32_t extra)
{
    




    JS_ASSERT(index + extra <= getDenseArrayCapacity());
    uint32_t &initlen = getElementsHeader()->initializedLength;
    if (initlen < index)
        markDenseArrayNotPacked(cx);

    if (initlen < index + extra) {
        JSCompartment *comp = compartment();
        size_t offset = initlen;
        for (js::HeapSlot *sp = elements + initlen;
             sp != elements + (index + extra);
             sp++, offset++)
            sp->init(comp, this, offset, js::MagicValue(JS_ARRAY_HOLE));
        initlen = index + extra;
    }
}

inline JSObject::EnsureDenseResult
JSObject::ensureDenseArrayElements(JSContext *cx, unsigned index, unsigned extra)
{
    JS_ASSERT(isDenseArray());

    unsigned currentCapacity = getDenseArrayCapacity();

    unsigned requiredCapacity;
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
    if (!growElements(cx, requiredCapacity))
        return ED_FAILED;

    ensureDenseArrayInitializedLength(cx, index, extra);
    return ED_OK;
}

#endif 
