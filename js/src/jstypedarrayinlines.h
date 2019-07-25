






































#ifndef jstypedarrayinlines_h
#define jstypedarrayinlines_h

#include "jsapi.h"
#include "jsobj.h"

inline uint32
JSObject::arrayBufferByteLength()
{
    JS_ASSERT(isArrayBuffer());
    return getElementsHeader()->length;
}

inline uint8 *
JSObject::arrayBufferDataOffset()
{
    return (uint8 *) elements;
}

namespace js {

static inline int32
ClampIntForUint8Array(int32 x)
{
    if (x < 0)
        return 0;
    if (x > 255)
        return 255;
    return x;
}

inline JSUint32
TypedArray::getLength(JSObject *obj) {
    return obj->getFixedSlot(FIELD_LENGTH).toInt32();
}

inline JSUint32
TypedArray::getByteOffset(JSObject *obj) {
    return obj->getFixedSlot(FIELD_BYTEOFFSET).toInt32();
}

inline JSUint32
TypedArray::getByteLength(JSObject *obj) {
    return obj->getFixedSlot(FIELD_BYTELENGTH).toInt32();
}

inline JSUint32
TypedArray::getType(JSObject *obj) {
    return obj->getFixedSlot(FIELD_TYPE).toInt32();
}

inline JSObject *
TypedArray::getBuffer(JSObject *obj) {
    return &obj->getFixedSlot(FIELD_BUFFER).toObject();
}

inline void *
TypedArray::getDataOffset(JSObject *obj) {
    return (void *)obj->getPrivate();
}

}
#endif 
