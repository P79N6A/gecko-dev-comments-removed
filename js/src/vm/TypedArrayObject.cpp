





#include "vm/TypedArrayObject.h"

#include "mozilla/Alignment.h"
#include "mozilla/FloatingPoint.h"
#include "mozilla/PodOperations.h"

#include <string.h>
#ifndef XP_WIN
# include <sys/mman.h>
#endif

#include "jsapi.h"
#include "jsarray.h"
#include "jscntxt.h"
#include "jscpucfg.h"
#include "jsnum.h"
#include "jsobj.h"
#include "jstypes.h"
#include "jsutil.h"
#ifdef XP_WIN
# include "jswin.h"
#endif
#include "jswrapper.h"

#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "jit/AsmJS.h"
#include "jit/AsmJSModule.h"
#include "vm/ArrayBufferObject.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/SharedArrayObject.h"
#include "vm/WrapperObject.h"

#include "jsatominlines.h"
#include "jsinferinlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using namespace js;
using namespace js::gc;
using namespace js::types;

using mozilla::IsNaN;
using mozilla::NegativeInfinity;
using mozilla::PodCopy;
using mozilla::PositiveInfinity;
using JS::CanonicalizeNaN;
using JS::GenericNaN;

static bool
ValueIsLength(const Value &v, uint32_t *len)
{
    if (v.isInt32()) {
        int32_t i = v.toInt32();
        if (i < 0)
            return false;
        *len = i;
        return true;
    }

    if (v.isDouble()) {
        double d = v.toDouble();
        if (IsNaN(d))
            return false;

        uint32_t length = uint32_t(d);
        if (d != double(length))
            return false;

        *len = length;
        return true;
    }

    return false;
}









inline bool
TypedArrayObject::isArrayIndex(jsid id, uint32_t *ip)
{
    uint32_t index;
    if (js_IdIsIndex(id, &index) && index < length()) {
        if (ip)
            *ip = index;
        return true;
    }

    return false;
}

void
TypedArrayObject::neuter(void *newData)
{
    setSlot(LENGTH_SLOT, Int32Value(0));
    setSlot(BYTELENGTH_SLOT, Int32Value(0));
    setSlot(BYTEOFFSET_SLOT, Int32Value(0));
    setPrivate(newData);
}

ArrayBufferObject *
TypedArrayObject::sharedBuffer() const
{
    return &bufferValue(const_cast<TypedArrayObject*>(this)).toObject().as<SharedArrayBufferObject>();
}

 int
TypedArrayObject::lengthOffset()
{
    return JSObject::getFixedSlotOffset(LENGTH_SLOT);
}

 int
TypedArrayObject::dataOffset()
{
    return JSObject::getPrivateDataOffset(DATA_SLOT);
}



uint32_t JS_FASTCALL
js::ClampDoubleToUint8(const double x)
{
    
    if (!(x >= 0))
        return 0;

    if (x > 255)
        return 255;

    double toTruncate = x + 0.5;
    uint8_t y = uint8_t(toTruncate);

    




    if (y == toTruncate) {
        







        return (y & ~1);
    }

    return y;
}

static bool
ToDoubleForTypedArray(ThreadSafeContext *cx, const Value &vp, double *d)
{
    if (vp.isDouble()) {
        *d = vp.toDouble();
    } else if (vp.isNull()) {
        *d = 0.0;
    } else if (vp.isPrimitive()) {
        JS_ASSERT(vp.isString() || vp.isUndefined() || vp.isBoolean());
        if (vp.isString()) {
            if (!StringToNumber(cx, vp.toString(), d))
                return false;
        } else if (vp.isUndefined()) {
            *d = GenericNaN();
        } else {
            *d = double(vp.toBoolean());
        }
    } else {
        
        *d = GenericNaN();
    }

#ifdef JS_MORE_DETERMINISTIC
    
    
    
    
    *d = CanonicalizeNaN(*d);
#endif

    return true;
}

template<typename NativeType> static inline const int TypeIDOfType();
template<> inline const int TypeIDOfType<int8_t>() { return ScalarTypeDescr::TYPE_INT8; }
template<> inline const int TypeIDOfType<uint8_t>() { return ScalarTypeDescr::TYPE_UINT8; }
template<> inline const int TypeIDOfType<int16_t>() { return ScalarTypeDescr::TYPE_INT16; }
template<> inline const int TypeIDOfType<uint16_t>() { return ScalarTypeDescr::TYPE_UINT16; }
template<> inline const int TypeIDOfType<int32_t>() { return ScalarTypeDescr::TYPE_INT32; }
template<> inline const int TypeIDOfType<uint32_t>() { return ScalarTypeDescr::TYPE_UINT32; }
template<> inline const int TypeIDOfType<float>() { return ScalarTypeDescr::TYPE_FLOAT32; }
template<> inline const int TypeIDOfType<double>() { return ScalarTypeDescr::TYPE_FLOAT64; }
template<> inline const int TypeIDOfType<uint8_clamped>() { return ScalarTypeDescr::TYPE_UINT8_CLAMPED; }

template<typename ElementType>
static inline JSObject *
NewArray(JSContext *cx, uint32_t nelements);

namespace {

template<typename NativeType>
class TypedArrayObjectTemplate : public TypedArrayObject
{
  public:
    typedef NativeType ThisType;
    typedef TypedArrayObjectTemplate<NativeType> ThisTypedArrayObject;
    static const int ArrayTypeID() { return TypeIDOfType<NativeType>(); }
    static const bool ArrayTypeIsUnsigned() { return TypeIsUnsigned<NativeType>(); }
    static const bool ArrayTypeIsFloatingPoint() { return TypeIsFloatingPoint<NativeType>(); }

    static const size_t BYTES_PER_ELEMENT = sizeof(ThisType);

    static inline const Class *protoClass()
    {
        return &TypedArrayObject::protoClasses[ArrayTypeID()];
    }

    static inline const Class *fastClass()
    {
        return &TypedArrayObject::classes[ArrayTypeID()];
    }

    static bool is(HandleValue v) {
        return v.isObject() && v.toObject().hasClass(fastClass());
    }

    static bool
    setIndexValue(ThreadSafeContext *cx, JSObject *tarray, uint32_t index, const Value &value)
    {
        JS_ASSERT(tarray);
        JS_ASSERT(index < tarray->as<TypedArrayObject>().length());

        if (value.isInt32()) {
            setIndex(tarray, index, NativeType(value.toInt32()));
            return true;
        }

        double d;
        if (!ToDoubleForTypedArray(cx, value, &d))
            return false;

        
        
        

        
        if (ArrayTypeIsFloatingPoint()) {
            setIndex(tarray, index, NativeType(d));
        } else if (ArrayTypeIsUnsigned()) {
            JS_ASSERT(sizeof(NativeType) <= 4);
            uint32_t n = ToUint32(d);
            setIndex(tarray, index, NativeType(n));
        } else if (ArrayTypeID() == ScalarTypeDescr::TYPE_UINT8_CLAMPED) {
            
            
            setIndex(tarray, index, NativeType(d));
        } else {
            JS_ASSERT(sizeof(NativeType) <= 4);
            int32_t n = ToInt32(d);
            setIndex(tarray, index, NativeType(n));
        }

        return true;
    }

    static TypedArrayObject *
    makeProtoInstance(JSContext *cx, HandleObject proto)
    {
        JS_ASSERT(proto);

        RootedObject obj(cx, NewBuiltinClassInstance(cx, fastClass()));
        if (!obj)
            return nullptr;

        types::TypeObject *type = cx->getNewType(obj->getClass(), proto.get());
        if (!type)
            return nullptr;
        obj->setType(type);

        return &obj->as<TypedArrayObject>();
    }

    static TypedArrayObject *
    makeTypedInstance(JSContext *cx, uint32_t len)
    {
        if (len * sizeof(NativeType) >= TypedArrayObject::SINGLETON_TYPE_BYTE_LENGTH) {
            return &NewBuiltinClassInstance(cx, fastClass(),
                                            SingletonObject)->as<TypedArrayObject>();
        }

        jsbytecode *pc;
        RootedScript script(cx, cx->currentScript(&pc));
        NewObjectKind newKind = script
                                ? UseNewTypeForInitializer(script, pc, fastClass())
                                : GenericObject;
        RootedObject obj(cx, NewBuiltinClassInstance(cx, fastClass(), newKind));
        if (!obj)
            return nullptr;

        if (script) {
            if (!types::SetInitializerObjectType(cx, script, pc, obj, newKind))
                return nullptr;
        }

        return &obj->as<TypedArrayObject>();
    }

    static JSObject *
    makeInstance(JSContext *cx, HandleObject bufobj, uint32_t byteOffset, uint32_t len,
                 HandleObject proto)
    {
        Rooted<TypedArrayObject*> obj(cx);
        if (proto)
            obj = makeProtoInstance(cx, proto);
        else
            obj = makeTypedInstance(cx, len);
        if (!obj)
            return nullptr;
        JS_ASSERT_IF(obj->isTenured(),
                     obj->tenuredGetAllocKind() == gc::FINALIZE_OBJECT8_BACKGROUND);

        obj->setSlot(TYPE_SLOT, Int32Value(ArrayTypeID()));
        obj->setSlot(BUFFER_SLOT, ObjectValue(*bufobj));

        Rooted<ArrayBufferObject *> buffer(cx, &AsArrayBuffer(bufobj));

        InitArrayBufferViewDataPointer(obj, buffer, byteOffset);
        obj->setSlot(LENGTH_SLOT, Int32Value(len));
        obj->setSlot(BYTEOFFSET_SLOT, Int32Value(byteOffset));
        obj->setSlot(BYTELENGTH_SLOT, Int32Value(len * sizeof(NativeType)));
        obj->setSlot(NEXT_VIEW_SLOT, PrivateValue(nullptr));

        js::Shape *empty = EmptyShape::getInitialShape(cx, fastClass(),
                                                       obj->getProto(), obj->getParent(), obj->getMetadata(),
                                                       gc::FINALIZE_OBJECT8_BACKGROUND);
        if (!empty)
            return nullptr;
        obj->setLastPropertyInfallible(empty);

#ifdef DEBUG
        uint32_t bufferByteLength = buffer->byteLength();
        uint32_t arrayByteLength = obj->byteLength();
        uint32_t arrayByteOffset = obj->byteOffset();
        JS_ASSERT_IF(!buffer->isNeutered(), buffer->dataPointer() <= obj->viewData());
        JS_ASSERT(bufferByteLength - arrayByteOffset >= arrayByteLength);
        JS_ASSERT(arrayByteOffset <= bufferByteLength);

        
        JS_ASSERT(obj->numFixedSlots() == DATA_SLOT);
#endif

        buffer->addView(obj);

        return obj;
    }

    static JSObject *
    makeInstance(JSContext *cx, HandleObject bufobj, uint32_t byteOffset, uint32_t len)
    {
        RootedObject nullproto(cx, nullptr);
        return makeInstance(cx, bufobj, byteOffset, len, nullproto);
    }

    





    static bool
    class_constructor(JSContext *cx, unsigned argc, Value *vp)
    {
        
        CallArgs args = CallArgsFromVp(argc, vp);
        JSObject *obj = create(cx, args);
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    static JSObject *
    create(JSContext *cx, const CallArgs& args)
    {
        
        uint32_t len = 0;
        if (args.length() == 0 || ValueIsLength(args[0], &len))
            return fromLength(cx, len);

        
        if (!args[0].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr;
        }

        RootedObject dataObj(cx, &args.get(0).toObject());

        







        if (!UncheckedUnwrap(dataObj)->is<ArrayBufferObject>() &&
            !UncheckedUnwrap(dataObj)->is<SharedArrayBufferObject>())
        {
            return fromArray(cx, dataObj);
        }

        
        int32_t byteOffset = 0;
        int32_t length = -1;

        if (args.length() > 1) {
            if (!ToInt32(cx, args[1], &byteOffset))
                return nullptr;
            if (byteOffset < 0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_TYPED_ARRAY_NEGATIVE_ARG, "1");
                return nullptr;
            }

            if (args.length() > 2) {
                if (!ToInt32(cx, args[2], &length))
                    return nullptr;
                if (length < 0) {
                    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                         JSMSG_TYPED_ARRAY_NEGATIVE_ARG, "2");
                    return nullptr;
                }
            }
        }

        Rooted<JSObject*> proto(cx, nullptr);
        return fromBuffer(cx, dataObj, byteOffset, length, proto);
    }

    static bool IsThisClass(HandleValue v) {
        return v.isObject() && v.toObject().hasClass(fastClass());
    }

    template<Value ValueGetter(TypedArrayObject *tarr)>
    static bool
    GetterImpl(JSContext *cx, CallArgs args)
    {
        JS_ASSERT(IsThisClass(args.thisv()));
        args.rval().set(ValueGetter(&args.thisv().toObject().as<TypedArrayObject>()));
        return true;
    }

    
    
    
    template<Value ValueGetter(TypedArrayObject *tarr)>
    static bool
    Getter(JSContext *cx, unsigned argc, Value *vp)
    {
        CallArgs args = CallArgsFromVp(argc, vp);
        return CallNonGenericMethod<ThisTypedArrayObject::IsThisClass,
                                    ThisTypedArrayObject::GetterImpl<ValueGetter> >(cx, args);
    }

    
    template<Value ValueGetter(TypedArrayObject *tarr)>
    static bool
    DefineGetter(JSContext *cx, PropertyName *name, HandleObject proto)
    {
        RootedId id(cx, NameToId(name));
        unsigned flags = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;

        Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
        JSObject *getter = NewFunction(cx, NullPtr(), Getter<ValueGetter>, 0,
                                       JSFunction::NATIVE_FUN, global, NullPtr());
        if (!getter)
            return false;

        return DefineNativeProperty(cx, proto, id, UndefinedHandleValue,
                                    JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr,
                                    flags, 0, 0);
    }

    static
    bool defineGetters(JSContext *cx, HandleObject proto)
    {
        if (!DefineGetter<lengthValue>(cx, cx->names().length, proto))
            return false;

        if (!DefineGetter<bufferValue>(cx, cx->names().buffer, proto))
            return false;

        if (!DefineGetter<byteLengthValue>(cx, cx->names().byteLength, proto))
            return false;

        if (!DefineGetter<byteOffsetValue>(cx, cx->names().byteOffset, proto))
            return false;

        return true;
    }

    
    static bool
    fun_subarray_impl(JSContext *cx, CallArgs args)
    {
        JS_ASSERT(IsThisClass(args.thisv()));
        Rooted<TypedArrayObject*> tarray(cx, &args.thisv().toObject().as<TypedArrayObject>());

        
        uint32_t length = tarray->length();
        uint32_t begin = 0, end = length;

        if (args.length() > 0) {
            if (!ToClampedIndex(cx, args[0], length, &begin))
                return false;

            if (args.length() > 1) {
                if (!ToClampedIndex(cx, args[1], length, &end))
                    return false;
            }
        }

        if (begin > end)
            begin = end;

        JSObject *nobj = createSubarray(cx, tarray, begin, end);
        if (!nobj)
            return false;
        args.rval().setObject(*nobj);
        return true;
    }

    static bool
    fun_subarray(JSContext *cx, unsigned argc, Value *vp)
    {
        CallArgs args = CallArgsFromVp(argc, vp);
        return CallNonGenericMethod<ThisTypedArrayObject::IsThisClass,
                                    ThisTypedArrayObject::fun_subarray_impl>(cx, args);
    }

    
    static bool
    fun_move_impl(JSContext *cx, CallArgs args)
    {
        JS_ASSERT(IsThisClass(args.thisv()));
        Rooted<TypedArrayObject*> tarray(cx, &args.thisv().toObject().as<TypedArrayObject>());

        if (args.length() < 3) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        uint32_t srcBegin;
        uint32_t srcEnd;
        uint32_t dest;

        uint32_t length = tarray->length();
        if (!ToClampedIndex(cx, args[0], length, &srcBegin) ||
            !ToClampedIndex(cx, args[1], length, &srcEnd) ||
            !ToClampedIndex(cx, args[2], length, &dest) ||
            srcBegin > srcEnd)
        {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        uint32_t nelts = srcEnd - srcBegin;

        JS_ASSERT(dest + nelts >= dest);
        if (dest + nelts > length) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        uint32_t byteDest = dest * sizeof(NativeType);
        uint32_t byteSrc = srcBegin * sizeof(NativeType);
        uint32_t byteSize = nelts * sizeof(NativeType);

#ifdef DEBUG
        uint32_t viewByteLength = tarray->byteLength();
        JS_ASSERT(byteDest <= viewByteLength);
        JS_ASSERT(byteSrc <= viewByteLength);
        JS_ASSERT(byteDest + byteSize <= viewByteLength);
        JS_ASSERT(byteSrc + byteSize <= viewByteLength);

        
        JS_ASSERT(byteDest + byteSize >= byteDest);
        JS_ASSERT(byteSrc + byteSize >= byteSrc);
#endif

        uint8_t *data = static_cast<uint8_t*>(tarray->viewData());
        memmove(&data[byteDest], &data[byteSrc], byteSize);
        args.rval().setUndefined();
        return true;
    }

    static bool
    fun_move(JSContext *cx, unsigned argc, Value *vp)
    {
        CallArgs args = CallArgsFromVp(argc, vp);
        return CallNonGenericMethod<ThisTypedArrayObject::IsThisClass,
                                    ThisTypedArrayObject::fun_move_impl>(cx, args);
    }

    
    static bool
    fun_set_impl(JSContext *cx, CallArgs args)
    {
        JS_ASSERT(IsThisClass(args.thisv()));
        Rooted<TypedArrayObject*> tarray(cx, &args.thisv().toObject().as<TypedArrayObject>());

        
        if (args.length() == 0 || !args[0].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        int32_t offset = 0;
        if (args.length() > 1) {
            if (!ToInt32(cx, args[1], &offset))
                return false;

            if (offset < 0 || uint32_t(offset) > tarray->length()) {
                
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_TYPED_ARRAY_BAD_INDEX, "2");
                return false;
            }
        }

        if (!args[0].isObject()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return false;
        }

        RootedObject arg0(cx, args[0].toObjectOrNull());
        if (arg0->is<TypedArrayObject>()) {
            if (arg0->as<TypedArrayObject>().length() > tarray->length() - offset) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
                return false;
            }

            if (!copyFromTypedArray(cx, tarray, arg0, offset))
                return false;
        } else {
            uint32_t len;
            if (!GetLengthProperty(cx, arg0, &len))
                return false;

            
            if (len > tarray->length() - offset) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_BAD_ARRAY_LENGTH);
                return false;
            }

            if (!copyFromArray(cx, tarray, arg0, len, offset))
                return false;
        }

        args.rval().setUndefined();
        return true;
    }

    static bool
    fun_set(JSContext *cx, unsigned argc, Value *vp)
    {
        CallArgs args = CallArgsFromVp(argc, vp);
        return CallNonGenericMethod<ThisTypedArrayObject::IsThisClass,
                                    ThisTypedArrayObject::fun_set_impl>(cx, args);
    }

  public:
    static JSObject *
    fromBuffer(JSContext *cx, HandleObject bufobj, uint32_t byteOffset, int32_t lengthInt,
               HandleObject proto)
    {
        if (!ObjectClassIs(bufobj, ESClass_ArrayBuffer, cx)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        JS_ASSERT(IsArrayBuffer(bufobj) || bufobj->is<ProxyObject>());
        if (bufobj->is<ProxyObject>()) {
            










            JSObject *wrapped = CheckedUnwrap(bufobj);
            if (!wrapped) {
                JS_ReportError(cx, "Permission denied to access object");
                return nullptr;
            }
            if (IsArrayBuffer(wrapped)) {
                













                Rooted<JSObject*> proto(cx);
                if (!GetBuiltinPrototype(cx, JSCLASS_CACHED_PROTO_KEY(fastClass()), &proto))
                    return nullptr;

                InvokeArgs args(cx);
                if (!args.init(3))
                    return nullptr;

                args.setCallee(cx->compartment()->maybeGlobal()->createArrayFromBuffer<NativeType>());
                args.setThis(ObjectValue(*bufobj));
                args[0].setNumber(byteOffset);
                args[1].setInt32(lengthInt);
                args[2].setObject(*proto);

                if (!Invoke(cx, args))
                    return nullptr;
                return &args.rval().toObject();
            }
        }

        if (!IsArrayBuffer(bufobj)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        ArrayBufferObject &buffer = AsArrayBuffer(bufobj);

        if (byteOffset > buffer.byteLength() || byteOffset % sizeof(NativeType) != 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        uint32_t len;
        if (lengthInt == -1) {
            len = (buffer.byteLength() - byteOffset) / sizeof(NativeType);
            if (len * sizeof(NativeType) != buffer.byteLength() - byteOffset) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_TYPED_ARRAY_BAD_ARGS);
                return nullptr; 
            }
        } else {
            len = uint32_t(lengthInt);
        }

        
        uint32_t arrayByteLength = len * sizeof(NativeType);
        if (len >= INT32_MAX / sizeof(NativeType) || byteOffset >= INT32_MAX - arrayByteLength) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        if (arrayByteLength + byteOffset > buffer.byteLength()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        return makeInstance(cx, bufobj, byteOffset, len, proto);
    }

    static JSObject *
    fromLength(JSContext *cx, uint32_t nelements)
    {
        RootedObject buffer(cx, createBufferWithSizeAndCount(cx, nelements));
        if (!buffer)
            return nullptr;
        return makeInstance(cx, buffer, 0, nelements);
    }

    static JSObject *
    fromArray(JSContext *cx, HandleObject other)
    {
        uint32_t len;
        if (other->is<TypedArrayObject>()) {
            len = other->as<TypedArrayObject>().length();
        } else if (!GetLengthProperty(cx, other, &len)) {
            return nullptr;
        }

        RootedObject bufobj(cx, createBufferWithSizeAndCount(cx, len));
        if (!bufobj)
            return nullptr;

        RootedObject obj(cx, makeInstance(cx, bufobj, 0, len));
        if (!obj || !copyFromArray(cx, obj, other, len))
            return nullptr;
        return obj;
    }

    static const NativeType
    getIndex(JSObject *obj, uint32_t index)
    {
        return *(static_cast<const NativeType*>(obj->as<TypedArrayObject>().viewData()) + index);
    }

    static void
    setIndex(JSObject *obj, uint32_t index, NativeType val)
    {
        *(static_cast<NativeType*>(obj->as<TypedArrayObject>().viewData()) + index) = val;
    }

    static Value getIndexValue(JSObject *tarray, uint32_t index);

    static JSObject *
    createSubarray(JSContext *cx, HandleObject tarrayArg, uint32_t begin, uint32_t end)
    {
        Rooted<TypedArrayObject*> tarray(cx, &tarrayArg->as<TypedArrayObject>());

        JS_ASSERT(begin <= tarray->length());
        JS_ASSERT(end <= tarray->length());

        RootedObject bufobj(cx, tarray->buffer());
        JS_ASSERT(bufobj);

        JS_ASSERT(begin <= end);
        uint32_t length = end - begin;

        JS_ASSERT(begin < UINT32_MAX / sizeof(NativeType));
        uint32_t arrayByteOffset = tarray->byteOffset();
        JS_ASSERT(UINT32_MAX - begin * sizeof(NativeType) >= arrayByteOffset);
        uint32_t byteOffset = arrayByteOffset + begin * sizeof(NativeType);

        return makeInstance(cx, bufobj, byteOffset, length);
    }

  protected:
    static NativeType
    nativeFromDouble(double d)
    {
        if (!ArrayTypeIsFloatingPoint() && MOZ_UNLIKELY(IsNaN(d)))
            return NativeType(int32_t(0));
        if (TypeIsFloatingPoint<NativeType>())
            return NativeType(d);
        if (TypeIsUnsigned<NativeType>())
            return NativeType(ToUint32(d));
        return NativeType(ToInt32(d));
    }

    static bool
    nativeFromValue(JSContext *cx, const Value &v, NativeType *result)
    {
        if (v.isInt32()) {
            *result = v.toInt32();
            return true;
        }

        if (v.isDouble()) {
            *result = nativeFromDouble(v.toDouble());
            return true;
        }

        



        if (v.isPrimitive() && !v.isMagic() && !v.isUndefined()) {
            RootedValue primitive(cx, v);
            double dval;
            
            if (!ToNumber(cx, primitive, &dval))
                return false;
            *result = nativeFromDouble(dval);
            return true;
        }

        *result = ArrayTypeIsFloatingPoint()
                  ? NativeType(GenericNaN())
                  : NativeType(int32_t(0));
        return true;
    }

    static bool
    copyFromArray(JSContext *cx, HandleObject thisTypedArrayObj,
                  HandleObject ar, uint32_t len, uint32_t offset = 0)
    {
        
        if (len == 0)
            return true;

        Rooted<TypedArrayObject*> thisTypedArray(cx, &thisTypedArrayObj->as<TypedArrayObject>());
        JS_ASSERT(offset <= thisTypedArray->length());
        JS_ASSERT(len <= thisTypedArray->length() - offset);
        if (ar->is<TypedArrayObject>())
            return copyFromTypedArray(cx, thisTypedArray, ar, offset);

#ifdef DEBUG
        JSRuntime *runtime = cx->runtime();
        uint64_t gcNumber = runtime->gcNumber;
#endif

        NativeType *dest = static_cast<NativeType*>(thisTypedArray->viewData()) + offset;
        SkipRoot skipDest(cx, &dest);

        if (ar->is<ArrayObject>() && !ar->isIndexed() && ar->getDenseInitializedLength() >= len) {
            JS_ASSERT(ar->as<ArrayObject>().length() == len);

            




            const Value *src = ar->getDenseElements();
            SkipRoot skipSrc(cx, &src);
            uint32_t i = 0;
            do {
                NativeType n;
                if (!nativeFromValue(cx, src[i], &n))
                    return false;
                dest[i] = n;
            } while (++i < len);
            JS_ASSERT(runtime->gcNumber == gcNumber);
        } else {
            RootedValue v(cx);

            uint32_t i = 0;
            do {
                if (!JSObject::getElement(cx, ar, ar, i, &v))
                    return false;
                NativeType n;
                if (!nativeFromValue(cx, v, &n))
                    return false;

                len = Min(len, thisTypedArray->length());
                if (i >= len)
                    break;

                
                dest = static_cast<NativeType*>(thisTypedArray->viewData()) + offset;
                dest[i] = n;
            } while (++i < len);
        }

        return true;
    }

    static bool
    copyFromTypedArray(JSContext *cx, JSObject *thisTypedArrayObj, JSObject *tarrayObj,
                       uint32_t offset)
    {
        TypedArrayObject *thisTypedArray = &thisTypedArrayObj->as<TypedArrayObject>();
        TypedArrayObject *tarray = &tarrayObj->as<TypedArrayObject>();
        JS_ASSERT(offset <= thisTypedArray->length());
        JS_ASSERT(tarray->length() <= thisTypedArray->length() - offset);
        if (tarray->buffer() == thisTypedArray->buffer())
            return copyFromWithOverlap(cx, thisTypedArray, tarray, offset);

        NativeType *dest = static_cast<NativeType*>(thisTypedArray->viewData()) + offset;

        if (tarray->type() == thisTypedArray->type()) {
            js_memcpy(dest, tarray->viewData(), tarray->byteLength());
            return true;
        }

        unsigned srclen = tarray->length();
        switch (tarray->type()) {
          case ScalarTypeDescr::TYPE_INT8: {
            int8_t *src = static_cast<int8_t*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_UINT8:
          case ScalarTypeDescr::TYPE_UINT8_CLAMPED: {
            uint8_t *src = static_cast<uint8_t*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_INT16: {
            int16_t *src = static_cast<int16_t*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_UINT16: {
            uint16_t *src = static_cast<uint16_t*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_INT32: {
            int32_t *src = static_cast<int32_t*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_UINT32: {
            uint32_t *src = static_cast<uint32_t*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_FLOAT32: {
            float *src = static_cast<float*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_FLOAT64: {
            double *src = static_cast<double*>(tarray->viewData());
            for (unsigned i = 0; i < srclen; ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          default:
            MOZ_ASSUME_UNREACHABLE("copyFrom with a TypedArrayObject of unknown type");
        }

        return true;
    }

    static bool
    copyFromWithOverlap(JSContext *cx, JSObject *selfObj, JSObject *tarrayObj, uint32_t offset)
    {
        TypedArrayObject *self = &selfObj->as<TypedArrayObject>();
        TypedArrayObject *tarray = &tarrayObj->as<TypedArrayObject>();

        JS_ASSERT(offset <= self->length());

        NativeType *dest = static_cast<NativeType*>(self->viewData()) + offset;
        uint32_t byteLength = tarray->byteLength();

        if (tarray->type() == self->type()) {
            memmove(dest, tarray->viewData(), byteLength);
            return true;
        }

        
        
        void *srcbuf = cx->malloc_(byteLength);
        if (!srcbuf)
            return false;
        js_memcpy(srcbuf, tarray->viewData(), byteLength);

        switch (tarray->type()) {
          case ScalarTypeDescr::TYPE_INT8: {
            int8_t *src = (int8_t*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_UINT8:
          case ScalarTypeDescr::TYPE_UINT8_CLAMPED: {
            uint8_t *src = (uint8_t*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_INT16: {
            int16_t *src = (int16_t*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_UINT16: {
            uint16_t *src = (uint16_t*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_INT32: {
            int32_t *src = (int32_t*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_UINT32: {
            uint32_t *src = (uint32_t*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_FLOAT32: {
            float *src = (float*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          case ScalarTypeDescr::TYPE_FLOAT64: {
            double *src = (double*) srcbuf;
            for (unsigned i = 0; i < tarray->length(); ++i)
                *dest++ = NativeType(*src++);
            break;
          }
          default:
            MOZ_ASSUME_UNREACHABLE("copyFromWithOverlap with a TypedArrayObject of unknown type");
        }

        js_free(srcbuf);
        return true;
    }

    static JSObject *
    createBufferWithSizeAndCount(JSContext *cx, uint32_t count)
    {
        size_t size = sizeof(NativeType);
        if (size != 0 && count >= INT32_MAX / size) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_NEED_DIET, "size and count");
            return nullptr;
        }

        uint32_t bytelen = size * count;
        return ArrayBufferObject::create(cx, bytelen);
    }
};

class Int8ArrayObject : public TypedArrayObjectTemplate<int8_t> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_INT8 };
    static const JSProtoKey key = JSProto_Int8Array;
    static const JSFunctionSpec jsfuncs[];
};
class Uint8ArrayObject : public TypedArrayObjectTemplate<uint8_t> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_UINT8 };
    static const JSProtoKey key = JSProto_Uint8Array;
    static const JSFunctionSpec jsfuncs[];
};
class Int16ArrayObject : public TypedArrayObjectTemplate<int16_t> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_INT16 };
    static const JSProtoKey key = JSProto_Int16Array;
    static const JSFunctionSpec jsfuncs[];
};
class Uint16ArrayObject : public TypedArrayObjectTemplate<uint16_t> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_UINT16 };
    static const JSProtoKey key = JSProto_Uint16Array;
    static const JSFunctionSpec jsfuncs[];
};
class Int32ArrayObject : public TypedArrayObjectTemplate<int32_t> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_INT32 };
    static const JSProtoKey key = JSProto_Int32Array;
    static const JSFunctionSpec jsfuncs[];
};
class Uint32ArrayObject : public TypedArrayObjectTemplate<uint32_t> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_UINT32 };
    static const JSProtoKey key = JSProto_Uint32Array;
    static const JSFunctionSpec jsfuncs[];
};
class Float32ArrayObject : public TypedArrayObjectTemplate<float> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_FLOAT32 };
    static const JSProtoKey key = JSProto_Float32Array;
    static const JSFunctionSpec jsfuncs[];
};
class Float64ArrayObject : public TypedArrayObjectTemplate<double> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_FLOAT64 };
    static const JSProtoKey key = JSProto_Float64Array;
    static const JSFunctionSpec jsfuncs[];
};
class Uint8ClampedArrayObject : public TypedArrayObjectTemplate<uint8_clamped> {
  public:
    enum { ACTUAL_TYPE = ScalarTypeDescr::TYPE_UINT8_CLAMPED };
    static const JSProtoKey key = JSProto_Uint8ClampedArray;
    static const JSFunctionSpec jsfuncs[];
};

} 

template<typename T>
bool
ArrayBufferObject::createTypedArrayFromBufferImpl(JSContext *cx, CallArgs args)
{
    typedef TypedArrayObjectTemplate<T> ArrayType;
    JS_ASSERT(IsArrayBuffer(args.thisv()));
    JS_ASSERT(args.length() == 3);

    Rooted<JSObject*> buffer(cx, &args.thisv().toObject());
    Rooted<JSObject*> proto(cx, &args[2].toObject());

    Rooted<JSObject*> obj(cx);
    double byteOffset = args[0].toNumber();
    MOZ_ASSERT(0 <= byteOffset);
    MOZ_ASSERT(byteOffset <= UINT32_MAX);
    MOZ_ASSERT(byteOffset == uint32_t(byteOffset));
    obj = ArrayType::fromBuffer(cx, buffer, uint32_t(byteOffset), args[1].toInt32(), proto);
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

template<typename T>
bool
ArrayBufferObject::createTypedArrayFromBuffer(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<IsArrayBuffer, createTypedArrayFromBufferImpl<T> >(cx, args);
}



template<typename NativeType>
Value
TypedArrayObjectTemplate<NativeType>::getIndexValue(JSObject *tarray, uint32_t index)
{
    JS_STATIC_ASSERT(sizeof(NativeType) < 4);

    return Int32Value(getIndex(tarray, index));
}

namespace {


template<>
Value
TypedArrayObjectTemplate<int32_t>::getIndexValue(JSObject *tarray, uint32_t index)
{
    return Int32Value(getIndex(tarray, index));
}

template<>
Value
TypedArrayObjectTemplate<uint32_t>::getIndexValue(JSObject *tarray, uint32_t index)
{
    uint32_t val = getIndex(tarray, index);
    return NumberValue(val);
}

template<>
Value
TypedArrayObjectTemplate<float>::getIndexValue(JSObject *tarray, uint32_t index)
{
    float val = getIndex(tarray, index);
    double dval = val;

    









    return DoubleValue(CanonicalizeNaN(dval));
}

template<>
Value
TypedArrayObjectTemplate<double>::getIndexValue(JSObject *tarray, uint32_t index)
{
    double val = getIndex(tarray, index);

    






    return DoubleValue(CanonicalizeNaN(val));
}

} 

static NewObjectKind
DataViewNewObjectKind(JSContext *cx, uint32_t byteLength, JSObject *proto)
{
    if (!proto && byteLength >= TypedArrayObject::SINGLETON_TYPE_BYTE_LENGTH)
        return SingletonObject;
    jsbytecode *pc;
    JSScript *script = cx->currentScript(&pc);
    if (!script)
        return GenericObject;
    return types::UseNewTypeForInitializer(script, pc, &DataViewObject::class_);
}

inline DataViewObject *
DataViewObject::create(JSContext *cx, uint32_t byteOffset, uint32_t byteLength,
                       Handle<ArrayBufferObject*> arrayBuffer, JSObject *protoArg)
{
    JS_ASSERT(byteOffset <= INT32_MAX);
    JS_ASSERT(byteLength <= INT32_MAX);

    RootedObject proto(cx, protoArg);
    RootedObject obj(cx);

    NewObjectKind newKind = DataViewNewObjectKind(cx, byteLength, proto);
    obj = NewBuiltinClassInstance(cx, &class_, newKind);
    if (!obj)
        return nullptr;

    if (proto) {
        types::TypeObject *type = cx->getNewType(&class_, TaggedProto(proto));
        if (!type)
            return nullptr;
        obj->setType(type);
    } else if (byteLength >= TypedArrayObject::SINGLETON_TYPE_BYTE_LENGTH) {
        JS_ASSERT(obj->hasSingletonType());
    } else {
        jsbytecode *pc;
        RootedScript script(cx, cx->currentScript(&pc));
        if (script) {
            if (!types::SetInitializerObjectType(cx, script, pc, obj, newKind))
                return nullptr;
        }
    }

    DataViewObject &dvobj = obj->as<DataViewObject>();
    dvobj.setFixedSlot(BYTEOFFSET_SLOT, Int32Value(byteOffset));
    dvobj.setFixedSlot(BYTELENGTH_SLOT, Int32Value(byteLength));
    dvobj.setFixedSlot(BUFFER_SLOT, ObjectValue(*arrayBuffer));
    dvobj.setFixedSlot(NEXT_VIEW_SLOT, PrivateValue(nullptr));
    InitArrayBufferViewDataPointer(&dvobj, arrayBuffer, byteOffset);
    JS_ASSERT(byteOffset + byteLength <= arrayBuffer->byteLength());

    
    JS_ASSERT(dvobj.numFixedSlots() == DATA_SLOT);

    arrayBuffer->addView(&dvobj);

    return &dvobj;
}

bool
DataViewObject::construct(JSContext *cx, JSObject *bufobj, const CallArgs &args, HandleObject proto)
{
    if (!IsArrayBuffer(bufobj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_NOT_EXPECTED_TYPE,
                             "DataView", "ArrayBuffer", bufobj->getClass()->name);
        return false;
    }

    Rooted<ArrayBufferObject*> buffer(cx, &AsArrayBuffer(bufobj));
    uint32_t bufferLength = buffer->byteLength();
    uint32_t byteOffset = 0;
    uint32_t byteLength = bufferLength;

    if (args.length() > 1) {
        if (!ToUint32(cx, args[1], &byteOffset))
            return false;
        if (byteOffset > INT32_MAX) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_ARG_INDEX_OUT_OF_RANGE, "1");
            return false;
        }

        if (args.length() > 2) {
            if (!ToUint32(cx, args[2], &byteLength))
                return false;
            if (byteLength > INT32_MAX) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_ARG_INDEX_OUT_OF_RANGE, "2");
                return false;
            }
        } else {
            if (byteOffset > bufferLength) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                     JSMSG_ARG_INDEX_OUT_OF_RANGE, "1");
                return false;
            }

            byteLength = bufferLength - byteOffset;
        }
    }

    
    JS_ASSERT(byteOffset <= INT32_MAX);
    JS_ASSERT(byteLength <= INT32_MAX);

    if (byteOffset + byteLength > bufferLength) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_ARG_INDEX_OUT_OF_RANGE, "1");
        return false;
    }

    JSObject *obj = DataViewObject::create(cx, byteOffset, byteLength, buffer, proto);
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

bool
DataViewObject::class_constructor(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject bufobj(cx);
    if (!GetFirstArgumentAsObject(cx, args, "DataView constructor", &bufobj))
        return false;

    if (bufobj->is<WrapperObject>() && IsArrayBuffer(UncheckedUnwrap(bufobj))) {
        Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
        Rooted<JSObject*> proto(cx, global->getOrCreateDataViewPrototype(cx));
        if (!proto)
            return false;

        InvokeArgs args2(cx);
        if (!args2.init(args.length() + 1))
            return false;
        args2.setCallee(global->createDataViewForThis());
        args2.setThis(ObjectValue(*bufobj));
        PodCopy(args2.array(), args.array(), args.length());
        args2[argc].setObject(*proto);
        if (!Invoke(cx, args2))
            return false;
        args.rval().set(args2.rval());
        return true;
    }

    return construct(cx, bufobj, args, NullPtr());
}

 bool
DataViewObject::getDataPointer(JSContext *cx, Handle<DataViewObject*> obj,
                               CallArgs args, size_t typeSize, uint8_t **data)
{
    uint32_t offset;
    JS_ASSERT(args.length() > 0);
    if (!ToUint32(cx, args[0], &offset))
        return false;
    if (offset > UINT32_MAX - typeSize || offset + typeSize > obj->byteLength()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_ARG_INDEX_OUT_OF_RANGE, "1");
        return false;
    }

    *data = static_cast<uint8_t*>(obj->dataPointer()) + offset;
    return true;
}

static inline bool
needToSwapBytes(bool littleEndian)
{
#if IS_LITTLE_ENDIAN
    return !littleEndian;
#else
    return littleEndian;
#endif
}

static inline uint8_t
swapBytes(uint8_t x)
{
    return x;
}

static inline uint16_t
swapBytes(uint16_t x)
{
    return ((x & 0xff) << 8) | (x >> 8);
}

static inline uint32_t
swapBytes(uint32_t x)
{
    return ((x & 0xff) << 24) |
           ((x & 0xff00) << 8) |
           ((x & 0xff0000) >> 8) |
           ((x & 0xff000000) >> 24);
}

static inline uint64_t
swapBytes(uint64_t x)
{
    uint32_t a = x & UINT32_MAX;
    uint32_t b = x >> 32;
    return (uint64_t(swapBytes(a)) << 32) | swapBytes(b);
}

template <typename DataType> struct DataToRepType { typedef DataType result; };
template <> struct DataToRepType<int8_t>   { typedef uint8_t result; };
template <> struct DataToRepType<uint8_t>  { typedef uint8_t result; };
template <> struct DataToRepType<int16_t>  { typedef uint16_t result; };
template <> struct DataToRepType<uint16_t> { typedef uint16_t result; };
template <> struct DataToRepType<int32_t>  { typedef uint32_t result; };
template <> struct DataToRepType<uint32_t> { typedef uint32_t result; };
template <> struct DataToRepType<float>    { typedef uint32_t result; };
template <> struct DataToRepType<double>   { typedef uint64_t result; };

template <typename DataType>
struct DataViewIO
{
    typedef typename DataToRepType<DataType>::result ReadWriteType;

    static void fromBuffer(DataType *dest, const uint8_t *unalignedBuffer, bool wantSwap)
    {
        JS_ASSERT((reinterpret_cast<uintptr_t>(dest) & (Min<size_t>(MOZ_ALIGNOF(void*), sizeof(DataType)) - 1)) == 0);
        memcpy((void *) dest, unalignedBuffer, sizeof(ReadWriteType));
        if (wantSwap) {
            ReadWriteType *rwDest = reinterpret_cast<ReadWriteType *>(dest);
            *rwDest = swapBytes(*rwDest);
        }
    }

    static void toBuffer(uint8_t *unalignedBuffer, const DataType *src, bool wantSwap)
    {
        JS_ASSERT((reinterpret_cast<uintptr_t>(src) & (Min<size_t>(MOZ_ALIGNOF(void*), sizeof(DataType)) - 1)) == 0);
        ReadWriteType temp = *reinterpret_cast<const ReadWriteType *>(src);
        if (wantSwap)
            temp = swapBytes(temp);
        memcpy(unalignedBuffer, (void *) &temp, sizeof(ReadWriteType));
    }
};

template<typename NativeType>
 bool
DataViewObject::read(JSContext *cx, Handle<DataViewObject*> obj,
                     CallArgs &args, NativeType *val, const char *method)
{
    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_MORE_ARGS_NEEDED, method, "0", "s");
        return false;
    }

    uint8_t *data;
    if (!getDataPointer(cx, obj, args, sizeof(NativeType), &data))
        return false;

    bool fromLittleEndian = args.length() >= 2 && ToBoolean(args[1]);
    DataViewIO<NativeType>::fromBuffer(val, data, needToSwapBytes(fromLittleEndian));
    return true;
}

template <typename NativeType>
static inline bool
WebIDLCast(JSContext *cx, HandleValue value, NativeType *out)
{
    int32_t temp;
    if (!ToInt32(cx, value, &temp))
        return false;
    
    
    
    *out = static_cast<NativeType>(temp);
    return true;
}

template <>
inline bool
WebIDLCast<float>(JSContext *cx, HandleValue value, float *out)
{
    double temp;
    if (!ToNumber(cx, value, &temp))
        return false;
    *out = static_cast<float>(temp);
    return true;
}

template <>
inline bool
WebIDLCast<double>(JSContext *cx, HandleValue value, double *out)
{
    return ToNumber(cx, value, out);
}

template<typename NativeType>
 bool
DataViewObject::write(JSContext *cx, Handle<DataViewObject*> obj,
                      CallArgs &args, const char *method)
{
    if (args.length() < 2) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_MORE_ARGS_NEEDED, method, "1", "");
        return false;
    }

    uint8_t *data;
    SkipRoot skipData(cx, &data);
    if (!getDataPointer(cx, obj, args, sizeof(NativeType), &data))
        return false;

    NativeType value;
    if (!WebIDLCast(cx, args[1], &value))
        return false;

    bool toLittleEndian = args.length() >= 3 && ToBoolean(args[2]);
    DataViewIO<NativeType>::toBuffer(data, &value, needToSwapBytes(toLittleEndian));
    return true;
}

bool
DataViewObject::getInt8Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    int8_t val;
    if (!read(cx, thisView, args, &val, "getInt8"))
        return false;
    args.rval().setInt32(val);
    return true;
}

bool
DataViewObject::fun_getInt8(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getInt8Impl>(cx, args);
}

bool
DataViewObject::getUint8Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    uint8_t val;
    if (!read(cx, thisView, args, &val, "getUint8"))
        return false;
    args.rval().setInt32(val);
    return true;
}

bool
DataViewObject::fun_getUint8(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getUint8Impl>(cx, args);
}

bool
DataViewObject::getInt16Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    int16_t val;
    if (!read(cx, thisView, args, &val, "getInt16"))
        return false;
    args.rval().setInt32(val);
    return true;
}

bool
DataViewObject::fun_getInt16(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getInt16Impl>(cx, args);
}

bool
DataViewObject::getUint16Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    uint16_t val;
    if (!read(cx, thisView, args, &val, "getUint16"))
        return false;
    args.rval().setInt32(val);
    return true;
}

bool
DataViewObject::fun_getUint16(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getUint16Impl>(cx, args);
}

bool
DataViewObject::getInt32Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    int32_t val;
    if (!read(cx, thisView, args, &val, "getInt32"))
        return false;
    args.rval().setInt32(val);
    return true;
}

bool
DataViewObject::fun_getInt32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getInt32Impl>(cx, args);
}

bool
DataViewObject::getUint32Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    uint32_t val;
    if (!read(cx, thisView, args, &val, "getUint32"))
        return false;
    args.rval().setNumber(val);
    return true;
}

bool
DataViewObject::fun_getUint32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getUint32Impl>(cx, args);
}

bool
DataViewObject::getFloat32Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    float val;
    if (!read(cx, thisView, args, &val, "getFloat32"))
        return false;

    args.rval().setDouble(CanonicalizeNaN(val));
    return true;
}

bool
DataViewObject::fun_getFloat32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getFloat32Impl>(cx, args);
}

bool
DataViewObject::getFloat64Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    double val;
    if (!read(cx, thisView, args, &val, "getFloat64"))
        return false;

    args.rval().setDouble(CanonicalizeNaN(val));
    return true;
}

bool
DataViewObject::fun_getFloat64(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getFloat64Impl>(cx, args);
}

bool
DataViewObject::setInt8Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<int8_t>(cx, thisView, args, "setInt8"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setInt8(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setInt8Impl>(cx, args);
}

bool
DataViewObject::setUint8Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<uint8_t>(cx, thisView, args, "setUint8"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setUint8(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setUint8Impl>(cx, args);
}

bool
DataViewObject::setInt16Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<int16_t>(cx, thisView, args, "setInt16"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setInt16(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setInt16Impl>(cx, args);
}

bool
DataViewObject::setUint16Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<uint16_t>(cx, thisView, args, "setUint16"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setUint16(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setUint16Impl>(cx, args);
}

bool
DataViewObject::setInt32Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<int32_t>(cx, thisView, args, "setInt32"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setInt32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setInt32Impl>(cx, args);
}

bool
DataViewObject::setUint32Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<uint32_t>(cx, thisView, args, "setUint32"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setUint32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setUint32Impl>(cx, args);
}

bool
DataViewObject::setFloat32Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<float>(cx, thisView, args, "setFloat32"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setFloat32(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setFloat32Impl>(cx, args);
}

bool
DataViewObject::setFloat64Impl(JSContext *cx, CallArgs args)
{
    JS_ASSERT(is(args.thisv()));

    Rooted<DataViewObject*> thisView(cx, &args.thisv().toObject().as<DataViewObject>());

    if (!write<double>(cx, thisView, args, "setFloat64"))
        return false;
    args.rval().setUndefined();
    return true;
}

bool
DataViewObject::fun_setFloat64(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, setFloat64Impl>(cx, args);
}

Value
TypedArrayObject::getElement(uint32_t index)
{
    JS_ASSERT(index < length());

    switch (type()) {
      case ScalarTypeDescr::TYPE_INT8:
        return TypedArrayObjectTemplate<int8_t>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_UINT8:
        return TypedArrayObjectTemplate<uint8_t>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_UINT8_CLAMPED:
        return TypedArrayObjectTemplate<uint8_clamped>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_INT16:
        return TypedArrayObjectTemplate<int16_t>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_UINT16:
        return TypedArrayObjectTemplate<uint16_t>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_INT32:
        return TypedArrayObjectTemplate<int32_t>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_UINT32:
        return TypedArrayObjectTemplate<uint32_t>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_FLOAT32:
        return TypedArrayObjectTemplate<float>::getIndexValue(this, index);
        break;
      case ScalarTypeDescr::TYPE_FLOAT64:
        return TypedArrayObjectTemplate<double>::getIndexValue(this, index);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown TypedArray type");
        break;
    }
}

bool
TypedArrayObject::setElement(ThreadSafeContext *cx, uint32_t index, const Value &value)
{
    JS_ASSERT(index < length());

    switch (type()) {
      case ScalarTypeDescr::TYPE_INT8:
        return TypedArrayObjectTemplate<int8_t>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_UINT8:
        return TypedArrayObjectTemplate<uint8_t>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_UINT8_CLAMPED:
        return TypedArrayObjectTemplate<uint8_clamped>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_INT16:
        return TypedArrayObjectTemplate<int16_t>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_UINT16:
        return TypedArrayObjectTemplate<uint16_t>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_INT32:
        return TypedArrayObjectTemplate<int32_t>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_UINT32:
        return TypedArrayObjectTemplate<uint32_t>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_FLOAT32:
        return TypedArrayObjectTemplate<float>::setIndexValue(cx, this, index, value);
        break;
      case ScalarTypeDescr::TYPE_FLOAT64:
        return TypedArrayObjectTemplate<double>::setIndexValue(cx, this, index, value);
        break;
      default:
        MOZ_ASSUME_UNREACHABLE("Unknown TypedArray type");
        break;
    }
}









#ifndef RELEASE_BUILD
# define IMPL_TYPED_ARRAY_STATICS(_typedArray)                                     \
const JSFunctionSpec _typedArray##Object::jsfuncs[] = {                            \
    JS_SELF_HOSTED_FN("@@iterator", "ArrayValues", 0, 0),                          \
    JS_FN("subarray", _typedArray##Object::fun_subarray, 2, JSFUN_GENERIC_NATIVE), \
    JS_FN("set", _typedArray##Object::fun_set, 2, JSFUN_GENERIC_NATIVE),           \
    JS_FN("move", _typedArray##Object::fun_move, 3, JSFUN_GENERIC_NATIVE),         \
    JS_FS_END                                                                      \
}
#else
# define IMPL_TYPED_ARRAY_STATICS(_typedArray)                                     \
const JSFunctionSpec _typedArray##Object::jsfuncs[] = {                            \
    JS_SELF_HOSTED_FN("@@iterator", "ArrayValues", 0, 0),                          \
    JS_FN("subarray", _typedArray##Object::fun_subarray, 2, JSFUN_GENERIC_NATIVE), \
    JS_FN("set", _typedArray##Object::fun_set, 2, JSFUN_GENERIC_NATIVE),           \
    JS_FS_END                                                                      \
}
#endif

#define IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Name,NativeType)                                    \
  JS_FRIEND_API(JSObject *) JS_New ## Name ## Array(JSContext *cx, uint32_t nelements)          \
  {                                                                                             \
      return TypedArrayObjectTemplate<NativeType>::fromLength(cx, nelements);                   \
  }                                                                                             \
  JS_FRIEND_API(JSObject *) JS_New ## Name ## ArrayFromArray(JSContext *cx, HandleObject other) \
  {                                                                                             \
      return TypedArrayObjectTemplate<NativeType>::fromArray(cx, other);                        \
  }                                                                                             \
  JS_FRIEND_API(JSObject *) JS_New ## Name ## ArrayWithBuffer(JSContext *cx,                    \
                               HandleObject arrayBuffer, uint32_t byteOffset, int32_t length)   \
  {                                                                                             \
      return TypedArrayObjectTemplate<NativeType>::fromBuffer(cx, arrayBuffer, byteOffset,      \
                                                              length, js::NullPtr());           \
  }                                                                                             \
  JS_FRIEND_API(bool) JS_Is ## Name ## Array(JSObject *obj)                                     \
  {                                                                                             \
      if (!(obj = CheckedUnwrap(obj)))                                                          \
          return false;                                                                         \
      const Class *clasp = obj->getClass();                                                     \
      return (clasp == &TypedArrayObject::classes[TypedArrayObjectTemplate<NativeType>::ArrayTypeID()]); \
  }

IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Int8, int8_t)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Uint8, uint8_t)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Uint8Clamped, uint8_clamped)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Int16, int16_t)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Uint16, uint16_t)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Int32, int32_t)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Uint32, uint32_t)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Float32, float)
IMPL_TYPED_ARRAY_JSAPI_CONSTRUCTORS(Float64, double)

#define IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Name, ExternalType, InternalType)              \
  JS_FRIEND_API(JSObject *) JS_GetObjectAs ## Name ## Array(JSObject *obj,                  \
                                                            uint32_t *length,               \
                                                            ExternalType **data)            \
  {                                                                                         \
      if (!(obj = CheckedUnwrap(obj)))                                                      \
          return nullptr;                                                                   \
                                                                                            \
      const Class *clasp = obj->getClass();                                                 \
      if (clasp != &TypedArrayObject::classes[TypedArrayObjectTemplate<InternalType>::ArrayTypeID()]) \
          return nullptr;                                                                   \
                                                                                            \
      TypedArrayObject *tarr = &obj->as<TypedArrayObject>();                                \
      *length = tarr->length();                                                             \
      *data = static_cast<ExternalType *>(tarr->viewData());                                \
                                                                                            \
      return obj;                                                                           \
  }

IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Int8, int8_t, int8_t)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Uint8, uint8_t, uint8_t)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Uint8Clamped, uint8_t, uint8_clamped)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Int16, int16_t, int16_t)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Uint16, uint16_t, uint16_t)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Int32, int32_t, int32_t)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Uint32, uint32_t, uint32_t)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Float32, float, float)
IMPL_TYPED_ARRAY_COMBINED_UNWRAPPERS(Float64, double, double)

#define IMPL_TYPED_ARRAY_PROTO_CLASS(_typedArray)                              \
{                                                                              \
    #_typedArray "Prototype",                                                  \
    JSCLASS_HAS_RESERVED_SLOTS(TypedArrayObject::RESERVED_SLOTS) |             \
    JSCLASS_HAS_PRIVATE |                                                      \
    JSCLASS_HAS_CACHED_PROTO(JSProto_##_typedArray),                           \
    JS_PropertyStub,         /* addProperty */                                 \
    JS_DeletePropertyStub,   /* delProperty */                                 \
    JS_PropertyStub,         /* getProperty */                                 \
    JS_StrictPropertyStub,   /* setProperty */                                 \
    JS_EnumerateStub,                                                          \
    JS_ResolveStub,                                                            \
    JS_ConvertStub                                                             \
}

#define IMPL_TYPED_ARRAY_FAST_CLASS(_typedArray)                               \
{                                                                              \
    #_typedArray,                                                              \
    JSCLASS_HAS_RESERVED_SLOTS(TypedArrayObject::RESERVED_SLOTS) |             \
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |                        \
    JSCLASS_HAS_CACHED_PROTO(JSProto_##_typedArray),                           \
    JS_PropertyStub,         /* addProperty */                                 \
    JS_DeletePropertyStub,   /* delProperty */                                 \
    JS_PropertyStub,         /* getProperty */                                 \
    JS_StrictPropertyStub,   /* setProperty */                                 \
    JS_EnumerateStub,                                                          \
    JS_ResolveStub,                                                            \
    JS_ConvertStub,                                                            \
    nullptr,                 /* finalize */                                    \
    nullptr,                 /* call        */                                 \
    nullptr,                 /* hasInstance */                                 \
    nullptr,                 /* construct   */                                 \
    ArrayBufferViewObject::trace, /* trace  */                                 \
}

template<class ArrayType>
static inline bool
InitTypedArrayClass(JSContext *cx)
{
    Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
    if (global->isStandardClassResolved(ArrayType::key))
        return true;

    RootedObject proto(cx, global->createBlankPrototype(cx, ArrayType::protoClass()));
    if (!proto)
        return false;

    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, ArrayType::class_constructor,
                                     ClassName(ArrayType::key, cx), 3);
    if (!ctor)
        return false;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return false;

    RootedValue bytesValue(cx, Int32Value(ArrayType::BYTES_PER_ELEMENT));

    if (!JSObject::defineProperty(cx, ctor,
                                  cx->names().BYTES_PER_ELEMENT, bytesValue,
                                  JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY) ||
        !JSObject::defineProperty(cx, proto,
                                  cx->names().BYTES_PER_ELEMENT, bytesValue,
                                  JS_PropertyStub, JS_StrictPropertyStub,
                                  JSPROP_PERMANENT | JSPROP_READONLY))
    {
        return false;
    }

    if (!ArrayType::defineGetters(cx, proto))
        return false;

    if (!JS_DefineFunctions(cx, proto, ArrayType::jsfuncs))
        return false;

    RootedFunction fun(cx);
    fun =
        NewFunction(cx, NullPtr(),
                    ArrayBufferObject::createTypedArrayFromBuffer<typename ArrayType::ThisType>,
                    0, JSFunction::NATIVE_FUN, global, NullPtr());
    if (!fun)
        return false;

    if (!GlobalObject::initBuiltinConstructor(cx, global, ArrayType::key, ctor, proto))
        return false;

    global->setCreateArrayFromBuffer<typename ArrayType::ThisType>(fun);

    return true;
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

const Class TypedArrayObject::classes[ScalarTypeDescr::TYPE_MAX] = {
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

const Class TypedArrayObject::protoClasses[ScalarTypeDescr::TYPE_MAX] = {
    IMPL_TYPED_ARRAY_PROTO_CLASS(Int8Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Uint8Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Int16Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Uint16Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Int32Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Uint32Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Float32Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Float64Array),
    IMPL_TYPED_ARRAY_PROTO_CLASS(Uint8ClampedArray)
};

#define CHECK(t, a) { if (t == a::IsThisClass) return true; }
JS_FRIEND_API(bool)
js::IsTypedArrayThisCheck(JS::IsAcceptableThis test)
{
    CHECK(test, Int8ArrayObject);
    CHECK(test, Uint8ArrayObject);
    CHECK(test, Int16ArrayObject);
    CHECK(test, Uint16ArrayObject);
    CHECK(test, Int32ArrayObject);
    CHECK(test, Uint32ArrayObject);
    CHECK(test, Float32ArrayObject);
    CHECK(test, Float64ArrayObject);
    CHECK(test, Uint8ClampedArrayObject);
    return false;
}
#undef CHECK

static JSObject *
InitArrayBufferClass(JSContext *cx)
{
    Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
    if (global->isStandardClassResolved(JSProto_ArrayBuffer))
        return &global->getPrototype(JSProto_ArrayBuffer).toObject();

    RootedObject arrayBufferProto(cx, global->createBlankPrototype(cx, &ArrayBufferObject::protoClass));
    if (!arrayBufferProto)
        return nullptr;

    RootedFunction ctor(cx, global->createConstructor(cx, ArrayBufferObject::class_constructor,
                                                      cx->names().ArrayBuffer, 1));
    if (!ctor)
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, ctor, arrayBufferProto))
        return nullptr;

    RootedId byteLengthId(cx, NameToId(cx->names().byteLength));
    unsigned flags = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;
    JSObject *getter = NewFunction(cx, NullPtr(), ArrayBufferObject::byteLengthGetter, 0,
                                   JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return nullptr;

    if (!DefineNativeProperty(cx, arrayBufferProto, byteLengthId, UndefinedHandleValue,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr, flags, 0, 0))
        return nullptr;

    if (!JS_DefineFunctions(cx, ctor, ArrayBufferObject::jsstaticfuncs))
        return nullptr;

    if (!JS_DefineFunctions(cx, arrayBufferProto, ArrayBufferObject::jsfuncs))
        return nullptr;

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_ArrayBuffer,
                                              ctor, arrayBufferProto))
    {
        return nullptr;
    }

    return arrayBufferProto;
}

const Class DataViewObject::protoClass = {
    "DataViewPrototype",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_HAS_RESERVED_SLOTS(DataViewObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_DataView),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

const Class DataViewObject::class_ = {
    "DataView",
    JSCLASS_HAS_PRIVATE |
    JSCLASS_IMPLEMENTS_BARRIERS |
    JSCLASS_HAS_RESERVED_SLOTS(DataViewObject::RESERVED_SLOTS) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_DataView),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,                 
    nullptr,                 
    nullptr,                 
    nullptr,                 
    ArrayBufferViewObject::trace, 
};

const JSFunctionSpec DataViewObject::jsfuncs[] = {
    JS_FN("getInt8",    DataViewObject::fun_getInt8,      1,0),
    JS_FN("getUint8",   DataViewObject::fun_getUint8,     1,0),
    JS_FN("getInt16",   DataViewObject::fun_getInt16,     2,0),
    JS_FN("getUint16",  DataViewObject::fun_getUint16,    2,0),
    JS_FN("getInt32",   DataViewObject::fun_getInt32,     2,0),
    JS_FN("getUint32",  DataViewObject::fun_getUint32,    2,0),
    JS_FN("getFloat32", DataViewObject::fun_getFloat32,   2,0),
    JS_FN("getFloat64", DataViewObject::fun_getFloat64,   2,0),
    JS_FN("setInt8",    DataViewObject::fun_setInt8,      2,0),
    JS_FN("setUint8",   DataViewObject::fun_setUint8,     2,0),
    JS_FN("setInt16",   DataViewObject::fun_setInt16,     3,0),
    JS_FN("setUint16",  DataViewObject::fun_setUint16,    3,0),
    JS_FN("setInt32",   DataViewObject::fun_setInt32,     3,0),
    JS_FN("setUint32",  DataViewObject::fun_setUint32,    3,0),
    JS_FN("setFloat32", DataViewObject::fun_setFloat32,   3,0),
    JS_FN("setFloat64", DataViewObject::fun_setFloat64,   3,0),
    JS_FS_END
};

template<Value ValueGetter(DataViewObject *view)>
bool
DataViewObject::getterImpl(JSContext *cx, CallArgs args)
{
    args.rval().set(ValueGetter(&args.thisv().toObject().as<DataViewObject>()));
    return true;
}

template<Value ValueGetter(DataViewObject *view)>
bool
DataViewObject::getter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<is, getterImpl<ValueGetter> >(cx, args);
}

template<Value ValueGetter(DataViewObject *view)>
bool
DataViewObject::defineGetter(JSContext *cx, PropertyName *name, HandleObject proto)
{
    RootedId id(cx, NameToId(name));
    unsigned flags = JSPROP_SHARED | JSPROP_GETTER | JSPROP_PERMANENT;

    Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
    JSObject *getter = NewFunction(cx, NullPtr(), DataViewObject::getter<ValueGetter>, 0,
                                   JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return false;

    return DefineNativeProperty(cx, proto, id, UndefinedHandleValue,
                                JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr,
                                flags, 0, 0);
}

 bool
DataViewObject::initClass(JSContext *cx)
{
    Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
    if (global->isStandardClassResolved(JSProto_DataView))
        return true;

    RootedObject proto(cx, global->createBlankPrototype(cx, &DataViewObject::protoClass));
    if (!proto)
        return false;

    RootedFunction ctor(cx, global->createConstructor(cx, DataViewObject::class_constructor,
                                                      cx->names().DataView, 3));
    if (!ctor)
        return false;

    if (!LinkConstructorAndPrototype(cx, ctor, proto))
        return false;

    if (!defineGetter<bufferValue>(cx, cx->names().buffer, proto))
        return false;

    if (!defineGetter<byteLengthValue>(cx, cx->names().byteLength, proto))
        return false;

    if (!defineGetter<byteOffsetValue>(cx, cx->names().byteOffset, proto))
        return false;

    if (!JS_DefineFunctions(cx, proto, DataViewObject::jsfuncs))
        return false;

    




    RootedFunction fun(cx, NewFunction(cx, NullPtr(), ArrayBufferObject::createDataViewForThis,
                                       0, JSFunction::NATIVE_FUN, global, NullPtr()));
    if (!fun)
        return false;

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_DataView, ctor, proto))
        return false;

    global->setCreateDataViewForThis(fun);

    return true;
}

void
DataViewObject::neuter(void *newData)
{
    setSlot(BYTELENGTH_SLOT, Int32Value(0));
    setSlot(BYTEOFFSET_SLOT, Int32Value(0));
    setPrivate(newData);
}

JSObject *
js_InitTypedArrayClasses(JSContext *cx, HandleObject obj)
{
    if (!InitTypedArrayClass<Int8ArrayObject>(cx) ||
        !InitTypedArrayClass<Uint8ArrayObject>(cx) ||
        !InitTypedArrayClass<Int16ArrayObject>(cx) ||
        !InitTypedArrayClass<Uint16ArrayObject>(cx) ||
        !InitTypedArrayClass<Int32ArrayObject>(cx) ||
        !InitTypedArrayClass<Uint32ArrayObject>(cx) ||
        !InitTypedArrayClass<Float32ArrayObject>(cx) ||
        !InitTypedArrayClass<Float64ArrayObject>(cx) ||
        !InitTypedArrayClass<Uint8ClampedArrayObject>(cx) ||
        !DataViewObject::initClass(cx))
    {
        return nullptr;
    }

    return InitArrayBufferClass(cx);
}

bool
js::IsTypedArrayConstructor(HandleValue v, uint32_t type)
{
    switch (type) {
      case ScalarTypeDescr::TYPE_INT8:
        return IsNativeFunction(v, Int8ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_UINT8:
        return IsNativeFunction(v, Uint8ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_INT16:
        return IsNativeFunction(v, Int16ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_UINT16:
        return IsNativeFunction(v, Uint16ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_INT32:
        return IsNativeFunction(v, Int32ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_UINT32:
        return IsNativeFunction(v, Uint32ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_FLOAT32:
        return IsNativeFunction(v, Float32ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_FLOAT64:
        return IsNativeFunction(v, Float64ArrayObject::class_constructor);
      case ScalarTypeDescr::TYPE_UINT8_CLAMPED:
        return IsNativeFunction(v, Uint8ClampedArrayObject::class_constructor);
    }
    MOZ_ASSUME_UNREACHABLE("unexpected typed array type");
}

bool
js::IsTypedArrayBuffer(HandleValue v)
{
    return v.isObject() &&
           (v.toObject().is<ArrayBufferObject>() ||
            v.toObject().is<SharedArrayBufferObject>());
}

ArrayBufferObject &
js::AsTypedArrayBuffer(HandleValue v)
{
    JS_ASSERT(IsTypedArrayBuffer(v));
    if (v.toObject().is<ArrayBufferObject>())
        return v.toObject().as<ArrayBufferObject>();
    return v.toObject().as<SharedArrayBufferObject>();
}

bool
js::StringIsTypedArrayIndex(JSLinearString *str, uint64_t *indexp)
{
    const jschar *s = str->chars();
    const jschar *end = s + str->length();

    if (s == end)
        return false;

    bool negative = false;
    if (*s == '-') {
        negative = true;
        if (++s == end)
            return false;
    }

    if (!JS7_ISDEC(*s))
        return false;

    uint64_t index = 0;
    uint32_t digit = JS7_UNDEC(*s++);

    
    if (digit == 0 && s != end)
        return false;

    index = digit;

    for (; s < end; s++) {
        if (!JS7_ISDEC(*s))
            return false;

        digit = JS7_UNDEC(*s);

        
        if ((UINT64_MAX - digit) / 10 < index)
            index = UINT64_MAX;
        else
            index = 10 * index + digit;
    }

    if (negative)
        *indexp = UINT64_MAX;
    else
        *indexp = index;
    return true;
}



JS_FRIEND_API(bool)
JS_IsTypedArrayObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<TypedArrayObject>() : false;
}

JS_FRIEND_API(uint32_t)
JS_GetTypedArrayLength(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->as<TypedArrayObject>().length();
}

JS_FRIEND_API(uint32_t)
JS_GetTypedArrayByteOffset(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->as<TypedArrayObject>().byteOffset();
}

JS_FRIEND_API(uint32_t)
JS_GetTypedArrayByteLength(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->as<TypedArrayObject>().byteLength();
}

JS_FRIEND_API(JSArrayBufferViewType)
JS_GetArrayBufferViewType(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return ArrayBufferView::TYPE_MAX;

    if (obj->is<TypedArrayObject>())
        return static_cast<JSArrayBufferViewType>(obj->as<TypedArrayObject>().type());
    else if (obj->is<DataViewObject>())
        return ArrayBufferView::TYPE_DATAVIEW;
    MOZ_ASSUME_UNREACHABLE("invalid ArrayBufferView type");
}

JS_FRIEND_API(int8_t *)
JS_GetInt8ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_INT8);
    return static_cast<int8_t *>(tarr->viewData());
}

JS_FRIEND_API(uint8_t *)
JS_GetUint8ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_UINT8);
    return static_cast<uint8_t *>(tarr->viewData());
}

JS_FRIEND_API(uint8_t *)
JS_GetUint8ClampedArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_UINT8_CLAMPED);
    return static_cast<uint8_t *>(tarr->viewData());
}

JS_FRIEND_API(int16_t *)
JS_GetInt16ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_INT16);
    return static_cast<int16_t *>(tarr->viewData());
}

JS_FRIEND_API(uint16_t *)
JS_GetUint16ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_UINT16);
    return static_cast<uint16_t *>(tarr->viewData());
}

JS_FRIEND_API(int32_t *)
JS_GetInt32ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_INT32);
    return static_cast<int32_t *>(tarr->viewData());
}

JS_FRIEND_API(uint32_t *)
JS_GetUint32ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_UINT32);
    return static_cast<uint32_t *>(tarr->viewData());
}

JS_FRIEND_API(float *)
JS_GetFloat32ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_FLOAT32);
    return static_cast<float *>(tarr->viewData());
}

JS_FRIEND_API(double *)
JS_GetFloat64ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT(tarr->type() == ArrayBufferView::TYPE_FLOAT64);
    return static_cast<double *>(tarr->viewData());
}

JS_FRIEND_API(bool)
JS_IsDataViewObject(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    return obj ? obj->is<DataViewObject>() : false;
}

JS_FRIEND_API(uint32_t)
JS_GetDataViewByteOffset(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->as<DataViewObject>().byteOffset();
}

JS_FRIEND_API(void *)
JS_GetDataViewData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    return obj->as<DataViewObject>().dataPointer();
}

JS_FRIEND_API(uint32_t)
JS_GetDataViewByteLength(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return 0;
    return obj->as<DataViewObject>().byteLength();
}
