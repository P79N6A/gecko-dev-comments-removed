





#ifndef builtin_BinaryData_h
#define builtin_BinaryData_h

#include "jsapi.h"
#include "jsobj.h"
#include "jsfriendapi.h"
#include "gc/Heap.h"

namespace js {
typedef float float32_t;
typedef double float64_t;

enum {
    NUMERICTYPE_UINT8 = 0,
    NUMERICTYPE_UINT16,
    NUMERICTYPE_UINT32,
    NUMERICTYPE_UINT64,
    NUMERICTYPE_INT8,
    NUMERICTYPE_INT16,
    NUMERICTYPE_INT32,
    NUMERICTYPE_INT64,
    NUMERICTYPE_FLOAT32,
    NUMERICTYPE_FLOAT64,
    NUMERICTYPES
};

enum TypeCommonSlots {
    SLOT_MEMSIZE = 0,
    SLOT_ALIGN,
    TYPE_RESERVED_SLOTS
};

enum BlockCommonSlots {
    SLOT_DATATYPE = 0,
    SLOT_BLOCKREFOWNER,
    BLOCK_RESERVED_SLOTS
};

static Class DataClass = {
    "Data",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Data),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static Class TypeClass = {
    "Type",
    JSCLASS_HAS_CACHED_PROTO(JSProto_Type),
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

template <typename T>
class NumericType
{
    private:
        static Class * typeToClass();
    public:
        static bool convert(JSContext *cx, HandleValue val, T *converted);
        static bool reify(JSContext *cx, void *mem, MutableHandleValue vp);
        static JSBool call(JSContext *cx, unsigned argc, Value *vp);
};

template <typename T>
JS_ALWAYS_INLINE
bool NumericType<T>::reify(JSContext *cx, void *mem, MutableHandleValue vp)
{
    vp.setInt32(* ((T*)mem) );
    return true;
}

template <>
JS_ALWAYS_INLINE
bool NumericType<float32_t>::reify(JSContext *cx, void *mem, MutableHandleValue vp)
{
    vp.setNumber(* ((float32_t*)mem) );
    return true;
}

template <>
JS_ALWAYS_INLINE
bool NumericType<float64_t>::reify(JSContext *cx, void *mem, MutableHandleValue vp)
{
    vp.setNumber(* ((float64_t*)mem) );
    return true;
}

#define BINARYDATA_FOR_EACH_NUMERIC_TYPES(macro_)\
    macro_(NUMERICTYPE_UINT8,    uint8)\
    macro_(NUMERICTYPE_UINT16,   uint16)\
    macro_(NUMERICTYPE_UINT32,   uint32)\
    macro_(NUMERICTYPE_UINT64,   uint64)\
    macro_(NUMERICTYPE_INT8,     int8)\
    macro_(NUMERICTYPE_INT16,    int16)\
    macro_(NUMERICTYPE_INT32,    int32)\
    macro_(NUMERICTYPE_INT64,    int64)\
    macro_(NUMERICTYPE_FLOAT32,  float32)\
    macro_(NUMERICTYPE_FLOAT64,  float64)

#define BINARYDATA_NUMERIC_CLASSES(constant_, type_)\
{\
    #type_,\
    JSCLASS_HAS_RESERVED_SLOTS(1) |\
    JSCLASS_HAS_CACHED_PROTO(JSProto_##type_),\
    JS_PropertyStub,       /* addProperty */\
    JS_DeletePropertyStub, /* delProperty */\
    JS_PropertyStub,       /* getProperty */\
    JS_StrictPropertyStub, /* setProperty */\
    JS_EnumerateStub,\
    JS_ResolveStub,\
    JS_ConvertStub,\
    NULL,\
    NULL,\
    NumericType<type_##_t>::call,\
    NULL,\
    NULL,\
    NULL\
},

static Class NumericTypeClasses[NUMERICTYPES] = {
    BINARYDATA_FOR_EACH_NUMERIC_TYPES(BINARYDATA_NUMERIC_CLASSES)
};






class ArrayType : public JSObject
{
    private:
        static JSObject *create(JSContext *cx, HandleObject arrayTypeGlobal,
                                HandleObject elementType, uint32_t length);
    public:
        static Class class_;

        static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);
        static JSBool repeat(JSContext *cx, unsigned int argc, jsval *vp);

        static JSBool toString(JSContext *cx, unsigned int argc, jsval *vp);

        static uint32_t length(JSContext *cx, HandleObject obj);
        static JSObject *elementType(JSContext *cx, HandleObject obj);
        static bool convertAndCopyTo(JSContext *cx, HandleObject exemplar,
                            HandleValue from, uint8_t *mem);
        static bool reify(JSContext *cx, HandleObject type, HandleObject owner,
                          size_t offset, MutableHandleValue to);
};


class BinaryArray
{
    private:
        static JSObject *createEmpty(JSContext *cx, HandleObject type);

        
        static JSObject *create(JSContext *cx, HandleObject type);
        
        static JSObject *create(JSContext *cx, HandleObject type,
                                HandleValue initial);

    public:
        static Class class_;

        
        static JSObject *create(JSContext *cx, HandleObject type,
                                HandleObject owner, size_t offset);
        static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);

        static void finalize(FreeOp *op, JSObject *obj);
        static void obj_trace(JSTracer *tracer, JSObject *obj);

        static JSBool forEach(JSContext *cx, unsigned int argc, jsval *vp);
        static JSBool subarray(JSContext *cx, unsigned int argc, jsval *vp);
        static JSBool fill(JSContext *cx, unsigned int argc, jsval *vp);

        static JSBool obj_lookupGeneric(JSContext *cx, HandleObject obj,
                                        HandleId id, MutableHandleObject objp,
                                        MutableHandleShape propp);

        static JSBool obj_lookupProperty(JSContext *cx, HandleObject obj,
                                         HandlePropertyName name,
                                         MutableHandleObject objp,
                                         MutableHandleShape propp);

        static JSBool obj_lookupElement(JSContext *cx, HandleObject obj,
                                        uint32_t index, MutableHandleObject objp,
                                        MutableHandleShape propp);

        static JSBool obj_lookupSpecial(JSContext *cx, HandleObject obj,
                                        HandleSpecialId sid,
                                        MutableHandleObject objp,
                                        MutableHandleShape propp);

        static JSBool obj_getGeneric(JSContext *cx, HandleObject obj,
                                     HandleObject receiver,
                                     HandleId id,
                                     MutableHandleValue vp);

        static JSBool obj_getProperty(JSContext *cx, HandleObject obj,
                                      HandleObject receiver,
                                      HandlePropertyName name,
                                      MutableHandleValue vp);

        static JSBool obj_getElement(JSContext *cx, HandleObject obj,
                                     HandleObject receiver,
                                     uint32_t index,
                                     MutableHandleValue vp);

        static JSBool obj_getElementIfPresent(JSContext *cx, HandleObject obj,
                                              HandleObject receiver,
                                              uint32_t index,
                                              MutableHandleValue vp,
                                              bool *present);

        static JSBool obj_getSpecial(JSContext *cx, HandleObject obj,
                                     HandleObject receiver,
                                     HandleSpecialId sid,
                                     MutableHandleValue vp);

        static JSBool obj_setGeneric(JSContext *cx, HandleObject obj,
                                     HandleId id, MutableHandleValue vp,
                                     JSBool strict);

        static JSBool obj_setProperty(JSContext *cx, HandleObject obj,
                                      HandlePropertyName name,
                                      MutableHandleValue vp,
                                      JSBool strict);

        static JSBool obj_setElement(JSContext *cx, HandleObject obj,
                                     uint32_t index, MutableHandleValue vp,
                                     JSBool strict);

        static JSBool obj_setSpecial(JSContext *cx, HandleObject obj,
                                     HandleSpecialId sid,
                                     MutableHandleValue vp,
                                     JSBool strict);

        static JSBool obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                               HandleId id, unsigned *attrsp);

        static JSBool obj_getPropertyAttributes(JSContext *cx, HandleObject obj,
                                                HandlePropertyName name,
                                                unsigned *attrsp);

        static JSBool obj_getElementAttributes(JSContext *cx, HandleObject obj,
                                               uint32_t index, unsigned *attrsp);

        static JSBool obj_getSpecialAttributes(JSContext *cx, HandleObject obj,
                                               HandleSpecialId sid,
                                               unsigned *attrsp);

        static JSBool obj_enumerate(JSContext *cx, HandleObject obj,
                                    JSIterateOp enum_op,
                                    MutableHandleValue statep,
                                    MutableHandleId idp);

        static JSBool lengthGetter(JSContext *cx, unsigned int argc, jsval *vp);

};

class StructType : public JSObject
{
    private:
        static JSObject *create(JSContext *cx, HandleObject structTypeGlobal,
                                HandleObject fields);
        



        static bool layout(JSContext *cx, HandleObject structType,
                           HandleObject fields);

    public:
        static Class class_;

        static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);
        static JSBool toString(JSContext *cx, unsigned int argc, jsval *vp);

        static bool convertAndCopyTo(JSContext *cx, HandleObject exemplar,
                                     HandleValue from, uint8_t *mem);

        static bool reify(JSContext *cx, HandleObject type, HandleObject owner,
                          size_t offset, MutableHandleValue to);

        static void finalize(js::FreeOp *op, JSObject *obj);
};

class BinaryStruct : public JSObject
{
    private:
        static JSObject *createEmpty(JSContext *cx, HandleObject type);
        static JSObject *create(JSContext *cx, HandleObject type);

    public:
        static Class class_;

        static JSObject *create(JSContext *cx, HandleObject type,
                                HandleObject owner, size_t offset);
        static JSBool construct(JSContext *cx, unsigned int argc, jsval *vp);

        static void finalize(js::FreeOp *op, JSObject *obj);
        static void obj_trace(JSTracer *tracer, JSObject *obj);

        static JSBool obj_getGeneric(JSContext *cx, HandleObject obj,
                                     HandleObject receiver, HandleId id,
                                     MutableHandleValue vp);

        static JSBool obj_getProperty(JSContext *cx, HandleObject obj,
                                      HandleObject receiver,
                                      HandlePropertyName name,
                                      MutableHandleValue vp);

        static JSBool obj_getSpecial(JSContext *cx, HandleObject obj,
                                     HandleObject receiver, HandleSpecialId sid,
                                     MutableHandleValue vp);

        static JSBool obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                     MutableHandleValue vp, JSBool strict);

        static JSBool obj_setProperty(JSContext *cx, HandleObject obj,
                                      HandlePropertyName name,
                                      MutableHandleValue vp,
                                      JSBool strict);

};
}

#endif 
