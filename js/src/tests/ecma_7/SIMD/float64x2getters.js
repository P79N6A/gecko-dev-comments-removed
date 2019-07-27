
var BUGNUMBER = 1031203;
var float64x2 = SIMD.float64x2;
var int32x4 = SIMD.int32x4;

var summary = 'float64x2 getters';






function test() {
  print(BUGNUMBER + ": " + summary);

  
  var f = float64x2(11, 22);
  assertEq(f.x, 11);
  assertEq(f.y, 22);

  
  var g = f.__lookupGetter__("x");
  assertEq(g.call(f), 11);

  
  assertThrowsInstanceOf(function() {
    g.call({})
  }, TypeError, "Getter applicable to random objects");
  assertThrowsInstanceOf(function() {
    g.call(0xDEADBEEF)
  }, TypeError, "Getter applicable to integers");
  assertThrowsInstanceOf(function() {
    var T = new TypedObject.StructType({x: TypedObject.float64,
                                        y: TypedObject.float64});
    var v = new T({x: 11, y: 22});
    g.call(v)
  }, TypeError, "Getter applicable to structs");
  assertThrowsInstanceOf(function() {
    var t = new int32x4(1, 2, 3, 4);
    g.call(t)
  }, TypeError, "Getter applicable to int32x4");

  if (typeof reportCompare === "function")
    reportCompare(true, true);
}

test();
