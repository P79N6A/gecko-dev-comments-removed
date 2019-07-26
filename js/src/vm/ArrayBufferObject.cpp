





#include "vm/ArrayBufferObject.h"

#include "mozilla/Alignment.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"

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

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "jit/AsmJS.h"
#include "jit/AsmJSModule.h"
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
    JS_NULL_CLASS_EXT
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







static void *
AllocateArrayBufferContents(JSContext *maybecx, uint32_t nbytes, void *oldptr = nullptr, size_t oldnbytes = 0)
{
    void *p;

    
    if (oldptr) {
        p = maybecx ? maybecx->runtime()->reallocCanGC(oldptr, nbytes) : js_realloc(oldptr, nbytes);

        
        if (p && nbytes > oldnbytes)
            memset(reinterpret_cast<uint8_t *>(p) + oldnbytes, 0, nbytes - oldnbytes);
    } else {
        p = maybecx ? maybecx->runtime()->callocCanGC(nbytes) : js_calloc(nbytes);
    }

    if (!p && maybecx)
        js_ReportOutOfMemory(maybecx);

    return p;
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
ArrayBufferObject::neuter(JSContext *cx, Handle<ArrayBufferObject*> buffer, void *newData)
{
    JS_ASSERT(buffer->canNeuter(cx));

    
    

    for (ArrayBufferViewObject *view = buffer->viewList(); view; view = view->nextView()) {
        view->neuter(newData);

        
        MarkObjectStateChange(cx, view);
    }

    if (newData != buffer->dataPointer())
        buffer->changeContents(cx, newData);

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
ArrayBufferObject::changeContents(JSContext *cx, void *newData)
{
    JS_ASSERT(!isAsmJSArrayBuffer());
    JS_ASSERT(!isSharedArrayBuffer());

    
    ArrayBufferViewObject *viewListHead = viewList();
    for (ArrayBufferViewObject *view = viewListHead; view; view = view->nextView()) {
        
        
        
        uint8_t *viewDataPointer = view->dataPointer();
        if (viewDataPointer) {
            JS_ASSERT(newData);
            viewDataPointer += static_cast<uint8_t *>(newData) - dataPointer();
            view->setPrivate(viewDataPointer);
        }

        
        MarkObjectStateChange(cx, view);
    }

    if (ownsData())
        releaseData(cx->runtime()->defaultFreeOp());

    setDataPointer(static_cast<uint8_t *>(newData), OwnsData);
}

#if defined(JS_CPU_X64)

JS_STATIC_ASSERT(AsmJSAllocationGranularity == AsmJSPageSize);
#endif

#if defined(JS_ION) && defined(JS_CPU_X64)
 bool
ArrayBufferObject::prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
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
    data = mmap(nullptr, AsmJSMappedSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (data == MAP_FAILED)
        return false;
# endif

    
    JS_ASSERT(buffer->byteLength() % AsmJSAllocationGranularity == 0);
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

    
    buffer->changeContents(cx, data);
    JS_ASSERT(data == buffer->dataPointer());

    
    
    buffer->setIsAsmJSArrayBuffer();

    return true;
}

void
ArrayBufferObject::releaseAsmJSArray(FreeOp *fop)
{
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
ArrayBufferObject::prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer)
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
ArrayBufferObject::releaseAsmJSArray(FreeOp *fop)
{
    fop->free_(dataPointer());
}
#endif

bool
ArrayBufferObject::canNeuterAsmJSArrayBuffer(JSContext *cx, ArrayBufferObject &buffer)
{
    JS_ASSERT(!buffer.isSharedArrayBuffer());
#ifdef JS_ION
    AsmJSActivation *act = cx->mainThread().asmJSActivationStackFromOwnerThread();
    for (; act; act = act->prevAsmJS()) {
        if (act->module().maybeHeapBufferObject() == &buffer)
            break;
    }
    if (!act)
        return true;

    return false;
#else
    return true;
#endif
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

    if (isAsmJSArrayBuffer())
        releaseAsmJSArray(fop);
    else
        fop->free_(dataPointer());
}

void
ArrayBufferObject::setDataPointer(void *data, OwnsState ownsData)
{
    MOZ_ASSERT_IF(!is<SharedArrayBufferObject>(), data != nullptr);
    setSlot(DATA_SLOT, PrivateValue(data));
    setOwnsData(ownsData);
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
ArrayBufferObject::create(JSContext *cx, uint32_t nbytes, void *data ,
                          NewObjectKind newKind )
{
    
    
    
    
    size_t reservedSlots = JSCLASS_RESERVED_SLOTS(&class_);

    size_t nslots = reservedSlots;
    if (!data) {
        size_t usableSlots = JSObject::MAX_FIXED_SLOTS - reservedSlots;
        if (nbytes <= usableSlots * sizeof(Value)) {
            int newSlots = (nbytes - 1) / sizeof(Value) + 1;
            JS_ASSERT(int(nbytes) <= newSlots * int(sizeof(Value)));
            nslots = reservedSlots + newSlots;
        } else {
            data = AllocateArrayBufferContents(cx, nbytes);
            if (!data)
                return nullptr;
        }
    }

    JS_ASSERT(!(class_.flags & JSCLASS_HAS_PRIVATE));
    gc::AllocKind allocKind = GetGCObjectKind(nslots);

    Rooted<ArrayBufferObject*> obj(cx, NewBuiltinClassInstance<ArrayBufferObject>(cx, allocKind, newKind));
    if (!obj) {
        if (data)
            js_free(data);
        return nullptr;
    }
    JS_ASSERT(obj->getClass() == &class_);

    JS_ASSERT(!gc::IsInsideNursery(cx->runtime(), obj));

    if (data) {
        obj->initialize(nbytes, data, OwnsData);
    } else {
        void *data = &obj->fixedSlots()[reservedSlots];
        memset(data, 0, nbytes);
        obj->initialize(nbytes, data, DoesntOwnData);
    }

    return obj;
}

JSObject *
ArrayBufferObject::createSlice(JSContext *cx, Handle<ArrayBufferObject*> arrayBuffer,
                               uint32_t begin, uint32_t end)
{
    JS_ASSERT(begin <= arrayBuffer->byteLength());
    JS_ASSERT(end <= arrayBuffer->byteLength());
    JS_ASSERT(begin <= end);
    uint32_t length = end - begin;

    if (!arrayBuffer->hasData())
        return create(cx, 0);

    JSObject *slice = create(cx, length);
    if (!slice)
        return nullptr;
    memcpy(slice->as<ArrayBufferObject>().dataPointer(), arrayBuffer->dataPointer() + begin, length);
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
        void *data = AllocateArrayBufferContents(cx, buffer->byteLength());
        if (!data)
            return false;
        memcpy(data, buffer->dataPointer(), buffer->byteLength());
        buffer->changeContents(cx, data);
    }

    return true;
}

 void *
ArrayBufferObject::stealContents(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
    if (!buffer->canNeuter(cx)) {
        js_ReportOverRecursed(cx);
        return nullptr;
    }

    void *oldData = buffer->dataPointer();
    void *newData = AllocateArrayBufferContents(cx, buffer->byteLength());
    if (!newData)
        return nullptr;

    if (buffer->hasStealableContents()) {
        buffer->setOwnsData(DoesntOwnData);
        ArrayBufferObject::neuter(cx, buffer, newData);
        return oldData;
    } else {
        memcpy(newData, oldData, buffer->byteLength());
        ArrayBufferObject::neuter(cx, buffer, oldData);
        return newData;
    }

    return oldData;
}

 void
ArrayBufferObject::addSizeOfExcludingThis(JSObject *obj, mozilla::MallocSizeOf mallocSizeOf, JS::ObjectsExtraSizes *sizes)
{
    ArrayBufferObject &buffer = AsArrayBuffer(obj);

    if (!buffer.ownsData())
        return;

    if (MOZ_UNLIKELY(buffer.isAsmJSArrayBuffer())) {
#if defined (JS_CPU_X64)
        
        
        sizes->nonHeapElementsAsmJS += buffer.byteLength();
#else
        sizes->mallocHeapElementsAsmJS += mallocSizeOf(buffer.dataPointer());
#endif
    } else if (buffer.dataPointer()) {
        sizes->mallocHeapElementsNonAsmJS += mallocSizeOf(buffer.dataPointer());
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
    if (!IS_GC_MARKING_TRACER(trc) && !trc->runtime->isHeapMinorCollecting())
        return;

    
    
    
    
    
    
    
    
    
    
    

    ArrayBufferObject &buffer = AsArrayBuffer(obj);
    ArrayBufferViewObject *viewsHead = buffer.viewList();
    if (!viewsHead)
        return;

    buffer.setViewList(UpdateObjectIfRelocated(trc->runtime, &viewsHead));

    if (viewsHead->nextView() == nullptr) {
        
        
        
        MarkObjectUnbarriered(trc, &viewsHead, "arraybuffer.singleview");
        buffer.setViewListNoBarrier(viewsHead);
    } else {
        
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
        ArrayBufferObject &buf = AsArrayBuffer(&bufSlot.toObject());
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



JS_FRIEND_API(bool)
JS_IsArrayBufferViewObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<ArrayBufferViewObject>() : false;
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
JS_NeuterArrayBuffer(JSContext *cx, HandleObject obj)
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

    ArrayBufferObject::neuter(cx, buffer, buffer->dataPointer());
    return true;
}

JS_FRIEND_API(JSObject *)
JS_NewArrayBuffer(JSContext *cx, uint32_t nbytes)
{
    JS_ASSERT(nbytes <= INT32_MAX);
    return ArrayBufferObject::create(cx, nbytes);
}

JS_PUBLIC_API(JSObject *)
JS_NewArrayBufferWithContents(JSContext *cx, size_t nbytes, void *contents)
{
    JS_ASSERT(contents);
    return ArrayBufferObject::create(cx, nbytes, contents, TenuredObject);
}

JS_PUBLIC_API(void *)
JS_AllocateArrayBufferContents(JSContext *maybecx, uint32_t nbytes)
{
    return AllocateArrayBufferContents(maybecx, nbytes);
}

JS_PUBLIC_API(void *)
JS_ReallocateArrayBufferContents(JSContext *maybecx, uint32_t nbytes, void *oldContents, uint32_t oldNbytes)
{
    return AllocateArrayBufferContents(maybecx, nbytes, oldContents, oldNbytes);
}

JS_FRIEND_API(bool)
JS_IsArrayBufferObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<ArrayBufferObject>() : false;
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
    return ArrayBufferObject::stealContents(cx, buffer);
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
JS_GetArrayBufferViewBuffer(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    return obj->as<ArrayBufferViewObject>().bufferObject();
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

