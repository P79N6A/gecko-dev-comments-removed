





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
#include "js/MemoryMetrics.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/SharedArrayObject.h"
#include "vm/WrapperObject.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using mozilla::DebugOnly;

using namespace js;
using namespace js::gc;
using namespace js::types;






bool
js::ToClampedIndex(JSContext *cx, HandleValue v, uint32_t length, uint32_t *out)
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
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

const Class ArrayBufferObject::class_ = {
    "ArrayBuffer",
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer) |
    JSCLASS_BACKGROUND_FINALIZE,
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    ArrayBufferObject::finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    ArrayBufferObject::obj_trace,
    JS_NULL_CLASS_SPEC,
    {
        nullptr,    
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
    JS_FS_END
};

bool
js::IsArrayBuffer(HandleValue v)
{
    return v.isObject() &&
           (v.toObject().is<ArrayBufferObject>() ||
            v.toObject().is<SharedArrayBufferObject>());
}

bool
js::IsArrayBuffer(HandleObject obj)
{
    return obj->is<ArrayBufferObject>() || obj->is<SharedArrayBufferObject>();
}

bool
js::IsArrayBuffer(JSObject *obj)
{
    return obj->is<ArrayBufferObject>() || obj->is<SharedArrayBufferObject>();
}

ArrayBufferObject &
js::AsArrayBuffer(HandleObject obj)
{
    JS_ASSERT(IsArrayBuffer(obj));
    if (obj->is<SharedArrayBufferObject>())
        return obj->as<SharedArrayBufferObject>();
    return obj->as<ArrayBufferObject>();
}

ArrayBufferObject &
js::AsArrayBuffer(JSObject *obj)
{
    JS_ASSERT(IsArrayBuffer(obj));
    if (obj->is<SharedArrayBufferObject>())
        return obj->as<SharedArrayBufferObject>();
    return obj->as<ArrayBufferObject>();
}

MOZ_ALWAYS_INLINE bool
ArrayBufferObject::byteLengthGetterImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsArrayBuffer(args.thisv()));
    args.rval().setInt32(args.thisv().toObject().as<ArrayBufferObject>().byteLength());
    return true;
}

bool
ArrayBufferObject::byteLengthGetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, byteLengthGetterImpl>(cx, args);
}

bool
ArrayBufferObject::fun_slice_impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsArrayBuffer(args.thisv()));

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

    JSObject *nobj = createSlice(cx, thisObj, begin, end);
    if (!nobj)
        return false;
    args.rval().setObject(*nobj);
    return true;
}

bool
ArrayBufferObject::fun_slice(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, fun_slice_impl>(cx, args);
}




bool
ArrayBufferObject::fun_isView(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setBoolean(args.get(0).isObject() &&
                           JS_IsArrayBufferViewObject(&args.get(0).toObject()));
    return true;
}




bool
ArrayBufferObject::class_constructor(JSContext *cx, unsigned argc, Value *vp)
{
    int32_t nbytes = 0;
    CallArgs args = CallArgsFromVp(argc, vp);
    if (argc > 0 && !ToInt32(cx, args[0], &nbytes))
        return false;

    if (nbytes < 0) {
        




        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
        return false;
    }

    JSObject *bufobj = create(cx, uint32_t(nbytes));
    if (!bufobj)
        return false;
    args.rval().setObject(*bufobj);
    return true;
}

static ArrayBufferObject::BufferContents
AllocateArrayBufferContents(JSContext *cx, uint32_t nbytes)
{
    uint8_t *p = cx->runtime()->pod_callocCanGC<uint8_t>(nbytes);
    if (!p)
        js_ReportOutOfMemory(cx);

    return ArrayBufferObject::BufferContents::create<ArrayBufferObject::PLAIN_BUFFER>(p);
}

ArrayBufferViewObject *
ArrayBufferObject::viewList() const
{
    return reinterpret_cast<ArrayBufferViewObject *>(getSlot(VIEW_LIST_SLOT).toPrivate());
}

void
ArrayBufferObject::setViewListNoBarrier(ArrayBufferViewObject *viewsHead)
{
    setSlot(VIEW_LIST_SLOT, PrivateValue(viewsHead));
}

void
ArrayBufferObject::setViewList(ArrayBufferViewObject *viewsHead)
{
    if (ArrayBufferViewObject *oldHead = viewList())
        ArrayBufferViewObject::writeBarrierPre(oldHead);
    setViewListNoBarrier(viewsHead);
    PostBarrierTypedArrayObject(this);
}

bool
ArrayBufferObject::canNeuter(JSContext *cx)
{
    if (isSharedArrayBuffer())
        return false;

    if (isAsmJSArrayBuffer()) {
        if (!ArrayBufferObject::canNeuterAsmJSArrayBuffer(cx, *this))
            return false;
    }

    return true;
}

 void
ArrayBufferObject::neuter(JSContext *cx, Handle<ArrayBufferObject*> buffer,
                          BufferContents newContents)
{
    JS_ASSERT(buffer->canNeuter(cx));

    
    

    for (ArrayBufferViewObject *view = buffer->viewList(); view; view = view->nextView()) {
        view->neuter(newContents.data());

        
        MarkObjectStateChange(cx, view);
    }

    if (newContents.data() != buffer->dataPointer())
        buffer->setNewOwnedData(cx->runtime()->defaultFreeOp(), newContents);

    buffer->setByteLength(0);
    buffer->setViewList(nullptr);
    buffer->setIsNeutered();

    
    
    if (buffer->inLiveList()) {
        ArrayBufferVector &gcLiveArrayBuffers = cx->compartment()->gcLiveArrayBuffers;
        DebugOnly<bool> found = false;
        for (size_t i = 0; i < gcLiveArrayBuffers.length(); i++) {
            if (buffer == gcLiveArrayBuffers[i]) {
                found = true;
                gcLiveArrayBuffers[i] = gcLiveArrayBuffers.back();
                gcLiveArrayBuffers.popBack();
                break;
            }
        }
        JS_ASSERT(found);
        buffer->setInLiveList(false);
    }
}

void
ArrayBufferObject::setNewOwnedData(FreeOp* fop, BufferContents newContents)
{
    JS_ASSERT(!isAsmJSArrayBuffer());
    JS_ASSERT(!isSharedArrayBuffer());

    if (ownsData()) {
        JS_ASSERT(newContents.data() != dataPointer());
        releaseData(fop);
    }

    setDataPointer(newContents, OwnsData);
}

void
ArrayBufferObject::changeContents(JSContext *cx, BufferContents newContents)
{
    
    uint8_t* oldDataPointer = dataPointer();
    setNewOwnedData(cx->runtime()->defaultFreeOp(), newContents);

    
    ArrayBufferViewObject *viewListHead = viewList();
    for (ArrayBufferViewObject *view = viewListHead; view; view = view->nextView()) {
        
        
        
        uint8_t *viewDataPointer = view->dataPointer();
        if (viewDataPointer) {
            JS_ASSERT(newContents);
            ptrdiff_t offset = viewDataPointer - oldDataPointer;
            viewDataPointer = static_cast<uint8_t *>(newContents.data()) + offset;
            view->setPrivate(viewDataPointer);
        }

        
        MarkObjectStateChange(cx, view);
    }
}

 bool
ArrayBufferObject::prepareForAsmJSNoSignals(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
    if (buffer->isAsmJSArrayBuffer())
        return true;

    if (buffer->isSharedArrayBuffer())
        return true;

    if (!ensureNonInline(cx, buffer))
        return false;

    buffer->setIsAsmJSArrayBuffer();
    return true;
}

void
ArrayBufferObject::releaseAsmJSArrayNoSignals(FreeOp *fop)
{
    JS_ASSERT(!(bufferKind() & MAPPED_BUFFER));
    fop->free_(dataPointer());
}

#ifdef JS_CODEGEN_X64
 bool
ArrayBufferObject::prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer,
                                   bool usesSignalHandlers)
{
    
    if (!usesSignalHandlers)
        return prepareForAsmJSNoSignals(cx, buffer);

    if (buffer->isAsmJSArrayBuffer())
        return true;

    
    if (buffer->isSharedArrayBuffer())
        return true;

    
    void *data;
# ifdef XP_WIN
    data = VirtualAlloc(nullptr, AsmJSMappedSize, MEM_RESERVE, PAGE_NOACCESS);
    if (!data)
        return false;
# else
    data = MozTaggedAnonymousMmap(nullptr, AsmJSMappedSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0, "asm-js-reserved");
    if (data == MAP_FAILED)
        return false;
# endif

    
    JS_ASSERT(buffer->byteLength() % AsmJSPageSize == 0);
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

    
    
    BufferContents newContents = BufferContents::create<BufferKind(ASMJS_BUFFER|MAPPED_BUFFER)>(data);
    buffer->changeContents(cx, newContents);
    JS_ASSERT(data == buffer->dataPointer());

    return true;
}

void
ArrayBufferObject::releaseAsmJSArray(FreeOp *fop)
{
    if (!(bufferKind() & MAPPED_BUFFER)) {
        releaseAsmJSArrayNoSignals(fop);
        return;
    }

    void *data = dataPointer();

    JS_ASSERT(uintptr_t(data) % AsmJSPageSize == 0);
# ifdef XP_WIN
    VirtualFree(data, 0, MEM_RELEASE);
# else
    munmap(data, AsmJSMappedSize);
#   if defined(MOZ_VALGRIND) && defined(VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE)
    
    
    if (AsmJSMappedSize > 0) {
        VALGRIND_ENABLE_ADDR_ERROR_REPORTING_IN_RANGE(data, AsmJSMappedSize);
    }
#   endif
# endif
}
#else 
bool
ArrayBufferObject::prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer,
                                   bool usesSignalHandlers)
{
    
    
    JS_ASSERT(!usesSignalHandlers);
    return prepareForAsmJSNoSignals(cx, buffer);
}

void
ArrayBufferObject::releaseAsmJSArray(FreeOp *fop)
{
    
    releaseAsmJSArrayNoSignals(fop);
}
#endif

bool
ArrayBufferObject::canNeuterAsmJSArrayBuffer(JSContext *cx, ArrayBufferObject &buffer)
{
    JS_ASSERT(!buffer.isSharedArrayBuffer());
    AsmJSActivation *act = cx->mainThread().asmJSActivationStack();
    for (; act; act = act->prevAsmJS()) {
        if (act->module().maybeHeapBufferObject() == &buffer)
            break;
    }
    if (!act)
        return true;

    return false;
}

ArrayBufferObject::BufferContents
ArrayBufferObject::createMappedContents(int fd, size_t offset, size_t length)
{
    void *data = AllocateMappedContent(fd, offset, length, ARRAY_BUFFER_ALIGNMENT);
    return BufferContents::create<MAPPED_BUFFER>(data);
}

void
ArrayBufferObject::releaseMappedArray()
{
    if(!isMappedArrayBuffer() || isNeutered())
        return;

    DeallocateMappedContent(dataPointer(), byteLength());
}

void
ArrayBufferObject::addView(ArrayBufferViewObject *view)
{
    
    
    
    

    ArrayBufferViewObject *viewsHead = viewList();
    if (viewsHead == nullptr) {
        
        
        JS_ASSERT(view->nextView() == nullptr);
    } else {
        view->setNextView(viewsHead);
    }

    setViewList(view);
}

uint8_t *
ArrayBufferObject::dataPointer() const
{
    if (isSharedArrayBuffer())
        return (uint8_t *)this->as<SharedArrayBufferObject>().dataPointer();
    return static_cast<uint8_t *>(getSlot(DATA_SLOT).toPrivate());
}

void
ArrayBufferObject::releaseData(FreeOp *fop)
{
    JS_ASSERT(ownsData());

    BufferKind bufkind = bufferKind();
    if (bufkind & ASMJS_BUFFER)
        releaseAsmJSArray(fop);
    else if (bufkind & MAPPED_BUFFER)
        releaseMappedArray();
    else
        fop->free_(dataPointer());
}

void
ArrayBufferObject::setDataPointer(BufferContents contents, OwnsState ownsData)
{
    MOZ_ASSERT_IF(!is<SharedArrayBufferObject>(), contents.data());
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

ArrayBufferObject *
ArrayBufferObject::create(JSContext *cx, uint32_t nbytes, BufferContents contents,
                          NewObjectKind newKind )
{
    JS_ASSERT_IF(contents.kind() & MAPPED_BUFFER, contents);

    
    
    
    
    size_t reservedSlots = JSCLASS_RESERVED_SLOTS(&class_);

    size_t nslots = reservedSlots;
    bool allocated = false;
    if (contents) {
        
        size_t nAllocated = nbytes;
        if (contents.kind() & MAPPED_BUFFER)
            nAllocated = JS_ROUNDUP(nbytes, js::gc::SystemPageSize());
        cx->zone()->updateMallocCounter(nAllocated);
    } else {
        size_t usableSlots = JSObject::MAX_FIXED_SLOTS - reservedSlots;
        if (nbytes <= usableSlots * sizeof(Value)) {
            int newSlots = (nbytes - 1) / sizeof(Value) + 1;
            JS_ASSERT(int(nbytes) <= newSlots * int(sizeof(Value)));
            nslots = reservedSlots + newSlots;
            contents = BufferContents::createUnowned(nullptr);
        } else {
            contents = AllocateArrayBufferContents(cx, nbytes);
            if (!contents)
                return nullptr;
            allocated = true;
        }
    }

    JS_ASSERT(!(class_.flags & JSCLASS_HAS_PRIVATE));
    gc::AllocKind allocKind = GetGCObjectKind(nslots);

    Rooted<ArrayBufferObject*> obj(cx, NewBuiltinClassInstance<ArrayBufferObject>(cx, allocKind, newKind));
    if (!obj) {
        if (allocated)
            js_free(contents.data());
        return nullptr;
    }

    JS_ASSERT(obj->getClass() == &class_);
    JS_ASSERT(!gc::IsInsideNursery(obj));

    if (!contents) {
        void *data = obj->fixedData(reservedSlots);
        memset(data, 0, nbytes);
        obj->initialize(nbytes, BufferContents::createUnowned(data), DoesntOwnData);
    } else {
        obj->initialize(nbytes, contents, OwnsData);
    }

    return obj;
}

ArrayBufferObject *
ArrayBufferObject::create(JSContext *cx, uint32_t nbytes,
                          NewObjectKind newKind )
{
    return create(cx, nbytes, BufferContents::createUnowned(nullptr));
}

JSObject *
ArrayBufferObject::createSlice(JSContext *cx, Handle<ArrayBufferObject*> arrayBuffer,
                               uint32_t begin, uint32_t end)
{
    uint32_t bufLength = arrayBuffer->byteLength();
    if (begin > bufLength || end > bufLength || begin > end) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPE_ERR_BAD_ARGS);
        return nullptr;
    }

    uint32_t length = end - begin;

    if (!arrayBuffer->hasData())
        return create(cx, 0);

    ArrayBufferObject *slice = create(cx, length);
    if (!slice)
        return nullptr;
    memcpy(slice->dataPointer(), arrayBuffer->dataPointer() + begin, length);
    return slice;
}

bool
ArrayBufferObject::createDataViewForThisImpl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(IsArrayBuffer(args.thisv()));

    




    JS_ASSERT(args.length() >= 2);

    Rooted<JSObject*> proto(cx, &args[args.length() - 1].toObject());

    Rooted<JSObject*> buffer(cx, &args.thisv().toObject());

    



    CallArgs frobbedArgs = CallArgsFromVp(args.length() - 1, args.base());
    return DataViewObject::construct(cx, buffer, frobbedArgs, proto);
}

bool
ArrayBufferObject::createDataViewForThis(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, createDataViewForThisImpl>(cx, args);
}

 bool
ArrayBufferObject::ensureNonInline(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
    if (!buffer->ownsData()) {
        MOZ_ASSERT(!buffer->isSharedArrayBuffer());
        BufferContents contents = AllocateArrayBufferContents(cx, buffer->byteLength());
        if (!contents)
            return false;
        memcpy(contents.data(), buffer->dataPointer(), buffer->byteLength());
        buffer->changeContents(cx, contents);
    }

    return true;
}

 ArrayBufferObject::BufferContents
ArrayBufferObject::stealContents(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
    if (!buffer->canNeuter(cx)) {
        js_ReportOverRecursed(cx);
        return BufferContents::createUnowned(nullptr);
    }

    BufferContents oldContents(buffer->dataPointer(), buffer->bufferKind());
    BufferContents newContents = AllocateArrayBufferContents(cx, buffer->byteLength());
    if (!newContents)
        return BufferContents::createUnowned(nullptr);

    if (buffer->hasStealableContents()) {
        
        
        
        buffer->setOwnsData(DoesntOwnData);
        ArrayBufferObject::neuter(cx, buffer, newContents);
        return oldContents;
    }

    
    
    memcpy(newContents.data(), oldContents.data(), buffer->byteLength());
    ArrayBufferObject::neuter(cx, buffer, oldContents);
    return newContents;
}

 void
ArrayBufferObject::addSizeOfExcludingThis(JSObject *obj, mozilla::MallocSizeOf mallocSizeOf,
                                          JS::ClassInfo *info)
{
    ArrayBufferObject &buffer = AsArrayBuffer(obj);

    if (!buffer.ownsData())
        return;

    if (MOZ_UNLIKELY(buffer.bufferKind() & ASMJS_BUFFER)) {
        
        
        if (buffer.bufferKind() & MAPPED_BUFFER)
            info->objectsNonHeapElementsAsmJS += buffer.byteLength();
        else
            info->objectsMallocHeapElementsAsmJS += mallocSizeOf(buffer.dataPointer());
    } else if (MOZ_UNLIKELY(buffer.bufferKind() & MAPPED_BUFFER)) {
        info->objectsNonHeapElementsMapped += buffer.byteLength();
    } else if (buffer.dataPointer()) {
        info->objectsMallocHeapElementsNonAsmJS += mallocSizeOf(buffer.dataPointer());
    }
}

 void
ArrayBufferObject::finalize(FreeOp *fop, JSObject *obj)
{
    ArrayBufferObject &buffer = obj->as<ArrayBufferObject>();

    if (buffer.ownsData())
        buffer.releaseData(fop);
}

 void
ArrayBufferObject::obj_trace(JSTracer *trc, JSObject *obj)
{
    JSRuntime *rt = trc->runtime();
    if (!IS_GC_MARKING_TRACER(trc) && !rt->isHeapMinorCollecting() && !rt->isHeapCompacting()
#ifdef JSGC_FJGENERATIONAL
        && !rt->isFJMinorCollecting()
#endif
        )
    {
        return;
    }

    
    
    
    
    
    
    
    
    
    
    

    ArrayBufferObject &buffer = AsArrayBuffer(obj);
    ArrayBufferViewObject *viewsHead = buffer.viewList();
    if (!viewsHead)
        return;

    ArrayBufferViewObject *tmp = viewsHead;
    buffer.setViewList(UpdateObjectIfRelocated(rt, &tmp));

    if (tmp->nextView() == nullptr) {
        
        
        
        MarkObjectUnbarriered(trc, &viewsHead, "arraybuffer.singleview");
        buffer.setViewListNoBarrier(viewsHead);
    } else if (!rt->isHeapCompacting()) {
        
        ArrayBufferVector &gcLiveArrayBuffers = buffer.compartment()->gcLiveArrayBuffers;

        
        
        if (buffer.inLiveList()) {
#ifdef DEBUG
            bool found = false;
            for (size_t i = 0; i < gcLiveArrayBuffers.length(); i++)
                found |= gcLiveArrayBuffers[i] == &buffer;
            JS_ASSERT(found);
#endif
        } else if (gcLiveArrayBuffers.append(&buffer)) {
            buffer.setInLiveList(true);
        } else {
            CrashAtUnhandlableOOM("OOM while updating live array buffers");
        }
    } else {
        
        ArrayBufferViewObject *prev = nullptr;
        ArrayBufferViewObject *view = viewsHead;
        while (view) {
            JS_ASSERT(buffer.compartment() == MaybeForwarded(view)->compartment());
            MarkObjectUnbarriered(trc, &view, "arraybuffer.singleview");
            if (prev)
                prev->setNextView(view);
            else
                buffer.setViewListNoBarrier(view);
            view = view->nextView();
        }
    }
}

 void
ArrayBufferObject::sweep(JSCompartment *compartment)
{
    JSRuntime *rt = compartment->runtimeFromMainThread();
    ArrayBufferVector &gcLiveArrayBuffers = compartment->gcLiveArrayBuffers;

    for (size_t i = 0; i < gcLiveArrayBuffers.length(); i++) {
        ArrayBufferObject *buffer = gcLiveArrayBuffers[i];

        JS_ASSERT(buffer->inLiveList());
        buffer->setInLiveList(false);

        ArrayBufferViewObject *viewsHead = buffer->viewList();
        JS_ASSERT(viewsHead);
        buffer->setViewList(UpdateObjectIfRelocated(rt, &viewsHead));

        
        
        ArrayBufferViewObject *prevLiveView = nullptr;
        ArrayBufferViewObject *view = viewsHead;
        while (view) {
            JS_ASSERT(buffer->compartment() == view->compartment());
            ArrayBufferViewObject *nextView = view->nextView();
            if (!IsObjectAboutToBeFinalized(&view)) {
                view->setNextView(prevLiveView);
                prevLiveView = view;
            }
            view = UpdateObjectIfRelocated(rt, &nextView);
        }

        buffer->setViewList(prevLiveView);
    }

    gcLiveArrayBuffers.clear();
}

 void
ArrayBufferObject::objectMoved(JSObject *obj, const JSObject *old)
{
    ArrayBufferObject &dst = obj->as<ArrayBufferObject>();
    const ArrayBufferObject &src = old->as<ArrayBufferObject>();

    
    const size_t reservedSlots = JSCLASS_RESERVED_SLOTS(&ArrayBufferObject::class_);
    if (src.dataPointer() == src.fixedData(reservedSlots))
        dst.setSlot(DATA_SLOT, PrivateValue(dst.fixedData(reservedSlots)));
}

void
ArrayBufferObject::resetArrayBufferList(JSCompartment *comp)
{
    ArrayBufferVector &gcLiveArrayBuffers = comp->gcLiveArrayBuffers;

    for (size_t i = 0; i < gcLiveArrayBuffers.length(); i++) {
        ArrayBufferObject *buffer = gcLiveArrayBuffers[i];

        JS_ASSERT(buffer->inLiveList());
        buffer->setInLiveList(false);
    }

    gcLiveArrayBuffers.clear();
}

 bool
ArrayBufferObject::saveArrayBufferList(JSCompartment *comp, ArrayBufferVector &vector)
{
    const ArrayBufferVector &gcLiveArrayBuffers = comp->gcLiveArrayBuffers;

    for (size_t i = 0; i < gcLiveArrayBuffers.length(); i++) {
        if (!vector.append(gcLiveArrayBuffers[i]))
            return false;
    }

    return true;
}

 void
ArrayBufferObject::restoreArrayBufferLists(ArrayBufferVector &vector)
{
    for (size_t i = 0; i < vector.length(); i++) {
        ArrayBufferObject *buffer = vector[i];

        JS_ASSERT(!buffer->inLiveList());
        buffer->setInLiveList(true);

        buffer->compartment()->gcLiveArrayBuffers.infallibleAppend(buffer);
    }
}











 void
ArrayBufferViewObject::trace(JSTracer *trc, JSObject *obj)
{
    HeapSlot &bufSlot = obj->getReservedSlotRef(BUFFER_SLOT);
    MarkSlot(trc, &bufSlot, "typedarray.buffer");

    
    
    if (bufSlot.isObject()) {
        ArrayBufferObject &buf = AsArrayBuffer(MaybeForwarded(&bufSlot.toObject()));
        int32_t offset = obj->getReservedSlot(BYTEOFFSET_SLOT).toInt32();
        MOZ_ASSERT(buf.dataPointer() != nullptr);
        obj->initPrivate(buf.dataPointer() + offset);
    }

    
    IsSlotMarked(&obj->getReservedSlotRef(NEXT_VIEW_SLOT));
}

void
ArrayBufferViewObject::neuter(void *newData)
{
    MOZ_ASSERT(newData != nullptr);
    if (is<DataViewObject>())
        as<DataViewObject>().neuter(newData);
    else if (is<TypedArrayObject>())
        as<TypedArrayObject>().neuter(newData);
    else
        as<TypedObject>().neuter(newData);
}

 ArrayBufferObject *
ArrayBufferViewObject::bufferObject(JSContext *cx, Handle<ArrayBufferViewObject *> thisObject)
{
    if (thisObject->is<TypedArrayObject>()) {
        Rooted<TypedArrayObject *> typedArray(cx, &thisObject->as<TypedArrayObject>());
        if (!TypedArrayObject::ensureHasBuffer(cx, typedArray))
            return nullptr;
        return thisObject->as<TypedArrayObject>().buffer();
    }
    MOZ_ASSERT(thisObject->is<DataViewObject>());
    return &thisObject->as<DataViewObject>().arrayBuffer();
}



JS_FRIEND_API(bool)
JS_IsArrayBufferViewObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<ArrayBufferViewObject>() : false;
}

JS_FRIEND_API(JSObject *)
js::UnwrapArrayBufferView(JSObject *obj)
{
    if (JSObject *unwrapped = CheckedUnwrap(obj))
        return unwrapped->is<ArrayBufferViewObject>() ? unwrapped : nullptr;
    return nullptr;
}

JS_FRIEND_API(uint32_t)
JS_GetArrayBufferByteLength(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? AsArrayBuffer(obj).byteLength() : 0;
}

JS_FRIEND_API(uint8_t *)
JS_GetArrayBufferData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    return AsArrayBuffer(obj).dataPointer();
}

JS_FRIEND_API(uint8_t *)
JS_GetStableArrayBufferData(JSContext *cx, HandleObject objArg)
{
    JSObject *obj = CheckedUnwrap(objArg);
    if (!obj)
        return nullptr;

    Rooted<ArrayBufferObject*> buffer(cx, &AsArrayBuffer(obj));
    if (!ArrayBufferObject::ensureNonInline(cx, buffer))
        return nullptr;

    return buffer->dataPointer();
}

JS_FRIEND_API(bool)
JS_NeuterArrayBuffer(JSContext *cx, HandleObject obj,
                     NeuterDataDisposition changeData)
{
    if (!obj->is<ArrayBufferObject>()) {
        JS_ReportError(cx, "ArrayBuffer object required");
        return false;
    }

    Rooted<ArrayBufferObject*> buffer(cx, &obj->as<ArrayBufferObject>());

    if (!buffer->canNeuter(cx)) {
        js_ReportOverRecursed(cx);
        return false;
    }

    if (changeData == ChangeData && buffer->hasStealableContents()) {
        ArrayBufferObject::BufferContents newContents =
            AllocateArrayBufferContents(cx, buffer->byteLength());
        if (!newContents)
            return false;
        ArrayBufferObject::neuter(cx, buffer, newContents);
    } else {
        ArrayBufferObject::neuter(cx, buffer, buffer->contents());
    }

    return true;
}

JS_FRIEND_API(bool)
JS_IsNeuteredArrayBufferObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return false;

    return obj->is<ArrayBufferObject>()
           ? obj->as<ArrayBufferObject>().isNeutered()
           : false;
}

JS_FRIEND_API(JSObject *)
JS_NewArrayBuffer(JSContext *cx, uint32_t nbytes)
{
    JS_ASSERT(nbytes <= INT32_MAX);
    return ArrayBufferObject::create(cx, nbytes);
}

JS_PUBLIC_API(JSObject *)
JS_NewArrayBufferWithContents(JSContext *cx, size_t nbytes, void *data)
{
    JS_ASSERT(data);
    ArrayBufferObject::BufferContents contents =
        ArrayBufferObject::BufferContents::create<ArrayBufferObject::PLAIN_BUFFER>(data);
    return ArrayBufferObject::create(cx, nbytes, contents, TenuredObject);
}

JS_FRIEND_API(bool)
JS_IsArrayBufferObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<ArrayBufferObject>() : false;
}

JS_FRIEND_API(bool)
JS_ArrayBufferHasData(JSObject *obj)
{
    return CheckedUnwrap(obj)->as<ArrayBufferObject>().hasData();
}

JS_FRIEND_API(JSObject *)
js::UnwrapArrayBuffer(JSObject *obj)
{
    if (JSObject *unwrapped = CheckedUnwrap(obj))
        return unwrapped->is<ArrayBufferObject>() ? unwrapped : nullptr;
    return nullptr;
}

JS_PUBLIC_API(void *)
JS_StealArrayBufferContents(JSContext *cx, HandleObject objArg)
{
    JSObject *obj = CheckedUnwrap(objArg);
    if (!obj)
        return nullptr;

    if (!obj->is<ArrayBufferObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return nullptr;
    }

    Rooted<ArrayBufferObject*> buffer(cx, &obj->as<ArrayBufferObject>());
    return ArrayBufferObject::stealContents(cx, buffer).data();
}

JS_PUBLIC_API(JSObject *)
JS_NewMappedArrayBufferWithContents(JSContext *cx, size_t nbytes, void *data)
{
    JS_ASSERT(data);
    ArrayBufferObject::BufferContents contents =
        ArrayBufferObject::BufferContents::create<ArrayBufferObject::MAPPED_BUFFER>(data);
    return ArrayBufferObject::create(cx, nbytes, contents, TenuredObject);
}

JS_PUBLIC_API(void *)
JS_CreateMappedArrayBufferContents(int fd, size_t offset, size_t length)
{
    return ArrayBufferObject::createMappedContents(fd, offset, length).data();
}

JS_PUBLIC_API(void)
JS_ReleaseMappedArrayBufferContents(void *contents, size_t length)
{
    DeallocateMappedContent(contents, length);
}

JS_FRIEND_API(bool)
JS_IsMappedArrayBufferObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return false;

    return obj->is<ArrayBufferObject>()
           ? obj->as<ArrayBufferObject>().isMappedArrayBuffer()
           : false;
}

JS_FRIEND_API(void *)
JS_GetArrayBufferViewData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    return obj->is<DataViewObject>() ? obj->as<DataViewObject>().dataPointer()
                                     : obj->as<TypedArrayObject>().viewData();
}

JS_FRIEND_API(JSObject *)
JS_GetArrayBufferViewBuffer(JSContext *cx, HandleObject objArg)
{
    JSObject *obj = CheckedUnwrap(objArg);
    if (!obj)
        return nullptr;
    Rooted<ArrayBufferViewObject *> viewObject(cx, &obj->as<ArrayBufferViewObject>());
    return ArrayBufferViewObject::bufferObject(cx, viewObject);
}

JS_FRIEND_API(uint32_t)
JS_GetArrayBufferViewByteLength(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->is<DataViewObject>()
           ? obj->as<DataViewObject>().byteLength()
           : obj->as<TypedArrayObject>().byteLength();
}

JS_FRIEND_API(JSObject *)
JS_GetObjectAsArrayBufferView(JSObject *obj, uint32_t *length, uint8_t **data)
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
js::GetArrayBufferViewLengthAndData(JSObject *obj, uint32_t *length, uint8_t **data)
{
    MOZ_ASSERT(obj->is<ArrayBufferViewObject>());

    *length = obj->is<DataViewObject>()
              ? obj->as<DataViewObject>().byteLength()
              : obj->as<TypedArrayObject>().byteLength();

    *data = static_cast<uint8_t*>(obj->is<DataViewObject>()
                                  ? obj->as<DataViewObject>().dataPointer()
                                  : obj->as<TypedArrayObject>().viewData());
}

JS_FRIEND_API(JSObject *)
JS_GetObjectAsArrayBuffer(JSObject *obj, uint32_t *length, uint8_t **data)
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
js::GetArrayBufferLengthAndData(JSObject *obj, uint32_t *length, uint8_t **data)
{
    MOZ_ASSERT(IsArrayBuffer(obj));
    *length = AsArrayBuffer(obj).byteLength();
    *data = AsArrayBuffer(obj).dataPointer();
}
