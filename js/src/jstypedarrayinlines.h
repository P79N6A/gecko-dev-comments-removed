






































#ifndef jstypedarrayinlines_h
#define jstypedarrayinlines_h

#include "jsapi.h"
#include "jsvalue.h"
#include "jsobj.h"

namespace js {
inline JSUint32
ArrayBuffer::getByteLength(JSObject *obj)
{
    return *((JSUint32*) obj->getSlotsPtr());
}

inline uint8 *
ArrayBuffer::getDataOffset(JSObject *obj) {
    uint64 *base = ((uint64*)obj->getSlotsPtr()) + 1;
    return (uint8*) base;
}

inline JSUint32
TypedArray::getLength(JSObject *obj) {
    return obj->getSlot(FIELD_LENGTH).toInt32();
}

inline JSUint32
TypedArray::getByteOffset(JSObject *obj) {
    return obj->getSlot(FIELD_BYTEOFFSET).toInt32();
}

inline JSUint32
TypedArray::getByteLength(JSObject *obj) {
    return obj->getSlot(FIELD_BYTELENGTH).toInt32();
}

inline JSUint32
TypedArray::getType(JSObject *obj) {
    return obj->getSlot(FIELD_TYPE).toInt32();
}

inline JSObject *
TypedArray::getBuffer(JSObject *obj) {
    return &obj->getSlot(FIELD_BUFFER).toObject();
}

inline void *
TypedArray::getDataOffset(JSObject *obj) {
    return (void *)((uint8*)obj->getSlot(FIELD_DATA).toPrivate() + getByteOffset(obj));
}
}
#endif 
