





#ifndef builtin_TypedObject_h
#define builtin_TypedObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "builtin/TypeRepresentation.h"



















































































namespace js {

class StructTypeDescr;






class TypedObjectModuleObject : public JSObject {
  public:
    enum Slot {
        ArrayTypePrototype,
        StructTypePrototype,
        SlotCount
    };

    static const Class class_;

    static bool getSuitableClaspAndProto(JSContext *cx,
                                         TypeRepresentation::Kind kind,
                                         const Class **clasp,
                                         MutableHandleObject proto);
};







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

bool TypeDescrToSource(JSContext *cx, unsigned int argc, Value *vp);

class TypeDescr : public JSObject
{
  public:
    JSObject &typeRepresentationOwnerObj() const {
        return getReservedSlot(JS_TYPEOBJ_SLOT_TYPE_REPR).toObject();
    }

    TypeRepresentation *typeRepresentation() const {
        return TypeRepresentation::fromOwnerObject(typeRepresentationOwnerObj());
    }

    TypeRepresentation::Kind kind() const {
        return typeRepresentation()->kind();
    }
};

typedef Handle<TypeDescr*> HandleTypeDescr;

bool InitializeCommonTypeDescriptorProperties(JSContext *cx,
                                              HandleTypeDescr obj,
                                              HandleObject typeReprOwnerObj);

class SizedTypeDescr : public TypeDescr
{
  public:
    SizedTypeRepresentation *typeRepresentation() const {
        return ((TypeDescr*)this)->typeRepresentation()->asSized();
    }

    size_t size() {
        return typeRepresentation()->size();
    }
};

typedef Handle<SizedTypeDescr*> HandleSizedTypeDescr;

class SimpleTypeDescr : public SizedTypeDescr
{
};





class ScalarTypeDescr : public SimpleTypeDescr
{
  public:
    static const Class class_;
    static const JSFunctionSpec typeObjectMethods[];
    typedef ScalarTypeRepresentation TypeRepr;

    static bool call(JSContext *cx, unsigned argc, Value *vp);
};




class ReferenceTypeDescr : public SimpleTypeDescr
{
  public:
    static const Class class_;
    static const JSFunctionSpec typeObjectMethods[];
    typedef ReferenceTypeRepresentation TypeRepr;

    static bool call(JSContext *cx, unsigned argc, Value *vp);
};




class X4TypeDescr : public SizedTypeDescr
{
  private:
  public:
    static const Class class_;

    static bool call(JSContext *cx, unsigned argc, Value *vp);
    static bool is(const Value &v);
};






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
        return getReservedSlot(JS_TYPEOBJ_SLOT_ARRAY_ELEM_TYPE).toObject().as<SizedTypeDescr>();
    }
};




class SizedArrayTypeDescr : public SizedTypeDescr
{
  public:
    static const Class class_;

    SizedTypeDescr &elementType() {
        return getReservedSlot(JS_TYPEOBJ_SLOT_ARRAY_ELEM_TYPE).toObject().as<SizedTypeDescr>();
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

    static bool convertAndCopyTo(JSContext *cx,
                                 StructTypeRepresentation *typeRepr,
                                 HandleValue from, uint8_t *mem);
};

class StructTypeDescr : public SizedTypeDescr {
  public:
    static const Class class_;
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
        JS_ASSERT(typeRepresentation()->isAnyArray());
        return getReservedSlot(JS_DATUM_SLOT_LENGTH).toInt32();
    }

    size_t size() const {
        TypeRepresentation *typeRepr = typeRepresentation();
        switch (typeRepr->kind()) {
          case TypeRepresentation::Scalar:
          case TypeRepresentation::X4:
          case TypeRepresentation::Reference:
          case TypeRepresentation::Struct:
          case TypeRepresentation::SizedArray:
            return typeRepr->asSized()->size();

          case TypeRepresentation::UnsizedArray:
            return typeRepr->asUnsizedArray()->element()->size() * length();
        }
        MOZ_ASSUME_UNREACHABLE("unhandled typerepresentation kind");
    }

    uint8_t *typedMem(size_t offset) const {
        JS_ASSERT(offset < size());
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

