#include "TypedObjectConstants.h"






#define TYPE_TYPE_REPR(obj) \
    UnsafeGetReservedSlot(obj, JS_TYPEOBJ_SLOT_TYPE_REPR)



#define TYPED_TYPE_OBJ(obj) \
    UnsafeGetReservedSlot(obj, JS_TYPEDOBJ_SLOT_TYPE_OBJ)



#define REPR_KIND(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_KIND))
#define REPR_SIZE(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_SIZE))
#define REPR_ALIGNMENT(obj) \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_ALIGNMENT))
#define REPR_LENGTH(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_LENGTH))
#define REPR_TYPE(obj)   \
    TO_INT32(UnsafeGetReservedSlot(obj, JS_TYPEREPR_SLOT_TYPE))

#define HAS_PROPERTY(obj, prop) \
    callFunction(std_Object_hasOwnProperty, obj, prop)

function TYPED_TYPE_REPR(obj) {
  
  return TYPE_TYPE_REPR(TYPED_TYPE_OBJ(obj));
}





















function TypedObjectPointer(typeRepr, typeObj, owner, offset) {
  this.typeRepr = typeRepr;
  this.typeObj = typeObj;
  this.owner = owner;
  this.offset = offset;
}

MakeConstructible(TypedObjectPointer, {});

#ifdef DEBUG
TypedObjectPointer.prototype.toString = function() {
  return "Ptr(" + this.typeObj.toSource() + " @ " + this.offset + ")";
};
#endif

TypedObjectPointer.prototype.copy = function() {
  return new TypedObjectPointer(this.typeRepr, this.typeObj,
                                this.owner, this.offset);
};

TypedObjectPointer.prototype.reset = function(inPtr) {
  this.typeRepr = inPtr.typeRepr;
  this.typeObj = inPtr.typeObj;
  this.owner = inPtr.owner;
  this.offset = inPtr.offset;
  return this;
};

TypedObjectPointer.prototype.kind = function() {
  return REPR_KIND(this.typeRepr);
}









TypedObjectPointer.prototype.moveTo = function(propName) {
  switch (this.kind()) {
  case JS_TYPEREPR_SCALAR_KIND:
    break;

  case JS_TYPEREPR_ARRAY_KIND:
    
    
    
    
    var index = TO_INT32(propName);
    if (index === propName && index < REPR_LENGTH(this.typeRepr))
      return this.moveToElem(index);
    break;

  case JS_TYPEREPR_STRUCT_KIND:
    if (HAS_PROPERTY(this.typeObj.fieldTypes, propName))
      return this.moveToField(propName);
    break;
  }

  ThrowError(JSMSG_TYPEDOBJECT_NO_SUCH_PROP, propName);
};




TypedObjectPointer.prototype.moveToElem = function(index) {
  assert(this.kind() == JS_TYPEREPR_ARRAY_KIND,
         "moveToElem invoked on non-array");
  assert(index < REPR_LENGTH(this.typeRepr),
         "moveToElem invoked with out-of-bounds index");

  var elementTypeObj = this.typeObj.elementType;
  var elementTypeRepr = TYPE_TYPE_REPR(elementTypeObj);
  this.typeRepr = elementTypeRepr;
  this.typeObj = elementTypeObj;
  var elementSize = REPR_SIZE(elementTypeRepr);

  
  
  this.offset += std_Math_imul(index, elementSize);

  return this;
};




TypedObjectPointer.prototype.moveToField = function(propName) {
  assert(this.kind() == JS_TYPEREPR_STRUCT_KIND,
         "moveToField invoked on non-struct");
  assert(HAS_PROPERTY(this.typeObj.fieldTypes, propName),
         "moveToField invoked with undefined field");

  var fieldTypeObj = this.typeObj.fieldTypes[propName];
  var fieldOffset = TO_INT32(this.typeObj.fieldOffsets[propName]);
  this.typeObj = fieldTypeObj;
  this.typeRepr = TYPE_TYPE_REPR(fieldTypeObj);

  
  
  this.offset += fieldOffset;

  return this;
}









TypedObjectPointer.prototype.set = function(fromValue) {
  var typeRepr = this.typeRepr;

  
  
  
  if (IsObject(fromValue) && HaveSameClass(fromValue, this.owner)) {
    if (TYPED_TYPE_REPR(fromValue) === typeRepr) {
      var size = REPR_SIZE(typeRepr);
      Memcpy(this.owner, this.offset, fromValue, 0, size);
      return;
    }
  }

  switch (REPR_KIND(typeRepr)) {
  case JS_TYPEREPR_SCALAR_KIND:
    this.setScalar(fromValue);
    return;

  case JS_TYPEREPR_ARRAY_KIND:
    if (!IsObject(fromValue))
      break;

    
    var length = REPR_LENGTH(typeRepr);
    if (fromValue.length !== length)
      break;

    
    var tempPtr = this.copy().moveToElem(0);
    var size = REPR_SIZE(tempPtr.typeRepr);
    for (var i = 0; i < length; i++) {
      tempPtr.set(fromValue[i]);
      tempPtr.offset += size;
    }
    return;

  case JS_TYPEREPR_STRUCT_KIND:
    if (!IsObject(fromValue))
      break;

    
    var tempPtr = this.copy();
    var fieldNames = this.typeObj.fieldNames;
    for (var i = 0; i < fieldNames.length; i++) {
      var fieldName = fieldNames[i];
      tempPtr.reset(this).moveToField(fieldName).set(fromValue[fieldName]);
    }
    return;
  }

  ThrowError(JSMSG_CANT_CONVERT_TO,
             typeof(fromValue),
             this.typeRepr.toSource())
}


TypedObjectPointer.prototype.setScalar = function(fromValue) {
  assert(REPR_KIND(this.typeRepr) == JS_TYPEREPR_SCALAR_KIND,
         "setScalar called with non-scalar");

  var type = REPR_TYPE(this.typeRepr);
  switch (type) {
  case JS_SCALARTYPEREPR_INT8:
    return Store_int8(this.owner, this.offset,
                     TO_INT32(fromValue) & 0xFF);

  case JS_SCALARTYPEREPR_UINT8:
    return Store_uint8(this.owner, this.offset,
                      TO_UINT32(fromValue) & 0xFF);

  case JS_SCALARTYPEREPR_UINT8_CLAMPED:
    var v = ClampToUint8(+fromValue);
    return Store_int8(this.owner, this.offset, v);

  case JS_SCALARTYPEREPR_INT16:
    return Store_int16(this.owner, this.offset,
                      TO_INT32(fromValue) & 0xFFFF);

  case JS_SCALARTYPEREPR_UINT16:
    return Store_uint16(this.owner, this.offset,
                       TO_UINT32(fromValue) & 0xFFFF);

  case JS_SCALARTYPEREPR_INT32:
    return Store_int32(this.owner, this.offset,
                      TO_INT32(fromValue));

  case JS_SCALARTYPEREPR_UINT32:
    return Store_uint32(this.owner, this.offset,
                       TO_UINT32(fromValue));

  case JS_SCALARTYPEREPR_FLOAT32:
    return Store_float32(this.owner, this.offset, +fromValue);

  case JS_SCALARTYPEREPR_FLOAT64:
    return Store_float64(this.owner, this.offset, +fromValue);
  }

  assert(false, "Unhandled scalar type: " + type);
}







function ConvertAndCopyTo(destTypeRepr,
                          destTypeObj,
                          destTypedObj,
                          destOffset,
                          fromValue)
{
  var ptr = new TypedObjectPointer(destTypeRepr, destTypeObj,
                                   destTypedObj, destOffset);
  ptr.set(fromValue);
}

function FillTypedArrayWithValue(destArray, fromValue) {
  var typeRepr = TYPED_TYPE_REPR(destArray);
  var length = REPR_LENGTH(typeRepr);
  if (length === 0)
    return;

  
  var ptr = new TypedObjectPointer(typeRepr,
                                   TYPED_TYPE_OBJ(destArray),
                                   destArray,
                                   0);
  ptr.moveToElem(0);
  ptr.set(fromValue);

  
  var elementSize = REPR_SIZE(ptr.typeRepr);
  var totalSize = length * elementSize;
  for (var offset = elementSize; offset < totalSize; offset += elementSize)
    Memcpy(destArray, offset, destArray, 0, elementSize);
}


