






































#ifndef jstypedarray_h
#define jstypedarray_h

#include "jsapi.h"

typedef struct JSProperty JSProperty;

namespace js {









struct JS_FRIEND_API(ArrayBuffer) {
    static Class jsclass;
    static JSPropertySpec jsprops[];

    static JSBool prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp);
    static void class_finalize(JSContext *cx, JSObject *obj);

    static JSBool class_constructor(JSContext *cx, JSObject *obj, uintN argc, Value *argv,
                                    Value *rval);

    static bool create(JSContext *cx, JSObject *obj, uintN argc,
                       Value *argv, Value *rval);

    static ArrayBuffer *fromJSObject(JSObject *obj);

    ArrayBuffer()
        : data(0), byteLength()
    {
    }

    ~ArrayBuffer();

    bool allocateStorage(JSContext *cx, uint32 bytes);
    void freeStorage(JSContext *cx);

    void *offsetData(uint32 offs) {
        return (void*) (((intptr_t)data) + offs);
    }

    void *data;
    uint32 byteLength;
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

    static void obj_dropProperty(JSContext *cx, JSObject *obj, JSProperty *prop);

    static void obj_trace(JSTracer *trc, JSObject *obj);

    static JSBool obj_getAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                                    uintN *attrsp);

    static JSBool obj_setAttributes(JSContext *cx, JSObject *obj, jsid id, JSProperty *prop,
                                    uintN *attrsp);

    static int32 lengthOffset() { return offsetof(TypedArray, length); }
    static int32 dataOffset() { return offsetof(TypedArray, data); }
    static int32 typeOffset() { return offsetof(TypedArray, type); }

  public:
    TypedArray() : buffer(0) { }

    bool isArrayIndex(JSContext *cx, jsid id, jsuint *ip = NULL);
    bool valid() { return buffer != 0; }

    ArrayBuffer *buffer;
    JSObject *bufferJS;
    uint32 byteOffset;
    uint32 byteLength;
    uint32 length;
    uint32 type;

    void *data;
};

} 



JS_BEGIN_EXTERN_C

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







JS_FRIEND_API(JSBool)
js_ReparentTypedArrayToScope(JSContext *cx, JSObject *obj, JSObject *scope);

JS_END_EXTERN_C

#endif 
