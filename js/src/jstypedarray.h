






































#ifndef jstypedarray_h
#define jstypedarray_h

#include "jsapi.h"
#include "jsclass.h"

#include "gc/Barrier.h"

typedef struct JSProperty JSProperty;

namespace js {









class ArrayBufferObject : public JSObject
{
  public:
    static Class protoClass;
    static JSPropertySpec jsprops[];
    static JSFunctionSpec jsfuncs[];

    static JSBool prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);

    static JSBool fun_slice(JSContext *cx, unsigned argc, Value *vp);

    static JSBool class_constructor(JSContext *cx, unsigned argc, Value *vp);

    static JSObject *create(JSContext *cx, uint32_t nbytes, uint8_t *contents = NULL);

    static JSObject *createSlice(JSContext *cx, ArrayBufferObject &arrayBuffer,
                                 uint32_t begin, uint32_t end);

    static void
    obj_trace(JSTracer *trc, JSObject *obj);

    static JSBool
    obj_lookupGeneric(JSContext *cx, JSObject *obj, jsid id,
                      JSObject **objp, JSProperty **propp);
    static JSBool
    obj_lookupProperty(JSContext *cx, JSObject *obj, PropertyName *name,
                       JSObject **objp, JSProperty **propp);
    static JSBool
    obj_lookupElement(JSContext *cx, JSObject *obj, uint32_t index,
                      JSObject **objp, JSProperty **propp);
    static JSBool
    obj_lookupSpecial(JSContext *cx, JSObject *obj, SpecialId sid, JSObject **objp,
                      JSProperty **propp);

    static JSBool
    obj_defineGeneric(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                      PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool
    obj_defineProperty(JSContext *cx, JSObject *obj, PropertyName *name, const Value *v,
                       PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool
    obj_defineElement(JSContext *cx, JSObject *obj, uint32_t index, const Value *v,
                      PropertyOp getter, StrictPropertyOp setter, unsigned attrs);
    static JSBool
    obj_defineSpecial(JSContext *cx, JSObject *obj, SpecialId sid, const Value *v,
                      PropertyOp getter, StrictPropertyOp setter, unsigned attrs);

    static JSBool
    obj_getGeneric(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);

    static JSBool
    obj_getProperty(JSContext *cx, JSObject *obj, JSObject *receiver, PropertyName *name,
                    Value *vp);

    static JSBool
    obj_getElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t index, Value *vp);
    static JSBool
    obj_getElementIfPresent(JSContext *cx, JSObject *obj, JSObject *receiver, uint32_t index,
                            Value *vp, bool *present);

    static JSBool
    obj_getSpecial(JSContext *cx, JSObject *obj, JSObject *receiver, SpecialId sid, Value *vp);

    static JSBool
    obj_setGeneric(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);
    static JSBool
    obj_setProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *vp, JSBool strict);
    static JSBool
    obj_setElement(JSContext *cx, JSObject *obj, uint32_t index, Value *vp, JSBool strict);
    static JSBool
    obj_setSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *vp, JSBool strict);

    static JSBool
    obj_getGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp);
    static JSBool
    obj_getPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp);
    static JSBool
    obj_getElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp);
    static JSBool
    obj_getSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp);

    static JSBool
    obj_setGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp);
    static JSBool
    obj_setPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp);
    static JSBool
    obj_setElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp);
    static JSBool
    obj_setSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp);

    static JSBool
    obj_deleteProperty(JSContext *cx, JSObject *obj, PropertyName *name, Value *rval, JSBool strict);
    static JSBool
    obj_deleteElement(JSContext *cx, JSObject *obj, uint32_t index, Value *rval, JSBool strict);
    static JSBool
    obj_deleteSpecial(JSContext *cx, JSObject *obj, SpecialId sid, Value *rval, JSBool strict);

    static JSBool
    obj_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                  Value *statep, jsid *idp);

    static JSType
    obj_typeOf(JSContext *cx, JSObject *obj);

    bool
    allocateSlots(JSContext *cx, uint32_t size, uint8_t *contents = NULL);

    inline uint32_t byteLength() const;

    inline uint8_t * dataPointer() const;

   



    inline bool hasData() const;
};









struct TypedArray {
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

    enum {
        
        FIELD_LENGTH = 0,
        FIELD_BYTEOFFSET,
        FIELD_BYTELENGTH,
        FIELD_TYPE,
        FIELD_BUFFER,
        FIELD_MAX,
        NUM_FIXED_SLOTS = 7
    };

    
    static Class classes[TYPE_MAX];

    
    
    static Class protoClasses[TYPE_MAX];

    static JSPropertySpec jsprops[];

    static JSBool prop_getBuffer(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool prop_getByteOffset(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool prop_getLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);

    static JSBool obj_lookupGeneric(JSContext *cx, JSObject *obj, jsid id,
                                    JSObject **objp, JSProperty **propp);
    static JSBool obj_lookupProperty(JSContext *cx, JSObject *obj, PropertyName *name,
                                     JSObject **objp, JSProperty **propp);
    static JSBool obj_lookupElement(JSContext *cx, JSObject *obj, uint32_t index,
                                    JSObject **objp, JSProperty **propp);
    static JSBool obj_lookupSpecial(JSContext *cx, JSObject *obj, SpecialId sid,
                                    JSObject **objp, JSProperty **propp);

    static JSBool obj_getGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp);
    static JSBool obj_getPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp);
    static JSBool obj_getElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp);
    static JSBool obj_getSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp);

    static JSBool obj_setGenericAttributes(JSContext *cx, JSObject *obj, jsid id, unsigned *attrsp);
    static JSBool obj_setPropertyAttributes(JSContext *cx, JSObject *obj, PropertyName *name, unsigned *attrsp);
    static JSBool obj_setElementAttributes(JSContext *cx, JSObject *obj, uint32_t index, unsigned *attrsp);
    static JSBool obj_setSpecialAttributes(JSContext *cx, JSObject *obj, SpecialId sid, unsigned *attrsp);

    static uint32_t getLength(JSObject *obj);
    static uint32_t getByteOffset(JSObject *obj);
    static uint32_t getByteLength(JSObject *obj);
    static uint32_t getType(JSObject *obj);
    static ArrayBufferObject * getBuffer(JSObject *obj);
    static void * getDataOffset(JSObject *obj);

  public:
    static bool
    isArrayIndex(JSContext *cx, JSObject *obj, jsid id, uint32_t *ip = NULL);

    static inline uint32_t slotWidth(int atype) {
        switch (atype) {
          case js::TypedArray::TYPE_INT8:
          case js::TypedArray::TYPE_UINT8:
          case js::TypedArray::TYPE_UINT8_CLAMPED:
            return 1;
          case js::TypedArray::TYPE_INT16:
          case js::TypedArray::TYPE_UINT16:
            return 2;
          case js::TypedArray::TYPE_INT32:
          case js::TypedArray::TYPE_UINT32:
          case js::TypedArray::TYPE_FLOAT32:
            return 4;
          case js::TypedArray::TYPE_FLOAT64:
            return 8;
          default:
            JS_NOT_REACHED("invalid typed array type");
            return 0;
        }
    }

    static inline int slotWidth(JSObject *obj) {
        return slotWidth(getType(obj));
    }

    static int lengthOffset();
    static int dataOffset();
};

inline bool
IsTypedArrayClass(const Class *clasp)
{
    return &TypedArray::classes[0] <= clasp &&
           clasp < &TypedArray::classes[TypedArray::TYPE_MAX];
}

inline bool
IsTypedArrayProtoClass(const Class *clasp)
{
    return &TypedArray::protoClasses[0] <= clasp &&
           clasp < &TypedArray::protoClasses[TypedArray::TYPE_MAX];
}

inline bool
IsTypedArray(JSObject *obj)
{
    return IsTypedArrayClass(obj->getClass());
}

inline bool
IsTypedArrayProto(JSObject *obj)
{
    return IsTypedArrayProtoClass(obj->getClass());
}

} 

#endif 
