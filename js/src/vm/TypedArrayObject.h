





#ifndef vm_TypedArrayObject_h
#define vm_TypedArrayObject_h

#include "jsapi.h"
#include "jsclass.h"
#include "jsobj.h"

#include "gc/Barrier.h"

typedef struct JSProperty JSProperty;

namespace js {

typedef Vector<ArrayBufferObject *, 0, SystemAllocPolicy> ArrayBufferVector;

















class ArrayBufferViewObject;










class ArrayBufferObject : public JSObject
{
    static bool byteLengthGetterImpl(JSContext *cx, CallArgs args);
    static bool fun_slice_impl(JSContext *cx, CallArgs args);

  public:
    static Class class_;

    static Class protoClass;
    static const JSFunctionSpec jsfuncs[];

    static JSBool byteLengthGetter(JSContext *cx, unsigned argc, Value *vp);

    static JSBool fun_slice(JSContext *cx, unsigned argc, Value *vp);

    static JSBool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *create(JSContext *cx, uint32_t nbytes, uint8_t *contents = NULL);

    static JSObject *createSlice(JSContext *cx, ArrayBufferObject &arrayBuffer,
                                 uint32_t begin, uint32_t end);

    static bool createDataViewForThisImpl(JSContext *cx, CallArgs args);
    static JSBool createDataViewForThis(JSContext *cx, unsigned argc, Value *vp);

    template<typename T>
    static bool createTypedArrayFromBufferImpl(JSContext *cx, CallArgs args);

    template<typename T>
    static JSBool createTypedArrayFromBuffer(JSContext *cx, unsigned argc, Value *vp);

    static void obj_trace(JSTracer *trc, JSObject *obj);

    static JSBool obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                    MutableHandleObject objp, MutableHandleShape propp);
    static JSBool obj_lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                     MutableHandleObject objp, MutableHandleShape propp);
    static JSBool obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                    MutableHandleObject objp, MutableHandleShape propp);
    static JSBool obj_lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                    MutableHandleObject objp, MutableHandleShape propp);

    static JSBool obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                    PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool obj_defineProperty(JSContext *cx, HandleObject obj,
                                     HandlePropertyName name, HandleValue v,
                                     PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                    PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool obj_defineSpecial(JSContext *cx, HandleObject obj,
                                    HandleSpecialId sid, HandleValue v,
                                    PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static JSBool obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                                 HandleId id, MutableHandleValue vp);

    static JSBool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                  HandlePropertyName name, MutableHandleValue vp);

    static JSBool obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                                 uint32_t index, MutableHandleValue vp);
    static JSBool obj_getElementIfPresent(JSContext *cx, HandleObject obj, HandleObject receiver,
                                          uint32_t index, MutableHandleValue vp, bool *present);

    static JSBool obj_getSpecial(JSContext *cx, HandleObject obj, HandleObject receiver,
                                 HandleSpecialId sid, MutableHandleValue vp);

    static JSBool obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                 MutableHandleValue vp, JSBool strict);
    static JSBool obj_setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                  MutableHandleValue vp, JSBool strict);
    static JSBool obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                                 MutableHandleValue vp, JSBool strict);
    static JSBool obj_setSpecial(JSContext *cx, HandleObject obj,
                                 HandleSpecialId sid, MutableHandleValue vp, JSBool strict);

    static JSBool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                           HandleId id, unsigned *attrsp);
    static JSBool obj_getPropertyAttributes(JSContext *cx, HandleObject obj,
                                            HandlePropertyName name, unsigned *attrsp);
    static JSBool obj_getElementAttributes(JSContext *cx, HandleObject obj,
                                           uint32_t index, unsigned *attrsp);
    static JSBool obj_getSpecialAttributes(JSContext *cx, HandleObject obj,
                                           HandleSpecialId sid, unsigned *attrsp);

    static JSBool obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                           HandleId id, unsigned *attrsp);
    static JSBool obj_setPropertyAttributes(JSContext *cx, HandleObject obj,
                                            HandlePropertyName name, unsigned *attrsp);
    static JSBool obj_setElementAttributes(JSContext *cx, HandleObject obj,
                                           uint32_t index, unsigned *attrsp);
    static JSBool obj_setSpecialAttributes(JSContext *cx, HandleObject obj,
                                           HandleSpecialId sid, unsigned *attrsp);

    static JSBool obj_deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                     JSBool *succeeded);
    static JSBool obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                    JSBool *succeeded);
    static JSBool obj_deleteSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                    JSBool *succeeded);

    static JSBool obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                                MutableHandleValue statep, MutableHandleId idp);

    static void sweep(JSCompartment *rt);

    static void resetArrayBufferList(JSCompartment *rt);
    static bool saveArrayBufferList(JSCompartment *c, ArrayBufferVector &vector);
    static void restoreArrayBufferLists(ArrayBufferVector &vector);

    static bool stealContents(JSContext *cx, JSObject *obj, void **contents,
                              uint8_t **data);

    static void setElementsHeader(js::ObjectElements *header, uint32_t bytes) {
        header->flags = 0;
        header->initializedLength = bytes;

        
        
        header->length = 0;
        header->capacity = 0;
    }

    static uint32_t headerInitializedLength(const js::ObjectElements *header) {
        return header->initializedLength;
    }

    void addView(ArrayBufferViewObject *view);

    bool allocateSlots(JSContext *cx, uint32_t size, uint8_t *contents = NULL);
    void changeContents(JSContext *cx, ObjectElements *newHeader);

    



    bool uninlineData(JSContext *cx);

    uint32_t byteLength() const {
        return getElementsHeader()->initializedLength;
    }

    inline uint8_t * dataPointer() const {
        return (uint8_t *) elements;
    }

    



    bool hasData() const {
        return getClass() == &class_;
    }

    bool isAsmJSArrayBuffer() const {
        return getElementsHeader()->isAsmJSArrayBuffer();
    }
    static bool prepareForAsmJS(JSContext *cx, Handle<ArrayBufferObject*> buffer);
    static void neuterAsmJSArrayBuffer(ArrayBufferObject &buffer);
    static void releaseAsmJSArrayBuffer(FreeOp *fop, JSObject *obj);
};







class ArrayBufferViewObject : public JSObject
{
  protected:
    
    static const size_t BYTEOFFSET_SLOT  = 0;

    
    static const size_t BYTELENGTH_SLOT  = 1;

    
    static const size_t BUFFER_SLOT      = 2;

    
    static const size_t NEXT_VIEW_SLOT   = 3;

    




    static const size_t NEXT_BUFFER_SLOT = 4;

    static const size_t NUM_SLOTS        = 5;

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

    void prependToViews(HeapPtr<ArrayBufferViewObject> *views);

    void neuter();

    static void trace(JSTracer *trc, JSObject *obj);
};









class TypedArrayObject : public ArrayBufferViewObject
{
  protected:
    
    
    static const size_t LENGTH_SLOT    = ArrayBufferViewObject::NUM_SLOTS;
    static const size_t TYPE_SLOT      = ArrayBufferViewObject::NUM_SLOTS + 1;
    static const size_t RESERVED_SLOTS = ArrayBufferViewObject::NUM_SLOTS + 2;
    static const size_t DATA_SLOT      = 7; 

  public:
    enum {
        TYPE_INT8 = 0,
        TYPE_UINT8,
        TYPE_INT16,
        TYPE_UINT16,
        TYPE_INT32,
        TYPE_UINT32,
        TYPE_FLOAT32,
        TYPE_FLOAT64,

        



        TYPE_UINT8_CLAMPED,

        TYPE_MAX
    };

    static Class classes[TYPE_MAX];
    static Class protoClasses[TYPE_MAX];

    static JSBool obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                    MutableHandleObject objp, MutableHandleShape propp);
    static JSBool obj_lookupProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                     MutableHandleObject objp, MutableHandleShape propp);
    static JSBool obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                    MutableHandleObject objp, MutableHandleShape propp);
    static JSBool obj_lookupSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid,
                                    MutableHandleObject objp, MutableHandleShape propp);

    static JSBool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                           HandleId id, unsigned *attrsp);
    static JSBool obj_getPropertyAttributes(JSContext *cx, HandleObject obj,
                                            HandlePropertyName name, unsigned *attrsp);
    static JSBool obj_getElementAttributes(JSContext *cx, HandleObject obj,
                                           uint32_t index, unsigned *attrsp);
    static JSBool obj_getSpecialAttributes(JSContext *cx, HandleObject obj,
                                           HandleSpecialId sid, unsigned *attrsp);

    static JSBool obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                           HandleId id, unsigned *attrsp);
    static JSBool obj_setPropertyAttributes(JSContext *cx, HandleObject obj,
                                            HandlePropertyName name, unsigned *attrsp);
    static JSBool obj_setElementAttributes(JSContext *cx, HandleObject obj,
                                           uint32_t index, unsigned *attrsp);
    static JSBool obj_setSpecialAttributes(JSContext *cx, HandleObject obj,
                                           HandleSpecialId sid, unsigned *attrsp);

    static Value bufferValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(BUFFER_SLOT);
    }
    static Value byteOffsetValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(BYTEOFFSET_SLOT);
    }
    static Value byteLengthValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(BYTELENGTH_SLOT);
    }
    static Value lengthValue(TypedArrayObject *tarr) {
        return tarr->getFixedSlot(LENGTH_SLOT);
    }

    ArrayBufferObject *buffer() const {
        return &bufferValue(const_cast<TypedArrayObject*>(this)).toObject().as<ArrayBufferObject>();
    }
    uint32_t byteOffset() const {
        return byteOffsetValue(const_cast<TypedArrayObject*>(this)).toInt32();
    }
    uint32_t byteLength() const {
        return byteLengthValue(const_cast<TypedArrayObject*>(this)).toInt32();
    }
    uint32_t length() const {
        return lengthValue(const_cast<TypedArrayObject*>(this)).toInt32();
    }

    uint32_t type() const {
        return getFixedSlot(TYPE_SLOT).toInt32();
    }
    void *viewData() const {
        return static_cast<void*>(getPrivate(DATA_SLOT));
    }

    inline bool isArrayIndex(jsid id, uint32_t *ip = NULL);
    void copyTypedArrayElement(uint32_t index, MutableHandleValue vp);

    void neuter();

    static uint32_t slotWidth(int atype) {
        switch (atype) {
          case js::TypedArrayObject::TYPE_INT8:
          case js::TypedArrayObject::TYPE_UINT8:
          case js::TypedArrayObject::TYPE_UINT8_CLAMPED:
            return 1;
          case js::TypedArrayObject::TYPE_INT16:
          case js::TypedArrayObject::TYPE_UINT16:
            return 2;
          case js::TypedArrayObject::TYPE_INT32:
          case js::TypedArrayObject::TYPE_UINT32:
          case js::TypedArrayObject::TYPE_FLOAT32:
            return 4;
          case js::TypedArrayObject::TYPE_FLOAT64:
            return 8;
          default:
            MOZ_ASSUME_UNREACHABLE("invalid typed array type");
        }
    }

    int slotWidth() {
        return slotWidth(type());
    }

    



    static const uint32_t SINGLETON_TYPE_BYTE_LENGTH = 1024 * 1024 * 10;

    static int lengthOffset();
    static int dataOffset();
};

inline bool
IsTypedArrayClass(const Class *clasp)
{
    return &TypedArrayObject::classes[0] <= clasp &&
           clasp < &TypedArrayObject::classes[TypedArrayObject::TYPE_MAX];
}

inline bool
IsTypedArrayProtoClass(const Class *clasp)
{
    return &TypedArrayObject::protoClasses[0] <= clasp &&
           clasp < &TypedArrayObject::protoClasses[TypedArrayObject::TYPE_MAX];
}

bool
IsTypedArrayConstructor(const Value &v, uint32_t type);

bool
IsTypedArrayBuffer(const Value &v);

static inline unsigned
TypedArrayShift(ArrayBufferView::ViewType viewType)
{
    switch (viewType) {
      case ArrayBufferView::TYPE_INT8:
      case ArrayBufferView::TYPE_UINT8:
      case ArrayBufferView::TYPE_UINT8_CLAMPED:
        return 0;
      case ArrayBufferView::TYPE_INT16:
      case ArrayBufferView::TYPE_UINT16:
        return 1;
      case ArrayBufferView::TYPE_INT32:
      case ArrayBufferView::TYPE_UINT32:
      case ArrayBufferView::TYPE_FLOAT32:
        return 2;
      case ArrayBufferView::TYPE_FLOAT64:
        return 3;
      default:;
    }
    MOZ_ASSUME_UNREACHABLE("Unexpected array type");
}

class DataViewObject : public ArrayBufferViewObject
{
    static const size_t RESERVED_SLOTS = ArrayBufferViewObject::NUM_SLOTS;
    static const size_t DATA_SLOT      = 7; 

  private:
    static Class protoClass;

    static bool is(const Value &v) {
        return v.isObject() && v.toObject().hasClass(&class_);
    }

    template<Value ValueGetter(DataViewObject *view)>
    static bool
    getterImpl(JSContext *cx, CallArgs args);

    template<Value ValueGetter(DataViewObject *view)>
    static JSBool
    getter(JSContext *cx, unsigned argc, Value *vp);

    template<Value ValueGetter(DataViewObject *view)>
    static bool
    defineGetter(JSContext *cx, PropertyName *name, HandleObject proto);

  public:
    static Class class_;

    static Value byteOffsetValue(DataViewObject *view) {
        Value v = view->getReservedSlot(BYTEOFFSET_SLOT);
        JS_ASSERT(v.toInt32() >= 0);
        return v;
    }

    static Value byteLengthValue(DataViewObject *view) {
        Value v = view->getReservedSlot(BYTELENGTH_SLOT);
        JS_ASSERT(v.toInt32() >= 0);
        return v;
    }

    uint32_t byteOffset() const {
        return byteOffsetValue(const_cast<DataViewObject*>(this)).toInt32();
    }

    uint32_t byteLength() const {
        return byteLengthValue(const_cast<DataViewObject*>(this)).toInt32();
    }

    bool hasBuffer() const {
        return getReservedSlot(BUFFER_SLOT).isObject();
    }

    ArrayBufferObject &arrayBuffer() const {
        return getReservedSlot(BUFFER_SLOT).toObject().as<ArrayBufferObject>();
    }

    static Value bufferValue(DataViewObject *view) {
        return view->hasBuffer() ? ObjectValue(view->arrayBuffer()) : UndefinedValue();
    }

    void *dataPointer() const {
        return getPrivate();
    }

    static JSBool class_constructor(JSContext *cx, unsigned argc, Value *vp);
    static JSBool constructWithProto(JSContext *cx, unsigned argc, Value *vp);
    static JSBool construct(JSContext *cx, JSObject *bufobj, const CallArgs &args,
                            HandleObject proto);

    static inline DataViewObject *
    create(JSContext *cx, uint32_t byteOffset, uint32_t byteLength,
           Handle<ArrayBufferObject*> arrayBuffer, JSObject *proto);

    static bool getInt8Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getInt8(JSContext *cx, unsigned argc, Value *vp);

    static bool getUint8Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getUint8(JSContext *cx, unsigned argc, Value *vp);

    static bool getInt16Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getInt16(JSContext *cx, unsigned argc, Value *vp);

    static bool getUint16Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getUint16(JSContext *cx, unsigned argc, Value *vp);

    static bool getInt32Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getInt32(JSContext *cx, unsigned argc, Value *vp);

    static bool getUint32Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getUint32(JSContext *cx, unsigned argc, Value *vp);

    static bool getFloat32Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getFloat32(JSContext *cx, unsigned argc, Value *vp);

    static bool getFloat64Impl(JSContext *cx, CallArgs args);
    static JSBool fun_getFloat64(JSContext *cx, unsigned argc, Value *vp);

    static bool setInt8Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setInt8(JSContext *cx, unsigned argc, Value *vp);

    static bool setUint8Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setUint8(JSContext *cx, unsigned argc, Value *vp);

    static bool setInt16Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setInt16(JSContext *cx, unsigned argc, Value *vp);

    static bool setUint16Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setUint16(JSContext *cx, unsigned argc, Value *vp);

    static bool setInt32Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setInt32(JSContext *cx, unsigned argc, Value *vp);

    static bool setUint32Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setUint32(JSContext *cx, unsigned argc, Value *vp);

    static bool setFloat32Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setFloat32(JSContext *cx, unsigned argc, Value *vp);

    static bool setFloat64Impl(JSContext *cx, CallArgs args);
    static JSBool fun_setFloat64(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *initClass(JSContext *cx);
    static bool getDataPointer(JSContext *cx, Handle<DataViewObject*> obj,
                               CallArgs args, size_t typeSize, uint8_t **data);
    template<typename NativeType>
    static bool read(JSContext *cx, Handle<DataViewObject*> obj,
                     CallArgs &args, NativeType *val, const char *method);
    template<typename NativeType>
    static bool write(JSContext *cx, Handle<DataViewObject*> obj,
                      CallArgs &args, const char *method);

    void neuter();

  private:
    static const JSFunctionSpec jsfuncs[];
};

static inline int32_t
ClampIntForUint8Array(int32_t x)
{
    if (x < 0)
        return 0;
    if (x > 255)
        return 255;
    return x;
}

} 

template <>
inline bool
JSObject::is<js::TypedArrayObject>() const
{
    return js::IsTypedArrayClass(getClass());
}

template <>
inline bool
JSObject::is<js::ArrayBufferViewObject>() const
{
    return is<js::DataViewObject>() || is<js::TypedArrayObject>();
}

#endif
