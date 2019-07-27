





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




















class SharedArrayBufferObject : public ArrayBufferObjectMaybeShared
{
    static bool byteLengthGetterImpl(JSContext *cx, CallArgs args);

  public:
    
    
    static const uint8_t RAWBUF_SLOT = 0;

    static const uint8_t RESERVED_SLOTS = 1;

    static const Class class_;
    static const Class protoClass;
    static const JSFunctionSpec jsfuncs[];
    static const JSFunctionSpec jsstaticfuncs[];

    static bool byteLengthGetter(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_isView(JSContext *cx, unsigned argc, Value *vp);

    static bool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    
    static SharedArrayBufferObject *New(JSContext *cx, uint32_t length);

    
    static SharedArrayBufferObject *New(JSContext *cx, SharedArrayRawBuffer *buffer);

    static void Finalize(FreeOp *fop, JSObject *obj);

    static void addSizeOfExcludingThis(JSObject *obj, mozilla::MallocSizeOf mallocSizeOf,
                                       JS::ClassInfo *info);

    SharedArrayRawBuffer *rawBufferObject() const;

    
    
    void *globalID() const {
        
        
        
        return dataPointer();
    }

    uint32_t byteLength() const {
        return rawBufferObject()->byteLength();
    }

    uint8_t *dataPointer() const {
        return rawBufferObject()->dataPointer();
    }

private:
    void acceptRawBuffer(SharedArrayRawBuffer *buffer);
    void dropRawBuffer();
};

bool IsSharedArrayBuffer(HandleValue v);
bool IsSharedArrayBuffer(HandleObject o);

SharedArrayBufferObject &AsSharedArrayBuffer(HandleObject o);

} 

#endif 
