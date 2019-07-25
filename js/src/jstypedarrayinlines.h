






































#ifndef jstypedarrayinlines_h
#define jstypedarrayinlines_h

#include "jsapi.h"
#include "jsobj.h"

inline uint32_t
JSObject::arrayBufferByteLength()
{
    JS_ASSERT(isArrayBuffer());
    return getElementsHeader()->length;
}

inline uint8_t *
JSObject::arrayBufferDataOffset()
{
    return (uint8_t *) elements;
}

namespace js {

static inline int32_t
ClampIntForUint8Array(int32_t x)
{
    if (x < 0)
        return 0;
    if (x > 255)
        return 255;
    return x;
}

inline uint32_t
TypedArray::getLength(JSObject *obj) {
    JS_ASSERT(IsFastOrSlowTypedArray(obj));
    return obj->getFixedSlot(FIELD_LENGTH).toInt32();
}

inline uint32_t
TypedArray::getByteOffset(JSObject *obj) {
    JS_ASSERT(IsFastOrSlowTypedArray(obj));
    return obj->getFixedSlot(FIELD_BYTEOFFSET).toInt32();
}

inline uint32_t
TypedArray::getByteLength(JSObject *obj) {
    JS_ASSERT(IsFastOrSlowTypedArray(obj));
    return obj->getFixedSlot(FIELD_BYTELENGTH).toInt32();
}

inline uint32_t
TypedArray::getType(JSObject *obj) {
    JS_ASSERT(IsFastOrSlowTypedArray(obj));
    return obj->getFixedSlot(FIELD_TYPE).toInt32();
}

inline JSObject *
TypedArray::getBuffer(JSObject *obj) {
    JS_ASSERT(IsFastOrSlowTypedArray(obj));
    return &obj->getFixedSlot(FIELD_BUFFER).toObject();
}

inline void *
TypedArray::getDataOffset(JSObject *obj) {
    JS_ASSERT(IsFastOrSlowTypedArray(obj));
    return (void *)obj->getPrivate(NUM_FIXED_SLOTS);
}

}
#endif 
