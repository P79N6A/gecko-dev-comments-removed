





#ifndef builtin_TypedObject_h
#define builtin_TypedObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "vm/ArrayBufferObject.h"





























































































namespace js {







template <typename T>
static T ConvertScalar(double d)
{
    if (TypeIsFloatingPoint<T>()) {
        return T(d);
    } else if (TypeIsUnsigned<T>()) {
        uint32_t n = ToUint32(d);
        return T(n);
    } else {
        int32_t n = ToInt32(d);
        return T(n);
    }
}






class TypedProto : public JSObject
{
  public:
    static const Class class_;

    inline void initTypeDescrSlot(TypeDescr &descr);

    TypeDescr &typeDescr() const {
        return getReservedSlot(JS_TYPROTO_SLOT_DESCR).toObject().as<TypeDescr>();
    }
};

class TypeDescr : public JSObject
{
  public:
    
    
    
    
    
    
    static const Class class_;

    enum Kind {
        Scalar = JS_TYPEREPR_SCALAR_KIND,
        Reference = JS_TYPEREPR_REFERENCE_KIND,
        X4 = JS_TYPEREPR_X4_KIND,
        Struct = JS_TYPEREPR_STRUCT_KIND,
        SizedArray = JS_TYPEREPR_SIZED_ARRAY_KIND,
        UnsizedArray = JS_TYPEREPR_UNSIZED_ARRAY_KIND,
    };

    static bool isSized(Kind kind) {
        return kind > JS_TYPEREPR_MAX_UNSIZED_KIND;
    }

    JSAtom &stringRepr() const {
        return getReservedSlot(JS_DESCR_SLOT_STRING_REPR).toString()->asAtom();
    }

    TypeDescr::Kind kind() const {
        return (TypeDescr::Kind) getReservedSlot(JS_DESCR_SLOT_KIND).toInt32();
    }

    bool opaque() const {
        return getReservedSlot(JS_DESCR_SLOT_OPAQUE).toBoolean();
    }

    bool transparent() const {
        return !opaque();
    }

    int32_t alignment() {
        return getReservedSlot(JS_DESCR_SLOT_ALIGNMENT).toInt32();
    }
};

typedef Handle<TypeDescr*> HandleTypeDescr;

class SizedTypeDescr : public TypeDescr
{
  public:
    int32_t size() {
        return getReservedSlot(JS_DESCR_SLOT_SIZE).toInt32();
    }

    void initInstances(const JSRuntime *rt, uint8_t *mem, size_t length);
    void traceInstances(JSTracer *trace, uint8_t *mem, size_t length);
};

typedef Handle<SizedTypeDescr*> HandleSizedTypeDescr;

class SimpleTypeDescr : public SizedTypeDescr
{
};





class ScalarTypeDescr : public SimpleTypeDescr
{
  public:
    
    enum Type {
        TYPE_INT8 = JS_SCALARTYPEREPR_INT8,
        TYPE_UINT8 = JS_SCALARTYPEREPR_UINT8,
        TYPE_INT16 = JS_SCALARTYPEREPR_INT16,
        TYPE_UINT16 = JS_SCALARTYPEREPR_UINT16,
        TYPE_INT32 = JS_SCALARTYPEREPR_INT32,
        TYPE_UINT32 = JS_SCALARTYPEREPR_UINT32,
        TYPE_FLOAT32 = JS_SCALARTYPEREPR_FLOAT32,
        TYPE_FLOAT64 = JS_SCALARTYPEREPR_FLOAT64,

        



        TYPE_UINT8_CLAMPED = JS_SCALARTYPEREPR_UINT8_CLAMPED,
    };
    static const int32_t TYPE_MAX = TYPE_UINT8_CLAMPED + 1;

    static const TypeDescr::Kind Kind = TypeDescr::Scalar;
    static const bool Opaque = false;
    static int32_t size(Type t);
    static int32_t alignment(Type t);
    static const char *typeName(Type type);

    static const Class class_;
    static const JSFunctionSpec typeObjectMethods[];

    ScalarTypeDescr::Type type() const {
        return (ScalarTypeDescr::Type) getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32();
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
};




#define JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(macro_)                     \
    macro_(ScalarTypeDescr::TYPE_INT8,    int8_t,   int8)            \
    macro_(ScalarTypeDescr::TYPE_UINT8,   uint8_t,  uint8)           \
    macro_(ScalarTypeDescr::TYPE_INT16,   int16_t,  int16)           \
    macro_(ScalarTypeDescr::TYPE_UINT16,  uint16_t, uint16)          \
    macro_(ScalarTypeDescr::TYPE_INT32,   int32_t,  int32)           \
    macro_(ScalarTypeDescr::TYPE_UINT32,  uint32_t, uint32)          \
    macro_(ScalarTypeDescr::TYPE_FLOAT32, float,    float32)         \
    macro_(ScalarTypeDescr::TYPE_FLOAT64, double,   float64)


#define JS_FOR_EACH_SCALAR_TYPE_REPR(macro_)                                    \
    JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(macro_)                           \
    macro_(ScalarTypeDescr::TYPE_UINT8_CLAMPED, uint8_t, uint8Clamped)




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

    static const TypeDescr::Kind Kind = TypeDescr::Reference;
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



class ComplexTypeDescr : public SizedTypeDescr
{
  public:
    
    
    TypedProto &instancePrototype() const {
        return getReservedSlot(JS_DESCR_SLOT_TYPROTO).toObject().as<TypedProto>();
    }
};




class X4TypeDescr : public ComplexTypeDescr
{
  public:
    enum Type {
        TYPE_INT32 = JS_X4TYPEREPR_INT32,
        TYPE_FLOAT32 = JS_X4TYPEREPR_FLOAT32,
    };

    static const TypeDescr::Kind Kind = TypeDescr::X4;
    static const bool Opaque = false;
    static const Class class_;
    static int32_t size(Type t);
    static int32_t alignment(Type t);

    X4TypeDescr::Type type() const {
        return (X4TypeDescr::Type) getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32();
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
    static bool is(const Value &v);
};

#define JS_FOR_EACH_X4_TYPE_REPR(macro_)                             \
    macro_(X4TypeDescr::TYPE_INT32, int32_t, int32)                  \
    macro_(X4TypeDescr::TYPE_FLOAT32, float, float32)

bool IsTypedObjectClass(const Class *clasp); 
bool IsTypedObjectArray(JSObject& obj);

bool CreateUserSizeAndAlignmentProperties(JSContext *cx, HandleTypeDescr obj);






class ArrayMetaTypeDescr : public JSObject
{
  private:
    friend class UnsizedArrayTypeDescr;

    
    
    
    
    
    
    
    
    
    
    
    template<class T>
    static T *create(JSContext *cx,
                     HandleObject arrayTypePrototype,
                     HandleSizedTypeDescr elementType,
                     HandleAtom stringRepr,
                     int32_t size);

  public:
    
    
    static const JSPropertySpec typeObjectProperties[];
    static const JSFunctionSpec typeObjectMethods[];

    
    
    static const JSPropertySpec typedObjectProperties[];
    static const JSFunctionSpec typedObjectMethods[];

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};









class UnsizedArrayTypeDescr : public TypeDescr
{
  public:
    static const Class class_;
    static const TypeDescr::Kind Kind = TypeDescr::UnsizedArray;

    
    
    static bool dimension(JSContext *cx, unsigned int argc, jsval *vp);

    SizedTypeDescr &elementType() {
        return getReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE).toObject().as<SizedTypeDescr>();
    }
};




class SizedArrayTypeDescr : public ComplexTypeDescr
{
  public:
    static const Class class_;
    static const TypeDescr::Kind Kind = TypeDescr::SizedArray;

    SizedTypeDescr &elementType() {
        return getReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE).toObject().as<SizedTypeDescr>();
    }

    int32_t length() {
        return getReservedSlot(JS_DESCR_SLOT_SIZED_ARRAY_LENGTH).toInt32();
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

    
    size_t fieldCount();

    
    
    bool fieldIndex(jsid id, size_t *out);

    
    JSAtom &fieldName(size_t index);

    
    SizedTypeDescr &fieldDescr(size_t index);

    
    int32_t fieldOffset(size_t index);
};

typedef Handle<StructTypeDescr*> HandleStructTypeDescr;






class TypedObjectModuleObject : public JSObject {
  public:
    enum Slot {
        ArrayTypePrototype,
        StructTypePrototype,
        SlotCount
    };

    static const Class class_;
};





class TypedObject : public ArrayBufferViewObject
{
  private:
    static const bool IsTypedObjectClass = true;

    template<class T>
    static bool obj_getArrayElement(JSContext *cx,
                                    Handle<TypedObject*> typedObj,
                                    Handle<TypeDescr*> typeDescr,
                                    uint32_t index,
                                    MutableHandleValue vp);

    template<class T>
    static bool obj_setArrayElement(JSContext *cx,
                                    Handle<TypedObject*> typedObj,
                                    Handle<TypeDescr*> typeDescr,
                                    uint32_t index,
                                    MutableHandleValue vp);

  protected:
    static void obj_trace(JSTracer *trace, JSObject *object);

    static bool obj_lookupGeneric(JSContext *cx, HandleObject obj,
                                  HandleId id, MutableHandleObject objp,
                                  MutableHandleShape propp);

    static bool obj_lookupProperty(JSContext *cx, HandleObject obj,
                                   HandlePropertyName name,
                                   MutableHandleObject objp,
                                   MutableHandleShape propp);

    static bool obj_lookupElement(JSContext *cx, HandleObject obj,
                                  uint32_t index, MutableHandleObject objp,
                                  MutableHandleShape propp);

    static bool obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_defineProperty(JSContext *cx, HandleObject obj,
                                   HandlePropertyName name, HandleValue v,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleId id, MutableHandleValue vp);

    static bool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandlePropertyName name, MutableHandleValue vp);

    static bool obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                               uint32_t index, MutableHandleValue vp);

    static bool obj_getUnsizedArrayElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                                         uint32_t index, MutableHandleValue vp);

    static bool obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                               MutableHandleValue vp, bool strict);
    static bool obj_setProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                MutableHandleValue vp, bool strict);
    static bool obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                               MutableHandleValue vp, bool strict);

    static bool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);
    static bool obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                         HandleId id, unsigned *attrsp);

    static bool obj_deleteProperty(JSContext *cx, HandleObject obj, HandlePropertyName name,
                                   bool *succeeded);
    static bool obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                                  bool *succeeded);

    static bool obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                              MutableHandleValue statep, MutableHandleId idp);

  public:
    static size_t offsetOfOwnerSlot();

    
    
    
    
    
    
    static size_t offsetOfDataSlot();

    
    static size_t offsetOfByteOffsetSlot();

    
    static TypedObject *createUnattachedWithClass(JSContext *cx,
                                                 const Class *clasp,
                                                 HandleTypeDescr type,
                                                 int32_t length);

    
    
    
    
    
    
    
    
    static TypedObject *createUnattached(JSContext *cx, HandleTypeDescr type,
                                        int32_t length);

    
    
    
    static TypedObject *createDerived(JSContext *cx,
                                      HandleSizedTypeDescr type,
                                      Handle<TypedObject*> typedContents,
                                      int32_t offset);

    
    
    
    static TypedObject *createZeroed(JSContext *cx,
                                     HandleTypeDescr typeObj,
                                     int32_t length);

    
    
    
    static bool constructSized(JSContext *cx, unsigned argc, Value *vp);

    
    static bool constructUnsized(JSContext *cx, unsigned argc, Value *vp);

    
    void attach(ArrayBufferObject &buffer, int32_t offset);

    
    void attach(TypedObject &typedObj, int32_t offset);

    
    void neuter(void *newData);

    int32_t offset() const {
        return getReservedSlot(JS_TYPEDOBJ_SLOT_BYTEOFFSET).toInt32();
    }

    ArrayBufferObject &owner() const {
        return getReservedSlot(JS_TYPEDOBJ_SLOT_OWNER).toObject().as<ArrayBufferObject>();
    }

    TypedProto &typedProto() const {
        return getProto()->as<TypedProto>();
    }

    TypeDescr &typeDescr() const {
        return typedProto().typeDescr();
    }

    uint8_t *typedMem() const {
        return (uint8_t*) getPrivate();
    }

    int32_t byteLength() const {
        return getReservedSlot(JS_TYPEDOBJ_SLOT_BYTELENGTH).toInt32();
    }

    int32_t length() const {
        return getReservedSlot(JS_TYPEDOBJ_SLOT_LENGTH).toInt32();
    }

    int32_t size() const {
        switch (typeDescr().kind()) {
          case TypeDescr::Scalar:
          case TypeDescr::X4:
          case TypeDescr::Reference:
          case TypeDescr::Struct:
          case TypeDescr::SizedArray:
            return typeDescr().as<SizedTypeDescr>().size();

          case TypeDescr::UnsizedArray: {
            SizedTypeDescr &elementType = typeDescr().as<UnsizedArrayTypeDescr>().elementType();
            return elementType.size() * length();
          }
        }
        MOZ_ASSUME_UNREACHABLE("unhandled typerepresentation kind");
    }

    uint8_t *typedMem(size_t offset) const {
        
        
        
        
        
        JS_ASSERT(offset <= (size_t) size());
        return typedMem() + offset;
    }
};

typedef Handle<TypedObject*> HandleTypedObject;

class TransparentTypedObject : public TypedObject
{
  public:
    static const Class class_;
};

typedef Handle<TransparentTypedObject*> HandleTransparentTypedObject;

class OpaqueTypedObject : public TypedObject
{
  public:
    static const Class class_;
    static const JSFunctionSpec handleStaticMethods[];
};






bool NewOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp);






bool NewDerivedTypedObject(JSContext *cx, unsigned argc, Value *vp);







bool AttachTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo AttachTypedObjectJitInfo;







bool intrinsic_SetTypedObjectOffset(JSContext *cx, unsigned argc, Value *vp);
bool SetTypedObjectOffset(ThreadSafeContext *, unsigned argc, Value *vp);
extern const JSJitInfo intrinsic_SetTypedObjectOffsetJitInfo;






bool ObjectIsTypeDescr(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypeDescrJitInfo;






bool ObjectIsTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypedObjectJitInfo;






bool ObjectIsOpaqueTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsOpaqueTypedObjectJitInfo;






bool ObjectIsTransparentTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTransparentTypedObjectJitInfo;



bool TypeDescrIsSimpleType(ThreadSafeContext *, unsigned argc, Value *vp);
extern const JSJitInfo TypeDescrIsSimpleTypeJitInfo;

bool TypeDescrIsArrayType(ThreadSafeContext *, unsigned argc, Value *vp);
extern const JSJitInfo TypeDescrIsArrayTypeJitInfo;

bool TypeDescrIsSizedArrayType(ThreadSafeContext *, unsigned argc, Value *vp);
extern const JSJitInfo TypeDescrIsSizedArrayTypeJitInfo;

bool TypeDescrIsUnsizedArrayType(ThreadSafeContext *, unsigned argc, Value *vp);
extern const JSJitInfo TypeDescrIsUnsizedArrayTypeJitInfo;







bool TypedObjectIsAttached(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo TypedObjectIsAttachedJitInfo;






bool ClampToUint8(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ClampToUint8JitInfo;












bool Memcpy(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo MemcpyJitInfo;










bool GetTypedObjectModule(JSContext *cx, unsigned argc, Value *vp);







bool GetFloat32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp);







bool GetInt32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp);

















#define JS_STORE_SCALAR_CLASS_DEFN(_constant, T, _name)                       \
class StoreScalar##T {                                                        \
  public:                                                                     \
    static bool Func(ThreadSafeContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};














#define JS_STORE_REFERENCE_CLASS_DEFN(_constant, T, _name)                    \
class StoreReference##T {                                                     \
  private:                                                                    \
    static void store(T* heap, const Value &v);                               \
                                                                              \
  public:                                                                     \
    static bool Func(ThreadSafeContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};









#define JS_LOAD_SCALAR_CLASS_DEFN(_constant, T, _name)                        \
class LoadScalar##T {                                                         \
  public:                                                                     \
    static bool Func(ThreadSafeContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};









#define JS_LOAD_REFERENCE_CLASS_DEFN(_constant, T, _name)                     \
class LoadReference##T {                                                      \
  private:                                                                    \
    static void load(T* heap, MutableHandleValue v);                          \
                                                                              \
  public:                                                                     \
    static bool Func(ThreadSafeContext *cx, unsigned argc, Value *vp);        \
    static const JSJitInfo JitInfo;                                           \
};



JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_STORE_SCALAR_CLASS_DEFN)
JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_LOAD_SCALAR_CLASS_DEFN)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_STORE_REFERENCE_CLASS_DEFN)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_LOAD_REFERENCE_CLASS_DEFN)

inline bool
IsTypedObjectClass(const Class *class_)
{
    return class_ == &TransparentTypedObject::class_ ||
           class_ == &OpaqueTypedObject::class_;
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
           clasp == &SizedArrayTypeDescr::class_ ||
           clasp == &X4TypeDescr::class_;
}

inline bool
IsSizedTypeDescrClass(const Class* clasp)
{
    return IsSimpleTypeDescrClass(clasp) ||
           IsComplexTypeDescrClass(clasp);
}

inline bool
IsTypeDescrClass(const Class* clasp)
{
    return IsSizedTypeDescrClass(clasp) ||
           clasp == &UnsizedArrayTypeDescr::class_;
}

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
JSObject::is<js::SizedTypeDescr>() const
{
    return IsSizedTypeDescrClass(getClass());
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

template<>
inline bool
JSObject::is<js::SizedArrayTypeDescr>() const
{
    return getClass() == &js::SizedArrayTypeDescr::class_;
}

template<>
inline bool
JSObject::is<js::UnsizedArrayTypeDescr>() const
{
    return getClass() == &js::UnsizedArrayTypeDescr::class_;
}

inline void
js::TypedProto::initTypeDescrSlot(TypeDescr &descr)
{
    initReservedSlot(JS_TYPROTO_SLOT_DESCR, ObjectValue(descr));
}

#endif
