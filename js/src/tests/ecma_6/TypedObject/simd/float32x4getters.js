
var BUGNUMBER = 938728;
var summary = 'float32x4 getters';






var float32x4 = TypedObject.float32x4;

function test() {
  print(BUGNUMBER + ": " + summary);

  
  var f = float32x4(11, 22, 33, 44);
  assertEq(f.x, 11);
  assertEq(f.y, 22);
  assertEq(f.z, 33);
  assertEq(f.w, 44);

  
  var g = f.__lookupGetter__("x");
  assertEq(g.call(f), 11);

  
  assertThrowsInstanceOf(function() {
    g.call({})
  }, TypeError, "Getter applicable to random objects");
  assertThrowsInstanceOf(function() {
    g.call(0xDEADBEEF)
  }, TypeError, "Getter applicable to integers");
  assertThrowsInstanceOf(function() {
    var T = new TypedObject.StructType({x: TypedObject.float32,
                                        y: TypedObject.float32,
                                        z: TypedObject.float32,
                                        w: TypedObject.float32});
    var v = new T({x: 11, y: 22, z: 33, w: 44});
    g.call(v)
  }, TypeError, "Getter applicable to structs");
  assertThrowsInstanceOf(function() {
    var t = new TypedObject.int32x4(1, 2, 3, 4);
    g.call(t)
  }, TypeError, "Getter applicable to int32x4");

  if (typeof reportCompare === "function")
    reportCompare(true, true);
  print("Tests complete");
}

test();
