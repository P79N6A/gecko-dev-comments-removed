





#ifndef builtin_TypedObject_h
#define builtin_TypedObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "builtin/TypedObjectSimple.h"
#include "vm/TypedArrayObject.h"



















































































namespace js {






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

bool InitializeCommonTypeDescriptorProperties(JSContext *cx,
                                              HandleTypeDescr obj,
                                              HandleObject typeReprOwnerObj);






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





class TypedDatum : public ArrayBufferViewObject
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

    
    
    
    
    
    
    
    
    static TypedDatum *createUnattached(JSContext *cx, HandleTypeDescr type,
                                        int32_t length);

    
    
    
    static TypedDatum *createDerived(JSContext *cx,
                                     HandleSizedTypeDescr type,
                                     Handle<TypedDatum*> typedContents,
                                     size_t offset);

    
    
    
    static TypedDatum *createZeroed(JSContext *cx,
                                    HandleTypeDescr typeObj,
                                    int32_t length);

    
    
    
    static bool constructSized(JSContext *cx, unsigned argc, Value *vp);

    
    static bool constructUnsized(JSContext *cx, unsigned argc, Value *vp);

    
    void attach(ArrayBufferObject &buffer, int32_t offset);

    
    void attach(TypedDatum &datum, int32_t offset);

    
    void neuter(JSContext *cx);

    int32_t offset() const {
        return getReservedSlot(JS_DATUM_SLOT_BYTEOFFSET).toInt32();
    }

    ArrayBufferObject &owner() const {
        return getReservedSlot(JS_DATUM_SLOT_OWNER).toObject().as<ArrayBufferObject>();
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

class TransparentTypedObject : public TypedDatum
{
  public:
    static const Class class_;
};

typedef Handle<TransparentTypedObject*> HandleTransparentTypedObject;

class OpaqueTypedObject : public TypedDatum
{
  public:
    static const Class class_;
    static const JSFunctionSpec handleStaticMethods[];
};






bool NewOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp);






bool NewDerivedTypedDatum(JSContext *cx, unsigned argc, Value *vp);







bool AttachDatum(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo AttachDatumJitInfo;






bool ObjectIsTypeDescr(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypeDescrJitInfo;






bool ObjectIsOpaqueTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsOpaqueTypedObjectJitInfo;






bool ObjectIsTransparentTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTransparentTypedObjectJitInfo;







bool DatumIsAttached(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo DatumIsAttachedJitInfo;






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
IsTypedDatumClass(const Class *class_)
{
    return class_ == &TransparentTypedObject::class_ ||
           class_ == &OpaqueTypedObject::class_;
}

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
    return IsTypedDatumClass(getClass());
}

#endif

