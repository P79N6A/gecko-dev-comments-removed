

if (getBuildConfiguration().parallelJS) {
  load(libdir + "asserts.js");

  var desc = Object.getOwnPropertyDescriptor(Array, "build");
  assertEq(desc.configurable, true);
  assertEq(desc.enumerable, false);
  assertEq(desc.writable, true);
  assertEq(Array.build.length, 2);
  assertThrowsInstanceOf(() => new Array.build(), TypeError);  

  
  for (let v of [undefined, null, false, "cow"])
    assertThrowsInstanceOf(() => Array.build(1, v), TypeError);

  
  assertThrowsInstanceOf(() => Array.build(-1, function() {}), RangeError);

  
  for (let v of [undefined, null, false, "cow"])
    assertEq(Array.isArray(Array.build.call(v, 1, function() {})), true);
}
