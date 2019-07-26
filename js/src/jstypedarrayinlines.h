





#ifndef jstypedarrayinlines_h
#define jstypedarrayinlines_h

#include "jsapi.h"
#include "jsobj.h"
#include "jstypedarray.h"

#include "jscntxtinlines.h"
#include "jsobjinlines.h"

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

#ifdef JSGC_GENERATIONAL
class ArrayBufferViewByteOffsetRef : public gc::BufferableRef
{
    JSObject *obj;

  public:
    explicit ArrayBufferViewByteOffsetRef(JSObject *obj) : obj(obj) {}

    void mark(JSTracer *trc) {
        MarkObjectUnbarriered(trc, &obj, "TypedArray");
        obj->getClass()->trace(trc, obj);
    }
};
#endif

static inline void
InitArrayBufferViewDataPointer(JSObject *obj, ArrayBufferObject *buffer, size_t byteOffset)
{
    




    obj->initPrivate(buffer->dataPointer() + byteOffset);
#ifdef JSGC_GENERATIONAL
    if (IsInsideNursery(obj->runtime(), buffer) && buffer->hasFixedElements())
        obj->runtime()->gcStoreBuffer.putGeneric(ArrayBufferViewByteOffsetRef(obj));
#endif
}

} 

#endif 
