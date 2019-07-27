





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

#include "builtin/TypedObjectConstants.h"
#include "gc/Barrier.h"
#include "gc/Marking.h"
#include "vm/ArrayBufferObject.h"
#include "vm/GlobalObject.h"
#include "vm/Interpreter.h"
#include "vm/NumericConversions.h"
#include "vm/TypedArrayCommon.h"
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









TypedArrayLayout TypedArrayObject::layout_(false, 
                                           true,  
                                           &TypedArrayObject::classes[0],
                                           &TypedArrayObject::classes[Scalar::TypeMax]);

TypedArrayLayout::TypedArrayLayout(bool isShared, bool isNeuterable, const Class *firstClass,
                                   const Class *maxClass)
    : isShared_(isShared)
    , isNeuterable_(isNeuterable)
    , firstClass_(firstClass)
    , maxClass_(maxClass)
{
}

 int
TypedArrayLayout::lengthOffset()
{
    return JSObject::getFixedSlotOffset(LENGTH_SLOT);
}

 int
TypedArrayLayout::dataOffset()
{
    return JSObject::getPrivateDataOffset(DATA_SLOT);
}

void
TypedArrayObject::neuter(void *newData)
{
    setSlot(TypedArrayLayout::LENGTH_SLOT, Int32Value(0));
    setSlot(TypedArrayLayout::BYTEOFFSET_SLOT, Int32Value(0));
    setPrivate(newData);
}

 bool
TypedArrayObject::is(HandleValue v)
{
    return v.isObject() && v.toObject().is<TypedArrayObject>();
}

 bool
TypedArrayObject::ensureHasBuffer(JSContext *cx, Handle<TypedArrayObject *> tarray)
{
    if (tarray->buffer())
        return true;

    Rooted<ArrayBufferObject *> buffer(cx, ArrayBufferObject::create(cx, tarray->byteLength()));
    if (!buffer)
        return false;

    if (!buffer->addView(cx, tarray))
        return false;

    memcpy(buffer->dataPointer(), tarray->viewData(), tarray->byteLength());
    InitArrayBufferViewDataPointer(tarray, buffer, 0);

    tarray->setSlot(TypedArrayLayout::BUFFER_SLOT, ObjectValue(*buffer));
    return true;
}

 void
TypedArrayObject::ObjectMoved(JSObject *obj, const JSObject *old)
{
    const TypedArrayObject &src = old->as<TypedArrayObject>();
    if (!src.hasBuffer()) {
        JS_ASSERT(old->getPrivate() == old->fixedData(FIXED_DATA_START));
        obj->setPrivate(obj->fixedData(FIXED_DATA_START));
    }
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
        







        return y & ~1;
    }

    return y;
}

template<typename ElementType>
static inline JSObject *
NewArray(JSContext *cx, uint32_t nelements);

namespace {








template<typename NativeType>
class TypedArrayObjectTemplate : public TypedArrayObject
{
    friend class TypedArrayObject;

  public:
    typedef NativeType ElementType;

    static Scalar::Type ArrayTypeID() { return TypeIDOfType<NativeType>(); }
    static bool ArrayTypeIsUnsigned() { return TypeIsUnsigned<NativeType>(); }
    static bool ArrayTypeIsFloatingPoint() { return TypeIsFloatingPoint<NativeType>(); }

    static const size_t BYTES_PER_ELEMENT = sizeof(NativeType);

    static JSObject *
    createPrototype(JSContext *cx, JSProtoKey key)
    {
        Handle<GlobalObject*> global = cx->global();
        RootedObject typedArrayProto(cx, GlobalObject::getOrCreateTypedArrayPrototype(cx, global));
        if (!typedArrayProto)
            return nullptr;

        const Class *clasp = TypedArrayObject::protoClassForType(ArrayTypeID());
        return global->createBlankPrototypeInheriting(cx, clasp, *typedArrayProto);
    }

    static JSObject *
    createConstructor(JSContext *cx, JSProtoKey key)
    {
        Handle<GlobalObject*> global = cx->global();
        RootedFunction ctorProto(cx, GlobalObject::getOrCreateTypedArrayConstructor(cx, global));
        if (!ctorProto)
            return nullptr;

        RootedObject ctorObj(cx, NewObjectWithGivenProto(cx, &JSFunction::class_,
                                                         ctorProto, global, SingletonObject));
        if (!ctorObj)
            return nullptr;

        return NewFunction(cx, ctorObj, class_constructor, 3, JSFunction::NATIVE_CTOR, global,
                           ClassName(key, cx), JSFunction::FinalizeKind);
    }

    static bool
    finishClassInit(JSContext *cx, HandleObject ctor, HandleObject proto)
    {
        RootedValue bytesValue(cx, Int32Value(BYTES_PER_ELEMENT));
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

        RootedFunction fun(cx);
        fun = NewFunction(cx, NullPtr(), ArrayBufferObject::createTypedArrayFromBuffer<NativeType>,
                          0, JSFunction::NATIVE_FUN, cx->global(), NullPtr());
        if (!fun)
            return false;

        cx->global()->setCreateArrayFromBuffer<NativeType>(fun);

        return true;
    }

    static inline const Class *instanceClass()
    {
        return TypedArrayObject::classForType(ArrayTypeID());
    }

    static bool is(HandleValue v) {
        return v.isObject() && v.toObject().hasClass(instanceClass());
    }

    static void
    setIndexValue(TypedArrayObject &tarray, uint32_t index, double d)
    {
        
        
        

        
        if (ArrayTypeIsFloatingPoint()) {
            setIndex(tarray, index, NativeType(d));
        } else if (ArrayTypeIsUnsigned()) {
            JS_ASSERT(sizeof(NativeType) <= 4);
            uint32_t n = ToUint32(d);
            setIndex(tarray, index, NativeType(n));
        } else if (ArrayTypeID() == Scalar::Uint8Clamped) {
            
            
            setIndex(tarray, index, NativeType(d));
        } else {
            JS_ASSERT(sizeof(NativeType) <= 4);
            int32_t n = ToInt32(d);
            setIndex(tarray, index, NativeType(n));
        }
    }

    static TypedArrayObject *
    makeProtoInstance(JSContext *cx, HandleObject proto, AllocKind allocKind)
    {
        JS_ASSERT(proto);

        RootedObject obj(cx, NewBuiltinClassInstance(cx, instanceClass(), allocKind));
        if (!obj)
            return nullptr;

        types::TypeObject *type = cx->getNewType(obj->getClass(), TaggedProto(proto.get()));
        if (!type)
            return nullptr;
        obj->setType(type);

        return &obj->as<TypedArrayObject>();
    }

    static TypedArrayObject *
    makeTypedInstance(JSContext *cx, uint32_t len, gc::AllocKind allocKind)
    {
        const Class *clasp = instanceClass();
        if (len * sizeof(NativeType) >= TypedArrayObject::SINGLETON_TYPE_BYTE_LENGTH) {
            JSObject *obj = NewBuiltinClassInstance(cx, clasp, allocKind, SingletonObject);
            if (!obj)
                return nullptr;
            return &obj->as<TypedArrayObject>();
        }

        jsbytecode *pc;
        RootedScript script(cx, cx->currentScript(&pc));
        NewObjectKind newKind = script
                                ? UseNewTypeForInitializer(script, pc, clasp)
                                : GenericObject;
        RootedObject obj(cx, NewBuiltinClassInstance(cx, clasp, allocKind, newKind));
        if (!obj)
            return nullptr;

        if (script) {
            if (!types::SetInitializerObjectType(cx, script, pc, obj, newKind))
                return nullptr;
        }

        return &obj->as<TypedArrayObject>();
    }

    static TypedArrayObject *
    makeInstance(JSContext *cx, Handle<ArrayBufferObject *> buffer, uint32_t byteOffset, uint32_t len,
                 HandleObject proto)
    {
        JS_ASSERT_IF(!buffer, byteOffset == 0);

        gc::AllocKind allocKind = buffer
                                  ? GetGCObjectKind(instanceClass())
                                  : AllocKindForLazyBuffer(len * sizeof(NativeType));

        Rooted<TypedArrayObject*> obj(cx);
        if (proto)
            obj = makeProtoInstance(cx, proto, allocKind);
        else
            obj = makeTypedInstance(cx, len, allocKind);
        if (!obj)
            return nullptr;

        obj->setSlot(TypedArrayLayout::BUFFER_SLOT, ObjectOrNullValue(buffer));

        if (buffer) {
            InitArrayBufferViewDataPointer(obj, buffer, byteOffset);
        } else {
            void *data = obj->fixedData(FIXED_DATA_START);
            obj->initPrivate(data);
            memset(data, 0, len * sizeof(NativeType));
        }

        obj->setSlot(TypedArrayLayout::LENGTH_SLOT, Int32Value(len));
        obj->setSlot(TypedArrayLayout::BYTEOFFSET_SLOT, Int32Value(byteOffset));

#ifdef DEBUG
        if (buffer) {
            uint32_t arrayByteLength = obj->byteLength();
            uint32_t arrayByteOffset = obj->byteOffset();
            uint32_t bufferByteLength = buffer->byteLength();
            JS_ASSERT_IF(!buffer->isNeutered(), buffer->dataPointer() <= obj->viewData());
            JS_ASSERT(bufferByteLength - arrayByteOffset >= arrayByteLength);
            JS_ASSERT(arrayByteOffset <= bufferByteLength);
        }

        
        JS_ASSERT(obj->numFixedSlots() == TypedArrayLayout::DATA_SLOT);
#endif

        if (buffer) {
            if (!buffer->addView(cx, obj))
                return nullptr;
        }

        return obj;
    }

    static TypedArrayObject *
    makeInstance(JSContext *cx, Handle<ArrayBufferObject*> buffer,
                 uint32_t byteOffset, uint32_t len)
    {
        RootedObject proto(cx, nullptr);
        return makeInstance(cx, buffer, byteOffset, len, proto);
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

        









        if (!UncheckedUnwrap(dataObj)->is<ArrayBufferObject>())
            return fromArray(cx, dataObj);

        
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

        return fromBuffer(cx, dataObj, byteOffset, length);
    }

  public:
    static JSObject *
    fromBuffer(JSContext *cx, HandleObject bufobj, uint32_t byteOffset, int32_t lengthInt) {
        RootedObject proto(cx, nullptr);
        return fromBufferWithProto(cx, bufobj, byteOffset, lengthInt, proto);
    }

    static JSObject *
    fromBufferWithProto(JSContext *cx, HandleObject bufobj, uint32_t byteOffset, int32_t lengthInt,
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
                if (!GetBuiltinPrototype(cx, JSCLASS_CACHED_PROTO_KEY(instanceClass()), &proto))
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

        Rooted<ArrayBufferObject *> buffer(cx, &AsArrayBuffer(bufobj));

        if (byteOffset > buffer->byteLength() || byteOffset % sizeof(NativeType) != 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        uint32_t len;
        if (lengthInt == -1) {
            len = (buffer->byteLength() - byteOffset) / sizeof(NativeType);
            if (len * sizeof(NativeType) != buffer->byteLength() - byteOffset) {
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

        if (arrayByteLength + byteOffset > buffer->byteLength()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPED_ARRAY_BAD_ARGS);
            return nullptr; 
        }

        return makeInstance(cx, buffer, byteOffset, len, proto);
    }

    static bool
    maybeCreateArrayBuffer(JSContext *cx, uint32_t nelements, MutableHandle<ArrayBufferObject *> buffer)
    {
        
        
        JS_STATIC_ASSERT((INLINE_BUFFER_LIMIT / sizeof(NativeType)) * sizeof(NativeType) == INLINE_BUFFER_LIMIT);

        if (nelements <= INLINE_BUFFER_LIMIT / sizeof(NativeType)) {
            
            return true;
        }

        if (nelements >= INT32_MAX / sizeof(NativeType)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_NEED_DIET, "size and count");
            return false;
        }

        buffer.set(ArrayBufferObject::create(cx, nelements * sizeof(NativeType)));
        return !!buffer;
    }

    static JSObject *
    fromLength(JSContext *cx, uint32_t nelements)
    {
        Rooted<ArrayBufferObject *> buffer(cx);
        if (!maybeCreateArrayBuffer(cx, nelements, &buffer))
            return nullptr;
        return makeInstance(cx, buffer, 0, nelements);
    }

    static JSObject *
    fromArray(JSContext *cx, HandleObject other);

    static const NativeType
    getIndex(JSObject *obj, uint32_t index)
    {
        TypedArrayObject &tarray = obj->as<TypedArrayObject>();
        MOZ_ASSERT(index < tarray.length());
        return static_cast<const NativeType*>(tarray.viewData())[index];
    }

    static void
    setIndex(TypedArrayObject &tarray, uint32_t index, NativeType val)
    {
        MOZ_ASSERT(index < tarray.length());
        static_cast<NativeType*>(tarray.viewData())[index] = val;
    }

    static Value getIndexValue(JSObject *tarray, uint32_t index);
};

typedef TypedArrayObjectTemplate<int8_t> Int8Array;
typedef TypedArrayObjectTemplate<uint8_t> Uint8Array;
typedef TypedArrayObjectTemplate<int16_t> Int16Array;
typedef TypedArrayObjectTemplate<uint16_t> Uint16Array;
typedef TypedArrayObjectTemplate<int32_t> Int32Array;
typedef TypedArrayObjectTemplate<uint32_t> Uint32Array;
typedef TypedArrayObjectTemplate<float> Float32Array;
typedef TypedArrayObjectTemplate<double> Float64Array;
typedef TypedArrayObjectTemplate<uint8_clamped> Uint8ClampedArray;

} 

template<typename T>
struct TypedArrayObject::OfType
{
    typedef TypedArrayObjectTemplate<T> Type;
};

template<typename T>
 JSObject *
TypedArrayObjectTemplate<T>::fromArray(JSContext *cx, HandleObject other)
{
    uint32_t len;
    if (other->is<TypedArrayObject>()) {
        len = other->as<TypedArrayObject>().length();
    } else if (!GetLengthProperty(cx, other, &len)) {
        return nullptr;
    }

    Rooted<ArrayBufferObject *> buffer(cx);
    if (!maybeCreateArrayBuffer(cx, len, &buffer))
        return nullptr;

    Rooted<TypedArrayObject*> obj(cx, makeInstance(cx, buffer, 0, len));
    if (!obj || !TypedArrayMethods<TypedArrayObject>::setFromArrayLike(cx, obj, other, len))
        return nullptr;
    return obj;
}

bool
TypedArrayConstructor(JSContext *cx, unsigned argc, Value *vp)
{
    JS_ReportError(cx, "%%TypedArray%% calling/constructing not implemented yet");
    return false;
}














static bool
TypedArray_lengthGetter(JSContext *cx, unsigned argc, Value *vp)
{
    return TypedArrayObject::Getter<TypedArrayObject::lengthValue>(cx, argc, vp); \
}

static bool
TypedArray_byteLengthGetter(JSContext *cx, unsigned argc, Value *vp)
{
    return TypedArrayObject::Getter<TypedArrayObject::byteLengthValue>(cx, argc, vp);
}

static bool
TypedArray_byteOffsetGetter(JSContext *cx, unsigned argc, Value *vp)
{
    return TypedArrayObject::Getter<TypedArrayObject::byteOffsetValue>(cx, argc, vp);
}

bool
BufferGetterImpl(JSContext *cx, CallArgs args)
{
    MOZ_ASSERT(TypedArrayObject::is(args.thisv()));
    Rooted<TypedArrayObject *> tarray(cx, &args.thisv().toObject().as<TypedArrayObject>());
    if (!TypedArrayObject::ensureHasBuffer(cx, tarray))
        return false;
    args.rval().set(TypedArrayObject::bufferValue(tarray));
    return true;
}

static bool
TypedArray_bufferGetter(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<TypedArrayObject::is, BufferGetterImpl>(cx, args);
}

 const JSPropertySpec
TypedArrayObject::protoAccessors[] = {
    JS_PSG("length", TypedArray_lengthGetter, 0),
    JS_PSG("buffer", TypedArray_bufferGetter, 0),
    JS_PSG("byteLength", TypedArray_byteLengthGetter, 0),
    JS_PSG("byteOffset", TypedArray_byteOffsetGetter, 0),
    JS_PS_END
};

 bool
TypedArrayObject::copyWithin(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<TypedArrayObject::is,
                                TypedArrayMethods<TypedArrayObject>::copyWithin>(cx, args);
}

 bool
TypedArrayObject::set(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<TypedArrayObject::is,
                                TypedArrayMethods<TypedArrayObject>::set>(cx, args);
}

 bool
TypedArrayObject::subarray(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    return CallNonGenericMethod<TypedArrayObject::is,
                                TypedArrayMethods<TypedArrayObject>::subarray>(cx, args);
}

 const JSFunctionSpec
TypedArrayObject::protoFunctions[] = {
    JS_SELF_HOSTED_FN("@@iterator", "ArrayValues", 0, 0),
    JS_FN("subarray", TypedArrayObject::subarray, 2, 0),
    JS_FN("set", TypedArrayObject::set, 2, 0),
    JS_FN("copyWithin", TypedArrayObject::copyWithin, 2, 0),
    JS_FS_END
};

 const JSFunctionSpec
TypedArrayObject::staticFunctions[] = {
    
    JS_FS_END
};

 const Class
TypedArrayObject::sharedTypedArrayPrototypeClass = {
    
    
    
    
    
    
    "???",
    JSCLASS_HAS_CACHED_PROTO(JSProto_TypedArray),
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
    nullptr,                
    {
        GenericCreateConstructor<TypedArrayConstructor, 3, JSFunction::FinalizeKind>,
        GenericCreatePrototype,
        TypedArrayObject::staticFunctions,
        TypedArrayObject::protoFunctions,
        TypedArrayObject::protoAccessors,
        nullptr,
        ClassSpec::DontDefineConstructor
    }
};

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
    obj = ArrayType::fromBufferWithProto(cx, buffer, uint32_t(byteOffset), args[1].toInt32(),
                                         proto);
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

    
    if (byteOffset + byteLength > arrayBuffer->byteLength()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_ARG_INDEX_OUT_OF_RANGE, "1");
        return nullptr;

    }

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
    dvobj.setFixedSlot(TypedArrayLayout::BYTEOFFSET_SLOT, Int32Value(byteOffset));
    dvobj.setFixedSlot(TypedArrayLayout::LENGTH_SLOT, Int32Value(byteLength));
    dvobj.setFixedSlot(TypedArrayLayout::BUFFER_SLOT, ObjectValue(*arrayBuffer));
    InitArrayBufferViewDataPointer(&dvobj, arrayBuffer, byteOffset);
    JS_ASSERT(byteOffset + byteLength <= arrayBuffer->byteLength());

    
    JS_ASSERT(dvobj.numFixedSlots() == TypedArrayLayout::DATA_SLOT);

    if (!arrayBuffer->addView(cx, &dvobj))
        return nullptr;

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
        args2[args.length()].setObject(*proto);
        if (!Invoke(cx, args2))
            return false;
        args.rval().set(args2.rval());
        return true;
    }

    return construct(cx, bufobj, args, NullPtr());
}

template <typename NativeType>
 uint8_t *
DataViewObject::getDataPointer(JSContext *cx, Handle<DataViewObject*> obj, uint32_t offset)
{
    const size_t TypeSize = sizeof(NativeType);
    if (offset > UINT32_MAX - TypeSize || offset + TypeSize > obj->byteLength()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_ARG_INDEX_OUT_OF_RANGE, "1");
        return nullptr;
    }

    return static_cast<uint8_t*>(obj->dataPointer()) + offset;
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

    uint32_t offset;
    if (!ToUint32(cx, args[0], &offset))
        return false;

    bool fromLittleEndian = args.length() >= 2 && ToBoolean(args[1]);

    uint8_t *data = DataViewObject::getDataPointer<NativeType>(cx, obj, offset);
    if (!data)
        return false;

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

    uint32_t offset;
    if (!ToUint32(cx, args[0], &offset))
        return false;

    NativeType value;
    if (!WebIDLCast(cx, args[1], &value))
        return false;

    bool toLittleEndian = args.length() >= 3 && ToBoolean(args[2]);

    uint8_t *data = DataViewObject::getDataPointer<NativeType>(cx, obj, offset);
    if (!data)
        return false;

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
    switch (type()) {
      case Scalar::Int8:
        return Int8Array::getIndexValue(this, index);
      case Scalar::Uint8:
        return Uint8Array::getIndexValue(this, index);
      case Scalar::Int16:
        return Int16Array::getIndexValue(this, index);
      case Scalar::Uint16:
        return Uint16Array::getIndexValue(this, index);
      case Scalar::Int32:
        return Int32Array::getIndexValue(this, index);
      case Scalar::Uint32:
        return Uint32Array::getIndexValue(this, index);
      case Scalar::Float32:
        return Float32Array::getIndexValue(this, index);
      case Scalar::Float64:
        return Float64Array::getIndexValue(this, index);
      case Scalar::Uint8Clamped:
        return Uint8ClampedArray::getIndexValue(this, index);
      case Scalar::TypeMax:
        break;
    }

    MOZ_CRASH("Unknown TypedArray type");
}

void
TypedArrayObject::setElement(TypedArrayObject &obj, uint32_t index, double d)
{
    MOZ_ASSERT(index < obj.length());

    switch (obj.type()) {
      case Scalar::Int8:
        Int8Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Uint8:
        Uint8Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Uint8Clamped:
        Uint8ClampedArray::setIndexValue(obj, index, d);
        return;
      case Scalar::Int16:
        Int16Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Uint16:
        Uint16Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Int32:
        Int32Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Uint32:
        Uint32Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Float32:
        Float32Array::setIndexValue(obj, index, d);
        return;
      case Scalar::Float64:
        Float64Array::setIndexValue(obj, index, d);
        return;
      case Scalar::TypeMax:
        break;
    }

    MOZ_CRASH("Unknown TypedArray type");
}









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
                                                              length);                          \
  }                                                                                             \
  JS_FRIEND_API(bool) JS_Is ## Name ## Array(JSObject *obj)                                     \
  {                                                                                             \
      if (!(obj = CheckedUnwrap(obj)))                                                          \
          return false;                                                                         \
      const Class *clasp = obj->getClass();                                                     \
      return clasp == TypedArrayObjectTemplate<NativeType>::instanceClass();                    \
  } \
  JS_FRIEND_API(JSObject *) js::Unwrap ## Name ## Array(JSObject *obj)                          \
  {                                                                                             \
      obj = CheckedUnwrap(obj);                                                                 \
      if (!obj)                                                                                 \
          return nullptr;                                                                       \
      const Class *clasp = obj->getClass();                                                     \
      if (clasp == TypedArrayObjectTemplate<NativeType>::instanceClass())                       \
          return obj;                                                                           \
      return nullptr;                                                                           \
  } \
  const js::Class* const js::detail::Name ## ArrayClassPtr =                                    \
      &js::TypedArrayObject::classes[TypedArrayObjectTemplate<NativeType>::ArrayTypeID()];

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
      if (clasp != TypedArrayObjectTemplate<InternalType>::instanceClass())                 \
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

#define TYPED_ARRAY_CLASS_SPEC(_typedArray)                                    \
{                                                                              \
    _typedArray::createConstructor,                                            \
    _typedArray::createPrototype,                                              \
    nullptr,                                                                   \
    nullptr,                                                                   \
    nullptr,                                                                   \
    _typedArray::finishClassInit,                                              \
    JSProto_TypedArray                                                         \
}

#define IMPL_TYPED_ARRAY_CLASS(_typedArray)                                    \
{                                                                              \
    #_typedArray,                                                              \
    JSCLASS_HAS_RESERVED_SLOTS(TypedArrayLayout::RESERVED_SLOTS) |             \
    JSCLASS_HAS_PRIVATE | JSCLASS_IMPLEMENTS_BARRIERS |                        \
    JSCLASS_HAS_CACHED_PROTO(JSProto_##_typedArray),                           \
    JS_PropertyStub,         /* addProperty */                                 \
    JS_DeletePropertyStub,   /* delProperty */                                 \
    JS_PropertyStub,         /* getProperty */                                 \
    JS_StrictPropertyStub,   /* setProperty */                                 \
    JS_EnumerateStub,                                                          \
    JS_ResolveStub,                                                            \
    JS_ConvertStub,                                                            \
    nullptr,                 /* finalize    */                                 \
    nullptr,                 /* call        */                                 \
    nullptr,                 /* hasInstance */                                 \
    nullptr,                 /* construct   */                                 \
    ArrayBufferViewObject::trace, /* trace  */                                 \
    TYPED_ARRAY_CLASS_SPEC(_typedArray),                                       \
    {                                                                          \
        nullptr,             /* outerObject */                                 \
        nullptr,             /* innerObject */                                 \
        nullptr,             /* iteratorObject */                              \
        false,               /* isWrappedNative */                             \
        nullptr,             /* weakmapKeyDelegateOp */                        \
        TypedArrayObject::ObjectMoved                                          \
    }                                                                          \
}

const Class TypedArrayObject::classes[Scalar::TypeMax] = {
    IMPL_TYPED_ARRAY_CLASS(Int8Array),
    IMPL_TYPED_ARRAY_CLASS(Uint8Array),
    IMPL_TYPED_ARRAY_CLASS(Int16Array),
    IMPL_TYPED_ARRAY_CLASS(Uint16Array),
    IMPL_TYPED_ARRAY_CLASS(Int32Array),
    IMPL_TYPED_ARRAY_CLASS(Uint32Array),
    IMPL_TYPED_ARRAY_CLASS(Float32Array),
    IMPL_TYPED_ARRAY_CLASS(Float64Array),
    IMPL_TYPED_ARRAY_CLASS(Uint8ClampedArray)
};








#define IMPL_TYPED_ARRAY_PROTO_CLASS(typedArray) \
{ \
    /*
     * Actually ({}).toString.call(Uint8Array.prototype) should throw, because
     * Uint8Array.prototype lacks the the typed array internal slots.  (Same as
     * with %TypedArray%.prototype.)  It's not clear this is desirable (see
     * above), but it's what we've always done, so keep doing it til we
     * implement @@toStringTag or ES6 changes.
     */ \
    #typedArray "Prototype", \
    JSCLASS_HAS_CACHED_PROTO(JSProto_##typedArray), \
    JS_PropertyStub,        /* addProperty */ \
    JS_DeletePropertyStub,  /* delProperty */ \
    JS_PropertyStub,        /* getProperty */ \
    JS_StrictPropertyStub,  /* setProperty */ \
    JS_EnumerateStub, \
    JS_ResolveStub, \
    JS_ConvertStub, \
    nullptr,                 /* finalize    */ \
    nullptr,                 /* call        */ \
    nullptr,                 /* hasInstance */ \
    nullptr,                 /* construct   */ \
    nullptr,                 /* trace  */ \
    { \
        typedArray::createConstructor, \
        typedArray::createPrototype, \
        nullptr, \
        nullptr, \
        nullptr, \
        nullptr, \
        JSProto_TypedArray \
    } \
}

const Class TypedArrayObject::protoClasses[Scalar::TypeMax] = {
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

JSObject *
js_InitArrayBufferClass(JSContext *cx, HandleObject obj)
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

    if (!GlobalObject::initBuiltinConstructor(cx, global, JSProto_ArrayBuffer,
                                              ctor, arrayBufferProto))
    {
        return nullptr;
    }

    if (!LinkConstructorAndPrototype(cx, ctor, arrayBufferProto))
        return nullptr;

    RootedId byteLengthId(cx, NameToId(cx->names().byteLength));
    unsigned attrs = JSPROP_SHARED | JSPROP_GETTER;
    JSObject *getter = NewFunction(cx, NullPtr(), ArrayBufferObject::byteLengthGetter, 0,
                                   JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return nullptr;

    if (!DefineNativeProperty(cx, arrayBufferProto, byteLengthId, UndefinedHandleValue,
                              JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr, attrs))
        return nullptr;

    if (!JS_DefineFunctions(cx, ctor, ArrayBufferObject::jsstaticfuncs))
        return nullptr;

    if (!JS_DefineFunctions(cx, arrayBufferProto, ArrayBufferObject::jsfuncs))
        return nullptr;

    return arrayBufferProto;
}

 bool
TypedArrayObject::isOriginalLengthGetter(Native native)
{
    return native == TypedArray_lengthGetter;
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
    unsigned attrs = JSPROP_SHARED | JSPROP_GETTER;

    Rooted<GlobalObject*> global(cx, cx->compartment()->maybeGlobal());
    JSObject *getter = NewFunction(cx, NullPtr(), DataViewObject::getter<ValueGetter>, 0,
                                   JSFunction::NATIVE_FUN, global, NullPtr());
    if (!getter)
        return false;

    return DefineNativeProperty(cx, proto, id, UndefinedHandleValue,
                                JS_DATA_TO_FUNC_PTR(PropertyOp, getter), nullptr, attrs);
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
    setSlot(TypedArrayLayout::LENGTH_SLOT, Int32Value(0));
    setSlot(TypedArrayLayout::BYTEOFFSET_SLOT, Int32Value(0));
    setPrivate(newData);
}

JSObject *
js_InitDataViewClass(JSContext *cx, HandleObject obj)
{
    if (!DataViewObject::initClass(cx))
        return nullptr;
    return &cx->global()->getPrototype(JSProto_DataView).toObject();
}

bool
js::IsTypedArrayConstructor(HandleValue v, uint32_t type)
{
    switch (type) {
      case Scalar::Int8:
        return IsNativeFunction(v, Int8Array::class_constructor);
      case Scalar::Uint8:
        return IsNativeFunction(v, Uint8Array::class_constructor);
      case Scalar::Int16:
        return IsNativeFunction(v, Int16Array::class_constructor);
      case Scalar::Uint16:
        return IsNativeFunction(v, Uint16Array::class_constructor);
      case Scalar::Int32:
        return IsNativeFunction(v, Int32Array::class_constructor);
      case Scalar::Uint32:
        return IsNativeFunction(v, Uint32Array::class_constructor);
      case Scalar::Float32:
        return IsNativeFunction(v, Float32Array::class_constructor);
      case Scalar::Float64:
        return IsNativeFunction(v, Float64Array::class_constructor);
      case Scalar::Uint8Clamped:
        return IsNativeFunction(v, Uint8ClampedArray::class_constructor);
      case Scalar::TypeMax:
        break;
    }
    MOZ_CRASH("unexpected typed array type");
}

template <typename CharT>
bool
js::StringIsTypedArrayIndex(const CharT *s, size_t length, uint64_t *indexp)
{
    const CharT *end = s + length;

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

template bool
js::StringIsTypedArrayIndex(const char16_t *s, size_t length, uint64_t *indexp);

template bool
js::StringIsTypedArrayIndex(const Latin1Char *s, size_t length, uint64_t *indexp);



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

JS_FRIEND_API(js::Scalar::Type)
JS_GetArrayBufferViewType(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return Scalar::TypeMax;

    if (obj->is<TypedArrayObject>())
        return obj->as<TypedArrayObject>().type();
    else if (obj->is<DataViewObject>())
        return Scalar::TypeMax;
    MOZ_CRASH("invalid ArrayBufferView type");
}

JS_FRIEND_API(int8_t *)
JS_GetInt8ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Int8);
    return static_cast<int8_t *>(tarr->viewData());
}

JS_FRIEND_API(uint8_t *)
JS_GetUint8ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Uint8);
    return static_cast<uint8_t *>(tarr->viewData());
}

JS_FRIEND_API(uint8_t *)
JS_GetUint8ClampedArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Uint8Clamped);
    return static_cast<uint8_t *>(tarr->viewData());
}

JS_FRIEND_API(int16_t *)
JS_GetInt16ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Int16);
    return static_cast<int16_t *>(tarr->viewData());
}

JS_FRIEND_API(uint16_t *)
JS_GetUint16ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Uint16);
    return static_cast<uint16_t *>(tarr->viewData());
}

JS_FRIEND_API(int32_t *)
JS_GetInt32ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Int32);
    return static_cast<int32_t *>(tarr->viewData());
}

JS_FRIEND_API(uint32_t *)
JS_GetUint32ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Uint32);
    return static_cast<uint32_t *>(tarr->viewData());
}

JS_FRIEND_API(float *)
JS_GetFloat32ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Float32);
    return static_cast<float *>(tarr->viewData());
}

JS_FRIEND_API(double *)
JS_GetFloat64ArrayData(JSObject *obj)
{
    obj = CheckedUnwrap(obj);
    if (!obj)
        return nullptr;
    TypedArrayObject *tarr = &obj->as<TypedArrayObject>();
    JS_ASSERT((int32_t) tarr->type() == Scalar::Float64);
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
