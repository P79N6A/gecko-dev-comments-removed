






































#ifndef jstypedarray_h
#define jstypedarray_h

#include "jsapi.h"
#include "jsvalue.h"

typedef struct JSProperty JSProperty;

namespace js {









struct JS_FRIEND_API(ArrayBuffer) {
    static Class slowClass;
    static Class fastClass;
    static JSPropertySpec jsprops[];

    static JSBool prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);

    static JSBool class_constructor(JSContext *cx, uintN argc, Value *vp);

    static JSObject *create(JSContext *cx, int32 nbytes);

    ArrayBuffer()
    {
    }

    ~ArrayBuffer();

    static void
    obj_trace(JSTracer *trc, JSObject *obj);

    static JSBool
    obj_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                       JSObject **objp, JSProperty **propp);

    static JSBool
    obj_defineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                       PropertyOp getter, StrictPropertyOp setter, uintN attrs);

    static JSBool
    obj_getProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp);

    static JSBool
    obj_setProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict);

    static JSBool
    obj_getAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

    static JSBool
    obj_setAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

    static JSBool
    obj_deleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict);

    static JSBool
    obj_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                  Value *statep, jsid *idp);

    static JSType
    obj_typeOf(JSContext *cx, JSObject *obj);

    static JSObject *
    getArrayBuffer(JSObject *obj);

    static inline unsigned int
    getByteLength(JSObject *obj) {
        return *((unsigned int*) obj->slots);
    }

    static inline uint8 *
    getDataOffset(JSObject *obj) {
        uint64 *base = ((uint64*)obj->slots) + 1;
        return (uint8*) base;
    }
};









struct JS_FRIEND_API(TypedArray) {
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

    
    static Class fastClasses[TYPE_MAX];

    
    
    static Class slowClasses[TYPE_MAX];

    static JSPropertySpec jsprops[];

    static TypedArray *fromJSObject(JSObject *obj);

    static JSBool prop_getBuffer(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool prop_getByteOffset(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static JSBool prop_getLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);

    static JSBool obj_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                                     JSObject **objp, JSProperty **propp);

    static void obj_trace(JSTracer *trc, JSObject *obj);

    static JSBool obj_getAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

    static JSBool obj_setAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp);

    static int32 lengthOffset() { return offsetof(TypedArray, length); }
    static int32 dataOffset() { return offsetof(TypedArray, data); }
    static int32 typeOffset() { return offsetof(TypedArray, type); }
    static void *offsetData(JSObject *obj, uint32 offs);

  public:
    TypedArray() : bufferJS(0) { }

    bool isArrayIndex(JSContext *cx, jsid id, jsuint *ip = NULL);
    bool valid() { return bufferJS != 0; }

    JSObject *bufferJS;
    uint32 byteOffset;
    uint32 byteLength;
    uint32 length;
    uint32 type;

    void *data;

    inline int slotWidth() const {
        switch (type) {
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
            JS_NOT_REACHED("invalid typed array");
            return 0;
        }
    }
};

} 



JS_FRIEND_API(JSObject *)
js_InitTypedArrayClasses(JSContext *cx, JSObject *obj);

JS_FRIEND_API(JSBool)
js_IsTypedArray(JSObject *obj);

JS_FRIEND_API(JSBool)
js_IsArrayBuffer(JSObject *obj);

JS_FRIEND_API(JSObject *)
js_CreateArrayBuffer(JSContext *cx, jsuint nbytes);





JS_FRIEND_API(JSObject *)
js_CreateTypedArray(JSContext *cx, jsint atype, jsuint nelements);






JS_FRIEND_API(JSObject *)
js_CreateTypedArrayWithArray(JSContext *cx, jsint atype, JSObject *arrayArg);








JS_FRIEND_API(JSObject *)
js_CreateTypedArrayWithBuffer(JSContext *cx, jsint atype, JSObject *bufArg,
                              jsint byteoffset, jsint length);

extern int32 JS_FASTCALL
js_TypedArray_uint8_clamp_double(const double x);

#endif 
