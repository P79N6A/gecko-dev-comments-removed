





#include "builtin/TypedObject.h"

#include "mozilla/Casting.h"
#include "mozilla/CheckedInt.h"

#include "jscompartment.h"
#include "jsfun.h"
#include "jsutil.h"

#include "gc/Marking.h"
#include "js/Vector.h"
#include "vm/GlobalObject.h"
#include "vm/String.h"
#include "vm/StringBuffer.h"
#include "vm/TypedArrayObject.h"
#include "vm/WeakMapObject.h"

#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "vm/NativeObject-inl.h"
#include "vm/Shape-inl.h"

using mozilla::AssertedCast;
using mozilla::CheckedInt32;
using mozilla::DebugOnly;
using mozilla::PodCopy;

using namespace js;

const Class js::TypedObjectModuleObject::class_ = {
    "TypedObject",
    JSCLASS_HAS_RESERVED_SLOTS(SlotCount) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_TypedObject)
};

static const JSFunctionSpec TypedObjectMethods[] = {
    JS_SELF_HOSTED_FN("objectType", "TypeOfTypedObject", 1, 0),
    JS_SELF_HOSTED_FN("storage", "StorageOfTypedObject", 1, 0),
    JS_FS_END
};

static void
ReportCannotConvertTo(JSContext* cx, HandleValue fromValue, const char* toType)
{
    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
                         InformalValueTypeName(fromValue), toType);
}

template<class T>
static inline T*
ToObjectIf(HandleValue value)
{
    if (!value.isObject())
        return nullptr;

    if (!value.toObject().is<T>())
        return nullptr;

    return &value.toObject().as<T>();
}

static inline CheckedInt32 roundUpToAlignment(CheckedInt32 address, int32_t align)
{
    MOZ_ASSERT(IsPowerOfTwo(align));

    
    
    
    
    
    
    

    return ((address + (align - 1)) / align) * align;
}


























static bool
ConvertAndCopyTo(JSContext* cx,
                 HandleTypeDescr typeObj,
                 HandleTypedObject typedObj,
                 int32_t offset,
                 HandleAtom name,
                 HandleValue val)
{
    RootedFunction func(
        cx, SelfHostedFunction(cx, cx->names().ConvertAndCopyTo));
    if (!func)
        return false;

    InvokeArgs args(cx);
    if (!args.init(5))
        return false;

    args.setCallee(ObjectValue(*func));
    args[0].setObject(*typeObj);
    args[1].setObject(*typedObj);
    args[2].setInt32(offset);
    if (name)
        args[3].setString(name);
    else
        args[3].setNull();
    args[4].set(val);

    return Invoke(cx, args);
}

static bool
ConvertAndCopyTo(JSContext* cx, HandleTypedObject typedObj, HandleValue val)
{
    Rooted<TypeDescr*> type(cx, &typedObj->typeDescr());
    return ConvertAndCopyTo(cx, type, typedObj, 0, NullPtr(), val);
}





static bool
Reify(JSContext* cx,
      HandleTypeDescr type,
      HandleTypedObject typedObj,
      size_t offset,
      MutableHandleValue to)
{
    RootedFunction func(cx, SelfHostedFunction(cx, cx->names().Reify));
    if (!func)
        return false;

    InvokeArgs args(cx);
    if (!args.init(3))
        return false;

    args.setCallee(ObjectValue(*func));
    args[0].setObject(*type);
    args[1].setObject(*typedObj);
    args[2].setInt32(offset);

    if (!Invoke(cx, args))
        return false;

    to.set(args.rval());
    return true;
}



static JSObject*
GetPrototype(JSContext* cx, HandleObject obj)
{
    RootedValue prototypeVal(cx);
    if (!GetProperty(cx, obj, obj, cx->names().prototype,
                               &prototypeVal))
    {
        return nullptr;
    }
    if (!prototypeVal.isObject()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_INVALID_PROTOTYPE);
        return nullptr;
    }
    return &prototypeVal.toObject();
}









const Class js::TypedProto::class_ = {
    "TypedProto",
    JSCLASS_HAS_RESERVED_SLOTS(JS_TYPROTO_SLOTS)
};










const Class js::ScalarTypeDescr::class_ = {
    "Scalar",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS) | JSCLASS_BACKGROUND_FINALIZE,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    TypeDescr::finalize,
    ScalarTypeDescr::call
};

const JSFunctionSpec js::ScalarTypeDescr::typeObjectMethods[] = {
    JS_SELF_HOSTED_FN("toSource", "DescrToSource", 0, 0),
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_FS_END
};

int32_t
ScalarTypeDescr::size(Type t)
{
    return Scalar::byteSize(t);
}

int32_t
ScalarTypeDescr::alignment(Type t)
{
    return Scalar::byteSize(t);
}

 const char*
ScalarTypeDescr::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_, name_) \
        case constant_: return #name_;
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
#undef NUMERIC_TYPE_TO_STRING
      case Scalar::Float32x4:
      case Scalar::Int32x4:
      case Scalar::MaxTypedArrayViewType:
        MOZ_CRASH();
    }
    MOZ_CRASH("Invalid type");
}

bool
ScalarTypeDescr::call(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             args.callee().getClass()->name, "0", "s");
        return false;
    }

    Rooted<ScalarTypeDescr*> descr(cx, &args.callee().as<ScalarTypeDescr>());
    ScalarTypeDescr::Type type = descr->type();

    double number;
    if (!ToNumber(cx, args[0], &number))
        return false;

    if (type == Scalar::Uint8Clamped)
        number = ClampDoubleToUint8(number);

    switch (type) {
#define SCALARTYPE_CALL(constant_, type_, name_)                             \
      case constant_: {                                                       \
          type_ converted = ConvertScalar<type_>(number);                     \
          args.rval().setNumber((double) converted);                          \
          return true;                                                        \
      }

        JS_FOR_EACH_SCALAR_TYPE_REPR(SCALARTYPE_CALL)
#undef SCALARTYPE_CALL
      case Scalar::Float32x4:
      case Scalar::Int32x4:
      case Scalar::MaxTypedArrayViewType:
        MOZ_CRASH();
    }
    return true;
}











const Class js::ReferenceTypeDescr::class_ = {
    "Reference",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS) | JSCLASS_BACKGROUND_FINALIZE,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    TypeDescr::finalize,
    ReferenceTypeDescr::call
};

const JSFunctionSpec js::ReferenceTypeDescr::typeObjectMethods[] = {
    JS_SELF_HOSTED_FN("toSource", "DescrToSource", 0, 0),
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_FS_END
};

static const int32_t ReferenceSizes[] = {
#define REFERENCE_SIZE(_kind, _type, _name)                        \
    sizeof(_type),
    JS_FOR_EACH_REFERENCE_TYPE_REPR(REFERENCE_SIZE) 0
#undef REFERENCE_SIZE
};

int32_t
ReferenceTypeDescr::size(Type t)
{
    return ReferenceSizes[t];
}

int32_t
ReferenceTypeDescr::alignment(Type t)
{
    return ReferenceSizes[t];
}

 const char*
ReferenceTypeDescr::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_, name_) \
        case constant_: return #name_;
        JS_FOR_EACH_REFERENCE_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
#undef NUMERIC_TYPE_TO_STRING
    }
    MOZ_CRASH("Invalid type");
}

bool
js::ReferenceTypeDescr::call(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    MOZ_ASSERT(args.callee().is<ReferenceTypeDescr>());
    Rooted<ReferenceTypeDescr*> descr(cx, &args.callee().as<ReferenceTypeDescr>());

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_MORE_ARGS_NEEDED,
                             descr->typeName(), "0", "s");
        return false;
    }

    switch (descr->type()) {
      case ReferenceTypeDescr::TYPE_ANY:
        args.rval().set(args[0]);
        return true;

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        RootedObject obj(cx, ToObject(cx, args[0]));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        RootedString obj(cx, ToString<CanGC>(cx, args[0]));
        if (!obj)
            return false;
        args.rval().setString(&*obj);
        return true;
      }
    }

    MOZ_CRASH("Unhandled Reference type");
}







static const int32_t SimdSizes[] = {
#define SIMD_SIZE(_kind, _type, _name, _lanes)                 \
    sizeof(_type) * _lanes,
    JS_FOR_EACH_SIMD_TYPE_REPR(SIMD_SIZE) 0
#undef SIMD_SIZE
};

static int32_t SimdLanes[] = {
#define SIMD_LANE(_kind, _type, _name, _lanes)                 \
    _lanes,
    JS_FOR_EACH_SIMD_TYPE_REPR(SIMD_LANE) 0
#undef SIMD_LANE
};

int32_t
SimdTypeDescr::size(Type t)
{
    return SimdSizes[t];
}

int32_t
SimdTypeDescr::alignment(Type t)
{
    return SimdSizes[t];
}

int32_t
SimdTypeDescr::lanes(Type t)
{
    return SimdLanes[t];
}






































static TypedProto*
CreatePrototypeObjectForComplexTypeInstance(JSContext* cx, HandleObject ctorPrototype)
{
    RootedObject ctorPrototypePrototype(cx, GetPrototype(cx, ctorPrototype));
    if (!ctorPrototypePrototype)
        return nullptr;

    return NewObjectWithProto<TypedProto>(cx, ctorPrototypePrototype, SingletonObject);
}

const Class ArrayTypeDescr::class_ = {
    "ArrayType",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS) | JSCLASS_BACKGROUND_FINALIZE,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    TypeDescr::finalize,
    nullptr, 
    nullptr, 
    TypedObject::construct
};

const JSPropertySpec ArrayMetaTypeDescr::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayMetaTypeDescr::typeObjectMethods[] = {
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    JS_SELF_HOSTED_FN("toSource", "DescrToSource", 0, 0),
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_SELF_HOSTED_FN("build",    "TypedObjectArrayTypeBuild", 3, 0),
    JS_SELF_HOSTED_FN("from",     "TypedObjectArrayTypeFrom", 3, 0),
    JS_FS_END
};

const JSPropertySpec ArrayMetaTypeDescr::typedObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayMetaTypeDescr::typedObjectMethods[] = {
    {"forEach", {nullptr, nullptr}, 1, 0, "ArrayForEach"},
    {"redimension", {nullptr, nullptr}, 1, 0, "TypedObjectArrayRedimension"},
    JS_SELF_HOSTED_FN("map",        "TypedObjectArrayMap",        2, 0),
    JS_SELF_HOSTED_FN("reduce",     "TypedObjectArrayReduce",     2, 0),
    JS_SELF_HOSTED_FN("filter",     "TypedObjectArrayFilter",     1, 0),
    JS_FS_END
};

bool
js::CreateUserSizeAndAlignmentProperties(JSContext* cx, HandleTypeDescr descr)
{
    
    if (descr->transparent()) {
        
        RootedValue typeByteLength(cx, Int32Value(descr->size()));
        if (!DefineProperty(cx, descr, cx->names().byteLength, typeByteLength,
                            nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }

        
        RootedValue typeByteAlignment(cx, Int32Value(descr->alignment()));
        if (!DefineProperty(cx, descr, cx->names().byteAlignment, typeByteAlignment,
                            nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }
    } else {
        
        if (!DefineProperty(cx, descr, cx->names().byteLength, UndefinedHandleValue,
                            nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }

        
        if (!DefineProperty(cx, descr, cx->names().byteAlignment, UndefinedHandleValue,
                            nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }
    }

    return true;
}

static bool
CreateTraceList(JSContext* cx, HandleTypeDescr descr);

ArrayTypeDescr*
ArrayMetaTypeDescr::create(JSContext* cx,
                           HandleObject arrayTypePrototype,
                           HandleTypeDescr elementType,
                           HandleAtom stringRepr,
                           int32_t size,
                           int32_t length)
{
    Rooted<ArrayTypeDescr*> obj(cx);
    obj = NewObjectWithProto<ArrayTypeDescr>(cx, arrayTypePrototype, SingletonObject);
    if (!obj)
        return nullptr;

    obj->initReservedSlot(JS_DESCR_SLOT_KIND, Int32Value(ArrayTypeDescr::Kind));
    obj->initReservedSlot(JS_DESCR_SLOT_STRING_REPR, StringValue(stringRepr));
    obj->initReservedSlot(JS_DESCR_SLOT_ALIGNMENT, Int32Value(elementType->alignment()));
    obj->initReservedSlot(JS_DESCR_SLOT_SIZE, Int32Value(size));
    obj->initReservedSlot(JS_DESCR_SLOT_OPAQUE, BooleanValue(elementType->opaque()));
    obj->initReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE, ObjectValue(*elementType));
    obj->initReservedSlot(JS_DESCR_SLOT_ARRAY_LENGTH, Int32Value(length));

    RootedValue elementTypeVal(cx, ObjectValue(*elementType));
    if (!DefineProperty(cx, obj, cx->names().elementType, elementTypeVal,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }

    RootedValue lengthValue(cx, NumberValue(length));
    if (!DefineProperty(cx, obj, cx->names().length, lengthValue,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }

    if (!CreateUserSizeAndAlignmentProperties(cx, obj))
        return nullptr;

    
    
    Rooted<TypedProto*> prototypeObj(cx);
    if (elementType->getReservedSlot(JS_DESCR_SLOT_ARRAYPROTO).isObject()) {
        prototypeObj = &elementType->getReservedSlot(JS_DESCR_SLOT_ARRAYPROTO).toObject().as<TypedProto>();
    } else {
        prototypeObj = CreatePrototypeObjectForComplexTypeInstance(cx, arrayTypePrototype);
        if (!prototypeObj)
            return nullptr;
        elementType->setReservedSlot(JS_DESCR_SLOT_ARRAYPROTO, ObjectValue(*prototypeObj));
    }

    obj->initReservedSlot(JS_DESCR_SLOT_TYPROTO, ObjectValue(*prototypeObj));

    if (!LinkConstructorAndPrototype(cx, obj, prototypeObj))
        return nullptr;

    if (!CreateTraceList(cx, obj))
        return nullptr;

    return obj;
}

bool
ArrayMetaTypeDescr::construct(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_NOT_FUNCTION, "ArrayType");
        return false;
    }

    RootedObject arrayTypeGlobal(cx, &args.callee());

    
    if (args.length() < 2) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "ArrayType", "1", "");
        return false;
    }

    if (!args[0].isObject() || !args[0].toObject().is<TypeDescr>()) {
        ReportCannotConvertTo(cx, args[0], "ArrayType element specifier");
        return false;
    }

    if (!args[1].isInt32() || args[1].toInt32() < 0) {
        ReportCannotConvertTo(cx, args[1], "ArrayType length specifier");
        return false;
    }

    Rooted<TypeDescr*> elementType(cx, &args[0].toObject().as<TypeDescr>());

    int32_t length = args[1].toInt32();

    
    CheckedInt32 size = CheckedInt32(elementType->size()) * length;
    if (!size.isValid()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return false;
    }

    
    StringBuffer contents(cx);
    contents.append("new ArrayType(");
    contents.append(&elementType->stringRepr());
    contents.append(", ");
    if (!NumberValueToStringBuffer(cx, NumberValue(length), contents))
        return false;
    contents.append(")");
    RootedAtom stringRepr(cx, contents.finishAtom());
    if (!stringRepr)
        return false;

    
    RootedObject arrayTypePrototype(cx, GetPrototype(cx, arrayTypeGlobal));
    if (!arrayTypePrototype)
        return false;

    
    Rooted<ArrayTypeDescr*> obj(cx);
    obj = create(cx, arrayTypePrototype, elementType, stringRepr, size.value(), length);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

bool
js::IsTypedObjectArray(JSObject& obj)
{
    if (!obj.is<TypedObject>())
        return false;
    TypeDescr& d = obj.as<TypedObject>().typeDescr();
    return d.is<ArrayTypeDescr>();
}





const Class StructTypeDescr::class_ = {
    "StructType",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS) | JSCLASS_BACKGROUND_FINALIZE,
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    nullptr, 
    TypeDescr::finalize,
    nullptr, 
    nullptr, 
    TypedObject::construct
};

const JSPropertySpec StructMetaTypeDescr::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec StructMetaTypeDescr::typeObjectMethods[] = {
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    JS_SELF_HOSTED_FN("toSource", "DescrToSource", 0, 0),
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_FS_END
};

const JSPropertySpec StructMetaTypeDescr::typedObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec StructMetaTypeDescr::typedObjectMethods[] = {
    JS_FS_END
};

JSObject*
StructMetaTypeDescr::create(JSContext* cx,
                            HandleObject metaTypeDescr,
                            HandleObject fields)
{
    
    AutoIdVector ids(cx);
    if (!GetPropertyKeys(cx, fields, JSITER_OWNONLY | JSITER_SYMBOLS, &ids))
        return nullptr;

    
    
    
    StringBuffer stringBuffer(cx);     
    AutoValueVector fieldNames(cx);    
    AutoValueVector fieldTypeObjs(cx); 
    AutoValueVector fieldOffsets(cx);  
    RootedObject userFieldOffsets(cx); 
    RootedObject userFieldTypes(cx);   
    CheckedInt32 sizeSoFar(0);         
    int32_t alignment = 1;             
    bool opaque = false;               

    userFieldOffsets = NewObjectWithProto<PlainObject>(cx, NullPtr(), TenuredObject);
    if (!userFieldOffsets)
        return nullptr;

    userFieldTypes = NewObjectWithProto<PlainObject>(cx, NullPtr(), TenuredObject);
    if (!userFieldTypes)
        return nullptr;

    if (!stringBuffer.append("new StructType({")) {
        ReportOutOfMemory(cx);
        return nullptr;
    }

    RootedValue fieldTypeVal(cx);
    RootedId id(cx);
    Rooted<TypeDescr*> fieldType(cx);
    for (unsigned int i = 0; i < ids.length(); i++) {
        id = ids[i];

        
        uint32_t unused;
        if (!JSID_IS_ATOM(id) || JSID_TO_ATOM(id)->isIndex(&unused)) {
            RootedValue idValue(cx, IdToValue(id));
            ReportCannotConvertTo(cx, idValue, "StructType field name");
            return nullptr;
        }

        
        
        if (!GetProperty(cx, fields, fields, id, &fieldTypeVal))
            return nullptr;
        fieldType = ToObjectIf<TypeDescr>(fieldTypeVal);
        if (!fieldType) {
            ReportCannotConvertTo(cx, fieldTypeVal, "StructType field specifier");
            return nullptr;
        }

        
        RootedValue fieldName(cx, IdToValue(id));
        if (!fieldNames.append(fieldName)) {
            ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!fieldTypeObjs.append(ObjectValue(*fieldType))) {
            ReportOutOfMemory(cx);
            return nullptr;
        }

        
        if (!DefineProperty(cx, userFieldTypes, id, fieldTypeObjs[i], nullptr, nullptr,
                            JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return nullptr;
        }

        
        if (i > 0 && !stringBuffer.append(", ")) {
            ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!stringBuffer.append(JSID_TO_ATOM(id))) {
            ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!stringBuffer.append(": ")) {
            ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!stringBuffer.append(&fieldType->stringRepr())) {
            ReportOutOfMemory(cx);
            return nullptr;
        }

        
        
        CheckedInt32 offset = roundUpToAlignment(sizeSoFar, fieldType->alignment());
        if (!offset.isValid()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return nullptr;
        }
        MOZ_ASSERT(offset.value() >= 0);
        if (!fieldOffsets.append(Int32Value(offset.value()))) {
            ReportOutOfMemory(cx);
            return nullptr;
        }

        
        RootedValue offsetValue(cx, Int32Value(offset.value()));
        if (!DefineProperty(cx, userFieldOffsets, id, offsetValue, nullptr, nullptr,
                            JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return nullptr;
        }

        
        sizeSoFar = offset + fieldType->size();
        if (!sizeSoFar.isValid()) {
            JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return nullptr;
        }

        
        if (fieldType->opaque())
            opaque = true;

        
        alignment = js::Max(alignment, fieldType->alignment());
    }

    
    if (!stringBuffer.append("})")) {
        ReportOutOfMemory(cx);
        return nullptr;
    }
    RootedAtom stringRepr(cx, stringBuffer.finishAtom());
    if (!stringRepr)
        return nullptr;

    
    CheckedInt32 totalSize = roundUpToAlignment(sizeSoFar, alignment);
    if (!totalSize.isValid()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return nullptr;
    }

    
    RootedObject structTypePrototype(cx, GetPrototype(cx, metaTypeDescr));
    if (!structTypePrototype)
        return nullptr;

    Rooted<StructTypeDescr*> descr(cx);
    descr = NewObjectWithProto<StructTypeDescr>(cx, structTypePrototype,
                                                SingletonObject);
    if (!descr)
        return nullptr;

    descr->initReservedSlot(JS_DESCR_SLOT_KIND, Int32Value(type::Struct));
    descr->initReservedSlot(JS_DESCR_SLOT_STRING_REPR, StringValue(stringRepr));
    descr->initReservedSlot(JS_DESCR_SLOT_ALIGNMENT, Int32Value(alignment));
    descr->initReservedSlot(JS_DESCR_SLOT_SIZE, Int32Value(totalSize.value()));
    descr->initReservedSlot(JS_DESCR_SLOT_OPAQUE, BooleanValue(opaque));

    
    {
        RootedObject fieldNamesVec(cx);
        fieldNamesVec = NewDenseCopiedArray(cx, fieldNames.length(),
                                            fieldNames.begin(), NullPtr(),
                                            TenuredObject);
        if (!fieldNamesVec)
            return nullptr;
        descr->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_NAMES,
                                     ObjectValue(*fieldNamesVec));
    }

    
    {
        RootedObject fieldTypeVec(cx);
        fieldTypeVec = NewDenseCopiedArray(cx, fieldTypeObjs.length(),
                                           fieldTypeObjs.begin(), NullPtr(),
                                           TenuredObject);
        if (!fieldTypeVec)
            return nullptr;
        descr->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_TYPES,
                                     ObjectValue(*fieldTypeVec));
    }

    
    {
        RootedObject fieldOffsetsVec(cx);
        fieldOffsetsVec = NewDenseCopiedArray(cx, fieldOffsets.length(),
                                              fieldOffsets.begin(), NullPtr(),
                                              TenuredObject);
        if (!fieldOffsetsVec)
            return nullptr;
        descr->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS,
                                     ObjectValue(*fieldOffsetsVec));
    }

    
    if (!FreezeObject(cx, userFieldOffsets))
        return nullptr;
    if (!FreezeObject(cx, userFieldTypes))
        return nullptr;
    RootedValue userFieldOffsetsValue(cx, ObjectValue(*userFieldOffsets));
    if (!DefineProperty(cx, descr, cx->names().fieldOffsets, userFieldOffsetsValue,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }
    RootedValue userFieldTypesValue(cx, ObjectValue(*userFieldTypes));
    if (!DefineProperty(cx, descr, cx->names().fieldTypes, userFieldTypesValue,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }

    if (!CreateUserSizeAndAlignmentProperties(cx, descr))
        return nullptr;

    Rooted<TypedProto*> prototypeObj(cx);
    prototypeObj = CreatePrototypeObjectForComplexTypeInstance(cx, structTypePrototype);
    if (!prototypeObj)
        return nullptr;

    descr->initReservedSlot(JS_DESCR_SLOT_TYPROTO, ObjectValue(*prototypeObj));

    if (!LinkConstructorAndPrototype(cx, descr, prototypeObj))
        return nullptr;

    if (!CreateTraceList(cx, descr))
        return nullptr;

    return descr;
}

bool
StructMetaTypeDescr::construct(JSContext* cx, unsigned int argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                             JSMSG_NOT_FUNCTION, "StructType");
        return false;
    }

    if (args.length() >= 1 && args[0].isObject()) {
        RootedObject metaTypeDescr(cx, &args.callee());
        RootedObject fields(cx, &args[0].toObject());
        RootedObject obj(cx, create(cx, metaTypeDescr, fields));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                         JSMSG_TYPEDOBJECT_STRUCTTYPE_BAD_ARGS);
    return false;
}

size_t
StructTypeDescr::fieldCount() const
{
    return fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_NAMES).getDenseInitializedLength();
}

size_t
StructTypeDescr::maybeForwardedFieldCount() const
{
    return maybeForwardedFieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_NAMES).getDenseInitializedLength();
}

bool
StructTypeDescr::fieldIndex(jsid id, size_t* out) const
{
    ArrayObject& fieldNames = fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_NAMES);
    size_t l = fieldNames.getDenseInitializedLength();
    for (size_t i = 0; i < l; i++) {
        JSAtom& a = fieldNames.getDenseElement(i).toString()->asAtom();
        if (JSID_IS_ATOM(id, &a)) {
            *out = i;
            return true;
        }
    }
    return false;
}

JSAtom&
StructTypeDescr::fieldName(size_t index) const
{
    return fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_NAMES).getDenseElement(index).toString()->asAtom();
}

size_t
StructTypeDescr::fieldOffset(size_t index) const
{
    ArrayObject& fieldOffsets = fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS);
    MOZ_ASSERT(index < fieldOffsets.getDenseInitializedLength());
    return AssertedCast<size_t>(fieldOffsets.getDenseElement(index).toInt32());
}

size_t
StructTypeDescr::maybeForwardedFieldOffset(size_t index) const
{
    ArrayObject& fieldOffsets = maybeForwardedFieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS);
    MOZ_ASSERT(index < fieldOffsets.getDenseInitializedLength());
    return AssertedCast<size_t>(fieldOffsets.getDenseElement(index).toInt32());
}

TypeDescr&
StructTypeDescr::fieldDescr(size_t index) const
{
    ArrayObject& fieldDescrs = fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_TYPES);
    MOZ_ASSERT(index < fieldDescrs.getDenseInitializedLength());
    return fieldDescrs.getDenseElement(index).toObject().as<TypeDescr>();
}

TypeDescr&
StructTypeDescr::maybeForwardedFieldDescr(size_t index) const
{
    ArrayObject& fieldDescrs = maybeForwardedFieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_TYPES);
    MOZ_ASSERT(index < fieldDescrs.getDenseInitializedLength());
    JSObject& descr =
        *MaybeForwarded(&fieldDescrs.getDenseElement(index).toObject());
    return descr.as<TypeDescr>();
}

















































template<typename T>
static bool
DefineSimpleTypeDescr(JSContext* cx,
                      Handle<GlobalObject*> global,
                      HandleObject module,
                      typename T::Type type,
                      HandlePropertyName className)
{
    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return false;

    RootedObject funcProto(cx, global->getOrCreateFunctionPrototype(cx));
    if (!funcProto)
        return false;

    Rooted<T*> descr(cx);
    descr = NewObjectWithProto<T>(cx, funcProto, SingletonObject);
    if (!descr)
        return false;

    descr->initReservedSlot(JS_DESCR_SLOT_KIND, Int32Value(T::Kind));
    descr->initReservedSlot(JS_DESCR_SLOT_STRING_REPR, StringValue(className));
    descr->initReservedSlot(JS_DESCR_SLOT_ALIGNMENT, Int32Value(T::alignment(type)));
    descr->initReservedSlot(JS_DESCR_SLOT_SIZE, Int32Value(T::size(type)));
    descr->initReservedSlot(JS_DESCR_SLOT_OPAQUE, BooleanValue(T::Opaque));
    descr->initReservedSlot(JS_DESCR_SLOT_TYPE, Int32Value(type));

    if (!CreateUserSizeAndAlignmentProperties(cx, descr))
        return false;

    if (!JS_DefineFunctions(cx, descr, T::typeObjectMethods))
        return false;

    
    
    Rooted<TypedProto*> proto(cx);
    proto = NewObjectWithProto<TypedProto>(cx, objProto, TenuredObject);
    if (!proto)
        return false;
    descr->initReservedSlot(JS_DESCR_SLOT_TYPROTO, ObjectValue(*proto));

    RootedValue descrValue(cx, ObjectValue(*descr));
    if (!DefineProperty(cx, module, className, descrValue, nullptr, nullptr, 0))
        return false;

    if (!CreateTraceList(cx, descr))
        return false;

    return true;
}



template<typename T>
static JSObject*
DefineMetaTypeDescr(JSContext* cx,
                    const char* name,
                    Handle<GlobalObject*> global,
                    Handle<TypedObjectModuleObject*> module,
                    TypedObjectModuleObject::Slot protoSlot)
{
    RootedAtom className(cx, Atomize(cx, name, strlen(name)));
    if (!className)
        return nullptr;

    RootedObject funcProto(cx, global->getOrCreateFunctionPrototype(cx));
    if (!funcProto)
        return nullptr;

    

    RootedObject proto(cx, NewObjectWithProto<PlainObject>(cx, funcProto,
                                                           SingletonObject));
    if (!proto)
        return nullptr;

    

    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return nullptr;
    RootedObject protoProto(cx);
    protoProto = NewObjectWithProto<PlainObject>(cx, objProto, SingletonObject);
    if (!protoProto)
        return nullptr;

    RootedValue protoProtoValue(cx, ObjectValue(*protoProto));
    if (!DefineProperty(cx, proto, cx->names().prototype, protoProtoValue,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }

    

    const int constructorLength = 2;
    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, T::construct, className, constructorLength);
    if (!ctor ||
        !LinkConstructorAndPrototype(cx, ctor, proto) ||
        !DefinePropertiesAndFunctions(cx, proto,
                                      T::typeObjectProperties,
                                      T::typeObjectMethods) ||
        !DefinePropertiesAndFunctions(cx, protoProto,
                                      T::typedObjectProperties,
                                      T::typedObjectMethods))
    {
        return nullptr;
    }

    module->initReservedSlot(protoSlot, ObjectValue(*proto));

    return ctor;
}







bool
GlobalObject::initTypedObjectModule(JSContext* cx, Handle<GlobalObject*> global)
{
    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return false;

    Rooted<TypedObjectModuleObject*> module(cx);
    module = NewObjectWithProto<TypedObjectModuleObject>(cx, objProto);
    if (!module)
        return false;

    if (!JS_DefineFunctions(cx, module, TypedObjectMethods))
        return false;

    

#define BINARYDATA_SCALAR_DEFINE(constant_, type_, name_)                       \
    if (!DefineSimpleTypeDescr<ScalarTypeDescr>(cx, global, module, constant_,      \
                                            cx->names().name_))                 \
        return false;
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_SCALAR_DEFINE)
#undef BINARYDATA_SCALAR_DEFINE

#define BINARYDATA_REFERENCE_DEFINE(constant_, type_, name_)                    \
    if (!DefineSimpleTypeDescr<ReferenceTypeDescr>(cx, global, module, constant_,   \
                                               cx->names().name_))              \
        return false;
    JS_FOR_EACH_REFERENCE_TYPE_REPR(BINARYDATA_REFERENCE_DEFINE)
#undef BINARYDATA_REFERENCE_DEFINE

    

    RootedObject arrayType(cx);
    arrayType = DefineMetaTypeDescr<ArrayMetaTypeDescr>(
        cx, "ArrayType", global, module, TypedObjectModuleObject::ArrayTypePrototype);
    if (!arrayType)
        return false;

    RootedValue arrayTypeValue(cx, ObjectValue(*arrayType));
    if (!DefineProperty(cx, module, cx->names().ArrayType, arrayTypeValue,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return false;
    }

    

    RootedObject structType(cx);
    structType = DefineMetaTypeDescr<StructMetaTypeDescr>(
        cx, "StructType", global, module, TypedObjectModuleObject::StructTypePrototype);
    if (!structType)
        return false;

    RootedValue structTypeValue(cx, ObjectValue(*structType));
    if (!DefineProperty(cx, module, cx->names().StructType, structTypeValue,
                        nullptr, nullptr, JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return false;
    }

    
    RootedValue moduleValue(cx, ObjectValue(*module));
    global->setConstructor(JSProto_TypedObject, moduleValue);
    if (!DefineProperty(cx, global, cx->names().TypedObject, moduleValue, nullptr, nullptr, 0))
        return false;

    return module;
}

JSObject*
js::InitTypedObjectModuleObject(JSContext* cx, HandleObject obj)
{
    MOZ_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject*> global(cx, &obj->as<GlobalObject>());
    return global->getOrCreateTypedObjectModule(cx);
}





int32_t
TypedObject::offset() const
{
    if (is<InlineTypedObject>())
        return 0;
    return typedMem() - typedMemBase();
}

int32_t
TypedObject::length() const
{
    return typeDescr().as<ArrayTypeDescr>().length();
}

uint8_t*
TypedObject::typedMem() const
{
    MOZ_ASSERT(isAttached());

    if (is<InlineTypedObject>())
        return as<InlineTypedObject>().inlineTypedMem();
    return as<OutlineTypedObject>().outOfLineTypedMem();
}

uint8_t*
TypedObject::typedMemBase() const
{
    MOZ_ASSERT(isAttached());
    MOZ_ASSERT(is<OutlineTypedObject>());

    JSObject& owner = as<OutlineTypedObject>().owner();
    if (owner.is<ArrayBufferObject>())
        return owner.as<ArrayBufferObject>().dataPointer();
    return owner.as<InlineTypedObject>().inlineTypedMem();
}

bool
TypedObject::isAttached() const
{
    if (is<InlineTransparentTypedObject>()) {
        ObjectWeakMap* table = compartment()->lazyArrayBuffers;
        if (table) {
            JSObject* buffer = table->lookup(this);
            if (buffer)
                return !buffer->as<ArrayBufferObject>().isNeutered();
        }
        return true;
    }
    if (is<InlineOpaqueTypedObject>())
        return true;
    if (!as<OutlineTypedObject>().outOfLineTypedMem())
        return false;
    JSObject& owner = as<OutlineTypedObject>().owner();
    if (owner.is<ArrayBufferObject>() && owner.as<ArrayBufferObject>().isNeutered())
        return false;
    return true;
}

bool
TypedObject::maybeForwardedIsAttached() const
{
    if (is<InlineTypedObject>())
        return true;
    if (!as<OutlineTypedObject>().outOfLineTypedMem())
        return false;
    JSObject& owner = *MaybeForwarded(&as<OutlineTypedObject>().owner());
    if (owner.is<ArrayBufferObject>() && owner.as<ArrayBufferObject>().isNeutered())
        return false;
    return true;
}

 bool
TypedObject::GetBuffer(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject& obj = args[0].toObject();
    ArrayBufferObject* buffer;
    if (obj.is<OutlineTransparentTypedObject>())
        buffer = obj.as<OutlineTransparentTypedObject>().getOrCreateBuffer(cx);
    else
        buffer = obj.as<InlineTransparentTypedObject>().getOrCreateBuffer(cx);
    if (!buffer)
        return false;
    args.rval().setObject(*buffer);
    return true;
}

 bool
TypedObject::GetByteOffset(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setInt32(args[0].toObject().as<TypedObject>().offset());
    return true;
}





 OutlineTypedObject*
OutlineTypedObject::createUnattached(JSContext* cx,
                                     HandleTypeDescr descr,
                                     int32_t length,
                                     gc::InitialHeap heap)
{
    if (descr->opaque())
        return createUnattachedWithClass(cx, &OutlineOpaqueTypedObject::class_, descr, length, heap);
    else
        return createUnattachedWithClass(cx, &OutlineTransparentTypedObject::class_, descr, length, heap);
}

void
OutlineTypedObject::setOwnerAndData(JSObject* owner, uint8_t* data)
{
    
    
    MOZ_ASSERT_IF(owner && owner->is<ArrayBufferObject>(),
                  !owner->as<ArrayBufferObject>().forInlineTypedObject());

    
    
    owner_ = owner;
    data_ = data;

    
    
    if (owner && !IsInsideNursery(this) && IsInsideNursery(owner))
        runtimeFromMainThread()->gc.storeBuffer.putWholeCellFromMainThread(this);
}

 OutlineTypedObject*
OutlineTypedObject::createUnattachedWithClass(JSContext* cx,
                                              const Class* clasp,
                                              HandleTypeDescr descr,
                                              int32_t length,
                                              gc::InitialHeap heap)
{
    MOZ_ASSERT(clasp == &OutlineTransparentTypedObject::class_ ||
               clasp == &OutlineOpaqueTypedObject::class_);

    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, clasp,
                                                             TaggedProto(&descr->typedProto()),
                                                             descr));
    if (!group)
        return nullptr;

    NewObjectKind newKind = (heap == gc::TenuredHeap) ? MaybeSingletonObject : GenericObject;
    OutlineTypedObject* obj = NewObjectWithGroup<OutlineTypedObject>(cx, group,
                                                                     gc::AllocKind::OBJECT0,
                                                                     newKind);
    if (!obj)
        return nullptr;

    obj->setOwnerAndData(nullptr, nullptr);
    return obj;
}

void
OutlineTypedObject::attach(JSContext* cx, ArrayBufferObject& buffer, int32_t offset)
{
    MOZ_ASSERT(!isAttached());
    MOZ_ASSERT(offset >= 0);
    MOZ_ASSERT((size_t) (offset + size()) <= buffer.byteLength());

    
    
    if (buffer.forInlineTypedObject()) {
        InlineTypedObject& realOwner = buffer.firstView()->as<InlineTypedObject>();
        attach(cx, realOwner, offset);
        return;
    }

    buffer.setHasTypedObjectViews();

    if (!buffer.addView(cx, this))
        CrashAtUnhandlableOOM("TypedObject::attach");

    setOwnerAndData(&buffer, buffer.dataPointer() + offset);
}

void
OutlineTypedObject::attach(JSContext* cx, TypedObject& typedObj, int32_t offset)
{
    MOZ_ASSERT(!isAttached());
    MOZ_ASSERT(typedObj.isAttached());

    JSObject* owner = &typedObj;
    if (typedObj.is<OutlineTypedObject>()) {
        owner = &typedObj.as<OutlineTypedObject>().owner();
        offset += typedObj.offset();
    }

    if (owner->is<ArrayBufferObject>()) {
        attach(cx, owner->as<ArrayBufferObject>(), offset);
    } else {
        MOZ_ASSERT(owner->is<InlineTypedObject>());
        setOwnerAndData(owner, owner->as<InlineTypedObject>().inlineTypedMem() + offset);
    }
}



static int32_t
TypedObjLengthFromType(TypeDescr& descr)
{
    switch (descr.kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Struct:
      case type::Simd:
        return 0;

      case type::Array:
        return descr.as<ArrayTypeDescr>().length();
    }
    MOZ_CRASH("Invalid kind");
}

 OutlineTypedObject*
OutlineTypedObject::createDerived(JSContext* cx, HandleTypeDescr type,
                                  HandleTypedObject typedObj, int32_t offset)
{
    MOZ_ASSERT(offset <= typedObj->size());
    MOZ_ASSERT(offset + type->size() <= typedObj->size());

    int32_t length = TypedObjLengthFromType(*type);

    const js::Class* clasp = typedObj->opaque()
                             ? &OutlineOpaqueTypedObject::class_
                             : &OutlineTransparentTypedObject::class_;
    Rooted<OutlineTypedObject*> obj(cx);
    obj = createUnattachedWithClass(cx, clasp, type, length);
    if (!obj)
        return nullptr;

    obj->attach(cx, *typedObj, offset);
    return obj;
}

 TypedObject*
TypedObject::createZeroed(JSContext* cx, HandleTypeDescr descr, int32_t length, gc::InitialHeap heap)
{
    
    if ((size_t) descr->size() <= InlineTypedObject::MaximumSize) {
        InlineTypedObject* obj = InlineTypedObject::create(cx, descr, heap);
        if (!obj)
            return nullptr;
        descr->initInstances(cx->runtime(), obj->inlineTypedMem(), 1);
        return obj;
    }

    
    Rooted<OutlineTypedObject*> obj(cx, OutlineTypedObject::createUnattached(cx, descr, length, heap));
    if (!obj)
        return nullptr;

    
    size_t totalSize = descr->size();
    Rooted<ArrayBufferObject*> buffer(cx);
    buffer = ArrayBufferObject::create(cx, totalSize);
    if (!buffer)
        return nullptr;
    descr->initInstances(cx->runtime(), buffer->dataPointer(), 1);
    obj->attach(cx, *buffer, 0);
    return obj;
}

static bool
ReportTypedObjTypeError(JSContext* cx,
                        const unsigned errorNumber,
                        HandleTypedObject obj)
{
    
    char* typeReprStr = JS_EncodeString(cx, &obj->typeDescr().stringRepr());
    if (!typeReprStr)
        return false;

    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                         errorNumber, typeReprStr);

    JS_free(cx, (void*) typeReprStr);
    return false;
}

 void
OutlineTypedObject::obj_trace(JSTracer* trc, JSObject* object)
{
    OutlineTypedObject& typedObj = object->as<OutlineTypedObject>();

    TraceEdge(trc, &typedObj.shape_, "OutlineTypedObject_shape");

    if (!typedObj.owner_)
        return;

    
    
    
    
    TypeDescr& descr = typedObj.maybeForwardedTypeDescr();

    
    JSObject* oldOwner = typedObj.owner_;
    gc::MarkObjectUnbarriered(trc, &typedObj.owner_, "typed object owner");
    JSObject* owner = typedObj.owner_;

    uint8_t* oldData = typedObj.outOfLineTypedMem();
    uint8_t* newData = oldData;

    
    
    
    
    MOZ_ASSERT_IF(owner->is<ArrayBufferObject>(),
                  !owner->as<ArrayBufferObject>().forInlineTypedObject());
    if (owner != oldOwner &&
        (owner->is<InlineTypedObject>() ||
         owner->as<ArrayBufferObject>().hasInlineData()))
    {
        newData += reinterpret_cast<uint8_t*>(owner) - reinterpret_cast<uint8_t*>(oldOwner);
        typedObj.setData(newData);

        trc->runtime()->gc.nursery.maybeSetForwardingPointer(trc, oldData, newData,  false);
    }

    if (!descr.opaque() || !typedObj.maybeForwardedIsAttached())
        return;

    descr.traceInstances(trc, newData, 1);
}

bool
TypeDescr::hasProperty(const JSAtomState& names, jsid id)
{
    switch (kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
        return false;

      case type::Array:
      {
        uint32_t index;
        return IdIsIndex(id, &index) || JSID_IS_ATOM(id, names.length);
      }

      case type::Struct:
      {
        size_t index;
        return as<StructTypeDescr>().fieldIndex(id, &index);
      }
    }

    MOZ_CRASH("Unexpected kind");
}

 bool
TypedObject::obj_lookupProperty(JSContext* cx, HandleObject obj, HandleId id,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    if (obj->as<TypedObject>().typeDescr().hasProperty(cx->names(), id)) {
        MarkNonNativePropertyFound<CanGC>(propp);
        objp.set(obj);
        return true;
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        objp.set(nullptr);
        propp.set(nullptr);
        return true;
    }

    return LookupProperty(cx, proto, id, objp, propp);
}

static bool
ReportPropertyError(JSContext* cx,
                    const unsigned errorNumber,
                    HandleId id)
{
    RootedValue idVal(cx, IdToValue(id));
    RootedString str(cx, ValueToSource(cx, idVal));
    if (!str)
        return false;

    char* propName = JS_EncodeString(cx, str);
    if (!propName)
        return false;

    JS_ReportErrorNumber(cx, GetErrorMessage, nullptr,
                         errorNumber, propName);

    JS_free(cx, propName);
    return false;
}

bool
TypedObject::obj_defineProperty(JSContext* cx, HandleObject obj, HandleId id,
                                Handle<JSPropertyDescriptor> desc,
                                ObjectOpResult& result)
{
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());
    return ReportTypedObjTypeError(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, typedObj);
}

bool
TypedObject::obj_hasProperty(JSContext* cx, HandleObject obj, HandleId id, bool* foundp)
{
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());
    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
        break;

      case type::Array: {
        if (JSID_IS_ATOM(id, cx->names().length)) {
            *foundp = true;
            return true;
        }
        uint32_t index;
        
        if (IdIsIndex(id, &index)) {
            *foundp = (index < uint32_t(typedObj->length()));
            return true;
        }
        break;
      }

      case type::Struct:
        size_t index;
        if (typedObj->typeDescr().as<StructTypeDescr>().fieldIndex(id, &index)) {
            *foundp = true;
            return true;
        }
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *foundp = false;
        return true;
    }

    return HasProperty(cx, proto, id, foundp);
}

bool
TypedObject::obj_getProperty(JSContext* cx, HandleObject obj, HandleObject receiver,
                             HandleId id, MutableHandleValue vp)
{
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());

    
    uint32_t index;
    if (IdIsIndex(id, &index))
        return obj_getElement(cx, obj, receiver, index, vp);

    

    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
        break;

      case type::Simd:
        break;

      case type::Array:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            if (!typedObj->isAttached()) {
                JS_ReportErrorNumber(
                    cx, GetErrorMessage,
                    nullptr, JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);
                return false;
            }

            vp.setInt32(typedObj->length());
            return true;
        }
        break;

      case type::Struct: {
        Rooted<StructTypeDescr*> descr(cx, &typedObj->typeDescr().as<StructTypeDescr>());

        size_t fieldIndex;
        if (!descr->fieldIndex(id, &fieldIndex))
            break;

        size_t offset = descr->fieldOffset(fieldIndex);
        Rooted<TypeDescr*> fieldType(cx, &descr->fieldDescr(fieldIndex));
        return Reify(cx, fieldType, typedObj, offset, vp);
      }
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return GetProperty(cx, proto, receiver, id, vp);
}

bool
TypedObject::obj_getElement(JSContext* cx, HandleObject obj, HandleObject receiver,
                            uint32_t index, MutableHandleValue vp)
{
    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr*> descr(cx, &typedObj->typeDescr());

    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
      case type::Struct:
        break;

      case type::Array:
        return obj_getArrayElement(cx, typedObj, descr, index, vp);
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return GetElement(cx, proto, receiver, index, vp);
}

 bool
TypedObject::obj_getArrayElement(JSContext* cx,
                                 Handle<TypedObject*> typedObj,
                                 Handle<TypeDescr*> typeDescr,
                                 uint32_t index,
                                 MutableHandleValue vp)
{
    
    if (index >= (size_t) typedObj->length()) {
        vp.setUndefined();
        return true;
    }

    Rooted<TypeDescr*> elementType(cx, &typeDescr->as<ArrayTypeDescr>().elementType());
    size_t offset = elementType->size() * index;
    return Reify(cx, elementType, typedObj, offset, vp);
}

bool
TypedObject::obj_setProperty(JSContext* cx, HandleObject obj, HandleId id, HandleValue v,
                             HandleValue receiver, ObjectOpResult& result)
{
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());

    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
        break;

      case type::Simd:
        break;

      case type::Array: {
        if (JSID_IS_ATOM(id, cx->names().length)) {
            if (receiver.isObject() && obj == &receiver.toObject()) {
                JS_ReportErrorNumber(cx, GetErrorMessage,
                                     nullptr, JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
                return false;
            }
            return result.failReadOnly();
        }

        uint32_t index;
        if (IdIsIndex(id, &index)) {
            if (!receiver.isObject() || obj != &receiver.toObject())
                return SetPropertyByDefining(cx, obj, id, v, receiver, false, result);

            if (index >= uint32_t(typedObj->length())) {
                JS_ReportErrorNumber(cx, GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BINARYARRAY_BAD_INDEX);
                return false;
            }

            Rooted<TypeDescr*> elementType(cx);
            elementType = &typedObj->typeDescr().as<ArrayTypeDescr>().elementType();
            size_t offset = elementType->size() * index;
            if (!ConvertAndCopyTo(cx, elementType, typedObj, offset, NullPtr(), v))
                return false;
            return result.succeed();
        }
        break;
      }

      case type::Struct: {
        Rooted<StructTypeDescr*> descr(cx, &typedObj->typeDescr().as<StructTypeDescr>());

        size_t fieldIndex;
        if (!descr->fieldIndex(id, &fieldIndex))
            break;

        if (!receiver.isObject() || obj != &receiver.toObject())
            return SetPropertyByDefining(cx, obj, id, v, receiver, false, result);

        size_t offset = descr->fieldOffset(fieldIndex);
        Rooted<TypeDescr*> fieldType(cx, &descr->fieldDescr(fieldIndex));
        RootedAtom fieldName(cx, &descr->fieldName(fieldIndex));
        if (!ConvertAndCopyTo(cx, fieldType, typedObj, offset, fieldName, v))
            return false;
        return result.succeed();
      }
    }

    return SetPropertyOnProto(cx, obj, id, v, receiver, result);
}

bool
TypedObject::obj_getOwnPropertyDescriptor(JSContext* cx, HandleObject obj, HandleId id,
                                          MutableHandle<JSPropertyDescriptor> desc)
{
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());
    if (!typedObj->isAttached()) {
        JS_ReportErrorNumber(cx, GetErrorMessage, nullptr, JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);
        return false;
    }

    Rooted<TypeDescr*> descr(cx, &typedObj->typeDescr());
    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
        break;

      case type::Array:
      {
        uint32_t index;
        if (IdIsIndex(id, &index)) {
            if (!obj_getArrayElement(cx, typedObj, descr, index, desc.value()))
                return false;
            desc.setAttributes(JSPROP_ENUMERATE | JSPROP_PERMANENT);
            desc.object().set(obj);
            return true;
        }

        if (JSID_IS_ATOM(id, cx->names().length)) {
            desc.value().setInt32(typedObj->length());
            desc.setAttributes(JSPROP_READONLY | JSPROP_PERMANENT);
            desc.object().set(obj);
            return true;
        }
        break;
      }

      case type::Struct:
      {
        Rooted<StructTypeDescr*> descr(cx, &typedObj->typeDescr().as<StructTypeDescr>());

        size_t fieldIndex;
        if (!descr->fieldIndex(id, &fieldIndex))
            break;

        size_t offset = descr->fieldOffset(fieldIndex);
        Rooted<TypeDescr*> fieldType(cx, &descr->fieldDescr(fieldIndex));
        if (!Reify(cx, fieldType, typedObj, offset, desc.value()))
            return false;

        desc.setAttributes(JSPROP_ENUMERATE | JSPROP_PERMANENT);
        desc.object().set(obj);
        return true;
      }
    }

    desc.object().set(nullptr);
    return true;
}

static bool
IsOwnId(JSContext* cx, HandleObject obj, HandleId id)
{
    uint32_t index;
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());
    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
        return false;

      case type::Array:
        return IdIsIndex(id, &index) || JSID_IS_ATOM(id, cx->names().length);

      case type::Struct:
        size_t index;
        if (typedObj->typeDescr().as<StructTypeDescr>().fieldIndex(id, &index))
            return true;
    }

    return false;
}

bool
TypedObject::obj_deleteProperty(JSContext* cx, HandleObject obj, HandleId id, ObjectOpResult& result)
{
    if (IsOwnId(cx, obj, id))
        return ReportPropertyError(cx, JSMSG_CANT_DELETE, id);

    RootedObject proto(cx, obj->getProto());
    if (!proto)
        return result.succeed();

    return DeleteProperty(cx, proto, id, result);
}

bool
TypedObject::obj_enumerate(JSContext* cx, HandleObject obj, AutoIdVector& properties)
{
    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject*> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr*> descr(cx, &typedObj->typeDescr());

    RootedId id(cx);
    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd: {
        
        break;
      }

      case type::Array: {
        if (!properties.reserve(typedObj->length()))
            return false;

        for (int32_t index = 0; index < typedObj->length(); index++) {
            id = INT_TO_JSID(index);
            properties.infallibleAppend(id);
        }
        break;
      }

      case type::Struct: {
        size_t fieldCount = descr->as<StructTypeDescr>().fieldCount();
        if (!properties.reserve(fieldCount))
            return false;

        for (size_t index = 0; index < fieldCount; index++) {
            id = AtomToId(&descr->as<StructTypeDescr>().fieldName(index));
            properties.infallibleAppend(id);
        }
        break;
      }
    }

    return true;
}

void
OutlineTypedObject::neuter(void* newData)
{
    setData(reinterpret_cast<uint8_t*>(newData));
}





 InlineTypedObject*
InlineTypedObject::create(JSContext* cx, HandleTypeDescr descr, gc::InitialHeap heap)
{
    gc::AllocKind allocKind = allocKindForTypeDescriptor(descr);

    const Class* clasp = descr->opaque()
                         ? &InlineOpaqueTypedObject::class_
                         : &InlineTransparentTypedObject::class_;

    RootedObjectGroup group(cx, ObjectGroup::defaultNewGroup(cx, clasp,
                                                             TaggedProto(&descr->typedProto()),
                                                             descr));
    if (!group)
        return nullptr;

    NewObjectKind newKind = (heap == gc::TenuredHeap) ? MaybeSingletonObject : GenericObject;
    return NewObjectWithGroup<InlineTypedObject>(cx, group, allocKind, newKind);
}

 InlineTypedObject*
InlineTypedObject::createCopy(JSContext* cx, Handle<InlineTypedObject*> templateObject,
                              gc::InitialHeap heap)
{
    Rooted<TypeDescr*> descr(cx, &templateObject->typeDescr());
    InlineTypedObject* res = create(cx, descr, heap);
    if (!res)
        return nullptr;

    memcpy(res->inlineTypedMem(), templateObject->inlineTypedMem(), templateObject->size());
    return res;
}

 void
InlineTypedObject::obj_trace(JSTracer* trc, JSObject* object)
{
    InlineTypedObject& typedObj = object->as<InlineTypedObject>();

    TraceEdge(trc, &typedObj.shape_, "InlineTypedObject_shape");

    
    
    
    if (typedObj.is<InlineTransparentTypedObject>())
        return;

    
    
    TypeDescr& descr = typedObj.maybeForwardedTypeDescr();

    descr.traceInstances(trc, typedObj.inlineTypedMem(), 1);
}

 void
InlineTypedObject::objectMovedDuringMinorGC(JSTracer* trc, JSObject* dst, JSObject* src)
{
    
    
    
    
    TypeDescr& descr = dst->as<InlineTypedObject>().typeDescr();
    if (descr.kind() == type::Array) {
        
        
        
        uint8_t* oldData = reinterpret_cast<uint8_t*>(src) + offsetOfDataStart();
        uint8_t* newData = dst->as<InlineTypedObject>().inlineTypedMem();
        trc->runtime()->gc.nursery.maybeSetForwardingPointer(trc, oldData, newData,
                                                             size_t(descr.size()) >= sizeof(uintptr_t));
    }
}

ArrayBufferObject*
InlineTransparentTypedObject::getOrCreateBuffer(JSContext* cx)
{
    ObjectWeakMap*& table = cx->compartment()->lazyArrayBuffers;
    if (!table) {
        table = cx->new_<ObjectWeakMap>(cx);
        if (!table)
            return nullptr;
    }

    JSObject* obj = table->lookup(this);
    if (obj)
        return &obj->as<ArrayBufferObject>();

    ArrayBufferObject::BufferContents contents =
        ArrayBufferObject::BufferContents::createPlain(inlineTypedMem());
    size_t nbytes = typeDescr().size();

    
    
    gc::AutoSuppressGC suppress(cx);

    ArrayBufferObject* buffer =
        ArrayBufferObject::create(cx, nbytes, contents, ArrayBufferObject::DoesntOwnData);
    if (!buffer)
        return nullptr;

    
    
    
    
    
    JS_ALWAYS_TRUE(buffer->addView(cx, this));

    buffer->setForInlineTypedObject();
    buffer->setHasTypedObjectViews();

    if (!table->add(cx, this, buffer))
        return nullptr;

    if (IsInsideNursery(this)) {
        
        
        cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(buffer);
    }

    return buffer;
}

ArrayBufferObject*
OutlineTransparentTypedObject::getOrCreateBuffer(JSContext* cx)
{
    if (owner().is<ArrayBufferObject>())
        return &owner().as<ArrayBufferObject>();
    return owner().as<InlineTransparentTypedObject>().getOrCreateBuffer(cx);
}





#define DEFINE_TYPEDOBJ_CLASS(Name, Trace)        \
    const Class Name::class_ = {                         \
        # Name,                                          \
        Class::NON_NATIVE | JSCLASS_IMPLEMENTS_BARRIERS, \
        nullptr,        /* addProperty */                \
        nullptr,        /* delProperty */                \
        nullptr,        /* getProperty */                \
        nullptr,        /* setProperty */                \
        nullptr,        /* enumerate   */                \
        nullptr,        /* resolve     */                \
        nullptr,        /* convert     */                \
        nullptr,        /* finalize    */                \
        nullptr,        /* call        */                \
        nullptr,        /* hasInstance */                \
        nullptr,        /* construct   */                \
        Trace,                                           \
        JS_NULL_CLASS_SPEC,                              \
        JS_NULL_CLASS_EXT,                               \
        {                                                \
            TypedObject::obj_lookupProperty,             \
            TypedObject::obj_defineProperty,             \
            TypedObject::obj_hasProperty,                \
            TypedObject::obj_getProperty,                \
            TypedObject::obj_setProperty,                \
            TypedObject::obj_getOwnPropertyDescriptor,   \
            TypedObject::obj_deleteProperty,             \
            nullptr, nullptr, /* watch/unwatch */        \
            nullptr,   /* getElements */                 \
            TypedObject::obj_enumerate,                  \
            nullptr, /* thisObject */                    \
        }                                                \
    }

DEFINE_TYPEDOBJ_CLASS(OutlineTransparentTypedObject, OutlineTypedObject::obj_trace);
DEFINE_TYPEDOBJ_CLASS(OutlineOpaqueTypedObject,      OutlineTypedObject::obj_trace);
DEFINE_TYPEDOBJ_CLASS(InlineTransparentTypedObject,  InlineTypedObject::obj_trace);
DEFINE_TYPEDOBJ_CLASS(InlineOpaqueTypedObject,       InlineTypedObject::obj_trace);

static int32_t
LengthForType(TypeDescr& descr)
{
    switch (descr.kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Struct:
      case type::Simd:
        return 0;

      case type::Array:
        return descr.as<ArrayTypeDescr>().length();
    }

    MOZ_CRASH("Invalid kind");
}

static bool
CheckOffset(int32_t offset,
            int32_t size,
            int32_t alignment,
            int32_t bufferLength)
{
    MOZ_ASSERT(size >= 0);
    MOZ_ASSERT(alignment >= 0);

    
    if (offset < 0)
        return false;

    
    if (offset > bufferLength)
        return false;
    if (offset + size < offset)
        return false;
    if (offset + size > bufferLength)
        return false;

    
    if ((offset % alignment) != 0)
        return false;

    return true;
}

 bool
TypedObject::construct(JSContext* cx, unsigned int argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    MOZ_ASSERT(args.callee().is<TypeDescr>());
    Rooted<TypeDescr*> callee(cx, &args.callee().as<TypeDescr>());

    
    
    
    
    
    

    
    if (args.length() == 0) {
        int32_t length = LengthForType(*callee);
        Rooted<TypedObject*> obj(cx, createZeroed(cx, callee, length));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    
    if (args[0].isObject() && args[0].toObject().is<ArrayBufferObject>()) {
        Rooted<ArrayBufferObject*> buffer(cx);
        buffer = &args[0].toObject().as<ArrayBufferObject>();

        if (callee->opaque() || buffer->isNeutered()) {
            JS_ReportErrorNumber(cx, GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        int32_t offset;
        if (args.length() >= 2 && !args[1].isUndefined()) {
            if (!args[1].isInt32()) {
                JS_ReportErrorNumber(cx, GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
                return false;
            }

            offset = args[1].toInt32();
        } else {
            offset = 0;
        }

        if (args.length() >= 3 && !args[2].isUndefined()) {
            JS_ReportErrorNumber(cx, GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        if (!CheckOffset(offset, callee->size(), callee->alignment(),
                         buffer->byteLength()))
        {
            JS_ReportErrorNumber(cx, GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        Rooted<OutlineTypedObject*> obj(cx);
        obj = OutlineTypedObject::createUnattached(cx, callee, LengthForType(*callee));
        if (!obj)
            return false;

        obj->attach(cx, *buffer, offset);
        args.rval().setObject(*obj);
        return true;
    }

    
    if (args[0].isObject()) {
        
        int32_t length = LengthForType(*callee);
        Rooted<TypedObject*> obj(cx, createZeroed(cx, callee, length));
        if (!obj)
            return false;

        
        if (!ConvertAndCopyTo(cx, obj, args[0]))
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    
    JS_ReportErrorNumber(cx, GetErrorMessage,
                         nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
    return false;
}





bool
js::NewOpaqueTypedObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypeDescr>());

    Rooted<TypeDescr*> descr(cx, &args[0].toObject().as<TypeDescr>());
    int32_t length = TypedObjLengthFromType(*descr);
    Rooted<OutlineTypedObject*> obj(cx);
    obj = OutlineTypedObject::createUnattachedWithClass(cx, &OutlineOpaqueTypedObject::class_, descr, length);
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

bool
js::NewDerivedTypedObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypeDescr>());
    MOZ_ASSERT(args[1].isObject() && args[1].toObject().is<TypedObject>());
    MOZ_ASSERT(args[2].isInt32());

    Rooted<TypeDescr*> descr(cx, &args[0].toObject().as<TypeDescr>());
    Rooted<TypedObject*> typedObj(cx, &args[1].toObject().as<TypedObject>());
    int32_t offset = args[2].toInt32();

    Rooted<TypedObject*> obj(cx);
    obj = OutlineTypedObject::createDerived(cx, descr, typedObj, offset);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

bool
js::AttachTypedObject(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);
    MOZ_ASSERT(args[2].isInt32());

    OutlineTypedObject& handle = args[0].toObject().as<OutlineTypedObject>();
    TypedObject& target = args[1].toObject().as<TypedObject>();
    MOZ_ASSERT(!handle.isAttached());
    size_t offset = args[2].toInt32();

    handle.attach(cx, target, offset);

    return true;
}

bool
js::SetTypedObjectOffset(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    MOZ_ASSERT(args[1].isInt32());

    OutlineTypedObject& typedObj = args[0].toObject().as<OutlineTypedObject>();
    int32_t offset = args[1].toInt32();

    MOZ_ASSERT(typedObj.isAttached());
    typedObj.setData(typedObj.typedMemBase() + offset);
    args.rval().setUndefined();
    return true;
}

bool
js::ObjectIsTypeDescr(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<TypeDescr>());
    return true;
}

bool
js::ObjectIsTypedObject(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<TypedObject>());
    return true;
}

bool
js::ObjectIsOpaqueTypedObject(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    JSObject& obj = args[0].toObject();
    args.rval().setBoolean(obj.is<TypedObject>() && obj.as<TypedObject>().opaque());
    return true;
}

bool
js::ObjectIsTransparentTypedObject(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    JSObject& obj = args[0].toObject();
    args.rval().setBoolean(obj.is<TypedObject>() && !obj.as<TypedObject>().opaque());
    return true;
}

bool
js::TypeDescrIsSimpleType(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<js::TypeDescr>());
    args.rval().setBoolean(args[0].toObject().is<js::SimpleTypeDescr>());
    return true;
}

bool
js::TypeDescrIsArrayType(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<js::TypeDescr>());
    JSObject& obj = args[0].toObject();
    args.rval().setBoolean(obj.is<js::ArrayTypeDescr>());
    return true;
}

bool
js::TypedObjectIsAttached(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    TypedObject& typedObj = args[0].toObject().as<TypedObject>();
    args.rval().setBoolean(typedObj.isAttached());
    return true;
}

bool
js::TypedObjectTypeDescr(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    TypedObject& typedObj = args[0].toObject().as<TypedObject>();
    args.rval().setObject(typedObj.typeDescr());
    return true;
}

bool
js::ClampToUint8(JSContext*, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isNumber());
    args.rval().setNumber(ClampDoubleToUint8(args[0].toNumber()));
    return true;
}

bool
js::GetTypedObjectModule(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->getTypedObjectModule());
    return true;
}

bool
js::GetFloat32x4TypeDescr(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->float32x4TypeDescr());
    return true;
}

bool
js::GetFloat64x2TypeDescr(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->float64x2TypeDescr());
    return true;
}

bool
js::GetInt32x4TypeDescr(JSContext* cx, unsigned argc, Value* vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->int32x4TypeDescr());
    return true;
}

#define JS_STORE_SCALAR_CLASS_IMPL(_constant, T, _name)                         \
bool                                                                            \
js::StoreScalar##T::Func(JSContext*, unsigned argc, Value* vp)         \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    MOZ_ASSERT(args.length() == 3);                                             \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());     \
    MOZ_ASSERT(args[1].isInt32());                                              \
    MOZ_ASSERT(args[2].isNumber());                                             \
                                                                                \
    TypedObject& typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                   \
                                                                                \
    T* target = reinterpret_cast<T*>(typedObj.typedMem(offset));                \
    double d = args[2].toNumber();                                              \
    *target = ConvertScalar<T>(d);                                              \
    args.rval().setUndefined();                                                 \
    return true;                                                                \
}

#define JS_STORE_REFERENCE_CLASS_IMPL(_constant, T, _name)                      \
bool                                                                            \
js::StoreReference##T::Func(JSContext* cx, unsigned argc, Value* vp)    \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    MOZ_ASSERT(args.length() == 4);                                             \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());     \
    MOZ_ASSERT(args[1].isInt32());                                              \
    MOZ_ASSERT(args[2].isString() || args[2].isNull());                         \
                                                                                \
    TypedObject& typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    jsid id = args[2].isString()                                                \
              ? IdToTypeId(AtomToId(&args[2].toString()->asAtom()))             \
              : JSID_VOID;                                                      \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                   \
                                                                                \
    T* target = reinterpret_cast<T*>(typedObj.typedMem(offset));                \
    if (!store(cx, target, args[3], &typedObj, id))                             \
        return false;                                                           \
    args.rval().setUndefined();                                                 \
    return true;                                                                \
}

#define JS_LOAD_SCALAR_CLASS_IMPL(_constant, T, _name)                                  \
bool                                                                                    \
js::LoadScalar##T::Func(JSContext*, unsigned argc, Value* vp)                  \
{                                                                                       \
    CallArgs args = CallArgsFromVp(argc, vp);                                           \
    MOZ_ASSERT(args.length() == 2);                                                     \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());             \
    MOZ_ASSERT(args[1].isInt32());                                                      \
                                                                                        \
    TypedObject& typedObj = args[0].toObject().as<TypedObject>();                       \
    int32_t offset = args[1].toInt32();                                                 \
                                                                                        \
    /* Should be guaranteed by the typed objects API: */                                \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                           \
                                                                                        \
    T* target = reinterpret_cast<T*>(typedObj.typedMem(offset));                        \
    args.rval().setNumber((double) *target);                                            \
    return true;                                                                        \
}

#define JS_LOAD_REFERENCE_CLASS_IMPL(_constant, T, _name)                       \
bool                                                                            \
js::LoadReference##T::Func(JSContext*, unsigned argc, Value* vp)       \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    MOZ_ASSERT(args.length() == 2);                                             \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());     \
    MOZ_ASSERT(args[1].isInt32());                                              \
                                                                                \
    TypedObject& typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                   \
                                                                                \
    T* target = reinterpret_cast<T*>(typedObj.typedMem(offset));                \
    load(target, args.rval());                                                  \
    return true;                                                                \
}





bool
StoreReferenceHeapValue::store(JSContext* cx, HeapValue* heap, const Value& v,
                               TypedObject* obj, jsid id)
{
    
    
    
    if (!v.isUndefined()) {
        if (cx->isJSContext())
            AddTypePropertyId(cx->asJSContext(), obj, id, v);
        else if (!HasTypePropertyId(obj, id, v))
            return false;
    }

    *heap = v;
    return true;
}

bool
StoreReferenceHeapPtrObject::store(JSContext* cx, HeapPtrObject* heap, const Value& v,
                                   TypedObject* obj, jsid id)
{
    MOZ_ASSERT(v.isObjectOrNull()); 

    
    
    
    if (v.isObject()) {
        if (cx->isJSContext())
            AddTypePropertyId(cx->asJSContext(), obj, id, v);
        else if (!HasTypePropertyId(obj, id, v))
            return false;
    }

    *heap = v.toObjectOrNull();
    return true;
}

bool
StoreReferenceHeapPtrString::store(JSContext* cx, HeapPtrString* heap, const Value& v,
                                   TypedObject* obj, jsid id)
{
    MOZ_ASSERT(v.isString()); 

    
    *heap = v.toString();

    return true;
}

void
LoadReferenceHeapValue::load(HeapValue* heap,
                             MutableHandleValue v)
{
    v.set(*heap);
}

void
LoadReferenceHeapPtrObject::load(HeapPtrObject* heap,
                                 MutableHandleValue v)
{
    if (*heap)
        v.setObject(**heap);
    else
        v.setNull();
}

void
LoadReferenceHeapPtrString::load(HeapPtrString* heap,
                                 MutableHandleValue v)
{
    v.setString(*heap);
}



JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_STORE_SCALAR_CLASS_IMPL)
JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_LOAD_SCALAR_CLASS_IMPL)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_STORE_REFERENCE_CLASS_IMPL)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_LOAD_REFERENCE_CLASS_IMPL)




template<typename V>
static void
visitReferences(TypeDescr& descr,
                uint8_t* mem,
                V& visitor)
{
    if (descr.transparent())
        return;

    switch (descr.kind()) {
      case type::Scalar:
      case type::Simd:
        return;

      case type::Reference:
        visitor.visitReference(descr.as<ReferenceTypeDescr>(), mem);
        return;

      case type::Array:
      {
        ArrayTypeDescr& arrayDescr = descr.as<ArrayTypeDescr>();
        TypeDescr& elementDescr = arrayDescr.maybeForwardedElementType();
        for (int32_t i = 0; i < arrayDescr.length(); i++) {
            visitReferences(elementDescr, mem, visitor);
            mem += elementDescr.size();
        }
        return;
      }

      case type::Struct:
      {
        StructTypeDescr& structDescr = descr.as<StructTypeDescr>();
        for (size_t i = 0; i < structDescr.maybeForwardedFieldCount(); i++) {
            TypeDescr& descr = structDescr.maybeForwardedFieldDescr(i);
            size_t offset = structDescr.maybeForwardedFieldOffset(i);
            visitReferences(descr, mem + offset, visitor);
        }
        return;
      }
    }

    MOZ_CRASH("Invalid type repr kind");
}




namespace {

class MemoryInitVisitor {
    const JSRuntime* rt_;

  public:
    explicit MemoryInitVisitor(const JSRuntime* rt)
      : rt_(rt)
    {}

    void visitReference(ReferenceTypeDescr& descr, uint8_t* mem);
};

} 

void
MemoryInitVisitor::visitReference(ReferenceTypeDescr& descr, uint8_t* mem)
{
    switch (descr.type()) {
      case ReferenceTypeDescr::TYPE_ANY:
      {
        js::HeapValue* heapValue = reinterpret_cast<js::HeapValue*>(mem);
        heapValue->init(UndefinedValue());
        return;
      }

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        js::HeapPtrObject* objectPtr =
            reinterpret_cast<js::HeapPtrObject*>(mem);
        objectPtr->init(nullptr);
        return;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        js::HeapPtrString* stringPtr =
            reinterpret_cast<js::HeapPtrString*>(mem);
        stringPtr->init(rt_->emptyString);
        return;
      }
    }

    MOZ_CRASH("Invalid kind");
}

void
TypeDescr::initInstances(const JSRuntime* rt, uint8_t* mem, size_t length)
{
    MOZ_ASSERT(length >= 1);

    MemoryInitVisitor visitor(rt);

    
    memset(mem, 0, size());
    if (opaque())
        visitReferences(*this, mem, visitor);

    
    uint8_t* target = mem;
    for (size_t i = 1; i < length; i++) {
        target += size();
        memcpy(target, mem, size());
    }
}




namespace {

class MemoryTracingVisitor {
    JSTracer* trace_;

  public:

    explicit MemoryTracingVisitor(JSTracer* trace)
      : trace_(trace)
    {}

    void visitReference(ReferenceTypeDescr& descr, uint8_t* mem);
};

} 

void
MemoryTracingVisitor::visitReference(ReferenceTypeDescr& descr, uint8_t* mem)
{
    switch (descr.type()) {
      case ReferenceTypeDescr::TYPE_ANY:
      {
        HeapValue* heapValue = reinterpret_cast<js::HeapValue*>(mem);
        TraceEdge(trace_, heapValue, "reference-val");
        return;
      }

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        HeapPtrObject* objectPtr = reinterpret_cast<js::HeapPtrObject*>(mem);
        if (*objectPtr)
            gc::MarkObject(trace_, objectPtr, "reference-obj");
        return;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        HeapPtrString* stringPtr = reinterpret_cast<js::HeapPtrString*>(mem);
        if (*stringPtr)
            gc::MarkString(trace_, stringPtr, "reference-str");
        return;
      }
    }

    MOZ_CRASH("Invalid kind");
}

void
TypeDescr::traceInstances(JSTracer* trace, uint8_t* mem, size_t length)
{
    MemoryTracingVisitor visitor(trace);

    for (size_t i = 0; i < length; i++) {
        visitReferences(*this, mem, visitor);
        mem += size();
    }
}

namespace {

struct TraceListVisitor {
    typedef Vector<int32_t, 0, SystemAllocPolicy> VectorType;
    VectorType stringOffsets, objectOffsets, valueOffsets;

    void visitReference(ReferenceTypeDescr& descr, uint8_t* mem);

    bool fillList(Vector<int32_t>& entries);
};

} 

void
TraceListVisitor::visitReference(ReferenceTypeDescr& descr, uint8_t* mem)
{
    VectorType* offsets;
    switch (descr.type()) {
      case ReferenceTypeDescr::TYPE_ANY: offsets = &valueOffsets; break;
      case ReferenceTypeDescr::TYPE_OBJECT: offsets = &objectOffsets; break;
      case ReferenceTypeDescr::TYPE_STRING: offsets = &stringOffsets; break;
      default: MOZ_CRASH("Invalid kind");
    }

    if (!offsets->append((uintptr_t) mem))
        CrashAtUnhandlableOOM("TraceListVisitor::visitReference");
}

bool
TraceListVisitor::fillList(Vector<int32_t>& entries)
{
    return entries.appendAll(stringOffsets) &&
           entries.append(-1) &&
           entries.appendAll(objectOffsets) &&
           entries.append(-1) &&
           entries.appendAll(valueOffsets) &&
           entries.append(-1);
}

static bool
CreateTraceList(JSContext* cx, HandleTypeDescr descr)
{
    
    
    
    
    if ((size_t) descr->size() > InlineTypedObject::MaximumSize || descr->transparent())
        return true;

    TraceListVisitor visitor;
    visitReferences(*descr, nullptr, visitor);

    Vector<int32_t> entries(cx);
    if (!visitor.fillList(entries))
        return false;

    
    MOZ_ASSERT(entries.length() >= 3);
    if (entries.length() == 3)
        return true;

    int32_t* list = cx->pod_malloc<int32_t>(entries.length());
    if (!list)
        return false;

    PodCopy(list, entries.begin(), entries.length());

    descr->initReservedSlot(JS_DESCR_SLOT_TRACE_LIST, PrivateValue(list));
    return true;
}

 void
TypeDescr::finalize(FreeOp* fop, JSObject* obj)
{
    if (obj->as<TypeDescr>().hasTraceList())
        js_free(const_cast<int32_t*>(obj->as<TypeDescr>().traceList()));
}
