





#ifndef vm_ArrayBufferObject_h
#define vm_ArrayBufferObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "vm/Runtime.h"

typedef struct JSProperty JSProperty;

namespace js {

class ArrayBufferViewObject;
























































class ArrayBufferObjectMaybeShared;

uint32_t AnyArrayBufferByteLength(const ArrayBufferObjectMaybeShared* buf);
uint8_t* AnyArrayBufferDataPointer(const ArrayBufferObjectMaybeShared* buf);
ArrayBufferObjectMaybeShared& AsAnyArrayBuffer(HandleValue val);

class ArrayBufferObjectMaybeShared : public NativeObject
{
  public:
    uint32_t byteLength() {
        return AnyArrayBufferByteLength(this);
    }

    uint8_t* dataPointer() {
        return AnyArrayBufferDataPointer(this);
    }
};













class ArrayBufferObject : public ArrayBufferObjectMaybeShared
{
    static bool byteLengthGetterImpl(JSContext* cx, CallArgs args);
    static bool fun_slice_impl(JSContext* cx, CallArgs args);

  public:
    static const uint8_t DATA_SLOT = 0;
    static const uint8_t BYTE_LENGTH_SLOT = 1;
    static const uint8_t FIRST_VIEW_SLOT = 2;
    static const uint8_t FLAGS_SLOT = 3;

    static const uint8_t RESERVED_SLOTS = 4;

    static const size_t ARRAY_BUFFER_ALIGNMENT = 8;

    static_assert(FLAGS_SLOT == JS_ARRAYBUFFER_FLAGS_SLOT,
                  "self-hosted code with burned-in constants must get the "
                  "right flags slot");

  public:

    enum OwnsState {
        DoesntOwnData = 0,
        OwnsData = 1,
    };

    enum BufferKind {
        PLAIN               = 0, 
        ASMJS_MALLOCED      = 1,
        ASMJS_MAPPED        = 2,
        MAPPED              = 3,

        KIND_MASK           = 0x3
    };

  protected:

    enum ArrayBufferFlags {
        
        BUFFER_KIND_MASK    = BufferKind::KIND_MASK,

        NEUTERED            = 0x4,

        
        
        
        
        
        
        
        
        
        OWNS_DATA           = 0x8,

        
        
        
        
        FOR_INLINE_TYPED_OBJECT = 0x10,

        
        TYPED_OBJECT_VIEWS  = 0x20
    };

    static_assert(JS_ARRAYBUFFER_NEUTERED_FLAG == NEUTERED,
                  "self-hosted code with burned-in constants must use the "
                  "correct NEUTERED bit value");
  public:

    class BufferContents {
        uint8_t* data_;
        BufferKind kind_;

        friend class ArrayBufferObject;

        BufferContents(uint8_t* data, BufferKind kind) : data_(data), kind_(kind) {
            MOZ_ASSERT((kind_ & ~KIND_MASK) == 0);
        }

      public:

        template<BufferKind Kind>
        static BufferContents create(void* data)
        {
            return BufferContents(static_cast<uint8_t*>(data), Kind);
        }

        static BufferContents createPlain(void* data)
        {
            return BufferContents(static_cast<uint8_t*>(data), PLAIN);
        }

        uint8_t* data() const { return data_; }
        BufferKind kind() const { return kind_; }

        explicit operator bool() const { return data_ != nullptr; }
    };

    static const Class class_;

    static const Class protoClass;
    static const JSFunctionSpec jsfuncs[];
    static const JSFunctionSpec jsstaticfuncs[];

    static bool byteLengthGetter(JSContext* cx, unsigned argc, Value* vp);

    static bool fun_slice(JSContext* cx, unsigned argc, Value* vp);

    static bool fun_isView(JSContext* cx, unsigned argc, Value* vp);
#ifdef NIGHTLY_BUILD
    static bool fun_transfer(JSContext* cx, unsigned argc, Value* vp);
#endif

    static bool class_constructor(JSContext* cx, unsigned argc, Value* vp);

    static ArrayBufferObject* create(JSContext* cx, uint32_t nbytes,
                                     BufferContents contents,
                                     OwnsState ownsState = OwnsData,
                                     NewObjectKind newKind = GenericObject);
    static ArrayBufferObject* create(JSContext* cx, uint32_t nbytes,
                                     NewObjectKind newKind = GenericObject);

    static JSObject* createSlice(JSContext* cx, Handle<ArrayBufferObject*> arrayBuffer,
                                 uint32_t begin, uint32_t end);

    static bool createDataViewForThisImpl(JSContext* cx, CallArgs args);
    static bool createDataViewForThis(JSContext* cx, unsigned argc, Value* vp);

    template<typename T>
    static bool createTypedArrayFromBufferImpl(JSContext* cx, CallArgs args);

    template<typename T>
    static bool createTypedArrayFromBuffer(JSContext* cx, unsigned argc, Value* vp);

    static void trace(JSTracer* trc, JSObject* obj);
    static void objectMoved(JSObject* obj, const JSObject* old);

    static BufferContents stealContents(JSContext* cx,
                                        Handle<ArrayBufferObject*> buffer,
                                        bool hasStealableContents);

    bool hasStealableContents() const {
        
        if (!ownsData())
            return false;

        
        
        
        
        
        return !isNeutered();
    }

    
    
    bool hasMallocedContents() const {
        return (ownsData() && isPlain()) || isAsmJSMalloced();
    }

    static void addSizeOfExcludingThis(JSObject* obj, mozilla::MallocSizeOf mallocSizeOf,
                                       JS::ClassInfo* info);

    
    
    
    
    
    ArrayBufferViewObject* firstView();

    bool addView(JSContext* cx, JSObject* view);

    void setNewOwnedData(FreeOp* fop, BufferContents newContents);
    void changeContents(JSContext* cx, BufferContents newContents);

    



    static bool ensureNonInline(JSContext* cx, Handle<ArrayBufferObject*> buffer);

    
    static MOZ_WARN_UNUSED_RESULT bool
    neuter(JSContext* cx, Handle<ArrayBufferObject*> buffer, BufferContents newContents);

  private:
    void neuterView(JSContext* cx, ArrayBufferViewObject* view,
                    BufferContents newContents);
    void changeViewContents(JSContext* cx, ArrayBufferViewObject* view,
                            uint8_t* oldDataPointer, BufferContents newContents);
    void setFirstView(ArrayBufferViewObject* view);

    uint8_t* inlineDataPointer() const;

  public:
    uint8_t* dataPointer() const;
    size_t byteLength() const;
    BufferContents contents() const {
        return BufferContents(dataPointer(), bufferKind());
    }
    bool hasInlineData() const {
        return dataPointer() == inlineDataPointer();
    }

    void releaseData(FreeOp* fop);

    



    bool hasData() const {
        return getClass() == &class_;
    }

    BufferKind bufferKind() const { return BufferKind(flags() & BUFFER_KIND_MASK); }
    bool isPlain() const { return bufferKind() == PLAIN; }
    bool isAsmJSMapped() const { return bufferKind() == ASMJS_MAPPED; }
    bool isAsmJSMalloced() const { return bufferKind() == ASMJS_MALLOCED; }
    bool isAsmJS() const { return isAsmJSMapped() || isAsmJSMalloced(); }
    bool isMapped() const { return bufferKind() == MAPPED; }
    bool isNeutered() const { return flags() & NEUTERED; }

    static bool prepareForAsmJS(JSContext* cx, Handle<ArrayBufferObject*> buffer,
                                bool usesSignalHandlers);
    static bool prepareForAsmJSNoSignals(JSContext* cx, Handle<ArrayBufferObject*> buffer);

    static void finalize(FreeOp* fop, JSObject* obj);

    static BufferContents createMappedContents(int fd, size_t offset, size_t length);

    static size_t offsetOfFlagsSlot() {
        return getFixedSlotOffset(FLAGS_SLOT);
    }
    static size_t offsetOfDataSlot() {
        return getFixedSlotOffset(DATA_SLOT);
    }

    static uint32_t neuteredFlag() { return NEUTERED; }

    void setForInlineTypedObject() {
        setFlags(flags() | FOR_INLINE_TYPED_OBJECT);
    }
    void setHasTypedObjectViews() {
        setFlags(flags() | TYPED_OBJECT_VIEWS);
    }

    bool forInlineTypedObject() const { return flags() & FOR_INLINE_TYPED_OBJECT; }

  protected:
    void setDataPointer(BufferContents contents, OwnsState ownsState);
    void setByteLength(size_t length);

    uint32_t flags() const;
    void setFlags(uint32_t flags);

    bool ownsData() const { return flags() & OWNS_DATA; }
    void setOwnsData(OwnsState owns) {
        setFlags(owns ? (flags() | OWNS_DATA) : (flags() & ~OWNS_DATA));
    }

    bool hasTypedObjectViews() const { return flags() & TYPED_OBJECT_VIEWS; }

    void setIsAsmJSMalloced() { setFlags((flags() & ~KIND_MASK) | ASMJS_MALLOCED); }
    void setIsNeutered() { setFlags(flags() | NEUTERED); }

    void initialize(size_t byteLength, BufferContents contents, OwnsState ownsState) {
        setByteLength(byteLength);
        setFlags(0);
        setFirstView(nullptr);
        setDataPointer(contents, ownsState);
    }
};







class ArrayBufferViewObject : public JSObject
{
  public:
    static ArrayBufferObject* bufferObject(JSContext* cx, Handle<ArrayBufferViewObject*> obj);

    void neuter(void* newData);

    uint8_t* dataPointer();
    void setDataPointer(uint8_t* data);

    static void trace(JSTracer* trc, JSObject* obj);
};

bool
ToClampedIndex(JSContext* cx, HandleValue v, uint32_t length, uint32_t* out);




bool IsArrayBuffer(HandleValue v);
bool IsArrayBuffer(HandleObject obj);
bool IsArrayBuffer(JSObject* obj);
ArrayBufferObject& AsArrayBuffer(HandleObject obj);
ArrayBufferObject& AsArrayBuffer(JSObject* obj);

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



class InnerViewTable
{
  public:
    typedef Vector<ArrayBufferViewObject*, 1, SystemAllocPolicy> ViewVector;

    friend class ArrayBufferObject;

  private:
    typedef HashMap<JSObject*,
                    ViewVector,
                    DefaultHasher<JSObject*>,
                    SystemAllocPolicy> Map;

    
    
    Map map;

    
    
    Vector<JSObject*, 0, SystemAllocPolicy> nurseryKeys;

    
    bool nurseryKeysValid;

    
    bool sweepEntry(JSObject** pkey, ViewVector& views);

    bool addView(JSContext* cx, ArrayBufferObject* obj, ArrayBufferViewObject* view);
    ViewVector* maybeViewsUnbarriered(ArrayBufferObject* obj);
    void removeViews(ArrayBufferObject* obj);

  public:
    InnerViewTable()
      : nurseryKeysValid(true)
    {}

    
    
    void sweep(JSRuntime* rt);
    void sweepAfterMinorGC(JSRuntime* rt);

    bool needsSweepAfterMinorGC() {
        return !nurseryKeys.empty() || !nurseryKeysValid;
    }

    size_t sizeOfExcludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

extern JSObject*
InitArrayBufferClass(JSContext* cx, HandleObject obj);

} 

template <>
bool
JSObject::is<js::ArrayBufferViewObject>() const;

#endif 
