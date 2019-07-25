






































#ifndef jstypedarrayinlines_h
#define jstypedarrayinlines_h

#include "jsapi.h"
#include "jsvalue.h"
#include "jsobj.h"

namespace js {
inline JSUint32
ArrayBuffer::getByteLength(JSObject *obj)
{
    return *((JSUint32*) obj->slots);
}

inline uint8 *
ArrayBuffer::getDataOffset(JSObject *obj) {
    uint64 *base = ((uint64*)obj->slots) + 1;
    return (uint8*) base;
}
}
#endif 
