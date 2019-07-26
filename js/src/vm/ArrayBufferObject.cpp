





#include "vm/ArrayBufferObject.h"

#include "mozilla/Alignment.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"

#include <string.h>
#ifndef XP_WIN
# include <sys/mman.h>
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
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/SharedArrayObject.h"
#include "vm/WrapperObject.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;






static const uint8_t ARRAYBUFFER_RESERVED_SLOTS = JSObject::MAX_FIXED_SLOTS - 1;



js::ArrayBufferObject * const js::UNSET_BUFFER_LINK = reinterpret_cast<js::ArrayBufferObject*>(0x2);






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
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(ARRAYBUFFER_RESERVED_SLOTS) |
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
    JSCLASS_HAS_PRIVATE |
    JSCLASS_IMPLEMENTS_BARRIERS |
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(ARRAYBUFFER_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,        
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
        ArrayBufferObject::obj_defineGeneric,
        ArrayBufferObject::obj_defineProperty,
        ArrayBufferObject::obj_defineElement,
        ArrayBufferObject::obj_getGeneric,
        ArrayBufferObject::obj_getProperty,
        ArrayBufferObject::obj_getElement,
        ArrayBufferObject::obj_setGeneric,
        ArrayBufferObject::obj_setProperty,
        ArrayBufferObject::obj_setElement,
        ArrayBufferObject::obj_getGenericAttributes,
        ArrayBufferObject::obj_setGenericAttributes,
        ArrayBufferObject::obj_deleteProperty,
        ArrayBufferObject::obj_deleteElement,
        nullptr, nullptr, 
        nullptr,          
        ArrayBufferObject::obj_enumerate,
        nullptr,          
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







static ObjectElements *
AllocateArrayBufferContents(JSContext *maybecx, uint32_t nbytes, void *oldptr = nullptr)
{
    uint32_t size = nbytes + sizeof(ObjectElements);
    JS_ASSERT(size > nbytes); 
    ObjectElements *newheader;

    
    if (oldptr) {
        ObjectElements *oldheader = static_cast<ObjectElements *>(oldptr);
        uint32_t oldnbytes = ArrayBufferObject::headerInitializedLength(oldheader);

        void *p = maybecx ? maybecx->runtime()->reallocCanGC(oldptr, size) : js_realloc(oldptr, size);
        newheader = static_cast<ObjectElements *>(p);

        
        if (newheader && nbytes > oldnbytes)
            memset(reinterpret_cast<uint8_t*>(newheader->elements()) + oldnbytes, 0, nbytes - oldnbytes);
    } else {
        void *p = maybecx ? maybecx->runtime()->callocCanGC(size) : js_calloc(size);
        newheader = static_cast<ObjectElements *>(p);
    }
    if (!newheader) {
        if (maybecx)
            js_ReportOutOfMemory(maybecx);
        return nullptr;
    }

    ArrayBufferObject::updateElementsHeader(newheader, nbytes);

    return newheader;
}







struct OldObjectRepresentationHack {
    uint32_t flags;
    uint32_t initializedLength;
    EncapsulatedPtr<ArrayBufferViewObject> views;
};

static ArrayBufferViewObject *
GetViewList(ArrayBufferObject *obj)
{
    return reinterpret_cast<OldObjectRepresentationHack*>(obj->getElementsHeader())->views;
}

static void
SetViewList(ArrayBufferObject *obj, ArrayBufferViewObject *viewsHead)
{
    reinterpret_cast<OldObjectRepresentationHack*>(obj->getElementsHeader())->views = viewsHead;
    PostBarrierTypedArrayObject(obj);
}

static void
InitViewList(ArrayBufferObject *obj, ArrayBufferViewObject *viewsHead)
{
    reinterpret_cast<OldObjectRepresentationHack*>(obj->getElementsHeader())->views.init(viewsHead);
    PostBarrierTypedArrayObject(obj);
}

static EncapsulatedPtr<ArrayBufferViewObject> &
GetViewListRef(ArrayBufferObject *obj)
{
    JS_ASSERT(obj->runtimeFromMainThread()->isHeapBusy());
    return reinterpret_cast<OldObjectRepresentationHack*>(obj->getElementsHeader())->views;
}

 bool
ArrayBufferObject::neuterViews(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
    ArrayBufferViewObject *view;
    size_t numViews = 0;
    for (view = GetViewList(buffer); view; view = view->nextView()) {
        numViews++;
        view->neuter(cx);

        
        MarkObjectStateChange(cx, view);
    }

    
    
    if (buffer->isAsmJSArrayBuffer()) {
        if (!ArrayBufferObject::neuterAsmJSArrayBuffer(cx, *buffer))
            return false;
    }

    
    if (numViews > 1 && GetViewList(buffer)->bufferLink() != UNSET_BUFFER_LINK) {
        ArrayBufferObject *prev = buffer->compartment()->gcLiveArrayBuffers;
        if (prev == buffer) {
            buffer->compartment()->gcLiveArrayBuffers = GetViewList(prev)->bufferLink();
        } else {
            for (ArrayBufferObject *b = GetViewList(prev)->bufferLink();
                 b;
                 b = GetViewList(b)->bufferLink())
            {
                if (b == buffer) {
                    GetViewList(prev)->setBufferLink(GetViewList(b)->bufferLink());
                    break;
                }
                prev = b;
            }
        }
    }

    return true;
}

uint8_t *
ArrayBufferObject::dataPointer() const {
    if (isSharedArrayBuffer())
        return (uint8_t *)this->as<SharedArrayBufferObject>().dataPointer();
    return (uint8_t *)elements;
}

void
ArrayBufferObject::changeContents(JSContext *cx, ObjectElements *newHeader)
{
    JS_ASSERT(!isAsmJSArrayBuffer());
    JS_ASSERT(!isSharedArrayBuffer());

    
    uint32_t byteLengthCopy = byteLength();
    uintptr_t oldDataPointer = uintptr_t(dataPointer());
    ArrayBufferViewObject *viewListHead = GetViewList(this);

    
    uintptr_t newDataPointer = uintptr_t(newHeader->elements());
    for (ArrayBufferViewObject *view = viewListHead; view; view = view->nextView()) {
        
        
        
        
        
        
        uint8_t *viewDataPointer = static_cast<uint8_t*>(view->getPrivate());
        if (viewDataPointer) {
            viewDataPointer += newDataPointer - oldDataPointer;
            view->setPrivate(viewDataPointer);
        }

        
        MarkObjectStateChange(cx, view);
    }

    
    
    SetViewList(this, nullptr);

#ifdef JSGC_GENERATIONAL
    ObjectElements *oldHeader = ObjectElements::fromElements(elements);
    JS_ASSERT(oldHeader != newHeader);
    JSRuntime *rt = runtimeFromMainThread();
    if (hasDynamicElements())
        rt->gcNursery.notifyRemovedElements(this, oldHeader);
#endif

    elements = newHeader->elements();

#ifdef JSGC_GENERATIONAL
    if (hasDynamicElements())
        rt->gcNursery.notifyNewElements(this, newHeader);
#endif

    initElementsHeader(newHeader, byteLengthCopy);
    InitViewList(this, viewListHead);
}

void
ArrayBufferObject::neuter(ObjectElements *newHeader, JSContext *cx)
{
    MOZ_ASSERT(!isSharedArrayBuffer());

    if (hasStealableContents()) {
        MOZ_ASSERT(newHeader);

        ObjectElements *oldHeader = getElementsHeader();
        MOZ_ASSERT(newHeader != oldHeader);

        changeContents(cx, newHeader);

        FreeOp fop(cx->runtime(), false);
        fop.free_(oldHeader);
    } else {
        elements = newHeader->elements();
    }

    uint32_t byteLen = 0;
    updateElementsHeader(newHeader, byteLen);

    newHeader->setIsNeuteredBuffer();
}

 bool
ArrayBufferObject::ensureNonInline(JSContext *cx, Handle<ArrayBufferObject*> buffer)
{
    JS_ASSERT(!buffer->isSharedArrayBuffer());
    if (buffer->hasDynamicElements())
        return true;

    ObjectElements *newHeader = AllocateArrayBufferContents(cx, buffer->byteLength());
    if (!newHeader)
        return false;

    void *newHeaderDataPointer = reinterpret_cast<void*>(newHeader->elements());
    memcpy(newHeaderDataPointer, buffer->dataPointer(), buffer->byteLength());

    buffer->changeContents(cx, newHeader);
    return true;
}

#if defined(JS_CPU_X64)

JS_STATIC_ASSERT(sizeof(ObjectElements) < AsmJSPageSize);
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

    
    void *p;
# ifdef XP_WIN
    p = VirtualAlloc(nullptr, AsmJSMappedSize, MEM_RESERVE, PAGE_NOACCESS);
    if (!p)
        return false;
# else
    p = mmap(nullptr, AsmJSMappedSize, PROT_NONE, MAP_PRIVATE | MAP_ANON, -1, 0);
    if (p == MAP_FAILED)
        return false;
# endif

    
    JS_ASSERT(buffer->byteLength() % AsmJSAllocationGranularity == 0);
# ifdef XP_WIN
    if (!VirtualAlloc(p, AsmJSPageSize + buffer->byteLength(), MEM_COMMIT, PAGE_READWRITE)) {
        VirtualFree(p, 0, MEM_RELEASE);
        return false;
    }
# else
    if (mprotect(p, AsmJSPageSize + buffer->byteLength(), PROT_READ | PROT_WRITE)) {
        munmap(p, AsmJSMappedSize);
        return false;
    }
# endif

    
    uint8_t *data = reinterpret_cast<uint8_t*>(p) + AsmJSPageSize;
    memcpy(data, buffer->dataPointer(), buffer->byteLength());

    
    ObjectElements *newHeader = reinterpret_cast<ObjectElements*>(data - sizeof(ObjectElements));
    ObjectElements *oldHeader = buffer->hasDynamicElements() ? buffer->getElementsHeader()
                                                             : nullptr;
    buffer->changeContents(cx, newHeader);
    js_free(oldHeader);

    
    
    newHeader->setIsAsmJSArrayBuffer();
    JS_ASSERT(data == buffer->dataPointer());
    return true;
}

void
ArrayBufferObject::releaseAsmJSArrayBuffer(FreeOp *fop, JSObject *obj)
{
    ArrayBufferObject &buffer = obj->as<ArrayBufferObject>();
    JS_ASSERT(buffer.isAsmJSArrayBuffer());

    uint8_t *p = buffer.dataPointer() - AsmJSPageSize ;
    JS_ASSERT(uintptr_t(p) % AsmJSPageSize == 0);
# ifdef XP_WIN
    VirtualFree(p, 0, MEM_RELEASE);
# else
    munmap(p, AsmJSMappedSize);
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

    JS_ASSERT(buffer->hasDynamicElements());
    buffer->getElementsHeader()->setIsAsmJSArrayBuffer();
    return true;
}

void
ArrayBufferObject::releaseAsmJSArrayBuffer(FreeOp *fop, JSObject *obj)
{
    fop->free_(obj->as<ArrayBufferObject>().getElementsHeader());
}
#endif

bool
ArrayBufferObject::neuterAsmJSArrayBuffer(JSContext *cx, ArrayBufferObject &buffer)
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

    js_ReportOverRecursed(cx);
    return false;
#else
    return true;
#endif
}

void
ArrayBufferObject::addView(ArrayBufferViewObject *view)
{
    
    JS_ASSERT(view->bufferLink() == UNSET_BUFFER_LINK);

    
    
    
    

    ArrayBufferViewObject *viewsHead = GetViewList(this);
    if (viewsHead == nullptr) {
        
        
        JS_ASSERT(view->nextView() == nullptr);
    } else {
        view->prependToViews(viewsHead);
    }

    SetViewList(this, view);
}

ArrayBufferObject *
ArrayBufferObject::create(JSContext *cx, uint32_t nbytes, bool clear ,
                          NewObjectKind newKind )
{
    Rooted<ArrayBufferObject*> obj(cx, NewBuiltinClassInstance<ArrayBufferObject>(cx, newKind));
    if (!obj)
        return nullptr;
    JS_ASSERT_IF(obj->isTenured(), obj->tenuredGetAllocKind() == gc::FINALIZE_OBJECT16_BACKGROUND);
    JS_ASSERT(obj->getClass() == &class_);

    js::Shape *empty = EmptyShape::getInitialShape(cx, &class_,
                                                   obj->getProto(), obj->getParent(), obj->getMetadata(),
                                                   gc::FINALIZE_OBJECT16_BACKGROUND);
    if (!empty)
        return nullptr;
    obj->setLastPropertyInfallible(empty);

    
    
    
    JS_ASSERT(!obj->hasDynamicSlots());
    JS_ASSERT(!obj->hasDynamicElements());

    
    
    size_t usableSlots = ARRAYBUFFER_RESERVED_SLOTS - ObjectElements::VALUES_PER_HEADER;

    if (nbytes > sizeof(Value) * usableSlots) {
        ObjectElements *header = AllocateArrayBufferContents(cx, nbytes);
        if (!header)
            return nullptr;
        obj->elements = header->elements();

#ifdef JSGC_GENERATIONAL
        JSRuntime *rt = obj->runtimeFromMainThread();
        rt->gcNursery.notifyNewElements(obj, header);
#endif
        obj->initElementsHeader(obj->getElementsHeader(), nbytes);
    } else {
        
        obj->setFixedElements();
        obj->initElementsHeader(obj->getElementsHeader(), nbytes);
        if (clear)
            memset(obj->dataPointer(), 0, nbytes);
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

    JSObject *slice = create(cx, length, false);
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
ArrayBufferObject::stealContents(JSContext *cx, Handle<ArrayBufferObject*> buffer, void **contents,
                                 uint8_t **data)
{
    uint32_t byteLen = buffer->byteLength();

    
    
    ObjectElements *transferableHeader;
    ObjectElements *newHeader;
    bool stolen = buffer->hasStealableContents();
    if (stolen) {
        transferableHeader = buffer->getElementsHeader();

        newHeader = AllocateArrayBufferContents(cx, byteLen);
        if (!newHeader)
            return false;
    } else {
        transferableHeader = AllocateArrayBufferContents(cx, byteLen);
        if (!transferableHeader)
            return false;

        initElementsHeader(transferableHeader, byteLen);
        void *headerDataPointer = reinterpret_cast<void*>(transferableHeader->elements());
        memcpy(headerDataPointer, buffer->dataPointer(), byteLen);

        
        newHeader = buffer->getElementsHeader();
    }

    JS_ASSERT(!IsInsideNursery(cx->runtime(), transferableHeader));
    *contents = transferableHeader;
    *data = reinterpret_cast<uint8_t *>(transferableHeader + 1);

    
    
    if (!ArrayBufferObject::neuterViews(cx, buffer))
        return false;

    
    
    
    if (stolen)
        buffer->changeContents(cx, ObjectElements::fromElements(buffer->fixedElements()));

    buffer->neuter(newHeader, cx);
    return true;
}

void
ArrayBufferObject::obj_trace(JSTracer *trc, JSObject *obj)
{
    



    JSObject *delegate = static_cast<JSObject*>(obj->getPrivate());
    if (delegate) {
        JS_SET_TRACING_LOCATION(trc, &obj->privateRef(obj->numFixedSlots()));
        MarkObjectUnbarriered(trc, &delegate, "arraybuffer.delegate");
        obj->setPrivateUnbarriered(delegate);
    }

    if (!IS_GC_MARKING_TRACER(trc) && !trc->runtime->isHeapMinorCollecting())
        return;

    
    
    
    
    
    
    
    
    
    
    

    ArrayBufferObject &buffer = AsArrayBuffer(obj);
    ArrayBufferViewObject *viewsHead = UpdateObjectIfRelocated(trc->runtime,
                                                               &GetViewListRef(&buffer));
    if (!viewsHead)
        return;

    viewsHead = UpdateObjectIfRelocated(trc->runtime, &GetViewListRef(&buffer));
    ArrayBufferViewObject *firstView = viewsHead;
    if (firstView->nextView() == nullptr) {
        
        
        
        MarkObject(trc, &GetViewListRef(&buffer), "arraybuffer.singleview");
    } else {
        
        
        
        if (firstView->bufferLink() == UNSET_BUFFER_LINK) {
            JS_ASSERT(obj->compartment() == firstView->compartment());
            ArrayBufferObject **bufList = &obj->compartment()->gcLiveArrayBuffers;
            firstView->setBufferLink(*bufList);
            *bufList = &AsArrayBuffer(obj);
        } else {
#ifdef DEBUG
            bool found = false;
            for (ArrayBufferObject *p = obj->compartment()->gcLiveArrayBuffers;
                 p;
                 p = GetViewList(p)->bufferLink())
            {
                if (p == obj)
                {
                    JS_ASSERT(!found);
                    found = true;
                }
            }
#endif
        }
    }
}

 void
ArrayBufferObject::sweep(JSCompartment *compartment)
{
    JSRuntime *rt = compartment->runtimeFromMainThread();
    ArrayBufferObject *buffer = compartment->gcLiveArrayBuffers;
    JS_ASSERT(buffer != UNSET_BUFFER_LINK);
    compartment->gcLiveArrayBuffers = nullptr;

    while (buffer) {
        ArrayBufferViewObject *viewsHead = UpdateObjectIfRelocated(rt, &GetViewListRef(buffer));
        JS_ASSERT(viewsHead);

        ArrayBufferObject *nextBuffer = viewsHead->bufferLink();
        JS_ASSERT(nextBuffer != UNSET_BUFFER_LINK);
        viewsHead->setBufferLink(UNSET_BUFFER_LINK);

        
        
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
        SetViewList(buffer, prevLiveView);

        buffer = nextBuffer;
    }
}

void
ArrayBufferObject::resetArrayBufferList(JSCompartment *comp)
{
    ArrayBufferObject *buffer = comp->gcLiveArrayBuffers;
    JS_ASSERT(buffer != UNSET_BUFFER_LINK);
    comp->gcLiveArrayBuffers = nullptr;

    while (buffer) {
        ArrayBufferViewObject *view = GetViewList(buffer);
        JS_ASSERT(view);

        ArrayBufferObject *nextBuffer = view->bufferLink();
        JS_ASSERT(nextBuffer != UNSET_BUFFER_LINK);

        view->setBufferLink(UNSET_BUFFER_LINK);
        buffer = nextBuffer;
    }
}

 bool
ArrayBufferObject::saveArrayBufferList(JSCompartment *comp, ArrayBufferVector &vector)
{
    ArrayBufferObject *buffer = comp->gcLiveArrayBuffers;
    while (buffer) {
        JS_ASSERT(buffer != UNSET_BUFFER_LINK);
        if (!vector.append(buffer))
            return false;

        ArrayBufferViewObject *view = GetViewList(buffer);
        JS_ASSERT(view);
        buffer = view->bufferLink();
    }
    return true;
}

 void
ArrayBufferObject::restoreArrayBufferLists(ArrayBufferVector &vector)
{
    for (ArrayBufferObject **p = vector.begin(); p != vector.end(); p++) {
        ArrayBufferObject *buffer = *p;
        JSCompartment *comp = buffer->compartment();
        ArrayBufferViewObject *firstView = GetViewList(buffer);
        JS_ASSERT(firstView);
        JS_ASSERT(firstView->compartment() == comp);
        JS_ASSERT(firstView->bufferLink() == UNSET_BUFFER_LINK);
        firstView->setBufferLink(comp->gcLiveArrayBuffers);
        comp->gcLiveArrayBuffers = buffer;
    }
}

bool
ArrayBufferObject::obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                     MutableHandleObject objp, MutableHandleShape propp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;

    bool delegateResult = JSObject::lookupGeneric(cx, delegate, id, objp, propp);

    




    if (!delegateResult)
        return false;

    if (propp) {
        if (objp == delegate)
            objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        objp.set(nullptr);
        propp.set(nullptr);
        return true;
    }

    return JSObject::lookupGeneric(cx, proto, id, objp, propp);
}

bool
ArrayBufferObject::obj_lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                      MutableHandleObject objp, MutableHandleShape propp)
{
    Rooted<jsid> id(cx, NameToId(name));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

bool
ArrayBufferObject::obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                     MutableHandleObject objp, MutableHandleShape propp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;

    





    if (!JSObject::lookupElement(cx, delegate, index, objp, propp))
        return false;

    if (propp) {
        if (objp == delegate)
            objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (proto)
        return JSObject::lookupElement(cx, proto, index, objp, propp);

    objp.set(nullptr);
    propp.set(nullptr);
    return true;
}

bool
ArrayBufferObject::obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::DefineGeneric(cx, delegate, id, v, getter, setter, attrs);
}

bool
ArrayBufferObject::obj_defineProperty(JSContext *cx, HandleObject obj,
                                      HandlePropertyName name, HandleValue v,
                                      PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

bool
ArrayBufferObject::obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::DefineElement(cx, delegate, index, v, getter, setter, attrs);
}

bool
ArrayBufferObject::obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                                  HandleId id, MutableHandleValue vp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::GetProperty(cx, delegate, receiver, id, vp);
}

bool
ArrayBufferObject::obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                   HandlePropertyName name, MutableHandleValue vp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    Rooted<jsid> id(cx, NameToId(name));
    return baseops::GetProperty(cx, delegate, receiver, id, vp);
}

bool
ArrayBufferObject::obj_getElement(JSContext *cx, HandleObject obj,
                                  HandleObject receiver, uint32_t index, MutableHandleValue vp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::GetElement(cx, delegate, receiver, index, vp);
}

bool
ArrayBufferObject::obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                  MutableHandleValue vp, bool strict)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;

    return baseops::SetPropertyHelper<SequentialExecution>(cx, delegate, obj, id, 0, vp, strict);
}

bool
ArrayBufferObject::obj_setProperty(JSContext *cx, HandleObject obj,
                                   HandlePropertyName name, MutableHandleValue vp, bool strict)
{
    Rooted<jsid> id(cx, NameToId(name));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

bool
ArrayBufferObject::obj_setElement(JSContext *cx, HandleObject obj,
                                  uint32_t index, MutableHandleValue vp, bool strict)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;

    return baseops::SetElementHelper(cx, delegate, obj, index, 0, vp, strict);
}

bool
ArrayBufferObject::obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                            HandleId id, unsigned *attrsp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::GetAttributes(cx, delegate, id, attrsp);
}

bool
ArrayBufferObject::obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                            HandleId id, unsigned *attrsp)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::SetAttributes(cx, delegate, id, attrsp);
}

bool
ArrayBufferObject::obj_deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                      bool *succeeded)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::DeleteProperty(cx, delegate, name, succeeded);
}

bool
ArrayBufferObject::obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                     bool *succeeded)
{
    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::DeleteElement(cx, delegate, index, succeeded);
}

bool
ArrayBufferObject::obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                                 MutableHandleValue statep, MutableHandleId idp)
{
    statep.setNull();
    return true;
}











 void
ArrayBufferViewObject::trace(JSTracer *trc, JSObject *obj)
{
    HeapSlot &bufSlot = obj->getReservedSlotRef(BUFFER_SLOT);
    MarkSlot(trc, &bufSlot, "typedarray.buffer");

    

    if (bufSlot.isObject()) {
        ArrayBufferObject &buf = AsArrayBuffer(&bufSlot.toObject());
        if (buf.getElementsHeader()->isNeuteredBuffer()) {
            
            JS_ASSERT(obj->getPrivate() == nullptr);
        } else {
            int32_t offset = obj->getReservedSlot(BYTEOFFSET_SLOT).toInt32();
            obj->initPrivate(buf.dataPointer() + offset);
        }
    }

    
    IsSlotMarked(&obj->getReservedSlotRef(NEXT_VIEW_SLOT));
}

void
ArrayBufferViewObject::prependToViews(ArrayBufferViewObject *viewsHead)
{
    setNextView(viewsHead);

    
    
    setBufferLink(viewsHead->bufferLink());
    viewsHead->setBufferLink(UNSET_BUFFER_LINK);
}

void
ArrayBufferViewObject::neuter(JSContext *cx)
{
    if (is<DataViewObject>())
        as<DataViewObject>().neuter();
    else if (is<TypedArrayObject>())
        as<TypedArrayObject>().neuter(cx);
    else
        as<TypedObject>().neuter(cx);
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
JS_GetStableArrayBufferData(JSContext *cx, JSObject *obj)
{
    obj = CheckedUnwrap(obj);
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

    ObjectElements *newHeader;
    if (buffer->hasStealableContents()) {
        
        
        
        
        
        newHeader = AllocateArrayBufferContents(cx, buffer->byteLength());
        if (!newHeader)
            return false;
    } else {
        
        
        newHeader = buffer->getElementsHeader();
    }

    
    if (!ArrayBufferObject::neuterViews(cx, buffer)) {
        if (buffer->hasStealableContents()) {
            FreeOp fop(cx->runtime(), false);
            fop.free_(newHeader);
        }
        return false;
    }

    buffer->neuter(newHeader, cx);
    return true;
}

JS_FRIEND_API(JSObject *)
JS_NewArrayBuffer(JSContext *cx, uint32_t nbytes)
{
    JS_ASSERT(nbytes <= INT32_MAX);
    return ArrayBufferObject::create(cx, nbytes);
}

JS_PUBLIC_API(JSObject *)
JS_NewArrayBufferWithContents(JSContext *cx, void *contents)
{
    JS_ASSERT(contents);

    
    
    
    JSObject *obj = ArrayBufferObject::create(cx, 0, true, TenuredObject);
    if (!obj)
        return nullptr;
    js::ObjectElements *elements = reinterpret_cast<js::ObjectElements *>(contents);
    obj->setDynamicElements(elements);
    JS_ASSERT(GetViewList(&obj->as<ArrayBufferObject>()) == nullptr);

#ifdef JSGC_GENERATIONAL
    cx->runtime()->gcNursery.notifyNewElements(obj, elements);
#endif
    return obj;
}

JS_PUBLIC_API(bool)
JS_AllocateArrayBufferContents(JSContext *maybecx, uint32_t nbytes,
                               void **contents, uint8_t **data)
{
    js::ObjectElements *header = AllocateArrayBufferContents(maybecx, nbytes);
    if (!header)
        return false;

    ArrayBufferObject::updateElementsHeader(header, nbytes);

    *contents = header;
    *data = reinterpret_cast<uint8_t*>(header->elements());
    return true;
}

JS_PUBLIC_API(bool)
JS_ReallocateArrayBufferContents(JSContext *maybecx, uint32_t nbytes, void **contents, uint8_t **data)
{
    js::ObjectElements *header = AllocateArrayBufferContents(maybecx, nbytes, *contents);
    if (!header)
        return false;

    ArrayBufferObject::initElementsHeader(header, nbytes);

    *contents = header;
    *data = reinterpret_cast<uint8_t*>(header->elements());
    return true;
}

JS_FRIEND_API(bool)
JS_IsArrayBufferObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<ArrayBufferObject>() : false;
}

JS_PUBLIC_API(bool)
JS_StealArrayBufferContents(JSContext *cx, HandleObject objArg, void **contents, uint8_t **data)
{
    JSObject *obj = CheckedUnwrap(objArg);
    if (!obj)
        return false;

    if (!obj->is<ArrayBufferObject>()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
        return false;
    }

    Rooted<ArrayBufferObject*> buffer(cx, &obj->as<ArrayBufferObject>());
    if (!ArrayBufferObject::stealContents(cx, buffer, contents, data))
        return false;

    return true;
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

