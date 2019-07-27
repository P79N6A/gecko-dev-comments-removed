





#ifndef vm_ArrayBufferObject_inl_h
#define vm_ArrayBufferObject_inl_h



#include "vm/ArrayBufferObject.h"

#include "js/Value.h"

#include "vm/SharedArrayObject.h"

namespace js {

inline uint32_t
AnyArrayBufferByteLength(const ArrayBufferObjectMaybeShared *buf)
{
    if (buf->is<ArrayBufferObject>())
	return buf->as<ArrayBufferObject>().byteLength();
    return buf->as<SharedArrayBufferObject>().byteLength();
}

inline uint8_t *
AnyArrayBufferDataPointer(const ArrayBufferObjectMaybeShared *buf)
{
    if (buf->is<ArrayBufferObject>())
	return buf->as<ArrayBufferObject>().dataPointer();
    return buf->as<SharedArrayBufferObject>().dataPointer();
}

inline ArrayBufferObjectMaybeShared &
AsAnyArrayBuffer(HandleValue val)
{
    if (val.toObject().is<ArrayBufferObject>())
	return val.toObject().as<ArrayBufferObject>();
    return val.toObject().as<SharedArrayBufferObject>();
}

} 

#endif 
