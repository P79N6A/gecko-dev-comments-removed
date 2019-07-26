





#include "vm/SharedArrayObject.h"

#include "jsprf.h"
#include "jsobjinlines.h"

#ifdef XP_WIN
# include "jswin.h"
#else
# include <sys/mman.h>
#endif

#include "mozilla/Atomics.h"
#include "jit/AsmJS.h"

using namespace js;

using mozilla::IsNaN;
using mozilla::PodCopy;

#define SHAREDARRAYBUFFER_RESERVED_SLOTS 15





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

#ifdef JS_CPU_X64
    
    void *p = MapMemory(AsmJSMappedSize, false);
    if (!p)
        return nullptr;

    if (!MarkValidRegion(p, AsmJSPageSize + length)) {
        UnmapMemory(p, AsmJSMappedSize);
        return nullptr;
    }
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
#ifdef JS_CPU_X64
        UnmapMemory(p, AsmJSMappedSize);
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
    if (argc > 0 && !ToInt32(cx, args[0], &length))
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

    RootedObject obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return nullptr;

    JS_ASSERT(obj->getClass() == &class_);

    Rooted<js::Shape*> empty(cx);
    empty = EmptyShape::getInitialShape(cx, &class_, obj->getProto(), obj->getParent(),
                                        obj->getMetadata(), gc::FINALIZE_OBJECT16_BACKGROUND);
    if (!empty)
        return nullptr;
    obj->setLastPropertyInfallible(empty);

    obj->setFixedElements();
    obj->as<SharedArrayBufferObject>().initElementsHeader(obj->getElementsHeader(), length);
    obj->getElementsHeader()->setIsSharedArrayBuffer();

    SharedArrayRawBuffer *buffer = SharedArrayRawBuffer::New(length);
    if (!buffer)
        return nullptr;
    obj->as<SharedArrayBufferObject>().acceptRawBuffer(buffer);

    return obj;
}

JSObject *
SharedArrayBufferObject::New(JSContext *cx, SharedArrayRawBuffer *buffer)
{
    RootedObject obj(cx, NewBuiltinClassInstance(cx, &class_));
    if (!obj)
        return nullptr;

    JS_ASSERT(obj->getClass() == &class_);

    Rooted<js::Shape*> empty(cx);
    empty = EmptyShape::getInitialShape(cx, &class_, obj->getProto(), obj->getParent(),
                                        obj->getMetadata(), gc::FINALIZE_OBJECT16_BACKGROUND);
    if (!empty)
        return nullptr;
    obj->setLastPropertyInfallible(empty);

    obj->setFixedElements();
    obj->as<SharedArrayBufferObject>().initElementsHeader(obj->getElementsHeader(),
                                                          buffer->byteLength());
    obj->getElementsHeader()->setIsSharedArrayBuffer();

    obj->as<SharedArrayBufferObject>().acceptRawBuffer(buffer);

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
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(SHAREDARRAYBUFFER_RESERVED_SLOTS) |
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
    JSCLASS_HAS_PRIVATE |
    JSCLASS_IMPLEMENTS_BARRIERS |
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(SHAREDARRAYBUFFER_RESERVED_SLOTS) |
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
    JS_NULL_CLASS_EXT,
    {
        ArrayBufferObject::obj_lookupGeneric,
        ArrayBufferObject::obj_lookupProperty,
        ArrayBufferObject::obj_lookupElement,
        ArrayBufferObject::obj_lookupSpecial,
        ArrayBufferObject::obj_defineGeneric,
        ArrayBufferObject::obj_defineProperty,
        ArrayBufferObject::obj_defineElement,
        ArrayBufferObject::obj_defineSpecial,
        ArrayBufferObject::obj_getGeneric,
        ArrayBufferObject::obj_getProperty,
        ArrayBufferObject::obj_getElement,
        ArrayBufferObject::obj_getSpecial,
        ArrayBufferObject::obj_setGeneric,
        ArrayBufferObject::obj_setProperty,
        ArrayBufferObject::obj_setElement,
        ArrayBufferObject::obj_setSpecial,
        ArrayBufferObject::obj_getGenericAttributes,
        ArrayBufferObject::obj_setGenericAttributes,
        ArrayBufferObject::obj_deleteProperty,
        ArrayBufferObject::obj_deleteElement,
        ArrayBufferObject::obj_deleteSpecial,
        nullptr, nullptr, 
        nullptr,          
        ArrayBufferObject::obj_enumerate,
        nullptr,          
    }
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
    unsigned flags = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;
    JSObject *getter = NewFunction(cx, NullPtr(), SharedArrayBufferObject::byteLengthGetter, 0,
                                   JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return nullptr;

    RootedValue value(cx, UndefinedValue());
    if (!DefineNativeProperty(cx, proto, byteLengthId, value,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr, flags, 0, 0))
    {
        return nullptr;
    }

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_SharedArrayBuffer, ctor, proto))
        return nullptr;
    return proto;
}
