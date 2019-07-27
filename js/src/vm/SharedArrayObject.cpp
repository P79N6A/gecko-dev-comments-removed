





#include "vm/SharedArrayObject.h"

#include "jsprf.h"
#include "jsobjinlines.h"

#ifdef XP_WIN
# include "jswin.h"
#endif
#include "jswrapper.h"
#ifndef XP_WIN
# include <sys/mman.h>
#endif
#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

#include "mozilla/Atomics.h"

#include "asmjs/AsmJSValidate.h"
#include "vm/TypedArrayCommon.h"

using namespace js;

static inline void*
MapMemory(size_t length, bool commit)
{
#ifdef XP_WIN
    int prot = (commit ? MEM_COMMIT : MEM_RESERVE);
    int flags = (commit ? PAGE_READWRITE : PAGE_NOACCESS);
    return VirtualAlloc(nullptr, length, prot, flags);
#else
    int prot = (commit ? (PROT_READ | PROT_WRITE) : PROT_NONE);
    void* p = mmap(nullptr, length, prot, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED)
        return nullptr;
    return p;
#endif
}

static inline void
UnmapMemory(void* addr, size_t len)
{
#ifdef XP_WIN
    VirtualFree(addr, 0, MEM_RELEASE);
#else
    munmap(addr, len);
#endif
}

static inline bool
MarkValidRegion(void* addr, size_t len)
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

#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)




static const uint64_t SharedArrayMappedSize = AsmJSMappedSize + AsmJSPageSize;
static_assert(sizeof(SharedArrayRawBuffer) < AsmJSPageSize, "Page size not big enough");








static mozilla::Atomic<uint32_t, mozilla::ReleaseAcquire> numLive;
static const uint32_t maxLive = 1000;
#endif

SharedArrayRawBuffer*
SharedArrayRawBuffer::New(JSContext* cx, uint32_t length)
{
    
    
    MOZ_ASSERT(length != (uint32_t)-1);

    
    uint32_t allocSize = (length + 2*AsmJSPageSize - 1) & ~(AsmJSPageSize - 1);
    if (allocSize <= length)
        return nullptr;
#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
    
    
    if (++numLive >= maxLive) {
        JSRuntime* rt = cx->runtime();
        if (rt->largeAllocationFailureCallback)
            rt->largeAllocationFailureCallback(rt->largeAllocationFailureCallbackData);
        if (numLive >= maxLive) {
            numLive--;
            return nullptr;
        }
    }
    
    void* p = MapMemory(SharedArrayMappedSize, false);
    if (!p) {
        numLive--;
        return nullptr;
    }

    if (!MarkValidRegion(p, allocSize)) {
        UnmapMemory(p, SharedArrayMappedSize);
        numLive--;
        return nullptr;
    }
#   if defined(MOZ_VALGRIND) && defined(VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE)
    
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE((unsigned char*)p + allocSize,
                                                   SharedArrayMappedSize - allocSize);
#   endif
#else
    void* p = MapMemory(allocSize, true);
    if (!p)
        return nullptr;
#endif
    uint8_t* buffer = reinterpret_cast<uint8_t*>(p) + AsmJSPageSize;
    uint8_t* base = buffer - sizeof(SharedArrayRawBuffer);
    return new (base) SharedArrayRawBuffer(buffer, length);
}

void
SharedArrayRawBuffer::addReference()
{
    MOZ_ASSERT(this->refcount > 0);
    ++this->refcount; 
}

void
SharedArrayRawBuffer::dropReference()
{
    
    uint32_t refcount = --this->refcount; 

    
    if (refcount == 0) {
        uint8_t* p = this->dataPointer() - AsmJSPageSize;
        MOZ_ASSERT(uintptr_t(p) % AsmJSPageSize == 0);
#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
        numLive--;
        UnmapMemory(p, SharedArrayMappedSize);
#       if defined(MOZ_VALGRIND) \
           && defined(VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE)
        
        
        VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(p, SharedArrayMappedSize);
#       endif
#else
        UnmapMemory(p, this->length + AsmJSPageSize);
#endif
    }
}

const JSFunctionSpec SharedArrayBufferObject::jsfuncs[] = {
    
    JS_FS_END
};

const JSFunctionSpec SharedArrayBufferObject::jsstaticfuncs[] = {
    JS_FN("isView", SharedArrayBufferObject::fun_isView, 1, 0),
    JS_FS_END
};

MOZ_ALWAYS_INLINE bool
SharedArrayBufferObject::byteLengthGetterImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsSharedArrayBuffer(args.thisv()));
    args.rval().setInt32(args.thisv().toObject().as<SharedArrayBufferObject>().byteLength());
    return true;
}

bool
SharedArrayBufferObject::byteLengthGetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsSharedArrayBuffer, byteLengthGetterImpl>(cx, args);
}

bool
SharedArrayBufferObject::fun_isView(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(args.get(0).isObject() &&
                           JS_IsSharedTypedArrayObject(&args.get(0).toObject()));
    return true;
}

bool
SharedArrayBufferObject::class_constructor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        if (args.hasDefined(0) && IsObjectWithClass(args[0], ESClass_SharedArrayBuffer, cx)) {
            args.rval().set(args[0]);
            return true;
        }
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_SHARED_ARRAY_BAD_OBJECT);
        return false;
    }

    uint32_t length;
    bool overflow;
    if (!ToLengthClamped(cx, args.get(0), &length, &overflow)) {
        
        if (overflow || length > INT32_MAX)
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_SHARED_ARRAY_BAD_LENGTH);
        return false;
    }

    JSObject* bufobj = New(cx, length);
    if (!bufobj)
        return false;
    args.rval().setObject(*bufobj);
    return true;
}

SharedArrayBufferObject*
SharedArrayBufferObject::New(JSContext* cx, uint32_t length)
{
    SharedArrayRawBuffer* buffer = SharedArrayRawBuffer::New(cx, length);
    if (!buffer)
        return nullptr;

    return New(cx, buffer);
}

SharedArrayBufferObject*
SharedArrayBufferObject::New(JSContext* cx, SharedArrayRawBuffer* buffer)
{
    Rooted<SharedArrayBufferObject*> obj(cx, NewBuiltinClassInstance<SharedArrayBufferObject>(cx));
    if (!obj)
        return nullptr;

    MOZ_ASSERT(obj->getClass() == &class_);

    obj->acceptRawBuffer(buffer);

    return obj;
}

void
SharedArrayBufferObject::acceptRawBuffer(SharedArrayRawBuffer* buffer)
{
    setReservedSlot(RAWBUF_SLOT, PrivateValue(buffer));
}

void
SharedArrayBufferObject::dropRawBuffer()
{
    setReservedSlot(RAWBUF_SLOT, UndefinedValue());
}

SharedArrayRawBuffer*
SharedArrayBufferObject::rawBufferObject() const
{
    Value v = getReservedSlot(RAWBUF_SLOT);
    MOZ_ASSERT(!v.isUndefined());
    return reinterpret_cast<SharedArrayRawBuffer*>(v.toPrivate());
}

void
SharedArrayBufferObject::Finalize(FreeOp* fop, JSObject* obj)
{
    SharedArrayBufferObject& buf = obj->as<SharedArrayBufferObject>();

    
    
    Value v = buf.getReservedSlot(RAWBUF_SLOT);
    if (!v.isUndefined()) {
        buf.rawBufferObject()->dropReference();
        buf.dropRawBuffer();
    }
}

 void
SharedArrayBufferObject::addSizeOfExcludingThis(JSObject* obj, mozilla::MallocSizeOf mallocSizeOf,
                                                JS::ClassInfo* info)
{
    info->objectsNonHeapElementsMapped += obj->as<SharedArrayBufferObject>().byteLength();
}

const Class SharedArrayBufferObject::protoClass = {
    "SharedArrayBufferPrototype",
    JSCLASS_HAS_CACHED_PROTO(JSProto_SharedArrayBuffer)
};

const Class SharedArrayBufferObject::class_ = {
    "SharedArrayBuffer",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(SharedArrayBufferObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_SharedArrayBuffer),
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    SharedArrayBufferObject::Finalize,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT
};

JSObject*
js::InitSharedArrayBufferClass(JSContext* cx, HandleObject obj)
{
    MOZ_ASSERT(obj->isNative());
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    RootedNativeObject proto(cx, global->createBlankPrototype(cx, &SharedArrayBufferObject::protoClass));
    if (!proto)
        return nullptr;

    RootedFunction ctor(cx, global->createConstructor(cx, SharedArrayBufferObject::class_constructor,
                                                      cx->names().SharedArrayBuffer, 1));
    if (!ctor)
        return nullptr;

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_SharedArrayBuffer, ctor, proto))
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return nullptr;

    RootedId byteLengthId(cx, NameToId(cx->names().byteLength));
    unsigned attrs = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;
    JSObject* getter =
        NewNativeFunction(cx, SharedArrayBufferObject::byteLengthGetter, 0, NullPtr());
    if (!getter)
        return nullptr;

    if (!NativeDefineProperty(cx, proto, byteLengthId, UndefinedHandleValue,
                              JS_DATA_TO_FUNC_PTR(GetterOp, getter), nullptr, attrs))
        return nullptr;

    if (!JS_DefineFunctions(cx, ctor, SharedArrayBufferObject::jsstaticfuncs))
        return nullptr;

    if (!JS_DefineFunctions(cx, proto, SharedArrayBufferObject::jsfuncs))
        return nullptr;

    return proto;
}

bool
js::IsSharedArrayBuffer(HandleValue v)
{
    return v.isObject() && v.toObject().is<SharedArrayBufferObject>();
}

bool
js::IsSharedArrayBuffer(HandleObject o)
{
    return o->is<SharedArrayBufferObject>();
}

SharedArrayBufferObject&
js::AsSharedArrayBuffer(HandleObject obj)
{
    MOZ_ASSERT(IsSharedArrayBuffer(obj));
    return obj->as<SharedArrayBufferObject>();
}

JS_FRIEND_API(bool)
JS_IsSharedArrayBufferObject(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<ArrayBufferObject>() : false;
}
