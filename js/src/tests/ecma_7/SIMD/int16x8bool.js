
var Int16x8 = SIMD.Int16x8;






function test() {
  var a = Int16x8.bool(true, false, true, false, true, true, false, false);
  assertEqInt16x8(a, [-1, 0, -1, 0, -1, -1, 0, 0]);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();
