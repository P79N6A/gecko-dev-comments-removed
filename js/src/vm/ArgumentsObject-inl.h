







































#ifndef ArgumentsObject_inl_h___
#define ArgumentsObject_inl_h___

#include "ArgumentsObject.h"

namespace js {

inline void
ArgumentsObject::initInitialLength(uint32_t length)
{
    JS_ASSERT(getFixedSlot(INITIAL_LENGTH_SLOT).isUndefined());
    initFixedSlot(INITIAL_LENGTH_SLOT, Int32Value(length << PACKED_BITS_COUNT));
    JS_ASSERT((getFixedSlot(INITIAL_LENGTH_SLOT).toInt32() >> PACKED_BITS_COUNT) == int32_t(length));
    JS_ASSERT(!hasOverriddenLength());
}

inline uint32_t
ArgumentsObject::initialLength() const
{
    uint32_t argc = uint32_t(getFixedSlot(INITIAL_LENGTH_SLOT).toInt32()) >> PACKED_BITS_COUNT;
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);
    return argc;
}

inline void
ArgumentsObject::markLengthOverridden()
{
    uint32_t v = getFixedSlot(INITIAL_LENGTH_SLOT).toInt32() | LENGTH_OVERRIDDEN_BIT;
    setFixedSlot(INITIAL_LENGTH_SLOT, Int32Value(v));
}

inline bool
ArgumentsObject::hasOverriddenLength() const
{
    const js::Value &v = getFixedSlot(INITIAL_LENGTH_SLOT);
    return v.toInt32() & LENGTH_OVERRIDDEN_BIT;
}

inline void
ArgumentsObject::initData(ArgumentsData *data)
{
    JS_ASSERT(getFixedSlot(DATA_SLOT).isUndefined());
    initFixedSlot(DATA_SLOT, PrivateValue(data));
}

inline ArgumentsData *
ArgumentsObject::data() const
{
    return reinterpret_cast<js::ArgumentsData *>(getFixedSlot(DATA_SLOT).toPrivate());
}

inline const js::Value &
ArgumentsObject::element(uint32_t i) const
{
    JS_ASSERT(i < initialLength());
    return data()->slots[i];
}

inline const js::Value *
ArgumentsObject::elements() const
{
    return Valueify(data()->slots);
}

inline void
ArgumentsObject::setElement(uint32_t i, const js::Value &v)
{
    JS_ASSERT(i < initialLength());
    data()->slots[i] = v;
}

inline bool
ArgumentsObject::getElement(uint32_t i, Value *vp)
{
    if (i >= initialLength())
        return false;

    *vp = element(i);

    



    if (vp->isMagic(JS_ARGS_HOLE))
        return false;

    




    StackFrame *fp = maybeStackFrame();
    JS_ASSERT_IF(isStrictArguments(), !fp);
    if (fp)
        *vp = fp->canonicalActualArg(i);
    return true;
}

namespace detail {

struct STATIC_SKIP_INFERENCE CopyNonHoleArgsTo
{
    CopyNonHoleArgsTo(ArgumentsObject *argsobj, Value *dst) : argsobj(*argsobj), dst(dst) {}
    ArgumentsObject &argsobj;
    Value *dst;
    bool operator()(uint32_t argi, Value *src) {
        if (argsobj.element(argi).isMagic(JS_ARGS_HOLE))
            return false;
        *dst++ = *src;
        return true;
    }
};

} 

inline bool
ArgumentsObject::getElements(uint32_t start, uint32_t count, Value *vp)
{
    JS_ASSERT(start + count >= start);

    uint32_t length = initialLength();
    if (start > length || start + count > length)
        return false;

    StackFrame *fp = maybeStackFrame();

    
    if (!fp) {
        const Value *srcbeg = elements() + start;
        const Value *srcend = srcbeg + count;
        const Value *src = srcbeg;
        for (Value *dst = vp; src < srcend; ++dst, ++src) {
            if (src->isMagic(JS_ARGS_HOLE))
                return false;
            *dst = *src;
        }
        return true;
    }

    
    JS_ASSERT(fp->numActualArgs() <= StackSpace::ARGS_LENGTH_MAX);
    return fp->forEachCanonicalActualArg(detail::CopyNonHoleArgsTo(this, vp), start, count);
}

inline js::StackFrame *
ArgumentsObject::maybeStackFrame() const
{
    return reinterpret_cast<js::StackFrame *>(getFixedSlot(STACK_FRAME_SLOT).toPrivate());
}

inline void
ArgumentsObject::setStackFrame(StackFrame *frame)
{
    setFixedSlot(STACK_FRAME_SLOT, PrivateValue(frame));
}

inline size_t
ArgumentsObject::sizeOfMisc(JSMallocSizeOfFun mallocSizeOf) const
{
    return mallocSizeOf(data());
}

inline const js::Value &
NormalArgumentsObject::callee() const
{
    return data()->callee;
}

inline void
NormalArgumentsObject::clearCallee()
{
    data()->callee.set(compartment(), MagicValue(JS_ARGS_HOLE));
}

} 

#endif 
