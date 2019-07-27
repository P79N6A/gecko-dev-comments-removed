





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

#include "jsatominlines.h"
#include "jsobjinlines.h"

#include "vm/NativeObject-inl.h"
#include "vm/Shape-inl.h"

using mozilla::AssertedCast;
using mozilla::CheckedInt32;
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

static inline CheckedInt32 roundUpToAlignment(CheckedInt32 address, int32_t align)
{
    MOZ_ASSERT(IsPowerOfTwo(align));

    
    
    
    
    
    
    

    return ((address + (align - 1)) / align) * align;
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









const Class js::TypedProto::class_ = {
    "TypedProto",
    JSCLASS_HAS_RESERVED_SLOTS(JS_TYPROTO_SLOTS),
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
    nullptr
};










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

 const char *
ScalarTypeDescr::typeName(Type type)
{
    switch (type) {
#define NUMERIC_TYPE_TO_STRING(constant_, type_, name_) \
        case constant_: return #name_;
        JS_FOR_EACH_SCALAR_TYPE_REPR(NUMERIC_TYPE_TO_STRING)
#undef NUMERIC_TYPE_TO_STRING
      case Scalar::TypeMax:
        MOZ_CRASH();
    }
    MOZ_CRASH("Invalid type");
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

    Rooted<ScalarTypeDescr *> descr(cx, &args.callee().as<ScalarTypeDescr>());
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
      case Scalar::TypeMax:
        MOZ_CRASH();
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

 const char *
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
js::ReferenceTypeDescr::call(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    MOZ_ASSERT(args.callee().is<ReferenceTypeDescr>());
    Rooted<ReferenceTypeDescr *> descr(cx, &args.callee().as<ReferenceTypeDescr>());

    if (args.length() < 1) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
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
#define SIMD_SIZE(_kind, _type, _name)                        \
    sizeof(_type) * 4,
    JS_FOR_EACH_SIMD_TYPE_REPR(SIMD_SIZE) 0
#undef SIMD_SIZE
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








































static TypedProto *
CreatePrototypeObjectForComplexTypeInstance(JSContext *cx,
                                            Handle<TypeDescr*> descr,
                                            HandleObject ctorPrototype)
{
    RootedObject ctorPrototypePrototype(cx, GetPrototype(cx, ctorPrototype));
    if (!ctorPrototypePrototype)
        return nullptr;

    Rooted<TypedProto*> result(cx);
    result = NewObjectWithProto<TypedProto>(cx,
                                            &*ctorPrototypePrototype,
                                            nullptr,
                                            TenuredObject);
    if (!result)
        return nullptr;

    result->initTypeDescrSlot(*descr);
    return result;
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
    OutlineTypedObject::constructUnsized,
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
    OutlineTypedObject::constructSized,
    nullptr
};

const JSPropertySpec ArrayMetaTypeDescr::typeObjectProperties[] = {
    JS_PS_END
};

const JSFunctionSpec ArrayMetaTypeDescr::typeObjectMethods[] = {
    {"array", {nullptr, nullptr}, 1, 0, "ArrayShorthand"},
    JS_FN("dimension", UnsizedArrayTypeDescr::dimension, 1, 0),
    JS_SELF_HOSTED_FN("toSource", "DescrToSource", 0, 0),
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
js::CreateUserSizeAndAlignmentProperties(JSContext *cx, HandleTypeDescr descr)
{
    
    if (descr->transparent() && descr->is<SizedTypeDescr>()) {
        Rooted<SizedTypeDescr*> sizedDescr(cx, &descr->as<SizedTypeDescr>());

        
        RootedValue typeByteLength(cx, Int32Value(sizedDescr->size()));
        if (!JSObject::defineProperty(cx, descr, cx->names().byteLength,
                                      typeByteLength,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }

        
        RootedValue typeByteAlignment(cx, Int32Value(sizedDescr->alignment()));
        if (!JSObject::defineProperty(cx, descr, cx->names().byteAlignment,
                                      typeByteAlignment,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }
    } else {
        
        if (!JSObject::defineProperty(cx, descr, cx->names().byteLength,
                                      UndefinedHandleValue,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }

        
        if (!JSObject::defineProperty(cx, descr, cx->names().byteAlignment,
                                      UndefinedHandleValue,
                                      nullptr, nullptr,
                                      JSPROP_READONLY | JSPROP_PERMANENT))
        {
            return false;
        }
    }

    
    RootedValue variable(cx, BooleanValue(!descr->is<SizedTypeDescr>()));
    if (!JSObject::defineProperty(cx, descr, cx->names().variable,
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
                           HandleSizedTypeDescr elementType,
                           HandleAtom stringRepr,
                           int32_t size)
{
    Rooted<T*> obj(cx);
    obj = NewObjectWithProto<T>(cx, arrayTypePrototype, nullptr, TenuredObject);
    if (!obj)
        return nullptr;

    obj->initReservedSlot(JS_DESCR_SLOT_KIND, Int32Value(T::Kind));
    obj->initReservedSlot(JS_DESCR_SLOT_STRING_REPR, StringValue(stringRepr));
    obj->initReservedSlot(JS_DESCR_SLOT_ALIGNMENT, Int32Value(elementType->alignment()));
    obj->initReservedSlot(JS_DESCR_SLOT_SIZE, Int32Value(size));
    obj->initReservedSlot(JS_DESCR_SLOT_OPAQUE, BooleanValue(elementType->opaque()));
    obj->initReservedSlot(JS_DESCR_SLOT_ARRAY_ELEM_TYPE, ObjectValue(*elementType));

    RootedValue elementTypeVal(cx, ObjectValue(*elementType));
    if (!JSObject::defineProperty(cx, obj, cx->names().elementType,
                                  elementTypeVal, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return nullptr;

    if (!CreateUserSizeAndAlignmentProperties(cx, obj))
        return nullptr;

    Rooted<TypedProto*> prototypeObj(cx);
    prototypeObj = CreatePrototypeObjectForComplexTypeInstance(cx, obj, arrayTypePrototype);
    if (!prototypeObj)
        return nullptr;

    obj->initReservedSlot(JS_DESCR_SLOT_TYPROTO, ObjectValue(*prototypeObj));

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

    
    StringBuffer contents(cx);
    contents.append("new ArrayType(");
    contents.append(&elementType->stringRepr());
    contents.append(")");
    RootedAtom stringRepr(cx, contents.finishAtom());
    if (!stringRepr)
        return false;

    
    RootedObject arrayTypePrototype(cx, GetPrototype(cx, arrayTypeGlobal));
    if (!arrayTypePrototype)
        return false;

    
    Rooted<UnsizedArrayTypeDescr *> obj(cx);
    obj = create<UnsizedArrayTypeDescr>(cx, arrayTypePrototype, elementType,
                                        stringRepr, 0);
    if (!obj)
        return false;

    
    if (!JSObject::defineProperty(cx, obj, cx->names().length,
                                  UndefinedHandleValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

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
    int32_t length = args[0].toInt32();
    Rooted<SizedTypeDescr*> elementType(cx, &unsizedTypeDescr->elementType());

    
    CheckedInt32 size = CheckedInt32(elementType->size()) * length;
    if (!size.isValid()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return false;
    }

    
    StringBuffer contents(cx);
    contents.append("new ArrayType(");
    contents.append(&elementType->stringRepr());
    contents.append(").dimension(");
    if (!NumberValueToStringBuffer(cx, NumberValue(length), contents))
        return false;
    contents.append(")");
    RootedAtom stringRepr(cx, contents.finishAtom());
    if (!stringRepr)
        return false;

    
    Rooted<SizedArrayTypeDescr*> obj(cx);
    obj = ArrayMetaTypeDescr::create<SizedArrayTypeDescr>(cx, unsizedTypeDescr,
                                                          elementType,
                                                          stringRepr, size.value());
    if (!obj)
        return false;

    obj->initReservedSlot(JS_DESCR_SLOT_SIZED_ARRAY_LENGTH,
                          Int32Value(length));

    
    RootedValue lengthVal(cx, Int32Value(length));
    if (!JSObject::defineProperty(cx, obj, cx->names().length,
                                  lengthVal, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    
    
    RootedValue unsizedTypeDescrValue(cx, ObjectValue(*unsizedTypeDescr));
    if (!JSObject::defineProperty(cx, obj, cx->names().unsized,
                                  unsizedTypeDescrValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    args.rval().setObject(*obj);
    return true;
}

bool
js::IsTypedObjectArray(JSObject &obj)
{
    if (!obj.is<TypedObject>())
        return false;
    TypeDescr& d = obj.as<TypedObject>().typeDescr();
    return d.is<SizedArrayTypeDescr>() || d.is<UnsizedArrayTypeDescr>();
}





const Class StructTypeDescr::class_ = {
    "StructType",
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
    OutlineTypedObject::constructSized,
    nullptr  
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

JSObject *
StructMetaTypeDescr::create(JSContext *cx,
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

    userFieldOffsets = NewObjectWithProto<JSObject>(cx, nullptr, nullptr, TenuredObject);
    if (!userFieldOffsets)
        return nullptr;

    userFieldTypes = NewObjectWithProto<JSObject>(cx, nullptr, nullptr, TenuredObject);
    if (!userFieldTypes)
        return nullptr;

    if (!stringBuffer.append("new StructType({")) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }

    RootedValue fieldTypeVal(cx);
    RootedId id(cx);
    Rooted<SizedTypeDescr*> fieldType(cx);
    for (unsigned int i = 0; i < ids.length(); i++) {
        id = ids[i];

        
        uint32_t unused;
        if (!JSID_IS_ATOM(id) || JSID_TO_ATOM(id)->isIndex(&unused)) {
            RootedValue idValue(cx, IdToValue(id));
            ReportCannotConvertTo(cx, idValue, "StructType field name");
            return nullptr;
        }

        
        
        if (!JSObject::getGeneric(cx, fields, fields, id, &fieldTypeVal))
            return nullptr;
        fieldType = ToObjectIf<SizedTypeDescr>(fieldTypeVal);
        if (!fieldType) {
            ReportCannotConvertTo(cx, fieldTypeVal, "StructType field specifier");
            return nullptr;
        }

        
        RootedValue fieldName(cx, IdToValue(id));
        if (!fieldNames.append(fieldName)) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!fieldTypeObjs.append(ObjectValue(*fieldType))) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }

        
        if (!JSObject::defineGeneric(cx, userFieldTypes, id,
                                     fieldTypeObjs[i], nullptr, nullptr,
                                     JSPROP_READONLY | JSPROP_PERMANENT))
            return nullptr;

        
        if (i > 0 && !stringBuffer.append(", ")) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!stringBuffer.append(JSID_TO_ATOM(id))) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!stringBuffer.append(": ")) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }
        if (!stringBuffer.append(&fieldType->stringRepr())) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }

        
        
        CheckedInt32 offset = roundUpToAlignment(sizeSoFar, fieldType->alignment());
        if (!offset.isValid()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return nullptr;
        }
        MOZ_ASSERT(offset.value() >= 0);
        if (!fieldOffsets.append(Int32Value(offset.value()))) {
            js_ReportOutOfMemory(cx);
            return nullptr;
        }

        
        RootedValue offsetValue(cx, Int32Value(offset.value()));
        if (!JSObject::defineGeneric(cx, userFieldOffsets, id,
                                     offsetValue, nullptr, nullptr,
                                     JSPROP_READONLY | JSPROP_PERMANENT))
            return nullptr;

        
        sizeSoFar = offset + fieldType->size();
        if (!sizeSoFar.isValid()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return nullptr;
        }

        
        if (fieldType->opaque())
            opaque = true;

        
        alignment = js::Max(alignment, fieldType->alignment());
    }

    
    if (!stringBuffer.append("})")) {
        js_ReportOutOfMemory(cx);
        return nullptr;
    }
    RootedAtom stringRepr(cx, stringBuffer.finishAtom());
    if (!stringRepr)
        return nullptr;

    
    CheckedInt32 totalSize = roundUpToAlignment(sizeSoFar, alignment);
    if (!totalSize.isValid()) {
        JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                             JSMSG_TYPEDOBJECT_TOO_BIG);
        return nullptr;
    }

    
    RootedObject structTypePrototype(cx, GetPrototype(cx, metaTypeDescr));
    if (!structTypePrototype)
        return nullptr;

    Rooted<StructTypeDescr*> descr(cx);
    descr = NewObjectWithProto<StructTypeDescr>(cx, structTypePrototype, nullptr,
                                                TenuredObject);
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
                                            fieldNames.begin(), nullptr,
                                            TenuredObject);
        if (!fieldNamesVec)
            return nullptr;
        descr->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_NAMES,
                                     ObjectValue(*fieldNamesVec));
    }

    
    {
        RootedObject fieldTypeVec(cx);
        fieldTypeVec = NewDenseCopiedArray(cx, fieldTypeObjs.length(),
                                           fieldTypeObjs.begin(), nullptr,
                                           TenuredObject);
        if (!fieldTypeVec)
            return nullptr;
        descr->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_TYPES,
                                     ObjectValue(*fieldTypeVec));
    }

    
    {
        RootedObject fieldOffsetsVec(cx);
        fieldOffsetsVec = NewDenseCopiedArray(cx, fieldOffsets.length(),
                                              fieldOffsets.begin(), nullptr,
                                              TenuredObject);
        if (!fieldOffsetsVec)
            return nullptr;
        descr->initReservedSlot(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS,
                                     ObjectValue(*fieldOffsetsVec));
    }

    
    if (!JSObject::freeze(cx, userFieldOffsets))
        return nullptr;
    if (!JSObject::freeze(cx, userFieldTypes))
        return nullptr;
    RootedValue userFieldOffsetsValue(cx, ObjectValue(*userFieldOffsets));
    if (!JSObject::defineProperty(cx, descr, cx->names().fieldOffsets,
                                  userFieldOffsetsValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }
    RootedValue userFieldTypesValue(cx, ObjectValue(*userFieldTypes));
    if (!JSObject::defineProperty(cx, descr, cx->names().fieldTypes,
                                  userFieldTypesValue, nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
    {
        return nullptr;
    }

    if (!CreateUserSizeAndAlignmentProperties(cx, descr))
        return nullptr;

    Rooted<TypedProto*> prototypeObj(cx);
    prototypeObj = CreatePrototypeObjectForComplexTypeInstance(cx, descr, structTypePrototype);
    if (!prototypeObj)
        return nullptr;

    descr->initReservedSlot(JS_DESCR_SLOT_TYPROTO, ObjectValue(*prototypeObj));

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
StructTypeDescr::fieldIndex(jsid id, size_t *out) const
{
    NativeObject &fieldNames = fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_NAMES);
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

JSAtom &
StructTypeDescr::fieldName(size_t index) const
{
    return fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_NAMES).getDenseElement(index).toString()->asAtom();
}

size_t
StructTypeDescr::fieldOffset(size_t index) const
{
    NativeObject &fieldOffsets = fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS);
    MOZ_ASSERT(index < fieldOffsets.getDenseInitializedLength());
    return AssertedCast<size_t>(fieldOffsets.getDenseElement(index).toInt32());
}

size_t
StructTypeDescr::maybeForwardedFieldOffset(size_t index) const
{
    NativeObject &fieldOffsets = maybeForwardedFieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS);
    MOZ_ASSERT(index < fieldOffsets.getDenseInitializedLength());
    return AssertedCast<size_t>(fieldOffsets.getDenseElement(index).toInt32());
}

SizedTypeDescr&
StructTypeDescr::fieldDescr(size_t index) const
{
    NativeObject &fieldDescrs = fieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_TYPES);
    MOZ_ASSERT(index < fieldDescrs.getDenseInitializedLength());
    return fieldDescrs.getDenseElement(index).toObject().as<SizedTypeDescr>();
}

SizedTypeDescr&
StructTypeDescr::maybeForwardedFieldDescr(size_t index) const
{
    NativeObject &fieldDescrs = maybeForwardedFieldInfoObject(JS_DESCR_SLOT_STRUCT_FIELD_TYPES);
    MOZ_ASSERT(index < fieldDescrs.getDenseInitializedLength());
    JSObject &descr =
        *MaybeForwarded(&fieldDescrs.getDenseElement(index).toObject());
    return descr.as<SizedTypeDescr>();
}

















































template<typename T>
static bool
DefineSimpleTypeDescr(JSContext *cx,
                      Handle<GlobalObject *> global,
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
    descr = NewObjectWithProto<T>(cx, funcProto, global, TenuredObject);
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
    proto = NewObjectWithProto<TypedProto>(cx, objProto, nullptr, TenuredObject);
    if (!proto)
        return false;
    proto->initTypeDescrSlot(*descr);
    descr->initReservedSlot(JS_DESCR_SLOT_TYPROTO, ObjectValue(*proto));

    RootedValue descrValue(cx, ObjectValue(*descr));
    if (!JSObject::defineProperty(cx, module, className,
                                  descrValue, nullptr, nullptr, 0))
    {
        return false;
    }

    return true;
}



template<typename T>
static JSObject *
DefineMetaTypeDescr(JSContext *cx,
                    Handle<GlobalObject*> global,
                    HandleNativeObject module,
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
    if (!protoProto)
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
        cx, global, module, TypedObjectModuleObject::ArrayTypePrototype);
    if (!arrayType)
        return false;

    RootedValue arrayTypeValue(cx, ObjectValue(*arrayType));
    if (!JSObject::defineProperty(cx, module, cx->names().ArrayType,
                                  arrayTypeValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    

    RootedObject structType(cx);
    structType = DefineMetaTypeDescr<StructMetaTypeDescr>(
        cx, global, module, TypedObjectModuleObject::StructTypePrototype);
    if (!structType)
        return false;

    RootedValue structTypeValue(cx, ObjectValue(*structType));
    if (!JSObject::defineProperty(cx, module, cx->names().StructType,
                                  structTypeValue,
                                  nullptr, nullptr,
                                  JSPROP_READONLY | JSPROP_PERMANENT))
        return false;

    
    RootedValue moduleValue(cx, ObjectValue(*module));
    global->setConstructor(JSProto_TypedObject, moduleValue);
    if (!JSObject::defineProperty(cx, global, cx->names().TypedObject,
                                  moduleValue,
                                  nullptr, nullptr,
                                  0))
    {
        return false;
    }

    return module;
}

JSObject *
js_InitTypedObjectModuleObject(JSContext *cx, HandleObject obj)
{
    MOZ_ASSERT(obj->is<GlobalObject>());
    Rooted<GlobalObject *> global(cx, &obj->as<GlobalObject>());
    return global->getOrCreateTypedObjectModule(cx);
}

JSObject *
js_InitTypedObjectDummy(JSContext *cx, HandleObject obj)
{
    





    MOZ_CRASH("shouldn't be initializing TypedObject via the JSProtoKey initializer mechanism");
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
    if (typeDescr().is<SizedArrayTypeDescr>())
        return typeDescr().as<SizedArrayTypeDescr>().length();
    return as<OutlineTypedObject>().unsizedLength();
}

uint8_t *
TypedObject::typedMem() const
{
    MOZ_ASSERT(isAttached());

    if (is<InlineTypedObject>())
        return as<InlineTypedObject>().inlineTypedMem();
    return as<OutlineTypedObject>().outOfLineTypedMem();
}

uint8_t *
TypedObject::typedMemBase() const
{
    MOZ_ASSERT(isAttached());
    MOZ_ASSERT(is<OutlineTypedObject>());

    JSObject &owner = as<OutlineTypedObject>().owner();
    if (owner.is<ArrayBufferObject>())
        return owner.as<ArrayBufferObject>().dataPointer();
    return owner.as<InlineTypedObject>().inlineTypedMem();
}

bool
TypedObject::isAttached() const
{
    if (is<InlineTransparentTypedObject>()) {
        LazyArrayBufferTable *table = compartment()->lazyArrayBuffers;
        if (table) {
            ArrayBufferObject *buffer =
                table->maybeBuffer(&const_cast<TypedObject *>(this)->as<InlineTransparentTypedObject>());
            if (buffer)
                return !buffer->isNeutered();
        }
        return true;
    }
    if (is<InlineOpaqueTypedObject>())
        return true;
    if (!as<OutlineTypedObject>().outOfLineTypedMem())
        return false;
    JSObject &owner = as<OutlineTypedObject>().owner();
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
    JSObject &owner = *MaybeForwarded(&as<OutlineTypedObject>().owner());
    if (owner.is<ArrayBufferObject>() && owner.as<ArrayBufferObject>().isNeutered())
        return false;
    return true;
}

 bool
TypedObject::GetBuffer(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    JSObject &obj = args[0].toObject();
    ArrayBufferObject *buffer;
    if (obj.is<OutlineTransparentTypedObject>())
        buffer = obj.as<OutlineTransparentTypedObject>().getOrCreateBuffer(cx);
    else
        buffer = obj.as<InlineTransparentTypedObject>().getOrCreateBuffer(cx);
    if (!buffer)
        MOZ_CRASH();
    args.rval().setObject(*buffer);
    return true;
}

 bool
TypedObject::GetByteOffset(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    args.rval().setInt32(args[0].toObject().as<TypedObject>().offset());
    return true;
}





 OutlineTypedObject *
OutlineTypedObject::createUnattached(JSContext *cx,
                                     HandleTypeDescr descr,
                                     int32_t length)
{
    if (descr->opaque())
        return createUnattachedWithClass(cx, &OutlineOpaqueTypedObject::class_, descr, length);
    else
        return createUnattachedWithClass(cx, &OutlineTransparentTypedObject::class_, descr, length);
}

static JSObject *
PrototypeForTypeDescr(JSContext *cx, HandleTypeDescr descr)
{
    if (descr->is<SimpleTypeDescr>()) {
        
        return cx->global()->getOrCreateObjectPrototype(cx);
    }

    RootedValue protoVal(cx);
    if (!JSObject::getProperty(cx, descr, descr,
                               cx->names().prototype, &protoVal))
    {
        return nullptr;
    }

    return &protoVal.toObject();
}

void
OutlineTypedObject::setOwnerAndData(JSObject *owner, uint8_t *data)
{
    
    
    owner_ = owner;
    data_ = data;

    
    
    if (owner && !IsInsideNursery(this) && IsInsideNursery(owner))
        runtimeFromMainThread()->gc.storeBuffer.putWholeCellFromMainThread(this);
}

 OutlineTypedObject *
OutlineTypedObject::createUnattachedWithClass(JSContext *cx,
                                              const Class *clasp,
                                              HandleTypeDescr type,
                                              int32_t length)
{
    MOZ_ASSERT(clasp == &OutlineTransparentTypedObject::class_ ||
               clasp == &OutlineOpaqueTypedObject::class_);

    RootedObject proto(cx, PrototypeForTypeDescr(cx, type));
    if (!proto)
        return nullptr;

    gc::AllocKind allocKind = allocKindForTypeDescriptor(type);
    JSObject *obj = NewObjectWithClassProto(cx, clasp, proto, nullptr, allocKind);
    if (!obj)
        return nullptr;

    OutlineTypedObject *typedObj = &obj->as<OutlineTypedObject>();

    typedObj->setOwnerAndData(nullptr, nullptr);
    if (type->kind() == type::UnsizedArray)
        typedObj->setUnsizedLength(length);

    return typedObj;
}

void
OutlineTypedObject::attach(JSContext *cx, ArrayBufferObject &buffer, int32_t offset)
{
    MOZ_ASSERT(!isAttached());
    MOZ_ASSERT(offset >= 0);
    MOZ_ASSERT((size_t) (offset + size()) <= buffer.byteLength());

    buffer.setHasTypedObjectViews();

    if (!buffer.addView(cx, this))
        CrashAtUnhandlableOOM("TypedObject::attach");

    setOwnerAndData(&buffer, buffer.dataPointer() + offset);
}

void
OutlineTypedObject::attach(JSContext *cx, TypedObject &typedObj, int32_t offset)
{
    MOZ_ASSERT(!isAttached());
    MOZ_ASSERT(typedObj.isAttached());

    JSObject *owner = &typedObj;
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
TypedObjLengthFromType(TypeDescr &descr)
{
    switch (descr.kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Struct:
      case type::Simd:
        return 0;

      case type::SizedArray:
        return descr.as<SizedArrayTypeDescr>().length();

      case type::UnsizedArray:
        MOZ_CRASH("TypedObjLengthFromType() invoked on unsized type");
    }
    MOZ_CRASH("Invalid kind");
}

 OutlineTypedObject *
OutlineTypedObject::createDerived(JSContext *cx, HandleSizedTypeDescr type,
                                  HandleTypedObject typedObj, int32_t offset)
{
    MOZ_ASSERT(offset <= typedObj->size());
    MOZ_ASSERT(offset + type->size() <= typedObj->size());

    int32_t length = TypedObjLengthFromType(*type);

    const js::Class *clasp = typedObj->opaque()
                             ? &OutlineOpaqueTypedObject::class_
                             : &OutlineTransparentTypedObject::class_;
    Rooted<OutlineTypedObject*> obj(cx);
    obj = createUnattachedWithClass(cx, clasp, type, length);
    if (!obj)
        return nullptr;

    obj->attach(cx, *typedObj, offset);
    return obj;
}

 TypedObject *
TypedObject::createZeroed(JSContext *cx, HandleTypeDescr descr, int32_t length)
{
    
    if (descr->is<SizedTypeDescr>() &&
        (size_t) descr->as<SizedTypeDescr>().size() <= InlineTypedObject::MaximumSize)
    {
        InlineTypedObject *obj = InlineTypedObject::create(cx, descr);
        descr->as<SizedTypeDescr>().initInstances(cx->runtime(), obj->inlineTypedMem(), 1);
        return obj;
    }

    
    Rooted<OutlineTypedObject*> obj(cx, OutlineTypedObject::createUnattached(cx, descr, length));
    if (!obj)
        return nullptr;

    
    
    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Struct:
      case type::Simd:
      case type::SizedArray:
      {
        size_t totalSize = descr->as<SizedTypeDescr>().size();
        Rooted<ArrayBufferObject*> buffer(cx);
        buffer = ArrayBufferObject::create(cx, totalSize);
        if (!buffer)
            return nullptr;
        descr->as<SizedTypeDescr>().initInstances(cx->runtime(), buffer->dataPointer(), 1);
        obj->attach(cx, *buffer, 0);
        return obj;
      }

      case type::UnsizedArray:
      {
        Rooted<SizedTypeDescr*> elementTypeRepr(cx);
        elementTypeRepr = &descr->as<UnsizedArrayTypeDescr>().elementType();

        CheckedInt32 totalSize = CheckedInt32(elementTypeRepr->size()) * length;
        if (!totalSize.isValid()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                                 JSMSG_TYPEDOBJECT_TOO_BIG);
            return nullptr;
        }

        Rooted<ArrayBufferObject*> buffer(cx);
        buffer = ArrayBufferObject::create(cx, totalSize.value());
        if (!buffer)
            return nullptr;

        if (length)
            elementTypeRepr->initInstances(cx->runtime(), buffer->dataPointer(), length);
        obj->attach(cx, *buffer, 0);
        return obj;
      }
    }

    MOZ_CRASH("Bad TypeRepresentation Kind");
}

static bool
ReportTypedObjTypeError(JSContext *cx,
                        const unsigned errorNumber,
                        HandleTypedObject obj)
{
    
    char *typeReprStr = JS_EncodeString(cx, &obj->typeDescr().stringRepr());
    if (!typeReprStr)
        return false;

    JS_ReportErrorNumber(cx, js_GetErrorMessage, nullptr,
                         errorNumber, typeReprStr);

    JS_free(cx, (void *) typeReprStr);
    return false;
}

 void
OutlineTypedObject::obj_trace(JSTracer *trc, JSObject *object)
{
    OutlineTypedObject &typedObj = object->as<OutlineTypedObject>();

    if (!typedObj.owner_)
        return;

    
    
    
    
    TypeDescr &descr = typedObj.maybeForwardedTypeDescr();

    
    JSObject *oldOwner = typedObj.owner_;
    gc::MarkObjectUnbarriered(trc, &typedObj.owner_, "typed object owner");
    JSObject *owner = typedObj.owner_;

    uint8_t *mem = typedObj.outOfLineTypedMem();

    
    
    if (owner != oldOwner &&
        (owner->is<InlineTypedObject>() ||
         owner->as<ArrayBufferObject>().hasInlineData()))
    {
        mem += reinterpret_cast<uint8_t *>(owner) - reinterpret_cast<uint8_t *>(oldOwner);
        typedObj.setData(mem);
    }

    if (!descr.opaque() || !typedObj.maybeForwardedIsAttached())
        return;

    switch (descr.kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Struct:
      case type::SizedArray:
      case type::Simd:
        descr.as<SizedTypeDescr>().traceInstances(trc, mem, 1);
        break;

      case type::UnsizedArray:
      {
        SizedTypeDescr &elemType = descr.as<UnsizedArrayTypeDescr>().maybeForwardedElementType();
        elemType.traceInstances(trc, mem, typedObj.unsizedLength());
        break;
      }
    }
}

bool
TypedObject::obj_lookupGeneric(JSContext *cx, HandleObject obj, HandleId id,
                              MutableHandleObject objp, MutableHandleShape propp)
{
    MOZ_ASSERT(obj->is<TypedObject>());

    Rooted<TypeDescr*> descr(cx, &obj->as<TypedObject>().typeDescr());
    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
        break;

      case type::SizedArray:
      case type::UnsizedArray:
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

      case type::Struct:
      {
        StructTypeDescr &structDescr = descr->as<StructTypeDescr>();
        size_t index;
        if (structDescr.fieldIndex(id, &index)) {
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
    MOZ_ASSERT(obj->is<TypedObject>());
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
    Rooted<jsid> id(cx);
    if (!IndexToId(cx, index, &id))
        return false;
    return obj_defineGeneric(cx, obj, id, v, getter, setter, attrs);
}

bool
TypedObject::obj_getGeneric(JSContext *cx, HandleObject obj, HandleObject receiver,
                           HandleId id, MutableHandleValue vp)
{
    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());

    
    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_getElement(cx, obj, receiver, index, vp);

    

    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
        break;

      case type::Simd:
        break;

      case type::SizedArray:
      case type::UnsizedArray:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            if (!typedObj->isAttached()) {
                JS_ReportErrorNumber(
                    cx, js_GetErrorMessage,
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
    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr *> descr(cx, &typedObj->typeDescr());

    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
      case type::Struct:
        break;

      case type::SizedArray:
        return obj_getArrayElement<SizedArrayTypeDescr>(cx, typedObj, descr,
                                                        index, vp);

      case type::UnsizedArray:
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
    MOZ_ASSERT(typeDescr->is<T>());

    if (index >= (size_t) typedObj->length()) {
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
    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());

    uint32_t index;
    if (js_IdIsIndex(id, &index))
        return obj_setElement(cx, obj, index, vp, strict);

    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
        break;

      case type::Simd:
        break;

      case type::SizedArray:
      case type::UnsizedArray:
        if (JSID_IS_ATOM(id, cx->names().length)) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_CANT_REDEFINE_ARRAY_LENGTH);
            return false;
        }
        break;

      case type::Struct: {
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
    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr *> descr(cx, &typedObj->typeDescr());

    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
      case type::Struct:
        break;

      case type::SizedArray:
        return obj_setArrayElement<SizedArrayTypeDescr>(cx, typedObj, descr, index, vp);

      case type::UnsizedArray:
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
    MOZ_ASSERT(descr->is<T>());

    if (index >= (size_t) typedObj->length()) {
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
    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
        break;

      case type::Simd:
        break;

      case type::SizedArray:
      case type::UnsizedArray:
        if (js_IdIsIndex(id, &index)) {
            *attrsp = JSPROP_ENUMERATE | JSPROP_PERMANENT;
            return true;
        }
        if (JSID_IS_ATOM(id, cx->names().length)) {
            *attrsp = JSPROP_READONLY | JSPROP_PERMANENT;
            return true;
        }
        break;

      case type::Struct:
        size_t index;
        if (typedObj->typeDescr().as<StructTypeDescr>().fieldIndex(id, &index)) {
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
    switch (typedObj->typeDescr().kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
        return false;

      case type::SizedArray:
      case type::UnsizedArray:
        return js_IdIsIndex(id, &index) || JSID_IS_ATOM(id, cx->names().length);

      case type::Struct:
        size_t index;
        if (typedObj->typeDescr().as<StructTypeDescr>().fieldIndex(id, &index))
            return true;
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
TypedObject::obj_deleteGeneric(JSContext *cx, HandleObject obj, HandleId id, bool *succeeded)
{
    if (IsOwnId(cx, obj, id))
        return ReportPropertyError(cx, JSMSG_CANT_DELETE, id);

    RootedObject proto(cx, obj->getProto());
    if (!proto) {
        *succeeded = false;
        return true;
    }

    return JSObject::deleteGeneric(cx, proto, id, succeeded);
}

bool
TypedObject::obj_enumerate(JSContext *cx, HandleObject obj, JSIterateOp enum_op,
                           MutableHandleValue statep, MutableHandleId idp)
{
    int32_t index;

    MOZ_ASSERT(obj->is<TypedObject>());
    Rooted<TypedObject *> typedObj(cx, &obj->as<TypedObject>());
    Rooted<TypeDescr *> descr(cx, &typedObj->typeDescr());
    switch (descr->kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Simd:
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

      case type::SizedArray:
      case type::UnsizedArray:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(typedObj->length()));
            break;

          case JSENUMERATE_NEXT:
            index = statep.toInt32();

            if (index < typedObj->length()) {
                idp.set(INT_TO_JSID(index));
                statep.setInt32(index + 1);
            } else {
                MOZ_ASSERT(index == typedObj->length());
                statep.setNull();
            }

            break;

          case JSENUMERATE_DESTROY:
            statep.setNull();
            break;
        }
        break;

      case type::Struct:
        switch (enum_op) {
          case JSENUMERATE_INIT_ALL:
          case JSENUMERATE_INIT:
            statep.setInt32(0);
            idp.set(INT_TO_JSID(descr->as<StructTypeDescr>().fieldCount()));
            break;

          case JSENUMERATE_NEXT:
            index = static_cast<uint32_t>(statep.toInt32());

            if ((size_t) index < descr->as<StructTypeDescr>().fieldCount()) {
                idp.set(AtomToId(&descr->as<StructTypeDescr>().fieldName(index)));
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

void
OutlineTypedObject::neuter(void *newData)
{
    if (typeDescr().kind() == type::UnsizedArray)
        setUnsizedLength(0);
    setData(reinterpret_cast<uint8_t *>(newData));
}





 InlineTypedObject *
InlineTypedObject::create(JSContext *cx, HandleTypeDescr descr)
{
    gc::AllocKind allocKind = allocKindForTypeDescriptor(descr);

    RootedObject proto(cx, PrototypeForTypeDescr(cx, descr));
    if (!proto)
        return nullptr;

    const Class *clasp = descr->opaque()
                         ? &InlineOpaqueTypedObject::class_
                         : &InlineTransparentTypedObject::class_;

    RootedObject obj(cx, NewObjectWithClassProto(cx, clasp, proto, nullptr, allocKind));
    if (!obj)
        return nullptr;

    return &obj->as<InlineTypedObject>();
}

 void
InlineTypedObject::obj_trace(JSTracer *trc, JSObject *object)
{
    InlineTypedObject &typedObj = object->as<InlineTypedObject>();

    
    
    TypeDescr &descr = typedObj.maybeForwardedTypeDescr();

    descr.as<SizedTypeDescr>().traceInstances(trc, typedObj.inlineTypedMem(), 1);
}

ArrayBufferObject *
InlineTransparentTypedObject::getOrCreateBuffer(JSContext *cx)
{
    LazyArrayBufferTable *&table = cx->compartment()->lazyArrayBuffers;
    if (!table) {
        table = cx->new_<LazyArrayBufferTable>(cx);
        if (!table)
            return nullptr;
    }

    ArrayBufferObject *buffer = table->maybeBuffer(this);
    if (buffer)
        return buffer;

    ArrayBufferObject::BufferContents contents =
        ArrayBufferObject::BufferContents::createPlain(inlineTypedMem());
    size_t nbytes = typeDescr().as<SizedTypeDescr>().size();

    
    
    gc::AutoSuppressGC suppress(cx);

    buffer = ArrayBufferObject::create(cx, nbytes, contents, ArrayBufferObject::DoesntOwnData);
    if (!buffer)
        return nullptr;

    
    
    
    
    
    JS_ALWAYS_TRUE(buffer->addView(cx, this));

    buffer->setForInlineTypedObject();
    buffer->setHasTypedObjectViews();

    if (!table->addBuffer(cx, this, buffer))
        return nullptr;

    return buffer;
}

ArrayBufferObject *
OutlineTransparentTypedObject::getOrCreateBuffer(JSContext *cx)
{
    if (owner().is<ArrayBufferObject>())
        return &owner().as<ArrayBufferObject>();
    return owner().as<InlineTransparentTypedObject>().getOrCreateBuffer(cx);
}

LazyArrayBufferTable::LazyArrayBufferTable(JSContext *cx)
 : map(cx)
{
    if (!map.init())
        CrashAtUnhandlableOOM("LazyArrayBufferTable");
}

LazyArrayBufferTable::~LazyArrayBufferTable()
{
    WeakMapBase::removeWeakMapFromList(&map);
}

ArrayBufferObject *
LazyArrayBufferTable::maybeBuffer(InlineTransparentTypedObject *obj)
{
    if (Map::Ptr p = map.lookup(obj))
        return &p->value()->as<ArrayBufferObject>();
    return nullptr;
}

bool
LazyArrayBufferTable::addBuffer(JSContext *cx, InlineTransparentTypedObject *obj, ArrayBufferObject *buffer)
{
    MOZ_ASSERT(!map.has(obj));
    if (!map.put(obj, buffer)) {
        js_ReportOutOfMemory(cx);
        return false;
    }

#ifdef JSGC_GENERATIONAL
    MOZ_ASSERT(!IsInsideNursery(buffer));
    if (IsInsideNursery(obj)) {
        
        
        Map::Base *baseHashMap = static_cast<Map::Base *>(&map);

        typedef HashMap<JSObject *, JSObject *> UnbarrieredMap;
        UnbarrieredMap *unbarrieredMap = reinterpret_cast<UnbarrieredMap *>(baseHashMap);

        typedef gc::HashKeyRef<UnbarrieredMap, JSObject *> Ref;
        cx->runtime()->gc.storeBuffer.putGeneric(Ref(unbarrieredMap, obj));

        
        
        cx->runtime()->gc.storeBuffer.putWholeCellFromMainThread(buffer);
    }
#endif

    return true;
}

void
LazyArrayBufferTable::trace(JSTracer *trc)
{
    map.trace(trc);
}

size_t
LazyArrayBufferTable::sizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf)
{
    return mallocSizeOf(this) + map.sizeOfExcludingThis(mallocSizeOf);
}





#define DEFINE_TYPEDOBJ_CLASS(Name, Trace)        \
    const Class Name::class_ = {                         \
        # Name,                                          \
        Class::NON_NATIVE | JSCLASS_IMPLEMENTS_BARRIERS, \
        JS_PropertyStub,                                 \
        JS_DeletePropertyStub,                           \
        JS_PropertyStub,                                 \
        JS_StrictPropertyStub,                           \
        JS_EnumerateStub,                                \
        JS_ResolveStub,                                  \
        JS_ConvertStub,                                  \
        nullptr,        /* finalize    */                \
        nullptr,        /* call        */                \
        nullptr,        /* hasInstance */                \
        nullptr,        /* construct   */                \
        Trace,                                           \
        JS_NULL_CLASS_SPEC,                              \
        JS_NULL_CLASS_EXT,                               \
        {                                                \
            TypedObject::obj_lookupGeneric,              \
            TypedObject::obj_lookupProperty,             \
            TypedObject::obj_lookupElement,              \
            TypedObject::obj_defineGeneric,              \
            TypedObject::obj_defineProperty,             \
            TypedObject::obj_defineElement,              \
            TypedObject::obj_getGeneric,                 \
            TypedObject::obj_getProperty,                \
            TypedObject::obj_getElement,                 \
            TypedObject::obj_setGeneric,                 \
            TypedObject::obj_setProperty,                \
            TypedObject::obj_setElement,                 \
            TypedObject::obj_getGenericAttributes,       \
            TypedObject::obj_setGenericAttributes,       \
            TypedObject::obj_deleteGeneric,              \
            nullptr, nullptr, /* watch/unwatch */        \
            nullptr,   /* slice */                       \
            TypedObject::obj_enumerate,                  \
            nullptr, /* thisObject */                    \
        }                                                \
    }

DEFINE_TYPEDOBJ_CLASS(OutlineTransparentTypedObject, OutlineTypedObject::obj_trace);
DEFINE_TYPEDOBJ_CLASS(OutlineOpaqueTypedObject,      OutlineTypedObject::obj_trace);
DEFINE_TYPEDOBJ_CLASS(InlineTransparentTypedObject,  InlineTypedObject::obj_trace);
DEFINE_TYPEDOBJ_CLASS(InlineOpaqueTypedObject,       InlineTypedObject::obj_trace);

static int32_t
LengthForType(TypeDescr &descr)
{
    switch (descr.kind()) {
      case type::Scalar:
      case type::Reference:
      case type::Struct:
      case type::Simd:
        return 0;

      case type::SizedArray:
        return descr.as<SizedArrayTypeDescr>().length();

      case type::UnsizedArray:
        return 0;
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
TypedObject::constructSized(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    MOZ_ASSERT(args.callee().is<SizedTypeDescr>());
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

        if (callee->opaque() || buffer->isNeutered()) {
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

    
    JS_ReportErrorNumber(cx, js_GetErrorMessage,
                         nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
    return false;
}

 bool
TypedObject::constructUnsized(JSContext *cx, unsigned int argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);

    MOZ_ASSERT(args.callee().is<TypeDescr>());
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
            return false;
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

        if (callee->opaque()) {
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

        if (buffer->isNeutered()) {
            JS_ReportErrorNumber(cx, js_GetErrorMessage,
                                 nullptr, JSMSG_TYPEDOBJECT_BAD_ARGS);
            return false;
        }

        Rooted<OutlineTypedObject*> obj(cx);
        obj = OutlineTypedObject::createUnattached(cx, callee, length);
        if (!obj)
            return false;

        obj->attach(cx, args[0].toObject().as<ArrayBufferObject>(), offset);
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





bool
js::NewOpaqueTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<SizedTypeDescr>());

    Rooted<SizedTypeDescr*> descr(cx, &args[0].toObject().as<SizedTypeDescr>());
    int32_t length = TypedObjLengthFromType(*descr);
    Rooted<OutlineTypedObject*> obj(cx);
    obj = OutlineTypedObject::createUnattachedWithClass(cx, &OutlineOpaqueTypedObject::class_, descr, length);
    if (!obj)
        return false;
    args.rval().setObject(*obj);
    return true;
}

bool
js::NewDerivedTypedObject(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<SizedTypeDescr>());
    MOZ_ASSERT(args[1].isObject() && args[1].toObject().is<TypedObject>());
    MOZ_ASSERT(args[2].isInt32());

    Rooted<SizedTypeDescr*> descr(cx, &args[0].toObject().as<SizedTypeDescr>());
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
js::AttachTypedObject(ThreadSafeContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 3);
    MOZ_ASSERT(args[2].isInt32());

    OutlineTypedObject &handle = args[0].toObject().as<OutlineTypedObject>();
    TypedObject &target = args[1].toObject().as<TypedObject>();
    MOZ_ASSERT(!handle.isAttached());
    size_t offset = args[2].toInt32();

    if (cx->isForkJoinContext()) {
        LockedJSContext ncx(cx->asForkJoinContext());
        handle.attach(ncx, target, offset);
    } else {
        handle.attach(cx->asJSContext(), target, offset);
    }
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::AttachTypedObjectJitInfo,
                                      AttachTypedObjectJitInfo,
                                      js::AttachTypedObject);

bool
js::SetTypedObjectOffset(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 2);
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());
    MOZ_ASSERT(args[1].isInt32());

    OutlineTypedObject &typedObj = args[0].toObject().as<OutlineTypedObject>();
    int32_t offset = args[1].toInt32();

    MOZ_ASSERT(typedObj.isAttached());
    typedObj.setData(typedObj.typedMemBase() + offset);
    args.rval().setUndefined();
    return true;
}

bool
js::intrinsic_SetTypedObjectOffset(JSContext *cx, unsigned argc, Value *vp)
{
    
    
    return SetTypedObjectOffset(cx, argc, vp);
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::intrinsic_SetTypedObjectOffsetJitInfo,
                                      SetTypedObjectJitInfo,
                                      SetTypedObjectOffset);

bool
js::ObjectIsTypeDescr(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<TypeDescr>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsTypeDescrJitInfo, ObjectIsTypeDescrJitInfo,
                                      js::ObjectIsTypeDescr);

bool
js::ObjectIsTypedObject(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    args.rval().setBoolean(args[0].toObject().is<TypedObject>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsTypedObjectJitInfo,
                                      ObjectIsTypedObjectJitInfo,
                                      js::ObjectIsTypedObject);

bool
js::ObjectIsOpaqueTypedObject(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    JSObject &obj = args[0].toObject();
    args.rval().setBoolean(obj.is<TypedObject>() && obj.as<TypedObject>().opaque());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsOpaqueTypedObjectJitInfo,
                                      ObjectIsOpaqueTypedObjectJitInfo,
                                      js::ObjectIsOpaqueTypedObject);

bool
js::ObjectIsTransparentTypedObject(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    JSObject &obj = args[0].toObject();
    args.rval().setBoolean(obj.is<TypedObject>() && !obj.as<TypedObject>().opaque());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ObjectIsTransparentTypedObjectJitInfo,
                                      ObjectIsTransparentTypedObjectJitInfo,
                                      js::ObjectIsTransparentTypedObject);

bool
js::TypeDescrIsSimpleType(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<js::TypeDescr>());
    args.rval().setBoolean(args[0].toObject().is<js::SimpleTypeDescr>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::TypeDescrIsSimpleTypeJitInfo,
                                      TypeDescrIsSimpleTypeJitInfo,
                                      js::TypeDescrIsSimpleType);

bool
js::TypeDescrIsArrayType(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<js::TypeDescr>());
    JSObject& obj = args[0].toObject();
    args.rval().setBoolean(obj.is<js::SizedArrayTypeDescr>() ||
                           obj.is<js::UnsizedArrayTypeDescr>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::TypeDescrIsArrayTypeJitInfo,
                                      TypeDescrIsArrayTypeJitInfo,
                                      js::TypeDescrIsArrayType);

bool
js::TypeDescrIsSizedArrayType(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<js::TypeDescr>());
    args.rval().setBoolean(args[0].toObject().is<js::SizedArrayTypeDescr>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::TypeDescrIsSizedArrayTypeJitInfo,
                                      TypeDescrIsSizedArrayTypeJitInfo,
                                      js::TypeDescrIsSizedArrayType);

bool
js::TypeDescrIsUnsizedArrayType(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isObject());
    MOZ_ASSERT(args[0].toObject().is<js::TypeDescr>());
    args.rval().setBoolean(args[0].toObject().is<js::UnsizedArrayTypeDescr>());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::TypeDescrIsUnsizedArrayTypeJitInfo,
                                      TypeDescrIsUnsizedArrayTypeJitInfo,
                                      js::TypeDescrIsUnsizedArrayType);

bool
js::TypedObjectIsAttached(ThreadSafeContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();
    args.rval().setBoolean(typedObj.isAttached());
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::TypedObjectIsAttachedJitInfo,
                                      TypedObjectIsAttachedJitInfo,
                                      js::TypedObjectIsAttached);

bool
js::ClampToUint8(ThreadSafeContext *, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    MOZ_ASSERT(args.length() == 1);
    MOZ_ASSERT(args[0].isNumber());
    args.rval().setNumber(ClampDoubleToUint8(args[0].toNumber()));
    return true;
}

JS_JITINFO_NATIVE_PARALLEL_THREADSAFE(js::ClampToUint8JitInfo, ClampToUint8JitInfo,
                                      js::ClampToUint8);

bool
js::GetTypedObjectModule(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->getTypedObjectModule());
    return true;
}

bool
js::GetFloat32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->float32x4TypeDescr());
    return true;
}

bool
js::GetInt32x4TypeDescr(JSContext *cx, unsigned argc, Value *vp)
{
    CallArgs args = CallArgsFromVp(argc, vp);
    Rooted<GlobalObject*> global(cx, cx->global());
    MOZ_ASSERT(global);
    args.rval().setObject(global->int32x4TypeDescr());
    return true;
}

#define JS_STORE_SCALAR_CLASS_IMPL(_constant, T, _name)                         \
bool                                                                            \
js::StoreScalar##T::Func(ThreadSafeContext *, unsigned argc, Value *vp)         \
{                                                                               \
    CallArgs args = CallArgsFromVp(argc, vp);                                   \
    MOZ_ASSERT(args.length() == 3);                                             \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());     \
    MOZ_ASSERT(args[1].isInt32());                                              \
    MOZ_ASSERT(args[2].isNumber());                                             \
                                                                                \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                   \
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
    MOZ_ASSERT(args.length() == 3);                                             \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());     \
    MOZ_ASSERT(args[1].isInt32());                                              \
                                                                                \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                   \
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
    MOZ_ASSERT(args.length() == 2);                                                     \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());             \
    MOZ_ASSERT(args[1].isInt32());                                                      \
                                                                                        \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();                       \
    int32_t offset = args[1].toInt32();                                                 \
                                                                                        \
    /* Should be guaranteed by the typed objects API: */                                \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                           \
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
    MOZ_ASSERT(args.length() == 2);                                             \
    MOZ_ASSERT(args[0].isObject() && args[0].toObject().is<TypedObject>());     \
    MOZ_ASSERT(args[1].isInt32());                                              \
                                                                                \
    TypedObject &typedObj = args[0].toObject().as<TypedObject>();               \
    int32_t offset = args[1].toInt32();                                         \
                                                                                \
    /* Should be guaranteed by the typed objects API: */                        \
    MOZ_ASSERT(offset % MOZ_ALIGNOF(T) == 0);                                   \
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
    MOZ_ASSERT(v.isObjectOrNull()); 
    *heap = v.toObjectOrNull();
}

void
StoreReferenceHeapPtrString::store(HeapPtrString *heap, const Value &v)
{
    MOZ_ASSERT(v.isString()); 
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




template<typename V>
static void
visitReferences(SizedTypeDescr &descr,
                uint8_t *mem,
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

      case type::SizedArray:
      {
        SizedArrayTypeDescr &arrayDescr = descr.as<SizedArrayTypeDescr>();
        SizedTypeDescr &elementDescr = arrayDescr.maybeForwardedElementType();
        for (int32_t i = 0; i < arrayDescr.length(); i++) {
            visitReferences(elementDescr, mem, visitor);
            mem += elementDescr.size();
        }
        return;
      }

      case type::UnsizedArray:
      {
        MOZ_CRASH("Only Sized Type representations");
      }

      case type::Struct:
      {
        StructTypeDescr &structDescr = descr.as<StructTypeDescr>();
        for (size_t i = 0; i < structDescr.maybeForwardedFieldCount(); i++) {
            SizedTypeDescr &descr = structDescr.maybeForwardedFieldDescr(i);
            size_t offset = structDescr.maybeForwardedFieldOffset(i);
            visitReferences(descr, mem + offset, visitor);
        }
        return;
      }
    }

    MOZ_CRASH("Invalid type repr kind");
}




namespace js {
class MemoryInitVisitor {
    const JSRuntime *rt_;

  public:
    explicit MemoryInitVisitor(const JSRuntime *rt)
      : rt_(rt)
    {}

    void visitReference(ReferenceTypeDescr &descr, uint8_t *mem);
};
} 

void
js::MemoryInitVisitor::visitReference(ReferenceTypeDescr &descr, uint8_t *mem)
{
    switch (descr.type()) {
      case ReferenceTypeDescr::TYPE_ANY:
      {
        js::HeapValue *heapValue = reinterpret_cast<js::HeapValue *>(mem);
        heapValue->init(UndefinedValue());
        return;
      }

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        js::HeapPtrObject *objectPtr =
            reinterpret_cast<js::HeapPtrObject *>(mem);
        objectPtr->init(nullptr);
        return;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        js::HeapPtrString *stringPtr =
            reinterpret_cast<js::HeapPtrString *>(mem);
        stringPtr->init(rt_->emptyString);
        return;
      }
    }

    MOZ_CRASH("Invalid kind");
}

void
SizedTypeDescr::initInstances(const JSRuntime *rt, uint8_t *mem, size_t length)
{
    MOZ_ASSERT(length >= 1);

    MemoryInitVisitor visitor(rt);

    
    memset(mem, 0, size());
    if (opaque())
        visitReferences(*this, mem, visitor);

    
    uint8_t *target = mem;
    for (size_t i = 1; i < length; i++) {
        target += size();
        memcpy(target, mem, size());
    }
}




namespace js {
class MemoryTracingVisitor {
    JSTracer *trace_;

  public:

    explicit MemoryTracingVisitor(JSTracer *trace)
      : trace_(trace)
    {}

    void visitReference(ReferenceTypeDescr &descr, uint8_t *mem);
};
} 

void
js::MemoryTracingVisitor::visitReference(ReferenceTypeDescr &descr, uint8_t *mem)
{
    switch (descr.type()) {
      case ReferenceTypeDescr::TYPE_ANY:
      {
        js::HeapValue *heapValue = reinterpret_cast<js::HeapValue *>(mem);
        gc::MarkValue(trace_, heapValue, "reference-val");
        return;
      }

      case ReferenceTypeDescr::TYPE_OBJECT:
      {
        js::HeapPtrObject *objectPtr =
            reinterpret_cast<js::HeapPtrObject *>(mem);
        if (*objectPtr)
            gc::MarkObject(trace_, objectPtr, "reference-obj");
        return;
      }

      case ReferenceTypeDescr::TYPE_STRING:
      {
        js::HeapPtrString *stringPtr =
            reinterpret_cast<js::HeapPtrString *>(mem);
        if (*stringPtr)
            gc::MarkString(trace_, stringPtr, "reference-str");
        return;
      }
    }

    MOZ_CRASH("Invalid kind");
}

void
SizedTypeDescr::traceInstances(JSTracer *trace, uint8_t *mem, size_t length)
{
    MemoryTracingVisitor visitor(trace);

    for (size_t i = 0; i < length; i++) {
        visitReferences(*this, mem, visitor);
        mem += size();
    }
}

