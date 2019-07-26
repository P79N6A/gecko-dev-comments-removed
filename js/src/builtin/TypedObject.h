





#ifndef builtin_TypedObject_h
#define builtin_TypedObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"



















































































namespace js {

class TypeRepresentation;
class ScalarTypeRepresentation;
class ReferenceTypeRepresentation;
class X4TypeRepresentation;
class StructTypeDescr;







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

class TypeDescr : public JSObject
{
  public:
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

    JSObject &typeRepresentationOwnerObj() const {
        return getReservedSlot(JS_DESCR_SLOT_TYPE_REPR).toObject();
    }

    TypeRepresentation *typeRepresentation() const;

    TypeDescr::Kind kind() const;
};






class TypedObjectModuleObject : public JSObject {
  public:
    enum Slot {
        ArrayTypePrototype,
        StructTypePrototype,
        SlotCount
    };

    static const Class class_;

    static bool getSuitableClaspAndProto(JSContext *cx,
                                         TypeDescr::Kind kind,
                                         const Class **clasp,
                                         MutableHandleObject proto);
};

typedef Handle<TypeDescr*> HandleTypeDescr;

bool InitializeCommonTypeDescriptorProperties(JSContext *cx,
                                              HandleTypeDescr obj,
                                              HandleObject typeReprOwnerObj);

class SizedTypeDescr : public TypeDescr
{
  public:
    size_t size() {
        return getReservedSlot(JS_DESCR_SLOT_SIZE).toInt32();
    }
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

    static size_t size(Type t);
    static size_t alignment(Type t);
    static const char *typeName(Type type);

    static const Class class_;
    static const JSFunctionSpec typeObjectMethods[];
    typedef ScalarTypeRepresentation TypeRepr;

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

    static const Class class_;
    static const JSFunctionSpec typeObjectMethods[];
    typedef ReferenceTypeRepresentation TypeRepr;

    ReferenceTypeDescr::Type type() const {
        return (ReferenceTypeDescr::Type) getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32();
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
};

#define JS_FOR_EACH_REFERENCE_TYPE_REPR(macro_)                    \
    macro_(ReferenceTypeDescr::TYPE_ANY,    HeapValue, Any)        \
    macro_(ReferenceTypeDescr::TYPE_OBJECT, HeapPtrObject, Object) \
    macro_(ReferenceTypeDescr::TYPE_STRING, HeapPtrString, string)




class X4TypeDescr : public SizedTypeDescr
{
  public:
    enum Type {
        TYPE_INT32 = JS_X4TYPEREPR_INT32,
        TYPE_FLOAT32 = JS_X4TYPEREPR_FLOAT32,
    };

    static const Class class_;
    typedef X4TypeRepresentation TypeRepr;

    X4TypeDescr::Type type() const {
        return (X4TypeDescr::Type) getReservedSlot(JS_DESCR_SLOT_TYPE).toInt32();
    }

    static bool call(JSContext *cx, unsigned argc, Value *vp);
    static bool is(const Value &v);
};

#define JS_FOR_EACH_X4_TYPE_REPR(macro_)                             \
    macro_(X4TypeDescr::TYPE_INT32, int32_t, int32)                  \
    macro_(X4TypeDescr::TYPE_FLOAT32, float, float32)






class ArrayMetaTypeDescr : public JSObject
{
  private:
    friend class UnsizedArrayTypeDescr;

    
    
    
    
    
    
    
    
    
    
    template<class T>
    static T *create(JSContext *cx,
                     HandleObject arrayTypePrototype,
                     HandleObject arrayTypeReprObj,
                     HandleSizedTypeDescr elementType);

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

    
    
    static bool dimension(JSContext *cx, unsigned int argc, jsval *vp);

    SizedTypeDescr &elementType() {
        return getReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE).toObject().as<SizedTypeDescr>();
    }
};




class SizedArrayTypeDescr : public SizedTypeDescr
{
  public:
    static const Class class_;

    SizedTypeDescr &elementType() {
        return getReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE).toObject().as<SizedTypeDescr>();
    }

    size_t length() {
        return (size_t) getReservedSlot(JS_DESCR_SLOT_SIZED_ARRAY_LENGTH).toInt32();
    }
};






class StructMetaTypeDescr : public JSObject
{
  private:
    static JSObject *create(JSContext *cx, HandleObject structTypeGlobal,
                            HandleObject fields);

    



    static bool layout(JSContext *cx,
                       Handle<StructTypeDescr*> structType,
                       HandleObject fields);

  public:
    
    
    static const JSPropertySpec typeObjectProperties[];
    static const JSFunctionSpec typeObjectMethods[];

    
    
    static const JSPropertySpec typedObjectProperties[];
    static const JSFunctionSpec typedObjectMethods[];

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};

class StructTypeDescr : public SizedTypeDescr {
  public:
    static const Class class_;

    
    
    bool fieldIndex(jsid id, size_t *out);

    
    SizedTypeDescr &fieldDescr(size_t index);

    
    size_t fieldOffset(size_t index);
};

typedef Handle<StructTypeDescr*> HandleStructTypeDescr;





class TypedDatum : public JSObject
{
  private:
    static const bool IsTypedDatumClass = true;

    template<class T>
    static bool obj_getArrayElement(JSContext *cx,
                                    Handle<TypedDatum*> datum,
                                    Handle<TypeDescr*> typeDescr,
                                    uint32_t index,
                                    MutableHandleValue vp);

    template<class T>
    static bool obj_setArrayElement(JSContext *cx,
                                    Handle<TypedDatum*> datum,
                                    Handle<TypeDescr*> typeDescr,
                                    uint32_t index,
                                    MutableHandleValue vp);

  protected:
    static void obj_finalize(js::FreeOp *op, JSObject *obj);

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

    static bool obj_lookupSpecial(JSContext *cx, HandleObject obj,
                                  HandleSpecialId sid,
                                  MutableHandleObject objp,
                                  MutableHandleShape propp);

    static bool obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_defineProperty(JSContext *cx, HandleObject obj,
                                   HandlePropertyName name, HandleValue v,
                                   PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_defineSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, HandleValue v,
                                  PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static bool obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleId id, MutableHandleValue vp);

    static bool obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                                HandlePropertyName name, MutableHandleValue vp);

    static bool obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                               uint32_t index, MutableHandleValue vp);

    static bool obj_getUnsizedArrayElement(JSContext *cx, HandleObject obj, HandleObject receiver,
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

  public:
    
    
    
    
    
    
    static size_t dataOffset();

    static TypedDatum *createUnattachedWithClass(JSContext *cx,
                                                 const Class *clasp,
                                                 HandleTypeDescr type,
                                                 int32_t length);

    
    
    
    
    
    
    
    
    template<class T>
    static T *createUnattached(JSContext *cx, HandleTypeDescr type,
                               int32_t length);

    
    
    
    static TypedDatum *createDerived(JSContext *cx,
                                     HandleSizedTypeDescr type,
                                     Handle<TypedDatum*> typedContents,
                                     size_t offset);


    
    void attach(uint8_t *mem);

    
    void attach(TypedDatum &datum, uint32_t offset);

    TypedDatum &owner() const {
        return getReservedSlot(JS_DATUM_SLOT_OWNER).toObject().as<TypedDatum>();
    }

    TypeDescr &typeDescr() const {
        return getReservedSlot(JS_DATUM_SLOT_TYPE_DESCR).toObject().as<TypeDescr>();
    }

    TypeRepresentation *typeRepresentation() const {
        return typeDescr().typeRepresentation();
    }

    uint8_t *typedMem() const {
        return (uint8_t*) getPrivate();
    }

    size_t length() const {
        return getReservedSlot(JS_DATUM_SLOT_LENGTH).toInt32();
    }

    size_t size() const {
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
        
        
        
        
        
        JS_ASSERT(offset <= size());
        return typedMem() + offset;
    }
};

typedef Handle<TypedDatum*> HandleTypedDatum;

class TypedObject : public TypedDatum
{
  public:
    static const Class class_;

    
    
    
    static TypedObject *createZeroed(JSContext *cx,
                                     HandleTypeDescr typeObj,
                                     int32_t length);

    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};

typedef Handle<TypedObject*> HandleTypedObject;

class TypedHandle : public TypedDatum
{
  public:
    static const Class class_;
    static const JSFunctionSpec handleStaticMethods[];
};






bool NewTypedHandle(JSContext *cx, unsigned argc, Value *vp);






bool NewTypedHandle(JSContext *cx, unsigned argc, Value *vp);






bool NewDerivedTypedDatum(JSContext *cx, unsigned argc, Value *vp);







bool AttachHandle(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo AttachHandleJitInfo;






bool ObjectIsTypeDescr(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypeDescrJitInfo;






bool ObjectIsTypeRepresentation(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypeRepresentationJitInfo;






bool ObjectIsTypedHandle(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypedHandleJitInfo;






bool ObjectIsTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypedObjectJitInfo;







bool IsAttached(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo IsAttachedJitInfo;






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

} 

JSObject *
js_InitTypedObjectModuleObject(JSContext *cx, JS::HandleObject obj);

template <>
inline bool
JSObject::is<js::SimpleTypeDescr>() const
{
    return is<js::ScalarTypeDescr>() ||
           is<js::ReferenceTypeDescr>();
}

template <>
inline bool
JSObject::is<js::SizedTypeDescr>() const
{
    return is<js::SimpleTypeDescr>() ||
           is<js::StructTypeDescr>() ||
           is<js::SizedArrayTypeDescr>() ||
           is<js::X4TypeDescr>();
}

template <>
inline bool
JSObject::is<js::TypeDescr>() const
{
    return is<js::SizedTypeDescr>() ||
           is<js::UnsizedArrayTypeDescr>();
}

template <>
inline bool
JSObject::is<js::TypedDatum>() const
{
    return is<js::TypedObject>() || is<js::TypedHandle>();
}

#endif

