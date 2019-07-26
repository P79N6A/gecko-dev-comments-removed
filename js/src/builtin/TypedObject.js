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
    TO_INT32(UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_ARRAY_ELEM_TYPE))
#define DESCR_SIZED_ARRAY_LENGTH(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_SIZED_ARRAY_LENGTH))
#define DESCR_STRUCT_FIELD_NAMES(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRUCT_FIELD_NAMES)
#define DESCR_STRUCT_FIELD_TYPES(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRUCT_FIELD_TYPES)
#define DESCR_STRUCT_FIELD_OFFSETS(obj) \
    UnsafeGetReservedSlot(obj, JS_DESCR_SLOT_STRUCT_FIELD_OFFSETS)



#define TYPEDOBJ_BYTEOFFSET(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEDOBJ_SLOT_BYTEOFFSET))
#define TYPEDOBJ_BYTELENGTH(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEDOBJ_SLOT_BYTELENGTH))
#define TYPEDOBJ_TYPE_DESCR(obj) \
    UnsafeGetReservedSlot(obj, JS_TYPEDOBJ_SLOT_TYPE_DESCR)
#define TYPEDOBJ_OWNER(obj) \
    UnsafeGetReservedSlot(obj, JS_TYPEDOBJ_SLOT_OWNER)
#define TYPEDOBJ_LENGTH(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEDOBJ_SLOT_LENGTH))

#define HAS_PROPERTY(obj, prop) \
    callFunction(std_Object_hasOwnProperty, obj, prop)

























function TypedObjectPointer(descr, typedObj, offset) {
  assert(IsObject(descr) && ObjectIsTypeDescr(descr), "Not descr");
  assert(IsObject(typedObj) && ObjectIsTypedObject(typedObj), "Not typedObj");
  assert(TO_INT32(offset) === offset, "offset not int");

  this.descr = descr;
  this.typedObj = typedObj;
  this.offset = offset;
}

MakeConstructible(TypedObjectPointer, {});

TypedObjectPointer.fromTypedObject = function(typed) {
  return new TypedObjectPointer(TYPEDOBJ_TYPE_DESCR(typed), typed, 0);
}

#ifdef DEBUG
TypedObjectPointer.prototype.toString = function() {
  return "Ptr(" + DESCR_STRING_REPR(this.descr) + " @ " + this.offset + ")";
};
#endif

TypedObjectPointer.prototype.copy = function() {
  return new TypedObjectPointer(this.descr, this.typedObj, this.offset);
};

TypedObjectPointer.prototype.reset = function(inPtr) {
  this.descr = inPtr.descr;
  this.typedObj = inPtr.typedObj;
  this.offset = inPtr.offset;
  return this;
};

TypedObjectPointer.prototype.bump = function(size) {
  assert(TO_INT32(this.offset) === this.offset, "current offset not int");
  assert(TO_INT32(size) === size, "size not int");
  this.offset += size;
}

TypedObjectPointer.prototype.kind = function() {
  return DESCR_KIND(this.descr);
}



TypedObjectPointer.prototype.length = function() {
  switch (this.kind()) {
  case JS_TYPEREPR_SIZED_ARRAY_KIND:
    return DESCR_SIZED_ARRAY_LENGTH(this.descr);

  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    return this.typedObj.length;
  }
  assert(false, "Invalid kind for length");
  return false;
}









TypedObjectPointer.prototype.moveTo = function(propName) {
  switch (this.kind()) {
  case JS_TYPEREPR_SCALAR_KIND:
  case JS_TYPEREPR_REFERENCE_KIND:
  case JS_TYPEREPR_X4_KIND:
    break;

  case JS_TYPEREPR_SIZED_ARRAY_KIND:
    return this.moveToArray(propName, DESCR_SIZED_ARRAY_LENGTH(this.descr));

  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    return this.moveToArray(propName, this.typedObj.length);

  case JS_TYPEREPR_STRUCT_KIND:
    if (HAS_PROPERTY(this.descr.fieldTypes, propName))
      return this.moveToField(propName);
    break;
  }

  ThrowError(JSMSG_TYPEDOBJECT_NO_SUCH_PROP, propName);
  return undefined;
};

TypedObjectPointer.prototype.moveToArray = function(propName, length) {
  
  
  
  
  
  var index = TO_INT32(propName);
  if (index === propName && index >= 0 && index < length)
    return this.moveToElem(index);

  ThrowError(JSMSG_TYPEDOBJECT_NO_SUCH_PROP, propName);
  return undefined;
}




TypedObjectPointer.prototype.moveToElem = function(index) {
  assert(this.kind() == JS_TYPEREPR_SIZED_ARRAY_KIND ||
         this.kind() == JS_TYPEREPR_UNSIZED_ARRAY_KIND,
         "moveToElem invoked on non-array");
  assert(TO_INT32(index) === index,
         "moveToElem invoked with non-integer index");
  assert(index >= 0 && index < this.length(),
         "moveToElem invoked with negative index: " + index);

  var elementDescr = this.descr.elementType;
  this.descr = elementDescr;
  var elementSize = DESCR_SIZE(elementDescr);

  
  
  this.offset += std_Math_imul(index, elementSize);

  return this;
};

TypedObjectPointer.prototype.moveToField = function(propName) {
  var fieldNames = DESCR_STRUCT_FIELD_NAMES(this.descr);
  var index = fieldNames.indexOf(propName);
  if (index != -1)
    return this.moveToFieldIndex(index);

  ThrowError(JSMSG_TYPEDOBJECT_NO_SUCH_PROP, propName);
  return undefined;
}




TypedObjectPointer.prototype.moveToFieldIndex = function(index) {
  assert(this.kind() == JS_TYPEREPR_STRUCT_KIND,
         "moveToFieldIndex invoked on non-struct");
  assert(index >= 0 && index < DESCR_STRUCT_FIELD_NAMES(this.descr).length,
         "moveToFieldIndex invoked with invalid field index " + index);

  var fieldDescr = DESCR_STRUCT_FIELD_TYPES(this.descr)[index];
  var fieldOffset = TO_INT32(DESCR_STRUCT_FIELD_OFFSETS(this.descr)[index]);

  assert(IsObject(fieldDescr) && ObjectIsTypeDescr(fieldDescr),
         "bad field descr");
  assert(TO_INT32(fieldOffset) === fieldOffset,
         "bad field offset");
  assert(fieldOffset >= 0 &&
         (fieldOffset + DESCR_SIZE(fieldDescr)) <= DESCR_SIZE(this.descr),
         "out of bounds field offset");

  this.descr = fieldDescr;
  this.offset += fieldOffset;

  return this;
}












TypedObjectPointer.prototype.get = function() {
  assert(TypedObjectIsAttached(this.typedObj), "get() called with unattached typedObj");

  switch (this.kind()) {
  case JS_TYPEREPR_SCALAR_KIND:
    return this.getScalar();

  case JS_TYPEREPR_REFERENCE_KIND:
    return this.getReference();

  case JS_TYPEREPR_X4_KIND:
    return this.getX4();

  case JS_TYPEREPR_SIZED_ARRAY_KIND:
  case JS_TYPEREPR_STRUCT_KIND:
    return this.getDerived();

  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    assert(false, "Unhandled repr kind: " + this.kind());
  }

  assert(false, "Unhandled kind: " + this.kind());
  return undefined;
}

TypedObjectPointer.prototype.getDerived = function() {
  assert(!TypeDescrIsSimpleType(this.descr),
         "getDerived() used with simple type");
  return NewDerivedTypedObject(this.descr, this.typedObj, this.offset);
}

TypedObjectPointer.prototype.getDerivedIf = function(cond) {
  return (cond ? this.getDerived() : undefined);
}

TypedObjectPointer.prototype.getOpaque = function() {
  assert(!TypeDescrIsSimpleType(this.descr),
         "getDerived() used with simple type");
  var typedObj = NewOpaqueTypedObject(this.descr);
  AttachTypedObject(typedObj, this.typedObj, this.offset);
  return typedObj;
}

TypedObjectPointer.prototype.getOpaqueIf = function(cond) {
  return (cond ? this.getOpaque() : undefined);
}

TypedObjectPointer.prototype.getScalar = function() {
  var type = DESCR_TYPE(this.descr);
  switch (type) {
  case JS_SCALARTYPEREPR_INT8:
    return Load_int8(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_UINT8:
  case JS_SCALARTYPEREPR_UINT8_CLAMPED:
    return Load_uint8(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_INT16:
    return Load_int16(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_UINT16:
    return Load_uint16(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_INT32:
    return Load_int32(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_UINT32:
    return Load_uint32(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_FLOAT32:
    return Load_float32(this.typedObj, this.offset);

  case JS_SCALARTYPEREPR_FLOAT64:
    return Load_float64(this.typedObj, this.offset);
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}

TypedObjectPointer.prototype.getReference = function() {
  var type = DESCR_TYPE(this.descr);
  switch (type) {
  case JS_REFERENCETYPEREPR_ANY:
    return Load_Any(this.typedObj, this.offset);

  case JS_REFERENCETYPEREPR_OBJECT:
    return Load_Object(this.typedObj, this.offset);

  case JS_REFERENCETYPEREPR_STRING:
    return Load_string(this.typedObj, this.offset);
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}

TypedObjectPointer.prototype.getX4 = function() {
  var type = DESCR_TYPE(this.descr);
  switch (type) {
  case JS_X4TYPEREPR_FLOAT32:
    var x = Load_float32(this.typedObj, this.offset + 0);
    var y = Load_float32(this.typedObj, this.offset + 4);
    var z = Load_float32(this.typedObj, this.offset + 8);
    var w = Load_float32(this.typedObj, this.offset + 12);
    return GetFloat32x4TypeDescr()(x, y, z, w);

  case JS_X4TYPEREPR_INT32:
    var x = Load_int32(this.typedObj, this.offset + 0);
    var y = Load_int32(this.typedObj, this.offset + 4);
    var z = Load_int32(this.typedObj, this.offset + 8);
    var w = Load_int32(this.typedObj, this.offset + 12);
    return GetInt32x4TypeDescr()(x, y, z, w);
  }

  assert(false, "Unhandled x4 type: " + type);
  return undefined;
}







function SetTypedObjectValue(descr, typedObj, offset, fromValue) {
  new TypedObjectPointer(descr, typedObj, offset).set(fromValue);
}




TypedObjectPointer.prototype.set = function(fromValue) {
  assert(TypedObjectIsAttached(this.typedObj), "set() called with unattached typedObj");

  
  
  
  if (IsObject(fromValue) && ObjectIsTypedObject(fromValue)) {
    if (!this.descr.variable && DescrsEquiv(this.descr, TYPEDOBJ_TYPE_DESCR(fromValue))) {
      if (!TypedObjectIsAttached(fromValue))
        ThrowError(JSMSG_TYPEDOBJECT_HANDLE_UNATTACHED);

      var size = DESCR_SIZE(this.descr);
      Memcpy(this.typedObj, this.offset, fromValue, 0, size);
      return;
    }
  }

  switch (this.kind()) {
  case JS_TYPEREPR_SCALAR_KIND:
    this.setScalar(fromValue);
    return;

  case JS_TYPEREPR_REFERENCE_KIND:
    this.setReference(fromValue);
    return;

  case JS_TYPEREPR_X4_KIND:
    this.setX4(fromValue);
    return;

  case JS_TYPEREPR_SIZED_ARRAY_KIND:
    if (this.setArray(fromValue, DESCR_SIZED_ARRAY_LENGTH(this.descr)))
      return;
    break;

  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    if (this.setArray(fromValue, this.typedObj.length))
      return;
    break;

  case JS_TYPEREPR_STRUCT_KIND:
    if (!IsObject(fromValue))
      break;

    
    var tempPtr = this.copy();
    var fieldNames = DESCR_STRUCT_FIELD_NAMES(this.descr);
    for (var i = 0; i < fieldNames.length; i++) {
      var fieldName = fieldNames[i];
      tempPtr.reset(this).moveToFieldIndex(i).set(fromValue[fieldName]);
    }
    return;
  }

  ThrowError(JSMSG_CANT_CONVERT_TO,
             typeof(fromValue),
             DESCR_STRING_REPR(this.descr));
}

TypedObjectPointer.prototype.setArray = function(fromValue, length) {
  if (!IsObject(fromValue))
    return false;

  
  if (fromValue.length !== length)
    return false;

  
  if (length > 0) {
    var tempPtr = this.copy().moveToElem(0);
    var size = DESCR_SIZE(tempPtr.descr);
    for (var i = 0; i < length; i++) {
      tempPtr.set(fromValue[i]);
      tempPtr.offset += size;
    }
  }

  return true;
}


TypedObjectPointer.prototype.setScalar = function(fromValue) {
  assert(this.kind() == JS_TYPEREPR_SCALAR_KIND,
         "setScalar called with non-scalar");

  var type = DESCR_TYPE(this.descr);
  switch (type) {
  case JS_SCALARTYPEREPR_INT8:
    return Store_int8(this.typedObj, this.offset,
                     TO_INT32(fromValue) & 0xFF);

  case JS_SCALARTYPEREPR_UINT8:
    return Store_uint8(this.typedObj, this.offset,
                      TO_UINT32(fromValue) & 0xFF);

  case JS_SCALARTYPEREPR_UINT8_CLAMPED:
    var v = ClampToUint8(+fromValue);
    return Store_int8(this.typedObj, this.offset, v);

  case JS_SCALARTYPEREPR_INT16:
    return Store_int16(this.typedObj, this.offset,
                      TO_INT32(fromValue) & 0xFFFF);

  case JS_SCALARTYPEREPR_UINT16:
    return Store_uint16(this.typedObj, this.offset,
                       TO_UINT32(fromValue) & 0xFFFF);

  case JS_SCALARTYPEREPR_INT32:
    return Store_int32(this.typedObj, this.offset,
                      TO_INT32(fromValue));

  case JS_SCALARTYPEREPR_UINT32:
    return Store_uint32(this.typedObj, this.offset,
                       TO_UINT32(fromValue));

  case JS_SCALARTYPEREPR_FLOAT32:
    return Store_float32(this.typedObj, this.offset, +fromValue);

  case JS_SCALARTYPEREPR_FLOAT64:
    return Store_float64(this.typedObj, this.offset, +fromValue);
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}

TypedObjectPointer.prototype.setReference = function(fromValue) {
  var type = DESCR_TYPE(this.descr);
  switch (type) {
  case JS_REFERENCETYPEREPR_ANY:
    return Store_Any(this.typedObj, this.offset, fromValue);

  case JS_REFERENCETYPEREPR_OBJECT:
    var value = (fromValue === null ? fromValue : ToObject(fromValue));
    return Store_Object(this.typedObj, this.offset, value);

  case JS_REFERENCETYPEREPR_STRING:
    return Store_string(this.typedObj, this.offset, ToString(fromValue));
  }

  assert(false, "Unhandled scalar type: " + type);
  return undefined;
}


TypedObjectPointer.prototype.setX4 = function(fromValue) {
  
  
  
  
  ThrowError(JSMSG_CANT_CONVERT_TO,
             typeof(fromValue),
             DESCR_STRING_REPR(this.descr));
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

  var ptr = new TypedObjectPointer(destDescr, destTypedObj, destOffset);
  ptr.set(fromValue);
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

  var ptr = new TypedObjectPointer(sourceDescr, sourceTypedObj, sourceOffset);

  return ptr.get();
}

function FillTypedArrayWithValue(destArray, fromValue) {
  assert(IsObject(handle) && ObjectIsTypedObject(destArray),
         "FillTypedArrayWithValue: not typed handle");

  var descr = TYPEDOBJ_TYPE_DESCR(destArray);
  var length = DESCR_SIZED_ARRAY_LENGTH(descr);
  if (length === 0)
    return;

  
  var ptr = TypedObjectPointer.fromTypedObject(destArray);
  ptr.moveToElem(0);
  ptr.set(fromValue);

  
  var elementSize = DESCR_SIZE(ptr.descr);
  var totalSize = length * elementSize;
  for (var offset = elementSize; offset < totalSize; offset += elementSize)
    Memcpy(destArray, offset, destArray, 0, elementSize);
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

  
  
  var oldArrayType = TYPEDOBJ_TYPE_DESCR(this);
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




function X4ProtoString(type) {
  switch (type) {
  case JS_X4TYPEREPR_INT32:
    return "int32x4";
  case JS_X4TYPEREPR_FLOAT32:
    return "float32x4";
  }

  assert(false, "Unhandled type constant");
  return undefined;
}

function X4ToSource() {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    ThrowError(JSMSG_INCOMPATIBLE_PROTO, "X4", "toSource", typeof this);

  var descr = TYPEDOBJ_TYPE_DESCR(this);

  if (DESCR_KIND(descr) != JS_TYPEREPR_X4_KIND)
    ThrowError(JSMSG_INCOMPATIBLE_PROTO, "X4", "toSource", typeof this);

  var type = DESCR_TYPE(descr);
  return X4ProtoString(type)+"("+this.x+", "+this.y+", "+this.z+", "+this.w+")";
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

    if (ObjectIsTransparentTypedObject(obj))
      return { buffer: TYPEDOBJ_OWNER(obj),
               byteLength: TYPEDOBJ_BYTELENGTH(obj),
               byteOffset: TYPEDOBJ_BYTEOFFSET(obj) };
  }

  ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  return null; 
}





function TypeOfTypedObject(obj) {
  if (IsObject(obj) && ObjectIsTypedObject(obj))
    return TYPEDOBJ_TYPE_DESCR(obj);

  
  
  
  
  
  
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
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
    else
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  case JS_TYPEREPR_UNSIZED_ARRAY_KIND:
    var len = a;
    if (typeof b === "function")
      return BuildTypedSeqImpl(this, len, 1, b);
    else if (typeof b === "number" && typeof c === "function")
      return BuildTypedSeqImpl(this, len, b, c);
    else if (typeof b === "number")
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
    else
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  default:
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
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
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  } else {
    var explicitDepth = (typeof b === "number");
    if (explicitDepth && IsCallable(c))
      return MapTypedSeqImpl(a, b, this, c);
    else if (IsCallable(b))
      return MapTypedSeqImpl(a, 1, this, b);
    else if (explicitDepth)
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
    else
      return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  }
}


function TypedArrayMap(a, b) {
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TYPEDOBJ_TYPE_DESCR(this);
  if (!TypeDescrIsArrayType(thisType))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  
  if (typeof a === "number" && typeof b === "function")
    return MapTypedSeqImpl(this, a, thisType, b);
  else if (typeof a === "function")
    return MapTypedSeqImpl(this, 1, thisType, a);
  return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
}


function TypedArrayMapPar(a, b) {
  

  
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    return callFunction(TypedArrayMap, this, a, b);
  var thisType = TYPEDOBJ_TYPE_DESCR(this);
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
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TYPEDOBJ_TYPE_DESCR(this);
  if (!TypeDescrIsArrayType(thisType))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (a !== undefined && typeof a !== "function")
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  var outputType = thisType.elementType;
  return ReduceTypedSeqImpl(this, outputType, a, b);
}


function TypedArrayScatter(a, b, c, d) {
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TYPEDOBJ_TYPE_DESCR(this);
  if (!TypeDescrIsArrayType(thisType))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (!IsObject(a) || !ObjectIsTypeDescr(a) || !TypeDescrIsSizedArrayType(a))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (d !== undefined && typeof d !== "function")
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  return ScatterTypedSeqImpl(this, a, b, c, d);
}


function TypedArrayFilter(func) {
  
  if (!IsObject(this) || !ObjectIsTypedObject(this))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);
  var thisType = TYPEDOBJ_TYPE_DESCR(this);
  if (!TypeDescrIsArrayType(thisType))
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

  if (typeof func !== "function")
    return ThrowError(JSMSG_TYPEDOBJECT_BAD_ARGS);

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
  var outPointer = new TypedObjectPointer(grainType, result, 0);
  for (i = 0; i < totalLength; i++) {
    
    var userOutPointer = outPointer.getOpaqueIf(grainTypeIsComplex);

    
    callFunction(std_Array_push, indices, userOutPointer);
    var r = callFunction(std_Function_apply, func, undefined, indices);
    callFunction(std_Array_pop, indices);
    if (r !== undefined)
      outPointer.set(r); 

    
    IncrementIterationSpace(indices, iterationSpace);
    outPointer.bump(size);
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
  var outPointer = new TypedObjectPointer(outGrainType, result, 0);

  
  

  for (var i = 0; i < outLength; i++) {
    

    if (i in inArray) { 
      
      var element = inArray[i];

      
      var out = outPointer.getOpaqueIf(outGrainTypeIsComplex);

      
      var r = func(element, i, inArray, out);

      if (r !== undefined)
        outPointer.set(r); 
    }

    
    outPointer.bump(outUnitSize);
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

  
  

  var isDepth1Simple = depth == 1 && !(inGrainTypeIsComplex || outGrainTypeIsComplex);

  var inPointer = isDepth1Simple ? null : new TypedObjectPointer(inGrainType, inArray, 0);
  var outPointer = isDepth1Simple ? null : new TypedObjectPointer(outGrainType, result, 0);

  var inUnitSize = isDepth1Simple ? 0 : DESCR_SIZE(inGrainType);
  var outUnitSize = isDepth1Simple ? 0 : DESCR_SIZE(outGrainType);

  

  function DoMapTypedSeqDepth1() {
    for (var i = 0; i < totalLength; i++) {
      

      
      var element = inPointer.get();
      var out = outPointer.getOpaqueIf(outGrainTypeIsComplex);

      
      var r = func(element, i, inArray, out);
      if (r !== undefined)
        outPointer.set(r); 

      
      inPointer.bump(inUnitSize);
      outPointer.bump(outUnitSize);
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
      
      var element = inPointer.get();
      var out = outPointer.getOpaqueIf(outGrainTypeIsComplex);

      
      var args = [element];
      callFunction(std_Function_apply, std_Array_push, args, indices);
      callFunction(std_Array_push, args, inArray, out);
      var r = callFunction(std_Function_apply, func, void 0, args);
      if (r !== undefined)
        outPointer.set(r); 

      
      inPointer.bump(inUnitSize);
      outPointer.bump(outUnitSize);
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
    
    
    
    

    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    typedObj = NewDerivedTypedObject(TYPEDOBJ_TYPE_DESCR(typedObj),
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
    const inPointer = new TypedObjectPointer(inGrainType, inArray, 0);
    const inTypedObject = inPointer.getDerivedIf(inGrainTypeIsComplex);
    const outPointer = new TypedObjectPointer(outGrainType, outArray, 0);
    const outTypedObject = outPointer.getOpaqueIf(outGrainTypeIsComplex);
    ARRAY_PUSH(pointers, ({ inTypedObject: inTypedObject,
                            outTypedObject: outTypedObject }));
  }

  
  
  
  const inBaseOffset = TYPEDOBJ_BYTEOFFSET(inArray);

  ForkJoin(mapThread, 0, slicesInfo.count, ForkJoinMode(mode));
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

        
        
        
        if (outGrainTypeIsTransparent)
          ClearThreadLocalArenas();
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
  var inPointer = new TypedObjectPointer(elementType, array, 0);
  for (var i = 0; i < array.length; i++) {
    if (func(inPointer.get(), i, array)) {
      SET_BIT(flags, i);
      count++;
    }
    inPointer.bump(size);
  }

  var resultType = (arrayType.variable ? arrayType : arrayType.unsized);
  var result = new resultType(count);
  for (var i = 0, j = 0; i < array.length; i++) {
    if (GET_BIT(flags, i))
      result[j++] = array[i];
  }
  return result;
}
