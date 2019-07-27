
var int16x8 = SIMD.int16x8;






function test() {
  var a = int16x8.bool(true, false, true, false, true, true, false, false);
  assertEq(a.s0, -1);
  assertEq(a.s1, 0);
  assertEq(a.s2, -1);
  assertEq(a.s3, 0);
  assertEq(a.s4, -1);
  assertEq(a.s5, -1);
  assertEq(a.s6, 0);
  assertEq(a.s7, 0);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();
