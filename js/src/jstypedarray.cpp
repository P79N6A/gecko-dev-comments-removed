






































#include <string.h>

#include "jstypes.h"
#include "jsstdint.h"
#include "jsutil.h"
#include "jshash.h"
#include "jsprf.h"
#include "jsapi.h"
#include "jsarray.h"
#include "jsatom.h"
#include "jsbool.h"
#include "jsbuiltins.h"
#include "jscntxt.h"
#include "jsversion.h"
#include "jsgc.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstaticcheck.h"
#include "jsbit.h"
#include "jsvector.h"
#include "jstypedarray.h"

#include "jsobjinlines.h"

using namespace js;








ArrayBuffer *
ArrayBuffer::fromJSObject(JSObject *obj)
{
    while (!js_IsArrayBuffer(obj))
        obj = obj->getProto();
    return reinterpret_cast<ArrayBuffer*>(obj->getPrivate());
}

JSBool
ArrayBuffer::prop_getByteLength(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    ArrayBuffer *abuf = ArrayBuffer::fromJSObject(obj);
    if (abuf)
        *vp = INT_TO_JSVAL(jsint(abuf->byteLength));
    return true;
}

void
ArrayBuffer::class_finalize(JSContext *cx, JSObject *obj)
{
    ArrayBuffer *abuf = ArrayBuffer::fromJSObject(obj);
    if (abuf)
        abuf->freeStorage(cx);
    delete abuf;
}




JSBool
ArrayBuffer::class_constructor(JSContext *cx, JSObject *obj,
                               uintN argc, jsval *argv, jsval *rval)
{
    if (!JS_IsConstructing(cx)) {
        obj = NewBuiltinClassInstance(cx, &ArrayBuffer::jsclass);
        if (!obj)
            return false;
        *rval = OBJECT_TO_JSVAL(obj);
    }

    return create(cx, obj, argc, argv, rval);
}

bool
ArrayBuffer::create(JSContext *cx, JSObject *obj,
                    uintN argc, jsval *argv, jsval *rval)
{
    if (!obj) {
        obj = NewBuiltinClassInstance(cx, &ArrayBuffer::jsclass);
        if (!obj)
            return false;
        *rval = OBJECT_TO_JSVAL(obj);
    }

    if (argc == 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_TYPED_ARRAY_BAD_ARGS);
        return false;
    }

    int32_t nbytes;
    if (!ValueToECMAInt32(cx, argv[0], &nbytes))
        return false;
    if (nbytes < 0 || !INT_FITS_IN_JSVAL(nbytes)) {
        




        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_BAD_ARRAY_LENGTH);
        return false;
    }

    ArrayBuffer *abuf = new ArrayBuffer();
    if (!abuf) {
        JS_ReportOutOfMemory(cx);
        return false;
    }

    if (!abuf->allocateStorage(cx, nbytes)) {
        delete abuf;
        return false;
    }

    obj->setPrivate(abuf);
    return true;
}

bool
ArrayBuffer::allocateStorage(JSContext *cx, uint32 nbytes)
{
    JS_ASSERT(data == 0);

    if (nbytes) {
        data = cx->calloc(nbytes);
        if (!data) {
            JS_ReportOutOfMemory(cx);
            return false;
        }
    }

    byteLength = nbytes;
    return true;
}

void
ArrayBuffer::freeStorage(JSContext *cx)
{
    if (data) {
        cx->free(data);
#ifdef DEBUG
        
        data = NULL;
#endif
    }
}

ArrayBuffer::~ArrayBuffer()
{
    JS_ASSERT(data == NULL);
}









TypedArray *
TypedArray::fromJSObject(JSObject *obj)
{
    while (!js_IsTypedArray(obj))
        obj = obj->getProto();
    return reinterpret_cast<TypedArray*>(obj->getPrivate());
}

inline bool
TypedArray::isArrayIndex(JSContext *cx, jsid id, jsuint *ip)
{
    jsuint index;
    if (js_IdIsIndex(id, &index) && index < length) {
        if (ip)
            *ip = index;
        return true;
    }

    return false;
}

typedef jsval (* TypedArrayPropertyGetter)(TypedArray *tarray);

template <TypedArrayPropertyGetter Get>
class TypedArrayGetter {
  public:
    static inline bool get(JSContext *cx, JSObject *obj, jsid id, jsval *vp) {
        do {
            if (js_IsTypedArray(obj)) {
                TypedArray *tarray = TypedArray::fromJSObject(obj);
                if (tarray)
                    *vp = Get(tarray);
                return true;
            }
        } while ((obj = obj->getProto()) != NULL);
        return true;
    }
};

inline jsval
getBuffer(TypedArray *tarray)
{
    return OBJECT_TO_JSVAL(tarray->bufferJS);
}

JSBool
TypedArray::prop_getBuffer(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return TypedArrayGetter<getBuffer>::get(cx, obj, id, vp);
}

inline jsval
getByteOffset(TypedArray *tarray)
{
    return INT_TO_JSVAL(tarray->byteOffset);
}

JSBool
TypedArray::prop_getByteOffset(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return TypedArrayGetter<getByteOffset>::get(cx, obj, id, vp);
}

inline jsval
getByteLength(TypedArray *tarray)
{
    return INT_TO_JSVAL(tarray->byteLength);
}

JSBool
TypedArray::prop_getByteLength(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return TypedArrayGetter<getByteLength>::get(cx, obj, id, vp);
}

inline jsval
getLength(TypedArray *tarray)
{
    return INT_TO_JSVAL(tarray->length);
}

JSBool
TypedArray::prop_getLength(JSContext *cx, JSObject *obj, jsval id, jsval *vp)
{
    return TypedArrayGetter<getLength>::get(cx, obj, id, vp);
}

JSBool
TypedArray::obj_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                               JSObject **objp, JSProperty **propp)
{
    TypedArray *tarray = fromJSObject(obj);
    JS_ASSERT(tarray);

    if (tarray->isArrayIndex(cx, id)) {
        *propp = (JSProperty *) id;
        *objp = obj;
        return true;
    }

    JSObject *proto = obj->getProto();
    if (!proto) {
        *objp = NULL;
        *propp = NULL;
        return true;
    }

    return proto->lookupProperty(cx, id, objp, propp);
}

void
TypedArray::obj_trace(JSTracer *trc, JSObject *obj)
{
    TypedArray *tarray = fromJSObject(obj);
    JS_ASSERT(tarray);

    obj->traceProtoAndParent(trc);

    JS_CALL_OBJECT_TRACER(trc, tarray->bufferJS, "typedarray.buffer");
}

JSBool
TypedArray::obj_getAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    *attrsp = (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))
              ? JSPROP_PERMANENT | JSPROP_READONLY
              : JSPROP_PERMANENT | JSPROP_ENUMERATE;
    return true;
}

JSBool
TypedArray::obj_setAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                         JSMSG_CANT_SET_ARRAY_ATTRS);
    return false;
}



int32 JS_FASTCALL
js_TypedArray_uint8_clamp_double(const double x)
{
    
    if (!(x >= 0))
        return 0;

    if (x > 255)
        return 255;

    jsdouble toTruncate = x + 0.5;
    JSUint8 y = JSUint8(toTruncate);

    




    if (y == toTruncate) {
        







        return (y & ~1);
    }

    return y;
}

JS_DEFINE_CALLINFO_1(extern, INT32, js_TypedArray_uint8_clamp_double, DOUBLE, 1, nanojit::ACC_NONE)


struct uint8_clamped {
    uint8 val;

    uint8_clamped() { }
    uint8_clamped(const uint8_clamped& other) : val(other.val) { }

    
    uint8_clamped(uint8 x)    { *this = x; }
    uint8_clamped(uint16 x)   { *this = x; }
    uint8_clamped(uint32 x)   { *this = x; }
    uint8_clamped(int8 x)     { *this = x; }
    uint8_clamped(int16 x)    { *this = x; }
    uint8_clamped(int32 x)    { *this = x; }
    uint8_clamped(jsdouble x) { *this = x; }

    inline uint8_clamped& operator= (const uint8_clamped& x) {
        val = x.val;
        return *this;
    }

    inline uint8_clamped& operator= (uint8 x) {
        val = x;
        return *this;
    }

    inline uint8_clamped& operator= (uint16 x) {
        val = (x > 255) ? 255 : uint8(x);
        return *this;
    }

    inline uint8_clamped& operator= (uint32 x) {
        val = (x > 255) ? 255 : uint8(x);
        return *this;
    }

    inline uint8_clamped& operator= (int8 x) {
        val = (x >= 0) ? uint8(x) : 0;
        return *this;
    }

    inline uint8_clamped& operator= (int16 x) {
        val = (x >= 0)
              ? ((x < 255)
                 ? uint8(x)
                 : 255)
              : 0;
        return *this;
    }

    inline uint8_clamped& operator= (int32 x) { 
        val = (x >= 0)
              ? ((x < 255)
                 ? uint8(x)
                 : 255)
              : 0;
        return *this;
    }

    inline uint8_clamped& operator= (const jsdouble x) { 
        val = uint8(js_TypedArray_uint8_clamp_double(x));
        return *this;
    }

    inline operator uint8() const {
        return val;
    }
};


JS_STATIC_ASSERT(sizeof(uint8_clamped) == 1);

template<typename NativeType> static inline const int TypeIDOfType();
template<> inline const int TypeIDOfType<int8>() { return TypedArray::TYPE_INT8; }
template<> inline const int TypeIDOfType<uint8>() { return TypedArray::TYPE_UINT8; }
template<> inline const int TypeIDOfType<int16>() { return TypedArray::TYPE_INT16; }
template<> inline const int TypeIDOfType<uint16>() { return TypedArray::TYPE_UINT16; }
template<> inline const int TypeIDOfType<int32>() { return TypedArray::TYPE_INT32; }
template<> inline const int TypeIDOfType<uint32>() { return TypedArray::TYPE_UINT32; }
template<> inline const int TypeIDOfType<float>() { return TypedArray::TYPE_FLOAT32; }
template<> inline const int TypeIDOfType<double>() { return TypedArray::TYPE_FLOAT64; }
template<> inline const int TypeIDOfType<uint8_clamped>() { return TypedArray::TYPE_UINT8_CLAMPED; }

template<typename NativeType> static inline const bool TypeIsUnsigned() { return false; }
template<> inline const bool TypeIsUnsigned<uint8>() { return true; }
template<> inline const bool TypeIsUnsigned<uint16>() { return true; }
template<> inline const bool TypeIsUnsigned<uint32>() { return true; }

template<typename NativeType> static inline const bool TypeIsFloatingPoint() { return false; }
template<> inline const bool TypeIsFloatingPoint<float>() { return true; }
template<> inline const bool TypeIsFloatingPoint<double>() { return true; }

template<typename NativeType> class TypedArrayTemplate;

typedef TypedArrayTemplate<int8> Int8Array;
typedef TypedArrayTemplate<uint8> Uint8Array;
typedef TypedArrayTemplate<int16> Int16Array;
typedef TypedArrayTemplate<uint16> Uint16Array;
typedef TypedArrayTemplate<int32> Int32Array;
typedef TypedArrayTemplate<uint32> Uint32Array;
typedef TypedArrayTemplate<float> Float32Array;
typedef TypedArrayTemplate<double> Float64Array;
typedef TypedArrayTemplate<uint8_clamped> Uint8ClampedArray;

template<typename NativeType>
class TypedArrayTemplate
  : public TypedArray
{
  public:
    typedef TypedArrayTemplate<NativeType> ThisTypeArray;
    static const int ArrayTypeID() { return TypeIDOfType<NativeType>(); }
    static const bool ArrayTypeIsUnsigned() { return TypeIsUnsigned<NativeType>(); }
    static const bool ArrayTypeIsFloatingPoint() { return TypeIsFloatingPoint<NativeType>(); }

    static JSObjectOps fastObjectOps;
    static JSObjectMap fastObjectMap;

    static JSFunctionSpec jsfuncs[];

    static inline JSClass *slowClass()
    {
        return &TypedArray::slowClasses[ArrayTypeID()];
    }

    static inline JSClass *fastClass()
    {
        return &TypedArray::fastClasses[ArrayTypeID()];
    }

    static JSObjectOps *getObjectOps(JSContext *cx, JSClass *clasp)
    {
        return &fastObjectOps;
    }

    static JSBool
    obj_getProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
    {
        ThisTypeArray *tarray = ThisTypeArray::fromJSObject(obj);
        JS_ASSERT(tarray);

        if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
            *vp = INT_TO_JSVAL(tarray->length);
            return true;
        }

        jsuint index;
        if (tarray->isArrayIndex(cx, id, &index)) {
            
            tarray->copyIndexToValue(cx, index, vp);
        } else {
            JSObject *obj2;
            JSProperty *prop;
            JSScopeProperty *sprop;

            JSObject *proto = obj->getProto();
            if (!proto) {
                *vp = JSVAL_VOID;
                return true;
            }

            *vp = JSVAL_VOID;
            if (js_LookupPropertyWithFlags(cx, proto, id, cx->resolveFlags, &obj2, &prop) < 0)
                return false;

            if (prop) {
                if (obj2->isNative()) {
                    sprop = (JSScopeProperty *) prop;
                    if (!js_NativeGet(cx, obj, obj2, sprop, JSGET_METHOD_BARRIER, vp))
                        return false;
                    JS_UNLOCK_OBJ(cx, obj2);
                }
            }
        }

        return true;
    }

    static JSBool
    obj_setProperty(JSContext *cx, JSObject *obj, jsid id, jsval *vp)
    {
        ThisTypeArray *tarray = ThisTypeArray::fromJSObject(obj);
        JS_ASSERT(tarray);

        if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
            *vp = INT_TO_JSVAL(tarray->length);
            return true;
        }

        jsuint index;
        
        if (!tarray->isArrayIndex(cx, id, &index)) {
#if 0
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_INDEX);
            return false;
#endif
            
            
            
            
            
            *vp = JSVAL_VOID;
            return true;
        }

        if (JSVAL_IS_INT(*vp)) {
            tarray->setIndex(index, NativeType(JSVAL_TO_INT(*vp)));
            return true;
        }

        jsdouble d;

        if (JSVAL_IS_DOUBLE(*vp)) {
            d = *JSVAL_TO_DOUBLE(*vp);
        } else if (JSVAL_IS_NULL(*vp)) {
            d = 0.0f;
        } else if (JSVAL_IS_PRIMITIVE(*vp)) {
            JS_ASSERT(JSVAL_IS_STRING(*vp) || JSVAL_IS_SPECIAL(*vp));
            if (JSVAL_IS_STRING(*vp)) {
                
                ValueToNumber(cx, *vp, &d);
            } else if (*vp == JSVAL_VOID) {
                d = js_NaN;
            } else {
                d = (double) JSVAL_TO_BOOLEAN(*vp);
            }
        } else {
            
            d = js_NaN;
        }

        
        
        

        
        if (ArrayTypeIsFloatingPoint()) {
            tarray->setIndex(index, NativeType(d));
        } else if (ArrayTypeIsUnsigned()) {
            JS_ASSERT(sizeof(NativeType) <= 4);
            uint32 n = js_DoubleToECMAUint32(d);
            tarray->setIndex(index, NativeType(n));
        } else if (ArrayTypeID() == TypedArray::TYPE_UINT8_CLAMPED) {
            
            
            tarray->setIndex(index, NativeType(d));
        } else {
            JS_ASSERT(sizeof(NativeType) <= 4);
            int32 n = js_DoubleToECMAInt32(d);
            tarray->setIndex(index, NativeType(n));
        }

        return true;
    }

    static JSBool
    obj_defineProperty(JSContext *cx, JSObject *obj, jsid id, jsval value,
                       JSPropertyOp getter, JSPropertyOp setter, uintN attrs)
    {
        if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom))
            return true;

        return obj_setProperty(cx, obj, id, &value);
    }

    static JSBool
    obj_deleteProperty(JSContext *cx, JSObject *obj, jsval id, jsval *rval)
    {
        if (id == ATOM_TO_JSID(cx->runtime->atomState.lengthAtom)) {
            *rval = JSVAL_FALSE;
            return true;
        }

        TypedArray *tarray = TypedArray::fromJSObject(obj);
        JS_ASSERT(tarray);

        if (tarray->isArrayIndex(cx, id)) {
            *rval = JSVAL_FALSE;
            return true;
        }

        *rval = JSVAL_TRUE;
        return true;
    }

    static JSBool
    obj_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                  jsval *statep, jsid *idp)
    {
        ThisTypeArray *tarray = ThisTypeArray::fromJSObject(obj);
        JS_ASSERT(tarray);

        




        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
            *statep = JSVAL_TRUE;
            if (idp)
                *idp = INT_TO_JSID(tarray->length + 1);
            break;

          case JSENUMERATE_INIT:
            *statep = JSVAL_ZERO;
            if (idp)
                *idp = INT_TO_JSID(tarray->length);
            break;

          case JSENUMERATE_NEXT:
            if (*statep == JSVAL_TRUE) {
                *idp = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
                *statep = JSVAL_ZERO;
            } else {
                uint32 index = JSVAL_TO_INT(*statep);
                if (index < uint32(tarray->length)) {
                    *idp = *statep;
                    *statep = INT_TO_JSID(JSVAL_TO_INT(*statep) + 1);
                } else {
                    JS_ASSERT(index == tarray->length);
                    *statep = JSVAL_NULL;
                }
            }
            break;

          case JSENUMERATE_DESTROY:
            *statep = JSVAL_NULL;
            break;
        }

        return true;
    }

    static JSType
    obj_typeOf(JSContext *cx, JSObject *obj)
    {
        return JSTYPE_OBJECT;
    }

    





    static JSBool
    class_constructor(JSContext *cx, JSObject *obj,
                      uintN argc, jsval *argv, jsval *rval)
    {
        
        
        

        if (!JS_IsConstructing(cx)) {
            obj = NewBuiltinClassInstance(cx, slowClass());
            if (!obj)
                return false;
            *rval = OBJECT_TO_JSVAL(obj);
        }

        return create(cx, obj, argc, argv, rval);
    }

    static bool
    create(JSContext *cx, JSObject *obj, uintN argc, jsval *argv, jsval *rval)
    {
        if (!obj) {
            obj = NewBuiltinClassInstance(cx, slowClass());
            if (!obj)
                return false;
            *rval = OBJECT_TO_JSVAL(obj);
        }

        ThisTypeArray *tarray = 0;

        
        if (argc == 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        
        if (JSVAL_IS_INT(argv[0])) {
            int32 len = JSVAL_TO_INT(argv[0]);
            if (len < 0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_BAD_ARRAY_LENGTH);

                return false;
            }

            tarray = new ThisTypeArray();
            if (!tarray) {
                JS_ReportOutOfMemory(cx);
                return false;
            }

            if (!tarray->init(cx, len)) {
                delete tarray;
                return false;
            }
        } else if (!JSVAL_IS_PRIMITIVE(argv[0])) {
            int32_t byteOffset = -1;
            int32_t length = -1;

            if (argc > 1) {
                if (!ValueToInt32(cx, argv[1], &byteOffset))
                    return false;
                if (byteOffset < 0) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TYPED_ARRAY_NEGATIVE_ARG, "1");
                    return false;
                }
            }

            if (argc > 2) {
                if (!ValueToInt32(cx, argv[2], &length))
                    return false;
                if (length < 0) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TYPED_ARRAY_NEGATIVE_ARG, "2");
                    return false;
                }
            }

            tarray = new ThisTypeArray();
            if (!tarray) {
                JS_ReportOutOfMemory(cx);
                return false;
            }

            if (!tarray->init(cx, JSVAL_TO_OBJECT(argv[0]), byteOffset, length)) {
                delete tarray;
                return false;
            }
        } else {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        makeFastWithPrivate(cx, obj, tarray);
        return true;
    }

    static void
    class_finalize(JSContext *cx, JSObject *obj)
    {
        ThisTypeArray *tarray = ThisTypeArray::fromJSObject(obj);
        delete tarray;
    }

    
    static JSBool
    fun_slice(JSContext *cx, uintN argc, jsval *vp)
    {
        jsval *argv;
        JSObject *obj;

        argv = JS_ARGV(cx, vp);
        obj = JS_THIS_OBJECT(cx, vp);

        if (!JS_InstanceOf(cx, obj, ThisTypeArray::fastClass(), vp+2))
            return false;

        if (obj->getClass() != fastClass()) {
            
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_INCOMPATIBLE_METHOD,
                                 fastClass()->name, "slice", obj->getClass()->name);
            return false;
        }

        ThisTypeArray *tarray = ThisTypeArray::fromJSObject(obj);
        if (!tarray)
            return true;

        
        int32_t begin = 0, end = tarray->length;
        int32_t length = int32(tarray->length);

        if (argc > 0) {
            if (!ValueToInt32(cx, argv[0], &begin))
                return false;
            if (begin < 0) {
                begin += length;
                if (begin < 0)
                    begin = 0;
            } else if (begin > length) {
                begin = length;
            }

            if (argc > 1) {
                if (!ValueToInt32(cx, argv[1], &end))
                    return false;
                if (end < 0) {
                    end += length;
                    if (end < 0)
                        end = 0;
                } else if (end > length) {
                    end = length;
                }
            }
        }

        if (begin > end)
            begin = end;

        ThisTypeArray *ntarray = tarray->slice(begin, end);
        if (!ntarray) {
            
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        
        
        JSObject *nobj = JS_NewObject(cx, slowClass(), NULL, NULL);
        if (!nobj) {
            delete ntarray;
            return false;
        }

        makeFastWithPrivate(cx, nobj, ntarray);

        *vp = OBJECT_TO_JSVAL(nobj);
        return true;
    }

    static ThisTypeArray *
    fromJSObject(JSObject *obj)
    {
        JS_ASSERT(obj->getClass() == fastClass());
        return reinterpret_cast<ThisTypeArray*>(obj->getPrivate());
    }

    
    static void
    makeFastWithPrivate(JSContext *cx, JSObject *obj, ThisTypeArray *tarray)
    {
        JS_ASSERT(obj->getClass() == slowClass());

        obj->setPrivate(tarray);

        
        
        obj->classword ^= jsuword(slowClass());
        obj->classword |= jsuword(fastClass());

        obj->map = &fastObjectMap;
    }

  public:
    TypedArrayTemplate() { }

    bool
    init(JSContext *cx, uint32 len)
    {
        type = ArrayTypeID();
        return createBufferWithSizeAndCount(cx, sizeof(NativeType), len);
    }

    bool
    init(JSContext *cx, JSObject *other, int32 byteOffsetInt = -1, int32 lengthInt = -1)
    {
        type = ArrayTypeID();

        

        if (JS_IsArrayObject(cx, other)) {
            jsuint len;
            if (!JS_GetArrayLength(cx, other, &len))
                return false;
            if (!createBufferWithSizeAndCount(cx, sizeof(NativeType), len))
                return false;
            if (!copyFrom(cx, other, len))
                return false;
        } else if (js_IsTypedArray(other)) {
            TypedArray *tarray = TypedArray::fromJSObject(other);

            

            if (!createBufferWithSizeAndCount(cx, sizeof(NativeType), tarray->length))
                return false;
            if (!copyFrom(tarray))
                return false;
        } else if (other->getClass() == &ArrayBuffer::jsclass) {
            ArrayBuffer *abuf = ArrayBuffer::fromJSObject(other);

            
            uint32 boffset = (byteOffsetInt < 0) ? 0 : uint32(byteOffsetInt);

            if (boffset > abuf->byteLength || boffset % sizeof(NativeType) != 0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return false; 
            }

            uint32 len;
            if (lengthInt < 0) {
                len = (abuf->byteLength - boffset) / sizeof(NativeType);
                if (len * sizeof(NativeType) != (abuf->byteLength - boffset)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TYPED_ARRAY_BAD_ARGS);
                    return false; 
                }
            } else {
                len = (uint32) lengthInt;
            }

            
            uint32 arrayByteLength = len*sizeof(NativeType);
            if (uint32(len) >= INT32_MAX / sizeof(NativeType) ||
                uint32(boffset) >= INT32_MAX - arrayByteLength)
            {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return false; 
            }

            if (arrayByteLength + boffset > abuf->byteLength) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return false; 
            }

            buffer = abuf;
            bufferJS = other;
            byteOffset = boffset;
            byteLength = arrayByteLength;
            length = len;
            data = abuf->offsetData(boffset);
        } else {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        return true;
    }

    const NativeType
    getIndex(uint32 index) const
    {
        return *(static_cast<const NativeType*>(data) + index);
    }

    void
    setIndex(uint32 index, NativeType val)
    {
        *(static_cast<NativeType*>(data) + index) = val;
    }

    inline void copyIndexToValue(JSContext *cx, uint32 index, jsval *vp);

    ThisTypeArray *
    slice(uint32 begin, uint32 end)
    {
        if (begin > length || end > length)
            return NULL;

        ThisTypeArray *tarray = new ThisTypeArray();
        if (!tarray)
            return NULL;

        tarray->buffer = buffer;
        tarray->bufferJS = bufferJS;
        tarray->byteOffset = byteOffset + begin * sizeof(NativeType);
        tarray->byteLength = (end - begin) * sizeof(NativeType);
        tarray->length = end - begin;
        tarray->type = type;
        tarray->data = buffer->offsetData(tarray->byteOffset);

        return tarray;
    }

  protected:
    static NativeType
    nativeFromValue(JSContext *cx, jsval v)
    {
        if (JSVAL_IS_INT(v))
            return NativeType(JSVAL_TO_INT(v));

        if (JSVAL_IS_DOUBLE(v))
            return NativeType(*JSVAL_TO_DOUBLE(v));

        if (JSVAL_IS_PRIMITIVE(v) && v != JSVAL_HOLE) {
            jsdouble dval;
            ValueToNumber(cx, v, &dval);
            return NativeType(dval);
        }

        if (ArrayTypeIsFloatingPoint())
            return NativeType(js_NaN);

        return NativeType(int32(0));
    }
    
    bool
    copyFrom(JSContext *cx, JSObject *ar, jsuint len)
    {
        NativeType *dest = static_cast<NativeType*>(data);

        if (ar->isDenseArray() && ar->getDenseArrayCapacity() >= len) {
            JS_ASSERT(ar->getArrayLength() == len);

            jsval *src = ar->getDenseArrayElements();

            for (uintN i = 0; i < len; ++i) {
                jsval v = *src++;
                *dest++ = nativeFromValue(cx, v);
            }
        } else {
            
            jsval v;

            for (uintN i = 0; i < len; ++i) {
                if (!ar->getProperty(cx, INT_TO_JSID(i), &v))
                    return false;
                *dest++ = nativeFromValue(cx, v);
            }
        }

        return true;
    }

    bool
    copyFrom(TypedArray *tarray)
    {
        NativeType *dest = static_cast<NativeType*>(data);

        if (tarray->type == type) {
            memcpy(dest, tarray->data, tarray->byteLength);
            return true;
        }

        switch (tarray->type) {
          case TypedArray::TYPE_INT8: {
            int8 *src = static_cast<int8*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT8:
          case TypedArray::TYPE_UINT8_CLAMPED: {
            uint8 *src = static_cast<uint8*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_INT16: {
            int16 *src = static_cast<int16*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT16: {
            uint16 *src = static_cast<uint16*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_INT32: {
            int32 *src = static_cast<int32*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT32: {
            uint32 *src = static_cast<uint32*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_FLOAT32: {
            float *src = static_cast<float*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_FLOAT64: {
            double *src = static_cast<double*>(tarray->data);
            for (uintN i = 0; i < length; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          default:
            JS_NOT_REACHED("copyFrom with a TypedArray of unknown type");
            break;
        }

        return true;
    }

    bool
    createBufferWithSizeAndCount(JSContext *cx, uint32 size, uint32 count)
    {
        JS_ASSERT(size != 0);

        if (size != 0 && count >= INT32_MAX / size) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NEED_DIET, "size and count");
            return false;
        }

        int32 bytelen = size * count;
        if (!createBufferWithByteLength(cx, bytelen))
            return false;

        length = count;
        return true;
    }

    bool
    createBufferWithByteLength(JSContext *cx, int32 bytes)
    {
        if (!INT_FITS_IN_JSVAL(bytes)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NEED_DIET, "byte length");
            return false;
        }

        jsval argv = INT_TO_JSVAL(bytes);
        AutoValueRooter tvr(cx);
        if (!ArrayBuffer::create(cx, NULL, 1, &argv, tvr.addr()))
            return false;

        JSObject *obj = JSVAL_TO_OBJECT(tvr.value());

        bufferJS = obj;
        buffer = ArrayBuffer::fromJSObject(obj);

        byteOffset = 0;
        byteLength = bytes;
        data = buffer->data;

        return true;
    }
};



template<typename NativeType>
void
TypedArrayTemplate<NativeType>::copyIndexToValue(JSContext *cx, uint32 index, jsval *vp)
{
    JS_STATIC_ASSERT(sizeof(NativeType) < 4);

    *vp = INT_TO_JSVAL(getIndex(index));
}


template<>
void
TypedArrayTemplate<int32>::copyIndexToValue(JSContext *cx, uint32 index, jsval *vp)
{
    int32 val = getIndex(index);
    if (INT_FITS_IN_JSVAL(val)) {
        *vp = INT_TO_JSVAL(val);
    } else {
        jsdouble *dp = js_NewWeaklyRootedDouble(cx, jsdouble(val));
        *vp = dp ? DOUBLE_TO_JSVAL(dp) : JSVAL_VOID;
    }
}

template<>
void
TypedArrayTemplate<uint32>::copyIndexToValue(JSContext *cx, uint32 index, jsval *vp)
{
    uint32 val = getIndex(index);
    if (val < uint32(JSVAL_INT_MAX)) {
        *vp = INT_TO_JSVAL(int32(val));
    } else {
        jsdouble *dp = js_NewWeaklyRootedDouble(cx, jsdouble(val));
        *vp = dp ? DOUBLE_TO_JSVAL(dp) : JSVAL_VOID;
    }
}

template<>
void
TypedArrayTemplate<float>::copyIndexToValue(JSContext *cx, uint32 index, jsval *vp)
{
    float val = getIndex(index);
    if (!js_NewWeaklyRootedNumber(cx, jsdouble(val), vp))
        *vp = JSVAL_VOID;
}

template<>
void
TypedArrayTemplate<double>::copyIndexToValue(JSContext *cx, uint32 index, jsval *vp)
{
    double val = getIndex(index);
    if (!js_NewWeaklyRootedNumber(cx, jsdouble(val), vp))
        *vp = JSVAL_VOID;
}









JSClass ArrayBuffer::jsclass = {
    "ArrayBuffer",
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer),
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, ArrayBuffer::class_finalize,
    JSCLASS_NO_OPTIONAL_MEMBERS
};

JSPropertySpec ArrayBuffer::jsprops[] = {
    { "byteLength",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      ArrayBuffer::prop_getByteLength, ArrayBuffer::prop_getByteLength },
    {0,0,0,0,0}
};





JSPropertySpec TypedArray::jsprops[] = {
    { js_length_str,
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getLength, TypedArray::prop_getLength },
    { "byteLength",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getByteLength, TypedArray::prop_getByteLength },
    { "byteOffset",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getByteOffset, TypedArray::prop_getByteOffset },
    { "buffer",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getBuffer, TypedArray::prop_getBuffer },
    {0,0,0,0,0}
};






#define IMPL_TYPED_ARRAY_STATICS(_typedArray)                                  \
template<> JSObjectMap _typedArray::fastObjectMap(&_typedArray::fastObjectOps, \
                                                  JSObjectMap::SHAPELESS);     \
template<> JSObjectOps _typedArray::fastObjectOps = {                          \
    &_typedArray::fastObjectMap,                                               \
    _typedArray::obj_lookupProperty,                                           \
    _typedArray::obj_defineProperty,                                           \
    _typedArray::obj_getProperty,                                              \
    _typedArray::obj_setProperty,                                              \
    _typedArray::obj_getAttributes,                                            \
    _typedArray::obj_setAttributes,                                            \
    _typedArray::obj_deleteProperty,                                           \
    _typedArray::obj_enumerate,                                                \
    _typedArray::obj_typeOf,                                                   \
    _typedArray::obj_trace,                                                    \
    NULL,   /* thisObject */                                                   \
    NULL,   /* call */                                                         \
    NULL,   /* construct */                                                    \
    NULL,   /* hasInstance */                                                  \
    NULL    /* clear */                                                        \
};                                                                             \
template<> JSFunctionSpec _typedArray::jsfuncs[] = {                           \
    JS_FN("slice", _typedArray::fun_slice, 2, 0),                              \
    JS_FS_END                                                                  \
}

#define IMPL_TYPED_ARRAY_SLOW_CLASS(_typedArray)                               \
{                                                                              \
    #_typedArray,                                                              \
    JSCLASS_HAS_PRIVATE | JSCLASS_HAS_CACHED_PROTO(JSProto_##_typedArray),     \
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,        \
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub, JS_FinalizeStub,         \
    JSCLASS_NO_OPTIONAL_MEMBERS                                                \
}

#define IMPL_TYPED_ARRAY_FAST_CLASS(_typedArray)                               \
{                                                                              \
    #_typedArray,                                                              \
    JSCLASS_HAS_PRIVATE,                                                       \
    JS_PropertyStub, JS_PropertyStub, JS_PropertyStub, JS_PropertyStub,        \
    JS_EnumerateStub, JS_ResolveStub, JS_ConvertStub,                          \
    _typedArray::class_finalize,                                               \
    _typedArray::getObjectOps, NULL, NULL, NULL,                               \
    NULL, NULL, NULL, NULL                                                     \
}

#define INIT_TYPED_ARRAY_CLASS(_typedArray,_type)                              \
do {                                                                           \
    proto = js_InitClass(cx, obj, NULL,                                        \
                         &TypedArray::slowClasses[TypedArray::_type],          \
                         _typedArray::class_constructor, 3,                    \
                         _typedArray::jsprops,                                 \
                         _typedArray::jsfuncs,                                 \
                         NULL, NULL);                                          \
    if (!proto)                                                                \
        return NULL;                                                           \
    proto->setPrivate(0);                                                      \
} while (0)

IMPL_TYPED_ARRAY_STATICS(Int8Array);
IMPL_TYPED_ARRAY_STATICS(Uint8Array);
IMPL_TYPED_ARRAY_STATICS(Int16Array);
IMPL_TYPED_ARRAY_STATICS(Uint16Array);
IMPL_TYPED_ARRAY_STATICS(Int32Array);
IMPL_TYPED_ARRAY_STATICS(Uint32Array);
IMPL_TYPED_ARRAY_STATICS(Float32Array);
IMPL_TYPED_ARRAY_STATICS(Float64Array);
IMPL_TYPED_ARRAY_STATICS(Uint8ClampedArray);

JSClass TypedArray::fastClasses[TYPE_MAX] = {
    IMPL_TYPED_ARRAY_FAST_CLASS(Int8Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Uint8Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Int16Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Uint16Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Int32Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Uint32Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Float32Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Float64Array),
    IMPL_TYPED_ARRAY_FAST_CLASS(Uint8ClampedArray)
};

JSClass TypedArray::slowClasses[TYPE_MAX] = {
    IMPL_TYPED_ARRAY_SLOW_CLASS(Int8Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Uint8Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Int16Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Uint16Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Int32Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Uint32Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Float32Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Float64Array),
    IMPL_TYPED_ARRAY_SLOW_CLASS(Uint8ClampedArray)
};

JS_FRIEND_API(JSObject *)
js_InitTypedArrayClasses(JSContext *cx, JSObject *obj)
{
    
    JSObject *stop;
    if (!js_GetClassObject(cx, obj, JSProto_ArrayBuffer, &stop))
        return NULL;
    if (stop)
        return stop;

    JSObject *proto;

    INIT_TYPED_ARRAY_CLASS(Int8Array,TYPE_INT8);
    INIT_TYPED_ARRAY_CLASS(Uint8Array,TYPE_UINT8);
    INIT_TYPED_ARRAY_CLASS(Int16Array,TYPE_INT16);
    INIT_TYPED_ARRAY_CLASS(Uint16Array,TYPE_UINT16);
    INIT_TYPED_ARRAY_CLASS(Int32Array,TYPE_INT32);
    INIT_TYPED_ARRAY_CLASS(Uint32Array,TYPE_UINT32);
    INIT_TYPED_ARRAY_CLASS(Float32Array,TYPE_FLOAT32);
    INIT_TYPED_ARRAY_CLASS(Float64Array,TYPE_FLOAT64);
    INIT_TYPED_ARRAY_CLASS(Uint8ClampedArray,TYPE_UINT8_CLAMPED);

    proto = js_InitClass(cx, obj, NULL, &ArrayBuffer::jsclass,
                         ArrayBuffer::class_constructor, 1,
                         ArrayBuffer::jsprops, NULL, NULL, NULL);
    if (!proto)
        return NULL;

    proto->setPrivate(NULL);
    return proto;
}

JS_FRIEND_API(JSBool)
js_IsArrayBuffer(JSObject *obj)
{
    JS_ASSERT(obj);
    return obj->getClass() == &ArrayBuffer::jsclass;
}

JS_FRIEND_API(JSBool)
js_IsTypedArray(JSObject *obj)
{
    JS_ASSERT(obj);
    JSClass *clasp = obj->getClass();
    return clasp >= &TypedArray::fastClasses[0] &&
           clasp <  &TypedArray::fastClasses[TypedArray::TYPE_MAX];
}

JS_FRIEND_API(JSObject *)
js_CreateArrayBuffer(JSContext *cx, jsuint nbytes)
{
    AutoValueRooter tvr(cx);
    if (!js_NewNumberInRootedValue(cx, jsdouble(nbytes), tvr.addr()))
        return NULL;

    AutoValueRooter rval(cx);
    if (!ArrayBuffer::create(cx, NULL, 1, tvr.addr(), rval.addr()))
        return NULL;

    return JSVAL_TO_OBJECT(rval.value());
}

static inline JSBool
TypedArrayConstruct(JSContext *cx, jsint atype, uintN argc, jsval *argv, jsval *rv)
{
    switch (atype) {
      case TypedArray::TYPE_INT8:
        return !!Int8Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_UINT8:
        return !!Uint8Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_INT16:
        return !!Int16Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_UINT16:
        return !!Uint16Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_INT32:
        return !!Int32Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_UINT32:
        return !!Uint32Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_FLOAT32:
        return !!Float32Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_FLOAT64:
        return !!Float64Array::create(cx, NULL, argc, argv, rv);

      case TypedArray::TYPE_UINT8_CLAMPED:
        return !!Uint8ClampedArray::create(cx, NULL, argc, argv, rv);

      default:
        JS_NOT_REACHED("shouldn't have gotten here");
        return false;
    }
}

JS_FRIEND_API(JSObject *)
js_CreateTypedArray(JSContext *cx, jsint atype, jsuint nelements)
{
    JS_ASSERT(atype >= 0 && atype < TypedArray::TYPE_MAX);

    jsval vals[2] = { JSVAL_NULL, JSVAL_NULL };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vals), vals);

    if (!js_NewNumberInRootedValue(cx, jsdouble(nelements), &vals[0]))
        return NULL;

    if (!TypedArrayConstruct(cx, atype, 1, &vals[0], &vals[1]))
        return NULL;

    return JSVAL_TO_OBJECT(vals[1]);
}

JS_FRIEND_API(JSObject *)
js_CreateTypedArrayWithArray(JSContext *cx, jsint atype, JSObject *arrayArg)
{
    JS_ASSERT(atype >= 0 && atype < TypedArray::TYPE_MAX);

    jsval vals[2] = { JSVAL_NULL, JSVAL_NULL };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vals), vals);

    vals[0] = OBJECT_TO_JSVAL(arrayArg);

    if (!TypedArrayConstruct(cx, atype, 1, &vals[0], &vals[1]))
        return NULL;

    return JSVAL_TO_OBJECT(vals[1]);
}

JS_FRIEND_API(JSObject *)
js_CreateTypedArrayWithBuffer(JSContext *cx, jsint atype, JSObject *bufArg,
                              jsint byteoffset, jsint length)
{
    JS_ASSERT(atype >= 0 && atype < TypedArray::TYPE_MAX);
    JS_ASSERT(bufArg && ArrayBuffer::fromJSObject(bufArg));
    JS_ASSERT_IF(byteoffset < 0, length < 0);

    jsval vals[4] = { JSVAL_NULL, JSVAL_NULL, JSVAL_NULL, JSVAL_NULL };
    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vals), vals);

    int argc = 1;
    vals[0] = OBJECT_TO_JSVAL(bufArg);

    if (byteoffset >= 0) {
        if (!js_NewNumberInRootedValue(cx, jsdouble(byteoffset), &vals[argc]))
            return NULL;

        argc++;
    }

    if (length >= 0) {
        if (!js_NewNumberInRootedValue(cx, jsdouble(length), &vals[argc]))
            return NULL;

        argc++;
    }

    if (!TypedArrayConstruct(cx, atype, argc, &vals[0], &vals[3]))
        return NULL;

    return JSVAL_TO_OBJECT(vals[3]);
}

JS_FRIEND_API(JSBool)
js_ReparentTypedArrayToScope(JSContext *cx, JSObject *obj, JSObject *scope)
{
    JS_ASSERT(obj);

    scope = JS_GetGlobalForObject(cx, scope);
    if (!scope)
        return JS_FALSE;

    if (!js_IsTypedArray(obj))
        return JS_FALSE;

    TypedArray *typedArray = TypedArray::fromJSObject(obj);

    JSObject *buffer = typedArray->bufferJS;
    JS_ASSERT(js_IsArrayBuffer(buffer));

    JSObject *proto;
    JSProtoKey key =
        JSCLASS_CACHED_PROTO_KEY(&TypedArray::slowClasses[typedArray->type]);
    if (!js_GetClassPrototype(cx, scope, key, &proto))
        return JS_FALSE;

    obj->setProto(proto);
    obj->setParent(scope);

    key = JSCLASS_CACHED_PROTO_KEY(&ArrayBuffer::jsclass);
    if (!js_GetClassPrototype(cx, scope, key, &proto))
        return JS_FALSE;

    buffer->setProto(proto);
    buffer->setParent(scope);

    return JS_TRUE;
}
