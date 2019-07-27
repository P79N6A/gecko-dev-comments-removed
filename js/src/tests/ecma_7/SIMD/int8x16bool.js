
var Int8x16 = SIMD.Int8x16;






function test() {
  var a = Int8x16.bool(true, false, true, false, true, true, false, false, true, true, true, false, false, false, true, true);
  assertEqInt8x16(a, [-1, 0, -1, 0, -1, -1, 0, 0, -1, -1, -1, 0, 0, 0, -1, -1]);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();
