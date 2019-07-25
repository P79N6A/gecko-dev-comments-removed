






































#ifndef jstypedarrayinlines_h
#define jstypedarrayinlines_h

#include "jsapi.h"
#include "jsvalue.h"
#include "jsobj.h"

namespace js {
inline JSUint32
ArrayBuffer::getByteLength(JSObject *obj)
{
    return obj->getFixedSlot(JSSLOT_ARRAY_BYTELENGTH).toPrivateUint32();
}

inline uint8 *
ArrayBuffer::getDataOffset(JSObject *obj)
{
    return (uint8 *) obj->getFixedSlot(JSSLOT_ARRAY_DATA).toPrivate();
}

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
