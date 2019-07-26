




load(libdir + "asserts.js")

const constructors = [
  Int8Array,
  Uint8Array,
  Uint8ClampedArray,
  Int16Array,
  Uint16Array,
  Int32Array,
  Uint32Array,
  Float32Array,
  Float64Array
];

for (constructor of constructors) {
  print("testing non-empty " + constructor.name);
  let a = new constructor(10);
  assertEq(Object.isExtensible(a), false);
  assertEq(Object.isSealed(a), true);
  assertEq(Object.isFrozen(a), false);

  
  Object.seal(a);

  
  assertThrowsInstanceOf(() => Object.freeze(a), InternalError);
}

print();

for (constructor of constructors) {
  print("testing zero-length " + constructor.name);
  let a = new constructor(0);
  assertEq(Object.isExtensible(a), false);
  assertEq(Object.isSealed(a), true);
  assertEq(Object.isFrozen(a), true);

  
  Object.seal(a);
  Object.freeze(a);
}




let a = new Uint8Array(1 << 24);
Object.isSealed(a);
Object.isFrozen(a);
