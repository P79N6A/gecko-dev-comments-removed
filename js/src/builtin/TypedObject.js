#include "TypedObjectConstants.h"






#define DESCR_KIND(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_KIND)
#define DESCR_STRING_REPR(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRING_REPR)
#define DESCR_ALIGNMENT(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_ALIGNMENT)
#define DESCR_SIZE(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_SIZE)
#define DESCR_OPAQUE(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_OPAQUE)
#define DESCR_TYPE(obj)   \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_TYPE)
#define DESCR_ARRAY_ELEMENT_TYPE(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_ARRAY_ELEM_TYPE)
#define DESCR_SIZED_ARRAY_LENGTH(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_SIZED_ARRAY_LENGTH))
#define DESCR_STRUCT_FIELD_NAMES(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRUCT_FIELD_NAMES)
#define DESCR_STRUCT_FIELD_TYPES(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRUCT_FIELD_TYPES)
#define DESCR_STRUCT_FIELD_OFFSETS(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS)



#define TYPROTO_DESCR(obj) \
    UnsafeGetReservedSlot(obj, JS_TYPROTO_SLOT_DESCR)



#define TYPEDOBJ_BYTEOFFSET(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_BUFVIEW_SLOT_BYTEOFFSET))
#define TYPEDOBJ_OWNER(obj) \
    UnsafeGetReservedSlot(obj, JS_BUFVIEW_SLOT_OWNER)
#define TYPEDOBJ_LENGTH(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_BUFVIEW_SLOT_LENGTH))

#define HAS_PROPERTY(obj, prop) \
    callFunction(std_Object_hasOwnProperty, obj, prop)

function TypedObjectTypeDescr(typedObj) {
  return TYPROTO_DESCR(std_Object_getPrototypeOf(typedObj));
}












function TypedObjectGet(descr, typedObj, offset) {
  assert(IsObject(descr) && ObjectIsTypeDescr(descr),
         "get() called with bad type descr");

  if (!TypedObjectIsAttached(typedObj))
    ThrowError(JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);

  switch (DESCR_KIND(descr)) {
  case JS_TYPEREPR_SCALAR_KIND:
    return TypedObjectGetScalar(descr, typedObj, offset);

  case JS_TYPEREPR_REFERENCE_KIND:
    return TypedObjectGetReference(descr, typedObj, offset);

  case JS_TYPEREPR_SIMD_KIND:
    return TypedObjectGetSimd(descr, typedObj, offset);

  case JS_TYPEREPR_SIZED_ARRAY_KIND:
  case JS_TYPEREPR_STRUCT_KIND:
    return TypedObjectGetDerived(descr, typedObj, offset);

  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    assert(false, "Unhandled repr kind: " + DESCR_KIND(descr));
  }

  assert(false, "Unhandled kind: " + DESCR_KIND(descr));
  return undefined;
}

function TypedObjectGetDerived(descr, typedObj, offset) {
  assert(!TypeDescrIsSimpleType(descr),
         "getDerived() used with simple type");
  return NewDerivedTypedObject(descr, typedObj, offset);
}

function TypedObjectGetDerivedIf(descr, typedObj, offset, cond) {
  return (cond ? TypedObjectGetDerived(descr, typedObj, offset) : undefined);
}

function TypedObjectGetOpaque(descr, typedObj, offset) {
  assert(!TypeDescrIsSimpleType(descr),
         "getDerived() used with simple type");
  var opaqueTypedObj = NewOpaqueTypedObject(descr);
  AttachTypedObject(opaqueTypedObj, typedObj, offset);
  return opaqueTypedObj;
}

function TypedObjectGetOpaqueIf(descr, typedObj, offset, cond) {
  return (cond ? TypedObjectGetOpaque(descr, typedObj, offset) : undefined);
}

function TypedObjectGetScalar(descr, typedObj, offset) {
  var type = DESCR_TYPE(descr);
  switch (type) {
  case JS_SCALARTYPEREPR_INT8:
    return Load_int8(typedObj, offset);

  case JS_SCALARTYPEREPR_UINT8:
  case JS_SCALARTYPEREPR_UINT8_CLAMPED:
    return Load_uint8(typedObj, offset);

  case JS_SCALARTYPEREPR_INT16:
    return Load_int16(typedObj, offset);

  case JS_SCALARTYPEREPR_UINT16:
    return Load_uint16(typedObj, offset);

  case JS_SCALARTYPEREPR_INT32:
    return Load_int32(typedObj, offset);

  case JS_SCALARTYPEREPR_UINT32:
    return Load_uint32(typedObj, offset);

  case JS_SCALARTYPEREPR_FLOAT32:
    return Load_float32(typedObj, offset);

  case JS_SCALARTYPEREPR_FLOAT64:
    return Load_float64(typedObj, offset);
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}

function TypedObjectGetReference(descr, typedObj, offset) {
  var type = DESCR_TYPE(descr);
  switch (type) {
  case JS_REFERENCETYPEREPR_ANY:
    return Load_Any(typedObj, offset);

  case JS_REFERENCETYPEREPR_OBJECT:
    return Load_Object(typedObj, offset);

  case JS_REFERENCETYPEREPR_STRING:
    return Load_string(typedObj, offset);
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}

function TypedObjectGetSimd(descr, typedObj, offset) {
  var type = DESCR_TYPE(descr);
  switch (type) {
  case JS_SIMDTYPEREPR_FLOAT32:
    var x = Load_float32(typedObj, offset + 0);
    var y = Load_float32(typedObj, offset + 4);
    var z = Load_float32(typedObj, offset + 8);
    var w = Load_float32(typedObj, offset + 12);
    return GetFloat32x4TypeDescr()(x, y, z, w);

  case JS_SIMDTYPEREPR_INT32:
    var x = Load_int32(typedObj, offset + 0);
    var y = Load_int32(typedObj, offset + 4);
    var z = Load_int32(typedObj, offset + 8);
    var w = Load_int32(typedObj, offset + 12);
    return GetInt32x4TypeDescr()(x, y, z, w);
  }

  assert(false, "Unhandled SIMD type: " + type);
  return undefined;
}









function TypedObjectSet(descr, typedObj, offset, fromValue) {
  if (!TypedObjectIsAttached(typedObj))
    ThrowError(JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);

  
  
  
  if (IsObject(fromValue) && ObjectIsTypedObject(fromValue)) {
    if (!descr.variable && DescrsEquiv(descr, TypedObjectTypeDescr(fromValue))) {
      if (!TypedObjectIsAttached(fromValue))
        ThrowError(JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);

      var size = DESCR_SIZE(descr);
      Memcpy(typedObj, offset, fromValue, 0, size);
      return;
    }
  }

  switch (DESCR_KIND(descr)) {
  case JS_TYPEREPR_SCALAR_KIND:
    TypedObjectSetScalar(descr, typedObj, offset, fromValue);
    return;

  case JS_TYPEREPR_REFERENCE_KIND:
    TypedObjectSetReference(descr, typedObj, offset, fromValue);
    return;

  case JS_TYPEREPR_SIMD_KIND:
    TypedObjectSetSimd(descr, typedObj, offset, fromValue);
    return;

  case JS_TYPEREPR_SIZED_ARRAY_KIND:
    var length = DESCR_SIZED_ARRAY_LENGTH(descr);
    if (TypedObjectSetArray(descr, length, typedObj, offset, fromValue))
      return;
    break;

  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    var length = typedObj.length;
    if (TypedObjectSetArray(descr, length, typedObj, offset, fromValue))
      return;
    break;

  case JS_TYPEREPR_STRUCT_KIND:
    if (!IsObject(fromValue))
      break;

    
    var fieldNames = DESCR_STRUCT_FIELD_NAMES(descr);
    var fieldDescrs = DESCR_STRUCT_FIELD_TYPES(descr);
    var fieldOffsets = DESCR_STRUCT_FIELD_OFFSETS(descr);
    for (var i = 0; i < fieldNames.length; i++) {
      var fieldName = fieldNames[i];
      var fieldDescr = fieldDescrs[i];
      var fieldOffset = fieldOffsets[i];
      var fieldValue = fromValue[fieldName];
      TypedObjectSet(fieldDescr, typedObj, offset + fieldOffset, fieldValue);
    }
    return;
  }

  ThrowError(JSMSG_CANT_CONVERT_TO,
             typeof(fromValue),
             DESCR_STRING_REPR(descr));
}

function TypedObjectSetArray(descr, length, typedObj, offset, fromValue) {
  if (!IsObject(fromValue))
    return false;

  
  if (fromValue.length !== length)
    return false;

  
  if (length > 0) {
    var elemDescr = DESCR_ARRAY_ELEMENT_TYPE(descr);
    var elemSize = DESCR_SIZE(elemDescr);
    var elemOffset = offset;
    for (var i = 0; i < length; i++) {
      TypedObjectSet(elemDescr, typedObj, elemOffset, fromValue[i]);
      elemOffset += elemSize;
    }
  }
  return true;
}


function TypedObjectSetScalar(descr, typedObj, offset, fromValue) {
  assert(DESCR_KIND(descr) === JS_TYPEREPR_SCALAR_KIND,
         "Expected scalar type descriptor");
  var type = DESCR_TYPE(descr);
  switch (type) {
  case JS_SCALARTYPEREPR_INT8:
    return Store_int8(typedObj, offset,
                     TO_INT32(fromValue) & 0xFF);

  case JS_SCALARTYPEREPR_UINT8:
    return Store_uint8(typedObj, offset,
                      TO_UINT32(fromValue) & 0xFF);

  case JS_SCALARTYPEREPR_UINT8_CLAMPED:
    var v = ClampToUint8(+fromValue);
    return Store_int8(typedObj, offset, v);

  case JS_SCALARTYPEREPR_INT16:
    return Store_int16(typedObj, offset,
                      TO_INT32(fromValue) & 0xFFFF);

  case JS_SCALARTYPEREPR_UINT16:
    return Store_uint16(typedObj, offset,
                       TO_UINT32(fromValue) & 0xFFFF);

  case JS_SCALARTYPEREPR_INT32:
    return Store_int32(typedObj, offset,
                      TO_INT32(fromValue));

  case JS_SCALARTYPEREPR_UINT32:
    return Store_uint32(typedObj, offset,
                       TO_UINT32(fromValue));

  case JS_SCALARTYPEREPR_FLOAT32:
    return Store_float32(typedObj, offset, +fromValue);

  case JS_SCALARTYPEREPR_FLOAT64:
    return Store_float64(typedObj, offset, +fromValue);
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}

function TypedObjectSetReference(descr, typedObj, offset, fromValue) {
  var type = DESCR_TYPE(descr);
  switch (type) {
  case JS_REFERENCETYPEREPR_ANY:
    return Store_Any(typedObj, offset, fromValue);

  case JS_REFERENCETYPEREPR_OBJECT:
    var value = (fromValue === null ? fromValue : ToObject(fromValue));
    return Store_Object(typedObj, offset, value);

  case JS_REFERENCETYPEREPR_STRING:
    return Store_string(typedObj, offset, ToString(fromValue));
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}


function TypedObjectSetSimd(descr, typedObj, offset, fromValue) {
  
  
  
  
  ThrowError(JSMSG_CANT_CONVERT_TO,
             typeof(fromValue),
             DESCR_STRING_REPR(descr));
}







function ConvertAndCopyTo(destDescr,
                          destTypedObj,
                          destOffset,
                          fromValue)
{
  assert(IsObject(destDescr) && ObjectIsTypeDescr(destDescr),
         "ConvertAndCopyTo: not type obj");
  assert(IsObject(destTypedObj) && ObjectIsTypedObject(destTypedObj),
         "ConvertAndCopyTo: not type typedObj");

  if (!TypedObjectIsAttached(destTypedObj))
    ThrowError(JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);

  TypedObjectSet(destDescr, destTypedObj, destOffset, fromValue);
}


function Reify(sourceDescr,
               sourceTypedObj,
               sourceOffset) {
  assert(IsObject(sourceDescr) && ObjectIsTypeDescr(sourceDescr),
         "Reify: not type obj");
  assert(IsObject(sourceTypedObj) && ObjectIsTypedObject(sourceTypedObj),
         "Reify: not type typedObj");

  if (!TypedObjectIsAttached(sourceTypedObj))
    ThrowError(JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);

  return TypedObjectGet(sourceDescr, sourceTypedObj, sourceOffset);
}


function TypeDescrEquivalent(otherDescr) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  if (!IsObject(otherDescr) || !ObjectIsTypeDescr(otherDescr))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  return DescrsEquiv(this, otherDescr);
}





















function TypedArrayRedimension(newArrayType) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (!IsObject(newArrayType) || !ObjectIsTypeDescr(newArrayType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  
  
  var oldArrayType = TypedObjectTypeDescr(this);
  var oldArrayReprKind = DESCR_KIND(oldArrayType);
  var oldElementType = oldArrayType;
  var oldElementCount = 1;
  switch (oldArrayReprKind) {
  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    oldElementCount *= this.length;
    oldElementType = oldElementType.elementType;
    break;

  case JS_TYPEREPR_SIZED_ARRAY_KIND:
    break;

  default:
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  }
  while (DESCR_KIND(oldElementType) === JS_TYPEREPR_SIZED_ARRAY_KIND) {
    oldElementCount *= oldElementType.length;
    oldElementType = oldElementType.elementType;
  }

  
  
  var newElementType = newArrayType;
  var newElementCount = 1;
  while (DESCR_KIND(newElementType) == JS_TYPEREPR_SIZED_ARRAY_KIND) {
    newElementCount *= newElementType.length;
    newElementType = newElementType.elementType;
  }

  
  if (oldElementCount !== newElementCount) {
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  }

  
  if (!DescrsEquiv(oldElementType, newElementType)) {
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  }

  
  assert(DESCR_SIZE(oldArrayType) == DESCR_SIZE(newArrayType),
         "Byte sizes should be equal");

  
  return NewDerivedTypedObject(newArrayType, this, 0);
}




function SimdProtoString(type) {
  switch (type) {
  case JS_SIMDTYPEREPR_INT32:
    return "int32x4";
  case JS_SIMDTYPEREPR_FLOAT32:
    return "float32x4";
  }

  assert(false, "Unhandled type constant");
  return undefined;
}

function SimdToSource() {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_INCOMPATIBLE_PROTO, "SIMD", "toSource", typeof this);

  var descr = TypedObjectTypeDescr(this);

  if (DESCR_KIND(descr) != JS_TYPEREPR_SIMD_KIND)
    ThrowError(JSMSG_INCOMPATIBLE_PROTO, "SIMD", "toSource", typeof this);

  var type = DESCR_TYPE(descr);
  return SimdProtoString(type)+"("+this.x+", "+this.y+", "+this.z+", "+this.w+")";
}




function DescrsEquiv(descr1, descr2) {
  assert(IsObject(descr1) && ObjectIsTypeDescr(descr1), "descr1 not descr");
  assert(IsObject(descr2) && ObjectIsTypeDescr(descr2), "descr2 not descr");

  
  
  
  
  

  return DESCR_STRING_REPR(descr1) === DESCR_STRING_REPR(descr2);
}




function DescrToSource() {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(JSMSG_INCOMPATIBLE_PROTO, "Type", "toSource", "value");

  return DESCR_STRING_REPR(this);
}


function ArrayShorthand(...dims) {
  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  var T = GetTypedObjectModule();

  if (dims.length == 0)
    return new T.ArrayType(this);

  var accum = this;
  for (var i = dims.length - 1; i >= 0; i--)
    accum = new T.ArrayType(accum).dimension(dims[i]);
  return accum;
}







function StorageOfTypedObject(obj) {
  if (IsObject(obj)) {
    if (ObjectIsOpaqueTypedObject(obj))
      return null;

    if (ObjectIsTransparentTypedObject(obj)) {
      var descr = TypedObjectTypeDescr(obj);
      var byteLength;
      if (DESCR_KIND(descr) == JS_TYPEREPR_UNSIZED_ARRAY_KIND)
        byteLength = DESCR_SIZE(descr.elementType) * obj.length;
      else
        byteLength = DESCR_SIZE(descr);

      return { buffer: TYPEDOBJ_OWNER(obj),
               byteLength: byteLength,
               byteOffset: TYPEDOBJ_BYTEOFFSET(obj) };
    }
  }

  ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
}





function TypeOfTypedObject(obj) {
  if (IsObject(obj) && ObjectIsTypedObject(obj))
    return TypedObjectTypeDescr(obj);

  
  
  
  
  
  
  var T = GetTypedObjectModule();
  switch (typeof obj) {
    case "object": return T.Object;
    case "function": return T.Object;
    case "string": return T.String;
    case "number": return T.float64;
    case "undefined": return T.Any;
    default: return T.Any;
  }
}





function TypedObjectArrayTypeBuild(a,b,c) {
  
  

  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var kind = DESCR_KIND(this);
  switch (kind) {
  case JS_TYPEREPR_SIZED_ARRAY_KIND:
    if (typeof a === "function") 
      return BuildTypedSeqImpl(this, this.length, 1, a);
    else if (typeof a === "number" && typeof b === "function")
      return BuildTypedSeqImpl(this, this.length, a, b);
    else if (typeof a === "number")
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
    else
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    var len = a;
    if (typeof b === "function")
      return BuildTypedSeqImpl(this, len, 1, b);
    else if (typeof b === "number" && typeof c === "function")
      return BuildTypedSeqImpl(this, len, b, c);
    else if (typeof b === "number")
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
    else
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  default:
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  }
}


function TypedObjectArrayTypeFrom(a, b, c) {
  

  if (!IsObject(this) || !ObjectIsTypeDescr(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  var untypedInput = !IsObject(a) || !ObjectIsTypedObject(a);

  
  
  
  

  if (untypedInput) {
    var explicitDepth = (b === 1);
    if (explicitDepth && IsCallable(c))
      return MapUntypedSeqImpl(a, this, c);
    else if (IsCallable(b))
      return MapUntypedSeqImpl(a, this, b);
    else
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  } else {
    var explicitDepth = (typeof b === "number");
    if (explicitDepth && IsCallable(c))
      return MapTypedSeqImpl(a, b, this, c);
    else if (IsCallable(b))
      return MapTypedSeqImpl(a, 1, this, b);
    else if (explicitDepth)
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
    else
      ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  }
}


function TypedArrayMap(a, b) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  
  if (typeof a === "number" && typeof b === "function")
    return MapTypedSeqImpl(this, a, thisType, b);
  else if (typeof a === "function")
    return MapTypedSeqImpl(this, 1, thisType, a);
  ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
}


function TypedArrayMapPar(a, b) {
  

  
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    return callFunction(TypedArrayMap, this, a, b);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    return callFunction(TypedArrayMap, this, a, b);

  if (typeof a === "number" && IsCallable(b))
    return MapTypedParImpl(this, a, thisType, b);
  else if (IsCallable(a))
    return MapTypedParImpl(this, 1, thisType, a);
  return callFunction(TypedArrayMap, this, a, b);
}


function TypedArrayReduce(a, b) {
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (a !== undefined && typeof a !== "function")
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  var outputType = thisType.elementType;
  return ReduceTypedSeqImpl(this, outputType, a, b);
}


function TypedArrayScatter(a, b, c, d) {
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (!IsObject(a) || !ObjectIsTypeDescr(a) || !TypeDescrIsSizedArrayType(a))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (d !== undefined && typeof d !== "function")
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  return ScatterTypedSeqImpl(this, a, b, c, d);
}


function TypedArrayFilter(func) {
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TypedObjectTypeDescr(this);
  if (!TypeDescrIsArrayType(thisType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (typeof func !== "function")
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  return FilterTypedSeqImpl(this, func);
}




function TypedObjectArrayTypeBuildPar(a,b,c) {
  return callFunction(TypedObjectArrayTypeBuild, this, a, b, c);
}


function TypedObjectArrayTypeFromPar(a,b,c) {
  

  
  
  if (!IsObject(this) || !ObjectIsTypeDescr(this) || !TypeDescrIsArrayType(this))
    return callFunction(TypedObjectArrayTypeFrom, this, a, b, c);
  if (!IsObject(a) || !ObjectIsTypedObject(a))
    return callFunction(TypedObjectArrayTypeFrom, this, a, b, c);

  
  if (typeof b === "number" && IsCallable(c))
    return MapTypedParImpl(a, b, this, c);
  if (IsCallable(b))
    return MapTypedParImpl(a, 1, this, b);
  return callFunction(TypedObjectArrayTypeFrom, this, a, b, c);
}


function TypedArrayReducePar(a, b) {
  return callFunction(TypedArrayReduce, this, a, b);
}


function TypedArrayScatterPar(a, b, c, d) {
  return callFunction(TypedArrayScatter, this, a, b, c, d);
}


function TypedArrayFilterPar(func) {
  return callFunction(TypedArrayFilter, this, func);
}


function NUM_BYTES(bits) {
  return (bits + 7) >> 3;
}
function SET_BIT(data, index) {
  var word = index >> 3;
  var mask = 1 << (index & 0x7);
  data[word] |= mask;
}
function GET_BIT(data, index) {
  var word = index >> 3;
  var mask = 1 << (index & 0x7);
  return (data[word] & mask) != 0;
}


function BuildTypedSeqImpl(arrayType, len, depth, func) {
  assert(IsObject(arrayType) && ObjectIsTypeDescr(arrayType), "Build called on non-type-object");

  if (depth <= 0 || TO_INT32(depth) !== depth)
    
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  
  
  
  
  
  var [iterationSpace, grainType, totalLength] =
    ComputeIterationSpace(arrayType, depth, len);

  
  var result = arrayType.variable ? new arrayType(len) : new arrayType();

  var indices = NewDenseArray(depth);
  for (var i = 0; i < depth; i++) {
    indices[i] = 0;
  }

  var grainTypeIsComplex = !TypeDescrIsSimpleType(grainType);
  var size = DESCR_SIZE(grainType);
  var outOffset = 0;
  for (i = 0; i < totalLength; i++) {
    
    var userOutPointer = TypedObjectGetOpaqueIf(grainType, result, outOffset,
                                                grainTypeIsComplex);

    
    callFunction(std_Array_push, indices, userOutPointer);
    var r = callFunction(std_Function_apply, func, undefined, indices);
    callFunction(std_Array_pop, indices);
    if (r !== undefined)
      TypedObjectSet(grainType, result, outOffset, r); 

    
    IncrementIterationSpace(indices, iterationSpace);
    outOffset += size;
  }

  return result;
}

function ComputeIterationSpace(arrayType, depth, len) {
  assert(IsObject(arrayType) && ObjectIsTypeDescr(arrayType), "ComputeIterationSpace called on non-type-object");
  assert(TypeDescrIsArrayType(arrayType), "ComputeIterationSpace called on non-array-type");
  assert(depth > 0, "ComputeIterationSpace called on non-positive depth");
  var iterationSpace = NewDenseArray(depth);
  iterationSpace[0] = len;
  var totalLength = len;
  var grainType = arrayType.elementType;

  for (var i = 1; i < depth; i++) {
    if (TypeDescrIsArrayType(grainType)) {
      var grainLen = grainType.length;
      iterationSpace[i] = grainLen;
      totalLength *= grainLen;
      grainType = grainType.elementType;
    } else {
      
      ThrowError(JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);
    }
  }
  return [iterationSpace, grainType, totalLength];
}

function IncrementIterationSpace(indices, iterationSpace) {
  
  
  
  
  
  

  assert(indices.length === iterationSpace.length,
         "indices dimension must equal iterationSpace dimension.");
  var n = indices.length - 1;
  while (true) {
    indices[n] += 1;
    if (indices[n] < iterationSpace[n])
      return;

    assert(indices[n] === iterationSpace[n],
         "Components of indices must match those of iterationSpace.");
    indices[n] = 0;
    if (n == 0)
      return;

    n -= 1;
  }
}


function MapUntypedSeqImpl(inArray, outputType, maybeFunc) {
  assert(IsObject(outputType), "1. Map/From called on non-object outputType");
  assert(ObjectIsTypeDescr(outputType), "1. Map/From called on non-type-object outputType");
  inArray = ToObject(inArray);
  assert(TypeDescrIsArrayType(outputType), "Map/From called on non array-type outputType");

  if (!IsCallable(maybeFunc))
    ThrowError(JSMSG_NOT_FUNCTION, DecompileArg(0, maybeFunc));
  var func = maybeFunc;

  
  

  var outLength = outputType.variable ? inArray.length : outputType.length;
  var outGrainType = outputType.elementType;

  
  var result = outputType.variable ? new outputType(inArray.length) : new outputType();

  var outUnitSize = DESCR_SIZE(outGrainType);
  var outGrainTypeIsComplex = !TypeDescrIsSimpleType(outGrainType);
  var outOffset = 0;

  
  

  for (var i = 0; i < outLength; i++) {
    

    if (i in inArray) { 
      
      var element = inArray[i];

      
      var out = TypedObjectGetOpaqueIf(outGrainType, result, outOffset,
                                       outGrainTypeIsComplex);

      
      var r = func(element, i, inArray, out);

      if (r !== undefined)
        TypedObjectSet(outGrainType, result, outOffset, r); 
    }

    
    outOffset += outUnitSize;
  }

  return result;
}


function MapTypedSeqImpl(inArray, depth, outputType, func) {
  assert(IsObject(outputType) && ObjectIsTypeDescr(outputType), "2. Map/From called on non-type-object outputType");
  assert(IsObject(inArray) && ObjectIsTypedObject(inArray), "Map/From called on non-object or untyped input array.");
  assert(TypeDescrIsArrayType(outputType), "Map/From called on non array-type outputType");

  if (depth <= 0 || TO_INT32(depth) !== depth)
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  
  var inputType = TypeOfTypedObject(inArray);
  var [inIterationSpace, inGrainType, _] =
    ComputeIterationSpace(inputType, depth, inArray.length);
  if (!IsObject(inGrainType) || !ObjectIsTypeDescr(inGrainType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var [iterationSpace, outGrainType, totalLength] =
    ComputeIterationSpace(outputType, depth, outputType.variable ? inArray.length : outputType.length);
  for (var i = 0; i < depth; i++)
    if (inIterationSpace[i] !== iterationSpace[i])
      
      ThrowError(JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);

  
  var result = outputType.variable ? new outputType(inArray.length) : new outputType();

  var inGrainTypeIsComplex = !TypeDescrIsSimpleType(inGrainType);
  var outGrainTypeIsComplex = !TypeDescrIsSimpleType(outGrainType);

  var inOffset = 0;
  var outOffset = 0;

  var isDepth1Simple = depth == 1 && !(inGrainTypeIsComplex || outGrainTypeIsComplex);

  var inUnitSize = isDepth1Simple ? 0 : DESCR_SIZE(inGrainType);
  var outUnitSize = isDepth1Simple ? 0 : DESCR_SIZE(outGrainType);

  

  function DoMapTypedSeqDepth1() {
    for (var i = 0; i < totalLength; i++) {
      

      
      var element = TypedObjectGet(inGrainType, inArray, inOffset);
      var out = TypedObjectGetOpaqueIf(outGrainType, result, outOffset,
                                       outGrainTypeIsComplex);

      
      var r = func(element, i, inArray, out);
      if (r !== undefined)
        TypedObjectSet(outGrainType, result, outOffset, r); 

      
      inOffset += inUnitSize;
      outOffset += outUnitSize;
    }

    return result;
  }

  function DoMapTypedSeqDepth1Simple(inArray, totalLength, func, result) {
    for (var i = 0; i < totalLength; i++) {
      var r = func(inArray[i], i, inArray, undefined);
      if (r !== undefined)
        result[i] = r;
    }

    return result;
  }

  function DoMapTypedSeqDepthN() {
    var indices = new Uint32Array(depth);

    for (var i = 0; i < totalLength; i++) {
      
      var element = TypedObjectGet(inGrainType, inArray, inOffset);
      var out = TypedObjectGetOpaqueIf(outGrainType, result, outOffset,
                                       outGrainTypeIsComplex);

      
      var args = [element];
      callFunction(std_Function_apply, std_Array_push, args, indices);
      callFunction(std_Array_push, args, inArray, out);
      var r = callFunction(std_Function_apply, func, void 0, args);
      if (r !== undefined)
        TypedObjectSet(outGrainType, result, outOffset, r); 

      
      inOffset += inUnitSize;
      outOffset += outUnitSize;
      IncrementIterationSpace(indices, iterationSpace);
    }

    return result;
  }

  if (isDepth1Simple)
    return DoMapTypedSeqDepth1Simple(inArray, totalLength, func, result);

  if (depth == 1)
    return DoMapTypedSeqDepth1();

  return DoMapTypedSeqDepthN();
}


function MapTypedParImpl(inArray, depth, outputType, func) {
  assert(IsObject(outputType) && ObjectIsTypeDescr(outputType),
         "Map/From called on non-type-object outputType");
  assert(IsObject(inArray) && ObjectIsTypedObject(inArray),
         "Map/From called on non-object or untyped input array.");
  assert(TypeDescrIsArrayType(outputType),
         "Map/From called on non array-type outputType");
  assert(typeof depth === "number",
         "Map/From called with non-numeric depth");
  assert(IsCallable(func),
         "Map/From called on something not callable");

  var inArrayType = TypeOfTypedObject(inArray);

  if (ShouldForceSequential() ||
      depth <= 0 ||
      TO_INT32(depth) !== depth ||
      !TypeDescrIsArrayType(inArrayType) ||
      !TypeDescrIsUnsizedArrayType(outputType))
  {
    
    return MapTypedSeqImpl(inArray, depth, outputType, func);
  }

  switch (depth) {
  case 1:
    return MapTypedParImplDepth1(inArray, inArrayType, outputType, func);
  default:
    return MapTypedSeqImpl(inArray, depth, outputType, func);
  }
}

function RedirectPointer(typedObj, offset, outputIsScalar) {
  if (!outputIsScalar || !InParallelSection()) {
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    typedObj = NewDerivedTypedObject(TypedObjectTypeDescr(typedObj),
                                     typedObj, 0);
  }

  SetTypedObjectOffset(typedObj, offset);
  return typedObj;
}
SetScriptHints(RedirectPointer,         { inline: true });

function MapTypedParImplDepth1(inArray, inArrayType, outArrayType, func) {
  assert(IsObject(inArrayType) && ObjectIsTypeDescr(inArrayType) &&
         TypeDescrIsArrayType(inArrayType),
         "DoMapTypedParDepth1: invalid inArrayType");
  assert(IsObject(outArrayType) && ObjectIsTypeDescr(outArrayType) &&
         TypeDescrIsArrayType(outArrayType),
         "DoMapTypedParDepth1: invalid outArrayType");
  assert(IsObject(inArray) && ObjectIsTypedObject(inArray),
         "DoMapTypedParDepth1: invalid inArray");

  
  const inGrainType = inArrayType.elementType;
  const outGrainType = outArrayType.elementType;
  const inGrainTypeSize = DESCR_SIZE(inGrainType);
  const outGrainTypeSize = DESCR_SIZE(outGrainType);
  const inGrainTypeIsComplex = !TypeDescrIsSimpleType(inGrainType);
  const outGrainTypeIsComplex = !TypeDescrIsSimpleType(outGrainType);

  const length = inArray.length;
  const mode = undefined;

  const outArray = new outArrayType(length);
  if (length === 0)
    return outArray;

  const outGrainTypeIsTransparent = ObjectIsTransparentTypedObject(outArray);

  
  const slicesInfo = ComputeSlicesInfo(length);
  const numWorkers = ForkJoinNumWorkers();
  assert(numWorkers > 0, "Should have at least the main thread");
  const pointers = [];
  for (var i = 0; i < numWorkers; i++) {
    const inTypedObject = TypedObjectGetDerivedIf(inGrainType, inArray, 0,
                                                  inGrainTypeIsComplex);
    const outTypedObject = TypedObjectGetOpaqueIf(outGrainType, outArray, 0,
                                                  outGrainTypeIsComplex);
    ARRAY_PUSH(pointers, ({ inTypedObject: inTypedObject,
                            outTypedObject: outTypedObject }));
  }

  
  
  
  const inBaseOffset = TYPEDOBJ_BYTEOFFSET(inArray);

  ForkJoin(mapThread, 0, slicesInfo.count, ForkJoinMode(mode), outArray);
  return outArray;

  function mapThread(workerId, sliceStart, sliceEnd) {
    assert(TO_INT32(workerId) === workerId,
           "workerId not int: " + workerId);
    assert(workerId < pointers.length,
           "workerId too large: " + workerId + " >= " + pointers.length);

    var pointerIndex = InParallelSection() ? workerId : 0;
    assert(!!pointers[pointerIndex],
          "no pointer data for workerId: " + workerId);

    const { inTypedObject, outTypedObject } = pointers[pointerIndex];
    const sliceShift = slicesInfo.shift;
    var sliceId;

    while (GET_SLICE(sliceStart, sliceEnd, sliceId)) {
      const indexStart = SLICE_START_INDEX(sliceShift, sliceId);
      const indexEnd = SLICE_END_INDEX(sliceShift, indexStart, length);

      var inOffset = inBaseOffset + std_Math_imul(inGrainTypeSize, indexStart);
      var outOffset = std_Math_imul(outGrainTypeSize, indexStart);

      
      
      
      
      
      
      
      
      
      
      const endOffset = std_Math_imul(outGrainTypeSize, indexEnd);
      SetForkJoinTargetRegion(outArray, outOffset, endOffset);

      for (var i = indexStart; i < indexEnd; i++) {
        var inVal = (inGrainTypeIsComplex
                     ? RedirectPointer(inTypedObject, inOffset,
                                       outGrainTypeIsTransparent)
                     : inArray[i]);
        var outVal = (outGrainTypeIsComplex
                      ? RedirectPointer(outTypedObject, outOffset,
                                        outGrainTypeIsTransparent)
                      : undefined);
        const r = func(inVal, i, inArray, outVal);
        if (r !== undefined) {
          if (outGrainTypeIsComplex)
            SetTypedObjectValue(outGrainType, outArray, outOffset, r);
          else
            UnsafePutElements(outArray, i, r);
        }
        inOffset += inGrainTypeSize;
        outOffset += outGrainTypeSize;

#ifndef JSGC_FJGENERATIONAL
        
        
        
        
        
        
        
        if (outGrainTypeIsTransparent)
          ClearThreadLocalArenas();
#endif
      }
    }

    return sliceId;
  }

  return undefined;
}
SetScriptHints(MapTypedParImplDepth1,         { cloneAtCallsite: true });

function ReduceTypedSeqImpl(array, outputType, func, initial) {
  assert(IsObject(array) && ObjectIsTypedObject(array), "Reduce called on non-object or untyped input array.");
  assert(IsObject(outputType) && ObjectIsTypeDescr(outputType), "Reduce called on non-type-object outputType");

  var start, value;

  if (initial === undefined && array.length < 1)
    
    ThrowError(JSMSG_TYPEDOBJECT_ARRAYTYPE_BAD_ARGS);

  
  

  if (TypeDescrIsSimpleType(outputType)) {
    if (initial === undefined) {
      start = 1;
      value = array[0];
    } else {
      start = 0;
      value = outputType(initial);
    }

    for (var i = start; i < array.length; i++)
      value = outputType(func(value, array[i]));

  } else {
    if (initial === undefined) {
      start = 1;
      value = new outputType(array[0]);
    } else {
      start = 0;
      value = initial;
    }

    for (var i = start; i < array.length; i++)
      value = func(value, array[i]);
  }

  return value;
}

function ScatterTypedSeqImpl(array, outputType, indices, defaultValue, conflictFunc) {
  assert(IsObject(array) && ObjectIsTypedObject(array), "Scatter called on non-object or untyped input array.");
  assert(IsObject(outputType) && ObjectIsTypeDescr(outputType), "Scatter called on non-type-object outputType");
  assert(TypeDescrIsSizedArrayType(outputType), "Scatter called on non-sized array type");
  assert(conflictFunc === undefined || typeof conflictFunc === "function", "Scatter called with invalid conflictFunc");

  var result = new outputType();
  var bitvec = new Uint8Array(result.length);
  var elemType = outputType.elementType;
  var i, j;
  if (defaultValue !== elemType(undefined)) {
    for (i = 0; i < result.length; i++) {
      result[i] = defaultValue;
    }
  }

  for (i = 0; i < indices.length; i++) {
    j = indices[i];
    if (!GET_BIT(bitvec, j)) {
      result[j] = array[i];
      SET_BIT(bitvec, j);
    } else if (conflictFunc === undefined) {
      ThrowError(JSMSG_PAR_ARRAY_SCATTER_CONFLICT);
    } else {
      result[j] = conflictFunc(result[j], elemType(array[i]));
    }
  }
  return result;
}

function FilterTypedSeqImpl(array, func) {
  assert(IsObject(array) && ObjectIsTypedObject(array), "Filter called on non-object or untyped input array.");
  assert(typeof func === "function", "Filter called with non-function predicate");

  var arrayType = TypeOfTypedObject(array);
  if (!TypeDescrIsArrayType(arrayType))
    ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  var elementType = arrayType.elementType;
  var flags = new Uint8Array(NUM_BYTES(array.length));
  var count = 0;
  var size = DESCR_SIZE(elementType);
  var inOffset = 0;
  for (var i = 0; i < array.length; i++) {
    var v = TypedObjectGet(elementType, array, inOffset);
    if (func(v, i, array)) {
      SET_BIT(flags, i);
      count++;
    }
    inOffset += size;
  }

  var resultType = (arrayType.variable ? arrayType : arrayType.unsized);
  var result = new resultType(count);
  for (var i = 0, j = 0; i < array.length; i++) {
    if (GET_BIT(flags, i))
      result[j++] = array[i];
  }
  return result;
}
