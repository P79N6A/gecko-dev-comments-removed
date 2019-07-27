





#ifndef vm_ArrayBufferObject_h
#define vm_ArrayBufferObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "vm/Runtime.h"

typedef struct JSProperty JSProperty;

namespace js {

class ArrayBufferViewObject;








































typedef Vector<ArrayBufferObject *, 0, SystemAllocPolicy> ArrayBufferVector;

class ArrayBufferObjectMaybeShared;

uint32_t AnyArrayBufferByteLength(const ArrayBufferObjectMaybeShared *buf);
uint8_t *AnyArrayBufferDataPointer(const ArrayBufferObjectMaybeShared *buf);
ArrayBufferObjectMaybeShared &AsAnyArrayBuffer(HandleValue val);

class ArrayBufferObjectMaybeShared : public JSObject
{
  public:
    uint32_t byteLength() {
        return AnyArrayBufferByteLength(this);
    }

    uint8_t *dataPointer() {
        return AnyArrayBufferDataPointer(this);
    }
};













class ArrayBufferObject : public ArrayBufferObjectMaybeShared
{
    static bool byteLengthGetterImpl(JSContext *cx, CallArgs args);
    static bool fun_slice_impl(JSContext *cx, CallArgs args);

  public:
    static const uint8_t DATA_SLOT = 0;
    static const uint8_t BYTE_LENGTH_SLOT = 1;
    static const uint8_t VIEW_LIST_SLOT = 2;
    static const uint8_t FLAGS_SLOT = 3;

    static const uint8_t RESERVED_SLOTS = 4;

    static const size_t ARRAY_BUFFER_ALIGNMENT = 8;

  public:

    enum BufferKind {
        PLAIN_BUFFER        =   0, 
        ASMJS_BUFFER        = 0x1,
        MAPPED_BUFFER       = 0x2,
        KIND_MASK           = ASMJS_BUFFER | MAPPED_BUFFER
    };

  protected:

    enum ArrayBufferFlags {
        
        BUFFER_KIND_MASK    = BufferKind::KIND_MASK,

        NEUTERED_BUFFER     = 0x8,

        
        IN_LIVE_LIST        =  0x10,

        
        
        
        
        OWNS_DATA           =  0x20,
    };

  public:

    class BufferContents {
        uint8_t *data_;
        BufferKind kind_;

        friend class ArrayBufferObject;

        typedef void (BufferContents::* ConvertibleToBool)();
        void nonNull() {}

        BufferContents(uint8_t *data, BufferKind kind) : data_(data), kind_(kind) {
            MOZ_ASSERT((kind_ & ~KIND_MASK) == 0);
        }

      public:

        template<BufferKind Kind>
        static BufferContents create(void *data)
        {
            return BufferContents(static_cast<uint8_t*>(data), Kind);
        }

        static BufferContents createUnowned(void *data)
        {
            return BufferContents(static_cast<uint8_t*>(data), PLAIN_BUFFER);
        }

        uint8_t *data() const { return data_; }
        BufferKind kind() const { return kind_; }

        operator ConvertibleToBool() const { return data_ ? &BufferContents::nonNull : nullptr; }
    };

    static const Class class_;

    static const Class protoClass;
    static const JSFunctionSpec jsfuncs[];
    static const JSFunctionSpec jsstaticfuncs[];

    static bool byteLengthGetter(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_slice(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_isView(JSContext *cx, unsigned argc, Value *vp);

    static bool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    static ArrayBufferObject *create(JSContext *cx, uint32_t nbytes,
                                     BufferContents contents,
                                     NewObjectKind newKind = GenericObject);
    static ArrayBufferObject *create(JSContext *cx, uint32_t nbytes,
                                     NewObjectKind newKind = GenericObject);

    static JSObject *createSlice(JSContext *cx, Handle<ArrayBufferObject*> arrayBuffer,
                                 uint32_t begin, uint32_t end);

    static bool createDataViewForThisImpl(JSContext *cx, CallArgs args);
    static bool createDataViewForThis(JSContext *cx, unsigned argc, Value *vp);

    template<typename T>
    static bool createTypedArrayFromBufferImpl(JSContext *cx, CallArgs args);

    template<typename T>
    static bool createTypedArrayFromBuffer(JSContext *cx, unsigned argc, Value *vp);

    static void obj_trace(JSTracer *trc, JSObject *obj);

    static void sweep(JSCompartment *rt);

    static void objectMoved(JSObject *obj, const JSObject *old);

    static void resetArrayBufferList(JSCompartment *rt);
    static bool saveArrayBufferList(JSCompartment *c, ArrayBufferVector &vector);
    static void restoreArrayBufferLists(ArrayBufferVector &vector);

    static BufferContents stealContents(JSContext *cx, Handle<ArrayBufferObject*> buffer);

    bool hasStealableContents() const {
        
        if (!ownsData())
            return false;

        
        
        if (isAsmJSArrayBuffer())
            return false;

        
        
        
        
        
        return !isNeutered();
    }

    static void addSizeOfExcludingThis(JSObject *obj, mozilla::MallocSizeOf mallocSizeOf,
                                       JS::ClassInfo *info);

    void addView(ArrayBufferViewObject *view);

    void setNewOwnedData(FreeOp* fop, BufferContents newContents);
    void changeContents(JSContext *cx, BufferContents newContents);

    



    static bool ensureNonInline(JSContext *cx, Handle<ArrayBufferObject*> buffer);

    bool canNeuter(JSContext *cx);

    
    static void neuter(JSContext *cx, Handle<ArrayBufferObject*> buffer, BufferContents newContents);

    uint8_t *dataPointer() const;
    size_t byteLength() const;
    BufferContents contents() const {
        return BufferContents(dataPointer(), bufferKind());
    }

    void releaseData(FreeOp *fop);

    



    bool hasData() const {
        return getClass() == &class_;
    }

    BufferKind bufferKind() const { return BufferKind(flags() & BUFFER_KIND_MASK); }
    bool isAsmJSArrayBuffer() const { return flags() & ASMJS_BUFFER; }
    bool isMappedArrayBuffer() const { return flags() & MAPPED_BUFFER; }
    bool isNeutered() const { return flags() & NEUTERED_BUFFER; }

    static bool prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer,
                                bool usesSignalHandlers);
    static bool prepareForAsmJSNoSignals(JSContext *cx, Handle<ArrayBufferObject*> buffer);
    static bool canNeuterAsmJSArrayBuffer(JSContext *cx, ArrayBufferObject &buffer);

    static void finalize(FreeOp *fop, JSObject *obj);

    static BufferContents createMappedContents(int fd, size_t offset, size_t length);

    static size_t flagsOffset() {
        return getFixedSlotOffset(FLAGS_SLOT);
    }

    static uint32_t neuteredFlag() { return NEUTERED_BUFFER; }

  protected:
    enum OwnsState {
        DoesntOwnData = 0,
        OwnsData = 1,
    };

    void setDataPointer(BufferContents contents, OwnsState ownsState);
    void setByteLength(size_t length);

    ArrayBufferViewObject *viewList() const;
    void setViewList(ArrayBufferViewObject *viewsHead);
    void setViewListNoBarrier(ArrayBufferViewObject *viewsHead);

    uint32_t flags() const;
    void setFlags(uint32_t flags);

    bool inLiveList() const { return flags() & IN_LIVE_LIST; }
    void setInLiveList(bool value) {
        setFlags(value ? (flags() | IN_LIVE_LIST) : (flags() & ~IN_LIVE_LIST));
    }

    bool ownsData() const { return flags() & OWNS_DATA; }
    void setOwnsData(OwnsState owns) {
        setFlags(owns ? (flags() | OWNS_DATA) : (flags() & ~OWNS_DATA));
    }

    void setIsAsmJSArrayBuffer() { setFlags(flags() | ASMJS_BUFFER); }
    void setIsMappedArrayBuffer() { setFlags(flags() | MAPPED_BUFFER); }
    void setIsNeutered() { setFlags(flags() | NEUTERED_BUFFER); }

    void initialize(size_t byteLength, BufferContents contents, OwnsState ownsState) {
        setByteLength(byteLength);
        setFlags(0);
        setViewListNoBarrier(nullptr);
        setDataPointer(contents, ownsState);
    }

    void releaseAsmJSArray(FreeOp *fop);
    void releaseAsmJSArrayNoSignals(FreeOp *fop);
    void releaseMappedArray();
};







class ArrayBufferViewObject : public JSObject
{
  protected:
    
    static const size_t BYTEOFFSET_SLOT  = JS_BUFVIEW_SLOT_BYTEOFFSET;

    
    static const size_t LENGTH_SLOT      = JS_BUFVIEW_SLOT_LENGTH;

    
    static const size_t BUFFER_SLOT      = JS_BUFVIEW_SLOT_OWNER;

    
    static const size_t NEXT_VIEW_SLOT   = JS_BUFVIEW_SLOT_NEXT_VIEW;

  public:
    static ArrayBufferObject *bufferObject(JSContext *cx, Handle<ArrayBufferViewObject *> obj);

    ArrayBufferViewObject *nextView() const {
        return static_cast<ArrayBufferViewObject*>(getFixedSlot(NEXT_VIEW_SLOT).toPrivate());
    }

    inline void setNextView(ArrayBufferViewObject *view);

    void neuter(void *newData);

    static void trace(JSTracer *trc, JSObject *obj);

    uint8_t *dataPointer() {
        return static_cast<uint8_t *>(getPrivate());
    }
};

bool
ToClampedIndex(JSContext *cx, HandleValue v, uint32_t length, uint32_t *out);

inline void
PostBarrierTypedArrayObject(JSObject *obj)
{
#ifdef JSGC_GENERATIONAL
    JS_ASSERT(obj);
    JSRuntime *rt = obj->runtimeFromMainThread();
    if (!rt->isHeapBusy() && !IsInsideNursery(JS::AsCell(obj)))
        rt->gc.storeBuffer.putWholeCellFromMainThread(obj);
#endif
}

inline void
InitArrayBufferViewDataPointer(ArrayBufferViewObject *obj, ArrayBufferObject *buffer, size_t byteOffset)
{
    




    MOZ_ASSERT(buffer->dataPointer() != nullptr);
    obj->initPrivate(buffer->dataPointer() + byteOffset);

    PostBarrierTypedArrayObject(obj);
}




bool IsArrayBuffer(HandleValue v);
bool IsArrayBuffer(HandleObject obj);
bool IsArrayBuffer(JSObject *obj);
ArrayBufferObject &AsArrayBuffer(HandleObject obj);
ArrayBufferObject &AsArrayBuffer(JSObject *obj);

inline void
ArrayBufferViewObject::setNextView(ArrayBufferViewObject *view)
{
    setFixedSlot(NEXT_VIEW_SLOT, PrivateValue(view));
    PostBarrierTypedArrayObject(this);
}

extern uint32_t JS_FASTCALL
ClampDoubleToUint8(const double x);

struct uint8_clamped {
    uint8_t val;

    uint8_clamped() { }
    uint8_clamped(const uint8_clamped& other) : val(other.val) { }

    
    explicit uint8_clamped(uint8_t x)    { *this = x; }
    explicit uint8_clamped(uint16_t x)   { *this = x; }
    explicit uint8_clamped(uint32_t x)   { *this = x; }
    explicit uint8_clamped(int8_t x)     { *this = x; }
    explicit uint8_clamped(int16_t x)    { *this = x; }
    explicit uint8_clamped(int32_t x)    { *this = x; }
    explicit uint8_clamped(double x)     { *this = x; }

    uint8_clamped& operator=(const uint8_clamped& x) {
        val = x.val;
        return *this;
    }

    uint8_clamped& operator=(uint8_t x) {
        val = x;
        return *this;
    }

    uint8_clamped& operator=(uint16_t x) {
        val = (x > 255) ? 255 : uint8_t(x);
        return *this;
    }

    uint8_clamped& operator=(uint32_t x) {
        val = (x > 255) ? 255 : uint8_t(x);
        return *this;
    }

    uint8_clamped& operator=(int8_t x) {
        val = (x >= 0) ? uint8_t(x) : 0;
        return *this;
    }

    uint8_clamped& operator=(int16_t x) {
        val = (x >= 0)
              ? ((x < 255)
                 ? uint8_t(x)
                 : 255)
              : 0;
        return *this;
    }

    uint8_clamped& operator=(int32_t x) {
        val = (x >= 0)
              ? ((x < 255)
                 ? uint8_t(x)
                 : 255)
              : 0;
        return *this;
    }

    uint8_clamped& operator=(const double x) {
        val = uint8_t(ClampDoubleToUint8(x));
        return *this;
    }

    operator uint8_t() const {
        return val;
    }

    void staticAsserts() {
        static_assert(sizeof(uint8_clamped) == 1,
                      "uint8_clamped must be layout-compatible with uint8_t");
    }
};


template<typename T> inline bool TypeIsFloatingPoint() { return false; }
template<> inline bool TypeIsFloatingPoint<float>() { return true; }
template<> inline bool TypeIsFloatingPoint<double>() { return true; }

template<typename T> inline bool TypeIsUnsigned() { return false; }
template<> inline bool TypeIsUnsigned<uint8_t>() { return true; }
template<> inline bool TypeIsUnsigned<uint16_t>() { return true; }
template<> inline bool TypeIsUnsigned<uint32_t>() { return true; }

} 

#endif 
