






































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
#include "jsgcmark.h"
#include "jsinterp.h"
#include "jslock.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jsstaticcheck.h"
#include "jsbit.h"
#include "jsvector.h"
#include "jstypedarray.h"
#include "jsutil.h"

#include "vm/GlobalObject.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"
#include "jstypedarrayinlines.h"

using namespace js;
using namespace js::gc;
using namespace js::types;


static const uint8 ARRAYBUFFER_RESERVED_SLOTS = 16;

static bool
ValueIsLength(JSContext *cx, const Value &v, jsuint *len)
{
    if (v.isInt32()) {
        int32_t i = v.toInt32();
        if (i < 0)
            return false;
        *len = i;
        return true;
    }

    if (v.isDouble()) {
        jsdouble d = v.toDouble();
        if (JSDOUBLE_IS_NaN(d))
            return false;

        jsuint length = jsuint(d);
        if (d != jsdouble(length))
            return false;

        *len = length;
        return true;
    }

    return false;
}














JSObject *
ArrayBuffer::getArrayBuffer(JSObject *obj)
{
    while (obj && !js_IsArrayBuffer(obj))
        obj = obj->getProto();
    return obj;
}

JSBool
ArrayBuffer::prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    JSObject *arrayBuffer = getArrayBuffer(obj);
    if (!arrayBuffer) {
        vp->setInt32(0);
        return true;
    }
    vp->setInt32(jsint(arrayBuffer->arrayBufferByteLength()));
    return true;
}




JSBool
ArrayBuffer::class_constructor(JSContext *cx, uintN argc, Value *vp)
{
    int32 nbytes = 0;
    if (argc > 0 && !ValueToECMAInt32(cx, vp[2], &nbytes))
        return false;

    JSObject *bufobj = create(cx, nbytes);
    if (!bufobj)
        return false;
    vp->setObject(*bufobj);
    return true;
}

bool
JSObject::allocateArrayBufferSlots(JSContext *cx, uint32 size)
{
    



    JS_ASSERT(isArrayBuffer() && !hasSlotsArray());

    uint32 bytes = size + sizeof(Value);
    if (size > sizeof(Value) * ARRAYBUFFER_RESERVED_SLOTS - sizeof(Value) ) {
        Value *tmpslots = (Value *)cx->calloc_(bytes);
        if (!tmpslots)
            return false;
        slots = tmpslots;
        




        capacity = bytes / sizeof(Value);
    } else {
        slots = fixedSlots();
        memset(slots, 0, bytes);
    }
    *((uint32*)slots) = size;
    return true;
}

static JSObject *
DelegateObject(JSContext *cx, JSObject *obj)
{
    if (!obj->getPrivate()) {
        JSObject *delegate = NewNonFunction<WithProto::Given>(cx, &ObjectClass, obj->getProto(), NULL);
        obj->setPrivate(delegate);
        return delegate;
    }
    return static_cast<JSObject*>(obj->getPrivate());
}

JSObject *
ArrayBuffer::create(JSContext *cx, int32 nbytes)
{
    JSObject *obj = NewBuiltinClassInstance(cx, &ArrayBuffer::slowClass);
    if (!obj)
        return NULL;

    if (nbytes < 0) {
        




        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_BAD_ARRAY_LENGTH);
        return NULL;
    }

    JS_ASSERT(obj->getClass() == &ArrayBuffer::slowClass);
    obj->setSharedNonNativeMap();
    obj->setClass(&ArrayBufferClass);

    



    if (!obj->allocateArrayBufferSlots(cx, nbytes))
        return NULL;

    return obj;
}

ArrayBuffer::~ArrayBuffer()
{
}

void
ArrayBuffer::obj_trace(JSTracer *trc, JSObject *obj)
{
    JSObject *delegate = static_cast<JSObject*>(obj->getPrivate());
    if (delegate)
        MarkObject(trc, *delegate, "arraybuffer.delegate");
}

static JSProperty * const PROPERTY_FOUND = reinterpret_cast<JSProperty *>(1);

JSBool
ArrayBuffer::obj_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                                JSObject **objp, JSProperty **propp)
{
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom)) {
        *propp = PROPERTY_FOUND;
        *objp = getArrayBuffer(obj);
        return true;
    }

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;

    JSBool delegateResult = delegate->lookupProperty(cx, id, objp, propp);

    




    if (!delegateResult)
        return false;

    if (*propp != NULL) {
        if (*objp == delegate)
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

JSBool
ArrayBuffer::obj_lookupElement(JSContext *cx, JSObject *obj, uint32 index,
                               JSObject **objp, JSProperty **propp)
{
    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;

    





    if (!delegate->lookupElement(cx, index, objp, propp))
        return false;

    if (*propp != NULL) {
        if (*objp == delegate)
            *objp = obj;
        return true;
    }

    if (JSObject *proto = obj->getProto())
        return proto->lookupElement(cx, index, objp, propp);

    *objp = NULL;
    *propp = NULL;
    return true;
}

JSBool
ArrayBuffer::obj_lookupSpecial(JSContext *cx, JSObject *obj, jsid id,
                               JSObject **objp, JSProperty **propp)
{
    return obj_lookupProperty(cx, obj, id, objp, propp);
}

JSBool
ArrayBuffer::obj_defineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                   PropertyOp getter, StrictPropertyOp setter, uintN attrs)
{
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom))
        return true;

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_DefineProperty(cx, delegate, id, v, getter, setter, attrs);
}

JSBool
ArrayBuffer::obj_defineElement(JSContext *cx, JSObject *obj, uint32 index, const Value *v,
                   PropertyOp getter, StrictPropertyOp setter, uintN attrs)
{
    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_DefineElement(cx, delegate, index, v, getter, setter, attrs);
}

JSBool
ArrayBuffer::obj_defineSpecial(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                               PropertyOp getter, StrictPropertyOp setter, uintN attrs)
{
    return obj_defineProperty(cx, obj, id, v, getter, setter, attrs);
}

JSBool
ArrayBuffer::obj_getProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    obj = getArrayBuffer(obj);
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom)) {
        vp->setInt32(obj->arrayBufferByteLength());
        return true;
    }

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_GetProperty(cx, delegate, receiver, id, vp);
}

JSBool
ArrayBuffer::obj_getElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32 index, Value *vp)
{
    JSObject *delegate = DelegateObject(cx, getArrayBuffer(obj));
    if (!delegate)
        return false;
    return js_GetElement(cx, delegate, receiver, index, vp);
}

JSBool
ArrayBuffer::obj_getSpecial(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
{
    return obj_getProperty(cx, obj, receiver, id, vp);
}

JSBool
ArrayBuffer::obj_setProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom))
        return true;

    if (JSID_IS_ATOM(id, cx->runtime->atomState.protoAtom)) {
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        
        JSObject *delegate = DelegateObject(cx, obj);
        if (!delegate)
            return false;

        JSObject *oldDelegateProto = delegate->getProto();

        if (!js_SetPropertyHelper(cx, delegate, id, 0, vp, strict))
            return false;

        if (delegate->getProto() != oldDelegateProto) {
            
            
            if (!SetProto(cx, obj, vp->toObjectOrNull(), true)) {
                
                
                SetProto(cx, delegate, oldDelegateProto, true);
                return false;
            }
        }
        return true;
    }

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;

    return js_SetPropertyHelper(cx, delegate, id, 0, vp, strict);
}

JSBool
ArrayBuffer::obj_setElement(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict)
{
    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;

    return js_SetElementHelper(cx, delegate, index, 0, vp, strict);
}

JSBool
ArrayBuffer::obj_setSpecial(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
{
    return obj_setProperty(cx, obj, id, vp, strict);
}

JSBool
ArrayBuffer::obj_getAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom)) {
        *attrsp = JSPROP_PERMANENT | JSPROP_READONLY;
        return true;
    }

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_GetAttributes(cx, delegate, id, attrsp);
}

JSBool
ArrayBuffer::obj_getElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_GetElementAttributes(cx, delegate, index, attrsp);
}

JSBool
ArrayBuffer::obj_getSpecialAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj_getAttributes(cx, obj, id, attrsp);
}

JSBool
ArrayBuffer::obj_setAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                             JSMSG_CANT_SET_ARRAY_ATTRS);
        return false;
    }

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_SetAttributes(cx, delegate, id, attrsp);
}

JSBool
ArrayBuffer::obj_setElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_SetElementAttributes(cx, delegate, index, attrsp);
}

JSBool
ArrayBuffer::obj_setSpecialAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj_setSpecialAttributes(cx, obj, id, attrsp);
}

JSBool
ArrayBuffer::obj_deleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    if (JSID_IS_ATOM(id, cx->runtime->atomState.byteLengthAtom)) {
        rval->setBoolean(false);
        return true;
    }

    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_DeleteProperty(cx, delegate, id, rval, strict);
}

JSBool
ArrayBuffer::obj_deleteElement(JSContext *cx, JSObject *obj, uint32 index, Value *rval, JSBool strict)
{
    JSObject *delegate = DelegateObject(cx, obj);
    if (!delegate)
        return false;
    return js_DeleteElement(cx, delegate, index, rval, strict);
}

JSBool
ArrayBuffer::obj_deleteSpecial(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
{
    return obj_deleteProperty(cx, obj, id, rval, strict);
}

JSBool
ArrayBuffer::obj_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
              Value *statep, jsid *idp)
{
    statep->setNull();
    return true;
}

JSType
ArrayBuffer::obj_typeOf(JSContext *cx, JSObject *obj)
{
    return JSTYPE_OBJECT;
}









JSObject *
TypedArray::getTypedArray(JSObject *obj)
{
    while (!js_IsTypedArray(obj))
        obj = obj->getProto();
    return obj;
}

inline bool
TypedArray::isArrayIndex(JSContext *cx, JSObject *obj, jsid id, jsuint *ip)
{
    jsuint index;
    if (js_IdIsIndex(id, &index) && index < getLength(obj)) {
        if (ip)
            *ip = index;
        return true;
    }

    return false;
}

typedef Value (* TypedArrayPropertyGetter)(JSObject *tarray);

template <TypedArrayPropertyGetter Get>
class TypedArrayGetter {
  public:
    static inline bool get(JSContext *cx, JSObject *obj, jsid id, Value *vp) {
        do {
            if (js_IsTypedArray(obj)) {
                JSObject *tarray = TypedArray::getTypedArray(obj);
                if (tarray)
                    *vp = Get(tarray);
                return true;
            }
        } while ((obj = obj->getProto()) != NULL);
        return true;
    }
};





inline Value
getBufferValue(JSObject *tarray)
{
    JSObject *buffer = TypedArray::getBuffer(tarray);
    return ObjectValue(*buffer);
}

JSBool
TypedArray::prop_getBuffer(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return TypedArrayGetter<getBufferValue>::get(cx, obj, id, vp);
}

inline Value
getByteOffsetValue(JSObject *tarray)
{
    return Int32Value(TypedArray::getByteOffset(tarray));
}

JSBool
TypedArray::prop_getByteOffset(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return TypedArrayGetter<getByteOffsetValue>::get(cx, obj, id, vp);
}

inline Value
getByteLengthValue(JSObject *tarray)
{
    return Int32Value(TypedArray::getByteLength(tarray));
}

JSBool
TypedArray::prop_getByteLength(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return TypedArrayGetter<getByteLengthValue>::get(cx, obj, id, vp);
}

inline Value
getLengthValue(JSObject *tarray)
{
    return Int32Value(TypedArray::getLength(tarray));
}

JSBool
TypedArray::prop_getLength(JSContext *cx, JSObject *obj, jsid id, Value *vp)
{
    return TypedArrayGetter<getLengthValue>::get(cx, obj, id, vp);
}

JSBool
TypedArray::obj_lookupProperty(JSContext *cx, JSObject *obj, jsid id,
                               JSObject **objp, JSProperty **propp)
{
    JSObject *tarray = getTypedArray(obj);
    JS_ASSERT(tarray);

    if (isArrayIndex(cx, tarray, id)) {
        *propp = PROPERTY_FOUND;
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

JSBool
TypedArray::obj_lookupElement(JSContext *cx, JSObject *obj, uint32 index,
                              JSObject **objp, JSProperty **propp)
{
    JSObject *tarray = getTypedArray(obj);
    JS_ASSERT(tarray);

    if (index < getLength(tarray)) {
        *propp = PROPERTY_FOUND;
        *objp = obj;
        return true;
    }

    if (JSObject *proto = obj->getProto())
        return proto->lookupElement(cx, index, objp, propp);

    *objp = NULL;
    *propp = NULL;
    return true;
}

JSBool
TypedArray::obj_lookupSpecial(JSContext *cx, JSObject *obj, jsid id,
                              JSObject **objp, JSProperty **propp)
{
    return obj_lookupProperty(cx, obj, id, objp, propp);
}

JSBool
TypedArray::obj_getAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    *attrsp = (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom))
              ? JSPROP_PERMANENT | JSPROP_READONLY
              : JSPROP_PERMANENT | JSPROP_ENUMERATE;
    return true;
}

JSBool
TypedArray::obj_getElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    *attrsp = JSPROP_PERMANENT | JSPROP_ENUMERATE;
    return true;
}

JSBool
TypedArray::obj_getSpecialAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj_getAttributes(cx, obj, id, attrsp);
}

JSBool
TypedArray::obj_setAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_SET_ARRAY_ATTRS);
    return false;
}

JSBool
TypedArray::obj_setElementAttributes(JSContext *cx, JSObject *obj, uint32 index, uintN *attrsp)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL, JSMSG_CANT_SET_ARRAY_ATTRS);
    return false;
}

JSBool
TypedArray::obj_setSpecialAttributes(JSContext *cx, JSObject *obj, jsid id, uintN *attrsp)
{
    return obj_setAttributes(cx, obj, id, attrsp);
}

 int
TypedArray::lengthOffset()
{
    return JSObject::getFixedSlotOffset(FIELD_LENGTH) + offsetof(jsval_layout, s.payload);
}

 int
TypedArray::dataOffset()
{
    return offsetof(JSObject, privateData);
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

JS_DEFINE_CALLINFO_1(extern, INT32, js_TypedArray_uint8_clamp_double, DOUBLE,
                     1, nanojit::ACCSET_NONE)


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

template<typename NativeType> static inline const bool ElementTypeMayBeDouble() { return false; }
template<> inline const bool ElementTypeMayBeDouble<uint32>() { return true; }
template<> inline const bool ElementTypeMayBeDouble<float>() { return true; }
template<> inline const bool ElementTypeMayBeDouble<double>() { return true; }

template<typename NativeType> class TypedArrayTemplate;

template<typename NativeType>
class TypedArrayTemplate
  : public TypedArray
{
  public:
    typedef NativeType ThisType;
    typedef TypedArrayTemplate<NativeType> ThisTypeArray;
    static const int ArrayTypeID() { return TypeIDOfType<NativeType>(); }
    static const bool ArrayTypeIsUnsigned() { return TypeIsUnsigned<NativeType>(); }
    static const bool ArrayTypeIsFloatingPoint() { return TypeIsFloatingPoint<NativeType>(); }
    static const bool ArrayElementTypeMayBeDouble() { return ElementTypeMayBeDouble<NativeType>(); }

    static const size_t BYTES_PER_ELEMENT = sizeof(ThisType);

    static inline Class *slowClass()
    {
        return &TypedArray::slowClasses[ArrayTypeID()];
    }

    static inline Class *fastClass()
    {
        return &TypedArray::fastClasses[ArrayTypeID()];
    }

    static void
    obj_trace(JSTracer *trc, JSObject *obj)
    {
        JSObject *buffer = static_cast<JSObject*>(getBuffer(obj));
        if (buffer)
            MarkObject(trc, *buffer, "typedarray.buffer");
    }

    static JSBool
    obj_getProperty(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
    {
        JSObject *tarray = getTypedArray(obj);

        if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
            vp->setNumber(getLength(tarray));
            return true;
        }

        jsuint index;
        if (isArrayIndex(cx, tarray, id, &index)) {
            
            copyIndexToValue(cx, tarray, index, vp);
        } else {
            JSObject *obj2;
            JSProperty *prop;
            const Shape *shape;

            JSObject *proto = obj->getProto();
            if (!proto) {
                vp->setUndefined();
                return true;
            }

            vp->setUndefined();
            if (!LookupPropertyWithFlags(cx, proto, id, cx->resolveFlags, &obj2, &prop))
                return false;

            if (prop) {
                if (obj2->isNative()) {
                    shape = (Shape *) prop;
                    if (!js_NativeGet(cx, obj, obj2, shape, JSGET_METHOD_BARRIER, vp))
                        return false;
                }
            }
        }

        return true;
    }

    static JSBool
    obj_getElement(JSContext *cx, JSObject *obj, JSObject *receiver, uint32 index, Value *vp)
    {
        JSObject *tarray = getTypedArray(obj);

        if (index < getLength(tarray)) {
            
            copyIndexToValue(cx, tarray, index, vp);
            return true;
        }

        JSObject *proto = obj->getProto();
        if (!proto) {
            vp->setUndefined();
            return true;
        }

        vp->setUndefined();

        jsid id;
        if (!IndexToId(cx, index, &id))
            return false;

        JSObject *obj2;
        JSProperty *prop;
        if (!LookupPropertyWithFlags(cx, proto, id, cx->resolveFlags, &obj2, &prop))
            return false;

        if (!prop || !obj2->isNative())
            return true;

        const Shape *shape = (Shape *) prop;
        return js_NativeGet(cx, obj, obj2, shape, JSGET_METHOD_BARRIER, vp);
    }

    static JSBool
    obj_getSpecial(JSContext *cx, JSObject *obj, JSObject *receiver, jsid id, Value *vp)
    {
        return obj_getProperty(cx, obj, receiver, id, vp);
    }

    static bool
    setElementTail(JSContext *cx, JSObject *tarray, uint32 index, Value *vp, JSBool strict)
    {
        JS_ASSERT(tarray);
        JS_ASSERT(index < getLength(tarray));

        if (vp->isInt32()) {
            setIndex(tarray, index, NativeType(vp->toInt32()));
            return true;
        }

        jsdouble d;
        if (vp->isDouble()) {
            d = vp->toDouble();
        } else if (vp->isNull()) {
            d = 0.0;
        } else if (vp->isPrimitive()) {
            JS_ASSERT(vp->isString() || vp->isUndefined() || vp->isBoolean());
            if (vp->isString()) {
                JS_ALWAYS_TRUE(ToNumber(cx, *vp, &d));
            } else if (vp->isUndefined()) {
                d = js_NaN;
            } else {
                d = double(vp->toBoolean());
            }
        } else {
            
            d = js_NaN;
        }

        
        
        

        
        if (ArrayTypeIsFloatingPoint()) {
            setIndex(tarray, index, NativeType(d));
        } else if (ArrayTypeIsUnsigned()) {
            JS_ASSERT(sizeof(NativeType) <= 4);
            uint32 n = js_DoubleToECMAUint32(d);
            setIndex(tarray, index, NativeType(n));
        } else if (ArrayTypeID() == TypedArray::TYPE_UINT8_CLAMPED) {
            
            
            setIndex(tarray, index, NativeType(d));
        } else {
            JS_ASSERT(sizeof(NativeType) <= 4);
            int32 n = js_DoubleToECMAInt32(d);
            setIndex(tarray, index, NativeType(n));
        }

        return true;
    }

    static JSBool
    obj_setProperty(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
    {
        JSObject *tarray = getTypedArray(obj);
        JS_ASSERT(tarray);

        if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
            vp->setNumber(getLength(tarray));
            return true;
        }

        jsuint index;
        
        if (!isArrayIndex(cx, tarray, id, &index)) {
            
            
            
            
            
            vp->setUndefined();
            return true;
        }

        return setElementTail(cx, tarray, index, vp, strict);
    }

    static JSBool
    obj_setElement(JSContext *cx, JSObject *obj, uint32 index, Value *vp, JSBool strict)
    {
        JSObject *tarray = getTypedArray(obj);
        JS_ASSERT(tarray);

        if (index >= getLength(tarray)) {
            
            
            
            
            
            vp->setUndefined();
            return true;
        }

        return setElementTail(cx, tarray, index, vp, strict);
    }

    static JSBool
    obj_setSpecial(JSContext *cx, JSObject *obj, jsid id, Value *vp, JSBool strict)
    {
        return obj_setProperty(cx, obj, id, vp, strict);
    }

    static JSBool
    obj_defineProperty(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                       PropertyOp getter, StrictPropertyOp setter, uintN attrs)
    {
        if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom))
            return true;

        Value tmp = *v;
        return obj_setProperty(cx, obj, id, &tmp, false);
    }

    static JSBool
    obj_defineElement(JSContext *cx, JSObject *obj, uint32 index, const Value *v,
                       PropertyOp getter, StrictPropertyOp setter, uintN attrs)
    {
        Value tmp = *v;
        return obj_setElement(cx, obj, index, &tmp, false);
    }

    static JSBool
    obj_defineSpecial(JSContext *cx, JSObject *obj, jsid id, const Value *v,
                      PropertyOp getter, StrictPropertyOp setter, uintN attrs)
    {
        return obj_defineProperty(cx, obj, id, v, getter, setter, attrs);
    }

    static JSBool
    obj_deleteProperty(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
    {
        if (JSID_IS_ATOM(id, cx->runtime->atomState.lengthAtom)) {
            rval->setBoolean(false);
            return true;
        }

        JSObject *tarray = TypedArray::getTypedArray(obj);
        JS_ASSERT(tarray);

        if (isArrayIndex(cx, tarray, id)) {
            rval->setBoolean(false);
            return true;
        }

        rval->setBoolean(true);
        return true;
    }

    static JSBool
    obj_deleteElement(JSContext *cx, JSObject *obj, uint32 index, Value *rval, JSBool strict)
    {
        JSObject *tarray = TypedArray::getTypedArray(obj);
        JS_ASSERT(tarray);

        if (index < getLength(tarray)) {
            rval->setBoolean(false);
            return true;
        }

        rval->setBoolean(true);
        return true;
    }

    static JSBool
    obj_deleteSpecial(JSContext *cx, JSObject *obj, jsid id, Value *rval, JSBool strict)
    {
        return obj_deleteProperty(cx, obj, id, rval, strict);
    }

    static JSBool
    obj_enumerate(JSContext *cx, JSObject *obj, JSIterateOp enum_op,
                  Value *statep, jsid *idp)
    {
        JSObject *tarray = getTypedArray(obj);
        JS_ASSERT(tarray);

        




        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
            statep->setBoolean(true);
            if (idp)
                *idp = ::INT_TO_JSID(getLength(tarray) + 1);
            break;

          case JSENUMERATE_INIT:
            statep->setInt32(0);
            if (idp)
                *idp = ::INT_TO_JSID(getLength(tarray));
            break;

          case JSENUMERATE_NEXT:
            if (statep->isTrue()) {
                *idp = ATOM_TO_JSID(cx->runtime->atomState.lengthAtom);
                statep->setInt32(0);
            } else {
                uint32 index = statep->toInt32();
                if (index < getLength(tarray)) {
                    *idp = ::INT_TO_JSID(index);
                    statep->setInt32(index + 1);
                } else {
                    JS_ASSERT(index == getLength(tarray));
                    statep->setNull();
                }
            }
            break;

          case JSENUMERATE_DESTROY:
            statep->setNull();
            break;
        }

        return true;
    }

    static JSType
    obj_typeOf(JSContext *cx, JSObject *obj)
    {
        return JSTYPE_OBJECT;
    }

    static JSObject *
    createTypedArray(JSContext *cx, JSObject *bufobj, uint32 byteOffset, uint32 len)
    {
        JS_ASSERT(bufobj->isArrayBuffer());
        JSObject *obj = NewBuiltinClassInstance(cx, slowClass());
        if (!obj)
            return NULL;

        



        JSProtoKey key = JSCLASS_CACHED_PROTO_KEY(slowClass());
        types::TypeObject *type = types::GetTypeCallerInitObject(cx, key);
        if (!type)
            return NULL;
        obj->setType(type);

        obj->setSlot(FIELD_TYPE, Int32Value(ArrayTypeID()));
        obj->setSlot(FIELD_BUFFER, ObjectValue(*bufobj));

        




        obj->setPrivate(bufobj->arrayBufferDataOffset() + byteOffset);

        obj->setSlot(FIELD_LENGTH, Int32Value(len));
        obj->setSlot(FIELD_BYTEOFFSET, Int32Value(byteOffset));
        obj->setSlot(FIELD_BYTELENGTH, Int32Value(len * sizeof(NativeType)));

        DebugOnly<uint32> bufferByteLength = getBuffer(obj)->arrayBufferByteLength();
        JS_ASSERT(bufferByteLength - getByteOffset(obj) >= getByteLength(obj));
        JS_ASSERT(getByteOffset(obj) <= bufferByteLength);
        JS_ASSERT(getBuffer(obj)->arrayBufferDataOffset() <= getDataOffset(obj));
        JS_ASSERT(getDataOffset(obj) <= offsetData(obj, bufferByteLength));

        JS_ASSERT(obj->getClass() == slowClass());
        obj->setSharedNonNativeMap();
        obj->setClass(fastClass());

        
        obj->flags |= JSObject::NOT_EXTENSIBLE;

        return obj;
    }

    





    static JSBool
    class_constructor(JSContext *cx, uintN argc, Value *vp)
    {
        
        JSObject *obj = create(cx, argc, JS_ARGV(cx, vp));
        if (!obj)
            return false;
        vp->setObject(*obj);
        return true;
    }

    static JSObject *
    create(JSContext *cx, uintN argc, Value *argv)
    {
        

        
        jsuint len = 0;
        if (argc == 0 || ValueIsLength(cx, argv[0], &len)) {
            JSObject *bufobj = createBufferWithSizeAndCount(cx, len);
            if (!bufobj)
                return NULL;

            return createTypedArray(cx, bufobj, 0, len);
        }

        
        if (!argv[0].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_ARGS);
            return NULL;
        }

        JSObject *dataObj = &argv[0].toObject();

        
        if (js_IsTypedArray(dataObj)) {
            JSObject *otherTypedArray = getTypedArray(dataObj);
            JS_ASSERT(otherTypedArray);

            uint32 len = getLength(otherTypedArray);
            JSObject *bufobj = createBufferWithSizeAndCount(cx, len);
            if (!bufobj)
                return NULL;

            JSObject *obj = createTypedArray(cx, bufobj, 0, len);
            if (!obj || !copyFromTypedArray(cx, obj, otherTypedArray, 0))
                return NULL;
            return obj;
        }

        
        int32_t byteOffset = -1;
        int32_t length = -1;

        if (argc > 1) {
            if (!ValueToInt32(cx, argv[1], &byteOffset))
                return NULL;
            if (byteOffset < 0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_NEGATIVE_ARG, "1");
                return NULL;
            }

            if (argc > 2) {
                if (!ValueToInt32(cx, argv[2], &length))
                    return NULL;
                if (length < 0) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TYPED_ARRAY_NEGATIVE_ARG, "2");
                    return NULL;
                }
            }
        }

        
        return createTypedArrayWithOffsetLength(cx, dataObj, byteOffset, length);
    }

    
    static JSBool
    fun_subarray(JSContext *cx, uintN argc, Value *vp)
    {
        JSObject *obj = ToObject(cx, &vp[1]);
        if (!obj)
            return false;

        if (obj->getClass() != fastClass()) {
            
            ReportIncompatibleMethod(cx, vp, fastClass());
            return false;
        }

        JSObject *tarray = getTypedArray(obj);
        if (!tarray)
            return true;

        
        int32_t begin = 0, end = getLength(tarray);
        int32_t length = int32(getLength(tarray));

        if (argc > 0) {
            Value *argv = JS_ARGV(cx, vp);
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

        JSObject *nobj = createSubarray(cx, tarray, begin, end);
        if (!nobj)
            return false;
        vp->setObject(*nobj);
        return true;
    }

    
    static JSBool
    fun_set(JSContext *cx, uintN argc, Value *vp)
    {
        JSObject *obj = ToObject(cx, &vp[1]);
        if (!obj)
            return false;

        if (obj->getClass() != fastClass()) {
            
            ReportIncompatibleMethod(cx, vp, fastClass());
            return false;
        }

        JSObject *tarray = getTypedArray(obj);
        if (!tarray)
            return true;

        
        int32_t off = 0;

        Value *argv = JS_ARGV(cx, vp);
        if (argc > 1) {
            if (!ValueToInt32(cx, argv[1], &off))
                return false;

            if (off < 0 || uint32_t(off) > getLength(tarray)) {
                
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return false;
            }
        }

        uint32 offset(off);

        
        if (argc == 0 || !argv[0].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        JSObject *arg0 = argv[0].toObjectOrNull();
        if (js_IsTypedArray(arg0)) {
            JSObject *src = TypedArray::getTypedArray(arg0);
            if (!src ||
                getLength(src) > getLength(tarray) - offset)
            {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return false;
            }

            if (!copyFromTypedArray(cx, obj, src, offset))
                return false;
        } else {
            jsuint len;
            if (!js_GetLengthProperty(cx, arg0, &len))
                return false;

            
            if (len > getLength(tarray) - offset) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return false;
            }

            if (!copyFromArray(cx, obj, arg0, len, offset))
                return false;
        }

        vp->setUndefined();
        return true;
    }

  public:
    static JSObject *
    createTypedArrayWithOffsetLength(JSContext *cx, JSObject *other,
                                     int32 byteOffsetInt, int32 lengthInt)
    {
        JS_ASSERT(!js_IsTypedArray(other));

        
        if (other->isArrayBuffer()) {
            uint32 boffset = (byteOffsetInt < 0) ? 0 : uint32(byteOffsetInt);

            if (boffset > other->arrayBufferByteLength() || boffset % sizeof(NativeType) != 0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return NULL; 
            }

            uint32 len;
            if (lengthInt < 0) {
                len = (other->arrayBufferByteLength() - boffset) / sizeof(NativeType);
                if (len * sizeof(NativeType) != (other->arrayBufferByteLength() - boffset)) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                         JSMSG_TYPED_ARRAY_BAD_ARGS);
                    return NULL; 
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
                return NULL; 
            }

            if (arrayByteLength + boffset > other->arrayBufferByteLength()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return NULL; 
            }

            return createTypedArray(cx, other, boffset, len);
        }

        



        jsuint len;
        if (!js_GetLengthProperty(cx, other, &len))
            return NULL;

        JSObject *bufobj = createBufferWithSizeAndCount(cx, len);
        if (!bufobj)
            return NULL;

        JSObject *obj = createTypedArray(cx, bufobj, 0, len);
        if (!obj || !copyFromArray(cx, obj, other, len))
            return NULL;
        return obj;
    }

    static const NativeType
    getIndex(JSObject *obj, uint32 index)
    {
        return *(static_cast<const NativeType*>(getDataOffset(obj)) + index);
    }

    static void
    setIndex(JSObject *obj, uint32 index, NativeType val)
    {
        *(static_cast<NativeType*>(getDataOffset(obj)) + index) = val;
    }

    static void copyIndexToValue(JSContext *cx, JSObject *tarray, uint32 index, Value *vp);

    static JSObject *
    createSubarray(JSContext *cx, JSObject *tarray, uint32 begin, uint32 end)
    {
        JS_ASSERT(tarray);

        JS_ASSERT(0 <= begin);
        JS_ASSERT(begin <= getLength(tarray));
        JS_ASSERT(0 <= end);
        JS_ASSERT(end <= getLength(tarray));

        JSObject *bufobj = getBuffer(tarray);
        JS_ASSERT(bufobj);

        JS_ASSERT(begin <= end);
        uint32 length = end - begin;

        JS_ASSERT(begin < UINT32_MAX / sizeof(NativeType));
        JS_ASSERT(UINT32_MAX - begin * sizeof(NativeType) >= getByteOffset(tarray));
        uint32 byteOffset = getByteOffset(tarray) + begin * sizeof(NativeType);

        return createTypedArray(cx, bufobj, byteOffset, length);
    }

  protected:
    static NativeType
    nativeFromValue(JSContext *cx, const Value &v)
    {
        if (v.isInt32())
            return NativeType(v.toInt32());

        if (v.isDouble()) {
            double d = v.toDouble();
            if (!ArrayTypeIsFloatingPoint() && JS_UNLIKELY(JSDOUBLE_IS_NaN(d)))
                return NativeType(int32(0));
            if (TypeIsFloatingPoint<NativeType>())
                return NativeType(d);
            if (TypeIsUnsigned<NativeType>())
                return NativeType(js_DoubleToECMAUint32(d));
            return NativeType(js_DoubleToECMAInt32(d));
        }

        if (v.isPrimitive() && !v.isMagic()) {
            jsdouble dval;
            JS_ALWAYS_TRUE(ToNumber(cx, v, &dval));
            return NativeType(dval);
        }

        if (ArrayTypeIsFloatingPoint())
            return NativeType(js_NaN);

        return NativeType(int32(0));
    }

    static bool
    copyFromArray(JSContext *cx, JSObject *thisTypedArrayObj,
             JSObject *ar, jsuint len, jsuint offset = 0)
    {
        thisTypedArrayObj = getTypedArray(thisTypedArrayObj);
        JS_ASSERT(thisTypedArrayObj);

        JS_ASSERT(offset <= getLength(thisTypedArrayObj));
        JS_ASSERT(len <= getLength(thisTypedArrayObj) - offset);
        NativeType *dest = static_cast<NativeType*>(getDataOffset(thisTypedArrayObj)) + offset;

        if (ar->isDenseArray() && ar->getDenseArrayInitializedLength() >= len) {
            JS_ASSERT(ar->getArrayLength() == len);

            const Value *src = ar->getDenseArrayElements();

            for (uintN i = 0; i < len; ++i)
                *dest++ = nativeFromValue(cx, *src++);
        } else {
            
            Value v;

            for (uintN i = 0; i < len; ++i) {
                if (!ar->getElement(cx, i, &v))
                    return false;
                *dest++ = nativeFromValue(cx, v);
            }
        }

        return true;
    }

    static bool
    copyFromTypedArray(JSContext *cx, JSObject *thisTypedArrayObj, JSObject *tarray, jsuint offset)
    {
        thisTypedArrayObj = getTypedArray(thisTypedArrayObj);
        JS_ASSERT(thisTypedArrayObj);

        JS_ASSERT(offset <= getLength(thisTypedArrayObj));
        JS_ASSERT(getLength(tarray) <= getLength(thisTypedArrayObj) - offset);
        if (getBuffer(tarray) == getBuffer(thisTypedArrayObj))
            return copyFromWithOverlap(cx, thisTypedArrayObj, tarray, offset);

        NativeType *dest = static_cast<NativeType*>((void*)getDataOffset(thisTypedArrayObj)) + offset;

        if (getType(tarray) == getType(thisTypedArrayObj)) {
            memcpy(dest, getDataOffset(tarray), getByteLength(tarray));
            return true;
        }

        uintN srclen = getLength(tarray);
        switch (getType(tarray)) {
          case TypedArray::TYPE_INT8: {
            int8 *src = static_cast<int8*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT8:
          case TypedArray::TYPE_UINT8_CLAMPED: {
            uint8 *src = static_cast<uint8*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_INT16: {
            int16 *src = static_cast<int16*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT16: {
            uint16 *src = static_cast<uint16*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_INT32: {
            int32 *src = static_cast<int32*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT32: {
            uint32 *src = static_cast<uint32*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_FLOAT32: {
            float *src = static_cast<float*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_FLOAT64: {
            double *src = static_cast<double*>(getDataOffset(tarray));
            for (uintN i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          default:
            JS_NOT_REACHED("copyFrom with a TypedArray of unknown type");
            break;
        }

        return true;
    }

    static bool
    copyFromWithOverlap(JSContext *cx, JSObject *self, JSObject *tarray, jsuint offset)
    {
        JS_ASSERT(offset <= getLength(self));

        NativeType *dest = static_cast<NativeType*>(getDataOffset(self)) + offset;

        if (getType(tarray) == getType(self)) {
            memmove(dest, getDataOffset(tarray), getByteLength(tarray));
            return true;
        }

        
        
        void *srcbuf = cx->malloc_(getLength(tarray));
        if (!srcbuf)
            return false;
        memcpy(srcbuf, getDataOffset(tarray), getByteLength(tarray));

        switch (getType(tarray)) {
          case TypedArray::TYPE_INT8: {
            int8 *src = (int8*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT8:
          case TypedArray::TYPE_UINT8_CLAMPED: {
            uint8 *src = (uint8*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_INT16: {
            int16 *src = (int16*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT16: {
            uint16 *src = (uint16*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_INT32: {
            int32 *src = (int32*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_UINT32: {
            uint32 *src = (uint32*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_FLOAT32: {
            float *src = (float*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case TypedArray::TYPE_FLOAT64: {
            double *src = (double*) srcbuf;
            for (uintN i = 0; i < getLength(tarray); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          default:
            JS_NOT_REACHED("copyFromWithOverlap with a TypedArray of unknown type");
            break;
        }

        UnwantedForeground::free_(srcbuf);
        return true;
    }

    static void *
    offsetData(JSObject *obj, uint32 offs) {
        return (void*)(((uint8*)getDataOffset(obj)) + offs);
    }

    static JSObject *
    createBufferWithSizeAndCount(JSContext *cx, uint32 count)
    {
        size_t size = sizeof(NativeType);
        if (size != 0 && count >= INT32_MAX / size) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, NULL,
                                 JSMSG_NEED_DIET, "size and count");
            return NULL;
        }

        int32 bytelen = size * count;
        return ArrayBuffer::create(cx, bytelen);
    }
};

class Int8Array : public TypedArrayTemplate<int8> {
  public:
    enum { ACTUAL_TYPE = TYPE_INT8 };
    static const JSProtoKey key = JSProto_Int8Array;
    static JSFunctionSpec jsfuncs[];
};
class Uint8Array : public TypedArrayTemplate<uint8> {
  public:
    enum { ACTUAL_TYPE = TYPE_UINT8 };
    static const JSProtoKey key = JSProto_Uint8Array;
    static JSFunctionSpec jsfuncs[];
};
class Int16Array : public TypedArrayTemplate<int16> {
  public:
    enum { ACTUAL_TYPE = TYPE_INT16 };
    static const JSProtoKey key = JSProto_Int16Array;
    static JSFunctionSpec jsfuncs[];
};
class Uint16Array : public TypedArrayTemplate<uint16> {
  public:
    enum { ACTUAL_TYPE = TYPE_UINT16 };
    static const JSProtoKey key = JSProto_Uint16Array;
    static JSFunctionSpec jsfuncs[];
};
class Int32Array : public TypedArrayTemplate<int32> {
  public:
    enum { ACTUAL_TYPE = TYPE_INT32 };
    static const JSProtoKey key = JSProto_Int32Array;
    static JSFunctionSpec jsfuncs[];
};
class Uint32Array : public TypedArrayTemplate<uint32> {
  public:
    enum { ACTUAL_TYPE = TYPE_UINT32 };
    static const JSProtoKey key = JSProto_Uint32Array;
    static JSFunctionSpec jsfuncs[];
};
class Float32Array : public TypedArrayTemplate<float> {
  public:
    enum { ACTUAL_TYPE = TYPE_FLOAT32 };
    static const JSProtoKey key = JSProto_Float32Array;
    static JSFunctionSpec jsfuncs[];
};
class Float64Array : public TypedArrayTemplate<double> {
  public:
    enum { ACTUAL_TYPE = TYPE_FLOAT64 };
    static const JSProtoKey key = JSProto_Float64Array;
    static JSFunctionSpec jsfuncs[];
};
class Uint8ClampedArray : public TypedArrayTemplate<uint8_clamped> {
  public:
    enum { ACTUAL_TYPE = TYPE_UINT8_CLAMPED };
    static const JSProtoKey key = JSProto_Uint8ClampedArray;
    static JSFunctionSpec jsfuncs[];
};



template<typename NativeType>
void
TypedArrayTemplate<NativeType>::copyIndexToValue(JSContext *cx, JSObject *tarray, uint32 index, Value *vp)
{
    JS_STATIC_ASSERT(sizeof(NativeType) < 4);

    vp->setInt32(getIndex(tarray, index));
}


template<>
void
TypedArrayTemplate<int32>::copyIndexToValue(JSContext *cx, JSObject *tarray, uint32 index, Value *vp)
{
    int32 val = getIndex(tarray, index);
    vp->setInt32(val);
}

template<>
void
TypedArrayTemplate<uint32>::copyIndexToValue(JSContext *cx, JSObject *tarray, uint32 index, Value *vp)
{
    uint32 val = getIndex(tarray, index);
    vp->setNumber(val);
}

template<>
void
TypedArrayTemplate<float>::copyIndexToValue(JSContext *cx, JSObject *tarray, uint32 index, Value *vp)
{
    float val = getIndex(tarray, index);
    double dval = val;

    









    if (JS_UNLIKELY(JSDOUBLE_IS_NaN(dval)))
        dval = js_NaN;

    vp->setDouble(dval);
}

template<>
void
TypedArrayTemplate<double>::copyIndexToValue(JSContext *cx, JSObject *tarray, uint32 index, Value *vp)
{
    double val = getIndex(tarray, index);

    






    if (JS_UNLIKELY(JSDOUBLE_IS_NaN(val)))
        val = js_NaN;

    vp->setDouble(val);
}









Class ArrayBuffer::slowClass = {
    "ArrayBuffer",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(ARRAYBUFFER_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    JS_FinalizeStub
};

Class js::ArrayBufferClass = {
    "ArrayBuffer",
    JSCLASS_HAS_PRIVATE |
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(ARRAYBUFFER_RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_ArrayBuffer),
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    NULL,           
    ArrayBuffer::obj_trace,
    JS_NULL_CLASS_EXT,
    {
        ArrayBuffer::obj_lookupProperty,
        ArrayBuffer::obj_lookupProperty,
        ArrayBuffer::obj_lookupElement,
        ArrayBuffer::obj_lookupSpecial,
        ArrayBuffer::obj_defineProperty,
        ArrayBuffer::obj_defineProperty,
        ArrayBuffer::obj_defineElement,
        ArrayBuffer::obj_defineSpecial,
        ArrayBuffer::obj_getProperty,
        ArrayBuffer::obj_getProperty,
        ArrayBuffer::obj_getElement,
        ArrayBuffer::obj_getSpecial,
        ArrayBuffer::obj_setProperty,
        ArrayBuffer::obj_setProperty,
        ArrayBuffer::obj_setElement,
        ArrayBuffer::obj_setSpecial,
        ArrayBuffer::obj_getAttributes,
        ArrayBuffer::obj_getAttributes,
        ArrayBuffer::obj_getElementAttributes,
        ArrayBuffer::obj_getSpecialAttributes,
        ArrayBuffer::obj_setAttributes,
        ArrayBuffer::obj_setAttributes,
        ArrayBuffer::obj_setElementAttributes,
        ArrayBuffer::obj_setSpecialAttributes,
        ArrayBuffer::obj_deleteProperty,
        ArrayBuffer::obj_deleteProperty,
        ArrayBuffer::obj_deleteElement,
        ArrayBuffer::obj_deleteSpecial,
        ArrayBuffer::obj_enumerate,
        ArrayBuffer::obj_typeOf,
        NULL,       
        NULL,       
    }
};

JSPropertySpec ArrayBuffer::jsprops[] = {
    { "byteLength",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      ArrayBuffer::prop_getByteLength, JS_StrictPropertyStub },
    {0,0,0,0,0}
};





JSPropertySpec TypedArray::jsprops[] = {
    { js_length_str,
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getLength, JS_StrictPropertyStub },
    { "byteLength",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getByteLength, JS_StrictPropertyStub },
    { "byteOffset",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getByteOffset, JS_StrictPropertyStub },
    { "buffer",
      -1, JSPROP_SHARED | JSPROP_PERMANENT | JSPROP_READONLY,
      TypedArray::prop_getBuffer, JS_StrictPropertyStub },
    {0,0,0,0,0}
};





#define IMPL_TYPED_ARRAY_STATICS(_typedArray)                                  \
JSFunctionSpec _typedArray::jsfuncs[] = {                                      \
    JS_FN("subarray", _typedArray::fun_subarray, 2, JSFUN_GENERIC_NATIVE),     \
    JS_FN("set", _typedArray::fun_set, 2, JSFUN_GENERIC_NATIVE),               \
    JS_FS_END                                                                  \
}

#define IMPL_TYPED_ARRAY_SLOW_CLASS(_typedArray)                               \
{                                                                              \
    #_typedArray,                                                              \
    JSCLASS_HAS_RESERVED_SLOTS(TypedArray::FIELD_MAX) |                        \
    JSCLASS_HAS_PRIVATE |                                                      \
    JSCLASS_HAS_CACHED_PROTO(JSProto_##_typedArray),                           \
    JS_PropertyStub,         /* addProperty */                                 \
    JS_PropertyStub,         /* delProperty */                                 \
    JS_PropertyStub,         /* getProperty */                                 \
    JS_StrictPropertyStub,   /* setProperty */                                 \
    JS_EnumerateStub,                                                          \
    JS_ResolveStub,                                                            \
    JS_ConvertStub,                                                            \
    JS_FinalizeStub                                                            \
}

#define IMPL_TYPED_ARRAY_FAST_CLASS(_typedArray)                               \
{                                                                              \
    #_typedArray,                                                              \
    JSCLASS_HAS_RESERVED_SLOTS(TypedArray::FIELD_MAX) |                        \
    JSCLASS_HAS_PRIVATE |                                                      \
    Class::NON_NATIVE,                                                         \
    JS_PropertyStub,         /* addProperty */                                 \
    JS_PropertyStub,         /* delProperty */                                 \
    JS_PropertyStub,         /* getProperty */                                 \
    JS_StrictPropertyStub,   /* setProperty */                                 \
    JS_EnumerateStub,                                                          \
    JS_ResolveStub,                                                            \
    JS_ConvertStub,                                                            \
    NULL,                    /* finalize    */                                 \
    NULL,                    /* reserved0   */                                 \
    NULL,                    /* checkAccess */                                 \
    NULL,                    /* call        */                                 \
    NULL,                    /* construct   */                                 \
    NULL,                    /* xdrObject   */                                 \
    NULL,                    /* hasInstance */                                 \
    _typedArray::obj_trace,  /* trace       */                                 \
    JS_NULL_CLASS_EXT,                                                         \
    {                                                                          \
        _typedArray::obj_lookupProperty,                                       \
        _typedArray::obj_lookupProperty,                                       \
        _typedArray::obj_lookupElement,                                        \
        _typedArray::obj_lookupSpecial,                                        \
        _typedArray::obj_defineProperty,                                       \
        _typedArray::obj_defineProperty,                                       \
        _typedArray::obj_defineElement,                                        \
        _typedArray::obj_defineSpecial,                                        \
        _typedArray::obj_getProperty,                                          \
        _typedArray::obj_getProperty,                                          \
        _typedArray::obj_getElement,                                           \
        _typedArray::obj_getSpecial,                                           \
        _typedArray::obj_setProperty,                                          \
        _typedArray::obj_setProperty,                                          \
        _typedArray::obj_setElement,                                           \
        _typedArray::obj_setSpecial,                                           \
        _typedArray::obj_getAttributes,                                        \
        _typedArray::obj_getAttributes,                                        \
        _typedArray::obj_getElementAttributes,                                 \
        _typedArray::obj_getSpecialAttributes,                                 \
        _typedArray::obj_setAttributes,                                        \
        _typedArray::obj_setAttributes,                                        \
        _typedArray::obj_setElementAttributes,                                 \
        _typedArray::obj_setSpecialAttributes,                                 \
        _typedArray::obj_deleteProperty,                                       \
        _typedArray::obj_deleteProperty,                                       \
        _typedArray::obj_deleteElement,                                        \
        _typedArray::obj_deleteSpecial,                                        \
        _typedArray::obj_enumerate,                                            \
        _typedArray::obj_typeOf,                                               \
        NULL,                /* thisObject  */                                 \
        NULL,                /* clear       */                                 \
    }                                                                          \
}

template<class ArrayType>
static inline JSObject *
InitTypedArrayClass(JSContext *cx, GlobalObject *global)
{
    JSObject *proto = global->createBlankPrototype(cx, ArrayType::slowClass());
    if (!proto)
        return NULL;

    JSFunction *ctor =
        global->createConstructor(cx, ArrayType::class_constructor, ArrayType::fastClass(),
                                  cx->runtime->atomState.classAtoms[ArrayType::key], 3);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return NULL;

    if (!ctor->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.BYTES_PER_ELEMENTAtom),
                              Int32Value(ArrayType::BYTES_PER_ELEMENT),
                              JS_PropertyStub, JS_StrictPropertyStub,
                              JSPROP_PERMANENT | JSPROP_READONLY) ||
        !proto->defineProperty(cx, ATOM_TO_JSID(cx->runtime->atomState.BYTES_PER_ELEMENTAtom),
                               Int32Value(ArrayType::BYTES_PER_ELEMENT),
                               JS_PropertyStub, JS_StrictPropertyStub,
                               JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return NULL;
    }

    if (!DefinePropertiesAndBrand(cx, proto, ArrayType::jsprops, ArrayType::jsfuncs))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, ArrayType::key, ctor, proto))
        return NULL;

    return proto;
}

IMPL_TYPED_ARRAY_STATICS(Int8Array);
IMPL_TYPED_ARRAY_STATICS(Uint8Array);
IMPL_TYPED_ARRAY_STATICS(Int16Array);
IMPL_TYPED_ARRAY_STATICS(Uint16Array);
IMPL_TYPED_ARRAY_STATICS(Int32Array);
IMPL_TYPED_ARRAY_STATICS(Uint32Array);
IMPL_TYPED_ARRAY_STATICS(Float32Array);
IMPL_TYPED_ARRAY_STATICS(Float64Array);
IMPL_TYPED_ARRAY_STATICS(Uint8ClampedArray);

Class TypedArray::fastClasses[TYPE_MAX] = {
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

Class TypedArray::slowClasses[TYPE_MAX] = {
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

static JSObject *
InitArrayBufferClass(JSContext *cx, GlobalObject *global)
{
    JSObject *arrayBufferProto = global->createBlankPrototype(cx, &ArrayBuffer::slowClass);
    if (!arrayBufferProto)
        return NULL;

    JSFunction *ctor =
        global->createConstructor(cx, ArrayBuffer::class_constructor, &ArrayBufferClass,
                                  CLASS_ATOM(cx, ArrayBuffer), 1);
    if (!ctor)
        return NULL;

    if (!LinkConstructorAndPrototype(cx, ctor, arrayBufferProto))
        return NULL;

    if (!DefinePropertiesAndBrand(cx, arrayBufferProto, ArrayBuffer::jsprops, NULL))
        return NULL;

    if (!DefineConstructorAndPrototype(cx, global, JSProto_ArrayBuffer, ctor, arrayBufferProto))
        return NULL;

    return arrayBufferProto;
}

JS_FRIEND_API(JSObject *)
js_InitTypedArrayClasses(JSContext *cx, JSObject *obj)
{
    JS_ASSERT(obj->isNative());

    GlobalObject *global = obj->asGlobal();

    
    JSObject *stop;
    if (!js_GetClassObject(cx, global, JSProto_ArrayBuffer, &stop))
        return NULL;
    if (stop)
        return stop;

    if (!InitTypedArrayClass<Int8Array>(cx, global) ||
        !InitTypedArrayClass<Uint8Array>(cx, global) ||
        !InitTypedArrayClass<Int16Array>(cx, global) ||
        !InitTypedArrayClass<Uint16Array>(cx, global) ||
        !InitTypedArrayClass<Int32Array>(cx, global) ||
        !InitTypedArrayClass<Uint32Array>(cx, global) ||
        !InitTypedArrayClass<Float32Array>(cx, global) ||
        !InitTypedArrayClass<Float64Array>(cx, global) ||
        !InitTypedArrayClass<Uint8ClampedArray>(cx, global))
    {
        return NULL;
    }

    return InitArrayBufferClass(cx, global);
}

JS_FRIEND_API(JSBool)
js_IsArrayBuffer(JSObject *obj)
{
    JS_ASSERT(obj);
    return obj->isArrayBuffer();
}

namespace js {

bool
IsFastTypedArrayClass(const Class *clasp)
{
    return &TypedArray::fastClasses[0] <= clasp &&
           clasp < &TypedArray::fastClasses[TypedArray::TYPE_MAX];
}

} 

JSUint32
JS_GetArrayBufferByteLength(JSObject *obj)
{
    return obj->arrayBufferByteLength();
}

uint8 *
JS_GetArrayBufferData(JSObject *obj)
{
    return obj->arrayBufferDataOffset();
}

JS_FRIEND_API(JSBool)
js_IsTypedArray(JSObject *obj)
{
    JS_ASSERT(obj);
    Class *clasp = obj->getClass();
    return IsFastTypedArrayClass(clasp);
}

JS_FRIEND_API(JSObject *)
js_CreateArrayBuffer(JSContext *cx, jsuint nbytes)
{
    return ArrayBuffer::create(cx, nbytes);
}

static inline JSObject *
TypedArrayConstruct(JSContext *cx, jsint atype, uintN argc, Value *argv)
{
    switch (atype) {
      case TypedArray::TYPE_INT8:
        return Int8Array::create(cx, argc, argv);

      case TypedArray::TYPE_UINT8:
        return Uint8Array::create(cx, argc, argv);

      case TypedArray::TYPE_INT16:
        return Int16Array::create(cx, argc, argv);

      case TypedArray::TYPE_UINT16:
        return Uint16Array::create(cx, argc, argv);

      case TypedArray::TYPE_INT32:
        return Int32Array::create(cx, argc, argv);

      case TypedArray::TYPE_UINT32:
        return Uint32Array::create(cx, argc, argv);

      case TypedArray::TYPE_FLOAT32:
        return Float32Array::create(cx, argc, argv);

      case TypedArray::TYPE_FLOAT64:
        return Float64Array::create(cx, argc, argv);

      case TypedArray::TYPE_UINT8_CLAMPED:
        return Uint8ClampedArray::create(cx, argc, argv);

      default:
        JS_NOT_REACHED("shouldn't have gotten here");
        return NULL;
    }
}

JS_FRIEND_API(JSObject *)
js_CreateTypedArray(JSContext *cx, jsint atype, jsuint nelements)
{
    JS_ASSERT(atype >= 0 && atype < TypedArray::TYPE_MAX);

    Value nelems = Int32Value(nelements);
    return TypedArrayConstruct(cx, atype, 1, &nelems);
}

JS_FRIEND_API(JSObject *)
js_CreateTypedArrayWithArray(JSContext *cx, jsint atype, JSObject *arrayArg)
{
    JS_ASSERT(atype >= 0 && atype < TypedArray::TYPE_MAX);

    Value arrval = ObjectValue(*arrayArg);
    return TypedArrayConstruct(cx, atype, 1, &arrval);
}

JS_FRIEND_API(JSObject *)
js_CreateTypedArrayWithBuffer(JSContext *cx, jsint atype, JSObject *bufArg,
                              jsint byteoffset, jsint length)
{
    JS_ASSERT(atype >= 0 && atype < TypedArray::TYPE_MAX);
    JS_ASSERT(bufArg && js_IsArrayBuffer(bufArg));
    JS_ASSERT_IF(byteoffset < 0, length < 0);

    Value vals[4];

    int argc = 1;
    vals[0].setObject(*bufArg);

    if (byteoffset >= 0) {
        vals[argc].setInt32(byteoffset);
        argc++;
    }

    if (length >= 0) {
        vals[argc].setInt32(length);
        argc++;
    }

    AutoArrayRooter tvr(cx, JS_ARRAY_LENGTH(vals), vals);
    return TypedArrayConstruct(cx, atype, argc, &vals[0]);
}

JSUint32
JS_GetTypedArrayLength(JSObject *obj)
{
    return obj->getSlot(TypedArray::FIELD_LENGTH).toInt32();
}

JSUint32
JS_GetTypedArrayByteOffset(JSObject *obj)
{
    return obj->getSlot(TypedArray::FIELD_BYTEOFFSET).toInt32();
}

JSUint32
JS_GetTypedArrayByteLength(JSObject *obj)
{
    return obj->getSlot(TypedArray::FIELD_BYTELENGTH).toInt32();
}

JSUint32
JS_GetTypedArrayType(JSObject *obj)
{
    return obj->getSlot(TypedArray::FIELD_TYPE).toInt32();
}

JSObject *
JS_GetTypedArrayBuffer(JSObject *obj)
{
    return (JSObject *) obj->getSlot(TypedArray::FIELD_BUFFER).toPrivate();
}

void *
JS_GetTypedArrayData(JSObject *obj)
{
    return TypedArray::getDataOffset(obj);
}
