





#ifndef vm_SharedArrayObject_h
#define vm_SharedArrayObject_h

#include "mozilla/Atomics.h"

#include "jsapi.h"
#include "jsobj.h"
#include "jstypes.h"

#include "gc/Barrier.h"
#include "vm/ArrayBufferObject.h"

typedef struct JSProperty JSProperty;

namespace js {












class SharedArrayRawBuffer
{
  private:
    mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> refcount;
    uint32_t length;

  protected:
    SharedArrayRawBuffer(uint8_t *buffer, uint32_t length)
      : refcount(1), length(length)
    {
        JS_ASSERT(buffer == dataPointer());
    }

  public:
    static SharedArrayRawBuffer *New(uint32_t length);

    inline uint8_t *dataPointer() const {
        return ((uint8_t *)this) + sizeof(SharedArrayRawBuffer);
    }

    inline uint32_t byteLength() const {
        return length;
    }

    void addReference();
    void dropReference();
};







class SharedArrayBufferObject : public ArrayBufferObject
{
    static bool byteLengthGetterImpl(JSContext *cx, CallArgs args);

  public:
    static const Class class_;
    static const Class protoClass;

    
    
    static const int32_t RAWBUF_SLOT = 2;

    static bool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    
    static JSObject *New(JSContext *cx, uint32_t length);

    
    static JSObject *New(JSContext *cx, SharedArrayRawBuffer *buffer);

    static bool byteLengthGetter(JSContext *cx, unsigned argc, Value *vp);

    static void Finalize(FreeOp *fop, JSObject *obj);

    void acceptRawBuffer(SharedArrayRawBuffer *buffer);
    void dropRawBuffer();

    SharedArrayRawBuffer *rawBufferObject() const;
    uint8_t *dataPointer() const;
    uint32_t byteLength() const;
};

bool
IsSharedArrayBuffer(HandleValue v);

} 

#endif 
