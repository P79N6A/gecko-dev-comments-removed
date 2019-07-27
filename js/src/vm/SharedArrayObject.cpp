





#include "vm/SharedArrayObject.h"

#include "jsprf.h"
#include "jsobjinlines.h"

#ifdef XP_WIN
# include "jswin.h"
#else
# include <sys/mman.h>
#endif

#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

#include "mozilla/Atomics.h"

#include "asmjs/AsmJSValidate.h"

using namespace js;

using mozilla::IsNaN;
using mozilla::PodCopy;





static inline void *
MapMemory(size_t length, bool commit)
{
#ifdef XP_WIN
    int prot = (commit ? MEM_COMMIT : MEM_RESERVE);
    int flags = (commit ? PAGE_READWRITE : PAGE_NOACCESS);
    return VirtualAlloc(nullptr, length, prot, flags);
#else
    int prot = (commit ? (PROT_READ | PROT_WRITE) : PROT_NONE);
    void *p = mmap(nullptr, length, prot, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED)
        return nullptr;
    return p;
#endif
}

static inline void
UnmapMemory(void *addr, size_t len)
{
#ifdef XP_WIN
    VirtualFree(addr, 0, MEM_RELEASE);
#else
    munmap(addr, len);
#endif
}

static inline bool
MarkValidRegion(void *addr, size_t len)
{
#ifdef XP_WIN
    if (!VirtualAlloc(addr, len, MEM_COMMIT, PAGE_READWRITE))
        return false;
    return true;
#else
    if (mprotect(addr, len, PROT_READ | PROT_WRITE))
        return false;
    return true;
#endif
}

SharedArrayRawBuffer *
SharedArrayRawBuffer::New(uint32_t length)
{
    
    JS_ASSERT(IsValidAsmJSHeapLength(length));

#ifdef JS_CODEGEN_X64
    
    void *p = MapMemory(AsmJSMappedSize, false);
    if (!p)
        return nullptr;

    size_t validLength = AsmJSPageSize + length;
    if (!MarkValidRegion(p, validLength)) {
        UnmapMemory(p, AsmJSMappedSize);
        return nullptr;
    }
#   if defined(MOZ_VALGRIND) && defined(VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE)
    
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE((unsigned char*)p + validLength,
                                                   AsmJSMappedSize-validLength);
#   endif
#else
    uint32_t allocSize = length + AsmJSPageSize;
    if (allocSize <= length)
        return nullptr;

    void *p = MapMemory(allocSize, true);
    if (!p)
        return nullptr;
#endif
    uint8_t *buffer = reinterpret_cast<uint8_t*>(p) + AsmJSPageSize;
    uint8_t *base = buffer - sizeof(SharedArrayRawBuffer);
    return new (base) SharedArrayRawBuffer(buffer, length);
}

void
SharedArrayRawBuffer::addReference()
{
    JS_ASSERT(this->refcount > 0);
    ++this->refcount; 
}

void
SharedArrayRawBuffer::dropReference()
{
    
    uint32_t refcount = --this->refcount; 

    
    if (refcount == 0) {
        uint8_t *p = this->dataPointer() - AsmJSPageSize;
        JS_ASSERT(uintptr_t(p) % AsmJSPageSize == 0);
#ifdef JS_CODEGEN_X64
        UnmapMemory(p, AsmJSMappedSize);
#       if defined(MOZ_VALGRIND) \
           && defined(VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE)
        
        
        if (AsmJSMappedSize > 0) {
            VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(p, AsmJSMappedSize);
        }
#       endif
#else
        UnmapMemory(p, this->length + AsmJSPageSize);
#endif
    }
}




bool
js::IsSharedArrayBuffer(HandleValue v)
{
    return v.isObject() && v.toObject().is<SharedArrayBufferObject>();
}

MOZ_ALWAYS_INLINE bool
SharedArrayBufferObject::byteLengthGetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsSharedArrayBuffer(args.thisv()));
    args.rval().setInt32(args.thisv().toObject().as<SharedArrayBufferObject>().byteLength());
    return true;
}

bool
SharedArrayBufferObject::byteLengthGetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsSharedArrayBuffer, byteLengthGetterImpl>(cx, args);
}

bool
SharedArrayBufferObject::class_constructor(JSContext *cx, unsigned argc, Value *vp)
{
    int32_t length = 0;
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() > 0 && !ToInt32(cx, args[0], &length))
        return false;

    if (length < 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
        return false;
    }

    JSObject *bufobj = New(cx, uint32_t(length));
    if (!bufobj)
        return false;
    args.rval().setObject(*bufobj);
    return true;
}

JSObject *
SharedArrayBufferObject::New(JSContext *cx, uint32_t length)
{
    if (!IsValidAsmJSHeapLength(length)) {
        ScopedJSFreePtr<char> msg(
            JS_smprintf("SharedArrayBuffer byteLength 0x%x is not a valid length. The next valid "
                        "length is 0x%x", length, RoundUpToNextValidAsmJSHeapLength(length)));
        JS_ReportError(cx, msg.get());
        return nullptr;
    }

    SharedArrayRawBuffer *buffer = SharedArrayRawBuffer::New(length);
    if (!buffer)
        return nullptr;

    return New(cx, buffer);
}

JSObject *
SharedArrayBufferObject::New(JSContext *cx, SharedArrayRawBuffer *buffer)
{
    Rooted<SharedArrayBufferObject*> obj(cx, NewBuiltinClassInstance<SharedArrayBufferObject>(cx));
    if (!obj)
        return nullptr;

    JS_ASSERT(obj->getClass() == &class_);

    obj->initialize(buffer->byteLength(), BufferContents::createUnowned(nullptr), DoesntOwnData);

    obj->acceptRawBuffer(buffer);
    obj->setIsSharedArrayBuffer();

    return obj;
}

void
SharedArrayBufferObject::acceptRawBuffer(SharedArrayRawBuffer *buffer)
{
    setReservedSlot(SharedArrayBufferObject::RAWBUF_SLOT, PrivateValue(buffer));
}

void
SharedArrayBufferObject::dropRawBuffer()
{
    setReservedSlot(SharedArrayBufferObject::RAWBUF_SLOT, UndefinedValue());
}

SharedArrayRawBuffer *
SharedArrayBufferObject::rawBufferObject() const
{
    
    
    Value v = getReservedSlot(SharedArrayBufferObject::RAWBUF_SLOT);
    return (SharedArrayRawBuffer *)v.toPrivate();
}

uint8_t *
SharedArrayBufferObject::dataPointer() const
{
    return rawBufferObject()->dataPointer();
}

uint32_t
SharedArrayBufferObject::byteLength() const
{
    return rawBufferObject()->byteLength();
}

void
SharedArrayBufferObject::Finalize(FreeOp *fop, JSObject *obj)
{
    SharedArrayBufferObject &buf = obj->as<SharedArrayBufferObject>();

    
    
    Value v = buf.getReservedSlot(SharedArrayBufferObject::RAWBUF_SLOT);
    if (!v.isUndefined()) {
        buf.rawBufferObject()->dropReference();
        buf.dropRawBuffer();
    }
}





const Class SharedArrayBufferObject::protoClass = {
    "SharedArrayBufferPrototype",
    JSCLASS_HAS_CACHED_PROTO(JSProto_SharedArrayBuffer),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

const Class SharedArrayBufferObject::class_ = {
    "SharedArrayBuffer",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(SharedArrayBufferObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_SharedArrayBuffer),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    SharedArrayBufferObject::Finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    ArrayBufferObject::obj_trace,
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT
};

JSObject *
js_InitSharedArrayBufferClass(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->isNative());
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedObject proto(cx, global->createBlankPrototype(cx, &SharedArrayBufferObject::protoClass));
    if (!proto)
        return nullptr;

    RootedFunction ctor(cx, global->createConstructor(cx, SharedArrayBufferObject::class_constructor,
                                                      cx->names().SharedArrayBuffer, 1));
    if (!ctor)
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return nullptr;

    RootedId byteLengthId(cx, NameToId(cx->names().byteLength));
    unsigned attrs = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;
    JSObject *getter = NewFunction(cx, NullPtr(), SharedArrayBufferObject::byteLengthGetter, 0,
                                   JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return nullptr;

    RootedValue value(cx, UndefinedValue());
    if (!DefineNativeProperty(cx, proto, byteLengthId, value,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr, attrs))
    {
        return nullptr;
    }

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_SharedArrayBuffer, ctor, proto))
        return nullptr;
    return proto;
}
