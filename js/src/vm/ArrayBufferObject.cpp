





#include "vm/ArrayBufferObject.h"

#include "mozilla/Alignment.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"
#include "mozilla/TaggedAnonymousMemory.h"

#include <string.h>
#ifndef XP_WIN
# include <sys/mman.h>
#endif

#ifdef MOZ_VALGRIND
# include <valgrind/memcheck.h>
#endif

#include "jsapi.h"
#include "jsarray.h"
#include "jscntxt.h"
#include "jscpucfg.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jstypes.h"
#include "jsutil.h"
#ifdef XP_WIN
# include "jswin.h"
#endif
#include "jswrapper.h"

#include "asmjs/AsmJSModule.h"
#include "asmjs/AsmJSValidate.h"
#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "gc/Memory.h"
#include "js/Conversions.h"
#include "js/MemoryMetrics.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/WrapperObject.h"

#include "jsatominlines.h"

#include "vm/NativeObject-inl.h"
#include "vm/Shape-inl.h"

using JS::ToInt32;

using mozilla::DebugOnly;
using mozilla::UniquePtr;

using namespace js;
using namespace js::gc;






bool
js::ToClampedIndex(JSContext* cx, HandleValue v, uint32_t length, uint32_t* out)
{
    int32_t result;
    if (!ToInt32(cx, v, &result))
        return false;
    if (result < 0) {
        result += length;
        if (result < 0)
            result = 0;
    } else if (uint32_t(result) > length) {
        result = length;
    }
    *out = uint32_t(result);
    return true;
}













const Class ArrayBufferObject::protoClass = {
    "ArrayBufferPrototype",
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer)
};

const Class ArrayBufferObject::class_ = {
    "ArrayBuffer",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer) |
    JSCLASS_BACKGROUND_FINALIZE,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    ArrayBufferObject::finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    ArrayBufferObject::trace,
    JS_NULL_CLASS_SPEC,
    {
        nullptr,    
        nullptr,    
        false,      
        nullptr,    
        ArrayBufferObject::objectMoved
    }
};

const JSFunctionSpec ArrayBufferObject::jsfuncs[] = {
    JS_FN("slice", ArrayBufferObject::fun_slice, 2, JSFUN_GENERIC_NATIVE),
    JS_FS_END
};

const JSFunctionSpec ArrayBufferObject::jsstaticfuncs[] = {
    JS_FN("isView", ArrayBufferObject::fun_isView, 1, 0),
#ifdef NIGHTLY_BUILD
    JS_FN("transfer", ArrayBufferObject::fun_transfer, 2, 0),
#endif
    JS_FS_END
};

bool
js::IsArrayBuffer(HandleValue v)
{
    return v.isObject() && v.toObject().is<ArrayBufferObject>();
}

bool
js::IsArrayBuffer(HandleObject obj)
{
    return obj->is<ArrayBufferObject>();
}

bool
js::IsArrayBuffer(JSObject* obj)
{
    return obj->is<ArrayBufferObject>();
}

ArrayBufferObject&
js::AsArrayBuffer(HandleObject obj)
{
    MOZ_ASSERT(IsArrayBuffer(obj));
    return obj->as<ArrayBufferObject>();
}

ArrayBufferObject&
js::AsArrayBuffer(JSObject* obj)
{
    MOZ_ASSERT(IsArrayBuffer(obj));
    return obj->as<ArrayBufferObject>();
}

MOZ_ALWAYS_INLINE bool
ArrayBufferObject::byteLengthGetterImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsArrayBuffer(args.thisv()));
    args.rval().setInt32(args.thisv().toObject().as<ArrayBufferObject>().byteLength());
    return true;
}

bool
ArrayBufferObject::byteLengthGetter(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, byteLengthGetterImpl>(cx, args);
}

bool
ArrayBufferObject::fun_slice_impl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsArrayBuffer(args.thisv()));

    Rooted<ArrayBufferObject*> thisObj(cx, &args.thisv().toObject().as<ArrayBufferObject>());

    
    uint32_t length = thisObj->byteLength();
    uint32_t begin = 0, end = length;

    if (args.length() > 0) {
        if (!ToClampedIndex(cx, args[0], length, &begin))
            return false;

        if (args.length() > 1) {
            if (!ToClampedIndex(cx, args[1], length, &end))
                return false;
        }
    }

    if (begin > end)
        begin = end;

    JSObject* nobj = createSlice(cx, thisObj, begin, end);
    if (!nobj)
        return false;
    args.rval().setObject(*nobj);
    return true;
}

bool
ArrayBufferObject::fun_slice(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, fun_slice_impl>(cx, args);
}




bool
ArrayBufferObject::fun_isView(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(args.get(0).isObject() &&
                           JS_IsArrayBufferViewObject(&args.get(0).toObject()));
    return true;
}

#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
static void
ReleaseAsmJSMappedData(void* base)
{
    MOZ_ASSERT(uintptr_t(base) % AsmJSPageSize == 0);
#  ifdef XP_WIN
    VirtualFree(base, 0, MEM_RELEASE);
#  else
    munmap(base, AsmJSMappedSize);
#   if defined(MOZ_VALGRIND) && defined(VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE)
    
    
    if (AsmJSMappedSize > 0) {
        VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(base, AsmJSMappedSize);
    }
#   endif
#  endif
}
#else
static void
ReleaseAsmJSMappedData(void* base)
{
    MOZ_CRASH("asm.js only uses mapped buffers when using signal-handler OOB checking");
}
#endif

#ifdef NIGHTLY_BUILD
# if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
static bool
TransferAsmJSMappedBuffer(JSContext* cx, CallArgs args, Handle<ArrayBufferObject*> oldBuffer,
                          size_t newByteLength)
{
    size_t oldByteLength = oldBuffer->byteLength();
    MOZ_ASSERT(oldByteLength % AsmJSPageSize == 0);
    MOZ_ASSERT(newByteLength % AsmJSPageSize == 0);

    ArrayBufferObject::BufferContents stolen =
        ArrayBufferObject::stealContents(cx, oldBuffer,  true);
    if (!stolen)
        return false;

    MOZ_ASSERT(stolen.kind() == ArrayBufferObject::ASMJS_MAPPED);
    uint8_t* data = stolen.data();

    if (newByteLength > oldByteLength) {
        void* diffStart = data + oldByteLength;
        size_t diffLength = newByteLength - oldByteLength;
#  ifdef XP_WIN
        if (!VirtualAlloc(diffStart, diffLength, MEM_COMMIT, PAGE_READWRITE)) {
            ReleaseAsmJSMappedData(data);
            ReportOutOfMemory(cx);
            return false;
        }
#  else
        
        
        int flags = MAP_FIXED | MAP_PRIVATE | MAP_ANON;
        if (mmap(diffStart, diffLength, PROT_READ | PROT_WRITE, flags, -1, 0) == MAP_FAILED) {
            ReleaseAsmJSMappedData(data);
            ReportOutOfMemory(cx);
            return false;
        }
#  endif
    } else if (newByteLength < oldByteLength) {
        void* diffStart = data + newByteLength;
        size_t diffLength = oldByteLength - newByteLength;
#  ifdef XP_WIN
        if (!VirtualFree(diffStart, diffLength, MEM_DECOMMIT)) {
            ReleaseAsmJSMappedData(data);
            ReportOutOfMemory(cx);
            return false;
        }
#  else
        if (madvise(diffStart, diffLength, MADV_DONTNEED) ||
            mprotect(diffStart, diffLength, PROT_NONE))
        {
            ReleaseAsmJSMappedData(data);
            ReportOutOfMemory(cx);
            return false;
        }
#  endif
    }

    ArrayBufferObject::BufferContents newContents =
        ArrayBufferObject::BufferContents::create<ArrayBufferObject::ASMJS_MAPPED>(data);

    RootedObject newBuffer(cx, ArrayBufferObject::create(cx, newByteLength, newContents));
    if (!newBuffer) {
        ReleaseAsmJSMappedData(data);
        return false;
    }

    args.rval().setObject(*newBuffer);
    return true;
}
# endif  






bool
ArrayBufferObject::fun_transfer(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    HandleValue oldBufferArg = args.get(0);
    HandleValue newByteLengthArg = args.get(1);

    if (!oldBufferArg.isObject()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return false;
    }

    RootedObject oldBufferObj(cx, &oldBufferArg.toObject());
    if (!ObjectClassIs(oldBufferObj, ESClass_ArrayBuffer, cx)) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return false;
    }

    
    
    Rooted<ArrayBufferObject*> oldBuffer(cx);
    if (oldBufferObj->is<ArrayBufferObject>()) {
        oldBuffer = &oldBufferObj->as<ArrayBufferObject>();
    } else {
        JSObject* unwrapped = CheckedUnwrap(oldBufferObj);
        if (!unwrapped || !unwrapped->is<ArrayBufferObject>()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }
        oldBuffer = &unwrapped->as<ArrayBufferObject>();
    }

    if (oldBuffer->isNeutered()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_DETACHED);
        return false;
    }

    size_t oldByteLength = oldBuffer->byteLength();
    size_t newByteLength;
    if (newByteLengthArg.isUndefined()) {
        newByteLength = oldByteLength;
    } else {
        int32_t i32;
        if (!ToInt32(cx, newByteLengthArg, &i32))
            return false;
        if (i32 < 0) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
            return false;
        }
        newByteLength = size_t(i32);
    }

    UniquePtr<uint8_t, JS::FreePolicy> newData;
    if (!newByteLength) {
        if (!ArrayBufferObject::neuter(cx, oldBuffer, oldBuffer->contents()))
            return false;
    } else {
# if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
        
        
        if (oldBuffer->isAsmJSMapped() && (newByteLength % AsmJSPageSize) == 0)
            return TransferAsmJSMappedBuffer(cx, args, oldBuffer, newByteLength);
# endif

        
        
        
        bool steal = oldBuffer->hasMallocedContents();
        auto stolenContents = ArrayBufferObject::stealContents(cx, oldBuffer, steal);
        if (!stolenContents)
            return false;

        UniquePtr<uint8_t, JS::FreePolicy> oldData(stolenContents.data());
        if (newByteLength > oldByteLength) {
            
            
            
            
            newData.reset(cx->runtime()->pod_callocCanGC<uint8_t>(newByteLength));
            if (newData) {
                memcpy(newData.get(), oldData.get(), oldByteLength);
            } else {
                
                
                newData.reset(cx->pod_realloc(oldData.get(), oldByteLength, newByteLength));
                if (!newData)
                    return false;
                oldData.release();
                memset(newData.get() + oldByteLength, 0, newByteLength - oldByteLength);
            }
        } else if (newByteLength < oldByteLength) {
            newData.reset(cx->pod_realloc(oldData.get(), oldByteLength, newByteLength));
            if (!newData)
                return false;
            oldData.release();
        } else {
            newData = Move(oldData);
        }
    }

    RootedObject newBuffer(cx, JS_NewArrayBufferWithContents(cx, newByteLength, newData.get()));
    if (!newBuffer)
        return false;
    newData.release();

    args.rval().setObject(*newBuffer);
    return true;
}
#endif  




bool
ArrayBufferObject::class_constructor(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!WarnIfNotConstructing(cx, args, "ArrayBuffer"))
        return false;

    int32_t nbytes = 0;
    if (argc > 0 && !ToInt32(cx, args[0], &nbytes))
        return false;

    if (nbytes < 0) {
        




        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
        return false;
    }

    JSObject* bufobj = create(cx, uint32_t(nbytes));
    if (!bufobj)
        return false;
    args.rval().setObject(*bufobj);
    return true;
}

static ArrayBufferObject::BufferContents
AllocateArrayBufferContents(JSContext* cx, uint32_t nbytes)
{
    uint8_t* p = cx->runtime()->pod_callocCanGC<uint8_t>(nbytes);
    if (!p)
        ReportOutOfMemory(cx);

    return ArrayBufferObject::BufferContents::create<ArrayBufferObject::PLAIN>(p);
}

void
ArrayBufferObject::neuterView(JSContext* cx, ArrayBufferViewObject* view,
                              BufferContents newContents)
{
    view->neuter(newContents.data());

    
    MarkObjectStateChange(cx, view);
}

 bool
ArrayBufferObject::neuter(JSContext* cx, Handle<ArrayBufferObject*> buffer,
                          BufferContents newContents)
{
    if (buffer->isAsmJS() && !OnDetachAsmJSArrayBuffer(cx, buffer))
        return false;

    
    
    
    MOZ_ASSERT_IF(buffer->forInlineTypedObject(),
                  newContents.data() == buffer->dataPointer());

    
    
    
    
    if (buffer->hasTypedObjectViews()) {
        
        
        if (!cx->global()->getGroup(cx))
            CrashAtUnhandlableOOM("ArrayBufferObject::neuter");
        MarkObjectGroupFlags(cx, cx->global(), OBJECT_FLAG_TYPED_OBJECT_NEUTERED);
        cx->compartment()->neuteredTypedObjects = 1;
    }

    
    

    if (InnerViewTable::ViewVector* views = cx->compartment()->innerViews.maybeViewsUnbarriered(buffer)) {
        for (size_t i = 0; i < views->length(); i++)
            buffer->neuterView(cx, (*views)[i], newContents);
        cx->compartment()->innerViews.removeViews(buffer);
    }
    if (buffer->firstView()) {
        if (buffer->forInlineTypedObject()) {
            
            
            MOZ_ASSERT(buffer->firstView()->is<InlineTransparentTypedObject>());
        } else {
            buffer->neuterView(cx, buffer->firstView(), newContents);
            buffer->setFirstView(nullptr);
        }
    }

    if (newContents.data() != buffer->dataPointer())
        buffer->setNewOwnedData(cx->runtime()->defaultFreeOp(), newContents);

    buffer->setByteLength(0);
    buffer->setIsNeutered();
    return true;
}

void
ArrayBufferObject::setNewOwnedData(FreeOp* fop, BufferContents newContents)
{
    if (ownsData()) {
        MOZ_ASSERT(newContents.data() != dataPointer());
        releaseData(fop);
    }

    setDataPointer(newContents, OwnsData);
}

void
ArrayBufferObject::changeViewContents(JSContext* cx, ArrayBufferViewObject* view,
                                      uint8_t* oldDataPointer, BufferContents newContents)
{
    
    
    
    uint8_t* viewDataPointer = view->dataPointer();
    if (viewDataPointer) {
        MOZ_ASSERT(newContents);
        ptrdiff_t offset = viewDataPointer - oldDataPointer;
        viewDataPointer = static_cast<uint8_t*>(newContents.data()) + offset;
        view->setDataPointer(viewDataPointer);
    }

    
    MarkObjectStateChange(cx, view);
}

void
ArrayBufferObject::changeContents(JSContext* cx, BufferContents newContents)
{
    MOZ_ASSERT(!forInlineTypedObject());

    
    uint8_t* oldDataPointer = dataPointer();
    setNewOwnedData(cx->runtime()->defaultFreeOp(), newContents);

    
    if (InnerViewTable::ViewVector* views = cx->compartment()->innerViews.maybeViewsUnbarriered(this)) {
        for (size_t i = 0; i < views->length(); i++)
            changeViewContents(cx, (*views)[i], oldDataPointer, newContents);
    }
    if (firstView())
        changeViewContents(cx, firstView(), oldDataPointer, newContents);
}

 bool
ArrayBufferObject::prepareForAsmJSNoSignals(JSContext* cx, Handle<ArrayBufferObject*> buffer)
{
    if (buffer->forInlineTypedObject()) {
        JS_ReportError(cx, "ArrayBuffer can't be used by asm.js");
        return false;
    }

    if (!buffer->ownsData()) {
        BufferContents contents = AllocateArrayBufferContents(cx, buffer->byteLength());
        if (!contents)
            return false;
        memcpy(contents.data(), buffer->dataPointer(), buffer->byteLength());
        buffer->changeContents(cx, contents);
    }

    buffer->setIsAsmJSMalloced();
    return true;
}

#if defined(ASMJS_MAY_USE_SIGNAL_HANDLERS_FOR_OOB)
 bool
ArrayBufferObject::prepareForAsmJS(JSContext* cx, Handle<ArrayBufferObject*> buffer,
                                   bool usesSignalHandlers)
{
    
    if (!usesSignalHandlers)
        return prepareForAsmJSNoSignals(cx, buffer);

    if (buffer->isAsmJSMapped())
        return true;

    
    if (buffer->isAsmJSMalloced()) {
        JS_ReportError(cx, "can't access same buffer with and without signals enabled");
        return false;
    }

    if (buffer->forInlineTypedObject()) {
        JS_ReportError(cx, "ArrayBuffer can't be used by asm.js");
        return false;
    }

    
    void* data;
# ifdef XP_WIN
    data = VirtualAlloc(nullptr, AsmJSMappedSize, MEM_RESERVE, PAGE_NOACCESS);
    if (!data)
        return false;
# else
    data = MozTaggedAnonymousMmap(nullptr, AsmJSMappedSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0, "asm-js-reserved");
    if (data == MAP_FAILED)
        return false;
# endif

    
    MOZ_ASSERT(buffer->byteLength() % AsmJSPageSize == 0);
# ifdef XP_WIN
    if (!VirtualAlloc(data, buffer->byteLength(), MEM_COMMIT, PAGE_READWRITE)) {
        VirtualFree(data, 0, MEM_RELEASE);
        return false;
    }
# else
    size_t validLength = buffer->byteLength();
    if (mprotect(data, validLength, PROT_READ | PROT_WRITE)) {
        munmap(data, AsmJSMappedSize);
        return false;
    }
#   if defined(MOZ_VALGRIND) && defined(VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE)
    
    VALGRIND_DISABLE_ADDR_ERROR_REPORTING_IN_RANGE((unsigned char*)data + validLength,
                                                   AsmJSMappedSize-validLength);
#   endif
# endif

    
    memcpy(data, buffer->dataPointer(), buffer->byteLength());

    
    
    BufferContents newContents = BufferContents::create<ASMJS_MAPPED>(data);
    buffer->changeContents(cx, newContents);
    MOZ_ASSERT(data == buffer->dataPointer());

    return true;
}
#else 
bool
ArrayBufferObject::prepareForAsmJS(JSContext* cx, Handle<ArrayBufferObject*> buffer,
                                   bool usesSignalHandlers)
{
    
    MOZ_ASSERT(!usesSignalHandlers);
    return prepareForAsmJSNoSignals(cx, buffer);
}
#endif

ArrayBufferObject::BufferContents
ArrayBufferObject::createMappedContents(int fd, size_t offset, size_t length)
{
    void* data = AllocateMappedContent(fd, offset, length, ARRAY_BUFFER_ALIGNMENT);
    return BufferContents::create<MAPPED>(data);
}

uint8_t*
ArrayBufferObject::inlineDataPointer() const
{
    return static_cast<uint8_t*>(fixedData(JSCLASS_RESERVED_SLOTS(&class_)));
}

uint8_t*
ArrayBufferObject::dataPointer() const
{
    return static_cast<uint8_t*>(getSlot(DATA_SLOT).toPrivate());
}

void
ArrayBufferObject::releaseData(FreeOp* fop)
{
    MOZ_ASSERT(ownsData());

    switch (bufferKind()) {
      case PLAIN:
      case ASMJS_MALLOCED:
        fop->free_(dataPointer());
        break;
      case MAPPED:
        DeallocateMappedContent(dataPointer(), byteLength());
        break;
      case ASMJS_MAPPED:
        ReleaseAsmJSMappedData(dataPointer());
        break;
    }
}

void
ArrayBufferObject::setDataPointer(BufferContents contents, OwnsState ownsData)
{
    setSlot(DATA_SLOT, PrivateValue(contents.data()));
    setOwnsData(ownsData);
    setFlags((flags() & ~KIND_MASK) | contents.kind());
}

size_t
ArrayBufferObject::byteLength() const
{
    return size_t(getSlot(BYTE_LENGTH_SLOT).toDouble());
}

void
ArrayBufferObject::setByteLength(size_t length)
{
    setSlot(BYTE_LENGTH_SLOT, DoubleValue(length));
}

uint32_t
ArrayBufferObject::flags() const
{
    return uint32_t(getSlot(FLAGS_SLOT).toInt32());
}

void
ArrayBufferObject::setFlags(uint32_t flags)
{
    setSlot(FLAGS_SLOT, Int32Value(flags));
}

ArrayBufferObject*
ArrayBufferObject::create(JSContext* cx, uint32_t nbytes, BufferContents contents,
                          OwnsState ownsState ,
                          NewObjectKind newKind )
{
    MOZ_ASSERT_IF(contents.kind() == MAPPED, contents);

    
    
    
    
    size_t reservedSlots = JSCLASS_RESERVED_SLOTS(&class_);

    size_t nslots = reservedSlots;
    bool allocated = false;
    if (contents) {
        if (ownsState == OwnsData) {
            
            size_t nAllocated = nbytes;
            if (contents.kind() == MAPPED)
                nAllocated = JS_ROUNDUP(nbytes, js::gc::SystemPageSize());
            cx->zone()->updateMallocCounter(nAllocated);
        }
    } else {
        MOZ_ASSERT(ownsState == OwnsData);
        size_t usableSlots = NativeObject::MAX_FIXED_SLOTS - reservedSlots;
        if (nbytes <= usableSlots * sizeof(Value)) {
            int newSlots = (nbytes - 1) / sizeof(Value) + 1;
            MOZ_ASSERT(int(nbytes) <= newSlots * int(sizeof(Value)));
            nslots = reservedSlots + newSlots;
            contents = BufferContents::createPlain(nullptr);
        } else {
            contents = AllocateArrayBufferContents(cx, nbytes);
            if (!contents)
                return nullptr;
            allocated = true;
        }
    }

    MOZ_ASSERT(!(class_.flags & JSCLASS_HAS_PRIVATE));
    gc::AllocKind allocKind = GetGCObjectKind(nslots);

    Rooted<ArrayBufferObject*> obj(cx, NewBuiltinClassInstance<ArrayBufferObject>(cx, allocKind, newKind));
    if (!obj) {
        if (allocated)
            js_free(contents.data());
        return nullptr;
    }

    MOZ_ASSERT(obj->getClass() == &class_);
    MOZ_ASSERT(!gc::IsInsideNursery(obj));

    if (!contents) {
        void* data = obj->inlineDataPointer();
        memset(data, 0, nbytes);
        obj->initialize(nbytes, BufferContents::createPlain(data), DoesntOwnData);
    } else {
        obj->initialize(nbytes, contents, ownsState);
    }

    return obj;
}

ArrayBufferObject*
ArrayBufferObject::create(JSContext* cx, uint32_t nbytes,
                          NewObjectKind newKind )
{
    return create(cx, nbytes, BufferContents::createPlain(nullptr));
}

JSObject*
ArrayBufferObject::createSlice(JSContext* cx, Handle<ArrayBufferObject*> arrayBuffer,
                               uint32_t begin, uint32_t end)
{
    uint32_t bufLength = arrayBuffer->byteLength();
    if (begin > bufLength || end > bufLength || begin > end) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPE_ERR_BAD_ARGS);
        return nullptr;
    }

    uint32_t length = end - begin;

    if (!arrayBuffer->hasData())
        return create(cx, 0);

    ArrayBufferObject* slice = create(cx, length);
    if (!slice)
        return nullptr;
    memcpy(slice->dataPointer(), arrayBuffer->dataPointer() + begin, length);
    return slice;
}

bool
ArrayBufferObject::createDataViewForThisImpl(JSContext* cx, CallArgs args)
{
    MOZ_ASSERT(IsArrayBuffer(args.thisv()));

    




    MOZ_ASSERT(args.length() >= 2);

    Rooted<JSObject*> proto(cx, &args[args.length() - 1].toObject());

    Rooted<JSObject*> buffer(cx, &args.thisv().toObject());

    



    CallArgs frobbedArgs = CallArgsFromVp(args.length() - 1, args.base());
    return DataViewObject::construct(cx, buffer, frobbedArgs, proto);
}

bool
ArrayBufferObject::createDataViewForThis(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, createDataViewForThisImpl>(cx, args);
}

 ArrayBufferObject::BufferContents
ArrayBufferObject::stealContents(JSContext* cx, Handle<ArrayBufferObject*> buffer,
                                 bool hasStealableContents)
{
    MOZ_ASSERT_IF(hasStealableContents, buffer->hasStealableContents());

    BufferContents oldContents(buffer->dataPointer(), buffer->bufferKind());
    BufferContents newContents = AllocateArrayBufferContents(cx, buffer->byteLength());
    if (!newContents)
        return BufferContents::createPlain(nullptr);

    if (hasStealableContents) {
        
        
        
        buffer->setOwnsData(DoesntOwnData);
        if (!ArrayBufferObject::neuter(cx, buffer, newContents)) {
            js_free(newContents.data());
            return BufferContents::createPlain(nullptr);
        }
        return oldContents;
    }

    
    
    memcpy(newContents.data(), oldContents.data(), buffer->byteLength());
    if (!ArrayBufferObject::neuter(cx, buffer, oldContents)) {
        js_free(newContents.data());
        return BufferContents::createPlain(nullptr);
    }
    return newContents;
}

 void
ArrayBufferObject::addSizeOfExcludingThis(JSObject* obj, mozilla::MallocSizeOf mallocSizeOf,
                                          JS::ClassInfo* info)
{
    ArrayBufferObject& buffer = AsArrayBuffer(obj);

    if (!buffer.ownsData())
        return;

    switch (buffer.bufferKind()) {
      case PLAIN:
        info->objectsMallocHeapElementsNonAsmJS += mallocSizeOf(buffer.dataPointer());
        break;
      case MAPPED:
        info->objectsNonHeapElementsMapped += buffer.byteLength();
        break;
      case ASMJS_MALLOCED:
        info->objectsMallocHeapElementsAsmJS += mallocSizeOf(buffer.dataPointer());
        break;
      case ASMJS_MAPPED:
        info->objectsNonHeapElementsAsmJS += buffer.byteLength();
        break;
    }
}

 void
ArrayBufferObject::finalize(FreeOp* fop, JSObject* obj)
{
    ArrayBufferObject& buffer = obj->as<ArrayBufferObject>();

    if (buffer.ownsData())
        buffer.releaseData(fop);
}

 void
ArrayBufferObject::trace(JSTracer* trc, JSObject* obj)
{
    
    
    ArrayBufferObject& buf = obj->as<ArrayBufferObject>();

    if (!buf.forInlineTypedObject())
        return;

    JSObject* view = MaybeForwarded(buf.firstView());
    MOZ_ASSERT(view && view->is<InlineTransparentTypedObject>());

    TraceManuallyBarrieredEdge(trc, &view, "array buffer inline typed object owner");
    buf.setSlot(DATA_SLOT, PrivateValue(view->as<InlineTransparentTypedObject>().inlineTypedMem()));
}

 void
ArrayBufferObject::objectMoved(JSObject* obj, const JSObject* old)
{
    ArrayBufferObject& dst = obj->as<ArrayBufferObject>();
    const ArrayBufferObject& src = old->as<ArrayBufferObject>();

    
    if (src.hasInlineData())
        dst.setSlot(DATA_SLOT, PrivateValue(dst.inlineDataPointer()));
}

ArrayBufferViewObject*
ArrayBufferObject::firstView()
{
    return getSlot(FIRST_VIEW_SLOT).isObject()
        ? static_cast<ArrayBufferViewObject*>(&getSlot(FIRST_VIEW_SLOT).toObject())
        : nullptr;
}

void
ArrayBufferObject::setFirstView(ArrayBufferViewObject* view)
{
    setSlot(FIRST_VIEW_SLOT, ObjectOrNullValue(view));
}

bool
ArrayBufferObject::addView(JSContext* cx, JSObject* viewArg)
{
    
    
    
    MOZ_ASSERT(viewArg->is<ArrayBufferViewObject>() || viewArg->is<TypedObject>());
    ArrayBufferViewObject* view = static_cast<ArrayBufferViewObject*>(viewArg);

    if (!firstView()) {
        setFirstView(view);
        return true;
    }
    return cx->compartment()->innerViews.addView(cx, this, view);
}





static size_t VIEW_LIST_MAX_LENGTH = 500;

bool
InnerViewTable::addView(JSContext* cx, ArrayBufferObject* obj, ArrayBufferViewObject* view)
{
    
    MOZ_ASSERT(obj->firstView());

    if (!map.initialized() && !map.init())
        return false;

    Map::AddPtr p = map.lookupForAdd(obj);

    MOZ_ASSERT(!gc::IsInsideNursery(obj));
    bool addToNursery = nurseryKeysValid && gc::IsInsideNursery(view);

    if (p) {
        ViewVector& views = p->value();
        MOZ_ASSERT(!views.empty());

        if (addToNursery) {
            
            if (views.length() >= VIEW_LIST_MAX_LENGTH) {
                
                
                nurseryKeysValid = false;
            } else {
                for (size_t i = 0; i < views.length(); i++) {
                    if (gc::IsInsideNursery(views[i]))
                        addToNursery = false;
                }
            }
        }

        if (!views.append(view))
            return false;
    } else {
        if (!map.add(p, obj, ViewVector()))
            return false;
        JS_ALWAYS_TRUE(p->value().append(view));
    }

    if (addToNursery && !nurseryKeys.append(obj))
        nurseryKeysValid = false;

    return true;
}

InnerViewTable::ViewVector*
InnerViewTable::maybeViewsUnbarriered(ArrayBufferObject* obj)
{
    if (!map.initialized())
        return nullptr;

    Map::Ptr p = map.lookup(obj);
    if (p)
        return &p->value();
    return nullptr;
}

void
InnerViewTable::removeViews(ArrayBufferObject* obj)
{
    Map::Ptr p = map.lookup(obj);
    MOZ_ASSERT(p);

    map.remove(p);
}

bool
InnerViewTable::sweepEntry(JSObject** pkey, ViewVector& views)
{
    if (IsAboutToBeFinalizedUnbarriered(pkey))
        return true;

    MOZ_ASSERT(!views.empty());
    for (size_t i = 0; i < views.length(); i++) {
        if (IsAboutToBeFinalizedUnbarriered(&views[i])) {
            views[i--] = views.back();
            views.popBack();
        }
    }

    return views.empty();
}

void
InnerViewTable::sweep(JSRuntime* rt)
{
    MOZ_ASSERT(nurseryKeys.empty());

    if (!map.initialized())
        return;

    for (Map::Enum e(map); !e.empty(); e.popFront()) {
        JSObject* key = e.front().key();
        if (sweepEntry(&key, e.front().value()))
            e.removeFront();
        else if (key != e.front().key())
            e.rekeyFront(key);
    }
}

void
InnerViewTable::sweepAfterMinorGC(JSRuntime* rt)
{
    MOZ_ASSERT(!nurseryKeys.empty());

    if (nurseryKeysValid) {
        for (size_t i = 0; i < nurseryKeys.length(); i++) {
            JSObject* key = nurseryKeys[i];
            Map::Ptr p = map.lookup(key);
            if (!p)
                continue;

            if (sweepEntry(&key, p->value()))
                map.remove(nurseryKeys[i]);
            else
                map.rekeyIfMoved(nurseryKeys[i], key);
        }
        nurseryKeys.clear();
    } else {
        
        nurseryKeys.clear();
        sweep(rt);

        nurseryKeysValid = true;
    }
}

size_t
InnerViewTable::sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    if (!map.initialized())
        return 0;

    size_t vectorSize = 0;
    for (Map::Enum e(map); !e.empty(); e.popFront())
        vectorSize += e.front().value().sizeOfExcludingThis(mallocSizeOf);

    return vectorSize
         + map.sizeOfExcludingThis(mallocSizeOf)
         + nurseryKeys.sizeOfExcludingThis(mallocSizeOf);
}










 void
ArrayBufferViewObject::trace(JSTracer* trc, JSObject* objArg)
{
    NativeObject* obj = &objArg->as<NativeObject>();
    HeapSlot& bufSlot = obj->getReservedSlotRef(TypedArrayLayout::BUFFER_SLOT);
    TraceEdge(trc, &bufSlot, "typedarray.buffer");

    
    if (bufSlot.isObject()) {
        ArrayBufferObject& buf = AsArrayBuffer(MaybeForwarded(&bufSlot.toObject()));
        int32_t offset = obj->getReservedSlot(TypedArrayLayout::BYTEOFFSET_SLOT).toInt32();
        MOZ_ASSERT(buf.dataPointer() != nullptr);

        if (buf.forInlineTypedObject()) {
            
            
            JSObject* view = buf.firstView();

            
            TraceManuallyBarrieredEdge(trc, &view, "typed array nursery owner");
            MOZ_ASSERT(view->is<InlineTypedObject>() && view != obj);

            void* srcData = obj->getPrivate();
            void* dstData = view->as<InlineTypedObject>().inlineTypedMem() + offset;
            obj->setPrivateUnbarriered(dstData);

            
            
            
            trc->runtime()->gc.nursery.maybeSetForwardingPointer(trc, srcData, dstData,
                                                                  false);
        } else {
            
            
            
           obj->initPrivate(buf.dataPointer() + offset);
        }
    }
}

template <>
bool
JSObject::is<js::ArrayBufferViewObject>() const
{
    return is<DataViewObject>() || is<TypedArrayObject>();
}

void
ArrayBufferViewObject::neuter(void* newData)
{
    MOZ_ASSERT(newData != nullptr);
    if (is<DataViewObject>())
        as<DataViewObject>().neuter(newData);
    else if (is<TypedArrayObject>())
        as<TypedArrayObject>().neuter(newData);
    else
        as<OutlineTypedObject>().neuter(newData);
}

uint8_t*
ArrayBufferViewObject::dataPointer()
{
    if (is<DataViewObject>())
        return static_cast<uint8_t*>(as<DataViewObject>().dataPointer());
    if (is<TypedArrayObject>())
        return static_cast<uint8_t*>(as<TypedArrayObject>().viewData());
    return as<TypedObject>().typedMem();
}

void
ArrayBufferViewObject::setDataPointer(uint8_t* data)
{
    if (is<DataViewObject>())
        as<DataViewObject>().setPrivate(data);
    else if (is<TypedArrayObject>())
        as<TypedArrayObject>().setPrivate(data);
    else if (is<OutlineTypedObject>())
        as<OutlineTypedObject>().setData(data);
    else
        MOZ_CRASH();
}

 ArrayBufferObject*
ArrayBufferViewObject::bufferObject(JSContext* cx, Handle<ArrayBufferViewObject*> thisObject)
{
    if (thisObject->is<TypedArrayObject>()) {
        Rooted<TypedArrayObject*> typedArray(cx, &thisObject->as<TypedArrayObject>());
        if (!TypedArrayObject::ensureHasBuffer(cx, typedArray))
            return nullptr;
        return thisObject->as<TypedArrayObject>().buffer();
    }
    MOZ_ASSERT(thisObject->is<DataViewObject>());
    return &thisObject->as<DataViewObject>().arrayBuffer();
}



JS_FRIEND_API(bool)
JS_IsArrayBufferViewObject(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    return obj && obj->is<ArrayBufferViewObject>();
}

JS_FRIEND_API(JSObject*)
js::UnwrapArrayBufferView(JSObject* obj)
{
    if (JSObject* unwrapped = CheckedUnwrap(obj))
        return unwrapped->is<ArrayBufferViewObject>() ? unwrapped : nullptr;
    return nullptr;
}

JS_FRIEND_API(uint32_t)
JS_GetArrayBufferByteLength(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? AsArrayBuffer(obj).byteLength() : 0;
}

JS_FRIEND_API(uint8_t*)
JS_GetArrayBufferData(JSObject* obj, const JS::AutoCheckCannotGC&)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    return AsArrayBuffer(obj).dataPointer();
}

JS_FRIEND_API(bool)
JS_NeuterArrayBuffer(JSContext* cx, HandleObject obj,
                     NeuterDataDisposition changeData)
{
    if (!obj->is<ArrayBufferObject>()) {
        JS_ReportError(cx, "ArrayBuffer object required");
        return false;
    }

    Rooted<ArrayBufferObject*> buffer(cx, &obj->as<ArrayBufferObject>());

    if (changeData == ChangeData && buffer->hasStealableContents()) {
        ArrayBufferObject::BufferContents newContents =
            AllocateArrayBufferContents(cx, buffer->byteLength());
        if (!newContents)
            return false;
        if (!ArrayBufferObject::neuter(cx, buffer, newContents)) {
            js_free(newContents.data());
            return false;
        }
    } else {
        if (!ArrayBufferObject::neuter(cx, buffer, buffer->contents()))
            return false;
    }

    return true;
}

JS_FRIEND_API(bool)
JS_IsNeuteredArrayBufferObject(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return false;

    return obj->is<ArrayBufferObject>() && obj->as<ArrayBufferObject>().isNeutered();
}

JS_FRIEND_API(JSObject*)
JS_NewArrayBuffer(JSContext* cx, uint32_t nbytes)
{
    MOZ_ASSERT(nbytes <= INT32_MAX);
    return ArrayBufferObject::create(cx, nbytes);
}

JS_PUBLIC_API(JSObject*)
JS_NewArrayBufferWithContents(JSContext* cx, size_t nbytes, void* data)
{
    MOZ_ASSERT_IF(!data, nbytes == 0);
    ArrayBufferObject::BufferContents contents =
        ArrayBufferObject::BufferContents::create<ArrayBufferObject::PLAIN>(data);
    return ArrayBufferObject::create(cx, nbytes, contents, ArrayBufferObject::OwnsData, TenuredObject);
}

JS_FRIEND_API(bool)
JS_IsArrayBufferObject(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    return obj && obj->is<ArrayBufferObject>();
}

JS_FRIEND_API(bool)
JS_ArrayBufferHasData(JSObject* obj)
{
    return CheckedUnwrap(obj)->as<ArrayBufferObject>().hasData();
}

JS_FRIEND_API(JSObject*)
js::UnwrapArrayBuffer(JSObject* obj)
{
    if (JSObject* unwrapped = CheckedUnwrap(obj))
        return unwrapped->is<ArrayBufferObject>() ? unwrapped : nullptr;
    return nullptr;
}

JS_PUBLIC_API(void*)
JS_StealArrayBufferContents(JSContext* cx, HandleObject objArg)
{
    JSObject* obj = CheckedUnwrap(objArg);
    if (!obj)
        return nullptr;

    if (!obj->is<ArrayBufferObject>()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return nullptr;
    }

    Rooted<ArrayBufferObject*> buffer(cx, &obj->as<ArrayBufferObject>());
    if (buffer->isNeutered()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_DETACHED);
        return nullptr;
    }

    
    
    
    
    bool hasStealableContents = buffer->hasStealableContents() && buffer->hasMallocedContents();

    return ArrayBufferObject::stealContents(cx, buffer, hasStealableContents).data();
}

JS_PUBLIC_API(JSObject*)
JS_NewMappedArrayBufferWithContents(JSContext* cx, size_t nbytes, void* data)
{
    MOZ_ASSERT(data);
    ArrayBufferObject::BufferContents contents =
        ArrayBufferObject::BufferContents::create<ArrayBufferObject::MAPPED>(data);
    return ArrayBufferObject::create(cx, nbytes, contents, ArrayBufferObject::OwnsData, TenuredObject);
}

JS_PUBLIC_API(void*)
JS_CreateMappedArrayBufferContents(int fd, size_t offset, size_t length)
{
    return ArrayBufferObject::createMappedContents(fd, offset, length).data();
}

JS_PUBLIC_API(void)
JS_ReleaseMappedArrayBufferContents(void* contents, size_t length)
{
    DeallocateMappedContent(contents, length);
}

JS_FRIEND_API(bool)
JS_IsMappedArrayBufferObject(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return false;

    return obj->is<ArrayBufferObject>() && obj->as<ArrayBufferObject>().isMapped();
}

JS_FRIEND_API(void*)
JS_GetArrayBufferViewData(JSObject* obj, const JS::AutoCheckCannotGC&)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    return obj->is<DataViewObject>() ? obj->as<DataViewObject>().dataPointer()
                                     : obj->as<TypedArrayObject>().viewData();
}

JS_FRIEND_API(JSObject*)
JS_GetArrayBufferViewBuffer(JSContext* cx, HandleObject objArg)
{
    JSObject* obj = CheckedUnwrap(objArg);
    if (!obj)
        return nullptr;
    MOZ_ASSERT(obj->is<ArrayBufferViewObject>());

    Rooted<ArrayBufferViewObject*> viewObject(cx, static_cast<ArrayBufferViewObject*>(obj));
    return ArrayBufferViewObject::bufferObject(cx, viewObject);
}

JS_FRIEND_API(uint32_t)
JS_GetArrayBufferViewByteLength(JSObject* obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->is<DataViewObject>()
           ? obj->as<DataViewObject>().byteLength()
           : obj->as<TypedArrayObject>().byteLength();
}

JS_FRIEND_API(JSObject*)
JS_GetObjectAsArrayBufferView(JSObject* obj, uint32_t* length, uint8_t** data)
{
    if (!(obj = CheckedUnwrap(obj)))
        return nullptr;
    if (!(obj->is<ArrayBufferViewObject>()))
        return nullptr;

    *length = obj->is<DataViewObject>()
              ? obj->as<DataViewObject>().byteLength()
              : obj->as<TypedArrayObject>().byteLength();

    *data = static_cast<uint8_t*>(obj->is<DataViewObject>()
                                  ? obj->as<DataViewObject>().dataPointer()
                                  : obj->as<TypedArrayObject>().viewData());
    return obj;
}

JS_FRIEND_API(void)
js::GetArrayBufferViewLengthAndData(JSObject* obj, uint32_t* length, uint8_t** data)
{
    MOZ_ASSERT(obj->is<ArrayBufferViewObject>());

    *length = obj->is<DataViewObject>()
              ? obj->as<DataViewObject>().byteLength()
              : obj->as<TypedArrayObject>().byteLength();

    *data = static_cast<uint8_t*>(obj->is<DataViewObject>()
                                  ? obj->as<DataViewObject>().dataPointer()
                                  : obj->as<TypedArrayObject>().viewData());
}

JS_FRIEND_API(JSObject*)
JS_GetObjectAsArrayBuffer(JSObject* obj, uint32_t* length, uint8_t** data)
{
    if (!(obj = CheckedUnwrap(obj)))
        return nullptr;
    if (!IsArrayBuffer(obj))
        return nullptr;

    *length = AsArrayBuffer(obj).byteLength();
    *data = AsArrayBuffer(obj).dataPointer();

    return obj;
}

JS_FRIEND_API(void)
js::GetArrayBufferLengthAndData(JSObject* obj, uint32_t* length, uint8_t** data)
{
    MOZ_ASSERT(IsArrayBuffer(obj));
    *length = AsArrayBuffer(obj).byteLength();
    *data = AsArrayBuffer(obj).dataPointer();
}

JSObject*
js::InitArrayBufferClass(JSContext* cx, HandleObject obj)
{
    Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
    if (global->isStandardClassResolved(JSProto_ArrayBuffer))
        return &global->getPrototype(JSProto_ArrayBuffer).toObject();

    RootedNativeObject arrayBufferProto(cx, global->createBlankPrototype(cx, &ArrayBufferObject::protoClass));
    if (!arrayBufferProto)
        return nullptr;

    RootedFunction ctor(cx, global->createConstructor(cx, ArrayBufferObject::class_constructor,
                                                      cx->names().ArrayBuffer, 1));
    if (!ctor)
        return nullptr;

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_ArrayBuffer,
                                              ctor, arrayBufferProto))
    {
        return nullptr;
    }

    if (!LinkConstructorAndPrototype(cx, ctor, arrayBufferProto))
        return nullptr;

    RootedId byteLengthId(cx, NameToId(cx->names().byteLength));
    unsigned attrs = JSPROP_SHARED | JSPROP_GETTER;
    JSObject* getter =
        NewNativeFunction(cx, ArrayBufferObject::byteLengthGetter, 0, js::NullPtr());
    if (!getter)
        return nullptr;

    if (!NativeDefineProperty(cx, arrayBufferProto, byteLengthId, UndefinedHandleValue,
                              JS_DATA_TO_FUNC_PTR(GetterOp, getter), nullptr, attrs))
        return nullptr;

    if (!JS_DefineFunctions(cx, ctor, ArrayBufferObject::jsstaticfuncs))
        return nullptr;

    if (!JS_DefineFunctions(cx, arrayBufferProto, ArrayBufferObject::jsfuncs))
        return nullptr;

    return arrayBufferProto;
}

