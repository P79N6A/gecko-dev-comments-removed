







































#ifndef ArgumentsObject_inl_h___
#define ArgumentsObject_inl_h___

#include "ArgumentsObject.h"

namespace js {

inline void
ArgumentsObject::initInitialLength(uint32 length)
{
    JS_ASSERT(getFixedSlot(INITIAL_LENGTH_SLOT).isUndefined());
    initFixedSlot(INITIAL_LENGTH_SLOT, Int32Value(length << PACKED_BITS_COUNT));
    JS_ASSERT((getFixedSlot(INITIAL_LENGTH_SLOT).toInt32() >> PACKED_BITS_COUNT) == int32(length));
    JS_ASSERT(!hasOverriddenLength());
}

inline uint32
ArgumentsObject::initialLength() const
{
    uint32 argc = uint32(getFixedSlot(INITIAL_LENGTH_SLOT).toInt32()) >> PACKED_BITS_COUNT;
    JS_ASSERT(argc <= StackSpace::ARGS_LENGTH_MAX);
    return argc;
}

inline void
ArgumentsObject::markLengthOverridden()
{
    uint32 v = getFixedSlot(INITIAL_LENGTH_SLOT).toInt32() | LENGTH_OVERRIDDEN_BIT;
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
ArgumentsObject::element(uint32 i) const
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
ArgumentsObject::setElement(uint32 i, const js::Value &v)
{
    JS_ASSERT(i < initialLength());
    data()->slots[i] = v;
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

#define JS_ARGUMENTS_OBJECT_ON_TRACE ((void *)0xa126)
inline bool
ArgumentsObject::onTrace() const
{
    return getFixedSlot(STACK_FRAME_SLOT).toPrivate() == JS_ARGUMENTS_OBJECT_ON_TRACE;
}

inline void
ArgumentsObject::setOnTrace()
{
    setFixedSlot(STACK_FRAME_SLOT, PrivateValue(JS_ARGUMENTS_OBJECT_ON_TRACE));
}

inline void
ArgumentsObject::clearOnTrace()
{
    setFixedSlot(STACK_FRAME_SLOT, PrivateValue(NULL));
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
