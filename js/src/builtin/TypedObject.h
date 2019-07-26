





#ifndef builtin_TypedObject_h
#define builtin_TypedObject_h

#include "jsobj.h"

#include "builtin/TypedObjectConstants.h"
#include "builtin/TypeRepresentation.h"



















































































namespace js {






extern const Class TypedObjectClass;

template <ScalarTypeRepresentation::Type type, typename T>
class NumericType
{
  private:
    static const Class * typeToClass();
  public:
    static bool reify(JSContext *cx, void *mem, MutableHandleValue vp);
    static bool call(JSContext *cx, unsigned argc, Value *vp);
};






extern const Class NumericTypeClasses[ScalarTypeRepresentation::TYPE_MAX];




class ArrayType : public JSObject
{
  private:
  public:
    static const Class class_;

    
    
    static const JSPropertySpec typeObjectProperties[];
    static const JSFunctionSpec typeObjectMethods[];

    
    
    static const JSPropertySpec typedObjectProperties[];
    static const JSFunctionSpec typedObjectMethods[];

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *create(JSContext *cx, HandleObject arrayTypeGlobal,
                            HandleObject elementType, size_t length);
    static bool repeat(JSContext *cx, unsigned argc, Value *vp);
    static bool subarray(JSContext *cx, unsigned argc, Value *vp);

    static bool toSource(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *elementType(JSContext *cx, HandleObject obj);
};




class StructType : public JSObject
{
  private:
    static JSObject *create(JSContext *cx, HandleObject structTypeGlobal,
                            HandleObject fields);

    



    static bool layout(JSContext *cx, HandleObject structType,
                       HandleObject fields);

  public:
    static const Class class_;

    
    
    static const JSPropertySpec typeObjectProperties[];
    static const JSFunctionSpec typeObjectMethods[];

    
    
    static const JSPropertySpec typedObjectProperties[];
    static const JSFunctionSpec typedObjectMethods[];

    
    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);

    static bool toSource(JSContext *cx, unsigned argc, Value *vp);

    static bool convertAndCopyTo(JSContext *cx,
                                 StructTypeRepresentation *typeRepr,
                                 HandleValue from, uint8_t *mem);
};





class TypedDatum : public JSObject
{
  private:
    static const bool IsTypedDatumClass = true;

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

    static bool obj_getSpecial(JSContext *cx, HandleObject obj, HandleObject receiver,
                               HandleSpecialId sid, MutableHandleValue vp);

    static bool obj_getElementIfPresent(JSContext *cx, HandleObject obj,
                                        HandleObject receiver, uint32_t index,
                                        MutableHandleValue vp, bool *present);
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
                                                 HandleObject type);

    
    
    
    
    
    
    
    template<class T>
    static T *createUnattached(JSContext *cx, HandleObject type);

    
    
    
    static TypedDatum *createDerived(JSContext *cx,
                                     HandleObject type,
                                     HandleObject typedContents,
                                     size_t offset);

    
    void attach(void *mem);

    
    void attach(JSObject &datum, uint32_t offset);
};

class TypedObject : public TypedDatum
{
  public:
    static const Class class_;

    
    static JSObject *createZeroed(JSContext *cx, HandleObject type);

    
    static bool construct(JSContext *cx, unsigned argc, Value *vp);
};

class TypedHandle : public TypedDatum
{
  public:
    static const Class class_;
    static const JSFunctionSpec handleStaticMethods[];
};






bool NewTypedHandle(JSContext *cx, unsigned argc, Value *vp);






bool NewDerivedTypedDatum(JSContext *cx, unsigned argc, Value *vp);







bool AttachHandle(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo AttachHandleJitInfo;






bool ObjectIsTypeObject(ThreadSafeContext *cx, unsigned argc, Value *vp);
extern const JSJitInfo ObjectIsTypeObjectJitInfo;






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









#define JS_STORE_SCALAR_CLASS_DEFN(_constant, T, _name)                       \
class StoreScalar##T {                                                        \
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



JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_STORE_SCALAR_CLASS_DEFN)
JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_LOAD_SCALAR_CLASS_DEFN)

} 

#endif 

