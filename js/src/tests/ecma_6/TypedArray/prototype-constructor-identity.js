




var gTestfile = 'prototype-constructor-identity.js';

var BUGNUMBER = 896116;
var summary =
  "Typed array prototypes/constructors should be largely empty, inheriting "
  "most functionality from %TypedArray% and %TypedArray%.prototype";

print(BUGNUMBER + ": " + summary);





var TypedArray = Object.getPrototypeOf(Int8Array);

assertEq(TypedArray !== Function.prototype, true,
         "%TypedArray% should be in constructors' [[Prototype]] chains");
assertEq(Object.getPrototypeOf(TypedArray), Function.prototype,
         "%TypedArray%.prototype should inherit from Function.prototype");

assertEq(TypedArray.prototype.constructor, TypedArray,
         "bad %TypedArray%.prototype.constructor");


var typedArrayProps = Object.getOwnPropertyNames(TypedArray.prototype);
assertEq(typedArrayProps.indexOf("copyWithin") >= 0, true);
assertEq(typedArrayProps.indexOf("subarray") >= 0, true);
assertEq(typedArrayProps.indexOf("set") >= 0, true);

var ctors = [Int8Array, Uint8Array, Uint8ClampedArray,
             Int16Array, Uint16Array,
             Int32Array, Uint32Array,
             Float32Array, Float64Array];
ctors.forEach(function(ctor) {
  assertEq(Object.getPrototypeOf(ctor), TypedArray);

  var proto = ctor.prototype;

  
  var props = Object.getOwnPropertyNames(proto).sort();
  assertEq(props[0], "BYTES_PER_ELEMENT");
  assertEq(props[1], "constructor");
  assertEq(props.length, 2);

  
  assertEq(Object.getPrototypeOf(proto), TypedArray.prototype,
           "prototype should inherit from %TypedArray%.prototype");
});



reportCompare(true, true);

print("Tests complete");
