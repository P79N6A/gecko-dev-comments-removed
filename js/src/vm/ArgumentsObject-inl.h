







































#ifndef ArgumentsObject_inl_h___
#define ArgumentsObject_inl_h___

#include "ArgumentsObject.h"

namespace js {

inline void
ArgumentsObject::setInitialLength(uint32 length)
{
    JS_ASSERT(getSlot(INITIAL_LENGTH_SLOT).isUndefined());
    setSlot(INITIAL_LENGTH_SLOT, Int32Value(length << PACKED_BITS_COUNT));
    JS_ASSERT((getSlot(INITIAL_LENGTH_SLOT).toInt32() >> PACKED_BITS_COUNT) == length);
    JS_ASSERT(!hasOverriddenLength());
}

inline uint32
ArgumentsObject::initialLength() const
{
    uint32 argc = uint32(getSlot(INITIAL_LENGTH_SLOT).toInt32()) >> PACKED_BITS_COUNT;
    JS_ASSERT(argc <= JS_ARGS_LENGTH_MAX);
    return argc;
}

inline void
ArgumentsObject::markLengthOverridden()
{
    getSlotRef(INITIAL_LENGTH_SLOT).getInt32Ref() |= LENGTH_OVERRIDDEN_BIT;
}

inline bool
ArgumentsObject::hasOverriddenLength() const
{
    const js::Value &v = getSlot(INITIAL_LENGTH_SLOT);
    return v.toInt32() & LENGTH_OVERRIDDEN_BIT;
}

inline void
ArgumentsObject::setCalleeAndData(JSObject &callee, ArgumentsData *data)
{
    JS_ASSERT(getSlot(DATA_SLOT).isUndefined());
    setSlot(DATA_SLOT, PrivateValue(data));
    data->callee.setObject(callee);
}

inline ArgumentsData *
ArgumentsObject::data() const
{
    return reinterpret_cast<js::ArgumentsData *>(getSlot(DATA_SLOT).toPrivate());
}

inline const js::Value &
ArgumentsObject::element(uint32 i) const
{
    JS_ASSERT(i < initialLength());
    return data()->slots[i];
}

inline js::Value *
ArgumentsObject::elements() const
{
    return data()->slots;
}

inline Value *
ArgumentsObject::addressOfElement(uint32 i)
{
    JS_ASSERT(i < initialLength());
    return &data()->slots[i];
}

inline void
ArgumentsObject::setElement(uint32 i, const js::Value &v)
{
    JS_ASSERT(i < initialLength());
    data()->slots[i] = v;
}

inline const js::Value &
NormalArgumentsObject::callee() const
{
    return data()->callee;
}

inline void
NormalArgumentsObject::clearCallee()
{
    data()->callee = MagicValue(JS_ARGS_HOLE);
}

} 

#endif 
