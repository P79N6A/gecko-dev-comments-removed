
var BUGNUMBER = 1106428;
var int32x4 = SIMD.int32x4;

var summary = 'int32x4 bool';






function test() {
  print(BUGNUMBER + ": " + summary);

  var a = int32x4.bool(true, false, true, false);
  assertEq(a.x, -1);
  assertEq(a.y, 0);
  assertEq(a.z, -1);
  assertEq(a.w, 0);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();
