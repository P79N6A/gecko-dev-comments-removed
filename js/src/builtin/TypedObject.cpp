





#include "builtin/TypedObject.h"

#include "jscompartment.h"
#include "jsfun.h"
#include "jsobj.h"
#include "jsutil.h"

#include "builtin/TypeRepresentation.h"
#include "gc/Marking.h"
#include "js/Vector.h"
#include "vm/GlobalObject.h"
#include "vm/ObjectImpl.h"
#include "vm/String.h"
#include "vm/StringBuffer.h"
#include "vm/TypedArrayObject.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "vm/Shape-inl.h"

using mozilla::DebugOnly;

using namespace js;

const Class js::TypedObjectClass = {
    "TypedObject",
    JSCLASS_HAS_CACHED_PROTO(JSProto_TypedObject),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};














static bool Reify(JSContext *cx, TypeRepresentation *typeRepr, HandleObject type,
                  HandleObject owner, size_t offset, MutableHandleValue vp);






static bool ConvertAndCopyTo(JSContext *cx, TypeRepresentation *typeRepr,
                             HandleValue from, uint8_t *mem);

static void
ReportCannotConvertTo(JSContext *cx, HandleValue fromValue, const char *toType)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                         InformalValueTypeName(fromValue), toType);
}

static void
ReportCannotConvertTo(JSContext *cx, HandleObject fromObject, const char *toType)
{
    RootedValue fromValue(cx, ObjectValue(*fromObject));
    ReportCannotConvertTo(cx, fromValue, toType);
}

static void
ReportCannotConvertTo(JSContext *cx, HandleValue fromValue, HandleString toType)
{
    const char *fnName = JS_EncodeString(cx, toType);
    if (!fnName)
        return;
    ReportCannotConvertTo(cx, fromValue, fnName);
    JS_free(cx, (void *) fnName);
}

static void
ReportCannotConvertTo(JSContext *cx, HandleValue fromValue, TypeRepresentation *toType)
{
    StringBuffer contents(cx);
    if (!toType->appendString(cx, contents))
        return;

    RootedString result(cx, contents.finishString());
    if (!result)
        return;

    ReportCannotConvertTo(cx, fromValue, result);
}

static int32_t
Clamp(int32_t value, int32_t min, int32_t max)
{
    JS_ASSERT(min < max);
    if (value < min)
        return min;
    if (value > max)
        return max;
    return value;
}

static inline JSObject *
ToObjectIfObject(HandleValue value)
{
    if (!value.isObject())
        return nullptr;

    return &value.toObject();
}

static inline bool
IsNumericType(HandleObject type)
{
    return type &&
           &NumericTypeClasses[0] <= type->getClass() &&
           type->getClass() < &NumericTypeClasses[ScalarTypeRepresentation::TYPE_MAX];
}

static inline bool
IsArrayType(HandleObject type)
{
    return type && type->hasClass(&ArrayType::class_);
}

static inline bool
IsStructType(HandleObject type)
{
    return type && type->hasClass(&StructType::class_);
}

static inline bool
IsComplexType(HandleObject type)
{
    return IsArrayType(type) || IsStructType(type);
}

static inline bool
IsBinaryType(HandleObject type)
{
    return IsNumericType(type) || IsComplexType(type);
}

static inline bool
IsBlock(HandleObject obj)
{
    return obj && obj->hasClass(&BinaryBlock::class_);
}

static inline uint8_t *
BlockMem(HandleObject val)
{
    JS_ASSERT(IsBlock(val));
    return (uint8_t*) val->getPrivate();
}





static JSObject *
typeRepresentationOwnerObj(HandleObject typeObj)
{
    JS_ASSERT(IsBinaryType(typeObj));
    return &typeObj->getFixedSlot(SLOT_TYPE_REPR).toObject();
}







static TypeRepresentation *
typeRepresentation(HandleObject typeObj)
{
    return TypeRepresentation::fromOwnerObject(typeRepresentationOwnerObj(typeObj));
}

static inline JSObject *
GetType(HandleObject block)
{
    JS_ASSERT(IsBlock(block));
    return &block->getFixedSlot(SLOT_DATATYPE).toObject();
}




static bool
ConvertAndCopyTo(JSContext *cx, HandleValue val, HandleObject block)
{
    uint8_t *memory = BlockMem(block);
    RootedObject type(cx, GetType(block));
    TypeRepresentation *typeRepr = typeRepresentation(type);
    return ConvertAndCopyTo(cx, typeRepr, val, memory);
}

static inline size_t
TypeSize(HandleObject type)
{
    return typeRepresentation(type)->size();
}

static inline size_t
BlockSize(JSContext *cx, HandleObject val)
{
    RootedObject type(cx, GetType(val));
    return TypeSize(type);
}

static inline bool
IsBlockOfKind(JSContext *cx, HandleObject obj, TypeRepresentation::Kind kind)
{
    if (!IsBlock(obj))
        return false;
    RootedObject objType(cx, GetType(obj));
    TypeRepresentation *repr = typeRepresentation(objType);
    return repr->kind() == kind;
}

static inline bool
IsBinaryArray(JSContext *cx, HandleObject obj)
{
    return IsBlockOfKind(cx, obj, TypeRepresentation::Array);
}

static inline bool
IsBinaryStruct(JSContext *cx, HandleObject obj)
{
    return IsBlockOfKind(cx, obj, TypeRepresentation::Struct);
}

static bool
TypeEquivalent(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject thisObj(cx, ToObjectIfObject(args.thisv()));
    if (!thisObj || !IsBinaryType(thisObj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_INCOMPATIBLE_PROTO,
                             "Type", "equivalent",
                             InformalValueTypeName(args.thisv()));
        return false;
    }

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "Type.equivalent", "1", "s");
        return false;
    }

    RootedObject otherObj(cx, ToObjectIfObject(args[0]));
    if (!otherObj || !IsBinaryType(otherObj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_TYPEDOBJECT_NOT_TYPE_OBJECT);
        return false;
    }

    TypeRepresentation *thisRepr = typeRepresentation(thisObj);
    TypeRepresentation *otherRepr = typeRepresentation(otherObj);
    args.rval().setBoolean(thisRepr == otherRepr);
    return true;
}

#define BINARYDATA_NUMERIC_CLASSES(constant_, type_, name_)                   \
{                                                                             \
    #name_,                                                                   \
    JSCLASS_HAS_RESERVED_SLOTS(TYPE_RESERVED_SLOTS),                          \
    JS_PropertyStub,       /* addProperty */                                  \
    JS_DeletePropertyStub, /* delProperty */                                  \
    JS_PropertyStub,       /* getProperty */                                  \
    JS_StrictPropertyStub, /* setProperty */                                  \
    JS_EnumerateStub,                                                         \
    JS_ResolveStub,                                                           \
    JS_ConvertStub,                                                           \
    nullptr,                                                                  \
    nullptr,                                                                  \
    NumericType<constant_, type_>::call,                                      \
    nullptr,                                                                  \
    nullptr,                                                                  \
    nullptr                                                                   \
},

const Class js::NumericTypeClasses[ScalarTypeRepresentation::TYPE_MAX] = {
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_NUMERIC_CLASSES)
};

#define BINARYDATA_TYPE_TO_CLASS(constant_, type_, name_)                     \
    template <>                                                               \
    const Class *                                                             \
    NumericType<constant_, type_>::typeToClass()                              \
    {                                                                         \
        return &NumericTypeClasses[constant_];                                \
    }






namespace js {
JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_TYPE_TO_CLASS);

template <ScalarTypeRepresentation::Type type, typename T>
JS_ALWAYS_INLINE
bool NumericType<type, T>::reify(JSContext *cx, void *mem, MutableHandleValue vp)
{
    vp.setNumber((double)*((T*)mem) );
    return true;
}

template <ScalarTypeRepresentation::Type type, typename T>
bool
NumericType<type, T>::convert(JSContext *cx, HandleValue val, T* converted)
{
    if (val.isInt32()) {
        *converted = T(val.toInt32());
        return true;
    }

    double d;
    if (!ToDoubleForTypedArray(cx, val, &d))
        return false;

    if (TypeIsFloatingPoint<T>()) {
        *converted = T(d);
    } else if (TypeIsUnsigned<T>()) {
        uint32_t n = ToUint32(d);
        *converted = T(n);
    } else {
        int32_t n = ToInt32(d);
        *converted = T(n);
    }

    return true;
}

template <>
bool
NumericType<ScalarTypeRepresentation::TYPE_UINT8_CLAMPED, uint8_t>::convert(
    JSContext *cx, HandleValue val, uint8_t* converted)
{
    double d;
    if (val.isInt32()) {
        d = val.toInt32();
    } else {
        if (!ToDoubleForTypedArray(cx, val, &d))
            return false;
    }
    *converted = ClampDoubleToUint8(d);
    return true;
}

} 

template<ScalarTypeRepresentation::Type N>
static bool
NumericTypeToString(JSContext *cx, unsigned int argc, Value *vp)
{
    static_assert(N < ScalarTypeRepresentation::TYPE_MAX, "bad numeric type");
    CallArgs args = CallArgsFromVp(argc, vp);
    JSString *s = JS_NewStringCopyZ(cx, ScalarTypeRepresentation::typeName(N));
    if (!s)
        return false;
    args.rval().setString(s);
    return true;
}

static bool
ConvertAndCopyScalarTo(JSContext *cx, ScalarTypeRepresentation *typeRepr,
                       HandleValue from, uint8_t *mem)
{
#define CONVERT_CASES(constant_, type_, name_)                                \
    case constant_: {                                                         \
        type_ temp;                                                           \
        if (!NumericType<constant_, type_>::convert(cx, from, &temp))         \
            return false;                                                     \
        memcpy(mem, &temp, sizeof(type_));                                    \
        return true; }

    switch (typeRepr->type()) {
        JS_FOR_EACH_SCALAR_TYPE_REPR(CONVERT_CASES)
    }
#undef CONVERT_CASES
    return false;
}

static bool
ReifyScalar(JSContext *cx, ScalarTypeRepresentation *typeRepr, HandleObject type,
            HandleObject owner, size_t offset, MutableHandleValue to)
{
    JS_ASSERT(&NumericTypeClasses[0] <= type->getClass() &&
              type->getClass() <= &NumericTypeClasses[ScalarTypeRepresentation::TYPE_MAX]);

    switch (typeRepr->type()) {
#define REIFY_CASES(constant_, type_, name_)                                  \
        case constant_:                                                       \
          return NumericType<constant_, type_>::reify(                        \
                cx, BlockMem(owner) + offset, to);
        JS_FOR_EACH_SCALAR_TYPE_REPR(REIFY_CASES)
    }
#undef REIFY_CASES

    MOZ_ASSUME_UNREACHABLE("Invalid type");
    return false;
}

template <ScalarTypeRepresentation::Type type, typename T>
bool
NumericType<type, T>::call(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             args.callee().getClass()->name, "0", "s");
        return false;
    }

    RootedValue arg(cx, args[0]);
    T answer;
    if (!convert(cx, arg, &answer))
        return false; 

    RootedValue reified(cx);
    if (!NumericType<type, T>::reify(cx, &answer, &reified)) {
        return false;
    }

    args.rval().set(reified);
    return true;
}




































static JSObject *
CreateComplexTypeInstancePrototype(JSContext *cx,
                                   HandleObject typeObjectCtor)
{
    RootedValue ctorPrototypeVal(cx);
    if (!JSObject::getProperty(cx, typeObjectCtor, typeObjectCtor,
                               cx->names().prototype,
                               &ctorPrototypeVal))
    {
        return nullptr;
    }
    JS_ASSERT(ctorPrototypeVal.isObject()); 
    RootedObject ctorPrototypeObj(cx, &ctorPrototypeVal.toObject());

    RootedValue ctorPrototypePrototypeVal(cx);
    if (!JSObject::getProperty(cx, ctorPrototypeObj, ctorPrototypeObj,
                               cx->names().prototype,
                               &ctorPrototypePrototypeVal))
    {
        return nullptr;
    }

    JS_ASSERT(ctorPrototypePrototypeVal.isObject()); 
    RootedObject proto(cx, &ctorPrototypePrototypeVal.toObject());
    return NewObjectWithGivenProto(cx, &JSObject::class_, proto,
                                   &typeObjectCtor->global());
}

template<typename T>
static JSObject *
CreateMetaTypeObject(JSContext *cx,
                     Handle<GlobalObject*> global)
{
    RootedAtom className(cx, Atomize(cx, T::class_.name,
                                     strlen(T::class_.name)));
    if (!className)
        return nullptr;

    RootedObject funcProto(cx, global->getOrCreateFunctionPrototype(cx));
    if (!funcProto)
        return nullptr;

    

    RootedObject proto(
        cx, NewObjectWithGivenProto(cx, &JSObject::class_, funcProto,
                                    global, SingletonObject));
    if (!proto)
        return nullptr;

    

    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return nullptr;
    RootedObject protoProto(
        cx, NewObjectWithGivenProto(cx, &JSObject::class_, objProto,
                                    global, SingletonObject));
    if (!proto)
        return nullptr;

    RootedValue protoProtoValue(cx, ObjectValue(*protoProto));
    if (!JSObject::defineProperty(cx, proto, cx->names().prototype,
                                  protoProtoValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    

    RootedFunction ctor(
        cx, global->createConstructor(cx, T::construct, className, 2));
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndBrand(cx, proto,
                                  T::typeObjectProperties,
                                  T::typeObjectMethods) ||
        !DefinePropertiesAndBrand(cx, protoProto,
                                  T::typedObjectProperties,
                                  T::typedObjectMethods))
    {
        return nullptr;
    }

    return ctor;
}

const Class ArrayType::class_ = {
    "ArrayType",
    JSCLASS_HAS_RESERVED_SLOTS(ARRAY_TYPE_RESERVED_SLOTS),
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
    BinaryBlock::construct,
    nullptr
};

const JSPropertySpec ArrayType::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayType::typeObjectMethods[] = {
    JS_FN("repeat", ArrayType::repeat, 1, 0),
    JS_FN("toSource", ArrayType::toSource, 0, 0),
    JS_FS_END
};

const JSPropertySpec ArrayType::typedObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayType::typedObjectMethods[] = {
    JS_FN("subarray", ArrayType::subarray, 2, 0),
    {"forEach", {nullptr, nullptr}, 1, 0, "ArrayForEach"},
    JS_FS_END
};

static JSObject *
ArrayElementType(HandleObject array)
{
    JS_ASSERT(IsArrayType(array));
    return &array->getFixedSlot(SLOT_ARRAY_ELEM_TYPE).toObject();
}

static bool
ConvertAndCopyArrayTo(JSContext *cx, ArrayTypeRepresentation *typeRepr,
                      HandleValue from, uint8_t *mem)
{
    if (!from.isObject()) {
        ReportCannotConvertTo(cx, from, typeRepr);
        return false;
    }

    
    RootedObject fromObject(cx, &from.toObject());
    if (IsBlock(fromObject)) {
        RootedObject fromType(cx, GetType(fromObject));
        TypeRepresentation *fromTypeRepr = typeRepresentation(fromType);
        if (typeRepr == fromTypeRepr) {
            memcpy(mem, BlockMem(fromObject), typeRepr->size());
            return true;
        }
    }

    
    RootedValue fromLenVal(cx);
    if (!JSObject::getProperty(cx, fromObject, fromObject,
                               cx->names().length, &fromLenVal))
        return false;

    if (!fromLenVal.isInt32()) {
        ReportCannotConvertTo(cx, from, typeRepr);
        return false;
    }

    int32_t fromLenInt32 = fromLenVal.toInt32();
    if (fromLenInt32 < 0) {
        ReportCannotConvertTo(cx, from, typeRepr);
        return false;
    }

    size_t fromLen = (size_t) fromLenInt32;
    if (typeRepr->length() != fromLen) {
        ReportCannotConvertTo(cx, from, typeRepr);
        return false;
    }

    TypeRepresentation *elementType = typeRepr->element();
    uint8_t *p = mem;

    RootedValue fromElem(cx);
    for (size_t i = 0; i < fromLen; i++) {
        if (!JSObject::getElement(cx, fromObject, fromObject, i, &fromElem))
            return false;

        if (!ConvertAndCopyTo(cx, elementType, fromElem, p))
            return false;

        p += elementType->size();
    }

    return true;
}

static bool
FillBinaryArrayWithValue(JSContext *cx, HandleObject array, HandleValue val)
{
    JS_ASSERT(IsBinaryArray(cx, array));

    
    RootedObject type(cx, GetType(array));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(type)->asArray();
    uint8_t *base = BlockMem(array);
    if (!ConvertAndCopyTo(cx, typeRepr->element(), val, base))
        return false;

    
    size_t elementSize = typeRepr->element()->size();
    for (size_t i = 1; i < typeRepr->length(); i++) {
        uint8_t *dest = base + elementSize * i;
        memcpy(dest, base, elementSize);
    }

    return true;
}

bool
ArrayType::repeat(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "repeat()", "0", "s");
        return false;
    }

    RootedObject thisObj(cx, ToObjectIfObject(args.thisv()));
    if (!thisObj || !IsArrayType(thisObj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_INCOMPATIBLE_PROTO,
                             "ArrayType", "repeat",
                             InformalValueTypeName(args.thisv()));
        return false;
    }

    RootedObject binaryArray(cx, BinaryBlock::createZeroed(cx, thisObj));
    if (!binaryArray)
        return false;

    RootedValue val(cx, args[0]);
    if (!FillBinaryArrayWithValue(cx, binaryArray, val))
        return false;

    args.rval().setObject(*binaryArray);
    return true;
}

bool
ArrayType::toSource(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject thisObj(cx, ToObjectIfObject(args.thisv()));
    if (!thisObj || !IsArrayType(thisObj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "ArrayType", "toSource",
                             InformalValueTypeName(args.thisv()));
        return false;
    }

    StringBuffer contents(cx);
    if (!typeRepresentation(thisObj)->appendString(cx, contents))
        return false;

    RootedString result(cx, contents.finishString());
    if (!result)
        return false;

    args.rval().setString(result);
    return true;
}





















 bool
ArrayType::subarray(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "subarray()", "0", "s");
        return false;
    }

    if (!args[0].isInt32()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_SUBARRAY_INTEGER_ARG, "1");
        return false;
    }

    RootedObject thisObj(cx, &args.thisv().toObject());
    if (!IsBinaryArray(cx, thisObj)) {
        ReportCannotConvertTo(cx, thisObj, "binary array");
        return false;
    }

    RootedObject type(cx, GetType(thisObj));
    ArrayTypeRepresentation *typeRepr = typeRepresentation(type)->asArray();
    size_t length = typeRepr->length();

    int32_t begin = args[0].toInt32();
    int32_t end = length;

    if (args.length() >= 2) {
        if (!args[1].isInt32()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                    JSMSG_TYPEDOBJECT_SUBARRAY_INTEGER_ARG, "2");
            return false;
        }

        end = args[1].toInt32();
    }

    if (begin < 0)
        begin = length + begin;
    if (end < 0)
        end = length + end;

    begin = Clamp(begin, 0, length);
    end = Clamp(end, 0, length);

    int32_t sublength = end - begin; 
    sublength = Clamp(sublength, 0, length);

    RootedObject globalObj(cx, cx->compartment()->maybeGlobal());
    JS_ASSERT(globalObj);
    Rooted<GlobalObject*> global(cx, &globalObj->as<GlobalObject>());
    RootedObject arrayTypeGlobal(cx, global->getArrayType(cx));

    RootedObject elementType(cx, ArrayElementType(type));
    RootedObject subArrayType(cx, ArrayType::create(cx, arrayTypeGlobal,
                                                    elementType, sublength));
    if (!subArrayType)
        return false;

    int32_t elementSize = typeRepr->element()->size();
    size_t offset = elementSize * begin;

    RootedObject subarray(cx, BinaryBlock::createDerived(cx, subArrayType, thisObj, offset));
    if (!subarray)
        return false;

    args.rval().setObject(*subarray);
    return true;
}

static bool
ArrayFillSubarray(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "fill()", "0", "s");
        return false;
    }

    RootedObject thisObj(cx, ToObjectIfObject(args.thisv()));
    if (!thisObj || !IsBinaryArray(cx, thisObj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             nullptr, JSMSG_INCOMPATIBLE_PROTO,
                             "ArrayType", "fill",
                             InformalValueTypeName(args.thisv()));
        return false;
    }

    Value funArrayTypeVal = GetFunctionNativeReserved(&args.callee(), 0);
    JS_ASSERT(funArrayTypeVal.isObject());

    RootedObject type(cx, GetType(thisObj));
    TypeRepresentation *typeRepr = typeRepresentation(type);
    RootedObject funArrayType(cx, &funArrayTypeVal.toObject());
    TypeRepresentation *funArrayTypeRepr = typeRepresentation(funArrayType);
    if (typeRepr != funArrayTypeRepr) {
        RootedValue thisObjValue(cx, ObjectValue(*thisObj));
        ReportCannotConvertTo(cx, thisObjValue, funArrayTypeRepr);
        return false;
    }

    args.rval().setUndefined();
    RootedValue val(cx, args[0]);
    return FillBinaryArrayWithValue(cx, thisObj, val);
}

static bool
InitializeCommonTypeDescriptorProperties(JSContext *cx,
                                         HandleObject obj,
                                         HandleObject typeReprOwnerObj)
{
    TypeRepresentation *typeRepr =
        TypeRepresentation::fromOwnerObject(typeReprOwnerObj);

    
    if (!JS_DefineFunction(cx, obj, "equivalent",
                           TypeEquivalent, 1, 0))
    {
        return false;
    }

    
    RootedValue typeByteLength(cx, NumberValue(typeRepr->size()));
    if (!JSObject::defineProperty(cx, obj, cx->names().byteLength,
                                  typeByteLength,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return false;
    }

    
    RootedValue typeByteAlignment(cx, NumberValue(typeRepr->alignment()));
    if (!JSObject::defineProperty(cx, obj, cx->names().byteAlignment,
                                  typeByteAlignment,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return false;
    }

    
    RootedValue variable(cx, JSVAL_FALSE);
    if (!JSObject::defineProperty(cx, obj, cx->names().variable,
                                  variable,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return false;
    }

    return true;
}

JSObject *
ArrayType::create(JSContext *cx,
                  HandleObject metaTypeObject,
                  HandleObject elementType,
                  size_t length)
{
    JS_ASSERT(elementType);
    JS_ASSERT(IsBinaryType(elementType));

    TypeRepresentation *elementTypeRepr = typeRepresentation(elementType);
    RootedObject typeReprObj(
        cx, ArrayTypeRepresentation::Create(cx, elementTypeRepr, length));
    if (!typeReprObj)
        return nullptr;

    RootedValue prototypeVal(cx);
    if (!JSObject::getProperty(cx, metaTypeObject, metaTypeObject,
                               cx->names().prototype,
                               &prototypeVal))
    {
        return nullptr;
    }
    JS_ASSERT(prototypeVal.isObject()); 

    RootedObject obj(
        cx, NewObjectWithClassProto(cx, &ArrayType::class_,
                                    &prototypeVal.toObject(), cx->global()));
    if (!obj)
        return nullptr;
    obj->initFixedSlot(SLOT_TYPE_REPR, ObjectValue(*typeReprObj));

    RootedValue elementTypeVal(cx, ObjectValue(*elementType));
    if (!JSObject::defineProperty(cx, obj, cx->names().elementType,
                                  elementTypeVal, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    obj->initFixedSlot(SLOT_ARRAY_ELEM_TYPE, elementTypeVal);

    RootedValue lengthVal(cx, Int32Value(length));
    if (!JSObject::defineProperty(cx, obj, cx->names().length,
                                  lengthVal, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    if (!InitializeCommonTypeDescriptorProperties(cx, obj, typeReprObj))
        return nullptr;

    RootedObject prototypeObj(
        cx, CreateComplexTypeInstancePrototype(cx, metaTypeObject));
    if (!prototypeObj)
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, obj, prototypeObj))
        return nullptr;

    JSFunction *fillFun = DefineFunctionWithReserved(cx, prototypeObj, "fill", ArrayFillSubarray, 1, 0);
    if (!fillFun)
        return nullptr;

    
    
    
    SetFunctionNativeReserved(fillFun, 0, ObjectValue(*obj));

    return obj;
}

bool
ArrayType::construct(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_NOT_FUNCTION, "ArrayType");
        return false;
    }

    if (args.length() != 2 ||
        !args[0].isObject() ||
        !args[1].isNumber() ||
        args[1].toNumber() < 0)
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    RootedObject arrayTypeGlobal(cx, &args.callee());
    RootedObject elementType(cx, &args[0].toObject());

    if (!IsBinaryType(elementType)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    if (!args[1].isInt32()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    int32_t length = args[1].toInt32();
    if (length < 0) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    RootedObject obj(cx, create(cx, arrayTypeGlobal, elementType, length));
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}





const Class StructType::class_ = {
    "StructType",
    JSCLASS_HAS_RESERVED_SLOTS(STRUCT_TYPE_RESERVED_SLOTS) |
    JSCLASS_HAS_PRIVATE, 
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
    BinaryBlock::construct,
    nullptr  
};

const JSPropertySpec StructType::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec StructType::typeObjectMethods[] = {
    JS_FN("toSource", StructType::toSource, 0, 0),
    JS_FS_END
};

const JSPropertySpec StructType::typedObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec StructType::typedObjectMethods[] = {
    JS_FS_END
};

static bool
ConvertAndCopyStructTo(JSContext *cx,
                       StructTypeRepresentation *typeRepr,
                       HandleValue from,
                       uint8_t *mem)
{
    if (!from.isObject()) {
        ReportCannotConvertTo(cx, from, typeRepr);
        return false;
    }

    
    RootedObject fromObject(cx, &from.toObject());
    if (IsBlock(fromObject)) {
        RootedObject fromType(cx, GetType(fromObject));
        TypeRepresentation *fromTypeRepr = typeRepresentation(fromType);
        if (typeRepr == fromTypeRepr) {
            memcpy(mem, BlockMem(fromObject), typeRepr->size());
            return true;
        }
    }

    
    RootedId fieldId(cx);
    RootedValue fromProp(cx);
    for (size_t i = 0; i < typeRepr->fieldCount(); i++) {
        const StructField &field = typeRepr->field(i);

        fieldId = field.id;
        if (!JSObject::getGeneric(cx, fromObject, fromObject, fieldId, &fromProp))
            return false;

        if (!ConvertAndCopyTo(cx, field.typeRepr, fromProp,
                              mem + field.offset))
            return false;
    }
    return true;
}






bool
StructType::layout(JSContext *cx, HandleObject structType, HandleObject fields)
{
    AutoIdVector ids(cx);
    if (!GetPropertyNames(cx, fields, JSITER_OWNONLY, &ids))
        return false;

    AutoValueVector fieldTypeObjs(cx);
    AutoObjectVector fieldTypeReprObjs(cx);

    RootedValue fieldTypeVal(cx);
    RootedId id(cx);
    for (unsigned int i = 0; i < ids.length(); i++) {
        id = ids[i];

        if (!JSObject::getGeneric(cx, fields, fields, id, &fieldTypeVal))
            return false;

        uint32_t index;
        if (js_IdIsIndex(id, &index)) {
            RootedValue idValue(cx, IdToJsval(id));
            ReportCannotConvertTo(cx, idValue, "StructType field name");
            return false;
        }

        RootedObject fieldType(cx, ToObjectIfObject(fieldTypeVal));
        if (!fieldType || !IsBinaryType(fieldType)) {
            ReportCannotConvertTo(cx, fieldTypeVal, "StructType field specifier");
            return false;
        }

        if (!fieldTypeObjs.append(fieldTypeVal)) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        if (!fieldTypeReprObjs.append(typeRepresentationOwnerObj(fieldType))) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    
    RootedObject typeReprObj(
        cx, StructTypeRepresentation::Create(cx, ids, fieldTypeReprObjs));
    if (!typeReprObj)
        return false;
    StructTypeRepresentation *typeRepr =
        TypeRepresentation::fromOwnerObject(typeReprObj)->asStruct();
    structType->initReservedSlot(SLOT_TYPE_REPR, ObjectValue(*typeReprObj));

    
    RootedObject fieldTypeVec(
        cx, NewDenseCopiedArray(cx, fieldTypeObjs.length(),
                                fieldTypeObjs.begin()));
    if (!fieldTypeVec)
        return false;

    structType->initFixedSlot(SLOT_STRUCT_FIELD_TYPES,
                             ObjectValue(*fieldTypeVec));

    
    AutoValueVector fieldNameValues(cx);
    for (unsigned int i = 0; i < ids.length(); i++) {
        RootedValue value(cx, IdToValue(ids[i]));
        if (!fieldNameValues.append(value))
            return false;
    }
    RootedObject fieldNamesVec(
        cx, NewDenseCopiedArray(cx, fieldNameValues.length(),
                                fieldNameValues.begin()));
    if (!fieldNamesVec)
        return false;
    RootedValue fieldNamesVecValue(cx, ObjectValue(*fieldNamesVec));
    if (!JSObject::defineProperty(cx, structType, cx->names().fieldNames,
                                  fieldNamesVecValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    
    
    
    
    RootedObject fieldOffsets(
        cx,
        NewObjectWithClassProto(cx, &JSObject::class_, nullptr, nullptr));
    RootedObject fieldTypes(
        cx,
        NewObjectWithClassProto(cx, &JSObject::class_, nullptr, nullptr));
    for (size_t i = 0; i < typeRepr->fieldCount(); i++) {
        const StructField &field = typeRepr->field(i);
        RootedId fieldId(cx, field.id);

        
        RootedValue offset(cx, NumberValue(field.offset));
        if (!JSObject::defineGeneric(cx, fieldOffsets, fieldId,
                                     offset, nullptr, nullptr,
                                     JSPROP_READONLY | JSPROP_PERMANENT))
            return false;

        
        if (!JSObject::defineGeneric(cx, fieldTypes, fieldId,
                                     fieldTypeObjs.handleAt(i), nullptr, nullptr,
                                     JSPROP_READONLY | JSPROP_PERMANENT))
            return false;
    }

    RootedValue fieldOffsetsValue(cx, ObjectValue(*fieldOffsets));
    if (!JSObject::defineProperty(cx, structType, cx->names().fieldOffsets,
                                  fieldOffsetsValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    RootedValue fieldTypesValue(cx, ObjectValue(*fieldTypes));
    if (!JSObject::defineProperty(cx, structType, cx->names().fieldTypes,
                                  fieldTypesValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    return true;
}

JSObject *
StructType::create(JSContext *cx, HandleObject metaTypeObject,
                   HandleObject fields)
{
    RootedValue prototypeVal(cx);
    if (!JSObject::getProperty(cx, metaTypeObject, metaTypeObject,
                               cx->names().prototype,
                               &prototypeVal))
        return nullptr;
    JS_ASSERT(prototypeVal.isObject()); 

    RootedObject obj(
        cx, NewObjectWithClassProto(cx, &StructType::class_,
                                    &prototypeVal.toObject(), cx->global()));
    if (!obj)
        return nullptr;

    if (!StructType::layout(cx, obj, fields))
        return nullptr;

    RootedObject fieldsProto(cx);
    if (!JSObject::getProto(cx, fields, &fieldsProto))
        return nullptr;

    RootedObject typeReprObj(cx, typeRepresentationOwnerObj(obj));
    if (!InitializeCommonTypeDescriptorProperties(cx, obj, typeReprObj))
        return nullptr;

    RootedObject prototypeObj(
        cx, CreateComplexTypeInstancePrototype(cx, metaTypeObject));
    if (!prototypeObj)
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, obj, prototypeObj))
        return nullptr;

    return obj;
}

bool
StructType::construct(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_NOT_FUNCTION, "StructType");
        return false;
    }

    if (args.length() >= 1 && args[0].isObject()) {
        RootedObject metaTypeObject(cx, &args.callee());
        RootedObject fields(cx, &args[0].toObject());
        RootedObject obj(cx, create(cx, metaTypeObject, fields));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                         JSMSG_TYPEDOBJECT_STRUCTTYPE_BAD_ARGS);
    return false;
}

bool
StructType::toSource(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject thisObj(cx, ToObjectIfObject(args.thisv()));
    if (!thisObj || !IsStructType(thisObj)) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_INCOMPATIBLE_PROTO,
                             "StructType", "toSource",
                             InformalValueTypeName(args.thisv()));
        return false;
    }

    StringBuffer contents(cx);
    if (!typeRepresentation(thisObj)->appendString(cx, contents))
        return false;

    RootedString result(cx, contents.finishString());
    if (!result)
        return false;

    args.rval().setString(result);
    return true;
}

static bool
ConvertAndCopyTo(JSContext *cx,
                 TypeRepresentation *typeRepr,
                 HandleValue from,
                 uint8_t *mem)
{
    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        return ConvertAndCopyScalarTo(cx, typeRepr->asScalar(), from, mem);

      case TypeRepresentation::Array:
        return ConvertAndCopyArrayTo(cx, typeRepr->asArray(), from, mem);

      case TypeRepresentation::Struct:
        return ConvertAndCopyStructTo(cx, typeRepr->asStruct(), from, mem);
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
    return false;
}



static bool
ReifyComplex(JSContext *cx, HandleObject type, HandleObject owner,
             size_t offset, MutableHandleValue to)
{
    RootedObject obj(cx, BinaryBlock::createDerived(cx, type, owner, offset));
    if (!obj)
        return false;
    to.setObject(*obj);
    return true;
}

static bool
Reify(JSContext *cx, TypeRepresentation *typeRepr, HandleObject type,
      HandleObject owner, size_t offset, MutableHandleValue to)
{
    JS_ASSERT(typeRepr == typeRepresentation(type));
    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        return ReifyScalar(cx, typeRepr->asScalar(), type, owner, offset, to);
      case TypeRepresentation::Struct:
      case TypeRepresentation::Array:
        return ReifyComplex(cx, type, owner, offset, to);
    }
    MOZ_ASSUME_UNREACHABLE("Invalid typeRepr kind");
}




















































template<ScalarTypeRepresentation::Type type>
static bool
DefineNumericClass(JSContext *cx, Handle<GlobalObject*> global,
                   HandleObject module, HandlePropertyName className);

JSObject *
js_InitTypedObjectClass(JSContext *cx, HandleObject obj)
{
    







    JS_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());

    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return nullptr;

    RootedObject module(cx, NewObjectWithClassProto(cx, &JSObject::class_,
                                                    objProto, global));

    

    RootedValue moduleValue(cx, ObjectValue(*module));

    

#define BINARYDATA_NUMERIC_DEFINE(constant_, type_, name_)                      \
    if (!DefineNumericClass<constant_>(cx, global, module, cx->names().name_))  \
        return nullptr;
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_NUMERIC_DEFINE)
#undef BINARYDATA_NUMERIC_DEFINE

    

    RootedObject arrayType(cx, CreateMetaTypeObject<ArrayType>(cx, global));
    if (!arrayType)
        return nullptr;

    RootedValue arrayTypeValue(cx, ObjectValue(*arrayType));
    if (!JSObject::defineProperty(cx, module, cx->names().ArrayType,
                                  arrayTypeValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    

    RootedObject structType(cx, CreateMetaTypeObject<StructType>(cx, global));
    if (!structType)
        return nullptr;

    RootedValue structTypeValue(cx, ObjectValue(*structType));
    if (!JSObject::defineProperty(cx, module, cx->names().StructType,
                                  structTypeValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    
    if (!JSObject::defineProperty(cx, global, cx->names().TypedObject,
                                  moduleValue,
                                  nullptr, nullptr,
                                  0))
        return nullptr;
    global->setReservedSlot(JSProto_TypedObject, moduleValue);
    global->setArrayType(arrayType);
    global->markStandardClassInitializedNoProto(&TypedObjectClass);

    return module;
}

JSObject *
js_InitTypedObjectDummy(JSContext *cx, HandleObject obj)
{
    





    MOZ_ASSUME_UNREACHABLE("shouldn't be initializing TypedObject via the JSProtoKey initializer mechanism");
}

template<ScalarTypeRepresentation::Type type>
static bool
DefineNumericClass(JSContext *cx,
                   Handle<GlobalObject*> global,
                   HandleObject module,
                   HandlePropertyName className)
{
    RootedObject funcProto(cx, global->getOrCreateFunctionPrototype(cx));
    if (!funcProto)
        return false;

    RootedObject numFun(cx, NewObjectWithClassProto(cx, &NumericTypeClasses[type],
                                                    funcProto, global));
    if (!numFun)
        return false;

    RootedObject typeReprObj(cx, ScalarTypeRepresentation::Create(cx, type));
    if (!typeReprObj)
        return false;

    numFun->initFixedSlot(SLOT_TYPE_REPR, ObjectValue(*typeReprObj));

    if (!InitializeCommonTypeDescriptorProperties(cx, numFun, typeReprObj))
        return false;

    if (!JS_DefineFunction(cx, numFun, "toString",
                           NumericTypeToString<type>, 0, 0))
        return false;

    if (!JS_DefineFunction(cx, numFun, "toSource",
                           NumericTypeToString<type>, 0, 0))
    {
        return false;
    }

    RootedValue numFunValue(cx, ObjectValue(*numFun));
    if (!JSObject::defineProperty(cx, module, className,
                                  numFunValue, nullptr, nullptr, 0))
        return false;

    return true;
}




const Class BinaryBlock::class_ = {
    "BinaryBlock",
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(BLOCK_RESERVED_SLOTS) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_IMPLEMENTS_BARRIERS,
    JS_PropertyStub,
    JS_DeletePropertyStub,
    JS_PropertyStub,
    JS_StrictPropertyStub,
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    BinaryBlock::obj_finalize,
    nullptr,        
    nullptr,        
    nullptr,        
    nullptr,        
    BinaryBlock::obj_trace,
    JS_NULL_CLASS_EXT,
    {
        BinaryBlock::obj_lookupGeneric,
        BinaryBlock::obj_lookupProperty,
        BinaryBlock::obj_lookupElement,
        BinaryBlock::obj_lookupSpecial,
        BinaryBlock::obj_defineGeneric,
        BinaryBlock::obj_defineProperty,
        BinaryBlock::obj_defineElement,
        BinaryBlock::obj_defineSpecial,
        BinaryBlock::obj_getGeneric,
        BinaryBlock::obj_getProperty,
        BinaryBlock::obj_getElement,
        BinaryBlock::obj_getElementIfPresent,
        BinaryBlock::obj_getSpecial,
        BinaryBlock::obj_setGeneric,
        BinaryBlock::obj_setProperty,
        BinaryBlock::obj_setElement,
        BinaryBlock::obj_setSpecial,
        BinaryBlock::obj_getGenericAttributes,
        BinaryBlock::obj_setGenericAttributes,
        BinaryBlock::obj_deleteProperty,
        BinaryBlock::obj_deleteElement,
        BinaryBlock::obj_deleteSpecial,
        BinaryBlock::obj_enumerate,
        nullptr, 
    }
};

static bool
ReportBlockTypeError(JSContext *cx,
                     const unsigned errorNumber,
                     HandleObject obj)
{
    JS_ASSERT(IsBlock(obj));

    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    StringBuffer contents(cx);
    if (!typeRepr->appendString(cx, contents))
        return false;

    RootedString result(cx, contents.finishString());
    if (!result)
        return false;

    char *typeReprStr = JS_EncodeString(cx, result.get());
    if (!typeReprStr)
        return false;

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                         errorNumber, typeReprStr);

    JS_free(cx, (void *) typeReprStr);
    return false;
}

 void
BinaryBlock::obj_trace(JSTracer *trace, JSObject *object)
{
    JS_ASSERT(object->hasClass(&class_));

    for (size_t i = 0; i < BLOCK_RESERVED_SLOTS; i++)
        gc::MarkSlot(trace, &object->getReservedSlotRef(i), "BinaryBlockSlot");
}

 void
BinaryBlock::obj_finalize(js::FreeOp *op, JSObject *obj)
{
    if (!obj->getFixedSlot(SLOT_BLOCKREFOWNER).isNull())
        return;

    if (void *mem = obj->getPrivate())
        op->free_(mem);
}

 JSObject *
BinaryBlock::createNull(JSContext *cx, HandleObject type, HandleValue owner)
{
    JS_ASSERT(IsBinaryType(type));

    RootedValue protoVal(cx);
    if (!JSObject::getProperty(cx, type, type,
                               cx->names().prototype, &protoVal))
        return nullptr;

    RootedObject obj(cx,
        NewObjectWithClassProto(cx, &class_, &protoVal.toObject(), nullptr));
    obj->initFixedSlot(SLOT_DATATYPE, ObjectValue(*type));
    obj->initFixedSlot(SLOT_BLOCKREFOWNER, owner);

    
    
    if (cx->typeInferenceEnabled()) {
        RootedTypeObject typeObj(cx, obj->getType(cx));
        if (typeObj) {
            TypeRepresentation *typeRepr = typeRepresentation(type);
            if (!typeObj->addTypedObjectAddendum(cx, typeRepr))
                return nullptr;
        }
    }

    return obj;
}

 JSObject *
BinaryBlock::createZeroed(JSContext *cx, HandleObject type)
{
    RootedValue owner(cx, NullValue());
    RootedObject obj(cx, createNull(cx, type, owner));
    if (!obj)
        return nullptr;

    TypeRepresentation *typeRepr = typeRepresentation(type);
    size_t memsize = typeRepr->size();
    void *memory = JS_malloc(cx, memsize);
    if (!memory)
        return nullptr;
    memset(memory, 0, memsize);
    obj->setPrivate(memory);
    return obj;
}

 JSObject *
BinaryBlock::createDerived(JSContext *cx, HandleObject type,
                           HandleObject owner, size_t offset)
{
    JS_ASSERT(IsBlock(owner));
    JS_ASSERT(offset <= BlockSize(cx, owner));
    JS_ASSERT(offset + TypeSize(type) <= BlockSize(cx, owner));

    RootedValue ownerValue(cx, ObjectValue(*owner));
    RootedObject obj(cx, createNull(cx, type, ownerValue));
    if (!obj)
        return nullptr;

    obj->setPrivate(BlockMem(owner) + offset);
    return obj;
}

 bool
BinaryBlock::construct(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    RootedObject callee(cx, &args.callee());
    JS_ASSERT(IsBinaryType(callee));

    RootedObject obj(cx, createZeroed(cx, callee));
    if (!obj)
        return false;

    if (argc == 1) {
        RootedValue initial(cx, args[0]);
        if (!ConvertAndCopyTo(cx, initial, obj))
            return false;
    }

    args.rval().setObject(*obj);
    return true;
}

bool
BinaryBlock::obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(IsBlock(obj));

    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        break;

      case TypeRepresentation::Array: {
        uint32_t index;
        if (js_IdIsIndex(id, &index))
            return obj_lookupElement(cx, obj, index, objp, propp);

        if (JSID_IS_ATOM(id, cx->names().length)) {
            MarkNonNativePropertyFound(propp);
            objp.set(obj);
            return true;
        }
        break;
      }

      case TypeRepresentation::Struct: {
        RootedObject type(cx, GetType(obj));
        JS_ASSERT(IsStructType(type));
        StructTypeRepresentation *typeRepr =
            typeRepresentation(type)->asStruct();
        const StructField *field = typeRepr->fieldNamed(id);
        if (field) {
            MarkNonNativePropertyFound(propp);
            objp.set(obj);
            return true;
        }
        break;
      }
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        objp.set(nullptr);
        propp.set(nullptr);
        return true;
    }

    return JSObject::lookupGeneric(cx, proto, id, objp, propp);
}

bool
BinaryBlock::obj_lookupProperty(JSContext *cx,
                                HandleObject obj,
                                HandlePropertyName name,
                                MutableHandleObject objp,
                                MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

bool
BinaryBlock::obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(IsBlock(obj));
    MarkNonNativePropertyFound(propp);
    objp.set(obj);
    return true;
}

static bool
ReportPropertyError(JSContext *cx,
                    const unsigned errorNumber,
                    HandleId id)
{
    RootedString str(cx, IdToString(cx, id));
    if (!str)
        return false;

    char *propName = JS_EncodeString(cx, str);
    if (!propName)
        return false;

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                         errorNumber, propName);

    JS_free(cx, propName);
    return false;
}

bool
BinaryBlock::obj_lookupSpecial(JSContext *cx, HandleObject obj,
                               HandleSpecialId sid, MutableHandleObject objp,
                               MutableHandleShape propp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

bool
BinaryBlock::obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                               PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return ReportPropertyError(cx, JSMSG_UNDEFINED_PROP, id);
}

bool
BinaryBlock::obj_defineProperty(JSContext *cx, HandleObject obj,
                                HandlePropertyName name, HandleValue v,
                                PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

bool
BinaryBlock::obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                               PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::DefineElement(cx, delegate, index, v, getter, setter, attrs);
}

bool
BinaryBlock::obj_defineSpecial(JSContext *cx, HandleObject obj, HandleSpecialId sid, HandleValue v,
                               PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, SPECIALID_TO_JSID(sid));
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

bool
BinaryBlock::obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                             HandleId id, MutableHandleValue vp)
{
    JS_ASSERT(IsBlock(obj));

    
    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_getElement(cx, obj, receiver, index, vp);

    

    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);
    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        break;

      case TypeRepresentation::Array:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            vp.setInt32(typeRepr->asArray()->length());
            return true;
        }
        break;

      case TypeRepresentation::Struct: {
        StructTypeRepresentation *structTypeRepr = typeRepr->asStruct();
        const StructField *field = structTypeRepr->fieldNamed(id);
        if (!field)
            break;

        
        
        
        
        
        
        
        
        
        
        
        
        
        RootedObject fieldTypes(
            cx,
            &type->getFixedSlot(SLOT_STRUCT_FIELD_TYPES).toObject());
        RootedValue fieldTypeVal(cx);
        if (!JSObject::getElement(cx, fieldTypes, fieldTypes,
                                  field->index, &fieldTypeVal))
            return false;

        RootedObject fieldType(cx, &fieldTypeVal.toObject());
        return Reify(cx, field->typeRepr, fieldType, obj, field->offset, vp);
      }
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return JSObject::getGeneric(cx, proto, receiver, id, vp);
}

bool
BinaryBlock::obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                              HandlePropertyName name, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

bool
BinaryBlock::obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                             uint32_t index, MutableHandleValue vp)
{
    bool present;
    return obj_getElementIfPresent(cx, obj, receiver, index, vp, &present);
}

bool
BinaryBlock::obj_getElementIfPresent(JSContext *cx, HandleObject obj,
                                     HandleObject receiver, uint32_t index,
                                     MutableHandleValue vp, bool *present)
{
    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
      case TypeRepresentation::Struct:
        break;

      case TypeRepresentation::Array: {
        *present = true;
        ArrayTypeRepresentation *arrayTypeRepr = typeRepr->asArray();

        if (index >= arrayTypeRepr->length()) {
            vp.setUndefined();
            return true;
        }

        RootedObject elementType(cx, ArrayElementType(type));
        size_t offset = arrayTypeRepr->element()->size() * index;
        return Reify(cx, arrayTypeRepr->element(), elementType, obj, offset, vp);
      }
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *present = false;
        vp.setUndefined();
        return true;
    }

    return JSObject::getElementIfPresent(cx, proto, receiver, index, vp, present);
}

bool
BinaryBlock::obj_getSpecial(JSContext *cx, HandleObject obj,
                            HandleObject receiver, HandleSpecialId sid,
                            MutableHandleValue vp)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

bool
BinaryBlock::obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                             MutableHandleValue vp, bool strict)
{
    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_setElement(cx, obj, index, vp, strict);

    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case ScalarTypeRepresentation::Scalar:
        break;

      case ScalarTypeRepresentation::Array:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
            return false;
        }
        break;

      case ScalarTypeRepresentation::Struct: {
        const StructField *field = typeRepr->asStruct()->fieldNamed(id);
        if (!field)
            break;

        uint8_t *loc = BlockMem(obj) + field->offset;
        return ConvertAndCopyTo(cx, field->typeRepr, vp, loc);
      }
    }

    return ReportBlockTypeError(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, obj);
}

bool
BinaryBlock::obj_setProperty(JSContext *cx, HandleObject obj,
                             HandlePropertyName name, MutableHandleValue vp,
                             bool strict)
{
    RootedId id(cx, NameToId(name));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

bool
BinaryBlock::obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                             MutableHandleValue vp, bool strict)
{
    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case ScalarTypeRepresentation::Scalar:
      case ScalarTypeRepresentation::Struct:
        break;

      case ScalarTypeRepresentation::Array: {
        ArrayTypeRepresentation *arrayTypeRepr = typeRepr->asArray();

        if (index >= arrayTypeRepr->length()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BINARYARRAY_BAD_INDEX);
            return false;
        }

        size_t offset = arrayTypeRepr->element()->size() * index;
        uint8_t *mem = BlockMem(obj) + offset;
        return ConvertAndCopyTo(cx, arrayTypeRepr->element(), vp, mem);
      }
    }

    return ReportBlockTypeError(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, obj);
}

bool
BinaryBlock::obj_setSpecial(JSContext *cx, HandleObject obj,
                             HandleSpecialId sid, MutableHandleValue vp,
                             bool strict)
{
    RootedId id(cx, SPECIALID_TO_JSID(sid));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

bool
BinaryBlock::obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                       HandleId id, unsigned *attrsp)
{
    uint32_t index;
    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        break;

      case TypeRepresentation::Array:
        if (js_IdIsIndex(id, &index)) {
            *attrsp = JSPROP_ENUMERATE | JSPROP_PERMANENT;
            return true;
        }
        if (JSID_IS_ATOM(id, cx->names().length)) {
            *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
            return true;
        }
        break;

      case TypeRepresentation::Struct:
        if (typeRepr->asStruct()->fieldNamed(id) != nullptr) {
            *attrsp = JSPROP_ENUMERATE | JSPROP_PERMANENT;
            return true;
        }
        break;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *attrsp = 0;
        return true;
    }

    return JSObject::getGenericAttributes(cx, proto, id, attrsp);
}

static bool
IsOwnId(JSContext *cx, HandleObject obj, HandleId id)
{
    uint32_t index;
    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        return false;

      case TypeRepresentation::Array:
        return js_IdIsIndex(id, &index) || JSID_IS_ATOM(id, cx->names().length);

      case TypeRepresentation::Struct:
        return typeRepr->asStruct()->fieldNamed(id) != nullptr;
    }

    return false;
}

bool
BinaryBlock::obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                       HandleId id, unsigned *attrsp)
{
    RootedObject type(cx, GetType(obj));

    if (IsOwnId(cx, obj, id))
        return ReportPropertyError(cx, JSMSG_CANT_REDEFINE_PROP, id);

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *attrsp = 0;
        return true;
    }

    return JSObject::setGenericAttributes(cx, proto, id, attrsp);
}

bool
BinaryBlock::obj_deleteProperty(JSContext *cx, HandleObject obj,
                                HandlePropertyName name, bool *succeeded)
{
    Rooted<jsid> id(cx, NameToId(name));
    if (IsOwnId(cx, obj, id))
        return ReportPropertyError(cx, JSMSG_CANT_DELETE, id);

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *succeeded = false;
        return true;
    }

    return JSObject::deleteProperty(cx, proto, name, succeeded);
}

bool
BinaryBlock::obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
                               bool *succeeded)
{
    RootedId id(cx);
    if (!IndexToId(cx, index, &id))
        return false;

    if (IsOwnId(cx, obj, id))
        return ReportPropertyError(cx, JSMSG_CANT_DELETE, id);

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *succeeded = false;
        return true;
    }

    return JSObject::deleteElement(cx, proto, index, succeeded);
}

bool
BinaryBlock::obj_deleteSpecial(JSContext *cx, HandleObject obj,
                               HandleSpecialId sid, bool *succeeded)
{
    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *succeeded = false;
        return true;
    }

    return JSObject::deleteSpecial(cx, proto, sid, succeeded);
}

bool
BinaryBlock::obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                           MutableHandleValue statep, MutableHandleId idp)
{
    uint32_t index;

    RootedObject type(cx, GetType(obj));
    TypeRepresentation *typeRepr = typeRepresentation(type);

    switch (typeRepr->kind()) {
      case TypeRepresentation::Scalar:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(0));

          case JSENUMERATE_NEXT:
          case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
        }
        break;

      case TypeRepresentation::Array:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typeRepr->asArray()->length()));
            break;

          case JSENUMERATE_NEXT:
            index = static_cast<int32_t>(statep.toInt32());

            if (index < typeRepr->asArray()->length()) {
                idp.set(INT_TO_JSID(index));
                statep.setInt32(index + 1);
            } else {
                JS_ASSERT(index == typeRepr->asArray()->length());
                statep.setNull();
            }

            break;

          case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
        }
        break;

      case TypeRepresentation::Struct:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typeRepr->asStruct()->fieldCount()));
            break;

          case JSENUMERATE_NEXT:
            index = static_cast<uint32_t>(statep.toInt32());

            if (index < typeRepr->asStruct()->fieldCount()) {
                idp.set(typeRepr->asStruct()->field(index).id);
                statep.setInt32(index + 1);
            } else {
                statep.setNull();
            }

            break;

          case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
        }
        break;
    }

    return true;
}

 size_t
BinaryBlock::dataOffset()
{
    return JSObject::getPrivateDataOffset(BLOCK_RESERVED_SLOTS + 1);
}
