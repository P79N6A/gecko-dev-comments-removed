





#ifndef vm_ArrayBufferObject_h
#define vm_ArrayBufferObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "vm/Runtime.h"

typedef struct JSProperty JSProperty;

namespace js {

class ArrayBufferViewObject;



















typedef Vector<ArrayBufferObject *, 0, SystemAllocPolicy> ArrayBufferVector;










class ArrayBufferObject : public JSObject
{
    static bool byteLengthGetterImpl(JSContext *cx, CallArgs args);
    static bool fun_slice_impl(JSContext *cx, CallArgs args);

  public:
    static const Class class_;

    static const Class protoClass;
    static const JSFunctionSpec jsfuncs[];
    static const JSFunctionSpec jsstaticfuncs[];

    static bool byteLengthGetter(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_slice(JSContext *cx, unsigned argc, Value *vp);

    static bool fun_isView(JSContext *cx, unsigned argc, Value *vp);

    static bool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    static ArrayBufferObject *create(JSContext *cx, uint32_t nbytes, bool clear = true,
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

    static bool obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                  MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                   MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  MutableHandleObject objp, MutableHandleShape propp);
    static bool obj_lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                  MutableHandleObject objp, MutableHandleShape propp);

    static bool obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static bool obj_defineProperty(JSContext *cx, HandleObject obj,
                                   HandlePropertyName name, HandleValue v,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static bool obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static bool obj_defineSpecial(JSContext *cx, HandleObject obj,
                                  HandleSpecialId sid, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleId id, MutableHandleValue vp);

    static bool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandlePropertyName name, MutableHandleValue vp);

    static bool obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                               uint32_t index, MutableHandleValue vp);

    static bool obj_getSpecial(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleSpecialId sid, MutableHandleValue vp);

    static bool obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                               MutableHandleValue vp, bool strict);
    static bool obj_setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                MutableHandleValue vp, bool strict);
    static bool obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                               MutableHandleValue vp, bool strict);
    static bool obj_setSpecial(JSContext *cx, HandleObject obj,
                               HandleSpecialId sid, MutableHandleValue vp, bool strict);

    static bool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);
    static bool obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);

    static bool obj_deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                   bool *succeeded);
    static bool obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  bool *succeeded);
    static bool obj_deleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                  bool *succeeded);

    static bool obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                              MutableHandleValue statep, MutableHandleId idp);

    static void sweep(JSCompartment *rt);

    static void resetArrayBufferList(JSCompartment *rt);
    static bool saveArrayBufferList(JSCompartment *c, ArrayBufferVector &vector);
    static void restoreArrayBufferLists(ArrayBufferVector &vector);

    static bool stealContents(JSContext *cx, Handle<ArrayBufferObject*> buffer, void **contents,
                              uint8_t **data);

    static void updateElementsHeader(js::ObjectElements *header, uint32_t bytes) {
        header->initializedLength = bytes;

        
        
        
        header->length = 0;
        header->capacity = 0;
    }

    static void initElementsHeader(js::ObjectElements *header, uint32_t bytes) {
        header->flags = 0;
        updateElementsHeader(header, bytes);
    }

    static uint32_t headerInitializedLength(const js::ObjectElements *header) {
        return header->initializedLength;
    }

    void addView(ArrayBufferViewObject *view);

    void changeContents(JSContext *cx, ObjectElements *newHeader);

    



    static bool ensureNonInline(JSContext *cx, Handle<ArrayBufferObject*> buffer);

    uint32_t byteLength() const {
        return getElementsHeader()->initializedLength;
    }

    


    static bool neuterViews(JSContext *cx, Handle<ArrayBufferObject*> buffer);

    uint8_t * dataPointer() const;

    



    void neuter(JSContext *cx);

    



    bool hasData() const {
        return getClass() == &class_;
    }

    bool isSharedArrayBuffer() const {
        return getElementsHeader()->isSharedArrayBuffer();
    }

    bool isAsmJSArrayBuffer() const {
        return getElementsHeader()->isAsmJSArrayBuffer();
    }
    bool isNeutered() const {
        return getElementsHeader()->isNeuteredBuffer();
    }
    static bool prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer);
    static bool neuterAsmJSArrayBuffer(JSContext *cx, ArrayBufferObject &buffer);
    static void releaseAsmJSArrayBuffer(FreeOp *fop, JSObject *obj);
};







class ArrayBufferViewObject : public JSObject
{
  protected:
    
    static const size_t BYTEOFFSET_SLOT  = JS_TYPEDOBJ_SLOT_BYTEOFFSET;

    
    static const size_t BYTELENGTH_SLOT  = JS_TYPEDOBJ_SLOT_BYTELENGTH;

    
    static const size_t BUFFER_SLOT      = JS_TYPEDOBJ_SLOT_OWNER;

    
    static const size_t NEXT_VIEW_SLOT   = JS_TYPEDOBJ_SLOT_NEXT_VIEW;

    




    static const size_t NEXT_BUFFER_SLOT = JS_TYPEDOBJ_SLOT_NEXT_BUFFER;

  public:
    JSObject *bufferObject() const {
        return &getFixedSlot(BUFFER_SLOT).toObject();
    }

    ArrayBufferObject *bufferLink() {
        return static_cast<ArrayBufferObject*>(getFixedSlot(NEXT_BUFFER_SLOT).toPrivate());
    }

    inline void setBufferLink(ArrayBufferObject *buffer);

    ArrayBufferViewObject *nextView() const {
        return static_cast<ArrayBufferViewObject*>(getFixedSlot(NEXT_VIEW_SLOT).toPrivate());
    }

    inline void setNextView(ArrayBufferViewObject *view);

    void prependToViews(ArrayBufferViewObject *viewsHead);

    void neuter(JSContext *cx);

    static void trace(JSTracer *trc, JSObject *obj);
};

bool
ToClampedIndex(JSContext *cx, HandleValue v, uint32_t length, uint32_t *out);

inline void
PostBarrierTypedArrayObject(JSObject *obj)
{
#ifdef JSGC_GENERATIONAL
    JS_ASSERT(obj);
    JSRuntime *rt = obj->runtimeFromMainThread();
    if (!rt->isHeapBusy() && !IsInsideNursery(rt, obj))
        rt->gcStoreBuffer.putWholeCell(obj);
#endif
}

inline void
InitArrayBufferViewDataPointer(ArrayBufferViewObject *obj, ArrayBufferObject *buffer, size_t byteOffset)
{
    




    obj->initPrivate(buffer->dataPointer() + byteOffset);
    PostBarrierTypedArrayObject(obj);
}





bool IsArrayBuffer(HandleValue v);
bool IsArrayBuffer(HandleObject obj);
bool IsArrayBuffer(JSObject *obj);
ArrayBufferObject &AsArrayBuffer(HandleObject obj);
ArrayBufferObject &AsArrayBuffer(JSObject *obj);

inline void
ArrayBufferViewObject::setBufferLink(ArrayBufferObject *buffer)
{
    setFixedSlot(NEXT_BUFFER_SLOT, PrivateValue(buffer));
    PostBarrierTypedArrayObject(this);
}

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

    
    uint8_clamped(uint8_t x)    { *this = x; }
    uint8_clamped(uint16_t x)   { *this = x; }
    uint8_clamped(uint32_t x)   { *this = x; }
    uint8_clamped(int8_t x)     { *this = x; }
    uint8_clamped(int16_t x)    { *this = x; }
    uint8_clamped(int32_t x)    { *this = x; }
    uint8_clamped(double x)     { *this = x; }

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


template<typename T> inline const bool TypeIsFloatingPoint() { return false; }
template<> inline const bool TypeIsFloatingPoint<float>() { return true; }
template<> inline const bool TypeIsFloatingPoint<double>() { return true; }

template<typename T> inline const bool TypeIsUnsigned() { return false; }
template<> inline const bool TypeIsUnsigned<uint8_t>() { return true; }
template<> inline const bool TypeIsUnsigned<uint16_t>() { return true; }
template<> inline const bool TypeIsUnsigned<uint32_t>() { return true; }

} 

#endif 
