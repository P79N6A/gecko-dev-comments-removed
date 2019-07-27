





#ifndef builtin_TypedObject_h
#define builtin_TypedObject_h

#include "jsobj.h"
#include "jsweakmap.h"

#include "builtin/TypedObjectConstants.h"
#include "js/Conversions.h"
#include "vm/ArrayBufferObject.h"














































































namespace js {







template <typename T>
static T ConvertScalar(double d)
{
    if (TypeIsFloatingPoint<T>()) {
        return T(d);
    } else if (TypeIsUnsigned<T>()) {
        uint32_t n = JS::ToUint32(d);
        return T(n);
    } else {
        int32_t n = JS::ToInt32(d);
        return T(n);
    }
}

namespace type {

enum Kind {
    Scalar = JS_TYPEREPR_SCALAR_KIND,
    Reference = JS_TYPEREPR_REFERENCE_KIND,
    Simd = JS_TYPEREPR_SIMD_KIND,
    Struct = JS_TYPEREPR_STRUCT_KIND,
    Array = JS_TYPEREPR_ARRAY_KIND
};

}




class SimpleTypeDescr;
class ComplexTypeDescr;
class SimdTypeDescr;
class StructTypeDescr;
class TypedProto;




class TypedProto : public NativeObject
{
  public:
    static const Class class_;
};

class TypeDescr : public NativeObject
{
  public:
    TypedProto &typedProto() const {
        return getReservedSlot(JS_DESCR_SLOT_TYPROTO).toObject().as<TypedProto>();
    }

    JSAtom &stringRepr() const {
        return getReservedSlot(JS_DESCR_SLOT_STRING_REPR).toString()->asAtom();
    }

    type::Kind kind() const {
        return (type::Kind) getReservedSlot(JS_DESCR_SLOT_KIND).toInt32();
    }

    bool opaque() const {
        return getReservedSlot(JS_DESCR_SLOT_OPAQUE).toBoolean();
    }

    bool transparent() const {
        return !opaque();
    }

    int32_t alignment() const {
        return getReservedSlot(JS_DESCR_SLOT_ALIGNMENT).toInt32();
    }

    int32_t size() const {
        return getReservedSlot(JS_DESCR_SLOT_SIZE).toInt32();
    }

    
    
    
    
    
    
    
    
    
    
    bool hasTraceList() const {
        return !getFixedSlot(JS_DESCR_SLOT_TRACE_LIST).isUndefined();
    }
    const int32_t *traceList() const {
        MOZ_ASSERT(hasTraceList());
        return reinterpret_cast<int32_t *>(getFixedSlot(JS_DESCR_SLOT_TRACE_LIST).toPrivate());
    }

    void initInstances(const JSRuntime *rt, uint8_t *mem, size_t length);
    void traceInstances(JSTracer *trace, uint8_t *mem, size_t length);

    static void finalize(FreeOp *fop, JSObject *obj);
};

typedef Handle<TypeDescr*> HandleTypeDescr;

class SimpleTypeDescr : public TypeDescr
{
};





class ScalarTypeDescr : public SimpleTypeDescr
{
  public:
    typedef Scalar::Type Type;

    static const type::Kind Kind = type::Scalar;
    static const bool Opaque = false;
    static int32_t size(Type t);
    static int32_t alignment(Type t);
    static const char *typeName(Type type);

    static const Class class_;
    static const JSFunctionSpec typeObjectMethods[];

    Type type() const {
        
        
        
        
        static_assert(Scalar::Int8 == JS_SCALARTYPEREPR_INT8,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Uint8 == JS_SCALARTYPEREPR_UINT8,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Int16 == JS_SCALARTYPEREPR_INT16,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Uint16 == JS_SCALARTYPEREPR_UINT16,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Int32 == JS_SCALARTYPEREPR_INT32,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Uint32 == JS_SCALARTYPEREPR_UINT32,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Float32 == JS_SCALARTYPEREPR_FLOAT32,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Float64 == JS_SCALARTYPEREPR_FLOAT64,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Uint8Clamped == JS_SCALARTYPEREPR_UINT8_CLAMPED,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Float32x4 == JS_SCALARTYPEREPR_FLOAT32X4,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");
        static_assert(Scalar::Int32x4 == JS_SCALARTYPEREPR_INT32X4,
                      "TypedObjectConstants.h must be consistent with Scalar::Type");

        return Type(getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32());
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
};




#define JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(macro_)       \
    macro_(Scalar::Int8,    int8_t,   int8)                     \
    macro_(Scalar::Uint8,   uint8_t,  uint8)                    \
    macro_(Scalar::Int16,   int16_t,  int16)                    \
    macro_(Scalar::Uint16,  uint16_t, uint16)                   \
    macro_(Scalar::Int32,   int32_t,  int32)                    \
    macro_(Scalar::Uint32,  uint32_t, uint32)                   \
    macro_(Scalar::Float32, float,    float32)                  \
    macro_(Scalar::Float64, double,   float64)


#define JS_FOR_EACH_SCALAR_TYPE_REPR(macro_)                    \
    JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(macro_)           \
    macro_(Scalar::Uint8Clamped, uint8_t, uint8Clamped)




class ReferenceTypeDescr : public SimpleTypeDescr
{
  public:
    
    enum Type {
        TYPE_ANY = JS_REFERENCETYPEREPR_ANY,
        TYPE_OBJECT = JS_REFERENCETYPEREPR_OBJECT,
        TYPE_STRING = JS_REFERENCETYPEREPR_STRING,
    };
    static const int32_t TYPE_MAX = TYPE_STRING + 1;
    static const char *typeName(Type type);

    static const type::Kind Kind = type::Reference;
    static const bool Opaque = true;
    static const Class class_;
    static int32_t size(Type t);
    static int32_t alignment(Type t);
    static const JSFunctionSpec typeObjectMethods[];

    ReferenceTypeDescr::Type type() const {
        return (ReferenceTypeDescr::Type) getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32();
    }

    const char *typeName() const {
        return typeName(type());
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
};

#define JS_FOR_EACH_REFERENCE_TYPE_REPR(macro_)                    \
    macro_(ReferenceTypeDescr::TYPE_ANY,    HeapValue, Any)        \
    macro_(ReferenceTypeDescr::TYPE_OBJECT, HeapPtrObject, Object) \
    macro_(ReferenceTypeDescr::TYPE_STRING, HeapPtrString, string)



class ComplexTypeDescr : public TypeDescr
{
  public:
    
    
    TypedProto &instancePrototype() const {
        return getReservedSlot(JS_DESCR_SLOT_TYPROTO).toObject().as<TypedProto>();
    }
};




class SimdTypeDescr : public ComplexTypeDescr
{
  public:
    enum Type {
        TYPE_INT32 = JS_SIMDTYPEREPR_INT32,
        TYPE_FLOAT32 = JS_SIMDTYPEREPR_FLOAT32,
        TYPE_FLOAT64 = JS_SIMDTYPEREPR_FLOAT64
    };

    static const type::Kind Kind = type::Simd;
    static const bool Opaque = false;
    static const Class class_;
    static int32_t size(Type t);
    static int32_t alignment(Type t);
    static int32_t lanes(Type t);

    SimdTypeDescr::Type type() const {
        return (SimdTypeDescr::Type) getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32();
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
    static bool is(const Value &v);
};

#define JS_FOR_EACH_SIMD_TYPE_REPR(macro_)                             \
    macro_(SimdTypeDescr::TYPE_INT32, int32_t, int32, 4)               \
    macro_(SimdTypeDescr::TYPE_FLOAT32, float, float32, 4)             \
    macro_(SimdTypeDescr::TYPE_FLOAT64, double, float64, 2)

bool IsTypedObjectClass(const Class *clasp); 
bool IsTypedObjectArray(JSObject& obj);

bool CreateUserSizeAndAlignmentProperties(JSContext *cx, HandleTypeDescr obj);

class ArrayTypeDescr;






class ArrayMetaTypeDescr : public JSObject
{
  private:
    
    
    
    
    
    
    static ArrayTypeDescr *create(JSContext *cx,
                                  HandleObject arrayTypePrototype,
                                  HandleTypeDescr elementType,
                                  HandleAtom stringRepr,
                                  int32_t size,
                                  int32_t length);

  public:
    
    
    static const JSPropertySpec typeObjectProperties[];
    static const JSFunctionSpec typeObjectMethods[];

    
    
    static const JSPropertySpec typedObjectProperties[];
    static const JSFunctionSpec typedObjectMethods[];

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};




class ArrayTypeDescr : public ComplexTypeDescr
{
  public:
    static const Class class_;
    static const type::Kind Kind = type::Array;

    TypeDescr &elementType() const {
        return getReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE).toObject().as<TypeDescr>();
    }

    TypeDescr &maybeForwardedElementType() const {
        JSObject *elemType = &getReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE).toObject();
        return MaybeForwarded(elemType)->as<TypeDescr>();
    }

    int32_t length() const {
        return getReservedSlot(JS_DESCR_SLOT_ARRAY_LENGTH).toInt32();
    }

    static int32_t offsetOfLength() {
        return getFixedSlotOffset(JS_DESCR_SLOT_ARRAY_LENGTH);
    }
};






class StructMetaTypeDescr : public JSObject
{
  private:
    static JSObject *create(JSContext *cx, HandleObject structTypeGlobal,
                            HandleObject fields);

  public:
    
    
    static const JSPropertySpec typeObjectProperties[];
    static const JSFunctionSpec typeObjectMethods[];

    
    
    static const JSPropertySpec typedObjectProperties[];
    static const JSFunctionSpec typedObjectMethods[];

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};

class StructTypeDescr : public ComplexTypeDescr
{
  public:
    static const Class class_;

    
    size_t fieldCount() const;
    size_t maybeForwardedFieldCount() const;

    
    
    bool fieldIndex(jsid id, size_t *out) const;

    
    JSAtom &fieldName(size_t index) const;

    
    TypeDescr &fieldDescr(size_t index) const;
    TypeDescr &maybeForwardedFieldDescr(size_t index) const;

    
    size_t fieldOffset(size_t index) const;
    size_t maybeForwardedFieldOffset(size_t index) const;

  private:
    ArrayObject &fieldInfoObject(size_t slot) const {
        return getReservedSlot(slot).toObject().as<ArrayObject>();
    }

    ArrayObject &maybeForwardedFieldInfoObject(size_t slot) const {
        return MaybeForwarded(&getReservedSlot(slot).toObject())->as<ArrayObject>();
    }
};

typedef Handle<StructTypeDescr*> HandleStructTypeDescr;






class TypedObjectModuleObject : public NativeObject {
  public:
    enum Slot {
        ArrayTypePrototype,
        StructTypePrototype,
        SlotCount
    };

    static const Class class_;
};


class TypedObject : public JSObject
{
  private:
    static const bool IsTypedObjectClass = true;

    static bool obj_getArrayElement(JSContext *cx,
                                    Handle<TypedObject*> typedObj,
                                    Handle<TypeDescr*> typeDescr,
                                    uint32_t index,
                                    MutableHandleValue vp);

    static bool obj_setArrayElement(JSContext *cx,
                                    Handle<TypedObject*> typedObj,
                                    Handle<TypeDescr*> typeDescr,
                                    uint32_t index,
                                    MutableHandleValue vp);

  protected:
    static bool obj_lookupProperty(JSContext *cx, HandleObject obj,
                                   HandleId id, MutableHandleObject objp,
                                   MutableHandleShape propp);

    static bool obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  MutableHandleObject objp, MutableHandleShape propp);

    static bool obj_defineProperty(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandleId id, MutableHandleValue vp);

    static bool obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                               uint32_t index, MutableHandleValue vp);

    static bool obj_setProperty(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleValue vp, bool strict);

    static bool obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                               MutableHandleValue vp, bool strict);

    static bool obj_getOwnPropertyDescriptor(JSContext *cx, HandleObject obj, HandleId id,
                                             MutableHandle<JSPropertyDescriptor> desc);

    static bool obj_setPropertyAttributes(JSContext *cx, HandleObject obj,
                                          HandleId id, unsigned *attrsp);

    static bool obj_deleteProperty(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded);

    static bool obj_enumerate(JSContext *cx, HandleObject obj, AutoIdVector &properties);

  public:
    TypedProto &typedProto() const {
        return getProto()->as<TypedProto>();
    }

    TypedProto &maybeForwardedTypedProto() const {
        return MaybeForwarded(getProto())->as<TypedProto>();
    }

    TypeDescr &typeDescr() const {
        return group()->typeDescr();
    }

    TypeDescr &maybeForwardedTypeDescr() const {
        return MaybeForwarded(&typeDescr())->as<TypeDescr>();
    }

    int32_t offset() const;
    int32_t length() const;
    uint8_t *typedMem() const;
    uint8_t *typedMemBase() const;
    bool isAttached() const;
    bool maybeForwardedIsAttached() const;

    int32_t size() const {
        return typeDescr().size();
    }

    uint8_t *typedMem(size_t offset) const {
        
        
        
        
        
        MOZ_ASSERT(offset <= (size_t) size());
        return typedMem() + offset;
    }

    inline bool opaque() const;

    
    
    
    static TypedObject *createZeroed(JSContext *cx, HandleTypeDescr typeObj, int32_t length,
                                     gc::InitialHeap heap = gc::DefaultHeap);

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);

    
    static bool GetBuffer(JSContext *cx, unsigned argc, Value *vp);
    static bool GetByteOffset(JSContext *cx, unsigned argc, Value *vp);
};

typedef Handle<TypedObject*> HandleTypedObject;

class OutlineTypedObject : public TypedObject
{
    
    
    
    JSObject *owner_;

    
    uint8_t *data_;

    void setOwnerAndData(JSObject *owner, uint8_t *data);

  public:
    
    static size_t offsetOfData() { return offsetof(OutlineTypedObject, data_); }
    static size_t offsetOfOwner() { return offsetof(OutlineTypedObject, owner_); }

    JSObject &owner() const {
        MOZ_ASSERT(owner_);
        return *owner_;
    }

    JSObject *maybeOwner() const {
        return owner_;
    }

    uint8_t *outOfLineTypedMem() const {
        return data_;
    }

    void setData(uint8_t *data) {
        data_ = data;
    }

    
    static OutlineTypedObject *createUnattachedWithClass(JSContext *cx,
                                                         const Class *clasp,
                                                         HandleTypeDescr type,
                                                         int32_t length,
                                                         gc::InitialHeap heap = gc::DefaultHeap);

    
    
    
    
    
    
    
    
    static OutlineTypedObject *createUnattached(JSContext *cx, HandleTypeDescr type,
                                                int32_t length, gc::InitialHeap heap = gc::DefaultHeap);

    
    
    
    static OutlineTypedObject *createDerived(JSContext *cx,
                                             HandleTypeDescr type,
                                             Handle<TypedObject*> typedContents,
                                             int32_t offset);

    
    void attach(JSContext *cx, ArrayBufferObject &buffer, int32_t offset);

    
    void attach(JSContext *cx, TypedObject &typedObj, int32_t offset);

    
    void neuter(void *newData);

    static void obj_trace(JSTracer *trace, JSObject *object);
};


class OutlineTransparentTypedObject : public OutlineTypedObject
{
  public:
    static const Class class_;

    ArrayBufferObject *getOrCreateBuffer(JSContext *cx);
};



class OutlineOpaqueTypedObject : public OutlineTypedObject
{
  public:
    static const Class class_;
};


class InlineTypedObject : public TypedObject
{
    
    uint8_t data_[1];

  public:
    static const size_t MaximumSize = JSObject::MAX_BYTE_SIZE - sizeof(TypedObject);

    static gc::AllocKind allocKindForTypeDescriptor(TypeDescr *descr) {
        size_t nbytes = descr->size();
        MOZ_ASSERT(nbytes <= MaximumSize);

        return gc::GetGCObjectKindForBytes(nbytes + sizeof(TypedObject));
    }

    uint8_t *inlineTypedMem() const {
        static_assert(offsetof(InlineTypedObject, data_) == sizeof(JSObject),
                      "The data for an inline typed object must follow the shape and type.");
        return (uint8_t *) &data_;
    }

    static void obj_trace(JSTracer *trace, JSObject *object);
    static void objectMovedDuringMinorGC(JSTracer *trc, JSObject *dst, JSObject *src);

    static size_t offsetOfDataStart() {
        return offsetof(InlineTypedObject, data_);
    }

    static InlineTypedObject *create(JSContext *cx, HandleTypeDescr descr,
                                     gc::InitialHeap heap = gc::DefaultHeap);
    static InlineTypedObject *createCopy(JSContext *cx, Handle<InlineTypedObject *> templateObject,
                                         gc::InitialHeap heap);
};



class InlineTransparentTypedObject : public InlineTypedObject
{
  public:
    static const Class class_;

    ArrayBufferObject *getOrCreateBuffer(JSContext *cx);
};


class InlineOpaqueTypedObject : public InlineTypedObject
{
  public:
    static const Class class_;
};






bool NewOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp);






bool NewDerivedTypedObject(JSContext *cx, unsigned argc, Value *vp);







bool AttachTypedObject(JSContext *cx, unsigned argc, Value *vp);







bool SetTypedObjectOffset(JSContext *, unsigned argc, Value *vp);






bool ObjectIsTypeDescr(JSContext *cx, unsigned argc, Value *vp);






bool ObjectIsTypedObject(JSContext *cx, unsigned argc, Value *vp);






bool ObjectIsOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp);






bool ObjectIsTransparentTypedObject(JSContext *cx, unsigned argc, Value *vp);



bool TypeDescrIsSimpleType(JSContext *, unsigned argc, Value *vp);

bool TypeDescrIsArrayType(JSContext *, unsigned argc, Value *vp);







bool TypedObjectIsAttached(JSContext *cx, unsigned argc, Value *vp);






bool TypedObjectTypeDescr(JSContext *cx, unsigned argc, Value *vp);






bool ClampToUint8(JSContext *cx, unsigned argc, Value *vp);










bool GetTypedObjectModule(JSContext *cx, unsigned argc, Value *vp);







bool GetFloat32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp);







bool GetFloat64x2TypeDescr(JSContext *cx, unsigned argc, Value *vp);







bool GetInt32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp);

















#define JS_STORE_SCALAR_CLASS_DEFN(_constant, T, _name)                       \
class StoreScalar##T {                                                        \
  public:                                                                     \
    static bool Func(JSContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};














#define JS_STORE_REFERENCE_CLASS_DEFN(_constant, T, _name)                    \
class StoreReference##T {                                                     \
  private:                                                                    \
    static bool store(JSContext *cx, T* heap, const Value &v,         \
                      TypedObject *obj, jsid id);                             \
                                                                              \
  public:                                                                     \
    static bool Func(JSContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};









#define JS_LOAD_SCALAR_CLASS_DEFN(_constant, T, _name)                        \
class LoadScalar##T {                                                         \
  public:                                                                     \
    static bool Func(JSContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};









#define JS_LOAD_REFERENCE_CLASS_DEFN(_constant, T, _name)                     \
class LoadReference##T {                                                      \
  private:                                                                    \
    static void load(T* heap, MutableHandleValue v);                          \
                                                                              \
  public:                                                                     \
    static bool Func(JSContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};



JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_STORE_SCALAR_CLASS_DEFN)
JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_LOAD_SCALAR_CLASS_DEFN)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_STORE_REFERENCE_CLASS_DEFN)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_LOAD_REFERENCE_CLASS_DEFN)

inline bool
IsTypedObjectClass(const Class *class_)
{
    return class_ == &OutlineTransparentTypedObject::class_ ||
           class_ == &InlineTransparentTypedObject::class_ ||
           class_ == &OutlineOpaqueTypedObject::class_ ||
           class_ == &InlineOpaqueTypedObject::class_;
}

inline bool
IsOpaqueTypedObjectClass(const Class *class_)
{
    return class_ == &OutlineOpaqueTypedObject::class_ ||
           class_ == &InlineOpaqueTypedObject::class_;
}

inline bool
IsOutlineTypedObjectClass(const Class *class_)
{
    return class_ == &OutlineOpaqueTypedObject::class_ ||
           class_ == &OutlineTransparentTypedObject::class_;
}

inline bool
IsInlineTypedObjectClass(const Class *class_)
{
    return class_ == &InlineOpaqueTypedObject::class_ ||
           class_ == &InlineTransparentTypedObject::class_;
}

inline const Class *
GetOutlineTypedObjectClass(bool opaque)
{
    return opaque ? &OutlineOpaqueTypedObject::class_ : &OutlineTransparentTypedObject::class_;
}

inline bool
IsSimpleTypeDescrClass(const Class* clasp)
{
    return clasp == &ScalarTypeDescr::class_ ||
           clasp == &ReferenceTypeDescr::class_;
}

inline bool
IsComplexTypeDescrClass(const Class* clasp)
{
    return clasp == &StructTypeDescr::class_ ||
           clasp == &ArrayTypeDescr::class_ ||
           clasp == &SimdTypeDescr::class_;
}

inline bool
IsTypeDescrClass(const Class* clasp)
{
    return IsSimpleTypeDescrClass(clasp) ||
           IsComplexTypeDescrClass(clasp);
}

inline bool
TypedObject::opaque() const
{
    return IsOpaqueTypedObjectClass(getClass());
}




class LazyArrayBufferTable
{
  private:
    
    
    
    
    typedef WeakMap<PreBarrieredObject, RelocatablePtrObject> Map;
    Map map;

  public:
    explicit LazyArrayBufferTable(JSContext *cx);
    ~LazyArrayBufferTable();

    ArrayBufferObject *maybeBuffer(InlineTransparentTypedObject *obj);
    bool addBuffer(JSContext *cx, InlineTransparentTypedObject *obj, ArrayBufferObject *buffer);

    void trace(JSTracer *trc);
    size_t sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf);
};

} 

JSObject *
js_InitTypedObjectModuleObject(JSContext *cx, JS::HandleObject obj);

template <>
inline bool
JSObject::is<js::SimpleTypeDescr>() const
{
    return IsSimpleTypeDescrClass(getClass());
}

template <>
inline bool
JSObject::is<js::ComplexTypeDescr>() const
{
    return IsComplexTypeDescrClass(getClass());
}

template <>
inline bool
JSObject::is<js::TypeDescr>() const
{
    return IsTypeDescrClass(getClass());
}

template <>
inline bool
JSObject::is<js::TypedObject>() const
{
    return IsTypedObjectClass(getClass());
}

template <>
inline bool
JSObject::is<js::OutlineTypedObject>() const
{
    return getClass() == &js::OutlineTransparentTypedObject::class_ ||
           getClass() == &js::OutlineOpaqueTypedObject::class_;
}

template <>
inline bool
JSObject::is<js::InlineTypedObject>() const
{
    return getClass() == &js::InlineTransparentTypedObject::class_ ||
           getClass() == &js::InlineOpaqueTypedObject::class_;
}

#endif
