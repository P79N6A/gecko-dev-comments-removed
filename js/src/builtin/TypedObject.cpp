





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

const Class js::TypedObjectModuleObject::class_ = {
    "TypedObject",
    JSCLASS_HAS_RESERVED_SLOTS(SlotCount) |
    JSCLASS_HAS_CACHED_PROTO(JSProto_TypedObject),
    JS_PropertyStub,         
    JS_DeletePropertyStub,   
    JS_PropertyStub,         
    JS_StrictPropertyStub,   
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub
};

static const JSFunctionSpec TypedObjectMethods[] = {
    JS_SELF_HOSTED_FN("objectType", "TypeOfTypedObject", 1, 0),
    JS_SELF_HOSTED_FN("storage", "StorageOfTypedObject", 1, 0),
    JS_FS_END
};

static void
ReportCannotConvertTo(JSContext *cx, HandleValue fromValue, const char *toType)
{
    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_CANT_CONVERT_TO,
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


























static bool
ConvertAndCopyTo(JSContext *cx,
                 HandleTypeDescr typeObj,
                 HandleTypedObject typedObj,
                 int32_t offset,
                 HandleValue val)
{
    RootedFunction func(
        cx, SelfHostedFunction(cx, cx->names().ConvertAndCopyTo));
    if (!func)
        return false;

    InvokeArgs args(cx);
    if (!args.init(4))
        return false;

    args.setCallee(ObjectValue(*func));
    args[0].setObject(*typeObj);
    args[1].setObject(*typedObj);
    args[2].setInt32(offset);
    args[3].set(val);

    return Invoke(cx, args);
}

static bool
ConvertAndCopyTo(JSContext *cx, HandleTypedObject typedObj, HandleValue val)
{
    Rooted<TypeDescr*> type(cx, &typedObj->typeDescr());
    return ConvertAndCopyTo(cx, type, typedObj, 0, val);
}





static bool
Reify(JSContext *cx,
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



static JSObject *
GetPrototype(JSContext *cx, HandleObject obj)
{
    RootedValue prototypeVal(cx);
    if (!JSObject::getProperty(cx, obj, obj, cx->names().prototype,
                               &prototypeVal))
    {
        return nullptr;
    }
    if (!prototypeVal.isObject()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_INVALID_PROTOTYPE);
        return nullptr;
    }
    return &prototypeVal.toObject();
}





TypeRepresentation *
TypeDescr::typeRepresentation() const {
    return TypeRepresentation::fromOwnerObject(typeRepresentationOwnerObj());
}

TypeDescr::Kind
TypeDescr::kind() const {
    return typeRepresentation()->kind();
}

bool
TypeDescr::opaque() const {
    return typeRepresentation()->opaque();
}










const Class js::ScalarTypeDescr::class_ = {
    "Scalar",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS),
    JS_PropertyStub,       
    JS_DeletePropertyStub, 
    JS_PropertyStub,       
    JS_StrictPropertyStub, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,
    ScalarTypeDescr::call,
    nullptr,
    nullptr,
    nullptr
};

const JSFunctionSpec js::ScalarTypeDescr::typeObjectMethods[] = {
    JS_SELF_HOSTED_FN("toSource", "DescrToSourceMethod", 0, 0),
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_FS_END
};

static size_t ScalarSizes[] = {
#define SCALAR_SIZE(_kind, _type, _name)                        \
    sizeof(_type),
    JS_FOR_EACH_SCALAR_TYPE_REPR(SCALAR_SIZE) 0
#undef SCALAR_SIZE
};

size_t
ScalarTypeDescr::size(Type t)
{
    return ScalarSizes[t];
}

size_t
ScalarTypeDescr::alignment(Type t)
{
    return ScalarSizes[t];
}

 const char *
ScalarTypeDescr::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_, name_) \
        case constant_: return #name_;
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
    }
    MOZ_ASSUME_UNREACHABLE("Invalid type");
}

bool
ScalarTypeDescr::call(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             args.callee().getClass()->name, "0", "s");
        return false;
    }

    ScalarTypeRepresentation *typeRepr =
        args.callee().as<ScalarTypeDescr>().typeRepresentation()->asScalar();
    ScalarTypeDescr::Type type = typeRepr->type();

    double number;
    if (!ToNumber(cx, args[0], &number))
        return false;

    if (type == ScalarTypeDescr::TYPE_UINT8_CLAMPED)
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

    }
    return true;
}











const Class js::ReferenceTypeDescr::class_ = {
    "Reference",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS),
    JS_PropertyStub,       
    JS_DeletePropertyStub, 
    JS_PropertyStub,       
    JS_StrictPropertyStub, 
    JS_EnumerateStub,
    JS_ResolveStub,
    JS_ConvertStub,
    nullptr,
    ReferenceTypeDescr::call,
    nullptr,
    nullptr,
    nullptr
};

const JSFunctionSpec js::ReferenceTypeDescr::typeObjectMethods[] = {
    JS_SELF_HOSTED_FN("toSource", "DescrToSourceMethod", 0, 0),
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_FS_END
};

 const char *
ReferenceTypeDescr::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_, name_) \
        case constant_: return #name_;
        JS_FOR_EACH_REFERENCE_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
    }
    MOZ_ASSUME_UNREACHABLE("Invalid type");
}

bool
js::ReferenceTypeDescr::call(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    JS_ASSERT(args.callee().is<ReferenceTypeDescr>());
    ReferenceTypeRepresentation *typeRepr =
        args.callee().as<ReferenceTypeDescr>().typeRepresentation()->asReference();

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_MORE_ARGS_NEEDED,
                             typeRepr->typeName(), "0", "s");
        return false;
    }

    switch (typeRepr->type()) {
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

    MOZ_ASSUME_UNREACHABLE("Unhandled Reference type");
}








































static JSObject *
CreatePrototypeObjectForComplexTypeInstance(JSContext *cx,
                                            HandleObject ctorPrototype)
{
    RootedObject ctorPrototypePrototype(cx, GetPrototype(cx, ctorPrototype));
    if (!ctorPrototypePrototype)
        return nullptr;

    return NewObjectWithProto<JSObject>(cx, &*ctorPrototypePrototype, nullptr,
                                        TenuredObject);
}

const Class UnsizedArrayTypeDescr::class_ = {
    "ArrayType",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS),
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
    TypedObject::constructUnsized,
    nullptr
};

const Class SizedArrayTypeDescr::class_ = {
    "ArrayType",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS),
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
    TypedObject::constructSized,
    nullptr
};

const JSPropertySpec ArrayMetaTypeDescr::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayMetaTypeDescr::typeObjectMethods[] = {
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    JS_FN("dimension", UnsizedArrayTypeDescr::dimension, 1, 0),
    JS_SELF_HOSTED_FN("toSource", "DescrToSourceMethod", 0, 0),
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_SELF_HOSTED_FN("build",    "TypedObjectArrayTypeBuild", 3, 0),
    JS_SELF_HOSTED_FN("buildPar", "TypedObjectArrayTypeBuildPar", 3, 0),
    JS_SELF_HOSTED_FN("from",     "TypedObjectArrayTypeFrom", 3, 0),
    JS_SELF_HOSTED_FN("fromPar",  "TypedObjectArrayTypeFromPar", 3, 0),
    JS_FS_END
};

const JSPropertySpec ArrayMetaTypeDescr::typedObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayMetaTypeDescr::typedObjectMethods[] = {
    {"forEach", {nullptr, nullptr}, 1, 0, "ArrayForEach"},
    {"redimension", {nullptr, nullptr}, 1, 0, "TypedArrayRedimension"},
    JS_SELF_HOSTED_FN("map",        "TypedArrayMap",        2, 0),
    JS_SELF_HOSTED_FN("mapPar",     "TypedArrayMapPar",     2, 0),
    JS_SELF_HOSTED_FN("reduce",     "TypedArrayReduce",     2, 0),
    JS_SELF_HOSTED_FN("reducePar",  "TypedArrayReducePar",  2, 0),
    JS_SELF_HOSTED_FN("scatter",    "TypedArrayScatter",    4, 0),
    JS_SELF_HOSTED_FN("scatterPar", "TypedArrayScatterPar", 4, 0),
    JS_SELF_HOSTED_FN("filter",     "TypedArrayFilter",     1, 0),
    JS_SELF_HOSTED_FN("filterPar",  "TypedArrayFilterPar",  1, 0),
    JS_FS_END
};

bool
js::InitializeCommonTypeDescriptorProperties(JSContext *cx,
                                             HandleTypeDescr obj,
                                             HandleObject typeReprOwnerObj)
{
    TypeRepresentation *typeRepr =
        TypeRepresentation::fromOwnerObject(*typeReprOwnerObj);

    obj->initReservedSlot(JS_DESCR_SLOT_ALIGNMENT,
                          Int32Value(typeRepr->alignment()));

    
    
    if (typeRepr->isSized()) {
        SizedTypeRepresentation *sizedTypeRepr = typeRepr->asSized();
        obj->initReservedSlot(JS_DESCR_SLOT_SIZE,
                              Int32Value(sizedTypeRepr->size()));
    }

    
    if (typeRepr->transparent() && typeRepr->isSized()) {
        SizedTypeRepresentation *sizedTypeRepr = typeRepr->asSized();

        
        RootedValue typeByteLength(cx, NumberValue(sizedTypeRepr->size()));
        if (!JSObject::defineProperty(cx, obj, cx->names().byteLength,
                                      typeByteLength,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }

        
        RootedValue typeByteAlignment(
            cx, NumberValue(sizedTypeRepr->alignment()));
        if (!JSObject::defineProperty(cx, obj, cx->names().byteAlignment,
                                      typeByteAlignment,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }
    } else {
        
        if (!JSObject::defineProperty(cx, obj, cx->names().byteLength,
                                      UndefinedHandleValue,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }

        
        if (!JSObject::defineProperty(cx, obj, cx->names().byteAlignment,
                                      UndefinedHandleValue,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }
    }

    
    RootedValue variable(cx, BooleanValue(!typeRepr->isSized()));
    if (!JSObject::defineProperty(cx, obj, cx->names().variable,
                                  variable,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return false;
    }

    return true;
}

template<class T>
T *
ArrayMetaTypeDescr::create(JSContext *cx,
                           HandleObject arrayTypePrototype,
                           HandleObject arrayTypeReprObj,
                           HandleSizedTypeDescr elementType)
{
    JS_ASSERT(TypeRepresentation::isOwnerObject(*arrayTypeReprObj));

    Rooted<T*> obj(cx, NewObjectWithProto<T>(cx, arrayTypePrototype, nullptr,
                                             TenuredObject));
    if (!obj)
        return nullptr;
    obj->initReservedSlot(JS_DESCR_SLOT_TYPE_REPR,
                          ObjectValue(*arrayTypeReprObj));

    RootedValue elementTypeVal(cx, ObjectValue(*elementType));
    if (!JSObject::defineProperty(cx, obj, cx->names().elementType,
                                  elementTypeVal, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    obj->initReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE, elementTypeVal);

    if (!InitializeCommonTypeDescriptorProperties(cx, obj, arrayTypeReprObj))
        return nullptr;

    RootedObject prototypeObj(cx);
    prototypeObj = CreatePrototypeObjectForComplexTypeInstance(cx, arrayTypePrototype);
    if (!prototypeObj)
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, obj, prototypeObj))
        return nullptr;

    return obj;
}

bool
ArrayMetaTypeDescr::construct(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_NOT_FUNCTION, "ArrayType");
        return false;
    }

    RootedObject arrayTypeGlobal(cx, &args.callee());

    
    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr, JSMSG_MORE_ARGS_NEEDED,
                             "ArrayType", "0", "");
        return false;
    }

    if (!args[0].isObject() || !args[0].toObject().is<SizedTypeDescr>()) {
        ReportCannotConvertTo(cx, args[0], "ArrayType element specifier");
        return false;
    }

    Rooted<SizedTypeDescr*> elementType(cx);
    elementType = &args[0].toObject().as<SizedTypeDescr>();
    SizedTypeRepresentation *elementTypeRepr =
        elementType->typeRepresentation()->asSized();

    
    RootedObject arrayTypeReprObj(
        cx, UnsizedArrayTypeRepresentation::Create(cx, elementTypeRepr));
    if (!arrayTypeReprObj)
        return false;

    
    RootedObject arrayTypePrototype(cx, GetPrototype(cx, arrayTypeGlobal));
    if (!arrayTypePrototype)
        return nullptr;

    
    Rooted<UnsizedArrayTypeDescr *> obj(cx);
    obj = create<UnsizedArrayTypeDescr>(cx, arrayTypePrototype,
                                        arrayTypeReprObj, elementType);
    if (!obj)
        return false;

    
    if (!JSObject::defineProperty(cx, obj, cx->names().length,
                                  UndefinedHandleValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    args.rval().setObject(*obj);
    return true;
}

 bool
UnsizedArrayTypeDescr::dimension(JSContext *cx, unsigned int argc, jsval *vp)
{
    
    
    CallArgs args = CallArgsFromVp(argc, vp);
    if (args.length() != 1 ||
        !args.thisv().isObject() ||
        !args.thisv().toObject().is<UnsizedArrayTypeDescr>() ||
        !args[0].isInt32() ||
        args[0].toInt32() < 0)
    {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);
        return false;
    }

    
    Rooted<UnsizedArrayTypeDescr*> unsizedTypeDescr(cx);
    unsizedTypeDescr = &args.thisv().toObject().as<UnsizedArrayTypeDescr>();
    UnsizedArrayTypeRepresentation *unsizedTypeRepr =
        unsizedTypeDescr->typeRepresentation()->asUnsizedArray();
    int32_t length = args[0].toInt32();

    
    RootedObject sizedTypeReprObj(cx);
    sizedTypeReprObj =
        SizedArrayTypeRepresentation::Create(cx, unsizedTypeRepr->element(),
                                             length);
    if (!sizedTypeReprObj)
        return false;

    
    Rooted<SizedTypeDescr*> elementType(cx, &unsizedTypeDescr->elementType());
    Rooted<SizedArrayTypeDescr*> obj(cx);
    obj = ArrayMetaTypeDescr::create<SizedArrayTypeDescr>(
        cx, unsizedTypeDescr, sizedTypeReprObj, elementType);
    if (!obj)
        return false;

    obj->initReservedSlot(JS_DESCR_SLOT_SIZED_ARRAY_LENGTH, Int32Value(length));

    
    RootedValue lengthVal(cx, Int32Value(length));
    if (!JSObject::defineProperty(cx, obj, cx->names().length,
                                  lengthVal, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    
    
    RootedValue unsizedTypeDescrValue(cx, ObjectValue(*unsizedTypeDescr));
    if (!JSObject::defineProperty(cx, obj, cx->names().unsized,
                                  unsizedTypeDescrValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    args.rval().setObject(*obj);
    return true;
}





const Class StructTypeDescr::class_ = {
    "StructType",
    JSCLASS_HAS_RESERVED_SLOTS(JS_DESCR_SLOTS) |
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
    TypedObject::constructSized,
    nullptr  
};

const JSPropertySpec StructMetaTypeDescr::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec StructMetaTypeDescr::typeObjectMethods[] = {
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    JS_SELF_HOSTED_FN("toSource", "DescrToSourceMethod", 0, 0),
    {"equivalent", {nullptr, nullptr}, 1, 0, "TypeDescrEquivalent"},
    JS_FS_END
};

const JSPropertySpec StructMetaTypeDescr::typedObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec StructMetaTypeDescr::typedObjectMethods[] = {
    JS_FS_END
};






bool
StructMetaTypeDescr::layout(JSContext *cx,
                            HandleStructTypeDescr structType,
                            HandleObject fields)
{
    AutoIdVector ids(cx);
    if (!GetPropertyNames(cx, fields, JSITER_OWNONLY, &ids))
        return false;

    AutoPropertyNameVector fieldNames(cx);
    AutoValueVector fieldTypeObjs(cx);
    AutoObjectVector fieldTypeReprObjs(cx);

    RootedValue fieldTypeVal(cx);
    RootedId id(cx);
    for (unsigned int i = 0; i < ids.length(); i++) {
        id = ids[i];

        
        uint32_t unused;
        if (!JSID_IS_ATOM(id) || JSID_TO_ATOM(id)->isIndex(&unused)) {
            RootedValue idValue(cx, IdToValue(id));
            ReportCannotConvertTo(cx, idValue, "StructType field name");
            return false;
        }

        if (!fieldNames.append(JSID_TO_ATOM(id)->asPropertyName())) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        if (!JSObject::getGeneric(cx, fields, fields, id, &fieldTypeVal))
            return false;

        Rooted<SizedTypeDescr*> fieldType(cx);
        fieldType = ToObjectIf<SizedTypeDescr>(fieldTypeVal);
        if (!fieldType) {
            ReportCannotConvertTo(cx, fieldTypeVal, "StructType field specifier");
            return false;
        }

        if (!fieldTypeObjs.append(fieldTypeVal)) {
            js_ReportOutOfMemory(cx);
            return false;
        }

        if (!fieldTypeReprObjs.append(&fieldType->typeRepresentationOwnerObj())) {
            js_ReportOutOfMemory(cx);
            return false;
        }
    }

    
    RootedObject typeReprObj(cx);
    typeReprObj = StructTypeRepresentation::Create(cx, fieldNames, fieldTypeReprObjs);
    if (!typeReprObj)
        return false;
    StructTypeRepresentation *typeRepr =
        TypeRepresentation::fromOwnerObject(*typeReprObj)->asStruct();
    structType->initReservedSlot(JS_DESCR_SLOT_TYPE_REPR,
                                 ObjectValue(*typeReprObj));

    
    {
        AutoValueVector fieldNameValues(cx);
        for (unsigned int i = 0; i < ids.length(); i++) {
            RootedValue value(cx, IdToValue(ids[i]));
            if (!fieldNameValues.append(value))
                return false;
        }
        RootedObject fieldNamesVec(cx);
        fieldNamesVec = NewDenseCopiedArray(cx, fieldNameValues.length(),
                                            fieldNameValues.begin(), nullptr,
                                            TenuredObject);
        if (!fieldNamesVec)
            return false;
        structType->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_NAMES,
                                     ObjectValue(*fieldNamesVec));
    }

    
    {
        RootedObject fieldTypeVec(cx);
        fieldTypeVec = NewDenseCopiedArray(cx, fieldTypeObjs.length(),
                                           fieldTypeObjs.begin(), nullptr,
                                           TenuredObject);
        if (!fieldTypeVec)
            return false;
        structType->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_TYPES,
                                     ObjectValue(*fieldTypeVec));
    }

    
    {
        AutoValueVector fieldOffsets(cx);
        for (size_t i = 0; i < typeRepr->fieldCount(); i++) {
            const StructField &field = typeRepr->field(i);
            if (!fieldOffsets.append(Int32Value(field.offset)))
                return false;
        }
        RootedObject fieldOffsetsVec(cx);
        fieldOffsetsVec = NewDenseCopiedArray(cx, fieldOffsets.length(),
                                              fieldOffsets.begin(), nullptr,
                                              TenuredObject);
        if (!fieldOffsetsVec)
            return false;
        structType->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS,
                                     ObjectValue(*fieldOffsetsVec));
    }

    
    
    
    RootedObject fieldOffsets(cx);
    fieldOffsets = NewObjectWithProto<JSObject>(cx, nullptr, nullptr, TenuredObject);
    if (!fieldOffsets)
        return false;
    RootedObject fieldTypes(cx);
    fieldTypes = NewObjectWithProto<JSObject>(cx, nullptr, nullptr, TenuredObject);
    if (!fieldTypes)
        return false;
    for (size_t i = 0; i < typeRepr->fieldCount(); i++) {
        const StructField &field = typeRepr->field(i);
        RootedId fieldId(cx, NameToId(field.propertyName));

        
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

    if (!JSObject::freeze(cx, fieldOffsets))
        return false;

    if (!JSObject::freeze(cx, fieldTypes))
        return false;

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
StructMetaTypeDescr::create(JSContext *cx,
                            HandleObject metaTypeDescr,
                            HandleObject fields)
{
    RootedObject structTypePrototype(cx, GetPrototype(cx, metaTypeDescr));
    if (!structTypePrototype)
        return nullptr;

    Rooted<StructTypeDescr*> descr(cx);
    descr = NewObjectWithProto<StructTypeDescr>(cx, structTypePrototype, nullptr,
                                                TenuredObject);
    if (!descr)
        return nullptr;

    if (!StructMetaTypeDescr::layout(cx, descr, fields))
        return nullptr;

    RootedObject fieldsProto(cx);
    if (!JSObject::getProto(cx, fields, &fieldsProto))
        return nullptr;

    RootedObject typeReprObj(cx, &descr->typeRepresentationOwnerObj());
    if (!InitializeCommonTypeDescriptorProperties(cx, descr, typeReprObj))
        return nullptr;

    RootedObject prototypeObj(
        cx, CreatePrototypeObjectForComplexTypeInstance(cx, structTypePrototype));
    if (!prototypeObj)
        return nullptr;

    if (!LinkConstructorAndPrototype(cx, descr, prototypeObj))
        return nullptr;

    return descr;
}

bool
StructMetaTypeDescr::construct(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    if (!args.isConstructing()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
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

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                         JSMSG_TYPEDOBJECT_STRUCTTYPE_BAD_ARGS);
    return false;
}

bool
StructTypeDescr::fieldIndex(jsid id, size_t *out)
{
    JSObject &fieldNames =
        getReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_NAMES).toObject();
    size_t l = fieldNames.getDenseInitializedLength();
    for (size_t i = 0; i < l; i++) {
        JSAtom &a = fieldNames.getDenseElement(i).toString()->asAtom();
        if (JSID_IS_ATOM(id, &a)) {
            *out = i;
            return true;
        }
    }
    return false;
}

size_t
StructTypeDescr::fieldOffset(size_t index)
{
    JSObject &fieldOffsets =
        getReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS).toObject();
    JS_ASSERT(index < fieldOffsets.getDenseInitializedLength());
    return fieldOffsets.getDenseElement(index).toInt32();
}

SizedTypeDescr&
StructTypeDescr::fieldDescr(size_t index)
{
    JSObject &fieldDescrs =
        getReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_TYPES).toObject();
    JS_ASSERT(index < fieldDescrs.getDenseInitializedLength());
    return fieldDescrs.getDenseElement(index).toObject().as<SizedTypeDescr>();
}

















































template<typename T>
static bool
DefineSimpleTypeDescr(JSContext *cx,
                      Handle<GlobalObject *> global,
                      HandleObject module,
                      typename T::Type type,
                      HandlePropertyName className)
{
    RootedObject funcProto(cx, global->getOrCreateFunctionPrototype(cx));
    JS_ASSERT(funcProto);

    Rooted<T*> numFun(cx, NewObjectWithProto<T>(cx, funcProto, global,
                                                TenuredObject));
    if (!numFun)
        return false;

    RootedObject typeReprObj(cx, T::TypeRepr::Create(cx, type));
    if (!typeReprObj)
        return false;

    numFun->initReservedSlot(JS_DESCR_SLOT_TYPE_REPR,
                             ObjectValue(*typeReprObj));

    if (!InitializeCommonTypeDescriptorProperties(cx, numFun, typeReprObj))
        return false;

    numFun->initReservedSlot(JS_DESCR_SLOT_TYPE, Int32Value(type));

    if (!JS_DefineFunctions(cx, numFun, T::typeObjectMethods))
        return false;

    RootedValue numFunValue(cx, ObjectValue(*numFun));
    if (!JSObject::defineProperty(cx, module, className,
                                  numFunValue, nullptr, nullptr, 0))
    {
        return false;
    }

    return true;
}



template<typename T>
static JSObject *
DefineMetaTypeDescr(JSContext *cx,
                    Handle<GlobalObject*> global,
                    HandleObject module,
                    TypedObjectModuleObject::Slot protoSlot)
{
    RootedAtom className(cx, Atomize(cx, T::class_.name,
                                     strlen(T::class_.name)));
    if (!className)
        return nullptr;

    RootedObject funcProto(cx, global->getOrCreateFunctionPrototype(cx));
    if (!funcProto)
        return nullptr;

    

    RootedObject proto(cx, NewObjectWithProto<JSObject>(cx, funcProto, global,
                                                        SingletonObject));
    if (!proto)
        return nullptr;

    

    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return nullptr;
    RootedObject protoProto(cx);
    protoProto = NewObjectWithProto<JSObject>(cx, objProto,
                                              global, SingletonObject);
    if (!proto)
        return nullptr;

    RootedValue protoProtoValue(cx, ObjectValue(*protoProto));
    if (!JSObject::defineProperty(cx, proto, cx->names().prototype,
                                  protoProtoValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    

    const int constructorLength = 2;
    RootedFunction ctor(cx);
    ctor = global->createConstructor(cx, T::construct, className, constructorLength);
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

    module->initReservedSlot(protoSlot, ObjectValue(*proto));

    return ctor;
}







bool
GlobalObject::initTypedObjectModule(JSContext *cx, Handle<GlobalObject*> global)
{
    RootedObject objProto(cx, global->getOrCreateObjectPrototype(cx));
    if (!objProto)
        return false;

    Rooted<TypedObjectModuleObject*> module(cx);
    module = NewObjectWithProto<TypedObjectModuleObject>(cx, objProto, global);
    if (!module)
        return false;

    if (!JS_DefineFunctions(cx, module, TypedObjectMethods))
        return false;

    

#define BINARYDATA_SCALAR_DEFINE(constant_, type_, name_)                       \
    if (!DefineSimpleTypeDescr<ScalarTypeDescr>(cx, global, module, constant_,      \
                                            cx->names().name_))                 \
        return nullptr;
    JS_FOR_EACH_SCALAR_TYPE_REPR(BINARYDATA_SCALAR_DEFINE)
#undef BINARYDATA_SCALAR_DEFINE

#define BINARYDATA_REFERENCE_DEFINE(constant_, type_, name_)                    \
    if (!DefineSimpleTypeDescr<ReferenceTypeDescr>(cx, global, module, constant_,   \
                                               cx->names().name_))              \
        return nullptr;
    JS_FOR_EACH_REFERENCE_TYPE_REPR(BINARYDATA_REFERENCE_DEFINE)
#undef BINARYDATA_REFERENCE_DEFINE

    

    RootedObject arrayType(cx);
    arrayType = DefineMetaTypeDescr<ArrayMetaTypeDescr>(
        cx, global, module, TypedObjectModuleObject::ArrayTypePrototype);
    if (!arrayType)
        return nullptr;

    RootedValue arrayTypeValue(cx, ObjectValue(*arrayType));
    if (!JSObject::defineProperty(cx, module, cx->names().ArrayType,
                                  arrayTypeValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    

    RootedObject structType(cx);
    structType = DefineMetaTypeDescr<StructMetaTypeDescr>(
        cx, global, module, TypedObjectModuleObject::StructTypePrototype);
    if (!structType)
        return nullptr;

    RootedValue structTypeValue(cx, ObjectValue(*structType));
    if (!JSObject::defineProperty(cx, module, cx->names().StructType,
                                  structTypeValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    
    RootedValue moduleValue(cx, ObjectValue(*module));
    global->setConstructor(JSProto_TypedObject, moduleValue);
    if (!JSObject::defineProperty(cx, global, cx->names().TypedObject,
                                  moduleValue,
                                  nullptr, nullptr,
                                  0))
    {
        return nullptr;
    }

    return module;
}

JSObject *
js_InitTypedObjectModuleObject(JSContext *cx, HandleObject obj)
{
    JS_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());
    return global->getOrCreateTypedObjectModule(cx);
}

JSObject *
js_InitTypedObjectDummy(JSContext *cx, HandleObject obj)
{
    





    MOZ_ASSUME_UNREACHABLE("shouldn't be initializing TypedObject via the JSProtoKey initializer mechanism");
}














 bool
TypedObjectModuleObject::getSuitableClaspAndProto(JSContext *cx,
                                                  TypeDescr::Kind kind,
                                                  const Class **clasp,
                                                  MutableHandleObject proto)
{
    Rooted<GlobalObject *> global(cx, cx->global());
    JS_ASSERT(global);
    switch (kind) {
      case TypeDescr::Scalar:
        *clasp = &ScalarTypeDescr::class_;
        proto.set(global->getOrCreateFunctionPrototype(cx));
        break;

      case TypeDescr::Reference:
        *clasp = &ReferenceTypeDescr::class_;
        proto.set(global->getOrCreateFunctionPrototype(cx));
        break;

      case TypeDescr::X4:
        *clasp = &X4TypeDescr::class_;
        proto.set(global->getOrCreateFunctionPrototype(cx));
        break;

      case TypeDescr::Struct:
        *clasp = &StructTypeDescr::class_;
        proto.set(&global->getTypedObjectModule().getSlot(StructTypePrototype).toObject());
        break;

      case TypeDescr::SizedArray:
        *clasp = &SizedArrayTypeDescr::class_;
        proto.set(&global->getTypedObjectModule().getSlot(ArrayTypePrototype).toObject());
        break;

      case TypeDescr::UnsizedArray:
        *clasp = &UnsizedArrayTypeDescr::class_;
        proto.set(&global->getTypedObjectModule().getSlot(ArrayTypePrototype).toObject());
        break;
    }

    return !!proto;
}





 TypedObject *
TypedObject::createUnattached(JSContext *cx,
                             HandleTypeDescr descr,
                             int32_t length)
{
    if (descr->opaque())
        return createUnattachedWithClass(cx, &OpaqueTypedObject::class_, descr, length);
    else
        return createUnattachedWithClass(cx, &TransparentTypedObject::class_, descr, length);
}


 TypedObject *
TypedObject::createUnattachedWithClass(JSContext *cx,
                                      const Class *clasp,
                                      HandleTypeDescr type,
                                      int32_t length)
{
    JS_ASSERT(clasp == &TransparentTypedObject::class_ ||
              clasp == &OpaqueTypedObject::class_);
    JS_ASSERT(JSCLASS_RESERVED_SLOTS(clasp) == JS_TYPEDOBJ_SLOTS);
    JS_ASSERT(clasp->hasPrivate());

    RootedObject proto(cx);
    if (type->is<SimpleTypeDescr>()) {
        
        proto = type->global().getOrCreateObjectPrototype(cx);
    } else {
        RootedValue protoVal(cx);
        if (!JSObject::getProperty(cx, type, type,
                                   cx->names().prototype, &protoVal))
        {
            return nullptr;
        }
        proto = &protoVal.toObject();
    }

    RootedObject obj(cx, NewObjectWithClassProto(cx, clasp, &*proto, nullptr));
    if (!obj)
        return nullptr;

    obj->setPrivate(nullptr);
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_BYTEOFFSET, Int32Value(0));
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_BYTELENGTH, Int32Value(0));
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_OWNER, NullValue());
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_NEXT_VIEW, PrivateValue(nullptr));
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_NEXT_BUFFER, PrivateValue(UNSET_BUFFER_LINK));
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_LENGTH, Int32Value(length));
    obj->initReservedSlot(JS_TYPEDOBJ_SLOT_TYPE_DESCR, ObjectValue(*type));

    
    
    if (cx->typeInferenceEnabled() && !type->is<SimpleTypeDescr>()) {
        
        RootedTypeObject typeObj(cx, obj->getType(cx));
        if (typeObj) {
            if (!typeObj->addTypedObjectAddendum(cx, type))
                return nullptr;
        }
    }

    return static_cast<TypedObject*>(&*obj);
}

void
TypedObject::attach(ArrayBufferObject &buffer, int32_t offset)
{
    JS_ASSERT(offset >= 0);
    JS_ASSERT(offset + size() <= buffer.byteLength());

    buffer.addView(this);
    InitArrayBufferViewDataPointer(this, &buffer, offset);
    setReservedSlot(JS_TYPEDOBJ_SLOT_BYTEOFFSET, Int32Value(offset));
    setReservedSlot(JS_TYPEDOBJ_SLOT_OWNER, ObjectValue(buffer));
}

void
TypedObject::attach(TypedObject &typedObj, int32_t offset)
{
    JS_ASSERT(typedObj.typedMem() != NULL);

    attach(typedObj.owner(), typedObj.offset() + offset);
}



static int32_t
TypedObjLengthFromType(TypeDescr &descr)
{
    TypeRepresentation *typeRepr = descr.typeRepresentation();
    switch (typeRepr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::Struct:
      case TypeDescr::X4:
        return 0;

      case TypeDescr::SizedArray:
        return typeRepr->asSizedArray()->length();

      case TypeDescr::UnsizedArray:
        MOZ_ASSUME_UNREACHABLE("TypedObjLengthFromType() invoked on unsized type");
    }
    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

 TypedObject *
TypedObject::createDerived(JSContext *cx, HandleSizedTypeDescr type,
                           HandleTypedObject typedObj, size_t offset)
{
    JS_ASSERT(typedObj->typedMem() != NULL);
    JS_ASSERT(offset <= typedObj->size());
    JS_ASSERT(offset + type->size() <= typedObj->size());

    int32_t length = TypedObjLengthFromType(*type);

    const js::Class *clasp = typedObj->getClass();
    Rooted<TypedObject*> obj(cx);
    obj = createUnattachedWithClass(cx, clasp, type, length);
    if (!obj)
        return nullptr;

    obj->attach(*typedObj, offset);
    return obj;
}

 TypedObject *
TypedObject::createZeroed(JSContext *cx,
                         HandleTypeDescr descr,
                         int32_t length)
{
    
    Rooted<TypedObject*> obj(cx, createUnattached(cx, descr, length));
    if (!obj)
        return nullptr;

    
    
    TypeRepresentation *typeRepr = descr->typeRepresentation();
    switch (descr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::Struct:
      case TypeDescr::X4:
      case TypeDescr::SizedArray:
      {
        size_t totalSize = descr->as<SizedTypeDescr>().size();
        Rooted<ArrayBufferObject*> buffer(cx);
        buffer = ArrayBufferObject::create(cx, totalSize, false);
        if (!buffer)
            return nullptr;
        typeRepr->asSized()->initInstance(cx->runtime(), buffer->dataPointer(), 1);
        obj->attach(*buffer, 0);
        return obj;
      }

      case TypeDescr::UnsizedArray:
      {
        SizedTypeRepresentation *elementTypeRepr =
            typeRepr->asUnsizedArray()->element();

        int32_t totalSize;
        if (!SafeMul(elementTypeRepr->size(), length, &totalSize)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return nullptr;
        }

        Rooted<ArrayBufferObject*> buffer(cx);
        buffer = ArrayBufferObject::create(cx, totalSize, false);
        if (!buffer)
            return nullptr;

        if (length)
            elementTypeRepr->initInstance(cx->runtime(), buffer->dataPointer(), length);
        obj->attach(*buffer, 0);
        return obj;
      }
    }

    MOZ_ASSUME_UNREACHABLE("Bad TypeRepresentation Kind");
}

static bool
ReportTypedObjTypeError(JSContext *cx,
                     const unsigned errorNumber,
                     HandleTypedObject obj)
{
    
    RootedFunction func(
        cx, SelfHostedFunction(cx, cx->names().DescrToSource));
    if (!func)
        return false;
    InvokeArgs args(cx);
    if (!args.init(1))
        return false;
    args.setCallee(ObjectValue(*func));
    args[0].setObject(obj->typeDescr());
    if (!Invoke(cx, args))
        return false;

    RootedString result(cx, args.rval().toString());
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
TypedObject::obj_trace(JSTracer *trace, JSObject *object)
{
    gc::MarkSlot(trace, &object->getReservedSlotRef(JS_TYPEDOBJ_SLOT_TYPE_DESCR),
                 "TypedObjectTypeDescr");

    ArrayBufferViewObject::trace(trace, object);

    JS_ASSERT(object->is<TypedObject>());
    TypedObject &typedObj = object->as<TypedObject>();
    TypeRepresentation *repr = typedObj.typeRepresentation();
    if (repr->opaque()) {
        uint8_t *mem = typedObj.typedMem();
        if (!mem)
            return; 

        switch (repr->kind()) {
          case TypeDescr::Scalar:
          case TypeDescr::Reference:
          case TypeDescr::Struct:
          case TypeDescr::SizedArray:
          case TypeDescr::X4:
            repr->asSized()->traceInstance(trace, mem, 1);
            break;

          case TypeDescr::UnsizedArray:
            repr->asUnsizedArray()->element()->traceInstance(trace, mem, typedObj.length());
            break;
        }
    }
}

bool
TypedObject::obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                              MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(obj->is<TypedObject>());

    Rooted<TypeDescr*> typeDescr(cx, &obj->as<TypedObject>().typeDescr());
    TypeRepresentation *typeRepr = typeDescr->typeRepresentation();

    switch (typeRepr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::X4:
        break;

      case TypeDescr::SizedArray:
      case TypeDescr::UnsizedArray:
      {
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

      case TypeDescr::Struct:
      {
        StructTypeRepresentation *structTypeRepr = typeRepr->asStruct();
        const StructField *field = structTypeRepr->fieldNamed(id);
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
TypedObject::obj_lookupProperty(JSContext *cx,
                                HandleObject obj,
                                HandlePropertyName name,
                                MutableHandleObject objp,
                                MutableHandleShape propp)
{
    RootedId id(cx, NameToId(name));
    return obj_lookupGeneric(cx, obj, id, objp, propp);
}

bool
TypedObject::obj_lookupElement(JSContext *cx, HandleObject obj, uint32_t index,
                                MutableHandleObject objp, MutableHandleShape propp)
{
    JS_ASSERT(obj->is<TypedObject>());
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
TypedObject::obj_defineGeneric(JSContext *cx, HandleObject obj, HandleId id, HandleValue v,
                              PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    return ReportPropertyError(cx, JSMSG_UNDEFINED_PROP, id);
}

bool
TypedObject::obj_defineProperty(JSContext *cx, HandleObject obj,
                               HandlePropertyName name, HandleValue v,
                               PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    Rooted<jsid> id(cx, NameToId(name));
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

bool
TypedObject::obj_defineElement(JSContext *cx, HandleObject obj, uint32_t index, HandleValue v,
                               PropertyOp getter, StrictPropertyOp setter, unsigned attrs)
{
    AutoRooterGetterSetter gsRoot(cx, attrs, &getter, &setter);

    RootedObject delegate(cx, ArrayBufferDelegate(cx, obj));
    if (!delegate)
        return false;
    return baseops::DefineElement(cx, delegate, index, v, getter, setter, attrs);
}

bool
TypedObject::obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                           HandleId id, MutableHandleValue vp)
{
    JS_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());

    
    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_getElement(cx, obj, receiver, index, vp);

    

    TypeRepresentation *typeRepr = typedObj->typeRepresentation();
    switch (typeRepr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
        break;

      case TypeDescr::X4:
        break;

      case TypeDescr::SizedArray:
      case TypeDescr::UnsizedArray:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            if (!typedObj->typedMem()) { 
                JS_ReportErrorNumber(
                    cx, js_GetErrorMessage,
                    nullptr, JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);
                return false;
            }

            vp.setInt32(typedObj->length());
            return true;
        }
        break;

      case TypeDescr::Struct: {
        Rooted<StructTypeDescr*> descr(cx, &typedObj->typeDescr().as<StructTypeDescr>());

        size_t fieldIndex;
        if (!descr->fieldIndex(id, &fieldIndex))
            break;

        size_t offset = descr->fieldOffset(fieldIndex);
        Rooted<SizedTypeDescr*> fieldType(cx, &descr->fieldDescr(fieldIndex));
        return Reify(cx, fieldType, typedObj, offset, vp);
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
TypedObject::obj_getProperty(JSContext *cx, HandleObject obj, HandleObject receiver,
                              HandlePropertyName name, MutableHandleValue vp)
{
    RootedId id(cx, NameToId(name));
    return obj_getGeneric(cx, obj, receiver, id, vp);
}

bool
TypedObject::obj_getElement(JSContext *cx, HandleObject obj, HandleObject receiver,
                             uint32_t index, MutableHandleValue vp)
{
    JS_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr *> descr(cx, &typedObj->typeDescr());

    switch (descr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::X4:
      case TypeDescr::Struct:
        break;

      case TypeDescr::SizedArray:
        return obj_getArrayElement<SizedArrayTypeDescr>(cx, typedObj, descr,
                                                        index, vp);

      case TypeDescr::UnsizedArray:
        return obj_getArrayElement<UnsizedArrayTypeDescr>(cx, typedObj, descr,
                                                          index, vp);
    }

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        vp.setUndefined();
        return true;
    }

    return JSObject::getElement(cx, proto, receiver, index, vp);
}

template<class T>
 bool
TypedObject::obj_getArrayElement(JSContext *cx,
                                Handle<TypedObject*> typedObj,
                                Handle<TypeDescr*> typeDescr,
                                uint32_t index,
                                MutableHandleValue vp)
{
    JS_ASSERT(typeDescr->is<T>());

    if (index >= typedObj->length()) {
        vp.setUndefined();
        return true;
    }

    Rooted<SizedTypeDescr*> elementType(cx, &typeDescr->as<T>().elementType());
    size_t offset = elementType->size() * index;
    return Reify(cx, elementType, typedObj, offset, vp);
}

bool
TypedObject::obj_setGeneric(JSContext *cx, HandleObject obj, HandleId id,
                           MutableHandleValue vp, bool strict)
{
    JS_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());

    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_setElement(cx, obj, index, vp, strict);

    TypeRepresentation *typeRepr = typedObj->typeRepresentation();
    switch (typeRepr->kind()) {
      case ScalarTypeDescr::Scalar:
      case TypeDescr::Reference:
        break;

      case ScalarTypeDescr::X4:
        break;

      case ScalarTypeDescr::SizedArray:
      case ScalarTypeDescr::UnsizedArray:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
            return false;
        }
        break;

      case ScalarTypeDescr::Struct: {
        Rooted<StructTypeDescr*> descr(cx, &typedObj->typeDescr().as<StructTypeDescr>());

        size_t fieldIndex;
        if (!descr->fieldIndex(id, &fieldIndex))
            break;

        size_t offset = descr->fieldOffset(fieldIndex);
        Rooted<SizedTypeDescr*> fieldType(cx, &descr->fieldDescr(fieldIndex));
        return ConvertAndCopyTo(cx, fieldType, typedObj, offset, vp);
      }
    }

    return ReportTypedObjTypeError(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, typedObj);
}

bool
TypedObject::obj_setProperty(JSContext *cx, HandleObject obj,
                             HandlePropertyName name, MutableHandleValue vp,
                             bool strict)
{
    RootedId id(cx, NameToId(name));
    return obj_setGeneric(cx, obj, id, vp, strict);
}

bool
TypedObject::obj_setElement(JSContext *cx, HandleObject obj, uint32_t index,
                           MutableHandleValue vp, bool strict)
{
    JS_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr *> descr(cx, &typedObj->typeDescr());

    switch (descr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::X4:
      case TypeDescr::Struct:
        break;

      case TypeDescr::SizedArray:
        return obj_setArrayElement<SizedArrayTypeDescr>(cx, typedObj, descr, index, vp);

      case TypeDescr::UnsizedArray:
        return obj_setArrayElement<UnsizedArrayTypeDescr>(cx, typedObj, descr, index, vp);
    }

    return ReportTypedObjTypeError(cx, JSMSG_OBJECT_NOT_EXTENSIBLE, typedObj);
}

template<class T>
 bool
TypedObject::obj_setArrayElement(JSContext *cx,
                                Handle<TypedObject*> typedObj,
                                Handle<TypeDescr*> descr,
                                uint32_t index,
                                MutableHandleValue vp)
{
    JS_ASSERT(descr->is<T>());

    if (index >= typedObj->length()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage,
                             nullptr, JSMSG_TYPEDOBJECT_BINARYARRAY_BAD_INDEX);
        return false;
    }

    Rooted<SizedTypeDescr*> elementType(cx);
    elementType = &descr->as<T>().elementType();
    size_t offset = elementType->size() * index;
    return ConvertAndCopyTo(cx, elementType, typedObj, offset, vp);
}

bool
TypedObject::obj_getGenericAttributes(JSContext *cx, HandleObject obj,
                                     HandleId id, unsigned *attrsp)
{
    uint32_t index;
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    TypeRepresentation *typeRepr = typedObj->typeRepresentation();

    switch (typeRepr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
        break;

      case TypeDescr::X4:
        break;

      case TypeDescr::SizedArray:
      case TypeDescr::UnsizedArray:
        if (js_IdIsIndex(id, &index)) {
            *attrsp = JSPROP_ENUMERATE | JSPROP_PERMANENT;
            return true;
        }
        if (JSID_IS_ATOM(id, cx->names().length)) {
            *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
            return true;
        }
        break;

      case TypeDescr::Struct:
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
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    TypeRepresentation *typeRepr = typedObj->typeRepresentation();

    switch (typeRepr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::X4:
        return false;

      case TypeDescr::SizedArray:
      case TypeDescr::UnsizedArray:
        return js_IdIsIndex(id, &index) || JSID_IS_ATOM(id, cx->names().length);

      case TypeDescr::Struct:
        return typeRepr->asStruct()->fieldNamed(id) != nullptr;
    }

    return false;
}

bool
TypedObject::obj_setGenericAttributes(JSContext *cx, HandleObject obj,
                                       HandleId id, unsigned *attrsp)
{
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
TypedObject::obj_deleteProperty(JSContext *cx, HandleObject obj,
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
TypedObject::obj_deleteElement(JSContext *cx, HandleObject obj, uint32_t index,
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
TypedObject::obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                           MutableHandleValue statep, MutableHandleId idp)
{
    uint32_t index;

    JS_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    TypeRepresentation *typeRepr = typedObj->typeRepresentation();

    switch (typeRepr->kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::X4:
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

      case TypeDescr::SizedArray:
      case TypeDescr::UnsizedArray:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typedObj->length()));
            break;

          case JSENUMERATE_NEXT:
            index = static_cast<int32_t>(statep.toInt32());

            if (index < typedObj->length()) {
                idp.set(INT_TO_JSID(index));
                statep.setInt32(index + 1);
            } else {
                JS_ASSERT(index == typedObj->length());
                statep.setNull();
            }

            break;

          case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
        }
        break;

      case TypeDescr::Struct:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typeRepr->asStruct()->fieldCount()));
            break;

          case JSENUMERATE_NEXT:
            index = static_cast<uint32_t>(statep.toInt32());

            if (index < typeRepr->asStruct()->fieldCount()) {
                idp.set(NameToId(typeRepr->asStruct()->field(index).propertyName));
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
TypedObject::dataOffset()
{
    
    return JSObject::getPrivateDataOffset(JS_TYPEDOBJ_SLOT_DATA);
}

void
TypedObject::neuter(JSContext *cx)
{
    setSlot(JS_TYPEDOBJ_SLOT_LENGTH, Int32Value(0));
    setSlot(JS_TYPEDOBJ_SLOT_BYTELENGTH, Int32Value(0));
    setSlot(JS_TYPEDOBJ_SLOT_BYTEOFFSET, Int32Value(0));
    setPrivate(nullptr);
}





const Class TransparentTypedObject::class_ = {
    "TypedObject",
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(JS_TYPEDOBJ_SLOTS) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_IMPLEMENTS_BARRIERS,
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
    TypedObject::obj_trace,
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    {
        TypedObject::obj_lookupGeneric,
        TypedObject::obj_lookupProperty,
        TypedObject::obj_lookupElement,
        TypedObject::obj_defineGeneric,
        TypedObject::obj_defineProperty,
        TypedObject::obj_defineElement,
        TypedObject::obj_getGeneric,
        TypedObject::obj_getProperty,
        TypedObject::obj_getElement,
        TypedObject::obj_setGeneric,
        TypedObject::obj_setProperty,
        TypedObject::obj_setElement,
        TypedObject::obj_getGenericAttributes,
        TypedObject::obj_setGenericAttributes,
        TypedObject::obj_deleteProperty,
        TypedObject::obj_deleteElement,
        nullptr, nullptr, 
        nullptr,   
        TypedObject::obj_enumerate,
        nullptr, 
    }
};

static int32_t
LengthForType(TypeDescr &descr)
{
    switch (descr.kind()) {
      case TypeDescr::Scalar:
      case TypeDescr::Reference:
      case TypeDescr::Struct:
      case TypeDescr::X4:
        return 0;

      case TypeDescr::SizedArray:
        return descr.as<SizedArrayTypeDescr>().length();

      case TypeDescr::UnsizedArray:
        return 0;
    }

    MOZ_ASSUME_UNREACHABLE("Invalid kind");
}

static bool
CheckOffset(int32_t offset,
            int32_t size,
            int32_t alignment,
            int32_t bufferLength)
{
    JS_ASSERT(size >= 0);
    JS_ASSERT(alignment >= 0);

    
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
TypedObject::constructSized(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    JS_ASSERT(args.callee().is<SizedTypeDescr>());
    Rooted<SizedTypeDescr*> callee(cx, &args.callee().as<SizedTypeDescr>());

    
    
    
    
    
    

    
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

        if (buffer->isNeutered()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        int32_t offset;
        if (args.length() >= 2 && !args[1].isUndefined()) {
            if (!args[1].isInt32()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
                return false;
            }

            offset = args[1].toInt32();
        } else {
            offset = 0;
        }

        if (args.length() >= 3 && !args[2].isUndefined()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        if (!CheckOffset(offset, callee->size(), callee->alignment(),
                         buffer->byteLength()))
        {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        Rooted<TypedObject*> obj(cx);
        obj = TypedObject::createUnattached(cx, callee, LengthForType(*callee));
        if (!obj)
            return false;

        obj->attach(*buffer, offset);
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

    
    JS_ReportErrorNumber(cx, js_GetErrorMessage,
                         nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
    return false;
}

 bool
TypedObject::constructUnsized(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    JS_ASSERT(args.callee().is<TypeDescr>());
    Rooted<UnsizedArrayTypeDescr*> callee(cx);
    callee = &args.callee().as<UnsizedArrayTypeDescr>();

    
    
    
    
    
    
    
    
    

    
    if (args.length() == 0) {
        Rooted<TypedObject*> obj(cx, createZeroed(cx, callee, 0));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    
    if (args[0].isInt32()) {
        int32_t length = args[0].toInt32();
        if (length < 0) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return nullptr;
        }
        Rooted<TypedObject*> obj(cx, createZeroed(cx, callee, length));
        if (!obj)
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    
    if (args[0].isObject() && args[0].toObject().is<ArrayBufferObject>()) {
        Rooted<ArrayBufferObject*> buffer(cx);
        buffer = &args[0].toObject().as<ArrayBufferObject>();

        if (buffer->isNeutered()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        int32_t offset;
        if (args.length() >= 2 && !args[1].isUndefined()) {
            if (!args[1].isInt32()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
                return false;
            }

            offset = args[1].toInt32();
        } else {
            offset = 0;
        }

        if (!CheckOffset(offset, 0, callee->alignment(), buffer->byteLength())) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        int32_t elemSize = callee->elementType().size();
        int32_t bytesRemaining = buffer->byteLength() - offset;
        int32_t maximumLength = bytesRemaining / elemSize;

        int32_t length;
        if (args.length() >= 3 && !args[2].isUndefined()) {
            if (!args[2].isInt32()) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
                return false;
            }
            length = args[2].toInt32();

            if (length < 0 || length > maximumLength) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
                return false;
            }
        } else {
            if ((bytesRemaining % elemSize) != 0) {
                JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                     nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
                return false;
            }

            length = maximumLength;
        }

        Rooted<TypedObject*> obj(cx);
        obj = TypedObject::createUnattached(cx, callee, length);
        if (!obj)
            return false;

        obj->attach(args[0].toObject().as<ArrayBufferObject>(), offset);
    }

    
    if (args[0].isObject()) {
        
        RootedObject arg(cx, &args[0].toObject());
        RootedValue lengthVal(cx);
        if (!JSObject::getProperty(cx, arg, arg, cx->names().length, &lengthVal))
            return false;
        if (!lengthVal.isInt32()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }
        int32_t length = lengthVal.toInt32();

        
        int32_t elementSize = callee->elementType().size();
        int32_t byteLength;
        if (!SafeMul(elementSize, length, &byteLength)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        
        Rooted<TypedObject*> obj(cx, createZeroed(cx, callee, length));
        if (!obj)
            return false;

        
        if (!ConvertAndCopyTo(cx, obj, args[0]))
            return false;
        args.rval().setObject(*obj);
        return true;
    }

    
    JS_ReportErrorNumber(cx, js_GetErrorMessage,
                         nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
    return false;
}





const Class OpaqueTypedObject::class_ = {
    "Handle",
    Class::NON_NATIVE |
    JSCLASS_HAS_RESERVED_SLOTS(JS_TYPEDOBJ_SLOTS) |
    JSCLASS_HAS_PRIVATE |
    JSCLASS_IMPLEMENTS_BARRIERS,
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
    TypedObject::obj_trace,
    JS_NULL_CLASS_SPEC,
    JS_NULL_CLASS_EXT,
    {
        TypedObject::obj_lookupGeneric,
        TypedObject::obj_lookupProperty,
        TypedObject::obj_lookupElement,
        TypedObject::obj_defineGeneric,
        TypedObject::obj_defineProperty,
        TypedObject::obj_defineElement,
        TypedObject::obj_getGeneric,
        TypedObject::obj_getProperty,
        TypedObject::obj_getElement,
        TypedObject::obj_setGeneric,
        TypedObject::obj_setProperty,
        TypedObject::obj_setElement,
        TypedObject::obj_getGenericAttributes,
        TypedObject::obj_setGenericAttributes,
        TypedObject::obj_deleteProperty,
        TypedObject::obj_deleteElement,
        nullptr, nullptr, 
        nullptr, 
        TypedObject::obj_enumerate,
        nullptr, 
    }
};

const JSFunctionSpec OpaqueTypedObject::handleStaticMethods[] = {
    {"move", {nullptr, nullptr}, 3, 0, "HandleMove"},
    {"get", {nullptr, nullptr}, 1, 0, "HandleGet"},
    {"set", {nullptr, nullptr}, 2, 0, "HandleSet"},
    {"isHandle", {nullptr, nullptr}, 1, 0, "HandleTest"},
    JS_FS_END
};





bool
js::NewOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<SizedTypeDescr>());

    Rooted<SizedTypeDescr*> descr(cx, &args[0].toObject().as<SizedTypeDescr>());
    int32_t length = TypedObjLengthFromType(*descr);
    Rooted<TypedObject*> obj(cx);
    obj = TypedObject::createUnattachedWithClass(cx, &OpaqueTypedObject::class_, descr, length);
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

bool
js::NewDerivedTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 3);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<SizedTypeDescr>());
    JS_ASSERT(args[1].isObject() && args[1].toObject().is<TypedObject>());
    JS_ASSERT(args[2].isInt32());

    Rooted<SizedTypeDescr*> descr(cx, &args[0].toObject().as<SizedTypeDescr>());
    Rooted<TypedObject*> typedObj(cx, &args[1].toObject().as<TypedObject>());
    int32_t offset = args[2].toInt32();

    Rooted<TypedObject*> obj(cx);
    obj = TypedObject::createDerived(cx, descr, typedObj, offset);
    if (!obj)
        return false;

    args.rval().setObject(*obj);
    return true;
}

bool
js::AttachTypedObject(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 3);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    JS_ASSERT(args[1].isObject() && args[1].toObject().is<TypedObject>());
    JS_ASSERT(args[2].isInt32());

    TypedObject &handle = args[0].toObject().as<TypedObject>();
    TypedObject &target = args[1].toObject().as<TypedObject>();
    JS_ASSERT(handle.typedMem() == nullptr); 
    size_t offset = args[2].toInt32();
    handle.attach(target, offset);
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::AttachTypedObjectJitInfo,
                                      AttachTypedObjectJitInfo,
                                      js::AttachTypedObject);

bool
js::SetTypedObjectOffset(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(argc == 2);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    JS_ASSERT(args[1].isInt32());

    TypedObject &typedObj = args[0].toObject().as<TypedObject>();
    int32_t offset = args[1].toInt32();

    JS_ASSERT(typedObj.typedMem() != nullptr); 

    typedObj.setPrivate(typedObj.owner().dataPointer() + offset);
    typedObj.setReservedSlot(JS_TYPEDOBJ_SLOT_BYTEOFFSET, Int32Value(offset));
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::SetTypedObjectOffsetJitInfo,
                                      SetTypedObjectJitInfo,
                                      js::SetTypedObjectOffset);

bool
js::ObjectIsTypeDescr(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<TypeDescr>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsTypeDescrJitInfo, ObjectIsTypeDescrJitInfo,
                                      js::ObjectIsTypeDescr);

bool
js::ObjectIsOpaqueTypedObject(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<OpaqueTypedObject>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsOpaqueTypedObjectJitInfo,
                                      ObjectIsOpaqueTypedObjectJitInfo,
                                      js::ObjectIsOpaqueTypedObject);

bool
js::ObjectIsTransparentTypedObject(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<TransparentTypedObject>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsTransparentTypedObjectJitInfo,
                                      ObjectIsTransparentTypedObjectJitInfo,
                                      js::ObjectIsTransparentTypedObject);

bool
js::TypedObjectIsAttached(ThreadSafeContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();
    args.rval().setBoolean(typedObj.typedMem() != nullptr);
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::TypedObjectIsAttachedJitInfo,
                                      TypedObjectIsAttachedJitInfo,
                                      js::TypedObjectIsAttached);

bool
js::ClampToUint8(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 1);
    JS_ASSERT(args[0].isNumber());
    args.rval().setNumber(ClampDoubleToUint8(args[0].toNumber()));
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ClampToUint8JitInfo, ClampToUint8JitInfo,
                                      js::ClampToUint8);

bool
js::Memcpy(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JS_ASSERT(args.length() == 5);
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    JS_ASSERT(args[1].isInt32());
    JS_ASSERT(args[2].isObject() && args[2].toObject().is<TypedObject>());
    JS_ASSERT(args[3].isInt32());
    JS_ASSERT(args[4].isInt32());

    TypedObject &targetTypedObj = args[0].toObject().as<TypedObject>();
    int32_t targetOffset = args[1].toInt32();
    TypedObject &sourceTypedObj = args[2].toObject().as<TypedObject>();
    int32_t sourceOffset = args[3].toInt32();
    int32_t size = args[4].toInt32();

    JS_ASSERT(targetOffset >= 0);
    JS_ASSERT(sourceOffset >= 0);
    JS_ASSERT(size >= 0);
    JS_ASSERT((size_t) (size + targetOffset) <= targetTypedObj.size());
    JS_ASSERT((size_t) (size + sourceOffset) <= sourceTypedObj.size());

    uint8_t *target = targetTypedObj.typedMem(targetOffset);
    uint8_t *source = sourceTypedObj.typedMem(sourceOffset);
    memcpy(target, source, size);
    args.rval().setUndefined();
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::MemcpyJitInfo, MemcpyJitInfo, js::Memcpy);

bool
js::GetTypedObjectModule(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    JS_ASSERT(global);
    args.rval().setObject(global->getTypedObjectModule());
    return true;
}

bool
js::GetFloat32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    JS_ASSERT(global);
    args.rval().setObject(global->float32x4TypeDescr());
    return true;
}

bool
js::GetInt32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    JS_ASSERT(global);
    args.rval().setObject(global->int32x4TypeDescr());
    return true;
}

#define JS_STORE_SCALAR_CLASS_IMPL(_constant, T, _name)                         \
bool                                                                            \
js::StoreScalar##T::Func(ThreadSafeContext *, unsigned argc, Value *vp)         \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    JS_ASSERT(args.length() == 3);                                              \
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());      \
    JS_ASSERT(args[1].isInt32());                                               \
    JS_ASSERT(args[2].isNumber());                                              \
                                                                                \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    JS_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                    \
                                                                                \
    T *target = reinterpret_cast<T*>(typedObj.typedMem(offset));                \
    double d = args[2].toNumber();                                              \
    *target = ConvertScalar<T>(d);                                              \
    args.rval().setUndefined();                                                 \
    return true;                                                                \
}                                                                               \
                                                                                \
JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::StoreScalar##T::JitInfo,              \
                                      StoreScalar##T,                           \
                                      js::StoreScalar##T::Func);

#define JS_STORE_REFERENCE_CLASS_IMPL(_constant, T, _name)                      \
bool                                                                            \
js::StoreReference##T::Func(ThreadSafeContext *, unsigned argc, Value *vp)      \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    JS_ASSERT(args.length() == 3);                                              \
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());      \
    JS_ASSERT(args[1].isInt32());                                               \
                                                                                \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    JS_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                    \
                                                                                \
    T *target = reinterpret_cast<T*>(typedObj.typedMem(offset));                \
    store(target, args[2]);                                                     \
    args.rval().setUndefined();                                                 \
    return true;                                                                \
}                                                                               \
                                                                                \
JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::StoreReference##T::JitInfo,           \
                                      StoreReference##T,                        \
                                      js::StoreReference##T::Func);

#define JS_LOAD_SCALAR_CLASS_IMPL(_constant, T, _name)                                  \
bool                                                                                    \
js::LoadScalar##T::Func(ThreadSafeContext *, unsigned argc, Value *vp)                  \
{                                                                                       \
    CallArgs args = CallArgsFromVp(argc, vp);                                           \
    JS_ASSERT(args.length() == 2);                                                      \
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());              \
    JS_ASSERT(args[1].isInt32());                                                       \
                                                                                        \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();                       \
    int32_t offset = args[1].toInt32();                                                 \
                                                                                        \
    /* Should be guaranteed by the typed objects API: */                                \
    JS_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                            \
                                                                                        \
    T *target = reinterpret_cast<T*>(typedObj.typedMem(offset));                        \
    args.rval().setNumber((double) *target);                                            \
    return true;                                                                        \
}                                                                                       \
                                                                                        \
JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::LoadScalar##T::JitInfo, LoadScalar##T,        \
                                      js::LoadScalar##T::Func);

#define JS_LOAD_REFERENCE_CLASS_IMPL(_constant, T, _name)                       \
bool                                                                            \
js::LoadReference##T::Func(ThreadSafeContext *, unsigned argc, Value *vp)       \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    JS_ASSERT(args.length() == 2);                                              \
    JS_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());      \
    JS_ASSERT(args[1].isInt32());                                               \
                                                                                \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    JS_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                    \
                                                                                \
    T *target = reinterpret_cast<T*>(typedObj.typedMem(offset));                \
    load(target, args.rval());                                                  \
    return true;                                                                \
}                                                                               \
                                                                                \
JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::LoadReference##T::JitInfo,            \
                                      LoadReference##T,                         \
                                      js::LoadReference##T::Func);





void
StoreReferenceHeapValue::store(HeapValue *heap, const Value &v)
{
    *heap = v;
}

void
StoreReferenceHeapPtrObject::store(HeapPtrObject *heap, const Value &v)
{
    JS_ASSERT(v.isObjectOrNull()); 
    *heap = v.toObjectOrNull();
}

void
StoreReferenceHeapPtrString::store(HeapPtrString *heap, const Value &v)
{
    JS_ASSERT(v.isString()); 
    *heap = v.toString();
}

void
LoadReferenceHeapValue::load(HeapValue *heap,
                             MutableHandleValue v)
{
    v.set(*heap);
}

void
LoadReferenceHeapPtrObject::load(HeapPtrObject *heap,
                                 MutableHandleValue v)
{
    if (*heap)
        v.setObject(**heap);
    else
        v.setNull();
}

void
LoadReferenceHeapPtrString::load(HeapPtrString *heap,
                                 MutableHandleValue v)
{
    v.setString(*heap);
}



JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_STORE_SCALAR_CLASS_IMPL)
JS_FOR_EACH_UNIQUE_SCALAR_TYPE_REPR_CTYPE(JS_LOAD_SCALAR_CLASS_IMPL)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_STORE_REFERENCE_CLASS_IMPL)
JS_FOR_EACH_REFERENCE_TYPE_REPR(JS_LOAD_REFERENCE_CLASS_IMPL)
