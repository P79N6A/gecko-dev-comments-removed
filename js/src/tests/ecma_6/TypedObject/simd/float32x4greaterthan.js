
var BUGNUMBER = 946042;
var float32x4 = SIMD.float32x4;
var int32x4 = SIMD.int32x4;

var summary = 'float32x4 greaterThan';

function test() {
  print(BUGNUMBER + ": " + summary);

  
  

  var a = float32x4(1, 20, 3, 40);
  var b = float32x4(10, 2, 30, 4);
  var c = SIMD.float32x4.greaterThan(b, a);
  assertEq(c.x, -1);
  assertEq(c.y, 0);
  assertEq(c.z, -1);
  assertEq(c.w, 0);

  var d = float32x4(10.8399, 20.37, 3.07, 4.6802);
  var e = float32x4(10.8401, 20.367, 3.1, 4.6801);
  var f = float32x4.greaterThan(e, d);
  assertEq(f.x, -1);
  assertEq(f.y, 0);
  assertEq(f.z, -1);
  assertEq(f.w, 0);

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();

